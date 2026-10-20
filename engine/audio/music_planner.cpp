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

inline bool isTonicTriadDegree(std::int32_t degree) noexcept {
    const std::int32_t m = floorMod(degree, 7);
    return m == 0 || m == 2 || m == 4;
}

inline std::int32_t absI32(std::int32_t v) noexcept {
    return v < 0 ? -v : v;
}

// Canonical candidate event order (sampleFrame, role, midiNote, durationFrames) (INC3-R05)
inline bool canonicalEventLess(const CandidateEvent& a, const CandidateEvent& b) noexcept {
    if (a.sampleFrame != b.sampleFrame) return a.sampleFrame < b.sampleFrame;
    if (a.role != b.role) return static_cast<std::uint8_t>(a.role) < static_cast<std::uint8_t>(b.role);
    if (a.midiNote != b.midiNote) return a.midiNote < b.midiNote;
    if (a.durationFrames != b.durationFrames) return a.durationFrames < b.durationFrames;
    return a.isOpenPad < b.isOpenPad;
}

// Absolute beat index at the start of `bar`, mirroring MusicClock::frameAtBar() segment selection.
struct BarAnchor {
    std::uint64_t beat{0};
    TempoMap tempoMap{};
};

BarAnchor beatAtBar(const MusicClock& clock, std::uint64_t bar) noexcept {
    std::size_t segIdx = 0;
    for (std::size_t i = 1; i < clock.segmentCount(); ++i) {
        if (clock.segment(i).startBar <= bar) {
            segIdx = i;
        } else {
            break;
        }
    }
    while (segIdx > 0 && clock.segment(segIdx).startBar == bar && clock.segment(segIdx).beatOffsetInBar != 0) {
        --segIdx;
    }
    const auto& seg = clock.segment(segIdx);
    const std::uint64_t beatsPerBar = seg.tempoMap.beatsPerBar;
    const std::uint64_t deltaBars = (bar >= seg.startBar) ? (bar - seg.startBar) : 0;
    const std::uint64_t beatsToNextBar = (seg.beatOffsetInBar == 0) ? 0 : (beatsPerBar - seg.beatOffsetInBar);
    const std::uint64_t remainingBars = (deltaBars == 0) ? 0 : (deltaBars - (seg.beatOffsetInBar == 0 ? 0 : 1));
    const std::uint64_t deltaBeats = (deltaBars == 0)
        ? 0
        : core::saturatingAdd(beatsToNextBar, core::saturatingMul(remainingBars, beatsPerBar));
    return BarAnchor{.beat = core::saturatingAdd(seg.startBeat, deltaBeats), .tempoMap = seg.tempoMap};
}

// Deterministic quantization of the plan's float tension target into integer milli-units (plan boundary,
// outside the score path). Returns nullopt for non-finite or out-of-range input.
std::optional<std::int32_t> quantizeTensionMilli(float tension) noexcept {
    if (!std::isfinite(tension) || tension < 0.0f || tension > 1.0f) return std::nullopt;
    return static_cast<std::int32_t>(std::lround(static_cast<double>(tension) * 1000.0));
}

constexpr std::uint8_t kFallbackCandidateIndexBase = 0xF0;

std::uint64_t packCandidateId(std::uint32_t section, std::uint32_t phrase, std::uint8_t index) noexcept {
    // section: bits 40..63 (24 bits), phrase: bits 8..39 (full 32 bits), index: bits 0..7
    return ((static_cast<std::uint64_t>(section) & 0xFFFFFFULL) << 40) |
           (static_cast<std::uint64_t>(phrase) << 8) |
           static_cast<std::uint64_t>(index);
}

} // namespace

// --- 1. Modal Scales & Pitch Calculations ---

std::int32_t degreeToMidiNote(const ModalScale& scale, std::int32_t scaleDegree, std::int8_t chromaticOffset) noexcept {
    const std::int32_t oct = floorDiv(scaleDegree, 7);
    const auto deg = static_cast<std::uint8_t>(floorMod(scaleDegree, 7));
    const std::int32_t interval = modeInterval(scale.mode, deg);
    return scale.rootMidiNote + oct * 12 + interval + chromaticOffset;
}

float midiNoteToPitchHz(std::int32_t midiNote) noexcept {
    const float clampedMidi = std::clamp(static_cast<float>(midiNote), 0.0f, 127.0f);
    const float hz = 440.0f * std::pow(2.0f, (clampedMidi - 69.0f) / 12.0f);
    return std::clamp(hz, 20.0f, 16000.0f);
}

