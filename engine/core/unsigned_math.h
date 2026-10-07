#pragma once
#include <cstdint>

namespace cymatica::core {
struct Quotient { std::uint64_t value; std::uint64_t remainder; };

// Exact unsigned multiply/divide, saturating only an unrepresentable quotient.
// The portable remainder recurrence never forms an overflowing sum/product.
inline Quotient multiplyDivide(std::uint64_t a, std::uint64_t b, std::uint64_t d) noexcept {
    if (d == 0) return {0, 0};
    if (b == 0) return {0, 0};
    if (a <= UINT64_MAX / b) return {a * b / d, a * b % d};
    const auto whole = b / d;
    const auto part = b % d;
    if (whole != 0 && a > UINT64_MAX / whole) return {UINT64_MAX, 0};
    const auto base = a * whole;
    std::uint64_t q = 0, r = 0;
    for (int bit = 63; bit >= 0; --bit) {
        const bool carry = r >= d - r;
        r = carry ? r - (d - r) : r + r;
        q = q * 2 + static_cast<std::uint64_t>(carry);
        if (((a >> bit) & 1ULL) != 0) {
            const bool extra = r >= d - part;
            r = extra ? r - (d - part) : r + part;
            q += static_cast<std::uint64_t>(extra);
        }
    }
    if (q > UINT64_MAX - base) return {UINT64_MAX, 0};
    return {base + q, r};
}
inline std::uint64_t saturatingAdd(std::uint64_t a, std::uint64_t b) noexcept {
    return b > UINT64_MAX - a ? UINT64_MAX : a + b;
}
} // namespace cymatica::core
