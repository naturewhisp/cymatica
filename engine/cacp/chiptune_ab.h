#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario chiptune_ab_v1 (spike R-M25-9, pre-registration docs/research/M2_5_Chiptune_Palette_Spike_Preregistration.md):
// fixture-m2-4-v1 rendered with the production palette and with the offline chiptune palette (same plan and events),
// per-stem loudness for the declared calibration, timbre separability of the roles (measurement package 5), clips
// for the cumulative listening. MEASURED, never a gate; the game keeps the production palette.

struct ChiptuneRender {
    std::string captureInvalid{};                 // empty when the capture is valid
    std::string goldenPcmStatus{"NOT_RUN"};
    std::uint64_t pcmContentHash{0};
    bool finite{false};
    std::optional<double> mixLufs{};
    std::array<std::optional<double>, 4> stemLufs{};
    double truePeakDbtp{-120.0};
    double maxGainReductionDb{0.0};
};

struct ChiptuneAbResult {
    std::string status{"NOT_RUN"};               // MEASURED or INCONCLUSIVE
    std::vector<std::string> reasons{};
    ChiptuneRender production{}, chiptune{};
    bool chiptuneRepeatIdentical{false};          // X3
    std::array<double, 4> stemDeltaLu{};          // chiptune - production, per role (X4)
    double mixDeltaLu{0.0};
    std::array<double, 4> suggestedGainDb{};      // current constant + (production - chiptune), rounded to 0.5 dB
    bool x4LoudnessMatched{false};
    // X5: leave-one-out accuracy, pairs Trama-Corpo, Trama-Vettore, Corpo-Vettore
    std::array<double, 3> accProduction{}, accChiptune{};
    bool x5RolesDistinct{false};
    std::vector<std::string> clips{};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] ChiptuneAbResult runChiptuneAbV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
