#pragma once

#include "stft.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace cymatica::cacp {

// Mel spectrogram of descriptors_v1 (ADR-0006 D-M2-14): HTK mel scale m = 2595 log10(1 + f / 700), bands
// equally spaced in mel between fMin and fMax, triangular filters on the STFT bin frequencies b * fs / N
// (same construction as the E1 research script). Magnitude, not power, goes through the filterbank.
struct MelConfig {
    std::size_t bands{128};
    double fMinHz{30.0};
    double fMaxHz{16000.0};
    double dbRange{80.0}; // normalisation to the top dbRange dB
};

inline constexpr MelConfig kDescriptorsV1Mel{};

[[nodiscard]] double hzToMelHtk(double hz) noexcept;
[[nodiscard]] double melToHzHtk(double mel) noexcept;

// Filterbank weights [band][bin], bins = N/2 + 1
struct MelFilterbank {
    std::size_t bands{0};
    std::size_t bins{0};
    std::vector<double> weights{};
    std::vector<double> centreHz{};
};

[[nodiscard]] MelFilterbank melFilterbank(const MelConfig& mel, const StftConfig& stft, std::uint32_t sampleRate);

// Mel spectrogram in dB, normalised to [0, 1] over the top dbRange dB of the whole matrix ([frame][band]);
// 20 log10(max(value, 1e-10)) as in the research script.
// Silence is never rendered as energy (DIF-5b-01): if every mel value is exactly 0 the status is ExactSilence,
// if the maximum does not exceed the 1e-10 floor it is BelowNumericalFloor; in both cases every normalised cell
// is 0 and maxDb is empty. A non-Ok spectrogram status propagates with an empty matrix. For Ok inputs the
// normalisation is relative to the matrix maximum, whose absolute level is reported in maxDb.
inline constexpr double kMelMagnitudeFloor = 1e-10;

struct MelSpectrogram {
    AnalysisStatus status{AnalysisStatus::Ok};
    std::size_t frames{0};
    std::size_t bands{0};
    std::optional<double> maxDb{};   // absolute level of the brightest cell (dB re magnitude 1), Ok only
    std::vector<float> normalized{};
};

[[nodiscard]] MelSpectrogram melSpectrogramNormalized(const MagnitudeSpectrogram& mag, const MelFilterbank& fb, double dbRange);

} // namespace cymatica::cacp
