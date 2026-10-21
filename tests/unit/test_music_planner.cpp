#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "audio_probe.h"
#include "audio_scheduler.h"
#include "euclidean_rhythm.h"
#include "music_clock.h"
#include "music_planner.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <set>
#include <vector>

using namespace cymatica::audio;
using namespace cymatica::core;

namespace {

struct PlannedRun {
    std::vector<std::vector<MusicIntentEvent>> intents;
    std::vector<PhraseDecisionTrace> traces;
    std::vector<PlannedNoteOff> noteOffs;
    SimulatedVoiceTracker tracker;
};

MusicClock probeClock() {
    auto clock = MusicPlanner::makePlannerProbeV1Clock();
    REQUIRE(clock.has_value());
    return *clock;
}

PlannedRun planProbeForm(std::uint64_t seed, AudioScheduler* scheduler = nullptr,
                         PolicyVersionId policy = DEFAULT_POLICY_VERSION_ID) {
    const MusicClock clock = probeClock();
    const auto phrases = MusicPlanner::makePlannerProbeV1Phrases(clock);
    MusicPlanner planner({.runSeed = seed, .policyVersion = policy, .candidateCount = 6, .sampleRate = 48000});
    planner.setFormPlan(MusicPlanner::makePlannerProbeV1Form());
    PlannedRun run{};
    const std::array<std::uint64_t, 2> epochs{1, 2};
    for (std::size_t p = 0; p < phrases.size(); ++p) {
        std::vector<MusicIntentEvent> out;
        planner.planPhrase(phrases[p], clock, phrases[p].startFrame, epochs[p], out, scheduler);
        run.intents.push_back(out);
    }
    run.traces = planner.decisionTraces();
    run.noteOffs = planner.plannedNoteOffs();
    run.tracker = planner.voiceTracker();
    return run;
}

// Independent oracle: frame of step `stepFromStart` (16th notes) after `startFrame` at integer BPM in 4/4,
// floor(step * 60 * 48000 / (bpm * 4)) relative to the segment start.
std::uint64_t oracleStepFrame(std::uint64_t segmentStartFrame, std::uint64_t stepFromSegmentStart, std::uint64_t bpm) {
    return segmentStartFrame + (stepFromSegmentStart * 60ULL * 48000ULL) / (bpm * 4ULL);
}

// Independent oracle: equal-tempered MIDI pitch, clamped like the planner's legal range
float oracleMidiHz(int midi) {
    return std::clamp(440.0f * std::pow(2.0f, (static_cast<float>(midi) - 69.0f) / 12.0f), 20.0f, 16000.0f);
}

// --- Canonical little-endian serialization + FNV-1a 64 (golden regression) ---
struct Fnv1a64 {
    std::uint64_t h{0xcbf29ce484222325ULL};
    void byte(std::uint8_t b) { h ^= b; h *= 0x100000001b3ULL; }
    void u64(std::uint64_t v) { for (int i = 0; i < 8; ++i) byte(static_cast<std::uint8_t>(v >> (8 * i))); }
    void u32(std::uint32_t v) { for (int i = 0; i < 4; ++i) byte(static_cast<std::uint8_t>(v >> (8 * i))); }
    void i32(std::int32_t v) { u32(static_cast<std::uint32_t>(v)); }
    void f32(float v) { u32(std::bit_cast<std::uint32_t>(v)); }
};

std::uint64_t canonicalHash(const PlannedRun& run) {
    Fnv1a64 f;
    for (const auto& phrase : run.intents) {
        f.u64(phrase.size());
        for (const auto& e : phrase) {
            f.u64(e.eventId); f.u64(e.sampleFrame); f.u64(e.tick); f.u64(e.transportEpoch);
            f.byte(static_cast<std::uint8_t>(e.role)); f.f32(e.pitchHz); f.f32(e.velocity);
            f.u32(e.durationFrames); f.f32(e.timbreParam);
        }
    }
    f.u64(run.noteOffs.size());
    for (const auto& o : run.noteOffs) {
        f.u64(o.eventId); f.byte(static_cast<std::uint8_t>(o.role)); f.u64(o.sampleFrame); f.u64(o.transportEpoch);
    }
    f.u64(run.traces.size());
    for (const auto& t : run.traces) {
        f.u64(t.decisionId); f.u64(t.tieBreakDecisionId); f.u64(t.runSeed); f.u32(t.policyVersion);
        f.u32(t.scoringProfileVersion); f.u32(t.sectionIndex); f.u32(t.phraseIndex); f.u32(t.startBar);
        f.i32(t.targetTensionMilli); f.byte(static_cast<std::uint8_t>(t.planViolation));
        f.u32(t.candidatesGenerated); f.u32(t.candidatesValid);
        for (std::size_t c = 0; c < t.candidatesGenerated; ++c) {
            const auto& e = t.candidates[c];
            f.u64(e.candidateId); f.byte(static_cast<std::uint8_t>(e.transformKind)); f.i32(e.transformParam);
            f.u32(e.motifId); f.byte(static_cast<std::uint8_t>(e.violation)); f.i32(e.score); f.u64(e.tieBreakHash);
            f.u32(e.simulatedSteals); f.u32(e.eventCount);
            f.i32(e.features.voiceLeadingDistance); f.i32(e.features.tensionEstimateMilli);
            f.i32(e.features.anchorFidelityMilli); f.i32(e.features.onsetDensityMilli); f.u32(e.features.simulatedSteals);
        }
        f.u64(t.winningCandidateId); f.i32(t.winningScore); f.byte(static_cast<std::uint8_t>(t.winningTransformKind));
        f.i32(t.winningTransformParam); f.u32(t.winningMotif.motifId); f.u32(t.tiedTopCandidates);
        f.byte(t.fallbackTriggered ? 1 : 0); f.byte(static_cast<std::uint8_t>(t.fallbackKind));
        for (const auto v : t.fallbackLevelViolations) f.byte(static_cast<std::uint8_t>(v));
        f.u64(t.firstEventId); f.u64(t.finalEventCount); f.u32(t.plannerNoteOffCount);
    }
    return f.h;
}

} // namespace

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

