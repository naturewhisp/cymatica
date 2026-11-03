#pragma once

#include "evaluation_pack.h"
#include "music_intent.h"
#include "music_planner.h"
#include "stem_analysis.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Seed diversity of fixture-m2-4-v1 (ADR-0006 D-M2-16 and precision 5e-04; entry research §2.3; Inc-5 phase
// 5e.2). Exploratory, never a gate. Offline planning only (no render). The seed set, N, k, grid and weights are
// fixed here, before the measurement.

inline constexpr std::uint64_t kDiversitySeedFirst = 1;
inline constexpr std::uint64_t kDiversitySeedLast = 64;   // N = 64
inline constexpr std::size_t kDiversityMinDistinct = 8;   // k
inline constexpr std::uint32_t kDiversityGridPerBar = 16; // sixteenths
inline constexpr std::uint32_t kDiversityMaxDurationClass = 16;

struct FixturePlan {
    bool ok{false};
    std::string error{};
    std::vector<std::vector<audio::MusicIntentEvent>> intentsPerPhrase{};
    std::vector<audio::PlannedNoteOff> noteOffs{};
    std::vector<PackPhrase> phrases{};
};
// Plans fixture-m2-4-v1 offline with the given seed (same planner configuration as fixture_probe_v1)
[[nodiscard]] FixturePlan planFixtureM24V1(std::uint64_t seed);

// Per-phrase content fingerprint (frames relative to the phrase start, all roles or one role; DIF-5e.0-02):
// only the NoteOffs bound to the phrase's own NoteOns are joined. nullopt on an invalid fingerprint.
[[nodiscard]] std::optional<std::uint64_t> phraseContentFingerprint(const std::vector<audio::MusicIntentEvent>& phraseIntents,
                                                                    const std::vector<audio::PlannedNoteOff>& noteOffs,
                                                                    std::uint64_t phraseStartFrame, std::uint32_t sampleRate,
                                                                    std::optional<audio::MusicRole> role = std::nullopt);

// Musical group of every phrase (DIF-5e.0-02): phrases with the same per-phrase fingerprint share the group
// "g<16 hex>". A fingerprint that cannot be computed gives an empty result (fail-closed, DIF-5e.2-03): an
// unverifiable phrase is never counted as an independent group.
[[nodiscard]] std::vector<std::string> phraseGroupIds(const FixturePlan& plan, std::uint32_t sampleRate = 48000);

// Symbolic per-phrase fingerprint (ADR-0006 5e.3c, AG-MM-07): every note on a grid of Q = 32 units per bar of the
// phrase, q = round((frame - start) * 32 * barCount / (end - start)) for the onset and the end (duration NoteOff or
// the planner NoteOffs bound to the note), plus role, pitch in cents re 440 Hz, velocity and timbre x1000; canonical
// order, FNV-1a 64. The +-1-sample floor jitter of a fractional tempo (e.g. 140 BPM) vanishes, so symbolically equal
// phrases share the fingerprint; differences under half a unit merge (conservative). nullopt on an empty phrase
// span, zero bars or a non-finite field (fail-closed).
inline constexpr std::uint32_t kSymbolicGridPerBar = 32;
[[nodiscard]] std::optional<std::uint64_t> symbolicPhraseFingerprint(const std::vector<audio::MusicIntentEvent>& phraseIntents,
                                                                     const std::vector<audio::PlannedNoteOff>& noteOffs,
                                                                     std::uint64_t phraseStartFrame, std::uint64_t phraseEndFrame,
                                                                     std::uint32_t barCount);
// Musical group of every phrase by the symbolic fingerprint ("s<16 hex>"); empty when one is not computable
[[nodiscard]] std::vector<std::string> symbolicPhraseGroupIds(const FixturePlan& plan);

// Token of a note for M2-M4, on a grid of sixteenths of the phrase's bar
struct NoteToken {
    std::size_t role{0};
    std::int64_t onset{0};        // sixteenths from the start of the piece
    int midi{0};                  // rounded MIDI note (69 = 440 Hz)
    std::uint32_t durationClass{0};
};
[[nodiscard]] std::vector<NoteToken> planTokens(const FixturePlan& plan);

// M2 feature vector of one plan: pitch-class histogram (12, normalised), IOI histogram (0..16 sixteenths and
// overflow, normalised), note count / 1000, pitch range / 88, pitch-class entropy / log2(12)
[[nodiscard]] std::vector<double> m2Features(const std::vector<NoteToken>& tokens);
// M3 per role: 1 - max over translations of |Q ∩ (R + t)| / max(|Q|, |R|) on points (onset, midi), multiset
// intersection (simultaneous notes at the same pitch counted with their multiplicity, DIF-5e.2-02); in [0, 1]
[[nodiscard]] double m3CardinalityDistance(const std::vector<NoteToken>& a, const std::vector<NoteToken>& b, std::size_t role);
// M4 per role: Levenshtein distance (unit costs) on tokens (Δonset, interval, duration class) / max length
[[nodiscard]] double m4EditDistance(const std::vector<NoteToken>& a, const std::vector<NoteToken>& b, std::size_t role);

struct DiversityResult {
    std::string status{"NOT_RUN"};            // MEASURED or INCONCLUSIVE (planning failure)
    std::string outcome{};                    // NULL_DIVERSITY, INSUFFICIENT_DISTINCT, DISTRIBUTIONS
    std::string generalisation{};             // INCONCLUSIVE when fewer than 2 distinct contents
    std::vector<std::uint64_t> seeds{};
    std::vector<std::uint64_t> fingerprints{}; // global content fingerprint per seed
    std::size_t distinctFingerprints{0};
    // M1: share of seed pairs with identical global fingerprint; mean share of identical phrases per role
    double m1IdenticalPairShare{0.0};
    std::array<double, kRoleCount> m1IdenticalPhraseShare{};
    // M2-M4 over the pairs of seeds with distinct content (identical pairs are 0 by identity and reported apart)
    std::size_t pairs{0};
    std::size_t distinctPairs{0};
    std::vector<double> m2Distances{};
    std::array<std::vector<double>, kRoleCount> m3Distances{};
    std::array<std::vector<double>, kRoleCount> m4Distances{};
    std::size_t phraseGroups1337{0};          // distinct musical groups among the phrases of seed 1337 (frame rule, v2)
    std::size_t phraseGroups1337Symbolic{0};  // the same with the symbolic rule (v3, ADR-0006 5e.3c)
    std::string error{};
};
[[nodiscard]] DiversityResult measureSeedDiversity();
[[nodiscard]] std::string diversityJson(const DiversityResult& d); // JSON object text

} // namespace cymatica::cacp
