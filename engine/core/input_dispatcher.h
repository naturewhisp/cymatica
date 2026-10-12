#pragma once

#include "input_action.h"

#include <functional>

namespace cymatica::core {

// Unified input dispatcher shared between physical user inputs and CACP agent runner.
// Owned and operated exclusively on the simulation/coordinator thread (ADR-0003 D-AO-01, Spec §31.5).
class InputDispatcher {
public:
    using ActionHandler = std::function<void(const NormalizedAction&)>;

    explicit InputDispatcher(ActionSource initialSource = ActionSource::PhysicalInput);

    void setActiveSource(ActionSource source) noexcept;
    [[nodiscard]] ActionSource activeSource() const noexcept;

    // Diagnostic session mutual exclusion: while running, physical inputs are ignored to prevent races.
    void startDiagnosticSession() noexcept;
    void endDiagnosticSession() noexcept;
    [[nodiscard]] bool isDiagnosticRunning() const noexcept;

    // Normalizes physical key/button state transition (Released -> Pressed -> Held -> Released)
    NormalizedAction processPhysicalInput(InputAction action, bool isDown, std::uint64_t currentTick);

    // Injects an action directly from an agent / test runner (only applied if activeSource == AgentRunner)
    bool injectAction(const NormalizedAction& action);

    // Query current action state
    [[nodiscard]] ActionState state(InputAction action) const noexcept;
    [[nodiscard]] bool isDown(InputAction action) const noexcept;

    // Reset all internal states to Released
    void reset() noexcept;

    // Optional handler invoked synchronously whenever a normalized action is emitted
    void setActionHandler(ActionHandler handler);

private:
    void dispatch(const NormalizedAction& action);

    ActionSource activeSource_{ActionSource::PhysicalInput};
    bool diagnosticSession_{false};
    ActionState toneShiftState_{ActionState::Released};
    ActionHandler handler_{nullptr};
};

} // namespace cymatica::core