TEST_CASE("MusicPlanner replay determinism: canonical equality of intents, NoteOffs and traces", "[music_planner][replay_determinism]") {
    const PlannedRun a = planProbeForm(0xCAFEBABEULL);
    const PlannedRun b = planProbeForm(0xCAFEBABEULL);
    REQUIRE(a.traces.size() == 2);
    REQUIRE(!a.intents[0].empty());
    REQUIRE(!a.intents[1].empty());
    CHECK(a.intents == b.intents);
    CHECK(a.noteOffs == b.noteOffs);
    CHECK(a.traces == b.traces);

    // Trace is self-describing: seed, policy, scoring profile and per-candidate records
    const auto& t0 = a.traces[0];
    CHECK(t0.runSeed == 0xCAFEBABEULL);
    CHECK(t0.policyVersion == DEFAULT_POLICY_VERSION_ID.toU32());
    CHECK(t0.scoringProfileVersion == kPlannerScoringProfileV1.versionId);
    CHECK(t0.candidatesGenerated == 6);
    CHECK(t0.decisionId == deriveDecisionId(0, 0, 0, kPhraseDecisionSlot, DEFAULT_POLICY_VERSION_ID.toU32()));
    CHECK(t0.tieBreakDecisionId == deriveDecisionId(0, 0, 0, kTieBreakDecisionSlot, DEFAULT_POLICY_VERSION_ID.toU32()));
    CHECK(t0.decisionId != t0.tieBreakDecisionId);
}

TEST_CASE("Simulated voice state is independent of the scheduler pointer", "[music_planner][replay_determinism]") {
    AudioScheduler scheduler({.lookaheadFrames = 7200, .sampleRate = 48000});
    const PlannedRun withSched = planProbeForm(1337ULL, &scheduler);
    const PlannedRun without = planProbeForm(1337ULL, nullptr);
    CHECK(withSched.traces == without.traces);
    CHECK(withSched.intents == without.intents);
    for (std::size_t i = 0; i < SimulatedVoiceTracker::kTotalSlots; ++i) {
        const auto& s1 = withSched.tracker.slots()[i];
        const auto& s2 = without.tracker.slots()[i];
        CHECK(s1.active == s2.active);
        CHECK(s1.eventId == s2.eventId);
        CHECK(s1.inRelease == s2.inRelease);
        CHECK(s1.noteOffFrame == s2.noteOffFrame);
    }
    // Every NoteOn produces exactly one NoteOff in the scheduler (duration-based or explicit open-pad NoteOff)
    std::size_t intents = withSched.intents[0].size() + withSched.intents[1].size();
    CHECK(scheduler.stagedEventCount() == 2 * intents);
}

TEST_CASE("Seed isolation is metamorphic and the tie-break key is domain separated", "[music_planner][stream_isolation]") {
    const PlannedRun a = planProbeForm(1337ULL);
    const PlannedRun b = planProbeForm(0x0123456789ABCDEFULL);
    REQUIRE(a.traces.size() == b.traces.size());
    bool comparable = true;
    for (std::size_t p = 0; p < a.traces.size() && comparable; ++p) {
        const auto& ta = a.traces[p];
        const auto& tb = b.traces[p];
        // Scores and violations never depend on the seed
        for (std::size_t c = 0; c < ta.candidatesGenerated; ++c) {
            CHECK(ta.candidates[c].score == tb.candidates[c].score);
            CHECK(ta.candidates[c].violation == tb.candidates[c].violation);
            CHECK(ta.candidates[c].features == tb.candidates[c].features);
        }
        if (ta.winningCandidateId != tb.winningCandidateId) {
            // Only a tie among equal-score candidates may be decided differently
            CHECK(ta.tiedTopCandidates >= 2);
            CHECK(ta.winningScore == tb.winningScore);
            comparable = false;
        } else {
            CHECK(a.intents[p] == b.intents[p]);
        }
    }

    // Explicit policy version: a different policy changes decision ids and tie-break hashes
    const PlannedRun c = planProbeForm(1337ULL, nullptr, PolicyVersionId{1, 0, 2, 0});
    CHECK(c.traces[0].tieBreakDecisionId != a.traces[0].tieBreakDecisionId);
    CHECK(c.traces[0].candidates[0].tieBreakHash != a.traces[0].candidates[0].tieBreakHash);

    // The tie-break hash is exactly the documented MusicForm key and collides with no other stream
    MusicPlanner planner({.runSeed = 1337ULL});
    for (const auto& t : a.traces) {
        for (std::size_t i = 0; i < t.candidatesGenerated; ++i) {
            const auto& e = t.candidates[i];
            const auto sample = static_cast<std::uint32_t>(e.candidateId & 0xFFFFFFFFULL);
            const RandomKey formKey{1337ULL, static_cast<std::uint32_t>(StreamId::MusicForm), t.tieBreakDecisionId, sample};
            CHECK(randomU64(formKey, t.policyVersion) == e.tieBreakHash);
            CHECK(planner.tieBreakHash(t.tieBreakDecisionId, e.candidateId) == e.tieBreakHash);
            for (const auto other : {StreamId::Harmony, StreamId::Rhythm, StreamId::Director, StreamId::Pattern}) {
                const RandomKey otherKey{1337ULL, static_cast<std::uint32_t>(other), t.tieBreakDecisionId, sample};
                CHECK(randomU64(otherKey, t.policyVersion) != e.tieBreakHash);
            }
        }
    }

    // candidateId packing never overflows into the section field for large phrase indices
    const MusicClock clock = probeClock();
    auto phrases = MusicPlanner::makePlannerProbeV1Phrases(clock);
    PhrasePlan pBig = phrases[0];
    pBig.phraseIndex = 300;
    MusicPlanner bigPlanner({.runSeed = 1337ULL});
    std::vector<MusicIntentEvent> outBig;
    bigPlanner.planPhrase(pBig, clock, pBig.startFrame, 1, outBig, nullptr);
    const auto& tBig = bigPlanner.decisionTraces().back();
    std::set<std::uint64_t> ids;
    for (std::size_t i = 0; i < tBig.candidatesGenerated; ++i) {
        const std::uint64_t id = tBig.candidates[i].candidateId;
        CHECK((id >> 40) == pBig.sectionIndex);
        CHECK(((id >> 8) & 0xFFFFFFFFULL) == 300ULL);
        ids.insert(id);
    }
    CHECK(ids.size() == tBig.candidatesGenerated);
}

