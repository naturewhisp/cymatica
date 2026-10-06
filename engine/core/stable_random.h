#pragma once

#include "random_key.h"
#include "version_ids.h"

#include <cstdint>

namespace cymatica::core {

// Stafford Mix 13 (SplitMix64 finalizer / mixer)
// Highly uniform avalanche, 64-bit reversible bijection.
[[nodiscard]] constexpr std::uint64_t staffordMix13(std::uint64_t z) noexcept {
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

// Fixed domain separation constants
inline constexpr std::uint64_t GOLDEN_RATIO_64  = 0x9e3779b97f4a7c15ULL;
inline constexpr std::uint64_t DOMAIN_STREAM_64  = 0xa0761d6478bd642fULL;
inline constexpr std::uint64_t DOMAIN_SAMPLE_64  = 0xe7037ed1a0b428dbULL;
inline constexpr std::uint64_t DOMAIN_DECISION_64 = 0x8ebc6af09c88c6e3ULL;

// Deterministic multi-element mixing functions (ADR-0002)
[[nodiscard]] constexpr std::uint64_t stableMix3(std::uint64_t a, std::uint64_t b, std::uint64_t c) noexcept {
    std::uint64_t h = staffordMix13(a ^ DOMAIN_STREAM_64);
    h = staffordMix13(h + b * GOLDEN_RATIO_64);
    h = staffordMix13(h + c * 0x94d049bb133111ebULL);
    return h;
}

[[nodiscard]] constexpr std::uint64_t stableMix4(std::uint64_t a, std::uint64_t b, std::uint64_t c, std::uint64_t d) noexcept {
    std::uint64_t h = staffordMix13(a ^ DOMAIN_SAMPLE_64);
    h = staffordMix13(h + b * GOLDEN_RATIO_64);
    h = staffordMix13(h + c * 0x94d049bb133111ebULL);
    h = staffordMix13(h + d * 0xbf58476d1ce4e5b9ULL);
    return h;
}

[[nodiscard]] constexpr std::uint64_t stableMix5(std::uint64_t a, std::uint64_t b, std::uint64_t c, std::uint64_t d, std::uint64_t e) noexcept {
    std::uint64_t h = staffordMix13(a ^ DOMAIN_DECISION_64);
    h = staffordMix13(h + b * GOLDEN_RATIO_64);
    h = staffordMix13(h + c * 0x94d049bb133111ebULL);
    h = staffordMix13(h + d * 0xbf58476d1ce4e5b9ULL);
    h = staffordMix13(h + e * 0xd6e8feb86659fd93ULL);
    return h;
}

// Derives a 64-bit stream seed from run seed, stream ID and numeric policy version ID (§15.4)
[[nodiscard]] constexpr std::uint64_t deriveStreamSeed(std::uint64_t runSeed, std::uint32_t streamId, std::uint32_t policyVersionId) noexcept {
    return stableMix3(runSeed, static_cast<std::uint64_t>(streamId), static_cast<std::uint64_t>(policyVersionId));
}

// Derives a deterministic decision ID from musical coordinates (§15.5)
[[nodiscard]] constexpr std::uint64_t deriveDecisionId(
    std::uint32_t section,
    std::uint32_t phrase,
    std::uint32_t bar,
    std::uint32_t decisionSlot,
    std::uint32_t policyVersionId
) noexcept {
    return stableMix5(
        static_cast<std::uint64_t>(section),
        static_cast<std::uint64_t>(phrase),
        static_cast<std::uint64_t>(bar),
        static_cast<std::uint64_t>(decisionSlot),
        static_cast<std::uint64_t>(policyVersionId)
    );
}

// Stateless pseudo-random extraction functions (§15.3, ADR-0002)
[[nodiscard]] std::uint64_t randomU64(RandomKey key, std::uint32_t policyVersionId = DEFAULT_POLICY_VERSION_ID.toU32()) noexcept;

// Bit-stable uniform float in [0.0f, 1.0f) using upper 24 bits
[[nodiscard]] float randomF32(RandomKey key, std::uint32_t policyVersionId = DEFAULT_POLICY_VERSION_ID.toU32()) noexcept;

// Unbiased integer in half-open range [min, max)
[[nodiscard]] std::int32_t randomRangeI32(RandomKey key, std::int32_t min, std::int32_t max, std::uint32_t policyVersionId = DEFAULT_POLICY_VERSION_ID.toU32()) noexcept;

} // namespace cymatica::core
