// CP-3 I3 prototype V1: the option lowers only the Corpo bass voice and is off by default
#include <catch2/catch_test_macros.hpp>

#include "audio_probe.h"
#include "music_planner.h"

#include <cmath>
#include <vector>

using namespace cymatica::audio;

namespace {
std::vector<MusicIntentEvent> planAll(bool octaveDown) {
    const auto clock = MusicPlanner::makeFixtureM24V1Clock();
    REQUIRE(clock.has_value());
    MusicPlanner planner({.runSeed = 1337, .candidateCount = 8, .corpoBassOctaveDown = octaveDown});
    planner.setFormPlan(MusicPlanner::makeFixtureM24V1Form());
    std::vector<MusicIntentEvent> all;
    for (const auto& p : MusicPlanner::makeFixtureM24V1Phrases(*clock)) {
        std::vector<MusicIntentEvent> out;
        planner.planPhrase(p, *clock, 0, p.sectionIndex + 1ULL, out, nullptr);
        all.insert(all.end(), out.begin(), out.end());
    }
    return all;
}
} // namespace

TEST_CASE("The Corpo octave option lowers only the Corpo bass voice", "[bass_prototype]") {
    CHECK_FALSE(MusicPlannerConfig{}.corpoBassOctaveDown);
    const auto base = planAll(false), down = planAll(true);
    REQUIRE(base.size() == down.size());
    std::size_t lowered = 0, corpo = 0;
    for (std::size_t i = 0; i < base.size(); ++i) {
        CHECK(base[i].role == down[i].role);
        CHECK(base[i].sampleFrame == down[i].sampleFrame);
        CHECK(base[i].durationFrames == down[i].durationFrames);
        if (base[i].role == MusicRole::Corpo) ++corpo;
        if (base[i].pitchHz != down[i].pitchHz) {
            CHECK(base[i].role == MusicRole::Corpo);
            CHECK(std::abs(down[i].pitchHz * 2.0f - base[i].pitchHz) < 0.01f);
            ++lowered;
        }
    }
    CHECK(corpo == 192);
    CHECK(lowered == 96); // one bass note per bar
}

TEST_CASE("The prototype option never claims the PCM golden", "[bass_prototype]") {
    FixtureProbeResult r{};
    r.goldenApplicable = true;
    r.pcmContentHash = 42;
    r.corpoBassOctaveDownOverride = true;
    CHECK(fixturePcmGoldenStatus(r, 42) == "NOT_APPLICABLE");
}
