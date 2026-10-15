// Single translation unit containing MINIAUDIO_IMPLEMENTATION in the whole project
// (spec section 28.2, ADR-0001). No other target may open an audio device.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "audio_engine.h"
#include "audio_probe.h"
#include "music_clock.h"

#include <atomic>
#include <memory>

#if defined(_MSC_VER) || defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#endif

namespace cymatica::audio {

struct AudioEngine::Impl {
    static constexpr std::size_t kCommandCapacity = 128;

    ma_device device{};
    ma_waveform waveform{};
    bool deviceInitialized{false};
    std::atomic<bool> deviceStarted{false};
    std::uint32_t sampleRate{48000};
    std::uint32_t channels{2};

    std::atomic<AudioProbe*> audioProbe{nullptr}; // Lock-free PCM tap (ADR-0003 D-AO-02)
    std::atomic<std::uint32_t> probeActiveWriters_{0}; // Quiescence tracker (DIF-M2-01)

    // Master control state owned by the control/game thread (§20.5, DIF-M1-17)
    AudioControlFrame masterControl_{};
    std::size_t inFlightCommands_{0}; // Tracked on control/game thread for guaranteed ack delivery (DIF-M1-18)

    static constexpr std::size_t kScheduledEventCapacity = 256;

    // Transport cursors and epoch (§7.5, §20.8, ADR-0002)
    std::uint64_t deviceFramesRendered{0};
    std::uint64_t transportEpoch{1};
    TransportState transportState{TransportState::Running};
    float activeVolume{0.0f};
    std::atomic<std::uint64_t> renderCursor{0};        // Synchronized authoritative 48 kHz timeline (DIF-M1-16)
    std::atomic<std::uint64_t> droppedAcks{0};         // Dropped command acks defensive counter (DIF-M1-07)
    std::uint64_t telemetrySequence{0};               // Sequence counter for telemetry drops (DIF-M1-13)
    MusicClock musicClock{};

    // Lock-free exchange channels (§20.5–§20.7, ADR-0002, ADR-0003 D-M2-03)
    TripleBuffer<AudioTelemetryFrame> telemetryBuffer{};
    TripleBuffer<AudioControlFrame> controlBuffer{};
    SpscQueue<AudioCommand, kCommandCapacity> commandQueue{};
    SpscQueue<AudioCommandAck, kCommandCapacity> ackQueue{};
    SpscQueue<ScheduledAudioEvent, kScheduledEventCapacity> scheduledEventQueue{};
    std::atomic<std::uint64_t> droppedScheduledEvents{0};
    std::atomic<std::uint64_t> totalScheduledEventsProcessed{0};
    std::atomic<std::uint64_t> deferredScheduledEvents{0}; // Monotonic count of deferred events (DIF-M2-R4-02)

    // Staging buffer for future scheduled commands (prevents head-of-line blocking on immediate transport commands)
    static constexpr std::size_t kMaxPendingScheduledCommands = 32;
    std::array<AudioCommand, kMaxPendingScheduledCommands> pendingScheduledCommands_{};
    std::size_t pendingScheduledCount_{0};

    // VoiceAllocator (ADR-0003 D-M2-03, Inc-1)
    VoiceAllocator voiceAllocator{};

    // Voice lifecycle & micro-fadeout (DIF-M2-R4-01, ADR-0003 D-M2-03)
    bool activeNotePlaying_{false};
    std::uint64_t activeNoteEpoch_{0};
    std::uint64_t lastEpochTransitionFrame_{0};
    static constexpr std::uint32_t kVoiceFadeFrames = 96; // 2 ms micro-fadeout at 48 kHz
    std::uint32_t voiceFadeRemaining_{0};
    float voiceFadeAmp_{0.0f};

    void renderWaveformSlice(float* pOut, std::uint32_t offset, std::uint32_t count, std::uint64_t logicalFrame) noexcept {
        if (pOut == nullptr || count == 0) return;
        float* dest = pOut + (static_cast<std::size_t>(offset) * channels);

        if (activeVolume > 1e-6f || activeNotePlaying_) {
            ma_waveform_read_pcm_frames(&waveform, dest, count, nullptr);
            if (voiceFadeRemaining_ > 0) {
                const std::uint32_t rampFrames = (std::min)(count, voiceFadeRemaining_);
                for (std::uint32_t i = 0; i < rampFrames; ++i) {
                    const float factor = static_cast<float>(voiceFadeRemaining_ - i) / static_cast<float>(kVoiceFadeFrames);
                    for (std::uint32_t c = 0; c < channels; ++c) {
                        dest[static_cast<std::size_t>(i) * channels + c] *= factor;
                    }
                }
                voiceFadeRemaining_ -= rampFrames;
                if (voiceFadeRemaining_ == 0) {
                    ma_waveform_set_amplitude(&waveform, 0.0f);
                    voiceFadeAmp_ = 0.0f;
                }
                if (rampFrames < count) {
                    const std::size_t zeroStart = static_cast<std::size_t>(rampFrames) * channels;
                    const std::size_t zeroCount = static_cast<std::size_t>(count - rampFrames) * channels;
                    for (std::size_t s = 0; s < zeroCount; ++s) {
                        dest[zeroStart + s] = 0.0f;
                    }
                }
            }
            // Mix polyphonic voices from VoiceAllocator on top of test tone
            voiceAllocator.renderBlock(dest, count, logicalFrame, (transportState == TransportState::Paused), true);
        } else {
            // Pure polyphonic synthesis without background test tone (zero-out destination then render)
            voiceAllocator.renderBlock(dest, count, logicalFrame, (transportState == TransportState::Paused), false);
        }
    }

