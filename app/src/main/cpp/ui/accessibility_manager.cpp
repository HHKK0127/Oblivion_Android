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

// === テキストサイズ ===

void AccessibilityManager::setTextScale(float scale) {
    textScale = std::max(0.5f, std::min(scale, 2.0f));
    ACCESS_LOGI("Text scale: %.2f", textScale);
    notifyChange();
}

// === コントラスト ===

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

// === 操作感度 ===

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

// === 色覚サポート ===

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

// === コールバック ===

void AccessibilityManager::registerChangeCallback(AccessibilityChangeCallback callback) {
    changeCallbacks.push_back(std::move(callback));
}

// === リセット ===

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

// === 内部ヘルパー ===

void AccessibilityManager::notifyChange() {
    for (const auto& cb : changeCallbacks) {
        cb();
    }
}

// 色覚変換: 各タイプのシミュレーション行列を適用
// 参考: Brettel, Viénot, Mollon (1997)

glm::vec4 AccessibilityManager::applyProtanopia(const glm::vec4& color) const {
    // 赤色盲: 赤成分を減衰
    float r = color.x * 0.567f + color.y * 0.433f + color.z * 0.0f;
    float g = color.x * 0.558f + color.y * 0.442f + color.z * 0.0f;
    float b = color.x * 0.0f   + color.y * 0.242f + color.z * 0.758f;
    return glm::vec4(r, g, b, color.w);
}

glm::vec4 AccessibilityManager::applyDeuteranopia(const glm::vec4& color) const {
    // 緑色盲: 緑成分を減衰
    float r = color.x * 0.625f + color.y * 0.375f + color.z * 0.0f;
    float g = color.x * 0.7f   + color.y * 0.3f   + color.z * 0.0f;
    float b = color.x * 0.0f   + color.y * 0.3f   + color.z * 0.7f;
    return glm::vec4(r, g, b, color.w);
}

glm::vec4 AccessibilityManager::applyTritanopia(const glm::vec4& color) const {
    // 青色盲: 青成分を減衰
    float r = color.x * 0.95f  + color.y * 0.05f  + color.z * 0.0f;
    float g = color.x * 0.0f   + color.y * 0.433f + color.z * 0.567f;
    float b = color.x * 0.0f   + color.y * 0.475f + color.z * 0.525f;
    return glm::vec4(r, g, b, color.w);
}
