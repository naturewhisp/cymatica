#pragma once

#include "../core/stable_random.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace cymatica::audio {

inline constexpr float kTwoPi = 6.28318530717958647692f;
inline constexpr float kPi = 3.14159265358979323846f;

// Portable subnormal flush for realtime audio integrators and delay lines
[[nodiscard]] inline float flushSubnormal(float v) noexcept {
    return (std::abs(v) < 1e-20f) ? 0.0f : v;
}

// One-pole exponential smoother for continuous parameters to prevent zipper noise
class OnePoleSmoother {
public:
    void init(float initialValue, float timeConstantSeconds, std::uint32_t sampleRate) noexcept {
        currentValue_ = initialValue;
        targetValue_ = initialValue;
        if (timeConstantSeconds > 0.0f && sampleRate > 0) {
            alpha_ = 1.0f - std::exp(-1.0f / (timeConstantSeconds * static_cast<float>(sampleRate)));
        } else {
            alpha_ = 1.0f;
        }
    }

    void reset(float initialValue = 0.0f) noexcept {
        currentValue_ = initialValue;
        targetValue_ = initialValue;
    }

    void setTauMs(float tauMs, float sampleRate) noexcept {
        if (tauMs > 0.0f && sampleRate > 0.0f) {
            alpha_ = 1.0f - std::exp(-1.0f / ((tauMs * 0.001f) * sampleRate));
        } else {
            alpha_ = 1.0f;
        }
    }

    void setTarget(float target) noexcept {
        targetValue_ = target;
    }

    void snapTo(float value) noexcept {
        currentValue_ = value;
        targetValue_ = value;
    }

    float process(float target) noexcept {
        targetValue_ = target;
        return next();
    }

    [[nodiscard]] float next() noexcept {
        currentValue_ += alpha_ * (targetValue_ - currentValue_);
        currentValue_ = flushSubnormal(currentValue_);
        return currentValue_;
    }

    [[nodiscard]] float current() const noexcept {
        return currentValue_;
    }

private:
    float currentValue_{0.0f};
    float targetValue_{0.0f};
    float alpha_{1.0f};
};

// 2-point PolyBLEP band-limited oscillator (Välimäki & Huovilainen 2007)
// Suppresses aliasing > 40 dB across audio band with zero table lookups
class PolyBlepOscillator {
public:
    void reset(float phase = 0.0f) noexcept {
        phase_ = phase;
    }

    // Generates a band-limited sawtooth in [-1.0f, +1.0f] using internal phase
    [[nodiscard]] float processSaw(float freqHz, std::uint32_t sampleRate) noexcept {
        if (sampleRate == 0) return 0.0f;
        const float dt = std::clamp(freqHz / static_cast<float>(sampleRate), 0.0f, 0.45f);
        if (dt <= 0.0f) return 0.0f;

        // Naive rising sawtooth: 2 * phase - 1
        const float naiveSaw = 2.0f * phase_ - 1.0f;

        // 2-point PolyBLEP correction residual
        float blep = 0.0f;
        if (phase_ < dt) {
            const float x = phase_ / dt;
            blep = 2.0f * x - x * x - 1.0f;
        } else if (phase_ > 1.0f - dt) {
            const float x = (phase_ - 1.0f) / dt;
            blep = x * x + 2.0f * x + 1.0f;
        }

        const float out = naiveSaw - blep;

        // Advance phase
        phase_ += dt;
        if (phase_ >= 1.0f) {
            phase_ -= 1.0f;
        }

        return flushSubnormal(out);
    }

    // Generates band-limited sawtooth from externally driven radian phase and increment
    [[nodiscard]] float nextSample(float phaseRad, float phaseIncRad) noexcept {
        const float phaseNorm = std::clamp(phaseRad / kTwoPi, 0.0f, 1.0f);
        const float dt = std::clamp(phaseIncRad / kTwoPi, 0.0f, 0.45f);
        if (dt <= 0.0f) return 0.0f;

        const float naiveSaw = 2.0f * phaseNorm - 1.0f;
        float blep = 0.0f;
        if (phaseNorm < dt) {
            const float x = phaseNorm / dt;
            blep = 2.0f * x - x * x - 1.0f;
        } else if (phaseNorm > 1.0f - dt) {
            const float x = (phaseNorm - 1.0f) / dt;
            blep = x * x + 2.0f * x + 1.0f;
        }
        return flushSubnormal(naiveSaw - blep);
    }

    [[nodiscard]] float phase() const noexcept { return phase_; }

private:
    float phase_{0.0f};
};

