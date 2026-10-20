#pragma once

#include "audio_telemetry.h"
#include "music_intent.h"
#include "role_bus_processor.h"
#include "synth_dsp.h"

#include <array>
#include <cmath>
#include <cstdint>

namespace cymatica::audio {

// Voice lifecycle states according to Spec §16, §20.2 and ADR-0003 D-M2-03
enum class VoiceState : std::uint8_t {
    Idle = 0,
    Attack = 1,
    Decay = 2,
    Sustain = 3,
    Release = 4
};

// Static steal tail preallocated per slot (ADR-0003, ADR-0004)
struct StealTail {
    static constexpr std::uint32_t kDefaultTailLength = 96; // 2 ms @ 48 kHz (linear fade-out)
    static constexpr std::uint32_t kTailLength = kDefaultTailLength;

    bool active{false};
    // Tail samples are fed into the role bus input (pre bus gain, ducking, Trama dry/wet), so the start level
    // matches the voice's actual post-bus contribution. Transport-stop tails never feed the Trama echo line.
    bool feedsDelay{true};
    float startAmplitude[2]{0.0f, 0.0f}; // L, R (pre-bus voice output)
    std::uint32_t totalSamples{kDefaultTailLength};
    std::uint32_t remainingSamples{0};

    void reset(std::uint32_t tailLen = kDefaultTailLength) noexcept {
        active = false;
        feedsDelay = true;
        startAmplitude[0] = 0.0f;
        startAmplitude[1] = 0.0f;
        totalSamples = (tailLen > 0) ? tailLen : kDefaultTailLength;
        remainingSamples = 0;
    }
};

// Dedicated DSP voice slot (Spec §16, ADR-0003 D-M2-03)
struct VoiceSlot {
    std::uint8_t slotIndex{0};       // Global slot index: 0..7
    MusicRole role{MusicRole::Pulso};
    VoiceState state{VoiceState::Idle};

    std::uint64_t eventId{0};
    std::uint64_t transportEpoch{1};
    std::uint64_t generationToken{0}; // Incremented on allocation/steal to protect against stale NoteOff
    std::uint64_t startFrame{0};      // Internal 48 kHz logical frame when NoteOn started
    std::uint32_t durationFrames{0};  // 0 = open pad (unbounded until NoteOff/backstop)

    // Note synthesis parameters
    float pitchHz{220.0f};
    float velocity{1.0f};
    float timbreParam{0.0f};

    // Envelope generator state
    float envelopeLevel{0.0f};
    float releaseStartLevel{0.0f};
    std::uint32_t stageFrame{0};

    // ADSR configuration (in samples at current sample rate)
    std::uint32_t attackSamples{48};    // 1 ms @ 48 kHz
    std::uint32_t decaySamples{480};    // 10 ms @ 48 kHz
    float sustainLevel{0.8f};
    std::uint32_t releaseSamples{96};   // 2 ms @ 48 kHz (ADR-0003 de-click release)

    // Synthesis phase (C++20 native test synthesis for Inc-1)
    float phase{0.0f};
    float phaseIncrement{0.0f};

    // Inc-2 Synth DSP components (synth-palette-v1)
    PolyBlepOscillator osc{};
    TptSvfFilter filter{};
    OnePoleSmoother pitchSmoother{};
    float fmModPhase{0.0f};
    float fmModPhaseIncrement{0.0f};
    float vibratoPhase{0.0f};
    float vibratoPhaseIncrement{0.0f};
    float pitchSweepEnv{1.0f};
    std::uint32_t noteFrameIndex{0};

    float lastOutput[2]{0.0f, 0.0f}; // Last pre-bus voice output (bus input domain), L/R

    // Dedicated static steal tail (1 per slot, 8 total)
    StealTail stealTail{};

    [[nodiscard]] float currentVoiceOutput() const noexcept {
        if (state == VoiceState::Idle) return 0.0f;
        return lastOutput[0];
    }

