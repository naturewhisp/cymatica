#pragma once

#include "analysis_status.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace cymatica::cacp {

// STFT of descriptors_v1 (ADR-0006 D-M2-14): symmetric Hann window of N samples, hop H, frame k covering
// samples [k*H, k*H + N) (no padding), frames while k*H + N <= length. The time of frame k is its centre,
// (k*H + N/2) / fs ("centre" convention of Dixon 2006). Same framing as the E1 research script.
struct StftConfig {
    std::size_t fftSize{2048};
    std::size_t hop{256};
};

inline constexpr StftConfig kDescriptorsV1Stft{};

// Symmetric Hann window w[n] = 0.5 - 0.5 cos(2 pi n / (N - 1)) (numpy.hanning)
[[nodiscard]] std::vector<double> hannSymmetric(std::size_t n);

// Magnitude spectrogram |X[k][b]|, b in [0, N/2], row-major [frame][bin]. status != Ok means no frames
// (NonFiniteInput for NaN/inf samples, InvalidConfig for a non power-of-two size or a zero hop).
struct MagnitudeSpectrogram {
    AnalysisStatus status{AnalysisStatus::Ok};
    std::size_t frames{0};
    std::size_t bins{0};
    std::vector<float> data{};

    [[nodiscard]] float at(std::size_t frame, std::size_t bin) const noexcept { return data[frame * bins + bin]; }
};

// Contract check of a spectrogram received from any source (DIF-5b-R2-02): its own status if not Ok;
// InvalidConfig when data.size() != frames * bins, frames > 0 with bins == 0, or a negative value (a magnitude is
// never negative); NonFiniteInput when a value is NaN/inf; else Ok.
[[nodiscard]] AnalysisStatus validateSpectrogram(const MagnitudeSpectrogram& mag) noexcept;

[[nodiscard]] std::size_t stftFrameCount(std::size_t length, const StftConfig& cfg) noexcept;
[[nodiscard]] MagnitudeSpectrogram stftMagnitude(std::span<const float> mono, const StftConfig& cfg = kDescriptorsV1Stft);
[[nodiscard]] double frameCentreSeconds(std::size_t frame, const StftConfig& cfg, std::uint32_t sampleRate) noexcept;

// Mono downmix (L + R) / 2 of an interleaved stereo buffer, as in the E1 research script
[[nodiscard]] std::vector<float> monoFromStereo(std::span<const float> interleavedStereo);

} // namespace cymatica::cacp
