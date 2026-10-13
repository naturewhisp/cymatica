#pragma once

#include <cstdint>

namespace cymatica::audio {

// Four symbolic musical roles defined in Spec §16, §20.5 and ADR-0003 D-M2-02
enum class MusicRole : std::uint8_t {
    Pulso   = 0, // Rhythmic anchor, low percussions, fundamental euclidean grid
    Corpo   = 1, // Harmony, chords/pads, tonal bassline, modal definition
    Trama   = 2, // Arpeggios, polyrhythmic decor, micro-counterpoint, texture
    Vettore = 3  // Directional lead, melodic ascent/descent, tension sweep
};

// Symbolic intent event emitted by music planner/coordinator (ADR-0003 D-M2-02)
struct MusicIntentEvent {
    std::uint64_t eventId{0};
    std::uint64_t sampleFrame{0};      // Internal 48 kHz logical timeline
    std::uint64_t tick{0};             // Fixed-step simulation tick (120 Hz)
    std::uint64_t transportEpoch{1};   // Associated transport epoch
    MusicRole role{MusicRole::Pulso};
    float pitchHz{220.0f};             // Nominal pitch frequency
    float velocity{1.0f};              // Normalized intensity in [0.0f, 1.0f]
    std::uint32_t durationFrames{0};   // Duration in sample frames
    float timbreParam{0.0f};           // Timbre/filter morphology in [0.0f, 1.0f]

    [[nodiscard]] constexpr bool operator==(const MusicIntentEvent& other) const noexcept = default;
};

// Priority ordering for audio events at the same sample frame (Spec §7.5)
// Lower integer value = higher execution priority: NoteOff (0) < ParameterChange (1) < NoteOn (2)
enum class AudioEventType : std::uint8_t {
    NoteOff         = 0,
    ParameterChange = 1,
    NoteOn          = 2
};

using VoiceId = std::uint8_t;

// DSP scheduled audio event dispatched ahead-of-time (Spec §16.9, ADR-0003 D-M2-03)
struct ScheduledAudioEvent {
    std::uint64_t eventId{0};
    std::uint64_t sampleFrame{0};
    std::uint64_t transportEpoch{1};
    AudioEventType type{AudioEventType::NoteOn};
    VoiceId voice{0};
    MusicRole role{MusicRole::Pulso};
    float valueA{0.0f}; // e.g., pitchHz for NoteOn
    float valueB{1.0f}; // e.g., velocity for NoteOn
    std::uint32_t durationFrames{0};
    float timbreParam{0.0f};
    std::uint32_t flags{0};

    [[nodiscard]] constexpr bool operator==(const ScheduledAudioEvent& other) const noexcept = default;
};

static_assert(sizeof(ScheduledAudioEvent) <= 48, "ScheduledAudioEvent must be compact and trivially copyable");

} // namespace cymatica::audio