TEST_CASE("Motif pure transformations preserve semantic invariants", "[music_planner][motif_invariance]") {
    const Motif base = MusicPlanner::makeDorianBaseMotif();
    const ModalScale scale{.rootMidiNote = 62, .mode = ScaleMode::Dorian};

    // 1. Modal Transpose (+2 degrees)
    const Motif transposed = applyModalTranspose(base, 2);
    REQUIRE(verifyContourPreserved(base, transposed));
    REQUIRE(verifyMotifTransformInvariants(base, transposed, MotifTransformKind::ModalTranspose, 2));
    REQUIRE(transposed.noteCount == base.noteCount);
    for (std::size_t i = 0; i < base.noteCount; ++i) {
        CHECK(transposed.notes[i].scaleDegree == base.notes[i].scaleDegree + 2);
        CHECK(transposed.notes[i].isAnchor == base.notes[i].isAnchor);
    }

    // 2. Contour Inversion around axis degree 2
    const Motif inverted = applyContourInversion(base, 2);
    REQUIRE(verifyInversionSymmetric(base, inverted, 2));
    REQUIRE(verifyMotifTransformInvariants(base, inverted, MotifTransformKind::ContourInversion, 2));
    // d' = 2 - (d - 2) = 4 - d
    for (std::size_t i = 0; i < base.noteCount; ++i) {
        const std::int16_t expected = static_cast<std::int16_t>(4 - base.notes[i].scaleDegree);
        CHECK(inverted.notes[i].scaleDegree == expected);
    }
    // Wrong pivot must be detected
    CHECK_FALSE(verifyMotifTransformInvariants(base, inverted, MotifTransformKind::ContourInversion, 3));

    // 3. Euclidean Rhythm Variation
    const Motif rhythmic = applyEuclideanRhythmVariation(base, 4, 3);
    REQUIRE(rhythmic.noteCount <= base.noteCount);
    REQUIRE(verifyMotifTransformInvariants(base, rhythmic, MotifTransformKind::EuclideanRhythmVariation, 3));

    // 4. Reorchestration / octave shift
    const Motif reorch = applyReorchestration(base, MusicRole::Trama, 0);
    CHECK(reorch.targetRole == MusicRole::Trama);
    const Motif octave = applyReorchestration(base, MusicRole::Vettore, 1);
    CHECK(verifyMotifTransformInvariants(base, octave, MotifTransformKind::OctaveShift, 1));
    CHECK_FALSE(verifyMotifTransformInvariants(base, octave, MotifTransformKind::OctaveShift, 2));

    // 5. Chromatic Alteration: anchor note must never receive chromatic alteration
    const Motif chrom = applyChromaticAlteration(base, 0, 1); // Note 0 is anchor
    CHECK(chrom.notes[0].chromaticOffset == 0);               // Must remain 0 for anchors
    const Motif chromNonAnchor = applyChromaticAlteration(base, 1, 1); // Note 1 is non-anchor
    CHECK(chromNonAnchor.notes[1].chromaticOffset == 1);
    CHECK(verifyMotifTransformInvariants(base, chromNonAnchor, MotifTransformKind::ChromaticAlteration, 1));
    Motif anchorAltered = base;
    anchorAltered.notes[0].chromaticOffset = 1;
    CHECK_FALSE(verifyMotifTransformInvariants(base, anchorAltered, MotifTransformKind::ChromaticAlteration, 1));

    // 6. Identity rejects any modification
    CHECK(verifyMotifTransformInvariants(base, base, MotifTransformKind::Identity, 0));
    CHECK_FALSE(verifyMotifTransformInvariants(base, transposed, MotifTransformKind::Identity, 0));

    // Derived motif ids are distinct per transform (Identity keeps the base id)
    CHECK(deriveTransformedMotifId(101, MotifTransformKind::Identity, 0) == 101);
    CHECK(deriveTransformedMotifId(101, MotifTransformKind::ModalTranspose, 1) !=
          deriveTransformedMotifId(101, MotifTransformKind::ModalTranspose, 6));
    CHECK(deriveTransformedMotifId(101, MotifTransformKind::ModalTranspose, 1) !=
          deriveTransformedMotifId(101, MotifTransformKind::ContourInversion, 1));

    // Frequency conversion range check
    for (std::int16_t d = -14; d <= 21; ++d) {
        const float hz = degreeToPitchHz(scale, d, 0);
        CHECK(std::isfinite(hz));
        CHECK(hz >= 20.0f);
        CHECK(hz <= 16000.0f);
    }

    // Every winning candidate of the planned form satisfies its transform invariants (from the trace)
    const PlannedRun run = planProbeForm(1337ULL);
    for (const auto& t : run.traces) {
        CHECK(verifyMotifTransformInvariants(t.baseMotif, t.winningMotif, t.winningTransformKind, t.winningTransformParam));
        std::set<std::uint32_t> motifIds;
        for (std::size_t i = 0; i < t.candidatesGenerated; ++i) motifIds.insert(t.candidates[i].motifId);
        CHECK(motifIds.size() == t.candidatesGenerated);
    }
}

