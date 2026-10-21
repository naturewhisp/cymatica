#pragma once

#include "audio_scheduler.h"
#include "euclidean_rhythm.h"
#include "fixed_step.h"
#include "music_clock.h"
#include "music_intent.h"
#include "music_position.h"
#include "random_key.h"
#include "stable_random.h"
#include "version_ids.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cymatica::audio {

// --- 1. Modal Scales & Tuning (ADR-0003 D-M2-05, INC3-R06) ---

enum class ScaleMode : std::uint8_t {
    Ionian      = 0,
    Dorian      = 1,
    Phrygian    = 2,
    Lydian      = 3,
    Mixolydian  = 4,
    Aeolian     = 5,
    Locrian     = 6
};

struct ModalScale {
    std::int32_t rootMidiNote{62}; // 62 = D4 (D Dorian baseline), 55 = G3 (G Aeolian baseline)
    ScaleMode mode{ScaleMode::Dorian};

    [[nodiscard]] constexpr bool operator==(const ModalScale& other) const noexcept = default;
};

// Returns interval in semitones [0..11] for a mode and degree in [0..6]
[[nodiscard]] constexpr std::int8_t modeInterval(ScaleMode mode, std::uint8_t degree) noexcept {
    constexpr std::array<std::array<std::int8_t, 7>, 7> kIntervals{{
        /* Ionian     */ {0, 2, 4, 5, 7, 9, 11},
        /* Dorian     */ {0, 2, 3, 5, 7, 9, 10},
        /* Phrygian   */ {0, 1, 3, 5, 7, 8, 10},
        /* Lydian     */ {0, 2, 4, 6, 7, 9, 11},
        /* Mixolydian */ {0, 2, 4, 5, 7, 9, 10},
        /* Aeolian    */ {0, 2, 3, 5, 7, 8, 10},
        /* Locrian    */ {0, 1, 3, 5, 6, 8, 10}
    }};
    const auto mIdx = static_cast<std::size_t>(mode) % 7;
    const auto dIdx = static_cast<std::size_t>(degree) % 7;
    return kIntervals[mIdx][dIdx];
}

// Converts scale degree and optional chromatic offset into physical Hz pitch
[[nodiscard]] float degreeToPitchHz(const ModalScale& scale, std::int32_t scaleDegree, std::int8_t chromaticOffset = 0) noexcept;

// Converts scale degree to MIDI note number with floored octave division
[[nodiscard]] std::int32_t degreeToMidiNote(const ModalScale& scale, std::int32_t scaleDegree, std::int8_t chromaticOffset = 0) noexcept;

// Equal-tempered MIDI -> Hz (A4 = 69 = 440 Hz), clamped to the planner's legal pitch range [20, 16000] Hz
[[nodiscard]] float midiNoteToPitchHz(std::int32_t midiNote) noexcept;


// --- 2. Symbolic Motif & Pure Transformations (Spec §16.4, ADR-0003 D-M2-05, INC3-R06) ---

inline constexpr std::size_t kMaxMotifNotes = 16;

struct MotifNote {
    std::int16_t scaleDegree{0};       // Degree relative to modal tonic
    std::int8_t chromaticOffset{0};    // Semitone offset (-1, 0, +1)
    std::uint8_t stepOffset{0};        // Subdivision onset [0..31]
    std::uint8_t durationSteps{1};     // Length in subdivisions [1..32]
    float velocity{0.8f};              // Normalized velocity [0.0f..1.0f]
    bool isAnchor{false};              // Harmonic anchor note (protected against chromatic alterations)

    [[nodiscard]] constexpr bool operator==(const MotifNote& other) const noexcept = default;
};

struct Motif {
    std::uint32_t motifId{0};
    std::uint32_t parentMotifId{0};
    MusicRole targetRole{MusicRole::Vettore};
    std::int8_t registerOctaveOffset{0};
    std::uint8_t lengthSteps{16};      // e.g. 16 subdivisions = 1 bar in 4/4
    std::uint8_t noteCount{0};
    std::array<MotifNote, kMaxMotifNotes> notes{};

    [[nodiscard]] constexpr bool operator==(const Motif& other) const noexcept = default;
};

enum class MotifTransformKind : std::uint8_t {
    Identity                = 0,
    ModalTranspose          = 1,
    OctaveShift             = 2,
    Reorchestrate           = 3,
    ContourInversion        = 4,
    EuclideanRhythmVariation = 5,
    ChromaticAlteration     = 6
};

