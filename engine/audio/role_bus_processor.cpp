#include "role_bus_processor.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace cymatica::audio {

void RoleBusProcessor::init(std::uint32_t sampleRate) noexcept {
    sampleRate_ = (sampleRate > 0) ? sampleRate : 48000;

    // Micro-ducking attack (2 ms) and release (120 ms)
    duckingAttackAlpha_ = 1.0f - std::exp(-1.0f / (0.002f * static_cast<float>(sampleRate_)));
    duckingReleaseAlpha_ = 1.0f - std::exp(-1.0f / (0.120f * static_cast<float>(sampleRate_)));

    // Nominal delay times: L = 11 ms, R = 17 ms
    delayLengthL_ = std::clamp<std::size_t>(
        static_cast<std::size_t>(0.011f * static_cast<float>(sampleRate_)), 1, kMaxDelayFrames - 1
    );
    delayLengthR_ = std::clamp<std::size_t>(
        static_cast<std::size_t>(0.017f * static_cast<float>(sampleRate_)), 1, kMaxDelayFrames - 1
    );

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
}

void RoleBusProcessor::onStop(std::uint32_t declickFrames) noexcept {
    const std::size_t readIdxL = (delayWriteHead_ + kMaxDelayFrames - delayLengthL_) % kMaxDelayFrames;
    const std::size_t readIdxR = (delayWriteHead_ + kMaxDelayFrames - delayLengthR_) % kMaxDelayFrames;
    const float lastDelayedL = delayLineL_[readIdxL];
    const float lastDelayedR = delayLineR_[readIdxR];

    // Wipe circulating delay lines immediately to prevent echoes
    delayLineL_.fill(0.0f);
    delayLineR_.fill(0.0f);
    delayWriteHead_ = 0;
    duckingEnv_ = 0.0f;
    lastDuckingGain_ = 1.0f;
    for (auto& buf : busBuffers_) {
        buf.fill(0.0f);
    }

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
}

void RoleBusProcessor::accumulateRoleSample(MusicRole role, std::uint32_t frameIndex, float sampleL, float sampleR) noexcept {
    const std::size_t r = static_cast<std::size_t>(role);
    if (r >= kRoleCount || frameIndex >= kMaxBlockFrames) return;

    busBuffers_[r][frameIndex * 2] += sampleL;
    busBuffers_[r][frameIndex * 2 + 1] += sampleR;
}