TEST_CASE("Triad voicing, per-bar chord realization, Corpo durations and open-pad leases", "[music_planner][cross_phrase_lease_and_triads]") {
    // 1. voiceTriad with optional voice-leading reference
    const ChordSpec chord{0, 2, 4, HarmonicFunction::Stability, true};
    const TriadVoicing v1 = voiceTriad(chord, std::nullopt);
    CHECK(v1.corpoBassDegree == -7); // Root in bass (0 - 7 = -7)
    CHECK(v1.corpoHarmonyDegree == 4);
    CHECK(v1.upperColorDegree == 2);
    // Degree 0 is a legal previous harmony (no -1 sentinel): third (2) is closer to 0 than fifth (4)
    const TriadVoicing v0 = voiceTriad(chord, std::int16_t{0});
    CHECK(v0.corpoHarmonyDegree == 2);
    CHECK(v0.isThirdInCorpo);

    const ChordSpec chordG{3, 5, 0, HarmonicFunction::Preparation, true};
    const TriadVoicing v2 = voiceTriad(chordG, v1.corpoHarmonyDegree);
    CHECK(v2.corpoBassDegree == -4); // Root in bass (3 - 7 = -4)

    // 2. Section A (per-bar chords): bar b realizes chords[b], Corpo notes last exactly one bar
    const MusicClock clock = probeClock();
    const auto phrases = MusicPlanner::makePlannerProbeV1Phrases(clock);
    AudioScheduler scheduler({.lookaheadFrames = 7200, .sampleRate = 48000});
    MusicPlanner planner({.runSeed = 777ULL, .candidateCount = 6, .sampleRate = 48000});
    std::vector<MusicIntentEvent> outA;
    planner.planPhrase(phrases[0], clock, phrases[0].startFrame, 1, outA, &scheduler);
    REQUIRE(!planner.decisionTraces().back().fallbackTriggered);

    constexpr std::array<int, 7> kDorian{0, 2, 3, 5, 7, 9, 10};
    std::vector<MusicIntentEvent> corpoA;
    std::copy_if(outA.begin(), outA.end(), std::back_inserter(corpoA), [](const MusicIntentEvent& e) { return e.role == MusicRole::Corpo; });
    REQUIRE(corpoA.size() == 8);
    for (std::uint32_t b = 0; b < 4; ++b) {
        const auto& bass = corpoA[2 * b];
        CHECK(bass.sampleFrame == clock.frameAtBar(b));
        CHECK(bass.durationFrames == clock.frameAtBar(b + 1) - clock.frameAtBar(b));
        CHECK(corpoA[2 * b + 1].durationFrames == bass.durationFrames);
        // Bass = chord root one octave below the D4 tonic (independent Dorian table)
        const int root = phrases[0].chords[b].rootDegree;
        const int expectedMidi = 62 - 12 + kDorian[static_cast<std::size_t>(root)];
        CHECK(bass.pitchHz == oracleMidiHz(expectedMidi));
    }
    CHECK(planner.plannedNoteOffs().empty());

    // 3. Section B (open pad): one sustained pad on chords[0], explicit NoteOff at the phrase end
    std::vector<MusicIntentEvent> outB;
    planner.planPhrase(phrases[1], clock, phrases[1].startFrame, 2, outB, &scheduler);
    std::vector<MusicIntentEvent> corpoB;
    std::copy_if(outB.begin(), outB.end(), std::back_inserter(corpoB), [](const MusicIntentEvent& e) { return e.role == MusicRole::Corpo; });
    REQUIRE(corpoB.size() == 2);
    for (const auto& e : corpoB) {
        CHECK(e.durationFrames == 0);
        CHECK(e.sampleFrame == clock.frameAtBar(4));
    }
    REQUIRE(planner.plannedNoteOffs().size() == 2);
    for (const auto& off : planner.plannedNoteOffs()) {
        CHECK(off.sampleFrame == clock.frameAtBar(8));
        CHECK(off.transportEpoch == 2);
    }
    CHECK(scheduler.stagedEventCount() == 2 * (outA.size() + outB.size()));

    // Corpo overlap in emitted intents never exceeds the 2-voice budget
    struct Edge { std::uint64_t f; int d; };
    std::vector<Edge> edges;
    for (const auto* v : {&outA, &outB}) {
        for (const auto& e : *v) {
            if (e.role != MusicRole::Corpo) continue;
            const std::uint64_t off = e.durationFrames > 0 ? e.sampleFrame + e.durationFrames : clock.frameAtBar(8);
            edges.push_back({e.sampleFrame, +1});
            edges.push_back({off, -1});
        }
    }
    std::sort(edges.begin(), edges.end(), [](const Edge& x, const Edge& y) { return x.f != y.f ? x.f < y.f : x.d < y.d; });
    int active = 0;
    int maxActive = 0;
    for (const auto& e : edges) { active += e.d; maxActive = std::max(maxActive, active); }
    CHECK(maxActive == 2);
    CHECK(active == 0);
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

    // Third note exceeds 2-slot budget: Rule 2 steal of the oldest held note (slot 0)
    const int slotSteal = tracker.simulateNoteOn(MusicRole::Pulso, 103, 400, 1000, 330.0f, stolen);
    CHECK(slotSteal == 0); // Stole slot 0
    CHECK(stolen == true);
    CHECK(tracker.heldVoiceSteals() == 1);
    CHECK(tracker.activeCountForRole(MusicRole::Pulso, 400) == 2);

    // Release note 102 at 600 (enters 2 ms / 96-sample de-click tail until 696)
    tracker.simulateNoteOff(MusicRole::Pulso, 102, 600);
    tracker.updateAtFrame(700);
    CHECK(tracker.activeCountForRole(MusicRole::Pulso, 700) == 1);

    // Nominal NoteOff of note 103 at 1400
    tracker.updateAtFrame(1400);
    CHECK(tracker.activeCountForRole(MusicRole::Pulso, 1400) == 0);

    // Rule 1: both slots in release at a same-frame onset -> release steal, not a held-voice steal
    SimulatedVoiceTracker t2;
    (void)t2.simulateNoteOn(MusicRole::Corpo, 1, 0, 1000, 100.0f, stolen);
    (void)t2.simulateNoteOn(MusicRole::Corpo, 2, 0, 1000, 150.0f, stolen);
    (void)t2.simulateNoteOn(MusicRole::Corpo, 3, 1000, 1000, 100.0f, stolen);
    CHECK(stolen);
    (void)t2.simulateNoteOn(MusicRole::Corpo, 4, 1000, 1000, 150.0f, stolen);
    CHECK(stolen);
    CHECK(t2.heldVoiceSteals() == 0);
    CHECK(t2.activeCountForRole(MusicRole::Corpo, 1000) == 2);
}

