#include "ui_slider.h"

#include "ui_draw_helper.h"

#include <algorithm>

UISlider::UISlider(const std::string& name) : UIComponent(name) {}

void UISlider::setValue(float v) {
    value_ = std::max(min_, std::min(max_, v));
}

void UISlider::stepValue(int direction) {
    if (direction == 0) return;
    setValue(value_ + (direction > 0 ? step_ : -step_));
}

float UISlider::normalized() const {
    if (max_ <= min_) return 0.0f;
    return (value_ - min_) / (max_ - min_);
}

void UISlider::render() {
    if (!isVisible()) return;

    const glm::vec4 trackColor(0.25f, 0.25f, 0.28f, 0.9f);
    const glm::vec4 fillColor(0.70f, 0.60f, 0.30f, 1.0f);
    const glm::vec4 handleColor(0.95f, 0.90f, 0.70f, 1.0f);

    // Draw in absolute (screen) coordinates so the slider lands correctly under
    // a parent whose origin is not (0,0) - UIButton and MenuTextComponent already
    // do this. The chargen panel is the only user of this widget.
    const glm::vec2 abs = getAbsolutePosition();

    const float trackH = size.y * 0.25f;
    const float trackY = abs.y + (size.y - trackH) * 0.5f;
    const float handleW = size.x * 0.06f;

    UIDrawHelper::drawColoredQuad(abs.x, trackY, size.x, trackH, trackColor,
                                  screenWidth, screenHeight);
    UIDrawHelper::drawColoredQuad(abs.x, trackY, size.x * normalized(), trackH,
                                  fillColor, screenWidth, screenHeight);

    const float handleX = abs.x + (size.x - handleW) * normalized();
    UIDrawHelper::drawColoredQuad(handleX, abs.y, handleW, size.y, handleColor,
                                  screenWidth, screenHeight);

    renderChildren();
}
