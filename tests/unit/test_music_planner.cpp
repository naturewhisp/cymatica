#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "audio_probe.h"
#include "audio_scheduler.h"
#include "euclidean_rhythm.h"
#include "music_clock.h"
#include "music_planner.h"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace cymatica::audio;
using namespace cymatica::core;

TEST_CASE("Bjorklund canonical partitioning and circular rotation", "[music_planner][euclidean_rhythm]") {
    // Canonical E(5,8) = [1, 0, 1, 1, 0, 1, 1, 0] with downbeat on 0
    constexpr auto r58 = euclideanRhythm(8, 5);
    STATIC_REQUIRE(r58[0] == true);
    STATIC_REQUIRE(r58[1] == false);
    STATIC_REQUIRE(r58[2] == true);
    STATIC_REQUIRE(r58[3] == true);
    STATIC_REQUIRE(r58[4] == false);
    STATIC_REQUIRE(r58[5] == true);
    STATIC_REQUIRE(r58[6] == true);
    STATIC_REQUIRE(r58[7] == false);
    REQUIRE(countEuclideanPulses(r58, 8) == 5);

    // Canonical E(3,8) = [1, 0, 0, 1, 0, 0, 1, 0] (Tresillo with downbeat on 0)
    constexpr auto r38 = euclideanRhythm(8, 3);
    STATIC_REQUIRE(r38[0] == true);
    STATIC_REQUIRE(r38[1] == false);
    STATIC_REQUIRE(r38[2] == false);
    STATIC_REQUIRE(r38[3] == true);
    STATIC_REQUIRE(r38[4] == false);
    STATIC_REQUIRE(r38[5] == false);
    STATIC_REQUIRE(r38[6] == true);
    STATIC_REQUIRE(r38[7] == false);
    REQUIRE(countEuclideanPulses(r38, 8) == 3);

    // Circular rotation test: rotate E(5,8) by +1 step right
    const auto r58_rot1 = euclideanRhythm(8, 5, 1);
    REQUIRE(r58_rot1[0] == false);
    REQUIRE(r58_rot1[1] == true);
    REQUIRE(r58_rot1[2] == false);
    REQUIRE(r58_rot1[3] == true);
    REQUIRE(r58_rot1[4] == true);
    REQUIRE(r58_rot1[5] == false);
    REQUIRE(r58_rot1[6] == true);
    REQUIRE(r58_rot1[7] == true);

    // Rotation by full period (8) is identical to 0
    const auto r58_rot8 = euclideanRhythm(8, 5, 8);
    for (std::size_t i = 0; i < 8; ++i) {
        REQUIRE(r58_rot8[i] == r58[i]);
    }

    // Boundary edge cases: k = 0 (all false)
    const auto r08 = euclideanRhythm(8, 0);
    REQUIRE(countEuclideanPulses(r08, 8) == 0);
    for (std::size_t i = 0; i < 8; ++i) {
        REQUIRE(r08[i] == false);
    }

    // Boundary edge cases: k >= n (all true)
    const auto r88 = euclideanRhythm(8, 8);
    REQUIRE(countEuclideanPulses(r88, 8) == 8);
    for (std::size_t i = 0; i < 8; ++i) {
        REQUIRE(r88[i] == true);
    }

    // k > n clamped to all true
    const auto r10_8 = euclideanRhythm(8, 10);
    REQUIRE(countEuclideanPulses(r10_8, 8) == 8);
}

