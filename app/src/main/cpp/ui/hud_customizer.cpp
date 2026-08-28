#include "hud_customizer.h"
#include <algorithm>
#include <cmath>

HUDCustomizer::HUDCustomizer()
    : screenWidth(1080), screenHeight(1920),
      dragging(false), draggedElementId(""),
      dragOffset(0.0f, 0.0f) {
}

void HUDCustomizer::initialize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    HCUST_LOGI("HUDCustomizer initialized (%dx%d)", width, height);
}

void HUDCustomizer::registerElement(const CustomHUDElement& element) {
    elements[element.elementId] = element;
    HCUST_LOGI("Registered HUD element: %s at (%.0f, %.0f)",
               element.elementId.c_str(), element.position.x, element.position.y);
}

const CustomHUDElement* HUDCustomizer::getElement(const std::string& elementId) const {
    auto it = elements.find(elementId);
    return (it != elements.end()) ? &it->second : nullptr;
}

CustomHUDElement* HUDCustomizer::getElementMutable(const std::string& elementId) {
    auto it = elements.find(elementId);
    return (it != elements.end()) ? &it->second : nullptr;
}

std::string HUDCustomizer::beginDrag(float touchX, float touchY) {
    // Hit test elements in reverse order (top-most first)
    for (auto it = elements.rbegin(); it != elements.rend(); ++it) {
        const auto& elem = it->second;
        if (!elem.visible || elem.locked) continue;

        float left = elem.position.x;
        float top = elem.position.y;
        float right = elem.position.x + elem.currentSize.x;
        float bottom = elem.position.y + elem.currentSize.y;

        if (touchX >= left && touchX <= right && touchY >= top && touchY <= bottom) {
            dragging = true;
            draggedElementId = elem.elementId;
            dragOffset = glm::vec2(touchX - elem.position.x, touchY - elem.position.y);
            HCUST_LOGD("Begin drag: %s", elem.elementId.c_str());
            return elem.elementId;
        }
    }
    return "";
}

void HUDCustomizer::updateDrag(float touchX, float touchY) {
    if (!dragging || draggedElementId.empty()) return;

    auto it = elements.find(draggedElementId);
    if (it == elements.end()) return;

    glm::vec2 newPos(touchX - dragOffset.x, touchY - dragOffset.y);
    it->second.position = clampPosition(newPos, it->second.currentSize);
}

void HUDCustomizer::endDrag() {
    if (dragging) {
        HCUST_LOGD("End drag: %s at (%.0f, %.0f)",
                   draggedElementId.c_str(),
                   elements[draggedElementId].position.x,
                   elements[draggedElementId].position.y);
        notifyChange(draggedElementId);
    }
    dragging = false;
    draggedElementId.clear();
    dragOffset = glm::vec2(0.0f, 0.0f);
}

void HUDCustomizer::setElementPosition(const std::string& elementId, const glm::vec2& position) {
    auto it = elements.find(elementId);
    if (it != elements.end()) {
        it->second.position = clampPosition(position, it->second.currentSize);
        notifyChange(elementId);
    }
}

void HUDCustomizer::setElementScale(const std::string& elementId, float scale) {
    auto it = elements.find(elementId);
    if (it != elements.end()) {
        float clampedScale = std::max(0.5f, std::min(2.0f, scale));
        it->second.scale = clampedScale;
        it->second.currentSize = glm::vec2(
            it->second.baseSize.x * clampedScale,
            it->second.baseSize.y * clampedScale);
        notifyChange(elementId);
    }
}

void HUDCustomizer::setElementOpacity(const std::string& elementId, float opacity) {
    auto it = elements.find(elementId);
    if (it != elements.end()) {
        it->second.opacity = std::max(0.0f, std::min(1.0f, opacity));
        notifyChange(elementId);
    }
}

void HUDCustomizer::setElementVisible(const std::string& elementId, bool visible) {
    auto it = elements.find(elementId);
    if (it != elements.end()) {
        it->second.visible = visible;
        notifyChange(elementId);
    }
}

void HUDCustomizer::setElementLocked(const std::string& elementId, bool locked) {
    auto it = elements.find(elementId);
    if (it != elements.end()) {
        it->second.locked = locked;
    }
}

void HUDCustomizer::applyPreset(LayoutPreset preset) {
    switch (preset) {
        case LayoutPreset::DEFAULT:
            setupDefaultPreset();
            break;
        case LayoutPreset::COMPACT:
            setupCompactPreset();
            break;
        case LayoutPreset::ACCESSIBILITY:
            setupAccessibilityPreset();
            break;
    }
    HCUST_LOGI("Applied preset: %s", getPresetName(preset).c_str());
}

std::string HUDCustomizer::getPresetName(LayoutPreset preset) const {
    switch (preset) {
        case LayoutPreset::DEFAULT:       return "Default";
        case LayoutPreset::COMPACT:       return "Compact";
        case LayoutPreset::ACCESSIBILITY: return "Accessibility";
    }
    return "Unknown";
}

bool HUDCustomizer::saveLayout(const std::string& layoutName) {
    SavedLayout layout;
    layout.layoutName = layoutName;
    layout.elements = elements;
    layout.screenWidth = screenWidth;
    layout.screenHeight = screenHeight;

    savedLayouts[layoutName] = layout;
    HCUST_LOGI("Layout saved: %s (%lu elements)",
               layoutName.c_str(),
               static_cast<unsigned long>(elements.size()));
    return true;
}

