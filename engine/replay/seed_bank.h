#pragma once

#include "random_key.h"
#include "stable_random.h"
#include "version_ids.h"

#include <cstdint>

namespace cymatica::replay {

// SeedBank manages deterministic pseudo-random stream derivation for a run.
// Strictly stateless with respect to generator calls; holds only immutable run configuration.
class SeedBank {
public:
    explicit SeedBank(
        std::uint64_t runSeed,
        core::PolicyVersionId policyVersion = core::DEFAULT_POLICY_VERSION_ID
    ) noexcept;

    [[nodiscard]] std::uint64_t runSeed() const noexcept { return runSeed_; }
    [[nodiscard]] core::PolicyVersionId policyVersion() const noexcept { return policyVersion_; }

    // Derives stable 64-bit seed for a specific stream (§15.4)
    [[nodiscard]] std::uint64_t streamSeed(core::StreamId stream) const noexcept;

    // Constructs a stateless RandomKey for a decision and sample index
    [[nodiscard]] core::RandomKey makeKey(
        core::StreamId stream,
        std::uint64_t decisionId,
        std::uint32_t sampleIndex = 0
    ) const noexcept;

    // Direct sampling helpers
    [[nodiscard]] std::uint64_t sampleU64(
        core::StreamId stream,
        std::uint64_t decisionId,
        std::uint32_t sampleIndex = 0
    ) const noexcept;

    [[nodiscard]] float sampleF32(
        core::StreamId stream,
        std::uint64_t decisionId,
        std::uint32_t sampleIndex = 0
    ) const noexcept;

    [[nodiscard]] std::int32_t sampleRangeI32(
        core::StreamId stream,
        std::uint64_t decisionId,
        std::int32_t min,
        std::int32_t max,
        std::uint32_t sampleIndex = 0
    ) const noexcept;

private:
    std::uint64_t runSeed_{0};
    core::PolicyVersionId policyVersion_{core::DEFAULT_POLICY_VERSION_ID};
};

} // namespace cymatica::replay
