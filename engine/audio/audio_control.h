#pragma once

#include <cstdint>

namespace cymatica::audio {

// Continuous latest-value parameters sent from Game -> Audio via TripleBuffer (Spec §22.1)
struct AudioControlFrame {
    float dissonance{0.0f};
    float playerPerformance{1.0f};
    std::uint32_t requestedArchetypeId{0};
    float toneFrequencyHz{220.0f};
    float toneVolume{0.25f};

    [[nodiscard]] constexpr bool operator==(const AudioControlFrame& other) const noexcept = default;
};

// Discrete identified command types for actions requiring acknowledgement (§20.7, ADR-0002)
enum class AudioCommandType : std::uint32_t {
    None    = 0,
    Start   = 1,
    Pause   = 2,
    Resume  = 3,
    Stop    = 4,
    SetTone = 5,
};

// Transport state machine defined in Spec §20.8 (DIF-M1-19)
enum class TransportState : std::uint32_t {
    Running  = 0,
    Pausing  = 1,
    Paused   = 2,
    Resuming = 3,
};

[[nodiscard]] constexpr bool isValidTransportTransition(TransportState from, TransportState to) noexcept {
    if (from == to) return true;
    switch (from) {
        case TransportState::Running:
            return to == TransportState::Pausing || to == TransportState::Paused;
        case TransportState::Pausing:
            return to == TransportState::Paused;
        case TransportState::Paused:
            return to == TransportState::Resuming || to == TransportState::Running;
        case TransportState::Resuming:
            return to == TransportState::Running;
        default:
            return false;
    }
}

enum class AudioCommandStatus : std::uint32_t {
    Pending  = 0,
    Applied  = 1,
    Rejected = 2,
};

// Command sent Game -> Audio SPSC
struct AudioCommand {
    std::uint64_t commandId{0};
    AudioCommandType type{AudioCommandType::None};
    float paramF32{0.0f};
    std::uint64_t paramU64{0};

    [[nodiscard]] constexpr bool operator==(const AudioCommand& other) const noexcept = default;
};

// Acknowledgement emitted Audio -> Game SPSC
struct AudioCommandAck {
    std::uint64_t commandId{0};
    AudioCommandStatus status{AudioCommandStatus::Pending};
    std::uint64_t sampleFrame{0};
    std::uint64_t transportEpoch{0};

    [[nodiscard]] constexpr bool operator==(const AudioCommandAck& other) const noexcept = default;
};

} // namespace cymatica::audio
