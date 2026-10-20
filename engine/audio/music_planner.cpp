#include "music_planner.h"
#include "unsigned_math.h"

#include <algorithm>
#include <cmath>

namespace cymatica::audio {

namespace {

inline std::int32_t floorDiv(std::int32_t a, std::int32_t b) noexcept {
    std::int32_t res = a / b;
    std::int32_t rem = a % b;
    if (rem != 0 && ((a < 0) ^ (b < 0))) {
        --res;
    }
    return res;
}

inline std::int32_t floorMod(std::int32_t a, std::int32_t b) noexcept {
    return a - floorDiv(a, b) * b;
}

} // namespace

// --- 1. Modal Scales & Pitch Calculations ---

std::int32_t degreeToMidiNote(const ModalScale& scale, std::int32_t scaleDegree, std::int8_t chromaticOffset) noexcept {
    const std::int32_t oct = floorDiv(scaleDegree, 7);
    const auto deg = static_cast<std::uint8_t>(floorMod(scaleDegree, 7));
    const std::int32_t interval = modeInterval(scale.mode, deg);
    return scale.rootMidiNote + oct * 12 + interval + chromaticOffset;
}

float degreeToPitchHz(const ModalScale& scale, std::int32_t scaleDegree, std::int8_t chromaticOffset) noexcept {
    const std::int32_t midi = degreeToMidiNote(scale, scaleDegree, chromaticOffset);
    const float clampedMidi = std::clamp(static_cast<float>(midi), 0.0f, 127.0f);
    const float hz = 440.0f * std::pow(2.0f, (clampedMidi - 69.0f) / 12.0f);
    return std::clamp(hz, 20.0f, 16000.0f);
}


// --- 2. Symbolic Motif & Pure Transformations (INC3-R06) ---

Motif applyModalTranspose(const Motif& in, std::int16_t degreeDelta) noexcept {
    Motif out = in;
    out.parentMotifId = in.motifId;

    for (std::size_t i = 0; i < out.noteCount; ++i) {
        std::int16_t newDegree = static_cast<std::int16_t>(out.notes[i].scaleDegree + degreeDelta);
        // Register bounding per role: keep in typical modal range [-7, 14] by octave wrapping if exceeded
        if (newDegree > 14) {
            newDegree = static_cast<std::int16_t>(newDegree - 7);
        } else if (newDegree < -7) {
            newDegree = static_cast<std::int16_t>(newDegree + 7);
        }
        out.notes[i].scaleDegree = newDegree;
    }
    return out;
}

Motif applyContourInversion(const Motif& in, std::int16_t pivotDegree) noexcept {
    Motif out = in;
    out.parentMotifId = in.motifId;

    for (std::size_t i = 0; i < out.noteCount; ++i) {
        const std::int16_t invDegree = static_cast<std::int16_t>(2 * pivotDegree - out.notes[i].scaleDegree);
        out.notes[i].scaleDegree = invDegree;
        if (out.notes[i].isAnchor) {
            // Anchor preservation: ensure landing on chord tone (octave root, 3rd, or 5th)
            const auto mDeg = static_cast<std::uint8_t>(floorMod(invDegree, 7));
            if (mDeg != 0 && mDeg != 2 && mDeg != 4) {
                // Snap to nearest diatonic triad member (0, 2, or 4)
                if (mDeg == 1) out.notes[i].scaleDegree = static_cast<std::int16_t>(invDegree - 1);
                else if (mDeg == 3) out.notes[i].scaleDegree = static_cast<std::int16_t>(invDegree - 1);
                else if (mDeg == 5) out.notes[i].scaleDegree = static_cast<std::int16_t>(invDegree - 1);
                else if (mDeg == 6) out.notes[i].scaleDegree = static_cast<std::int16_t>(invDegree + 1);
            }
        }
    }
    return out;
}

Motif applyEuclideanRhythmVariation(const Motif& in, std::uint32_t pulses, std::int32_t rotation) noexcept {
    Motif out = in;
    out.parentMotifId = in.motifId;

    const std::uint32_t steps = (in.lengthSteps > 0 && in.lengthSteps <= 32) ? in.lengthSteps : 16u;
    const auto rhythm = core::euclideanRhythm(steps, pulses, rotation);

    std::uint8_t activeOnsets[32]{};
    std::uint8_t onsetCount = 0;
    for (std::uint8_t s = 0; s < steps; ++s) {
        if (rhythm[s]) {
            activeOnsets[onsetCount++] = s;
        }
    }

    if (onsetCount == 0) return out;

    const std::uint8_t count = std::min(in.noteCount, onsetCount);
    for (std::uint8_t i = 0; i < count; ++i) {
        out.notes[i].stepOffset = activeOnsets[i];
        const std::uint8_t nextOnset = (i + 1 < count) ? activeOnsets[i + 1] : static_cast<std::uint8_t>(steps);
        out.notes[i].durationSteps = (nextOnset > activeOnsets[i]) ? (nextOnset - activeOnsets[i]) : 1u;
    }
    out.noteCount = count;
    return out;
}

Motif applyReorchestration(const Motif& in, MusicRole newRole, std::int8_t octaveDelta) noexcept {
    Motif out = in;
    out.parentMotifId = in.motifId;
    out.targetRole = newRole;
    out.registerOctaveOffset = static_cast<std::int8_t>(out.registerOctaveOffset + octaveDelta);
    return out;
}

Motif applyChromaticAlteration(const Motif& in, std::uint8_t noteIdx, std::int8_t semitoneDelta) noexcept {
    Motif out = in;
    out.parentMotifId = in.motifId;

    if (noteIdx < out.noteCount && !out.notes[noteIdx].isAnchor) {
        const std::int8_t cur = out.notes[noteIdx].chromaticOffset;
        out.notes[noteIdx].chromaticOffset = static_cast<std::int8_t>(std::clamp(cur + semitoneDelta, -1, 1));
    }
    return out;
}

bool verifyContourPreserved(const Motif& original, const Motif& transposed) noexcept {
    if (original.noteCount != transposed.noteCount || original.noteCount < 2) {
        return original.noteCount == transposed.noteCount;
    }
    for (std::size_t i = 0; i < original.noteCount - 1; ++i) {
        const int diffOrig = original.notes[i + 1].scaleDegree - original.notes[i].scaleDegree;
        const int diffTrans = transposed.notes[i + 1].scaleDegree - transposed.notes[i].scaleDegree;
        if (diffOrig != diffTrans) return false;
    }
    return true;
}

bool verifyInversionSymmetric(const Motif& original, const Motif& inverted, std::int16_t pivot) noexcept {
    if (original.noteCount != inverted.noteCount) return false;
    for (std::size_t i = 0; i < original.noteCount; ++i) {
        if (!original.notes[i].isAnchor) {
            const int expected = 2 * pivot - original.notes[i].scaleDegree;
            if (inverted.notes[i].scaleDegree != expected) return false;
        }
    }
    return true;
}


// --- 3. Harmony & Triad Voicing (INC3-R02) ---

TriadVoicing voiceTriad(const ChordSpec& chord, std::int16_t prevCorpoHarmonyDegree) noexcept {
    TriadVoicing v{};
    // Voice 0 in Corpo takes root in bass (octave -1)
    v.corpoBassDegree = static_cast<std::int16_t>(chord.rootDegree - 7);

    // Voice 1 in Corpo: minimal voice-leading leap from previous harmony degree
    if (prevCorpoHarmonyDegree >= 0) {
        const int distFifth = std::abs(chord.fifthDegree - prevCorpoHarmonyDegree);
        const int distThird = std::abs(chord.thirdDegree - prevCorpoHarmonyDegree);
        if (distThird < distFifth) {
            v.corpoHarmonyDegree = chord.thirdDegree;
            v.upperColorDegree = chord.fifthDegree;
            v.isThirdInCorpo = true;
        } else {
            v.corpoHarmonyDegree = chord.fifthDegree;
            v.upperColorDegree = chord.thirdDegree;
            v.isThirdInCorpo = false;
        }
    } else {
        // Default to root + fifth in Corpo, third to upper role
        v.corpoHarmonyDegree = chord.fifthDegree;
        v.upperColorDegree = chord.thirdDegree;
        v.isThirdInCorpo = false;
    }
    return v;
}


// --- 4. Simulated Voice Tracker (Cross-Phrase State Tracking, INC3-R02) ---

void SimulatedVoiceTracker::updateAtFrame(std::uint64_t frame) noexcept {
    for (auto& s : slots_) {
        if (s.active && s.expireFrame > 0 && s.expireFrame <= frame) {
            s.active = false;
        }
    }
}

std::uint32_t SimulatedVoiceTracker::activeCountForRole(MusicRole role, std::uint64_t frame) const noexcept {
    const auto rIdx = static_cast<std::size_t>(role);
    if (rIdx >= 4) return 0;
    const std::size_t s0 = rIdx * kSlotsPerRole;
    const std::size_t s1 = s0 + 1;
    std::uint32_t count = 0;
    if (slots_[s0].active && (slots_[s0].expireFrame == 0 || slots_[s0].expireFrame > frame)) ++count;
    if (slots_[s1].active && (slots_[s1].expireFrame == 0 || slots_[s1].expireFrame > frame)) ++count;
    return count;
}

int SimulatedVoiceTracker::simulateNoteOn(MusicRole role, std::uint64_t eventId, std::uint64_t sampleFrame,
                                        std::uint32_t durationFrames, float pitchHz, bool& wasStolen) noexcept {
    const auto rIdx = static_cast<std::size_t>(role);
    if (rIdx >= 4) {
        wasStolen = false;
        return -1;
    }
    updateAtFrame(sampleFrame);

    const std::size_t s0 = rIdx * kSlotsPerRole;
    const std::size_t s1 = s0 + 1;

    // Rule 0: pick idle slot
    if (!slots_[s0].active) {
        slots_[s0] = {true, role, eventId, sampleFrame, (durationFrames > 0) ? (sampleFrame + durationFrames) : 0ULL, pitchHz, false};
        wasStolen = false;
        return static_cast<int>(s0);
    }
    if (!slots_[s1].active) {
        slots_[s1] = {true, role, eventId, sampleFrame, (durationFrames > 0) ? (sampleFrame + durationFrames) : 0ULL, pitchHz, false};
        wasStolen = false;
        return static_cast<int>(s1);
    }

    // Rule 1: pick slot in release
    if (slots_[s0].inRelease && !slots_[s1].inRelease) {
        slots_[s0] = {true, role, eventId, sampleFrame, (durationFrames > 0) ? (sampleFrame + durationFrames) : 0ULL, pitchHz, false};
        wasStolen = true;
        return static_cast<int>(s0);
    }
    if (slots_[s1].inRelease && !slots_[s0].inRelease) {
        slots_[s1] = {true, role, eventId, sampleFrame, (durationFrames > 0) ? (sampleFrame + durationFrames) : 0ULL, pitchHz, false};
        wasStolen = true;
        return static_cast<int>(s1);
    }

    // Rule 2: pick oldest startFrame
    const std::size_t victim = (slots_[s0].startFrame <= slots_[s1].startFrame) ? s0 : s1;
    slots_[victim] = {true, role, eventId, sampleFrame, (durationFrames > 0) ? (sampleFrame + durationFrames) : 0ULL, pitchHz, false};
    wasStolen = true;
    return static_cast<int>(victim);
}

void SimulatedVoiceTracker::simulateNoteOff(MusicRole role, std::uint64_t eventId, std::uint64_t sampleFrame) noexcept {
    const auto rIdx = static_cast<std::size_t>(role);
    if (rIdx >= 4) return;
    const std::size_t s0 = rIdx * kSlotsPerRole;
    const std::size_t s1 = s0 + 1;

    if (slots_[s0].active && slots_[s0].eventId == eventId) {
        slots_[s0].inRelease = true;
        slots_[s0].expireFrame = sampleFrame + 96; // 2 ms de-click tail
    }
    if (slots_[s1].active && slots_[s1].eventId == eventId) {
        slots_[s1].inRelease = true;
        slots_[s1].expireFrame = sampleFrame + 96;
    }
}

void SimulatedVoiceTracker::reset() noexcept {
    slots_.fill(SimulatedVoiceSlot{});
}


// --- 5. MusicPlanner Implementation ---

MusicPlanner::MusicPlanner(const MusicPlannerConfig& config) noexcept
    : config_(config) {
    if (config_.candidateCount < kMinCandidates) config_.candidateCount = static_cast<std::uint32_t>(kMinCandidates);
    if (config_.candidateCount > kMaxCandidates) config_.candidateCount = static_cast<std::uint32_t>(kMaxCandidates);
    formPlan_ = makeFixtureM24V1Form();
}

void MusicPlanner::setFormPlan(const FormPlan& form) noexcept {
    formPlan_ = form;
}

void MusicPlanner::reset(std::uint64_t newSeed) noexcept {
    if (newSeed != 0) {
        config_.runSeed = newSeed;
    }
    nextEventId_ = 1;
    totalFallbacks_ = 0;
    traces_.clear();
    voiceTracker_.reset();
}

FormPlan MusicPlanner::makeFixtureM24V1Form() noexcept {
    FormPlan fp{};
    fp.sectionCount = 3;

    // Section 0 (A): D Dorian, 120 BPM 4/4 (32 bars)
    fp.sections[0] = SectionPlan{
        .sectionIndex = 0,
        .startBar = 0,
        .barCount = 32,
        .tempoMap = TempoMap{.bpm = RationalBpm{120, 1}, .timeSignatureNum = 4, .timeSignatureDen = 4, .subdivisionsPerBeat = 4, .beatsPerBar = 4, .barsPerPhrase = 4},
        .scale = ModalScale{.rootMidiNote = 62, .mode = ScaleMode::Dorian}, // D4
        .baseTension = 0.20f,
        .sectionMotif = makeDorianBaseMotif(),
        .sustainPadIntoNextSection = true
    };

    // Section 1 (B): G Aeolian, 140 BPM 4/4 (32 bars)
    fp.sections[1] = SectionPlan{
        .sectionIndex = 1,
        .startBar = 32,
        .barCount = 32,
        .tempoMap = TempoMap{.bpm = RationalBpm{140, 1}, .timeSignatureNum = 4, .timeSignatureDen = 4, .subdivisionsPerBeat = 4, .beatsPerBar = 4, .barsPerPhrase = 4},
        .scale = ModalScale{.rootMidiNote = 55, .mode = ScaleMode::Aeolian}, // G3
        .baseTension = 0.50f,
        .sectionMotif = makeAeolianBaseMotif(),
        .sustainPadIntoNextSection = true
    };

    // Section 2 (A'): D Dorian, 120 BPM 4/4 (32 bars)
    fp.sections[2] = SectionPlan{
        .sectionIndex = 2,
        .startBar = 64,
        .barCount = 32,
        .tempoMap = TempoMap{.bpm = RationalBpm{120, 1}, .timeSignatureNum = 4, .timeSignatureDen = 4, .subdivisionsPerBeat = 4, .beatsPerBar = 4, .barsPerPhrase = 4},
        .scale = ModalScale{.rootMidiNote = 62, .mode = ScaleMode::Dorian}, // D4
        .baseTension = 0.70f,
        .sectionMotif = makeDorianBaseMotif(),
        .sustainPadIntoNextSection = false
    };

    return fp;
}

FormPlan MusicPlanner::makePlannerProbeV1Form() noexcept {
    FormPlan fp{};
    fp.sectionCount = 2;

    // 8-bar test form: Section A (4 bars, 120 BPM D Dorian) -> Section B (4 bars, 140 BPM G Aeolian)
    fp.sections[0] = SectionPlan{
        .sectionIndex = 0,
        .startBar = 0,
        .barCount = 4,
        .tempoMap = TempoMap{.bpm = RationalBpm{120, 1}, .timeSignatureNum = 4, .timeSignatureDen = 4, .subdivisionsPerBeat = 4, .beatsPerBar = 4, .barsPerPhrase = 4},
        .scale = ModalScale{.rootMidiNote = 62, .mode = ScaleMode::Dorian},
        .baseTension = 0.25f,
        .sectionMotif = makeDorianBaseMotif(),
        .sustainPadIntoNextSection = true
    };

    fp.sections[1] = SectionPlan{
        .sectionIndex = 1,
        .startBar = 4,
        .barCount = 4,
        .tempoMap = TempoMap{.bpm = RationalBpm{140, 1}, .timeSignatureNum = 4, .timeSignatureDen = 4, .subdivisionsPerBeat = 4, .beatsPerBar = 4, .barsPerPhrase = 4},
        .scale = ModalScale{.rootMidiNote = 55, .mode = ScaleMode::Aeolian},
        .baseTension = 0.60f,
        .sectionMotif = makeAeolianBaseMotif(),
        .sustainPadIntoNextSection = false
    };

    return fp;
}

Motif MusicPlanner::makeDorianBaseMotif() noexcept {
    Motif m{};
    m.motifId = 101;
    m.targetRole = MusicRole::Vettore;
    m.lengthSteps = 16;
    m.noteCount = 4;
    m.notes[0] = MotifNote{.scaleDegree = 0, .chromaticOffset = 0, .stepOffset = 0, .durationSteps = 4, .velocity = 0.85f, .isAnchor = true};  // D4 tonic
    m.notes[1] = MotifNote{.scaleDegree = 2, .chromaticOffset = 0, .stepOffset = 4, .durationSteps = 4, .velocity = 0.75f, .isAnchor = false}; // F4 minor 3rd
    m.notes[2] = MotifNote{.scaleDegree = 4, .chromaticOffset = 0, .stepOffset = 8, .durationSteps = 4, .velocity = 0.80f, .isAnchor = true};  // A4 perfect 5th
    m.notes[3] = MotifNote{.scaleDegree = 3, .chromaticOffset = 0, .stepOffset = 12, .durationSteps = 4, .velocity = 0.70f, .isAnchor = false}; // G4 perfect 4th
    return m;
}

Motif MusicPlanner::makeAeolianBaseMotif() noexcept {
    Motif m{};
    m.motifId = 201;
    m.targetRole = MusicRole::Vettore;
    m.lengthSteps = 16;
    m.noteCount = 4;
    m.notes[0] = MotifNote{.scaleDegree = 0, .chromaticOffset = 0, .stepOffset = 0, .durationSteps = 4, .velocity = 0.85f, .isAnchor = true};  // G3 tonic
    m.notes[1] = MotifNote{.scaleDegree = 4, .chromaticOffset = 0, .stepOffset = 4, .durationSteps = 4, .velocity = 0.80f, .isAnchor = true};  // D4 fifth
    m.notes[2] = MotifNote{.scaleDegree = 5, .chromaticOffset = 0, .stepOffset = 8, .durationSteps = 4, .velocity = 0.75f, .isAnchor = false}; // Eb4 minor 6th
    m.notes[3] = MotifNote{.scaleDegree = 4, .chromaticOffset = 0, .stepOffset = 12, .durationSteps = 4, .velocity = 0.70f, .isAnchor = false}; // D4 fifth
    return m;
}

void MusicPlanner::generateCandidates(const PhrasePlan& phrase,
                                     std::uint64_t phraseDecisionId,
                                     std::uint64_t committedHorizonFrame,
                                     std::vector<PhraseCandidate>& outCandidates) const {
    (void)phraseDecisionId;
    outCandidates.clear();
    outCandidates.reserve(config_.candidateCount);

    const std::uint64_t phraseDurationFrames = (phrase.endFrame > phrase.startFrame)
        ? (phrase.endFrame - phrase.startFrame) : 192000ULL;
    const std::uint64_t barFrames = phraseDurationFrames / 4;
    const std::uint64_t stepFrames = barFrames / 16; // 16 subdivisions per bar

    const Motif baseMotif = (phrase.scale.mode == ScaleMode::Aeolian) ? makeAeolianBaseMotif() : makeDorianBaseMotif();

    for (std::uint32_t k = 0; k < config_.candidateCount; ++k) {
        PhraseCandidate cand{};
        cand.candidateId = static_cast<std::uint32_t>((phrase.sectionIndex << 16) | (phrase.phraseIndex << 8) | k);

        // Derive variation for candidate k
        switch (k % 6) {
            case 0:
                cand.transformedMotif = baseMotif; // Identity (conservative)
                break;
            case 1:
                cand.transformedMotif = applyModalTranspose(baseMotif, static_cast<std::int16_t>(2 + (k % 3)));
                break;
            case 2:
                cand.transformedMotif = applyContourInversion(baseMotif, 2);
                break;
            case 3:
                cand.transformedMotif = applyEuclideanRhythmVariation(baseMotif, 4, static_cast<std::int32_t>(k));
                break;
            case 4:
                cand.transformedMotif = applyChromaticAlteration(baseMotif, 1, 1);
                break;
            case 5:
                cand.transformedMotif = applyReorchestration(baseMotif, MusicRole::Vettore, 1);
                break;
        }

        // Voicing for the phrase's primary chord
        cand.voicing = voiceTriad(phrase.chords[0]);

        // Pulso Bjorklund rhythm
        cand.pulsoPulses = 4 + (k % 3); // 4, 5, or 6 pulses per bar
        cand.pulsoRotation = static_cast<std::int32_t>(k % 4);
        const auto pulsoRhythm = core::euclideanRhythm(16, cand.pulsoPulses, cand.pulsoRotation);

        // Trama Bjorklund rhythm
        cand.tramaPulses = 6 + (k % 3); // 6, 7, or 8 pulses per bar
        cand.tramaRotation = static_cast<std::int32_t>((k + 2) % 4);
        const auto tramaRhythm = core::euclideanRhythm(16, cand.tramaPulses, cand.tramaRotation);

        std::size_t evtIdx = 0;

        // 1. Pulso events (4 bars)
        for (std::uint32_t b = 0; b < 4 && evtIdx < kMaxEventsPerPhrase; ++b) {
            const std::uint64_t barStart = phrase.startFrame + b * barFrames;
            for (std::uint32_t s = 0; s < 16 && evtIdx < kMaxEventsPerPhrase; ++s) {
                if (pulsoRhythm[s]) {
                    const std::uint64_t frame = barStart + s * stepFrames;
                    if (frame >= committedHorizonFrame && frame < phrase.endFrame) {
                        cand.events[evtIdx++] = CandidateEvent{
                            .localEventId = static_cast<std::uint32_t>(k * 1000 + evtIdx),
                            .sampleFrame = frame,
                            .role = MusicRole::Pulso,
                            .pitchHz = 60.0f + static_cast<float>(k * 2),
                            .velocity = (s == 0) ? 0.95f : 0.75f,
                            .durationFrames = static_cast<std::uint32_t>(stepFrames / 2),
                            .timbreParam = 0.30f + 0.05f * static_cast<float>(k % 4),
                            .isOpenPad = false
                        };
                    }
                }
            }
        }

        // 2. Corpo events: 2 voices (Bass root + Harmony fifth/third)
        // Sustained pad across phrase or per-bar chord
        for (std::uint32_t b = 0; b < 4 && evtIdx + 1 < kMaxEventsPerPhrase; ++b) {
            const std::uint64_t barStart = phrase.startFrame + b * barFrames;
            if (b == 0 || !phrase.chords[0].openPadSustain) {
                const std::uint64_t frame = barStart;
                if (frame >= committedHorizonFrame && frame < phrase.endFrame) {
                    const bool isOpen = phrase.chords[0].openPadSustain;
                    const std::uint32_t dur = isOpen ? 0u : static_cast<std::uint32_t>(barFrames * 4);

                    // Voice 0: Bass
                    cand.events[evtIdx++] = CandidateEvent{
                        .localEventId = static_cast<std::uint32_t>(k * 1000 + evtIdx),
                        .sampleFrame = frame,
                        .role = MusicRole::Corpo,
                        .pitchHz = degreeToPitchHz(phrase.scale, cand.voicing.corpoBassDegree),
                        .velocity = 0.85f,
                        .durationFrames = dur,
                        .timbreParam = 0.40f,
                        .isOpenPad = isOpen
                    };
                    // Voice 1: Harmony (Fifth or Third)
                    cand.events[evtIdx++] = CandidateEvent{
                        .localEventId = static_cast<std::uint32_t>(k * 1000 + evtIdx),
                        .sampleFrame = frame,
                        .role = MusicRole::Corpo,
                        .pitchHz = degreeToPitchHz(phrase.scale, cand.voicing.corpoHarmonyDegree),
                        .velocity = 0.75f,
                        .durationFrames = dur,
                        .timbreParam = 0.40f,
                        .isOpenPad = isOpen
                    };
                }
            }
        }

        // 3. Trama events: texture & complementary triad color tone
        for (std::uint32_t b = 0; b < 4 && evtIdx < kMaxEventsPerPhrase; ++b) {
            const std::uint64_t barStart = phrase.startFrame + b * barFrames;
            for (std::uint32_t s = 0; s < 16 && evtIdx < kMaxEventsPerPhrase; ++s) {
                if (tramaRhythm[s] && !pulsoRhythm[s]) { // Complementary syncopation
                    const std::uint64_t frame = barStart + s * stepFrames;
                    if (frame >= committedHorizonFrame && frame < phrase.endFrame) {
                        const std::int16_t deg = (s % 2 == 0) ? cand.voicing.upperColorDegree : cand.voicing.corpoHarmonyDegree;
                        cand.events[evtIdx++] = CandidateEvent{
                            .localEventId = static_cast<std::uint32_t>(k * 1000 + evtIdx),
                            .sampleFrame = frame,
                            .role = MusicRole::Trama,
                            .pitchHz = degreeToPitchHz(phrase.scale, deg + 7), // Upper octave
                            .velocity = 0.70f,
                            .durationFrames = static_cast<std::uint32_t>(stepFrames),
                            .timbreParam = 0.50f,
                            .isOpenPad = false
                        };
                    }
                }
            }
        }

        // 4. Vettore events: melodic motif
        for (std::uint32_t b = 0; b < 4 && evtIdx < kMaxEventsPerPhrase; ++b) {
            const std::uint64_t barStart = phrase.startFrame + b * barFrames;
            for (std::size_t m = 0; m < cand.transformedMotif.noteCount && evtIdx < kMaxEventsPerPhrase; ++m) {
                const auto& note = cand.transformedMotif.notes[m];
                const std::uint64_t frame = barStart + note.stepOffset * stepFrames;
                if (frame >= committedHorizonFrame && frame < phrase.endFrame) {
                    cand.events[evtIdx++] = CandidateEvent{
                        .localEventId = static_cast<std::uint32_t>(k * 1000 + evtIdx),
                        .sampleFrame = frame,
                        .role = MusicRole::Vettore,
                        .pitchHz = degreeToPitchHz(phrase.scale, note.scaleDegree, note.chromaticOffset),
                        .velocity = note.velocity,
                        .durationFrames = static_cast<std::uint32_t>(note.durationSteps * stepFrames),
                        .timbreParam = 0.60f,
                        .isOpenPad = false
                    };
                }
            }
        }

        cand.eventCount = evtIdx;
        outCandidates.push_back(cand);
    }
}

HardConstraintViolation MusicPlanner::validateHardConstraints(
    const PhraseCandidate& candidate,
    const PhrasePlan& phrase,
    std::uint64_t committedHorizonFrame
) const noexcept {
    if (candidate.eventCount == 0) {
        return HardConstraintViolation::CommitmentHorizonViolation;
    }
    if (candidate.eventCount > kMaxEventsPerPhrase) {
        return HardConstraintViolation::SpscCapacityExceeded;
    }

    // Check individual parameters
    for (std::size_t i = 0; i < candidate.eventCount; ++i) {
        const auto& e = candidate.events[i];
        if (!std::isfinite(e.pitchHz) || e.pitchHz < 20.0f || e.pitchHz > 16000.0f) {
            return HardConstraintViolation::InvalidParameters;
        }
        if (!std::isfinite(e.velocity) || e.velocity < 0.0f || e.velocity > 1.0f) {
            return HardConstraintViolation::InvalidParameters;
        }
        if (!std::isfinite(e.timbreParam) || e.timbreParam < 0.0f || e.timbreParam > 1.0f) {
            return HardConstraintViolation::InvalidParameters;
        }
        if (e.sampleFrame < committedHorizonFrame || e.sampleFrame >= phrase.endFrame) {
            return HardConstraintViolation::CommitmentHorizonViolation;
        }
    }

    // Check simultaneous onsets: max 2 NoteOns per role at exact same sampleFrame
    for (std::size_t i = 0; i < candidate.eventCount; ++i) {
        std::uint32_t sameRoleSameFrame = 0;
        for (std::size_t j = 0; j < candidate.eventCount; ++j) {
            if (candidate.events[i].role == candidate.events[j].role &&
                candidate.events[i].sampleFrame == candidate.events[j].sampleFrame) {
                ++sameRoleSameFrame;
            }
        }
        if (sameRoleSameFrame > 2) {
            return HardConstraintViolation::SimultaneousOnsetLimitExceeded;
        }
    }

    return HardConstraintViolation::None;
}

std::int32_t MusicPlanner::scoreCandidateFixedPoint(
    const PhraseCandidate& candidate,
    const PhrasePlan& phrase
) const noexcept {
    // 1. Voice Leading Score: minimal leap from root (0..1000)
    std::int32_t sVl = 850;
    if (candidate.voicing.corpoHarmonyDegree == 4) {
        sVl = 1000; // Perfect fifth = very stable
    } else if (candidate.voicing.corpoHarmonyDegree == 2) {
        sVl = 900;  // Third = sweet modal color
    }

    // 2. Tension Adherence Score (0..1000): match phrase.targetTension
    float estimatedTension = 0.20f;
    if (candidate.transformedMotif.motifId != 101 && candidate.transformedMotif.motifId != 201) {
        estimatedTension += 0.30f; // Transform adds tension
    }
    if (candidate.pulsoPulses > 4) {
        estimatedTension += 0.15f;
    }
    const float tensionDiff = std::abs(estimatedTension - phrase.targetTension);
    const auto sTen = static_cast<std::int32_t>(std::clamp(1000.0f - tensionDiff * 1000.0f, 0.0f, 1000.0f));

    // 3. Motif Identity Score (0..1000): anchor fidelity
    std::int32_t sMot = 1000;
    for (std::size_t i = 0; i < candidate.transformedMotif.noteCount; ++i) {
        if (candidate.transformedMotif.notes[i].isAnchor && candidate.transformedMotif.notes[i].chromaticOffset != 0) {
            sMot -= 200;
        }
    }
    sMot = std::clamp(sMot, 0, 1000);

    // 4. Rhythmic Balance Score (0..1000)
    std::int32_t sRhy = 800;
    if (candidate.pulsoPulses + candidate.tramaPulses <= 14) {
        sRhy = 1000; // Well-spaced rhythm
    }

    // 5. Voice Stealing Penalty: 250 per steal
    const std::int32_t pSteal = static_cast<std::int32_t>(candidate.simulatedStealsCount * 250);

    // Ponderated sum: W_vl=25, W_ten=25, W_mot=25, W_rhy=25, W_steal=10
    const std::int32_t total = 25 * sVl + 25 * sTen + 25 * sMot + 25 * sRhy - 10 * pSteal;
    return std::clamp(total, 0, 100000);
}

PhraseCandidate MusicPlanner::selectWinningCandidate(
    std::vector<PhraseCandidate>& candidates,
    std::uint64_t phraseDecisionId
) const noexcept {
    // Compute tie-break hashes
    for (auto& cand : candidates) {
        const core::RandomKey key{
            config_.runSeed,
            static_cast<std::uint32_t>(core::StreamId::MusicForm),
            phraseDecisionId,
            cand.candidateId
        };
        cand.tieBreakHash = core::staffordMix13(core::randomU64(key));
    }

    // Strict total order:
    // 1. scoreFixedPoint DESC
    // 2. tieBreakHash DESC
    // 3. candidateId ASC
    auto bestIt = std::max_element(candidates.begin(), candidates.end(), [](const PhraseCandidate& a, const PhraseCandidate& b) {
        if (a.scoreFixedPoint != b.scoreFixedPoint) {
            return a.scoreFixedPoint < b.scoreFixedPoint;
        }
        if (a.tieBreakHash != b.tieBreakHash) {
            return a.tieBreakHash < b.tieBreakHash;
        }
        return a.candidateId > b.candidateId; // lower candidateId wins
    });

    return (bestIt != candidates.end()) ? *bestIt : PhraseCandidate{};
}

PhraseCandidate MusicPlanner::buildFallbackPhrase(
    const PhrasePlan& phrase,
    std::uint64_t committedHorizonFrame
) const noexcept {
    PhraseCandidate fallback{};
    fallback.candidateId = 999999;
    fallback.violation = HardConstraintViolation::None;
    fallback.scoreFixedPoint = 0;

    // Check if temporal horizon allows scheduling
    if (committedHorizonFrame >= phrase.endFrame) {
        // Level 4: Silent Recovery (zero new events)
        fallback.eventCount = 0;
        return fallback;
    }

    const std::uint64_t safeStart = std::max(phrase.startFrame, committedHorizonFrame);
    if (safeStart + 4800 >= phrase.endFrame) {
        // Not enough frame budget left: silent recovery
        fallback.eventCount = 0;
        return fallback;
    }

    // Level 1-2: sparse root note on Corpo + downbeat pulse
    fallback.events[0] = CandidateEvent{
        .localEventId = 999001,
        .sampleFrame = safeStart,
        .role = MusicRole::Corpo,
        .pitchHz = degreeToPitchHz(phrase.scale, -7), // Root bass
        .velocity = 0.70f,
        .durationFrames = static_cast<std::uint32_t>(phrase.endFrame - safeStart),
        .timbreParam = 0.20f,
        .isOpenPad = false
    };
    fallback.events[1] = CandidateEvent{
        .localEventId = 999002,
        .sampleFrame = safeStart,
        .role = MusicRole::Pulso,
        .pitchHz = 60.0f,
        .velocity = 0.80f,
        .durationFrames = 2400,
        .timbreParam = 0.20f,
        .isOpenPad = false
    };
    fallback.eventCount = 2;
    return fallback;
}

void MusicPlanner::planPhrase(const PhrasePlan& phrase,
                             std::uint64_t committedHorizonFrame,
                             std::uint64_t currentTransportEpoch,
                             std::vector<MusicIntentEvent>& outIntents,
                             AudioScheduler* scheduler) {
    const std::uint64_t decisionId = core::deriveDecisionId(
        phrase.sectionIndex, phrase.phraseIndex, phrase.startBar, 0, config_.policyVersion.toU32()
    );

    std::vector<PhraseCandidate> candidates;
    generateCandidates(phrase, decisionId, committedHorizonFrame, candidates);

    std::vector<PhraseCandidate> validCandidates;
    for (auto& cand : candidates) {
        cand.violation = validateHardConstraints(cand, phrase, committedHorizonFrame);
        if (cand.violation == HardConstraintViolation::None) {
            cand.scoreFixedPoint = scoreCandidateFixedPoint(cand, phrase);
            validCandidates.push_back(cand);
        }
    }

    PhraseCandidate winner{};
    bool fallbackTriggered = false;
    FallbackKind fbKind = FallbackKind::None;

    if (!validCandidates.empty()) {
        winner = selectWinningCandidate(validCandidates, decisionId);
    } else {
        winner = buildFallbackPhrase(phrase, committedHorizonFrame);
        fallbackTriggered = true;
        fbKind = (winner.eventCount == 0) ? FallbackKind::SilentRecovery : FallbackKind::RarefiedMotif;
        ++totalFallbacks_;
    }

    // Materialize winner into final MusicIntentEvents with sequential canonical eventId (INC3-R05)
    for (std::size_t i = 0; i < winner.eventCount; ++i) {
        const auto& ce = winner.events[i];
        const std::uint64_t finalId = nextEventId_++;

        bool wasStolen = false;
        voiceTracker_.simulateNoteOn(ce.role, finalId, ce.sampleFrame, ce.durationFrames, ce.pitchHz, wasStolen);

        const MusicIntentEvent intent{
            .eventId = finalId,
            .sampleFrame = ce.sampleFrame,
            .tick = ce.sampleFrame / 400, // 400 samples/tick @ 120 Hz
            .transportEpoch = currentTransportEpoch,
            .role = ce.role,
            .pitchHz = ce.pitchHz,
            .velocity = ce.velocity,
            .durationFrames = ce.durationFrames,
            .timbreParam = ce.timbreParam
        };
        outIntents.push_back(intent);

        if (scheduler != nullptr) {
            scheduler->scheduleIntent(intent, currentTransportEpoch);
            if (ce.isOpenPad && phrase.endFrame > ce.sampleFrame) {
                // Schedule explicit matching NoteOff at phrase boundary (INC3-R01, INC3-R03)
                scheduler->scheduleNoteOff(finalId, ce.role, phrase.endFrame, currentTransportEpoch);
                voiceTracker_.simulateNoteOff(ce.role, finalId, phrase.endFrame);
            }
        }
    }

    traces_.push_back(PhraseDecisionTrace{
        .sectionIndex = phrase.sectionIndex,
        .phraseIndex = phrase.phraseIndex,
        .candidatesGenerated = static_cast<std::uint32_t>(candidates.size()),
        .candidatesValid = static_cast<std::uint32_t>(validCandidates.size()),
        .winningCandidateId = winner.candidateId,
        .winningScore = winner.scoreFixedPoint,
        .fallbackTriggered = fallbackTriggered,
        .fallbackKind = fbKind,
        .finalEventCount = winner.eventCount
    });
}

} // namespace cymatica::audio
