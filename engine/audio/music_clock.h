#pragma once

#include "music_position.h"

#include <cstdint>
#include <optional>

namespace cymatica::audio {

class MusicClock {
public:
    // Default constructor creates a valid default 48 kHz timeline at 120 BPM 4/4
    MusicClock() noexcept;

    // Explicit static factory failing on invalid or unsupported configuration (§7.5, DIF-M1-22)
    [[nodiscard]] static std::optional<MusicClock> create(
        std::uint64_t internalSampleRate = 48000,
        const TempoMap& tempoMap = {}
    ) noexcept;

    // Convert absolute sample frame to structured MusicPosition (§7.5, ADR-0002)
    [[nodiscard]] MusicPosition positionAtFrame(std::uint64_t frame, std::uint64_t epoch = 0) const noexcept;

    // Unsigned frame conversions saturate at UINT64_MAX when unrepresentable.
    // Absolute frame at beat boundary
    [[nodiscard]] std::uint64_t frameAtBeat(std::uint64_t totalBeats) const noexcept;

    // Absolute frame at bar boundary
    [[nodiscard]] std::uint64_t frameAtBar(std::uint64_t totalBars) const noexcept;

    // Absolute frame at phrase boundary
    [[nodiscard]] std::uint64_t frameAtPhrase(std::uint64_t totalPhrases) const noexcept;

    // Sample rate conversion with exact rational factor (§7.5, ADR-0002, DIF-M1-22)
    [[nodiscard]] std::uint64_t deviceToInternalFrames(std::uint64_t deviceFrames, std::uint64_t deviceRate) const noexcept;
    [[nodiscard]] std::uint64_t internalToDeviceFrames(std::uint64_t internalFrames, std::uint64_t deviceRate) const noexcept;

    [[nodiscard]] std::uint64_t internalSampleRate() const noexcept { return sampleRate_; }
    [[nodiscard]] const TempoMap& tempoMap() const noexcept { return tempoMap_; }
    [[nodiscard]] bool setTempoMap(const TempoMap& map) noexcept;

private:
    struct DirectTag {};
    MusicClock(std::uint64_t internalSampleRate, const TempoMap& tempoMap, DirectTag) noexcept;

    std::uint64_t sampleRate_{48000};
    TempoMap tempoMap_;
};

} // namespace cymatica::audio
