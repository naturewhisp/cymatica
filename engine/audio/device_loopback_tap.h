#pragma once

#include <cstdint>
#include <vector>

namespace cymatica::audio {

// What device_probe_v1 needs from a capture of the Windows mix (M2.5.1d, ADR-0007).
// cymatica_audio only knows this interface: it never opens a capture device. The implementation lives in the
// diagnostic target cymatica_audio_diag, so the engine library keeps a single device owner (CONS-01).
class DeviceLoopbackTap {
public:
    virtual ~DeviceLoopbackTap() = default;

    [[nodiscard]] virtual bool start() = 0;
    virtual void stop() = 0;

    [[nodiscard]] virtual std::uint64_t capturedFrames() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t overflowFrames() const noexcept = 0; // frames that did not fit in the buffer
    [[nodiscard]] virtual std::uint32_t mixSampleRate() const noexcept = 0;  // format of the mix before any conversion
    [[nodiscard]] virtual std::uint32_t mixChannels() const noexcept = 0;
    // Interleaved stereo float. Read only after stop(): the capture thread owns it while running.
    [[nodiscard]] virtual const std::vector<float>& samples() const noexcept = 0;
};

} // namespace cymatica::audio
