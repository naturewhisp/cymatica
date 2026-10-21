#include "audio_probe.h"
#include "audio_engine.h"
#include "audio_scheduler.h"
#include "music_clock.h"
#include "music_planner.h"
#include "unsigned_math.h"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#if defined(_MSC_VER)
#include <intrin.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace cymatica::audio {

AudioProbe::AudioProbe(std::uint32_t capacityFrames, std::uint32_t channels, std::uint32_t sampleRate)
    : capacityFrames_(capacityFrames > 0 ? capacityFrames : kDefaultCapacityFrames)
    , channels_(channels > 0 ? channels : 2)
    , sampleRate_(sampleRate > 0 ? sampleRate : 48000)
    , capacitySamples_(capacityFrames_ * channels_)
    , ringBuffer_(capacitySamples_, 0.0f) {}

void AudioProbe::writeFrames(const float* pInterleavedInput, std::uint32_t frameCount) noexcept {
    if (pInterleavedInput == nullptr || frameCount == 0) return;

    const std::uint64_t writeIdx = writeHead_.load(std::memory_order_relaxed);
    const std::uint64_t readIdx = readHead_.load(std::memory_order_acquire);
    const std::uint64_t usedFrames = (writeIdx >= readIdx) ? (writeIdx - readIdx) : 0;
    const std::uint64_t freeFrames = (capacityFrames_ > usedFrames) ? (capacityFrames_ - usedFrames) : 0;

    // Bounded, non-blocking overflow handling: drop on insufficient space (Spec §20.2, §31.5)
    if (freeFrames < frameCount) {
        droppedFrames_.fetch_add(frameCount, std::memory_order_relaxed);
        return;
    }

    const std::uint32_t startFrame = static_cast<std::uint32_t>(writeIdx % capacityFrames_);
    const std::uint32_t framesToEnd = capacityFrames_ - startFrame;
    const std::uint32_t firstChunkFrames = std::min(frameCount, framesToEnd);
    const std::uint32_t secondChunkFrames = frameCount - firstChunkFrames;

    // Copy chunk 1
    std::memcpy(&ringBuffer_[static_cast<std::size_t>(startFrame) * channels_],
                pInterleavedInput,
                static_cast<std::size_t>(firstChunkFrames) * channels_ * sizeof(float));

    // Copy wrapped chunk 2 if any
    if (secondChunkFrames > 0) {
        std::memcpy(&ringBuffer_[0],
                    pInterleavedInput + (static_cast<std::size_t>(firstChunkFrames) * channels_),
                    static_cast<std::size_t>(secondChunkFrames) * channels_ * sizeof(float));
    }

    writeHead_.store(writeIdx + frameCount, std::memory_order_release);
}

std::uint32_t AudioProbe::readFrames(float* pInterleavedOutput, std::uint32_t maxFrames) noexcept {
    if (pInterleavedOutput == nullptr || maxFrames == 0) return 0;

    const std::uint64_t writeIdx = writeHead_.load(std::memory_order_acquire);
    const std::uint64_t readIdx = readHead_.load(std::memory_order_relaxed);
    const std::uint64_t available = (writeIdx >= readIdx) ? (writeIdx - readIdx) : 0;
    const std::uint32_t toRead = static_cast<std::uint32_t>(std::min(available, static_cast<std::uint64_t>(maxFrames)));

    if (toRead == 0) return 0;

    const std::uint32_t startFrame = static_cast<std::uint32_t>(readIdx % capacityFrames_);
    const std::uint32_t framesToEnd = capacityFrames_ - startFrame;
    const std::uint32_t firstChunkFrames = std::min(toRead, framesToEnd);
    const std::uint32_t secondChunkFrames = toRead - firstChunkFrames;

    std::memcpy(pInterleavedOutput,
                &ringBuffer_[static_cast<std::size_t>(startFrame) * channels_],
                static_cast<std::size_t>(firstChunkFrames) * channels_ * sizeof(float));

    if (secondChunkFrames > 0) {
        std::memcpy(pInterleavedOutput + (static_cast<std::size_t>(firstChunkFrames) * channels_),
                    &ringBuffer_[0],
                    static_cast<std::size_t>(secondChunkFrames) * channels_ * sizeof(float));
    }

    readHead_.store(readIdx + toRead, std::memory_order_release);
    return toRead;
}

std::uint64_t AudioProbe::droppedFrames() const noexcept {
    return droppedFrames_.load(std::memory_order_relaxed);
}

std::uint32_t AudioProbe::availableFrames() const noexcept {
    const std::uint64_t writeIdx = writeHead_.load(std::memory_order_acquire);
    const std::uint64_t readIdx = readHead_.load(std::memory_order_relaxed);
    return static_cast<std::uint32_t>((writeIdx >= readIdx) ? (writeIdx - readIdx) : 0);
}

std::uint32_t AudioProbe::capacityFrames() const noexcept {
    return capacityFrames_;
}

std::uint32_t AudioProbe::channels() const noexcept {
    return channels_;
}

std::uint32_t AudioProbe::sampleRate() const noexcept {
    return sampleRate_;
}

void AudioProbe::reset() noexcept {
    readHead_.store(0, std::memory_order_relaxed);
    writeHead_.store(0, std::memory_order_relaxed);
    droppedFrames_.store(0, std::memory_order_relaxed);
}

namespace analysis {

float computeRmsDbfs(const float* pSamples, std::size_t sampleCount) noexcept {
    if (pSamples == nullptr || sampleCount == 0) return -120.0f;

    double sumSq = 0.0;
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const double s = pSamples[i];
        if (std::isfinite(s)) {
            sumSq += s * s;
        }
    }
    const double rms = std::sqrt(sumSq / static_cast<double>(sampleCount));
    if (!std::isfinite(rms) || rms <= 1e-6) return -120.0f;
    return static_cast<float>(20.0 * std::log10(rms));
}

float estimatePitchHz(const float* pSamples,
                      std::size_t sampleCount,
                      std::uint32_t sampleRate,
                      float minFreqHz,
                      float maxFreqHz) noexcept {
    if (pSamples == nullptr || sampleCount < 64 || sampleRate == 0) return 0.0f;

    const int kMin = std::max(2, static_cast<int>(std::floor(static_cast<float>(sampleRate) / maxFreqHz)));
    const int kMax = std::min(static_cast<int>(sampleCount / 2),
                              static_cast<int>(std::ceil(static_cast<float>(sampleRate) / minFreqHz)));

    if (kMin >= kMax) return 0.0f;

    // Autocorrelation: R(k) = sum(x[n] * x[n+k])
    const int numLags = kMax - kMin + 1;
    std::vector<double> r(numLags, 0.0);
    double maxVal = -1e30;

    for (int k = kMin; k <= kMax; ++k) {
        double sum = 0.0;
        const std::size_t nEnd = sampleCount - static_cast<std::size_t>(k);
        for (std::size_t n = 0; n < nEnd; ++n) {
            sum += static_cast<double>(pSamples[n]) * static_cast<double>(pSamples[n + k]);
        }
        sum /= static_cast<double>(nEnd);
        r[k - kMin] = sum;
        if (sum > maxVal) {
            maxVal = sum;
        }
    }

    if (maxVal <= 0.0) return 0.0f;

    // Standard McLeod/YIN rule: pick first local peak >= 80% of max to avoid octave subharmonic error
    int bestLag = -1;
    const double threshold = 0.80 * maxVal;
    for (int i = 1; i < numLags - 1; ++i) {
        if (r[i] >= threshold && r[i] >= r[i - 1] && r[i] >= r[i + 1]) {
            bestLag = kMin + i;
            break;
        }
    }
    if (bestLag < 0) {
        for (int i = 0; i < numLags; ++i) {
            if (r[i] == maxVal) {
                bestLag = kMin + i;
                break;
            }
        }
    }

    if (bestLag < kMin || bestLag > kMax) return 0.0f;

    // Parabolic interpolation around peak lag
    double delta = 0.0;
    if (bestLag > kMin && bestLag < kMax) {
        auto calcR = [&](int k) -> double {
            double r = 0.0;
            const std::size_t nEnd = sampleCount - static_cast<std::size_t>(k);
            for (std::size_t n = 0; n < nEnd; ++n) {
                r += static_cast<double>(pSamples[n]) * static_cast<double>(pSamples[n + k]);
            }
            return r / static_cast<double>(nEnd);
        };
        const double y0 = calcR(bestLag - 1);
        const double y1 = r[bestLag - kMin];
        const double y2 = calcR(bestLag + 1);
        const double denom = 2.0 * (2.0 * y1 - y0 - y2);
        if (std::abs(denom) > 1e-12) {
            delta = (y2 - y0) / denom;
            delta = std::clamp(delta, -0.5, 0.5);
        }
    }

    const double refinedLag = static_cast<double>(bestLag) + delta;
    if (refinedLag <= 0.0) return 0.0f;
    return static_cast<float>(static_cast<double>(sampleRate) / refinedLag);
}

#pragma pack(push, 1)
struct WavHeader {
    char riffId[4]{'R', 'I', 'F', 'F'};
    std::uint32_t riffSize{0};
    char waveId[4]{'W', 'A', 'V', 'E'};
    char fmtId[4]{'f', 'm', 't', ' '};
    std::uint32_t fmtSize{16};
    std::uint16_t audioFormat{1}; // 1 = PCM
    std::uint16_t numChannels{2};
    std::uint32_t sampleRate{48000};
    std::uint32_t byteRate{192000}; // sampleRate * numChannels * 2
    std::uint16_t blockAlign{4};   // numChannels * 2
    std::uint16_t bitsPerSample{16};
    char dataId[4]{'d', 'a', 't', 'a'};
    std::uint32_t dataSize{0};
};
#pragma pack(pop)

bool writeWavStereo16(const std::string& filePath,
                      const float* pInterleavedSamples,
                      std::size_t frameCount,
                      std::uint32_t sampleRate) {
    if (pInterleavedSamples == nullptr || frameCount == 0 || sampleRate == 0) return false;

    std::filesystem::path p(filePath);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }

    std::ofstream out(filePath, std::ios::binary);
    if (!out.is_open()) return false;

    const std::uint32_t numChannels = 2;
    const std::uint32_t bytesPerSample = 2;
    const std::uint32_t dataBytes = static_cast<std::uint32_t>(frameCount * numChannels * bytesPerSample);

    WavHeader hdr{};
    hdr.riffSize = 36 + dataBytes;
    hdr.numChannels = static_cast<std::uint16_t>(numChannels);
    hdr.sampleRate = sampleRate;
    hdr.byteRate = sampleRate * numChannels * bytesPerSample;
    hdr.blockAlign = static_cast<std::uint16_t>(numChannels * bytesPerSample);
    hdr.bitsPerSample = 16;
    hdr.dataSize = dataBytes;

    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));

    const std::size_t totalSamples = frameCount * numChannels;
    std::vector<std::int16_t> pcm(totalSamples);
    for (std::size_t i = 0; i < totalSamples; ++i) {
        const float raw = pInterleavedSamples[i];
        const float s = std::isfinite(raw) ? std::clamp(raw, -1.0f, 1.0f) : 0.0f;
        pcm[i] = static_cast<std::int16_t>(std::round(s * 32767.0f));
    }
    out.write(reinterpret_cast<const char*>(pcm.data()), pcm.size() * sizeof(std::int16_t));
    return out.good();
}

// ITU-R BS.1770-4/-5 Annex 2 48-tap (4 phases x 12 taps) polyphase interpolation filter coefficients (4 phases, 12 taps per phase)
static constexpr float kBs1770Polyphase[4][12] = {
    { +0.0017089843750f, +0.0109863281250f, -0.0196533203125f, +0.0332031250000f, -0.0594482421875f, +0.1373291015625f, +0.9721679687500f, -0.1022949218750f, +0.0476074218750f, -0.0266113281250f, +0.0148925781250f, -0.0083007812500f },
    { -0.0291748046875f, +0.0292968750000f, -0.0517578125000f, +0.0891113281250f, -0.1665039062500f, +0.4650878906250f, +0.7797851562500f, -0.2003173828125f, +0.1015625000000f, -0.0582275390625f, +0.0330810546875f, -0.0189208984375f },
    { -0.0189208984375f, +0.0330810546875f, -0.0582275390625f, +0.1015625000000f, -0.2003173828125f, +0.7797851562500f, +0.4650878906250f, -0.1665039062500f, +0.0891113281250f, -0.0517578125000f, +0.0292968750000f, -0.0291748046875f },
    { -0.0083007812500f, +0.0148925781250f, -0.0266113281250f, +0.0476074218750f, -0.1022949218750f, +0.9721679687500f, +0.1373291015625f, -0.0594482421875f, +0.0332031250000f, -0.0196533203125f, +0.0109863281250f, +0.0017089843750f }
};

float estimateTruePeakLinear(const float* pInterleavedSamples,
                             std::size_t frameCount,
                             std::uint32_t channels) noexcept {
    if (pInterleavedSamples == nullptr || frameCount == 0 || channels == 0) return 0.0f;

    float maxPeak = 0.0f;

    for (std::uint32_t ch = 0; ch < channels; ++ch) {
        // Phases interpolate between x[i] and x[i + 1]; i = -1 covers the onset from the preceding silence.
        for (int64_t i = -1; i < static_cast<int64_t>(frameCount); ++i) {
            if (i >= 0) {
                const float sRaw = pInterleavedSamples[static_cast<std::size_t>(i) * channels + ch];
                if (std::isfinite(sRaw)) {
                    maxPeak = (std::max)(maxPeak, std::abs(sRaw));
                }
            }

            // Gather 12 surrounding samples: i - 5 to i + 6. Outside the capture the signal is digital
            // silence (the device receives zeros before/after the stream), so boundaries are zero-padded.
            float window[12];
            for (int k = 0; k < 12; ++k) {
                const int64_t idx = i - 5 + k;
                float val = 0.0f;
                if (idx >= 0 && idx < static_cast<int64_t>(frameCount)) {
                    val = pInterleavedSamples[static_cast<std::size_t>(idx) * channels + ch];
                }
                window[k] = std::isfinite(val) ? val : 0.0f;
            }

            // Convolve with 4 polyphase filters
            for (int p = 0; p < 4; ++p) {
                float interp = 0.0f;
                for (int k = 0; k < 12; ++k) {
                    interp += kBs1770Polyphase[p][k] * window[k];
                }
                if (std::isfinite(interp)) {
                    maxPeak = (std::max)(maxPeak, std::abs(interp));
                }
            }
        }
    }

    return maxPeak;
}

float estimateTruePeakDbtp(const float* pInterleavedSamples,
                           std::size_t frameCount,
                           std::uint32_t channels) noexcept {
    const float lin = estimateTruePeakLinear(pInterleavedSamples, frameCount, channels);
    if (!std::isfinite(lin) || lin <= 1e-6f) return -120.0f;
    return 20.0f * std::log10(lin);
}

} // namespace analysis

