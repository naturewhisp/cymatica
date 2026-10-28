#pragma once

#include "analysis_status.h"

#include <cstddef>
#include <optional>
#include <span>

namespace cymatica::cacp {

// Side/Mid descriptor of descriptors_v1 (ADR-0006 D-M2-14; CACP methods dossier E3):
// 10 log10(sum side^2 / sum mid^2), side = (L - R) / 2, mid = (L + R) / 2, over an interleaved stereo window.
// Structured result (DIF-5b-R2-02), so that a checker can tell the cases apart:
//   NonFiniteInput - a NaN/inf sample; InvalidConfig - empty or odd-length buffer;
//   ExactSilence   - side and mid energies both zero;
//   Ok             - value defined, or undefined with sideIsZero (L = R, mono: -inf) / midIsZero (L = -R: +inf).
struct SideMidResult {
    AnalysisStatus status{AnalysisStatus::Ok};
    std::optional<double> db{};
    bool sideIsZero{false};
    bool midIsZero{false};
};

[[nodiscard]] SideMidResult sideMid(std::span<const float> interleavedStereo) noexcept;

// Compatibility wrapper: the value when defined, nullopt otherwise (null / INCONCLUSIVE)
[[nodiscard]] std::optional<double> sideMidDb(std::span<const float> interleavedStereo) noexcept;

} // namespace cymatica::cacp
