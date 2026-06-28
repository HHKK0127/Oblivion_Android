#include "ui_joystick.h"
#include "ui_draw_helper.h"
#include <cmath>
#include <algorithm>

UIJoystick::UIJoystick(float x, float y, float radius)
    : UIComponent(x - radius, y - radius, radius * 2, radius * 2),
      centerX(x), centerY(y), radius(radius), knobRadius(radius * 0.4f),
      activePointerId(-1), inputValue(0.0f, 0.0f) {
    knobPos = glm::vec2(x, y);
}

void UIJoystick::render(int screenW, int screenH) {
    if (!visible) return;

    // Draw base (semi-transparent dark circle/square placeholder)
    UIDrawHelper::drawColoredQuad(
        centerX - radius, centerY - radius, radius * 2, radius * 2,
        glm::vec4(0.2f.x, 0.2f.y, 0.2f.z, 0.2f, 0.2f, 0.5f), screenW, screenH
    );

    // Draw knob (semi-transparent white circle/square placeholder)
    glm::vec4 knobColor = isActive() ? glm::vec4(1.0f.x, 1.0f.y, 1.0f.z, 1.0f, 1.0f, 0.8f) : glm::vec4(0.8f.x, 0.8f.y, 0.8f.z, 0.8f, 0.8f, 0.6f);
    UIDrawHelper::drawColoredQuad(
        knobPos.x - knobRadius, knobPos.y - knobRadius, knobRadius * 2, knobRadius * 2,
        knobColor, screenW, screenH
    );
}

bool UIJoystick::onEvent(const UIEvent& event) {
    if (!visible || !enabled) return false;

    // For TOUCH_MOVE and TOUCH_UP, process if we are tracking this pointer, 
    // EVEN IF the touch is outside the bounding box
    if ((event.type == UIEventType::TOUCH_MOVE || event.type == UIEventType::TOUCH_UP) && 
        activePointerId == event.pointerId) {
        if (event.type == UIEventType::TOUCH_MOVE) {
            return onTouchMove(event.x, event.y, event.dx, event.dy, event.pointerId);
        } else {
            return onTouchUp(event.x, event.y, event.pointerId);
        }
    }

    // For other events or if we are not tracking, check bounds
    if (!contains(event.x, event.y)) {
        return false;
    }

    if (event.type == UIEventType::TOUCH_DOWN) {
        return onTouchDown(event.x, event.y, event.pointerId);
    }
    
    return false;
}

bool UIJoystick::onTouchDown(float x, float y, int pointerId) {
    if (!visible) return false;

    // Only allow one pointer to control the joystick
    if (activePointerId != -1) return false;

    // Check if touch is within the base radius
    float dx = x - centerX;
    float dy = y - centerY;
    float distSq = dx * dx + dy * dy;

    // Use a slightly larger hit area for ease of use
    float hitRadius = radius * 1.5f; 
    
    if (distSq <= hitRadius * hitRadius) {
        activePointerId = pointerId;
        updateKnobPosition(x, y);
        return true;
    }

    return false;
}

bool UIJoystick::onTouchMove(float x, float y, float dx, float dy, int pointerId) {
    if (!visible || activePointerId != pointerId) return false;

    updateKnobPosition(x, y);
    return true;
}

bool UIJoystick::onTouchUp(float x, float y, int pointerId) {
    if (!visible || activePointerId != pointerId) return false;

    // Reset joystick
    activePointerId = -1;
    knobPos = glm::vec2(centerX, centerY);
    inputValue = glm::vec2(0.0f, 0.0f);
    return true;
}

void UIJoystick::updateKnobPosition(float touchX, float touchY) {
    float dx = touchX - centerX;
    float dy = touchY - centerY;
    float dist = std::sqrt(dx * dx + dy * dy);

    if (dist > radius) {
        // Clamp to edge
        float ratio = radius / dist;
        knobPos.x = centerX + dx * ratio;
        knobPos.y = centerY + dy * ratio;
        inputValue.x = dx / dist; // Normalized -1 to 1
        inputValue.y = dy / dist; // Normalized -1 to 1
    } else {
        knobPos.x = touchX;
        knobPos.y = touchY;
        inputValue.x = dx / radius;
        inputValue.y = dy / radius;
    }
}