TEST_CASE("Integer score uses the versioned profile and simulated steals penalize candidates", "[music_planner][scoring]") {
    const PlannedRun run = planProbeForm(1337ULL);
    const auto& prof = kPlannerScoringProfileV1;
    CHECK(prof.versionId == 1);
    CHECK(prof.weightVoiceLeading + prof.weightTension + prof.weightMotif + prof.weightRhythm == 100);

    bool anySteal = false;
    std::set<std::int32_t> distinctScores;
    for (const auto& t : run.traces) {
        for (std::size_t i = 0; i < t.candidatesGenerated; ++i) {
            const auto& e = t.candidates[i];
            if (e.violation != HardConstraintViolation::None) continue;
            distinctScores.insert(e.score);
            if (e.simulatedSteals > 0) anySteal = true;
            CHECK(e.features.simulatedSteals == e.simulatedSteals);
            // Recompute the documented integer formula from the recorded features
            const auto f = e.features;
            const std::int32_t sVl = std::clamp(1000 - f.voiceLeadingDistance * prof.voiceLeadingPenaltyPerDegree, 0, 1000);
            const std::int32_t sTen = std::clamp(1000 - std::abs(f.tensionEstimateMilli - t.targetTensionMilli), 0, 1000);
            const std::int32_t sMot = std::clamp(f.anchorFidelityMilli, 0, 1000);
            const std::int32_t sRhy = std::clamp(1000 - (std::abs(f.onsetDensityMilli - prof.targetOnsetsPerBarMilli) * prof.rhythmPenaltyPerOnset) / 1000, 0, 1000);
            const std::int64_t total = 25LL * sVl + 25LL * sTen + 25LL * sMot + 25LL * sRhy -
                                       static_cast<std::int64_t>(prof.penaltyPerSteal) * e.simulatedSteals;
            CHECK(e.score == static_cast<std::int32_t>(std::clamp<std::int64_t>(total, 0, 100000)));
        }
    }
    // Long Trama sustain candidates overlap three notes -> held-voice steals are simulated and penalized
    CHECK(anySteal);
    // Transforms produce a non-constant score landscape
    CHECK(distinctScores.size() >= 4);
    CHECK(run.traces[0].targetTensionMilli == 250);
    CHECK(run.traces[1].targetTensionMilli == 600);
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
            .midiNote = 69,
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

    // Post-selection sequential eventId assignment in canonical (frame, role, pitch, duration) order
    const PlannedRun run = planProbeForm(999ULL);
    std::uint64_t expectedId = 1;
    for (const auto& phrase : run.intents) {
        REQUIRE(!phrase.empty());
        for (std::size_t i = 0; i < phrase.size(); ++i) {
            CHECK(phrase[i].eventId == expectedId++);
            if (i > 0) {
                const auto& p = phrase[i - 1];
                const auto& c = phrase[i];
                const bool ordered = p.sampleFrame < c.sampleFrame ||
                    (p.sampleFrame == c.sampleFrame && (p.role < c.role ||
                    (p.role == c.role && (p.pitchHz < c.pitchHz ||
                    (p.pitchHz == c.pitchHz && p.durationFrames <= c.durationFrames)))));
                CHECK(ordered);
            }
            // Normative M1 mapping (120 Hz -> 400 frames/tick): ceil(frame / 400), oracle computed here
            CHECK(phrase[i].tick == (phrase[i].sampleFrame + 399) / 400);
        }
    }
}

TEST_CASE("Onset grid is derived from MusicClock (zero drift at 140 BPM) with oracle pitches", "[music_planner][clock_grid]") {
    const MusicClock clock = probeClock();
    const auto phrases = MusicPlanner::makePlannerProbeV1Phrases(clock);
    REQUIRE(phrases[0].startFrame == 0);
    REQUIRE(phrases[0].endFrame == 384000);
    REQUIRE(phrases[1].startFrame == 384000);
    REQUIRE(phrases[1].endFrame == 713142);

    const PlannedRun run = planProbeForm(1337ULL);
    // Every onset of section B lies exactly on the rational 16th-note grid of 140 BPM anchored at 384000
    std::set<std::uint64_t> gridB;
    for (std::uint64_t s = 0; s < 64; ++s) gridB.insert(oracleStepFrame(384000, s, 140));
    for (const auto& e : run.intents[1]) {
        CHECK(gridB.count(e.sampleFrame) == 1);
        CHECK(e.sampleFrame + e.durationFrames <= 713142);
    }
    // Last step of section B: exact floor-rational value (the old truncated grid was 15 frames early)
    CHECK(oracleStepFrame(384000, 63, 140) == clock.frameAtBeatStep(16 + 15, 3, 4));
    CHECK(oracleStepFrame(384000, 63, 140) == 384000 + (63ULL * 60 * 48000) / 560);

    // Section A grid at 120 BPM: 6000 frames per 16th
    for (const auto& e : run.intents[0]) {
        if (e.role == MusicRole::Pulso || e.role == MusicRole::Trama || e.role == MusicRole::Vettore || e.role == MusicRole::Corpo) {
            CHECK(e.sampleFrame % 6000 == 0);
        }
    }

    // Pulso pitch: scale degree -14 (two octaves below the tonic), independent MIDI formula
    for (const auto& e : run.intents[0]) {
        if (e.role == MusicRole::Pulso) CHECK(e.pitchHz == oracleMidiHz(62 - 24));
    }
    for (const auto& e : run.intents[1]) {
        if (e.role == MusicRole::Pulso) CHECK(e.pitchHz == oracleMidiHz(55 - 24));
    }

    // Vettore pitches of the winner: tonic + Dorian intervals + octave/chromatic offsets
    constexpr std::array<int, 7> kDorian{0, 2, 3, 5, 7, 9, 10};
    const auto& t0 = run.traces[0];
    std::set<float> expectedVettore;
    for (std::size_t m = 0; m < t0.winningMotif.noteCount; ++m) {
        const auto& n = t0.winningMotif.notes[m];
        const int degree = n.scaleDegree + 7 * t0.winningMotif.registerOctaveOffset;
        const int oct = (degree >= 0) ? degree / 7 : -((-degree + 6) / 7);
        const int deg = degree - oct * 7;
        expectedVettore.insert(oracleMidiHz(62 + oct * 12 + kDorian[static_cast<std::size_t>(deg)] + n.chromaticOffset));
    }
    for (const auto& e : run.intents[0]) {
        if (e.role == MusicRole::Vettore) CHECK(expectedVettore.count(e.pitchHz) == 1);
    }
}

