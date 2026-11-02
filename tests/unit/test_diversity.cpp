#include <catch2/catch_test_macros.hpp>

#include "diversity.h"
#include "plan_content_fingerprint.h"

#include <set>
#include <string>
#include <vector>

using namespace cymatica::cacp;
using cymatica::audio::MusicIntentEvent;
using cymatica::audio::MusicRole;
using cymatica::audio::PlannedNoteOff;

namespace {

MusicIntentEvent note(std::uint64_t id, std::uint64_t frame, MusicRole role, float pitch, std::uint32_t dur = 4800) {
    return MusicIntentEvent{.eventId = id, .sampleFrame = frame, .tick = 0, .transportEpoch = 1, .role = role, .pitchHz = pitch,
                            .velocity = 0.8f, .durationFrames = dur, .timbreParam = 0.5f};
}

// Two phrases of one bar each (48000 frames); the second may repeat the first's material shifted in time
FixturePlan twoPhrases(bool samePitch) {
    FixturePlan p;
    p.ok = true;
    p.phrases = {{0, 0, 0, 1, 0, 48000}, {1, 0, 1, 1, 48000, 96000}};
    p.intentsPerPhrase = {{note(1, 0, MusicRole::Corpo, 220.0f), note(2, 12000, MusicRole::Pulso, 55.0f)},
                          {note(3, 48000, MusicRole::Corpo, samePitch ? 220.0f : 247.0f), note(4, 60000, MusicRole::Pulso, 55.0f)}};
    return p;
}

} // namespace

TEST_CASE("Per-phrase content fingerprint: same material anywhere, one group (DIF-5e.0-02)", "[diversity][fingerprint]") {
    const FixturePlan same = twoPhrases(true);
    const auto a = phraseContentFingerprint(same.intentsPerPhrase[0], same.noteOffs, 0, 48000);
    const auto b = phraseContentFingerprint(same.intentsPerPhrase[1], same.noteOffs, 48000, 48000);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    CHECK(*a == *b); // positive: same material at a different position
    const auto groups = phraseGroupIds(same);
    REQUIRE(groups.size() == 2);
    CHECK(groups[0] == groups[1]); // review case: identical fingerprints -> a single musical group
    // The absolute fingerprint still tells the two positions apart
    CHECK(cymatica::audio::musicalContentFingerprint({same.intentsPerPhrase[0]}, {}, 48000).hash !=
          cymatica::audio::musicalContentFingerprint({same.intentsPerPhrase[1]}, {}, 48000).hash);
    // Negative: a single event different -> another fingerprint, another group
    const FixturePlan diff = twoPhrases(false);
    const auto groupsDiff = phraseGroupIds(diff);
    CHECK(groupsDiff[0] != groupsDiff[1]);
    // Only the phrase's own NoteOffs are joined: a NoteOff of another phrase does not change it
    std::vector<PlannedNoteOff> offs{{.eventId = 3, .role = MusicRole::Corpo, .sampleFrame = 90000, .transportEpoch = 1}};
    CHECK(phraseContentFingerprint(same.intentsPerPhrase[0], offs, 0, 48000) == a);
    CHECK(phraseContentFingerprint(same.intentsPerPhrase[1], offs, 48000, 48000) != b);
    // An event before the origin gives no fingerprint
    CHECK_FALSE(phraseContentFingerprint(same.intentsPerPhrase[1], same.noteOffs, 50000, 48000).has_value());
    // The default origin keeps the existing absolute fingerprint
    CHECK(cymatica::audio::musicalContentFingerprint({same.intentsPerPhrase[1]}, {}, 48000, std::nullopt, 0).hash ==
          cymatica::audio::musicalContentFingerprint({same.intentsPerPhrase[1]}, {}, 48000).hash);
}

TEST_CASE("Diversity metrics M2-M4 on known token sets (5e-04)", "[diversity][metrics]") {
    std::vector<NoteToken> a{{1, 0, 60, 4}, {1, 4, 62, 4}, {1, 8, 64, 4}, {1, 12, 65, 4}};
    // Same sequence transposed and shifted: identity for M3 and M4 (intervals and IOIs unchanged)
    std::vector<NoteToken> b{{1, 32, 67, 4}, {1, 36, 69, 4}, {1, 40, 71, 4}, {1, 44, 72, 4}};
    CHECK(m3CardinalityDistance(a, b, 1) == 0.0);
    CHECK(m4EditDistance(a, b, 1) == 0.0);
    CHECK(m3CardinalityDistance(a, a, 1) == 0.0);
    // One note changed: M3 = 1/4, M4 = 2/4 (its own token and the next interval change)
    auto c = a;
    c[1].midi = 61;
    CHECK(m3CardinalityDistance(a, c, 1) == 0.25);
    CHECK(m4EditDistance(a, c, 1) == 0.5);
    // Empty role on both sides: 0; on one side: 1
    CHECK(m3CardinalityDistance(a, b, 2) == 0.0);
    CHECK(m3CardinalityDistance(a, {}, 1) == 1.0);
    CHECK(m4EditDistance(a, {}, 1) == 1.0);
    // M2: identical features for identical plans, different for different pitch classes
    CHECK(m2Features(a) == m2Features(a));
    CHECK(m2Features(a) != m2Features(c));
    CHECK(m2Features(a).size() == 33);
}

TEST_CASE("Seed diversity of the fixture is measured over the declared scan (5e-04)", "[diversity][fixture]") {
    const DiversityResult d = measureSeedDiversity();
    INFO(d.error);
    REQUIRE(d.status == "MEASURED");
    CHECK(d.seeds.size() == 64);
    CHECK(d.pairs == 64 * 63 / 2);
    CHECK(d.distinctFingerprints >= 1);
    CHECK(d.phraseGroups1337 >= 1);
    CHECK(d.phraseGroups1337 <= 24);
    if (d.distinctFingerprints == 1) {
        CHECK(d.outcome == "NULL_DIVERSITY");
        CHECK(d.generalisation == "INCONCLUSIVE");
        CHECK(d.m1IdenticalPairShare == 1.0);
        CHECK(d.distinctPairs == 0);
        CHECK(d.m2Distances.empty());
    }
    // Planning is deterministic: same seed, same plan
    const auto p1 = planFixtureM24V1(7), p2 = planFixtureM24V1(7);
    CHECK(phraseGroupIds(p1) == phraseGroupIds(p2));
}
