#include <catch2/catch_test_macros.hpp>

#include "diversity.h"
#include "plan_content_fingerprint.h"

#include <cmath>
#include <limits>
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

TEST_CASE("M3 counts simultaneous notes at the same pitch with their multiplicity (DIF-5e.2-02)", "[diversity][metrics]") {
    // Reviewer counterexample: A = {(0,60),(0,60)}, B = {(0,60),(0,60),(4,62)}: two common notes, not four
    const std::vector<NoteToken> a{{1, 0, 60, 4}, {1, 0, 60, 4}};
    const std::vector<NoteToken> b{{1, 0, 60, 4}, {1, 0, 60, 4}, {1, 4, 62, 4}};
    const double d = m3CardinalityDistance(a, b, 1);
    CHECK(std::abs(d - 1.0 / 3.0) < 1e-12);
    CHECK(d >= 0.0);
    CHECK(d <= 1.0);
    CHECK(m3CardinalityDistance(a, a, 1) == 0.0);
    CHECK(m3CardinalityDistance(b, b, 1) == 0.0);
    // Unequal multiplicities: three copies against one: one common note out of three
    const std::vector<NoteToken> three{{1, 0, 60, 4}, {1, 0, 60, 4}, {1, 0, 60, 4}};
    const std::vector<NoteToken> one{{1, 8, 65, 4}};
    CHECK(std::abs(m3CardinalityDistance(three, one, 1) - 2.0 / 3.0) < 1e-12);
    // A plan whose per-phrase fingerprint cannot be computed gives no groups (DIF-5e.2-03)
    FixturePlan p;
    p.ok = true;
    p.phrases = {{0, 0, 0, 1, 0, 48000}};
    p.intentsPerPhrase = {{MusicIntentEvent{.eventId = 1, .sampleFrame = 0, .tick = 0, .transportEpoch = 1, .role = MusicRole::Corpo,
                                            .pitchHz = std::numeric_limits<float>::quiet_NaN(), .velocity = 0.8f,
                                            .durationFrames = 100, .timbreParam = 0.5f}}};
    CHECK(phraseGroupIds(p).empty());
}

TEST_CASE("Symbolic per-phrase fingerprint ignores the +-1-sample jitter of a fractional tempo (ADR-0006 5e.3c, AG-MM-07)",
          "[diversity][symbolic]") {
    // Two one-bar phrases of 82285 and 82286 frames (140 BPM 4/4): the same material at floor-jittered frames
    const std::uint64_t s0 = 0, e0 = 82285, s1 = 82285, e1 = 164571;
    const std::vector<MusicIntentEvent> a{note(1, s0, MusicRole::Corpo, 220.0f, 20571), note(2, s0 + 41142, MusicRole::Pulso, 55.0f, 2571)};
    const std::vector<MusicIntentEvent> b{note(3, s1, MusicRole::Corpo, 220.0f, 20572), note(4, s1 + 41143, MusicRole::Pulso, 55.0f, 2571)};
    const auto fa = symbolicPhraseFingerprint(a, {}, s0, e0, 1), fb = symbolicPhraseFingerprint(b, {}, s1, e1, 1);
    REQUIRE(fa.has_value());
    REQUIRE(fb.has_value());
    CHECK(*fa == *fb);
    // The frame-relative fingerprint of v2 separates them: this is the defect the symbolic rule corrects
    CHECK(phraseContentFingerprint(a, {}, s0, 48000) != phraseContentFingerprint(b, {}, s1, 48000));
    // A musical difference (pitch, grid position, velocity) changes the symbolic fingerprint
    auto c = b;
    c[0].pitchHz = 247.0f;
    CHECK(symbolicPhraseFingerprint(c, {}, s1, e1, 1) != fb);
    c = b;
    c[1].sampleFrame = s1 + 41143 + 2571; // one grid unit (a thirty-second note) later
    CHECK(symbolicPhraseFingerprint(c, {}, s1, e1, 1) != fb);
    c = b;
    c[0].velocity = 0.6f;
    CHECK(symbolicPhraseFingerprint(c, {}, s1, e1, 1) != fb);
    // Explicit planner releases are bound to their note and quantized on the same grid
    const std::vector<PlannedNoteOff> offA{{.eventId = 1, .role = MusicRole::Corpo, .sampleFrame = e0, .transportEpoch = 1}};
    const std::vector<PlannedNoteOff> offB{{.eventId = 3, .role = MusicRole::Corpo, .sampleFrame = e1, .transportEpoch = 1}};
    CHECK(symbolicPhraseFingerprint(a, offA, s0, e0, 1) == symbolicPhraseFingerprint(b, offB, s1, e1, 1));
    CHECK(symbolicPhraseFingerprint(a, offA, s0, e0, 1) != fa);
    // Fail-closed: empty span, zero bars, non-finite field
    CHECK_FALSE(symbolicPhraseFingerprint(a, {}, 10, 10, 1).has_value());
    CHECK_FALSE(symbolicPhraseFingerprint(a, {}, s0, e0, 0).has_value());
    c = b;
    c[0].timbreParam = std::numeric_limits<float>::infinity();
    CHECK_FALSE(symbolicPhraseFingerprint(c, {}, s1, e1, 1).has_value());
}