namespace {

double safeJsonDouble(double v) noexcept { return std::isfinite(v) ? v : 0.0; }

float safeJsonFloat(float v) noexcept {
    return std::isfinite(v) ? v : 0.0f;
}

std::string escapeJsonString(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

} // namespace

std::string ToneProbeResult::toJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"manifest_version\": \"1.0\",\n";
    ss << "  \"scenario_id\": \"tone_probe_v0\",\n";
    ss << "  \"build_id\": \"" << escapeJsonString(buildId) << "\",\n";
    ss << "  \"verdict\": \"" << (passed ? "PASS" : "FAIL") << "\",\n";
    ss << "  \"transport_epoch\": " << transportEpoch << ",\n";
    ss << "  \"simulation_ticks\": " << simulationTicks << ",\n";
    ss << "  \"requested_sample_frames\": " << requestedSampleFrames << ",\n";
    ss << "  \"applied_sample_frames\": " << appliedSampleFrames << ",\n";
    ss << "  \"presentation_quality\": \"" << escapeJsonString(presentationQuality) << "\",\n";
    ss << "  \"wav_write_success\": " << (wavWriteSuccess ? "true" : "false") << ",\n";
    ss << "  \"manifest_write_success\": " << (manifestWriteSuccess ? "true" : "false") << ",\n";
    ss << "  \"timing_profile\": {\n";
    ss << "    \"sample_rate\": 48000,\n";
    ss << "    \"simulation_hz\": 120,\n";
    ss << "    \"frames_per_tick\": 400\n";
    ss << "  },\n";
    ss << "  \"telemetry\": {\n";
    ss << "    \"total_captured_frames\": " << totalCapturedFrames << ",\n";
    ss << "    \"dropped_frames\": " << droppedFrames << "\n";
    ss << "  },\n";
    ss << "  \"phase_1_baseline\": {\n";
    ss << "    \"start_ms\": " << phase1Baseline.startMs << ",\n";
    ss << "    \"start_sample_frame\": " << phase1Baseline.startSampleFrame << ",\n";
    ss << "    \"duration_ms\": " << phase1Baseline.durationMs << ",\n";
    ss << "    \"target_freq_hz\": " << safeJsonFloat(phase1Baseline.targetFreqHz) << ",\n";
    ss << "    \"estimated_freq_hz\": " << safeJsonFloat(phase1Baseline.estimatedFreqHz) << ",\n";
    ss << "    \"rms_dbfs\": " << safeJsonFloat(phase1Baseline.rmsDbfs) << ",\n";
    ss << "    \"freq_passed\": " << (phase1Baseline.freqPassed ? "true" : "false") << ",\n";
    ss << "    \"rms_passed\": " << (phase1Baseline.rmsPassed ? "true" : "false") << ",\n";
    ss << "    \"passed\": " << (phase1Baseline.passed ? "true" : "false") << "\n";
    ss << "  },\n";
    ss << "  \"phase_2_hold\": {\n";
    ss << "    \"start_ms\": " << phase2Hold.startMs << ",\n";
    ss << "    \"start_sample_frame\": " << phase2Hold.startSampleFrame << ",\n";
    ss << "    \"duration_ms\": " << phase2Hold.durationMs << ",\n";
    ss << "    \"target_freq_hz\": " << safeJsonFloat(phase2Hold.targetFreqHz) << ",\n";
    ss << "    \"estimated_freq_hz\": " << safeJsonFloat(phase2Hold.estimatedFreqHz) << ",\n";
    ss << "    \"rms_dbfs\": " << safeJsonFloat(phase2Hold.rmsDbfs) << ",\n";
    ss << "    \"freq_passed\": " << (phase2Hold.freqPassed ? "true" : "false") << ",\n";
    ss << "    \"rms_passed\": " << (phase2Hold.rmsPassed ? "true" : "false") << ",\n";
    ss << "    \"passed\": " << (phase2Hold.passed ? "true" : "false") << "\n";
    ss << "  },\n";
    ss << "  \"phase_3_release\": {\n";
    ss << "    \"start_ms\": " << phase3Release.startMs << ",\n";
    ss << "    \"start_sample_frame\": " << phase3Release.startSampleFrame << ",\n";
    ss << "    \"duration_ms\": " << phase3Release.durationMs << ",\n";
    ss << "    \"target_freq_hz\": " << safeJsonFloat(phase3Release.targetFreqHz) << ",\n";
    ss << "    \"estimated_freq_hz\": " << safeJsonFloat(phase3Release.estimatedFreqHz) << ",\n";
    ss << "    \"rms_dbfs\": " << safeJsonFloat(phase3Release.rmsDbfs) << ",\n";
    ss << "    \"freq_passed\": " << (phase3Release.freqPassed ? "true" : "false") << ",\n";
    ss << "    \"rms_passed\": " << (phase3Release.rmsPassed ? "true" : "false") << ",\n";
    ss << "    \"passed\": " << (phase3Release.passed ? "true" : "false") << "\n";
    ss << "  },\n";
    ss << "  \"artifacts\": {\n";
    ss << "    \"wav_file\": \"" << escapeJsonString(wavFilePath) << "\",\n";
    ss << "    \"manifest_file\": \"" << escapeJsonString(manifestFilePath) << "\"\n";
    ss << "  }";
    if (!failureReason.empty()) {
        ss << ",\n  \"failure_reason\": \"" << escapeJsonString(failureReason) << "\"";
    }
    ss << "\n}\n";
    return ss.str();
}

static ToneProbeResult evaluateToneRecording(const std::vector<float>& stereoSamples,
                                             std::uint32_t sampleRate,
                                             std::uint64_t droppedFrames,
                                             const std::string& outputDir,
                                             bool writeFiles) {
    ToneProbeResult result{};
    result.totalCapturedFrames = stereoSamples.size() / 2;
    result.droppedFrames = droppedFrames;
    result.appliedSampleFrames = result.totalCapturedFrames;

    const std::size_t totalExpectedFrames = 3 * sampleRate; // 3.0 s = 144,000 frames
    if (result.totalCapturedFrames < totalExpectedFrames) {
        result.passed = false;
        result.failureReason = "Incomplete capture: expected at least 144000 frames, got " +
                               std::to_string(result.totalCapturedFrames);
        return result;
    }

    // Extract mono (left channel) for analysis
    std::vector<float> mono(result.totalCapturedFrames);
    for (std::size_t i = 0; i < result.totalCapturedFrames; ++i) {
        mono[i] = stereoSamples[i * 2];
    }

    // Phase 1 stable window: 500 ms to 750 ms (sample 24000 to 36000)
    const std::size_t p1Start = (500 * sampleRate) / 1000;
    const std::size_t windowFrames = (250 * sampleRate) / 1000; // 12000 frames
    result.phase1Baseline.startMs = 500;
    result.phase1Baseline.startSampleFrame = p1Start;
    result.phase1Baseline.durationMs = 250;
    result.phase1Baseline.targetFreqHz = 220.0f;
    result.phase1Baseline.estimatedFreqHz = analysis::estimatePitchHz(&mono[p1Start], windowFrames, sampleRate);
    result.phase1Baseline.rmsDbfs = analysis::computeRmsDbfs(&mono[p1Start], windowFrames);
    result.phase1Baseline.freqPassed = std::abs(result.phase1Baseline.estimatedFreqHz - 220.0f) <= 2.0f;
    result.phase1Baseline.rmsPassed = result.phase1Baseline.rmsDbfs > -40.0f;
    result.phase1Baseline.passed = result.phase1Baseline.freqPassed && result.phase1Baseline.rmsPassed;

    // Phase 2 stable window: 1500 ms to 1750 ms (sample 72000 to 84000)
    const std::size_t p2Start = (1500 * sampleRate) / 1000;
    result.phase2Hold.startMs = 1500;
    result.phase2Hold.startSampleFrame = p2Start;
    result.phase2Hold.durationMs = 250;
    result.phase2Hold.targetFreqHz = 440.0f;
    result.phase2Hold.estimatedFreqHz = analysis::estimatePitchHz(&mono[p2Start], windowFrames, sampleRate);
    result.phase2Hold.rmsDbfs = analysis::computeRmsDbfs(&mono[p2Start], windowFrames);
    result.phase2Hold.freqPassed = std::abs(result.phase2Hold.estimatedFreqHz - 440.0f) <= 2.0f;
    result.phase2Hold.rmsPassed = result.phase2Hold.rmsDbfs > -40.0f;
    result.phase2Hold.passed = result.phase2Hold.freqPassed && result.phase2Hold.rmsPassed;

    // Phase 3 stable window: 2500 ms to 2750 ms (sample 120000 to 132000)
    const std::size_t p3Start = (2500 * sampleRate) / 1000;
    result.phase3Release.startMs = 2500;
    result.phase3Release.startSampleFrame = p3Start;
    result.phase3Release.durationMs = 250;
    result.phase3Release.targetFreqHz = 220.0f;
    result.phase3Release.estimatedFreqHz = analysis::estimatePitchHz(&mono[p3Start], windowFrames, sampleRate);
    result.phase3Release.rmsDbfs = analysis::computeRmsDbfs(&mono[p3Start], windowFrames);
    result.phase3Release.freqPassed = std::abs(result.phase3Release.estimatedFreqHz - 220.0f) <= 2.0f;
    result.phase3Release.rmsPassed = result.phase3Release.rmsDbfs > -40.0f;
    result.phase3Release.passed = result.phase3Release.freqPassed && result.phase3Release.rmsPassed;

    const bool noDrops = (droppedFrames == 0);
    result.passed = result.phase1Baseline.passed &&
                    result.phase2Hold.passed &&
                    result.phase3Release.passed &&
                    noDrops;

    if (!result.passed) {
        std::string err;
        if (!result.phase1Baseline.passed) err += "[Phase 1 baseline failed] ";
        if (!result.phase2Hold.passed) err += "[Phase 2 hold failed] ";
        if (!result.phase3Release.passed) err += "[Phase 3 release failed] ";
        if (!noDrops) err += "[Probe dropped " + std::to_string(droppedFrames) + " frames] ";
        result.failureReason = err;
    }

    if (writeFiles) {
        std::filesystem::path dir(outputDir);
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        result.wavFilePath = (dir / "tone_probe_v0.wav").lexically_normal().string();
        result.manifestFilePath = (dir / "tone_probe_v0_manifest.json").lexically_normal().string();

        const bool wavOk = analysis::writeWavStereo16(result.wavFilePath, stereoSamples.data(), result.totalCapturedFrames, sampleRate);
        result.wavWriteSuccess = wavOk;
        if (!wavOk) {
            result.passed = false;
            result.failureReason += "[WAV write failed] ";
        }

        std::ofstream mf(result.manifestFilePath);
        if (!mf.is_open()) {
            result.passed = false;
            result.manifestWriteSuccess = false;
            result.failureReason += "[Manifest write failed] ";
        } else {
            result.manifestWriteSuccess = true;
            mf << result.toJson();
            if (!mf.good()) {
                result.passed = false;
                result.manifestWriteSuccess = false;
                result.failureReason += "[Manifest write failed] ";
            }
        }
    }

    return result;
}

ToneProbeResult runToneProbeV0(AudioEngine& engine,
                               cymatica::core::InputDispatcher& dispatcher,
                               const std::string& outputDir,
                               bool writeFiles) {
    AudioProbe probe(AudioProbe::kDefaultCapacityFrames, 2, engine.sampleRate());
    engine.attachAudioProbe(&probe);

    dispatcher.startDiagnosticSession();

    const std::uint32_t sampleRate = engine.sampleRate();
    constexpr std::uint32_t kFramesPerTick = 400; // 48000 / 120
    constexpr std::uint32_t kTotalTicks = 360;    // 3.0 s * 120 ticks/s = 360 ticks
    constexpr float kAmplitude = 0.10f;

    std::vector<float> recordedStereo;
    recordedStereo.reserve(static_cast<std::size_t>(kTotalTicks) * kFramesPerTick * 2);
    std::vector<float> readChunk(kFramesPerTick * 2);
    std::vector<float> tickBlock(kFramesPerTick * 2);

    for (std::uint32_t tick = 0; tick < kTotalTicks; ++tick) {
        // Timeline scenario:
        // [0.0s, 1.0s) = tick 0..119: baseline Released (220 Hz)
        // [1.0s, 2.0s) = tick 120..239: hold Pressed/Held (440 Hz)
        // [2.0s, 3.0s) = tick 240..359: release Released (220 Hz)
        cymatica::core::NormalizedAction act{
            .action = cymatica::core::InputAction::ToneShift,
            .state = (tick >= 120 && tick < 240) ?
                     ((tick == 120) ? cymatica::core::ActionState::Pressed : cymatica::core::ActionState::Held) :
                     cymatica::core::ActionState::Released,
            .source = cymatica::core::ActionSource::AgentRunner,
            .targetTick = tick
        };
        dispatcher.injectAction(act);

        const float targetFreq = dispatcher.isDown(cymatica::core::InputAction::ToneShift) ? 440.0f : 220.0f;
        auto& ctrl = engine.controlWriter();
        ctrl.toneFrequencyHz = targetFreq;
        ctrl.toneVolume = kAmplitude;
        engine.publishControl();

        // Process fixed-step tick block (zero alloc in realtime path)
        engine.processBlock(tickBlock.data(), kFramesPerTick);

        // Consume frames from probe into recording accumulator
        const std::uint32_t framesRead = probe.readFrames(readChunk.data(), kFramesPerTick);
        if (framesRead > 0) {
            recordedStereo.insert(recordedStereo.end(), readChunk.begin(), readChunk.begin() + (framesRead * 2));
        }
    }

    engine.detachAudioProbe();
    dispatcher.endDiagnosticSession();

    auto res = evaluateToneRecording(recordedStereo, sampleRate, probe.droppedFrames(), outputDir, writeFiles);
    res.simulationTicks = kTotalTicks;
    res.requestedSampleFrames = static_cast<std::uint64_t>(kTotalTicks) * kFramesPerTick;
    res.appliedSampleFrames = res.totalCapturedFrames;
    (void)engine.updateTelemetry();
    res.transportEpoch = engine.telemetry().transportEpoch;
    res.presentationQuality = "Estimated";
    return res;
}

ToneProbeResult runToneProbeV0Offline(const std::string& outputDir, bool writeFiles) {
    AudioEngine engine;
    if (!engine.initOffline({48000, 2, 220.0f})) {
        ToneProbeResult res{};
        res.passed = false;
        res.failureReason = "Failed to initialize AudioEngine in offline mode";
        return res;
    }
    // Set running state in offline engine for direct processBlock rendering without physical device callback (DIF-M2-01)
    engine.startTone(220.0f, 0.10f);

    cymatica::core::InputDispatcher dispatcher(cymatica::core::ActionSource::PhysicalInput);
    auto res = runToneProbeV0(engine, dispatcher, outputDir, writeFiles);
    engine.shutdown();
    return res;
}

std::string VoiceProbeResult::toJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"manifest_version\": \"1.0\",\n";
    ss << "  \"scenario_id\": \"voice_probe_v1\",\n";
    ss << "  \"build_id\": \"" << escapeJsonString(buildId) << "\",\n";
    ss << "  \"verdict\": \"" << (passed ? "PASS" : "FAIL") << "\",\n";
    ss << "  \"transport_epoch\": " << transportEpoch << ",\n";
    ss << "  \"simulation_ticks\": " << simulationTicks << ",\n";
    ss << "  \"requested_sample_frames\": " << requestedSampleFrames << ",\n";
    ss << "  \"applied_sample_frames\": " << totalCapturedFrames << ",\n";
    ss << "  \"presentation_quality\": \"" << escapeJsonString(presentationQuality) << "\",\n";
    ss << "  \"timing_profile\": {\n";
    ss << "    \"sample_rate\": 48000,\n";
    ss << "    \"simulation_hz\": 120,\n";
    ss << "    \"frames_per_tick\": 400\n";
    ss << "  },\n";
    ss << "  \"dropped_frames\": " << droppedFrames << ",\n";
    ss << "  \"voice_steals_detected\": " << voiceStealsDetected << ",\n";
    ss << "  \"active_voices_phase_1\": " << activeVoicesPhase1 << ",\n";
    ss << "  \"peak_amplitude\": " << safeJsonFloat(peakAmplitude) << ",\n";
    ss << "  \"clipped_samples\": " << clippedSamples << ",\n";
    ss << "  \"checks\": {\n";
    ss << "    \"steal_continuity_passed\": " << (stealContinuityPassed ? "true" : "false") << ",\n";
    ss << "    \"pause_silence_passed\": " << (pauseSilencePassed ? "true" : "false") << ",\n";
    ss << "    \"resume_passed\": " << (resumePassed ? "true" : "false") << ",\n";
    ss << "    \"stop_silence_passed\": " << (stopSilencePassed ? "true" : "false") << ",\n";
    ss << "    \"headroom_passed\": " << (headroomPassed ? "true" : "false") << ",\n";
    ss << "    \"zero_clipping_passed\": " << (clippedSamples == 0 ? "true" : "false") << ",\n";
    ss << "    \"dsp_anomalies_passed\": " << (dspAnomaliesPassed ? "true" : "false") << "\n";
    ss << "  },\n";
    ss << "  \"wav_write_success\": " << (wavWriteSuccess ? "true" : "false") << ",\n";
    ss << "  \"manifest_write_success\": " << (manifestWriteSuccess ? "true" : "false") << ",\n";
    ss << "  \"artifacts\": {\n";
    ss << "    \"wav_file\": \"" << escapeJsonString(wavFilePath) << "\",\n";
    ss << "    \"manifest_file\": \"" << escapeJsonString(manifestFilePath) << "\"\n";
    ss << "  }";
    if (!failureReason.empty()) {
        ss << ",\n  \"failure_reason\": \"" << escapeJsonString(failureReason) << "\"";
    }
    ss << "\n}\n";
    return ss.str();
}

