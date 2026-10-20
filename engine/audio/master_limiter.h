#pragma once

#include "synth_dsp.h"
#include <array>
#include <cmath>
#include <cstdint>

namespace cymatica::audio {

// Common terminal digital protection stage: always active, fail-closed, zero latency (L = 0) at device
// sample rate. Exactly one instance exists, owned by AudioEngine and applied to the final pOutput sum
// (voices + diagnostic tone) before the AudioProbe tap. (ADR-0004 AUD-D-02, ADR-0003 D-M2-06, Spec §20.2)
//
// Guarantee scope: the final hard clamp bounds every *sample* peak to <= kHardCeilingLinear (-1.0 dBFS).
// The inter-sample (true-peak) margin produced by the causal hybrid detector is an empirical property,
// verified by the independent ITU-R BS.1770-5 meter (analysis::estimateTruePeakDbtp) only on the tested
// vectors (T-ADV-01..06 and the CACP probes); it is not a guarantee for arbitrary input.
class MasterLimiter {
public:
    // Internal operating threshold: -1.80 dBFS = 0.81283052f (margin below the -1.0 dBFS ceiling; the resulting
    // <= -1.0 dBTP behaviour is measured on the tested vectors, not guaranteed for arbitrary input)
    static constexpr float kDefaultThresholdDbfs = -1.80f;
    static constexpr float kThresholdLinear = 0.8128305161f; // 10^(-1.80/20)

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
    // Maximum gain reduction since the last reset, converted lazily from the minimum tracked gain
    // (no per-sample log10 on the realtime path).
    [[nodiscard]] float maxGainReductionDb() const noexcept {
        return (minGain_ < 0.999f) ? -20.0f * std::log10((std::max)(minGain_, 1e-6f)) : 0.0f;
    }
    // Monotonic count of non-finite input samples sanitised; preserved across reset() so the engine can
    // publish it as part of AudioTelemetryFrame::dspAnomalyCount. Cleared only by resetTelemetry().
    [[nodiscard]] std::uint32_t anomalyCount() const noexcept { return anomalyCount_; }

    void resetTelemetry() noexcept {
        minGain_ = 1.0f;
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
    float minGain_{1.0f};
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
