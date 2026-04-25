#pragma once

#include <glm/glm.hpp>

class TouchInputHandler {
public:
    TouchInputHandler();

    void onTouchDown(float x, float y);
    void onTouchMove(float x, float y);
    void onTouchUp(float x, float y);

    glm::vec2 getDragDelta() const { return dragDelta; }
    bool isDragging() const { return isActive; }

    void reset();

private:
    glm::vec2 lastPosition;
    glm::vec2 dragDelta;
    bool isActive = false;
};