float degreeToPitchHz(const ModalScale& scale, std::int32_t scaleDegree, std::int8_t chromaticOffset) noexcept {
    return midiNoteToPitchHz(degreeToMidiNote(scale, scaleDegree, chromaticOffset));
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
        out.notes[i].durationSteps = (nextOnset > activeOnsets[i]) ? static_cast<std::uint8_t>(nextOnset - activeOnsets[i]) : 1u;
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

std::uint32_t deriveTransformedMotifId(std::uint32_t baseMotifId, MotifTransformKind kind,
                                       std::uint32_t variantIndex) noexcept {
    if (kind == MotifTransformKind::Identity) return baseMotifId;
    // base * 100 + kind * 10 + variant: distinct per (kind, variant) and human readable in traces.
    const std::uint64_t id = static_cast<std::uint64_t>(baseMotifId) * 100ULL +
                             static_cast<std::uint64_t>(kind) * 10ULL +
                             static_cast<std::uint64_t>(variantIndex % 10u);
    return static_cast<std::uint32_t>(id & 0xFFFFFFFFULL);
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

bool verifyMotifTransformInvariants(const Motif& base, const Motif& t,
                                    MotifTransformKind kind, std::int16_t param) noexcept {
    if (t.noteCount > base.noteCount || t.noteCount > kMaxMotifNotes || t.lengthSteps != base.lengthSteps) {
        return false;
    }
    const auto sameRhythm = [&](std::size_t i) {
        return t.notes[i].stepOffset == base.notes[i].stepOffset &&
               t.notes[i].durationSteps == base.notes[i].durationSteps &&
               t.notes[i].isAnchor == base.notes[i].isAnchor;
    };
    switch (kind) {
        case MotifTransformKind::Identity:
            if (t.noteCount != base.noteCount || t.registerOctaveOffset != base.registerOctaveOffset) return false;
            for (std::size_t i = 0; i < t.noteCount; ++i) {
                if (!(t.notes[i] == base.notes[i])) return false;
            }
            return true;
        case MotifTransformKind::ModalTranspose:
            if (!verifyContourPreserved(base, t)) return false;
            for (std::size_t i = 0; i < t.noteCount; ++i) {
                if (!sameRhythm(i) || t.notes[i].chromaticOffset != base.notes[i].chromaticOffset) return false;
                const int shift = t.notes[i].scaleDegree - base.notes[i].scaleDegree;
                if (shift != param && shift != param - 7 && shift != param + 7) return false;
            }
            return true;
        case MotifTransformKind::ContourInversion:
            if (!verifyInversionSymmetric(base, t, param)) return false;
            for (std::size_t i = 0; i < t.noteCount; ++i) {
                if (!sameRhythm(i)) return false;
                if (t.notes[i].isAnchor && !isTonicTriadDegree(t.notes[i].scaleDegree)) return false;
            }
            return true;
        case MotifTransformKind::EuclideanRhythmVariation: {
            if (t.noteCount == 0) return false;
            int prevOnset = -1;
            for (std::size_t i = 0; i < t.noteCount; ++i) {
                const auto& n = t.notes[i];
                if (n.scaleDegree != base.notes[i].scaleDegree || n.chromaticOffset != base.notes[i].chromaticOffset ||
                    n.isAnchor != base.notes[i].isAnchor) {
                    return false;
                }
                if (static_cast<int>(n.stepOffset) <= prevOnset || n.durationSteps == 0 ||
                    static_cast<std::uint32_t>(n.stepOffset) + n.durationSteps > t.lengthSteps) {
                    return false;
                }
                prevOnset = n.stepOffset;
            }
            return true;
        }
        case MotifTransformKind::ChromaticAlteration: {
            if (t.noteCount != base.noteCount) return false;
            std::uint32_t altered = 0;
            for (std::size_t i = 0; i < t.noteCount; ++i) {
                if (!sameRhythm(i) || t.notes[i].scaleDegree != base.notes[i].scaleDegree) return false;
                if (t.notes[i].chromaticOffset != base.notes[i].chromaticOffset) {
                    if (t.notes[i].isAnchor || absI32(t.notes[i].chromaticOffset) > 1) return false;
                    ++altered;
                }
            }
            return altered <= 1;
        }
        case MotifTransformKind::OctaveShift:
        case MotifTransformKind::Reorchestrate:
            if (t.noteCount != base.noteCount) return false;
            if (t.registerOctaveOffset != base.registerOctaveOffset + param) return false;
            for (std::size_t i = 0; i < t.noteCount; ++i) {
                if (!(t.notes[i] == base.notes[i])) return false;
            }
            return true;
    }
    return false;
}


// --- 3. Harmony & Triad Voicing (INC3-R02) ---

TriadVoicing voiceTriad(const ChordSpec& chord, std::optional<std::int16_t> prevCorpoHarmonyDegree) noexcept {
    TriadVoicing v{};
    // Voice 0 in Corpo takes root in bass (octave -1)
    v.corpoBassDegree = static_cast<std::int16_t>(chord.rootDegree - 7);

    // Voice 1 in Corpo: minimal voice-leading leap from previous harmony degree
    if (prevCorpoHarmonyDegree.has_value()) {
        const int distFifth = std::abs(chord.fifthDegree - *prevCorpoHarmonyDegree);
        const int distThird = std::abs(chord.thirdDegree - *prevCorpoHarmonyDegree);
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
        if (!s.active) continue;
        if (!s.inRelease && s.noteOffFrame > 0 && s.noteOffFrame <= frame) {
            s.inRelease = true;
            s.expireFrame = core::saturatingAdd(s.noteOffFrame, kReleaseTailFrames);
        }
        if (s.inRelease && s.expireFrame <= frame) {
            s.active = false;
            s.inRelease = false;
        }
    }
}

std::uint32_t SimulatedVoiceTracker::activeCountForRole(MusicRole role, std::uint64_t frame) const noexcept {
    const auto rIdx = static_cast<std::size_t>(role);
    if (rIdx >= 4) return 0;
    std::uint32_t count = 0;
    for (std::size_t i = rIdx * kSlotsPerRole; i < (rIdx + 1) * kSlotsPerRole; ++i) {
        const auto& s = slots_[i];
        if (s.active && !s.inRelease && (s.noteOffFrame == 0 || s.noteOffFrame > frame)) ++count;
    }
    return count;
}

int SimulatedVoiceTracker::simulateNoteOn(MusicRole role, std::uint64_t eventId, std::uint64_t sampleFrame,
                                        std::uint32_t durationFrames, float pitchHz, bool& wasStolen) noexcept {
    const auto rIdx = static_cast<std::size_t>(role);
    wasStolen = false;
    if (rIdx >= 4) {
        return -1;
    }
    updateAtFrame(sampleFrame);

    const std::size_t s0 = rIdx * kSlotsPerRole;
    const std::size_t s1 = s0 + 1;
    const SimulatedVoiceSlot fresh{
        .active = true,
        .role = role,
        .eventId = eventId,
        .startFrame = sampleFrame,
        .noteOffFrame = (durationFrames > 0) ? core::saturatingAdd(sampleFrame, static_cast<std::uint64_t>(durationFrames)) : 0ULL,
        .expireFrame = 0,
        .pitchHz = pitchHz,
        .inRelease = false
    };

    // Rule 0: pick idle slot (slot 0 before 1)
    if (!slots_[s0].active) {
        slots_[s0] = fresh;
        return static_cast<int>(s0);
    }
    if (!slots_[s1].active) {
        slots_[s1] = fresh;
        return static_cast<int>(s1);
    }

    // Rule 1: steal among slots in release (amplitude unknown in simulation: oldest startFrame, then slot index)
    if (slots_[s0].inRelease || slots_[s1].inRelease) {
        std::size_t victim = slots_[s0].inRelease ? s0 : s1;
        if (slots_[s0].inRelease && slots_[s1].inRelease) {
            victim = (slots_[s0].startFrame <= slots_[s1].startFrame) ? s0 : s1;
        }
        slots_[victim] = fresh;
        wasStolen = true;
        return static_cast<int>(victim);
    }

    // Rule 2: both held -> steal oldest startFrame (slot 0 on ties)
    const std::size_t victim = (slots_[s0].startFrame <= slots_[s1].startFrame) ? s0 : s1;
    slots_[victim] = fresh;
    wasStolen = true;
    ++heldVoiceSteals_;
    return static_cast<int>(victim);
}

void SimulatedVoiceTracker::simulateNoteOff(MusicRole role, std::uint64_t eventId, std::uint64_t sampleFrame) noexcept {
    const auto rIdx = static_cast<std::size_t>(role);
    if (rIdx >= 4) return;
    for (std::size_t i = rIdx * kSlotsPerRole; i < (rIdx + 1) * kSlotsPerRole; ++i) {
        auto& s = slots_[i];
        if (s.active && !s.inRelease && s.eventId == eventId) {
            s.inRelease = true;
            s.noteOffFrame = sampleFrame;
            s.expireFrame = core::saturatingAdd(sampleFrame, kReleaseTailFrames);
        }
    }
}

void SimulatedVoiceTracker::reset() noexcept {
    slots_.fill(SimulatedVoiceSlot{});
    heldVoiceSteals_ = 0;
}


// --- 5. MusicPlanner Implementation ---

// Bar/step grid of one phrase, derived exclusively from the planning MusicClock.
struct MusicPlanner::PhraseGrid {
    const MusicClock* clock{nullptr};
    HardConstraintViolation violation{HardConstraintViolation::StructuralInvariantViolation};
    std::uint32_t barCount{0};
    std::array<std::uint64_t, kMaxPhraseBars> barBeat{};
    std::array<std::uint32_t, kMaxPhraseBars> stepsPerBeat{};
    std::array<std::uint32_t, kMaxPhraseBars> stepsPerBar{};
    std::array<std::uint64_t, kMaxPhraseBars + 1> barFrame{};

    [[nodiscard]] bool valid() const noexcept { return violation == HardConstraintViolation::None; }

    // Frame of grid step `s` of bar `b` (s may equal stepsPerBar[b] = next bar start)
    [[nodiscard]] std::uint64_t stepFrame(std::uint32_t b, std::uint32_t s) const noexcept {
        return clock->frameAtBeatStep(barBeat[b], s, stepsPerBeat[b]);
    }
    // Frame of the midpoint of step `s` of bar `b` (exact half-step grid)
    [[nodiscard]] std::uint64_t halfStepFrame(std::uint32_t b, std::uint32_t s) const noexcept {
        return clock->frameAtBeatStep(barBeat[b], 2ULL * s + 1ULL, 2U * stepsPerBeat[b]);
    }
};

namespace {

void buildGrid(const PhrasePlan& phrase, const MusicClock& clock, std::uint32_t sampleRate,
               std::uint64_t& outStartCheck, std::uint64_t& outEndCheck, bool& ok,
               std::array<std::uint64_t, kMaxPhraseBars>& barBeat,
               std::array<std::uint32_t, kMaxPhraseBars>& stepsPerBeat,
               std::array<std::uint32_t, kMaxPhraseBars>& stepsPerBar,
               std::array<std::uint64_t, kMaxPhraseBars + 1>& barFrame) noexcept {
    ok = false;
    if (clock.internalSampleRate() != sampleRate) return;
    if (phrase.barCount == 0 || phrase.barCount > kMaxPhraseBars) return;
    outStartCheck = clock.frameAtBar(phrase.startBar);
    outEndCheck = clock.frameAtBar(static_cast<std::uint64_t>(phrase.startBar) + phrase.barCount);
    if (phrase.startFrame != outStartCheck || phrase.endFrame != outEndCheck || phrase.endFrame <= phrase.startFrame) {
        return;
    }
    for (std::uint32_t b = 0; b < phrase.barCount; ++b) {
        const std::uint64_t bar = static_cast<std::uint64_t>(phrase.startBar) + b;
        const BarAnchor anchor = beatAtBar(clock, bar);
        const std::uint64_t spb = anchor.tempoMap.subdivisionsPerBeat;
        const std::uint64_t steps = spb * anchor.tempoMap.beatsPerBar;
        if (spb == 0 || steps == 0 || steps > kMaxStepsPerBar) return;
        barBeat[b] = anchor.beat;
        stepsPerBeat[b] = static_cast<std::uint32_t>(spb);
        stepsPerBar[b] = static_cast<std::uint32_t>(steps);
        barFrame[b] = clock.frameAtBar(bar);
        // Grid closure: beat anchor and the step grid must coincide with the clock's bar boundaries
        if (clock.frameAtBeat(anchor.beat) != barFrame[b]) return;
        if (clock.frameAtBeatStep(anchor.beat, steps, static_cast<std::uint32_t>(spb)) != clock.frameAtBar(bar + 1)) return;
    }
    barFrame[phrase.barCount] = outEndCheck;
    ok = true;
}

} // namespace

MusicPlanner::MusicPlanner(const MusicPlannerConfig& config) noexcept
    : config_(config) {
    if (config_.candidateCount < kMinCandidates) config_.candidateCount = static_cast<std::uint32_t>(kMinCandidates);
    if (config_.candidateCount > kMaxCandidates) config_.candidateCount = static_cast<std::uint32_t>(kMaxCandidates);
    if (config_.simulationHz == 0) config_.simulationHz = core::FixedStepConfig{}.simulationHz;
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
    plannedNoteOffs_.clear();
    voiceTracker_.reset();
    lastCorpoVoicing_.reset();
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

std::optional<MusicClock> MusicPlanner::makePlannerProbeV1Clock() noexcept {
    const FormPlan form = makePlannerProbeV1Form();
    auto clockOpt = MusicClock::create(48000, form.sections[0].tempoMap);
    if (!clockOpt.has_value()) return std::nullopt;
    const std::uint64_t boundary = clockOpt->frameAtBar(form.sections[1].startBar);
    if (!clockOpt->setTempoMap(form.sections[1].tempoMap, boundary)) return std::nullopt;
    return clockOpt;
}

PhrasePlan MusicPlanner::makePhrasePlan(const MusicClock& clock, const SectionPlan& section,
                                        std::uint32_t phraseIndex, const std::array<ChordSpec, 4>& chords,
                                        CadenceGoal cadenceGoal) noexcept {
    const std::uint32_t barsPerPhrase = section.tempoMap.barsPerPhrase;
    const std::uint32_t startBar = section.startBar + phraseIndex * barsPerPhrase;
    return PhrasePlan{
        .sectionIndex = section.sectionIndex,
        .phraseIndex = phraseIndex,
        .startBar = startBar,
        .barCount = barsPerPhrase,
        .startFrame = clock.frameAtBar(startBar),
        .endFrame = clock.frameAtBar(static_cast<std::uint64_t>(startBar) + barsPerPhrase),
        .scale = section.scale,
        .chords = chords,
        .targetTension = section.baseTension,
        .dominantRole = MusicRole::Vettore,
        .cadenceGoal = cadenceGoal,
        .baseMotifId = section.sectionMotif.motifId
    };
}

std::array<PhrasePlan, 2> MusicPlanner::makePlannerProbeV1Phrases(const MusicClock& clock) noexcept {
    const FormPlan form = makePlannerProbeV1Form();
    // Section A: one chord per bar (Corpo re-voiced each bar with voice leading)
    const std::array<ChordSpec, 4> chordsA{
        ChordSpec{0, 2, 4, HarmonicFunction::Stability, false},
        ChordSpec{3, 5, 0, HarmonicFunction::Preparation, false},
        ChordSpec{4, 6, 1, HarmonicFunction::Tension, false},
        ChordSpec{0, 2, 4, HarmonicFunction::Resolution, false}
    };
    // Section B: open Corpo pad on the first chord, explicit planner NoteOff at the phrase end
    const std::array<ChordSpec, 4> chordsB{
        ChordSpec{0, 2, 4, HarmonicFunction::Stability, true},
        ChordSpec{5, 0, 2, HarmonicFunction::Preparation, true},
        ChordSpec{6, 1, 3, HarmonicFunction::Tension, true},
        ChordSpec{0, 2, 4, HarmonicFunction::Resolution, true}
    };
    return {
        makePhrasePlan(clock, form.sections[0], 0, chordsA, CadenceGoal::Sustain),
        makePhrasePlan(clock, form.sections[1], 0, chordsB, CadenceGoal::ModalResolution)
    };
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

Motif MusicPlanner::resolveBaseMotif(std::uint32_t baseMotifId, const ModalScale& scale) noexcept {
    const Motif dorian = makeDorianBaseMotif();
    const Motif aeolian = makeAeolianBaseMotif();
    if (baseMotifId == dorian.motifId) return dorian;
    if (baseMotifId == aeolian.motifId) return aeolian;
    return (scale.mode == ScaleMode::Aeolian) ? aeolian : dorian;
}

namespace {

struct CandidateRecipe {
    MotifTransformKind kind{MotifTransformKind::Identity};
    std::int16_t param{0};
};

// Fixed bounded recipe table for candidate k in [0, kMaxCandidates)
constexpr std::array<CandidateRecipe, kMaxCandidates> kCandidateRecipes{{
    {MotifTransformKind::Identity, 0},
    {MotifTransformKind::ModalTranspose, 2},
    {MotifTransformKind::ContourInversion, 2},
    {MotifTransformKind::EuclideanRhythmVariation, 3},  // param = rotation
    {MotifTransformKind::ChromaticAlteration, 1},       // param = semitone delta on the first non-anchor note
    {MotifTransformKind::OctaveShift, 1},               // param = octave delta
    {MotifTransformKind::ModalTranspose, -2},
    {MotifTransformKind::ContourInversion, 4},
}};

// Trama note length in grid steps: staccato, legato, half-bar sustain (may cross the bar line, capped at the phrase end)
constexpr std::array<std::uint32_t, 3> kTramaSustainSteps{1, 2, 8};

Motif applyRecipe(const Motif& base, const CandidateRecipe& r) noexcept {
    switch (r.kind) {
        case MotifTransformKind::Identity:
            return base;
        case MotifTransformKind::ModalTranspose:
            return applyModalTranspose(base, r.param);
        case MotifTransformKind::ContourInversion:
            return applyContourInversion(base, r.param);
        case MotifTransformKind::EuclideanRhythmVariation:
            return applyEuclideanRhythmVariation(base, base.noteCount, r.param);
        case MotifTransformKind::ChromaticAlteration: {
            std::uint8_t idx = 0;
            while (idx < base.noteCount && base.notes[idx].isAnchor) ++idx;
            return applyChromaticAlteration(base, idx, static_cast<std::int8_t>(r.param));
        }
        case MotifTransformKind::OctaveShift:
        case MotifTransformKind::Reorchestrate:
            return applyReorchestration(base, MusicRole::Vettore, static_cast<std::int8_t>(r.param));
    }
    return base;
}

// Appends a NoteOn to the candidate without ever truncating silently: requestedEventCount always counts.
void pushEvent(PhraseCandidate& cand, const CandidateEvent& e) noexcept {
    if (cand.eventCount < kMaxEventsPerPhrase) {
        cand.events[cand.eventCount++] = e;
    }
    ++cand.requestedEventCount;
}

CandidateEvent makeEvent(std::uint64_t frame, std::uint64_t offFrame, MusicRole role, std::int32_t midi,
                         float velocity, float timbre, bool openPad) noexcept {
    return CandidateEvent{
        .localEventId = 0,
        .sampleFrame = frame,
        .role = role,
        .midiNote = static_cast<std::int16_t>(std::clamp(midi, -1, 128)),
        .pitchHz = midiNoteToPitchHz(midi),
        .velocity = velocity,
        .durationFrames = openPad ? 0u : static_cast<std::uint32_t>(std::min<std::uint64_t>(
            (offFrame > frame) ? (offFrame - frame) : 0ULL, 0xFFFFFFFFULL)),
        .timbreParam = timbre,
        .isOpenPad = openPad
    };
}

void canonicalizeEvents(PhraseCandidate& cand) noexcept {
    std::sort(cand.events.begin(), cand.events.begin() + static_cast<std::ptrdiff_t>(cand.eventCount), canonicalEventLess);
    for (std::size_t i = 0; i < cand.eventCount; ++i) {
        cand.events[i].localEventId = static_cast<std::uint32_t>(i);
    }
}

std::int32_t voicingDistance(const TriadVoicing& a, const TriadVoicing& b) noexcept {
    return absI32(a.corpoBassDegree - b.corpoBassDegree) + absI32(a.corpoHarmonyDegree - b.corpoHarmonyDegree);
}

} // namespace

void MusicPlanner::generateCandidates(const PhrasePlan& phrase,
                                     const PhraseGrid& grid,
                                     const Motif& baseMotif,
                                     std::vector<PhraseCandidate>& outCandidates) const {
    outCandidates.clear();
    outCandidates.reserve(config_.candidateCount);

    const auto& prof = kPlannerScoringProfileV1;
    const bool openPadPhrase = phrase.chords[0].openPadSustain;
    const std::int32_t pulsoMidi = degreeToMidiNote(phrase.scale, -14);

    std::int32_t anchorsTotal = 0;
    for (std::size_t i = 0; i < baseMotif.noteCount; ++i) {
        if (baseMotif.notes[i].isAnchor) ++anchorsTotal;
    }

    for (std::uint32_t k = 0; k < config_.candidateCount; ++k) {
        PhraseCandidate cand{};
        const auto& recipe = kCandidateRecipes[k % kMaxCandidates];
        cand.candidateIndex = static_cast<std::uint8_t>(k);
        cand.candidateId = packCandidateId(phrase.sectionIndex, phrase.phraseIndex, cand.candidateIndex);
        cand.transformKind = recipe.kind;
        cand.transformParam = recipe.param;
        cand.transformedMotif = applyRecipe(baseMotif, recipe);
        cand.transformedMotif.motifId = deriveTransformedMotifId(baseMotif.motifId, recipe.kind, k);
        cand.voicingPolicy = (k % 2 == 0) ? CorpoVoicingPolicy::VoiceLed : CorpoVoicingPolicy::RootFifth;
        cand.pulsoPulses = 4 + (k % 3);                 // 4, 5 or 6 pulses per bar
        cand.pulsoRotation = static_cast<std::int32_t>(k % 4);
        cand.tramaPulses = 6 + ((k + 1) % 3);           // 6, 7 or 8 pulses per bar
        cand.tramaRotation = static_cast<std::int32_t>((k + 2) % 4);
        cand.tramaSustainSteps = kTramaSustainSteps[(k / 2) % kTramaSustainSteps.size()];

        bool structuralOk = true;
        std::optional<TriadVoicing> prevVoicing = lastCorpoVoicing_;
        std::int32_t vlDistance = 0;
        std::int32_t harmonicTensionSum = 0;
        std::uint32_t melodicRhythmicOnsets = 0;

        for (std::uint32_t b = 0; b < grid.barCount; ++b) {
            const std::uint32_t steps = grid.stepsPerBar[b];
            const ChordSpec& chord = phrase.chords[b % 4];
            harmonicTensionSum += prof.harmonicFunctionTensionMilli[static_cast<std::size_t>(chord.function) % 4];

            // Per-bar voicing chain (voice leading from the previous chord onset when policy says so)
            const std::optional<std::int16_t> prevHarmony = (cand.voicingPolicy == CorpoVoicingPolicy::VoiceLed && prevVoicing.has_value())
                ? std::optional<std::int16_t>(prevVoicing->corpoHarmonyDegree) : std::nullopt;
            const TriadVoicing barVoicing = voiceTriad(chord, prevHarmony);
            if (b == 0) cand.voicing = barVoicing;

            const auto pulsoRhythm = core::euclideanRhythm(steps, cand.pulsoPulses, cand.pulsoRotation);
            const auto tramaRhythm = core::euclideanRhythm(steps, cand.tramaPulses, cand.tramaRotation);

            // 1. Pulso: Bjorklund grid, half-step hits
            for (std::uint32_t s = 0; s < steps; ++s) {
                if (!pulsoRhythm[s]) continue;
                pushEvent(cand, makeEvent(grid.stepFrame(b, s), grid.halfStepFrame(b, s), MusicRole::Pulso, pulsoMidi,
                                          (s == 0) ? 0.95f : 0.75f, 0.30f + 0.05f * static_cast<float>(k % 4), false));
                ++melodicRhythmicOnsets;
            }

            // 2. Corpo: one chord per bar (lasting until the next chord onset), or one open pad per phrase
            const bool emitCorpo = !openPadPhrase || b == 0;
            if (emitCorpo) {
                const std::uint64_t onFrame = grid.barFrame[b];
                const std::uint64_t offFrame = grid.barFrame[b + 1];
                pushEvent(cand, makeEvent(onFrame, offFrame, MusicRole::Corpo, degreeToMidiNote(phrase.scale, barVoicing.corpoBassDegree),
                                          0.85f, 0.40f, openPadPhrase));
                pushEvent(cand, makeEvent(onFrame, offFrame, MusicRole::Corpo, degreeToMidiNote(phrase.scale, barVoicing.corpoHarmonyDegree),
                                          0.75f, 0.40f, openPadPhrase));
                if (prevVoicing.has_value()) vlDistance += voicingDistance(barVoicing, *prevVoicing);
                cand.lastCorpoVoicing = barVoicing;
            }
            // The voicing chain follows the sounding Corpo harmony
            prevVoicing = emitCorpo ? std::optional<TriadVoicing>(barVoicing) : prevVoicing;

            // 3. Trama: complementary syncopation, colour tones of the current bar's chord
            for (std::uint32_t s = 0; s < steps; ++s) {
                if (!tramaRhythm[s] || pulsoRhythm[s]) continue;
                const std::int16_t deg = (s % 2 == 0) ? barVoicing.upperColorDegree : barVoicing.corpoHarmonyDegree;
                const std::uint64_t offFrame = std::min(grid.stepFrame(b, s + cand.tramaSustainSteps), phrase.endFrame);
                pushEvent(cand, makeEvent(grid.stepFrame(b, s), offFrame, MusicRole::Trama,
                                          degreeToMidiNote(phrase.scale, deg + 7), 0.70f, 0.50f, false));
                ++melodicRhythmicOnsets;
            }

            // 4. Vettore: transformed motif (register offset applied in octaves)
            const Motif& motif = cand.transformedMotif;
            for (std::size_t m = 0; m < motif.noteCount; ++m) {
                const auto& note = motif.notes[m];
                if (note.stepOffset >= steps || note.durationSteps == 0) {
                    structuralOk = false;
                    continue;
                }
                const std::uint32_t offStep = std::min<std::uint32_t>(note.stepOffset + note.durationSteps, steps);
                const std::int32_t degree = note.scaleDegree + 7 * motif.registerOctaveOffset;
                pushEvent(cand, makeEvent(grid.stepFrame(b, note.stepOffset), grid.stepFrame(b, offStep), MusicRole::Vettore,
                                          degreeToMidiNote(phrase.scale, degree, note.chromaticOffset),
                                          note.velocity, 0.60f, false));
                ++melodicRhythmicOnsets;
            }
        }

        canonicalizeEvents(cand);
        cand.violation = structuralOk ? HardConstraintViolation::None : HardConstraintViolation::StructuralInvariantViolation;

        // Integer features (all deterministic, no float)
        auto& f = cand.features;
        f.voiceLeadingDistance = vlDistance;

        const Motif& motif = cand.transformedMotif;
        std::int32_t registerSum = 0;
        for (std::size_t m = 0; m < motif.noteCount; ++m) {
            registerSum += motif.notes[m].scaleDegree + 7 * motif.registerOctaveOffset;
        }
        const std::int32_t registerMilli = (motif.noteCount > 0)
            ? (registerSum * prof.tensionPerRegisterDegreeMilli) / static_cast<std::int32_t>(motif.noteCount) : 0;
        const std::int32_t extraPulses = static_cast<std::int32_t>(cand.pulsoPulses + cand.tramaPulses) -
                                         static_cast<std::int32_t>(prof.densityBaselinePulses);
        const std::int32_t densityMilli = std::max(0, extraPulses) * prof.tensionPerExtraPulseMilli;
        const std::int32_t harmonicMilli = (grid.barCount > 0) ? harmonicTensionSum / static_cast<std::int32_t>(grid.barCount) : 0;
        f.tensionEstimateMilli = std::clamp(
            prof.transformTensionMilli[static_cast<std::size_t>(cand.transformKind) % kMotifTransformKindCount] +
            densityMilli + registerMilli + harmonicMilli, 0, 1000);

        std::int32_t anchorsPreserved = 0;
        for (std::size_t i = 0; i < baseMotif.noteCount; ++i) {
            if (!baseMotif.notes[i].isAnchor || i >= motif.noteCount) continue;
            const auto& t = motif.notes[i];
            if (t.stepOffset == baseMotif.notes[i].stepOffset && t.chromaticOffset == 0 && isTonicTriadDegree(t.scaleDegree)) {
                ++anchorsPreserved;
            }
        }
        f.anchorFidelityMilli = (anchorsTotal > 0) ? (anchorsPreserved * 1000) / anchorsTotal : 1000;
        f.onsetDensityMilli = (grid.barCount > 0)
            ? static_cast<std::int32_t>((melodicRhythmicOnsets * 1000U) / grid.barCount) : 0;

        outCandidates.push_back(cand);
    }
}

void MusicPlanner::buildFallbackLevel(FallbackKind level,
                                      const PhrasePlan& phrase,
                                      const PhraseGrid& grid,
                                      const Motif& baseMotif,
                                      std::uint64_t committedHorizonFrame,
                                      PhraseCandidate& out) const noexcept {
    out = PhraseCandidate{};
    out.candidateIndex = static_cast<std::uint8_t>(kFallbackCandidateIndexBase + static_cast<std::uint8_t>(level));
    out.candidateId = packCandidateId(phrase.sectionIndex, phrase.phraseIndex, out.candidateIndex);
    out.transformKind = MotifTransformKind::Identity;
    out.transformedMotif = baseMotif;
    out.violation = HardConstraintViolation::None;

    if (level == FallbackKind::SilentRecovery) {
        return; // L3: zero events, always admissible
    }
    if (!grid.valid()) {
        out.violation = grid.violation;
        return;
    }

    const std::uint64_t safeStart = std::max(phrase.startFrame, committedHorizonFrame);

    if (level == FallbackKind::RarefiedMotif) {
        // L1: Vettore motif anchors + Pulso downbeats, only at/after the committed horizon
        const std::int32_t pulsoMidi = degreeToMidiNote(phrase.scale, -14);
        for (std::uint32_t b = 0; b < grid.barCount; ++b) {
            const std::uint32_t steps = grid.stepsPerBar[b];
            const std::uint64_t downbeat = grid.stepFrame(b, 0);
            if (downbeat >= safeStart) {
                pushEvent(out, makeEvent(downbeat, grid.halfStepFrame(b, 0), MusicRole::Pulso, pulsoMidi, 0.80f, 0.20f, false));
            }
            for (std::size_t m = 0; m < baseMotif.noteCount; ++m) {
                const auto& note = baseMotif.notes[m];
                if (!note.isAnchor || note.stepOffset >= steps || note.durationSteps == 0) continue;
                const std::uint64_t on = grid.stepFrame(b, note.stepOffset);
                if (on < safeStart) continue;
                const std::uint32_t offStep = std::min<std::uint32_t>(note.stepOffset + note.durationSteps, steps);
                pushEvent(out, makeEvent(on, grid.stepFrame(b, offStep), MusicRole::Vettore,
                                         degreeToMidiNote(phrase.scale, note.scaleDegree + 7 * baseMotif.registerOctaveOffset, note.chromaticOffset),
                                         note.velocity, 0.40f, false));
            }
        }
    } else if (level == FallbackKind::PadSustainOnly) {
        // L2: Corpo root + fifth sustained from the horizon to the phrase end
        if (safeStart < phrase.endFrame) {
            const TriadVoicing v = voiceTriad(phrase.chords[0]);
            pushEvent(out, makeEvent(safeStart, phrase.endFrame, MusicRole::Corpo, degreeToMidiNote(phrase.scale, v.corpoBassDegree), 0.70f, 0.20f, false));
            pushEvent(out, makeEvent(safeStart, phrase.endFrame, MusicRole::Corpo, degreeToMidiNote(phrase.scale, v.corpoHarmonyDegree), 0.60f, 0.20f, false));
            out.voicing = v;
            out.lastCorpoVoicing = v;
        }
    }
    canonicalizeEvents(out);
}

HardConstraintViolation MusicPlanner::validateHardConstraints(
    const PhraseCandidate& candidate,
    const PhrasePlan& phrase,
    std::uint64_t committedHorizonFrame,
    bool allowEmpty
) const noexcept {
    if (candidate.violation != HardConstraintViolation::None) {
        return candidate.violation; // Structural defect recorded at generation time
    }
    // Capacity: a candidate exceeding capacity is rejected, never truncated
    if (candidate.requestedEventCount > kMaxEventsPerPhrase || candidate.eventCount > kMaxEventsPerPhrase ||
        candidate.requestedEventCount != candidate.eventCount) {
        return HardConstraintViolation::SpscCapacityExceeded;
    }
    if (candidate.eventCount == 0) {
        return allowEmpty ? HardConstraintViolation::None : HardConstraintViolation::StructuralInvariantViolation;
    }

    const std::uint64_t earliest = std::max(phrase.startFrame, committedHorizonFrame);
    std::array<std::uint64_t, 2 * kMaxEventsPerPhrase> spscFrames{};
    std::size_t spscCount = 0;

    for (std::size_t i = 0; i < candidate.eventCount; ++i) {
        const auto& e = candidate.events[i];
        if (static_cast<std::size_t>(e.role) >= 4) {
            return HardConstraintViolation::InvalidParameters;
        }
        if (e.midiNote < 0 || e.midiNote > 127) {
            return HardConstraintViolation::InvalidParameters;
        }
        if (!std::isfinite(e.pitchHz) || e.pitchHz < 20.0f || e.pitchHz > 16000.0f) {
            return HardConstraintViolation::InvalidParameters;
        }
        if (!std::isfinite(e.velocity) || e.velocity < 0.0f || e.velocity > 1.0f) {
            return HardConstraintViolation::InvalidParameters;
        }
        if (!std::isfinite(e.timbreParam) || e.timbreParam < 0.0f || e.timbreParam > 1.0f) {
            return HardConstraintViolation::InvalidParameters;
        }
        if (e.sampleFrame < earliest || e.sampleFrame >= phrase.endFrame) {
            return HardConstraintViolation::CommitmentHorizonViolation;
        }
        if (e.isOpenPad) {
            // Open pads: Corpo only, closed by the explicit planner NoteOff at the phrase end
            if (e.durationFrames != 0 || e.role != MusicRole::Corpo) {
                return HardConstraintViolation::InvalidParameters;
            }
        } else {
            if (e.durationFrames == 0) {
                return HardConstraintViolation::InvalidParameters;
            }
            if (e.sampleFrame + e.durationFrames > phrase.endFrame) {
                return HardConstraintViolation::CommitmentHorizonViolation;
            }
        }
        spscFrames[spscCount++] = e.sampleFrame;
        spscFrames[spscCount++] = e.isOpenPad ? phrase.endFrame : (e.sampleFrame + e.durationFrames);
    }

    // Simultaneous onsets: max 2 NoteOns per role at exact same sampleFrame (events are canonically sorted)
    for (std::size_t i = 0; i < candidate.eventCount; ++i) {
        std::uint32_t sameRoleSameFrame = 0;
        for (std::size_t j = i; j < candidate.eventCount && candidate.events[j].sampleFrame == candidate.events[i].sampleFrame; ++j) {
            if (candidate.events[j].role == candidate.events[i].role) ++sameRoleSameFrame;
        }
        if (sameRoleSameFrame > 2) {
            return HardConstraintViolation::SimultaneousOnsetLimitExceeded;
        }
    }

    // SPSC density: NoteOn + NoteOff events within any lookahead window [t, t + lookahead) <= 128
    std::sort(spscFrames.begin(), spscFrames.begin() + static_cast<std::ptrdiff_t>(spscCount));
    const std::uint64_t window = std::max<std::uint64_t>(config_.lookaheadFrames, 1);
    std::size_t lo = 0;
    for (std::size_t hi = 0; hi < spscCount; ++hi) {
        while (spscFrames[hi] - spscFrames[lo] >= window) ++lo;
        if (hi - lo + 1 > kMaxSpscEventsPerLookaheadWindow) {
            return HardConstraintViolation::SpscCapacityExceeded;
        }
    }

    return HardConstraintViolation::None;
}

std::uint32_t MusicPlanner::simulateCandidate(const PhraseCandidate& candidate,
                                              const PhrasePlan& phrase,
                                              std::uint64_t firstEventId,
                                              SimulatedVoiceTracker& tracker) const noexcept {
    const std::uint32_t before = tracker.heldVoiceSteals();
    // NoteOns in canonical (time) order; nominal NoteOffs are advanced lazily by the tracker at each onset.
    for (std::size_t i = 0; i < candidate.eventCount; ++i) {
        const auto& e = candidate.events[i];
        bool stolen = false;
        (void)tracker.simulateNoteOn(e.role, firstEventId + i, e.sampleFrame, e.durationFrames, e.pitchHz, stolen);
    }
    // Explicit open-pad NoteOffs at the phrase end (after every onset of the phrase, which are < endFrame)
    for (std::size_t i = 0; i < candidate.eventCount; ++i) {
        const auto& e = candidate.events[i];
        if (e.isOpenPad) {
            tracker.simulateNoteOff(e.role, firstEventId + i, phrase.endFrame);
        }
    }
    return tracker.heldVoiceSteals() - before;
}

std::int32_t MusicPlanner::scoreCandidateFixedPoint(
    const PhraseCandidate& candidate,
    std::int32_t targetTensionMilli
) const noexcept {
    const auto& prof = kPlannerScoringProfileV1;
    const auto& f = candidate.features;

    // 1. Voice leading continuity (0..1000)
    const std::int32_t sVl = std::clamp(1000 - f.voiceLeadingDistance * prof.voiceLeadingPenaltyPerDegree, 0, 1000);
    // 2. Tension adherence (0..1000)
    const std::int32_t sTen = std::clamp(1000 - absI32(f.tensionEstimateMilli - targetTensionMilli), 0, 1000);
    // 3. Motif identity: anchor fidelity (0..1000)
    const std::int32_t sMot = std::clamp(f.anchorFidelityMilli, 0, 1000);
    // 4. Rhythmic density balance (0..1000)
    const std::int32_t densityDevMilli = absI32(f.onsetDensityMilli - prof.targetOnsetsPerBarMilli);
    const std::int32_t sRhy = std::clamp(1000 - (densityDevMilli * prof.rhythmPenaltyPerOnset) / 1000, 0, 1000);

    const std::int64_t total =
        static_cast<std::int64_t>(prof.weightVoiceLeading) * sVl +
        static_cast<std::int64_t>(prof.weightTension) * sTen +
        static_cast<std::int64_t>(prof.weightMotif) * sMot +
        static_cast<std::int64_t>(prof.weightRhythm) * sRhy -
        static_cast<std::int64_t>(prof.penaltyPerSteal) * static_cast<std::int64_t>(candidate.simulatedStealsCount);
    return static_cast<std::int32_t>(std::clamp<std::int64_t>(total, 0, 100000));
}

std::uint64_t MusicPlanner::tieBreakHash(std::uint64_t tieBreakDecisionId, std::uint64_t candidateId) const noexcept {
    // sampleIndex: low 32 bits of the candidate id (unique within one decision); the decision id already
    // binds section/phrase/bar/slot/policy. Explicit policy version (ADR-0002), never the default argument.
    const core::RandomKey key{
        config_.runSeed,
        static_cast<std::uint32_t>(core::StreamId::MusicForm),
        tieBreakDecisionId,
        static_cast<std::uint32_t>(candidateId & 0xFFFFFFFFULL)
    };
    return core::randomU64(key, config_.policyVersion.toU32());
}

PhraseCandidate MusicPlanner::selectWinningCandidate(
    std::vector<PhraseCandidate>& candidates,
    std::uint64_t tieBreakDecisionId
) const noexcept {
    for (auto& cand : candidates) {
        cand.tieBreakHash = tieBreakHash(tieBreakDecisionId, cand.candidateId);
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

void MusicPlanner::planPhrase(const PhrasePlan& phrase,
                             const MusicClock& clock,
                             std::uint64_t committedHorizonFrame,
                             std::uint64_t currentTransportEpoch,
                             std::vector<MusicIntentEvent>& outIntents,
                             AudioScheduler* scheduler) {
    const std::uint32_t policy = config_.policyVersion.toU32();
    PhraseDecisionTrace trace{};
    trace.decisionId = core::deriveDecisionId(phrase.sectionIndex, phrase.phraseIndex, phrase.startBar, kPhraseDecisionSlot, policy);
    trace.tieBreakDecisionId = core::deriveDecisionId(phrase.sectionIndex, phrase.phraseIndex, phrase.startBar, kTieBreakDecisionSlot, policy);
    trace.runSeed = config_.runSeed;
    trace.policyVersion = policy;
    trace.scoringProfileVersion = kPlannerScoringProfileV1.versionId;
    trace.sectionIndex = phrase.sectionIndex;
    trace.phraseIndex = phrase.phraseIndex;
    trace.startBar = phrase.startBar;
    trace.firstEventId = nextEventId_;

    // Grid & plan invariants (derived from the planning clock only)
    PhraseGrid grid{};
    grid.clock = &clock;
    {
        std::uint64_t startCheck = 0;
        std::uint64_t endCheck = 0;
        bool ok = false;
        buildGrid(phrase, clock, config_.sampleRate, startCheck, endCheck, ok,
                  grid.barBeat, grid.stepsPerBeat, grid.stepsPerBar, grid.barFrame);
        const auto tensionMilli = quantizeTensionMilli(phrase.targetTension);
        if (ok && tensionMilli.has_value()) {
            grid.violation = HardConstraintViolation::None;
            grid.barCount = phrase.barCount;
            trace.targetTensionMilli = *tensionMilli;
        }
    }
    trace.planViolation = grid.violation;

    Motif baseMotif = resolveBaseMotif(phrase.baseMotifId, phrase.scale);
    if (phrase.sectionIndex < formPlan_.sectionCount &&
        formPlan_.sections[phrase.sectionIndex].sectionMotif.motifId == phrase.baseMotifId &&
        formPlan_.sections[phrase.sectionIndex].sectionMotif.noteCount > 0) {
        baseMotif = formPlan_.sections[phrase.sectionIndex].sectionMotif;
    }
    trace.baseMotifId = baseMotif.motifId;
    trace.baseMotif = baseMotif;

    std::vector<PhraseCandidate> candidates;
    if (grid.valid()) {
        generateCandidates(phrase, grid, baseMotif, candidates);
    }

    // Hard constraints first, then cross-phrase steal simulation on a tracker copy, then integer scoring
    std::vector<PhraseCandidate> validCandidates;
    std::vector<SimulatedVoiceTracker> validTrackers;
    validCandidates.reserve(candidates.size());
    validTrackers.reserve(candidates.size());
    for (auto& cand : candidates) {
        cand.violation = validateHardConstraints(cand, phrase, committedHorizonFrame, false);
        if (cand.violation == HardConstraintViolation::None) {
            SimulatedVoiceTracker trackerCopy = voiceTracker_;
            cand.simulatedStealsCount = simulateCandidate(cand, phrase, nextEventId_, trackerCopy);
            cand.features.simulatedSteals = cand.simulatedStealsCount;
            cand.scoreFixedPoint = scoreCandidateFixedPoint(cand, trace.targetTensionMilli);
            validCandidates.push_back(cand);
            validTrackers.push_back(trackerCopy);
        }
    }

    PhraseCandidate winner{};
    SimulatedVoiceTracker winnerTracker = voiceTracker_;

    if (!validCandidates.empty()) {
        winner = selectWinningCandidate(validCandidates, trace.tieBreakDecisionId);
        for (std::size_t i = 0; i < validCandidates.size(); ++i) {
            if (validCandidates[i].candidateId == winner.candidateId) {
                winnerTracker = validTrackers[i];
            }
            if (validCandidates[i].scoreFixedPoint == winner.scoreFixedPoint) {
                ++trace.tiedTopCandidates;
            }
        }
        // Copy tie-break hashes back for the trace
        for (auto& cand : candidates) {
            for (const auto& v : validCandidates) {
                if (v.candidateId == cand.candidateId) {
                    cand.tieBreakHash = v.tieBreakHash;
                    cand.simulatedStealsCount = v.simulatedStealsCount;
                    cand.scoreFixedPoint = v.scoreFixedPoint;
                }
            }
        }
    } else {
        // Ordered fail-closed fallback ladder: each level validated with the same hard constraints
        trace.fallbackTriggered = true;
        ++totalFallbacks_;
        constexpr std::array<FallbackKind, kFallbackLevelCount> kLadder{
            FallbackKind::RarefiedMotif, FallbackKind::PadSustainOnly, FallbackKind::SilentRecovery};
        for (std::size_t lvl = 0; lvl < kLadder.size(); ++lvl) {
            PhraseCandidate fb{};
            buildFallbackLevel(kLadder[lvl], phrase, grid, baseMotif, committedHorizonFrame, fb);
            const bool allowEmpty = (kLadder[lvl] == FallbackKind::SilentRecovery);
            fb.violation = validateHardConstraints(fb, phrase, committedHorizonFrame, allowEmpty);
            trace.fallbackLevelViolations[lvl] = fb.violation;
            trace.fallbackLevelsAttempted = static_cast<std::uint8_t>(lvl + 1);
            if (fb.violation == HardConstraintViolation::None) {
                SimulatedVoiceTracker trackerCopy = voiceTracker_;
                fb.simulatedStealsCount = simulateCandidate(fb, phrase, nextEventId_, trackerCopy);
                fb.features.simulatedSteals = fb.simulatedStealsCount;
                winner = fb;
                winnerTracker = trackerCopy;
                trace.fallbackKind = kLadder[lvl];
                break;
            }
        }
    }

    // Trace: every generated candidate (fixed-size array)
    trace.candidatesGenerated = static_cast<std::uint32_t>(candidates.size());
    trace.candidatesValid = static_cast<std::uint32_t>(validCandidates.size());
    for (std::size_t i = 0; i < candidates.size() && i < kMaxCandidates; ++i) {
        const auto& c = candidates[i];
        trace.candidates[i] = CandidateTraceEntry{
            .candidateId = c.candidateId,
            .transformKind = c.transformKind,
            .transformParam = c.transformParam,
            .motifId = c.transformedMotif.motifId,
            .violation = c.violation,
            .score = c.scoreFixedPoint,
            .tieBreakHash = c.tieBreakHash,
            .simulatedSteals = c.simulatedStealsCount,
            .eventCount = static_cast<std::uint32_t>(c.requestedEventCount),
            .features = c.features
        };
    }
    trace.winningCandidateId = winner.candidateId;
    trace.winningScore = winner.scoreFixedPoint;
    trace.winningTransformKind = winner.transformKind;
    trace.winningTransformParam = winner.transformParam;
    trace.winningMotif = winner.transformedMotif;

    // Normative M1 frame -> tick mapping (ceil: first tick whose boundary reaches the frame, ADR-0002)
    const core::FixedStepAccumulator tickMap{core::FixedStepConfig{
        .internalSampleRate = config_.sampleRate, .simulationHz = config_.simulationHz}};

    // Commit the winner: canonical order -> sequential canonical eventIds (INC3-R05)
    for (std::size_t i = 0; i < winner.eventCount; ++i) {
        const auto& ce = winner.events[i];
        const std::uint64_t finalId = nextEventId_++;

        const MusicIntentEvent intent{
            .eventId = finalId,
            .sampleFrame = ce.sampleFrame,
            .tick = tickMap.sampleFrameToTick(ce.sampleFrame),
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
        }

        if (ce.isOpenPad) {
            // Explicit matching NoteOff at the phrase boundary (INC3-R01, INC3-R03)
            const PlannedNoteOff off{.eventId = finalId, .role = ce.role, .sampleFrame = phrase.endFrame,
                                     .transportEpoch = currentTransportEpoch};
            plannedNoteOffs_.push_back(off);
            ++trace.plannerNoteOffCount;
            if (scheduler != nullptr) {
                scheduler->scheduleNoteOff(off.eventId, off.role, off.sampleFrame, off.transportEpoch);
            }
        }
    }

    // Simulated voice state advances only with the committed winner, independent of the scheduler pointer
    voiceTracker_ = winnerTracker;
    if (winner.lastCorpoVoicing.has_value()) {
        lastCorpoVoicing_ = winner.lastCorpoVoicing;
    }

    trace.finalEventCount = winner.eventCount;
    traces_.push_back(trace);
}

} // namespace cymatica::audio
