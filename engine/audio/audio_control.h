#pragma once

#include "music_position.h"

#include <cstdint>
#include <type_traits>

namespace cymatica::audio {

// Continuous latest-value parameters sent from Game -> Audio via TripleBuffer (Spec §22.1)
struct AudioControlFrame {
    float dissonance{0.0f};
    float playerPerformance{1.0f};
    std::uint32_t requestedArchetypeId{0};
    float toneFrequencyHz{220.0f};
    float toneVolume{0.25f};

    [[nodiscard]] constexpr bool operator==(const AudioControlFrame& other) const noexcept = default;
};

// Discrete identified command types for actions requiring acknowledgement (§20.7, ADR-0002, ADR-0003 D-M2-01)
enum class AudioCommandType : std::uint32_t {
    None        = 0,
    Start       = 1,
    Pause       = 2,
    Resume      = 3,
    Stop        = 4,
    SetTone     = 5,
    SetTempoMap = 6,
};

// Transport state machine defined in Spec §20.8 (DIF-M1-19)
enum class TransportState : std::uint32_t {
    Running  = 0,
    Pausing  = 1,
    Paused   = 2,
    Resuming = 3,
    Stopped  = 4,
};

[[nodiscard]] constexpr bool isValidTransportTransition(TransportState from, TransportState to) noexcept {
    if (from == to) return true;
    if (to == TransportState::Stopped) return true; // * -> Stopped is valid per Spec §20.8
    switch (from) {
        case TransportState::Running:
            return to == TransportState::Pausing;
        case TransportState::Pausing:
            return to == TransportState::Paused;
        case TransportState::Paused:
            return to == TransportState::Resuming;
        case TransportState::Resuming:
            return to == TransportState::Running;
        case TransportState::Stopped:
            return to == TransportState::Running;
        default:
            return false;
    }
}

enum class AudioCommandStatus : std::uint32_t {
    Pending  = 0,
    Applied  = 1,
    Rejected = 2,
};

// Command sent Game -> Audio SPSC (Spec §20.7, ADR-0002, ADR-0003 D-M2-01)
struct AudioCommand {
    std::uint64_t commandId{0};
    AudioCommandType type{AudioCommandType::None};
    float paramF32{0.0f};
    std::uint64_t paramU64{0};
    std::uint64_t paramExtraU64{0};
    std::uint64_t targetSampleFrame{0};

    [[nodiscard]] constexpr bool operator==(const AudioCommand& other) const noexcept = default;

    // Helper for compact, trivially-copyable SetTempoMap payload (ADR-0003 D-M2-01)
    static AudioCommand makeSetTempoMap(
        std::uint64_t cmdId,
        const TempoMap& map,
        std::uint64_t targetFrame = 0
    ) noexcept {
        AudioCommand cmd;
        cmd.commandId = cmdId;
        cmd.type = AudioCommandType::SetTempoMap;
        cmd.paramU64 = (static_cast<std::uint64_t>(map.bpm.numerator) << 32) |
                       (static_cast<std::uint64_t>(map.bpm.denominator) & 0xFFFFFFFFULL);
        cmd.paramExtraU64 = static_cast<std::uint64_t>(map.timeSignatureNum & 0xFF) |
                           (static_cast<std::uint64_t>(map.timeSignatureDen & 0xFF) << 8) |
                           (static_cast<std::uint64_t>(map.beatsPerBar & 0xFF) << 16) |
                           (static_cast<std::uint64_t>(map.subdivisionsPerBeat & 0xFFFF) << 24) |
                           (static_cast<std::uint64_t>(map.barsPerPhrase & 0xFFFF) << 40);
        cmd.targetSampleFrame = targetFrame;
        return cmd;
    }

    [[nodiscard]] TempoMap extractTempoMap() const noexcept {
        TempoMap map;
        map.bpm.numerator = static_cast<std::uint32_t>(paramU64 >> 32);
        map.bpm.denominator = static_cast<std::uint32_t>(paramU64 & 0xFFFFFFFFULL);
        map.timeSignatureNum = static_cast<std::uint32_t>(paramExtraU64 & 0xFF);
        map.timeSignatureDen = static_cast<std::uint32_t>((paramExtraU64 >> 8) & 0xFF);
        map.beatsPerBar = static_cast<std::uint32_t>((paramExtraU64 >> 16) & 0xFF);
        map.subdivisionsPerBeat = static_cast<std::uint32_t>((paramExtraU64 >> 24) & 0xFFFF);
        map.barsPerPhrase = static_cast<std::uint32_t>((paramExtraU64 >> 40) & 0xFFFF);
        return map;
    }
};

static_assert(std::is_trivially_copyable_v<AudioCommand>, "AudioCommand must be trivially copyable");

// Acknowledgement emitted Audio -> Game SPSC
struct AudioCommandAck {
    std::uint64_t commandId{0};
    AudioCommandStatus status{AudioCommandStatus::Pending};
    std::uint64_t sampleFrame{0};
    std::uint64_t transportEpoch{0};

    [[nodiscard]] constexpr bool operator==(const AudioCommandAck& other) const noexcept = default;
};

} // namespace cymatica::audio
