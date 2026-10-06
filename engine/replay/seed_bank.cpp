#include "seed_bank.h"

namespace cymatica::replay {

SeedBank::SeedBank(std::uint64_t runSeed, core::PolicyVersionId policyVersion) noexcept
    : runSeed_(runSeed), policyVersion_(policyVersion) {}

std::uint64_t SeedBank::streamSeed(core::StreamId stream) const noexcept {
    return core::deriveStreamSeed(runSeed_, static_cast<std::uint32_t>(stream), policyVersion_.toU32());
}

core::RandomKey SeedBank::makeKey(core::StreamId stream, std::uint64_t decisionId, std::uint32_t sampleIndex) const noexcept {
    return core::RandomKey{
        .runSeed = runSeed_,
        .streamId = static_cast<std::uint32_t>(stream),
        .decisionId = decisionId,
        .sampleIndex = sampleIndex,
    };
}

std::uint64_t SeedBank::sampleU64(core::StreamId stream, std::uint64_t decisionId, std::uint32_t sampleIndex) const noexcept {
    return core::randomU64(makeKey(stream, decisionId, sampleIndex), policyVersion_.toU32());
}

float SeedBank::sampleF32(core::StreamId stream, std::uint64_t decisionId, std::uint32_t sampleIndex) const noexcept {
    return core::randomF32(makeKey(stream, decisionId, sampleIndex), policyVersion_.toU32());
}

std::int32_t SeedBank::sampleRangeI32(core::StreamId stream, std::uint64_t decisionId, std::int32_t min, std::int32_t max, std::uint32_t sampleIndex) const noexcept {
    return core::randomRangeI32(makeKey(stream, decisionId, sampleIndex), min, max, policyVersion_.toU32());
}

} // namespace cymatica::replay
