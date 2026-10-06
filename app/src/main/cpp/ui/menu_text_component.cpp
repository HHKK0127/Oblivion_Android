// Text UIComponent for the menu XML layer. See menu_text_component.h.

#include "menu_text_component.h"

namespace oblivion::ui {

MenuTextComponent::MenuTextComponent(const std::string& name, TextRenderer* renderer)
    : UIComponent(name), textRenderer_(renderer) {}

float MenuTextComponent::measureWidth() {
    if (!textRenderer_ || text_.empty()) return 0.0f;
    return textRenderer_->getTextWidth(text_, scale_);
}

void MenuTextComponent::render() {
    if (!isInitialized() || !isVisible()) return;

    // Draws the (transparent) background and any children.
    UIComponent::render();

    if (!textRenderer_ || text_.empty()) return;

    textRenderer_->setActiveFont(font_);

    const glm::vec2 abs = getAbsolutePosition();
    float x = abs.x;
    if (justify_ == "center") {
        x -= measureWidth() * 0.5f;
    } else if (justify_ == "right") {
        x -= measureWidth();
    }
    textRenderer_->renderText(text_, x, abs.y, color_, scale_);
}

}  // namespace oblivion::ui
