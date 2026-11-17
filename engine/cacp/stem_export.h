#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario stem_export_v1 (measurement package 4, docs/research/M2_5_Pkg4_Specific_Loudness_Masking_Preregistration.md):
// for each fixture section the 8-bar excerpt of F4a (phrases 3-4), written as 32-bit float WAV: bus mix, every stem
// and the bus mix without each role. Input of the offline loudness analysis. Never a gate.

// RIFF/WAVE, IEEE float 32-bit, interleaved stereo
[[nodiscard]] std::vector<std::uint8_t> encodeWavStereoFloat32(std::span<const float> interleaved, std::uint32_t sampleRate);

struct StemExportResult {
    std::string status{"NOT_RUN"};
    std::vector<std::string> reasons{};
    std::vector<std::string> files{};
    std::string goldenPcmStatus{};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] StemExportResult runStemExportV1(const std::string& outputDir = "artifacts/cacp");

} // namespace cymatica::cacp
