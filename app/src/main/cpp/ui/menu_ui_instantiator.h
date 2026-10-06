#pragma once

// Menu XML -> live UIComponent tree. The GL half of M4; see menu_ui_builder.h
// for the GL free conversion and docs/MENU_XML_INTERPRETER_DESIGN.md section 5.5.
//
// MenuUiBuilder stays host testable by producing plain descriptors. This class
// turns those descriptors into UIComponent / texture objects, which needs a GL
// context, so it is only exercised on the device.

#include "menu_ui_builder.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class UIComponent;
class UISystem;
class TextRenderer;

namespace oblivion::ui {

class MenuUiInstantiator {
public:
    // Resolve an asset path to a GL texture id (0 when the texture is missing).
    // The caller supplies this so textures can be cached and reused across menus.
    using TextureResolver = std::function<unsigned int(const std::string& asset_path)>;

    MenuUiInstantiator(UISystem* ui_system, TextRenderer* text_renderer);

    void setTextureResolver(TextureResolver resolver) { texture_resolver_ = std::move(resolver); }

    // Create a component for `node` and, recursively, for its children. Returns
    // the node's own component, the single child when the node itself is not
    // drawable (<nif> and friends), or nullptr when nothing is drawable.
    std::shared_ptr<UIComponent> instantiate(const MenuUiNode& node);

    // Convert every widget of a resolved menu, register the roots with the
    // UISystem on `layer` and return them in document order.
    std::vector<std::shared_ptr<UIComponent>> instantiateMenu(const ResolvedMenu& menu,
                                                              int layer = 0);

    // Font metrics hook for MenuEvalContext::text_size, so the interpreter can
    // measure a <string> at a <font> and centre text tiles.
    static std::function<bool(const std::string&, const std::string&, float&, float&)>
    makeTextSizeHook(TextRenderer* renderer, float scale = 1.0f);

    // Texture metrics hook for MenuEvalContext::texture_size, so filewidth /
    // fileheight and image auto sizing resolve.
    static std::function<bool(const std::string&, float&, float&)> makeTextureSizeHook();

private:
    // Build the component for one node without its children.
    std::shared_ptr<UIComponent> makeComponent(const MenuUiNode& node);

    UISystem* ui_system_;
    TextRenderer* text_renderer_;
    TextureResolver texture_resolver_;
};

}  // namespace oblivion::ui