inline constexpr std::size_t kMotifTransformKindCount = 7;

// Pure transformation functions preserving contour invariants
[[nodiscard]] Motif applyModalTranspose(const Motif& in, std::int16_t degreeDelta) noexcept;
[[nodiscard]] Motif applyContourInversion(const Motif& in, std::int16_t pivotDegree) noexcept;
[[nodiscard]] Motif applyEuclideanRhythmVariation(const Motif& in, std::uint32_t pulses, std::int32_t rotation) noexcept;
[[nodiscard]] Motif applyReorchestration(const Motif& in, MusicRole newRole, std::int8_t octaveDelta) noexcept;
[[nodiscard]] Motif applyChromaticAlteration(const Motif& in, std::uint8_t noteIdx, std::int8_t semitoneDelta) noexcept;

// Distinct, stable derived identifier for a transformed motif (Identity keeps the base id).
[[nodiscard]] std::uint32_t deriveTransformedMotifId(std::uint32_t baseMotifId,
                                                     MotifTransformKind kind,
                                                     std::uint32_t variantIndex) noexcept;

// Mathematical invariant verifiers
[[nodiscard]] bool verifyContourPreserved(const Motif& original, const Motif& transposed) noexcept;
[[nodiscard]] bool verifyInversionSymmetric(const Motif& original, const Motif& inverted, std::int16_t pivot) noexcept;
// Verifies the identity-preserving invariants of `transformed` relative to `base` for the given transform
// kind and parameter (the same parameter recorded in the decision trace). Pure; usable as an oracle.
[[nodiscard]] bool verifyMotifTransformInvariants(const Motif& base, const Motif& transformed,
                                                  MotifTransformKind kind, std::int16_t transformParam) noexcept;


// --- 3. Harmony, Triad Voicing & Open-Pad Leases (ADR-0003 D-M2-04/05, INC3-R02) ---

enum class HarmonicFunction : std::uint8_t {
    Stability   = 0,
    Preparation = 1,
    Tension     = 2,
    Resolution  = 3
};

enum class CadenceGoal : std::uint8_t {
    Sustain         = 0,
    HalfCadence     = 1,
    ModalResolution = 2,
    ContrastShift   = 3
};

struct ChordSpec {
    std::int16_t rootDegree{0};
    std::int16_t thirdDegree{2};
    std::int16_t fifthDegree{4};
    HarmonicFunction function{HarmonicFunction::Stability};
    bool openPadSustain{false}; // chords[0].openPadSustain: Corpo plays one open pad (durationFrames == 0) per phrase
};

// Orchestrates a triad across the 2-voice Corpo budget + Trama/Vettore (INC3-R02)
struct TriadVoicing {
    std::int16_t corpoBassDegree{0};    // Corpo voice 0: bass root
    std::int16_t corpoHarmonyDegree{4}; // Corpo voice 1: fifth or third
    std::int16_t upperColorDegree{2};   // Assigned to Trama (arpeggio) or Vettore (anchor)
    bool isThirdInCorpo{false};

    [[nodiscard]] constexpr bool operator==(const TriadVoicing& other) const noexcept = default;
};

// prevCorpoHarmonyDegree: Corpo harmony degree of the previous chord onset, if any (voice leading reference).
[[nodiscard]] TriadVoicing voiceTriad(const ChordSpec& chord,
                                      std::optional<std::int16_t> prevCorpoHarmonyDegree = std::nullopt) noexcept;


// --- 4. Hierarchical Plan: FormPlan -> SectionPlan -> PhrasePlan ---

inline constexpr std::uint32_t kMaxPhraseBars = 4;     // One ChordSpec per bar (chords[b % 4])
inline constexpr std::uint32_t kMaxStepsPerBar = 32;   // Bounded by core::kMaxEuclideanSteps

struct PhrasePlan {
    std::uint32_t sectionIndex{0};
    std::uint32_t phraseIndex{0};
    std::uint32_t startBar{0};
    std::uint32_t barCount{4};
    std::uint64_t startFrame{0};       // Must equal clock.frameAtBar(startBar)
    std::uint64_t endFrame{0};         // Must equal clock.frameAtBar(startBar + barCount)
    ModalScale scale{};
    std::array<ChordSpec, 4> chords{};
    float targetTension{0.0f}; // [0.0f, 1.0f]; quantized once to integer milli-units at planning entry
    MusicRole dominantRole{MusicRole::Vettore};
    CadenceGoal cadenceGoal{CadenceGoal::Sustain};
    std::uint32_t baseMotifId{0};
    bool isCrossSectionPadPhrase{false};
    std::uint64_t crossSectionNoteOffFrame{0};
    bool delayCorpoOnsetByOneBeat{false};
};

