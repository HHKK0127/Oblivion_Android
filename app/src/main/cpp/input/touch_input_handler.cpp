#include "touch_input_handler.h"

TouchInputHandler::TouchInputHandler()
    : lastPosition(0.0f), dragDelta(0.0f), isActive(false) {
}

void TouchInputHandler::onTouchDown(float x, float y) {
    lastPosition = glm::vec2(x, y);
    isActive = true;
    dragDelta = glm::vec2(0.0f);
}

void TouchInputHandler::onTouchMove(float x, float y) {
    if (isActive) {
        glm::vec2 currentPos(x, y);
        dragDelta = currentPos - lastPosition;
        lastPosition = currentPos;
    }
}

void TouchInputHandler::onTouchUp(float x, float y) {
    isActive = false;
    dragDelta = glm::vec2(0.0f);
}

void TouchInputHandler::reset() {
    dragDelta = glm::vec2(0.0f);
    isActive = false;
}
