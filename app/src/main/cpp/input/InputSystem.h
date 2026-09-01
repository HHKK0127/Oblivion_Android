#pragma once

#include "../util/ThreadSafeQueue.h"
#include <vector>
#include <atomic>
#include <mutex>

namespace oblivion {

enum class TouchAction { DOWN, MOVE, UP, CANCEL };

struct TouchEvent {
    int pointerId;
    float x;
    float y;
    TouchAction action;
    double timestamp;
};

class InputSystem {
public:
    InputSystem();
    ~InputSystem() = default;

    // Called from UI thread (Java/Kotlin -> JNI)
    void onTouchEvent(int pointerId, float x, float y, int action);

    // Called from game loop thread
    void processEvents();
    bool isTouching(int pointerId) const;
    void getTouchPosition(int pointerId, float& x, float& y) const;
    void clear();

private:
    ThreadSafeQueue<TouchEvent> eventQueue_;
    std::vector<TouchEvent> activeTouches_;
    mutable std::mutex touchMutex_;

    TouchAction convertAction(int action);
};

} // namespace oblivion
