#pragma once

#include <cstddef>
#include <optional>
#include <span>

namespace cymatica::cacp {

// Side/Mid descriptor of descriptors_v1 (ADR-0006 D-M2-14; CACP methods dossier E3):
// 10 log10(sum side^2 / sum mid^2), side = (L - R) / 2, mid = (L + R) / 2, over an interleaved stereo window.
// Undefined (nullopt -> serialised as null / INCONCLUSIVE) when either energy is zero or the result is not finite.
[[nodiscard]] std::optional<double> sideMidDb(std::span<const float> interleavedStereo) noexcept;

} // namespace cymatica::cacp
