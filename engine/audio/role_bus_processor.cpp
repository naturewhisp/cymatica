#include "role_bus_processor.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace cymatica::audio {

namespace {
// Nominal bus gain target for a role. Inc-1 legacy test voices (timbreParam == 0) keep unity bus gain so the
// Inc-1 amplitude contract and its tests remain valid; the switch between the two targets is smoothed with
// tau = 5 ms and is only re-evaluated while the role has active voices (held while the role is idle).
constexpr float nominalBusGain(std::size_t role, float timbreParam) noexcept {
    if (timbreParam <= 0.0f) return 1.0f;
    switch (role) {
        case 0: return RoleBusProcessor::kPulsoGain;
        case 1: return RoleBusProcessor::kCorpoGain;
        case 2: return RoleBusProcessor::kTramaGain;
        case 3: return RoleBusProcessor::kVettoreGain;
        default: return 1.0f;
    }
}

constexpr float tramaWetTarget(float timbreParam) noexcept {
    return std::clamp(RoleBusProcessor::kDefaultTramaWet + 0.25f * timbreParam, 0.0f, 0.45f);
}
} // namespace

void RoleBusProcessor::init(std::uint32_t sampleRate) noexcept {
    sampleRate_ = (sampleRate > 0) ? sampleRate : 48000;
    const float fs = static_cast<float>(sampleRate_);

    // Micro-ducking attack (2 ms) and release (120 ms)
    duckingAttackAlpha_ = 1.0f - std::exp(-1.0f / (0.002f * fs));
    duckingReleaseAlpha_ = 1.0f - std::exp(-1.0f / (0.120f * fs));

    // Nominal delay times: L = 11 ms, R = 17 ms
    delayLengthL_ = std::clamp<std::size_t>(static_cast<std::size_t>(0.011f * fs), 1, kMaxDelayFrames - 1);
    delayLengthR_ = std::clamp<std::size_t>(static_cast<std::size_t>(0.017f * fs), 1, kMaxDelayFrames - 1);

    for (auto& sm : busGainSmoothers_) {
        sm.setTauMs(kBusSmoothingTauMs, fs);
    }
    tramaWetSmoother_.setTauMs(kBusSmoothingTauMs, fs);

    reset();
}

void RoleBusProcessor::reset() noexcept {
    duckingEnv_ = 0.0f;
    lastDuckingGain_ = 1.0f;
    delayWriteHead_ = 0;
    delayLineL_.fill(0.0f);
    delayLineR_.fill(0.0f);
    delayTail_.reset(96);
    for (auto& buf : busBuffers_) {
        buf.fill(0.0f);
    }
    tramaNoDelayBuffer_.fill(0.0f);
    for (auto& sm : busGainSmoothers_) {
        sm.snapTo(1.0f);
    }
    tramaWetSmoother_.snapTo(kDefaultTramaWet);
    roleInputSilentPrev_.fill(true);
    beginTelemetryWindow();
}

void RoleBusProcessor::onStop(std::uint32_t declickFrames) noexcept {
    const float lastDelayedL = delayLineL_[delayReadIndex(delayLengthL_)];
    const float lastDelayedR = delayLineR_[delayReadIndex(delayLengthR_)];

    // Wipe circulating delay lines immediately to prevent echoes
    delayLineL_.fill(0.0f);
    delayLineR_.fill(0.0f);
    delayWriteHead_ = 0;
    for (auto& buf : busBuffers_) {
        buf.fill(0.0f);
    }
    tramaNoDelayBuffer_.fill(0.0f);

    // Capture instantaneous delay output into anti-click tail
    const bool hasTail = (std::abs(lastDelayedL) > 1e-6f || std::abs(lastDelayedR) > 1e-6f);
    if (hasTail) {
        delayTail_.startAmplitude[0] = lastDelayedL;
        delayTail_.startAmplitude[1] = lastDelayedR;
        delayTail_.totalSamples = (declickFrames > 0) ? declickFrames : 96;
        delayTail_.remainingSamples = delayTail_.totalSamples;
        delayTail_.active = true;
    } else {
        delayTail_.reset(declickFrames);
    }
}

void RoleBusProcessor::beginBlock(std::uint32_t frameCount) noexcept {
    const std::size_t samplesToClear = static_cast<std::size_t>(std::min<std::uint32_t>(frameCount, static_cast<std::uint32_t>(kMaxBlockFrames))) * 2;
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        std::memset(busBuffers_[r].data(), 0, samplesToClear * sizeof(float));
    }
    std::memset(tramaNoDelayBuffer_.data(), 0, samplesToClear * sizeof(float));
}

void RoleBusProcessor::accumulateRoleSample(MusicRole role, std::uint32_t frameIndex, float sampleL, float sampleR,
                                            bool feedsDelay) noexcept {
    const std::size_t r = static_cast<std::size_t>(role);
    if (r >= kRoleCount || frameIndex >= kMaxBlockFrames) return;

    if (!feedsDelay && role == MusicRole::Trama) {
        tramaNoDelayBuffer_[frameIndex * 2] += sampleL;
        tramaNoDelayBuffer_[frameIndex * 2 + 1] += sampleR;
        return;
    }
    busBuffers_[r][frameIndex * 2] += sampleL;
    busBuffers_[r][frameIndex * 2 + 1] += sampleR;
}

