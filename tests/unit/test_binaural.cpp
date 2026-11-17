#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "binaural.h"

#include <array>
#include <cmath>
#include <vector>

using namespace cymatica::cacp;

TEST_CASE("package 3: 24 ERB-spaced centres from 100 Hz to 8 kHz", "[binaural]") {
    const auto c = erbSpacedCentres();
    REQUIRE(c.size() == 24);
    REQUIRE_THAT(c.front(), Catch::Matchers::WithinAbs(100.0, 1e-6));
    REQUIRE_THAT(c.back(), Catch::Matchers::WithinAbs(8000.0, 1e-6));
    for (std::size_t i = 1; i < c.size(); ++i) REQUIRE(c[i] > c[i - 1]);
}

TEST_CASE("package 3: the gammatone band has unity gain at its centre and rejects a distant tone", "[binaural]") {
    const double fc = 1000.0;
    std::vector<float> at(48000), far(48000);
    for (std::size_t i = 0; i < at.size(); ++i) {
        at[i] = static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * fc * static_cast<double>(i) / 48000.0));
        far[i] = static_cast<float>(std::sin(2.0 * 3.14159265358979323846 * 4000.0 * static_cast<double>(i) / 48000.0));
    }
    const auto y = gammatoneBand(at, fc, 48000);
    const auto z = gammatoneBand(far, fc, 48000);
    double peak = 0.0, peakFar = 0.0;
    for (std::size_t i = 24000; i < y.size(); ++i) {
        peak = std::max(peak, static_cast<double>(std::abs(y[i])));
        peakFar = std::max(peakFar, static_cast<double>(std::abs(z[i])));
    }
    REQUIRE_THAT(peak, Catch::Matchers::WithinAbs(1.0, 0.01));
    REQUIRE(peakFar < 0.01);   // two octaves away: below -40 dB
}

TEST_CASE("package 3: a 6 dB pan gives 6 dB ILD in every band and a centred copy gives ITD 0 and IACC 1", "[binaural]") {
    std::vector<float> st(2 * 24000);
    std::uint64_t s = 7;
    for (std::size_t i = 0; i < 24000; ++i) {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        const float v = static_cast<float>(static_cast<double>(s >> 11) / 9007199254740992.0 - 0.5);
        st[2 * i] = v;
        st[2 * i + 1] = 0.5f * v;
    }
    const std::array<std::size_t, 1> w{0};
    const auto b = binauralOverWindows(st, w, 24000);
    REQUIRE(b.defined);
    REQUIRE_THAT(b.ildWeightedDb, Catch::Matchers::WithinAbs(6.0206, 1e-3));
    REQUIRE(b.itdLowFrames == 0);
    REQUIRE(b.iaccWeighted > 0.999);
    REQUIRE(b.ildBandStdDb < 1e-3);
}
