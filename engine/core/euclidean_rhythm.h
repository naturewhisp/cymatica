#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace cymatica::core {

inline constexpr std::size_t kMaxEuclideanSteps = 32;

namespace detail {

// Pure constexpr canonical Bjorklund partitioning algorithm (ADR-0003 D-M2-02, INC3-R08)
// Generates exact maximally-even distributions (Toussaint 2005) with downbeat on index 0.
constexpr std::array<bool, kMaxEuclideanSteps> bjorklundCanonical(std::uint32_t steps, std::uint32_t pulses) noexcept {
    std::array<bool, kMaxEuclideanSteps> result{};
    if (steps == 0 || pulses == 0) {
        return result;
    }
    if (pulses >= steps) {
        for (std::uint32_t i = 0; i < steps; ++i) {
            result[i] = true;
        }
        return result;
    }

    // Sequence structure for partitioning: up to 32 sequences of max 32 bits
    struct BitSeq {
        std::uint32_t data{0}; // Bit 0 is first onset
        std::uint8_t len{0};
    };

    std::array<BitSeq, kMaxEuclideanSteps> heads{};
    std::array<BitSeq, kMaxEuclideanSteps> rems{};

    for (std::uint32_t i = 0; i < pulses; ++i) {
        heads[i] = BitSeq{.data = 1u, .len = 1u}; // [1]
    }
    const std::uint32_t zeros = steps - pulses;
    for (std::uint32_t i = 0; i < zeros; ++i) {
        rems[i] = BitSeq{.data = 0u, .len = 1u};   // [0]
    }

    std::uint32_t headCount = pulses;
    std::uint32_t remCount = zeros;

    while (remCount > 1) {
        const std::uint32_t pairs = (headCount < remCount) ? headCount : remCount;
        for (std::uint32_t i = 0; i < pairs; ++i) {
            // Append rem[i] to heads[i]
            heads[i].data |= (rems[i].data << heads[i].len);
            heads[i].len = static_cast<std::uint8_t>(heads[i].len + rems[i].len);
        }

        if (headCount <= remCount) {
            // Remainder exceeds heads: leftover remainders become the new remainders
            const std::uint32_t newRemCount = remCount - headCount;
            for (std::uint32_t i = 0; i < newRemCount; ++i) {
                rems[i] = rems[headCount + i];
            }
            remCount = newRemCount;
        } else {
            // Heads exceed remainder: leftover heads become the new remainders
            const std::uint32_t newRemCount = headCount - remCount;
            for (std::uint32_t i = 0; i < newRemCount; ++i) {
                rems[i] = heads[remCount + i];
            }
            headCount = pairs;
            remCount = newRemCount;
        }
    }

    // Flatten sequences into final pattern
    std::uint32_t writeIdx = 0;
    for (std::uint32_t i = 0; i < headCount; ++i) {
        for (std::uint32_t b = 0; b < heads[i].len && writeIdx < steps; ++b) {
            result[writeIdx++] = ((heads[i].data >> b) & 1u) != 0;
        }
    }
    for (std::uint32_t i = 0; i < remCount; ++i) {
        for (std::uint32_t b = 0; b < rems[i].len && writeIdx < steps; ++b) {
            result[writeIdx++] = ((rems[i].data >> b) & 1u) != 0;
        }
    }

    return result;
}

} // namespace detail

// Pure Bjorklund Euclidean Rhythm generator (Spec §16.5, ADR-0003 D-M2-02, INC3-R08):
// - Clamps `steps` to [0, 32].
// - Canonical Downbeat on 0 convention: if pulses >= 1, base pattern has onset at index 0.
// - `rotation`: circular shift. rotation > 0 delays onsets (moves right by rotation steps).
[[nodiscard]] constexpr std::array<bool, kMaxEuclideanSteps> euclideanRhythm(
    std::uint32_t steps,
    std::uint32_t pulses,
    std::int32_t rotation = 0
) noexcept {
    std::array<bool, kMaxEuclideanSteps> out{};
    if (steps == 0) return out;
    if (steps > kMaxEuclideanSteps) steps = static_cast<std::uint32_t>(kMaxEuclideanSteps);

    const auto base = detail::bjorklundCanonical(steps, pulses);
    if (steps <= 1 || pulses == 0 || pulses >= steps) {
        return base;
    }

    // Circular rotation
    const std::int32_t s = static_cast<std::int32_t>(steps);
    const std::uint32_t rot = static_cast<std::uint32_t>(((rotation % s) + s) % s);

    if (rot == 0) {
        return base;
    }

    for (std::uint32_t i = 0; i < steps; ++i) {
        out[(i + rot) % steps] = base[i];
    }
    return out;
}

[[nodiscard]] constexpr std::uint32_t countEuclideanPulses(
    const std::array<bool, kMaxEuclideanSteps>& pattern,
    std::uint32_t steps
) noexcept {
    const std::uint32_t count = (steps < kMaxEuclideanSteps) ? steps : static_cast<std::uint32_t>(kMaxEuclideanSteps);
    std::uint32_t total = 0;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (pattern[i]) ++total;
    }
    return total;
}

} // namespace cymatica::core
