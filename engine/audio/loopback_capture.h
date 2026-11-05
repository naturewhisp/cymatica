#pragma once

#include <cstdint>
#include <vector>

namespace cymatica::audio {

// Diagnostic capture of what the Windows mixer sends to the default render endpoint (M2.5.1d, ADR-0007).
// Runs on its own device thread, outside the engine, and only inside device_probe_v1: the runtime never uses it.
class LoopbackCapture {
public:
    LoopbackCapture(std::uint64_t maxFrames, std::uint32_t sampleRate);
    ~LoopbackCapture();
    LoopbackCapture(const LoopbackCapture&) = delete;
    LoopbackCapture& operator=(const LoopbackCapture&) = delete;

    [[nodiscard]] bool start();
    void stop();

    [[nodiscard]] std::uint64_t capturedFrames() const noexcept;
    [[nodiscard]] std::uint64_t overflowFrames() const noexcept; // frames that did not fit in the buffer
    [[nodiscard]] std::uint32_t mixSampleRate() const noexcept;  // format of the mix before any conversion
    [[nodiscard]] std::uint32_t mixChannels() const noexcept;
    // Interleaved stereo float. Read only after stop(): the capture thread owns it while running.
    [[nodiscard]] const std::vector<float>& samples() const noexcept;

private:
    struct Impl;
    Impl* pImpl_{nullptr};
};

} // namespace cymatica::audio
