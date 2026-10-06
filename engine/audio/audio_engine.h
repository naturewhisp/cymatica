#pragma once

#include <cstdint>

namespace cymatica::audio {

struct AudioEngineConfig {
    std::uint32_t sampleRate{48000};
    std::uint32_t channels{2};
    float defaultToneFreqHz{220.0f};
};

// M0 audio owner: opens the single playback device and renders a test tone.
// All member functions must be called from one control thread; only the
// miniaudio callback runs on the audio thread.
class AudioEngine {
public:
    AudioEngine() = default;
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    bool init(const AudioEngineConfig& config = {});
    void shutdown();

    // Sets tone parameters and starts the device if needed.
    bool startTone(float freqHz, float amplitude);
    // Latest-value update, applied by the callback at the next block.
    void setToneFrequency(float freqHz) noexcept;
    void stopTone();

    [[nodiscard]] bool isRunning() const noexcept;
    [[nodiscard]] std::uint64_t framesRendered() const noexcept;
    // Actual device rate (may differ from the requested one).
    [[nodiscard]] std::uint32_t sampleRate() const noexcept;

private:
    struct Impl;
    Impl* pImpl_{nullptr};
};

} // namespace cymatica::audio