TEST_CASE("MusicPlanner replay determinism with isolated seed", "[music_planner][replay_determinism]") {
    MusicPlanner planner1({.runSeed = 0xCAFEBABEULL, .candidateCount = 6, .sampleRate = 48000});
    MusicPlanner planner2({.runSeed = 0xCAFEBABEULL, .candidateCount = 6, .sampleRate = 48000});

    const PhrasePlan phrase{
        .sectionIndex = 0,
        .phraseIndex = 0,
        .startBar = 0,
        .barCount = 4,
        .startFrame = 0,
        .endFrame = 192000,
        .scale = ModalScale{.rootMidiNote = 62, .mode = ScaleMode::Dorian},
        .chords = {
            ChordSpec{0, 2, 4, HarmonicFunction::Stability, true},
            ChordSpec{3, 5, 0, HarmonicFunction::Preparation, true},
            ChordSpec{4, 6, 1, HarmonicFunction::Tension, true},
            ChordSpec{0, 2, 4, HarmonicFunction::Resolution, true}
        },
        .targetTension = 0.25f,
        .dominantRole = MusicRole::Vettore,
        .cadenceGoal = CadenceGoal::Sustain,
        .baseMotifId = 101
    };

    std::vector<MusicIntentEvent> out1;
    std::vector<MusicIntentEvent> out2;

    planner1.planPhrase(phrase, 0, 1, out1, nullptr);
    planner2.planPhrase(phrase, 0, 1, out2, nullptr);

    REQUIRE(!out1.empty());
    REQUIRE(out1.size() == out2.size());

    for (std::size_t i = 0; i < out1.size(); ++i) {
        CHECK(out1[i].eventId == out2[i].eventId);
        CHECK(out1[i].sampleFrame == out2[i].sampleFrame);
        CHECK(out1[i].role == out2[i].role);
        CHECK(out1[i].pitchHz == out2[i].pitchHz);
        CHECK(out1[i].velocity == out2[i].velocity);
        CHECK(out1[i].durationFrames == out2[i].durationFrames);
        CHECK(out1[i].timbreParam == out2[i].timbreParam);
    }

    REQUIRE(planner1.decisionTraces().size() == 1);
    REQUIRE(planner2.decisionTraces().size() == 1);
    CHECK(planner1.decisionTraces()[0].winningCandidateId == planner2.decisionTraces()[0].winningCandidateId);
    CHECK(planner1.decisionTraces()[0].winningScore == planner2.decisionTraces()[0].winningScore);
}

TEST_CASE("MusicPlanner RNG stream isolation", "[music_planner][stream_isolation]") {
    MusicPlanner pBase({.runSeed = 1337ULL, .candidateCount = 6, .sampleRate = 48000});

    const PhrasePlan phrase{
        .sectionIndex = 0,
        .phraseIndex = 0,
        .startBar = 0,
        .barCount = 4,
        .startFrame = 0,
        .endFrame = 192000,
        .scale = ModalScale{.rootMidiNote = 62, .mode = ScaleMode::Dorian},
        .chords = {
            ChordSpec{0, 2, 4, HarmonicFunction::Stability, true},
            ChordSpec{3, 5, 0, HarmonicFunction::Preparation, true},
            ChordSpec{4, 6, 1, HarmonicFunction::Tension, true},
            ChordSpec{0, 2, 4, HarmonicFunction::Resolution, true}
        },
        .targetTension = 0.35f,
        .dominantRole = MusicRole::Vettore,
        .cadenceGoal = CadenceGoal::Sustain,
        .baseMotifId = 101
    };

    std::vector<MusicIntentEvent> outBase;
    pBase.planPhrase(phrase, 0, 1, outBase, nullptr);

    // Mutate unrelated gameplay streams extensively
    const RandomKey kDir{1337ULL, static_cast<std::uint32_t>(StreamId::Director), 1, 0};
    for (int i = 0; i < 50; ++i) {
        (void)staffordMix13(randomU64(kDir));
    }
    const RandomKey kPattern{1337ULL, static_cast<std::uint32_t>(StreamId::Pattern), 42, 7};
    for (int i = 0; i < 50; ++i) {
        (void)staffordMix13(randomU64(kPattern));
    }

    MusicPlanner pMutated({.runSeed = 1337ULL, .candidateCount = 6, .sampleRate = 48000});
    std::vector<MusicIntentEvent> outMutated;
    pMutated.planPhrase(phrase, 0, 1, outMutated, nullptr);

    REQUIRE(outBase.size() == outMutated.size());
    for (std::size_t i = 0; i < outBase.size(); ++i) {
        CHECK(outBase[i].eventId == outMutated[i].eventId);
        CHECK(outBase[i].sampleFrame == outMutated[i].sampleFrame);
        CHECK(outBase[i].role == outMutated[i].role);
        CHECK(outBase[i].pitchHz == outMutated[i].pitchHz);
    }
}

