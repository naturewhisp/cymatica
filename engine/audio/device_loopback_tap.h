#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace cymatica::audio {

// Master volume of the endpoint the loopback captures (CP-1, R-M25-12), read only. valid is false when unreadable.
struct EndpointVolumeReading {
    bool valid{false};
    float masterDb{0.0f};
    float scalar{0.0f};       // 0..1, the value of the Windows volume slider
    bool mute{false};
    float minDb{0.0f};
    float maxDb{0.0f};
    float stepDb{0.0f};
    bool hardwareVolume{false};
    bool hardwareMute{false};
};

// Audio sessions of this process on that endpoint (CP-1, H2): the per-application volume of the Windows mixer, read only
struct SessionVolumeEntry {
    float level{1.0f};                       // ISimpleAudioVolume, 0..1
    bool mute{false};
    std::uint32_t channels{0};               // 0 = channel volumes not readable
    std::array<float, 8> channelLevels{};    // IChannelAudioVolume, first `channels` entries
    bool active{false};                      // AudioSessionStateActive
};
struct SessionVolumeReading {
    bool valid{false};                       // enumeration succeeded (count may still be 0)
    std::uint32_t count{0};
    std::array<SessionVolumeEntry, 4> sessions{};
};

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
    // Endpoint volume read when the capture started and when it stopped (invalid when the tap does not read it)
    [[nodiscard]] virtual EndpointVolumeReading volumeAtStart() const noexcept { return {}; }
    [[nodiscard]] virtual EndpointVolumeReading volumeAtStop() const noexcept { return {}; }
    [[nodiscard]] virtual SessionVolumeReading sessionsAtStart() const noexcept { return {}; }
    [[nodiscard]] virtual SessionVolumeReading sessionsAtStop() const noexcept { return {}; }
};

} // namespace cymatica::audio
