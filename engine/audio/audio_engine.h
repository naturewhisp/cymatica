#pragma once

#include "audio_control.h"
#include "audio_telemetry.h"
#include "master_limiter.h"
#include "music_intent.h"
#include "realtime_exchange.h"
#include "voice_allocator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace cymatica::audio {

class AudioProbe;

// Preallocated lock-free callback profiling histogram (Spec §20.7, ADR-0005 D-M2-10)
// Owned exclusively by the audio thread; 500 bins of 10 us (0..5000 us) + overflow bin.
struct CallbackProfileHistogram {
    static constexpr std::uint32_t kBinWidthUs = 10;
    static constexpr std::uint32_t kMaxUs = 5000;
    static constexpr std::size_t kBinCount = kMaxUs / kBinWidthUs; // 500 bins
    std::array<std::uint64_t, kBinCount> bins{};
    std::uint64_t overflowCount{0};
    std::uint64_t totalBlocks{0};
    std::uint64_t totalDurationNs{0};
    std::uint64_t maxDurationNs{0};
    std::uint64_t minDurationNs{UINT64_MAX};
    std::uint64_t blocksExceeding2Ms{0};          // Duration > 2.0 ms * frameCount / 512
    std::uint64_t blocksExceedingBlockPeriod{0};    // Duration > frameCount / sampleRate
    std::uint64_t lastDurationNs{0};

    void record(std::uint64_t durationNs, std::uint32_t frameCount, std::uint32_t sampleRate) noexcept {
        lastDurationNs = durationNs;
        totalDurationNs += durationNs;
        ++totalBlocks;
        if (durationNs > maxDurationNs) maxDurationNs = durationNs;
        if (durationNs < minDurationNs) minDurationNs = durationNs;

        const std::uint64_t durationUs = durationNs / 1000ULL;
        const std::size_t binIdx = static_cast<std::size_t>(durationUs / kBinWidthUs);
        if (binIdx < kBinCount) {
            ++bins[binIdx];
        } else {
            ++overflowCount;
        }

        const std::uint64_t thresh2MsNs = (2000000ULL * frameCount) / 512ULL;
        if (durationNs > thresh2MsNs) {
            ++blocksExceeding2Ms;
        }

        if (sampleRate > 0) {
            const std::uint64_t blockPeriodNs = (static_cast<std::uint64_t>(frameCount) * 1000000000ULL) / sampleRate;
            if (durationNs > blockPeriodNs) {
                ++blocksExceedingBlockPeriod;
            }
        }
    }

    void reset() noexcept {
        bins.fill(0);
        overflowCount = 0;
        totalBlocks = 0;
        totalDurationNs = 0;
        maxDurationNs = 0;
        minDurationNs = UINT64_MAX;
        blocksExceeding2Ms = 0;
        blocksExceedingBlockPeriod = 0;
        lastDurationNs = 0;
    }

    [[nodiscard]] double meanMs() const noexcept {
        if (totalBlocks == 0) return 0.0;
        return static_cast<double>(totalDurationNs) / (static_cast<double>(totalBlocks) * 1000000.0);
    }

    [[nodiscard]] double maxMs() const noexcept {
        return static_cast<double>(maxDurationNs) / 1000000.0;
    }

    // p is a percentage in [0, 100] (e.g. 99.9); returns the upper bound of the bin, capped at the maximum.
    [[nodiscard]] double percentileMs(double p) const noexcept {
        if (totalBlocks == 0) return 0.0;
        const double frac = ((p >= 0.0) ? (std::min)(p, 100.0) : 0.0) / 100.0; // NaN -> 0
        const double rawTarget = frac * static_cast<double>(totalBlocks);
        const auto targetCount = (rawTarget <= 1.0)
            ? 1ULL
            : static_cast<std::uint64_t>(std::ceil(rawTarget - 1e-9));
        std::uint64_t accum = 0;
        for (std::size_t i = 0; i < kBinCount; ++i) {
            accum += bins[i];
            if (accum >= targetCount) {
                const double binUpperMs = static_cast<double>((i + 1) * kBinWidthUs) / 1000.0;
                return (std::min)(binUpperMs, maxMs());
            }
        }
        return maxMs();
    }
};