    void triggerNoteOff() noexcept {
        if (activeNotePlaying_ || voiceFadeAmp_ > 0.0f) {
            activeNotePlaying_ = false;
            activeNoteEpoch_ = 0;
            voiceFadeRemaining_ = kVoiceFadeFrames;
        } else {
            ma_waveform_set_amplitude(&waveform, 0.0f);
            voiceFadeRemaining_ = 0;
            voiceFadeAmp_ = 0.0f;
        }
    }

    void triggerNoteOn(const ScheduledAudioEvent& evt, float masterVol) noexcept {
        if (masterVol <= 1e-6f) {
            // No continuous diagnostic tone active: ScheduledAudioEvent is handled exclusively by VoiceAllocator.
            return;
        }
        voiceFadeRemaining_ = 0;
        voiceFadeAmp_ = evt.valueB * masterVol;
        activeNotePlaying_ = true;
        activeNoteEpoch_ = evt.transportEpoch;
        ma_waveform_set_frequency(&waveform, evt.valueA);
        ma_waveform_set_amplitude(&waveform, voiceFadeAmp_);
    }

    bool stagePendingCommand(const AudioCommand& cmd) noexcept {
        if (pendingScheduledCount_ >= kMaxPendingScheduledCommands) {
            return false;
        }
        std::size_t insertIdx = pendingScheduledCount_;
        while (insertIdx > 0 && pendingScheduledCommands_[insertIdx - 1].targetSampleFrame > cmd.targetSampleFrame) {
            pendingScheduledCommands_[insertIdx] = pendingScheduledCommands_[insertIdx - 1];
            --insertIdx;
        }
        pendingScheduledCommands_[insertIdx] = cmd;
        ++pendingScheduledCount_;
        return true;
    }

    AudioCommandStatus executeCommand(const AudioCommand& cmd,
                                      std::uint64_t blockStartFrame,
                                      std::uint64_t /*blockEndFrame*/,
                                      std::uint64_t& effectiveFrame) noexcept {
        AudioCommandStatus status = AudioCommandStatus::Applied;
        effectiveFrame = cmd.targetSampleFrame;

        switch (cmd.type) {
            case AudioCommandType::Start:
                if (transportState == TransportState::Running ||
                    isValidTransportTransition(transportState, TransportState::Running)) {
                    activeVolume = cmd.paramF32;
                    ma_waveform_set_amplitude(&waveform, activeVolume);
                    transportState = TransportState::Running;
                } else {
                    status = AudioCommandStatus::Rejected;
                }
                break;
            case AudioCommandType::Pause:
                if (isValidTransportTransition(transportState, TransportState::Pausing)) {
                    transportState = TransportState::Pausing;
                    voiceAllocator.onPause();
                    triggerNoteOff();
                    transportState = TransportState::Paused;
                    ++transportEpoch;
                    lastEpochTransitionFrame_ = effectiveFrame;
                } else {
                    status = AudioCommandStatus::Rejected;
                }
                break;
            case AudioCommandType::Resume:
                if (isValidTransportTransition(transportState, TransportState::Resuming)) {
                    transportState = TransportState::Resuming;
                    voiceAllocator.onResume();
                    ma_waveform_set_amplitude(&waveform, activeVolume);
                    transportState = TransportState::Running;
                    ++transportEpoch;
                    lastEpochTransitionFrame_ = effectiveFrame;
                } else {
                    status = AudioCommandStatus::Rejected;
                }
                break;
            case AudioCommandType::Stop:
                if (isValidTransportTransition(transportState, TransportState::Stopped)) {
                    voiceAllocator.onStop();
                    triggerNoteOff();
                    transportState = TransportState::Stopped;
                    ++transportEpoch;
                    lastEpochTransitionFrame_ = effectiveFrame;
                    // Reject any remaining staged future commands upon transport stop
                    for (std::size_t j = 0; j < pendingScheduledCount_; ++j) {
                        if (!ackQueue.tryPush(AudioCommandAck{
                            .commandId = pendingScheduledCommands_[j].commandId,
                            .status = AudioCommandStatus::Rejected,
                            .sampleFrame = blockStartFrame,
                            .transportEpoch = transportEpoch,
                        })) {
                            droppedAcks.fetch_add(1, std::memory_order_relaxed);
                        }
                    }
                    pendingScheduledCount_ = 0;
                } else {
                    status = AudioCommandStatus::Rejected;
                }
                break;
            case AudioCommandType::SetTone:
                ma_waveform_set_frequency(&waveform, cmd.paramF32);
                break;
            case AudioCommandType::SetTempoMap: {
                const TempoMap newMap = cmd.extractTempoMap();
                if (MusicClock::isSupportedTempoMap(newMap, sampleRate)) {
                    effectiveFrame = cmd.targetSampleFrame;
                    if (musicClock.setTempoMap(newMap, effectiveFrame)) {
                        ++transportEpoch;
                        lastEpochTransitionFrame_ = effectiveFrame;
                        status = AudioCommandStatus::Applied;
                    } else {
                        status = AudioCommandStatus::Rejected;
                    }
                } else {
                    status = AudioCommandStatus::Rejected;
                }
                break;
            }
            default:
                status = AudioCommandStatus::Rejected;
                break;
        }
        return status;
    }

