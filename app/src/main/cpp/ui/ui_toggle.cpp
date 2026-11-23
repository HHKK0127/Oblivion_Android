#include "ui_toggle.h"

#include "ui_draw_helper.h"

UIToggle::UIToggle(const std::string& name) : UIComponent(name) {}

void UIToggle::render() {
    if (!isVisible()) return;

    const glm::vec4 boxColor(0.20f, 0.20f, 0.22f, 0.9f);
    const glm::vec4 markColor(0.70f, 0.60f, 0.30f, 1.0f);
    const glm::vec4 borderColor(0.80f, 0.75f, 0.55f, 1.0f);

    const float box = size.y * 0.8f;
    const float boxX = position.x;
    const float boxY = position.y + (size.y - box) * 0.5f;

    UIDrawHelper::drawColoredQuad(boxX, boxY, box, box, boxColor, screenWidth, screenHeight);
    if (checked_) {
        const float inset = box * 0.25f;
        UIDrawHelper::drawColoredQuad(boxX + inset, boxY + inset, box - inset * 2.0f,
                                      box - inset * 2.0f, markColor, screenWidth, screenHeight);
    }
    UIDrawHelper::drawBorder(boxX, boxY, box, box, 2.0f, borderColor, screenWidth, screenHeight);

    renderChildren();
}
