#include <catch2/catch_test_macros.hpp>

#include "fixed_step.h"

#include <limits>

using namespace cymatica::core;

TEST_CASE("FixedStepAccumulator boundary formula matches Spec section 7.5 exactly", "[fixed_step][boundary]") {
    FixedStepAccumulator acc({48000, 120, 4, 16});

    REQUIRE(acc.tickToSampleFrame(0) == 0);
    REQUIRE(acc.tickToSampleFrame(1) == 400); // 48000 / 120 = 400
    REQUIRE(acc.tickToSampleFrame(120) == 48000);
    // 1 virtual hour: 120 * 3600 = 432,000 ticks = 172,800,000 frames
    REQUIRE(acc.tickToSampleFrame(120ULL * 3600ULL) == 48000ULL * 3600ULL);

    REQUIRE(acc.sampleFrameToTick(0) == 0);
    // Spec §7.5 / DIF-M1-15: first tick whose boundary is >= sampleFrame
    REQUIRE(acc.sampleFrameToTick(1) == 1);
    REQUIRE(acc.sampleFrameToTick(200) == 1);
    REQUIRE(acc.sampleFrameToTick(399) == 1);
    REQUIRE(acc.sampleFrameToTick(400) == 1);
    REQUIRE(acc.sampleFrameToTick(401) == 2);
    REQUIRE(acc.sampleFrameToTick(799) == 2);
    REQUIRE(acc.sampleFrameToTick(800) == 2);
    REQUIRE(acc.sampleFrameToTick(801) == 3);
    REQUIRE(acc.sampleFrameToTick(48000) == 120);

    // Rational tick duration in nanoseconds: 3 ticks = exact 25,000,000 ns
    REQUIRE(acc.tickDurationNs(0) == 8'333'333);
    REQUIRE(acc.tickDurationNs(1) == 8'333'333);
    REQUIRE(acc.tickDurationNs(2) == 8'333'334);
    REQUIRE(acc.tickDurationNs(0) + acc.tickDurationNs(1) + acc.tickDurationNs(2) == 25'000'000);
}