struct SectionPlan {
    std::uint32_t sectionIndex{0};
    std::uint32_t startBar{0};
    std::uint32_t barCount{32};
    TempoMap tempoMap{};
    ModalScale scale{};
    float baseTension{0.2f};
    Motif sectionMotif{};
    bool sustainPadIntoNextSection{false};
};

inline constexpr std::size_t kMaxSections = 8;

struct FormPlan {
    std::size_t sectionCount{0};
    std::array<SectionPlan, kMaxSections> sections{};
};


// --- 5. Candidate Generation, Validation, Integer Scoring & Tie-Break (INC3-R01..R05) ---

inline constexpr std::size_t kMinCandidates = 4;
inline constexpr std::size_t kMaxCandidates = 8;
// Max NoteOn events per phrase candidate. Each NoteOn produces exactly one NoteOff (scheduled from its
// duration or an explicit planner NoteOff for open pads), so a phrase commits at most 2x this many SPSC events.
inline constexpr std::size_t kMaxEventsPerPhrase = 128;
// Hard constraint: SPSC events (NoteOn + NoteOff) whose frame lies within any lookahead window
// [t, t + lookaheadFrames) of one candidate. Half the 256-slot engine SPSC (ADR-0003 D-M2-03).
inline constexpr std::size_t kMaxSpscEventsPerLookaheadWindow = 128;

enum class HardConstraintViolation : std::uint8_t {
    None                            = 0,
    InvalidParameters               = 1,
    SimultaneousOnsetLimitExceeded  = 2, // >= 3 NoteOns of one role at exact same sampleFrame
    CommitmentHorizonViolation      = 3, // event before committed horizon or outside phrase
    SpscCapacityExceeded            = 4, // > kMaxEventsPerPhrase NoteOns, or > 128 SPSC events in a lookahead window
    StructuralInvariantViolation    = 5, // phrase plan inconsistent with MusicClock, unsupported grid, empty level
};

struct CandidateEvent {
    std::uint32_t localEventId{0};     // Canonical index inside the candidate (after canonical sort)
    std::uint64_t sampleFrame{0};
    MusicRole role{MusicRole::Pulso};
    std::int16_t midiNote{60};         // Integer pitch (canonical sort key and oracle reference)
    float pitchHz{220.0f};             // midiNoteToPitchHz(midiNote)
    float velocity{1.0f};
    std::uint32_t durationFrames{0};   // 0 = open pad (explicit planner NoteOff at phrase end)
    float timbreParam{0.0f};
    bool isOpenPad{false};
};

// Integer scoring features (all values deterministic integers, no float in the score path)
struct CandidateFeatures {
    std::int32_t voiceLeadingDistance{0};   // Sum of |scale-degree| moves of the Corpo voices across chord onsets
    std::int32_t tensionEstimateMilli{0};   // Estimated phrase tension in milli-units [0..1000]
    std::int32_t anchorFidelityMilli{0};    // Preserved motif anchors / total anchors in milli-units
    std::int32_t onsetDensityMilli{0};      // Pulso+Trama+Vettore NoteOns per bar in milli-onsets
    std::uint32_t simulatedSteals{0};       // Rule-2 steals of held (non-released) voices

    [[nodiscard]] constexpr bool operator==(const CandidateFeatures& other) const noexcept = default;
};

enum class CorpoVoicingPolicy : std::uint8_t {
    VoiceLed   = 0, // voiceTriad with voice leading from the previous chord onset
    RootFifth  = 1  // voiceTriad without reference (root + fifth in Corpo)
};

class SimulatedVoiceTracker;