TEST_CASE("Motif pure transformations preserve semantic invariants", "[music_planner][motif_invariance]") {
    const Motif base = MusicPlanner::makeDorianBaseMotif();
    const ModalScale scale{.rootMidiNote = 62, .mode = ScaleMode::Dorian};

    // 1. Modal Transpose (+2 degrees)
    const Motif transposed = applyModalTranspose(base, 2);
    REQUIRE(verifyContourPreserved(base, transposed));
    REQUIRE(transposed.noteCount == base.noteCount);
    for (std::size_t i = 0; i < base.noteCount; ++i) {
        CHECK(transposed.notes[i].scaleDegree == base.notes[i].scaleDegree + 2);
        CHECK(transposed.notes[i].isAnchor == base.notes[i].isAnchor);
    }

    // 2. Contour Inversion around axis degree 2
    const Motif inverted = applyContourInversion(base, 2);
    REQUIRE(verifyInversionSymmetric(base, inverted, 2));
    // d' = 2 - (d - 2) = 4 - d
    for (std::size_t i = 0; i < base.noteCount; ++i) {
        const std::int16_t expected = static_cast<std::int16_t>(4 - base.notes[i].scaleDegree);
        CHECK(inverted.notes[i].scaleDegree == expected);
    }

    // 3. Euclidean Rhythm Variation
    const Motif rhythmic = applyEuclideanRhythmVariation(base, 5, 0);
    REQUIRE(rhythmic.noteCount <= base.noteCount);

    // 4. Reorchestration
    const Motif reorch = applyReorchestration(base, MusicRole::Trama, 0);
    CHECK(reorch.targetRole == MusicRole::Trama);

    // 5. Chromatic Alteration: anchor note must never receive chromatic alteration
    const Motif chrom = applyChromaticAlteration(base, 0, 1); // Note 0 is anchor
    CHECK(chrom.notes[0].chromaticOffset == 0);               // Must remain 0 for anchors
    const Motif chromNonAnchor = applyChromaticAlteration(base, 1, 1); // Note 1 is non-anchor
    CHECK(chromNonAnchor.notes[1].chromaticOffset == 1);

    // Frequency conversion range check
    for (std::int16_t d = -14; d <= 21; ++d) {
        const float hz = degreeToPitchHz(scale, d, 0);
        CHECK(std::isfinite(hz));
        CHECK(hz >= 20.0f);
        CHECK(hz <= 16000.0f);
    }
}

