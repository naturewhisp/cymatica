#include <catch2/catch_test_macros.hpp>

#include "audio_probe.h"
#include "procedural_scenario.h"
#include "staff_view.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

using namespace cymatica::cacp;

namespace {

cymatica::audio::FixtureProbeResult allBindingPassed() {
    cymatica::audio::FixtureProbeResult f;
    f.captureCompletePassed = true;
    f.determinismPassed = true;
    f.goldenApplicable = true;
    f.goldenTracePassed = true;
    f.goldenPlanPassed = true;
    f.noteOffPairingPassed = true;
    f.tempoAckPassed = true;
    f.schedulingAccountingPassed = true;
    f.nonSilentAudioPassed = true;
    f.roleActivityPassed = true;
    f.silenceBoundaryPassed = true;
    f.sampleCeilingPassed = true;
    f.clippedSamples = 0;
    f.dspAnomaliesPassed = true;
    f.safeStopPassed = true;
    f.eventChainPassed = true;
    f.coordinatorFailed = false;
    f.truePeakPassed = true;
    f.goldenPcmStatus = "PASS";
    return f;
}

std::array<std::uint8_t, 3> pixel(const StaffViewImage& img, std::size_t x, std::size_t y) {
    const std::size_t o = (y * img.width + x) * 3;
    return {img.rgb[o], img.rgb[o + 1], img.rgb[o + 2]};
}

} // namespace

TEST_CASE("procedural_v1 binding verdict: true peak is a measure, never a gate (5e-01, DIF-5e.0-01)", "[procedural]") {
    auto f = allBindingPassed();
    CHECK(proceduralBindingFailures(f).empty());
    // A true peak above -1 dBTP alone fails fixture_probe_v1's engineering check, not procedural_v1
    f.truePeakPassed = false;
    f.truePeakDbtp = -0.4f;
    CHECK(proceduralBindingFailures(f).empty());
    // Every binding check still fails the scenario
    auto check = [](auto mutate, const char* what) {
        auto g = allBindingPassed();
        mutate(g);
        INFO(what);
        CHECK_FALSE(proceduralBindingFailures(g).empty());
    };
    check([](auto& g) { g.captureCompletePassed = false; }, "capture");
    check([](auto& g) { g.determinismPassed = false; }, "determinism");
    check([](auto& g) { g.goldenPlanPassed = false; }, "golden plan");
    check([](auto& g) { g.eventChainPassed = false; }, "O3");
    check([](auto& g) { g.sampleCeilingPassed = false; }, "limiter sample ceiling");
    check([](auto& g) { g.clippedSamples = 1; }, "clipping");
    check([](auto& g) { g.dspAnomaliesPassed = false; }, "DSP anomalies");
    check([](auto& g) { g.coordinatorFailed = true; }, "coordinator");
    check([](auto& g) { g.silenceBoundaryPassed = false; }, "silence after Stop");
    // Goldens do not apply to another seed: not a failure by themselves
    auto other = allBindingPassed();
    other.goldenApplicable = false;
    other.goldenTracePassed = false;
    other.goldenPlanPassed = false;
    CHECK(proceduralBindingFailures(other).empty());
}

