#pragma once

#include "music_intent.h"
#include "music_planner.h"

#include <climits>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cymatica::audio {

// Musical content fingerprint (ADR-0006 D-M2-12, DIF-5.0-02). Unlike canonicalPlanHash (provenance: seed, ids,
// decisions), it identifies only what determines the music, as complete notes:
//   note = (role, sampleFrame, pitch in cents re 440 Hz, velocity x1000, timbre x1000, durationFrames,
//           sorted frames of the explicit planner NoteOffs bound to the note through its eventId)
//   orphan release = (role, sampleFrame) of a planner NoteOff with no NoteOn in the plan.
// eventId is only the join key and is never serialized; seed, tick, epoch and decision data are excluded.
// Encoding: little-endian, fixed-width fields, every list prefixed by its length; hashed with FNV-1a 64.
inline constexpr std::uint32_t kMusicalContentFingerprintFormatVersion = 1;
inline constexpr std::int32_t kFingerprintPitchNotApplicable = INT32_MIN; // pitchHz == 0

struct MusicalContentFingerprint {
    bool valid{false};               // false on a non-finite field (no fingerprint is produced)
    std::uint64_t hash{0};
    std::uint64_t noteCount{0};
    std::uint64_t explicitReleaseCount{0};
    std::uint64_t orphanReleaseCount{0};
    std::string error{};

    [[nodiscard]] bool sameContent(const MusicalContentFingerprint& o) const noexcept {
        return valid && o.valid && hash == o.hash && noteCount == o.noteCount &&
               explicitReleaseCount == o.explicitReleaseCount && orphanReleaseCount == o.orphanReleaseCount;
    }
};

// roleFilter restricts the fingerprint to one role (per-role variant); a single-phrase vector gives the
// per-phrase variant. originFrame is subtracted from every serialized frame (NoteOns and releases): with the
// phrase start it gives the per-phrase fingerprint of the entry research §2.3 (ADR-0006 DIF-5e.0-02), equal for
// the same material at different positions in the piece. The default 0 leaves the absolute fingerprint
// unchanged. A frame before the origin is an error (no fingerprint).
[[nodiscard]] MusicalContentFingerprint musicalContentFingerprint(
    const std::vector<std::vector<MusicIntentEvent>>& intentsPerPhrase, const std::vector<PlannedNoteOff>& noteOffs,
    std::uint32_t sampleRate = 48000, std::optional<MusicRole> roleFilter = std::nullopt, std::uint64_t originFrame = 0);

} // namespace cymatica::audio
