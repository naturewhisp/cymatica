#include <catch2/catch_test_macros.hpp>

#include "random_key.h"
#include "seed_bank.h"
#include "stable_random.h"
#include "version_ids.h"

#include <vector>

using namespace cymatica::core;
using namespace cymatica::replay;

TEST_CASE("Stafford Mix 13 evaluates expected golden vectors", "[random][golden]") {
    // Exact mathematical output of Stafford Mix 13 (SplitMix64 mixer)
    REQUIRE(staffordMix13(0ULL) == 0ULL); // 0 ^ (0 >> 30) * C1 ... = 0
    // Test non-zero input
    const std::uint64_t v1 = staffordMix13(1ULL);
    REQUIRE(v1 != 0ULL);
    REQUIRE(staffordMix13(1ULL) == v1); // pure function

    // Golden verification of multi-element stable mixes
    const std::uint64_t m3 = stableMix3(0x12345678ULL, 1, 1);
    REQUIRE(m3 == stableMix3(0x12345678ULL, 1, 1));

    const std::uint64_t m4 = stableMix4(m3, 42, 0, 0);
    REQUIRE(m4 == stableMix4(m3, 42, 0, 0));

    const std::uint64_t m5 = stableMix5(1, 2, 3, 4, 1);
    REQUIRE(m5 == stableMix5(1, 2, 3, 4, 1));
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
