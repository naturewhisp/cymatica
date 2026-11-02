#include <catch2/catch_test_macros.hpp>

#include "audio_probe.h"
#include "procedural_scenario.h"
#include "staff_view.h"

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