VoiceProbeResult runVoiceProbeV1Offline(const std::string& outputDir, bool writeFiles) {
    VoiceProbeResult result{};
    result.simulationTicks = 300;
    result.requestedSampleFrames = 120000;

    AudioEngine engine;
    if (!engine.initOffline({48000, 2, 220.0f})) {
        result.passed = false;
        result.failureReason = "Failed to initialize AudioEngine in offline mode";
        return result;
    }

    AudioProbe probe(48000, 2, 48000);
    engine.attachAudioProbe(&probe);

    std::vector<float> recordedStereo;
    recordedStereo.reserve(120000 * 2);

    constexpr std::uint32_t kFramesPerTick = 400; // 48000 / 120
    float tickBuffer[kFramesPerTick * 2]{};

    // 0.0s (tick 0, frame 0): Note 1 on Pulso (220 Hz)
    ScheduledAudioEvent n1{
        .eventId = 101,
        .sampleFrame = 0,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 220.0f,
        .valueB = 0.8f,
        .durationFrames = 0
    };
    engine.scheduleAudioEvent(n1);

    // 0.083s (tick 10, frame 4000): Note 2 on Pulso (330 Hz)
    ScheduledAudioEvent n2{
        .eventId = 102,
        .sampleFrame = 4000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 330.0f,
        .valueB = 0.8f,
        .durationFrames = 0
    };
    engine.scheduleAudioEvent(n2);

    // 0.5s (tick 60, frame 24000): Note 3 on Pulso (440 Hz) -> Voice Stealing on slot 0
    ScheduledAudioEvent n3{
        .eventId = 103,
        .sampleFrame = 24000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 440.0f,
        .valueB = 0.8f,
        .durationFrames = 0
    };
    engine.scheduleAudioEvent(n3);

    std::uint32_t activeVoicesAtTick30 = 0;

    for (std::uint32_t tick = 0; tick < 300; ++tick) {
        if (tick == 120) {
            engine.sendCommand(AudioCommand{.commandId = 1, .type = AudioCommandType::Pause});
        } else if (tick == 150) {
            engine.sendCommand(AudioCommand{.commandId = 2, .type = AudioCommandType::Resume});
        } else if (tick == 220) {
            engine.sendCommand(AudioCommand{.commandId = 3, .type = AudioCommandType::Stop});
        }

        engine.processBlock(tickBuffer, kFramesPerTick);

        float readBuf[kFramesPerTick * 2]{};
        const std::uint32_t readCount = probe.readFrames(readBuf, kFramesPerTick);
        recordedStereo.insert(recordedStereo.end(), readBuf, readBuf + (readCount * 2));

        if (tick == 30) {
            engine.updateTelemetry();
            activeVoicesAtTick30 = engine.telemetry().activeVoiceCount;
        }
    }

    engine.updateTelemetry();
    const std::uint32_t finalVoiceSteals = engine.telemetry().voiceDropCount;
    const std::uint32_t finalDspAnomalies = engine.telemetry().dspAnomalyCount;
    result.transportEpoch = engine.telemetry().transportEpoch;
    result.presentationQuality = "Estimated";

    engine.detachAudioProbe();
    engine.shutdown();

    result.totalCapturedFrames = recordedStereo.size() / 2;
    result.droppedFrames = probe.droppedFrames();
    result.activeVoicesPhase1 = activeVoicesAtTick30;
    result.voiceStealsDetected = finalVoiceSteals;

    // 1. Check steal continuity around frame 24000
    result.stealContinuityPassed = true;
    if (recordedStereo.size() >= 24010 * 2) {
        float maxJump = 0.0f;
        for (std::size_t i = 23995; i <= 24005; ++i) {
            const float dL = std::abs(recordedStereo[i * 2] - recordedStereo[(i - 1) * 2]);
            if (dL > maxJump) maxJump = dL;
        }
        if (maxJump > 0.40f) {
            result.stealContinuityPassed = false;
        }
    }

    // 2. Check pause silence: frames 48200..58000 must be strictly silent (< 1e-4)
    result.pauseSilencePassed = true;
    for (std::size_t i = 48200; i < 58000 && i < result.totalCapturedFrames; ++i) {
        if (std::abs(recordedStereo[i * 2]) > 1e-4f) {
            result.pauseSilencePassed = false;
            break;
        }
    }

    // 3. Check resume: frames 60500..70000 must have sounding signal
    result.resumePassed = false;
    for (std::size_t i = 60500; i < 70000 && i < result.totalCapturedFrames; ++i) {
        if (std::abs(recordedStereo[i * 2]) > 0.05f) {
            result.resumePassed = true;
            break;
        }
    }

    // 4. Check stop silence: frames 88200..120000 must be strictly silent (< 1e-4)
    result.stopSilencePassed = true;
    for (std::size_t i = 88200; i < result.totalCapturedFrames; ++i) {
        if (std::abs(recordedStereo[i * 2]) > 1e-4f) {
            result.stopSilencePassed = false;
            break;
        }
    }

    // 5. Check DSP anomalies (NaN / Inf in output or sanitized in VoiceAllocator)
    result.dspAnomaliesPassed = (finalDspAnomalies == 0);
    if (result.dspAnomaliesPassed) {
        for (float s : recordedStereo) {
            if (!std::isfinite(s)) {
                result.dspAnomaliesPassed = false;
                break;
            }
        }
    }

    // 6. Check headroom and digital clipping (ADR-0003 D-M2-06)
    float maxAmp = 0.0f;
    std::uint32_t clipCount = 0;
    for (float s : recordedStereo) {
        const float a = std::abs(s);
        if (a > maxAmp) maxAmp = a;
        if (a >= (32767.0f / 32768.0f)) {
            ++clipCount;
        }
    }
    result.peakAmplitude = maxAmp;
    result.clippedSamples = clipCount;
    // Headroom >= 6 dB (peak <= 0.5012f) and zero digital clipping
    result.headroomPassed = (result.peakAmplitude <= 0.5012f) && (result.clippedSamples == 0);

    result.passed = (result.droppedFrames == 0) &&
                    (result.activeVoicesPhase1 == 2) &&
                    (result.voiceStealsDetected == 1) &&
                    result.stealContinuityPassed &&
                    result.pauseSilencePassed &&
                    result.resumePassed &&
                    result.stopSilencePassed &&
                    result.headroomPassed &&
                    (result.clippedSamples == 0) &&
                    result.dspAnomaliesPassed;

    if (!result.passed) {
        std::ostringstream err;
        if (result.droppedFrames > 0) err << "dropped frames in probe; ";
        if (result.activeVoicesPhase1 != 2) err << "active voices phase 1 was " << result.activeVoicesPhase1 << " (expected 2); ";
        if (result.voiceStealsDetected != 1) err << "expected 1 voice steal, got " << result.voiceStealsDetected << "; ";
        if (!result.stealContinuityPassed) err << "steal continuity jump detected; ";
        if (!result.pauseSilencePassed) err << "pause was not silent; ";
        if (!result.resumePassed) err << "resume did not produce audio; ";
        if (!result.stopSilencePassed) err << "stop was not silent; ";
        if (!result.headroomPassed) err << "headroom violated (peak " << result.peakAmplitude << " > 0.5012); ";
        if (result.clippedSamples > 0) err << "clipped samples: " << result.clippedSamples << "; ";
        if (!result.dspAnomaliesPassed) err << "non-finite samples or DSP anomalies detected; ";
        result.failureReason = err.str();
    }

    if (writeFiles) {
        std::filesystem::path dir(outputDir);
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        result.wavFilePath = (dir / "voice_probe_v1.wav").lexically_normal().string();
        result.manifestFilePath = (dir / "voice_probe_v1_manifest.json").lexically_normal().string();

        const bool wavOk = analysis::writeWavStereo16(
            result.wavFilePath,
            recordedStereo.data(),
            result.totalCapturedFrames,
            48000
        );
        result.wavWriteSuccess = wavOk;
        if (!wavOk) {
            result.passed = false;
            result.failureReason += "[WAV write failed] ";
        }

        std::ofstream mf(result.manifestFilePath);
        if (!mf.is_open()) {
            result.passed = false;
            result.manifestWriteSuccess = false;
            result.failureReason += "[Manifest write failed] ";
        } else {
            result.manifestWriteSuccess = true;
            mf << result.toJson();
            if (!mf.good()) {
                result.passed = false;
                result.manifestWriteSuccess = false;
                result.failureReason += "[Manifest write failed] ";
            }
        }
    }

    return result;
}

std::string TimbreBusProbeResult::toJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"manifest_version\": \"1.0\",\n";
    ss << "  \"scenario_id\": \"timbre_bus_probe_v1\",\n";
    ss << "  \"build_id\": \"" << escapeJsonString(buildId) << "\",\n";
    ss << "  \"verdict\": \"" << (passed ? "PASS" : "FAIL") << "\",\n";
    ss << "  \"transport_epoch\": " << transportEpoch << ",\n";
    ss << "  \"simulation_ticks\": " << simulationTicks << ",\n";
    ss << "  \"requested_sample_frames\": " << requestedSampleFrames << ",\n";
    ss << "  \"applied_sample_frames\": " << totalCapturedFrames << ",\n";
    ss << "  \"presentation_quality\": \"" << escapeJsonString(presentationQuality) << "\",\n";
    ss << "  \"timing_profile\": {\n";
    ss << "    \"sample_rate\": 48000,\n";
    ss << "    \"simulation_hz\": 120,\n";
    ss << "    \"frames_per_tick\": 400\n";
    ss << "  },\n";
    ss << "  \"dropped_frames\": " << droppedFrames << ",\n";
    ss << "  \"peak_amplitude\": " << safeJsonFloat(peakAmplitude) << ",\n";
    ss << "  \"true_peak_dbtp\": " << safeJsonFloat(truePeakDbtp) << ",\n";
    ss << "  \"clipped_samples\": " << clippedSamples << ",\n";
    ss << "  \"checks\": {\n";
    ss << "    \"true_peak_passed\": " << (truePeakPassed ? "true" : "false") << ",\n";
    ss << "    \"role_isolation_passed\": " << (roleIsolationPassed ? "true" : "false") << ",\n";
    ss << "    \"ducking_passed\": " << (duckingPassed ? "true" : "false") << ",\n";
    ss << "    \"delay_passed\": " << (delayPassed ? "true" : "false") << ",\n";
    ss << "    \"zero_clipping_passed\": " << (clippedSamples == 0 ? "true" : "false") << ",\n";
    ss << "    \"dsp_anomalies_passed\": " << (dspAnomaliesPassed ? "true" : "false") << "\n";
    ss << "  },\n";
    ss << "  \"measurements\": {\n";
    ss << "    \"role_isolation_worst_leak_db\": " << safeJsonFloat(roleIsolationWorstLeakDb) << ",\n";
    ss << "    \"role_isolation_weakest_active_dbfs\": " << safeJsonFloat(roleIsolationWeakestActiveDbfs) << ",\n";
    ss << "    \"ducking_attenuation_db\": " << safeJsonFloat(duckingAttenuationDb) << ",\n";
    ss << "    \"ducking_pre_hit_deviation_db\": " << safeJsonFloat(duckingPreHitDeviationDb) << ",\n";
    ss << "    \"delay_lag_l_frames\": " << delayLagLFrames << ",\n";
    ss << "    \"delay_lag_r_frames\": " << delayLagRFrames << ",\n";
    ss << "    \"delay_echo_rms_dbfs\": " << safeJsonFloat(delayEchoRmsDbfs) << ",\n";
    ss << "    \"delay_late_echo_rms_dbfs\": " << safeJsonFloat(delayLateEchoRmsDbfs) << "\n";
    ss << "  },\n";
    ss << "  \"wav_write_success\": " << (wavWriteSuccess ? "true" : "false") << ",\n";
    ss << "  \"manifest_write_success\": " << (manifestWriteSuccess ? "true" : "false") << ",\n";
    ss << "  \"artifacts\": {\n";
    ss << "    \"wav_file\": \"" << escapeJsonString(wavFilePath) << "\",\n";
    ss << "    \"manifest_file\": \"" << escapeJsonString(manifestFilePath) << "\"\n";
    ss << "  }";
    if (!failureReason.empty()) {
        ss << ",\n  \"failure_reason\": \"" << escapeJsonString(failureReason) << "\"";
    }
    ss << "\n}\n";
    return ss.str();
}

namespace {

// One offline render of the timbre_bus_probe_v1 event timeline. The probe renders the same timeline in three
// variants so that each bus property is measured by difference against an independent reference.
enum class TimbreScenarioVariant : std::uint8_t {
    Main,          // Full scenario
    NoPulsoHit,    // Pulso hit 1003 (frame 32000) omitted: reference for Corpo ducking
    DelayBypassed  // Trama echo path bypassed (dry factor unchanged): reference for the echo
};

struct TimbreScenarioRender {
    bool initialized{false};
    std::vector<float> stereo{};
    std::vector<AudioTelemetryFrame> tickTelemetry{};
    std::uint64_t droppedFrames{0};
    std::uint64_t transportEpoch{1};
    std::uint32_t dspAnomalies{0};
};

constexpr std::uint32_t kTimbreFramesPerTick = 400; // 48000 / 120
constexpr std::uint32_t kTimbreTotalTicks = 360;    // 3.0 s * 120 ticks/s
constexpr std::uint64_t kTimbreTramaOnsetFrame = 56000;
constexpr std::uint64_t kTimbreTramaEndFrame = 68000 + 96; // duration expiry + 2 ms release

TimbreScenarioRender renderTimbreBusScenario(TimbreScenarioVariant variant) {
    TimbreScenarioRender out{};

    AudioEngine engine;
    if (!engine.initOffline({48000, 2, 220.0f})) {
        return out;
    }
    out.initialized = true;
    if (variant == TimbreScenarioVariant::DelayBypassed) {
        engine.voiceAllocator().roleBusProcessor().setDelayBypassed(true);
    }

    AudioProbe probe(AudioProbe::kDefaultCapacityFrames, 2, engine.sampleRate());
    (void)engine.attachAudioProbe(&probe);

    engine.sendCommand(AudioCommand{
        .commandId = 1,
        .type = AudioCommandType::Start,
        .targetSampleFrame = 0
    });

    engine.scheduleAudioEvent(ScheduledAudioEvent{
        .eventId = 1001,
        .sampleFrame = 0,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 110.0f,
        .valueB = 0.9f,
        .durationFrames = 2400,
        .timbreParam = 0.5f
    });

    engine.scheduleAudioEvent(ScheduledAudioEvent{
        .eventId = 1002,
        .sampleFrame = 16000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Corpo,
        .valueA = 220.0f,
        .valueB = 0.8f,
        .durationFrames = 0,
        .timbreParam = 0.3f
    });

    if (variant != TimbreScenarioVariant::NoPulsoHit) {
        engine.scheduleAudioEvent(ScheduledAudioEvent{
            .eventId = 1003,
            .sampleFrame = 32000,
            .transportEpoch = 1,
            .type = AudioEventType::NoteOn,
            .role = MusicRole::Pulso,
            .valueA = 110.0f,
            .valueB = 1.0f,
            .durationFrames = 2400,
            .timbreParam = 0.6f
        });
    }

    engine.scheduleAudioEvent(ScheduledAudioEvent{
        .eventId = 1002,
        .sampleFrame = 56000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOff,
        .role = MusicRole::Corpo
    });

    engine.scheduleAudioEvent(ScheduledAudioEvent{
        .eventId = 1004,
        .sampleFrame = kTimbreTramaOnsetFrame,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Trama,
        .valueA = 440.0f,
        .valueB = 0.7f,
        .durationFrames = 12000,
        .timbreParam = 0.4f
    });

    engine.scheduleAudioEvent(ScheduledAudioEvent{
        .eventId = 1005,
        .sampleFrame = 80000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Vettore,
        .valueA = 330.0f,
        .valueB = 0.8f,
        .durationFrames = 20000,
        .timbreParam = 0.5f
    });

    out.stereo.reserve(static_cast<std::size_t>(kTimbreTotalTicks) * kTimbreFramesPerTick * 2);
    out.tickTelemetry.reserve(kTimbreTotalTicks);
    std::vector<float> readChunk(kTimbreFramesPerTick * 2);
    std::vector<float> tickBlock(kTimbreFramesPerTick * 2);

    for (std::uint32_t tick = 0; tick < kTimbreTotalTicks; ++tick) {
        if (tick == 300) {
            engine.sendCommand(AudioCommand{
                .commandId = 2,
                .type = AudioCommandType::Stop,
                .targetSampleFrame = 120000
            });
        }

        engine.processBlock(tickBlock.data(), kTimbreFramesPerTick);
        (void)engine.updateTelemetry();
        out.tickTelemetry.push_back(engine.telemetry());

        const std::uint32_t framesRead = probe.readFrames(readChunk.data(), kTimbreFramesPerTick);
        if (framesRead > 0) {
            out.stereo.insert(out.stereo.end(), readChunk.begin(), readChunk.begin() + (framesRead * 2));
        }
    }

    out.droppedFrames = probe.droppedFrames();
    (void)engine.updateTelemetry();
    out.transportEpoch = engine.telemetry().transportEpoch;
    out.dspAnomalies = engine.telemetry().dspAnomalyCount;

    engine.detachAudioProbe();
    engine.shutdown();
    return out;
}

float linearToDb(double v) noexcept {
    return (v > 1e-12) ? static_cast<float>(20.0 * std::log10(v)) : -240.0f;
}

const ChannelFrame& roleChannel(const AudioTelemetryFrame& t, std::size_t role) noexcept {
    switch (role) {
        case 0: return t.pulse;
        case 1: return t.body;
        case 2: return t.texture;
        default: return t.vector;
    }
}

} // namespace

