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

        // 2. Consume identified commands and emit acknowledgements (§20.7, §20.8, DIF-M1-18, DIF-M1-19, ADR-0003 D-M2-01)
        AudioCommand cmd;
        for (std::uint32_t i = 0; i < AudioEngine::kMaxCommandsPerBlock; ++i) {
            if (!commandQueue.peek(cmd)) break;

            // Hold commands scheduled for future sample frames beyond the current block (DIF-M2-02)
            if (cmd.targetSampleFrame > 0 && cmd.targetSampleFrame >= blockEndFrame) {
                break;
            }

            (void)commandQueue.tryPop(cmd);
            AudioCommandStatus status = AudioCommandStatus::Applied;
            std::uint64_t effectiveFrame = blockStartFrame;

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
                        ma_waveform_set_amplitude(&waveform, 0.0f);
                        transportState = TransportState::Paused;
                        ++transportEpoch;
                    } else {
                        status = AudioCommandStatus::Rejected;
                    }
                    break;
                case AudioCommandType::Resume:
                    if (isValidTransportTransition(transportState, TransportState::Resuming)) {
                        transportState = TransportState::Resuming;
                        ma_waveform_set_amplitude(&waveform, activeVolume);
                        transportState = TransportState::Running;
                        ++transportEpoch;
                    } else {
                        status = AudioCommandStatus::Rejected;
                    }
                    break;
                case AudioCommandType::Stop:
                    if (isValidTransportTransition(transportState, TransportState::Stopped)) {
                        ma_waveform_set_amplitude(&waveform, 0.0f);
                        transportState = TransportState::Stopped;
                        ++transportEpoch;
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
                        if (cmd.targetSampleFrame > blockStartFrame && cmd.targetSampleFrame < blockEndFrame) {
                            effectiveFrame = cmd.targetSampleFrame;
                        } else {
                            effectiveFrame = blockStartFrame;
                        }
                        (void)musicClock.setTempoMap(newMap, effectiveFrame);
                        ++transportEpoch;
                    } else {
                        status = AudioCommandStatus::Rejected;
                    }
                    break;
                }
                default:
                    status = AudioCommandStatus::Rejected;
                    break;
            }

            if (!ackQueue.tryPush(AudioCommandAck{
                .commandId = cmd.commandId,
                .status = status,
                .sampleFrame = effectiveFrame,
                .transportEpoch = transportEpoch,
            })) {
                droppedAcks.fetch_add(1, std::memory_order_relaxed);
            }
        }

        // 3. Render PCM audio samples / handle paused/stopped silence (§20.8, DIF-M1-19, DIF-M2-03)
        if (transportState == TransportState::Paused || transportState == TransportState::Stopped) {
            if (pOutput != nullptr && frameCount > 0) {
                for (std::size_t i = 0; i < static_cast<std::size_t>(frameCount) * channels; ++i) {
                    pOutput[i] = 0.0f;
                }
            }
            // Timeline and scheduled event consumption are frozen during pause / stopped
        } else {
            // Render block with sample-accurate scheduled event offsets (Spec §7.5, ADR-0003 D-M2-03, DIF-M2-03)
            std::uint32_t currentDevOffset = 0;

            while (currentDevOffset < frameCount) {
                ScheduledAudioEvent schedEvt;
                if (scheduledEventQueue.peek(schedEvt) && schedEvt.sampleFrame < blockEndFrame) {
                    // Determine device offset for event
                    std::uint32_t targetDevOffset = currentDevOffset;
                    if (schedEvt.sampleFrame > blockStartFrame) {
                        const std::uint64_t devFrame = musicClock.internalToDeviceFrames(schedEvt.sampleFrame, sampleRate);
                        if (devFrame > deviceFramesRendered) {
                            targetDevOffset = static_cast<std::uint32_t>(devFrame - deviceFramesRendered);
                            if (targetDevOffset > frameCount) targetDevOffset = frameCount;
                        }
                    }
                    if (targetDevOffset < currentDevOffset) targetDevOffset = currentDevOffset;

                    // Render slice before event if any
                    if (targetDevOffset > currentDevOffset) {
                        const std::uint32_t sliceFrames = targetDevOffset - currentDevOffset;
                        if (pOutput != nullptr) {
                            ma_waveform_read_pcm_frames(&waveform, pOutput + (static_cast<std::size_t>(currentDevOffset) * channels), sliceFrames, nullptr);
                        }
                        currentDevOffset = targetDevOffset;
                    }

                    // Consume and apply event at exact offset
                    (void)scheduledEventQueue.tryPop(schedEvt);
                    totalScheduledEventsProcessed.fetch_add(1, std::memory_order_relaxed);

                    switch (schedEvt.type) {
                        case AudioEventType::NoteOn:
                            ma_waveform_set_frequency(&waveform, schedEvt.valueA);
                            ma_waveform_set_amplitude(&waveform, schedEvt.valueB * activeVolume);
                            break;
                        case AudioEventType::NoteOff:
                            ma_waveform_set_amplitude(&waveform, 0.0f);
                            break;
                        case AudioEventType::ParameterChange:
                            if (schedEvt.valueA > 0.0f) {
                                ma_waveform_set_frequency(&waveform, schedEvt.valueA);
                            }
                            if (schedEvt.valueB >= 0.0f) {
                                ma_waveform_set_amplitude(&waveform, schedEvt.valueB * activeVolume);
                            }
                            break;
                    }
                } else {
                    // No more events in this block: render remaining frames to block boundary
                    const std::uint32_t remainingFrames = frameCount - currentDevOffset;
                    if (pOutput != nullptr && remainingFrames > 0) {
                        ma_waveform_read_pcm_frames(&waveform, pOutput + (static_cast<std::size_t>(currentDevOffset) * channels), remainingFrames, nullptr);
                    }
                    currentDevOffset = frameCount;
                }
            }

            deviceFramesRendered += frameCount;
            renderCursor.store(musicClock.deviceToInternalFrames(deviceFramesRendered, sampleRate), std::memory_order_release);
        }

        // 3b. AudioProbe tap: lock-free, zero-alloc PCM capture with quiescence tracking (ADR-0003 D-AO-02, Spec §31.5, DIF-M2-01)
        AudioProbe* probe = audioProbe.load(std::memory_order_acquire);
        if (probe != nullptr && pOutput != nullptr && frameCount > 0) {
            probeActiveWriters_.fetch_add(1, std::memory_order_acquire);
            AudioProbe* activeProbe = audioProbe.load(std::memory_order_relaxed);
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

void AudioEngine::attachAudioProbe(AudioProbe* probe) noexcept {
    if (pImpl_) {
        pImpl_->audioProbe.store(probe, std::memory_order_release);
    }
}

void AudioEngine::detachAudioProbe() noexcept {
    if (pImpl_) {
        pImpl_->audioProbe.store(nullptr, std::memory_order_release);
        // Quiescence guarantee: wait until any in-flight processBlock writer has finished using the probe (DIF-M2-01)
        while (pImpl_->probeActiveWriters_.load(std::memory_order_acquire) > 0) {
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

} // namespace cymatica::audio
