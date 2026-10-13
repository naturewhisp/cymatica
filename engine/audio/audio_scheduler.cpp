#include "audio_scheduler.h"
#include "unsigned_math.h"

#include <algorithm>

namespace cymatica::audio {

AudioScheduler::AudioScheduler(const AudioSchedulerConfig& config) noexcept
    : config_(config) {}

void AudioScheduler::setTransportEpoch(std::uint64_t epoch) noexcept {
    currentEpoch_ = epoch;
}

std::uint64_t AudioScheduler::transportEpoch() const noexcept {
    return currentEpoch_;
}

void AudioScheduler::stageEvent(const ScheduledAudioEvent& event) {
    // Maintain stable ordering: (sampleFrame, eventPriority, eventId) per Spec §7.5
    auto it = std::lower_bound(stagedEvents_.begin(), stagedEvents_.end(), event, EventPriorityComparator{});
    stagedEvents_.insert(it, event);
}

void AudioScheduler::scheduleIntent(const MusicIntentEvent& intent, std::uint64_t transportEpoch) {
    const std::uint64_t effectiveEpoch = (transportEpoch != 0)
        ? transportEpoch
        : ((intent.transportEpoch != 0) ? intent.transportEpoch : currentEpoch_);

    // Generate NoteOn event
    ScheduledAudioEvent onEvt{
        .eventId = intent.eventId,
        .sampleFrame = intent.sampleFrame,
        .transportEpoch = effectiveEpoch,
        .type = AudioEventType::NoteOn,
        .voice = 0,
        .role = intent.role,
        .valueA = intent.pitchHz,
        .valueB = intent.velocity,
        .durationFrames = intent.durationFrames,
        .timbreParam = intent.timbreParam,
        .flags = 0
    };
    stageEvent(onEvt);

    // If duration is specified, generate corresponding NoteOff event (overflow protected)
    if (intent.durationFrames > 0) {
        const std::uint64_t offSampleFrame = core::saturatingAdd(
            intent.sampleFrame,
            static_cast<std::uint64_t>(intent.durationFrames)
        );
        ScheduledAudioEvent offEvt{
            .eventId = intent.eventId,
            .sampleFrame = offSampleFrame,
            .transportEpoch = effectiveEpoch,
            .type = AudioEventType::NoteOff,
            .voice = 0,
            .role = intent.role,
            .valueA = 0.0f,
            .valueB = 0.0f,
            .durationFrames = 0,
            .timbreParam = 0.0f,
            .flags = 0
        };
        stageEvent(offEvt);
    }
}

std::size_t AudioScheduler::dispatchDueEvents(std::uint64_t currentRenderCursor, AudioEngine& audioEngine) {
    const std::uint64_t horizonFrame = core::saturatingAdd(currentRenderCursor, config_.lookaheadFrames);
    std::size_t dispatched = 0;

    auto it = stagedEvents_.begin();
    while (it != stagedEvents_.end()) {
        if (it->sampleFrame <= horizonFrame) {
            if (audioEngine.scheduleAudioEvent(*it)) {
                ++dispatched;
                ++totalDispatched_;
                it = stagedEvents_.erase(it);
            } else {
                // Backpressure: AudioEngine SPSC queue full (Spec §20.7)
                ++backpressureDrops_;
                // Stop dispatching further this update cycle to maintain chronological ordering
                break;
            }
        } else {
            // Future event beyond lookahead horizon
            break;
        }
    }
    return dispatched;
}

std::size_t AudioScheduler::stagedEventCount() const noexcept {
    return stagedEvents_.size();
}

std::uint64_t AudioScheduler::totalDispatchedEvents() const noexcept {
    return totalDispatched_;
}

std::uint64_t AudioScheduler::backpressureDrops() const noexcept {
    return backpressureDrops_;
}

std::uint64_t AudioScheduler::lookaheadFrames() const noexcept {
    return config_.lookaheadFrames;
}

void AudioScheduler::setLookaheadFrames(std::uint64_t frames) noexcept {
    config_.lookaheadFrames = frames;
}

void AudioScheduler::clear() noexcept {
    stagedEvents_.clear();
}

} // namespace cymatica::audio