TimbreBusProbeResult runTimbreBusProbeV1Offline(const std::string& outputDir, bool writeFiles) {
    TimbreBusProbeResult result{};
    result.simulationTicks = kTimbreTotalTicks;
    result.requestedSampleFrames = static_cast<std::uint64_t>(kTimbreTotalTicks) * kTimbreFramesPerTick;
    result.presentationQuality = "Estimated";

    const TimbreScenarioRender mainRun = renderTimbreBusScenario(TimbreScenarioVariant::Main);
    const TimbreScenarioRender noHitRun = renderTimbreBusScenario(TimbreScenarioVariant::NoPulsoHit);
    const TimbreScenarioRender bypassRun = renderTimbreBusScenario(TimbreScenarioVariant::DelayBypassed);
    if (!mainRun.initialized || !noHitRun.initialized || !bypassRun.initialized) {
        result.passed = false;
        result.failureReason = "Failed to initialize AudioEngine in offline mode";
        return result;
    }

    const std::vector<float>& recordedStereo = mainRun.stereo;
    result.totalCapturedFrames = recordedStereo.size() / 2;
    result.droppedFrames = mainRun.droppedFrames + noHitRun.droppedFrames + bypassRun.droppedFrames;
    result.transportEpoch = mainRun.transportEpoch;
    const std::uint32_t finalDspAnomalies = mainRun.dspAnomalies + noHitRun.dspAnomalies + bypassRun.dspAnomalies;

    const std::uint64_t minFrames = result.requestedSampleFrames;
    if (result.totalCapturedFrames < minFrames || noHitRun.stereo.size() / 2 < minFrames ||
        bypassRun.stereo.size() / 2 < minFrames || mainRun.tickTelemetry.size() < kTimbreTotalTicks ||
        noHitRun.tickTelemetry.size() < kTimbreTotalTicks) {
        result.passed = false;
        result.failureReason = "Incomplete capture: expected at least " +
                               std::to_string(result.requestedSampleFrames) + " frames, got " +
                               std::to_string(result.totalCapturedFrames);
        return result;
    }

    // 1. Digital sample peak and clipping check
    float maxAmp = 0.0f;
    std::uint32_t clipCount = 0;
    for (float s : recordedStereo) {
        const float a = std::abs(s);
        if (a > maxAmp) maxAmp = a;
        if (a >= (32767.0f / 32768.0f)) {
            ++clipCount;
        }
    }
    result.peakAmplitude = maxAmp;
    result.clippedSamples = clipCount;

    // 2. Independent True-Peak verification (ITU-R BS.1770-5 48-tap FIR)
    result.truePeakDbtp = analysis::estimateTruePeakDbtp(recordedStereo.data(), result.totalCapturedFrames, 2);
    result.truePeakPassed = (result.truePeakDbtp <= -1.0f);

    // 3. DSP anomalies (voice/bus sanitisations + terminal limiter guard, all three renders) and finiteness
    result.dspAnomaliesPassed = (finalDspAnomalies == 0);
    if (result.dspAnomaliesPassed) {
        for (float s : recordedStereo) {
            if (!std::isfinite(s)) {
                result.dspAnomaliesPassed = false;
                break;
            }
        }
    }

    // 4. Role isolation from per-callback role bus telemetry: inside each role's solo window the role bus
    // carries energy above -40 dBFS and every other role bus stays at least 60 dB below it.
    struct RoleWindow {
        std::size_t role;
        std::uint32_t firstTick;
        std::uint32_t lastTick;
    };
    constexpr RoleWindow kRoleWindows[] = {
        {0, 1, 4},     // Pulso   frames   400..2000
        {1, 45, 75},   // Corpo   frames 18000..30400
        {2, 145, 165}, // Trama   frames 58000..66400
        {3, 205, 245}, // Vettore frames 82000..98400
    };
    result.roleIsolationPassed = true;
    float worstLeakDb = -240.0f;
    float weakestActiveDbfs = 0.0f;
    for (const auto& w : kRoleWindows) {
        for (std::uint32_t t = w.firstTick; t <= w.lastTick; ++t) {
            const auto& telem = mainRun.tickTelemetry[t];
            const double active = roleChannel(telem, w.role).energy;
            const float activeDbfs = linearToDb(active);
            weakestActiveDbfs = std::min(weakestActiveDbfs, activeDbfs);
            if (activeDbfs <= -40.0f) {
                result.roleIsolationPassed = false;
            }
            for (std::size_t other = 0; other < 4; ++other) {
                if (other == w.role) continue;
                const double leak = roleChannel(telem, other).energy;
                const float leakDb = (active > 0.0) ? linearToDb(leak / active) : 0.0f;
                worstLeakDb = std::max(worstLeakDb, leakDb);
                if (leakDb > -60.0f) {
                    result.roleIsolationPassed = false;
                }
            }
        }
    }
    result.roleIsolationWorstLeakDb = worstLeakDb;
    result.roleIsolationWeakestActiveDbfs = weakestActiveDbfs;

    // 5. Ducking: Corpo bus energy during the Pulso hit (ticks 80..100) against the reference render without
    // the hit. Required peak attenuation in [1.5, 3.0 + 0.1] dB; before the hit the buses must be identical.
    float maxDuckDb = 0.0f;
    for (std::uint32_t t = 80; t <= 100; ++t) {
        const double withHit = mainRun.tickTelemetry[t].body.energy;
        const double reference = noHitRun.tickTelemetry[t].body.energy;
        if (withHit > 0.0 && reference > 0.0) {
            maxDuckDb = std::max(maxDuckDb, linearToDb(reference / withHit));
        }
    }
    float preHitDeviationDb = 0.0f;
    for (std::uint32_t t = 60; t < 80; ++t) {
        const double withHit = mainRun.tickTelemetry[t].body.energy;
        const double reference = noHitRun.tickTelemetry[t].body.energy;
        if (withHit > 0.0 && reference > 0.0) {
            preHitDeviationDb = std::max(preHitDeviationDb, std::abs(linearToDb(reference / withHit)));
        } else if (withHit != reference) {
            preHitDeviationDb = 240.0f;
        }
    }
    result.duckingAttenuationDb = maxDuckDb;
    result.duckingPreHitDeviationDb = preHitDeviationDb;
    result.duckingPassed = (maxDuckDb >= 1.5f) && (maxDuckDb <= 3.1f) && (preHitDeviationDb <= 0.05f);

    // 6. Delay: the echo path is isolated exactly as (main - bypassed). Its first non-zero sample after the
    // Trama onset must arrive at the nominal 11 ms (L) / 17 ms (R) taps (+1 frame: the attack starts at 0),
    // carry real energy, and decay after the note ends (bounded feedback).
    const auto echoSample = [&](std::size_t frame, std::size_t ch) noexcept {
        return recordedStereo[frame * 2 + ch] - bypassRun.stereo[frame * 2 + ch];
    };
    const auto firstEchoLag = [&](std::size_t ch) noexcept -> std::int64_t {
        for (std::size_t n = static_cast<std::size_t>(kTimbreTramaOnsetFrame); n < kTimbreTramaEndFrame; ++n) {
            if (std::abs(echoSample(n, ch)) > 1e-7f) {
                return static_cast<std::int64_t>(n) - static_cast<std::int64_t>(kTimbreTramaOnsetFrame);
            }
        }
        return -1;
    };
    const std::int64_t expectedLagL = static_cast<std::int64_t>(0.011f * 48000.0f) + 1;
    const std::int64_t expectedLagR = static_cast<std::int64_t>(0.017f * 48000.0f) + 1;
    result.delayLagLFrames = firstEchoLag(0);
    result.delayLagRFrames = firstEchoLag(1);

    double echoSumSq = 0.0;
    for (std::size_t n = static_cast<std::size_t>(kTimbreTramaOnsetFrame) + 1000; n < kTimbreTramaEndFrame; ++n) {
        const double l = echoSample(n, 0);
        const double r = echoSample(n, 1);
        echoSumSq += l * l + r * r;
    }
    const double echoFrames = static_cast<double>(kTimbreTramaEndFrame - kTimbreTramaOnsetFrame - 1000);
    result.delayEchoRmsDbfs = linearToDb(std::sqrt(echoSumSq / (2.0 * echoFrames)));

    double lateSumSq = 0.0;
    for (std::size_t n = kTimbreTramaEndFrame + 5000; n < kTimbreTramaEndFrame + 6000; ++n) {
        const double l = echoSample(n, 0);
        const double r = echoSample(n, 1);
        lateSumSq += l * l + r * r;
    }
    result.delayLateEchoRmsDbfs = linearToDb(std::sqrt(lateSumSq / 2000.0));

    result.delayPassed = (std::llabs(result.delayLagLFrames - expectedLagL) <= 1) &&
                         (std::llabs(result.delayLagRFrames - expectedLagR) <= 1) &&
                         (result.delayEchoRmsDbfs > -50.0f) &&
                         (result.delayLateEchoRmsDbfs < result.delayEchoRmsDbfs - 60.0f);

    result.passed = (result.droppedFrames == 0) &&
                    result.truePeakPassed &&
                    result.roleIsolationPassed &&
                    result.duckingPassed &&
                    result.delayPassed &&
                    result.dspAnomaliesPassed &&
                    (result.clippedSamples == 0);

    if (!result.passed) {
        std::ostringstream err;
        if (result.droppedFrames > 0) err << "dropped frames in probe; ";
        if (!result.truePeakPassed) err << "true peak ceiling violated (" << result.truePeakDbtp << " > -1.0 dBTP); ";
        if (!result.roleIsolationPassed) {
            err << "role isolation failed: worst leak " << result.roleIsolationWorstLeakDb
                << " dB, weakest active " << result.roleIsolationWeakestActiveDbfs << " dBFS; ";
        }
        if (!result.duckingPassed) {
            err << "ducking check failed: attenuation=" << result.duckingAttenuationDb
                << " dB pre-hit deviation=" << result.duckingPreHitDeviationDb << " dB; ";
        }
        if (!result.delayPassed) {
            err << "delay check failed: lagL=" << result.delayLagLFrames << " lagR=" << result.delayLagRFrames
                << " echo=" << result.delayEchoRmsDbfs << " dBFS late=" << result.delayLateEchoRmsDbfs << " dBFS; ";
        }
        if (result.clippedSamples > 0) err << "clipped samples: " << result.clippedSamples << "; ";
        if (!result.dspAnomaliesPassed) err << "DSP anomalies or non-finite values detected; ";
        result.failureReason = err.str();
    }

    if (writeFiles) {
        std::filesystem::path dir(outputDir);
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        result.wavFilePath = (dir / "timbre_bus_probe_v1.wav").lexically_normal().string();
        result.manifestFilePath = (dir / "timbre_bus_probe_v1_manifest.json").lexically_normal().string();

        const bool wavOk = analysis::writeWavStereo16(
            result.wavFilePath,
            recordedStereo.data(),
            result.totalCapturedFrames,
            48000
        );
        result.wavWriteSuccess = wavOk;
        if (!wavOk) {
            result.passed = false;
            result.failureReason += "[WAV write failed] ";
        }

        std::ofstream mf(result.manifestFilePath);
        if (!mf.is_open()) {
            result.passed = false;
            result.manifestWriteSuccess = false;
            result.failureReason += "[Manifest write failed] ";
        } else {
            result.manifestWriteSuccess = true;
            mf << result.toJson();
            if (!mf.good()) {
                result.passed = false;
                result.manifestWriteSuccess = false;
                result.failureReason += "[Manifest write failed] ";
            }
        }
    }

    return result;
}


std::string PlannerProbeResult::toJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"manifest_version\": \"1.1\",\n";
    ss << "  \"scenario_id\": \"planner_probe_v1\",\n";
    ss << "  \"build_id\": \"" << escapeJsonString(buildId) << "\",\n";
    ss << "  \"verdict\": \"" << (passed ? "PASS" : "FAIL") << "\",\n";
    ss << "  \"scoring_profile\": \"" << kPlannerScoringProfileV1.name << "\",\n";
    ss << "  \"scoring_profile_version\": " << kPlannerScoringProfileV1.versionId << ",\n";
    ss << "  \"final_transport_epoch\": " << transportEpoch << ",\n";
    ss << "  \"simulation_ticks\": " << simulationTicks << ",\n";
    ss << "  \"requested_sample_frames\": " << requestedSampleFrames << ",\n";
    ss << "  \"applied_sample_frames\": " << totalCapturedFrames << ",\n";
    ss << "  \"content_end_frame\": " << contentEndFrame << ",\n";
    ss << "  \"stop_frame\": " << stopFrame << ",\n";
    ss << "  \"presentation_quality\": \"" << escapeJsonString(presentationQuality) << "\",\n";
    ss << "  \"timing_profile\": {\n";
    ss << "    \"sample_rate\": 48000,\n";
    ss << "    \"simulation_hz\": " << core::FixedStepConfig{}.simulationHz << ",\n";
    ss << "    \"frames_per_tick\": " << (48000U / core::FixedStepConfig{}.simulationHz) << ",\n";
    ss << "    \"lookahead_frames\": " << AudioSchedulerConfig{}.lookaheadFrames << "\n";
    ss << "  },\n";
    ss << "  \"tempo_change\": {\n";
    ss << "    \"requested_frame\": " << tempoChangeFrame << ",\n";
    ss << "    \"ack_status\": " << tempoAckStatus << ",\n";
    ss << "    \"ack_frame\": " << tempoAckFrame << ",\n";
    ss << "    \"ack_epoch\": " << tempoAckEpoch << "\n";
    ss << "  },\n";
    ss << "  \"stop\": {\n";
    ss << "    \"ack_status\": " << stopAckStatus << ",\n";
    ss << "    \"ack_frame\": " << stopAckFrame << "\n";
    ss << "  },\n";
    ss << "  \"generated_events\": " << generatedEvents << ",\n";
    ss << "  \"planner_note_offs\": " << plannerNoteOffs << ",\n";
    ss << "  \"expected_scheduled_events\": " << expectedScheduledEvents << ",\n";
    ss << "  \"committed_events\": " << committedEvents << ",\n";
    ss << "  \"engine_processed_events\": " << engineProcessedEvents << ",\n";
    ss << "  \"dropped_scheduled_events\": " << droppedScheduledEvents << ",\n";
    ss << "  \"deferred_scheduled_events\": " << deferredScheduledEvents << ",\n";
    ss << "  \"scheduler_backpressure\": " << schedulerBackpressure << ",\n";
    ss << "  \"orphan_note_recovered\": " << orphanNoteRecoveredCount << ",\n";
    ss << "  \"max_corpo_overlap\": " << maxCorpoOverlap << ",\n";
    ss << "  \"winning_candidate_a\": " << winningCandidateIdA << ",\n";
    ss << "  \"winning_score_a\": " << winningScoreA << ",\n";
    ss << "  \"winning_candidate_b\": " << winningCandidateIdB << ",\n";
    ss << "  \"winning_score_b\": " << winningScoreB << ",\n";
    ss << "  \"section_a_bar1_rms_dbfs\": " << safeJsonFloat(sectionARmsDbfs) << ",\n";
    ss << "  \"section_b_bar1_rms_dbfs\": " << safeJsonFloat(sectionBRmsDbfs) << ",\n";
    ss << "  \"dropped_frames\": " << droppedFrames << ",\n";
    ss << "  \"peak_amplitude\": " << safeJsonFloat(peakAmplitude) << ",\n";
    ss << "  \"true_peak_dbtp\": " << safeJsonFloat(truePeakDbtp) << ",\n";
    ss << "  \"clipped_samples\": " << clippedSamples << ",\n";
    ss << "  \"checks\": {\n";
    ss << "    \"determinism_passed\": " << (determinismPassed ? "true" : "false") << ",\n";
    ss << "    \"stream_isolation_passed\": " << (streamIsolationPassed ? "true" : "false") << ",\n";
    ss << "    \"motif_invariance_passed\": " << (motifInvariancePassed ? "true" : "false") << ",\n";
    ss << "    \"triad_budget_passed\": " << (triadBudgetPassed ? "true" : "false") << ",\n";
    ss << "    \"fallback_passed\": " << (fallbackPassed ? "true" : "false") << ",\n";
    ss << "    \"scheduling_accounting_passed\": " << (schedulingAccountingPassed ? "true" : "false") << ",\n";
    ss << "    \"note_off_pairing_passed\": " << (noteOffPairingPassed ? "true" : "false") << ",\n";
    ss << "    \"tempo_ack_passed\": " << (tempoAckPassed ? "true" : "false") << ",\n";
    ss << "    \"non_silent_audio_passed\": " << (nonSilentAudioPassed ? "true" : "false") << ",\n";
    ss << "    \"true_peak_passed\": " << (truePeakPassed ? "true" : "false") << ",\n";
    ss << "    \"zero_clipping_passed\": " << (clippedSamples == 0 ? "true" : "false") << ",\n";
    ss << "    \"dsp_anomalies_passed\": " << (dspAnomaliesPassed ? "true" : "false") << "\n";
    ss << "  },\n";
    ss << "  \"wav_write_success\": " << (wavWriteSuccess ? "true" : "false") << ",\n";
    ss << "  \"manifest_write_success\": " << (manifestWriteSuccess ? "true" : "false") << ",\n";
    ss << "  \"artifacts\": {\n";
    ss << "    \"wav_file\": \"" << escapeJsonString(wavFilePath) << "\",\n";
    ss << "    \"manifest_file\": \"" << escapeJsonString(manifestFilePath) << "\"\n";
    ss << "  }";
    if (!failureReason.empty()) {
        ss << ",\n  \"failure_reason\": \"" << escapeJsonString(failureReason) << "\"";
    }
    ss << "\n}\n";
    return ss.str();
}

