#include "voice_allocator.h"
#include "../core/unsigned_math.h"

#include <algorithm>
#include <cmath>

namespace cymatica::audio {

namespace {
constexpr float kTwoPi = 6.28318530717958647692f;
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

    for (std::uint32_t f = 0; f < frameCount; ++f) {
        std::uint64_t logicalOffset = f;
        if (sampleRate_ != 48000 && sampleRate_ > 0) {
            logicalOffset = core::multiplyDivide(
                static_cast<std::uint64_t>(f), 48000ULL, static_cast<std::uint64_t>(sampleRate_)).value;
        }
        const std::uint64_t sampleLogicalFrame = isTransportPaused
            ? currentLogicalFrame
            : core::saturatingAdd(currentLogicalFrame, logicalOffset);

        // Check failsafe lifetimes at sample boundary
        if (!isTransportPaused) {
            checkLifetimes(sampleLogicalFrame);
        }

        // Sum voices
        float left = 0.0f;
        float right = 0.0f;

        for (auto& slot : slots_) {
            // 1. Voice synthesis & envelope progression
            if (slot.state != VoiceState::Idle) {
                const float s = std::sin(slot.phase) * slot.velocity * slot.envelopeLevel * kNominalVoiceScale;
                left += s;
                right += s;

                if (!isTransportPaused) {
                    slot.phase += slot.phaseIncrement;
                    if (slot.phase >= kTwoPi) {
                        slot.phase -= kTwoPi;
                    }

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
                                slot.state = VoiceState::Sustain;
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

            // 2. Steal tail decay (physical tail continues even during pause)
            if (slot.stealTail.active) {
                const std::uint32_t tailLen = (slot.stealTail.totalSamples > 0)
                    ? slot.stealTail.totalSamples
                    : declickFrames_;
                const float factor = static_cast<float>(slot.stealTail.remainingSamples) /
                                     static_cast<float>(tailLen);
                left += slot.stealTail.startAmplitude[0] * factor;
                right += slot.stealTail.startAmplitude[1] * factor;

                --slot.stealTail.remainingSamples;
                if (slot.stealTail.remainingSamples == 0) {
                    slot.stealTail.active = false;
                }
            }
        }

        // 3. Transport physical de-click ramp
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

        if (isTransportPaused && transportRamp_ == TransportRamp::None && transportRampLevel_ <= 0.0f) {
            left = 0.0f;
            right = 0.0f;
        } else {
            left *= transportRampLevel_;
            right *= transportRampLevel_;
        }

        // Sanitise non-finite values
        if (!std::isfinite(left)) {
            left = 0.0f;
            ++dspAnomalyCount_;
        }
        if (!std::isfinite(right)) {
            right = 0.0f;
            ++dspAnomalyCount_;
        }

        const std::size_t outIdx = static_cast<std::size_t>(f) * 2;
        if (accumulate) {
            pOut[outIdx] += left;
            pOut[outIdx + 1] += right;
        } else {
            pOut[outIdx] = left;
            pOut[outIdx + 1] = right;
        }
    }
}

} // namespace cymatica::audio
