#pragma once

#include "event_chain_log.h"
#include "music_intent.h"
#include "music_planner.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace cymatica::audio {

// Expected audio events E_exp derived from a plan, exactly as AudioScheduler stages them: one NoteOn per intent,
// one derived NoteOff per intent with durationFrames > 0 (saturating frame), one NoteOff per planner NoteOff.
[[nodiscard]] std::vector<EventIdentity> expectedAudioEvents(
    const std::vector<std::vector<MusicIntentEvent>>& intentsPerPhrase, const std::vector<PlannedNoteOff>& noteOffs);

enum class ChainPath : std::uint8_t {
    Nominal       = 0, // no cancellation, no drop
    InjectedFault = 1  // drops allowed from the fault cut frame on, cancellations with an admitted reason
};

struct ChainReconcileInput {
    std::span<const EventIdentity> expected;              // E_exp
    std::span<const SchedulerEventRecord> schedulerLog;   // E_sent and E_canceled
    std::uint64_t schedulerLogOverflow{0};
    std::span<const AppliedEventRecord> appliedLog;       // E_cons
    std::uint64_t appliedLogOverflow{0};
    std::span<const EventIdentity> expectedIgnored;       // NoteOffs allowed to be Ignored (declared in advance)
    ChainPath path{ChainPath::Nominal};
    std::uint64_t faultCutFrame{0};                       // InjectedFault only
    bool spscDrained{false};                              // SPSC queue verified empty in quiescence
};

struct ChainReconcileResult {
    bool passed{false};
    std::size_t expected{0};
    std::size_t sent{0};
    std::size_t canceled{0};
    std::size_t consumed{0};    // musical records (ParameterChange excluded)
    std::size_t applied{0};
    std::size_t ignored{0};
    std::size_t dropped{0};
    std::size_t parameterChanges{0};
    std::size_t mismatches{0};
    std::string firstMismatch{};
};

// Identity-based reconciliation (D-M2-12 points 2-7, precisions of 5a): E_exp = E_sent (+) E_canceled,
// E_sent = E_cons after the SPSC drain, dispositions admitted per path. Counter equality alone never passes.
[[nodiscard]] ChainReconcileResult reconcileEventChain(const ChainReconcileInput& in);

} // namespace cymatica::audio
