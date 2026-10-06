#include <catch2/catch_test_macros.hpp>

#include "random_key.h"
#include "seed_bank.h"
#include "stable_random.h"
#include "version_ids.h"

#include <limits>
#include <vector>

using namespace cymatica::core;
using namespace cymatica::replay;

TEST_CASE("Stafford Mix 13 and stable mixes evaluate expected golden vectors", "[random][golden]") {
    // Exact mathematical output of Stafford Mix 13 (SplitMix64 mixer, DIF-M1-02)
    REQUIRE(staffordMix13(0ULL) == 0ULL);
    REQUIRE(staffordMix13(1ULL) == 0x5692161d100b05e5ULL);
    REQUIRE(staffordMix13(0x123456789abcdef0ULL) == 0x9629f58e8ec5b906ULL);

    // Exact golden verification of multi-element stable mixes
    REQUIRE(stableMix3(0x12345678ULL, 1, 1) == 0x76c8dabe106b66adULL);

    constexpr std::uint64_t m3 = 0x76c8dabe106b66adULL;
    REQUIRE(stableMix4(m3, 42, 0, 0) == 0x8e244bcb7b42f6a2ULL);

    REQUIRE(stableMix5(1, 2, 3, 4, 1) == 0xaafd32eca762fa8aULL);

    // deriveStreamSeed exact golden vector (§15.4)
    constexpr std::uint64_t kSeed = 0xabcdef0123456789ULL;
    REQUIRE(deriveStreamSeed(kSeed, static_cast<std::uint32_t>(StreamId::Pattern), DEFAULT_POLICY_VERSION_ID.toU32()) == 0x9362822d9741d797ULL);

    // sampleU64 via SeedBank exact golden vectors
    SeedBank bank(kSeed);
    REQUIRE(bank.sampleU64(StreamId::Pattern, 100, 0) == 0xde76c98b842bfd3ULL);
    REQUIRE(bank.sampleU64(StreamId::Pattern, 100, 1) == 0x443683c257518ea9ULL);
}

TEST_CASE("Stream isolation: VFX stream consumption does not alter gameplay streams", "[random][isolation]") {
    // Acceptance criterion Spec §32: VFX stream non modifica gameplay stream
    constexpr std::uint64_t kSeed = 0xabcdef0123456789ULL;
    SeedBank bank(kSeed);

    std::vector<std::uint64_t> patternStreamBaseline;
    std::vector<std::uint64_t> directorStreamBaseline;

    patternStreamBaseline.reserve(1000);
    directorStreamBaseline.reserve(1000);

    for (std::uint32_t i = 0; i < 1000; ++i) {
        patternStreamBaseline.push_back(bank.sampleU64(StreamId::Pattern, 100, i));
        directorStreamBaseline.push_back(bank.sampleU64(StreamId::Director, 200, i));
    }

    // Now interleave heavy sampling on VFX and Cosmetic streams
    std::vector<std::uint64_t> patternStreamWithVfx;
    std::vector<std::uint64_t> directorStreamWithVfx;

    patternStreamWithVfx.reserve(1000);
    directorStreamWithVfx.reserve(1000);

    for (std::uint32_t i = 0; i < 1000; ++i) {
        // Sample lots of VFX noise
        for (std::uint32_t v = 0; v < 50; ++v) {
            (void)bank.sampleU64(StreamId::Vfx, 999, v);
            (void)bank.sampleU64(StreamId::Cosmetic, 888, v);
        }

        patternStreamWithVfx.push_back(bank.sampleU64(StreamId::Pattern, 100, i));
        directorStreamWithVfx.push_back(bank.sampleU64(StreamId::Director, 200, i));
    }

    REQUIRE(patternStreamBaseline == patternStreamWithVfx);
    REQUIRE(directorStreamBaseline == directorStreamWithVfx);
}

TEST_CASE("randomF32 produces values strictly in [0.0f, 1.0f)", "[random][math]") {
    constexpr std::uint64_t kSeed = 0x42ULL;
    SeedBank bank(kSeed);

    for (std::uint32_t i = 0; i < 5000; ++i) {
        const float val = bank.sampleF32(StreamId::Pattern, 1, i);
        REQUIRE(val >= 0.0f);
        REQUIRE(val < 1.0f);
    }
}

