#pragma once

// Menu XML -> UI node tree (M4 of docs/MENU_XML_INTERPRETER_DESIGN.md).
//
// The interpreter (menu_xml_interpreter.h) resolves a menu into a tree of
// ResolvedWidget carrying concrete numbers. This builder turns that tree into a
// GL independent tree of MenuUiNode: it maps widget kinds onto UI primitives,
// resolves DDS paths onto Android asset paths and picks the texture scale mode.
// ui/menu_ui_instantiator.h then materialises the descriptors into UIComponent
// instances on the device.
//
// Keeping the conversion itself GL free lets the host test suite cover it
// (design doc section 8); only the instantiator needs a GL context.
//
// Coordinate model: ResolvedWidget::x/y are top-left offsets relative to the
// parent tile, which is exactly what UIComponent::setPosition() expects, so the
// builder uses x/y and never abs_x/abs_y (that would double count the parent
// chain). Text tiles are the one exception: their x/y is the anchor the string
// grows from and <justify> says in which direction, so the instantiator applies
// the anchor after measuring the string.

#include "menu_xml_interpreter.h"

#include <string>
#include <vector>

namespace oblivion::ui {

// How a texture is mapped onto its quad.
enum class MenuScaleMode {
    Stretch,  // stretch the texture over the whole quad
    Fit,      // keep the texture aspect, letterbox/pillarbox inside the quad
    Crop,     // keep the texture aspect, crop the overflow
};

// A UI primitive derived from a widget type.
enum class MenuNodeKind {
    Rectangle,  // <rect>: a solid colour quad
    Image,      // <image>: a textured quad
    Text,       // <text>: a string
    Ignored,    // <nif> and anything the UI layer cannot draw natively
};

// One UI primitive together with its children.
struct MenuUiNode {
    std::string name;
    MenuNodeKind kind = MenuNodeKind::Ignored;

    // Position and size, relative to the parent node (the menu origin for roots).
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float depth = 0.0f;
    // Colour normalised to 0..1. The menus author 0..255 (alpha values in the
    // shipped data are only 0, 160, 200 and 255) so the builder divides by 255.
    float alpha = 1.0f;  // stays 1.0 when the tile declares no <alpha>
    float red = 0.0f, green = 0.0f, blue = 0.0f;
    bool visible = true;

    // Image only.
    std::string texture_path;  // Android asset path, empty when unresolved
    MenuScaleMode scale_mode = MenuScaleMode::Stretch;
    bool fill_rect = false;        // <zoom> < 0: fit the authored tile rect
    float zoom_scale = 1.0f;       // <zoom> > 0, as a fraction (zoom / 100)
    float texture_scale = 0.0f;    // uniform fit scale for fill_rect (0 = unknown)
    float texture_width = 0.0f;    // native texture size (0 = unknown)
    float texture_height = 0.0f;

    // Text only.
    std::string text;
    std::string font;               // raw <font> value as authored
    int font_index = 0;             // <font> mapped onto TextRenderer::FontType
    std::string justify;            // "left" / "center" / "right"
    float wrap_width = 0.0f;        // <wrapwidth>, 0 = unwrapped

    std::vector<MenuUiNode> children;
};

class MenuUiBuilder {
public:
    // Highest <font> value the shipped menus use (TahomaBoldSmall).
    static constexpr int kMaxFontIndex = 5;

    // Map a <filename> value onto a runtime asset path. The menus name DDS
    // textures with Windows separators, e.g.
    //   "Menus\Loading\loading_background.dds" -> "textures/ui/loading_background.png"
    // The shipped art is flattened into textures/ui/ by basename, so the
    // directory part and the extension are dropped. Returns "" for an empty or
    // unusable value.
    static std::string convertTexturePath(const std::string& filename);

    // Map a <font> value onto the TextRenderer::FontType index. Unparseable or
    // out of range values fall back to 0 (Roboto).
    static int fontIndex(const std::string& font);

    // Convert one resolved widget (recursively) into a UI node.
    static MenuUiNode buildNode(const ResolvedWidget& widget);

    // Convert every widget of a resolved menu into a root node, in document order.
    static std::vector<MenuUiNode> build(const ResolvedMenu& menu);
};

}  // namespace oblivion::ui