// Vadim Zavalishin / Andy Simper Trapezoidal Topology-Preserving Transform (TPT) SVF 2-pole filter
// Inconditionally BIBO stable under arbitrary fast parameter modulation, zero delay feedback
class TptSvfFilter {
public:
    void reset() noexcept {
        s1_ = 0.0f;
        s2_ = 0.0f;
    }

    struct Output {
        float lp{0.0f};
        float bp{0.0f};
        float hp{0.0f};
    };

    [[nodiscard]] Output process(float x, float cutoffHz, float q, std::uint32_t sampleRate) noexcept {
        if (sampleRate == 0) return Output{x, 0.0f, 0.0f};

        // Clamp parameters for stability
        const float safeCutoff = std::clamp(cutoffHz, 20.0f, 0.45f * static_cast<float>(sampleRate));
        const float safeQ = std::clamp(q, 0.5f, 10.0f);
        const float R = 1.0f / (2.0f * safeQ); // damping in [0.05, 1.0]

        // Pre-warped cutoff
        const float g = std::tan((kPi * safeCutoff) / static_cast<float>(sampleRate));

        // Trapezoidal TPT coefficients
        const float a1 = 1.0f / (1.0f + g * (g + 2.0f * R));
        const float a2 = g * a1;
        const float a3 = g * a2;

        const float v3 = x - s2_;
        const float v1 = a1 * s1_ + a2 * v3;
        const float v2 = s2_ + a2 * s1_ + a3 * v3;
        const float hp = x - 2.0f * R * v1 - v2;

        // State update with subnormal flush
        s1_ = flushSubnormal(2.0f * v1 - s1_);
        s2_ = flushSubnormal(2.0f * v2 - s2_);

        return Output{
            .lp = flushSubnormal(v2),
            .bp = flushSubnormal(v1),
            .hp = flushSubnormal(hp)
        };
    }

    [[nodiscard]] float processLowpass(float x, float cutoffHz, float q, float sampleRate) noexcept {
        if (sampleRate <= 0.0f) return x;
        const float safeCutoff = std::clamp(cutoffHz, 20.0f, 0.45f * sampleRate);
        const float safeQ = std::clamp(q, 0.5f, 10.0f);
        const float R = 1.0f / (2.0f * safeQ);

        const float g = std::tan((kPi * safeCutoff) / sampleRate);
        const float a1 = 1.0f / (1.0f + g * (g + 2.0f * R));
        const float a2 = g * a1;
        const float a3 = g * a2;

        const float v3 = x - s2_;
        const float v1 = a1 * s1_ + a2 * v3;
        const float v2 = s2_ + a2 * s1_ + a3 * v3;

        s1_ = flushSubnormal(2.0f * v1 - s1_);
        s2_ = flushSubnormal(2.0f * v2 - s2_);

        return flushSubnormal(v2);
    }

private:
    float s1_{0.0f};
    float s2_{0.0f};
};

// Carson anti-aliasing bound for 2-operator FM synthesis:
// Bandwidth = 2 * (delta_f + f_m) = 2 * (I * f_m + f_m) = 2 * f_m * (I + 1)
// Required: f_c + f_m * (I + 1) <= 0.42 * f_s
[[nodiscard]] inline float carsonMaxModulationIndex(float carrierFreqHz, float modFreqHz, float sampleRate) noexcept {
    if (sampleRate <= 0.0f || modFreqHz <= 0.0f) return 0.0f;
    const float nyquistBound = 0.42f * sampleRate;
    if (carrierFreqHz >= nyquistBound) return 0.0f;
    const float maxI = ((nyquistBound - carrierFreqHz) / modFreqHz) - 1.0f;
    return std::clamp(maxI, 0.0f, 4.0f);
}

// Deterministic transient click noise generator for Pulso (15 ms) using pure stateless staffordMix13
[[nodiscard]] inline float pulsoClickSample(std::uint64_t eventId, std::uint32_t sampleIndex, std::uint8_t slotIndex) noexcept {
    const std::uint64_t h = core::staffordMix13(
        eventId ^ (static_cast<std::uint64_t>(sampleIndex) * 0x9E3779B97F4A7C15ULL) ^ static_cast<std::uint64_t>(slotIndex)
    );
    // Convert to uniform float in [-1.0f, +1.0f]
    const std::int32_t s32 = static_cast<std::int32_t>(h & 0xFFFFFFFFu);
    return static_cast<float>(s32) / 2147483648.0f;
}

} // namespace cymatica::audio
