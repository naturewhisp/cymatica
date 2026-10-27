#pragma once

#include "stft.h"

#include <cstddef>
#include <cstdint>
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
struct MelSpectrogram {
    std::size_t frames{0};
    std::size_t bands{0};
    std::vector<float> normalized{};
};

[[nodiscard]] MelSpectrogram melSpectrogramNormalized(const MagnitudeSpectrogram& mag, const MelFilterbank& fb, double dbRange);

} // namespace cymatica::cacp
