#include <catch2/catch_test_macros.hpp>

#include "release_null.h"
#include "stem_analysis.h"

#include <array>
#include <cmath>
#include <vector>

using namespace cymatica::cacp;

TEST_CASE("H-03: residual energy is split by the windows and is exactly zero for identical signals", "[release_null]") {
    const std::vector<float> a(20, 0.5f);
    auto b = a;
    const std::array<std::uint64_t, 1> starts{4};
    const auto mask = windowMask(starts, 2, 10);
    REQUIRE(residualEnergy(a, b, mask).total == 0.0);
    b[2 * 4] = 1.0f;  // frame 4, inside
    b[2 * 6] = 0.75f; // frame 6, outside
    const auto e = residualEnergy(a, b, mask);
    REQUIRE(e.total == 0.3125);
    REQUIRE(e.inside == 0.25);
    REQUIRE(e.maxAbsOutside == 0.25);
    REQUIRE(e.reference == 5.0);
}

TEST_CASE("H-03: the 4 kHz high-pass rejects 500 Hz and keeps 12 kHz", "[release_null]") {
    const auto level = [](double hz) {
        std::vector<float> x;
        for (int i = 0; i < 48000; ++i) {
            const auto s = static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * hz * i / 48000.0));
            x.push_back(s);
            x.push_back(s);
        }
        const auto y = highpass4k(x);
        double e = 0.0;
        for (std::size_t i = 24000; i < y.size(); ++i) e += y[i] * y[i];
        return 10.0 * std::log10(e / static_cast<double>(y.size() - 24000) * 2.0);
    };
    REQUIRE(level(500.0) < -65.0);
    REQUIRE(std::abs(level(4000.0) + 3.01) < 0.1);
    REQUIRE(std::abs(level(12000.0)) < 0.1);
}

TEST_CASE("H-03: the cut ratio is -1000 dB without a residual and 0 dB for a residual equal to the mix", "[release_null]") {
    const std::vector<double> mix(20, 0.5), zero(20, 0.0);
    const std::array<std::uint64_t, 1> cuts{2};
    REQUIRE(cutRatioDb(zero, mix, cuts, 3).at(0) == -1000.0);
    REQUIRE(cutRatioDb(mix, mix, cuts, 3).at(0) == 0.0);
}

TEST_CASE("H-03: median_db is the 50th percentile on an asymmetric vector", "[release_null]") {
    // unsorted and skewed: median -9, mean -25.4, 0.5th percentile -98.2
    const std::vector<double> odd{0.0, -100.0, -8.0, -10.0, -9.0};
    REQUIRE(medianDb(odd) == -9.0);
    REQUIRE(percentileLinear(odd, 50.0) == -9.0);
    REQUIRE(std::abs(percentileLinear(odd, 0.5) + 98.2) < 1e-9); // q is a percentage, not a fraction
    REQUIRE(medianDb({-100.0, 0.0, -2.0, -6.0}) == -4.0);       // even count: mean of the two central values
}
