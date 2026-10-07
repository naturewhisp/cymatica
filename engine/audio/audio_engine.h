#pragma once

#include "audio_control.h"
#include "audio_telemetry.h"
#include "realtime_exchange.h"

#include <cstdint>

namespace cymatica::audio {

struct AudioEngineConfig {
    std::uint32_t sampleRate{48000};
    std::uint32_t channels{2};
    float defaultToneFreqHz{220.0f};
};

// M0/M1 audio owner: manages playback device, realtime exchange channels, and test synthesis.
// Control thread interacts via TripleBuffer and SPSC queues. Audio callback runs in realtime path.
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    bool init(const AudioEngineConfig& config = {});
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

    // Direct block rendering (callable without audio device for deterministic testing & zero-alloc validation)
    void processBlock(float* pOutput, std::uint32_t frameCount) noexcept;

private:
    struct Impl;
    Impl* pImpl_{nullptr};
};

} // namespace cymatica::audio
