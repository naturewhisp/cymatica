#pragma once

#include "music_position.h"

#include <cstdint>

namespace cymatica::audio {

class MusicClock {
public:
    explicit MusicClock(
        std::uint64_t internalSampleRate = 48000,
        const TempoMap& tempoMap = {}
    ) noexcept;

    // Convert absolute sample frame to structured MusicPosition (§7.5, ADR-0002)
    [[nodiscard]] MusicPosition positionAtFrame(std::uint64_t frame, std::uint64_t epoch = 0) const noexcept;

    // Absolute frame at beat boundary
    [[nodiscard]] std::uint64_t frameAtBeat(std::uint64_t totalBeats) const noexcept;

    // Absolute frame at bar boundary
    [[nodiscard]] std::uint64_t frameAtBar(std::uint64_t totalBars) const noexcept;

    // Absolute frame at phrase boundary
    [[nodiscard]] std::uint64_t frameAtPhrase(std::uint64_t totalPhrases) const noexcept;

    // Sample rate conversion with exact rational factor (§7.5, ADR-0002)
    [[nodiscard]] std::uint64_t deviceToInternalFrames(std::uint64_t deviceFrames, std::uint64_t deviceRate) const noexcept;
    [[nodiscard]] std::uint64_t internalToDeviceFrames(std::uint64_t internalFrames, std::uint64_t deviceRate) const noexcept;

    [[nodiscard]] std::uint64_t internalSampleRate() const noexcept { return sampleRate_; }
    [[nodiscard]] const TempoMap& tempoMap() const noexcept { return tempoMap_; }
    void setTempoMap(const TempoMap& map) noexcept { tempoMap_ = map; }

private:
    std::uint64_t sampleRate_{48000};
    TempoMap tempoMap_;
};

} // namespace cymatica::audio
