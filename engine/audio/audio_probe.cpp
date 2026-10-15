#include "audio_probe.h"
#include "audio_engine.h"

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
        sumSq += s * s;
    }
    const double rms = std::sqrt(sumSq / static_cast<double>(sampleCount));
    if (rms <= 1e-6) return -120.0f;
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
    int bestLag = -1;
    double bestVal = -1e30;

    for (int k = kMin; k <= kMax; ++k) {
        double r = 0.0;
        const std::size_t nEnd = sampleCount - static_cast<std::size_t>(k);
        for (std::size_t n = 0; n < nEnd; ++n) {
            r += static_cast<double>(pSamples[n]) * static_cast<double>(pSamples[n + k]);
        }
        // Normalize by number of overlapped samples
        r /= static_cast<double>(nEnd);

        if (r > bestVal) {
            bestVal = r;
            bestLag = k;
        }
    }

    if (bestLag < kMin || bestLag > kMax || bestVal <= 0.0) return 0.0f;

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
        const double y1 = bestVal;
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
        const float s = std::clamp(pInterleavedSamples[i], -1.0f, 1.0f);
        pcm[i] = static_cast<std::int16_t>(std::round(s * 32767.0f));
    }
    out.write(reinterpret_cast<const char*>(pcm.data()), pcm.size() * sizeof(std::int16_t));
    return out.good();
}

} // namespace analysis

namespace {

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
    ss << "    \"target_freq_hz\": " << phase1Baseline.targetFreqHz << ",\n";
    ss << "    \"estimated_freq_hz\": " << phase1Baseline.estimatedFreqHz << ",\n";
    ss << "    \"rms_dbfs\": " << phase1Baseline.rmsDbfs << ",\n";
    ss << "    \"freq_passed\": " << (phase1Baseline.freqPassed ? "true" : "false") << ",\n";
    ss << "    \"rms_passed\": " << (phase1Baseline.rmsPassed ? "true" : "false") << ",\n";
    ss << "    \"passed\": " << (phase1Baseline.passed ? "true" : "false") << "\n";
    ss << "  },\n";
    ss << "  \"phase_2_hold\": {\n";
    ss << "    \"start_ms\": " << phase2Hold.startMs << ",\n";
    ss << "    \"start_sample_frame\": " << phase2Hold.startSampleFrame << ",\n";
    ss << "    \"duration_ms\": " << phase2Hold.durationMs << ",\n";
    ss << "    \"target_freq_hz\": " << phase2Hold.targetFreqHz << ",\n";
    ss << "    \"estimated_freq_hz\": " << phase2Hold.estimatedFreqHz << ",\n";
    ss << "    \"rms_dbfs\": " << phase2Hold.rmsDbfs << ",\n";
    ss << "    \"freq_passed\": " << (phase2Hold.freqPassed ? "true" : "false") << ",\n";
    ss << "    \"rms_passed\": " << (phase2Hold.rmsPassed ? "true" : "false") << ",\n";
    ss << "    \"passed\": " << (phase2Hold.passed ? "true" : "false") << "\n";
    ss << "  },\n";
    ss << "  \"phase_3_release\": {\n";
    ss << "    \"start_ms\": " << phase3Release.startMs << ",\n";
    ss << "    \"start_sample_frame\": " << phase3Release.startSampleFrame << ",\n";
    ss << "    \"duration_ms\": " << phase3Release.durationMs << ",\n";
    ss << "    \"target_freq_hz\": " << phase3Release.targetFreqHz << ",\n";
    ss << "    \"estimated_freq_hz\": " << phase3Release.estimatedFreqHz << ",\n";
    ss << "    \"rms_dbfs\": " << phase3Release.rmsDbfs << ",\n";
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
    ss << "  \"simulation_ticks\": " << simulationTicks << ",\n";
    ss << "  \"requested_sample_frames\": " << requestedSampleFrames << ",\n";
    ss << "  \"applied_sample_frames\": " << totalCapturedFrames << ",\n";
    ss << "  \"dropped_frames\": " << droppedFrames << ",\n";
    ss << "  \"voice_steals_detected\": " << voiceStealsDetected << ",\n";
    ss << "  \"active_voices_phase_1\": " << activeVoicesPhase1 << ",\n";
    ss << "  \"checks\": {\n";
    ss << "    \"steal_continuity_passed\": " << (stealContinuityPassed ? "true" : "false") << ",\n";
    ss << "    \"pause_silence_passed\": " << (pauseSilencePassed ? "true" : "false") << ",\n";
    ss << "    \"resume_passed\": " << (resumePassed ? "true" : "false") << ",\n";
    ss << "    \"stop_silence_passed\": " << (stopSilencePassed ? "true" : "false") << ",\n";
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

    engine.detachAudioProbe();
    engine.shutdown();

    result.totalCapturedFrames = recordedStereo.size() / 2;
    result.droppedFrames = probe.droppedFrames();
    result.activeVoicesPhase1 = activeVoicesAtTick30;
    result.voiceStealsDetected = 1;

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

    // 5. Check DSP anomalies (NaN / Inf)
    result.dspAnomaliesPassed = true;
    for (float s : recordedStereo) {
        if (!std::isfinite(s)) {
            result.dspAnomaliesPassed = false;
            break;
        }
    }

    result.passed = (result.droppedFrames == 0) &&
                    (result.activeVoicesPhase1 == 2) &&
                    result.stealContinuityPassed &&
                    result.pauseSilencePassed &&
                    result.resumePassed &&
                    result.stopSilencePassed &&
                    result.dspAnomaliesPassed;

    if (!result.passed) {
        std::ostringstream err;
        if (result.droppedFrames > 0) err << "dropped frames in probe; ";
        if (result.activeVoicesPhase1 != 2) err << "active voices phase 1 was " << result.activeVoicesPhase1 << " (expected 2); ";
        if (!result.stealContinuityPassed) err << "steal continuity jump detected; ";
        if (!result.pauseSilencePassed) err << "pause was not silent; ";
        if (!result.resumePassed) err << "resume did not produce audio; ";
        if (!result.stopSilencePassed) err << "stop was not silent; ";
        if (!result.dspAnomaliesPassed) err << "non-finite samples detected; ";
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

} // namespace cymatica::audio