PlannerProbeResult runPlannerProbeV1Offline(const std::string& outputDir, bool writeFiles) {
    PlannerProbeResult result{};
    result.presentationQuality = "Estimated";

    constexpr std::uint32_t kSampleRate = 48000;
    const std::uint32_t simulationHz = core::FixedStepConfig{}.simulationHz;
    const std::uint32_t framesPerTick = kSampleRate / simulationHz; // 400 (exact at 48 kHz / 120 Hz)
    const std::uint64_t lookahead = AudioSchedulerConfig{}.lookaheadFrames; // nominal 7200 (150 ms)

    const FormPlan form = MusicPlanner::makePlannerProbeV1Form();
    const auto clockOpt = MusicPlanner::makePlannerProbeV1Clock();
    if (!clockOpt.has_value()) {
        result.failureReason = "Failed to build the planner_probe_v1 MusicClock (A tempo map + B at frameAtBar(4))";
        return result;
    }
    const MusicClock& clock = *clockOpt;
    const std::array<PhrasePlan, 2> phrases = MusicPlanner::makePlannerProbeV1Phrases(clock);
    const std::array<std::uint64_t, 2> phraseEpochs{1, 2};
    result.tempoChangeFrame = clock.frameAtBar(form.sections[1].startBar); // 384000
    result.contentEndFrame = clock.frameAtBar(8);                           // 713142

    // --- Planning runs (Coordinator thread model) ---
    struct PlanRun {
        std::vector<std::vector<MusicIntentEvent>> intents;
        std::vector<PhraseDecisionTrace> traces;
        std::vector<PlannedNoteOff> noteOffs;
    };
    const auto runPlan = [&](std::uint64_t seed, AudioScheduler* sched) {
        MusicPlanner planner({.runSeed = seed, .candidateCount = 6, .sampleRate = kSampleRate,
                              .lookaheadFrames = lookahead});
        planner.setFormPlan(form);
        PlanRun run{};
        for (std::size_t p = 0; p < phrases.size(); ++p) {
            std::vector<MusicIntentEvent> out;
            planner.planPhrase(phrases[p], clock, phrases[p].startFrame, phraseEpochs[p], out, sched);
            run.intents.push_back(std::move(out));
        }
        run.traces = planner.decisionTraces();
        run.noteOffs = planner.plannedNoteOffs();
        return run;
    };

    AudioScheduler scheduler({.lookaheadFrames = lookahead, .sampleRate = kSampleRate});
    const PlanRun mainRun = runPlan(1337ULL, &scheduler);
    std::vector<MusicIntentEvent> allIntents;
    for (const auto& v : mainRun.intents) allIntents.insert(allIntents.end(), v.begin(), v.end());
    result.generatedEvents = static_cast<std::uint32_t>(allIntents.size());
    result.plannerNoteOffs = static_cast<std::uint32_t>(mainRun.noteOffs.size());
    result.expectedScheduledEvents = 2U * result.generatedEvents;
    if (mainRun.traces.size() == 2) {
        result.winningCandidateIdA = mainRun.traces[0].winningCandidateId;
        result.winningScoreA = mainRun.traces[0].winningScore;
        result.winningCandidateIdB = mainRun.traces[1].winningCandidateId;
        result.winningScoreB = mainRun.traces[1].winningScore;
    }

    // 1. Determinism: full canonical comparison of an independent run (no scheduler -> also proves that the
    //    planner state does not depend on the scheduler pointer)
    {
        const PlanRun rerun = runPlan(1337ULL, nullptr);
        result.determinismPassed = !allIntents.empty() &&
                                   rerun.intents == mainRun.intents &&
                                   rerun.traces == mainRun.traces &&
                                   rerun.noteOffs == mainRun.noteOffs;
    }

    // 2. Seed isolation (metamorphic): a different runSeed may only change tie-break decisions among
    //    equal-score candidates; scores and non-tied decisions (with their event content) stay identical.
    //    Domain separation: the MusicForm tie-break key is reproducible from the trace and no other StreamId
    //    with the same decision/sample coordinates yields the same value.
    {
        const PlanRun alt = runPlan(0xDEADBEEFCAFEULL, nullptr);
        bool ok = alt.traces.size() == mainRun.traces.size() && mainRun.traces.size() == phrases.size();
        bool comparable = true; // false once an (allowed) tie-break divergence changes downstream state
        for (std::size_t p = 0; ok && p < mainRun.traces.size(); ++p) {
            const auto& a = mainRun.traces[p];
            const auto& b = alt.traces[p];
            if (!comparable) break;
            for (std::size_t c = 0; c < a.candidatesGenerated; ++c) {
                if (a.candidates[c].score != b.candidates[c].score ||
                    a.candidates[c].violation != b.candidates[c].violation) ok = false;
            }
            if (a.winningCandidateId != b.winningCandidateId) {
                // Allowed only when the decision was a tie and the alternative winner shares the top score
                if (a.tiedTopCandidates < 2 || a.winningScore != b.winningScore) ok = false;
                comparable = false;
            } else if (mainRun.intents[p] != alt.intents[p]) {
                ok = false;
            }
        }
        for (const auto& t : mainRun.traces) {
            for (std::size_t c = 0; c < t.candidatesGenerated; ++c) {
                const auto& entry = t.candidates[c];
                if (entry.violation != HardConstraintViolation::None) continue;
                const auto sample = static_cast<std::uint32_t>(entry.candidateId & 0xFFFFFFFFULL);
                const core::RandomKey formKey{t.runSeed, static_cast<std::uint32_t>(core::StreamId::MusicForm),
                                              t.tieBreakDecisionId, sample};
                if (core::randomU64(formKey, t.policyVersion) != entry.tieBreakHash) ok = false;
                for (const auto other : {core::StreamId::Harmony, core::StreamId::Rhythm, core::StreamId::Director,
                                         core::StreamId::Pattern, core::StreamId::Adaptation}) {
                    const core::RandomKey otherKey{t.runSeed, static_cast<std::uint32_t>(other), t.tieBreakDecisionId, sample};
                    if (core::randomU64(otherKey, t.policyVersion) == entry.tieBreakHash) ok = false;
                }
            }
            if (t.tieBreakDecisionId == t.decisionId) ok = false; // dedicated decision slot
        }
        result.streamIsolationPassed = ok;
    }

    // 3. Motif invariance from the decision trace + realized Vettore notes on the clock grid
    {
        bool ok = mainRun.traces.size() == phrases.size();
        for (std::size_t p = 0; ok && p < mainRun.traces.size(); ++p) {
            const auto& t = mainRun.traces[p];
            const auto& phrase = phrases[p];
            if (t.fallbackTriggered || !(t.baseMotif == form.sections[phrase.sectionIndex].sectionMotif)) {
                ok = false;
                break;
            }
            if (!verifyMotifTransformInvariants(t.baseMotif, t.winningMotif, t.winningTransformKind, t.winningTransformParam)) {
                ok = false;
            }
            for (std::size_t i = 0; i < t.candidatesGenerated; ++i) {
                for (std::size_t j = i + 1; j < t.candidatesGenerated; ++j) {
                    if (t.candidates[i].motifId == t.candidates[j].motifId) ok = false; // distinct derived ids
                }
            }
            const auto& tm = form.sections[phrase.sectionIndex].tempoMap;
            for (std::uint32_t b = 0; b < phrase.barCount; ++b) {
                const std::uint64_t barBeat = static_cast<std::uint64_t>(phrase.startBar + b) * tm.beatsPerBar;
                for (std::size_t m = 0; m < t.winningMotif.noteCount; ++m) {
                    const auto& n = t.winningMotif.notes[m];
                    const std::uint64_t frame = clock.frameAtBeatStep(barBeat, n.stepOffset, tm.subdivisionsPerBeat);
                    const float hz = midiNoteToPitchHz(degreeToMidiNote(phrase.scale,
                        n.scaleDegree + 7 * t.winningMotif.registerOctaveOffset, n.chromaticOffset));
                    const bool found = std::any_of(mainRun.intents[p].begin(), mainRun.intents[p].end(),
                        [&](const MusicIntentEvent& e) {
                            return e.role == MusicRole::Vettore && e.sampleFrame == frame && e.pitchHz == hz;
                        });
                    if (!found) ok = false;
                }
            }
        }
        result.motifInvariancePassed = ok;
    }

    // 4. NoteOn/NoteOff pairing and Corpo budget on the emitted intents (+ planner NoteOffs)
    {
        struct Edge {
            std::uint64_t frame;
            int delta; // -1 NoteOff, +1 NoteOn (offs sort first at equal frames)
        };
        std::vector<Edge> corpoEdges;
        bool pairingOk = true;
        for (const auto& e : allIntents) {
            std::uint64_t offFrame = 0;
            std::size_t matches = 0;
            for (const auto& off : mainRun.noteOffs) {
                if (off.eventId == e.eventId && off.role == e.role) {
                    ++matches;
                    offFrame = off.sampleFrame;
                }
            }
            if (e.durationFrames > 0) {
                if (matches != 0) pairingOk = false; // scheduler NoteOff from duration only
                offFrame = e.sampleFrame + e.durationFrames;
            } else if (matches != 1) {
                pairingOk = false;
            }
            if (offFrame <= e.sampleFrame) pairingOk = false;
            if (e.role == MusicRole::Corpo) {
                corpoEdges.push_back({e.sampleFrame, +1});
                corpoEdges.push_back({offFrame, -1});
            }
        }
        if (mainRun.noteOffs.size() != static_cast<std::size_t>(std::count_if(allIntents.begin(), allIntents.end(),
                [](const MusicIntentEvent& e) { return e.durationFrames == 0; }))) {
            pairingOk = false;
        }
        result.noteOffPairingPassed = pairingOk && !allIntents.empty();

        std::sort(corpoEdges.begin(), corpoEdges.end(), [](const Edge& a, const Edge& b) {
            return a.frame != b.frame ? a.frame < b.frame : a.delta < b.delta;
        });
        int active = 0;
        int maxActive = 0;
        for (const auto& edge : corpoEdges) {
            active += edge.delta;
            maxActive = std::max(maxActive, active);
        }
        result.maxCorpoOverlap = static_cast<std::uint32_t>(maxActive);
        result.triadBudgetPassed = !corpoEdges.empty() && maxActive <= 2 && active == 0;
    }

    // 5. Fallback ladder under constructed conditions (fresh planners, same clock)
    {
        const PhrasePlan& pA = phrases[0];
        const auto planOnce = [&](const PhrasePlan& plan, std::uint64_t horizon, std::vector<MusicIntentEvent>& out) {
            MusicPlanner fb({.runSeed = 1337ULL, .candidateCount = 6, .sampleRate = kSampleRate, .lookaheadFrames = lookahead});
            fb.setFormPlan(form);
            fb.planPhrase(plan, clock, horizon, 1, out, nullptr);
            return fb.decisionTraces().back();
        };
        // L1: horizon one step after bar 2 start -> regular candidates violate the horizon
        const std::uint64_t hMid = clock.frameAtBeatStep(static_cast<std::uint64_t>(pA.startBar + 2) * 4, 1, 4);
        std::vector<MusicIntentEvent> o1;
        const auto t1 = planOnce(pA, hMid, o1);
        bool l1 = t1.fallbackKind == FallbackKind::RarefiedMotif && !o1.empty();
        for (const auto& e : o1) l1 = l1 && e.sampleFrame >= hMid;
        // L2: horizon after the last anchor (step 8) and downbeat of the last bar -> L1 empty
        const std::uint64_t hLate = clock.frameAtBeatStep(static_cast<std::uint64_t>(pA.startBar + 3) * 4, 9, 4);
        std::vector<MusicIntentEvent> o2;
        const auto t2 = planOnce(pA, hLate, o2);
        const bool l2 = t2.fallbackKind == FallbackKind::PadSustainOnly && o2.size() == 2 &&
                        o2[0].role == MusicRole::Corpo && o2[0].sampleFrame == hLate &&
                        o2[0].sampleFrame + o2[0].durationFrames == pA.endFrame;
        // L3a: horizon at the phrase end
        std::vector<MusicIntentEvent> o3;
        const auto t3 = planOnce(pA, pA.endFrame, o3);
        const bool l3a = t3.fallbackKind == FallbackKind::SilentRecovery && o3.empty();
        // L3b: phrase plan inconsistent with the clock -> StructuralInvariantViolation -> silent
        PhrasePlan bad = pA;
        bad.endFrame += 1;
        std::vector<MusicIntentEvent> o4;
        const auto t4 = planOnce(bad, 0, o4);
        const bool l3b = t4.fallbackKind == FallbackKind::SilentRecovery && o4.empty() &&
                         t4.planViolation == HardConstraintViolation::StructuralInvariantViolation;
        result.fallbackPassed = l1 && l2 && l3a && l3b;
    }

    // --- Offline render (120 Hz simulation ticks) ---
    AudioEngine engine;
    if (!engine.initOffline({kSampleRate, 2, 220.0f})) {
        result.passed = false;
        result.failureReason = "Failed to initialize AudioEngine in offline mode";
        return result;
    }
    AudioProbe probe(AudioProbe::kDefaultCapacityFrames, 2, engine.sampleRate());
    engine.attachAudioProbe(&probe);

    constexpr std::uint64_t kReleaseMarginFrames = 24000; // 0.5 s for release / delay tails after the last NoteOff
    constexpr std::uint64_t kPostStopFrames = 4000;       // 10 ticks rendered after Stop
    result.stopFrame = ((result.contentEndFrame + kReleaseMarginFrames + framesPerTick - 1) / framesPerTick) * framesPerTick;
    result.requestedSampleFrames = result.stopFrame + kPostStopFrames;
    result.simulationTicks = result.requestedSampleFrames / framesPerTick;

    engine.sendCommand(AudioCommand{.commandId = 1, .type = AudioCommandType::Start, .targetSampleFrame = 0});

    std::vector<float> recordedStereo;
    recordedStereo.reserve(static_cast<std::size_t>(result.requestedSampleFrames) * 2);
    std::vector<float> readChunk(static_cast<std::size_t>(framesPerTick) * 2);
    std::vector<float> tickBlock(static_cast<std::size_t>(framesPerTick) * 2);

    for (std::uint64_t tick = 0; tick < result.simulationTicks; ++tick) {
        const std::uint64_t currentFrame = tick * framesPerTick;

        if (currentFrame == result.tempoChangeFrame) {
            engine.sendCommand(AudioCommand::makeSetTempoMap(2, form.sections[1].tempoMap, result.tempoChangeFrame));
        }
        if (currentFrame == result.stopFrame) {
            engine.sendCommand(AudioCommand{.commandId = 3, .type = AudioCommandType::Stop, .targetSampleFrame = result.stopFrame});
        }

        // The scheduler adds its own lookahead to the render cursor
        scheduler.dispatchDueEvents(currentFrame, engine);

        engine.processBlock(tickBlock.data(), framesPerTick);

        const std::uint32_t framesRead = probe.readFrames(readChunk.data(), framesPerTick);
        if (framesRead > 0) {
            recordedStereo.insert(recordedStereo.end(), readChunk.begin(), readChunk.begin() + (framesRead * 2));
        }

        AudioCommandAck ack{};
        while (engine.pollAck(ack)) {
            if (ack.commandId == 2) {
                result.tempoAckStatus = static_cast<std::uint32_t>(ack.status);
                result.tempoAckFrame = ack.sampleFrame;
                result.tempoAckEpoch = ack.transportEpoch;
            } else if (ack.commandId == 3) {
                result.stopAckStatus = static_cast<std::uint32_t>(ack.status);
                result.stopAckFrame = ack.sampleFrame;
            }
        }
    }

    result.committedEvents = static_cast<std::uint32_t>(scheduler.totalDispatchedEvents());
    result.schedulerBackpressure = scheduler.backpressureDrops();
    result.totalCapturedFrames = recordedStereo.size() / 2;
    result.droppedFrames = probe.droppedFrames();
    (void)engine.updateTelemetry();
    result.transportEpoch = engine.telemetry().transportEpoch;
    result.orphanNoteRecoveredCount = engine.telemetry().orphanNoteRecoveredCount;
    result.droppedScheduledEvents = engine.droppedScheduledEvents();
    result.deferredScheduledEvents = engine.deferredScheduledEvents();
    result.engineProcessedEvents = engine.totalScheduledEventsProcessed();
    const std::uint32_t finalDspAnomalies = engine.telemetry().dspAnomalyCount;

    engine.detachAudioProbe();
    engine.shutdown();

    result.schedulingAccountingPassed =
        result.expectedScheduledEvents > 0 &&
        result.committedEvents == result.expectedScheduledEvents &&
        result.engineProcessedEvents == result.expectedScheduledEvents &&
        scheduler.stagedEventCount() == 0 &&
        result.droppedScheduledEvents == 0 &&
        result.deferredScheduledEvents == 0 &&
        result.orphanNoteRecoveredCount == 0;
    result.tempoAckPassed =
        result.tempoAckStatus == static_cast<std::uint32_t>(AudioCommandStatus::Applied) &&
        result.tempoAckFrame == result.tempoChangeFrame &&
        result.tempoChangeFrame == 384000ULL &&
        result.tempoAckEpoch == 2;

    if (result.totalCapturedFrames < result.requestedSampleFrames) {
        result.passed = false;
        result.failureReason = "Incomplete capture: expected " +
                               std::to_string(result.requestedSampleFrames) + " frames, got " +
                               std::to_string(result.totalCapturedFrames);
        return result;
    }

    // Peak and clipping
    float maxAmp = 0.0f;
    std::uint32_t clipCount = 0;
    for (float s : recordedStereo) {
        const float a = std::abs(s);
        if (a > maxAmp) maxAmp = a;
        if (a >= (32767.0f / 32768.0f)) {
            ++clipCount;
        }
    }
    result.peakAmplitude = maxAmp;
    result.clippedSamples = clipCount;

    // True Peak
    result.truePeakDbtp = analysis::estimateTruePeakDbtp(recordedStereo.data(), result.totalCapturedFrames, 2);
    result.truePeakPassed = (result.truePeakDbtp <= -1.0f);

    // DSP Anomalies
    result.dspAnomaliesPassed = (finalDspAnomalies == 0);
    if (result.dspAnomaliesPassed) {
        for (float s : recordedStereo) {
            if (!std::isfinite(s)) {
                result.dspAnomaliesPassed = false;
                break;
            }
        }
    }

    // Non-silent audio: RMS over bar 1 of section A [frameAtBar(1), frameAtBar(2)) and bar 1 of section B
    // [frameAtBar(5), frameAtBar(6)), both derived from the planning clock
    {
        const auto windowRms = [&](std::uint64_t bar) {
            const std::uint64_t from = clock.frameAtBar(bar);
            const std::uint64_t to = clock.frameAtBar(bar + 1);
            return analysis::computeRmsDbfs(&recordedStereo[static_cast<std::size_t>(from) * 2],
                                            static_cast<std::size_t>(to - from) * 2);
        };
        result.sectionARmsDbfs = windowRms(form.sections[0].startBar + 1);
        result.sectionBRmsDbfs = windowRms(form.sections[1].startBar + 1);
        result.nonSilentAudioPassed = (result.sectionARmsDbfs > -40.0f) && (result.sectionBRmsDbfs > -40.0f);
    }

    result.passed = (result.droppedFrames == 0) &&
                    result.determinismPassed &&
                    result.streamIsolationPassed &&
                    result.motifInvariancePassed &&
                    result.triadBudgetPassed &&
                    result.noteOffPairingPassed &&
                    result.fallbackPassed &&
                    result.schedulingAccountingPassed &&
                    result.tempoAckPassed &&
                    result.nonSilentAudioPassed &&
                    result.truePeakPassed &&
                    result.dspAnomaliesPassed &&
                    (result.clippedSamples == 0);

    if (!result.passed) {
        std::ostringstream err;
        if (result.droppedFrames > 0) err << "dropped frames in probe; ";
        if (!result.determinismPassed) err << "determinism check failed; ";
        if (!result.streamIsolationPassed) err << "seed/stream isolation check failed; ";
        if (!result.motifInvariancePassed) err << "motif invariance check failed; ";
        if (!result.triadBudgetPassed) err << "Corpo budget check failed (max overlap " << result.maxCorpoOverlap << "); ";
        if (!result.noteOffPairingPassed) err << "NoteOn/NoteOff pairing failed; ";
        if (!result.fallbackPassed) err << "fallback ladder check failed; ";
        if (!result.schedulingAccountingPassed) {
            err << "scheduling accounting failed (expected " << result.expectedScheduledEvents
                << ", committed " << result.committedEvents << ", processed " << result.engineProcessedEvents
                << ", dropped " << result.droppedScheduledEvents << ", deferred " << result.deferredScheduledEvents
                << ", orphans " << result.orphanNoteRecoveredCount << "); ";
        }
        if (!result.tempoAckPassed) {
            err << "SetTempoMap ack mismatch (status " << result.tempoAckStatus << ", frame " << result.tempoAckFrame
                << ", epoch " << result.tempoAckEpoch << "); ";
        }
        if (!result.nonSilentAudioPassed) err << "non-silent audio check failed: A=" << result.sectionARmsDbfs << " dBFS, B=" << result.sectionBRmsDbfs << " dBFS; ";
        if (!result.truePeakPassed) err << "true peak ceiling violated (" << result.truePeakDbtp << " > -1.0 dBTP); ";
        if (result.clippedSamples > 0) err << "clipped samples: " << result.clippedSamples << "; ";
        if (!result.dspAnomaliesPassed) err << "DSP anomalies or non-finite values detected; ";
        result.failureReason = err.str();
    }

    if (writeFiles) {
        std::filesystem::path dir(outputDir);
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        result.wavFilePath = (dir / "planner_probe_v1.wav").lexically_normal().string();
        result.manifestFilePath = (dir / "planner_probe_v1_manifest.json").lexically_normal().string();

        const bool wavOk = analysis::writeWavStereo16(
            result.wavFilePath,
            recordedStereo.data(),
            result.totalCapturedFrames,
            kSampleRate
        );
        result.wavWriteSuccess = wavOk;
        if (!wavOk) {
            result.passed = false;
            result.failureReason += "[WAV write failed] ";
        }

        std::ofstream mf(result.manifestFilePath);
        if (!mf.is_open()) {
            result.passed = false;
            result.manifestWriteSuccess = false;
            result.failureReason += "[Manifest write failed] ";
        } else {
            result.manifestWriteSuccess = true;
            mf << result.toJson();
            if (!mf.good()) {
                result.passed = false;
                result.manifestWriteSuccess = false;
                result.failureReason += "[Manifest write failed] ";
            }
        }
    }

    return result;
}