    // Realtime processing function: zero heap allocation, bounded execution (§20.2)
    void processBlock(float* pOutput, std::uint32_t frameCount) noexcept {
        const std::uint64_t blockStartFrame = renderCursor.load(std::memory_order_relaxed);
        const std::uint64_t blockEndFrame = musicClock.deviceToInternalFrames(deviceFramesRendered + frameCount, sampleRate);

        // 1. Consume continuous control updates
        if (controlBuffer.update()) {
            const auto& ctrl = controlBuffer.readSlot();
            activeVolume = ctrl.toneVolume;
            ma_waveform_set_frequency(&waveform, ctrl.toneFrequencyHz);
            if (transportState == TransportState::Running) {
                ma_waveform_set_amplitude(&waveform, activeVolume);
            }
        }

        // 2. Consume commands from commandQueue and stage them chronologically (DIF-M2-R3-04)
        for (std::uint32_t i = 0; i < AudioEngine::kMaxCommandsPerBlock; ++i) {
            AudioCommand cmd;
            if (!commandQueue.tryPop(cmd)) break;

            // Reject late commands whose explicit target frame has already passed (DIF-M2-R3-04)
            if (cmd.targetSampleFrame > 0 && cmd.targetSampleFrame < blockStartFrame) {
                if (!ackQueue.tryPush(AudioCommandAck{
                    .commandId = cmd.commandId,
                    .status = AudioCommandStatus::Rejected,
                    .sampleFrame = cmd.targetSampleFrame,
                    .transportEpoch = transportEpoch,
                })) {
                    droppedAcks.fetch_add(1, std::memory_order_relaxed);
                }
                continue;
            }

            // Quantize immediate SetTempoMap during playback to next musical boundary (DIF-M2-R3-04)
            if (cmd.type == AudioCommandType::SetTempoMap) {
                const auto newMap = cmd.extractTempoMap();
                if (!MusicClock::isSupportedTempoMap(newMap, sampleRate)) {
                    if (!ackQueue.tryPush(AudioCommandAck{
                        .commandId = cmd.commandId,
                        .status = AudioCommandStatus::Rejected,
                        .sampleFrame = cmd.targetSampleFrame > 0 ? cmd.targetSampleFrame : blockStartFrame,
                        .transportEpoch = transportEpoch,
                    })) {
                        droppedAcks.fetch_add(1, std::memory_order_relaxed);
                    }
                    continue;
                }

                if (cmd.targetSampleFrame == 0) {
                    if (blockStartFrame == 0 && renderCursor.load(std::memory_order_relaxed) == 0) {
                        cmd.targetSampleFrame = 0;
                    } else {
                        const auto& curMap = musicClock.tempoMap();
                        const bool meterChanged = (newMap.beatsPerBar != curMap.beatsPerBar ||
                                                   newMap.timeSignatureDen != curMap.timeSignatureDen ||
                                                   newMap.timeSignatureNum != curMap.timeSignatureNum);
                        cmd.targetSampleFrame = meterChanged
                            ? musicClock.nextBarBoundary(blockStartFrame)
                            : musicClock.nextBeatBoundary(blockStartFrame);
                    }
                }
            } else if (cmd.targetSampleFrame == 0) {
                cmd.targetSampleFrame = blockStartFrame;
            }

            if (!stagePendingCommand(cmd)) {
                if (!ackQueue.tryPush(AudioCommandAck{
                    .commandId = cmd.commandId,
                    .status = AudioCommandStatus::Rejected,
                    .sampleFrame = cmd.targetSampleFrame,
                    .transportEpoch = transportEpoch,
                })) {
                    droppedAcks.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }

        // 3. Handle paused / stopped state
        if (transportState == TransportState::Paused || transportState == TransportState::Stopped) {
            // Check if a Resume or Start command was staged for blockStartFrame
            while (pendingScheduledCount_ > 0 && pendingScheduledCommands_[0].targetSampleFrame <= blockStartFrame) {
                AudioCommand cmd = pendingScheduledCommands_[0];
                for (std::size_t j = 0; j + 1 < pendingScheduledCount_; ++j) {
                    pendingScheduledCommands_[j] = pendingScheduledCommands_[j + 1];
                }
                --pendingScheduledCount_;

                std::uint64_t effFrame = cmd.targetSampleFrame;
                const AudioCommandStatus status = executeCommand(cmd, blockStartFrame, blockEndFrame, effFrame);
                if (!ackQueue.tryPush(AudioCommandAck{
                    .commandId = cmd.commandId,
                    .status = status,
                    .sampleFrame = effFrame,
                    .transportEpoch = transportEpoch,
                })) {
                    droppedAcks.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }

        if (transportState == TransportState::Paused || transportState == TransportState::Stopped) {
            if (pOutput != nullptr && frameCount > 0) {
                for (std::size_t i = 0; i < static_cast<std::size_t>(frameCount) * channels; ++i) {
                    pOutput[i] = 0.0f;
                }
                // Render physical de-click tail if active during pause / stop (ADR-0003 D-M2-03)
                voiceAllocator.renderBlock(pOutput, frameCount, blockStartFrame, true, true);
            }
            // Timeline and scheduled event consumption are frozen during pause / stopped
        } else {
            // Render block with sample-accurate interleaving of scheduled events and commands
            // (Spec §7.5, ADR-0003 D-M2-03, DIF-M2-R3-02, DIF-M2-R3-05, DIF-M2-R4-01, DIF-M2-R4-02)
            std::uint32_t currentDevOffset = 0;
            std::uint32_t eventsExaminedInBlock = 0;

            while (currentDevOffset < frameCount) {
                const std::uint64_t currentLogicalFrame = (sampleRate == musicClock.internalSampleRate())
                    ? (blockStartFrame + currentDevOffset)
                    : musicClock.deviceToInternalFrames(deviceFramesRendered + currentDevOffset, sampleRate);

                // Monophonic test-tone safeguard (DIF-M2-R4-01): auto-release orphaned voice from past epoch
                // if no NoteOff arrived within 1s.
                // NOTE [M2.4 / M2.5 Handoff]: To be superseded in M2.4 by the 8-voice polyphonic synthesizer
                // (ADR-0003 D-M2-03), where sustained notes (e.g. Corpo pads) maintain their ADSR sustain
                // stage across epochs, and orphan detection is governed by per-voice envelope lifetimes.
                if (activeNotePlaying_ && activeNoteEpoch_ < transportEpoch &&
                    currentLogicalFrame > lastEpochTransitionFrame_ + 48000) {
                    triggerNoteOff();
                }

                // Check next matured command in this block
                bool hasCmd = (pendingScheduledCount_ > 0 && pendingScheduledCommands_[0].targetSampleFrame < blockEndFrame);
                std::uint32_t targetCmdDevOffset = frameCount;
                if (hasCmd) {
                    const std::uint64_t cmdFrame = pendingScheduledCommands_[0].targetSampleFrame;
                    if (cmdFrame <= currentLogicalFrame) {
                        targetCmdDevOffset = currentDevOffset;
                    } else {
                        const std::uint64_t devF = musicClock.internalToDeviceFrames(cmdFrame, sampleRate);
                        if (devF > deviceFramesRendered) {
                            const std::uint64_t off = devF - deviceFramesRendered;
                            targetCmdDevOffset = static_cast<std::uint32_t>(std::min<std::uint64_t>(off, frameCount));
                        } else {
                            targetCmdDevOffset = currentDevOffset;
                        }
                    }
                    if (targetCmdDevOffset < currentDevOffset) targetCmdDevOffset = currentDevOffset;
                }

                // Check next scheduled event with bounded budget & 3-way epoch policy
                // (DIF-M2-R3-05, DIF-M2-R4-01, DIF-M2-R4-02)
                bool hasEvt = false;
                std::uint32_t targetEvtDevOffset = frameCount;
                ScheduledAudioEvent schedEvt{};

                while (scheduledEventQueue.peek(schedEvt)) {
                    if (eventsExaminedInBlock >= AudioEngine::kMaxScheduledEventsPerBlock) {
                        // Hard-realtime budget exhausted for this block! (DIF-M2-R4-02)
                        if (schedEvt.sampleFrame < blockEndFrame) {
                            deferredScheduledEvents.fetch_add(1, std::memory_order_relaxed);
                        }
                        hasEvt = false;
                        break;
                    }

                    if (schedEvt.transportEpoch < transportEpoch) {
                        // Event from earlier epoch: check if this is the legitimate NoteOff for active voice (DIF-M2-R4-01, ADR-0003)
                        bool hasMatchingVoice = false;
                        if (schedEvt.type == AudioEventType::NoteOff) {
                            const std::size_t s0 = VoiceAllocator::roleFirstSlot(schedEvt.role);
                            const std::size_t s1 = s0 + 1;
                            const auto& slot0 = voiceAllocator.slot(s0);
                            const auto& slot1 = voiceAllocator.slot(s1);
                            if ((slot0.state != VoiceState::Idle && slot0.state != VoiceState::Release &&
                                 (slot0.eventId == schedEvt.eventId || schedEvt.eventId == 0)) ||
                                (slot1.state != VoiceState::Idle && slot1.state != VoiceState::Release &&
                                 (slot1.eventId == schedEvt.eventId || schedEvt.eventId == 0))) {
                                hasMatchingVoice = true;
                            }
                        }
                        const bool isMatchingActiveNoteOff = hasMatchingVoice ||
                                                             ((schedEvt.type == AudioEventType::NoteOff) &&
                                                              activeNotePlaying_ &&
                                                              (schedEvt.transportEpoch == activeNoteEpoch_));
                        if (isMatchingActiveNoteOff) {
                            // Voice preservation: keep event until its scheduled frame
                            if (schedEvt.sampleFrame < blockEndFrame) {
                                hasEvt = true;
                                if (schedEvt.sampleFrame <= currentLogicalFrame) {
                                    targetEvtDevOffset = currentDevOffset;
                                } else {
                                    const std::uint64_t devF = musicClock.internalToDeviceFrames(schedEvt.sampleFrame, sampleRate);
                                    if (devF > deviceFramesRendered) {
                                        const std::uint64_t off = devF - deviceFramesRendered;
                                        targetEvtDevOffset = static_cast<std::uint32_t>(std::min<std::uint64_t>(off, frameCount));
                                    } else {
                                        targetEvtDevOffset = currentDevOffset;
                                    }
                                }
                                if (targetEvtDevOffset < currentDevOffset) targetEvtDevOffset = currentDevOffset;
                            } else {
                                hasEvt = false;
                            }
                            break;
                        } else {
                            // Obsolete event from earlier epoch: drop without synthesizing DSP
                            (void)scheduledEventQueue.tryPop(schedEvt);
                            totalScheduledEventsProcessed.fetch_add(1, std::memory_order_relaxed);
                            ++eventsExaminedInBlock;
                            continue;
                        }
                    }

                    if (schedEvt.transportEpoch > transportEpoch) {
                        // Future epoch event (DIF-M2-R3-05)
                        if (schedEvt.sampleFrame < blockStartFrame ||
                            (schedEvt.sampleFrame <= currentLogicalFrame && (!hasCmd || pendingScheduledCommands_[0].targetSampleFrame > schedEvt.sampleFrame))) {
                            // Unmatchable: cursor passed or reached event time without epoch advancing
                            (void)scheduledEventQueue.tryPop(schedEvt);
                            droppedScheduledEvents.fetch_add(1, std::memory_order_relaxed);
                            ++eventsExaminedInBlock;
                            continue;
                        }
                        if (schedEvt.sampleFrame < blockEndFrame) {
                            hasEvt = true;
                            const std::uint64_t devF = musicClock.internalToDeviceFrames(schedEvt.sampleFrame, sampleRate);
                            if (devF > deviceFramesRendered) {
                                const std::uint64_t off = devF - deviceFramesRendered;
                                targetEvtDevOffset = static_cast<std::uint32_t>(std::min<std::uint64_t>(off, frameCount));
                            } else {
                                targetEvtDevOffset = currentDevOffset;
                            }
                            if (targetEvtDevOffset < currentDevOffset) targetEvtDevOffset = currentDevOffset;
                        } else {
                            hasEvt = false;
                        }
                        break;
                    }

                    // schedEvt.transportEpoch == transportEpoch
                    if (schedEvt.sampleFrame < blockEndFrame) {
                        hasEvt = true;
                        if (schedEvt.sampleFrame <= currentLogicalFrame) {
                            targetEvtDevOffset = currentDevOffset;
                        } else {
                            const std::uint64_t devF = musicClock.internalToDeviceFrames(schedEvt.sampleFrame, sampleRate);
                            if (devF > deviceFramesRendered) {
                                const std::uint64_t off = devF - deviceFramesRendered;
                                targetEvtDevOffset = static_cast<std::uint32_t>(std::min<std::uint64_t>(off, frameCount));
                            } else {
                                targetEvtDevOffset = currentDevOffset;
                            }
                        }
                        if (targetEvtDevOffset < currentDevOffset) targetEvtDevOffset = currentDevOffset;
                    } else {
                        hasEvt = false;
                    }
                    break;
                }

                if (hasCmd && (!hasEvt || targetCmdDevOffset <= targetEvtDevOffset)) {
                    // Render slice before command
                    if (targetCmdDevOffset > currentDevOffset) {
                        const std::uint32_t sliceFrames = targetCmdDevOffset - currentDevOffset;
                        renderWaveformSlice(pOutput, currentDevOffset, sliceFrames, currentLogicalFrame);
                        currentDevOffset = targetCmdDevOffset;
                    }

                    // Execute ALL commands that mature at or before current offset (handles Pause + Resume in same tick!)
                    while (pendingScheduledCount_ > 0 && pendingScheduledCommands_[0].targetSampleFrame <= currentLogicalFrame) {
                        AudioCommand cmd = pendingScheduledCommands_[0];
                        for (std::size_t j = 0; j + 1 < pendingScheduledCount_; ++j) {
                            pendingScheduledCommands_[j] = pendingScheduledCommands_[j + 1];
                        }
                        --pendingScheduledCount_;

                        std::uint64_t effFrame = cmd.targetSampleFrame;
                        const AudioCommandStatus status = executeCommand(cmd, blockStartFrame, blockEndFrame, effFrame);

                        if (!ackQueue.tryPush(AudioCommandAck{
                            .commandId = cmd.commandId,
                            .status = status,
                            .sampleFrame = effFrame,
                            .transportEpoch = transportEpoch,
                        })) {
                            droppedAcks.fetch_add(1, std::memory_order_relaxed);
                        }
                    }

                    // If paused or stopped, render physical de-click tail and break (ADR-0003 D-M2-03)
                    if (transportState != TransportState::Running) {
                        triggerNoteOff();
                        if (pOutput != nullptr && currentDevOffset < frameCount) {
                            const std::uint32_t rem = frameCount - currentDevOffset;
                            float* dest = pOutput + (static_cast<std::size_t>(currentDevOffset) * channels);
                            for (std::size_t s = 0; s < static_cast<std::size_t>(rem) * channels; ++s) {
                                dest[s] = 0.0f;
                            }
                            voiceAllocator.renderBlock(dest, rem, currentLogicalFrame, true, true);
                        }
                        break;
                    }
                } else if (hasEvt) {
                    // Render slice before event
                    if (targetEvtDevOffset > currentDevOffset) {
                        const std::uint32_t sliceFrames = targetEvtDevOffset - currentDevOffset;
                        renderWaveformSlice(pOutput, currentDevOffset, sliceFrames, currentLogicalFrame);
                        currentDevOffset = targetEvtDevOffset;
                    }

                    if (scheduledEventQueue.peek(schedEvt)) {
                        if (eventsExaminedInBlock >= AudioEngine::kMaxScheduledEventsPerBlock) {
                            deferredScheduledEvents.fetch_add(1, std::memory_order_relaxed);
                        } else {
                            if (schedEvt.transportEpoch < transportEpoch) {
                                // Monophonic & polyphonic voice matching (DIF-M2-R4-01, ADR-0003 D-M2-03)
                                bool hasMatchingVoice = false;
                                if (schedEvt.type == AudioEventType::NoteOff) {
                                    const std::size_t s0 = VoiceAllocator::roleFirstSlot(schedEvt.role);
                                    const std::size_t s1 = s0 + 1;
                                    const auto& slot0 = voiceAllocator.slot(s0);
                                    const auto& slot1 = voiceAllocator.slot(s1);
                                    if ((slot0.state != VoiceState::Idle && slot0.state != VoiceState::Release &&
                                         (slot0.eventId == schedEvt.eventId || schedEvt.eventId == 0)) ||
                                        (slot1.state != VoiceState::Idle && slot1.state != VoiceState::Release &&
                                         (slot1.eventId == schedEvt.eventId || schedEvt.eventId == 0))) {
                                        hasMatchingVoice = true;
                                    }
                                }
                                const bool isMatchingActiveNoteOff = hasMatchingVoice ||
                                                                     ((schedEvt.type == AudioEventType::NoteOff) &&
                                                                      activeNotePlaying_ &&
                                                                      (schedEvt.transportEpoch == activeNoteEpoch_));
                                (void)scheduledEventQueue.tryPop(schedEvt);
                                totalScheduledEventsProcessed.fetch_add(1, std::memory_order_relaxed);
                                ++eventsExaminedInBlock;
                                if (hasMatchingVoice) {
                                    voiceAllocator.onNoteOff(schedEvt);
                                }
                                if (isMatchingActiveNoteOff) {
                                    triggerNoteOff();
                                }
                            } else if (schedEvt.transportEpoch > transportEpoch) {
                                // Reached event frame without epoch advancing: unmatchable! (DIF-M2-R3-05)
                                (void)scheduledEventQueue.tryPop(schedEvt);
                                droppedScheduledEvents.fetch_add(1, std::memory_order_relaxed);
                                ++eventsExaminedInBlock;
                            } else {
                                (void)scheduledEventQueue.tryPop(schedEvt);
                                totalScheduledEventsProcessed.fetch_add(1, std::memory_order_relaxed);
                                ++eventsExaminedInBlock;

                                switch (schedEvt.type) {
                                    case AudioEventType::NoteOn:
                                        voiceAllocator.onNoteOn(schedEvt);
                                        triggerNoteOn(schedEvt, activeVolume);
                                        break;
                                    case AudioEventType::NoteOff:
                                        voiceAllocator.onNoteOff(schedEvt);
                                        triggerNoteOff();
                                        break;
                                    case AudioEventType::ParameterChange:
                                        if (schedEvt.valueA > 0.0f) {
                                            ma_waveform_set_frequency(&waveform, schedEvt.valueA);
                                        }
                                        if (schedEvt.valueB >= 0.0f) {
                                            voiceFadeAmp_ = schedEvt.valueB * (activeVolume > 0.0f ? activeVolume : 0.25f);
                                            ma_waveform_set_amplitude(&waveform, voiceFadeAmp_);
                                        }
                                        break;
                                }
                            }
                        }
                    }
                } else {
                    // Render remaining frames to block boundary
                    const std::uint32_t rem = frameCount - currentDevOffset;
                    if (pOutput != nullptr && rem > 0) {
                        renderWaveformSlice(pOutput, currentDevOffset, rem, currentLogicalFrame);
                    }
                    currentDevOffset = frameCount;
                }
            }

            if (transportState == TransportState::Running) {
                deviceFramesRendered += frameCount;
            } else {
                deviceFramesRendered += currentDevOffset;
            }
            renderCursor.store(musicClock.deviceToInternalFrames(deviceFramesRendered, sampleRate), std::memory_order_release);
        }

        // 3b. AudioProbe tap: lock-free, zero-alloc PCM capture with quiescence tracking (ADR-0003 D-AO-02, Spec §31.5, DIF-M2-01, DIF-M2-09)
        if (pOutput != nullptr && frameCount > 0) {
            probeActiveWriters_.fetch_add(1, std::memory_order_seq_cst);
            AudioProbe* activeProbe = audioProbe.load(std::memory_order_seq_cst);
            if (activeProbe != nullptr) {
                activeProbe->writeFrames(pOutput, frameCount);
            }
            probeActiveWriters_.fetch_sub(1, std::memory_order_release);
        }

        const std::uint64_t currentInternalCursor = renderCursor.load(std::memory_order_relaxed);
        const std::uint64_t presentationDeviceFrames = (transportState == TransportState::Running && deviceFramesRendered >= frameCount)
            ? deviceFramesRendered - frameCount : deviceFramesRendered;
        const std::uint64_t presentationCursor = musicClock.deviceToInternalFrames(presentationDeviceFrames, sampleRate);

        // 4. Publish latest telemetry (§22.1)
        auto& telem = telemetryBuffer.writeSlot();
        telem.sequenceNumber = ++telemetrySequence;
        telem.transportEpoch = transportEpoch;
        telem.transportState = transportState;
        telem.renderCursor = currentInternalCursor;
        telem.presentationCursor = presentationCursor;
        telem.presentationQuality = PresentationQuality::Estimated;
        telem.framesRenderedTotal = currentInternalCursor;

        const auto pos = musicClock.positionAtFrame(presentationCursor, transportEpoch);
        telem.bpm = musicClock.tempoMap().bpm.toF32();
        telem.beatPhase = pos.beatPhase;
        telem.beatConfidence = 1.0f;

        telem.globalEnergy = (transportState == TransportState::Running) ? 0.5f : 0.0f;
        telem.pulse.energy = (transportState == TransportState::Running) ? 0.5f : 0.0f;
        telem.pulse.pitchHz = 220.0f;
        telem.scheduledEventsDeferred = static_cast<std::uint32_t>(deferredScheduledEvents.load(std::memory_order_relaxed));
        telem.scheduledEventsDroppedTotal = static_cast<std::uint32_t>(droppedScheduledEvents.load(std::memory_order_relaxed));
        telem.activeVoiceCount = voiceAllocator.activeVoiceCount();
        telem.voiceDropCount = voiceAllocator.voiceDropCount();
        telem.orphanNoteRecoveredCount = voiceAllocator.orphanNoteRecoveredCount();
        telem.dspAnomalyCount = voiceAllocator.dspAnomalyCount();
        telemetryBuffer.publish();
    }

    static void dataCallback(ma_device* pDevice, void* pOutput, const void*, ma_uint32 frameCount) {
        auto* impl = static_cast<Impl*>(pDevice->pUserData);
        impl->processBlock(static_cast<float*>(pOutput), frameCount);
    }
};

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::init(const AudioEngineConfig& config) {
    if (pImpl_) return true;

    auto impl = std::make_unique<Impl>();
    impl->sampleRate = config.sampleRate;
    impl->channels = config.channels;
    impl->deviceInitialized = true;

    ma_device_config devCfg = ma_device_config_init(ma_device_type_playback);
    devCfg.playback.format = ma_format_f32;
    devCfg.playback.channels = config.channels;
    devCfg.sampleRate = config.sampleRate;
    devCfg.dataCallback = Impl::dataCallback;
    devCfg.pUserData = impl.get();

    if (ma_device_init(nullptr, &devCfg, &impl->device) != MA_SUCCESS) {
        return false;
    }

    ma_waveform_config waveCfg = ma_waveform_config_init(
        impl->device.playback.format,
        impl->device.playback.channels,
        impl->device.sampleRate,
        ma_waveform_type_sine,
        0.0f,
        config.defaultToneFreqHz
    );
    if (ma_waveform_init(&waveCfg, &impl->waveform) != MA_SUCCESS) {
        ma_device_uninit(&impl->device);
        return false;
    }
    impl->sampleRate = impl->device.sampleRate;
    impl->voiceAllocator.init(impl->sampleRate);

    // Seed initial master control values (DIF-M1-17)
    impl->masterControl_.toneFrequencyHz = config.defaultToneFreqHz;
    impl->masterControl_.toneVolume = 0.0f;

    auto& ctrl = impl->controlBuffer.writeSlot();
    ctrl = impl->masterControl_;
    impl->controlBuffer.publish();

    pImpl_ = impl.release();
    return true;
}

bool AudioEngine::initOffline(const AudioEngineConfig& config) {
    if (pImpl_) return true;

    auto impl = std::make_unique<Impl>();
    impl->sampleRate = config.sampleRate;
    impl->channels = config.channels;
    impl->deviceInitialized = false;

    ma_waveform_config waveCfg = ma_waveform_config_init(
        ma_format_f32,
        config.channels,
        config.sampleRate,
        ma_waveform_type_sine,
        0.0f,
        config.defaultToneFreqHz
    );
    if (ma_waveform_init(&waveCfg, &impl->waveform) != MA_SUCCESS) {
        return false;
    }
    impl->voiceAllocator.init(config.sampleRate);

    impl->masterControl_.toneFrequencyHz = config.defaultToneFreqHz;
    impl->masterControl_.toneVolume = 0.0f;

    auto& ctrl = impl->controlBuffer.writeSlot();
    ctrl = impl->masterControl_;
    impl->controlBuffer.publish();

    impl->transportState = TransportState::Running;

    pImpl_ = impl.release();
    return true;
}

void AudioEngine::shutdown() {
    if (!pImpl_) return;
    if (pImpl_->deviceInitialized) {
        if (pImpl_->deviceStarted.load(std::memory_order_acquire)) {
            ma_device_stop(&pImpl_->device);
            pImpl_->deviceStarted.store(false, std::memory_order_release);
        }
        ma_device_uninit(&pImpl_->device);
    }
    delete pImpl_;
    pImpl_ = nullptr;
}

bool AudioEngine::startTone(float freqHz, float amplitude) {
    if (!pImpl_) return false;
    pImpl_->masterControl_.toneFrequencyHz = freqHz;
    pImpl_->masterControl_.toneVolume = amplitude;
    publishControl();

    if (pImpl_->deviceInitialized) {
        if (!pImpl_->deviceStarted.load(std::memory_order_acquire)) {
            if (ma_device_start(&pImpl_->device) != MA_SUCCESS) {
                return false;
            }
            pImpl_->deviceStarted.store(true, std::memory_order_release);
        }
    } else {
        pImpl_->transportState = TransportState::Running;
    }
    return true;
}

void AudioEngine::setToneFrequency(float freqHz) noexcept {
    if (!pImpl_) return;
    pImpl_->masterControl_.toneFrequencyHz = freqHz;
    publishControl();
}

void AudioEngine::stopTone() {
    if (!pImpl_) return;
    if (pImpl_->deviceInitialized) {
        if (pImpl_->deviceStarted.load(std::memory_order_acquire)) {
            ma_device_stop(&pImpl_->device);
            pImpl_->deviceStarted.store(false, std::memory_order_release);
        }
    } else {
        pImpl_->transportState = TransportState::Stopped;
    }
}

bool AudioEngine::isRunning() const noexcept {
    if (!pImpl_) return false;
    if (pImpl_->deviceInitialized) {
        return pImpl_->deviceStarted.load(std::memory_order_acquire);
    }
    return pImpl_->transportState == TransportState::Running;
}

TransportState AudioEngine::transportState() const noexcept {
    return pImpl_ ? pImpl_->telemetryBuffer.readSlot().transportState : TransportState::Paused;
}

std::uint64_t AudioEngine::framesRendered() const noexcept {
    return pImpl_ ? pImpl_->renderCursor.load(std::memory_order_acquire) : 0;
}

std::uint32_t AudioEngine::sampleRate() const noexcept {
    return pImpl_ ? pImpl_->sampleRate : 0;
}

bool AudioEngine::updateTelemetry() noexcept {
    return pImpl_ ? pImpl_->telemetryBuffer.update() : false;
}

const AudioTelemetryFrame& AudioEngine::telemetry() const noexcept {
    static const AudioTelemetryFrame empty{};
    return pImpl_ ? pImpl_->telemetryBuffer.readSlot() : empty;
}

AudioControlFrame& AudioEngine::controlWriter() noexcept {
    static AudioControlFrame dummy{};
    return pImpl_ ? pImpl_->masterControl_ : dummy;
}

void AudioEngine::publishControl() noexcept {
    if (pImpl_) {
        auto& slot = pImpl_->controlBuffer.writeSlot();
        slot = pImpl_->masterControl_;
        pImpl_->controlBuffer.publish();
    }
}

bool AudioEngine::sendCommand(const AudioCommand& cmd) noexcept {
    if (!pImpl_) return false;
    // Guaranteed ack delivery backpressure (Spec §20.7, DIF-M1-18)
    if (pImpl_->inFlightCommands_ >= Impl::kCommandCapacity) {
        return false;
    }
    if (pImpl_->commandQueue.tryPush(cmd)) {
        ++pImpl_->inFlightCommands_;
        return true;
    }
    return false;
}

bool AudioEngine::pollAck(AudioCommandAck& ack) noexcept {
    if (!pImpl_) return false;
    if (pImpl_->ackQueue.tryPop(ack)) {
        if (pImpl_->inFlightCommands_ > 0) {
            --pImpl_->inFlightCommands_;
        }
        return true;
    }
    return false;
}

std::uint64_t AudioEngine::droppedAcks() const noexcept {
    return pImpl_ ? pImpl_->droppedAcks.load(std::memory_order_relaxed) : 0;
}

bool AudioEngine::scheduleAudioEvent(const ScheduledAudioEvent& event) noexcept {
    if (!pImpl_) return false;
    if (pImpl_->scheduledEventQueue.tryPush(event)) {
        return true;
    }
    pImpl_->droppedScheduledEvents.fetch_add(1, std::memory_order_relaxed);
    return false;
}

std::uint64_t AudioEngine::droppedScheduledEvents() const noexcept {
    return pImpl_ ? pImpl_->droppedScheduledEvents.load(std::memory_order_relaxed) : 0;
}

std::uint64_t AudioEngine::totalScheduledEventsProcessed() const noexcept {
    return pImpl_ ? pImpl_->totalScheduledEventsProcessed.load(std::memory_order_relaxed) : 0;
}

std::uint64_t AudioEngine::deferredScheduledEvents() const noexcept {
    return pImpl_ ? pImpl_->deferredScheduledEvents.load(std::memory_order_relaxed) : 0;
}

void AudioEngine::attachAudioProbe(AudioProbe* probe) noexcept {
    if (pImpl_) {
        pImpl_->audioProbe.store(probe, std::memory_order_seq_cst);
    }
}

void AudioEngine::detachAudioProbe() noexcept {
    if (pImpl_) {
        pImpl_->audioProbe.store(nullptr, std::memory_order_seq_cst);
        // Quiescence guarantee: wait until any in-flight processBlock writer has finished using the probe (DIF-M2-01, DIF-M2-09)
        while (pImpl_->probeActiveWriters_.load(std::memory_order_seq_cst) > 0) {
            #if defined(_MSC_VER) || defined(__x86_64__) || defined(_M_X64)
            _mm_pause();
            #endif
        }
    }
}

AudioProbe* AudioEngine::audioProbe() const noexcept {
    return pImpl_ ? pImpl_->audioProbe.load(std::memory_order_acquire) : nullptr;
}

void AudioEngine::processBlock(float* pOutput, std::uint32_t frameCount) noexcept {
    if (pImpl_) {
        pImpl_->processBlock(pOutput, frameCount);
    }
}

const VoiceAllocator& AudioEngine::voiceAllocator() const noexcept {
    static const VoiceAllocator empty{};
    return pImpl_ ? pImpl_->voiceAllocator : empty;
}

VoiceAllocator& AudioEngine::voiceAllocator() noexcept {
    static VoiceAllocator fallback{};
    return pImpl_ ? pImpl_->voiceAllocator : fallback;
}

} // namespace cymatica::audio
