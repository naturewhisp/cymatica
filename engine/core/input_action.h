#pragma once

#include <cstdint>

namespace cymatica::core {

// Authoritative input action definitions (ADR-0003, Spec §31.5)
enum class InputAction : std::uint8_t {
    ToneShift = 0 // Extensible in M3 with MoveX, MoveY, Dash
};

enum class ActionState : std::uint8_t {
    Pressed,
    Held,
    Released
};

enum class ActionSource : std::uint8_t {
    PhysicalInput,
    AgentRunner
};

struct NormalizedAction {
    InputAction action{InputAction::ToneShift};
    ActionState state{ActionState::Released};
    ActionSource source{ActionSource::PhysicalInput};
    std::uint64_t targetTick{0};

    [[nodiscard]] constexpr bool operator==(const NormalizedAction& other) const noexcept {
        return action == other.action &&
               state == other.state &&
               source == other.source &&
               targetTick == other.targetTick;
    }
};

} // namespace cymatica::core
