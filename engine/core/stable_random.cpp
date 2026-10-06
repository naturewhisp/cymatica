#include "stable_random.h"

namespace cymatica::core {

std::uint64_t randomU64(RandomKey key, std::uint32_t policyVersionId) noexcept {
    const std::uint64_t sSeed = deriveStreamSeed(key.runSeed, key.streamId, policyVersionId);
    return stableMix4(sSeed, key.decisionId, static_cast<std::uint64_t>(key.sampleIndex), 0);
}

float randomF32(RandomKey key, std::uint32_t policyVersionId) noexcept {
    const std::uint64_t val = randomU64(key, policyVersionId);
    const auto u24 = static_cast<std::uint32_t>(val >> 40); // high 24 bits
    return static_cast<float>(u24) * (1.0f / 16777216.0f); // exact [0.0f, 1.0f)
}

std::int32_t randomRangeI32(RandomKey key, std::int32_t min, std::int32_t max, std::uint32_t policyVersionId) noexcept {
    if (min >= max) {
        return min;
    }
    const auto range = static_cast<std::uint32_t>(max - min);
    std::uint64_t x = randomU64(key, policyVersionId);
    auto m = static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) * static_cast<std::uint64_t>(range);
    auto l = static_cast<std::uint32_t>(m);

    if (l < range) {
        const std::uint32_t t = static_cast<std::uint32_t>(-static_cast<std::int32_t>(range)) % range;
        std::uint32_t index = key.sampleIndex;
        while (l < t) {
            ++index;
            RandomKey retryKey = key;
            retryKey.sampleIndex = index;
            x = randomU64(retryKey, policyVersionId);
            m = static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) * static_cast<std::uint64_t>(range);
            l = static_cast<std::uint32_t>(m);
        }
    }
    return min + static_cast<std::int32_t>(m >> 32);
}

} // namespace cymatica::core
