#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Measurement package 3 (docs/research/M2_5_Pkg3_Binaural_Bands_Preregistration.md): ILD, ITD and IACC per
// auditory band. 4th-order gammatone (baseband shift + 4 cascaded complex one-pole sections, Holdsworth), 24 bands
// ERB-spaced 100 Hz - 8 kHz, bandwidth 1.019 ERB(fc), unity gain at fc. Offline, MEASURED, never a gate.
inline constexpr std::size_t kBinauralBands = 24;
inline constexpr double kBinauralLoHz = 100.0;
inline constexpr double kBinauralHiHz = 8000.0;
inline constexpr int kMaxLagFrames = 48;               // +-1 ms at 48 kHz
inline constexpr double kItdLowBandsHz = 1500.0;

[[nodiscard]] std::vector<double> erbSpacedCentres(std::size_t n = kBinauralBands, double lo = kBinauralLoHz, double hi = kBinauralHiHz);
// One gammatone band of a mono signal
[[nodiscard]] std::vector<float> gammatoneBand(std::span<const float> x, double fcHz, std::uint32_t sampleRate);

struct BandBinaural {
    double fcHz{0.0};
    double energyL{0.0}, energyR{0.0};
    double ildDb{0.0};
    int itdFrames{0};                  // lag of the maximum: positive = right channel lags (source to the left)
    double iacc{0.0};
};

struct BinauralSummary {
    std::vector<BandBinaural> bands{};
    double ildWeightedDb{0.0};         // energy-weighted mean ILD
    int itdLowFrames{0};               // median ITD of the bands below 1.5 kHz
    double iaccWeighted{0.0};          // energy-weighted mean IACC
    double ildBandStdDb{0.0};          // std of ILD over the bands above 10 % of the band-energy maximum
    bool defined{false};
};

// Binaural measures of an interleaved stereo buffer, summed over windows [start, start + windowFrames)
[[nodiscard]] BinauralSummary binauralOverWindows(std::span<const float> stereo, std::span<const std::size_t> starts,
                                                  std::size_t windowFrames, std::uint32_t sampleRate = 48000);

struct BinauralScenarioResult {
    std::string status{"NOT_RUN"};
    std::vector<std::string> reasons{};
    std::vector<double> c1PanDb{}, c1MeasuredDb{};
    std::vector<int> c2DelayFrames{}, c2MeasuredFrames{};
    double c3IldDb{0.0}, c3Iacc{0.0};
    double c4FirstWavefrontIldDb{0.0}, c4NoteIldDb{0.0};
    bool c6RepeatIdentical{false};
    BinauralSummary tramaFirstWavefront{}, tramaNote{};
    std::string goldenPcmStatus{};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string measuresJson() const;
    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] BinauralScenarioResult runBinauralV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