struct AudioEngineConfig {
    std::uint32_t sampleRate{48000};
    std::uint32_t channels{2};
    float defaultToneFreqHz{220.0f};
};

// M0/M1 audio owner: manages playback device, realtime exchange channels, and test synthesis.
// Control thread interacts via TripleBuffer and SPSC queues. Audio callback runs in realtime path.
class AudioEngine {
public:
    static constexpr std::uint32_t kMaxCommandsPerBlock = 16; // ADR-0002 command_budget_v1
    static constexpr std::uint32_t kMaxScheduledEventsPerBlock = 256; // Event-processing budget per callback block (DIF-M2-R4-02)

    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    bool init(const AudioEngineConfig& config = {});
    // Pure offline synthesis initialization without physical audio device or background callback (DIF-M2-01)
    bool initOffline(const AudioEngineConfig& config = {});
    void shutdown();

    // Device control
    bool startTone(float freqHz, float amplitude);
    void setToneFrequency(float freqHz) noexcept;
    void stopTone();

    [[nodiscard]] bool isRunning() const noexcept;
    [[nodiscard]] TransportState transportState() const noexcept;
    [[nodiscard]] std::uint64_t framesRendered() const noexcept;
    [[nodiscard]] std::uint32_t sampleRate() const noexcept;

    // --- Realtime Exchange API (Game / Control thread, Spec §20.5–§20.7) ---
    // Polls latest telemetry published by audio callback. Returns true if new frame acquired.
    bool updateTelemetry() noexcept;
    [[nodiscard]] const AudioTelemetryFrame& telemetry() const noexcept;

    // Access latest control frame for editing and publishing to audio callback
    [[nodiscard]] AudioControlFrame& controlWriter() noexcept;
    void publishControl() noexcept;

    // Send identified command to audio callback
    bool sendCommand(const AudioCommand& cmd) noexcept;

    // Poll acknowledgement from audio callback
    bool pollAck(AudioCommandAck& ack) noexcept;
    [[nodiscard]] std::uint64_t droppedAcks() const noexcept;

    // --- Ahead-of-time Scheduled Audio Events API (Coordinator thread sole producer, Spec §20.7, ADR-0003 D-M2-03) ---
    bool scheduleAudioEvent(const ScheduledAudioEvent& event) noexcept;
    [[nodiscard]] std::uint64_t droppedScheduledEvents() const noexcept;
    [[nodiscard]] std::uint64_t totalScheduledEventsProcessed() const noexcept;
    [[nodiscard]] std::uint64_t deferredScheduledEvents() const noexcept;

    // AudioProbe tap API (Spec §31.5, ADR-0003 D-AO-02)
    // Returns false (probe not attached) when the probe channel count differs from the engine (stereo).
    bool attachAudioProbe(AudioProbe* probe) noexcept;
    void detachAudioProbe() noexcept;
    [[nodiscard]] AudioProbe* audioProbe() const noexcept;

    // VoiceAllocator API (ADR-0003 D-M2-03, Inc-1)
    [[nodiscard]] const VoiceAllocator& voiceAllocator() const noexcept;
    [[nodiscard]] VoiceAllocator& voiceAllocator() noexcept;

    // MasterLimiter API (ADR-0004 AUD-D-02, Inc-2 terminal master protection)
    [[nodiscard]] const MasterLimiter& masterLimiter() const noexcept;
    [[nodiscard]] MasterLimiter& masterLimiter() noexcept;

    // Direct block rendering (callable without audio device for deterministic testing & zero-alloc validation)
    void processBlock(float* pOutput, std::uint32_t frameCount) noexcept;

    // Profiling histogram API (ADR-0005 D-M2-10, INC4-R06)
    // Offline mode: readable after quiescence or between blocks. In online mode, live telemetry is read via telemetry().
    [[nodiscard]] const CallbackProfileHistogram& callbackProfileHistogram() const noexcept;
    void resetCallbackProfileHistogram() noexcept;

private:
    struct Impl;
    Impl* pImpl_{nullptr};
};

} // namespace cymatica::audio
