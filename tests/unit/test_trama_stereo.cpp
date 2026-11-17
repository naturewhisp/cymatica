#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "trama_stereo.h"

#include <array>
#include <cmath>
#include <vector>

using namespace cymatica::cacp;

TEST_CASE("trama_stereo_v1: ILD of a 1.0 / 0.5 pan is +6.02 dB and of a centred source 0 dB", "[trama_stereo]") {
    std::vector<float> panned, centred;
    for (int i = 0; i < 480; ++i) {
        const float x = std::sin(0.05f * static_cast<float>(i));
        panned.push_back(x);
        panned.push_back(0.5f * x);
        centred.push_back(0.7906f * x);
        centred.push_back(0.7906f * x);
    }
    REQUIRE_THAT(*ildDb(panned, 0, 480).db, Catch::Matchers::WithinAbs(6.0206, 1e-3));
    REQUIRE_THAT(*ildDb(centred, 0, 480).db, Catch::Matchers::WithinAbs(0.0, 1e-9));
    REQUIRE_FALSE(ildDb(panned, 0, 0).db.has_value());   // empty window: undefined, never 0
    REQUIRE(*lrCorrelation(centred) > 0.999999);
}

TEST_CASE("trama_stereo_v1: first-wavefront ILD sums only the windows after each onset", "[trama_stereo]") {
    // Left-only burst at frame 100, right-only tail later: the first-wavefront window sees only the left burst
    std::vector<float> s(2 * 1000, 0.0f);
    for (int f = 100; f < 110; ++f) s[2 * f] = 1.0f;
    for (int f = 100; f < 110; ++f) s[2 * f + 1] = 0.25f;
    for (int f = 600; f < 700; ++f) s[2 * f + 1] = 1.0f;
    const std::array<std::size_t, 1> onsets{100};
    const IldResult w = ildOverWindows(s, onsets, 240);
    REQUIRE_THAT(*w.db, Catch::Matchers::WithinAbs(20.0 * std::log10(4.0), 1e-9));
    REQUIRE(*ildDb(s, 0, 1000).db < 0.0);   // the whole buffer is right-heavy: the two measures can disagree
}
