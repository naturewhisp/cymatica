#include "stable_random.h"

namespace cymatica::core {

std::uint64_t randomU64(RandomKey key, std::uint32_t policyVersionId, std::uint64_t retryTag) noexcept {
    const std::uint64_t sSeed = deriveStreamSeed(key.runSeed, key.streamId, policyVersionId);
    return stableMix4(sSeed, key.decisionId, static_cast<std::uint64_t>(key.sampleIndex), retryTag);
}

float randomF32(RandomKey key, std::uint32_t policyVersionId) noexcept {
    const std::uint64_t val = randomU64(key, policyVersionId, 0);
    const auto u24 = static_cast<std::uint32_t>(val >> 40); // high 24 bits
    return static_cast<float>(u24) * (1.0f / 16777216.0f); // exact [0.0f, 1.0f)
}

std::int32_t randomRangeI32(RandomKey key, std::int32_t min, std::int32_t max, std::uint32_t policyVersionId) noexcept {
    if (min >= max) {
        return min;
    }
    // Safe unsigned difference avoids signed integer overflow UB on large ranges (DIF-M1-04)
    const auto range = static_cast<std::uint32_t>(static_cast<std::uint32_t>(max) - static_cast<std::uint32_t>(min));
    std::uint64_t x = randomU64(key, policyVersionId, 0);
    auto m = static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) * static_cast<std::uint64_t>(range);
    auto l = static_cast<std::uint32_t>(m);

    if (l < range) {
        // Exact Lemire threshold without signed negation (DIF-M1-04)
        const std::uint32_t t = (0u - range) % range;
        std::uint64_t retryTag = 0;
        // Rejection loop increments domainTag/retryTag instead of sampleIndex to prevent correlation (DIF-M1-11)
        while (l < t) {
            ++retryTag;
            x = randomU64(key, policyVersionId, retryTag);
            m = static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) * static_cast<std::uint64_t>(range);
            l = static_cast<std::uint32_t>(m);
        }
    }
    return static_cast<std::int32_t>(static_cast<std::int64_t>(min) + static_cast<std::int64_t>(m >> 32));
}

} // namespace cymatica::core
