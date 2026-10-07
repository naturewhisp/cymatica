#include "fixed_step.h"

#include <algorithm>
#include <cmath>

#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#endif

namespace cymatica::core {

FixedStepAccumulator::FixedStepAccumulator(FixedStepConfig config) noexcept
    : config_(config) {
    if (config_.simulationHz == 0) {
        config_.simulationHz = 120;
    }
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

std::uint64_t FixedStepAccumulator::tickBoundaryNs(std::uint64_t tickIndex) const noexcept {
    // Exact rational nanosecond boundary defined in Spec §7.5 / ADR-0002:
    // boundary(k) = floor(k * 1e9 / simulationHz)
    if (config_.simulationHz == 0) return 0;
    const std::uint64_t q = tickIndex / config_.simulationHz;
    const std::uint64_t r = tickIndex % config_.simulationHz;
    return q * 1'000'000'000ULL + (r * 1'000'000'000ULL) / config_.simulationHz;
}

std::uint64_t FixedStepAccumulator::tickDurationNs(std::uint64_t tickIndex) const noexcept {
    // Exact rational tick duration derived from boundary formula:
    // duration(k) = boundary(k + 1) - boundary(k)
    // Telescoping sum over any K ticks is exact: sum_{k=0}^{K-1} duration(k) = boundary(K).
    return tickBoundaryNs(tickIndex + 1) - tickBoundaryNs(tickIndex);
}

std::uint64_t FixedStepAccumulator::targetTickForElapsedNs(std::uint64_t totalElapsedNs) const noexcept {
    // Largest tickIndex k such that tickBoundaryNs(k) <= totalElapsedNs.
    // Analytically: floor(k * 1e9 / S) <= T <=> k <= floor(((T + 1) * S - 1) / 1e9).
    if (config_.simulationHz == 0) return 0;
    if (totalElapsedNs == UINT64_MAX) return UINT64_MAX;

#if defined(__SIZEOF_INT128__)
    using uint128_t = unsigned __int128;
    const uint128_t tPlus1 = static_cast<uint128_t>(totalElapsedNs) + 1;
    const uint128_t num = tPlus1 * config_.simulationHz - 1;
    return static_cast<std::uint64_t>(num / 1'000'000'000ULL);
#elif defined(_MSC_VER) && defined(_M_X64)
    unsigned __int64 high = 0;
    const unsigned __int64 tPlus1 = totalElapsedNs + 1;
    unsigned __int64 low = _umul128(tPlus1, config_.simulationHz, &high);
    if (low == 0) {
        --high;
        low = UINT64_MAX;
    } else {
        --low;
    }
    unsigned __int64 rem = 0;
    return _udiv128(high, low, 1'000'000'000ULL, &rem);
#else
    const std::uint64_t q = (totalElapsedNs + 1) / 1'000'000'000ULL;
    const std::uint64_t r = (totalElapsedNs + 1) % 1'000'000'000ULL;
    if (r * config_.simulationHz == 0) {
        return (q * config_.simulationHz) - 1;
    }
    return q * config_.simulationHz + (r * config_.simulationHz - 1) / 1'000'000'000ULL;
#endif
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
