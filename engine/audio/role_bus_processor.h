#pragma once

#include "audio_telemetry.h"
#include "music_intent.h"
#include "synth_dsp.h"

#include <array>
#include <cstdint>

namespace cymatica::audio {

// Preallocated 4-bus stereo processor with deterministic micro-ducking,
// tempo-synced stereo micro-delay on Trama, and post-bus pre-master telemetry.
// (Spec §16.8, §22.1, ADR-0003 D-M2-06, ADR-0004 AUD-D-01/02)
class RoleBusProcessor {
public:
    static constexpr std::size_t kRoleCount = 4;
    static constexpr std::size_t kMaxDelayFrames = 24000; // 500 ms @ 48 kHz per channel

    // Nominal role bus gains (ADR-0003 D-M2-06)
    static constexpr float kPulsoGain   = 0.5011872336f; // -6.0 dB
    static constexpr float kCorpoGain   = 0.3981071705f; // -8.0 dB
    static constexpr float kTramaGain   = 0.3162277660f; // -10.0 dB
    static constexpr float kVettoreGain = 0.5011872336f; // -6.0 dB

    // Micro-ducking parameters (-3.0 dB attenuation on Corpo from Pulso)
    static constexpr float kMaxDuckingAttenDb = -3.0f;
    static constexpr float kMinDuckingGain = 0.7079457844f; // 10^(-3/20)

    RoleBusProcessor() noexcept = default;

    void init(std::uint32_t sampleRate) noexcept;
    void reset() noexcept;

    // Controls
    void setDuckingEnabled(bool enabled) noexcept { duckingEnabled_ = enabled; }
    [[nodiscard]] bool isDuckingEnabled() const noexcept { return duckingEnabled_; }

    // Clears the 4 internal stereo role accumulator buffers before voice rendering
    void beginBlock(std::uint32_t frameCount) noexcept;

    // Voice slot accumulates its stereo contribution into its assigned role bus
    void accumulateRoleSample(MusicRole role, std::uint32_t frameIndex, float sampleL, float sampleR) noexcept;

    // Process role bus effects (ducking Pulso -> Corpo, micro-delay on Trama, bus scaling)
    // and sum into interleaved stereo destination buffer `pDestInterleaved`.
    // Populates `ChannelFrame` telemetry for each of the 4 roles.
    void processAndMix(float* pDestInterleaved,
                       std::uint32_t frameCount,
                       const std::array<float, kRoleCount>& rolePitchHz,
                       const std::array<uint32_t, kRoleCount>& roleActiveVoices,
                       const std::array<float, kRoleCount>& roleTimbreParam,
                       AudioTelemetryFrame& outTelemetry) noexcept;

    // Access to processed role bus output (for testing/diagnostics)
    [[nodiscard]] float lastDuckingGain() const noexcept { return lastDuckingGain_; }

private:
    std::uint32_t sampleRate_{48000};
    bool duckingEnabled_{true};

    // Preallocated per-block role buffers (max 1024 frames = 21.3 ms)
    static constexpr std::size_t kMaxBlockFrames = 1024;
    std::array<std::array<float, kMaxBlockFrames * 2>, kRoleCount> busBuffers_{};

    // Pulso envelope follower for micro-ducking
    float duckingEnv_{0.0f};
    float duckingAttackAlpha_{0.0f};
    float duckingReleaseAlpha_{0.0f};
    float lastDuckingGain_{1.0f};

    // Stereo micro-delay on Trama (static preallocated buffers, zero heap alloc)
    std::array<float, kMaxDelayFrames> delayLineL_{};
    std::array<float, kMaxDelayFrames> delayLineR_{};
    std::size_t delayWriteHead_{0};
    std::size_t delayLengthL_{528}; // 11 ms @ 48 kHz
    std::size_t delayLengthR_{816}; // 17 ms @ 48 kHz
    float delayFeedback_{0.30f};
};

} // namespace cymatica::audio
