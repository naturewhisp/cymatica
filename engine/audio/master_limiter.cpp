#include "master_limiter.h"

#include <algorithm>
#include <cmath>

namespace cymatica::audio {

void MasterLimiter::init(std::uint32_t sampleRate) noexcept {
    sampleRate_ = (sampleRate > 0) ? sampleRate : 48000;
    // 2 ms hold: floor(0.002 * sampleRate)
    holdFrames_ = std::max<std::uint32_t>(1u, sampleRate_ / 500u);
    // 50 ms release: alpha = 1 - exp(-1 / (0.050 * fs))
    releaseAlpha_ = 1.0f - std::exp(-1.0f / (0.050f * static_cast<float>(sampleRate_)));
    thresholdLinear_ = kThresholdLinear;
    reset();
}

void MasterLimiter::reset() noexcept {
    currentGain_ = 1.0f;
    holdRemaining_ = 0;
    maxGainReductionDb_ = 0.0f;
    anomalyCount_ = 0;
    historyHead_ = 0;
    historyL_.fill(0.0f);
    historyR_.fill(0.0f);
    prevOutL_ = 0.0f;
    prevOutR_ = 0.0f;
}

float MasterLimiter::estimatePeakCausal(float inL, float inR) noexcept {
    historyL_[historyHead_] = inL;
    historyR_[historyHead_] = inR;

    const float samplePeak = std::max(std::abs(inL), std::abs(inR));

    // Finite differences across recent causal history
    const std::size_t idx1 = (historyHead_ + kHistoryLength - 1) % kHistoryLength;
    const std::size_t idx2 = (historyHead_ + kHistoryLength - 2) % kHistoryLength;

    const float prevL = historyL_[idx1];
    const float prevR = historyR_[idx1];
    const float prev2L = historyL_[idx2];
    const float prev2R = historyR_[idx2];

    const float d1L = inL - prevL;
    const float d1R = inR - prevR;
    const float d2L = inL - 2.0f * prevL + prev2L;
    const float d2R = inR - 2.0f * prevR + prev2R;

    // Nyquist analytical curvature bound
    const float curvL = std::sqrt(inL * inL + 0.65f * d1L * d1L + 0.35f * d2L * d2L);
    const float curvR = std::sqrt(inR * inR + 0.65f * d1R * d1R + 0.35f * d2R * d2R);

    // 4-phase causal FIR inter-sample peak estimators (interpolated at -0.25, -0.50, -0.75 samples)
    const float firL_25 = std::abs(0.82f * inL + 0.28f * prevL - 0.10f * prev2L);
    const float firR_25 = std::abs(0.82f * inR + 0.28f * prevR - 0.10f * prev2R);

    const float firL_50 = std::abs(0.58f * inL + 0.58f * prevL - 0.16f * prev2L);
    const float firR_50 = std::abs(0.58f * inR + 0.58f * prevR - 0.16f * prev2R);

    const float firL_75 = std::abs(0.28f * inL + 0.82f * prevL - 0.10f * prev2L);
    const float firR_75 = std::abs(0.28f * inR + 0.82f * prevR - 0.10f * prev2R);

    float peakEst = samplePeak;
    peakEst = std::max(peakEst, std::max(curvL, curvR));
    peakEst = std::max(peakEst, std::max(firL_25, firR_25));
    peakEst = std::max(peakEst, std::max(firL_50, firR_50));
    peakEst = std::max(peakEst, std::max(firL_75, firR_75));

    return peakEst;
}

void MasterLimiter::processFrame(float& ioL, float& ioR) noexcept {
    // 1. Master Finite Guard (inconditional fail-closed)
    if (!std::isfinite(ioL)) {
        ioL = 0.0f;
        ++anomalyCount_;
    }
    if (!std::isfinite(ioR)) {
        ioR = 0.0f;
        ++anomalyCount_;
    }

    // 2. Hybrid Causal True-Peak Detection
    const float peakEst = estimatePeakCausal(ioL, ioR);

    // 3. Stereo-linked Target Gain Computation
    float targetGain = 1.0f;
    if (peakEst > thresholdLinear_) {
        targetGain = thresholdLinear_ / std::max(peakEst, 1e-12f);
    }

    // 4. Instantaneous Attack, Hold, and Exponential Release
    if (targetGain < currentGain_) {
        currentGain_ = targetGain;
        holdRemaining_ = holdFrames_;
    } else {
        if (holdRemaining_ > 0) {
            --holdRemaining_;
        } else {
            currentGain_ += releaseAlpha_ * (1.0f - currentGain_);
            currentGain_ = std::min(currentGain_, 1.0f);
        }
    }
    currentGain_ = flushSubnormal(currentGain_);

    // Track gain reduction in dB
    if (currentGain_ < 0.999f) {
        const float grDb = -20.0f * std::log10(std::max(currentGain_, 1e-6f));
        if (grDb > maxGainReductionDb_) {
            maxGainReductionDb_ = grDb;
        }
    }

    // 5. Apply Gain
    float outL = ioL * currentGain_;
    float outR = ioR * currentGain_;

    // 6. Slew & Intersample Anti-Overshoot Guard
    // Limits severe high-frequency step jumps near full-scale to prevent Gibbs ringing in D/A true-peak reconstruction
    constexpr float kMaxStep = 1.35f * kHardCeilingLinear;
    if (std::abs(outL) > 0.70f * kHardCeilingLinear && std::abs(outL - prevOutL_) > kMaxStep) {
        outL = prevOutL_ + std::copysign(kMaxStep, outL - prevOutL_);
    }
    if (std::abs(outR) > 0.70f * kHardCeilingLinear && std::abs(outR - prevOutR_) > kMaxStep) {
        outR = prevOutR_ + std::copysign(kMaxStep, outR - prevOutR_);
    }

    // 7. Hard Safety Ceiling Clamp
    outL = std::clamp(outL, -kHardCeilingLinear, +kHardCeilingLinear);
    outR = std::clamp(outR, -kHardCeilingLinear, +kHardCeilingLinear);

    prevOutL_ = outL;
    prevOutR_ = outR;
    historyHead_ = (historyHead_ + 1) % kHistoryLength;

    ioL = outL;
    ioR = outR;
}

void MasterLimiter::processBlock(float* pInterleaved, std::uint32_t frameCount) noexcept {
    if (pInterleaved == nullptr || frameCount == 0) return;

    for (std::uint32_t f = 0; f < frameCount; ++f) {
        processFrame(pInterleaved[f * 2], pInterleaved[f * 2 + 1]);
    }
}

} // namespace cymatica::audio
