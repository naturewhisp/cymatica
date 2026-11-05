#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "audio_probe.h"
#include "music_planner.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace cymatica::audio;

namespace {

// ADR-0005 D-M2-10 verdict for one workload/CACP pair: 3 repetitions; a repetition failing only on the
// block-period criterion gets a single re-run and both results are reported.
void checkWorkload(ProfileWorkload workload, bool cacpEnabled) {
    for (std::uint32_t rep = 0; rep < kProfileRepetitions; ++rep) {
        ProfileWorkloadRun run = runProfileWorkload(workload, cacpEnabled);
        INFO("Workload " << (workload == ProfileWorkload::W ? "W" : "F") << " CACP=" << (cacpEnabled ? "ON" : "OFF")
             << " rep " << rep << ": p50=" << run.p50Ms << " p99.9=" << run.p999Ms << " max=" << run.maxMs
             << " ms, over period=" << run.blocksExceedingBlockPeriod << ", GR=" << run.maxGainReductionDb
             << " dB, steals=" << run.voiceSteals);
        REQUIRE(run.stressConditionsMet);
        if (workload == ProfileWorkload::W) {
            CHECK(run.totalBlocks == kProfileWorkloadWBlocks);
        }
#ifdef NDEBUG
        if (run.p999Passed && !run.blockPeriodPassed) {
            const ProfileWorkloadRun retry = runProfileWorkload(workload, cacpEnabled);
            WARN("block-period retry: first run over period=" << run.blocksExceedingBlockPeriod
                 << ", retry over period=" << retry.blocksExceedingBlockPeriod);
            REQUIRE(retry.stressConditionsMet);
            run = retry;
        }
        CHECK(run.p999Passed);
        CHECK(run.blockPeriodPassed);
#endif
    }
}

} // namespace

TEST_CASE("Realtime Profile: Workload W 10000 blocks stress", "[perf][realtime_profile][workload_w]") {
    SECTION("CACP OFF") { checkWorkload(ProfileWorkload::W, false); }
    SECTION("CACP ON") { checkWorkload(ProfileWorkload::W, true); }
}

TEST_CASE("Realtime Profile: Workload F full 96-bar fixture", "[perf][realtime_profile][workload_f]") {
    SECTION("CACP OFF") { checkWorkload(ProfileWorkload::F, false); }
    SECTION("CACP ON") { checkWorkload(ProfileWorkload::F, true); }
}

TEST_CASE("CallbackProfileHistogram callback intervals (M2.5.1)", "[audio_engine][profile_histogram]") {
    CallbackProfileHistogram hist{};
    hist.recordInterval(1000000ULL, 480);    // first callback: no interval yet
    CHECK(hist.intervalCount == 0);
    hist.recordInterval(11000000ULL, 480);   // 10 ms
    hist.recordInterval(11500000ULL, 512);   // 0.5 ms
    hist.recordInterval(41500000ULL, 240);   // 30 ms
    hist.recordInterval(341500000ULL, 480);  // 300 ms: overflow bin
    hist.recordInterval(341000000ULL, 480);  // clock going back: ignored, not a huge interval
    CHECK(hist.intervalCount == 4);
    CHECK(hist.maxIntervalNs == 300000000ULL);
    CHECK(hist.minBlockFrames == 240);
    CHECK(hist.maxBlockFrames == 512);
    CHECK(hist.intervalsAtLeastMs(0) == 4);
    CHECK(hist.intervalsAtLeastMs(10) == 3);
    CHECK(hist.intervalsAtLeastMs(11) == 2);
    CHECK(hist.intervalsAtLeastMs(30) == 2);
    CHECK(hist.intervalsAtLeastMs(31) == 1);
    CHECK(hist.intervalsAtLeastMs(1000) == 1);

    hist.reset();
    CHECK(hist.intervalCount == 0);
    CHECK(hist.maxIntervalNs == 0);
    CHECK(hist.intervalsAtLeastMs(0) == 0);
    hist.recordInterval(5000000ULL, 480);
    CHECK(hist.intervalCount == 0); // reset forgets the previous start
}

TEST_CASE("Profiling histogram needs quiescence: offline engine (DIF-INC4-R3-05)", "[audio_engine][profile_histogram]") {
    AudioEngine engine;
    CallbackProfileHistogram hist{};
    CHECK_FALSE(engine.copyCallbackProfileHistogram(hist)); // not initialised
    CHECK_FALSE(engine.resetCallbackProfileHistogram());

    REQUIRE(engine.initOffline());
    std::vector<float> buffer(512 * 2, 0.0f);
    engine.processBlock(buffer.data(), 512);
    REQUIRE(engine.copyCallbackProfileHistogram(hist));
    CHECK(hist.totalBlocks == 1);
    CHECK(engine.resetCallbackProfileHistogram());
    REQUIRE(engine.copyCallbackProfileHistogram(hist));
    CHECK(hist.totalBlocks == 0);
    AudioDeviceReport report{};
    CHECK_FALSE(engine.deviceReport(report)); // no device
}

namespace {

DeviceProbeResult completeDeviceRun() {
    DeviceProbeResult r;
    r.reportValid = r.histogramValid = r.coordinatorStopped = true;
    r.startAckValid = r.tempoAcksValid = r.stopAckValid = true;
    r.phrasesPlanned = r.expectedPhrases = 24;
    r.generatedEvents = 1392;
    r.dispatchedEvents = r.engineProcessedEvents = 2784;
    return r;
}

} // namespace

