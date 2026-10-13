#pragma once

#include "audio_engine.h"
#include "music_intent.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cymatica::audio {

struct AudioSchedulerConfig {
    // Nominal lookahead: 150 ms = 7200 frames at 48 kHz (18 ticks at 120 Hz) (ADR-0003 D-M2-03)
    // Allowed range: 100-250 ms (4800 to 12000 frames)
    std::uint64_t lookaheadFrames{7200};
    std::uint32_t sampleRate{48000};
};

// Ahead-of-time lookahead audio event scheduler (Spec §16.9, §20.7, ADR-0003 D-M2-03)
// Owned exclusively by the Coordinator thread; single producer to AudioEngine's SpscQueue.
class AudioScheduler {
public:
    explicit AudioScheduler(const AudioSchedulerConfig& config = {}) noexcept;

    // Stable ordering comparator implementing Spec §7.5:
    // (sampleFrame, eventPriority [NoteOff < ParameterChange < NoteOn], eventId)
    struct EventPriorityComparator {
        bool operator()(const ScheduledAudioEvent& a, const ScheduledAudioEvent& b) const noexcept {
            if (a.sampleFrame != b.sampleFrame) {
                return a.sampleFrame < b.sampleFrame;
            }
            if (a.type != b.type) {
                return static_cast<std::uint8_t>(a.type) < static_cast<std::uint8_t>(b.type);
            }
            return a.eventId < b.eventId;
        }
    };

    // Staging API (Coordinator thread only, outside realtime audio callback)
    void stageEvent(const ScheduledAudioEvent& event);
    void scheduleIntent(const MusicIntentEvent& intent, std::uint64_t transportEpoch = 0);

    // Transport epoch tracking
    void setTransportEpoch(std::uint64_t epoch) noexcept;
    [[nodiscard]] std::uint64_t transportEpoch() const noexcept;

    // Lookahead dispatch: pushes events within [currentRenderCursor, currentRenderCursor + lookaheadFrames]
    // into AudioEngine::scheduleAudioEvent. Returns number of events successfully dispatched in this call.
    std::size_t dispatchDueEvents(std::uint64_t currentRenderCursor, AudioEngine& audioEngine);

    // Backpressure & telemetry accessors
    [[nodiscard]] std::size_t stagedEventCount() const noexcept;
    [[nodiscard]] std::uint64_t totalDispatchedEvents() const noexcept;
    [[nodiscard]] std::uint64_t backpressureDrops() const noexcept;
    [[nodiscard]] std::uint64_t lookaheadFrames() const noexcept;
    void setLookaheadFrames(std::uint64_t frames) noexcept;

    void clear() noexcept;

private:
    AudioSchedulerConfig config_{};
    std::uint64_t currentEpoch_{1};
    std::vector<ScheduledAudioEvent> stagedEvents_{};
    std::uint64_t totalDispatched_{0};
    std::uint64_t backpressureDrops_{0};
};

} // namespace cymatica::audio
