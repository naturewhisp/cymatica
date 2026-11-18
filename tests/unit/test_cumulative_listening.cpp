// Cumulative listening (D-M2-50): declared rule of the second piece and golden applicability of a variant render
#include <catch2/catch_test_macros.hpp>

#include "audio_probe.h"
#include "cumulative_listening.h"
#include "music_planner.h"

using namespace cymatica;

TEST_CASE("The second listening piece is the smallest seed satisfying the declared rule", "[cumulative_listening]") {
    const auto seed = cacp::secondListeningSeed();
    REQUIRE(seed.has_value());
    CHECK(*seed == 34);
    const auto v = audio::MusicPlanner::drawFixtureVariant(*seed, audio::kPlannerPolicyVersion);
    CHECK(v.bHarmony == 2);
    CHECK(v.auraMotifA);
    CHECK(v.auraMotifB);
    CHECK(v.aTramaEntry != 4);
    CHECK(v.bDisplacementStart != 12);
    for (std::uint64_t s = 1; s < *seed; ++s) {
        const auto w = audio::MusicPlanner::drawFixtureVariant(s, audio::kPlannerPolicyVersion);
        CHECK_FALSE((w.bHarmony == 2 && w.auraMotifA && w.auraMotifB && w.aTramaEntry != 4 && w.bDisplacementStart != 12));
    }
    CHECK_FALSE(cacp::secondListeningSeed(1, *seed - 1).has_value());
}

TEST_CASE("A variant render never claims the PCM golden", "[cumulative_listening]") {
    audio::FixtureProbeResult r{};
    r.goldenApplicable = true;
    r.pcmContentHash = 42;
    CHECK(audio::fixturePcmGoldenStatus(r, 42) == "PASS");
    r.fixtureVariantOverride = true;
    CHECK(audio::fixturePcmGoldenStatus(r, 42) == "NOT_APPLICABLE");
}