// --- M2.4 Inc-4: fixture_probe_v1 and profile_probe_v1 (ADR-0005 D-M2-08..11) ---

namespace {

constexpr std::uint32_t kInc4SampleRate = 48000;
constexpr std::uint64_t kFixtureSeed = 1337ULL;
constexpr std::uint64_t kFixturePostStopFrames = 48000; // 1.0 s captured after the Stop frame

std::uint64_t fixtureEpochForSection(std::uint32_t sectionIndex) noexcept {
    return static_cast<std::uint64_t>(sectionIndex) + 1; // A = 1, B = 2, A' = 3 (expected epoch = current + 1)
}

void fnvMix(std::uint64_t& h, std::uint64_t v) noexcept {
    h ^= v;
    h *= 1099511628211ULL;
}

// Decision-trace hash of fixture_probe_v1 (field set unchanged since commit 9c61f88).
std::uint64_t fixtureTraceHash(const std::vector<PhraseDecisionTrace>& traces) noexcept {
    std::uint64_t h = 14695981039346656037ULL;
    for (const auto& t : traces) {
        fnvMix(h, t.decisionId);
        fnvMix(h, t.winningCandidateId);
        fnvMix(h, static_cast<std::uint64_t>(t.winningScore));
        fnvMix(h, static_cast<std::uint64_t>(t.winningTransformKind));
        fnvMix(h, static_cast<std::uint64_t>(t.winningTransformParam));
        fnvMix(h, static_cast<std::uint64_t>(t.finalEventCount));
        fnvMix(h, static_cast<std::uint64_t>(t.plannerNoteOffCount));
        for (std::size_t c = 0; c < t.candidatesGenerated; ++c) {
            fnvMix(h, t.candidates[c].candidateId);
            fnvMix(h, static_cast<std::uint64_t>(t.candidates[c].score));
        }
    }
    return h;
}

// FNV-1a 64 over the IEEE-754 bit patterns of the captured PCM (artifact content hash, D-M2-11).
std::uint64_t pcmContentHash(const std::vector<float>& samples) noexcept {
    std::uint64_t h = 14695981039346656037ULL;
    for (float s : samples) {
        std::uint32_t bits = 0;
        std::memcpy(&bits, &s, sizeof(bits));
        for (int b = 0; b < 4; ++b) {
            h ^= (bits >> (8 * b)) & 0xFFU;
            h *= 1099511628211ULL;
        }
    }
    return h;
}

// Just-in-time Coordinator of fixture-m2-4-v1 (ADR-0005 D-M2-09), shared by fixture_probe_v1 and workload F.
class FixtureCoordinator {
public:
    static constexpr std::uint64_t kStartCommandId = 1;
    static constexpr std::uint64_t kTempoBCommandId = 2;
    static constexpr std::uint64_t kTempoAPrimeCommandId = 3;
    static constexpr std::uint64_t kStopCommandId = 4;

    FixtureCoordinator(const MusicClock& clock, const FormPlan& form, const std::array<PhrasePlan, 24>& phrases)
        : clock_(clock), form_(form), phrases_(phrases),
          planner_({.runSeed = kFixtureSeed, .candidateCount = 8, .sampleRate = kInc4SampleRate,
                    .lookaheadFrames = AudioSchedulerConfig{}.lookaheadFrames}) {
        planner_.setFormPlan(form_);
        for (std::size_t i = 0; i < 2; ++i) {
            const std::uint32_t boundaryBar = form_.sections[i + 1].startBar;
            tempoBoundary_[i] = clock_.frameAtBar(boundaryBar);
            tempoSendFrame_[i] = clock_.frameAtBar(boundaryBar - 1); // at least one bar ahead
        }
        const SectionPlan& last = form_.sections[form_.sectionCount - 1];
        contentEndFrame_ = clock_.frameAtBar(static_cast<std::uint64_t>(last.startBar) + last.barCount);
        stopSendFrame_ = clock_.frameAtBar(static_cast<std::uint64_t>(last.startBar) + last.barCount - 1);
    }

    void start(AudioEngine& engine) {
        planNext(0); // phrase 0
        planNext(0); // phrase 1 (one phrase of anticipation)
        engine.sendCommand(AudioCommand{.commandId = kStartCommandId, .type = AudioCommandType::Start, .targetSampleFrame = 0});
    }

    // Coordinator work before rendering the block that starts at currentFrame.
    void beforeBlock(std::uint64_t currentFrame, AudioEngine& engine) {
        for (std::size_t i = 0; i < 2; ++i) {
            if (!tempoSent_[i] && currentFrame >= tempoSendFrame_[i]) {
                engine.sendCommand(AudioCommand::makeSetTempoMap(kTempoBCommandId + i, form_.sections[i + 1].tempoMap,
                                                                 tempoBoundary_[i]));
                tempoSent_[i] = true;
            }
        }
        if (!stopSent_ && currentFrame >= stopSendFrame_) {
            engine.sendCommand(AudioCommand{.commandId = kStopCommandId, .type = AudioCommandType::Stop,
                                            .targetSampleFrame = contentEndFrame_});
            stopSent_ = true;
        }
        // Phrase N+1 is planned when the planning cursor enters phrase N
        while (!stopped_ && next_ < phrases_.size() && currentFrame >= phrases_[next_ - 1].startFrame) {
            planNext(core::saturatingAdd(currentFrame, scheduler_.lookaheadFrames()));
        }
        if (!stopped_) {
            (void)scheduler_.dispatchDueEvents(currentFrame, engine);
        }
    }

    void drainAcks(AudioEngine& engine) {
        AudioCommandAck ack{};
        while (engine.pollAck(ack)) {
            if (ack.commandId == kStartCommandId) {
                ackStart_ = ack;
            } else if (ack.commandId == kTempoBCommandId || ack.commandId == kTempoAPrimeCommandId) {
                const std::size_t i = static_cast<std::size_t>(ack.commandId - kTempoBCommandId);
                ackTempo_[i] = ack;
                if (!tempoAckValid(i)) {
                    // Fail-closed (D-M2-09): revoke staged events from the planned boundary on
                    revokedEvents_ += scheduler_.revokeFromFrame(tempoBoundary_[i]);
                }
            } else if (ack.commandId == kStopCommandId) {
                ackStop_ = ack;
                if (ack.status == AudioCommandStatus::Applied && !stopped_) {
                    // Stop: empty the staging and reset the planner, simulated tracker included (D-M2-09)
                    stagedAtStop_ = scheduler_.stagedEventCount();
                    tracesAtStop_ = planner_.decisionTraces();
                    noteOffsAtStop_ = planner_.plannedNoteOffs();
                    scheduler_.clear();
                    planner_.reset(kFixtureSeed);
                    stopped_ = true;
                }
            }
        }
    }

    [[nodiscard]] bool tempoAckValid(std::size_t i) const noexcept {
        return ackTempo_[i].status == AudioCommandStatus::Applied && ackTempo_[i].sampleFrame == tempoBoundary_[i] &&
               ackTempo_[i].transportEpoch == fixtureEpochForSection(static_cast<std::uint32_t>(i + 1));
    }
    [[nodiscard]] bool startAckValid() const noexcept {
        return ackStart_.status == AudioCommandStatus::Applied && ackStart_.transportEpoch == 1;
    }
    [[nodiscard]] bool stopAckValid() const noexcept {
        return ackStop_.status == AudioCommandStatus::Applied && ackStop_.sampleFrame == contentEndFrame_;
    }

    [[nodiscard]] std::uint64_t contentEndFrame() const noexcept { return contentEndFrame_; }
    [[nodiscard]] std::uint64_t tempoBoundary(std::size_t i) const noexcept { return tempoBoundary_[i]; }
    [[nodiscard]] const AudioCommandAck& tempoAck(std::size_t i) const noexcept { return ackTempo_[i]; }
    [[nodiscard]] const AudioCommandAck& startAck() const noexcept { return ackStart_; }
    [[nodiscard]] const AudioCommandAck& stopAck() const noexcept { return ackStop_; }
    [[nodiscard]] std::size_t phrasesPlanned() const noexcept { return next_; }
    [[nodiscard]] bool stopped() const noexcept { return stopped_; }
    [[nodiscard]] std::size_t stagedAtStop() const noexcept { return stagedAtStop_; }
    [[nodiscard]] std::size_t revokedEvents() const noexcept { return revokedEvents_; }
    [[nodiscard]] std::uint64_t backpressureDrops() const noexcept { return scheduler_.backpressureDrops(); }
    [[nodiscard]] std::uint64_t lookaheadFrames() const noexcept { return scheduler_.lookaheadFrames(); }
    [[nodiscard]] const std::vector<PhraseDecisionTrace>& tracesAtStop() const noexcept { return tracesAtStop_; }
    [[nodiscard]] const std::vector<PlannedNoteOff>& noteOffsAtStop() const noexcept { return noteOffsAtStop_; }

private:
    void planNext(std::uint64_t horizon) {
        if (next_ >= phrases_.size()) return;
        const PhrasePlan& p = phrases_[next_];
        std::vector<MusicIntentEvent> out;
        planner_.planPhrase(p, clock_, horizon, fixtureEpochForSection(p.sectionIndex), out, &scheduler_);
        ++next_;
    }

    const MusicClock& clock_;
    const FormPlan& form_;
    const std::array<PhrasePlan, 24>& phrases_;
    MusicPlanner planner_;
    AudioScheduler scheduler_{};
    std::size_t next_{0};
    std::array<std::uint64_t, 2> tempoBoundary_{};
    std::array<std::uint64_t, 2> tempoSendFrame_{};
    std::array<bool, 2> tempoSent_{};
    std::uint64_t contentEndFrame_{0};
    std::uint64_t stopSendFrame_{0};
    bool stopSent_{false};
    bool stopped_{false};
    AudioCommandAck ackStart_{};
    std::array<AudioCommandAck, 2> ackTempo_{};
    AudioCommandAck ackStop_{};
    std::size_t stagedAtStop_{0};
    std::size_t revokedEvents_{0};
    std::vector<PhraseDecisionTrace> tracesAtStop_{};
    std::vector<PlannedNoteOff> noteOffsAtStop_{};
};

std::string cpuBrandString() {
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
    std::array<int, 4> regs{};
    __cpuid(regs.data(), static_cast<int>(0x80000000U));
    if (static_cast<unsigned>(regs[0]) < 0x80000004U) return "unknown";
    std::array<char, 49> brand{};
    for (unsigned leaf = 0; leaf < 3; ++leaf) {
        __cpuid(regs.data(), static_cast<int>(0x80000002U + leaf));
        std::memcpy(brand.data() + leaf * 16, regs.data(), 16);
    }
    std::string s(brand.data());
    const auto first = s.find_first_not_of(' ');
    if (first == std::string::npos) return "unknown";
    return s.substr(first, s.find_last_not_of(' ') - first + 1);
#else
    return "unknown";
#endif
}

std::string osVersionString() {
#if defined(_WIN32)
    using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
    const HMODULE ntdll = ::GetModuleHandleW(L"ntdll.dll");
    if (ntdll != nullptr) {
        const auto fn = reinterpret_cast<RtlGetVersionFn>(reinterpret_cast<void*>(::GetProcAddress(ntdll, "RtlGetVersion")));
        RTL_OSVERSIONINFOW info{};
        info.dwOSVersionInfoSize = sizeof(info);
        if (fn != nullptr && fn(&info) == 0) {
            return "Windows " + std::to_string(info.dwMajorVersion) + "." + std::to_string(info.dwMinorVersion) +
                   "." + std::to_string(info.dwBuildNumber);
        }
    }
    return "Windows (unknown version)";
#else
    return "unknown";
#endif
}

std::string compilerString() {
#if defined(_MSC_FULL_VER)
    return "MSVC " + std::to_string(_MSC_FULL_VER);
#elif defined(__clang__)
    return std::string("clang ") + __clang_version__;
#elif defined(__GNUC__)
    return std::string("gcc ") + __VERSION__;
#else
    return "unknown";
#endif
}

const char* workloadName(ProfileWorkload w) noexcept {
    return (w == ProfileWorkload::W) ? "W" : "F";
}

} // namespace