TEST_CASE("Staff view: lanes, notes by pitch, applied and detected markers (5e-06)", "[procedural][staff]") {
    StaffViewInput in;
    in.startFrame = 48000;
    in.endFrame = 48000 * 3; // 2 s at 100 px/s = 200 px
    in.pixelsPerSecond = 100.0;
    in.pitchRange = {{{50.0f, 200.0f}, {100.0f, 400.0f}, {200.0f, 800.0f}, {200.0f, 800.0f}}};
    in.planned.push_back({1, 48000, 48000 + 24000, 400.0f});        // corpo, top of its range, 0.5 s
    in.planned.push_back({1, 48000 + 48000, 48000 + 72000, 100.0f}); // corpo, bottom of its range
    in.appliedNoteOns.emplace_back(1, 48000);
    in.detected.push_back({1, 48000 + 4800, StaffOnsetKind::Matched});
    in.detected.push_back({2, 48000 + 48000, StaffOnsetKind::Unmatched});
    in.detected.push_back({3, 48000 + 72000, StaffOnsetKind::Release});
    in.barFrames = {48000, 96000};
    const StaffViewImage img = renderStaffView(in);
    REQUIRE(img.width == 200);
    REQUIRE(img.height == kStaffSeparator + kRoleCount * (kStaffLaneHeight + kStaffSeparator));
    REQUIRE_FALSE(img.png.empty());
    CHECK(renderStaffView(in).png == img.png); // deterministic

    const std::size_t corpoTop = staffLaneTop(1);
    // High note near the top of the note area, low note near the bottom, both in the corpo colour
    CHECK(pixel(img, 20, corpoTop + kStaffMarkerStrip + 1) == kStaffRoleColour[1]);
    CHECK(pixel(img, 120, corpoTop + kStaffLaneHeight - kStaffMarkerStrip - 2) == kStaffRoleColour[1]);
    // Applied NoteOn in the top strip (x = 0), detections in the bottom strips with their colours
    CHECK(pixel(img, 0, corpoTop + 1) == kStaffAppliedColour);
    CHECK(pixel(img, 10, corpoTop + kStaffLaneHeight - 1) == kStaffOnsetColour[0]);
    CHECK(pixel(img, 100, staffLaneTop(2) + kStaffLaneHeight - 1) == kStaffOnsetColour[1]);
    CHECK(pixel(img, 150, staffLaneTop(3) + kStaffLaneHeight - 1) == kStaffOnsetColour[2]);
    // Nothing of another role leaks into the pulso lane's note area
    CHECK(pixel(img, 20, staffLaneTop(0) + 30) != kStaffRoleColour[1]);
    // Bar line at the second bar (x = 100) in a lane gap without notes
    CHECK(pixel(img, 100, staffLaneTop(0) + 30)[0] == 55);

    // Invalid inputs: no image
    StaffViewInput bad = in;
    bad.endFrame = bad.startFrame;
    CHECK(renderStaffView(bad).png.empty());
    bad = in;
    bad.pixelsPerSecond = 0.0;
    CHECK(renderStaffView(bad).png.empty());
}

TEST_CASE("Golden PCM: an audio-only change fails, other seeds and faults are NOT_APPLICABLE (DIF-5e.1-01)", "[procedural][golden_pcm]") {
    auto f = allBindingPassed();
    f.pcmContentHash = cymatica::audio::kFixtureProbeV1GoldenPcmHash;
    CHECK(cymatica::audio::fixturePcmGoldenStatus(f) == "PASS");
    // Every other check valid, expected hash altered: the audio golden alone fails
    CHECK(cymatica::audio::fixturePcmGoldenStatus(f, cymatica::audio::kFixtureProbeV1GoldenPcmHash ^ 1ULL) == "FAIL");
    f.pcmContentHash ^= 1ULL; // a synthesis change that leaves events and decisions identical
    f.goldenPcmStatus = cymatica::audio::fixturePcmGoldenStatus(f);
    CHECK(f.goldenPcmStatus == "FAIL");
    const auto failures = proceduralBindingFailures(f);
    REQUIRE(failures.size() == 1);
    CHECK(failures[0].find("golden PCM") != std::string::npos);
    // Not computed is never a pass
    auto notRun = allBindingPassed();
    notRun.goldenPcmStatus = "NOT_RUN";
    CHECK_FALSE(proceduralBindingFailures(notRun).empty());
    // Another seed or an injected fault: the golden does not apply
    auto other = allBindingPassed();
    other.goldenApplicable = false;
    CHECK(cymatica::audio::fixturePcmGoldenStatus(other) == "NOT_APPLICABLE");
    auto fault = allBindingPassed();
    fault.injectedFault = cymatica::audio::FixtureInjectedFault::InvalidTempoMap;
    CHECK(cymatica::audio::fixturePcmGoldenStatus(fault) == "NOT_APPLICABLE");
}

