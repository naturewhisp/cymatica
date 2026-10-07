// Single translation unit containing MINIAUDIO_IMPLEMENTATION in the whole project
// (spec section 28.2, ADR-0001). No other target may open an audio device.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "audio_engine.h"
#include "music_clock.h"

#include <atomic>
#include <memory>

namespace cymatica::audio {

struct AudioEngine::Impl {
    static constexpr std::size_t kCommandCapacity = 128;

    ma_device device{};
    ma_waveform waveform{};
    std::atomic<bool> deviceStarted{false};
    std::uint32_t sampleRate{48000};
    std::uint32_t channels{2};

    // Master control state owned by the control/game thread (§20.5, DIF-M1-17)
    AudioControlFrame masterControl_{};
    std::size_t inFlightCommands_{0}; // Tracked on control/game thread for guaranteed ack delivery (DIF-M1-18)

    // Transport cursors and epoch (§7.5, §20.8, ADR-0002)
    std::uint64_t deviceFramesRendered{0};
    std::uint64_t transportEpoch{1};
    TransportState transportState{TransportState::Running};
    float activeVolume{0.0f};
    std::atomic<std::uint64_t> renderCursor{0};        // Synchronized authoritative 48 kHz timeline (DIF-M1-16)
    std::atomic<std::uint64_t> droppedAcks{0};         // Dropped command acks defensive counter (DIF-M1-07)
    std::uint64_t telemetrySequence{0};               // Sequence counter for telemetry drops (DIF-M1-13)
    MusicClock musicClock{};

    // Lock-free exchange channels (§20.5–§20.7, ADR-0002)
    TripleBuffer<AudioTelemetryFrame> telemetryBuffer{};
    TripleBuffer<AudioControlFrame> controlBuffer{};
    SpscQueue<AudioCommand, kCommandCapacity> commandQueue{};
    SpscQueue<AudioCommandAck, kCommandCapacity> ackQueue{};

    // Realtime processing function: zero heap allocation, bounded execution (§20.2)
    void processBlock(float* pOutput, std::uint32_t frameCount) noexcept {
        // 1. Consume continuous control updates
        if (controlBuffer.update()) {
            const auto& ctrl = controlBuffer.readSlot();
            activeVolume = ctrl.toneVolume;
            ma_waveform_set_frequency(&waveform, ctrl.toneFrequencyHz);
            if (transportState == TransportState::Running) {
                ma_waveform_set_amplitude(&waveform, activeVolume);
            }
        }

        // 2. Consume identified commands and emit acknowledgements (§20.7, §20.8, DIF-M1-18, DIF-M1-19)
        AudioCommand cmd;
        while (commandQueue.tryPop(cmd)) {
            AudioCommandStatus status = AudioCommandStatus::Applied;
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
                default:
                    status = AudioCommandStatus::Rejected;
                    break;
            }
            const std::uint64_t curCursor = renderCursor.load(std::memory_order_relaxed);
            if (!ackQueue.tryPush(AudioCommandAck{
                .commandId = cmd.commandId,
                .status = status,
                .sampleFrame = curCursor,
                .transportEpoch = transportEpoch,
            })) {
                droppedAcks.fetch_add(1, std::memory_order_relaxed);
            }
        }

        // 3. Render PCM audio samples / handle paused/stopped silence (§20.8, DIF-M1-19)
        if (transportState == TransportState::Paused || transportState == TransportState::Stopped) {
            if (pOutput != nullptr && frameCount > 0) {
                for (std::size_t i = 0; i < static_cast<std::size_t>(frameCount) * channels; ++i) {
                    pOutput[i] = 0.0f;
                }
            }
            // Timeline is frozen during pause / stopped
        } else {
            if (pOutput != nullptr && frameCount > 0) {
                ma_waveform_read_pcm_frames(&waveform, pOutput, frameCount, nullptr);
            }
            deviceFramesRendered += frameCount;
            renderCursor.store(musicClock.deviceToInternalFrames(deviceFramesRendered, sampleRate), std::memory_order_release);
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

void AudioEngine::shutdown() {
    if (!pImpl_) return;
    if (pImpl_->deviceStarted.load(std::memory_order_acquire)) {
        ma_device_stop(&pImpl_->device);
        pImpl_->deviceStarted.store(false, std::memory_order_release);
    }
    ma_device_uninit(&pImpl_->device);
    delete pImpl_;
    pImpl_ = nullptr;
}

bool AudioEngine::startTone(float freqHz, float amplitude) {
    if (!pImpl_) return false;
    pImpl_->masterControl_.toneFrequencyHz = freqHz;
    pImpl_->masterControl_.toneVolume = amplitude;
    publishControl();

    if (!pImpl_->deviceStarted.load(std::memory_order_acquire)) {
        if (ma_device_start(&pImpl_->device) != MA_SUCCESS) {
            return false;
        }
        pImpl_->deviceStarted.store(true, std::memory_order_release);
    }
    return true;
}

void AudioEngine::setToneFrequency(float freqHz) noexcept {
    if (!pImpl_) return;
    pImpl_->masterControl_.toneFrequencyHz = freqHz;
    publishControl();
}

void AudioEngine::stopTone() {
    if (!pImpl_ || !pImpl_->deviceStarted.load(std::memory_order_acquire)) return;
    ma_device_stop(&pImpl_->device);
    pImpl_->deviceStarted.store(false, std::memory_order_release);
}

bool AudioEngine::isRunning() const noexcept {
    return pImpl_ && pImpl_->deviceStarted.load(std::memory_order_acquire);
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

void AudioEngine::processBlock(float* pOutput, std::uint32_t frameCount) noexcept {
    if (pImpl_) {
        pImpl_->processBlock(pOutput, frameCount);
    }
}

} // namespace cymatica::audio
