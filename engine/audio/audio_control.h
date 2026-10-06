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
