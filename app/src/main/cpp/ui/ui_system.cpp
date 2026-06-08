#include "ui_system.h"
#include <algorithm>
#include "text_renderer.h"

UISystem::UISystem()
    : textRenderer(nullptr), screenWidth(1920), screenHeight(1080), initialized(false) {
}

UISystem::~UISystem() {
    cleanup();
}

bool UISystem::initialize(TextRenderer* renderer) {
    textRenderer = renderer;
    initialized = true;
    SYS_LOGI("UISystem initialized");
    return true;
}

void UISystem::cleanup() {
    clearComponents();
    textRenderer = nullptr;
    initialized = false;
    SYS_LOGI("UISystem cleaned up");
}

// === Component Management ===

void UISystem::registerComponent(std::shared_ptr<UIComponent> component, int layer) {
    if (!component) return;

    // Initialize if needed
    if (!component->isInitialized()) {
        component->initialize();
    }

    // Remove from existing layer if already registered
    for (auto& [existingLayer, components] : layers) {
        auto it = std::find(components.begin(), components.end(), component);
        if (it != components.end()) {
            components.erase(it);
            break;
        }
    }

    // Add to new layer
    layers[layer].push_back(component);

    // Register in name map
    nameMap[component->getName()] = component;

    SYS_LOGD("Registered UIComponent: %s (id=%u) on layer %d",
             component->getName().c_str(), component->getId(), layer);
}

void UISystem::unregisterComponent(std::shared_ptr<UIComponent> component) {
    if (!component) return;

    // Remove from layer
    for (auto& [layer, components] : layers) {
        auto it = std::find(components.begin(), components.end(), component);
        if (it != components.end()) {
            components.erase(it);
            break;
        }
    }

    // Remove from name map
    nameMap.erase(component->getName());

    SYS_LOGD("Unregistered UIComponent: %s", component->getName().c_str());
}

std::shared_ptr<UIComponent> UISystem::findComponent(const std::string& name) const {
    auto it = nameMap.find(name);
    if (it != nameMap.end()) {
        return it->second.lock();
    }
    return nullptr;
}

void UISystem::clearComponents() {
    layers.clear();
    nameMap.clear();
    focusedComponent.reset();
    SYS_LOGI("All UI components cleared");
}

// === Layer Management ===

void UISystem::setLayer(std::shared_ptr<UIComponent> component, int newLayer) {
    if (!component) return;

    // Find and remove from current layer
    for (auto& [layer, components] : layers) {
        auto it = std::find(components.begin(), components.end(), component);
        if (it != components.end()) {
            components.erase(it);
            break;
        }
    }

    // Add to new layer
    layers[newLayer].push_back(component);
}

// === Update & Render ===

void UISystem::update(float deltaTime) {
    if (!initialized) return;

    for (auto& [layer, components] : layers) {
        for (auto& comp : components) {
            if (comp && comp->isVisible()) {
                comp->update(deltaTime);
            }
        }
    }
}

void UISystem::render() {
    if (!initialized) return;

    // Save OpenGL state
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderLayers();

    // Restore state
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UISystem::renderLayers() {
    // Render in layer order (smallest first = back, largest last = front)
    for (auto& [layer, components] : layers) {
        for (auto& comp : components) {
            if (comp && comp->isVisible()) {
                comp->render();
            }
        }
    }
}

// === Event Dispatch ===

bool UISystem::onTouchDown(float x, float y, int pointerId) {
    UIEvent event(UIEventType::TOUCH_DOWN, x, y, pointerId);
    return dispatchEvent(event);
}

bool UISystem::onTouchUp(float x, float y, int pointerId) {
    UIEvent event(UIEventType::TOUCH_UP, x, y, pointerId);
    return dispatchEvent(event);
}

bool UISystem::onTouchMove(float x, float y, float dx, float dy, int pointerId) {
    UIEvent event(UIEventType::TOUCH_MOVE, x, y, pointerId);
    event.dx = dx;
    event.dy = dy;
    return dispatchEvent(event);
}

bool UISystem::dispatchEvent(const UIEvent& event) {
    if (!initialized) return false;

    // Dispatch to front-most layers first
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        auto& components = it->second;
        // Iterate in reverse for front-to-back within same layer
        for (auto compIt = components.rbegin(); compIt != components.rend(); ++compIt) {
            auto& comp = *compIt;
            if (comp && comp->isVisible() && comp->isEnabled()) {
                if (comp->onEvent(event)) {
                    return true;  // Event consumed
                }
            }
        }
    }
    return false;
}

// === Screen Size ===

void UISystem::setScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    SYS_LOGI("UISystem screen size set to %dx%d", width, height);

    for (auto& [layer, components] : layers) {
        for (auto& comp : components) {
            if (comp) {
                comp->onScreenResize(width, height);
            }
        }
    }
}

// === Focus Management ===

void UISystem::setFocusedComponent(std::shared_ptr<UIComponent> component) {
    focusedComponent = component;
}

// === Utility ===

void UISystem::setAllVisible(bool visible) {
    for (auto& [layer, components] : layers) {
        for (auto& comp : components) {
            if (comp) comp->setVisible(visible);
        }
    }
}

void UISystem::showOnlyLayer(int targetLayer) {
    for (auto& [layer, components] : layers) {
        bool show = (layer == targetLayer);
        for (auto& comp : components) {
            if (comp) comp->setVisible(show);
        }
    }
}

size_t UISystem::getComponentCount() const {
    size_t count = 0;
    for (const auto& [layer, components] : layers) {
        count += components.size();
    }
    return count;
}
