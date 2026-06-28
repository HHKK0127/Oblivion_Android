#pragma once

#include "ui_component.h"
#include <glm/glm.hpp>

class UIJoystick : public UIComponent {
public:
    /**
     * @brief ジョイスティックコンポーネントを作成
     * @param x 中心のX座標
     * @param y 中心のY座標
     * @param radius ジョイスティックの最大半径
     */
    UIJoystick(float x, float y, float radius);
    virtual ~UIJoystick() = default;

    virtual void render(int screenW, int screenH) override;
    virtual bool onEvent(const UIEvent& event) override;
    virtual bool onTouchDown(float x, float y, int pointerId) override;
    virtual bool onTouchMove(float x, float y, float dx, float dy, int pointerId) override;
    virtual bool onTouchUp(float x, float y, int pointerId) override;

    /**
     * @brief ジョイスティックの現在の入力値を取得 (-1.0 ~ 1.0)
     */
    glm::vec2 getInputValue() const { return inputValue; }
    
    /**
     * @brief アクティブ（操作中）かどうかを取得
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
