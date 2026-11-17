#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Measurement package 5 (docs/research/M2_5_Pkg5_Timbre_Descriptors_Preregistration.md): timbre descriptors of
// isolated notes (Peeters et al. 2011 definitions, own implementation on the descriptors_v1 STFT) and leave-one-out
// nearest-centroid separability between roles. Offline, MEASURED, never a gate.
inline constexpr std::size_t kTimbreDescriptors = 6;
inline constexpr std::array<const char*, kTimbreDescriptors> kTimbreDescriptorNames{
    "harmonic_centroid", "harmonic_spread", "odd_even_db", "attack_ms", "centroid_slope_per_s", "spectral_flux"};
inline constexpr std::array<float, 6> kTimbrePitchesHz{220.0f, 261.63f, 293.66f, 329.63f, 392.0f, 440.0f};

using TimbreVector = std::array<double, kTimbreDescriptors>;

// Descriptors of one mono note segment with fundamental f0 (Hz)
[[nodiscard]] TimbreVector timbreDescriptors(std::span<const float> mono, double f0Hz, std::uint32_t sampleRate = 48000);
// Energy-weighted mean spectral centroid and spread (Hz) of a mono segment on the descriptors_v1 STFT
[[nodiscard]] std::array<double, 2> centroidSpreadHz(std::span<const float> mono, std::uint32_t sampleRate = 48000);
// Leave-one-out nearest-centroid accuracy between two groups (features z-scored on the pooled notes)
[[nodiscard]] double looNearestCentroidAccuracy(const std::vector<TimbreVector>& a, const std::vector<TimbreVector>& b);
// Euclidean distance between group means after z-scoring on all the given groups
[[nodiscard]] double standardizedMeanDistance(const std::vector<std::vector<TimbreVector>>& groups, std::size_t i, std::size_t j);

struct TimbreScenarioResult {
    std::string status{"NOT_RUN"};
    std::vector<std::string> reasons{};
    std::array<std::vector<TimbreVector>, 3> roleNotes{};     // Corpo, Trama, Vettore
    double d1CorpoSplitAccuracy{0.0};
    double d2SineSawAccuracy{0.0};
    double d3Fm188SawAccuracy{0.0};
    bool d5RepeatIdentical{false};
    double accTramaCorpo{0.0}, accTramaVettore{0.0}, accCorpoVettore{0.0};
    double distTramaCorpo{0.0}, distTramaVettore{0.0}, distCorpoVettore{0.0};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string measuresJson() const;
    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] TimbreScenarioResult runTimbreDescriptorsV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
