// Single translation unit containing MINIAUDIO_IMPLEMENTATION in the whole project
// (spec section 28.2, ADR-0001). No other target may open an audio device.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "audio_engine.h"

#include <atomic>
#include <memory>

namespace cymatica::audio {

struct AudioEngine::Impl {
    ma_device device{};
    ma_waveform waveform{};
    bool deviceStarted{false};
    std::uint32_t sampleRate{0};
    std::uint32_t channels{2};

    // Transport cursors and epoch (§7.5, ADR-0002)
    std::uint64_t transportEpoch{1};
    std::uint64_t renderCursor{0};

    // Lock-free exchange channels (§20.5–§20.7, ADR-0002)
    TripleBuffer<AudioTelemetryFrame> telemetryBuffer{};
    TripleBuffer<AudioControlFrame> controlBuffer{};
    SpscQueue<AudioCommand, 64> commandQueue{};
    SpscQueue<AudioCommandAck, 64> ackQueue{};

    // Realtime processing function: zero heap allocation, bounded execution (§20.2)
    void processBlock(float* pOutput, std::uint32_t frameCount) noexcept {
        // 1. Consume continuous control updates
        if (controlBuffer.update()) {
            const auto& ctrl = controlBuffer.readSlot();
            ma_waveform_set_frequency(&waveform, ctrl.toneFrequencyHz);
            ma_waveform_set_amplitude(&waveform, ctrl.toneVolume);
        }

        // 2. Consume identified commands and emit acknowledgements (§20.7)
        AudioCommand cmd;
        while (commandQueue.tryPop(cmd)) {
            AudioCommandStatus status = AudioCommandStatus::Applied;
            switch (cmd.type) {
                case AudioCommandType::Start:
                    ma_waveform_set_amplitude(&waveform, cmd.paramF32);
                    break;
                case AudioCommandType::Pause:
                    ma_waveform_set_amplitude(&waveform, 0.0f);
                    ++transportEpoch;
                    break;
                case AudioCommandType::Resume:
                    ++transportEpoch;
                    break;
                case AudioCommandType::Stop:
                    ma_waveform_set_amplitude(&waveform, 0.0f);
                    ++transportEpoch;
                    break;
                case AudioCommandType::SetTone:
                    ma_waveform_set_frequency(&waveform, cmd.paramF32);
                    break;
                default:
                    status = AudioCommandStatus::Rejected;
                    break;
            }
            ackQueue.tryPush(AudioCommandAck{
                .commandId = cmd.commandId,
                .status = status,
                .sampleFrame = renderCursor,
                .transportEpoch = transportEpoch,
            });
        }

        // 3. Render PCM audio samples
        if (pOutput != nullptr && frameCount > 0) {
            ma_waveform_read_pcm_frames(&waveform, pOutput, frameCount, nullptr);
        }

        renderCursor += frameCount;

        // 4. Publish latest telemetry (§22.1)
        auto& telem = telemetryBuffer.writeSlot();
        telem.transportEpoch = transportEpoch;
        telem.renderCursor = renderCursor;
        telem.presentationCursor = renderCursor >= frameCount ? renderCursor - frameCount : 0;
        telem.presentationQuality = PresentationQuality::Estimated;
        telem.framesRenderedTotal = renderCursor;
        telem.bpm = 120.0f;
        telem.globalEnergy = 0.5f;
        telem.pulse.energy = 0.5f;
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

    // Seed initial control values
    auto& ctrl = impl->controlBuffer.writeSlot();
    ctrl.toneFrequencyHz = config.defaultToneFreqHz;
    ctrl.toneVolume = 0.0f;
    impl->controlBuffer.publish();

    pImpl_ = impl.release();
    return true;
}

void AudioEngine::shutdown() {
    if (!pImpl_) return;
    ma_device_uninit(&pImpl_->device);
    delete pImpl_;
    pImpl_ = nullptr;
}

bool AudioEngine::startTone(float freqHz, float amplitude) {
    if (!pImpl_) return false;
    auto& ctrl = pImpl_->controlBuffer.writeSlot();
    ctrl.toneFrequencyHz = freqHz;
    ctrl.toneVolume = amplitude;
    pImpl_->controlBuffer.publish();

    if (!pImpl_->deviceStarted) {
        if (ma_device_start(&pImpl_->device) != MA_SUCCESS) {
            return false;
        }
        pImpl_->deviceStarted = true;
    }
    return true;
}

void AudioEngine::setToneFrequency(float freqHz) noexcept {
    if (!pImpl_) return;
    auto& ctrl = pImpl_->controlBuffer.writeSlot();
    ctrl.toneFrequencyHz = freqHz;
    pImpl_->controlBuffer.publish();
}

void AudioEngine::stopTone() {
    if (!pImpl_ || !pImpl_->deviceStarted) return;
    ma_device_stop(&pImpl_->device);
    pImpl_->deviceStarted = false;
}

bool AudioEngine::isRunning() const noexcept {
    return pImpl_ && pImpl_->deviceStarted;
}

std::uint64_t AudioEngine::framesRendered() const noexcept {
    return pImpl_ ? pImpl_->renderCursor : 0;
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
    return pImpl_ ? pImpl_->controlBuffer.writeSlot() : dummy;
}

void AudioEngine::publishControl() noexcept {
    if (pImpl_) {
        pImpl_->controlBuffer.publish();
    }
}

bool AudioEngine::sendCommand(const AudioCommand& cmd) noexcept {
    return pImpl_ ? pImpl_->commandQueue.tryPush(cmd) : false;
}

bool AudioEngine::pollAck(AudioCommandAck& ack) noexcept {
    return pImpl_ ? pImpl_->ackQueue.tryPop(ack) : false;
}

void AudioEngine::processBlock(float* pOutput, std::uint32_t frameCount) noexcept {
    if (pImpl_) {
        pImpl_->processBlock(pOutput, frameCount);
    }
}

} // namespace cymatica::audio
