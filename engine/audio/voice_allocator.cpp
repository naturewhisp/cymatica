#include "voice_allocator.h"
#include "../core/unsigned_math.h"

#include <algorithm>
#include <cmath>

namespace cymatica::audio {

namespace {
constexpr float roleBusGain(MusicRole role, float timbreParam) noexcept {
    if (timbreParam == 0.0f) return 1.0f;
    switch (role) {
        case MusicRole::Pulso:   return RoleBusProcessor::kPulsoGain;
        case MusicRole::Corpo:   return RoleBusProcessor::kCorpoGain;
        case MusicRole::Trama:   return RoleBusProcessor::kTramaGain;
        case MusicRole::Vettore: return RoleBusProcessor::kVettoreGain;
    }
    return 1.0f;
}
}

VoiceAllocator::VoiceAllocator() noexcept {
    for (std::size_t i = 0; i < kTotalSlots; ++i) {
        slots_[i].slotIndex = static_cast<std::uint8_t>(i);
        slots_[i].role = static_cast<MusicRole>(i / kSlotsPerRole);
    }
}

void VoiceAllocator::init(std::uint32_t sampleRate) noexcept {
    sampleRate_ = (sampleRate > 0) ? sampleRate : 48000;
    // 2 ms physical de-click ramp: floor(0.002 * fs_device) -> 96 @ 48 kHz, 88 @ 44.1 kHz
    declickFrames_ = std::max<std::uint32_t>(1u, sampleRate_ / 500u);
    roleBusProcessor_.init(sampleRate_);
    masterLimiter_.init(sampleRate_);
    reset();
}

void VoiceAllocator::reset() noexcept {
    const std::uint32_t attackFrames = std::max<std::uint32_t>(1u, sampleRate_ / 1000u); // 1 ms
    const std::uint32_t decayFrames = std::max<std::uint32_t>(1u, sampleRate_ / 100u);   // 10 ms
    for (auto& slot : slots_) {
        slot.attackSamples = attackFrames;
        slot.decaySamples = decayFrames;
        slot.reset(declickFrames_);
    }
    roleBusProcessor_.reset();
    masterLimiter_.reset();
    transportRamp_ = TransportRamp::None;
    transportRampLevel_ = 1.0f;
    transportRampStep_ = 0.0f;
    transportRampRemaining_ = 0;
}

int VoiceAllocator::selectSlotForRole(MusicRole role) noexcept {
    if (static_cast<std::size_t>(role) >= kRoleCount) {
        return -1;
    }
    const std::size_t s0 = roleFirstSlot(role);
    const std::size_t s1 = s0 + 1;

    // Rule 0: Free (Idle) slot. Slot 0 prioritised before 1.
    if (slots_[s0].state == VoiceState::Idle) {
        return static_cast<int>(s0);
    }
    if (slots_[s1].state == VoiceState::Idle) {
        return static_cast<int>(s1);
    }

    // Rule 1: Slot in Release.
    const bool s0InRelease = (slots_[s0].state == VoiceState::Release);
    const bool s1InRelease = (slots_[s1].state == VoiceState::Release);

    if (s0InRelease && !s1InRelease) {
        return static_cast<int>(s0);
    }
    if (s1InRelease && !s0InRelease) {
        return static_cast<int>(s1);
    }
    if (s0InRelease && s1InRelease) {
        // Sanitize non-finite envelope levels before comparison (ADR-0003 D-M2-04 §5)
        if (!std::isfinite(slots_[s0].envelopeLevel)) {
            slots_[s0].envelopeLevel = 0.0f;
            ++dspAnomalyCount_;
        }
        if (!std::isfinite(slots_[s1].envelopeLevel)) {
            slots_[s1].envelopeLevel = 0.0f;
            ++dspAnomalyCount_;
        }
        const float env0 = std::abs(slots_[s0].envelopeLevel);
        const float env1 = std::abs(slots_[s1].envelopeLevel);
        if (env0 < env1) {
            return static_cast<int>(s0);
        }
        if (env1 < env0) {
            return static_cast<int>(s1);
        }
        if (slots_[s0].startFrame <= slots_[s1].startFrame) {
            return static_cast<int>(s0);
        }
        return static_cast<int>(s1);
    }

    // Rule 2: Voice Stealing among active voices (Attack, Decay, Sustain).
    // Pick older voice (smaller startFrame), breaking tie by lower slot index (s0 before s1).
    if (slots_[s0].startFrame <= slots_[s1].startFrame) {
        return static_cast<int>(s0);
    }
    return static_cast<int>(s1);
}

int VoiceAllocator::onNoteOn(const ScheduledAudioEvent& event) noexcept {
    const int idx = selectSlotForRole(event.role);
    if (idx < 0 || idx >= static_cast<int>(kTotalSlots)) {
        return -1;
    }

    VoiceSlot& slot = slots_[static_cast<std::size_t>(idx)];

    // Steal tail snapshot & voice stealing accounting
    if (slot.state != VoiceState::Idle) {
        if (slot.state != VoiceState::Release) {
            // Rule 2 voice steal on active voice
            ++voiceDropCount_;
        }

        // Capture current output into dedicated steal tail with 2 ms linear fade-out
        float curAmp = slot.currentVoiceOutput();
        if (!std::isfinite(curAmp)) {
            curAmp = 0.0f;
            ++dspAnomalyCount_;
        }

        if (slot.stealTail.active) {
            // Rapid successive steals (< 2 ms): snapshot composite sum without pre-master clamp
            const std::uint32_t tailLen = (slot.stealTail.totalSamples > 0)
                ? slot.stealTail.totalSamples
                : declickFrames_;
            const float factor = static_cast<float>(slot.stealTail.remainingSamples) /
                                 static_cast<float>(tailLen);
            float tailL = slot.stealTail.startAmplitude[0] * factor;
            float tailR = slot.stealTail.startAmplitude[1] * factor;
            if (!std::isfinite(tailL)) {
                tailL = 0.0f;
                ++dspAnomalyCount_;
            }
            if (!std::isfinite(tailR)) {
                tailR = 0.0f;
                ++dspAnomalyCount_;
            }

            slot.stealTail.startAmplitude[0] = tailL + curAmp;
            slot.stealTail.startAmplitude[1] = tailR + curAmp;
            slot.stealTail.totalSamples = declickFrames_;
            slot.stealTail.remainingSamples = declickFrames_;
        } else {
            slot.stealTail.startAmplitude[0] = curAmp;
            slot.stealTail.startAmplitude[1] = curAmp;
            slot.stealTail.totalSamples = declickFrames_;
            slot.stealTail.remainingSamples = declickFrames_;
            slot.stealTail.active = true;
        }
    }

    // Initialize slot for new note with defensive input sanitization
    ++slot.generationToken;
    slot.role = event.role;
    slot.eventId = event.eventId;
    slot.transportEpoch = event.transportEpoch;
    slot.startFrame = event.sampleFrame;
    slot.durationFrames = event.durationFrames;

    float pitch = 220.0f;
    if (!std::isfinite(event.valueA)) {
        ++dspAnomalyCount_;
    } else if (event.valueA > 0.0f) {
        const float maxPitch = std::max(1.0f, 0.45f * static_cast<float>(sampleRate_));
        pitch = std::clamp(event.valueA, 1.0f, maxPitch);
    }
    slot.pitchHz = pitch;

    float vel = 1.0f;
    if (!std::isfinite(event.valueB)) {
        ++dspAnomalyCount_;
    } else if (event.valueB >= 0.0f) {
        vel = std::min(event.valueB, 1.0f);
    }
    slot.velocity = vel;

    if (!std::isfinite(event.timbreParam)) {
        slot.timbreParam = 0.0f;
        ++dspAnomalyCount_;
    } else {
        slot.timbreParam = std::clamp(event.timbreParam, 0.0f, 1.0f);
    }

    slot.releaseSamples = declickFrames_;
    slot.phase = 0.0f;
    slot.phaseIncrement = (kTwoPi * slot.pitchHz) / static_cast<float>(sampleRate_);
    slot.state = VoiceState::Attack;
    slot.stageFrame = 0;
    slot.envelopeLevel = 0.0f;
    slot.releaseStartLevel = 0.0f;
    slot.noteFrameIndex = 0;
    slot.pitchSweepEnv = 1.0f;
    slot.lastOutput[0] = 0.0f;
    slot.lastOutput[1] = 0.0f;

    const float fs = static_cast<float>(sampleRate_);
    if (slot.timbreParam == 0.0f) {
        // Inc-1 legacy/test backward compatibility: 1 ms attack, 10 ms decay, sustain 0.8f
        slot.attackSamples = std::max<std::uint32_t>(1u, sampleRate_ / 1000u);
        slot.decaySamples = std::max<std::uint32_t>(1u, sampleRate_ / 100u);
        slot.sustainLevel = 0.8f;
    } else {
        switch (slot.role) {
            case MusicRole::Pulso: {
                // Percussive kick envelope: 1 ms attack, 50..120 ms decay, 0 sustain
                slot.attackSamples = std::max<std::uint32_t>(1u, sampleRate_ / 1000u);
                const float decaySec = 0.050f + 0.070f * slot.timbreParam;
                slot.decaySamples = std::max<std::uint32_t>(1u, static_cast<std::uint32_t>(decaySec * fs));
                slot.sustainLevel = 0.0f;
                break;
            }
            case MusicRole::Corpo: {
                // Warm subtractive pad: 15 ms attack, 100 ms decay, 0.75f sustain
                slot.attackSamples = std::max<std::uint32_t>(1u, static_cast<std::uint32_t>(0.015f * fs));
                slot.decaySamples = std::max<std::uint32_t>(1u, static_cast<std::uint32_t>(0.100f * fs));
                slot.sustainLevel = 0.75f;
                slot.osc.reset();
                slot.filter.reset();
                break;
            }
            case MusicRole::Trama: {
                // 2-op FM texture: 20 ms attack, 120 ms decay, 0.65f sustain
                slot.attackSamples = std::max<std::uint32_t>(1u, static_cast<std::uint32_t>(0.020f * fs));
                slot.decaySamples = std::max<std::uint32_t>(1u, static_cast<std::uint32_t>(0.120f * fs));
                slot.sustainLevel = 0.65f;
                slot.fmModPhase = 0.0f;
                break;
            }
            case MusicRole::Vettore: {
                // Agile lead: 5 ms attack, 50 ms decay, 0.85f sustain
                slot.attackSamples = std::max<std::uint32_t>(1u, static_cast<std::uint32_t>(0.005f * fs));
                slot.decaySamples = std::max<std::uint32_t>(1u, static_cast<std::uint32_t>(0.050f * fs));
                slot.sustainLevel = 0.85f;
                slot.osc.reset();
                slot.filter.reset();
                slot.pitchSmoother.reset(slot.pitchHz);
                slot.pitchSmoother.setTauMs(30.0f, fs);
                slot.vibratoPhase = 0.0f;
                break;
            }
        }
    }

    return idx;
}

int VoiceAllocator::onNoteOff(const ScheduledAudioEvent& event) noexcept {
    if (static_cast<std::size_t>(event.role) >= kRoleCount) {
        return -1;
    }
    const std::size_t s0 = roleFirstSlot(event.role);
    const std::size_t s1 = s0 + 1;

    int matched = -1;
    if (slots_[s0].state != VoiceState::Idle && slots_[s0].state != VoiceState::Release &&
        slots_[s0].eventId == event.eventId) {
        matched = static_cast<int>(s0);
    } else if (slots_[s1].state != VoiceState::Idle && slots_[s1].state != VoiceState::Release &&
               slots_[s1].eventId == event.eventId) {
        matched = static_cast<int>(s1);
    }

    if (matched >= 0) {
        auto& slot = slots_[static_cast<std::size_t>(matched)];
        slot.state = VoiceState::Release;
        slot.stageFrame = 0;
        slot.releaseStartLevel = slot.envelopeLevel;
        return matched;
    }

    // Unmatched / stale NoteOff ignored safely
    return -1;
}

void VoiceAllocator::checkLifetimes(std::uint64_t currentLogicalFrame) noexcept {
    for (auto& slot : slots_) {
        if (slot.state == VoiceState::Idle || slot.state == VoiceState::Release) {
            continue;
        }

        if (slot.durationFrames > 0) {
            // Note with explicit duration: failsafe timeout triggers release (saturating against overflow)
            const std::uint64_t expireFrame =
                core::saturatingAdd(slot.startFrame, static_cast<std::uint64_t>(slot.durationFrames));
            if (currentLogicalFrame >= expireFrame) {
                slot.state = VoiceState::Release;
                slot.stageFrame = 0;
                slot.releaseStartLevel = slot.envelopeLevel;
            }
        } else {
            // Open pad (durationFrames == 0): conservative safety backstop
            const std::uint64_t leaseExpireFrame =
                core::saturatingAdd(slot.startFrame, static_cast<std::uint64_t>(kMaxLeaseDurationFrames));
            if (currentLogicalFrame >= leaseExpireFrame) {
                slot.state = VoiceState::Release;
                slot.stageFrame = 0;
                slot.releaseStartLevel = slot.envelopeLevel;
                ++orphanNoteRecoveredCount_;
            }
        }
    }
}

void VoiceAllocator::onStart() noexcept {
    transportRamp_ = TransportRamp::None;
    transportRampLevel_ = 1.0f;
    transportRampStep_ = 0.0f;
    transportRampRemaining_ = 0;
}

void VoiceAllocator::onPause() noexcept {
    transportRamp_ = TransportRamp::FadingOut;
    transportRampRemaining_ = declickFrames_;
    transportRampStep_ = (transportRampLevel_ > 0.0f && declickFrames_ > 0)
        ? (transportRampLevel_ / static_cast<float>(declickFrames_))
        : 0.0f;
}

void VoiceAllocator::onResume() noexcept {
    transportRamp_ = TransportRamp::FadingIn;
    transportRampRemaining_ = declickFrames_;
    transportRampStep_ = (declickFrames_ > 0)
        ? ((1.0f - transportRampLevel_) / static_cast<float>(declickFrames_))
        : 1.0f;
}

void VoiceAllocator::onStop() noexcept {
    // 1. Capture each active voice + steal tail (scaled by current transportRampLevel_)
    //    into static steal tail for a single linear 2 ms physical de-click (ADR-0003 D-M2-03/04)
    const float effTransportGain = std::clamp(transportRampLevel_, 0.0f, 1.0f);

    for (auto& slot : slots_) {
        float voiceOut = slot.currentVoiceOutput();
        if (!std::isfinite(voiceOut)) {
            voiceOut = 0.0f;
            ++dspAnomalyCount_;
        }
        float curL = voiceOut;
        float curR = voiceOut;

        if (slot.stealTail.active) {
            const std::uint32_t tailLen = (slot.stealTail.totalSamples > 0)
                ? slot.stealTail.totalSamples
                : declickFrames_;
            const float factor = static_cast<float>(slot.stealTail.remainingSamples) /
                                 static_cast<float>(tailLen);
            curL += slot.stealTail.startAmplitude[0] * factor;
            curR += slot.stealTail.startAmplitude[1] * factor;
        }
        curL *= effTransportGain;
        curR *= effTransportGain;

        if (!std::isfinite(curL)) {
            curL = 0.0f;
            ++dspAnomalyCount_;
        }
        if (!std::isfinite(curR)) {
            curR = 0.0f;
            ++dspAnomalyCount_;
        }

        const bool hasTail = (std::abs(curL) > 1e-6f || std::abs(curR) > 1e-6f);

        // Reset slot synthesis state to Idle immediately (Spec §16, ADR-0003)
        slot.state = VoiceState::Idle;
        slot.eventId = 0;
        slot.transportEpoch = 1;
        ++slot.generationToken;
        slot.startFrame = 0;
        slot.durationFrames = 0;
        slot.envelopeLevel = 0.0f;
        slot.releaseStartLevel = 0.0f;
        slot.stageFrame = 0;
        slot.releaseSamples = declickFrames_;
        slot.phase = 0.0f;
        slot.phaseIncrement = 0.0f;

        // Maintain physical tail in preallocated static steal tail
        if (hasTail) {
            slot.stealTail.startAmplitude[0] = curL;
            slot.stealTail.startAmplitude[1] = curR;
            slot.stealTail.totalSamples = declickFrames_;
            slot.stealTail.remainingSamples = declickFrames_;
            slot.stealTail.active = true;
        } else {
            slot.stealTail.reset(declickFrames_);
        }
    }

    // 2. Restore transport ramp to unity so steal tails decay linearly (1 - k/N)
    //    and subsequent Start + NoteOn is never muted.
    transportRamp_ = TransportRamp::None;
    transportRampLevel_ = 1.0f;
    transportRampStep_ = 0.0f;
    transportRampRemaining_ = 0;
}

std::uint32_t VoiceAllocator::activeVoiceCount() const noexcept {
    std::uint32_t count = 0;
    for (const auto& slot : slots_) {
        if (slot.state != VoiceState::Idle) {
            ++count;
        }
    }
    return count;
}

void VoiceAllocator::renderBlock(float* pOut, std::uint32_t frameCount, std::uint64_t currentLogicalFrame,
                                 bool isTransportPaused, bool accumulate) noexcept {
    if (pOut == nullptr || frameCount == 0) return;

    if (!accumulate) {
        for (std::size_t i = 0; i < static_cast<std::size_t>(frameCount) * 2; ++i) {
            pOut[i] = 0.0f;
        }
    }

    const float fs = static_cast<float>(sampleRate_);
    std::uint32_t processedFrames = 0;

    while (processedFrames < frameCount) {
        const std::uint32_t subFrames = std::min(frameCount - processedFrames, static_cast<std::uint32_t>(kMaxBlockFrames));
        roleBusProcessor_.beginBlock(subFrames);

        for (std::uint32_t f = 0; f < subFrames; ++f) {
            const std::uint32_t totalF = processedFrames + f;
            std::uint64_t logicalOffset = totalF;
            if (sampleRate_ != 48000 && sampleRate_ > 0) {
                logicalOffset = core::multiplyDivide(
                    static_cast<std::uint64_t>(totalF), 48000ULL, static_cast<std::uint64_t>(sampleRate_)).value;
            }
            const std::uint64_t sampleLogicalFrame = isTransportPaused
                ? currentLogicalFrame
                : core::saturatingAdd(currentLogicalFrame, logicalOffset);

            // Check failsafe lifetimes at sample boundary
            if (!isTransportPaused) {
                checkLifetimes(sampleLogicalFrame);
            }

            for (auto& slot : slots_) {
                float sampleL = 0.0f;
                float sampleR = 0.0f;

                if (slot.state != VoiceState::Idle) {
                    float s = 0.0f;

                    if (slot.timbreParam == 0.0f) {
                        // Inc-1 legacy sine test mode for all roles
                        s = std::sin(slot.phase);
                        sampleL = s * slot.velocity * slot.envelopeLevel * kNominalVoiceScale;
                        sampleR = sampleL;
                    } else {
                        switch (slot.role) {
                            case MusicRole::Pulso: {
                                // Percussive kick + transient click
                                const float tauP = 0.015f * fs;
                                const float sweepFactor = std::exp(-static_cast<float>(slot.noteFrameIndex) / tauP);
                                const float instFreq = slot.pitchHz * (1.0f + 1.5f * sweepFactor);
                                const float dynInc = (kTwoPi * instFreq) / fs;
                                slot.phaseIncrement = dynInc;

                                const float body = std::sin(slot.phase);
                                float click = 0.0f;
                                if (slot.noteFrameIndex < 16) {
                                    click = pulsoClickSample(slot.eventId, slot.noteFrameIndex, slot.slotIndex) * (0.15f + 0.15f * slot.timbreParam);
                                }
                                s = body + click;
                                sampleL = s * slot.velocity * slot.envelopeLevel * kNominalVoiceScale;
                                sampleR = sampleL;
                                break;
                            }
                        case MusicRole::Corpo: {
                            // PolyBLEP saw + TPT SVF lowpass
                            const float saw = slot.osc.nextSample(slot.phase, slot.phaseIncrement);
                            const float baseCutoff = std::min(slot.pitchHz * 3.0f, 1500.0f) * (1.0f + 1.5f * slot.timbreParam);
                            const float cutoff = std::clamp(baseCutoff + slot.envelopeLevel * 1200.0f, 60.0f, 0.42f * fs);
                            const float q = 0.8f + 0.4f * slot.timbreParam;
                            s = slot.filter.processLowpass(saw, cutoff, q, fs);
                            const float scaled = s * slot.velocity * slot.envelopeLevel * kNominalVoiceScale;
                            // Subtle stereo spread
                            if (slot.slotIndex == 2) {
                                sampleL = scaled;
                                sampleR = scaled * 0.85f;
                            } else {
                                sampleL = scaled * 0.85f;
                                sampleR = scaled;
                            }
                            break;
                        }
                        case MusicRole::Trama: {
                            // 2-operator FM
                            const float harmonicRatio = (slot.timbreParam == 0.0f) ? 2.0f : (1.0f + 1.76f * slot.timbreParam);
                            const float modFreq = slot.pitchHz * harmonicRatio;
                            slot.fmModPhaseIncrement = (kTwoPi * modFreq) / fs;
                            const float maxIdx = carsonMaxModulationIndex(slot.pitchHz, modFreq, fs) *
                                                 std::clamp(slot.timbreParam + 0.25f, 0.0f, 1.0f);
                            const float modSignal = std::sin(slot.fmModPhase) * (maxIdx * slot.envelopeLevel);
                            s = std::sin(slot.phase + modSignal);
                            const float scaled = s * slot.velocity * slot.envelopeLevel * kNominalVoiceScale;
                            // Wide stereo spread
                            if (slot.slotIndex == 4) {
                                sampleL = scaled;
                                sampleR = scaled * 0.5f;
                            } else {
                                sampleL = scaled * 0.5f;
                                sampleR = scaled;
                            }
                            if (!isTransportPaused) {
                                slot.fmModPhase += slot.fmModPhaseIncrement;
                                if (slot.fmModPhase >= kTwoPi) slot.fmModPhase -= kTwoPi;
                            }
                            break;
                        }
                        case MusicRole::Vettore: {
                            // PolyBLEP saw with glide, vibrato, and TPT SVF
                            const float smoothedFreq = slot.pitchSmoother.process(slot.pitchHz);
                            const float vibDepth = 1.5f + 3.0f * slot.timbreParam;
                            const float instFreq = std::clamp(smoothedFreq + std::sin(slot.vibratoPhase) * vibDepth, 20.0f, 0.45f * fs);
                            const float dynInc = (kTwoPi * instFreq) / fs;
                            const float saw = slot.osc.nextSample(slot.phase, dynInc);
                            const float cutoff = std::clamp(instFreq * 2.5f + slot.envelopeLevel * 1500.0f, 100.0f, 0.42f * fs);
                            s = slot.filter.processLowpass(saw, cutoff, 1.0f, fs);
                            const float scaled = s * slot.velocity * slot.envelopeLevel * kNominalVoiceScale;
                            if (slot.slotIndex == 6) {
                                sampleL = scaled;
                                sampleR = scaled * 0.9f;
                            } else {
                                sampleL = scaled * 0.9f;
                                sampleR = scaled;
                            }
                            if (!isTransportPaused) {
                                slot.vibratoPhase += (kTwoPi * 5.5f) / fs;
                                if (slot.vibratoPhase >= kTwoPi) slot.vibratoPhase -= kTwoPi;
                            }
                            break;
                        }
                    }
                }

                    if (!isTransportPaused) {
                        slot.phase += slot.phaseIncrement;
                        if (slot.phase >= kTwoPi) {
                            slot.phase -= kTwoPi;
                        }
                        ++slot.noteFrameIndex;

                        // ADSR envelope step
                        ++slot.stageFrame;
                        switch (slot.state) {
                            case VoiceState::Attack:
                                if (slot.attackSamples > 0 && slot.stageFrame < slot.attackSamples) {
                                    slot.envelopeLevel = static_cast<float>(slot.stageFrame) /
                                                         static_cast<float>(slot.attackSamples);
                                } else {
                                    slot.envelopeLevel = 1.0f;
                                    slot.stageFrame = 0;
                                    slot.state = VoiceState::Decay;
                                }
                                break;
                            case VoiceState::Decay:
                                if (slot.decaySamples > 0 && slot.stageFrame < slot.decaySamples) {
                                    const float prog = static_cast<float>(slot.stageFrame) /
                                                       static_cast<float>(slot.decaySamples);
                                    slot.envelopeLevel = 1.0f - prog * (1.0f - slot.sustainLevel);
                                } else {
                                    slot.envelopeLevel = slot.sustainLevel;
                                    slot.stageFrame = 0;
                                    slot.state = (slot.sustainLevel > 0.0f) ? VoiceState::Sustain : VoiceState::Idle;
                                }
                                break;
                            case VoiceState::Sustain:
                                slot.envelopeLevel = slot.sustainLevel;
                                break;
                            case VoiceState::Release:
                                if (slot.releaseSamples > 0 && slot.stageFrame < slot.releaseSamples) {
                                    const float prog = static_cast<float>(slot.stageFrame) /
                                                       static_cast<float>(slot.releaseSamples);
                                    slot.envelopeLevel = slot.releaseStartLevel * (1.0f - prog);
                                } else {
                                    slot.envelopeLevel = 0.0f;
                                    slot.stageFrame = 0;
                                    slot.state = VoiceState::Idle;
                                    slot.phase = 0.0f;
                                }
                                break;
                            case VoiceState::Idle:
                                break;
                        }
                    }
                }

                // Sanitize voice output
                if (!std::isfinite(sampleL)) { sampleL = 0.0f; ++dspAnomalyCount_; }
                if (!std::isfinite(sampleR)) { sampleR = 0.0f; ++dspAnomalyCount_; }

                // Save post-bus output for physical steal tail continuity
                const float busGain = roleBusGain(slot.role, slot.timbreParam);
                slot.lastOutput[0] = sampleL * busGain;
                slot.lastOutput[1] = sampleR * busGain;

                roleBusProcessor_.accumulateRoleSample(slot.role, f, sampleL, sampleR);
            }
        }

        // Aggregate role pitches and active voice counts for telemetry
        std::array<float, kRoleCount> rolePitches{};
        std::array<std::uint32_t, kRoleCount> roleActive{};
        std::array<float, kRoleCount> roleTimbres{};
        for (std::size_t r = 0; r < kRoleCount; ++r) {
            const auto role = static_cast<MusicRole>(r);
            const std::size_t s0 = roleFirstSlot(role);
            const std::size_t s1 = s0 + 1;
            std::uint32_t active = 0;
            float pitch = 220.0f;
            float timbre = 0.0f;
            if (slots_[s0].state != VoiceState::Idle) {
                ++active;
                pitch = slots_[s0].pitchHz;
                timbre = slots_[s0].timbreParam;
            }
            if (slots_[s1].state != VoiceState::Idle) {
                ++active;
                pitch = slots_[s1].pitchHz;
                timbre = slots_[s1].timbreParam;
            }
            roleActive[r] = active;
            rolePitches[r] = pitch;
            roleTimbres[r] = timbre;
        }

        // Process role buses (ducking, delay, bus scaling) and mix to blockMixBuffer_
        roleBusProcessor_.processAndMix(
            blockMixBuffer_.data(),
            subFrames,
            rolePitches,
            roleActive,
            roleTimbres,
            latestTelemetry_
        );

        // Add active steal tails directly to mix buffer (post-bus physical anti-click decay)
        for (auto& slot : slots_) {
            if (slot.stealTail.active) {
                for (std::uint32_t f = 0; f < subFrames; ++f) {
                    if (slot.stealTail.remainingSamples == 0) {
                        slot.stealTail.active = false;
                        break;
                    }
                    const std::uint32_t tailLen = (slot.stealTail.totalSamples > 0)
                        ? slot.stealTail.totalSamples
                        : declickFrames_;
                    const float factor = static_cast<float>(slot.stealTail.remainingSamples) /
                                         static_cast<float>(tailLen);
                    float tailL = slot.stealTail.startAmplitude[0] * factor;
                    float tailR = slot.stealTail.startAmplitude[1] * factor;
                    if (!std::isfinite(tailL)) { tailL = 0.0f; ++dspAnomalyCount_; }
                    if (!std::isfinite(tailR)) { tailR = 0.0f; ++dspAnomalyCount_; }

                    blockMixBuffer_[f * 2] += tailL;
                    blockMixBuffer_[f * 2 + 1] += tailR;

                    --slot.stealTail.remainingSamples;
                }
            }
        }

        // Common terminal protection: zero-latency MasterLimiter
        masterLimiter_.processBlock(blockMixBuffer_.data(), subFrames);

        // Transport de-click ramp & silence gate + write to pOut
        for (std::uint32_t f = 0; f < subFrames; ++f) {
            if (transportRamp_ == TransportRamp::FadingOut) {
                transportRampLevel_ -= transportRampStep_;
                if (transportRampLevel_ <= 0.0f || --transportRampRemaining_ == 0) {
                    transportRampLevel_ = 0.0f;
                    transportRamp_ = TransportRamp::None;
                }
            } else if (transportRamp_ == TransportRamp::FadingIn) {
                transportRampLevel_ += transportRampStep_;
                if (transportRampLevel_ >= 1.0f || --transportRampRemaining_ == 0) {
                    transportRampLevel_ = 1.0f;
                    transportRamp_ = TransportRamp::None;
                }
            }

            float left = blockMixBuffer_[static_cast<std::size_t>(f) * 2];
            float right = blockMixBuffer_[static_cast<std::size_t>(f) * 2 + 1];

            if (isTransportPaused && transportRamp_ == TransportRamp::None && transportRampLevel_ <= 0.0f) {
                left = 0.0f;
                right = 0.0f;
            } else {
                left *= transportRampLevel_;
                right *= transportRampLevel_;
            }

            if (!std::isfinite(left)) { left = 0.0f; ++dspAnomalyCount_; }
            if (!std::isfinite(right)) { right = 0.0f; ++dspAnomalyCount_; }

            const std::size_t outIdx = static_cast<std::size_t>(processedFrames + f) * 2;
            if (accumulate) {
                pOut[outIdx] += left;
                pOut[outIdx + 1] += right;
            } else {
                pOut[outIdx] = left;
                pOut[outIdx + 1] = right;
            }
        }

        processedFrames += subFrames;
    }
}

} // namespace cymatica::audio
