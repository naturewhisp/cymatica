#include <catch2/catch_test_macros.hpp>

#include "audio_probe.h"
#include "procedural_scenario.h"
#include "staff_view.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <limits>

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
    REQUIRE(img.width == kStaffLabelWidth + 200);
    REQUIRE(img.height == staffViewHeight());
    constexpr std::size_t x0 = kStaffLabelWidth; // time axis origin
    REQUIRE_FALSE(img.png.empty());
    CHECK(renderStaffView(in).png == img.png); // deterministic

    const std::size_t corpoTop = staffLaneTop(1);
    // High note near the top of the note area, low note near the bottom, both in the corpo colour
    CHECK(pixel(img, x0 + 20, corpoTop + kStaffMarkerStrip + 1) == kStaffRoleColour[1]);
    CHECK(pixel(img, x0 + 120, corpoTop + kStaffLaneHeight - kStaffMarkerStrip - 2) == kStaffRoleColour[1]);
    // Applied NoteOn in the top strip (time 0), detections in the bottom strips with their colours
    CHECK(pixel(img, x0, corpoTop + 1) == kStaffAppliedColour);
    CHECK(pixel(img, x0 + 10, corpoTop + kStaffLaneHeight - 1) == kStaffOnsetColour[0]);
    CHECK(pixel(img, x0 + 100, staffLaneTop(2) + kStaffLaneHeight - 1) == kStaffOnsetColour[1]);
    CHECK(pixel(img, x0 + 150, staffLaneTop(3) + kStaffLaneHeight - 1) == kStaffOnsetColour[2]);
    // Nothing of another role leaks into the pulso lane's note area
    CHECK(pixel(img, x0 + 20, staffLaneTop(0) + 30) != kStaffRoleColour[1]);
    // Bar line at the second bar (x0 + 100) in a lane gap without notes
    CHECK(pixel(img, x0 + 100, staffLaneTop(0) + 30) == kStaffBarLineColour);

    // Readable views (P-M25-08): grid lines below the bar lines, instrument label, bar number and caption
    StaffViewInput grid = in;
    grid.beatFrames = {48000 + 24000};
    grid.stepFrames = {48000 + 6000, 48000 + 24000};    // a step under a beat: the beat wins
    grid.firstBarNumber = 33;
    grid.caption = "PHRASE 8  B  140 BPM 4/4";
    const StaffViewImage gi = renderStaffView(grid);
    CHECK(pixel(gi, x0 + 50, staffLaneTop(0) + 30) == kStaffBeatLineColour);
    CHECK(pixel(gi, x0 + 12, staffLaneTop(0) + 30) == kStaffStepLineColour);
    CHECK(pixel(gi, x0 + 100, staffLaneTop(0) + 30) == kStaffBarLineColour);
    const auto inkIn = [&](const StaffViewImage& im, std::size_t xa, std::size_t xb, std::size_t ya, std::size_t yb,
                           const std::array<std::uint8_t, 3>& colour) {
        std::size_t n = 0;
        for (std::size_t y = ya; y < yb; ++y) {
            for (std::size_t x = xa; x < xb; ++x) n += pixel(im, x, y) == colour ? 1 : 0;
        }
        return n;
    };
    // Each lane label is drawn in its role colour inside the label column, nothing of it right of the column
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        CHECK(inkIn(gi, 0, kStaffLabelWidth, staffLaneTop(r), staffLaneTop(r) + 30, kStaffRoleColour[r]) > 20);
    }
    // Header text (caption and bar numbers) exists only in the header rows; the footer carries the legend
    CHECK(inkIn(gi, 0, kStaffLabelWidth, 0, kStaffHeaderHeight, {170, 170, 170}) > 20);
    CHECK(inkIn(gi, x0, x0 + 30, 0, kStaffHeaderHeight, {170, 170, 170}) > 5);  // "33"
    CHECK(inkIn(gi, 0, gi.width, staffViewHeight() - kStaffFooterHeight, staffViewHeight(), kStaffOnsetColour[0]) > 20);
    CHECK(renderStaffView(grid).png == gi.png); // deterministic

    // Font: the glyphs used by the labels exist, unknown characters are blank, lower case is upper-cased
    for (const char* name : kStaffInstrumentNames) {
        for (const char* c = name; *c != 0; ++c) {
            if (*c != ' ') CHECK(staffGlyph(*c) != std::array<std::uint8_t, 7>{});
        }
    }
    CHECK(staffGlyph('?') == std::array<std::uint8_t, 7>{});
    CHECK(staffGlyph('k') == staffGlyph('K'));

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
    r.renderedFrames = 48000;
    r.truePeakMeasured = true;
    r.truePeakDbtp = -10.0;
    r.loudnessStatus = "OK";
    r.integratedLufs = -21.0;
    r.sections.push_back(SectionMeasures{0, 0, 48000, "OK", -21.0, -18.0, -20.0, -10.0});
    for (auto& o : r.onsets) {
        o.status = "MEASURED";
        o.precision50 = 1.0;
        o.recall50 = 1.0;
    }
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