bool HUDCustomizer::loadLayout(const std::string& layoutName) {
    auto it = savedLayouts.find(layoutName);
    if (it == savedLayouts.end()) {
        HCUST_LOGW("Layout not found: %s", layoutName.c_str());
        return false;
    }

    elements = it->second.elements;

    // Scale positions if screen size changed
    if (it->second.screenWidth != screenWidth || it->second.screenHeight != screenHeight) {
        float scaleX = static_cast<float>(screenWidth) / static_cast<float>(it->second.screenWidth);
        float scaleY = static_cast<float>(screenHeight) / static_cast<float>(it->second.screenHeight);

        for (auto& pair : elements) {
            pair.second.position.x *= scaleX;
            pair.second.position.y *= scaleY;
            pair.second.currentSize.x *= scaleX;
            pair.second.currentSize.y *= scaleY;
        }
    }

    HCUST_LOGI("Layout loaded: %s", layoutName.c_str());
    return true;
}

std::vector<std::string> HUDCustomizer::getSavedLayoutNames() const {
    std::vector<std::string> names;
    names.reserve(savedLayouts.size());
    for (const auto& pair : savedLayouts) {
        names.push_back(pair.first);
    }
    return names;
}

bool HUDCustomizer::deleteLayout(const std::string& layoutName) {
    auto it = savedLayouts.find(layoutName);
    if (it != savedLayouts.end()) {
        savedLayouts.erase(it);
        HCUST_LOGI("Layout deleted: %s", layoutName.c_str());
        return true;
    }
    return false;
}

void HUDCustomizer::resetToDefaults() {
    setupDefaultPreset();
    HCUST_LOGI("Reset to default layout");
}

void HUDCustomizer::onScreenResize(int newWidth, int newHeight) {
    float scaleX = static_cast<float>(newWidth) / static_cast<float>(screenWidth);
    float scaleY = static_cast<float>(newHeight) / static_cast<float>(screenHeight);

    for (auto& pair : elements) {
        pair.second.position.x *= scaleX;
        pair.second.position.y *= scaleY;
        pair.second.currentSize = glm::vec2(
            pair.second.baseSize.x * pair.second.scale,
            pair.second.baseSize.y * pair.second.scale);
    }

    screenWidth = newWidth;
    screenHeight = newHeight;
    HCUST_LOGI("Screen resized to %dx%d", newWidth, newHeight);
}

void HUDCustomizer::notifyChange(const std::string& elementId) {
    for (auto& cb : changeCallbacks) {
        cb(elementId);
    }
}

glm::vec2 HUDCustomizer::clampPosition(const glm::vec2& pos, const glm::vec2& size) const {
    float x = std::max(0.0f, std::min(pos.x, static_cast<float>(screenWidth) - size.x));
    float y = std::max(0.0f, std::min(pos.y, static_cast<float>(screenHeight) - size.y));
    return glm::vec2(x, y);
}

void HUDCustomizer::setupDefaultPreset() {
    float margin = 16.0f;
    float sw = static_cast<float>(screenWidth);
    float sh = static_cast<float>(screenHeight);

    for (auto& pair : elements) {
        auto& elem = pair.second;
        elem.scale = 1.0f;
        elem.opacity = 1.0f;
        elem.visible = true;
        elem.currentSize = glm::vec2(elem.baseSize.x * elem.scale, elem.baseSize.y * elem.scale);

        // Position based on element ID
        if (elem.elementId == "health_bar") {
            elem.position = glm::vec2(margin, margin);
        } else if (elem.elementId == "mana_bar") {
            elem.position = glm::vec2(sw - elem.currentSize.x - margin, margin);
        } else if (elem.elementId == "stamina_bar") {
            elem.position = glm::vec2(sw * 0.5f - elem.currentSize.x * 0.5f, margin);
        } else if (elem.elementId == "quick_slots") {
            elem.position = glm::vec2(sw - elem.currentSize.x - margin,
                                       sh - elem.currentSize.y - margin);
        } else if (elem.elementId == "compass") {
            elem.position = glm::vec2(sw * 0.5f - elem.currentSize.x * 0.5f, margin);
        } else if (elem.elementId == "minimap") {
            elem.position = glm::vec2(margin, margin + 60.0f);
        } else if (elem.elementId == "action_buttons") {
            elem.position = glm::vec2(sw - elem.currentSize.x - margin,
                                       sh * 0.6f);
        } else if (elem.elementId == "joystick") {
            elem.position = glm::vec2(margin, sh * 0.6f);
        }
    }
}

void HUDCustomizer::setupCompactPreset() {
    setupDefaultPreset();

    float scale = 0.75f;
    for (auto& pair : elements) {
        auto& elem = pair.second;
        elem.scale = scale;
        elem.opacity = 0.85f;
        elem.currentSize = glm::vec2(elem.baseSize.x * scale, elem.baseSize.y * scale);
    }
}

void HUDCustomizer::setupAccessibilityPreset() {
    setupDefaultPreset();

    float scale = 1.5f;
    for (auto& pair : elements) {
        auto& elem = pair.second;
        elem.scale = scale;
        elem.opacity = 1.0f;
        elem.currentSize = glm::vec2(elem.baseSize.x * scale, elem.baseSize.y * scale);
    }
}
