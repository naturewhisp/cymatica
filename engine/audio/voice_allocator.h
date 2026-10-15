#pragma once

#include "music_intent.h"

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
    static constexpr std::uint32_t kTailLength = 96; // 2 ms @ 48 kHz (linear fade-out)

    bool active{false};
    float startAmplitude[2]{0.0f, 0.0f}; // L, R
    std::uint32_t remainingSamples{0};

    void reset() noexcept {
        active = false;
        startAmplitude[0] = 0.0f;
        startAmplitude[1] = 0.0f;
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

    // Dedicated static steal tail (1 per slot, 8 total)
    StealTail stealTail{};

    [[nodiscard]] float currentVoiceOutput() const noexcept {
        if (state == VoiceState::Idle) return 0.0f;
        return std::sin(phase) * velocity * envelopeLevel * 0.25f;
    }

    void reset() noexcept {
        state = VoiceState::Idle;
        eventId = 0;
        transportEpoch = 1;
        ++generationToken;
        startFrame = 0;
        durationFrames = 0;
        envelopeLevel = 0.0f;
        releaseStartLevel = 0.0f;
        stageFrame = 0;
        phase = 0.0f;
        phaseIncrement = 0.0f;
        stealTail.reset();
    }
};

// Static 8-voice allocator with deterministic selection order and static steal tails
// (ADR-0003 D-M2-03, ADR-0004 AUD-D-01)
class VoiceAllocator {
public:
    static constexpr std::size_t kTotalSlots = 8;
    static constexpr std::size_t kSlotsPerRole = 2;
    static constexpr std::size_t kRoleCount = 4;
    // Nominal voice scaling to guarantee >= 6 dB headroom per role bus (ADR-0003 D-M2-06)
    static constexpr float kNominalVoiceScale = 0.25f;
    // Conservative lease backstop for open pads: 36 bars @ 120 BPM 4/4 (72s = 3,456,000 frames @ 48kHz)
    static constexpr std::uint32_t kMaxLeaseDurationFrames = 3456000;
    static constexpr std::uint32_t kTransportDeclickFrames = 96; // 2 ms @ 48 kHz

    VoiceAllocator() noexcept;

    void init(std::uint32_t sampleRate) noexcept;
    void reset() noexcept; // Stop: destructive reset of all 8 slots to Idle, bump generationTokens

    // Event handling (sample-accurate dispatch from AudioScheduler/AudioEngine)
    // Returns slot index allocated/stolen (0..7), or -1 if rejected
    int onNoteOn(const ScheduledAudioEvent& event) noexcept;

    // Returns slot index transitioned to Release (0..7), or -1 if no matching note found
    int onNoteOff(const ScheduledAudioEvent& event) noexcept;

    // Failsafe checks for duration expiration and open-pad lease backstops
    void checkLifetimes(std::uint64_t currentLogicalFrame) noexcept;

    // Transport transitions
    void onPause() noexcept;   // Freezes logical timeline; triggers 2 ms physical de-click fade-out
    void onResume() noexcept;  // Smooth 2 ms physical fade-in from frozen logical state
    void onStop() noexcept;    // Resets all voices to Idle; triggers 2 ms physical fade-out

    // Renders slice of length `frameCount` into `pOut` (interleaved stereo).
    // If `accumulate` is true, adds to existing buffer; otherwise overwrites.
    void renderBlock(float* pOut, std::uint32_t frameCount, std::uint64_t currentLogicalFrame,
                     bool isTransportPaused, bool accumulate = false) noexcept;

    // Slot & role helpers
    [[nodiscard]] const VoiceSlot& slot(std::size_t index) const noexcept { return slots_[index]; }
    [[nodiscard]] VoiceSlot& slot(std::size_t index) noexcept { return slots_[index]; }

    static constexpr std::size_t roleFirstSlot(MusicRole role) noexcept {
        return static_cast<std::size_t>(role) * kSlotsPerRole;
    }

    // Diagnostics & telemetry counters
    [[nodiscard]] std::uint32_t activeVoiceCount() const noexcept;
    [[nodiscard]] std::uint32_t voiceDropCount() const noexcept { return voiceDropCount_; }
    [[nodiscard]] std::uint32_t orphanNoteRecoveredCount() const noexcept { return orphanNoteRecoveredCount_; }
    [[nodiscard]] std::uint32_t dspAnomalyCount() const noexcept { return dspAnomalyCount_; }

private:
    std::array<VoiceSlot, kTotalSlots> slots_{};
    std::uint32_t sampleRate_{48000};
    std::uint32_t voiceDropCount_{0};
    std::uint32_t orphanNoteRecoveredCount_{0};
    std::uint32_t dspAnomalyCount_{0};

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
};

} // namespace cymatica::audio
