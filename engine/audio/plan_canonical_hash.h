#pragma once

#include "music_intent.h"
#include "music_planner.h"

#include <cstdint>
#include <vector>

namespace cymatica::audio {

// Canonical symbolic-plan hash shared by the golden tests and the CACP probes (DIF-INC4-R3-03).
// Little-endian serialization of every planned event, every planner NoteOff and every decision trace
// (floats by IEEE-754 bit pattern), hashed with FNV-1a 64. Pitches come from std::pow, so values are only
// guaranteed for the pinned MSVC toolchain/CRT (ADR-0001). Any change to the serialized field set or order
// requires a new format version and reviewed golden updates.
// Format 1: M2.4 to M2.5.3-4 F1. Format 2: adds the Corpo dyad repair fields of each trace (D-M2-41).
// Format 3: adds the Vettore chord-reading count and adaptations (bar, note, semitones) of each trace (D-M2-42).
// Format 4: adds the register shift of the dominant Vettore and its unmet flag (D-M2-43).
// Format 5: adds the origin of the base motif and the lineage reset flag (D-M2-45).
inline constexpr std::uint32_t kCanonicalPlanHashFormatVersion = 5;

[[nodiscard]] std::uint64_t canonicalPlanHash(const std::vector<std::vector<MusicIntentEvent>>& intentsPerPhrase,
                                              const std::vector<PlannedNoteOff>& noteOffs,
                                              const std::vector<PhraseDecisionTrace>& traces) noexcept;

} // namespace cymatica::audio