TEST_CASE("Hard constraints reject over-capacity candidates instead of truncating", "[music_planner][hard_constraints]") {
    const MusicClock clock = probeClock();
    const auto phrases = MusicPlanner::makePlannerProbeV1Phrases(clock);
    const PhrasePlan& p = phrases[0];
    MusicPlanner planner({.runSeed = 1u});

    const auto makeEvt = [](std::uint64_t frame, MusicRole role) {
        return CandidateEvent{.localEventId = 0, .sampleFrame = frame, .role = role, .midiNote = 60,
                              .pitchHz = 261.6f, .velocity = 0.5f, .durationFrames = 10, .timbreParam = 0.5f, .isOpenPad = false};
    };

    // More NoteOns requested than stored -> SpscCapacityExceeded (never a silently truncated candidate)
    PhraseCandidate over{};
    for (std::size_t i = 0; i < kMaxEventsPerPhrase; ++i) over.events[i] = makeEvt(1000 + i * 1000, MusicRole::Pulso);
    over.eventCount = kMaxEventsPerPhrase;
    over.requestedEventCount = kMaxEventsPerPhrase + 1;
    CHECK(planner.validateHardConstraints(over, p, 0, false) == HardConstraintViolation::SpscCapacityExceeded);

    // 64 NoteOns (+64 NoteOffs) inside one 7200-frame lookahead window: exactly 128 -> admissible
    PhraseCandidate dense{};
    for (std::size_t i = 0; i < 64; ++i) dense.events[i] = makeEvt(1000 + i * 50, static_cast<MusicRole>(i % 4));
    dense.eventCount = 64;
    dense.requestedEventCount = 64;
    CHECK(planner.validateHardConstraints(dense, p, 0, false) == HardConstraintViolation::None);
    // 65 -> 130 SPSC events in the window -> rejected
    dense.events[64] = makeEvt(1000 + 64 * 50, MusicRole::Pulso);
    dense.eventCount = 65;
    dense.requestedEventCount = 65;
    CHECK(planner.validateHardConstraints(dense, p, 0, false) == HardConstraintViolation::SpscCapacityExceeded);

    // Event before the committed horizon
    PhraseCandidate early{};
    early.events[0] = makeEvt(1000, MusicRole::Pulso);
    early.eventCount = 1;
    early.requestedEventCount = 1;
    CHECK(planner.validateHardConstraints(early, p, 2000, false) == HardConstraintViolation::CommitmentHorizonViolation);
    // Empty is only admissible for the silent level
    PhraseCandidate empty{};
    CHECK(planner.validateHardConstraints(empty, p, 0, false) == HardConstraintViolation::StructuralInvariantViolation);
    CHECK(planner.validateHardConstraints(empty, p, 0, true) == HardConstraintViolation::None);
}

