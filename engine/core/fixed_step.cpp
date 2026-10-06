#include "fixed_step.h"

#include <algorithm>

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
    if (config_.internalSampleRate % config_.simulationHz == 0) {
        return tickIndex * (config_.internalSampleRate / config_.simulationHz);
    }
    return (tickIndex * config_.internalSampleRate) / config_.simulationHz;
}

std::uint64_t FixedStepAccumulator::sampleFrameToTick(std::uint64_t sampleFrame) const noexcept {
    if (config_.internalSampleRate % config_.simulationHz == 0) {
        return sampleFrame / (config_.internalSampleRate / config_.simulationHz);
    }
    return (sampleFrame * config_.simulationHz) / config_.internalSampleRate;
}

StepResult FixedStepAccumulator::advanceNs(std::uint64_t deltaNs) noexcept {
    accumulatedNs_ += deltaNs;

    const std::uint64_t tickNs = nominalTickNs_;
    const auto ticksDue = static_cast<std::uint32_t>(accumulatedNs_ / tickNs);

    // Overload detection without silently discarding authoritative ticks (§20.3)
    const bool overload = (ticksDue >= config_.overloadThresholdTicks);

    const std::uint32_t ticksToRun = std::min(ticksDue, config_.maxCatchUpTicksPerUpdate);
    currentTick_ += ticksToRun;
    accumulatedNs_ -= static_cast<std::uint64_t>(ticksToRun) * tickNs;

    float alpha = static_cast<float>(accumulatedNs_) / static_cast<float>(tickNs);
    if (alpha > 1.0f) {
        alpha = 1.0f;
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
}

void FixedStepAccumulator::reconcileAfterSuspension() noexcept {
    // Reconcile accumulator debt after technical suspension (§20.3)
    accumulatedNs_ = 0;
}

} // namespace cymatica::core