TEST_CASE("Measure values are null unless their status is MEASURED (DIF-5e.1-R2-01)", "[procedural][measures]") {
    using nlohmann::json;
    ProceduralResult base;
    base.rendered = true;
    base.renderedFrames = 48000;
    base.truePeakMeasured = true;
    base.truePeakDbtp = -10.0;
    base.loudnessStatus = "OK";
    base.integratedLufs = -21.0;
    base.maxMomentaryLufs = -18.0;
    base.maxShortTermLufs = -20.0;
    base.sections.push_back(SectionMeasures{0, 0, 48000, "OK", -21.0, -18.0, -20.0, -10.0});
    for (auto& o : base.onsets) {
        o.status = "MEASURED";
        o.precision50 = 1.0;
        o.recall50 = 1.0;
        o.precisionExclReleases = 1.0;
    }
    WindowTruth w;
    w.id = "phrase_0";
    for (auto& rt : w.roles) {
        rt.signal = SignalLevel::Present;
        rt.rmsDbfs = -30.0;
        rt.stemToMixDb = -6.0;
    }
    base.phrasePresence.push_back(w);
    {
        const json j = json::parse(base.toJson());
        REQUIRE(proceduralMeasureStatuses(base).overall.status == "MEASURED");
        CHECK(j["measures"]["loudness"]["integrated_lufs"] == -21.0);
        CHECK(j["measures"]["sections"][0]["true_peak"]["dbtp"] == -10.0);
        CHECK(j["measures"]["sections"][0]["true_peak"]["status"]["status"] == "MEASURED");
    }
    // Reviewer case 1: OK with a non-finite Integrated value is not MEASURED
    {
        auto r = base;
        r.integratedLufs = std::numeric_limits<double>::quiet_NaN();
        CHECK(loudnessMeasureStatus("OK", r.integratedLufs).status == "INCONCLUSIVE");
        const json j = json::parse(r.toJson());
        CHECK(j["measures"]["loudness"]["status"]["status"] == "INCONCLUSIVE");
        CHECK(j["measures"]["loudness"]["integrated_lufs"].is_null());
        CHECK(j["measures"]["status"]["status"] == "INCONCLUSIVE");
    }
    CHECK(loudnessMeasureStatus("OK", -21.0, std::numeric_limits<double>::infinity()).status == "INCONCLUSIVE");
    // Reviewer case 2: an analysis error with a leftover numeric value serialises null, never the value
    {
        auto r = base;
        r.loudnessStatus = "NON_FINITE_INPUT";
        const json j = json::parse(r.toJson());
        CHECK(j["measures"]["loudness"]["status"]["status"] == "INCONCLUSIVE");
        CHECK(j["measures"]["loudness"]["integrated_lufs"].is_null());
        CHECK(j["measures"]["loudness"]["max_momentary_lufs"].is_null());
        CHECK(j["measures"]["loudness"]["max_short_term_lufs"].is_null());
    }
    // Reviewer case 3: a section with an invalid range has no true peak and no loudness values
    {
        auto r = base;
        r.sections[0] = SectionMeasures{0, 0, 48000, "INVALID_RANGE", std::nullopt, std::nullopt, std::nullopt, std::nullopt};
        const auto ms = proceduralMeasureStatuses(r);
        CHECK(ms.sectionTruePeaks[0].status == "INCONCLUSIVE");
        const json j = json::parse(r.toJson());
        CHECK(j["measures"]["sections"][0]["status"]["status"] == "INCONCLUSIVE");
        CHECK(j["measures"]["sections"][0]["true_peak"]["status"]["status"] == "INCONCLUSIVE");
        CHECK(j["measures"]["sections"][0]["true_peak"]["dbtp"].is_null());
        CHECK(j["measures"]["sections"][0]["integrated_lufs"].is_null());
        // The section true peak is independent of its loudness: valid TP, failed loudness
        r.sections[0] = SectionMeasures{0, 0, 48000, "NON_FINITE_INPUT", -21.0, std::nullopt, std::nullopt, -11.0};
        const json k = json::parse(r.toJson());
        CHECK(k["measures"]["sections"][0]["true_peak"]["dbtp"] == -11.0);
        CHECK(k["measures"]["sections"][0]["integrated_lufs"].is_null());
        // A non-finite section true peak is not MEASURED
        r.sections[0] = SectionMeasures{0, 0, 48000, "OK", -21.0, -18.0, -20.0, std::numeric_limits<double>::infinity()};
        CHECK(proceduralMeasureStatuses(r).sectionTruePeaks[0].status == "INCONCLUSIVE");
        CHECK(json::parse(r.toJson())["measures"]["sections"][0]["true_peak"]["dbtp"].is_null());
    }
    // No valid data from the capture: rendered, but no frames and nothing computed
    {
        ProceduralResult r;
        r.rendered = true;
        r.truePeakDbtp = 0.0;
        const auto ms = proceduralMeasureStatuses(r);
        CHECK(ms.truePeak.status == "INCONCLUSIVE");
        CHECK(ms.overall.status == "INCONCLUSIVE");
        const json j = json::parse(r.toJson());
        CHECK(j["measures"]["true_peak"]["dbtp"].is_null());
        CHECK(j["measures"]["true_peak"]["above_trial"].is_null());
        CHECK(j["measures"]["true_peak"]["max_gain_reduction_db"].is_null());
        CHECK(j["measures"]["loudness"]["integrated_lufs"].is_null());
    }
    // Onsets: an error status never serialises leftover numbers; MEASURED without finite values is INCONCLUSIVE
    {
        auto r = base;
        r.onsets[2].status = "NON_FINITE_INPUT";
        json j = json::parse(r.toJson());
        CHECK(j["onsets"]["roles"]["trama"]["precision_50"].is_null());
        CHECK(j["onsets"]["roles"]["trama"]["recall_50"].is_null());
        CHECK(j["onsets"]["roles"]["trama"]["precision_excl_releases_50"].is_null());
        r = base;
        r.onsets[1].recall50 = std::numeric_limits<double>::quiet_NaN();
        CHECK(proceduralMeasureStatuses(r).onsets.status == "INCONCLUSIVE");
        r.onsets[1].recall50.reset();
        CHECK(proceduralMeasureStatuses(r).onsets.status == "INCONCLUSIVE");
    }
}

