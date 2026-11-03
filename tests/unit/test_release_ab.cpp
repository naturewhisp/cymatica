#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "audio_probe.h"
#include "release_ab.h"
#include "voice_allocator.h"

#include <vector>

using namespace cymatica::audio;
using cymatica::cacp::ReleaseSetMeasure;

namespace {

ScheduledAudioEvent noteOn(std::uint64_t id, std::uint64_t frame, MusicRole role = MusicRole::Corpo) {
    return ScheduledAudioEvent{.eventId = id, .sampleFrame = frame, .transportEpoch = 1, .type = AudioEventType::NoteOn,
                               .role = role, .valueA = 220.0f, .valueB = 0.8f};
}

ScheduledAudioEvent noteOff(std::uint64_t id, std::uint64_t frame, MusicRole role = MusicRole::Corpo) {
    return ScheduledAudioEvent{.eventId = id, .sampleFrame = frame, .transportEpoch = 1, .type = AudioEventType::NoteOff, .role = role};
}

// Frames rendered after the NoteOff until the slot returns to Idle (bounded)
std::uint32_t releaseLength(std::uint32_t overrideFrames) {
    VoiceAllocator va;
    va.init(48000);
    va.setNoteReleaseFrames(overrideFrames);
    const int slot = va.onNoteOn(noteOn(1, 0));
    REQUIRE(slot >= 0);
    std::vector<float> buf(2 * 2000);
    va.renderBlock(buf.data(), 500, 0, false);
    REQUIRE(va.slot(static_cast<std::size_t>(slot)).state != VoiceState::Idle);
    REQUIRE(va.onNoteOff(noteOff(1, 500)) == slot);
    std::uint32_t n = 0;
    while (va.slot(static_cast<std::size_t>(slot)).state != VoiceState::Idle && n < 10000) {
        va.renderBlock(buf.data(), 1, 500 + n, false);
        ++n;
    }
    return n;
}

ReleaseSetMeasure measure(const char* set, double ms, std::size_t releaseDetections, std::uint64_t hash = 7) {
    ReleaseSetMeasure m;
    m.set = set;
    m.releaseMs = ms;
    m.eventsHash = hash;
    m.roles[0].status = "MEASURED";
    m.roles[0].releaseDetections = releaseDetections;
    return m;
}

} // namespace

TEST_CASE("Release override: the default is the 2 ms de-click, an override changes only ordinary NoteOff releases (5e-08)",
          "[release_ab][voice_allocator]") {
    VoiceAllocator va;
    va.init(48000);
    CHECK(va.noteReleaseFrames() == 96);
    REQUIRE(va.onNoteOn(noteOn(1, 0)) >= 0);
    CHECK(va.slot(2).releaseSamples == 96);
    va.setNoteReleaseFrames(1440);
    CHECK(va.noteReleaseFrames() == 1440);
    va.setNoteReleaseFrames(0);
    CHECK(va.noteReleaseFrames() == 96);

    // Release length measured on the rendered state: 96 frames by default, 1440 frames (30 ms) with the override
    const auto def = releaseLength(0);
    const auto ovr = releaseLength(1440);
    CHECK(def >= 96);
    CHECK(def <= 97);
    CHECK(ovr >= 1440);
    CHECK(ovr <= 1441);
}

TEST_CASE("Release override: steal tails keep the 2 ms de-click", "[release_ab][voice_allocator]") {
    VoiceAllocator va;
    va.init(48000);
    va.setNoteReleaseFrames(3840); // 80 ms
    std::vector<float> buf(2 * 600);
    REQUIRE(va.onNoteOn(noteOn(1, 0)) == 2);
    va.renderBlock(buf.data(), 200, 0, false);
    REQUIRE(va.onNoteOn(noteOn(2, 200)) == 3);
    va.renderBlock(buf.data(), 200, 200, false);
    REQUIRE(va.onNoteOn(noteOn(3, 400)) == 2); // both active: the older one is stolen
    CHECK(va.voiceDropCount() == 1);
    CHECK(va.slot(2).stealTail.active);
    CHECK(va.slot(2).stealTail.totalSamples == 96);
    CHECK(va.slot(2).releaseSamples == 3840); // the new note in that slot takes the experimental release
}

TEST_CASE("Release confounders: overlaps and release cuts are counted (5e-08)", "[release_ab][voice_allocator]") {
    std::vector<float> buf(2 * 600);
    {
        // Overlap: a NoteOn while the other voice of the role is still releasing
        VoiceAllocator va;
        va.init(48000);
        va.setNoteReleaseFrames(2400);
        REQUIRE(va.onNoteOn(noteOn(1, 0)) == 2);
        va.renderBlock(buf.data(), 200, 0, false);
        REQUIRE(va.onNoteOff(noteOff(1, 200)) == 2);
        va.renderBlock(buf.data(), 100, 200, false);
        REQUIRE(va.slot(2).state == VoiceState::Release);
        REQUIRE(va.onNoteOn(noteOn(2, 300)) == 3); // free slot first
        CHECK(va.releaseOverlapCount() == 1);
        CHECK(va.releaseCutCount() == 0);
        CHECK(va.voiceDropCount() == 0);
    }
    {
        // Cut: the new NoteOn takes the slot still in Release
        VoiceAllocator va;
        va.init(48000);
        va.setNoteReleaseFrames(2400);
        REQUIRE(va.onNoteOn(noteOn(1, 0)) == 2);
        REQUIRE(va.onNoteOn(noteOn(2, 0)) == 3);
        va.renderBlock(buf.data(), 200, 0, false);
        REQUIRE(va.onNoteOff(noteOff(1, 200)) == 2);
        va.renderBlock(buf.data(), 100, 200, false);
        REQUIRE(va.onNoteOn(noteOn(3, 300)) == 2);
        CHECK(va.releaseCutCount() == 1);
        CHECK(va.releaseOverlapCount() == 0);
        CHECK(va.voiceDropCount() == 0);
    }
}

