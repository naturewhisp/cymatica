#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "onset.h"
#include "stft.h"
#include "trama_diagnosis.h"

#include <cmath>
#include <vector>

using namespace cymatica::cacp;
using Catch::Matchers::WithinAbs;

namespace {

constexpr std::uint32_t kFs = 48000;

// A valid render with n references; the first `missed` are unmatched and have the lowest raw peaks
TramaRenderDiagnosis render(double ms, std::size_t n, std::size_t missed, double normaliser) {
    TramaRenderDiagnosis d;
    d.releaseMs = ms;
    d.analysisStatus = "Ok";
    d.normaliser = normaliser;
    d.references = n;
    for (std::size_t i = 0; i < n; ++i) {
        TramaReferenceDiagnosis r;
        r.index = i;
        r.matched = i >= missed;
        r.peakRaw = 1.0 + static_cast<double>(i);
        d.refs.push_back(r);
    }
    d.matched = n - missed;
    d.recall = static_cast<double>(n - missed) / static_cast<double>(n);
    d.counterfactualMatched = n;
    d.counterfactualRecall = 1.0;
    return d;
}

} // namespace

TEST_CASE("Onset threshold curve: moving average plus delta, the same curve the picker uses (5e.3b)", "[trama_diagnosis][onset]") {
    std::vector<double> nov(400, 0.0);
    nov[100] = 0.8;
    nov[200] = 1.0;
    OnsetParams p{};
    const auto thr = onsetThresholdCurve(nov, p, kDescriptorsV1Stft, kFs);
    REQUIRE(thr.status == AnalysisStatus::Ok);
    REQUIRE(thr.values.size() == nov.size());
    // w = round(0.1 * 48000 / 256) = 19 frames, kernel 39: at frame 100 the window holds only 0.8
    CHECK_THAT(thr.values[100], WithinAbs(0.8 / 39.0 + 0.10, 1e-15));
    CHECK_THAT(thr.values[0], WithinAbs(0.10, 1e-15));
    // Every picked frame is a local maximum above that curve
    const auto picked = pickOnsetFrames(nov, p, kDescriptorsV1Stft, kFs);
    REQUIRE(picked.frames.size() == 2);
    for (const auto f : picked.frames) CHECK(nov[f] > thr.values[f]);
    // Same domain rules as the picker
    OnsetParams bad = p;
    bad.thresholdHalfWindowS = -0.1;
    CHECK(onsetThresholdCurve(nov, bad, kDescriptorsV1Stft, kFs).status == AnalysisStatus::InvalidConfig);
    CHECK(pickOnsetFrames(nov, bad, kDescriptorsV1Stft, kFs).status == AnalysisStatus::InvalidConfig);
    nov[5] = std::nan("");
    CHECK(onsetThresholdCurve(nov, p, kDescriptorsV1Stft, kFs).status == AnalysisStatus::NonFiniteInput);
}

TEST_CASE("Spectral flux exposes its normaliser: values x normaliser is the raw flux, max value 1", "[trama_diagnosis][onset]") {
    std::vector<float> mono(kFs, 0.0f);
    for (std::size_t i = kFs / 4; i < kFs / 2; ++i) mono[i] = static_cast<float>(0.5 * std::sin(0.06 * static_cast<double>(i)));
    const auto mag = stftMagnitude(mono, kDescriptorsV1Stft);
    const auto nov = spectralFluxNovelty(mag, 100.0);
    REQUIRE(nov.status == AnalysisStatus::Ok);
    CHECK(nov.normaliser > 0.0);
    double mx = 0.0;
    for (const double v : nov.values) mx = (std::max)(mx, v);
    CHECK(mx == 1.0);
    // Silence: all-zero flux, floored normaliser
    const auto silent = spectralFluxNovelty(stftMagnitude(std::vector<float>(kFs, 0.0f), kDescriptorsV1Stft), 100.0);
    CHECK(silent.normaliser == 1e-12);
}

