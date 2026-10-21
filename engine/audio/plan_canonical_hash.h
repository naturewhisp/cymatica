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
inline constexpr std::uint32_t kCanonicalPlanHashFormatVersion = 1;

[[nodiscard]] std::uint64_t canonicalPlanHash(const std::vector<std::vector<MusicIntentEvent>>& intentsPerPhrase,
                                              const std::vector<PlannedNoteOff>& noteOffs,
                                              const std::vector<PhraseDecisionTrace>& traces) noexcept;

} // namespace cymatica::audio
