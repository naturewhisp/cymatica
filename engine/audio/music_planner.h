#pragma once

#include "audio_scheduler.h"
#include "euclidean_rhythm.h"
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

// Pure transformation functions preserving contour invariants
[[nodiscard]] Motif applyModalTranspose(const Motif& in, std::int16_t degreeDelta) noexcept;
[[nodiscard]] Motif applyContourInversion(const Motif& in, std::int16_t pivotDegree) noexcept;
[[nodiscard]] Motif applyEuclideanRhythmVariation(const Motif& in, std::uint32_t pulses, std::int32_t rotation) noexcept;
[[nodiscard]] Motif applyReorchestration(const Motif& in, MusicRole newRole, std::int8_t octaveDelta) noexcept;
[[nodiscard]] Motif applyChromaticAlteration(const Motif& in, std::uint8_t noteIdx, std::int8_t semitoneDelta) noexcept;

// Mathematical invariant verifiers
[[nodiscard]] bool verifyContourPreserved(const Motif& original, const Motif& transposed) noexcept;
[[nodiscard]] bool verifyInversionSymmetric(const Motif& original, const Motif& inverted, std::int16_t pivot) noexcept;


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
    bool openPadSustain{false}; // If true, Corpo plays open pad (durationFrames == 0)
};

// Orchestrates a triad across the 2-voice Corpo budget + Trama/Vettore (INC3-R02)
struct TriadVoicing {
    std::int16_t corpoBassDegree{0};    // Corpo voice 0: bass root
    std::int16_t corpoHarmonyDegree{4}; // Corpo voice 1: fifth or third
    std::int16_t upperColorDegree{2};   // Assigned to Trama (arpeggio) or Vettore (anchor)
    bool isThirdInCorpo{false};
};

[[nodiscard]] TriadVoicing voiceTriad(const ChordSpec& chord, std::int16_t prevCorpoHarmonyDegree = -1) noexcept;


// --- 4. Hierarchical Plan: FormPlan -> SectionPlan -> PhrasePlan ---

struct PhrasePlan {
    std::uint32_t sectionIndex{0};
    std::uint32_t phraseIndex{0};
    std::uint32_t startBar{0};
    std::uint32_t barCount{4};
    std::uint64_t startFrame{0};
    std::uint64_t endFrame{0};
    ModalScale scale{};
    std::array<ChordSpec, 4> chords{};
    float targetTension{0.0f}; // [0.0f, 1.0f]
    MusicRole dominantRole{MusicRole::Vettore};
    CadenceGoal cadenceGoal{CadenceGoal::Sustain};
    std::uint32_t baseMotifId{0};
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


// --- 5. Candidate Generation, Validation, Fixed-Point Scoring & Tie-Break (INC3-R01..R05) ---

inline constexpr std::size_t kMinCandidates = 4;
inline constexpr std::size_t kMaxCandidates = 8;
inline constexpr std::size_t kMaxEventsPerPhrase = 64;

enum class HardConstraintViolation : std::uint8_t {
    None                            = 0,
    InvalidParameters               = 1,
    SimultaneousOnsetLimitExceeded  = 2, // >= 3 NoteOns at exact same sampleFrame
    CommitmentHorizonViolation      = 3, // event before committed horizon or outside phrase
    SpscCapacityExceeded            = 4, // > 64 events planned in phrase
    StructuralInvariantViolation    = 5
};

struct CandidateEvent {
    std::uint32_t localEventId{0};     // Temporary candidate-local ID
    std::uint64_t sampleFrame{0};
    MusicRole role{MusicRole::Pulso};
    float pitchHz{220.0f};
    float velocity{1.0f};
    std::uint32_t durationFrames{0};   // 0 = open pad
    float timbreParam{0.0f};
    bool isOpenPad{false};
};

struct PhraseCandidate {
    std::uint32_t candidateId{0};
    Motif transformedMotif{};
    TriadVoicing voicing{};
    std::uint32_t pulsoPulses{4};
    std::int32_t pulsoRotation{0};
    std::uint32_t tramaPulses{6};
    std::int32_t tramaRotation{2};
    std::size_t eventCount{0};
    std::array<CandidateEvent, kMaxEventsPerPhrase> events{};
    HardConstraintViolation violation{HardConstraintViolation::None};
    std::int32_t scoreFixedPoint{0};    // 0..100000 deterministic fixed-point score
    std::uint64_t tieBreakHash{0};
    std::uint32_t simulatedStealsCount{0};
};

enum class FallbackKind : std::uint8_t {
    None            = 0,
    RarefiedMotif   = 1,
    PostCommitment  = 2,
    PadSustainOnly  = 3,
    SilentRecovery  = 4 // Zero new events emitted (INC3-R03)
};

struct PhraseDecisionTrace {
    std::uint32_t sectionIndex{0};
    std::uint32_t phraseIndex{0};
    std::uint32_t candidatesGenerated{0};
    std::uint32_t candidatesValid{0};
    std::uint32_t winningCandidateId{0};
    std::int32_t winningScore{0};
    bool fallbackTriggered{false};
    FallbackKind fallbackKind{FallbackKind::None};
    std::size_t finalEventCount{0};
};

// Simulated voice state for cross-phrase tracking (INC3-R02)
struct SimulatedVoiceSlot {
    bool active{false};
    MusicRole role{MusicRole::Pulso};
    std::uint64_t eventId{0};
    std::uint64_t startFrame{0};
    std::uint64_t expireFrame{0}; // 0 = open pad
    float pitchHz{220.0f};
    bool inRelease{false};
};

class SimulatedVoiceTracker {
public:
    static constexpr std::size_t kTotalSlots = 8;
    static constexpr std::size_t kSlotsPerRole = 2;

