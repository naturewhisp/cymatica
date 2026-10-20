#pragma once

#include "audio_control.h"
#include "audio_telemetry.h"
#include "master_limiter.h"
#include "music_intent.h"
#include "realtime_exchange.h"
#include "voice_allocator.h"

#include <cstdint>

namespace cymatica::audio {

class AudioProbe;

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

private:
    struct Impl;
    Impl* pImpl_{nullptr};
};

} // namespace cymatica::audio
