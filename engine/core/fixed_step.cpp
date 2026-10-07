#include "fixed_step.h"

#include <algorithm>
#include <cmath>

namespace cymatica::core {

FixedStepAccumulator::FixedStepAccumulator(FixedStepConfig config) noexcept
    : config_(config) {
    if (config_.simulationHz == 0) {
        config_.simulationHz = 120;
    }
    nominalTickNs_ = 1'000'000'000ULL / config_.simulationHz;
}

std::uint64_t FixedStepAccumulator::tickToSampleFrame(std::uint64_t tickIndex) const noexcept {
    // Spec §7.5: floor(k * internalSampleRate / simulationHz) with overflow-safe math
    // For 48000 Hz / 120 Hz, 48000 / 120 is exactly 400.
    if (config_.simulationHz == 0) return 0;
    if (config_.internalSampleRate % config_.simulationHz == 0) {
        return tickIndex * (config_.internalSampleRate / config_.simulationHz);
    }
    if (config_.internalSampleRate > 0 && tickIndex <= UINT64_MAX / config_.internalSampleRate) {
        return (tickIndex * config_.internalSampleRate) / config_.simulationHz;
    }
    const std::uint64_t q = tickIndex / config_.simulationHz;
    const std::uint64_t r = tickIndex % config_.simulationHz;
    return q * config_.internalSampleRate + (r * config_.internalSampleRate) / config_.simulationHz;
}

std::uint64_t FixedStepAccumulator::sampleFrameToTick(std::uint64_t sampleFrame) const noexcept {
    // Spec §7.5 / DIF-M1-15: first tick whose boundary is >= sampleFrame (integer ceil for sampleFrame > 0)
    if (sampleFrame == 0 || config_.simulationHz == 0 || config_.internalSampleRate == 0) {
        return 0;
    }
    if (config_.internalSampleRate % config_.simulationHz == 0) {
        const std::uint64_t framesPerTick = config_.internalSampleRate / config_.simulationHz;
        return (sampleFrame + framesPerTick - 1) / framesPerTick;
    }
    if (config_.simulationHz > 0 && sampleFrame <= (UINT64_MAX - config_.internalSampleRate) / config_.simulationHz) {
        return (sampleFrame * config_.simulationHz + config_.internalSampleRate - 1) / config_.internalSampleRate;
    }
    const std::uint64_t q = sampleFrame / config_.internalSampleRate;
    const std::uint64_t r = sampleFrame % config_.internalSampleRate;
    return q * config_.simulationHz + (r * config_.simulationHz + config_.internalSampleRate - 1) / config_.internalSampleRate;
}

std::uint64_t FixedStepAccumulator::tickDurationNs(std::uint64_t tickIndex) const noexcept {
    // Exact rational tick duration derived from boundary formula:
    // duration(k) = floor((k + 1) * 1e9 / simulationHz) - floor(k * 1e9 / simulationHz)
    // Telescoping sum over any K ticks is exact: sum_{k=0}^{K-1} duration(k) = floor(K * 1e9 / simulationHz).
    if (config_.simulationHz == 0) return 0;
    const std::uint64_t q = 1'000'000'000ULL / config_.simulationHz;
    const std::uint64_t r = 1'000'000'000ULL % config_.simulationHz;
    if (r == 0) return q;
    const std::uint64_t rem = (tickIndex % config_.simulationHz) * r % config_.simulationHz;
    return q + ((rem + r >= config_.simulationHz) ? 1ULL : 0ULL);
}

StepResult FixedStepAccumulator::advanceNs(std::uint64_t deltaNs) noexcept {
    accumulatedNs_ += deltaNs;

    const std::uint64_t tickNs = nominalTickNs_;
    if (tickNs == 0) {
        return StepResult{};
    }

    const auto ticksDue = static_cast<std::uint32_t>(accumulatedNs_ / tickNs);

    // Overload detection without silently discarding authoritative ticks (§20.3)
    const bool overload = (ticksDue >= config_.overloadThresholdTicks);
    const std::uint32_t ticksToRun = std::min(ticksDue, config_.maxCatchUpTicksPerUpdate);

    currentTick_ += ticksToRun;
    accumulatedNs_ -= static_cast<std::uint64_t>(ticksToRun) * tickNs;

    // Accumulate remainder of rational tick duration (§7.5, ADR-0002, DIF-M1-20)
    // 1 tick = 1e9 / simulationHz ns. Nominal is floor(1e9 / simulationHz), remainder is 1e9 % simulationHz.
    // For 120 Hz: nominal is 8'333'333 ns, remainder is 40 ns per second (1/3 ns per tick).
    if (config_.simulationHz > 0) {
        const std::uint64_t remainderPerTick = 1'000'000'000ULL % config_.simulationHz;
        remainderNsAccum_ += static_cast<std::uint64_t>(ticksToRun) * remainderPerTick;
        const std::uint64_t excessNs = accumulatedNs_ % tickNs;
        while (remainderNsAccum_ >= config_.simulationHz && excessNs > 0 && accumulatedNs_ > 0) {
            remainderNsAccum_ -= config_.simulationHz;
            --accumulatedNs_;
        }
    }

    // interpolationAlpha must strictly be in [0.0f, 1.0f) even under backlog (§20.3, DIF-M1-20)
    const std::uint64_t fractionalNs = accumulatedNs_ % tickNs;
    float alpha = static_cast<float>(fractionalNs) / static_cast<float>(tickNs);
    if (alpha >= 1.0f) {
        alpha = std::nextafter(1.0f, 0.0f);
    } else if (alpha < 0.0f) {
        alpha = 0.0f;
    }

    return StepResult{
        .ticksToRun = ticksToRun,
        .interpolationAlpha = alpha,
        .overloadSuspensionRequired = overload,
    };
}

StepResult FixedStepAccumulator::advanceSeconds(double deltaSeconds) noexcept {
    if (deltaSeconds < 0.0) {
        deltaSeconds = 0.0;
    }
    const auto deltaNs = static_cast<std::uint64_t>(deltaSeconds * 1e9);
    return advanceNs(deltaNs);
}

void FixedStepAccumulator::reset() noexcept {
    currentTick_ = 0;
    accumulatedNs_ = 0;
    remainderNsAccum_ = 0;
}

void FixedStepAccumulator::reconcileAfterSuspension() noexcept {
    // Reconcile accumulator debt after technical suspension (§20.3)
    accumulatedNs_ = 0;
    remainderNsAccum_ = 0;
}

void FixedStepAccumulator::reconcileToTick(std::uint64_t tick) noexcept {
    currentTick_ = tick;
    accumulatedNs_ = 0;
    remainderNsAccum_ = 0;
}

} // namespace cymatica::core
