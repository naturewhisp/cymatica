#pragma once

#include "synth_dsp.h"
#include <array>
#include <cmath>
#include <cstdint>

namespace cymatica::audio {

// Common terminal digital protection stage: always active, fail-closed,
// zero latency (L = 0) at device sample rate, verified by independent ITU-R BS.1770-5 true-peak meter.
// (ADR-0004 AUD-D-02, ADR-0003 D-M2-06, Spec §20.2)
class MasterLimiter {
public:
    // Internal operating threshold: -1.50 dBFS = 0.84139514f
    static constexpr float kDefaultThresholdDbfs = -1.50f;
    static constexpr float kThresholdLinear = 0.8413951416f; // 10^(-1.50/20)

    // Hard terminal safety ceiling: -1.0 dBFS = 0.89125094f
    static constexpr float kHardCeilingDbfs = -1.0f;
    static constexpr float kHardCeilingLinear = 0.8912509385f; // 10^(-1.0/20)

    MasterLimiter() noexcept = default;

    void init(std::uint32_t sampleRate) noexcept;
    void reset() noexcept;

    // Process a single stereo frame in-place with zero latency (L = 0).
    // Always active, fail-closed: finite guard -> hybrid true-peak detector ->
    // stereo-linked gain reduction -> anti-overshoot slew guard -> hard safety clamp.
    void processFrame(float& ioL, float& ioR) noexcept;

    // Process an interleaved stereo block of length `frameCount` in-place
    void processBlock(float* pInterleaved, std::uint32_t frameCount) noexcept;

    // Configuration & telemetry
    void setThresholdLinear(float threshold) noexcept {
        thresholdLinear_ = std::clamp(threshold, 0.1f, kHardCeilingLinear);
    }
    [[nodiscard]] float thresholdLinear() const noexcept { return thresholdLinear_; }

    [[nodiscard]] float currentGain() const noexcept { return currentGain_; }
    [[nodiscard]] float maxGainReductionDb() const noexcept { return maxGainReductionDb_; }
    [[nodiscard]] std::uint32_t anomalyCount() const noexcept { return anomalyCount_; }

    void resetTelemetry() noexcept {
        maxGainReductionDb_ = 0.0f;
        anomalyCount_ = 0;
    }

private:
    std::uint32_t sampleRate_{48000};
    float thresholdLinear_{kThresholdLinear};

    // Ballistics
    std::uint32_t holdFrames_{96}; // 2 ms @ 48 kHz
    std::uint32_t holdRemaining_{0};
    float releaseAlpha_{0.000416f}; // 50 ms exponential release

    float currentGain_{1.0f};
    float maxGainReductionDb_{0.0f};
    std::uint32_t anomalyCount_{0};

    // Causal circular history buffer for 12-sample polyphase & curvature detection
    static constexpr std::size_t kHistoryLength = 12;
    std::array<float, kHistoryLength> historyL_{};
    std::array<float, kHistoryLength> historyR_{};
    std::size_t historyHead_{0};

    // Previous output samples for slew guard
    float prevOutL_{0.0f};
    float prevOutR_{0.0f};

    // Estimates true intersample peak causally at current sample index
    [[nodiscard]] float estimatePeakCausal(float inL, float inR) noexcept;
};

} // namespace cymatica::audio