TEST_CASE("Global normalisation: a strong transient elsewhere hides a weak attack (mechanism M-NORM)", "[trama_diagnosis][onset]") {
    // Raw flux: weak attack 0.3 at frame 100; a strong release transient 2.0 at frame 300, far from it
    OnsetParams p{};
    p.delta = 0.20; // frozen Trama delta
    std::vector<double> raw(400, 0.0);
    raw[100] = 0.3;
    auto normalised = [](std::vector<double> v) {
        double mx = 0.0;
        for (const double x : v) mx = (std::max)(mx, x);
        for (auto& x : v) x /= mx;
        return v;
    };
    // Alone, the attack is the maximum and is detected
    auto alone = pickOnsetFrames(normalised(raw), p, kDescriptorsV1Stft, kFs);
    REQUIRE(alone.frames.size() == 1);
    CHECK(alone.frames[0] == 100);
    // With the transient 400 ms away, the same attack falls below delta x max: missed, although nothing near it changed
    raw[300] = 2.0;
    auto with = pickOnsetFrames(normalised(raw), p, kDescriptorsV1Stft, kFs);
    REQUIRE(with.frames.size() == 1);
    CHECK(with.frames[0] == 300);
}

TEST_CASE("5e.3b P1: global maximum on a release and normaliser ratio >= 1.2", "[trama_diagnosis]") {
    auto at2 = render(2.0, 40, 10, 12.0);
    auto at30 = render(30.0, 40, 0, 10.0);
    at2.globalMaxAtRelease = true;
    CHECK(evaluateP1(at2, at30).outcome == "SUPPORTED"); // ratio exactly 1.2
    at2.normaliser = 11.9;
    CHECK(evaluateP1(at2, at30).outcome == "NOT_SUPPORTED");
    at2.normaliser = 20.0;
    at2.globalMaxAtRelease = false;
    CHECK(evaluateP1(at2, at30).outcome == "NOT_SUPPORTED");
    at2.analysisStatus = "NON_FINITE_INPUT";
    CHECK(evaluateP1(at2, at30).outcome == "INCONCLUSIVE");
}

TEST_CASE("5e.3b P2: counterfactual recall >= 0.95 with a valid 30 ms control", "[trama_diagnosis]") {
    auto at2 = render(2.0, 40, 10, 12.0);
    auto at30 = render(30.0, 40, 0, 10.0);
    at2.counterfactualMatched = 38;
    at2.counterfactualRecall = 0.95;
    CHECK(evaluateP2(at2, at30).outcome == "SUPPORTED");
    at2.counterfactualMatched = 37;
    at2.counterfactualRecall = 0.925;
    CHECK(evaluateP2(at2, at30).outcome == "NOT_SUPPORTED");
    at30.counterfactualMatched = 39; // the recomputation itself loses a reference without releases
    at30.counterfactualRecall = 0.975;
    CHECK(evaluateP2(at2, at30).outcome == "INCONCLUSIVE");
    at30 = render(30.0, 40, 0, 10.0);
    at30.captureInvalid = "stem overflow";
    CHECK(evaluateP2(at2, at30).outcome == "INCONCLUSIVE");
}

TEST_CASE("5e.3b P3: missed references among the 12 lowest raw attack peaks", "[trama_diagnosis]") {
    auto at2 = render(2.0, 40, 10, 12.0); // the 10 missed have the lowest peaks
    CHECK(evaluateP3(at2).outcome == "SUPPORTED");
    // Move 3 missed references to the top of the ranking: 7 of 10 remain among the lowest 12
    for (std::size_t i = 0; i < 3; ++i) at2.refs[i].peakRaw = 100.0 + static_cast<double>(i);
    CHECK(evaluateP3(at2).outcome == "NOT_SUPPORTED");
    // Exactly 8: supported
    at2.refs[0].peakRaw = 1.0;
    CHECK(evaluateP3(at2).outcome == "SUPPORTED");
    // The rule is fixed for 10 missed
    CHECK(evaluateP3(render(2.0, 40, 9, 12.0)).outcome == "INCONCLUSIVE");
}