    [[nodiscard]] float currentVoiceOutputL() const noexcept {
        if (state == VoiceState::Idle) return 0.0f;
        return lastOutput[0];
    }

    [[nodiscard]] float currentVoiceOutputR() const noexcept {
        if (state == VoiceState::Idle) return 0.0f;
        return lastOutput[1];
    }

    void reset(std::uint32_t declickFrames = StealTail::kDefaultTailLength) noexcept {
        state = VoiceState::Idle;
        eventId = 0;
        transportEpoch = 1;
        ++generationToken;
        startFrame = 0;
        durationFrames = 0;
        envelopeLevel = 0.0f;
        releaseStartLevel = 0.0f;
        stageFrame = 0;
        releaseSamples = (declickFrames > 0) ? declickFrames : StealTail::kDefaultTailLength;
        phase = 0.0f;
        phaseIncrement = 0.0f;
        osc.reset();
        filter.reset();
        pitchSmoother.reset(pitchHz);
        fmModPhase = 0.0f;
        fmModPhaseIncrement = 0.0f;
        vibratoPhase = 0.0f;
        vibratoPhaseIncrement = 0.0f;
        pitchSweepEnv = 1.0f;
        noteFrameIndex = 0;
        lastOutput[0] = 0.0f;
        lastOutput[1] = 0.0f;
        stealTail.reset(releaseSamples);
    }
};

// Static 8-voice allocator with deterministic selection order and static steal tails
// (ADR-0003 D-M2-03, D-M2-04, ADR-0004 AUD-D-01)
class VoiceAllocator {
public:
    static constexpr std::size_t kTotalSlots = 8;
    static constexpr std::size_t kSlotsPerRole = 2;
    static constexpr std::size_t kRoleCount = 4;
    // Nominal voice scaling to guarantee >= 6 dB headroom per role bus (ADR-0003 D-M2-06)
    static constexpr float kNominalVoiceScale = 0.25f;
    // Conservative lease backstop for open pads: 36 bars @ 120 BPM 4/4 (72s = 3,456,000 frames @ 48kHz)
    static constexpr std::uint32_t kMaxLeaseDurationFrames = 3456000;
    static constexpr std::uint32_t kDefaultTransportDeclickFrames = 96; // 2 ms @ 48 kHz
    static constexpr std::uint32_t kTransportDeclickFrames = kDefaultTransportDeclickFrames;

    VoiceAllocator() noexcept;

    void init(std::uint32_t sampleRate) noexcept;
    void reset() noexcept; // Destructive reset of all 8 slots to Idle, bump generationTokens

    // Event handling (sample-accurate dispatch from AudioScheduler/AudioEngine)
    // Returns slot index allocated/stolen (0..7), or -1 if rejected
    int onNoteOn(const ScheduledAudioEvent& event) noexcept;

    // Returns slot index transitioned to Release (0..7), or -1 if no matching note found
    int onNoteOff(const ScheduledAudioEvent& event) noexcept;

    // Failsafe checks for duration expiration and open-pad lease backstops
    void checkLifetimes(std::uint64_t currentLogicalFrame) noexcept;

    // Transport transitions
    void onStart() noexcept;   // Restores full physical transport gain for a new run
    void onPause() noexcept;   // Freezes logical timeline; triggers 2 ms physical de-click fade-out
    void onResume() noexcept;  // Smooth 2 ms physical fade-in from frozen logical state
    void onStop() noexcept;    // Resets all voices to Idle; captures 2 ms physical fade-out in steal tails

    // Renders slice of length `frameCount` into `pOut` (interleaved stereo), upstream of the single terminal
    // MasterLimiter owned by AudioEngine (no limiting happens here).
    // If `accumulate` is true, adds to existing buffer; otherwise overwrites.
    // While paused, once the 2 ms de-click ramp has finished and no steal/delay tail is active, the slice is
    // silent and no DSP state (oscillators, filters, delay, ducking, smoothers) is touched.
    void renderBlock(float* pOut, std::uint32_t frameCount, std::uint64_t currentLogicalFrame,
                     bool isTransportPaused, bool accumulate = false) noexcept;

