#pragma once

#include <cstdint>

namespace cymatica::core {

struct FixedStepConfig {
    std::uint64_t internalSampleRate{48000};
    std::uint32_t simulationHz{120};
    std::uint32_t maxCatchUpTicksPerUpdate{4};  // Spec §20.3 / ADR-0002: max ticks per advance
    std::uint32_t overloadThresholdTicks{16};    // Threshold before entering technical suspension
};

struct StepResult {
    std::uint32_t ticksToRun{0};
    float interpolationAlpha{0.0f};           // [0.0f, 1.0f) for render interpolation
    bool overloadSuspensionRequired{false};   // Spec §20.3: system must enter technical suspension
};

class FixedStepAccumulator {
public:
    explicit FixedStepAccumulator(FixedStepConfig config = {}) noexcept;

    // Boundary formula defined in Spec §7.5: floor(k * sampleRate / simulationHz)
    [[nodiscard]] std::uint64_t tickToSampleFrame(std::uint64_t tickIndex) const noexcept;
    [[nodiscard]] std::uint64_t sampleFrameToTick(std::uint64_t sampleFrame) const noexcept;

    // Exact rational tick duration in nanoseconds (§7.5, ADR-0002)
    [[nodiscard]] std::uint64_t tickDurationNs(std::uint64_t tickIndex) const noexcept;

    // Advances by integer nanoseconds (exact, deterministic across 60 vs 120 FPS feeds)
    StepResult advanceNs(std::uint64_t deltaNs) noexcept;

    // Convenience helper converting seconds (double) to nanoseconds
    StepResult advanceSeconds(double deltaSeconds) noexcept;

    void reset() noexcept;

    [[nodiscard]] std::uint64_t currentTick() const noexcept { return currentTick_; }
    [[nodiscard]] std::uint64_t accumulatedNs() const noexcept { return accumulatedNs_; }
    [[nodiscard]] const FixedStepConfig& config() const noexcept { return config_; }

    // Reconcile/recover after technical suspension (§20.3)
    void reconcileAfterSuspension() noexcept;
    void reconcileToTick(std::uint64_t tick) noexcept;

private:
    FixedStepConfig config_;
    std::uint64_t currentTick_{0};
    std::uint64_t accumulatedNs_{0};
    std::uint64_t nominalTickNs_{8'333'333};
    std::uint64_t remainderNsAccum_{0};
};

} // namespace cymatica::core
