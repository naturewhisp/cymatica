#pragma once

#include "input_dispatcher.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace cymatica::audio {

class AudioEngine;

// AudioProbe lock-free SPSC PCM tap (Spec §31.5, ADR-0003 D-AO-02).
// Producer is the realtime audio callback (processBlock).
// Consumer is an asynchronous worker or test diagnostic runner.
// Invariants: zero heap allocation, zero blocking locks, bounded execution in realtime path.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
#endif
class AudioProbe {
public:
    static constexpr std::uint32_t kDefaultCapacityFrames = 24000; // 0.5 s @ 48 kHz stereo = 192 KB

    explicit AudioProbe(std::uint32_t capacityFrames = kDefaultCapacityFrames,
                        std::uint32_t channels = 2,
                        std::uint32_t sampleRate = 48000);
    ~AudioProbe() = default;

    AudioProbe(const AudioProbe&) = delete;
    AudioProbe& operator=(const AudioProbe&) = delete;

    // Realtime producer path: called from AudioEngine::Impl::processBlock.
    // If buffer is full, increments droppedFrames counter without blocking or allocating.
    void writeFrames(const float* pInterleavedInput, std::uint32_t frameCount) noexcept;

    // Consumer path: called from worker / diagnostic runner thread.
    // Reads up to maxFrames, returning the number of frames actually read.
    std::uint32_t readFrames(float* pInterleavedOutput, std::uint32_t maxFrames) noexcept;

    [[nodiscard]] std::uint64_t droppedFrames() const noexcept;
    [[nodiscard]] std::uint32_t availableFrames() const noexcept;
    [[nodiscard]] std::uint32_t capacityFrames() const noexcept;
    [[nodiscard]] std::uint32_t channels() const noexcept;
    [[nodiscard]] std::uint32_t sampleRate() const noexcept;

    void reset() noexcept;

private:
    const std::uint32_t capacityFrames_;
    const std::uint32_t channels_;
    const std::uint32_t sampleRate_;
    const std::uint32_t capacitySamples_; // capacityFrames_ * channels_

    std::vector<float> ringBuffer_;

    // SPSC cursors
    alignas(64) std::atomic<std::uint64_t> writeHead_{0};
    alignas(64) std::atomic<std::uint64_t> readHead_{0};
    alignas(64) std::atomic<std::uint64_t> droppedFrames_{0};
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// Quantitative analysis utilities (ADR-0003 D-AO-02)
namespace analysis {

// Computes Root-Mean-Square in dBFS (0 dBFS = full scale 1.0 sine/square)
float computeRmsDbfs(const float* pSamples, std::size_t sampleCount) noexcept;

// Estimates fundamental frequency in Hz using normalized autocorrelation with parabolic interpolation.
float estimatePitchHz(const float* pSamples,
                      std::size_t sampleCount,
                      std::uint32_t sampleRate,
                      float minFreqHz = 80.0f,
                      float maxFreqHz = 1000.0f) noexcept;

// Writes 16-bit PCM stereo WAV file
bool writeWavStereo16(const std::string& filePath,
                      const float* pInterleavedSamples,
                      std::size_t frameCount,
                      std::uint32_t sampleRate);

} // namespace analysis

#ifndef CYMATICA_BUILD_ID
#define CYMATICA_BUILD_ID "unknown"
#endif

// Profile metrics for tone_probe_v0 (Spec §31.5, ADR-0003 D-AO-02)
struct PhaseMetric {
    std::uint32_t startMs{0};
    std::uint64_t startSampleFrame{0};
    std::uint32_t durationMs{250};
    float targetFreqHz{220.0f};
    float estimatedFreqHz{0.0f};
    float rmsDbfs{-120.0f};
    bool freqPassed{false};
    bool rmsPassed{false};
    bool passed{false};
};

struct ToneProbeResult {
    bool passed{false};
    PhaseMetric phase1Baseline{};
    PhaseMetric phase2Hold{};
    PhaseMetric phase3Release{};
    std::uint64_t totalCapturedFrames{0};
    std::uint64_t droppedFrames{0};
    std::string buildId{CYMATICA_BUILD_ID};
    std::uint64_t transportEpoch{1};
    std::uint64_t simulationTicks{360};
    std::uint64_t requestedSampleFrames{144000};
    std::uint64_t appliedSampleFrames{144000};
    std::string presentationQuality{"Estimated"};
    bool wavWriteSuccess{false};
    bool manifestWriteSuccess{false};
    std::string wavFilePath{};
    std::string manifestFilePath{};
    std::string failureReason{};

    [[nodiscard]] std::string toJson() const;
};

// Runs the authoritative tone_probe_v0 scenario offline in 120 Hz simulation ticks
ToneProbeResult runToneProbeV0Offline(const std::string& outputDir = "artifacts/cacp",
                                      bool writeFiles = true);

// Runs tone_probe_v0 scenario against an already initialized AudioEngine and InputDispatcher
ToneProbeResult runToneProbeV0(AudioEngine& engine,
                               cymatica::core::InputDispatcher& dispatcher,
                               const std::string& outputDir = "artifacts/cacp",
                               bool writeFiles = true);

// Profile metrics for voice_probe_v1 (ADR-0003 D-M2-03, ADR-0004 AUD-D-01)
struct VoiceProbeResult {
    bool passed{false};
    std::uint64_t totalCapturedFrames{0};
    std::uint64_t droppedFrames{0};
    std::uint32_t voiceStealsDetected{0};
    std::uint32_t activeVoicesPhase1{0};
    bool stealContinuityPassed{false};
    bool pauseSilencePassed{false};
    bool resumePassed{false};
    bool stopSilencePassed{false};
    bool dspAnomaliesPassed{false};
    std::uint32_t clippedSamples{0};
    float peakAmplitude{0.0f};
    bool headroomPassed{false};
    std::string buildId{CYMATICA_BUILD_ID};
    std::uint64_t simulationTicks{300};
    std::uint64_t requestedSampleFrames{120000};
    bool wavWriteSuccess{false};
    bool manifestWriteSuccess{false};
    std::string wavFilePath{};
    std::string manifestFilePath{};
    std::string failureReason{};

    [[nodiscard]] std::string toJson() const;
};

// Runs the authoritative voice_probe_v1 scenario offline in 120 Hz simulation ticks (Inc-1)
VoiceProbeResult runVoiceProbeV1Offline(const std::string& outputDir = "artifacts/cacp",
                                        bool writeFiles = true);

} // namespace cymatica::audio
