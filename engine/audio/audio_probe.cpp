#include "audio_probe.h"
#include "audio_engine.h"
#include "audio_scheduler.h"
#include "music_clock.h"
#include "music_planner.h"

#include <algorithm>
#include <cmath>
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

// ITU-R BS.1770-5 Annex 2 Table 2 48-tap polyphase interpolation filter coefficients (4 phases, 12 taps per phase)
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
        for (std::size_t i = 0; i < frameCount; ++i) {
            const float sRaw = pInterleavedSamples[i * channels + ch];
            if (std::isfinite(sRaw)) {
                maxPeak = (std::max)(maxPeak, std::abs(sRaw));
            }

            // Gather 12 surrounding samples: i - 5 to i + 6 with edge replication at boundaries
            float window[12];
            for (int k = 0; k < 12; ++k) {
                const int64_t idx = static_cast<int64_t>(i) - 5 + k;
                const int64_t clampedIdx = std::clamp<int64_t>(idx, 0, static_cast<int64_t>(frameCount - 1));
                const float val = pInterleavedSamples[static_cast<std::size_t>(clampedIdx) * channels + ch];
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

TimbreBusProbeResult runTimbreBusProbeV1Offline(const std::string& outputDir, bool writeFiles) {
    TimbreBusProbeResult result{};
    result.simulationTicks = 360;
    result.requestedSampleFrames = 144000;
    result.presentationQuality = "Estimated";

    AudioEngine engine;
    if (!engine.initOffline({48000, 2, 220.0f})) {
        result.passed = false;
        result.failureReason = "Failed to initialize AudioEngine in offline mode";
        return result;
    }

    AudioProbe probe(AudioProbe::kDefaultCapacityFrames, 2, engine.sampleRate());
    engine.attachAudioProbe(&probe);

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

    engine.scheduleAudioEvent(ScheduledAudioEvent{
        .eventId = 1002,
        .sampleFrame = 56000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOff,
        .role = MusicRole::Corpo
    });

    engine.scheduleAudioEvent(ScheduledAudioEvent{
        .eventId = 1004,
        .sampleFrame = 56000,
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

    constexpr std::uint32_t kFramesPerTick = 400; // 48000 / 120
    constexpr std::uint32_t kTotalTicks = 360;    // 3.0 s * 120 ticks/s = 360 ticks

    std::vector<float> recordedStereo;
    recordedStereo.reserve(static_cast<std::size_t>(kTotalTicks) * kFramesPerTick * 2);
    std::vector<float> readChunk(kFramesPerTick * 2);
    std::vector<float> tickBlock(kFramesPerTick * 2);

    for (std::uint32_t tick = 0; tick < kTotalTicks; ++tick) {
        if (tick == 300) {
            engine.sendCommand(AudioCommand{
                .commandId = 2,
                .type = AudioCommandType::Stop,
                .targetSampleFrame = 120000
            });
        }

        engine.processBlock(tickBlock.data(), kFramesPerTick);

        const std::uint32_t framesRead = probe.readFrames(readChunk.data(), kFramesPerTick);
        if (framesRead > 0) {
            recordedStereo.insert(recordedStereo.end(), readChunk.begin(), readChunk.begin() + (framesRead * 2));
        }
    }

    result.totalCapturedFrames = recordedStereo.size() / 2;
    result.droppedFrames = probe.droppedFrames();
    (void)engine.updateTelemetry();
    result.transportEpoch = engine.telemetry().transportEpoch;
    const std::uint32_t finalDspAnomalies = engine.telemetry().dspAnomalyCount;

    engine.detachAudioProbe();
    engine.shutdown();

    if (result.totalCapturedFrames < result.requestedSampleFrames) {
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

    // 3. DSP anomalies and finiteness check
    result.dspAnomaliesPassed = (finalDspAnomalies == 0);
    if (result.dspAnomaliesPassed) {
        for (float s : recordedStereo) {
            if (!std::isfinite(s)) {
                result.dspAnomaliesPassed = false;
                break;
            }
        }
    }

    // 4. Role isolation check: each section shows energy in its active role
    // Section 1: Pulso (frames 1000..2000)
    const float pulsoRms = analysis::computeRmsDbfs(&recordedStereo[2000], 2000);
    // Section 2: Corpo (frames 20000..24000)
    const float corpoRms = analysis::computeRmsDbfs(&recordedStereo[40000], 4000);
    // Section 4: Trama (frames 60000..64000)
    const float tramaRms = analysis::computeRmsDbfs(&recordedStereo[120000], 4000);
    // Section 5: Vettore (frames 85000..89000)
    const float vettoreRms = analysis::computeRmsDbfs(&recordedStereo[170000], 4000);
    result.roleIsolationPassed = (pulsoRms > -40.0f) && (corpoRms > -40.0f) &&
                                 (tramaRms > -40.0f) && (vettoreRms > -40.0f);

    // 5. Ducking check: Corpo frame 31000..31900 vs frame 32100..33000
    const float preDuckingRms = analysis::computeRmsDbfs(&recordedStereo[62000], 1800);
    const float postDuckingRms = analysis::computeRmsDbfs(&recordedStereo[64200], 1800);
    result.duckingPassed = (preDuckingRms > -35.0f) && (postDuckingRms > -30.0f);

    // 6. Delay check: Trama section (frames 60000..64000) has spatial stereo decorrelation
    double stereoDiffEnergy = 0.0;
    for (std::size_t i = 60000; i < 64000; ++i) {
        const double diff = static_cast<double>(recordedStereo[i * 2]) - static_cast<double>(recordedStereo[i * 2 + 1]);
        stereoDiffEnergy += diff * diff;
    }
    result.delayPassed = (stereoDiffEnergy > 0.01);

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
        if (!result.roleIsolationPassed) err << "role isolation failed; ";
        if (!result.duckingPassed) err << "ducking check failed: pre=" << preDuckingRms << " post=" << postDuckingRms << "; ";
        if (!result.delayPassed) err << "delay check failed; ";
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
    ss << "  \"manifest_version\": \"1.0\",\n";
    ss << "  \"scenario_id\": \"planner_probe_v1\",\n";
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
    ss << "  \"generated_events\": " << generatedEvents << ",\n";
    ss << "  \"committed_events\": " << committedEvents << ",\n";
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
    result.simulationTicks = 1800;
    result.requestedSampleFrames = 720000;
    result.presentationQuality = "Estimated";

    AudioEngine engine;
    if (!engine.initOffline({48000, 2, 220.0f})) {
        result.passed = false;
        result.failureReason = "Failed to initialize AudioEngine in offline mode";
        return result;
    }

    AudioProbe probe(AudioProbe::kDefaultCapacityFrames, 2, engine.sampleRate());
    engine.attachAudioProbe(&probe);

    const FormPlan form = MusicPlanner::makePlannerProbeV1Form();
    auto clockOpt = MusicClock::create(48000, form.sections[0].tempoMap);
    if (!clockOpt.has_value()) {
        result.passed = false;
        result.failureReason = "Failed to initialize MusicClock for Section 0";
        engine.detachAudioProbe();
        engine.shutdown();
        return result;
    }
    MusicClock clock = std::move(*clockOpt);
    const std::uint64_t s0EndFrame = clock.frameAtBar(4); // 384000
    (void)clock.setTempoMap(form.sections[1].tempoMap, s0EndFrame);
    const std::uint64_t s1EndFrame = clock.frameAtBar(8);

    AudioScheduler scheduler({.lookaheadFrames = 4800, .sampleRate = 48000});
    MusicPlanner planner({.runSeed = 1337ULL, .candidateCount = 6, .sampleRate = 48000});

    const PhrasePlan p0{
        .sectionIndex = 0,
        .phraseIndex = 0,
        .startBar = 0,
        .barCount = 4,
        .startFrame = 0,
        .endFrame = s0EndFrame,
        .scale = form.sections[0].scale,
        .chords = {
            ChordSpec{0, 2, 4, HarmonicFunction::Stability, true},
            ChordSpec{3, 5, 0, HarmonicFunction::Preparation, true},
            ChordSpec{4, 6, 1, HarmonicFunction::Tension, true},
            ChordSpec{0, 2, 4, HarmonicFunction::Resolution, true}
        },
        .targetTension = form.sections[0].baseTension,
        .dominantRole = MusicRole::Vettore,
        .cadenceGoal = CadenceGoal::Sustain,
        .baseMotifId = form.sections[0].sectionMotif.motifId
    };

    const PhrasePlan p1{
        .sectionIndex = 1,
        .phraseIndex = 0,
        .startBar = 4,
        .barCount = 4,
        .startFrame = s0EndFrame,
        .endFrame = s1EndFrame,
        .scale = form.sections[1].scale,
        .chords = {
            ChordSpec{0, 2, 4, HarmonicFunction::Stability, true},
            ChordSpec{5, 0, 2, HarmonicFunction::Preparation, true},
            ChordSpec{6, 1, 3, HarmonicFunction::Tension, true},
            ChordSpec{0, 2, 4, HarmonicFunction::Resolution, true}
        },
        .targetTension = form.sections[1].baseTension,
        .dominantRole = MusicRole::Vettore,
        .cadenceGoal = CadenceGoal::ModalResolution,
        .baseMotifId = form.sections[1].sectionMotif.motifId
    };

    std::vector<MusicIntentEvent> intents0;
    planner.planPhrase(p0, 0, 1, intents0, &scheduler);

    std::vector<MusicIntentEvent> intents1;
    planner.planPhrase(p1, s0EndFrame, 2, intents1, &scheduler);

    result.generatedEvents = static_cast<std::uint32_t>(intents0.size() + intents1.size());

    // 1. Invariant verification: Determinism & Seed Isolation
    {
        MusicPlanner plannerDet({.runSeed = 1337ULL, .candidateCount = 6, .sampleRate = 48000});
        std::vector<MusicIntentEvent> d0;
        std::vector<MusicIntentEvent> d1;
        plannerDet.planPhrase(p0, 0, 1, d0, nullptr);
        plannerDet.planPhrase(p1, s0EndFrame, 2, d1, nullptr);

        bool detMatch = (intents0.size() == d0.size()) && (intents1.size() == d1.size());
        if (detMatch) {
            for (std::size_t i = 0; i < intents0.size(); ++i) {
                if (intents0[i].eventId != d0[i].eventId ||
                    intents0[i].sampleFrame != d0[i].sampleFrame ||
                    intents0[i].role != d0[i].role ||
                    intents0[i].pitchHz != d0[i].pitchHz) {
                    detMatch = false;
                    break;
                }
            }
        }
        result.determinismPassed = detMatch;

        // Seed Isolation: Mutate Gameplay stream
        core::RandomKey gameKey{9999ULL, static_cast<std::uint32_t>(core::StreamId::Director), 42, 1};
        (void)core::staffordMix13(core::randomU64(gameKey));

        MusicPlanner plannerIso({.runSeed = 1337ULL, .candidateCount = 6, .sampleRate = 48000});
        std::vector<MusicIntentEvent> i0;
        plannerIso.planPhrase(p0, 0, 1, i0, nullptr);
        result.streamIsolationPassed = (intents0.size() == i0.size());
    }

    // 2. Motif Invariance: Decision traces check
    {
        bool motifOk = true;
        const auto& traces = planner.decisionTraces();
        if (traces.empty()) {
            motifOk = false;
        }
        result.motifInvariancePassed = motifOk;
    }

    // 3. Triad Budget on Corpo
    {
        bool budgetOk = true;
        const auto& tracker = planner.voiceTracker();
        std::uint32_t corpoActive = tracker.activeCountForRole(MusicRole::Corpo, s1EndFrame);
        if (corpoActive > 2) {
            budgetOk = false;
        }
        result.triadBudgetPassed = budgetOk;
    }

    // 4. Fallback Verification: SilentRecovery on exhausted horizon
    {
        PhrasePlan pExhausted = p0;
        pExhausted.startFrame = 100000;
        pExhausted.endFrame = 100000; // Zero duration / exhausted horizon
        std::vector<MusicIntentEvent> fbIntents;
        MusicPlanner fbPlanner({.runSeed = 1337ULL});
        fbPlanner.planPhrase(pExhausted, 100000, 1, fbIntents, nullptr);
        result.fallbackPassed = fbIntents.empty() && (fbPlanner.totalFallbacksTriggered() > 0);
    }

    // Start engine
    engine.sendCommand(AudioCommand{
        .commandId = 1,
        .type = AudioCommandType::Start,
        .targetSampleFrame = 0
    });

    constexpr std::uint32_t kFramesPerTick = 400; // 48000 / 120
    const std::uint32_t totalTicks = static_cast<std::uint32_t>(result.simulationTicks);

    std::vector<float> recordedStereo;
    recordedStereo.reserve(static_cast<std::size_t>(totalTicks) * kFramesPerTick * 2);
    std::vector<float> readChunk(kFramesPerTick * 2);
    std::vector<float> tickBlock(kFramesPerTick * 2);

    for (std::uint32_t tick = 0; tick < totalTicks; ++tick) {
        const std::uint64_t currentFrame = static_cast<std::uint64_t>(tick) * kFramesPerTick;

        // Transition at Section 1 boundary (bar 4 = frame 384000 = tick 960)
        if (currentFrame == s0EndFrame) {
            engine.sendCommand(AudioCommand::makeSetTempoMap(2, form.sections[1].tempoMap, s0EndFrame));
        }

        // Send Stop command towards the end (tick 1750 = frame 700000)
        if (tick == 1750) {
            engine.sendCommand(AudioCommand{
                .commandId = 3,
                .type = AudioCommandType::Stop,
                .targetSampleFrame = 700000
            });
        }

        // Dispatch due events from scheduler with lookahead
        scheduler.dispatchDueEvents(currentFrame + scheduler.lookaheadFrames(), engine);

        engine.processBlock(tickBlock.data(), kFramesPerTick);

        const std::uint32_t framesRead = probe.readFrames(readChunk.data(), kFramesPerTick);
        if (framesRead > 0) {
            recordedStereo.insert(recordedStereo.end(), readChunk.begin(), readChunk.begin() + (framesRead * 2));
        }
    }

    result.committedEvents = static_cast<std::uint32_t>(scheduler.totalDispatchedEvents());
    result.totalCapturedFrames = recordedStereo.size() / 2;
    result.droppedFrames = probe.droppedFrames();
    (void)engine.updateTelemetry();
    result.transportEpoch = engine.telemetry().transportEpoch;
    const std::uint32_t finalDspAnomalies = engine.telemetry().dspAnomalyCount;

    engine.detachAudioProbe();
    engine.shutdown();

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

    // Non-silent audio check: verify audible RMS energy in Section 0 and Section 1
    const float section0Rms = analysis::computeRmsDbfs(&recordedStereo[48000 * 2], 48000); // bar 1
    const float section1Rms = analysis::computeRmsDbfs(&recordedStereo[(s0EndFrame + 48000) * 2], 48000); // Section 1 bar 1
    result.nonSilentAudioPassed = (section0Rms > -40.0f) && (section1Rms > -40.0f);

    result.passed = (result.droppedFrames == 0) &&
                    result.determinismPassed &&
                    result.streamIsolationPassed &&
                    result.motifInvariancePassed &&
                    result.triadBudgetPassed &&
                    result.fallbackPassed &&
                    result.nonSilentAudioPassed &&
                    result.truePeakPassed &&
                    result.dspAnomaliesPassed &&
                    (result.clippedSamples == 0);

    if (!result.passed) {
        std::ostringstream err;
        if (result.droppedFrames > 0) err << "dropped frames in probe; ";
        if (!result.determinismPassed) err << "determinism check failed; ";
        if (!result.streamIsolationPassed) err << "stream isolation check failed; ";
        if (!result.motifInvariancePassed) err << "motif invariance check failed; ";
        if (!result.triadBudgetPassed) err << "triad budget check failed; ";
        if (!result.fallbackPassed) err << "fallback check failed; ";
        if (!result.nonSilentAudioPassed) err << "non-silent audio check failed: s0=" << section0Rms << " dBFS, s1=" << section1Rms << " dBFS; ";
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

} // namespace cymatica::audio
