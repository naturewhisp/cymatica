#include "event_chain.h"

#include "unsigned_math.h"

#include <map>
#include <sstream>
#include <tuple>

namespace cymatica::audio {

namespace {

using IdentityKey = std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::uint8_t, std::uint8_t>;

IdentityKey keyOf(const EventIdentity& id) noexcept {
    return {id.eventId, id.sampleFrame, id.transportEpoch, static_cast<std::uint8_t>(id.type),
            static_cast<std::uint8_t>(id.role)};
}

const char* typeName(AudioEventType t) noexcept {
    switch (t) {
        case AudioEventType::NoteOff: return "NoteOff";
        case AudioEventType::ParameterChange: return "ParameterChange";
        case AudioEventType::NoteOn: return "NoteOn";
    }
    return "?";
}

std::string describe(const EventIdentity& id) {
    std::ostringstream s;
    s << typeName(id.type) << " eventId " << id.eventId << " role " << static_cast<int>(id.role) << " frame "
      << id.sampleFrame << " epoch " << id.transportEpoch;
    return s.str();
}

struct Tally {
    std::size_t expected{0};
    std::size_t sent{0};
    std::size_t canceled{0};
    std::size_t consumed{0};
};

} // namespace

std::vector<EventIdentity> expectedAudioEvents(const std::vector<std::vector<MusicIntentEvent>>& intentsPerPhrase,
                                               const std::vector<PlannedNoteOff>& noteOffs) {
    std::vector<EventIdentity> out;
    for (const auto& phrase : intentsPerPhrase) {
        for (const auto& e : phrase) {
            out.push_back({e.eventId, e.sampleFrame, e.transportEpoch, AudioEventType::NoteOn, e.role});
            if (e.durationFrames > 0) {
                out.push_back({e.eventId, core::saturatingAdd(e.sampleFrame, static_cast<std::uint64_t>(e.durationFrames)),
                               e.transportEpoch, AudioEventType::NoteOff, e.role});
            }
        }
    }
    for (const auto& off : noteOffs) {
        out.push_back({off.eventId, off.sampleFrame, off.transportEpoch, AudioEventType::NoteOff, off.role});
    }
    return out;
}

ChainReconcileResult reconcileEventChain(const ChainReconcileInput& in) {
    ChainReconcileResult r;
    auto mismatch = [&](const std::string& what) {
        if (r.mismatches == 0) r.firstMismatch = what;
        ++r.mismatches;
    };

    if (in.schedulerLogOverflow > 0) mismatch("scheduler log overflow (" + std::to_string(in.schedulerLogOverflow) + ")");
    if (in.appliedLogOverflow > 0) mismatch("applied-event log overflow (" + std::to_string(in.appliedLogOverflow) + ")");
    if (!in.spscDrained) mismatch("engine SPSC not drained: E_sent = E_cons cannot be checked");

    std::map<IdentityKey, Tally> tally;
    for (const auto& id : in.expected) {
        ++tally[keyOf(id)].expected;
    }
    r.expected = in.expected.size();

    std::map<IdentityKey, std::size_t> ignoredAllowed;
    for (const auto& id : in.expectedIgnored) {
        ++ignoredAllowed[keyOf(id)];
    }

    // Partition E_exp = E_sent (+) E_canceled, by identity
    for (const auto& rec : in.schedulerLog) {
        const auto it = tally.find(keyOf(rec.id));
        if (it == tally.end()) {
            mismatch("scheduler record outside E_exp: " + describe(rec.id));
            continue;
        }
        Tally& t = it->second;
        if (t.sent + t.canceled >= t.expected) {
            mismatch("event sent or canceled more than once: " + describe(rec.id));
            continue;
        }
        if (rec.kind == SchedulerEventKind::Sent) {
            ++t.sent;
            ++r.sent;
        } else {
            ++t.canceled;
            ++r.canceled;
            if (in.path == ChainPath::Nominal) {
                mismatch("cancellation on the nominal path: " + describe(rec.id));
            }
        }
    }
    for (const auto& id : in.expected) {
        const Tally& t = tally[keyOf(id)];
        if (t.sent + t.canceled != t.expected) {
            mismatch("expected event neither sent nor canceled: " + describe(id));
        }
    }

    // Causal admissibility of each cancellation (DIF-5a-01)
    std::map<std::pair<std::uint64_t, std::uint8_t>, std::size_t> revokedNoteOns;
    for (const auto& rec : in.schedulerLog) {
        if (rec.kind == SchedulerEventKind::Canceled && rec.reason == EventCancelReason::Revoked &&
            rec.id.type == AudioEventType::NoteOn) {
            ++revokedNoteOns[{rec.id.eventId, static_cast<std::uint8_t>(rec.id.role)}];
        }
    }
    for (const auto& rec : in.schedulerLog) {
        if (rec.kind != SchedulerEventKind::Canceled || in.path == ChainPath::Nominal) continue;
        switch (rec.reason) {
            case EventCancelReason::Revoked:
                if (!in.revokeBoundaryFrame.has_value()) {
                    mismatch("revocation without a declared revocation boundary: " + describe(rec.id));
                } else if (rec.contextFrame != *in.revokeBoundaryFrame) {
                    mismatch("revocation at boundary " + std::to_string(rec.contextFrame) + " instead of the failed boundary " +
                             std::to_string(*in.revokeBoundaryFrame) + ": " + describe(rec.id));
                } else if (rec.id.sampleFrame < *in.revokeBoundaryFrame) {
                    mismatch("event before the revocation boundary cannot be revoked: " + describe(rec.id));
                } else if (rec.id.type == AudioEventType::NoteOff &&
                           revokedNoteOns.find({rec.id.eventId, static_cast<std::uint8_t>(rec.id.role)}) == revokedNoteOns.end()) {
                    mismatch("NoteOff revoked although its NoteOn was not revoked (it must be preserved): " + describe(rec.id));
                }
                break;
            case EventCancelReason::ClearedAtStop:
                if (!in.stopClearFrame.has_value()) {
                    mismatch("clear at Stop without a declared applied Stop: " + describe(rec.id));
                } else if (rec.contextFrame != *in.stopClearFrame) {
                    mismatch("clear at Stop frame " + std::to_string(rec.contextFrame) + " instead of " +
                             std::to_string(*in.stopClearFrame) + ": " + describe(rec.id));
                } else if (rec.id.sampleFrame < *in.stopClearFrame) {
                    mismatch("event due before the Stop cleared instead of sent: " + describe(rec.id));
                }
                break;
            case EventCancelReason::None:
            case EventCancelReason::Cleared:
            default:
                mismatch("cancellation without an admitted reason: " + describe(rec.id));
                break;
        }
    }

    // E_sent = E_cons, with the dispositions admitted on this path
    for (const auto& rec : in.appliedLog) {
        if (rec.id.type == AudioEventType::ParameterChange) {
            ++r.parameterChanges; // diagnostic tone, outside the musical chain
            continue;
        }
        ++r.consumed;
        const auto it = tally.find(keyOf(rec.id));
        if (it == tally.end() || it->second.consumed >= it->second.sent) {
            mismatch("consumed event not sent by the scheduler (or consumed twice): " + describe(rec.id));
            continue;
        }
        ++it->second.consumed;

        switch (rec.disposition) {
            case EventDisposition::Applied:
                ++r.applied;
                if (rec.consumedFrame != rec.id.sampleFrame) {
                    mismatch("applied at frame " + std::to_string(rec.consumedFrame) + " instead of planned: " +
                             describe(rec.id));
                }
                break;
            case EventDisposition::Ignored: {
                ++r.ignored;
                auto allowed = ignoredAllowed.find(keyOf(rec.id));
                if (rec.id.type != AudioEventType::NoteOff || allowed == ignoredAllowed.end() || allowed->second == 0) {
                    mismatch("unexpected Ignored: " + describe(rec.id));
                } else {
                    --allowed->second;
                }
                break;
            }
            case EventDisposition::Dropped:
                ++r.dropped;
                if (in.path == ChainPath::Nominal) {
                    mismatch("Dropped on the nominal path: " + describe(rec.id));
                } else if (rec.reason == EventDropReason::None) {
                    mismatch("Dropped without an admitted reason: " + describe(rec.id));
                } else if (rec.consumedFrame < in.faultCutFrame) {
                    mismatch("Dropped before the fault cut frame " + std::to_string(in.faultCutFrame) + " (frame " +
                             std::to_string(rec.consumedFrame) + "): " + describe(rec.id));
                }
                break;
        }
    }
    for (const auto& id : in.expected) {
        const Tally& t = tally[keyOf(id)];
        if (t.consumed != t.sent) {
            mismatch("sent event not consumed by the engine: " + describe(id));
        }
    }

    r.passed = (r.mismatches == 0);
    return r;
}

} // namespace cymatica::audio
