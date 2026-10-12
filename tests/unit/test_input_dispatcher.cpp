#include <catch2/catch_test_macros.hpp>
#include "input_action.h"
#include "input_dispatcher.h"

using namespace cymatica::core;

TEST_CASE("InputDispatcher: physical input state transitions", "[input][dispatcher]") {
    InputDispatcher dispatcher;
    REQUIRE(dispatcher.activeSource() == ActionSource::PhysicalInput);
    REQUIRE(dispatcher.state(InputAction::ToneShift) == ActionState::Released);
    REQUIRE_FALSE(dispatcher.isDown(InputAction::ToneShift));

    NormalizedAction capturedAction{};
    int dispatchCount = 0;
    dispatcher.setActionHandler([&](const NormalizedAction& act) {
        capturedAction = act;
        ++dispatchCount;
    });

    // 1. Initial key press: Released -> Pressed
    auto a1 = dispatcher.processPhysicalInput(InputAction::ToneShift, true, 10);
    CHECK(a1.action == InputAction::ToneShift);
    CHECK(a1.state == ActionState::Pressed);
    CHECK(a1.source == ActionSource::PhysicalInput);
    CHECK(a1.targetTick == 10);
    CHECK(dispatcher.isDown(InputAction::ToneShift));
    CHECK(dispatchCount == 1);
    CHECK(capturedAction == a1);

    // 2. Continuous key down: Pressed -> Held
    auto a2 = dispatcher.processPhysicalInput(InputAction::ToneShift, true, 11);
    CHECK(a2.state == ActionState::Held);
    CHECK(dispatcher.isDown(InputAction::ToneShift));
    CHECK(dispatchCount == 2);
    CHECK(capturedAction == a2);

    // 3. Maintained key down: Held -> Held
    auto a3 = dispatcher.processPhysicalInput(InputAction::ToneShift, true, 12);
    CHECK(a3.state == ActionState::Held);
    CHECK(dispatchCount == 3);

    // 4. Key released: Held -> Released
    auto a4 = dispatcher.processPhysicalInput(InputAction::ToneShift, false, 13);
    CHECK(a4.state == ActionState::Released);
    CHECK_FALSE(dispatcher.isDown(InputAction::ToneShift));
    CHECK(dispatchCount == 4);
    CHECK(capturedAction == a4);

    // 5. Subsequent idle frame: Released -> Released (no duplicate event emitted)
    auto a5 = dispatcher.processPhysicalInput(InputAction::ToneShift, false, 14);
    CHECK(a5.state == ActionState::Released);
    CHECK(dispatchCount == 4); // count unchanged
}

TEST_CASE("InputDispatcher: mutual exclusion during diagnostic session", "[input][dispatcher]") {
    InputDispatcher dispatcher;

    // Normal physical input works
    dispatcher.processPhysicalInput(InputAction::ToneShift, true, 100);
    REQUIRE(dispatcher.state(InputAction::ToneShift) == ActionState::Pressed);

    // Start diagnostic session
    dispatcher.startDiagnosticSession();
    REQUIRE(dispatcher.isDiagnosticRunning());
    REQUIRE(dispatcher.activeSource() == ActionSource::AgentRunner);

    // Physical input must be ignored during diagnostic session
    auto act = dispatcher.processPhysicalInput(InputAction::ToneShift, false, 101);
    // State remains unmutated by physical input
    CHECK(act.state == ActionState::Pressed);
    CHECK(dispatcher.state(InputAction::ToneShift) == ActionState::Pressed);

    // Agent runner can inject actions
    NormalizedAction injectAct{
        .action = InputAction::ToneShift,
        .state = ActionState::Held,
        .source = ActionSource::AgentRunner,
        .targetTick = 102
    };
    CHECK(dispatcher.injectAction(injectAct));
    CHECK(dispatcher.state(InputAction::ToneShift) == ActionState::Held);

    // End diagnostic session restores physical control and resets state
    dispatcher.endDiagnosticSession();
    REQUIRE_FALSE(dispatcher.isDiagnosticRunning());
    REQUIRE(dispatcher.activeSource() == ActionSource::PhysicalInput);
    REQUIRE(dispatcher.state(InputAction::ToneShift) == ActionState::Released);
}

TEST_CASE("InputDispatcher: injection rejected when physical source is active", "[input][dispatcher]") {
    InputDispatcher dispatcher(ActionSource::PhysicalInput);

    NormalizedAction injectAct{
        .action = InputAction::ToneShift,
        .state = ActionState::Pressed,
        .source = ActionSource::AgentRunner,
        .targetTick = 50
    };

    // Rejected because activeSource is PhysicalInput
    CHECK_FALSE(dispatcher.injectAction(injectAct));
    CHECK(dispatcher.state(InputAction::ToneShift) == ActionState::Released);
}
