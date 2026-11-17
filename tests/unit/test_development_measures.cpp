#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "development_measures.h"

#include <vector>

using namespace cymatica::cacp;

namespace {
PhraseTokens phrase(const std::vector<int>& midi, std::int64_t step = 2) {
    PhraseTokens p;
    for (std::size_t i = 0; i < midi.size(); ++i) p.push_back({3, static_cast<std::int64_t>(i) * step, midi[i], 2});
    return p;
}
} // namespace

TEST_CASE("package 6: graded distances are transposition and augmentation aware as designed", "[development_measures]") {
    const auto base = phrase({62, 65, 67, 69, 72});
    const auto up = phrase({74, 77, 79, 81, 84});          // chromatic transposition: same intervals
    const auto aug = phrase({62, 65, 67, 69, 72}, 4);      // augmentation: same intervals, doubled IOI
    const auto near = phrase({62, 65, 67, 68, 72});        // one interval moved by a semitone
    REQUIRE(gradedDistance(base, up, 3, false) == 0.0);
    REQUIRE(gradedDistance(base, aug, 3, false) == 0.0);
    REQUIRE(gradedDistance(base, aug, 3, true) > 0.0);     // M5r sees the rhythm
    const double dn = gradedDistance(base, near, 3, false);
    REQUIRE(dn > 0.0);
    REQUIRE(dn < 0.2);                                     // graded: two intervals off by one semitone, not a full miss
    REQUIRE(gradedDistance(base, PhraseTokens{}, 3, false) == 1.0);
    REQUIRE(gradedDistance(PhraseTokens{}, PhraseTokens{}, 3, false) == 0.0);
}

TEST_CASE("package 6: parent rank, Foote novelty and peaks on a two-block matrix", "[development_measures]") {
    // Phrases 0-3 identical, 4-7 identical, the two blocks at distance 1
    std::vector<std::vector<double>> d(8, std::vector<double>(8, 0.0));
    for (std::size_t i = 0; i < 8; ++i) {
        for (std::size_t j = 0; j < 8; ++j) d[i][j] = ((i < 4) == (j < 4)) ? 0.0 : 1.0;
    }
    REQUIRE(parentRank(d, 3, 0) == 1);                    // ties share the best rank
    REQUIRE(parentRank(d, 5, 0) == 2);                    // phrase 4 is nearer to 5 than phrase 0
    REQUIRE(parentRank(d, 2, 5) == 0);                    // parent after child: undefined
    const auto nov = footeNovelty(d, 2);
    const auto peaks = topPeaks(nov, 1);
    REQUIRE(peaks.size() == 1);
    REQUIRE(peaks[0] == 4);                               // boundary between phrases 3 and 4
}