TEST_CASE("Release override on the engine: set in quiescence, golden PCM NOT_APPLICABLE (5e-08)", "[release_ab][golden_pcm]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    CHECK(engine.voiceAllocator().noteReleaseFrames() == 96);
    CHECK(engine.setNoteReleaseFrames(1440));
    CHECK(engine.voiceAllocator().noteReleaseFrames() == 1440);
    engine.shutdown();

    FixtureProbeResult f;
    f.pcmContentHash = kFixtureProbeV1GoldenPcmHash;
    CHECK(fixturePcmGoldenStatus(f) == "PASS");
    f.noteReleaseOverride = 1440;
    CHECK(fixturePcmGoldenStatus(f) == "NOT_APPLICABLE");
    f.pcmContentHash ^= 1ULL; // the audio differs by design: still not a golden FAIL
    CHECK(fixturePcmGoldenStatus(f) == "NOT_APPLICABLE");
}

TEST_CASE("release_ab_v1 same-events check: every variant matches its 2 ms baseline", "[release_ab]") {
    using cymatica::cacp::sameEventsViolations;
    std::vector<ReleaseSetMeasure> ms{measure("set_b", 2.0, 10), measure("set_b", 30.0, 1), measure("fixture", 2.0, 5, 9),
                                      measure("fixture", 30.0, 0, 9)};
    CHECK(sameEventsViolations(ms).empty());
    ms[1].eventsHash = 8;
    const auto v = sameEventsViolations(ms);
    REQUIRE(v.size() == 1);
    CHECK(v[0] == "set_b 30ms");
    // No baseline: every variant of that set is a violation
    std::vector<ReleaseSetMeasure> noBase{measure("set_a", 15.0, 0)};
    REQUIRE(sameEventsViolations(noBase).size() == 1);
}

TEST_CASE("release_ab_v1 H1: release detections at 30 ms <= 20 % of 2 ms in set B, planner and fixture", "[release_ab]") {
    using cymatica::cacp::evaluateH1;
    std::vector<ReleaseSetMeasure> ms{measure("set_b", 2.0, 10), measure("set_b", 30.0, 2), measure("planner_probe_v1", 2.0, 20),
                                      measure("planner_probe_v1", 30.0, 4), measure("fixture", 2.0, 50), measure("fixture", 30.0, 10)};
    CHECK(evaluateH1(ms).outcome == "SUPPORTED"); // exactly 20 % is supported
    ms[5].roles[0].releaseDetections = 11;
    CHECK(evaluateH1(ms).outcome == "NOT_SUPPORTED");
    ms[4].roles[0].releaseDetections = 0;
    CHECK(evaluateH1(ms).outcome == "INCONCLUSIVE"); // zero baseline
    ms[4].roles[0].releaseDetections = 50;
    ms[3].captureInvalid = "stem overflow";
    CHECK(evaluateH1(ms).outcome == "INCONCLUSIVE");
    ms.pop_back();
    ms[3].captureInvalid.clear();
    CHECK(evaluateH1(ms).outcome == "INCONCLUSIVE"); // missing fixture at 30 ms
}

TEST_CASE("release_ab_v1 H2: Trama recall on set B at 30 ms >= 0.90", "[release_ab]") {
    using cymatica::cacp::evaluateH2;
    auto m = measure("set_b", 30.0, 0);
    m.roles[2].status = "MEASURED";
    m.roles[2].recall50 = 0.90;
    CHECK(evaluateH2({m}).outcome == "SUPPORTED");
    m.roles[2].recall50 = 0.875;
    CHECK(evaluateH2({m}).outcome == "NOT_SUPPORTED");
    m.roles[2].status = "INSUFFICIENT_FRAMES";
    m.roles[2].recall50.reset();
    CHECK(evaluateH2({m}).outcome == "INCONCLUSIVE");
    CHECK(evaluateH2({measure("set_b", 2.0, 0)}).outcome == "INCONCLUSIVE");
}

TEST_CASE("release_ab_v1 R2: the unmatched Trama references are the 10 notes after 0.18 s / 0.22 s", "[release_ab]") {
    using cymatica::cacp::evaluateR2;
    std::vector<std::size_t> idx(10);
    std::vector<double> prev(10);
    for (std::size_t i = 0; i < 10; ++i) {
        idx[i] = i * 4 + 1;
        prev[i] = (i % 2 == 0) ? 0.18 : 0.22;
    }
    CHECK(evaluateR2(idx, prev, 40).outcome == "SUPPORTED");
    prev[3] = 0.45;
    CHECK(evaluateR2(idx, prev, 40).outcome == "NOT_SUPPORTED");
    prev[3] = 0.22;
    idx.pop_back();
    prev.pop_back();
    CHECK(evaluateR2(idx, prev, 40).outcome == "NOT_SUPPORTED"); // 9 of 10
    CHECK(evaluateR2({}, {}, 0).outcome == "INCONCLUSIVE");
}
