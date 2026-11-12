#pragma once

#include "device_loopback_tap.h"

#include <cstdint>
#include <vector>

namespace cymatica::audio {

// Diagnostic capture of what the Windows mixer sends to the default render endpoint (M2.5.1d, ADR-0007).
// Runs on its own device thread, outside the engine, and only inside device_probe_v1: the runtime never uses it.
// Lives in cymatica_audio_diag: this is the only capture device of the project and cymatica_audio does not contain it.
class LoopbackCapture final : public DeviceLoopbackTap {
public:
    LoopbackCapture(std::uint64_t maxFrames, std::uint32_t sampleRate);
    ~LoopbackCapture() override;
    LoopbackCapture(const LoopbackCapture&) = delete;
    LoopbackCapture& operator=(const LoopbackCapture&) = delete;

    [[nodiscard]] bool start() override;
    void stop() override;

    [[nodiscard]] std::uint64_t capturedFrames() const noexcept override;
    [[nodiscard]] std::uint64_t overflowFrames() const noexcept override; // frames that did not fit in the buffer
    [[nodiscard]] std::uint32_t mixSampleRate() const noexcept override;  // format of the mix before any conversion
    [[nodiscard]] std::uint32_t mixChannels() const noexcept override;
    // Interleaved stereo float. Read only after stop(): the capture thread owns it while running.
    [[nodiscard]] const std::vector<float>& samples() const noexcept override;
    [[nodiscard]] EndpointVolumeReading volumeAtStart() const noexcept override;
    [[nodiscard]] EndpointVolumeReading volumeAtStop() const noexcept override;
    [[nodiscard]] SessionVolumeReading sessionsAtStart() const noexcept override;
    [[nodiscard]] SessionVolumeReading sessionsAtStop() const noexcept override;

private:
    struct Impl;
    Impl* pImpl_{nullptr};
};

} // namespace cymatica::audio
