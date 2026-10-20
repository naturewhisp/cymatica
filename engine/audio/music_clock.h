#pragma once

#include "music_position.h"

#include <array>
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

    // Validate whether a tempo map configuration is supported (§7.5, ADR-0003 D-M2-01)
    [[nodiscard]] static bool isSupportedTempoMap(const TempoMap& map, std::uint64_t rate = 48000) noexcept;

    // Supported tempo segment representing an anchor point (§7.5, ADR-0003 D-M2-01)
    struct TempoSegment {
        std::uint64_t startFrame{0};
        std::uint64_t startBeat{0};
        std::uint64_t startBar{0};
        std::uint32_t beatOffsetInBar{0};
        TempoMap tempoMap{};
    };

    static constexpr std::size_t kMaxTempoSegments = 16;

    // Convert absolute sample frame to structured MusicPosition (§7.5, ADR-0002)
    [[nodiscard]] MusicPosition positionAtFrame(std::uint64_t frame, std::uint64_t epoch = 0) const noexcept;

    // Unsigned frame conversions saturate at UINT64_MAX when unrepresentable.
    // Absolute frame at beat boundary
    [[nodiscard]] std::uint64_t frameAtBeat(std::uint64_t totalBeats) const noexcept;

    // Absolute frame at a sub-beat grid point: beat `totalBeats` plus `stepIndex / stepsPerBeat` beats.
    // Uses the same per-segment floor-rational anchor as frameAtBeat(), so it coincides exactly with
    // frameAtBeat() whenever stepIndex is a multiple of stepsPerBeat (no float, zero drift).
    [[nodiscard]] std::uint64_t frameAtBeatStep(std::uint64_t totalBeats,
                                                std::uint64_t stepIndex,
                                                std::uint32_t stepsPerBeat) const noexcept;

    // Absolute frame at bar boundary
    [[nodiscard]] std::uint64_t frameAtBar(std::uint64_t totalBars) const noexcept;

    // Absolute frame at phrase boundary
    [[nodiscard]] std::uint64_t frameAtPhrase(std::uint64_t totalPhrases) const noexcept;

    // Calculate the next applicable beat/bar boundary >= frame (DIF-M2-R3-04)
    [[nodiscard]] std::uint64_t nextBeatBoundary(std::uint64_t frame) const noexcept;
    [[nodiscard]] std::uint64_t nextBarBoundary(std::uint64_t frame) const noexcept;

    // Sample rate conversion with exact rational factor (§7.5, ADR-0002, DIF-M1-22)
    [[nodiscard]] std::uint64_t deviceToInternalFrames(std::uint64_t deviceFrames, std::uint64_t deviceRate) const noexcept;
    [[nodiscard]] std::uint64_t internalToDeviceFrames(std::uint64_t internalFrames, std::uint64_t deviceRate) const noexcept;

    [[nodiscard]] std::uint64_t internalSampleRate() const noexcept { return sampleRate_; }
    [[nodiscard]] const TempoMap& tempoMap() const noexcept;
    [[nodiscard]] bool setTempoMap(const TempoMap& map) noexcept;
    [[nodiscard]] bool setTempoMap(const TempoMap& map, std::uint64_t targetSampleFrame) noexcept;

    [[nodiscard]] std::size_t segmentCount() const noexcept { return segmentCount_; }
    [[nodiscard]] const TempoSegment& segment(std::size_t index) const noexcept;

private:
    struct DirectTag {};
    MusicClock(std::uint64_t internalSampleRate, const TempoMap& tempoMap, DirectTag) noexcept;

    std::uint64_t sampleRate_{48000};
    std::array<TempoSegment, kMaxTempoSegments> segments_{};
    std::size_t segmentCount_{1};
};

} // namespace cymatica::audio
