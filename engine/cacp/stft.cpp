#include "stft.h"

#include "fft.h"

#include <cmath>
#include <complex>
#include <numbers>

namespace cymatica::cacp {

std::vector<double> hannSymmetric(std::size_t n) {
    std::vector<double> w(n, 1.0);
    if (n < 2) return w;
    for (std::size_t i = 0; i < n; ++i) {
        w[i] = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(n - 1));
    }
    return w;
}

std::size_t stftFrameCount(std::size_t length, const StftConfig& cfg) noexcept {
    if (cfg.hop == 0 || cfg.fftSize == 0 || length < cfg.fftSize) return 0;
    return 1 + (length - cfg.fftSize) / cfg.hop;
}

MagnitudeSpectrogram stftMagnitude(std::span<const float> mono, const StftConfig& cfg) {
    MagnitudeSpectrogram out;
    const FftPlan plan(cfg.fftSize);
    if (!plan.valid()) return out;
    out.frames = stftFrameCount(mono.size(), cfg);
    out.bins = cfg.fftSize / 2 + 1;
    out.data.resize(out.frames * out.bins);

    const std::vector<double> window = hannSymmetric(cfg.fftSize);
    std::vector<std::complex<double>> buf(cfg.fftSize);
    for (std::size_t k = 0; k < out.frames; ++k) {
        const std::size_t start = k * cfg.hop;
        for (std::size_t i = 0; i < cfg.fftSize; ++i) {
            buf[i] = {static_cast<double>(mono[start + i]) * window[i], 0.0};
        }
        plan.transform(buf);
        for (std::size_t b = 0; b < out.bins; ++b) {
            out.data[k * out.bins + b] = static_cast<float>(std::abs(buf[b]));
        }
    }
    return out;
}

double frameCentreSeconds(std::size_t frame, const StftConfig& cfg, std::uint32_t sampleRate) noexcept {
    if (sampleRate == 0) return 0.0;
    return (static_cast<double>(frame) * static_cast<double>(cfg.hop) + static_cast<double>(cfg.fftSize) / 2.0) /
           static_cast<double>(sampleRate);
}

std::vector<float> monoFromStereo(std::span<const float> interleavedStereo) {
    std::vector<float> mono(interleavedStereo.size() / 2);
    for (std::size_t i = 0; i < mono.size(); ++i) {
        mono[i] = static_cast<float>(0.5 * (static_cast<double>(interleavedStereo[2 * i]) +
                                            static_cast<double>(interleavedStereo[2 * i + 1])));
    }
    return mono;
}

} // namespace cymatica::cacp
