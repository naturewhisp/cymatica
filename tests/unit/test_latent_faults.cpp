// Characterization of the latent faults that M2.5.3 will touch (ADR-0007, R-06, M2.5.1b).
// These tests pin the CURRENT behaviour, faults included, and show that fixture-m2-4-v1 does not activate them.
// None of them states a desired behaviour: when M2.5.3 fixes a fault, the test that pins it must change with it.
#include <catch2/catch_test_macros.hpp>

#include "music_clock.h"
#include "music_planner.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <vector>

using namespace cymatica::audio;

TEST_CASE("CL-MM-01: phrase indices diverge when barsPerPhrase changes between segments", "[latent_fault][clock]") {
    TempoMap four;
    four.barsPerPhrase = 4;
    TempoMap two = four;
    two.barsPerPhrase = 2;
    auto clock = MusicClock::create(48000, four);
    REQUIRE(clock.has_value());
    REQUIRE(clock->setTempoMap(two, clock->frameAtBar(8)));

    // Two phrases of 4 bars precede the change, but the index jumps from 1 to 4: 2 and 3 never exist
    CHECK(clock->positionAtFrame(clock->frameAtBar(7)).phrase == 1);
    CHECK(clock->positionAtFrame(clock->frameAtBar(8)).phrase == 4);

    // The identity of AG-MM-01 breaks: frameAtPhrase reads the map of segment 0, positionAtFrame the current one
    const MusicPosition position = clock->positionAtFrame(clock->frameAtBar(10));
    CHECK(position.phrase == 5);
    CHECK(position.bar == 0);
    CHECK(clock->frameAtPhrase(position.phrase) == clock->frameAtBar(20));
    CHECK(clock->frameAtPhrase(position.phrase) != position.sampleFrame);
}

TEST_CASE("CL-MM-01: fixture-m2-4-v1 does not activate the fault", "[latent_fault][clock]") {
    const auto clock = MusicPlanner::makeFixtureM24V1Clock();
    REQUIRE(clock.has_value());
    for (std::uint32_t phrase = 0; phrase < 24; ++phrase) {
        const MusicPosition position = clock->positionAtFrame(clock->frameAtPhrase(phrase));
        CHECK(position.phrase == phrase);
        CHECK(position.bar == 0);
    }
}

TEST_CASE("AG-MM-02: phrasePhase reaches 1.0f on a supported map", "[latent_fault][clock]") {
    TempoMap slow;
    slow.bpm = RationalBpm{20, 1};
    slow.barsPerPhrase = 64;
    const auto clock = MusicClock::create(48000, slow);
    REQUIRE(clock.has_value());
    const MusicPosition last = clock->positionAtFrame(clock->frameAtPhrase(1) - 1);
    CHECK(last.phrase == 0); // still the first phrase, yet the documented range [0, 1) is left
    CHECK(last.phrasePhase == 1.0f);
}

TEST_CASE("AG-MM-02: fixture-m2-4-v1 does not activate the fault", "[latent_fault][clock]") {
    const auto clock = MusicPlanner::makeFixtureM24V1Clock();
    REQUIRE(clock.has_value());
    for (std::uint32_t phrase = 1; phrase <= 24; ++phrase) {
        CHECK(clock->positionAtFrame(clock->frameAtPhrase(phrase) - 1).phrasePhase < 1.0f);
    }
}

TEST_CASE("CL-MM-02: modal transposition folds notes one by one at the register edge", "[latent_fault][motif]") {
    Motif base{};
    base.lengthSteps = 16;
    base.noteCount = 2;
    base.notes[0] = MotifNote{.scaleDegree = 13, .stepOffset = 0, .durationSteps = 4};
    base.notes[1] = MotifNote{.scaleDegree = 12, .stepOffset = 4, .durationSteps = 4};

    const Motif moved = applyModalTranspose(base, 2);
    CHECK(moved.notes[0].scaleDegree == 8);  // 15 > 14: one octave down
    CHECK(moved.notes[1].scaleDegree == 14); // stays: the step of -1 becomes +6
    CHECK_FALSE(verifyMotifTransformInvariants(base, moved, MotifTransformKind::ModalTranspose, 2));
}

TEST_CASE("CL-MM-02: the base motifs of the fixture never reach the folding", "[latent_fault][motif]") {
    const Motif dorian = MusicPlanner::makeDorianBaseMotif();
    const Motif aeolian = MusicPlanner::makeAeolianBaseMotif();
    for (const Motif* base : {&dorian, &aeolian}) {
        for (std::int16_t delta = -7; delta <= 7; ++delta) {
            const Motif moved = applyModalTranspose(*base, delta);
            CHECK(verifyMotifTransformInvariants(*base, moved, MotifTransformKind::ModalTranspose, delta));
            for (std::size_t i = 0; i < base->noteCount; ++i) {
                CHECK(moved.notes[i].scaleDegree - base->notes[i].scaleDegree == delta);
            }
        }
    }
}

