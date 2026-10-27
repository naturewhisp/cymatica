#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace cymatica::cacp {

// Exploratory loudness of descriptors_v1 (ADR-0006 D-M2-14, D-M2-16): ITU-R BS.1770-5 K-weighting (Tables 1-2,
// 48 kHz coefficients), L/R weights 1.0, L = -0.691 + 10 log10(sum z); EBU Tech 3341 v4.0 Momentary (sliding
// rectangular 0.4 s) and Short-term (3 s); Integrated gated over 400 ms blocks with 75 % overlap, absolute gate
// -70 LUFS, relative gate -10 LU, incomplete final block discarded. MEASURED only, never a PASS.
// Undefined values are nullopt (serialised as null with status INCONCLUSIVE): unsupported sample rate,
// signal shorter than the window or block, zero energy, nothing above the absolute gate.
struct LoudnessConfig {
    double momentaryWindowS{0.4};
    double shortTermWindowS{3.0};
    double seriesStepS{0.01};   // evaluation step of the M and S series
    double blockS{0.4};
    double blockOverlap{0.75};
    double absoluteGateLufs{-70.0};
    double relativeGateLu{-10.0};
};

struct LoudnessResult {
    bool supported{false};                   // only 48 kHz stereo is implemented (coefficients of BS.1770-5)
    std::optional<double> integratedLufs{};
    std::optional<double> maxMomentaryLufs{};
    std::optional<double> maxShortTermLufs{};
    std::vector<std::optional<double>> momentaryLufs{}; // window ending at frame window + i * step (i >= 0)
    std::vector<std::optional<double>> shortTermLufs{};
    std::size_t blocks{0};
    std::size_t blocksAboveAbsoluteGate{0};
    std::size_t blocksAboveRelativeGate{0};
    double seriesStepS{0.01};
};

[[nodiscard]] LoudnessResult measureLoudnessStereo(std::span<const float> interleavedStereo, std::uint32_t sampleRate,
                                                   const LoudnessConfig& cfg = {});

} // namespace cymatica::cacp
