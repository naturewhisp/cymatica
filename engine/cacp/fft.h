#pragma once

#include <complex>
#include <cstddef>
#include <span>
#include <vector>

namespace cymatica::cacp {

// Offline analysis primitives of the CACP (ADR-0006 D-M2-14, Inc-5 phase 5b). Never linked into the realtime path.

[[nodiscard]] constexpr bool isPowerOfTwo(std::size_t n) noexcept { return n != 0 && (n & (n - 1)) == 0; }

// Iterative radix-2 Cooley-Tukey FFT of a fixed size N (power of two), with a precomputed twiddle table
// W_N^k = e^{-2 pi i k / N}. Forward: X[k] = sum x[n] e^{-2 pi i k n / N}; the inverse is unnormalised.
class FftPlan {
public:
    explicit FftPlan(std::size_t n);

    [[nodiscard]] std::size_t size() const noexcept { return n_; }
    [[nodiscard]] bool valid() const noexcept { return n_ != 0; }

    // In place; returns false, leaving data untouched, when data.size() != size() or the plan is invalid.
    bool transform(std::span<std::complex<double>> data, bool inverse = false) const noexcept;

private:
    std::size_t n_{0};
    std::vector<std::complex<double>> twiddles_{}; // N/2 forward twiddles
};

// Convenience: builds a plan for data.size(). Returns false when the size is not a power of two.
bool fftRadix2(std::span<std::complex<double>> data, bool inverse = false);

} // namespace cymatica::cacp
