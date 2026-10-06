#pragma once

#include <cstdint>

namespace cymatica::core {

// Stream hierarchy defined in Spec §15.2:
// Gameplay streams must not depend on VFX or cosmetic streams.
enum class StreamId : std::uint32_t {
    MusicForm         = 1,
    Harmony           = 2,
    Rhythm            = 3,
    Director          = 4,
    Pattern           = 5,
    Adaptation        = 6,
    AudioHumanization = 7,
    Vfx               = 8,
    Cosmetic          = 9,
};

// Stateless random key defined in Spec §15.3
struct RandomKey {
    std::uint64_t runSeed{0};
    std::uint32_t streamId{0};
    std::uint64_t decisionId{0};
    std::uint32_t sampleIndex{0};

    [[nodiscard]] constexpr bool operator==(const RandomKey& other) const noexcept = default;
};

} // namespace cymatica::core