TEST_CASE("randomRangeI32 produces values in half-open [min, max) without bias", "[random][math]") {
    constexpr std::uint64_t kSeed = 0x9999ULL;
    SeedBank bank(kSeed);

    constexpr std::int32_t kMin = -5;
    constexpr std::int32_t kMax = 10;
    std::vector<int> counts(kMax - kMin, 0);

    for (std::uint32_t i = 0; i < 15000; ++i) {
        const std::int32_t val = bank.sampleRangeI32(StreamId::Harmony, 42, kMin, kMax, i);
        REQUIRE(val >= kMin);
        REQUIRE(val < kMax);
        counts[val - kMin]++;
    }

    // Every bin in [-5, 10) must have received non-zero samples
    for (int c : counts) {
        REQUIRE(c > 0);
    }
}

TEST_CASE("randomRangeI32 handles large signed spans without integer overflow", "[random][math][bounds]") {
    RandomKey key{
        .runSeed = 0x12345678ULL,
        .streamId = static_cast<std::uint32_t>(StreamId::Pattern),
        .decisionId = 1,
        .sampleIndex = 0,
    };

    // Degenerate ranges
    REQUIRE(randomRangeI32(key, 5, 5) == 5);
    REQUIRE(randomRangeI32(key, 10, 5) == 10);

    // Large range crossing zero (would overflow with signed max - min)
    constexpr std::int32_t kLargeMin = -1'000'000'000;
    constexpr std::int32_t kLargeMax = 1'000'000'000;
    for (std::uint32_t i = 0; i < 1000; ++i) {
        key.sampleIndex = i;
        const std::int32_t val = randomRangeI32(key, kLargeMin, kLargeMax);
        REQUIRE(val >= kLargeMin);
        REQUIRE(val < kLargeMax);
    }

    // Span exceeding INT32_MAX (range = 3,000,000,000 > 2^31 - 1, DIF-M1-04)
    constexpr std::int32_t kSpan3BMin = -1'500'000'000;
    constexpr std::int32_t kSpan3BMax = 1'500'000'000;
    for (std::uint32_t i = 0; i < 1000; ++i) {
        key.sampleIndex = i;
        const std::int32_t val = randomRangeI32(key, kSpan3BMin, kSpan3BMax);
        REQUIRE(val >= kSpan3BMin);
        REQUIRE(val < kSpan3BMax);
    }

    // Full signed integer span from INT32_MIN to INT32_MAX (range = 4,294,967,295)
    constexpr std::int32_t kFullMin = std::numeric_limits<std::int32_t>::min();
    constexpr std::int32_t kFullMax = std::numeric_limits<std::int32_t>::max();
    for (std::uint32_t i = 0; i < 1000; ++i) {
        key.sampleIndex = i;
        const std::int32_t val = randomRangeI32(key, kFullMin, kFullMax);
        REQUIRE(val >= kFullMin);
        REQUIRE(val < kFullMax);
    }
}

TEST_CASE("randomRangeI32 rejection sampling does not mutate or correlate sampleIndex", "[random][isolation]") {
    // DIF-M1-11: verify that sampling a range preserves strict sampleIndex isolation
    // and uses retryTag domain separation instead of mutating sampleIndex.
    RandomKey key{
        .runSeed = 0xcafebeefULL,
        .streamId = static_cast<std::uint32_t>(StreamId::Pattern),
        .decisionId = 0,
        .sampleIndex = 0,
    };

    // Prove domain separation between retryTag and subsequent sampleIndex
    const std::uint64_t uRetry1 = randomU64(key, DEFAULT_POLICY_VERSION_ID.toU32(), 1);
    RandomKey key1 = key;
    key1.sampleIndex = 1;
    const std::uint64_t uSample1 = randomU64(key1, DEFAULT_POLICY_VERSION_ID.toU32(), 0);

    // Exact golden verification: retry 1 must NOT match sampleIndex 1 (DIF-M1-11)
    REQUIRE(uRetry1 == 0xff5b9a4e74829d80ULL);
    REQUIRE(uSample1 == 0xd8447077ce022025ULL);
    REQUIRE(uRetry1 != uSample1);

    const std::int32_t r0 = randomRangeI32(key, -1'500'000'000, 1'500'000'000);
    REQUIRE(r0 == -1111793990);
    REQUIRE(key.sampleIndex == 0); // key input was not mutated

    RandomKey keyDec4 = key;
    keyDec4.decisionId = 4;
    const std::int32_t r4 = randomRangeI32(keyDec4, -1'500'000'000, 1'500'000'000);
    REQUIRE(r4 == -24512338);

    // Subsequent draw at sampleIndex = 1 remains strictly independent
    REQUIRE(randomU64(key1) == 0xd8447077ce022025ULL);
}
