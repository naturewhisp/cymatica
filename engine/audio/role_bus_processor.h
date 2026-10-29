#pragma once

#include "audio_telemetry.h"
#include "music_intent.h"
#include "synth_dsp.h"

#include <array>
#include <cstdint>

namespace cymatica::audio {

// Offline per-role stem tap (ADR-0006 D-M2-13, Inc-5 phase 5c). Storage is allocated by the owner before the
// render; the bus processor only writes bounded frames into it (no allocation, no lock). Each role stem is the
// interleaved stereo bus output after bus gain, Corpo micro-ducking and the Trama delay, before the sum; busMix
// is that sum, i.e. the mix of the buses before the transport ramp, the diagnostic tone and the master limiter.
// Frames are written in render order: every output frame of the engine passes through exactly one
// VoiceAllocator::renderBlock call, which writes either processed frames or explicit zeros (frozen silence), so
// frame i of the sink is output frame i of the render. Install, remove and read only in quiescence.
struct RoleStemSink {
    std::array<float*, 4> stems{};    // per role, capacityFrames * 2 floats each
    float* busMix{nullptr};           // capacityFrames * 2 floats
    std::size_t capacityFrames{0};
    std::size_t writtenFrames{0};
    std::uint64_t overflowFrames{0};  // frames that did not fit: the capture is incomplete when > 0
};

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
    // Pulso bus envelope level that produces the full -3.0 dB duck: the nominal full-velocity Pulso bus
    // peak (kNominalVoiceScale 0.25 x kPulsoGain), so a full-velocity hit reaches the documented depth.
    static constexpr float kDuckingFullScaleLevel = 0.25f * kPulsoGain;

    // Continuous bus parameter smoothing (ADR-0004 §2.5: tau_smooth = 5 ms)
    static constexpr float kBusSmoothingTauMs = 5.0f;
    static constexpr float kDefaultTramaWet = 0.20f;

    RoleBusProcessor() noexcept = default;

    void init(std::uint32_t sampleRate) noexcept;
    void reset() noexcept;

    // Transport transition: extinguish circulating delay lines within 2 ms silence boundary without click (Inc-2).
    // The ducking follower and the smoothed bus gains are left untouched so that the stop tails, which still
    // flow through the buses, keep their pre-stop level.
    void onStop(std::uint32_t declickFrames = 96) noexcept;

    // Controls
    void setDuckingEnabled(bool enabled) noexcept { duckingEnabled_ = enabled; }
    [[nodiscard]] bool isDuckingEnabled() const noexcept { return duckingEnabled_; }
    // Diagnostic bypass of the Trama echo path (delay neither read nor written; dry factor unchanged).
    // Used offline by timbre_bus_probe_v1 to isolate the echo by difference; never toggled by the runtime.
    void setDelayBypassed(bool bypassed) noexcept { delayBypassed_ = bypassed; }
    [[nodiscard]] bool isDelayBypassed() const noexcept { return delayBypassed_; }

    // Clears the 4 internal stereo role accumulator buffers before voice rendering
    void beginBlock(std::uint32_t frameCount) noexcept;

    // Voice slot accumulates its stereo contribution into its assigned role bus.
    // `feedsDelay = false` routes a Trama contribution through gain, dry factor and telemetry but never into
    // the echo line (used for transport-stop tails, which must not re-emit echoes after the silence boundary).
    void accumulateRoleSample(MusicRole role, std::uint32_t frameIndex, float sampleL, float sampleR,
                              bool feedsDelay = true) noexcept;

    // Starts a telemetry accumulation window. processAndMix accumulates sums of squares and peaks across all
    // slices rendered inside the window and publishes the window-wide values on every call, so the telemetry
    // after the last slice of a callback describes the whole callback block (not only its last slice).
    void beginTelemetryWindow() noexcept;

    // Process role bus effects (ducking Pulso -> Corpo, micro-delay on Trama, smoothed bus scaling)
    // and sum into interleaved stereo destination buffer `pDestInterleaved`.
    // Populates `ChannelFrame` telemetry for each of the 4 roles over the current telemetry window.
    void processAndMix(float* pDestInterleaved,
                       std::uint32_t frameCount,
                       const std::array<float, kRoleCount>& rolePitchHz,
                       const std::array<uint32_t, kRoleCount>& roleActiveVoices,
                       const std::array<float, kRoleCount>& roleTimbreParam,
                       AudioTelemetryFrame& outTelemetry) noexcept;

