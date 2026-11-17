#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario trama_stereo_v1 (M2.5.2 F2): lateralisation measures of the Trama stem on the fixture render.
// Pre-registered in docs/research/M2_5_2_F2_Trama_Stereo_Preregistration.md. MEASURED, never a gate.
//   first-wavefront ILD: 10 log10(E_L / E_R) over the first 5 ms after every Trama attack (before any delay tap,
//   the earliest tap being 11 ms), summed over all attacks; whole-stem ILD; per-section ILD; Side/Mid; zero-lag
//   L/R correlation coefficient.

inline constexpr double kFirstWavefrontS = 0.005;

struct IldResult {
    std::optional<double> db{};   // undefined when either channel energy is zero
    double energyL{0.0};
    double energyR{0.0};
};

// ILD of an interleaved stereo buffer over [startFrame, endFrame)
[[nodiscard]] IldResult ildDb(std::span<const float> stereo, std::size_t startFrame, std::size_t endFrame) noexcept;
// ILD summed over windows [onset, onset + windowFrames) of an interleaved stereo buffer (windows clipped to the end)
[[nodiscard]] IldResult ildOverWindows(std::span<const float> stereo, std::span<const std::size_t> onsetFrames,
                                       std::size_t windowFrames) noexcept;
// Zero-lag Pearson correlation of L and R (nullopt when a channel has zero variance)
[[nodiscard]] std::optional<double> lrCorrelation(std::span<const float> stereo) noexcept;

struct TramaStereoResult {
    std::string status{"NOT_RUN"};           // MEASURED or INCONCLUSIVE
    std::vector<std::string> reasons{};
    std::size_t tramaAttacks{0};
    IldResult firstWavefront{};
    IldResult whole{};
    std::vector<IldResult> sections{};       // A, B, A'
    std::optional<double> sideMidDb{};
    std::optional<double> correlation{};
    std::optional<double> tramaLufs{};
    std::optional<double> mixLufs{};
    std::array<std::optional<double>, 4> stemLufs{};
    double truePeakDbtp{-120.0};
    double maxGainReductionDb{0.0};
    std::string goldenPcmStatus{};
    bool goldenTracePassed{false};
    bool goldenPlanPassed{false};
    std::uint64_t pcmContentHash{0};
    std::uint64_t eventsHash{0};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] TramaStereoResult runTramaStereoV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