TEST_CASE("Per-role onset status and values follow the effective status (DIF-5e.1-R3-01)", "[procedural][measures]") {
    using nlohmann::json;
    ProceduralResult r;
    r.rendered = true;
    r.renderedFrames = 48000;
    for (auto& o : r.onsets) {
        o.status = "MEASURED";
        o.references = 10;
        o.detections = 10;
        o.matched = 10;
        o.precision50 = 1.0;
        o.recall50 = 1.0;
        o.precisionExclReleases = 1.0;
    }
    // Reviewer counterexample: corpo MEASURED with precision 1.0 and recall NaN
    r.onsets[1].recall50 = std::numeric_limits<double>::quiet_NaN();
    const json j = json::parse(r.toJson());
    const json& corpo = j["onsets"]["roles"]["corpo"];
    CHECK(j["onsets"]["status"]["status"] == "INCONCLUSIVE");
    CHECK(corpo["status"] == "INCONCLUSIVE");
    CHECK(corpo["analysis_status"] == "MEASURED");
    CHECK_FALSE(corpo["reason"].get<std::string>().empty());
    CHECK(corpo["precision_50"].is_null()); // no partial evidence presented
    CHECK(corpo["recall_50"].is_null());
    CHECK(corpo["matched_50ms"].is_null());
    CHECK(corpo["precision_excl_releases_50"].is_null());
    // The other roles keep their valid measures
    CHECK(j["onsets"]["roles"]["pulso"]["status"] == "MEASURED");
    CHECK(j["onsets"]["roles"]["pulso"]["precision_50"] == 1.0);
    CHECK(j["onsets"]["roles"]["vettore"]["recall_50"] == 1.0);
    // Missing precision is the same as non-finite
    r.onsets[1].recall50 = 1.0;
    r.onsets[1].precision50.reset();
    CHECK(roleOnsetMeasureStatus(r.onsets[1]).status == "INCONCLUSIVE");
    // NO_REFERENCES keeps its approved meaning: no evidence, not an error, no values, aggregate unaffected
    r.onsets[1] = RoleOnsetSummary{};
    r.onsets[1].status = "NO_REFERENCES";
    const json k = json::parse(r.toJson());
    CHECK(k["onsets"]["roles"]["corpo"]["status"] == "NO_REFERENCES");
    CHECK(k["onsets"]["roles"]["corpo"]["recall_50"].is_null());
    CHECK(k["onsets"]["status"]["status"] == "MEASURED");
    // A detector error status is INCONCLUSIVE with the analysis status kept
    r.onsets[1].status = "NON_FINITE_INPUT";
    const json e = json::parse(r.toJson());
    CHECK(e["onsets"]["roles"]["corpo"]["status"] == "INCONCLUSIVE");
    CHECK(e["onsets"]["roles"]["corpo"]["analysis_status"] == "NON_FINITE_INPUT");
    CHECK(roleOnsetMeasureStatus(RoleOnsetSummary{}).status == "NOT_RUN");
}

