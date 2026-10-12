#include "input_dispatcher.h"

namespace cymatica::core {

InputDispatcher::InputDispatcher(ActionSource initialSource)
    : activeSource_(initialSource) {}

void InputDispatcher::setActiveSource(ActionSource source) noexcept {
    activeSource_ = source;
}

ActionSource InputDispatcher::activeSource() const noexcept {
    return activeSource_;
}

void InputDispatcher::startDiagnosticSession() noexcept {
    diagnosticSession_ = true;
    activeSource_ = ActionSource::AgentRunner;
}

void InputDispatcher::endDiagnosticSession() noexcept {
    diagnosticSession_ = false;
    activeSource_ = ActionSource::PhysicalInput;
    reset();
}

bool InputDispatcher::isDiagnosticRunning() const noexcept {
    return diagnosticSession_;
}

NormalizedAction InputDispatcher::processPhysicalInput(InputAction action, bool isDown, std::uint64_t currentTick) {
    NormalizedAction result{
        .action = action,
        .state = state(action),
        .source = ActionSource::PhysicalInput,
        .targetTick = currentTick
    };

    // Mutual exclusion: ignore physical input if diagnostic session is active or active source is AgentRunner
    if (diagnosticSession_ || activeSource_ != ActionSource::PhysicalInput) {
        return result;
    }

    if (action == InputAction::ToneShift) {
        if (isDown) {
            if (toneShiftState_ == ActionState::Released) {
                toneShiftState_ = ActionState::Pressed;
            } else {
                toneShiftState_ = ActionState::Held;
            }
            result.state = toneShiftState_;
            dispatch(result);
        } else {
            if (toneShiftState_ != ActionState::Released) {
                toneShiftState_ = ActionState::Released;
                result.state = toneShiftState_;
                dispatch(result);
            } else {
                result.state = ActionState::Released;
            }
        }
    }

    return result;
}

bool InputDispatcher::injectAction(const NormalizedAction& action) {
    if (activeSource_ != ActionSource::AgentRunner) {
        return false;
    }

    if (action.action == InputAction::ToneShift) {
        toneShiftState_ = action.state;
    }

    dispatch(action);
    return true;
}

ActionState InputDispatcher::state(InputAction action) const noexcept {
    if (action == InputAction::ToneShift) {
        return toneShiftState_;
    }
    return ActionState::Released;
}

bool InputDispatcher::isDown(InputAction action) const noexcept {
    const auto s = state(action);
    return s == ActionState::Pressed || s == ActionState::Held;
}

void InputDispatcher::reset() noexcept {
    toneShiftState_ = ActionState::Released;
}

void InputDispatcher::setActionHandler(ActionHandler handler) {
    handler_ = std::move(handler);
}

void InputDispatcher::dispatch(const NormalizedAction& action) {
    if (handler_) {
        handler_(action);
    }
}

} // namespace cymatica::core