    // Offline stem tap (D-M2-13): null by default; set only in quiescence
    void setStemSink(RoleStemSink* sink) noexcept { stemSink_ = sink; }
    [[nodiscard]] RoleStemSink* stemSink() const noexcept { return stemSink_; }
    // Frames rendered without bus processing (frozen pause/stop silence) are written to the sink as zeros
    void appendSilentStemFrames(std::uint32_t frameCount) noexcept;

    // True while the stop-time echo extinction tail is still decaying
    [[nodiscard]] bool hasActiveTail() const noexcept { return delayTail_.active; }

    // Access to processed role bus output (for testing/diagnostics)
    [[nodiscard]] float lastDuckingGain() const noexcept { return lastDuckingGain_; }
    [[nodiscard]] float minDuckingGainInWindow() const noexcept { return windowMinDuckingGain_; }
    [[nodiscard]] float rolePeakInWindow(MusicRole role) const noexcept {
        const auto r = static_cast<std::size_t>(role);
        return (r < kRoleCount) ? windowPeak_[r] : 0.0f;
    }
    [[nodiscard]] float currentBusGain(MusicRole role) const noexcept {
        const auto r = static_cast<std::size_t>(role);
        return (r < kRoleCount) ? busGainSmoothers_[r].current() : 0.0f;
    }
    [[nodiscard]] float currentTramaWet() const noexcept { return tramaWetSmoother_.current(); }

private:
    RoleStemSink* stemSink_{nullptr};
    std::uint32_t sampleRate_{48000};
    bool duckingEnabled_{true};
    bool delayBypassed_{false};

    // Preallocated per-block role buffers (max 1024 frames = 21.3 ms)
    static constexpr std::size_t kMaxBlockFrames = 1024;
    std::array<std::array<float, kMaxBlockFrames * 2>, kRoleCount> busBuffers_{};
    // Trama contributions that bypass the echo line (stop tails)
    std::array<float, kMaxBlockFrames * 2> tramaNoDelayBuffer_{};

    // Smoothed per-role bus gains and Trama wet amount (one-pole, tau = 5 ms)
    std::array<OnePoleSmoother, kRoleCount> busGainSmoothers_{};
    OnePoleSmoother tramaWetSmoother_{};
    // A bus whose input was exactly zero in the previous block may snap its gain (no audible step)
    std::array<bool, kRoleCount> roleInputSilentPrev_{true, true, true, true};

    // Pulso envelope follower for micro-ducking
    float duckingEnv_{0.0f};
    float duckingAttackAlpha_{0.0f};
    float duckingReleaseAlpha_{0.0f};
    float lastDuckingGain_{1.0f};

    // Telemetry window accumulators (M2.4 Inc-2 remediation: one window per callback block)
    std::array<double, kRoleCount> windowSumSq_{};
    std::array<float, kRoleCount> windowPeak_{};
    double windowSumSqGlobal_{0.0};
    std::uint64_t windowFrames_{0};
    float windowMinDuckingGain_{1.0f};

    // Stereo micro-delay on Trama (static preallocated buffers, zero heap alloc)
    std::array<float, kMaxDelayFrames> delayLineL_{};
    std::array<float, kMaxDelayFrames> delayLineR_{};
    std::size_t delayWriteHead_{0};
    std::size_t delayLengthL_{528}; // 11 ms @ 48 kHz
    std::size_t delayLengthR_{816}; // 17 ms @ 48 kHz
    float delayFeedback_{0.30f};

    // Preallocated linear de-click tail for delay extinction on Stop
    struct DelayTail {
        bool active{false};
        float startAmplitude[2]{0.0f, 0.0f};
        std::uint32_t totalSamples{96};
        std::uint32_t remainingSamples{0};

        void reset(std::uint32_t len = 96) noexcept {
            active = false;
            startAmplitude[0] = 0.0f;
            startAmplitude[1] = 0.0f;
            totalSamples = (len > 0) ? len : 96;
            remainingSamples = 0;
        }
    } delayTail_{};

    [[nodiscard]] std::size_t delayReadIndex(std::size_t length) const noexcept {
        return (delayWriteHead_ >= length) ? (delayWriteHead_ - length)
                                           : (delayWriteHead_ + kMaxDelayFrames - length);
    }
};

} // namespace cymatica::audio