TEST_CASE("Loudness per stem: values and relative levels only when measured (M2.5.1c)", "[procedural][measures][stem_loudness]") {
    using nlohmann::json;
    ProceduralResult r;
    const auto range = [](std::uint32_t section, double lufs) {
        return SectionMeasures{section, 0, 48000, "OK", lufs, lufs + 3.0, lufs + 1.0, std::nullopt};
    };
    r.busLoudness = {range(0, -20.0), {range(0, -20.0), range(1, -19.0)}};
    for (std::size_t i = 0; i < kRoleCount; ++i) {
        const double lufs = -30.0 + static_cast<double>(i);
        r.stemLoudness[i] = {range(0, lufs), {range(0, lufs), range(1, lufs - 2.0)}};
    }
    r.sections = {range(0, -20.0), range(1, -19.0)}; // mix sections: the ranges the bus must cover
    r.renderedFrames = 48000;                          // the bus whole range must be the whole render
    json s = json::parse(r.toJson())["measures"]["loudness_per_stem"];
    CHECK(s["status"]["status"] == "MEASURED");
    CHECK(s["roles"]["corpo"]["sections"][0]["aligned"] == true);
    CHECK(s["gate"] == false);
    CHECK(s["bus_mix"]["whole"]["integrated_lufs"] == -20.0);
    CHECK(s["roles"]["corpo"]["whole"]["integrated_lufs"] == -29.0);
    CHECK(s["roles"]["corpo"]["relative_to_bus_mix_lu"] == -9.0);
    CHECK(s["roles"]["pulso"]["sections"][1]["relative_to_bus_mix_lu"] == -13.0); // -32 - (-19)
    CHECK(s["roles"]["vettore"]["sections"][1]["section"] == 1);

    // A stem with nothing above the gates: its values and relative levels are null, the block is INCONCLUSIVE
    r.stemLoudness[2].sections[0].integratedLufs.reset();
    s = json::parse(r.toJson())["measures"]["loudness_per_stem"];
    CHECK(s["status"]["status"] == "INCONCLUSIVE");
    CHECK(s["status"]["reason"] == "trama section 0: INCONCLUSIVE");
    CHECK(s["roles"]["trama"]["sections"][0]["integrated_lufs"].is_null());
    CHECK(s["roles"]["trama"]["sections"][0]["relative_to_bus_mix_lu"].is_null());
    CHECK(s["roles"]["trama"]["whole"]["integrated_lufs"] == -28.0);

    // A bus mix that is not measured makes every relative level null
    r.stemLoudness[2].sections[0].integratedLufs = -30.0;
    r.busLoudness.whole.loudnessStatus = "NON_FINITE_INPUT";
    s = json::parse(r.toJson())["measures"]["loudness_per_stem"];
    CHECK(s["status"]["status"] == "INCONCLUSIVE");
    CHECK(s["roles"]["corpo"]["relative_to_bus_mix_lu"].is_null());
    CHECK(s["roles"]["corpo"]["whole"]["integrated_lufs"] == -29.0);

    // A stem without its last section is not complete (DIF-M2.5.1c-02)
    r.busLoudness.whole.loudnessStatus = "OK";
    REQUIRE(json::parse(r.toJson())["measures"]["loudness_per_stem"]["status"]["status"] == "MEASURED");
    r.stemLoudness[1].sections.pop_back();
    s = json::parse(r.toJson())["measures"]["loudness_per_stem"];
    CHECK(s["status"]["status"] == "INCONCLUSIVE");
    CHECK(s["status"]["reason"] == "corpo: 1 sections, 2 expected");
    r.stemLoudness[1].sections.push_back(range(1, -31.0));

    // A section with other boundaries: no relative level, INCONCLUSIVE
    r.stemLoudness[3].sections[1].endFrame = 47000;
    s = json::parse(r.toJson())["measures"]["loudness_per_stem"];
    CHECK(s["status"]["status"] == "INCONCLUSIVE");
    CHECK(s["roles"]["vettore"]["sections"][1]["aligned"] == false);
    CHECK(s["roles"]["vettore"]["sections"][1]["relative_to_bus_mix_lu"].is_null());
    CHECK(s["roles"]["vettore"]["sections"][0]["relative_to_bus_mix_lu"] == -7.0); // the aligned one stays
    r.stemLoudness[3].sections[1].endFrame = 48000;
    r.stemLoudness[3].sections[1].section = 7; // same frames, other section index
    CHECK(json::parse(r.toJson())["measures"]["loudness_per_stem"]["status"]["status"] == "INCONCLUSIVE");
    r.stemLoudness[3].sections[1].section = 1;

    // A whole range that differs from the bus mix
    r.stemLoudness[0].whole.endFrame = 40000;
    s = json::parse(r.toJson())["measures"]["loudness_per_stem"];
    CHECK(s["status"]["status"] == "INCONCLUSIVE");
    CHECK(s["roles"]["pulso"]["relative_to_bus_mix_lu"].is_null());
    r.stemLoudness[0].whole.endFrame = 48000;

    // A whole range shorter than the render, identical for bus and stems (DIF-M2.5.1c-R3-01)
    r.renderedFrames = 96000;
    s = json::parse(r.toJson())["measures"]["loudness_per_stem"];
    CHECK(s["status"]["status"] == "INCONCLUSIVE");
    CHECK(s["status"]["reason"] == "bus mix whole section 0 [0, 48000) is not the render [0, 96000)");
    r.renderedFrames = 48000;

    // Whole ranges measured but no section at all
    {
        ProceduralResult noSections = r;
        noSections.sections.clear();
        noSections.busLoudness.sections.clear();
        for (auto& stem : noSections.stemLoudness) stem.sections.clear();
        const json n = json::parse(noSections.toJson())["measures"]["loudness_per_stem"];
        CHECK(n["status"]["status"] == "INCONCLUSIVE");
        CHECK(n["status"]["reason"] == "no section to measure");
    }

    // A bus mix that does not cover the mix sections
    r.sections.push_back(range(2, -18.0));
    CHECK(json::parse(r.toJson())["measures"]["loudness_per_stem"]["status"]["status"] == "INCONCLUSIVE");
    r.sections.pop_back();
    CHECK(json::parse(r.toJson())["measures"]["loudness_per_stem"]["status"]["status"] == "MEASURED");

    // Nothing ran
    const json none = json::parse(ProceduralResult{}.toJson())["measures"]["loudness_per_stem"];
    CHECK(none["status"]["status"] == "NOT_RUN");
}