    SimulatedVoiceTracker() noexcept = default;

    void updateAtFrame(std::uint64_t frame) noexcept;
    [[nodiscard]] std::uint32_t activeCountForRole(MusicRole role, std::uint64_t frame) const noexcept;
    // Simulates voice allocation/stealing according to Rules 0-2 (ADR-0003 D-M2-04)
    // Returns slot index [0..7], sets wasStolen to true if steal occurred
    int simulateNoteOn(MusicRole role, std::uint64_t eventId, std::uint64_t sampleFrame,
                       std::uint32_t durationFrames, float pitchHz, bool& wasStolen) noexcept;
    void simulateNoteOff(MusicRole role, std::uint64_t eventId, std::uint64_t sampleFrame) noexcept;
    void reset() noexcept;

    [[nodiscard]] const std::array<SimulatedVoiceSlot, kTotalSlots>& slots() const noexcept { return slots_; }

private:
    std::array<SimulatedVoiceSlot, kTotalSlots> slots_{};
};


// --- 6. MusicPlanner Class (Coordinator Thread Only) ---

struct MusicPlannerConfig {
    std::uint64_t runSeed{1337ULL};
    core::PolicyVersionId policyVersion{core::DEFAULT_POLICY_VERSION_ID};
    std::uint32_t candidateCount{6}; // Default 6, allowed [4, 8]
    std::uint32_t sampleRate{48000};
};

class MusicPlanner {
public:
    explicit MusicPlanner(const MusicPlannerConfig& config = {}) noexcept;

    // Plan composition
    void setFormPlan(const FormPlan& form) noexcept;
    [[nodiscard]] const FormPlan& formPlan() const noexcept { return formPlan_; }

    // Generates events for a phrase plan given the committed horizon
    // Emits planned MusicIntentEvent items and schedules explicit open-pad NoteOffs
    void planPhrase(const PhrasePlan& phrase,
                    std::uint64_t committedHorizonFrame,
                    std::uint64_t currentTransportEpoch,
                    std::vector<MusicIntentEvent>& outIntents,
                    AudioScheduler* scheduler = nullptr);

    // Factory profiles
    [[nodiscard]] static FormPlan makeFixtureM24V1Form() noexcept;   // 96 bars A-B-A' (~182.86s)
    [[nodiscard]] static FormPlan makePlannerProbeV1Form() noexcept; // 8 bars A-B test (~14.86s)
    [[nodiscard]] static Motif makeDorianBaseMotif() noexcept;
    [[nodiscard]] static Motif makeAeolianBaseMotif() noexcept;

    // Accessors & Telemetry
    [[nodiscard]] std::uint64_t nextEventId() const noexcept { return nextEventId_; }
    [[nodiscard]] std::size_t decisionTraceCount() const noexcept { return traces_.size(); }
    [[nodiscard]] const std::vector<PhraseDecisionTrace>& decisionTraces() const noexcept { return traces_; }
    [[nodiscard]] std::uint32_t totalFallbacksTriggered() const noexcept { return totalFallbacks_; }
    [[nodiscard]] const SimulatedVoiceTracker& voiceTracker() const noexcept { return voiceTracker_; }

    void reset(std::uint64_t newSeed = 0) noexcept;

    // Candidate selection logic exposed for testing (INC3-R05)
    [[nodiscard]] PhraseCandidate selectWinningCandidate(
        std::vector<PhraseCandidate>& candidates,
        std::uint64_t phraseDecisionId
    ) const noexcept;

private:
    MusicPlannerConfig config_{};
    FormPlan formPlan_{};
    SimulatedVoiceTracker voiceTracker_{};
    std::uint64_t nextEventId_{1};
    std::uint32_t totalFallbacks_{0};
    std::vector<PhraseDecisionTrace> traces_{};

    void generateCandidates(const PhrasePlan& phrase,
                            std::uint64_t phraseDecisionId,
                            std::uint64_t committedHorizonFrame,
                            std::vector<PhraseCandidate>& outCandidates) const;

    [[nodiscard]] HardConstraintViolation validateHardConstraints(
        const PhraseCandidate& candidate,
        const PhrasePlan& phrase,
        std::uint64_t committedHorizonFrame
    ) const noexcept;

    [[nodiscard]] std::int32_t scoreCandidateFixedPoint(
        const PhraseCandidate& candidate,
        const PhrasePlan& phrase
    ) const noexcept;

    [[nodiscard]] PhraseCandidate buildFallbackPhrase(
        const PhrasePlan& phrase,
        std::uint64_t committedHorizonFrame
    ) const noexcept;
};

} // namespace cymatica::audio
