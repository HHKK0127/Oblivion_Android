#include "input_router_tests.h"

#include "../engine/input_router.h"
#include "../engine/state_manager.h"

#include <android/log.h>
#include <cmath>

#define LOG_TAG "InputRouterTests"
#define TEST_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define TEST_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

bool near(float actual, float expected, float epsilon = 0.001f) {
    return std::fabs(actual - expected) <= epsilon;
}

bool enterGameplay(StateManager& stateManager) {
    return stateManager.initialize()
        && stateManager.transitionTo(GamePlayState::MAIN_MENU)
        && stateManager.transitionTo(GamePlayState::CHARACTER_CREATION)
        && stateManager.transitionTo(GamePlayState::GAMEPLAY);
}

} // namespace

void InputRouterTests::record(const std::string& name, bool passed,
                              const std::string& message) {
    results.push_back({name, passed, message});
    if (passed) {
        TEST_LOGI("[PASS] %s", name.c_str());
    } else {
        TEST_LOGE("[FAIL] %s: %s", name.c_str(), message.c_str());
    }
}

int InputRouterTests::getPassCount() const {
    int count = 0;
    for (const auto& result : results) {
        if (result.passed) {
            ++count;
        }
    }
    return count;
}

int InputRouterTests::getFailCount() const {
    int count = 0;
    for (const auto& result : results) {
        if (!result.passed) {
            ++count;
        }
    }
    return count;
}

std::string InputRouterTests::getSummary() const {
    return "InputRouter Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool InputRouterTests::runAllTests() {
    results.clear();

    {
        StateManager stateManager;
        InputRouter router;
        const bool initialized = enterGameplay(stateManager)
            && router.initialize(&stateManager);
        router.setScreenSize(1000.0f, 1000.0f);
        router.setJoystickZone(100.0f, 900.0f, 100.0f);

        router.routeInput(1, 200.0f, 900.0f, 0); // ACTION_DOWN
        router.update(0.0f);
        const glm::vec2 joystick = router.getJoystickInput();
        record("gameplay ACTION_DOWN updates joystick",
               initialized && near(joystick.x, 1.0f) && near(joystick.y, 0.0f));
    }

    for (const GamePlayState menuState : {
             GamePlayState::PAUSED,
             GamePlayState::INVENTORY,
             GamePlayState::SAVE_MENU,
         }) {
        StateManager stateManager;
        InputRouter router;
        bool initialized = enterGameplay(stateManager)
            && router.initialize(&stateManager)
            && stateManager.transitionTo(menuState);
        router.setScreenSize(1000.0f, 1000.0f);
        router.setJoystickZone(100.0f, 900.0f, 100.0f);

        bool uiHandlerCalled = false;
        const InputZone expectedZone = menuState == GamePlayState::INVENTORY
            ? InputZone::INVENTORY
            : InputZone::UI_ELEMENT;
        router.registerHandler(expectedZone,
                              [&uiHandlerCalled](const ClassifiedInput& input) {
                                  uiHandlerCalled = input.action == InputAction::MOVE;
                                  return true;
                              });

        router.routeInput(2, 200.0f, 900.0f, 2); // ACTION_MOVE
        router.update(0.0f);
        const glm::vec2 joystick = router.getJoystickInput();
        const std::string stateName = gamePlayStateToString(menuState);
        record(stateName + " ACTION_MOVE does not update joystick",
               initialized && uiHandlerCalled
                   && near(joystick.x, 0.0f) && near(joystick.y, 0.0f));
    }

    return getFailCount() == 0;
}
