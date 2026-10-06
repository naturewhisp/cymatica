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
    ma_waveform waveform{};   // owned by the audio callback after init
    bool deviceStarted{false}; // control thread only
    std::uint32_t sampleRate{0};

    // Control thread -> callback: latest-value parameters (spec section 20.7).
    // Plain atomics of trivially small types; no payload copied concurrently.
    std::atomic<float> targetFreqHz{220.0f};
    std::atomic<float> targetAmplitude{0.0f};

    // Callback -> control thread telemetry.
    std::atomic<std::uint64_t> totalFrames{0};

    // Callback-private cache of applied values.
    float appliedFreqHz{-1.0f};
    float appliedAmplitude{-1.0f};

    // Realtime path: bounded work, no allocation, no locks, no I/O, no logging
    // (spec sections 4.5, 20.2). Parameter changes are applied here, so the
    // waveform state is never touched concurrently by the control thread.
    static void dataCallback(ma_device* pDevice, void* pOutput, const void*, ma_uint32 frameCount) {
        auto* impl = static_cast<Impl*>(pDevice->pUserData);
        const float freq = impl->targetFreqHz.load(std::memory_order_relaxed);
        const float amp = impl->targetAmplitude.load(std::memory_order_relaxed);
        if (freq != impl->appliedFreqHz) {
            ma_waveform_set_frequency(&impl->waveform, freq);
            impl->appliedFreqHz = freq;
        }
        if (amp != impl->appliedAmplitude) {
            ma_waveform_set_amplitude(&impl->waveform, amp);
            impl->appliedAmplitude = amp;
        }
        ma_waveform_read_pcm_frames(&impl->waveform, pOutput, frameCount, nullptr);
        impl->totalFrames.fetch_add(frameCount, std::memory_order_relaxed);
    }
};

AudioEngine::~AudioEngine() { shutdown(); }

bool AudioEngine::init(const AudioEngineConfig& config) {
    if (pImpl_) return true;

    auto impl = std::make_unique<Impl>();
    impl->targetFreqHz.store(config.defaultToneFreqHz, std::memory_order_relaxed);
    impl->targetAmplitude.store(0.0f, std::memory_order_relaxed); // silent until startTone

    ma_device_config devCfg = ma_device_config_init(ma_device_type_playback);
    devCfg.playback.format = ma_format_f32;
    devCfg.playback.channels = config.channels;
    devCfg.sampleRate = config.sampleRate;
    devCfg.dataCallback = Impl::dataCallback;
    devCfg.pUserData = impl.get();

    if (ma_device_init(nullptr, &devCfg, &impl->device) != MA_SUCCESS) return false;

    // The waveform is initialized before the device starts, so the callback
    // never observes a partially constructed object.
    ma_waveform_config waveCfg = ma_waveform_config_init(
        impl->device.playback.format, impl->device.playback.channels, impl->device.sampleRate,
        ma_waveform_type_sine, 0.0f, config.defaultToneFreqHz);
    if (ma_waveform_init(&waveCfg, &impl->waveform) != MA_SUCCESS) {
        ma_device_uninit(&impl->device);
        return false;
    }
    impl->sampleRate = impl->device.sampleRate;

    pImpl_ = impl.release();
    return true;
}

void AudioEngine::shutdown() {
    if (!pImpl_) return;
    // Stop/uninit on the control thread, never inside the callback (spec section 20.8).
    ma_device_uninit(&pImpl_->device);
    delete pImpl_;
    pImpl_ = nullptr;
}

bool AudioEngine::startTone(float freqHz, float amplitude) {
    if (!pImpl_) return false;
    pImpl_->targetFreqHz.store(freqHz, std::memory_order_relaxed);
    pImpl_->targetAmplitude.store(amplitude, std::memory_order_relaxed);
    if (!pImpl_->deviceStarted) {
        if (ma_device_start(&pImpl_->device) != MA_SUCCESS) return false;
        pImpl_->deviceStarted = true;
    }
    return true;
}

void AudioEngine::setToneFrequency(float freqHz) noexcept {
    if (pImpl_) pImpl_->targetFreqHz.store(freqHz, std::memory_order_relaxed);
}

void AudioEngine::stopTone() {
    if (!pImpl_ || !pImpl_->deviceStarted) return;
    ma_device_stop(&pImpl_->device);
    pImpl_->deviceStarted = false;
}

bool AudioEngine::isRunning() const noexcept { return pImpl_ && pImpl_->deviceStarted; }

std::uint64_t AudioEngine::framesRendered() const noexcept {
    return pImpl_ ? pImpl_->totalFrames.load(std::memory_order_relaxed) : 0;
}

std::uint32_t AudioEngine::sampleRate() const noexcept { return pImpl_ ? pImpl_->sampleRate : 0; }

} // namespace cymatica::audio
