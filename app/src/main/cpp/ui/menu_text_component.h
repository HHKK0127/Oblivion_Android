#pragma once

// A UIComponent that draws one string through the shared TextRenderer.
//
// UIComponent itself has no text support and UIButton carries button state
// (hover / pressed colours, hover scaling) that a plain menu label must not
// have, so the menu layer gets this small component instead.
//
// <justify> selects the anchor point of the authored x value (see
// menu_ui_builder.h): the shipped menus left justify on
// (parent().width - me().width) / 2 but centre justify on parent().width / 2, so
// render() shifts the drawn string left of the anchor for centre/right.

#include "text_renderer.h"
#include "ui_component.h"

#include <string>

namespace oblivion::ui {

class MenuTextComponent : public UIComponent {
public:
    MenuTextComponent(const std::string& name, TextRenderer* renderer);

    void setText(const std::string& text) { text_ = text; }
    void setFont(FontType font) { font_ = font; }
    void setTextColor(const glm::vec4& color) { color_ = color; }
    void setJustify(const std::string& justify) { justify_ = justify; }
    void setWrapWidth(float width) { wrap_width_ = width; }
    void setTextScale(float scale) { scale_ = scale; }

    const std::string& getText() const { return text_; }
    FontType getFont() const { return font_; }
    float getWrapWidth() const { return wrap_width_; }

    // Rendered width of the string at the current scale; used to place the anchor.
    float measureWidth();

    void render() override;

private:
    TextRenderer* textRenderer_;
    std::string text_;
    FontType font_ = FontType::Roboto;
    glm::vec4 color_ = glm::vec4(1.0f);
    std::string justify_ = "left";
    // <wrapwidth>. Multi-line wrapping is not implemented yet: the shipped menus
    // only need it for long body text, which no M4 target draws.
    float wrap_width_ = 0.0f;
    float scale_ = 1.0f;
};

}  // namespace oblivion::ui
