#pragma once

#include "stem_analysis.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace cymatica::cacp {

// Staff view of a phrase (ADR-0006 precision 5e-06, Inc-5 phase 5e.1): a deterministic RGB PNG with one lane per
// role, for multimodal inspection and for the expert's review. Diagnostic only, never part of a blind pack.
// Layout, top to bottom per lane: a strip with the applied NoteOns, the planned notes placed by pitch, a strip
// with the detected onsets coloured by their classification. Vertical lines mark the bars. No text: the legend
// is in the manifest of the scenario (staffViewLegendJson).

enum class StaffOnsetKind : std::uint8_t { Matched, Unmatched, Release };

struct StaffNote {
    std::size_t role{0};
    std::uint64_t startFrame{0};
    std::uint64_t endFrame{0};
    float pitchHz{0.0f};
};

struct StaffOnset {
    std::size_t role{0};
    std::uint64_t frame{0};
    StaffOnsetKind kind{StaffOnsetKind::Matched};
};

struct StaffViewInput {
    std::uint64_t startFrame{0};
    std::uint64_t endFrame{0};
    std::uint32_t sampleRate{48000};
    double pixelsPerSecond{100.0};
    std::vector<StaffNote> planned{};
    std::vector<std::pair<std::size_t, std::uint64_t>> appliedNoteOns{}; // (role, frame)
    std::vector<StaffOnset> detected{};
    std::vector<std::uint64_t> barFrames{};
    // Pitch range per role (Hz, low and high), fixed for the whole render so that phrases are comparable
    std::array<std::pair<float, float>, kRoleCount> pitchRange{};
};

inline constexpr std::size_t kStaffLaneHeight = 72;
inline constexpr std::size_t kStaffSeparator = 4;
inline constexpr std::size_t kStaffMarkerStrip = 8;
inline constexpr std::array<std::array<std::uint8_t, 3>, kRoleCount> kStaffRoleColour{{
    {230, 80, 80},    // pulso
    {90, 160, 240},   // corpo
    {190, 110, 230},  // trama
    {240, 200, 70},   // vettore
}};
inline constexpr std::array<std::uint8_t, 3> kStaffAppliedColour{255, 255, 255};
inline constexpr std::array<std::array<std::uint8_t, 3>, 3> kStaffOnsetColour{{
    {0, 220, 0},      // matched to an applied NoteOn
    {255, 0, 0},      // unmatched
    {255, 140, 0},    // unmatched, within the tolerance of an applied NoteOff (release)
}};

struct StaffViewImage {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::vector<std::uint8_t> rgb{};
    std::vector<std::uint8_t> png{};
};

// Empty png on an empty or inverted interval or a non-positive scale
[[nodiscard]] StaffViewImage renderStaffView(const StaffViewInput& in);

// Row (y) of the top of lane r and of its note area
[[nodiscard]] std::size_t staffLaneTop(std::size_t role) noexcept;

} // namespace cymatica::cacp
