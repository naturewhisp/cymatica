#include "fft.h"

#include <cmath>
#include <numbers>
#include <utility>

namespace cymatica::cacp {

FftPlan::FftPlan(std::size_t n) {
    if (!isPowerOfTwo(n)) return;
    n_ = n;
    twiddles_.resize(n / 2);
    for (std::size_t k = 0; k < n / 2; ++k) {
        // Direct evaluation per entry (no angle recurrence, no accumulated drift)
        const double angle = -2.0 * std::numbers::pi * static_cast<double>(k) / static_cast<double>(n);
        twiddles_[k] = {std::cos(angle), std::sin(angle)};
    }
}

bool FftPlan::transform(std::span<std::complex<double>> data, bool inverse) const noexcept {
    if (n_ == 0 || data.size() != n_) return false;

    // Bit-reversal permutation
    for (std::size_t i = 1, j = 0; i < n_; ++i) {
        std::size_t bit = n_ >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(data[i], data[j]);
    }

    for (std::size_t len = 2; len <= n_; len <<= 1) {
        const std::size_t half = len / 2;
        const std::size_t stride = n_ / len; // W_len^k = W_N^(k * N / len)
        for (std::size_t start = 0; start < n_; start += len) {
            for (std::size_t k = 0; k < half; ++k) {
                const std::complex<double> w = inverse ? std::conj(twiddles_[k * stride]) : twiddles_[k * stride];
                const std::complex<double> u = data[start + k];
                const std::complex<double> v = data[start + k + half] * w;
                data[start + k] = u + v;
                data[start + k + half] = u - v;
            }
        }
    }
    return true;
}

bool fftRadix2(std::span<std::complex<double>> data, bool inverse) {
    const FftPlan plan(data.size());
    return plan.transform(data, inverse);
}

} // namespace cymatica::cacp
