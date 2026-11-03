#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "audio_probe.h"
#include "release_ab.h"
#include "voice_allocator.h"

#include <memory>
#include <vector>

using namespace cymatica::audio;
using cymatica::cacp::ReleaseSetMeasure;

namespace {

ScheduledAudioEvent noteOn(std::uint64_t id, std::uint64_t frame, MusicRole role = MusicRole::Corpo, std::uint32_t durationFrames = 0) {
    return ScheduledAudioEvent{.eventId = id, .sampleFrame = frame, .transportEpoch = 1, .type = AudioEventType::NoteOn,
                               .role = role, .valueA = 220.0f, .valueB = 0.8f, .durationFrames = durationFrames};
}

ScheduledAudioEvent noteOff(std::uint64_t id, std::uint64_t frame, MusicRole role = MusicRole::Corpo) {
    return ScheduledAudioEvent{.eventId = id, .sampleFrame = frame, .transportEpoch = 1, .type = AudioEventType::NoteOff, .role = role};
}

// Offline engine with the release override; the allocator is driven directly through the engine's accessor
std::unique_ptr<AudioEngine> offlineEngine(std::uint32_t releaseFrames) {
    auto engine = std::make_unique<AudioEngine>();
    REQUIRE(engine->initOffline({48000, 2, 220.0f}));
    REQUIRE(engine->setNoteReleaseFrames(releaseFrames));
    return engine;
}

// Frames rendered after the NoteOff until the slot returns to Idle (bounded)
std::uint32_t releaseLength(std::uint32_t overrideFrames) {
    auto engine = offlineEngine(overrideFrames);
    VoiceAllocator& va = engine->voiceAllocator();
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

// A measure whose roles are all valid: role 0 MEASURED with the given release detections, the others NO_REFERENCES
ReleaseSetMeasure measure(const char* set, double ms, std::size_t releaseDetections, std::uint64_t hash = 7) {
    ReleaseSetMeasure m;
    m.set = set;
    m.releaseMs = ms;
    m.eventsHash = hash;
    for (auto& r : m.roles) r.status = "NO_REFERENCES";
    m.roles[0].status = "MEASURED";
    m.roles[0].releaseDetections = releaseDetections;
    return m;
}

ReleaseSetMeasure tramaRecall(double ms, std::optional<double> recall) {
    auto m = measure("set_b", ms, 0);
    m.roles[2].status = recall ? "MEASURED" : "INSUFFICIENT_FRAMES";
    m.roles[2].recall50 = recall;
    return m;
}

template <class T>
concept CanSetNoteRelease = requires(T& v) { v.setNoteReleaseFrames(1440U); };

} // namespace

TEST_CASE("Release override: the default is the 2 ms de-click, an override changes only ordinary NoteOff releases (5e-08)",
          "[release_ab][voice_allocator]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    VoiceAllocator& va = engine.voiceAllocator();
    CHECK(va.noteReleaseFrames() == 96);
    REQUIRE(va.onNoteOn(noteOn(1, 0)) >= 0);
    CHECK(va.slot(2).releaseSamples == 96);
    REQUIRE(engine.setNoteReleaseFrames(1440));
    CHECK(va.noteReleaseFrames() == 1440);
    REQUIRE(engine.setNoteReleaseFrames(0));
    CHECK(va.noteReleaseFrames() == 96);
    engine.shutdown();

    // Release length measured on the rendered state: 96 frames by default, 1440 frames (30 ms) with the override
    const auto def = releaseLength(0);
    const auto ovr = releaseLength(1440);
    CHECK(def >= 96);
    CHECK(def <= 97);
    CHECK(ovr >= 1440);
    CHECK(ovr <= 1441);
}

TEST_CASE("Release override is accepted by offline engines only and cannot bypass the engine (DIF-5e.3-01)", "[release_ab]") {
    // The allocator setter is private: the engine is its only writer
    STATIC_REQUIRE_FALSE(CanSetNoteRelease<VoiceAllocator>);
    // Rule of the engine, testable without hardware: an engine with a physical device refuses, started or not
    STATIC_REQUIRE(AudioEngine::noteReleaseOverrideAllowed(true, false));
    STATIC_REQUIRE_FALSE(AudioEngine::noteReleaseOverrideAllowed(true, true));
    STATIC_REQUIRE_FALSE(AudioEngine::noteReleaseOverrideAllowed(false, false));
    STATIC_REQUIRE_FALSE(AudioEngine::noteReleaseOverrideAllowed(false, true));
    // An engine never initialised refuses; an offline engine accepts
    AudioEngine none;
    CHECK_FALSE(none.setNoteReleaseFrames(1440));
    AudioEngine offline;
    REQUIRE(offline.initOffline({48000, 2, 220.0f}));
    CHECK(offline.setNoteReleaseFrames(1440));
    CHECK(offline.voiceAllocator().noteReleaseFrames() == 1440);
}

TEST_CASE("Release override: steal tails keep the 2 ms de-click", "[release_ab][voice_allocator]") {
    auto engine = offlineEngine(3840); // 80 ms
    VoiceAllocator& va = engine->voiceAllocator();
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

TEST_CASE("Release override: safety releases keep 2 ms (duration failsafe and open-pad lease, DIF-5e.3-02)", "[release_ab][voice_allocator]") {
    std::vector<float> buf(2 * 200);
    SECTION("finite duration failsafe") {
        auto engine = offlineEngine(3840);
        VoiceAllocator& va = engine->voiceAllocator();
        REQUIRE(va.onNoteOn(noteOn(1, 10000, MusicRole::Trama, 4800)) == 4);
        CHECK(va.slot(4).releaseSamples == 3840);
        va.checkLifetimes(14800);
        REQUIRE(va.slot(4).state == VoiceState::Release);
        CHECK(va.slot(4).releaseSamples == 96);
        va.renderBlock(buf.data(), 97, 14800, false);
        CHECK(va.slot(4).state == VoiceState::Idle);
    }
    SECTION("open-pad lease backstop") {
        auto engine = offlineEngine(3840);
        VoiceAllocator& va = engine->voiceAllocator();
        REQUIRE(va.onNoteOn(noteOn(500, 1000, MusicRole::Corpo, 0)) == 2);
        va.checkLifetimes(1000 + VoiceAllocator::kMaxLeaseDurationFrames);
        REQUIRE(va.slot(2).state == VoiceState::Release);
        CHECK(va.orphanNoteRecoveredCount() == 1);
        CHECK(va.slot(2).releaseSamples == 96);
    }
}

TEST_CASE("Release confounders: overlaps and release cuts are counted in total and per role (5e-08, DIF-5e.3-03)",
          "[release_ab][voice_allocator]") {
    std::vector<float> buf(2 * 600);
    {
        // Overlap: a NoteOn while the other voice of the role is still releasing
        auto engine = offlineEngine(2400);
        VoiceAllocator& va = engine->voiceAllocator();
        REQUIRE(va.onNoteOn(noteOn(1, 0)) == 2);
        va.renderBlock(buf.data(), 200, 0, false);
        REQUIRE(va.onNoteOff(noteOff(1, 200)) == 2);
        va.renderBlock(buf.data(), 100, 200, false);
        REQUIRE(va.slot(2).state == VoiceState::Release);
        REQUIRE(va.onNoteOn(noteOn(2, 300)) == 3); // free slot first
        CHECK(va.releaseOverlapCount() == 1);
        CHECK(va.releaseOverlapCount(MusicRole::Corpo) == 1);
        CHECK(va.releaseOverlapCount(MusicRole::Trama) == 0);
        CHECK(va.releaseCutCount() == 0);
        CHECK(va.voiceDropCount() == 0);
    }
    {
        // Cut on Corpo and a drop on Trama: totals equal the sums of the roles
        auto engine = offlineEngine(2400);
        VoiceAllocator& va = engine->voiceAllocator();
        REQUIRE(va.onNoteOn(noteOn(1, 0)) == 2);
        REQUIRE(va.onNoteOn(noteOn(2, 0)) == 3);
        REQUIRE(va.onNoteOn(noteOn(10, 0, MusicRole::Trama)) == 4);
        REQUIRE(va.onNoteOn(noteOn(11, 0, MusicRole::Trama)) == 5);
        va.renderBlock(buf.data(), 200, 0, false);
        REQUIRE(va.onNoteOff(noteOff(1, 200)) == 2);
        va.renderBlock(buf.data(), 100, 200, false);
        REQUIRE(va.onNoteOn(noteOn(3, 300)) == 2);
        REQUIRE(va.onNoteOn(noteOn(12, 300, MusicRole::Trama)) >= 4);
        CHECK(va.releaseCutCount(MusicRole::Corpo) == 1);
        CHECK(va.voiceDropCount(MusicRole::Trama) == 1);
        CHECK(va.releaseCutCount() == 1);
        CHECK(va.voiceDropCount() == 1);

        const VoiceConfounders c = voiceConfoundersOf(va);
        std::uint32_t drops = 0, cuts = 0, overlaps = 0;
        for (std::size_t r = 0; r < 4; ++r) {
            drops += c.roleVoiceDrops[r];
            cuts += c.roleReleaseCuts[r];
            overlaps += c.roleReleaseOverlaps[r];
        }
        CHECK(drops == c.voiceDrops);
        CHECK(cuts == c.releaseCuts);
        CHECK(overlaps == c.releaseOverlaps);
        CHECK(va.voiceDropCount(static_cast<MusicRole>(9)) == 0); // out-of-range role reads 0
    }
}

TEST_CASE("Release override on the engine: golden PCM NOT_APPLICABLE (5e-08)", "[release_ab][golden_pcm]") {
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

TEST_CASE("release_ab_v1 H1 is fail-closed on role analyses (DIF-5e.3-06)", "[release_ab]") {
    using cymatica::cacp::evaluateH1;
    std::vector<ReleaseSetMeasure> ms{measure("set_b", 2.0, 10), measure("set_b", 30.0, 2), measure("planner_probe_v1", 2.0, 20),
                                      measure("planner_probe_v1", 30.0, 4), measure("fixture", 2.0, 50), measure("fixture", 30.0, 10)};
    REQUIRE(evaluateH1(ms).outcome == "SUPPORTED");
    // A role whose analysis did not run at 30 ms must not count as zero detections
    auto notRun = ms;
    notRun[5].roles[0] = {};
    CHECK(notRun[5].roles[0].status == "NOT_RUN");
    CHECK(evaluateH1(notRun).outcome == "INCONCLUSIVE");
    // An invalid analysis status, even with release detections recorded
    auto invalid = ms;
    invalid[1].roles[3].status = "NON_FINITE_INPUT";
    invalid[1].roles[3].releaseDetections = 99;
    CHECK(evaluateH1(invalid).outcome == "INCONCLUSIVE");
    // NO_REFERENCES is valid and contributes nothing
    auto noRefs = ms;
    noRefs[1].roles[1].releaseDetections = 99; // ignored: the role has no references
    CHECK(noRefs[1].releaseDetectionsTotal() == 2);
    CHECK(evaluateH1(noRefs).outcome == "SUPPORTED");
}

TEST_CASE("release_ab_v1 H2: Trama recall on set B at 30 ms >= 0.90 and above the 2 ms baseline (DIF-5e.3-04)", "[release_ab]") {
    using cymatica::cacp::evaluateH2;
    CHECK(evaluateH2({tramaRecall(2.0, 0.75), tramaRecall(30.0, 1.0)}).outcome == "SUPPORTED");
    CHECK(evaluateH2({tramaRecall(2.0, 0.75), tramaRecall(30.0, 0.90)}).outcome == "SUPPORTED");
    CHECK(evaluateH2({tramaRecall(2.0, 0.75), tramaRecall(30.0, 0.875)}).outcome == "NOT_SUPPORTED");
    // No improvement: 1.000 at both releases is not supported
    CHECK(evaluateH2({tramaRecall(2.0, 1.0), tramaRecall(30.0, 1.0)}).outcome == "NOT_SUPPORTED");
    CHECK(evaluateH2({tramaRecall(2.0, 0.95), tramaRecall(30.0, 0.92)}).outcome == "NOT_SUPPORTED");
    // Both measures are required
    CHECK(evaluateH2({tramaRecall(30.0, 1.0)}).outcome == "INCONCLUSIVE");
    CHECK(evaluateH2({tramaRecall(2.0, std::nullopt), tramaRecall(30.0, 1.0)}).outcome == "INCONCLUSIVE");
    CHECK(evaluateH2({tramaRecall(2.0, 0.75), tramaRecall(30.0, std::nullopt)}).outcome == "INCONCLUSIVE");
    auto invalid = tramaRecall(2.0, 0.75);
    invalid.captureInvalid = "stem overflow";
    CHECK(evaluateH2({invalid, tramaRecall(30.0, 1.0)}).outcome == "INCONCLUSIVE");
}

TEST_CASE("release_ab_v1 R2 short-IOI sub-hypothesis: the unmatched Trama references are the 10 notes after 0.18 s / 0.22 s",
          "[release_ab]") {
    using cymatica::cacp::evaluateR2ShortIoi;
    std::vector<std::size_t> idx(10);
    std::vector<double> prev(10);
    for (std::size_t i = 0; i < 10; ++i) {
        idx[i] = i * 4 + 1;
        prev[i] = (i % 2 == 0) ? 0.18 : 0.22;
    }
    CHECK(evaluateR2ShortIoi(idx, prev, 40).outcome == "SUPPORTED");
    prev[3] = 0.45;
    CHECK(evaluateR2ShortIoi(idx, prev, 40).outcome == "NOT_SUPPORTED");
    prev[3] = 0.22;
    idx.pop_back();
    prev.pop_back();
    CHECK(evaluateR2ShortIoi(idx, prev, 40).outcome == "NOT_SUPPORTED"); // 9 of 10
    CHECK(evaluateR2ShortIoi({}, {}, 0).outcome == "INCONCLUSIVE");
    // The causal explanation is never decided by this scenario
    const cymatica::cacp::ReleaseAbResult r;
    CHECK(r.r2Causal.outcome == "INCONCLUSIVE");
}
