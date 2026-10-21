#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "audio_probe.h"
#include "music_planner.h"

#include <array>
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
