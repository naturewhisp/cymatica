#pragma once

#include <cstdint>

namespace cymatica::core {

// Explicit numeric policy identifier used by RNG derivation and RunRecord (Spec §15.4, §22.5, ADR-0002).
// Avoids hidden global mutable state.
struct PolicyVersionId {
    std::uint32_t schema{1};
    std::uint32_t major{0};
    std::uint32_t minor{1};
    std::uint32_t patch{0};

    [[nodiscard]] constexpr std::uint32_t toU32() const noexcept {
        return (schema << 24) | (major << 16) | (minor << 8) | patch;
    }

    [[nodiscard]] constexpr bool operator==(const PolicyVersionId& other) const noexcept = default;
};

inline constexpr PolicyVersionId DEFAULT_POLICY_VERSION_ID{1, 0, 1, 0};

// Generator version for stable pseudo-random streams (§15.3, ADR-0002)
inline constexpr std::uint32_t RNG_VERSION = 1;

// Schema version for RunRecord JSON (§15.7)
inline constexpr std::uint32_t RUN_RECORD_SCHEMA_VERSION = 1;

// Default policy version text (§15.7)
inline constexpr const char* DEFAULT_POLICY_VERSION_STRING = "cie-policy-0.1.0";

} // namespace cymatica::core