TEST_CASE("Symbolic groups of fixture-m2-4-v1 follow the prediction registered in ADR-0006 5e.3c", "[diversity][symbolic][fixture]") {
    const FixturePlan p = planFixtureM24V1(1337);
    REQUIRE(p.ok);
    const auto ids = symbolicPhraseGroupIds(p);
    REQUIRE(ids.size() == 24);
    // Predicted partition: A {0-3} {4-6} {7}, B {8} {9-14} {15}, A' {16} {17-23}.
    // M2.5.3-4 F1 (D-M2-40): the declared displacement of B phrases 12-13 splits them off; the return phrase 14 is
    // anchored like 9-11: B {8} {9-11, 14} {12-13} {15}, 9 groups.
    // M2.5.3-4 contract point 4 (D-M2-44): the role masks and the winners they change split B {9, 14} {10-11}
    // {12-13} and A' {16} {17-19} {20, 21, 23} {22}: 12 groups.
    // M2.5.3-4 contract point 5 (D-M2-45): the derivation chain alternates the figures of A {0, 2} {1, 3}, makes every
    // phrase of B distinct, and splits A' {16} {17, 19} {18} {20} {21, 23} {22}: 18 groups.
    // Contract point 5b (D-M2-46): the chain operations make A progress {0} {1} {2} {3} {4-6} {7}, keep every phrase of
    // B distinct, and A' retraces the path of A: phrases 18 and 19 equal 1 and 2 in every role. 20 groups.
    const std::vector<int> predicted{0, 1, 2, 3, 4, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 1, 2, 16, 17, 18, 19};
    for (std::size_t i = 0; i < ids.size(); ++i) {
        for (std::size_t j = 0; j < ids.size(); ++j) {
            INFO("phrases " << i << " and " << j);
            CHECK((ids[i] == ids[j]) == (predicted[i] == predicted[j]));
        }
    }
    CHECK(std::set<std::string>(ids.begin(), ids.end()).size() == 20); // 8 before M2.5.3-4 F1, 9 before point 4, 12 before point 5, 18 before 5b
    // The frame rule of v2 stays readable (14 groups before point 4, 16 after)
    const auto frameIds = phraseGroupIds(p);
    CHECK(std::set<std::string>(frameIds.begin(), frameIds.end()).size() == 20); // 14 before point 4 (D-M2-44), 16 before point 5, 19 before 5b
}

TEST_CASE("Contract point 6: the 64 seeds draw distinct authored paths and contents (D-M2-47)", "[diversity][seed_variants][slow]") {
    const DiversityResult d = measureSeedDiversity();
    REQUIRE(d.status == "MEASURED");
    REQUIRE(d.variants.size() == 64);
    // S4: distributions over at least 8 distinct contents, at least 40 distinct paths (expected about 57)
    CHECK(d.outcome == "DISTRIBUTIONS");
    CHECK(d.distinctFingerprints >= kDiversityMinDistinct);
    CHECK(d.distinctVariants >= 40);
    // S5: both opening motifs appear for A and for B
    std::size_t auraA = 0, auraB = 0;
    for (const auto& v : d.variants) { auraA += v.auraMotifA ? 1 : 0; auraB += v.auraMotifB ? 1 : 0; }
    CHECK(auraA > 0); CHECK(auraA < 64); CHECK(auraB > 0); CHECK(auraB < 64);
    // The reference fixture (seed 1337) is not drawn: it keeps the authored profile
    CHECK(planFixtureM24V1(1337).variant == cymatica::audio::kFixtureReferenceVariant);
}
