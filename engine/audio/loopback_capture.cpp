#include "loopback_capture.h"

#include "miniaudio.h"

#include <algorithm>
#include <atomic>
#include <cstring>

namespace cymatica::audio {

struct LoopbackCapture::Impl {
    ma_device device{};
    bool deviceInitialized{false};
    bool started{false};
    std::uint32_t sampleRate{48000};
    std::uint64_t maxFrames{0};
    std::vector<float> buffer{};
    std::atomic<std::uint64_t> frames{0};
    std::atomic<std::uint64_t> overflow{0};

    static void dataCallback(ma_device* pDevice, void*, const void* pInput, ma_uint32 frameCount) {
        auto* impl = static_cast<Impl*>(pDevice->pUserData);
        const std::uint64_t written = impl->frames.load(std::memory_order_relaxed);
        const std::uint64_t room = impl->maxFrames - written;
        const auto copied = static_cast<ma_uint32>((std::min<std::uint64_t>)(frameCount, room));
        if (copied > 0 && pInput != nullptr) {
            std::memcpy(impl->buffer.data() + written * 2, pInput, static_cast<std::size_t>(copied) * 2 * sizeof(float));
            impl->frames.store(written + copied, std::memory_order_release);
        }
        if (copied < frameCount) impl->overflow.fetch_add(frameCount - copied, std::memory_order_relaxed);
    }
};

LoopbackCapture::LoopbackCapture(std::uint64_t maxFrames, std::uint32_t sampleRate) : pImpl_(new Impl{}) {
    pImpl_->sampleRate = sampleRate;
    pImpl_->maxFrames = maxFrames;
    pImpl_->buffer.resize(static_cast<std::size_t>(maxFrames) * 2);
}

LoopbackCapture::~LoopbackCapture() {
    stop();
    if (pImpl_->deviceInitialized) ma_device_uninit(&pImpl_->device);
    delete pImpl_;
}

bool LoopbackCapture::start() {
    if (!pImpl_->deviceInitialized) {
        ma_device_config config = ma_device_config_init(ma_device_type_loopback);
        config.capture.format = ma_format_f32;
        config.capture.channels = 2;
        config.sampleRate = pImpl_->sampleRate;
        config.dataCallback = Impl::dataCallback;
        config.pUserData = pImpl_;
        ma_backend backends[] = {ma_backend_wasapi}; // loopback exists only on WASAPI
        if (ma_device_init_ex(backends, 1, nullptr, &config, &pImpl_->device) != MA_SUCCESS) return false;
        pImpl_->deviceInitialized = true;
    }
    if (ma_device_start(&pImpl_->device) != MA_SUCCESS) return false;
    pImpl_->started = true;
    return true;
}

void LoopbackCapture::stop() {
    if (pImpl_->started) {
        ma_device_stop(&pImpl_->device);
        pImpl_->started = false;
    }
}

std::uint64_t LoopbackCapture::capturedFrames() const noexcept {
    return pImpl_->frames.load(std::memory_order_acquire);
}

std::uint64_t LoopbackCapture::overflowFrames() const noexcept {
    return pImpl_->overflow.load(std::memory_order_relaxed);
}

std::uint32_t LoopbackCapture::mixSampleRate() const noexcept {
    return pImpl_->deviceInitialized ? pImpl_->device.capture.internalSampleRate : 0;
}

std::uint32_t LoopbackCapture::mixChannels() const noexcept {
    return pImpl_->deviceInitialized ? pImpl_->device.capture.internalChannels : 0;
}

const std::vector<float>& LoopbackCapture::samples() const noexcept {
    return pImpl_->buffer;
}

} // namespace cymatica::audio