namespace {

CandidateEvent shortPulso(std::uint64_t frame) {
    CandidateEvent e{};
    e.sampleFrame = frame;
    e.role = MusicRole::Pulso;
    e.midiNote = 38;
    e.pitchHz = 73.42f;
    e.velocity = 0.8f;
    e.durationFrames = 10;
    e.timbreParam = 0.3f;
    return e;
}

CandidateEvent openPad(std::uint64_t frame, std::int16_t midi) {
    CandidateEvent e{};
    e.sampleFrame = frame;
    e.role = MusicRole::Corpo;
    e.midiNote = midi;
    e.pitchHz = 146.83f;
    e.velocity = 0.8f;
    e.durationFrames = 0;
    e.timbreParam = 0.4f;
    e.isOpenPad = true;
    return e;
}

void push(PhraseCandidate& candidate, const CandidateEvent& e) {
    candidate.events[candidate.eventCount++] = e;
    candidate.requestedEventCount = candidate.eventCount;
}

} // namespace

TEST_CASE("CL-MM-04: the pad NoteOff of a cross-section phrase is counted at the phrase end", "[latent_fault][planner]") {
    const auto clock = MusicPlanner::makeFixtureM24V1Clock();
    REQUIRE(clock.has_value());
    const auto phrases = MusicPlanner::makeFixtureM24V1Phrases(*clock);
    const PhrasePlan& phrase = phrases[7];
    REQUIRE(phrase.isCrossSectionPadPhrase);
    const MusicPlanner planner({.sampleRate = 48000, .lookaheadFrames = 7200});
    REQUIRE(phrase.crossSectionNoteOffFrame >= phrase.endFrame + 7200); // really outside the last window

    // 64 notes: exactly 128 SPSC events in the window that closes the phrase
    auto dense = std::make_unique<PhraseCandidate>();
    for (std::uint64_t i = 0; i < 64; ++i) {
        push(*dense, shortPulso(phrase.endFrame - 7000 + i * 100));
    }
    CHECK(planner.validateHardConstraints(*dense, phrase, 0, false) == HardConstraintViolation::None);

    // Two pads held from the phrase start add nothing to that window, because their NoteOff comes a beat later.
    // The validator counts it at phrase.endFrame and rejects a candidate that respects the limit.
    auto withPads = std::make_unique<PhraseCandidate>();
    push(*withPads, openPad(phrase.startFrame, 50));
    push(*withPads, openPad(phrase.startFrame, 57));
    for (std::uint64_t i = 0; i < 64; ++i) {
        push(*withPads, shortPulso(phrase.endFrame - 7000 + i * 100));
    }
    CHECK(planner.validateHardConstraints(*withPads, phrase, 0, false) == HardConstraintViolation::SpscCapacityExceeded);
}

TEST_CASE("CL-MM-04: fixture-m2-4-v1 stays far below the density limit at the real NoteOff frames", "[latent_fault][planner]") {
    const auto clock = MusicPlanner::makeFixtureM24V1Clock();
    REQUIRE(clock.has_value());
    const auto phrases = MusicPlanner::makeFixtureM24V1Phrases(*clock);
    MusicPlanner planner({.runSeed = 1337, .candidateCount = 8, .sampleRate = 48000, .lookaheadFrames = 7200});
    planner.setFormPlan(MusicPlanner::makeFixtureM24V1Form());

    std::vector<std::uint64_t> frames;
    for (const PhrasePlan& phrase : phrases) {
        std::vector<MusicIntentEvent> intents;
        planner.planPhrase(phrase, *clock, phrase.startFrame, phrase.sectionIndex + 1, intents);
        for (const MusicIntentEvent& e : intents) {
            frames.push_back(e.sampleFrame);
            if (e.durationFrames != 0) frames.push_back(e.sampleFrame + e.durationFrames);
        }
    }
    for (const PlannedNoteOff& off : planner.plannedNoteOffs()) {
        frames.push_back(off.sampleFrame);
    }
    std::sort(frames.begin(), frames.end());
    std::size_t lo = 0;
    std::size_t peak = 0;
    for (std::size_t hi = 0; hi < frames.size(); ++hi) {
        while (frames[hi] - frames[lo] >= 7200) ++lo;
        peak = std::max(peak, hi - lo + 1);
    }
    CHECK(peak * 2 <= kMaxSpscEventsPerLookaheadWindow);
}