void RoleBusProcessor::processAndMix(float* pDestInterleaved,
                                     std::uint32_t frameCount,
                                     const std::array<float, kRoleCount>& rolePitchHz,
                                     const std::array<uint32_t, kRoleCount>& roleActiveVoices,
                                     const std::array<float, kRoleCount>& roleTimbreParam,
                                     AudioTelemetryFrame& outTelemetry) noexcept {
    if (pDestInterleaved == nullptr || frameCount == 0) return;

    const std::uint32_t nFrames = std::min<std::uint32_t>(frameCount, static_cast<std::uint32_t>(kMaxBlockFrames));

    double sumSqPulso = 0.0;
    double sumSqCorpo = 0.0;
    double sumSqTrama = 0.0;
    double sumSqVettore = 0.0;
    double sumSqGlobal = 0.0;

    const float tramaWet = std::clamp(0.20f + 0.25f * roleTimbreParam[static_cast<size_t>(MusicRole::Trama)], 0.0f, 0.45f);

    for (std::uint32_t f = 0; f < nFrames; ++f) {
        // 1. Pulso
        const float inL_pulso = busBuffers_[static_cast<size_t>(MusicRole::Pulso)][f * 2];
        const float inR_pulso = busBuffers_[static_cast<size_t>(MusicRole::Pulso)][f * 2 + 1];
        const float pulsoGain = (roleTimbreParam[static_cast<size_t>(MusicRole::Pulso)] > 0.0f) ? kPulsoGain : 1.0f;
        const float outL_pulso = inL_pulso * pulsoGain;
        const float outR_pulso = inR_pulso * pulsoGain;

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
            const float duckFactor = std::clamp(duckingEnv_ / 0.25f, 0.0f, 1.0f);
            duckGain = 1.0f - (1.0f - kMinDuckingGain) * duckFactor;
        }
        lastDuckingGain_ = duckGain;

        // 2. Corpo (scaled by nominal gain + micro-ducking)
        const float inL_corpo = busBuffers_[static_cast<size_t>(MusicRole::Corpo)][f * 2];
        const float inR_corpo = busBuffers_[static_cast<size_t>(MusicRole::Corpo)][f * 2 + 1];
        const float corpoGain = (roleTimbreParam[static_cast<size_t>(MusicRole::Corpo)] > 0.0f) ? kCorpoGain : 1.0f;
        const float outL_corpo = inL_corpo * corpoGain * duckGain;
        const float outR_corpo = inR_corpo * corpoGain * duckGain;

        // 3. Trama (scaled by nominal gain + stereo micro-delay)
        const float inL_trama = busBuffers_[static_cast<size_t>(MusicRole::Trama)][f * 2];
        const float inR_trama = busBuffers_[static_cast<size_t>(MusicRole::Trama)][f * 2 + 1];
        const float tramaGain = (roleTimbreParam[static_cast<size_t>(MusicRole::Trama)] > 0.0f) ? kTramaGain : 1.0f;
        const float rawL_trama = inL_trama * tramaGain;
        const float rawR_trama = inR_trama * tramaGain;

        // Read stereo taps from preallocated delay ring
        const std::size_t readIdxL = (delayWriteHead_ + kMaxDelayFrames - delayLengthL_) % kMaxDelayFrames;
        const std::size_t readIdxR = (delayWriteHead_ + kMaxDelayFrames - delayLengthR_) % kMaxDelayFrames;
        const float delayedL = delayLineL_[readIdxL];
        const float delayedR = delayLineR_[readIdxR];

        // Write with bounded feedback <= 0.45f
        delayLineL_[delayWriteHead_] = flushSubnormal(rawL_trama + delayFeedback_ * delayedL);
        delayLineR_[delayWriteHead_] = flushSubnormal(rawR_trama + delayFeedback_ * delayedR);
        delayWriteHead_ = (delayWriteHead_ + 1) % kMaxDelayFrames;

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
        const float inL_vettore = busBuffers_[static_cast<size_t>(MusicRole::Vettore)][f * 2];
        const float inR_vettore = busBuffers_[static_cast<size_t>(MusicRole::Vettore)][f * 2 + 1];
        const float vettoreGain = (roleTimbreParam[static_cast<size_t>(MusicRole::Vettore)] > 0.0f) ? kVettoreGain : 1.0f;
        const float outL_vettore = inL_vettore * vettoreGain;
        const float outR_vettore = inR_vettore * vettoreGain;

        // Sum 4 roles into master destination
        const float mixL = outL_pulso + outL_corpo + outL_trama + outL_vettore;
        const float mixR = outR_pulso + outR_corpo + outR_trama + outR_vettore;

        pDestInterleaved[f * 2] = mixL;
        pDestInterleaved[f * 2 + 1] = mixR;

        // Energy accumulations
        sumSqPulso += static_cast<double>(outL_pulso * outL_pulso + outR_pulso * outR_pulso);
        sumSqCorpo += static_cast<double>(outL_corpo * outL_corpo + outR_corpo * outR_corpo);
        sumSqTrama += static_cast<double>(outL_trama * outL_trama + outR_trama * outR_trama);
        sumSqVettore += static_cast<double>(outL_vettore * outL_vettore + outR_vettore * outR_vettore);
        sumSqGlobal += static_cast<double>(mixL * mixL + mixR * mixR);
    }

    // Populate ChannelFrame telemetry
    const double invSamples = 1.0 / (2.0 * static_cast<double>(nFrames));

    outTelemetry.pulse.energy = static_cast<float>(std::sqrt(sumSqPulso * invSamples));
    outTelemetry.pulse.pitchHz = rolePitchHz[static_cast<size_t>(MusicRole::Pulso)];
    outTelemetry.pulse.density = static_cast<float>(roleActiveVoices[static_cast<size_t>(MusicRole::Pulso)]) / 2.0f;
    outTelemetry.pulse.confidence = (roleActiveVoices[static_cast<size_t>(MusicRole::Pulso)] > 0) ? 1.0f : 0.0f;

    outTelemetry.body.energy = static_cast<float>(std::sqrt(sumSqCorpo * invSamples));
    outTelemetry.body.pitchHz = rolePitchHz[static_cast<size_t>(MusicRole::Corpo)];
    outTelemetry.body.density = static_cast<float>(roleActiveVoices[static_cast<size_t>(MusicRole::Corpo)]) / 2.0f;
    outTelemetry.body.confidence = (roleActiveVoices[static_cast<size_t>(MusicRole::Corpo)] > 0) ? 1.0f : 0.0f;

    outTelemetry.texture.energy = static_cast<float>(std::sqrt(sumSqTrama * invSamples));
    outTelemetry.texture.pitchHz = rolePitchHz[static_cast<size_t>(MusicRole::Trama)];
    outTelemetry.texture.density = static_cast<float>(roleActiveVoices[static_cast<size_t>(MusicRole::Trama)]) / 2.0f;
    outTelemetry.texture.confidence = (roleActiveVoices[static_cast<size_t>(MusicRole::Trama)] > 0) ? 1.0f : 0.0f;

    outTelemetry.vector.energy = static_cast<float>(std::sqrt(sumSqVettore * invSamples));
    outTelemetry.vector.pitchHz = rolePitchHz[static_cast<size_t>(MusicRole::Vettore)];
    outTelemetry.vector.density = static_cast<float>(roleActiveVoices[static_cast<size_t>(MusicRole::Vettore)]) / 2.0f;
    outTelemetry.vector.confidence = (roleActiveVoices[static_cast<size_t>(MusicRole::Vettore)] > 0) ? 1.0f : 0.0f;

    outTelemetry.globalEnergy = static_cast<float>(std::sqrt(sumSqGlobal * invSamples));
}

} // namespace cymatica::audio
