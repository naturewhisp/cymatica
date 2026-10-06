#pragma once

#include <cstdint>

namespace cymatica::audio {

enum class PresentationQuality : std::uint32_t {
    Invalid   = 0,
    Estimated = 1,
    Measured  = 2,
};

// Exact rational representation for BPM to avoid accumulating float roundoff (§7.5, ADR-0002)
struct RationalBpm {
    std::uint32_t numerator{120};
    std::uint32_t denominator{1};

    [[nodiscard]] constexpr float toF32() const noexcept {
        return denominator != 0 ? static_cast<float>(numerator) / static_cast<float>(denominator) : 0.0f;
    }

    [[nodiscard]] constexpr bool operator==(const RationalBpm& other) const noexcept = default;
};

struct TempoMap {
    RationalBpm bpm{120, 1};
    std::uint32_t timeSignatureNum{4};
    std::uint32_t timeSignatureDen{4};
    std::uint32_t subdivisionsPerBeat{4}; // 16th notes
    std::uint32_t beatsPerBar{4};
    std::uint32_t barsPerPhrase{4};       // 16 beats per phrase
};

struct MusicPosition {
    std::uint64_t sampleFrame{0};      // Authoritative timeline position (§7.5)
    std::uint64_t transportEpoch{0};   // Discontinuity counter
    std::uint32_t phrase{0};           // 0-indexed phrase
    std::uint32_t bar{0};              // 0-indexed bar within phrase
    std::uint32_t beat{0};             // 0-indexed beat within bar
    std::uint32_t subdivision{0};      // 0-indexed subdivision within beat
    float beatPhase{0.0f};             // [0.0f, 1.0f) derived view only, never authoritative
    float phrasePhase{0.0f};           // [0.0f, 1.0f) derived view

    [[nodiscard]] constexpr bool operator==(const MusicPosition& other) const noexcept = default;
};

} // namespace cymatica::audio
