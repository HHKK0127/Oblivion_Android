// Menu XML -> UI node tree. See menu_ui_builder.h and
// docs/MENU_XML_INTERPRETER_DESIGN.md section 5.5.

#include "menu_ui_builder.h"

#include <cstdlib>

namespace oblivion::ui {

namespace {

// XML attribute values carry padding, e.g. "<font> 1 </font>" and
// " Menus\Loading\load_in_game_default.dds ".
std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    const std::size_t b = s.find_first_not_of(ws);
    if (b == std::string::npos) return std::string();
    const std::size_t e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

MenuNodeKind kindOf(const std::string& type) {
    if (type == "rect") return MenuNodeKind::Rectangle;
    if (type == "image") return MenuNodeKind::Image;
    if (type == "text") return MenuNodeKind::Text;
    return MenuNodeKind::Ignored;
}

float traitNumber(const std::map<std::string, EvalValue>& traits, const char* name,
                  float fallback) {
    const auto it = traits.find(name);
    return it == traits.end() ? fallback : it->second.toNumber();
}

bool traitExists(const std::map<std::string, EvalValue>& traits, const char* name) {
    return traits.find(name) != traits.end();
}

// The menus author colour channels and alpha as bytes (0..255).
float byteToUnit(float value) {
    const float unit = value / 255.0f;
    return unit < 0.0f ? 0.0f : (unit > 1.0f ? 1.0f : unit);
}

}  // namespace

std::string MenuUiBuilder::convertTexturePath(const std::string& filename) {
    const std::string s = trim(filename);
    if (s.empty()) return std::string();

    // The menus store Windows paths; drop everything up to the last separator.
    const std::size_t sep = s.find_last_of("\\/");
    std::string base = (sep == std::string::npos) ? s : s.substr(sep + 1);
    if (base.empty()) return std::string();

    // Normalise the extension: the shipped art is DDS, the port uses PNG.
    const std::size_t dot = base.find_last_of('.');
    if (dot != std::string::npos && dot > 0) base = base.substr(0, dot);
    if (base.empty()) return std::string();

    return "textures/ui/" + base + ".png";
}

int MenuUiBuilder::fontIndex(const std::string& font) {
    const std::string s = trim(font);
    if (s.empty()) return 0;
    const char* begin = s.c_str();
    char* end = nullptr;
    const long value = std::strtol(begin, &end, 10);
    if (end == begin) return 0;  // no leading digits
    if (value < 0 || value > kMaxFontIndex) return 0;
    return static_cast<int>(value);
}

MenuUiNode MenuUiBuilder::buildNode(const ResolvedWidget& widget) {
    MenuUiNode node;
    node.name = widget.name;
    node.kind = kindOf(widget.type);
    node.x = widget.x;
    node.y = widget.y;
    node.width = widget.width;
    node.height = widget.height;
    node.depth = widget.depth;
    // A tile that declares no <alpha> means fully opaque, not "alpha 1/255".
    node.alpha = traitExists(widget.traits, "alpha") ? byteToUnit(widget.alpha) : 1.0f;
    node.red = byteToUnit(widget.red);
    node.green = byteToUnit(widget.green);
    node.blue = byteToUnit(widget.blue);
    node.visible = widget.visible;
    node.font = widget.font;
    node.font_index = fontIndex(widget.font);
    node.justify = widget.justify;

    switch (node.kind) {
        case MenuNodeKind::Image:
            node.texture_path = convertTexturePath(widget.filename);
            node.fill_rect = widget.fill_rect;
            node.zoom_scale = widget.zoom_scale;
            node.texture_scale = widget.texture_scale;
            node.texture_width = widget.texture_width;
            node.texture_height = widget.texture_height;
            // zoom < 0 means "fit the authored tile rect": the rect keeps its
            // aspect, so fit inside it instead of stretching the texture.
            node.scale_mode = widget.fill_rect ? MenuScaleMode::Fit : MenuScaleMode::Stretch;
            break;
        case MenuNodeKind::Text:
            node.text = widget.string;
            node.wrap_width = traitNumber(widget.traits, "wrapwidth", 0.0f);
            break;
        default:
            break;
    }

    node.children.reserve(widget.children.size());
    for (const auto& child : widget.children) {
        node.children.push_back(buildNode(child));
    }
    return node;
}

std::vector<MenuUiNode> MenuUiBuilder::build(const ResolvedMenu& menu) {
    std::vector<MenuUiNode> nodes;
    nodes.reserve(menu.widgets.size());
    for (const auto& widget : menu.widgets) {
        nodes.push_back(buildNode(widget));
    }
    return nodes;
}

}  // namespace oblivion::ui