struct PhraseCandidate {
    std::uint64_t candidateId{0};      // (section << 40) | (phrase << 8) | candidateIndex; no overflow for phrase < 2^32
    std::uint8_t candidateIndex{0};
    MotifTransformKind transformKind{MotifTransformKind::Identity};
    std::int16_t transformParam{0};
    Motif transformedMotif{};
    CorpoVoicingPolicy voicingPolicy{CorpoVoicingPolicy::VoiceLed};
    TriadVoicing voicing{};            // Voicing of the first chord onset
    std::optional<TriadVoicing> lastCorpoVoicing{}; // Voicing of the last Corpo onset (voice-leading state)
    std::uint32_t pulsoPulses{4};
    std::int32_t pulsoRotation{0};
    std::uint32_t tramaPulses{6};
    std::int32_t tramaRotation{2};
    std::uint32_t tramaSustainSteps{1};
    std::size_t requestedEventCount{0}; // NoteOns the generator wanted to emit (never silently truncated)
    std::size_t eventCount{0};          // NoteOns stored in `events` (== requested when <= capacity)
    std::array<CandidateEvent, kMaxEventsPerPhrase> events{};
    HardConstraintViolation violation{HardConstraintViolation::None};
    CandidateFeatures features{};
    std::int32_t scoreFixedPoint{0};    // 0..100000 deterministic integer score
    std::uint64_t tieBreakHash{0};
    std::uint32_t simulatedStealsCount{0};
};

enum class FallbackKind : std::uint8_t {
    None            = 0,
    RarefiedMotif   = 1, // L1: Vettore motif anchors + Pulso downbeats (from the committed horizon)
    PadSustainOnly  = 2, // L2: Corpo root + fifth sustained to the phrase end
    SilentRecovery  = 3  // L3: zero new events emitted (INC3-R03)
};

inline constexpr std::size_t kFallbackLevelCount = 3;

struct CandidateTraceEntry {
    std::uint64_t candidateId{0};
    MotifTransformKind transformKind{MotifTransformKind::Identity};
    std::int16_t transformParam{0};
    std::uint32_t motifId{0};
    HardConstraintViolation violation{HardConstraintViolation::None};
    std::int32_t score{0};
    std::uint64_t tieBreakHash{0};
    std::uint32_t simulatedSteals{0};
    std::uint32_t eventCount{0};
    CandidateFeatures features{};

    [[nodiscard]] constexpr bool operator==(const CandidateTraceEntry& other) const noexcept = default;
};

// Replayable decision record (Spec §15.5, ADR-0002): together with the PhrasePlan and MusicClock it
// identifies every candidate considered, why it was rejected or scored, and which one was committed.
struct PhraseDecisionTrace {
    std::uint64_t decisionId{0};          // deriveDecisionId(section, phrase, startBar, kPhraseDecisionSlot, policy)
    std::uint64_t tieBreakDecisionId{0};  // deriveDecisionId(..., kTieBreakDecisionSlot, policy)
    std::uint64_t runSeed{0};
    std::uint32_t policyVersion{0};
    std::uint32_t scoringProfileVersion{0};
    std::uint32_t sectionIndex{0};
    std::uint32_t phraseIndex{0};
    std::uint32_t startBar{0};
    std::int32_t targetTensionMilli{0};
    HardConstraintViolation planViolation{HardConstraintViolation::None};
    std::uint32_t candidatesGenerated{0};
    std::uint32_t candidatesValid{0};
    std::array<CandidateTraceEntry, kMaxCandidates> candidates{};
    std::uint64_t winningCandidateId{0};
    std::int32_t winningScore{0};
    MotifTransformKind winningTransformKind{MotifTransformKind::Identity};
    std::int16_t winningTransformParam{0};
    std::uint32_t baseMotifId{0};
    Motif baseMotif{};
    Motif winningMotif{};
    std::uint32_t tiedTopCandidates{0};   // Valid candidates sharing the top score (>1 => tie-break decided)
    bool fallbackTriggered{false};
    FallbackKind fallbackKind{FallbackKind::None};
    std::array<HardConstraintViolation, kFallbackLevelCount> fallbackLevelViolations{};
    std::uint8_t fallbackLevelsAttempted{0}; // Entries of fallbackLevelViolations beyond this count were not tried
    std::uint64_t firstEventId{0};
    std::size_t finalEventCount{0};
    std::uint32_t plannerNoteOffCount{0};

    [[nodiscard]] constexpr bool operator==(const PhraseDecisionTrace& other) const noexcept = default;
};

// Explicit planner NoteOff scheduled for an open pad (ADR-0003 D-M2-04 §4)
struct PlannedNoteOff {
    std::uint64_t eventId{0};
    MusicRole role{MusicRole::Corpo};
    std::uint64_t sampleFrame{0};
    std::uint64_t transportEpoch{0};

