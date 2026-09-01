#include "accessibility_manager.h"
#include <algorithm>
#include <cmath>
#include <vector>

AccessibilityManager::AccessibilityManager()
    : textScale(1.0f)
    , contrastMultiplier(1.0f)
    , touchSensitivity(1.0f)
    , cameraSensitivity(1.0f)
    , highContrastMode(false)
    , colorBlindMode(ColorBlindMode::NONE) {
}

void AccessibilityManager::initialize() {
    ACCESS_LOGI("AccessibilityManager initialized");
}

// === Text Size ===

void AccessibilityManager::setTextScale(float scale) {
    textScale = std::max(0.5f, std::min(scale, 2.0f));
    ACCESS_LOGI("Text scale: %.2f", textScale);
    notifyChange();
}

// === Contrast ===

void AccessibilityManager::setContrastMultiplier(float multiplier) {
    contrastMultiplier = std::max(0.5f, std::min(multiplier, 2.0f));
    ACCESS_LOGI("Contrast multiplier: %.2f", contrastMultiplier);
    notifyChange();
}

void AccessibilityManager::setHighContrastMode(bool enabled) {
    highContrastMode = enabled;
    ACCESS_LOGI("High contrast mode: %s", enabled ? "ON" : "OFF");
    notifyChange();
}

// === Input Sensitivity ===

void AccessibilityManager::setTouchSensitivity(float sensitivity) {
    touchSensitivity = std::max(0.5f, std::min(sensitivity, 2.0f));
    ACCESS_LOGI("Touch sensitivity: %.2f", touchSensitivity);
    notifyChange();
}

void AccessibilityManager::setCameraSensitivity(float sensitivity) {
    cameraSensitivity = std::max(0.5f, std::min(sensitivity, 2.0f));
    ACCESS_LOGI("Camera sensitivity: %.2f", cameraSensitivity);
    notifyChange();
}

// === Color Vision Support ===

void AccessibilityManager::setColorBlindMode(ColorBlindMode mode) {
    colorBlindMode = mode;
    ACCESS_LOGI("Color blind mode: %d", static_cast<int>(mode));
    notifyChange();
}

glm::vec4 AccessibilityManager::applyColorBlindFilter(const glm::vec4& color) const {
    switch (colorBlindMode) {
        case ColorBlindMode::PROTANOPIA:  return applyProtanopia(color);
        case ColorBlindMode::DEUTERANOPIA: return applyDeuteranopia(color);
        case ColorBlindMode::TRITANOPIA:  return applyTritanopia(color);
        default: return color;
    }
}

// === Callbacks ===

void AccessibilityManager::registerChangeCallback(AccessibilityChangeCallback callback) {
    changeCallbacks.push_back(std::move(callback));
}

// === Reset ===

void AccessibilityManager::resetToDefaults() {
    textScale = 1.0f;
    contrastMultiplier = 1.0f;
    touchSensitivity = 1.0f;
    cameraSensitivity = 1.0f;
    highContrastMode = false;
    colorBlindMode = ColorBlindMode::NONE;
    ACCESS_LOGI("Accessibility settings reset to defaults");
    notifyChange();
}

// === Internal Helpers ===

void AccessibilityManager::notifyChange() {
    for (const auto& cb : changeCallbacks) {
        cb();
    }
}

// Color vision conversion: apply simulation matrix for each type
// Reference: Brettel, Vienot, Mollon (1997)

glm::vec4 AccessibilityManager::applyProtanopia(const glm::vec4& color) const {
    // Protanopia: attenuate red component
    float r = color.x * 0.567f + color.y * 0.433f + color.z * 0.0f;
    float g = color.x * 0.558f + color.y * 0.442f + color.z * 0.0f;
    float b = color.x * 0.0f   + color.y * 0.242f + color.z * 0.758f;
    return glm::vec4(r, g, b, color.w);
}

glm::vec4 AccessibilityManager::applyDeuteranopia(const glm::vec4& color) const {
    // Deuteranopia: attenuate green component
    float r = color.x * 0.625f + color.y * 0.375f + color.z * 0.0f;
    float g = color.x * 0.7f   + color.y * 0.3f   + color.z * 0.0f;
    float b = color.x * 0.0f   + color.y * 0.3f   + color.z * 0.7f;
    return glm::vec4(r, g, b, color.w);
}

glm::vec4 AccessibilityManager::applyTritanopia(const glm::vec4& color) const {
    // Tritanopia: attenuate blue component
    float r = color.x * 0.95f  + color.y * 0.05f  + color.z * 0.0f;
    float g = color.x * 0.0f   + color.y * 0.433f + color.z * 0.567f;
    float b = color.x * 0.0f   + color.y * 0.475f + color.z * 0.525f;
    return glm::vec4(r, g, b, color.w);
}
