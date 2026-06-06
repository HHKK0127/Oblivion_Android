#include "InputSystem.h"
#include <chrono>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "InputSystem"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace oblivion {

InputSystem::InputSystem() {}

void InputSystem::onTouchEvent(int pointerId, float x, float y, int action) {
    TouchEvent event{
        pointerId, x, y,
        convertAction(action),
        std::chrono::duration<double>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count()
    };
    eventQueue_.push(event);
}

void InputSystem::processEvents() {
    while (auto eventOpt = eventQueue_.pop()) {
        auto& event = *eventOpt;

        std::lock_guard<std::mutex> lock(touchMutex_);

        if (event.action == TouchAction::DOWN) {
            // 既存を削除して追加
            activeTouches_.erase(
                std::remove_if(activeTouches_.begin(), activeTouches_.end(),
                    [&](const TouchEvent& e) { return e.pointerId == event.pointerId; }),
                activeTouches_.end());
            activeTouches_.push_back(event);
            LOGI("Touch DOWN: pointer=%d pos=(%.1f, %.1f)", event.pointerId, event.x, event.y);
        }
        else if (event.action == TouchAction::MOVE) {
            for (auto& touch : activeTouches_) {
                if (touch.pointerId == event.pointerId) {
                    touch.x = event.x;
                    touch.y = event.y;
                    break;
                }
            }
        }
        else { // UP or CANCEL
            activeTouches_.erase(
                std::remove_if(activeTouches_.begin(), activeTouches_.end(),
                    [&](const TouchEvent& e) { return e.pointerId == event.pointerId; }),
                activeTouches_.end());
            LOGI("Touch UP/CANCEL: pointer=%d", event.pointerId);
        }
    }
}

bool InputSystem::isTouching(int pointerId) const {
    std::lock_guard<std::mutex> lock(touchMutex_);
    return std::any_of(activeTouches_.begin(), activeTouches_.end(),
        [&](const TouchEvent& e) { return e.pointerId == pointerId; });
}

void InputSystem::getTouchPosition(int pointerId, float& x, float& y) const {
    std::lock_guard<std::mutex> lock(touchMutex_);
    for (const auto& touch : activeTouches_) {
        if (touch.pointerId == pointerId) {
            x = touch.x;
            y = touch.y;
            return;
        }
    }
    x = y = 0.0f;
}

void InputSystem::clear() {
    eventQueue_.clear();
    std::lock_guard<std::mutex> lock(touchMutex_);
    activeTouches_.clear();
}

TouchAction InputSystem::convertAction(int action) {
    // Android: ACTION_DOWN=0, UP=1, MOVE=2, CANCEL=3, POINTER_DOWN=5, POINTER_UP=6
    switch (action & 0xFF) {
        case 0: case 5: return TouchAction::DOWN;
        case 1: case 6: return TouchAction::UP;
        case 2: return TouchAction::MOVE;
        case 3: return TouchAction::CANCEL;
        default: return TouchAction::CANCEL;
    }
}

} // namespace oblivion