    [[nodiscard]] constexpr bool operator==(const PlannedNoteOff& other) const noexcept = default;
};

// Simulated voice state for cross-phrase tracking (INC3-R02)
struct SimulatedVoiceSlot {
    bool active{false};               // Sounding (held or in release tail)
    MusicRole role{MusicRole::Pulso};
    std::uint64_t eventId{0};
    std::uint64_t startFrame{0};
    std::uint64_t noteOffFrame{0};    // Nominal NoteOff (start + duration); 0 = open pad (explicit NoteOff)
    std::uint64_t expireFrame{0};     // End of the release tail (valid while inRelease)
    float pitchHz{220.0f};
    bool inRelease{false};
};

class SimulatedVoiceTracker {
public:
    static constexpr std::size_t kTotalSlots = 8;
    static constexpr std::size_t kSlotsPerRole = 2;
    static constexpr std::uint64_t kReleaseTailFrames = 96; // 2 ms de-click tail @ 48 kHz (ADR-0003 D-M2-04)

    SimulatedVoiceTracker() noexcept = default;

    // Advances nominal NoteOffs (held -> release) and release tails (release -> idle) up to `frame`.
    void updateAtFrame(std::uint64_t frame) noexcept;
    // Number of held (not released) voices of `role` at `frame`.
    [[nodiscard]] std::uint32_t activeCountForRole(MusicRole role, std::uint64_t frame) const noexcept;
    // Simulates voice allocation/stealing according to Rules 0-2 (ADR-0003 D-M2-04)
    // Returns slot index [0..7], sets wasStolen to true if any steal (Rule 1 or Rule 2) occurred.
    int simulateNoteOn(MusicRole role, std::uint64_t eventId, std::uint64_t sampleFrame,
                       std::uint32_t durationFrames, float pitchHz, bool& wasStolen) noexcept;
    void simulateNoteOff(MusicRole role, std::uint64_t eventId, std::uint64_t sampleFrame) noexcept;
    void reset() noexcept;

    [[nodiscard]] const std::array<SimulatedVoiceSlot, kTotalSlots>& slots() const noexcept { return slots_; }
    // Monotonic count of Rule-2 steals (held voice, not in release) simulated so far.
    [[nodiscard]] std::uint32_t heldVoiceSteals() const noexcept { return heldVoiceSteals_; }
    [[nodiscard]] std::uint64_t lastUpdatedFrame() const noexcept { return lastUpdatedFrame_; }

private:
    std::array<SimulatedVoiceSlot, kTotalSlots> slots_{};
    std::uint32_t heldVoiceSteals_{0};
    std::uint64_t lastUpdatedFrame_{0};
};


// --- 6. Versioned Scoring Profile (experimental initial profile, NOT validated values) ---

// All weights and feature constants of the deterministic integer score live here. Changing any value
// requires bumping `versionId` (the version is recorded in every decision trace and in the golden test).
struct PlannerScoringProfile {
    std::uint32_t versionId{0};
    const char* name{""};
    // Weights of the four 0..1000 sub-scores (sum 100 -> max total 100000) and the steal penalty
    std::int32_t weightVoiceLeading{0};
    std::int32_t weightTension{0};
    std::int32_t weightMotif{0};
    std::int32_t weightRhythm{0};
    std::int32_t penaltyPerSteal{0};                 // Total-score points per Rule-2 steal
    // Voice leading: sub-score points lost per scale degree moved
    std::int32_t voiceLeadingPenaltyPerDegree{0};
    // Tension estimate components (milli-units)
    std::array<std::int32_t, kMotifTransformKindCount> transformTensionMilli{};
    std::uint32_t densityBaselinePulses{0};          // Pulso + Trama pulses per bar with zero density tension
    std::int32_t tensionPerExtraPulseMilli{0};
    std::int32_t tensionPerRegisterDegreeMilli{0};   // per mean Vettore scale degree above the tonic
    std::array<std::int32_t, 4> harmonicFunctionTensionMilli{}; // Stability, Preparation, Tension, Resolution
    // Rhythmic density balance
    std::int32_t targetOnsetsPerBarMilli{0};
    std::int32_t rhythmPenaltyPerOnset{0};           // sub-score points lost per onset/bar of deviation
};

