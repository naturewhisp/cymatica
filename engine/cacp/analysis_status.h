#pragma once

#include <cmath>
#include <cstdint>
#include <span>

namespace cymatica::cacp {

// Uniform validity contract of the offline analysis primitives (ADR-0006 D-M2-14, DIF-5b-01/02): an undefined
// result is never presented as a valid descriptor. Serialised as a string next to the value; any status other
// than Ok means the value is null / INCONCLUSIVE.
enum class AnalysisStatus : std::uint8_t {
    Ok                  = 0,
    NonFiniteInput      = 1, // NaN or infinity in the input: no descriptor is produced
    ExactSilence        = 2, // input energy exactly zero (every sample is 0)
    BelowNumericalFloor = 3, // energy above zero but under the numerical floor of the descriptor
    InvalidConfig       = 4  // unsupported sample rate, inconsistent sizes or parameters
};

[[nodiscard]] constexpr const char* analysisStatusName(AnalysisStatus s) noexcept {
    switch (s) {
        case AnalysisStatus::Ok: return "OK";
        case AnalysisStatus::NonFiniteInput: return "NON_FINITE_INPUT";
        case AnalysisStatus::ExactSilence: return "EXACT_SILENCE";
        case AnalysisStatus::BelowNumericalFloor: return "BELOW_NUMERICAL_FLOOR";
        case AnalysisStatus::InvalidConfig: return "INVALID_CONFIG";
    }
    return "UNKNOWN";
}

template <typename T>
[[nodiscard]] bool allFinite(std::span<const T> values) noexcept {
    for (const T v : values) {
        if (!std::isfinite(v)) return false;
    }
    return true;
}

} // namespace cymatica::cacp