TEST_CASE("Triad voicing, voice budget <= 2 on Corpo, and open-pad leases", "[music_planner][cross_phrase_lease_and_triads]") {
    // 1. Test voiceTriad
    const ChordSpec chord{0, 2, 4, HarmonicFunction::Stability, true};
    const TriadVoicing v1 = voiceTriad(chord, -1);
    CHECK(v1.corpoBassDegree == -7); // Root in bass (0 - 7 = -7)
    CHECK((v1.corpoHarmonyDegree == 4 || v1.corpoHarmonyDegree == 2));
    CHECK((v1.upperColorDegree == 2 || v1.upperColorDegree == 4));

    // Subsequent chord voice leading: minimal leap
    const ChordSpec chordG{3, 5, 0, HarmonicFunction::Preparation, true};
    const TriadVoicing v2 = voiceTriad(chordG, v1.corpoHarmonyDegree);
    CHECK(v2.corpoBassDegree == -4); // Root in bass (3 - 7 = -4)

    // 2. Open-pad lease management via AudioScheduler
    AudioScheduler scheduler({.lookaheadFrames = 4800, .sampleRate = 48000});
    MusicPlanner planner({.runSeed = 777ULL, .candidateCount = 6, .sampleRate = 48000});

    const PhrasePlan phrase{
        .sectionIndex = 0,
        .phraseIndex = 0,
        .startBar = 0,
        .barCount = 4,
        .startFrame = 0,
        .endFrame = 96000,
        .scale = ModalScale{.rootMidiNote = 62, .mode = ScaleMode::Dorian},
        .chords = {
            ChordSpec{0, 2, 4, HarmonicFunction::Stability, true},
            ChordSpec{3, 5, 0, HarmonicFunction::Preparation, true},
            ChordSpec{4, 6, 1, HarmonicFunction::Tension, true},
            ChordSpec{0, 2, 4, HarmonicFunction::Resolution, true}
        },
        .targetTension = 0.20f,
        .dominantRole = MusicRole::Vettore,
        .cadenceGoal = CadenceGoal::Sustain,
        .baseMotifId = 101
    };

    std::vector<MusicIntentEvent> intents;
    planner.planPhrase(phrase, 0, 1, intents, &scheduler);

    REQUIRE(!intents.empty());
    REQUIRE(scheduler.stagedEventCount() > 0);

    // Verify that SimulatedVoiceTracker has <= 2 Corpo voices active
    const auto& tracker = planner.voiceTracker();
    CHECK(tracker.activeCountForRole(MusicRole::Corpo, 96000) <= 2);
}

TEST_CASE("Simulated voice tracking and voice stealing simulation", "[music_planner][voice_stealing_simulation]") {
    SimulatedVoiceTracker tracker;

    // Slot allocation on Pulso (role index 0: slots 0, 1)
    bool stolen = false;
    const int slot0 = tracker.simulateNoteOn(MusicRole::Pulso, 101, 0, 1000, 110.0f, stolen);
    CHECK(slot0 == 0);
    CHECK(!stolen);
    CHECK(tracker.activeCountForRole(MusicRole::Pulso, 0) == 1);

    const int slot1 = tracker.simulateNoteOn(MusicRole::Pulso, 102, 200, 1000, 220.0f, stolen);
    CHECK(slot1 == 1);
    CHECK(!stolen);
    CHECK(tracker.activeCountForRole(MusicRole::Pulso, 200) == 2);

    // Third note exceeds 2-slot budget: triggers stealing of oldest note (slot 0)
    const int slotSteal = tracker.simulateNoteOn(MusicRole::Pulso, 103, 400, 1000, 330.0f, stolen);
    CHECK(slotSteal == 0); // Stole slot 0
    CHECK(stolen == true);
    CHECK(tracker.activeCountForRole(MusicRole::Pulso, 400) == 2);

    // Release note 102 at 600 (enters 2 ms / 96-sample de-click tail until 696)
    tracker.simulateNoteOff(MusicRole::Pulso, 102, 600);
    tracker.updateAtFrame(700);
    CHECK(tracker.activeCountForRole(MusicRole::Pulso, 700) == 1);

    // Natural expiration of note 103 at 1400
    tracker.updateAtFrame(1400);
    CHECK(tracker.activeCountForRole(MusicRole::Pulso, 1400) == 0);
}