inline constexpr PlannerScoringProfile kPlannerScoringProfileV1{
    .versionId = 1,
    .name = "planner-scoring-v1-experimental",
    .weightVoiceLeading = 25,
    .weightTension = 25,
    .weightMotif = 25,
    .weightRhythm = 25,
    .penaltyPerSteal = 2500,
    .voiceLeadingPenaltyPerDegree = 40,
    //                      Identity, Transpose, Octave, Reorch, Inversion, Euclid, Chromatic
    .transformTensionMilli = {0,      150,       120,    100,    250,       200,    300},
    .densityBaselinePulses = 10,
    .tensionPerExtraPulseMilli = 50,
    .tensionPerRegisterDegreeMilli = 15,
    .harmonicFunctionTensionMilli = {0, 150, 350, 50},
    .targetOnsetsPerBarMilli = 13000,
    .rhythmPenaltyPerOnset = 100,
};


// --- 7. MusicPlanner Class (Coordinator Thread Only) ---

// Decision slots of StreamId::MusicForm (deriveDecisionId decisionSlot argument), one per draw kind.
inline constexpr std::uint32_t kPhraseDecisionSlot = 0;   // Phrase decision identity (trace key, no draw)
inline constexpr std::uint32_t kTieBreakDecisionSlot = 1; // Tie-break hash among equal-score candidates

struct MusicPlannerConfig {
    std::uint64_t runSeed{1337ULL};
    core::PolicyVersionId policyVersion{core::DEFAULT_POLICY_VERSION_ID};
    std::uint32_t candidateCount{6}; // Default 6, allowed [4, 8]
    std::uint32_t sampleRate{48000}; // Must equal the planning MusicClock internal rate
    std::uint32_t simulationHz{core::FixedStepConfig{}.simulationHz}; // Fixed-step tick rate (MusicIntentEvent::tick)
    std::uint64_t lookaheadFrames{AudioSchedulerConfig{}.lookaheadFrames}; // Window of the SPSC density constraint
};

class MusicPlanner {
public:
    explicit MusicPlanner(const MusicPlannerConfig& config = {}) noexcept;

    // Plan composition
    void setFormPlan(const FormPlan& form) noexcept;
    [[nodiscard]] const FormPlan& formPlan() const noexcept { return formPlan_; }

    // Generates events for a phrase plan given the Coordinator planning clock (tempo map(s) already set)
    // and the committed horizon. Every onset/duration is derived from clock.frameAtBeatStep()/frameAtBar().
    // Emits planned MusicIntentEvent items (canonical order, sequential eventIds) and, when a scheduler is
    // given, stages them plus the explicit open-pad NoteOffs. The simulated voice state is advanced
    // identically whether or not a scheduler is passed.
    void planPhrase(const PhrasePlan& phrase,
                    const MusicClock& clock,
                    std::uint64_t committedHorizonFrame,
                    std::uint64_t currentTransportEpoch,
                    std::vector<MusicIntentEvent>& outIntents,
                    AudioScheduler* scheduler = nullptr);

    // Builds a PhrasePlan of `section` whose bar/frame boundaries are derived from `clock`.
    [[nodiscard]] static PhrasePlan makePhrasePlan(const MusicClock& clock,
                                                   const SectionPlan& section,
                                                   std::uint32_t phraseIndex,
                                                   const std::array<ChordSpec, 4>& chords,
                                                   CadenceGoal cadenceGoal) noexcept;

    // Factory profiles
    [[nodiscard]] static FormPlan makeFixtureM24V1Form() noexcept;   // 96 bars A-B-A' (~182.86s)
    [[nodiscard]] static std::optional<MusicClock> makeFixtureM24V1Clock() noexcept;
    [[nodiscard]] static std::array<PhrasePlan, 24> makeFixtureM24V1Phrases(const MusicClock& clock) noexcept;
    [[nodiscard]] static FormPlan makePlannerProbeV1Form() noexcept; // 8 bars A-B test (~14.86s)
    // Planning clock of planner_probe_v1: section A tempo map, section B tempo map at frameAtBar(4).
    [[nodiscard]] static std::optional<MusicClock> makePlannerProbeV1Clock() noexcept;
    // The two phrases of planner_probe_v1 (A: per-bar chords, B: open pad), built from `clock`.
    [[nodiscard]] static std::array<PhrasePlan, 2> makePlannerProbeV1Phrases(const MusicClock& clock) noexcept;
    [[nodiscard]] static Motif makeDorianBaseMotif() noexcept;
    [[nodiscard]] static Motif makeAeolianBaseMotif() noexcept;
    [[nodiscard]] static Motif resolveBaseMotif(std::uint32_t baseMotifId, const ModalScale& scale) noexcept;