    // Slot & role helpers
    [[nodiscard]] const VoiceSlot& slot(std::size_t index) const noexcept { return slots_[index]; }
    [[nodiscard]] VoiceSlot& slot(std::size_t index) noexcept { return slots_[index]; }

    static constexpr std::size_t roleFirstSlot(MusicRole role) noexcept {
        return static_cast<std::size_t>(role) * kSlotsPerRole;
    }

    // Role bus accessors (Inc-2)
    [[nodiscard]] const RoleBusProcessor& roleBusProcessor() const noexcept { return roleBusProcessor_; }
    [[nodiscard]] RoleBusProcessor& roleBusProcessor() noexcept { return roleBusProcessor_; }
    [[nodiscard]] const AudioTelemetryFrame& latestTelemetry() const noexcept { return latestTelemetry_; }

    // Per-callback telemetry window: AudioEngine opens one window per processBlock so the role telemetry
    // accumulates across every slice rendered in that callback. Without an open window each renderBlock call
    // is its own window (standalone use).
    void beginTelemetryWindow() noexcept;
    void endTelemetryWindow() noexcept { telemetryWindowExternal_ = false; }

    // Diagnostics & telemetry counters
    [[nodiscard]] std::uint32_t declickFrames() const noexcept { return declickFrames_; }
    [[nodiscard]] std::uint32_t activeVoiceCount() const noexcept;
    [[nodiscard]] std::uint32_t voiceDropCount() const noexcept { return voiceDropCount_; }
    [[nodiscard]] std::uint32_t orphanNoteRecoveredCount() const noexcept { return orphanNoteRecoveredCount_; }
    [[nodiscard]] std::uint32_t dspAnomalyCount() const noexcept { return dspAnomalyCount_; }

private:
    std::array<VoiceSlot, kTotalSlots> slots_{};
    std::uint32_t sampleRate_{48000};
    std::uint32_t declickFrames_{kDefaultTransportDeclickFrames};
    std::uint32_t voiceDropCount_{0};
    std::uint32_t orphanNoteRecoveredCount_{0};
    std::uint32_t dspAnomalyCount_{0};

    // Inc-2 Role buses and Telemetry
    RoleBusProcessor roleBusProcessor_{};
    AudioTelemetryFrame latestTelemetry_{};
    bool telemetryWindowExternal_{false};

    // Role synthesis constants derived from the sample rate (ADR-0004 §2.1, §2.4)
    float pulsoSweepMul_{1.0f};          // exp(-1 / (tau_p * fs)), tau_p = 8 ms
    std::uint32_t pulsoClickFrames_{720}; // floor(0.015 * fs)
    float vettoreVibratoInc_{0.0f};      // 2*pi*5.5 / fs
    float lastVettorePitchHz_{0.0f};     // Previous Vettore pitch of the run (0 = first note: no glide)

    // Preallocated block mixing buffer (max 1024 frames = 21.3 ms)
    static constexpr std::size_t kMaxBlockFrames = 1024;
    std::array<float, kMaxBlockFrames * 2> blockMixBuffer_{};

    // Physical transport fade-out / fade-in ramp
    enum class TransportRamp : std::uint8_t {
        None,
        FadingOut,
        FadingIn
    };
    TransportRamp transportRamp_{TransportRamp::None};
    float transportRampLevel_{1.0f};
    float transportRampStep_{0.0f};
    std::uint32_t transportRampRemaining_{0};

    // Voice selection rules (0, 1, 2)
    [[nodiscard]] int selectSlotForRole(MusicRole role) noexcept;
    [[nodiscard]] bool anyStealTailActive() const noexcept;
};

} // namespace cymatica::audio