void RoleBusProcessor::beginTelemetryWindow() noexcept {
    windowSumSq_.fill(0.0);
    windowPeak_.fill(0.0f);
    windowSumSqGlobal_ = 0.0;
    windowFrames_ = 0;
    windowMinDuckingGain_ = 1.0f;
}

void RoleBusProcessor::processAndMix(float* pDestInterleaved,
                                     std::uint32_t frameCount,
                                     const std::array<float, kRoleCount>& rolePitchHz,
                                     const std::array<uint32_t, kRoleCount>& roleActiveVoices,
                                     const std::array<float, kRoleCount>& roleTimbreParam,
                                     AudioTelemetryFrame& outTelemetry) noexcept {
    if (pDestInterleaved == nullptr || frameCount == 0) return;

    const std::uint32_t nFrames = std::min<std::uint32_t>(frameCount, static_cast<std::uint32_t>(kMaxBlockFrames));

    constexpr std::size_t kP = static_cast<std::size_t>(MusicRole::Pulso);
    constexpr std::size_t kC = static_cast<std::size_t>(MusicRole::Corpo);
    constexpr std::size_t kT = static_cast<std::size_t>(MusicRole::Trama);
    constexpr std::size_t kV = static_cast<std::size_t>(MusicRole::Vettore);

    // Bus gain / wet targets are re-evaluated only while the role sounds; an idle role holds its last target
    // so a ringing delay or a decaying tail never sees a gain step when the role goes idle (ADR-0004 §2.5).
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        if (roleActiveVoices[r] == 0) continue;
        const float target = nominalBusGain(r, roleTimbreParam[r]);
        if (roleInputSilentPrev_[r]) {
            busGainSmoothers_[r].snapTo(target); // bus input was exactly zero: no audible step
        } else {
            busGainSmoothers_[r].setTarget(target);
        }
    }
    if (roleActiveVoices[kT] > 0) {
        tramaWetSmoother_.setTarget(tramaWetTarget(roleTimbreParam[kT]));
    }

    const float* busP = busBuffers_[kP].data();
    const float* busC = busBuffers_[kC].data();
    const float* busT = busBuffers_[kT].data();
    const float* busV = busBuffers_[kV].data();
    const float* busTNoDelay = tramaNoDelayBuffer_.data();

    std::array<bool, kRoleCount> inputNonZero{false, false, false, false};
    std::array<double, kRoleCount> sumSq{0.0, 0.0, 0.0, 0.0};
    std::array<float, kRoleCount> peak = windowPeak_;
    double sumSqGlobal = 0.0;
    float minDuck = windowMinDuckingGain_;

    for (std::uint32_t f = 0; f < nFrames; ++f) {
        const std::size_t iL = static_cast<std::size_t>(f) * 2;
        const std::size_t iR = iL + 1;

        const float gainP = busGainSmoothers_[kP].next();
        const float gainC = busGainSmoothers_[kC].next();
        const float gainT = busGainSmoothers_[kT].next();
        const float gainV = busGainSmoothers_[kV].next();
        const float tramaWet = tramaWetSmoother_.next();

        // 1. Pulso
        const float inL_pulso = busP[iL];
        const float inR_pulso = busP[iR];
        const float outL_pulso = inL_pulso * gainP;
        const float outR_pulso = inR_pulso * gainP;

        // Envelope follower for micro-ducking on Corpo
        const float pulsoAmp = std::max(std::abs(outL_pulso), std::abs(outR_pulso));
        if (pulsoAmp > duckingEnv_) {
            duckingEnv_ += duckingAttackAlpha_ * (pulsoAmp - duckingEnv_);
        } else {
            duckingEnv_ += duckingReleaseAlpha_ * (pulsoAmp - duckingEnv_);
        }
        duckingEnv_ = flushSubnormal(duckingEnv_);

        float duckGain = 1.0f;
        if (duckingEnabled_) {
            const float duckFactor = std::clamp(duckingEnv_ / kDuckingFullScaleLevel, 0.0f, 1.0f);
            duckGain = 1.0f - (1.0f - kMinDuckingGain) * duckFactor;
        }
        lastDuckingGain_ = duckGain;
        minDuck = std::min(minDuck, duckGain);

        // 2. Corpo (scaled by nominal gain + micro-ducking)
        const float inL_corpo = busC[iL];
        const float inR_corpo = busC[iR];
        const float outL_corpo = inL_corpo * gainC * duckGain;
        const float outR_corpo = inR_corpo * gainC * duckGain;

        // 3. Trama (scaled by nominal gain + stereo micro-delay)
        const float inL_trama = busT[iL];
        const float inR_trama = busT[iR];
        const float inL_tramaNoDelay = busTNoDelay[iL];
        const float inR_tramaNoDelay = busTNoDelay[iR];
        const float sendL_trama = inL_trama * gainT;
        const float sendR_trama = inR_trama * gainT;
        const float rawL_trama = sendL_trama + inL_tramaNoDelay * gainT;
        const float rawR_trama = sendR_trama + inR_tramaNoDelay * gainT;

        float delayedL = 0.0f;
        float delayedR = 0.0f;
        if (!delayBypassed_) {
            // Read stereo taps from preallocated delay ring
            delayedL = delayLineL_[delayReadIndex(delayLengthL_)];
            delayedR = delayLineR_[delayReadIndex(delayLengthR_)];

            // Write with bounded feedback <= 0.45f
            delayLineL_[delayWriteHead_] = flushSubnormal(sendL_trama + delayFeedback_ * delayedL);
            delayLineR_[delayWriteHead_] = flushSubnormal(sendR_trama + delayFeedback_ * delayedR);
            if (++delayWriteHead_ >= kMaxDelayFrames) {
                delayWriteHead_ = 0;
            }
        }

        float outL_trama = (1.0f - tramaWet) * rawL_trama + tramaWet * delayedL;
        float outR_trama = (1.0f - tramaWet) * rawR_trama + tramaWet * delayedR;

        // Apply decaying delay de-click tail if active during transport stop
        if (delayTail_.active) {
            const float factor = static_cast<float>(delayTail_.remainingSamples) /
                                 static_cast<float>(delayTail_.totalSamples);
            outL_trama += delayTail_.startAmplitude[0] * factor * tramaWet;
            outR_trama += delayTail_.startAmplitude[1] * factor * tramaWet;
            if (--delayTail_.remainingSamples == 0) {
                delayTail_.active = false;
            }
        }

        // 4. Vettore
        const float inL_vettore = busV[iL];
        const float inR_vettore = busV[iR];
        const float outL_vettore = inL_vettore * gainV;
        const float outR_vettore = inR_vettore * gainV;

        // Sum 4 roles into master destination
        const float mixL = outL_pulso + outL_corpo + outL_trama + outL_vettore;
        const float mixR = outR_pulso + outR_corpo + outR_trama + outR_vettore;

        pDestInterleaved[iL] = mixL;
        pDestInterleaved[iR] = mixR;

        inputNonZero[kP] = inputNonZero[kP] || (inL_pulso != 0.0f) || (inR_pulso != 0.0f);
        inputNonZero[kC] = inputNonZero[kC] || (inL_corpo != 0.0f) || (inR_corpo != 0.0f);
        inputNonZero[kT] = inputNonZero[kT] || (inL_trama != 0.0f) || (inR_trama != 0.0f) ||
                           (inL_tramaNoDelay != 0.0f) || (inR_tramaNoDelay != 0.0f);
        inputNonZero[kV] = inputNonZero[kV] || (inL_vettore != 0.0f) || (inR_vettore != 0.0f);

        // Energy and peak accumulations
        sumSq[kP] += static_cast<double>(outL_pulso * outL_pulso + outR_pulso * outR_pulso);
        sumSq[kC] += static_cast<double>(outL_corpo * outL_corpo + outR_corpo * outR_corpo);
        sumSq[kT] += static_cast<double>(outL_trama * outL_trama + outR_trama * outR_trama);
        sumSq[kV] += static_cast<double>(outL_vettore * outL_vettore + outR_vettore * outR_vettore);
        sumSqGlobal += static_cast<double>(mixL * mixL + mixR * mixR);
        peak[kP] = std::max(peak[kP], std::max(std::abs(outL_pulso), std::abs(outR_pulso)));
        peak[kC] = std::max(peak[kC], std::max(std::abs(outL_corpo), std::abs(outR_corpo)));
        peak[kT] = std::max(peak[kT], std::max(std::abs(outL_trama), std::abs(outR_trama)));
        peak[kV] = std::max(peak[kV], std::max(std::abs(outL_vettore), std::abs(outR_vettore)));
    }

    for (std::size_t r = 0; r < kRoleCount; ++r) {
        roleInputSilentPrev_[r] = !inputNonZero[r];
        windowSumSq_[r] += sumSq[r];
    }
    windowPeak_ = peak;
    windowSumSqGlobal_ += sumSqGlobal;
    windowFrames_ += nFrames;
    windowMinDuckingGain_ = minDuck;

    // Populate ChannelFrame telemetry over the whole telemetry window accumulated so far
    const double invSamples = (windowFrames_ > 0) ? 1.0 / (2.0 * static_cast<double>(windowFrames_)) : 0.0;
    const auto fillChannel = [&](ChannelFrame& ch, std::size_t r) noexcept {
        ch.energy = static_cast<float>(std::sqrt(windowSumSq_[r] * invSamples));
        ch.pitchHz = rolePitchHz[r];
        ch.density = static_cast<float>(roleActiveVoices[r]) / 2.0f;
        ch.confidence = (roleActiveVoices[r] > 0) ? 1.0f : 0.0f;
    };
    fillChannel(outTelemetry.pulse, kP);
    fillChannel(outTelemetry.body, kC);
    fillChannel(outTelemetry.texture, kT);
    fillChannel(outTelemetry.vector, kV);
    outTelemetry.globalEnergy = static_cast<float>(std::sqrt(windowSumSqGlobal_ * invSamples));
}

} // namespace cymatica::audio