TEST_CASE("5e.3b P4 and the causal outcome", "[trama_diagnosis]") {
    auto at2 = render(2.0, 40, 10, 12.0);
    CHECK(evaluateP4(at2).outcome == "NOT_SUPPORTED");
    for (std::size_t i = 0; i < 4; ++i) at2.refs[i].mergedCandidate = true;
    CHECK(evaluateP4(at2).outcome == "INCONCLUSIVE");
    at2.refs[4].mergedCandidate = true;
    CHECK(evaluateP4(at2).outcome == "SUPPORTED");
    at2.refs[30].mergedCandidate = true; // a matched reference does not count
    CHECK(evaluateP4(at2).detail.rfind("5 ", 0) == 0);

    const PredictionOutcome s{"SUPPORTED", ""}, n{"NOT_SUPPORTED", ""}, i{"INCONCLUSIVE", ""};
    const std::vector<std::string> valid{};
    CHECK(evaluateCausal(s, s, s, n, valid).outcome == "SUPPORTED");
    CHECK(evaluateCausal(s, s, s, s, valid).outcome == "INCONCLUSIVE"); // the alternative mechanism is also supported
    CHECK(evaluateCausal(s, n, s, n, valid).outcome == "NOT_SUPPORTED");
    CHECK(evaluateCausal(n, s, s, n, valid).outcome == "INCONCLUSIVE");
    CHECK(evaluateCausal(s, i, s, n, valid).outcome == "INCONCLUSIVE");
}

TEST_CASE("5e.3b causal outcome is fail-closed: P4 not excluded, invalid parity or repeat (DIF-5e.3b-01, -03)", "[trama_diagnosis]") {
    const PredictionOutcome s{"SUPPORTED", ""}, n{"NOT_SUPPORTED", ""}, i{"INCONCLUSIVE", ""};
    // An inconclusive P4 has not excluded the merge mechanism
    CHECK(evaluateCausal(s, s, s, i, {}).outcome == "INCONCLUSIVE");
    // Parity with evaluateRoleOnsets failed: every prediction supported, the causal outcome is still INCONCLUSIVE
    const auto parity = evaluateCausal(s, s, s, n, {"2.000 ms: step-by-step recall differs from evaluateRoleOnsets"});
    CHECK(parity.outcome == "INCONCLUSIVE");
    CHECK(parity.detail.find("recall differs") != std::string::npos);
    // Repeat not identical
    CHECK(evaluateCausal(s, s, s, n, {"repeat run differs"}).outcome == "INCONCLUSIVE");
    // Invalid evidence also blocks NOT_SUPPORTED
    CHECK(evaluateCausal(s, n, s, n, {"repeat run differs"}).outcome == "INCONCLUSIVE");
}

TEST_CASE("5e.3b P3 needs every raw peak measured: a missing peak is never the weakest attack (DIF-5e.3b-02)", "[trama_diagnosis]") {
    // Counterexample of the review: the ten missed references without a raw peak
    auto at2 = render(2.0, 40, 10, 12.0);
    for (std::size_t k = 0; k < 10; ++k) at2.refs[k].peakRaw.reset();
    CHECK(evaluateP3(at2).outcome == "INCONCLUSIVE");
    // A single missing, non-finite or negative peak, even on a matched reference
    auto one = render(2.0, 40, 10, 12.0);
    one.refs[25].peakRaw.reset();
    CHECK(evaluateP3(one).outcome == "INCONCLUSIVE");
    one = render(2.0, 40, 10, 12.0);
    one.refs[3].peakRaw = std::nan("");
    CHECK(evaluateP3(one).outcome == "INCONCLUSIVE");
    one = render(2.0, 40, 10, 12.0);
    one.refs[3].peakRaw = -1.0;
    CHECK(evaluateP3(one).outcome == "INCONCLUSIVE");
}
