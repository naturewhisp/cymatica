#include "plan_canonical_hash.h"

#include <bit>

namespace cymatica::audio {

namespace {

// Canonical little-endian serialization + FNV-1a 64 (format version kCanonicalPlanHashFormatVersion)
struct Fnv1a64 {
    std::uint64_t h{0xcbf29ce484222325ULL};
    void byte(std::uint8_t b) noexcept { h ^= b; h *= 0x100000001b3ULL; }
    void u64(std::uint64_t v) noexcept { for (int i = 0; i < 8; ++i) byte(static_cast<std::uint8_t>(v >> (8 * i))); }
    void u32(std::uint32_t v) noexcept { for (int i = 0; i < 4; ++i) byte(static_cast<std::uint8_t>(v >> (8 * i))); }
    void i32(std::int32_t v) noexcept { u32(static_cast<std::uint32_t>(v)); }
    void f32(float v) noexcept { u32(std::bit_cast<std::uint32_t>(v)); }
};

} // namespace

std::uint64_t canonicalPlanHash(const std::vector<std::vector<MusicIntentEvent>>& intentsPerPhrase,
                                const std::vector<PlannedNoteOff>& noteOffs,
                                const std::vector<PhraseDecisionTrace>& traces) noexcept {
    Fnv1a64 f;
    for (const auto& phrase : intentsPerPhrase) {
        f.u64(phrase.size());
        for (const auto& e : phrase) {
            f.u64(e.eventId); f.u64(e.sampleFrame); f.u64(e.tick); f.u64(e.transportEpoch);
            f.byte(static_cast<std::uint8_t>(e.role)); f.f32(e.pitchHz); f.f32(e.velocity);
            f.u32(e.durationFrames); f.f32(e.timbreParam);
        }
    }
    f.u64(noteOffs.size());
    for (const auto& o : noteOffs) {
        f.u64(o.eventId); f.byte(static_cast<std::uint8_t>(o.role)); f.u64(o.sampleFrame); f.u64(o.transportEpoch);
    }
    f.u64(traces.size());
    for (const auto& t : traces) {
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

} // namespace cymatica::audio