// Integration on the real fixture (OSS-M2.5.1c-03): four stems, ranges aligned with the mix, finite values.
// No LUFS value is binding.
TEST_CASE("Loudness per stem on the fixture render is complete and aligned (M2.5.1c)", "[procedural][stem_loudness][slow]") {
    using nlohmann::json;
    const ProceduralResult r = runProceduralV1("artifacts/cacp_test", false);
    REQUIRE(r.status == "PASS");
    const json m = json::parse(r.toJson())["measures"];
    const json s = m["loudness_per_stem"];
    CHECK(s["status"]["status"] == "MEASURED");
    REQUIRE(s["roles"].size() == kRoleCount);
    const auto finite = [](const json& v) { return v.is_number() && std::isfinite(v.get<double>()); };
    CHECK(finite(s["bus_mix"]["whole"]["integrated_lufs"]));
    REQUIRE(s["bus_mix"]["sections"].size() == m["sections"].size());
    for (std::size_t i = 0; i < m["sections"].size(); ++i) {
        CHECK(s["bus_mix"]["sections"][i]["start_frame"] == m["sections"][i]["start_frame"]);
        CHECK(s["bus_mix"]["sections"][i]["end_frame"] == m["sections"][i]["end_frame"]);
    }
    for (const char* role : kRoleNames) {
        const json& x = s["roles"][role];
        CHECK(finite(x["whole"]["integrated_lufs"]));
        CHECK(finite(x["relative_to_bus_mix_lu"]));
        REQUIRE(x["sections"].size() == m["sections"].size());
        for (const json& e : x["sections"]) {
            CHECK(e["aligned"] == true);
            CHECK(finite(e["relative_to_bus_mix_lu"]));
        }
    }
}