TEST_CASE("procedural_v1 measure statuses come from the results, never assumed (DIF-5e.1-02)", "[procedural][measures]") {
    using nlohmann::json;
    // Nothing ran: NOT_RUN everywhere, no value presented as measured
    {
        const ProceduralResult empty{};
        const auto ms = proceduralMeasureStatuses(empty);
        CHECK(ms.overall.status == "NOT_RUN");
        const json j = json::parse(empty.toJson());
        CHECK(j["measures"]["status"]["status"] == "NOT_RUN");
        CHECK(j["measures"]["true_peak"]["status"]["status"] == "NOT_RUN");
        CHECK(j["measures"]["true_peak"]["dbtp"].is_null());
        CHECK(j["measures"]["loudness"]["status"]["status"] == "NOT_RUN");
        CHECK(j["onsets"]["status"]["status"] == "NOT_RUN");
        CHECK(j["presence_per_phrase"]["status"]["status"] == "NOT_RUN");
        CHECK(j["binding_evidence"]["status"] == "NOT_RUN");
    }
    // A complete set of valid measures
    ProceduralResult r;
    r.rendered = true;
    r.truePeakMeasured = true;
    r.truePeakDbtp = -10.0;
    r.loudnessStatus = "OK";
    r.integratedLufs = -21.0;
    r.sections.push_back(SectionMeasures{0, 0, 48000, "OK", -21.0, -18.0, -20.0, -10.0});
    for (auto& o : r.onsets) o.status = "MEASURED";
    WindowTruth w;
    w.id = "phrase_0";
    for (auto& rt : w.roles) rt.signal = SignalLevel::Present;
    r.phrasePresence.push_back(w);
    CHECK(proceduralMeasureStatuses(r).overall.status == "MEASURED");
    // NO_REFERENCES is no evidence, not an error
    r.onsets[2].status = "NO_REFERENCES";
    CHECK(proceduralMeasureStatuses(r).onsets.status == "MEASURED");
    // Partial results: each failed analysis keeps its reason and makes the overall INCONCLUSIVE
    auto expectInconclusive = [](ProceduralResult x, auto mutate, const char* which, const char* what) {
        mutate(x);
        const auto ms = proceduralMeasureStatuses(x);
        INFO(what);
        CHECK(ms.overall.status == "INCONCLUSIVE");
        const json j = json::parse(x.toJson());
        CHECK(j["measures"]["status"]["status"] == "INCONCLUSIVE");
        if (std::string(which) == "loudness") {
            CHECK(ms.loudness.status == "INCONCLUSIVE");
            CHECK_FALSE(ms.loudness.reason.empty());
        }
        if (std::string(which) == "onsets") CHECK(ms.onsets.reason.find("trama") != std::string::npos);
        if (std::string(which) == "true_peak") CHECK(j["measures"]["true_peak"]["dbtp"].is_null());
        return ms;
    };
    (void)expectInconclusive(r, [](ProceduralResult& x) { x.loudnessStatus = "NON_FINITE_INPUT"; }, "loudness", "loudness error");
    (void)expectInconclusive(r, [](ProceduralResult& x) { x.loudnessStatus = "UNSUPPORTED"; }, "loudness", "unsupported rate");
    (void)expectInconclusive(r, [](ProceduralResult& x) { x.integratedLufs.reset(); }, "loudness", "integrated undefined");
    (void)expectInconclusive(r, [](ProceduralResult& x) { x.sections[0].loudnessStatus = "INVALID_RANGE"; }, "section", "section range");
    (void)expectInconclusive(r, [](ProceduralResult& x) { x.onsets[2].status = "NON_FINITE_INPUT"; }, "onsets", "onset error");
    (void)expectInconclusive(r, [](ProceduralResult& x) { x.truePeakMeasured = false; }, "true_peak", "true peak missing");
    (void)expectInconclusive(r, [](ProceduralResult& x) { x.phrasePresence[0].roles[1].signal = SignalLevel::Unavailable; }, "presence",
                             "presence unavailable");
    (void)expectInconclusive(r, [](ProceduralResult& x) { x.sections.clear(); }, "section", "no section measured");
}

TEST_CASE("procedural_v1 manifest carries the binding evidence of the render (DIF-5e.1-03)", "[procedural][binding_evidence]") {
    using nlohmann::json;
    ProceduralResult r;
    auto f = allBindingPassed();
    f.chainExpectedEvents = 10;
    f.chainSentEvents = 10;
    f.chainConsumedEvents = 10;
    f.chainAppliedEvents = 10;
    f.truePeakPassed = false;
    r.fixture = f;
    json ev = json::parse(r.toJson())["binding_evidence"];
    CHECK(ev["checks"]["event_chain"] == "PASS");
    CHECK(ev["checks"]["golden_canonical_plan"] == "PASS");
    CHECK(ev["checks"]["golden_pcm"] == "PASS");
    CHECK(ev["checks"]["true_peak_engineering_check"]["result"] == "FAIL");
    CHECK(ev["checks"]["true_peak_engineering_check"]["binding_in_procedural_v1"] == false);
    CHECK(ev["event_chain"]["expected"] == 10);
    CHECK(ev["event_chain"]["applied"] == 10);
    CHECK(ev["hashes"].contains("golden_pcm_content"));
    CHECK(ev["scheduling"].contains("dropped_scheduled_events"));
    // Goldens of another seed are NOT_APPLICABLE, distinguishable from a pass
    f.goldenApplicable = false;
    f.goldenPcmStatus = "NOT_APPLICABLE";
    r.fixture = f;
    ev = json::parse(r.toJson())["binding_evidence"];
    CHECK(ev["checks"]["golden_decision_trace"] == "NOT_APPLICABLE");
    CHECK(ev["checks"]["golden_canonical_plan"] == "NOT_APPLICABLE");
    CHECK(ev["checks"]["golden_pcm"] == "NOT_APPLICABLE");
}