    // Accessors & Telemetry
    [[nodiscard]] const MusicPlannerConfig& config() const noexcept { return config_; }
    [[nodiscard]] std::uint64_t nextEventId() const noexcept { return nextEventId_; }
    [[nodiscard]] std::size_t decisionTraceCount() const noexcept { return traces_.size(); }
    [[nodiscard]] const std::vector<PhraseDecisionTrace>& decisionTraces() const noexcept { return traces_; }
    [[nodiscard]] const std::vector<PlannedNoteOff>& plannedNoteOffs() const noexcept { return plannedNoteOffs_; }
    [[nodiscard]] std::uint32_t totalFallbacksTriggered() const noexcept { return totalFallbacks_; }
    [[nodiscard]] const SimulatedVoiceTracker& voiceTracker() const noexcept { return voiceTracker_; }

    void reset(std::uint64_t newSeed = 0) noexcept;

    // Checkpoint & rollback API for mutable planning horizon (ADR-0005 D-M2-09, INC4-R01)
    struct PlannerCheckpoint {
        SimulatedVoiceTracker voiceTracker{};
        std::optional<TriadVoicing> lastCorpoVoicing{};
        std::uint64_t nextEventId{1};
        std::uint32_t totalFallbacks{0};
        std::size_t traceCount{0};
        std::size_t plannedNoteOffCount{0};
    };
    [[nodiscard]] PlannerCheckpoint createCheckpoint() const noexcept;
    void restoreCheckpoint(const PlannerCheckpoint& cp) noexcept;

    // Candidate selection logic exposed for testing (INC3-R05). `tieBreakDecisionId` is the
    // kTieBreakDecisionSlot decision id; hashes use config().runSeed and config().policyVersion.
    [[nodiscard]] PhraseCandidate selectWinningCandidate(
        std::vector<PhraseCandidate>& candidates,
        std::uint64_t tieBreakDecisionId
    ) const noexcept;

    // Hard-constraint validation exposed for testing (ADR-0003 D-M2-05 §5). `candidate.events` must be in
    // canonical order. allowEmpty admits zero events (L3 SilentRecovery only).
    [[nodiscard]] HardConstraintViolation validateHardConstraints(
        const PhraseCandidate& candidate,
        const PhrasePlan& phrase,
        std::uint64_t committedHorizonFrame,
        bool allowEmpty
    ) const noexcept;

    // Tie-break hash of one candidate (StreamId::MusicForm, explicit policy version).
    [[nodiscard]] std::uint64_t tieBreakHash(std::uint64_t tieBreakDecisionId, std::uint64_t candidateId) const noexcept;

private:
    struct PhraseGrid;

    MusicPlannerConfig config_{};
    FormPlan formPlan_{};
    SimulatedVoiceTracker voiceTracker_{};
    std::optional<TriadVoicing> lastCorpoVoicing_{};
    std::uint64_t nextEventId_{1};
    std::uint32_t totalFallbacks_{0};
    std::vector<PhraseDecisionTrace> traces_{};
    std::vector<PlannedNoteOff> plannedNoteOffs_{};

    void generateCandidates(const PhrasePlan& phrase,
                            const PhraseGrid& grid,
                            const Motif& baseMotif,
                            std::vector<PhraseCandidate>& outCandidates) const;

    void buildFallbackLevel(FallbackKind level,
                            const PhrasePlan& phrase,
                            const PhraseGrid& grid,
                            const Motif& baseMotif,
                            std::uint64_t committedHorizonFrame,
                            PhraseCandidate& out) const noexcept;

    // Simulates the candidate's NoteOn/NoteOff stream (time order) on `tracker`, returns Rule-2 steals.
    // Provisional event ids are firstEventId + canonical index (equal to the final ids if committed).
    [[nodiscard]] std::uint32_t simulateCandidate(const PhraseCandidate& candidate,
                                                  const PhrasePlan& phrase,
                                                  std::uint64_t firstEventId,
                                                  SimulatedVoiceTracker& tracker) const noexcept;

    [[nodiscard]] std::int32_t scoreCandidateFixedPoint(
        const PhraseCandidate& candidate,
        std::int32_t targetTensionMilli
    ) const noexcept;
};

} // namespace cymatica::audio
