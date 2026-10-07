#pragma once

#include "audio_control.h"
#include "music_position.h"

#include <cstdint>

namespace cymatica::audio {

// Four-channel role representation defined in Spec §8 and §22.1
struct ChannelFrame {
    float energy{0.0f};
    float onset{0.0f};
    float pitchHz{0.0f};
    float density{0.0f};
    float confidence{0.0f};

    [[nodiscard]] constexpr bool operator==(const ChannelFrame& other) const noexcept = default;
};

// Compact latest-value telemetry published by audio callback to game thread (Spec §22.1)
struct AudioTelemetryFrame {
    std::uint64_t sequenceNumber{0};      // Spec §20.7 monotonic sequence counter for drop detection
    std::uint64_t transportEpoch{0};      // §7.5
    TransportState transportState{TransportState::Running}; // §20.8
    std::uint64_t renderCursor{0};        // First frame yet to be synthesized (internal 48kHz timeline)
    std::uint64_t presentationCursor{0};  // Estimated audible frame output
    PresentationQuality presentationQuality{PresentationQuality::Invalid};
    float bpm{120.0f};
    float beatPhase{0.0f};                // Derived view, never authoritative
    float beatConfidence{1.0f};
    ChannelFrame pulse{};
    ChannelFrame body{};
    ChannelFrame texture{};
    ChannelFrame vector{};
    float globalEnergy{0.0f};
    float spectralFlux{0.0f};
    std::uint32_t detectedArchetypeId{0};
    std::uint64_t framesRenderedTotal{0};

    [[nodiscard]] constexpr bool operator==(const AudioTelemetryFrame& other) const noexcept = default;
};

} // namespace cymatica::audio
