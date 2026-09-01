#pragma once

#include "ui_component.h"
#include <glm/glm.hpp>

class UIJoystick : public UIComponent {
public:
    /**
     * @brief Create joystick component
     * @param x Center X coordinate
     * @param y Center Y coordinate
     * @param radius Maximum joystick radius
     */
    UIJoystick(float x, float y, float radius);
    virtual ~UIJoystick() = default;

    virtual void render() override;
    virtual bool onEvent(const UIEvent& event) override;
    virtual bool onTouchDown(float x, float y, int pointerId) override;
    virtual bool onTouchMove(float x, float y, float dx, float dy, int pointerId) override;
    virtual bool onTouchUp(float x, float y, int pointerId) override;

    /**
     * @brief Get current joystick input value (-1.0 to 1.0)
     */
    glm::vec2 getInputValue() const { return inputValue; }
    
    /**
     * @brief Get whether active (in use)
     */
    bool isActive() const { return activePointerId != -1; }

private:
    float centerX;
    float centerY;
    float radius;
    float knobRadius;

    glm::vec2 knobPos;
    glm::vec2 inputValue;

    int activePointerId;

    void updateKnobPosition(float touchX, float touchY);
};