TEST_CASE("device_probe_v1 fails closed on an incomplete musical load (DIF-M2.5.1a-02)", "[audio_engine][device_probe]") {
    CHECK(deviceProbeFailures(completeDeviceRun()).empty());
    CHECK_FALSE(deviceProbeFailures(DeviceProbeResult{}).empty());

    // Valid acks and a reached Stop never hide a missing load
    auto fails = [](auto mutate) {
        DeviceProbeResult r = completeDeviceRun();
        mutate(r);
        return !deviceProbeFailures(r).empty();
    };
    CHECK(fails([](DeviceProbeResult& r) { r.phrasesPlanned = 23; }));
    CHECK(fails([](DeviceProbeResult& r) { r.expectedPhrases = 0; r.phrasesPlanned = 0; }));
    CHECK(fails([](DeviceProbeResult& r) { r.generatedEvents = 0; r.dispatchedEvents = 0; r.engineProcessedEvents = 0; }));
    CHECK(fails([](DeviceProbeResult& r) { r.engineProcessedEvents = 2783; }));
    CHECK(fails([](DeviceProbeResult& r) { r.dispatchedEvents = 2783; }));
    CHECK(fails([](DeviceProbeResult& r) { r.stagedEventsAtStop = 1; }));
    CHECK(fails([](DeviceProbeResult& r) { r.backpressureDrops = 1; }));
    CHECK(fails([](DeviceProbeResult& r) { r.orphanNoteRecovered = 1; }));
    CHECK(fails([](DeviceProbeResult& r) { r.droppedEvents = 1; }));
    CHECK(fails([](DeviceProbeResult& r) { r.deferredEvents = 1; }));
    CHECK(fails([](DeviceProbeResult& r) { r.timedOut = true; }));
    CHECK(fails([](DeviceProbeResult& r) { r.stopAckValid = false; }));
}

TEST_CASE("device_probe_v1 separates measure and persistence (DIF-M2.5.1a-01)", "[audio_engine][device_probe]") {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "cymatica_device_probe_test";
    fs::remove_all(dir);
    fs::create_directories(dir);

    DeviceProbeResult r = completeDeviceRun();
    r.completed = true;
    r.usage = "default";
    CHECK(r.succeeded()); // nothing requested

    // Unwritable path: a regular file sits where the directory should be
    const fs::path blocker = dir / "blocker";
    std::ofstream(blocker) << "x";
    CHECK_FALSE(writeDeviceProbeManifest(r, (blocker / "sub").string()));
    CHECK_FALSE(r.manifestWriteSuccess);
    CHECK(r.completed);
    CHECK_FALSE(r.succeeded());

    REQUIRE(writeDeviceProbeManifest(r, (dir / "ok").string()));
    CHECK(r.succeeded());
    CHECK(fs::file_size(r.manifestFilePath) > 0);
    fs::remove_all(dir);
}

TEST_CASE("CallbackProfileHistogram calculation and percentiles (DIF-INC4-01)", "[audio_engine][profile_histogram]") {
    CallbackProfileHistogram hist{};
    hist.reset();
    REQUIRE(hist.totalBlocks == 0);
    CHECK(hist.percentileMs(50.0) == 0.0);
    CHECK(hist.meanMs() == 0.0);
    CHECK(hist.maxMs() == 0.0);

    // 500 blocks of 99 us -> bin 9 (upper bound 100 us = 0.10 ms)
    for (int i = 0; i < 500; ++i) hist.record(99000ULL, 512, 48000);
    // 400 blocks of 199 us -> bin 19 (upper bound 200 us = 0.20 ms)
    for (int i = 0; i < 400; ++i) hist.record(199000ULL, 512, 48000);
    // 90 blocks of 499 us -> bin 49 (upper bound 500 us = 0.50 ms)
    for (int i = 0; i < 90; ++i) hist.record(499000ULL, 512, 48000);
    // 9 blocks of 999 us -> bin 99 (upper bound 1000 us = 1.00 ms)
    for (int i = 0; i < 9; ++i) hist.record(999000ULL, 512, 48000);
    // 1 block of 3000 us (3.00 ms)
    hist.record(3000000ULL, 512, 48000);

    CHECK(hist.totalBlocks == 1000);
    CHECK(hist.maxMs() == 3.0);
    CHECK(hist.blocksExceeding2Ms == 1);
    CHECK(hist.blocksExceedingBlockPeriod == 0);

    // p is always a percentage: 0.5 is the 0.5th percentile, not the median
    CHECK(hist.percentileMs(50.0) == 0.10);
    CHECK(hist.percentileMs(0.5) == 0.10);
    CHECK(hist.percentileMs(1.0) == 0.10);
    CHECK(hist.percentileMs(60.0) == 0.20);
    CHECK(hist.percentileMs(0.0) == 0.10);
    CHECK(hist.percentileMs(-5.0) == 0.10);
    CHECK(hist.percentileMs(250.0) == 3.00);
    CHECK(hist.percentileMs(90.0) == 0.20);
    CHECK(hist.percentileMs(99.0) == 0.50);
    CHECK(hist.percentileMs(99.9) == 1.00);
    CHECK(hist.percentileMs(100.0) == 3.00);

    CHECK(hist.percentileMs(50.0) <= hist.percentileMs(90.0));
    CHECK(hist.percentileMs(90.0) <= hist.percentileMs(99.0));
    CHECK(hist.percentileMs(99.0) <= hist.percentileMs(99.9));
    CHECK(hist.percentileMs(99.9) <= hist.maxMs());
}