TEST_CASE("60 FPS and 120 FPS feeds produce equivalent simulation tick sequences", "[fixed_step][equivalence]") {
    // Acceptance criterion Spec §32: 60/120 FPS producono stato equivalente
    FixedStepAccumulator acc60({48000, 120, 4, 16});
    FixedStepAccumulator acc120({48000, 120, 4, 16});

    // 2.0 seconds of simulation:
    // 60 FPS: 120 updates summing to exactly 2,000,000,000 ns
    // 120 FPS: 240 updates summing to exactly 2,000,000,000 ns
    std::uint64_t total60 = 0;
    for (int i = 0; i < 120; ++i) {
        const std::uint64_t nextNs = (static_cast<std::uint64_t>(i + 1) * 2'000'000'000ULL) / 120ULL;
        const std::uint64_t stepNs = nextNs - total60;
        total60 += stepNs;
        acc60.advanceNs(stepNs);
    }
    REQUIRE(total60 == 2'000'000'000ULL);

    std::uint64_t total120 = 0;
    for (int i = 0; i < 240; ++i) {
        const std::uint64_t nextNs = (static_cast<std::uint64_t>(i + 1) * 2'000'000'000ULL) / 240ULL;
        const std::uint64_t stepNs = nextNs - total120;
        total120 += stepNs;
        acc120.advanceNs(stepNs);
    }
    REQUIRE(total120 == 2'000'000'000ULL);

    // Both must have executed exactly 240 ticks over 2 seconds (120 Hz * 2s)
    REQUIRE(acc60.currentTick() == 240);
    REQUIRE(acc120.currentTick() == 240);

    // Also verify advanceSeconds(double) with exact rational fractions (Spec §20.3)
    FixedStepAccumulator accSec60({48000, 120, 4, 16});
    FixedStepAccumulator accSec120({48000, 120, 4, 16});
    for (int i = 0; i < 120; ++i) {
        accSec60.advanceSeconds(1.0 / 60.0);
    }
    for (int i = 0; i < 240; ++i) {
        accSec120.advanceSeconds(1.0 / 120.0);
    }
    REQUIRE(accSec60.currentTick() == 240);
    REQUIRE(accSec120.currentTick() == 240);

    // DIF-M1-20 reviewer check: 240 updates of nominal 8,333,333 ns sum to 1,999,999,920 ns
    // which is 80 ns short of the 2.0 s boundary (tick 240 boundary = 2,000,000,000 ns).
    // Exactly 239 ticks must have matured, not 240!
    FixedStepAccumulator accShort({48000, 120, 4, 16});
    for (int i = 0; i < 240; ++i) {
        accShort.advanceNs(8'333'333ULL);
    }
    REQUIRE(accShort.currentTick() == 239);
    // Completing the 2.0 second mark executes the 240th tick:
    accShort.advanceNs(80ULL);
    REQUIRE(accShort.currentTick() == 240);
}

TEST_CASE("FixedStepAccumulator limits catch-up per update without silently dropping ticks", "[fixed_step][catch_up]") {
    FixedStepAccumulator acc({48000, 120, 4, 16});

    // Feed a spike of 8 ticks of delta time (boundary of tick 8)
    const std::uint64_t spikeNs = acc.tickBoundaryNs(8);
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
    const std::uint64_t stallNs = acc.tickBoundaryNs(20);
    const auto step = acc.advanceNs(stallNs);

    REQUIRE(step.overloadSuspensionRequired == true);
    // Still runs up to maxCatchUpTicksPerUpdate (4 ticks)
    REQUIRE(step.ticksToRun == 4);

    // System reconciles after technical suspension (§20.3)
    acc.reconcileAfterSuspension();
    REQUIRE(acc.accumulatedNs() == 0);
}

TEST_CASE("FixedStepAccumulator reconcileToTick synchronizes state cleanly", "[fixed_step][reconcile]") {
    FixedStepAccumulator acc({48000, 120, 4, 16});
    acc.advanceNs(100'000'000ULL);
    REQUIRE(acc.currentTick() > 0);

    acc.reconcileToTick(42000);
    REQUIRE(acc.currentTick() == 42000);
    REQUIRE(acc.accumulatedNs() == 0);
}

TEST_CASE("FixedStepAccumulator interpolationAlpha is strictly within [0.0f, 1.0f) during backlog", "[fixed_step][alpha]") {
    FixedStepAccumulator acc({48000, 120, 4, 16});

    // Feed a backlog of 10 ticks (max catch up is 4)
    const std::uint64_t spikeNs = acc.tickBoundaryNs(10);
    const auto step = acc.advanceNs(spikeNs);

    REQUIRE(step.ticksToRun == 4);
    REQUIRE(step.interpolationAlpha >= 0.0f);
    REQUIRE(step.interpolationAlpha < 1.0f);
}

TEST_CASE("FixedStepAccumulator maintains zero drift with rational remainder over 60 virtual hours", "[fixed_step][timing]") {
    FixedStepAccumulator acc({48000, 120, 4, 16});

    // 60 virtual hours: 60 * 3600 = 216,000 seconds
    // At 120 Hz, each second is exactly 120 ticks. Total = 216,000 * 120 = 25,920,000 ticks.
    // Each second delivers exactly 1,000,000,000 ns across 30 updates of 4 ticks:
    // 10 updates of 33,333,334 ns + 20 updates of 33,333,333 ns = exactly 1,000,000,000 ns/s.
    for (int s = 0; s < 216000; ++s) {
        for (int chunk = 0; chunk < 10; ++chunk) {
            acc.advanceNs(33'333'334ULL);
        }
        for (int chunk = 0; chunk < 20; ++chunk) {
            acc.advanceNs(33'333'333ULL);
        }
    }
    // Exactly 216,000 * 120 = 25,920,000 ticks over 60 full virtual hours
    REQUIRE(acc.currentTick() == 25'920'000ULL);
    // After 60 full virtual hours of exact rational seconds, accumulated leftover is 0
    REQUIRE(acc.accumulatedNs() == 0);
}

TEST_CASE("FixedStepAccumulator handles non-finite, negative, and extreme delta inputs defensively", "[fixed_step][robustness]") {
    FixedStepAccumulator acc({48000, 120, 4, 16});

    // Negative delta time does not advance simulation or corrupt accumulator
    const auto stepNeg = acc.advanceSeconds(-1.0);
    REQUIRE(stepNeg.ticksToRun == 0);
    REQUIRE(acc.currentTick() == 0);

    // Non-finite delta time (NaN, +inf, -inf) does not corrupt state
    const auto stepNan = acc.advanceSeconds(std::numeric_limits<double>::quiet_NaN());
    REQUIRE(stepNan.ticksToRun == 0);
    REQUIRE(acc.currentTick() == 0);

    const auto stepInf = acc.advanceSeconds(std::numeric_limits<double>::infinity());
    REQUIRE(stepInf.ticksToRun == 0);
    REQUIRE(acc.currentTick() == 0);

    const auto stepNegInf = acc.advanceSeconds(-std::numeric_limits<double>::infinity());
    REQUIRE(stepNegInf.ticksToRun == 0);
    REQUIRE(acc.currentTick() == 0);

    // Sub-nanosecond positive delta does not underflow or corrupt fractional remainder
    const auto stepTiny = acc.advanceSeconds(1e-15);
    REQUIRE(stepTiny.ticksToRun == 0);
    REQUIRE(acc.currentTick() == 0);

    // Normal step proceeds cleanly after anomalous inputs
    acc.advanceSeconds(1.0 / 120.0);
    REQUIRE(acc.currentTick() == 1);

    // Saturating advanceNs does not wrap uint64 overflow
    FixedStepAccumulator accSat({48000, 120, 4, 16});
    accSat.advanceNs(UINT64_MAX);
    REQUIRE(accSat.totalElapsedNs() == UINT64_MAX);
    accSat.advanceNs(100ULL);
    REQUIRE(accSat.totalElapsedNs() == UINT64_MAX);
}

TEST_CASE("Fixed step conversions are exact or saturating at uint64 limits", "[fixed_step][overflow]") {
    FixedStepAccumulator acc;
    REQUIRE(acc.tickToSampleFrame(UINT64_MAX) == UINT64_MAX);
    REQUIRE(acc.tickToSampleFrame(46'116'860'184'273'879ULL) == 18'446'744'073'709'551'600ULL);
    REQUIRE(acc.sampleFrameToTick(UINT64_MAX) == 46'116'860'184'273'880ULL);
    REQUIRE(acc.sampleFrameToTick(UINT64_MAX - 15) == 46'116'860'184'273'879ULL);
    REQUIRE(acc.targetTickForElapsedNs(UINT64_MAX) == 2'213'609'288'845ULL);
    REQUIRE(acc.targetTickForElapsedNs(UINT64_MAX - 1) == 2'213'609'288'845ULL);
    REQUIRE(acc.tickBoundaryNs(UINT64_MAX) == UINT64_MAX);
    REQUIRE(acc.tickDurationNs(UINT64_MAX) == 8'333'333ULL);
    FixedStepAccumulator fractional({44'100, 120, 4, 16});
    REQUIRE(fractional.sampleFrameToTick(UINT64_MAX) == 50'195'221'969'277'692ULL);
    FixedStepAccumulator huge({UINT64_MAX, 120, 4, 16});
    REQUIRE(huge.tickToSampleFrame(120) == UINT64_MAX);
    REQUIRE(huge.tickToSampleFrame(119) == 18'293'021'206'428'638'684ULL);
    REQUIRE(huge.sampleFrameToTick(UINT64_MAX) == 120);
    FixedStepAccumulator small({1, 120, 4, 16});
    REQUIRE(small.sampleFrameToTick(UINT64_MAX) == UINT64_MAX);
}
