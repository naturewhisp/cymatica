#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario release_null_v1 (H-03, docs/research/M2_5_H03_Release_Null_Test_Preregistration.md): per-role null test of
// the NoteOff release and of the Corpo release cuts. Offline, MEASURED, never a gate; production is unchanged.

// Half-open frame windows [start, start + length), flagged in a mask of `frames` entries
[[nodiscard]] std::vector<bool> windowMask(std::span<const std::uint64_t> starts, std::uint64_t length, std::size_t frames);
struct ResidualEnergy {
    double total{0.0};
    double inside{0.0};        // energy on masked frames
    double reference{0.0};     // energy of the reference
    double maxAbsOutside{0.0}; // largest |residual| on unmasked frames
};
// Energy of b - a (interleaved stereo, same length) against a, split by the mask
[[nodiscard]] ResidualEnergy residualEnergy(std::span<const float> a, std::span<const float> b, const std::vector<bool>& mask);
// Median reported as median_db
[[nodiscard]] double medianDb(const std::vector<double>& v);
// 4th-order Butterworth high-pass (two biquads) at 4 kHz, 48 kHz, interleaved stereo
[[nodiscard]] std::vector<double> highpass4k(std::span<const float> interleaved);
// Energy ratio in dB of two signals on [cut, cut + frames), per cut; -1000 when the residual energy is 0
[[nodiscard]] std::vector<double> cutRatioDb(std::span<const double> residual, std::span<const double> mix,
                                             std::span<const std::uint64_t> cuts, std::size_t frames);

struct ReleaseNullResult {
    std::string status{"NOT_RUN"}; // MEASURED, TOOL_NOT_VALIDATED (a control failed) or INCONCLUSIVE
    std::string json{};
    bool manifestWriteSuccess{false};
};
[[nodiscard]] ReleaseNullResult runReleaseNullV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