TEST_CASE("Fail-closed fallback ladder L1 -> L2 -> L3 with validated levels", "[music_planner][fallback_ladder]") {
    const MusicClock clock = probeClock();
    const auto phrases = MusicPlanner::makePlannerProbeV1Phrases(clock);
    const PhrasePlan& pA = phrases[0];

    const auto planOnce = [&](const PhrasePlan& plan, std::uint64_t horizon, std::vector<MusicIntentEvent>& out) {
        MusicPlanner planner({.runSeed = 1337ULL});
        planner.setFormPlan(MusicPlanner::makePlannerProbeV1Form());
        planner.planPhrase(plan, clock, horizon, 1, out, nullptr);
        REQUIRE(planner.totalFallbacksTriggered() == 1);
        return planner.decisionTraces().back();
    };

    // L1 RarefiedMotif: horizon one 16th after bar 2 -> every regular candidate violates the horizon
    const std::uint64_t hMid = clock.frameAtBeatStep(8, 1, 4);
    std::vector<MusicIntentEvent> o1;
    const auto t1 = planOnce(pA, hMid, o1);
    CHECK(t1.candidatesValid == 0);
    CHECK(t1.fallbackKind == FallbackKind::RarefiedMotif);
    CHECK(t1.fallbackLevelViolations[0] == HardConstraintViolation::None);
    CHECK(t1.fallbackLevelsAttempted == 1);
    REQUIRE(!o1.empty());
    for (const auto& e : o1) {
        CHECK(e.sampleFrame >= hMid);
        CHECK((e.role == MusicRole::Pulso || e.role == MusicRole::Vettore));
    }
    // Bar 2 anchor at step 8 (240000) + bar 3 downbeat and anchors at steps 0 and 8 (288000 x2, 336000) -> 4 events
    CHECK(o1.size() == 4);

    // L2 PadSustainOnly: horizon after the last anchor of bar 3 -> L1 empty
    const std::uint64_t hLate = clock.frameAtBeatStep(12, 9, 4);
    std::vector<MusicIntentEvent> o2;
    const auto t2 = planOnce(pA, hLate, o2);
    CHECK(t2.fallbackKind == FallbackKind::PadSustainOnly);
    CHECK(t2.fallbackLevelViolations[0] == HardConstraintViolation::StructuralInvariantViolation);
    CHECK(t2.fallbackLevelsAttempted == 2);
    REQUIRE(o2.size() == 2);
    for (const auto& e : o2) {
        CHECK(e.role == MusicRole::Corpo);
        CHECK(e.sampleFrame == hLate);
        CHECK(e.sampleFrame + e.durationFrames == pA.endFrame);
    }

    // L3 SilentRecovery: horizon at the phrase end
    std::vector<MusicIntentEvent> o3;
    const auto t3 = planOnce(pA, pA.endFrame, o3);
    CHECK(t3.fallbackKind == FallbackKind::SilentRecovery);
    CHECK(o3.empty());
    CHECK(t3.finalEventCount == 0);

    // L3 on an invalid plan: endFrame inconsistent with clock.frameAtBar(startBar + barCount)
    PhrasePlan bad = pA;
    bad.endFrame = 192000; // old unit-test value, wrong for a 4-bar phrase at 120 BPM
    std::vector<MusicIntentEvent> o4;
    const auto t4 = planOnce(bad, 0, o4);
    CHECK(t4.planViolation == HardConstraintViolation::StructuralInvariantViolation);
    CHECK(t4.candidatesGenerated == 0);
    CHECK(t4.fallbackKind == FallbackKind::SilentRecovery);
    CHECK(o4.empty());

    // Planning clock sample rate must match the planner configuration
    MusicPlanner wrongRate({.runSeed = 1337ULL, .sampleRate = 44100});
    std::vector<MusicIntentEvent> o5;
    wrongRate.planPhrase(pA, clock, 0, 1, o5, nullptr);
    CHECK(wrongRate.decisionTraces().back().fallbackKind == FallbackKind::SilentRecovery);
    CHECK(o5.empty());
}

TEST_CASE("Golden regression: canonical event list + decision traces of the A->B probe form", "[music_planner][golden]") {
    // REGRESSION GOLDEN, NOT AN ORACLE. The constant below was produced by this implementation
    // (planner policy DEFAULT_POLICY_VERSION_ID, kPlannerScoringProfileV1, seed 1337, probe form A->B built
    // from MusicClock) by hashing the canonical little-endian serialization above with FNV-1a 64 (floats by
    // IEEE-754 bit pattern). It may only change together with a reviewed policy-version / scoring-profile bump.
    // Pitches come from std::pow, so the value is only guaranteed for the pinned MSVC toolchain/CRT (ADR-0001).
    // Correctness is established by the independent structural oracles in the other test cases.
    // Revision history: 0x8621158d2d35ecf8 (Inc-3 remediation) -> 0x2983fc5a5463d4cb (DIF-M2-R6-01: tick now
    // uses the normative M1 ceil mapping; sampleFrame, pitch and all decisions unchanged).
    constexpr std::uint64_t kGoldenHash = 0x2983fc5a5463d4cbULL;
    const PlannedRun run = planProbeForm(1337ULL);
    const std::uint64_t h = canonicalHash(run);
    INFO("canonical hash = 0x" << std::hex << h);
    CHECK(h == kGoldenHash);
    CHECK(canonicalHash(planProbeForm(1337ULL)) == h);
}

TEST_CASE("Offline CACP planner_probe_v1 validation", "[music_planner][fallback_and_cacp_non_silent]") {
    const auto res = runPlannerProbeV1Offline("artifacts/cacp", false);
    INFO(res.failureReason);
    REQUIRE(res.passed);
    CHECK(res.determinismPassed);
    CHECK(res.streamIsolationPassed);
    CHECK(res.motifInvariancePassed);
    CHECK(res.triadBudgetPassed);
    CHECK(res.noteOffPairingPassed);
    CHECK(res.fallbackPassed);
    CHECK(res.schedulingAccountingPassed);
    CHECK(res.tempoAckPassed);
    CHECK(res.tempoAckFrame == 384000);
    CHECK(res.tempoAckEpoch == 2);
    CHECK(res.droppedScheduledEvents == 0);
    CHECK(res.deferredScheduledEvents == 0);
    CHECK(res.orphanNoteRecoveredCount == 0);
    CHECK(res.committedEvents == res.expectedScheduledEvents);
    CHECK(res.stopFrame > res.contentEndFrame);
    CHECK(res.nonSilentAudioPassed);
    CHECK(res.truePeakPassed);
    CHECK(res.truePeakDbtp <= -1.0f);
    CHECK(res.clippedSamples == 0);
    CHECK(res.dspAnomaliesPassed);
    CHECK(res.droppedFrames == 0);
    CHECK(res.totalCapturedFrames == res.requestedSampleFrames);
}

TEST_CASE("MusicPlanner: checkpoint and rollback restores bit-exact state (INC4-R01)", "[music_planner][checkpoint_rollback]") {
    const MusicClock clock = probeClock();
    const auto phrases = MusicPlanner::makePlannerProbeV1Phrases(clock);

    MusicPlanner planner({.runSeed = 1337ULL, .candidateCount = 6, .sampleRate = 48000});
    planner.setFormPlan(MusicPlanner::makePlannerProbeV1Form());

    std::vector<MusicIntentEvent> outA;
    planner.planPhrase(phrases[0], clock, 0, 1, outA, nullptr);

    // Save checkpoint after phrase 0
    const auto checkpoint = planner.createCheckpoint();

    // Plan phrase 1 (Section B)
    std::vector<MusicIntentEvent> outB1;
    planner.planPhrase(phrases[1], clock, 0, 2, outB1, nullptr);
    const auto tracesB1 = planner.decisionTraces();
    const auto noteOffsB1 = planner.plannedNoteOffs();

    // Rollback to checkpoint
    planner.restoreCheckpoint(checkpoint);

    // Verify checkpoint restored trace and noteOff counts
    CHECK(planner.decisionTraces().size() == 1);
    CHECK(planner.plannedNoteOffs().empty());

    // Re-plan phrase 1
    std::vector<MusicIntentEvent> outB2;
    planner.planPhrase(phrases[1], clock, 0, 2, outB2, nullptr);
    const auto tracesB2 = planner.decisionTraces();
    const auto noteOffsB2 = planner.plannedNoteOffs();

    // Must be bit-exact identical
    CHECK(outB1 == outB2);
    CHECK(tracesB1 == tracesB2);
    CHECK(noteOffsB1 == noteOffsB2);
}