TEST_CASE("Candidate tie-break permutation invariance and canonical eventId assignment", "[music_planner][tiebreak_permutation_invariance]") {
    MusicPlanner planner({.runSeed = 999ULL, .candidateCount = 4, .sampleRate = 48000});

    std::vector<PhraseCandidate> cands;
    for (std::uint32_t i = 1; i <= 4; ++i) {
        PhraseCandidate c{};
        c.candidateId = i;
        c.scoreFixedPoint = 85000; // All tied in fixed-point score
        c.eventCount = 1;
        c.events[0] = CandidateEvent{
            .localEventId = i * 10,
            .sampleFrame = 1000,
            .role = MusicRole::Vettore,
            .pitchHz = 440.0f,
            .velocity = 0.8f,
            .durationFrames = 2400,
            .timbreParam = 0.5f,
            .isOpenPad = false
        };
        cands.push_back(c);
    }

    // Original ordering
    auto cands1 = cands;
    const auto winner1 = planner.selectWinningCandidate(cands1, 555);

    // Permuted ordering: reverse
    auto cands2 = cands;
    std::reverse(cands2.begin(), cands2.end());
    const auto winner2 = planner.selectWinningCandidate(cands2, 555);

    // Winning candidate ID must be bit-exact identical
    CHECK(winner1.candidateId == winner2.candidateId);
    CHECK(winner1.tieBreakHash == winner2.tieBreakHash);
    CHECK(winner1.scoreFixedPoint == winner2.scoreFixedPoint);

    // Post-selection sequential eventId assignment
    std::vector<MusicIntentEvent> out;
    const PhrasePlan phrase{
        .sectionIndex = 0,
        .phraseIndex = 0,
        .startBar = 0,
        .barCount = 4,
        .startFrame = 0,
        .endFrame = 96000,
        .scale = ModalScale{.rootMidiNote = 62, .mode = ScaleMode::Dorian},
        .chords = {
            ChordSpec{0, 2, 4, HarmonicFunction::Stability, false},
            ChordSpec{3, 5, 0, HarmonicFunction::Preparation, false},
            ChordSpec{4, 6, 1, HarmonicFunction::Tension, false},
            ChordSpec{0, 2, 4, HarmonicFunction::Resolution, false}
        },
        .targetTension = 0.20f,
        .dominantRole = MusicRole::Vettore,
        .cadenceGoal = CadenceGoal::Sustain,
        .baseMotifId = 101
    };

    planner.planPhrase(phrase, 0, 1, out, nullptr);
    REQUIRE(!out.empty());
    // Sequential eventId must be strictly monotonically increasing from 1
    for (std::size_t i = 1; i < out.size(); ++i) {
        CHECK(out[i].eventId == out[i - 1].eventId + 1);
    }
}

TEST_CASE("Fail-closed fallback hierarchy and offline CACP probe validation", "[music_planner][fallback_and_cacp_non_silent]") {
    MusicPlanner planner({.runSeed = 1337ULL});

    // 1. Fallback Level 4 (Silent Recovery): exhausted horizon
    PhrasePlan pExhausted{
        .sectionIndex = 0,
        .phraseIndex = 0,
        .startBar = 0,
        .barCount = 4,
        .startFrame = 50000,
        .endFrame = 50000, // Zero frame budget remaining
        .scale = ModalScale{.rootMidiNote = 62, .mode = ScaleMode::Dorian},
        .chords = {},
        .targetTension = 0.20f,
        .dominantRole = MusicRole::Vettore,
        .cadenceGoal = CadenceGoal::Sustain,
        .baseMotifId = 101
    };

    std::vector<MusicIntentEvent> outFb;
    planner.planPhrase(pExhausted, 50000, 1, outFb, nullptr);
    CHECK(outFb.empty());
    CHECK(planner.totalFallbacksTriggered() == 1);
    REQUIRE(!planner.decisionTraces().empty());
    CHECK(planner.decisionTraces().back().fallbackTriggered == true);
    CHECK(planner.decisionTraces().back().fallbackKind == FallbackKind::SilentRecovery);

    // 2. Authoritative CACP Probe Offline Validation
    const auto res = runPlannerProbeV1Offline("artifacts/cacp", false);
    REQUIRE(res.passed);
    CHECK(res.determinismPassed);
    CHECK(res.streamIsolationPassed);
    CHECK(res.motifInvariancePassed);
    CHECK(res.triadBudgetPassed);
    CHECK(res.fallbackPassed);
    CHECK(res.nonSilentAudioPassed);
    CHECK(res.truePeakPassed);
    CHECK(res.truePeakDbtp <= -1.0f);
    CHECK(res.clippedSamples == 0);
    CHECK(res.dspAnomaliesPassed);
    CHECK(res.droppedFrames == 0);
    CHECK(res.totalCapturedFrames >= res.requestedSampleFrames);
}
