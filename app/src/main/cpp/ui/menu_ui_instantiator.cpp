// Menu XML -> live UIComponent tree. See menu_ui_instantiator.h.

#include "menu_ui_instantiator.h"

#include "../engine/texture_loader.h"
#include "menu_text_component.h"
#include "text_renderer.h"
#include "ui_component.h"
#include "ui_system.h"

namespace oblivion::ui {

MenuUiInstantiator::MenuUiInstantiator(UISystem* ui_system, TextRenderer* text_renderer)
    : ui_system_(ui_system), text_renderer_(text_renderer) {}

std::shared_ptr<UIComponent> MenuUiInstantiator::makeComponent(const MenuUiNode& node) {
    switch (node.kind) {
        case MenuNodeKind::Rectangle: {
            auto component = std::make_shared<UIComponent>(node.name);
            component->setPosition(node.x, node.y);
            component->setSize(node.width, node.height);
            component->setBackgroundColor(glm::vec4(node.red, node.green, node.blue, node.alpha));
            return component;
        }
        case MenuNodeKind::Image: {
            auto component = std::make_shared<UIComponent>(node.name);
            component->setPosition(node.x, node.y);
            component->setSize(node.width, node.height);
            if (texture_resolver_ && !node.texture_path.empty()) {
                const GLuint texture = texture_resolver_(node.texture_path);
                if (texture != 0) {
                    component->setTexture(texture);
                    component->setTextureScaleMode(node.scale_mode == MenuScaleMode::Fit
                                                       ? TextureScaleMode::PRESERVE_ASPECT_FIT
                                                       : TextureScaleMode::STRETCH);
                }
            }
            // UIComponent::renderTexture() draws untinted, so the authored
            // colour and alpha are not applied to images yet. Rectangles and
            // text do honour them.
            return component;
        }
        case MenuNodeKind::Text: {
            auto component = std::make_shared<MenuTextComponent>(node.name, text_renderer_);
            component->setPosition(node.x, node.y);
            component->setSize(node.width, node.height);
            component->setText(node.text);
            component->setFont(static_cast<FontType>(node.font_index));
            component->setTextColor(glm::vec4(node.red, node.green, node.blue, node.alpha));
            component->setJustify(node.justify);
            component->setWrapWidth(node.wrap_width);
            component->setTextScale(node.zoom_scale);
            return component;
        }
        default:
            return nullptr;
    }
}

std::shared_ptr<UIComponent> MenuUiInstantiator::instantiate(const MenuUiNode& node) {
    std::shared_ptr<UIComponent> component = makeComponent(node);

    std::vector<std::shared_ptr<UIComponent>> child_components;
    child_components.reserve(node.children.size());
    for (const auto& child : node.children) {
        auto child_component = instantiate(child);
        if (child_component) child_components.push_back(child_component);
    }

    if (!component) {
        // Not drawable itself (<nif>). Hand the children up so they still reach
        // the tree; wrap them in a transparent carrier only when their relative
        // placement needs the node's own origin.
        if (child_components.empty()) return nullptr;
        if (child_components.size() == 1) return child_components.front();

        auto carrier = std::make_shared<UIComponent>(node.name);
        carrier->setPosition(node.x, node.y);
        carrier->setBackgroundColor(glm::vec4(0.0f));
        for (const auto& child : child_components) carrier->addChild(child);
        return carrier;
    }

    component->setVisible(node.visible);
    for (const auto& child : child_components) component->addChild(child);
    return component;
}

std::vector<std::shared_ptr<UIComponent>> MenuUiInstantiator::instantiateMenu(
    const ResolvedMenu& menu, int layer) {
    const std::vector<MenuUiNode> nodes = MenuUiBuilder::build(menu);
    std::vector<std::shared_ptr<UIComponent>> roots;
    roots.reserve(nodes.size());
    for (const auto& node : nodes) {
        auto component = instantiate(node);
        if (!component) continue;
        roots.push_back(component);
        if (ui_system_) ui_system_->registerComponent(component, layer);
    }
    return roots;
}

std::function<bool(const std::string&, const std::string&, float&, float&)>
MenuUiInstantiator::makeTextSizeHook(TextRenderer* renderer, float scale) {
    return [renderer, scale](const std::string& text, const std::string& font, float& width,
                             float& height) {
        if (renderer == nullptr) return false;
        renderer->setActiveFont(static_cast<FontType>(MenuUiBuilder::fontIndex(font)));
        width = renderer->getTextWidth(text, scale);
        height = renderer->getTextHeight(scale);
        return width > 0.0f || height > 0.0f;
    };
}

std::function<bool(const std::string&, float&, float&)>
MenuUiInstantiator::makeTextureSizeHook() {
    return [](const std::string& filename, float& width, float& height) {
        const std::string path = MenuUiBuilder::convertTexturePath(filename);
        if (path.empty()) return false;

        // The only way to learn a texture's native size is to load it; the
        // pixels are not needed here, so the texture is released again.
        const GLuint texture = TextureLoader::loadTextureFromAsset(path);
        if (texture == 0) return false;

        int texture_width = 0;
        int texture_height = 0;
        const bool sized =
            TextureLoader::getTextureSize(texture, texture_width, texture_height) &&
            texture_width > 0 && texture_height > 0;
        TextureLoader::deleteTexture(texture);
        if (!sized) return false;

        width = static_cast<float>(texture_width);
        height = static_cast<float>(texture_height);
        return true;
    };
}

}  // namespace oblivion::ui