TEST_CASE("SimulatedVoiceTracker: delays release transition until programmed sample frame (INC4-R03)", "[music_planner][simulated_tracker_cross_section_timing]") {
    SimulatedVoiceTracker tracker;
    bool wasStolen = false;
    const int slotIdx = tracker.simulateNoteOn(MusicRole::Corpo, 1, 1000, 0, 220.0f, wasStolen);
    REQUIRE(slotIdx >= 0);

    // Scheduled NoteOff far into the future (frame 10,000)
    tracker.simulateNoteOff(MusicRole::Corpo, 1, 10000);

    // At frame 5,000 (before 10,000): voice must NOT be inRelease yet!
    tracker.updateAtFrame(5000);
    CHECK(tracker.activeCountForRole(MusicRole::Corpo, 5000) == 1);
    CHECK_FALSE(tracker.slots()[static_cast<std::size_t>(slotIdx)].inRelease);

    // At frame 10,000: voice transitions to inRelease
    tracker.updateAtFrame(10000);
    CHECK(tracker.slots()[static_cast<std::size_t>(slotIdx)].inRelease);
}

TEST_CASE("Fixture M2.4 V1: 24 phrases, 96 bars, and golden rational boundaries (D-M2-08)", "[music_planner][fixture_phrases_and_boundaries]") {
    const auto optClock = MusicPlanner::makeFixtureM24V1Clock();
    REQUIRE(optClock.has_value());
    const MusicClock& clock = *optClock;
    const FormPlan form = MusicPlanner::makeFixtureM24V1Form();
    const auto phrases = MusicPlanner::makeFixtureM24V1Phrases(clock);

    REQUIRE(phrases.size() == 24);
    REQUIRE(form.sectionCount == 3);

    // Section A: phrases 0..7 (bars 0..31)
    CHECK(phrases[0].startFrame == 0);
    CHECK(phrases[7].endFrame == 3072000ULL); // Bar 32

    // Section B: phrases 8..15 (bars 32..63)
    CHECK(phrases[8].startFrame == 3072000ULL);
    CHECK(phrases[15].endFrame == 5705142ULL); // Bar 64

    // Section A': phrases 16..23 (bars 64..95)
    CHECK(phrases[16].startFrame == 5705142ULL);
    CHECK(phrases[23].endFrame == 8777142ULL); // Bar 96

    // Cross-section pad lease on phrase 7 and 15
    CHECK(phrases[7].isCrossSectionPadPhrase);
    CHECK(phrases[15].isCrossSectionPadPhrase);
    CHECK_FALSE(phrases[23].isCrossSectionPadPhrase);

    // Pad note off frame is +1 beat into next section
    // Section B is 140 BPM: 1 beat = (48000 * 60) / 140 = 20571.428... frames (frame 3092571)
    CHECK(phrases[7].crossSectionNoteOffFrame == clock.frameAtBeatStep(32 * 4 + 1, 0, 4));

    // Section A' is 120 BPM: 1 beat = (48000 * 60) / 120 = 24000 frames
    // 5705142 + 24000 = 5729142 (exact frameAtBeatStep(64 * 4 + 1, 0, 4))
    CHECK(phrases[15].crossSectionNoteOffFrame == clock.frameAtBeatStep(64 * 4 + 1, 0, 4));

    // Section B candidate recipes: strictly ModalTranspose and ContourInversion (INC4-R04)
    MusicPlanner planner({.runSeed = 1337ULL, .candidateCount = 8, .sampleRate = 48000});
    planner.setFormPlan(form);

    std::vector<MusicIntentEvent> outB;
    planner.planPhrase(phrases[8], clock, 0, 2, outB, nullptr);
    const auto traceB = planner.decisionTraces().back();
    for (std::size_t i = 0; i < traceB.candidatesGenerated; ++i) {
        const auto k = traceB.candidates[i].transformKind;
        CHECK((k == MotifTransformKind::ModalTranspose || k == MotifTransformKind::ContourInversion));
    }
}

TEST_CASE("Offline CACP fixture_probe_v1 validation", "[music_planner][fixture_probe_v1]") {
    const auto res = runFixtureProbeV1Offline("artifacts/cacp", false);
    INFO(res.failureReason);
    REQUIRE(res.passed);
    CHECK(res.determinismPassed);
    CHECK(res.noteOffPairingPassed);
    CHECK(res.tempoAckPassed);
    CHECK(res.nonSilentAudioPassed);
    CHECK(res.silenceBoundaryPassed);
    CHECK(res.truePeakPassed);
    CHECK(res.truePeakDbtp <= -1.0f);
    CHECK(res.peakAmplitude <= (MasterLimiter::kHardCeilingLinear + 1e-6f));
    CHECK(res.clippedSamples == 0);
    CHECK(res.dspAnomaliesPassed);
    CHECK(res.droppedFrames == 0);
    CHECK(res.droppedScheduledEvents == 0);
    CHECK(res.deferredScheduledEvents == 0);
    CHECK(res.orphanNoteRecoveredCount == 0);
    CHECK(res.totalCapturedFrames == res.requestedSampleFrames);
}