std::string FixtureProbeResult::toJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"manifest_version\": \"1.1\",\n";
    ss << "  \"scenario\": \"fixture_probe_v1\",\n";
    ss << "  \"build_id\": \"" << escapeJsonString(buildId) << "\",\n";
    ss << "  \"verdict\": \"" << (passed ? "PASS" : "FAIL") << "\",\n";
    ss << "  \"run_seed\": " << kFixtureSeed << ",\n";
    ss << "  \"sample_rate\": " << kInc4SampleRate << ",\n";
    ss << "  \"lookahead_frames\": " << lookaheadFrames << ",\n";
    ss << "  \"content_end_frame\": " << contentEndFrame << ",\n";
    ss << "  \"final_transport_epoch\": " << transportEpoch << ",\n";
    ss << "  \"simulation_ticks\": " << simulationTicks << ",\n";
    ss << "  \"requested_sample_frames\": " << requestedSampleFrames << ",\n";
    ss << "  \"applied_sample_frames\": " << totalCapturedFrames << ",\n";
    ss << "  \"dropped_frames\": " << droppedFrames << ",\n";
    ss << "  \"presentation_quality\": \"" << escapeJsonString(presentationQuality) << "\",\n";
    ss << "  \"phrases_planned\": " << phrasesPlanned << ",\n";
    ss << "  \"generated_events\": " << generatedEvents << ",\n";
    ss << "  \"planner_note_offs\": " << plannerNoteOffs << ",\n";
    ss << "  \"engine_processed_events\": " << engineProcessedEvents << ",\n";
    ss << "  \"dropped_scheduled_events\": " << droppedScheduledEvents << ",\n";
    ss << "  \"deferred_scheduled_events\": " << deferredScheduledEvents << ",\n";
    ss << "  \"scheduler_backpressure_drops\": " << schedulerBackpressureDrops << ",\n";
    ss << "  \"revoked_staged_events\": " << revokedStagedEvents << ",\n";
    ss << "  \"staged_events_at_stop\": " << stagedEventsAtStop << ",\n";
    ss << "  \"orphan_note_recovered\": " << orphanNoteRecoveredCount << ",\n";
    ss << "  \"voice_drop_count\": " << voiceDropCount << ",\n";
    ss << "  \"transport_acks\": [\n";
    for (std::size_t i = 0; i < transportAcks.size(); ++i) {
        const auto& a = transportAcks[i];
        ss << "    {\"command_id\": " << a.commandId << ", \"status\": " << static_cast<int>(a.status)
           << ", \"sample_frame\": " << a.sampleFrame << ", \"transport_epoch\": " << a.transportEpoch << "}"
           << (i + 1 < transportAcks.size() ? ",\n" : "\n");
    }
    ss << "  ],\n";
    ss << "  \"sections\": [\n";
    for (std::size_t s = 0; s < 3; ++s) {
        ss << "    {\"section\": " << s << ", \"rms_dbfs\": " << safeJsonFloat(sectionRmsDbfs[s])
           << ", \"energy_max\": {\"pulso\": " << safeJsonFloat(roleEnergyMax[s][0])
           << ", \"corpo\": " << safeJsonFloat(roleEnergyMax[s][1]) << ", \"trama\": " << safeJsonFloat(roleEnergyMax[s][2])
           << ", \"vettore\": " << safeJsonFloat(roleEnergyMax[s][3]) << "}}" << (s < 2 ? ",\n" : "\n");
    }
    ss << "  ],\n";
    ss << "  \"peak_amplitude\": " << safeJsonFloat(peakAmplitude) << ",\n";
    ss << "  \"true_peak_dbtp\": " << safeJsonFloat(truePeakDbtp) << ",\n";
    ss << "  \"max_gain_reduction_db\": " << safeJsonFloat(maxGainReductionDb) << ",\n";
    ss << "  \"clipped_samples\": " << clippedSamples << ",\n";
    ss << "  \"decision_trace_hash\": " << decisionTraceHash << ",\n";
    ss << "  \"golden_decision_trace_hash\": " << kFixtureProbeV1GoldenTraceHash << ",\n";
    ss << "  \"pcm_content_hash\": " << pcmContentHash << ",\n";
    ss << "  \"checks\": {\n";
    ss << "    \"capture_complete_passed\": " << (captureCompletePassed ? "true" : "false") << ",\n";
    ss << "    \"transport_ack_passed\": " << (tempoAckPassed ? "true" : "false") << ",\n";
    ss << "    \"note_off_pairing_passed\": " << (noteOffPairingPassed ? "true" : "false") << ",\n";
    ss << "    \"scheduling_accounting_passed\": " << (schedulingAccountingPassed ? "true" : "false") << ",\n";
    ss << "    \"non_silent_audio_passed\": " << (nonSilentAudioPassed ? "true" : "false") << ",\n";
    ss << "    \"role_activity_passed\": " << (roleActivityPassed ? "true" : "false") << ",\n";
    ss << "    \"silence_boundary_passed\": " << (silenceBoundaryPassed ? "true" : "false") << ",\n";
    ss << "    \"determinism_passed\": " << (determinismPassed ? "true" : "false") << ",\n";
    ss << "    \"golden_trace_passed\": " << (goldenTracePassed ? "true" : "false") << ",\n";
    ss << "    \"true_peak_passed\": " << (truePeakPassed ? "true" : "false") << ",\n";
    ss << "    \"sample_ceiling_passed\": " << (sampleCeilingPassed ? "true" : "false") << ",\n";
    ss << "    \"zero_clipping_passed\": " << (clippedSamples == 0 ? "true" : "false") << ",\n";
    ss << "    \"dsp_anomalies_passed\": " << (dspAnomaliesPassed ? "true" : "false") << "\n";
    ss << "  },\n";
    ss << "  \"perceptual_judgement\": \"not_run\",\n";
    ss << "  \"wav_write_success\": " << (wavWriteSuccess ? "true" : "false") << ",\n";
    ss << "  \"manifest_write_success\": " << (manifestWriteSuccess ? "true" : "false") << ",\n";
    ss << "  \"artifacts\": {\n";
    ss << "    \"wav_file\": \"" << escapeJsonString(wavFilePath) << "\",\n";
    ss << "    \"manifest_file\": \"" << escapeJsonString(manifestFilePath) << "\"\n";
    ss << "  }";
    if (!failureReason.empty()) {
        ss << ",\n  \"failure_reason\": \"" << escapeJsonString(failureReason) << "\"";
    }
    ss << "\n}\n";
    return ss.str();
}

std::string ProfileProbeResult::toJson() const {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"manifest_version\": \"1.1\",\n";
    ss << "  \"scenario\": \"profile_probe_v1\",\n";
    ss << "  \"build_id\": \"" << escapeJsonString(buildId) << "\",\n";
    ss << "  \"verdict\": \"" << (passed ? "PASS" : "FAIL") << "\",\n";
    ss << "  \"timing_gated\": " << (timingGated ? "true" : "false") << ",\n";
    ss << "  \"scope\": \"offline render cost on this machine; not a device realtime qualification (ADR-0005 D-M2-10)\",\n";
    ss << "  \"worst_p999_ms\": " << safeJsonDouble(worstP999Ms) << ",\n";
    ss << "  \"worst_max_ms\": " << safeJsonDouble(worstMaxMs) << ",\n";
    ss << "  \"runs\": [\n";
    for (std::size_t i = 0; i < runs.size(); ++i) {
        const auto& r = runs[i];
        ss << "    {\"workload\": \"" << workloadName(r.workload) << "\", \"cacp\": " << (r.cacpEnabled ? "true" : "false")
           << ", \"repetition\": " << r.repetition << ", \"retry\": " << (r.isRetry ? "true" : "false")
           << ", \"total_blocks\": " << r.totalBlocks
           << ", \"p50_ms\": " << safeJsonDouble(r.p50Ms) << ", \"p95_ms\": " << safeJsonDouble(r.p95Ms)
           << ", \"p99_ms\": " << safeJsonDouble(r.p99Ms) << ", \"p999_ms\": " << safeJsonDouble(r.p999Ms)
           << ", \"max_ms\": " << safeJsonDouble(r.maxMs) << ", \"mean_ms\": " << safeJsonDouble(r.meanMs)
           << ", \"blocks_exceeding_2ms\": " << r.blocksExceeding2Ms
           << ", \"blocks_exceeding_block_period\": " << r.blocksExceedingBlockPeriod
           << ", \"max_gain_reduction_db\": " << safeJsonFloat(r.maxGainReductionDb)
           << ", \"voice_steals\": " << r.voiceSteals
           << ", \"stress_conditions_met\": " << (r.stressConditionsMet ? "true" : "false")
           << ", \"p999_passed\": " << (r.p999Passed ? "true" : "false")
           << ", \"block_period_passed\": " << (r.blockPeriodPassed ? "true" : "false") << "}"
           << (i + 1 < runs.size() ? ",\n" : "\n");
    }
    ss << "  ],\n";
    ss << "  \"environment\": {\n";
    ss << "    \"commit\": \"" << escapeJsonString(buildId) << "\",\n";
    ss << "    \"build_config\": \"" << escapeJsonString(buildConfig) << "\",\n";
    ss << "    \"cpu\": \"" << escapeJsonString(cpuInfo) << "\",\n";
    ss << "    \"os_version\": \"" << escapeJsonString(osVersion) << "\",\n";
    ss << "    \"compiler\": \"" << escapeJsonString(compilerVersion) << "\",\n";
    ss << "    \"block_frames\": " << kProfileBlockFrames << ",\n";
    ss << "    \"sample_rate\": " << kInc4SampleRate << ",\n";
    ss << "    \"warmup_blocks\": " << kProfileWarmupBlocks << ",\n";
    ss << "    \"repetitions\": " << kProfileRepetitions << "\n";
    ss << "  },\n";
    ss << "  \"manifest_write_success\": " << (manifestWriteSuccess ? "true" : "false") << ",\n";
    ss << "  \"artifacts\": {\n";
    ss << "    \"manifest_file\": \"" << escapeJsonString(manifestFilePath) << "\"\n";
    ss << "  }";
    if (!failureReason.empty()) {
        ss << ",\n  \"failure_reason\": \"" << escapeJsonString(failureReason) << "\"";
    }
    ss << "\n}\n";
    return ss.str();
}

