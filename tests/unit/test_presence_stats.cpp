#include <catch2/catch_test_macros.hpp>

#include "presence_stats.h"

#include <cmath>
#include <string>
#include <vector>

using namespace cymatica::cacp;

namespace {

PresenceObservation obs(const std::string& group, bool truthPresent, bool claimed, const std::string& role = "trama",
                        const std::string& negativeKind = "removal", int phrase = 0) {
    return PresenceObservation{group, group + "/w", phrase, role, truthPresent, truthPresent ? std::string{} : negativeKind, claimed};
}

} // namespace

TEST_CASE("presence_stats_v1: n_eff, statuses and intervals on the reviewer's cases (DIF-5e.0-02)", "[presence_stats]") {
    // 4 groups, one negative claim each: n_eff = 4, MEASURED
    {
        std::vector<PresenceObservation> v{obs("g1", false, false), obs("g2", false, false), obs("g3", false, false), obs("g4", false, false)};
        const auto s = computePresenceStats(v);
        CHECK(s.negatives.groups == 4);
        CHECK(std::abs(s.negatives.nEff - 4.0) < 1e-12);
        CHECK(s.negatives.status == "MEASURED");
        CHECK(std::abs(s.negatives.low - 0.5101) < 1e-3); // Wilson 4/4
    }
    // 4 groups with 4/1/1/1 claims: n_eff = 49/19 = 2.58, INSUFFICIENT_DATA
    {
        std::vector<PresenceObservation> v;
        for (int k = 0; k < 4; ++k) v.push_back(obs("g1", false, false));
        for (const char* g : {"g2", "g3", "g4"}) v.push_back(obs(g, false, false));
        const auto s = computePresenceStats(v);
        CHECK(s.negatives.groups == 4);
        CHECK(std::abs(s.negatives.nEff - 49.0 / 19.0) < 1e-12);
        CHECK(s.negatives.status == "INSUFFICIENT_DATA");
        CHECK(std::isnan(s.negatives.low)); // no interval
        CHECK(s.negatives.rate == 1.0);     // descriptive value kept
    }
    // 3 groups, one claim each: DESCRIPTIVE_ONLY, no interval
    {
        std::vector<PresenceObservation> v{obs("g1", false, false), obs("g2", false, true), obs("g3", false, false)};
        const auto s = computePresenceStats(v);
        CHECK(s.negatives.status == "DESCRIPTIVE_ONLY");
        CHECK(std::isnan(s.negatives.low));
        CHECK(std::abs(s.negatives.rate - 2.0 / 3.0) < 1e-12);
    }
    CHECK(effectiveSampleSize({}) == 0.0);
    CHECK(effectiveSampleSize({3, 3, 3}) == 3.0);
}

TEST_CASE("presence_stats_v1: baseline only with both classes MEASURED; kinds and roles apart (5e-09)", "[presence_stats]") {
    std::vector<PresenceObservation> v;
    // 9 groups with 4 correct positives each (rho = 1: n_eff = 9), 4 groups with one correct negative each
    for (int g = 0; g < 9; ++g) {
        for (const char* r : {"pulso", "corpo", "trama", "vettore"}) v.push_back(obs("g" + std::to_string(g), true, true, r, "", g));
    }
    for (int g = 0; g < 4; ++g) v.push_back(obs("g" + std::to_string(g), false, false, "trama", g == 0 ? "natural" : "removal", g));
    auto s = computePresenceStats(v);
    CHECK(s.positives.claims == 36);
    CHECK(std::abs(s.positives.nEff - 9.0) < 1e-12);   // claims are not independent: 36 claims, 9 groups
    CHECK(std::abs(s.positives.low - 0.7009) < 1e-3);
    CHECK(std::abs(s.negatives.low - 0.5101) < 1e-3);
    REQUIRE(s.aboveTrivialBaseline.has_value());
    CHECK(*s.aboveTrivialBaseline);
    CHECK(std::abs(s.youdenLow - (0.70087 + 0.51012 - 1.0)) < 1e-3);
    CHECK(s.balancedAccuracy == 1.0);
    // Natural and removal negatives reported apart; balance per role
    CHECK(s.naturalNegatives.claims == 1);
    CHECK(s.removalNegatives.claims == 3);
    CHECK(s.negativesPerRole.at("trama").claims == 4);
    CHECK(s.positivesPerRole.at("pulso").claims == 9);
    // One fewer negative group: no baseline decision at all (not false: undecidable)
    v.pop_back();
    s = computePresenceStats(v);
    CHECK(s.negatives.status == "DESCRIPTIVE_ONLY");
    CHECK_FALSE(s.aboveTrivialBaseline.has_value());
    CHECK(std::isnan(s.youdenLow));
    // Fractional Wilson: x_eff = p * n_eff
    const auto [lo, hi] = wilsonEffective(0.5 * 2.58, 2.58);
    CHECK(lo < 0.5);
    CHECK(hi > 0.5);
    CHECK(std::isnan(wilsonEffective(1.0, 0.0).first));
}
