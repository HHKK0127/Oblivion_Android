#include "ui_toggle.h"

#include "ui_draw_helper.h"

UIToggle::UIToggle(const std::string& name) : UIComponent(name) {}

void UIToggle::render() {
    if (!isVisible()) return;

    const glm::vec4 boxColor(0.20f, 0.20f, 0.22f, 0.9f);
    const glm::vec4 markColor(0.70f, 0.60f, 0.30f, 1.0f);
    const glm::vec4 borderColor(0.80f, 0.75f, 0.55f, 1.0f);

    // Absolute (screen) coordinates, matching UIButton/MenuTextComponent, so the
    // widget is placed correctly under a parent with a non-(0,0) origin.
    const glm::vec2 abs = getAbsolutePosition();

    const float box = size.y * 0.8f;
    const float boxX = abs.x;
    const float boxY = abs.y + (size.y - box) * 0.5f;

    UIDrawHelper::drawColoredQuad(boxX, boxY, box, box, boxColor, screenWidth, screenHeight);
    if (checked_) {
        const float inset = box * 0.25f;
        UIDrawHelper::drawColoredQuad(boxX + inset, boxY + inset, box - inset * 2.0f,
                                      box - inset * 2.0f, markColor, screenWidth, screenHeight);
    }
    UIDrawHelper::drawBorder(boxX, boxY, box, box, 2.0f, borderColor, screenWidth, screenHeight);

    renderChildren();
}

void UIToggle::renderIndex(int index, int count) {
    if (!isVisible()) return;

    // Absolute (screen) coordinates, like render()/UIButton, so the arrows land
    // under a parent with a non-(0,0) origin.
    const glm::vec2 abs = getAbsolutePosition();
    const float h = size.y * 0.8f;
    const float y = abs.y + (size.y - h) * 0.5f;
    const float w = size.x;
    const float aw = h * 0.6f;

    // Left / right arrows (race_template_toggle_left/_right, spec §10.9).
    const glm::vec4 arrowColor(0.80f, 0.75f, 0.55f, 1.0f);
    UIDrawHelper::drawColoredQuad(abs.x, y, aw, h, arrowColor, screenWidth, screenHeight);
    UIDrawHelper::drawColoredQuad(abs.x + w - aw, y, aw, h, arrowColor, screenWidth,
                                  screenHeight);

    // Position marker: cell `index` of `count` between the arrows. `count` is the
    // descriptor's index span (index_max - index_min + 1); `index` is the value
    // the panel holds.
    if (count > 0) {
        const float track = w - aw * 2.0f;
        if (track > 0.0f) {
            const float cell = track / static_cast<float>(count);
            const glm::vec4 markColor(0.30f, 0.45f, 0.70f, 0.9f);
            UIDrawHelper::drawColoredQuad(abs.x + aw + cell * static_cast<float>(index), y, cell,
                                          h, markColor, screenWidth, screenHeight);
        }
    }
}
