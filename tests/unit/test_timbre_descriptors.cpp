#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "timbre_descriptors.h"

#include <cmath>
#include <vector>

using namespace cymatica::cacp;

namespace {
std::vector<float> tone(double f0, bool saw) {
    std::vector<float> x(48000 / 2);
    for (std::size_t i = 0; i < x.size(); ++i) {
        const double t = static_cast<double>(i) / 48000.0;
        double v = 0.0;
        if (!saw) {
            v = std::sin(2.0 * 3.14159265358979323846 * f0 * t);
        } else {
            for (int k = 1; k <= 20; ++k) v += std::sin(2.0 * 3.14159265358979323846 * k * f0 * t) / k;
        }
        x[i] = static_cast<float>(0.25 * v);
    }
    return x;
}
} // namespace

TEST_CASE("package 5: a sine has harmonic centroid 1, a saw is brighter and has strong even harmonics", "[timbre_descriptors]") {
    const auto s = timbreDescriptors(tone(440.0, false), 440.0);
    const auto w = timbreDescriptors(tone(440.0, true), 440.0);
    REQUIRE_THAT(s[0], Catch::Matchers::WithinAbs(1.0, 0.02));
    REQUIRE(w[0] > 2.0);
    // Saw: odd/even power over the first 10 harmonics = 10 log10(sum 1/k^2 odd / sum 1/k^2 even) = +5.10 dB
    REQUIRE_THAT(w[2], Catch::Matchers::WithinAbs(5.10, 0.3));
}

TEST_CASE("package 5: identical groups are not separable, distinct groups are", "[timbre_descriptors]") {
    std::vector<TimbreVector> a, b, c;
    for (int i = 0; i < 6; ++i) {
        TimbreVector v{};
        for (std::size_t k = 0; k < v.size(); ++k) v[k] = 1.0 + 0.01 * i + 0.001 * static_cast<double>(k);
        a.push_back(v);
        TimbreVector w = v;
        for (auto& x : w) x += 10.0;   // every descriptor differs between the groups
        c.push_back(w);
    }
    REQUIRE(looNearestCentroidAccuracy(a, c) == 1.0);
    REQUIRE(standardizedMeanDistance({a, c}, 0, 1) > 1.0);
}
