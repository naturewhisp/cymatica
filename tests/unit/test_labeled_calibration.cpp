#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "acao02_calibration.h"

#include <cmath>
#include <vector>

using namespace cymatica::cacp;

namespace {

// 40 attacks every 0.25 s, releases 0.100 s after each attack
struct Truth {
    std::vector<double> attacks, releases;
};
Truth truth(double shiftS = 0.0) {
    Truth t;
    for (int i = 0; i < 40; ++i) {
        t.attacks.push_back(1.0 + 0.25 * i + shiftS);
        t.releases.push_back(1.1 + 0.25 * i + shiftS);
    }
    return t;
}
constexpr OffsetWindow kWindow{0.010 - kLabeledOffsetMarginS, 0.010 + kLabeledOffsetMarginS}; // intrinsic 10 ms

} // namespace

TEST_CASE("package 1 V1: attack and release detections with a 10 ms latency are labelled and calibrated", "[labeled_calibration]") {
    const Truth t = truth();
    std::vector<double> est;
    for (const double a : t.attacks) est.push_back(a + 0.010);
    for (const double r : t.releases) est.push_back(r + 0.010);
    const LabeledCalibrationPoint p = labeledCalibrationPoint(t.attacks, t.releases, est, 0.2, kWindow);
    REQUIRE(p.admissible);
    REQUIRE_THAT(p.offsetS, Catch::Matchers::WithinAbs(0.010, 1e-4));
    REQUIRE(p.attackLabelled == 40);
    REQUIRE(p.releaseLabelled == 40);
    REQUIRE(p.falsePositives == 0);
    REQUIRE(p.attackRecall == 1.0);
    REQUIRE(p.precision == 1.0);
}

TEST_CASE("package 1 V2: detections only on the releases never give an admissible point", "[labeled_calibration]") {
    // The 95 ms degeneracy: the attacks are missed, every detection is a release. The previous procedure took the
    // median nearest error (about +100 ms) and scored precision and recall 1.
    const Truth t = truth();
    std::vector<double> est;
    for (const double r : t.releases) est.push_back(r + 0.010);
    const LabeledCalibrationPoint p = labeledCalibrationPoint(t.attacks, t.releases, est, 0.3, kWindow);
    REQUIRE_FALSE(p.admissible);
    REQUIRE(p.attackLabelled == 0);
    REQUIRE(p.releaseLabelled == 40);
    REQUIRE(std::isnan(p.offsetS));
    const std::vector<LabeledCalibrationPoint> grid{p};
    REQUIRE_FALSE(chooseLabeledPoint(grid).has_value());
}

TEST_CASE("package 1 V3: references shifted by 95 ms leave no admissible point", "[labeled_calibration]") {
    const Truth real = truth();
    const Truth shifted = truth(0.095);
    std::vector<double> est;
    for (const double a : real.attacks) est.push_back(a + 0.010);
    const LabeledCalibrationPoint p = labeledCalibrationPoint(shifted.attacks, shifted.releases, est, 0.2, kWindow);
    REQUIRE_FALSE(p.admissible);
    REQUIRE(p.attackRecall == 0.0);
}

TEST_CASE("package 1: an offset outside the window is labelled but not admissible", "[labeled_calibration]") {
    const Truth t = truth();
    std::vector<double> est;
    for (const double a : t.attacks) est.push_back(a + 0.040);   // 40 ms latency: inside the 50 ms label tolerance
    const LabeledCalibrationPoint p = labeledCalibrationPoint(t.attacks, t.releases, est, 0.2, kWindow);
    REQUIRE(p.attackLabelled == 40);
    REQUIRE_THAT(p.offsetS, Catch::Matchers::WithinAbs(0.040, 1e-9));
    REQUIRE_FALSE(p.admissible);   // 40 ms > 10 + 15 ms
}

TEST_CASE("package 1: choice is the admissible point with the highest F, ties to the larger delta", "[labeled_calibration]") {
    LabeledCalibrationPoint a, b, c;
    a.delta = 0.1; a.admissible = true; a.f = 0.9;
    b.delta = 0.2; b.admissible = true; b.f = 0.9;
    c.delta = 0.3; c.admissible = false; c.f = 1.0;
    const std::vector<LabeledCalibrationPoint> grid{a, b, c};
    REQUIRE(chooseLabeledPoint(grid)->delta == 0.2);
}
