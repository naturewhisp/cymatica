#include <catch2/catch_test_macros.hpp>

#include "fixed_step.h"

using namespace cymatica::core;

TEST_CASE("FixedStepAccumulator boundary formula matches Spec section 7.5 exactly", "[fixed_step][boundary]") {
    FixedStepAccumulator acc({48000, 120, 4, 16});

    REQUIRE(acc.tickToSampleFrame(0) == 0);
    REQUIRE(acc.tickToSampleFrame(1) == 400); // 48000 / 120 = 400
    REQUIRE(acc.tickToSampleFrame(120) == 48000);
    // 1 virtual hour: 120 * 3600 = 432,000 ticks = 172,800,000 frames
    REQUIRE(acc.tickToSampleFrame(120ULL * 3600ULL) == 48000ULL * 3600ULL);

    REQUIRE(acc.sampleFrameToTick(0) == 0);
    REQUIRE(acc.sampleFrameToTick(400) == 1);
    REQUIRE(acc.sampleFrameToTick(48000) == 120);
}

TEST_CASE("60 FPS and 120 FPS feeds produce equivalent simulation tick sequences", "[fixed_step][equivalence]") {
    // Acceptance criterion Spec §32: 60/120 FPS producono stato equivalente
    FixedStepAccumulator acc60({48000, 120, 4, 16});
    FixedStepAccumulator acc120({48000, 120, 4, 16});

    // 2.0 seconds of simulation:
    // 60 FPS: 120 updates of ~16.666 ms
    // 120 FPS: 240 updates of ~8.333 ms
    constexpr std::uint64_t ns60 = 1'000'000'000ULL / 60ULL;
    constexpr std::uint64_t ns120 = 1'000'000'000ULL / 120ULL;

    for (int i = 0; i < 120; ++i) {
        acc60.advanceNs(ns60);
    }

    for (int i = 0; i < 240; ++i) {
        acc120.advanceNs(ns120);
    }

    // Both must have executed exactly 240 ticks over 2 seconds (120 Hz * 2s)
    REQUIRE(acc60.currentTick() == 240);
    REQUIRE(acc120.currentTick() == 240);
}

TEST_CASE("FixedStepAccumulator limits catch-up per update without silently dropping ticks", "[fixed_step][catch_up]") {
    FixedStepAccumulator acc({48000, 120, 4, 16});

    // Feed a spike of 8 ticks of delta time (66.66 ms)
    constexpr std::uint64_t spikeNs = 8ULL * (1'000'000'000ULL / 120ULL);
    const auto step1 = acc.advanceNs(spikeNs);

    // Max catch up per update is 4: exactly 4 ticks executed in this update
    REQUIRE(step1.ticksToRun == 4);
    REQUIRE(step1.overloadSuspensionRequired == false);
    REQUIRE(acc.currentTick() == 4);

    // On the next nominal update (0 delta), the remaining 4 ticks are executed
    const auto step2 = acc.advanceNs(0);
    REQUIRE(step2.ticksToRun == 4);
    REQUIRE(acc.currentTick() == 8); // ALL 8 ticks executed, zero ticks lost!
}

TEST_CASE("FixedStepAccumulator signals technical suspension on severe overload", "[fixed_step][overload]") {
    FixedStepAccumulator acc({48000, 120, 4, 16});

    // Severe freeze: 20 ticks of debt (> overloadThresholdTicks = 16)
    constexpr std::uint64_t stallNs = 20ULL * (1'000'000'000ULL / 120ULL);
    const auto step = acc.advanceNs(stallNs);

    REQUIRE(step.overloadSuspensionRequired == true);
    // Still runs up to maxCatchUpTicksPerUpdate (4 ticks)
    REQUIRE(step.ticksToRun == 4);

    // System reconciles after technical suspension (§20.3)
    acc.reconcileAfterSuspension();
    REQUIRE(acc.accumulatedNs() == 0);
}