FixtureProbeResult runFixtureProbeV1Offline(const std::string& outputDir, bool writeFiles) {
    FixtureProbeResult result;
    result.buildId = CYMATICA_BUILD_ID;

    const auto optClock = MusicPlanner::makeFixtureM24V1Clock();
    if (!optClock.has_value()) {
        result.passed = false;
        result.failureReason = "Failed to create Fixture M2.4 V1 clock";
        return result;
    }
    const MusicClock& clock = *optClock;
    const FormPlan form = MusicPlanner::makeFixtureM24V1Form();
    const auto phrases = MusicPlanner::makeFixtureM24V1Phrases(clock);

    constexpr std::uint32_t kFramesPerTick = kInc4SampleRate / core::FixedStepConfig{}.simulationHz; // 400

    struct PlanRun {
        std::vector<std::vector<MusicIntentEvent>> intents;
        std::vector<PhraseDecisionTrace> traces;
        std::vector<PlannedNoteOff> noteOffs;
    };

    auto runPlanningPass = [&]() -> PlanRun {
        MusicPlanner p({.runSeed = kFixtureSeed, .candidateCount = 8, .sampleRate = kInc4SampleRate,
                        .lookaheadFrames = AudioSchedulerConfig{}.lookaheadFrames});
        p.setFormPlan(form);
        PlanRun run;
        run.intents.resize(phrases.size());
        for (std::size_t i = 0; i < phrases.size(); ++i) {
            p.planPhrase(phrases[i], clock, 0, fixtureEpochForSection(phrases[i].sectionIndex), run.intents[i], nullptr);
        }
        run.traces = p.decisionTraces();
        run.noteOffs = p.plannedNoteOffs();
        return run;
    };

    const PlanRun run1 = runPlanningPass();
    const PlanRun run2 = runPlanningPass();
    const bool offlineDeterminism = (run1.intents == run2.intents && run1.traces == run2.traces &&
                                     run1.noteOffs == run2.noteOffs && run1.traces.size() == phrases.size());

    result.decisionTraceHash = fixtureTraceHash(run1.traces);
    result.goldenTracePassed = (result.decisionTraceHash == kFixtureProbeV1GoldenTraceHash);

    for (const auto& phraseIntents : run1.intents) {
        result.generatedEvents += static_cast<std::uint32_t>(phraseIntents.size());
    }
    result.plannerNoteOffs = static_cast<std::uint32_t>(run1.noteOffs.size());

    // NoteOn/NoteOff pairing: timed notes carry their own NoteOff, open pads exactly one planner NoteOff
    {
        bool pairingOk = true;
        for (const auto& phraseIntents : run1.intents) {
            for (const auto& e : phraseIntents) {
                std::uint64_t offFrame = 0;
                std::size_t matches = 0;
                for (const auto& off : run1.noteOffs) {
                    if (off.eventId == e.eventId && off.role == e.role) {
                        ++matches;
                        offFrame = off.sampleFrame;
                    }
                }
                if (e.durationFrames > 0) {
                    if (matches != 0) pairingOk = false;
                    offFrame = e.sampleFrame + e.durationFrames;
                } else if (matches != 1) {
                    pairingOk = false;
                }
                if (offFrame <= e.sampleFrame) pairingOk = false;
            }
        }
        result.noteOffPairingPassed = pairingOk;
    }

    // --- Offline render driven by the just-in-time Coordinator ---
    AudioEngine engine;
    if (!engine.initOffline({kInc4SampleRate, 2, 220.0f})) {
        result.passed = false;
        result.failureReason = "Failed to initialize AudioEngine in offline mode";
        return result;
    }
    AudioProbe probe(AudioProbe::kDefaultCapacityFrames, 2, engine.sampleRate());
    engine.attachAudioProbe(&probe);

    FixtureCoordinator coordinator(clock, form, phrases);
    result.contentEndFrame = coordinator.contentEndFrame();
    result.lookaheadFrames = coordinator.lookaheadFrames();
    result.simulationTicks = (result.contentEndFrame + kFixturePostStopFrames + kFramesPerTick - 1) / kFramesPerTick;
    result.requestedSampleFrames = result.simulationTicks * kFramesPerTick; // 8,825,200 (exact tick boundary)

    std::vector<float> recordedStereo;
    recordedStereo.reserve(static_cast<std::size_t>(result.requestedSampleFrames) * 2);
    std::vector<float> readChunk(static_cast<std::size_t>(kFramesPerTick) * 2);
    std::vector<float> tickBlock(static_cast<std::size_t>(kFramesPerTick) * 2);
    bool captureContinuous = true;

    coordinator.start(engine);
    for (std::uint64_t tick = 0; tick < result.simulationTicks; ++tick) {
        const std::uint64_t currentFrame = tick * kFramesPerTick;
        coordinator.beforeBlock(currentFrame, engine);
        engine.processBlock(tickBlock.data(), kFramesPerTick);

        // Drain the probe completely on every block (no capture discontinuity)
        std::uint32_t drained = 0;
        for (;;) {
            const std::uint32_t framesRead = probe.readFrames(readChunk.data(), kFramesPerTick);
            if (framesRead == 0) break;
            drained += framesRead;
            recordedStereo.insert(recordedStereo.end(), readChunk.begin(), readChunk.begin() + (framesRead * 2));
        }
        if (drained != kFramesPerTick) captureContinuous = false;

        // Per-section, per-role post-bus activity from the telemetry (D-M2-11)
        if (engine.updateTelemetry() && currentFrame < result.contentEndFrame) {
            const auto& t = engine.telemetry();
            const std::size_t s = (currentFrame >= coordinator.tempoBoundary(1)) ? 2
                                : (currentFrame >= coordinator.tempoBoundary(0)) ? 1 : 0;
            const std::array<float, 4> energies{t.pulse.energy, t.body.energy, t.texture.energy, t.vector.energy};
            for (std::size_t r = 0; r < 4; ++r) {
                result.roleEnergyMax[s][r] = (std::max)(result.roleEnergyMax[s][r], energies[r]);
            }
        }
        coordinator.drainAcks(engine);
    }

    result.totalCapturedFrames = recordedStereo.size() / 2;
    result.droppedFrames = probe.droppedFrames();
    (void)engine.updateTelemetry();
    result.transportEpoch = engine.telemetry().transportEpoch;
    result.orphanNoteRecoveredCount = engine.telemetry().orphanNoteRecoveredCount;
    result.voiceDropCount = engine.telemetry().voiceDropCount;
    result.droppedScheduledEvents = engine.droppedScheduledEvents();
    result.deferredScheduledEvents = engine.deferredScheduledEvents();
    result.engineProcessedEvents = engine.totalScheduledEventsProcessed();
    result.maxGainReductionDb = engine.masterLimiter().maxGainReductionDb();
    const std::uint32_t finalDspAnomalies = engine.telemetry().dspAnomalyCount;

    engine.detachAudioProbe();
    engine.shutdown();

    result.phrasesPlanned = static_cast<std::uint32_t>(coordinator.phrasesPlanned());
    result.schedulerBackpressureDrops = coordinator.backpressureDrops();
    result.revokedStagedEvents = coordinator.revokedEvents();
    result.stagedEventsAtStop = coordinator.stagedAtStop();

    // Rendered plan == offline plan (the live Coordinator plans the same decisions just in time)
    result.determinismPassed = offlineDeterminism && coordinator.stopped() &&
                               coordinator.tracesAtStop() == run1.traces && coordinator.noteOffsAtStop() == run1.noteOffs;

    result.captureCompletePassed = captureContinuous && result.droppedFrames == 0 &&
                                   result.totalCapturedFrames == result.requestedSampleFrames;

    // Transport acks: Start epoch 1, SetTempoMap Applied at the planned frames with epochs 2 and 3, Stop Applied
    result.transportAcks = {coordinator.startAck(), coordinator.tempoAck(0), coordinator.tempoAck(1), coordinator.stopAck()};
    result.tempoAckPassed = coordinator.startAckValid() && coordinator.tempoAckValid(0) && coordinator.tempoAckValid(1) &&
                            coordinator.stopAckValid() && result.revokedStagedEvents == 0;

    result.schedulingAccountingPassed = result.droppedScheduledEvents == 0 && result.deferredScheduledEvents == 0 &&
                                        result.schedulerBackpressureDrops == 0 && result.stagedEventsAtStop == 0 &&
                                        result.orphanNoteRecoveredCount == 0 &&
                                        result.phrasesPlanned == phrases.size();

    // Peak, clipping, finite
    float maxAmp = 0.0f;
    bool finiteOk = true;
    for (float s : recordedStereo) {
        if (!std::isfinite(s)) finiteOk = false;
        const float a = std::abs(s);
        if (a > maxAmp) maxAmp = a;
        if (a >= (32767.0f / 32768.0f)) ++result.clippedSamples;
    }
    result.peakAmplitude = maxAmp;
    result.sampleCeilingPassed = finiteOk && (maxAmp <= MasterLimiter::kHardCeilingLinear);
    result.dspAnomaliesPassed = (finalDspAnomalies == 0 && finiteOk);

    // True peak: engineering check of the probe, not an M2.5 gate (AUD-D-03)
    result.truePeakDbtp = analysis::estimateTruePeakDbtp(recordedStereo.data(), result.totalCapturedFrames, 2);
    result.truePeakPassed = (result.truePeakDbtp <= -1.0f);

    // Numerical silence after the Stop declick ramp (2 ms = 96 frames at 48 kHz)
    {
        const std::uint64_t silenceStart = result.contentEndFrame + 96ULL;
        bool silenceOk = result.totalCapturedFrames > silenceStart;
        for (std::size_t i = static_cast<std::size_t>(silenceStart) * 2; silenceOk && i < recordedStereo.size(); ++i) {
            if (recordedStereo[i] != 0.0f) silenceOk = false;
        }
        result.silenceBoundaryPassed = silenceOk;
    }

    // Non-silence per section (whole-section RMS) and per role (telemetry post-bus energy)
    {
        const std::array<std::uint64_t, 4> bounds{0, coordinator.tempoBoundary(0), coordinator.tempoBoundary(1),
                                                  result.contentEndFrame};
        bool nonSilent = true;
        for (std::size_t s = 0; s < 3; ++s) {
            if (bounds[s + 1] * 2 <= recordedStereo.size() && bounds[s + 1] > bounds[s]) {
                result.sectionRmsDbfs[s] = analysis::computeRmsDbfs(&recordedStereo[static_cast<std::size_t>(bounds[s]) * 2],
                                                                    static_cast<std::size_t>(bounds[s + 1] - bounds[s]) * 2);
            }
            if (!(result.sectionRmsDbfs[s] > -40.0f)) nonSilent = false;
        }
        result.nonSilentAudioPassed = nonSilent;

        bool rolesActive = true;
        for (const auto& section : result.roleEnergyMax) {
            for (float e : section) {
                if (!(e > kFixtureRoleActivityFloor)) rolesActive = false;
            }
        }
        result.roleActivityPassed = rolesActive;
    }

    result.pcmContentHash = pcmContentHash(recordedStereo);

    result.passed = result.captureCompletePassed &&
                    result.determinismPassed &&
                    result.goldenTracePassed &&
                    result.noteOffPairingPassed &&
                    result.tempoAckPassed &&
                    result.schedulingAccountingPassed &&
                    result.nonSilentAudioPassed &&
                    result.roleActivityPassed &&
                    result.silenceBoundaryPassed &&
                    result.truePeakPassed &&
                    result.sampleCeilingPassed &&
                    (result.clippedSamples == 0) &&
                    result.dspAnomaliesPassed;

    if (!result.passed) {
        std::ostringstream err;
        if (!result.captureCompletePassed) err << "incomplete or discontinuous capture (" << result.totalCapturedFrames
                                               << " of " << result.requestedSampleFrames << " frames, dropped "
                                               << result.droppedFrames << "); ";
        if (!result.determinismPassed) err << "determinism check failed (offline passes or rendered plan differ); ";
        if (!result.goldenTracePassed) err << "decision trace hash " << result.decisionTraceHash << " != golden "
                                           << kFixtureProbeV1GoldenTraceHash << "; ";
        if (!result.noteOffPairingPassed) err << "NoteOn/NoteOff pairing failed; ";
        if (!result.tempoAckPassed) err << "transport ack check failed (revoked " << result.revokedStagedEvents << "); ";
        if (!result.schedulingAccountingPassed) err << "scheduling accounting failed (dropped " << result.droppedScheduledEvents
                                                    << ", deferred " << result.deferredScheduledEvents << ", orphans "
                                                    << result.orphanNoteRecoveredCount << ", staged at stop "
                                                    << result.stagedEventsAtStop << "); ";
        if (!result.nonSilentAudioPassed) err << "non-silent section check failed; ";
        if (!result.roleActivityPassed) err << "role activity check failed; ";
        if (!result.silenceBoundaryPassed) err << "silence boundary violated after Stop declick tail; ";
        if (!result.sampleCeilingPassed) err << "sample ceiling exceeded (" << maxAmp << " > " << MasterLimiter::kHardCeilingLinear << "); ";
        if (!result.truePeakPassed) err << "true peak ceiling violated (" << result.truePeakDbtp << " > -1.0 dBTP); ";
        if (result.clippedSamples > 0) err << "clipped samples: " << result.clippedSamples << "; ";
        if (!result.dspAnomaliesPassed) err << "DSP anomalies or non-finite samples detected; ";
        result.failureReason = err.str();
    }

    if (writeFiles) {
        std::filesystem::path dir(outputDir);
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        result.wavFilePath = (dir / "fixture_probe_v1.wav").lexically_normal().string();
        result.manifestFilePath = (dir / "fixture_probe_v1_manifest.json").lexically_normal().string();

        result.wavWriteSuccess = analysis::writeWavStereo16(result.wavFilePath, recordedStereo.data(),
                                                            result.totalCapturedFrames, kInc4SampleRate);
        if (!result.wavWriteSuccess) {
            result.passed = false;
            result.failureReason += "[WAV write failed] ";
        }

        std::ofstream mf(result.manifestFilePath);
        if (!mf.is_open()) {
            result.passed = false;
            result.manifestWriteSuccess = false;
            result.failureReason += "[Manifest write failed] ";
        } else {
            result.manifestWriteSuccess = true;
            mf << result.toJson();
            if (!mf.good()) {
                result.passed = false;
                result.manifestWriteSuccess = false;
                result.failureReason += "[Manifest write failed] ";
            }
        }
    }

    return result;
}

ProfileWorkloadRun runProfileWorkload(ProfileWorkload workload, bool cacpEnabled) {
    ProfileWorkloadRun run;
    run.workload = workload;
    run.cacpEnabled = cacpEnabled;

    AudioEngine engine;
    if (!engine.initOffline({kInc4SampleRate, 2, 220.0f})) {
        run.stressConditionsMet = false;
        return run;
    }
    AudioProbe probe(AudioProbe::kDefaultCapacityFrames, 2, kInc4SampleRate);
    if (cacpEnabled) engine.attachAudioProbe(&probe);

    std::vector<float> blockBuffer(static_cast<std::size_t>(kProfileBlockFrames) * 2, 0.0f);
    std::vector<float> probeDrain(static_cast<std::size_t>(kProfileBlockFrames) * 2, 0.0f);
    const auto renderBlock = [&]() {
        engine.processBlock(blockBuffer.data(), kProfileBlockFrames);
        if (cacpEnabled) {
            while (probe.readFrames(probeDrain.data(), kProfileBlockFrames) > 0) {}
        }
    };

    if (workload == ProfileWorkload::W) {
        // 8 held voices on every role (timbreParam > 0 engages filters and the Trama delay), then one
        // full-velocity NoteOn per block: every block steals a held voice. The voices alone peak at -7.13 dBFS (block-0 onset; -8.54 dBFS steady), so
        // the diagnostic tone (summed before the limiter) drives the limiter into gain reduction (ADR-0005 D-M2-10).
        engine.sendCommand(AudioCommand{.commandId = 1, .type = AudioCommandType::Start,
                                        .paramF32 = kProfileWorkloadWToneVolume, .targetSampleFrame = 0});
        const std::array<MusicRole, 4> roles{MusicRole::Pulso, MusicRole::Corpo, MusicRole::Trama, MusicRole::Vettore};
        std::uint64_t nextEventId = 1;
        const auto noteOn = [&](MusicRole role, std::uint64_t frame, float hz) {
            return ScheduledAudioEvent{.eventId = nextEventId++, .sampleFrame = frame, .transportEpoch = 1,
                                       .type = AudioEventType::NoteOn, .role = role, .valueA = hz, .valueB = 1.0f,
                                       .durationFrames = 0, .timbreParam = 0.5f};
        };
        for (std::size_t r = 0; r < roles.size(); ++r) {
            for (int v = 0; v < 2; ++v) {
                if (!engine.scheduleAudioEvent(noteOn(roles[r], 0, 220.0f + static_cast<float>(r * 50 + v * 25)))) {
                    run.stressConditionsMet = false;
                }
            }
        }
        const std::uint32_t totalBlocks = kProfileWarmupBlocks + kProfileWorkloadWBlocks;
        std::uint32_t stealsBefore = 0;
        for (std::uint32_t b = 0; b < totalBlocks; ++b) {
            if (b == kProfileWarmupBlocks) {
                // Discard warm-up measurements
                engine.resetCallbackProfileHistogram();
                engine.masterLimiter().resetTelemetry();
                (void)engine.updateTelemetry();
                stealsBefore = engine.telemetry().voiceDropCount;
            }
            const std::uint64_t frame = static_cast<std::uint64_t>(b) * kProfileBlockFrames;
            if (!engine.scheduleAudioEvent(noteOn(roles[b % roles.size()], frame, 220.0f + static_cast<float>((b % 4) * 50)))) {
                run.stressConditionsMet = false;
            }
            renderBlock();
            if (b >= kProfileWarmupBlocks) {
                if (!engine.updateTelemetry() || engine.telemetry().activeVoiceCount != 8) run.stressConditionsMet = false;
            }
        }
        (void)engine.updateTelemetry();
        run.voiceSteals = engine.telemetry().voiceDropCount - stealsBefore;
        run.maxGainReductionDb = engine.masterLimiter().maxGainReductionDb();
        // One steal per measured block and the limiter in gain reduction (ADR-0005 D-M2-10)
        if (run.voiceSteals < kProfileWorkloadWBlocks || !(run.maxGainReductionDb > 0.0f)) run.stressConditionsMet = false;
    } else {
        const auto optClock = MusicPlanner::makeFixtureM24V1Clock();
        if (!optClock.has_value()) {
            run.stressConditionsMet = false;
            return run;
        }
        const MusicClock& clock = *optClock;
        const FormPlan form = MusicPlanner::makeFixtureM24V1Form();
        const auto phrases = MusicPlanner::makeFixtureM24V1Phrases(clock);
        FixtureCoordinator coordinator(clock, form, phrases);
        const std::uint64_t totalFrames = coordinator.contentEndFrame() + kFixturePostStopFrames;
        const std::uint64_t totalBlocks = (totalFrames + kProfileBlockFrames - 1) / kProfileBlockFrames;

        coordinator.start(engine);
        for (std::uint64_t b = 0; b < totalBlocks; ++b) {
            if (b == kProfileWarmupBlocks) engine.resetCallbackProfileHistogram();
            coordinator.beforeBlock(b * kProfileBlockFrames, engine);
            renderBlock();
            coordinator.drainAcks(engine);
        }
        run.maxGainReductionDb = engine.masterLimiter().maxGainReductionDb();
        run.stressConditionsMet = coordinator.stopped() && coordinator.tempoAckValid(0) && coordinator.tempoAckValid(1) &&
                                  engine.droppedScheduledEvents() == 0 && engine.deferredScheduledEvents() == 0;
    }

    const CallbackProfileHistogram& hist = engine.callbackProfileHistogram();
    run.p50Ms = hist.percentileMs(50.0);
    run.p95Ms = hist.percentileMs(95.0);
    run.p99Ms = hist.percentileMs(99.0);
    run.p999Ms = hist.percentileMs(99.9);
    run.maxMs = hist.maxMs();
    run.meanMs = hist.meanMs();
    run.totalBlocks = hist.totalBlocks;
    run.blocksExceeding2Ms = hist.blocksExceeding2Ms;
    run.blocksExceedingBlockPeriod = hist.blocksExceedingBlockPeriod;
    run.p999Passed = (run.p999Ms <= 2.0); // 512-frame blocks: no normalisation needed
    run.blockPeriodPassed = (run.blocksExceedingBlockPeriod == 0);

    if (cacpEnabled) engine.detachAudioProbe();
    engine.shutdown();
    return run;
}

ProfileProbeResult runProfileProbeV1Offline(const std::string& outputDir, bool writeFiles) {
    ProfileProbeResult result;
    result.buildId = CYMATICA_BUILD_ID;
    result.osVersion = osVersionString();
    result.cpuInfo = cpuBrandString();
    result.compilerVersion = compilerString();
#ifdef NDEBUG
    result.buildConfig = "Release";
    result.timingGated = true;
#else
    result.buildConfig = "Debug"; // timings reported only (ADR-0005 D-M2-10)
    result.timingGated = false;
#endif

    bool allPassed = true;
    std::ostringstream err;
    for (const ProfileWorkload workload : {ProfileWorkload::W, ProfileWorkload::F}) {
        for (const bool cacp : {false, true}) {
            for (std::uint32_t rep = 0; rep < kProfileRepetitions; ++rep) {
                ProfileWorkloadRun run = runProfileWorkload(workload, cacp);
                run.repetition = rep;
                result.runs.push_back(run);
                // A repetition failing only on the block-period criterion gets a single re-run; both are recorded
                if (result.timingGated && run.stressConditionsMet && run.p999Passed && !run.blockPeriodPassed) {
                    run = runProfileWorkload(workload, cacp);
                    run.repetition = rep;
                    run.isRetry = true;
                    result.runs.push_back(run);
                }
                const bool timingOk = !result.timingGated || (run.p999Passed && run.blockPeriodPassed);
                if (!run.stressConditionsMet || !timingOk) {
                    allPassed = false;
                    err << "workload " << workloadName(workload) << " CACP " << (cacp ? "ON" : "OFF") << " rep " << rep
                        << ": stress=" << (run.stressConditionsMet ? "ok" : "FAIL") << " p99.9=" << run.p999Ms
                        << " ms, blocks over period=" << run.blocksExceedingBlockPeriod << "; ";
                }
                result.worstP999Ms = (std::max)(result.worstP999Ms, run.p999Ms);
                result.worstMaxMs = (std::max)(result.worstMaxMs, run.maxMs);
            }
        }
    }
    result.passed = allPassed;
    result.failureReason = err.str();

    if (writeFiles) {
        std::filesystem::path dir(outputDir);
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        result.manifestFilePath = (dir / "profile_probe_v1_manifest.json").lexically_normal().string();

        std::ofstream mf(result.manifestFilePath);
        if (!mf.is_open()) {
            result.passed = false;
            result.manifestWriteSuccess = false;
            result.failureReason += "[Manifest write failed] ";
        } else {
            result.manifestWriteSuccess = true;
            mf << result.toJson();
            if (!mf.good()) {
                result.passed = false;
                result.manifestWriteSuccess = false;
                result.failureReason += "[Manifest write failed] ";
            }
        }
    }

    return result;
}

} // namespace cymatica::audio
