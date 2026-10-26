#pragma once

#include "music_intent.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace cymatica::audio {

// O3 chain between the plan, the scheduler and the engine (ADR-0006 D-M2-12).
// An event is identified by (type, eventId, role, sampleFrame, transportEpoch); payload fields are not part
// of the identity because the scheduler copies them verbatim from the intent.
struct EventIdentity {
    std::uint64_t eventId{0};
    std::uint64_t sampleFrame{0};
    std::uint64_t transportEpoch{0};
    AudioEventType type{AudioEventType::NoteOn};
    MusicRole role{MusicRole::Pulso};

    [[nodiscard]] constexpr bool operator==(const EventIdentity& other) const noexcept = default;
    [[nodiscard]] static constexpr EventIdentity of(const ScheduledAudioEvent& e) noexcept {
        return {e.eventId, e.sampleFrame, e.transportEpoch, e.type, e.role};
    }
};

// Disposition of an event consumed by the engine from the SPSC (D-M2-12, INC5-REV-02)
enum class EventDisposition : std::uint8_t {
    Applied = 0, // delivered to the VoiceAllocator, which accepted it (slot >= 0; ParameterChange: executed)
    Ignored = 1, // delivered to the VoiceAllocator, which rejected it (slot -1)
    Dropped = 2  // discarded by the engine before reaching the VoiceAllocator
};

enum class EventDropReason : std::uint8_t {
    None                   = 0,
    StaleFrameOrEpoch      = 1, // sampleFrame before the block start, or epoch < minValidEpoch
    FutureEpochUnmatchable = 2, // future epoch whose frame is reached with no tempo command pending before it
    StopDrain              = 3  // bounded drain on Stop or while Stopped
};

struct AppliedEventRecord {
    EventIdentity id{};
    std::uint64_t consumedFrame{0}; // logical frame at which the engine consumed the event
    EventDisposition disposition{EventDisposition::Applied};
    EventDropReason reason{EventDropReason::None};
    std::int16_t slot{-1};
};

// What the scheduler did with a staged event (D-M2-12, DIF-5.0-R2-01, DIF-5.0-R3-01)
enum class SchedulerEventKind : std::uint8_t {
    Sent     = 0, // pushed into the engine SPSC by dispatchDueEvents
    Canceled = 1  // removed from the staging before being sent
};

enum class EventCancelReason : std::uint8_t {
    None          = 0,
    Revoked       = 1, // AudioScheduler::revokeFromFrame
    ClearedAtStop = 2  // AudioScheduler::clear on the Coordinator Stop
};

struct SchedulerEventRecord {
    EventIdentity id{};
    SchedulerEventKind kind{SchedulerEventKind::Sent};
    EventCancelReason reason{EventCancelReason::None};
};

// Fixed-capacity record log. Storage is allocated once by the owner, outside the realtime path; push() never
// allocates and counts the records that do not fit. Install, remove and read only in quiescence.
template <typename Record>
class FixedRecordLog {
public:
    explicit FixedRecordLog(std::size_t capacity) : records_(capacity) {}

    void push(const Record& r) noexcept {
        if (count_ < records_.size()) {
            records_[count_++] = r;
        } else {
            ++overflow_;
        }
    }
    void clear() noexcept { count_ = 0; overflow_ = 0; }

    [[nodiscard]] std::span<const Record> records() const noexcept { return {records_.data(), count_}; }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return records_.size(); }
    [[nodiscard]] std::uint64_t overflow() const noexcept { return overflow_; }

private:
    std::vector<Record> records_;
    std::size_t count_{0};
    std::uint64_t overflow_{0};
};

using AppliedEventLog = FixedRecordLog<AppliedEventRecord>;
using SchedulerEventLog = FixedRecordLog<SchedulerEventRecord>;

// Initial log capacity (D-M2-12): more than twice the 2,784 events consumed by fixture-m2-4-v1
inline constexpr std::size_t kEventChainLogCapacity = 8192;

} // namespace cymatica::audio
