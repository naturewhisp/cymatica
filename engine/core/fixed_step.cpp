#include "fixed_step.h"
#include "unsigned_math.h"

#include <algorithm>
#include <cmath>

namespace cymatica::core {

FixedStepAccumulator::FixedStepAccumulator(FixedStepConfig config) noexcept
    : config_(config) {
    if (config_.simulationHz == 0) {
        config_.simulationHz = 120;
    }
}

std::uint64_t FixedStepAccumulator::tickToSampleFrame(std::uint64_t tickIndex) const noexcept {
    return multiplyDivide(tickIndex, config_.internalSampleRate, config_.simulationHz).value;
}

std::uint64_t FixedStepAccumulator::sampleFrameToTick(std::uint64_t sampleFrame) const noexcept {
    const auto result = multiplyDivide(sampleFrame, config_.simulationHz, config_.internalSampleRate);
    return saturatingAdd(result.value, static_cast<std::uint64_t>(result.remainder != 0));
}

std::uint64_t FixedStepAccumulator::tickBoundaryNs(std::uint64_t tickIndex) const noexcept {
    return multiplyDivide(tickIndex, 1'000'000'000ULL, config_.simulationHz).value;
}

std::uint64_t FixedStepAccumulator::tickDurationNs(std::uint64_t tickIndex) const noexcept {
    // Derive the duration from phase, even when absolute boundaries saturate.
    const auto hz = config_.simulationHz;
    if (hz == 0) return 0;
    const auto phase = multiplyDivide(tickIndex % hz, 1'000'000'000ULL, hz).remainder;
    return 1'000'000'000ULL / hz + static_cast<std::uint64_t>(phase + 1'000'000'000ULL % hz >= hz);
}

std::uint64_t FixedStepAccumulator::targetTickForElapsedNs(std::uint64_t totalElapsedNs) const noexcept {
    // floor((T*S + S-1)/1e9), without forming T+1 at UINT64_MAX.
    if (config_.simulationHz == 0) return 0;
    const auto result = multiplyDivide(totalElapsedNs, config_.simulationHz, 1'000'000'000ULL);
    return saturatingAdd(result.value, (result.remainder + config_.simulationHz - 1ULL) / 1'000'000'000ULL);
}

StepResult FixedStepAccumulator::advanceNs(std::uint64_t deltaNs) noexcept {
    if (config_.simulationHz == 0) {
        return StepResult{};
    }

    if (UINT64_MAX - totalElapsedNs_ < deltaNs) {
        totalElapsedNs_ = UINT64_MAX;
    } else {
        totalElapsedNs_ += deltaNs;
    }

    const std::uint64_t targetTick = targetTickForElapsedNs(totalElapsedNs_);
    const std::uint64_t ticksDue = (targetTick >= currentTick_) ? (targetTick - currentTick_) : 0ULL;

    // Overload detection without silently discarding authoritative ticks (§20.3)
    const bool overload = (ticksDue >= config_.overloadThresholdTicks);
    const auto ticksToRun = static_cast<std::uint32_t>(
        std::min<std::uint64_t>(ticksDue, config_.maxCatchUpTicksPerUpdate)
    );

    currentTick_ += ticksToRun;

    // interpolationAlpha must strictly be in [0.0f, 1.0f) even under backlog (§20.3, DIF-M1-20)
    // Derived as the fraction of nanoseconds elapsed from the current tick boundary relative to tick duration.
    const std::uint64_t currBoundary = tickBoundaryNs(currentTick_);
    const std::uint64_t currDuration = tickDurationNs(currentTick_);
    float alpha = 0.0f;
    if (currDuration > 0 && totalElapsedNs_ >= currBoundary) {
        const std::uint64_t elapsedInTick = totalElapsedNs_ - currBoundary;
        alpha = static_cast<float>(elapsedInTick) / static_cast<float>(currDuration);
    }
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
    if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0) {
        return advanceNs(0);
    }
    const double totalNs = deltaSeconds * 1e9 + fractionalNs_;
    if (totalNs <= 0.0) {
        fractionalNs_ = totalNs;
        return advanceNs(0);
    }
    if (totalNs >= static_cast<double>(UINT64_MAX)) {
        fractionalNs_ = 0.0;
        return advanceNs(UINT64_MAX);
    }
    const double rounded = std::round(totalNs);
    const auto deltaNs = (std::abs(totalNs - rounded) < 1e-5)
        ? static_cast<std::uint64_t>(rounded)
        : static_cast<std::uint64_t>(totalNs);
    fractionalNs_ = totalNs - static_cast<double>(deltaNs);
    return advanceNs(deltaNs);
}

void FixedStepAccumulator::reset() noexcept {
    currentTick_ = 0;
    totalElapsedNs_ = 0;
    fractionalNs_ = 0.0;
}

std::uint64_t FixedStepAccumulator::accumulatedNs() const noexcept {
    const std::uint64_t boundary = tickBoundaryNs(currentTick_);
    return (totalElapsedNs_ >= boundary) ? (totalElapsedNs_ - boundary) : 0ULL;
}

void FixedStepAccumulator::reconcileAfterSuspension() noexcept {
    // Reconcile accumulator debt after technical suspension (§20.3)
    totalElapsedNs_ = tickBoundaryNs(currentTick_);
    fractionalNs_ = 0.0;
}

void FixedStepAccumulator::reconcileToTick(std::uint64_t tick) noexcept {
    currentTick_ = tick;
    totalElapsedNs_ = tickBoundaryNs(tick);
    fractionalNs_ = 0.0;
}

} // namespace cymatica::core
