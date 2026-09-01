#pragma once

#include <functional>
#include <glm/glm.hpp>
#include <android/log.h>

#define ACCESS_LOG_TAG "AccessibilityManager"
#define ACCESS_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, ACCESS_LOG_TAG, __VA_ARGS__)
#define ACCESS_LOGI(...) __android_log_print(ANDROID_LOG_INFO, ACCESS_LOG_TAG, __VA_ARGS__)

/**
 * @brief Color vision type
 */
enum class ColorBlindMode {
    NONE,
    PROTANOPIA,    // Protanopia (red-blind)
    DEUTERANOPIA,  // Deuteranopia (green-blind)
    TRITANOPIA     // Tritanopia (blue-blind)
};

/**
 * @brief Accessibility settings change callbacks
 */
using AccessibilityChangeCallback = std::function<void()>;

/**
 * @brief Accessibility management
 *
 * Phase 43: Manages game accessibility settings.
 * - Text size adjustment
 * - Contrast adjustment
 * - Input sensitivity adjustment
 * - Color vision support
 */
class AccessibilityManager {
public:
    AccessibilityManager();
    ~AccessibilityManager() = default;

    /**
     * @brief Initialize
     */
    void initialize();

    // === Text Size ===

    /**
     * @brief Set text scale factor (0.5 to 2.0)
     */
    void setTextScale(float scale);

    /**
     * @brief Get text scale factor
     */
    float getTextScale() const { return textScale; }

    // === Contrast ===

    /**
     * @brief Set contrast multiplier (0.5 to 2.0, 1.0 is default)
     */
    void setContrastMultiplier(float multiplier);

    /**
     * @brief Get contrast multiplier
     */
    float getContrastMultiplier() const { return contrastMultiplier; }

    /**
     * @brief Toggle high contrast mode
     */
    void setHighContrastMode(bool enabled);

    /**
     * @brief Is high contrast mode
     */
    bool isHighContrastMode() const { return highContrastMode; }

    // === Input Sensitivity ===

    /**
     * @brief Set touch sensitivity (0.5 to 2.0, 1.0 is default)
     */
    void setTouchSensitivity(float sensitivity);

    /**
     * @brief Get touch sensitivity
     */
    float getTouchSensitivity() const { return touchSensitivity; }

    /**
     * @brief Set camera sensitivity (0.5 to 2.0)
     */
    void setCameraSensitivity(float sensitivity);

    /**
     * @brief Get camera sensitivity
     */
    float getCameraSensitivity() const { return cameraSensitivity; }

    // === Color vision support ===

    /**
     * @brief Set color vision mode
     */
    void setColorBlindMode(ColorBlindMode mode);

    /**
     * @brief Get color vision mode
     */
    ColorBlindMode getColorBlindMode() const { return colorBlindMode; }

    /**
     * @brief Transform color according to color vision mode
     * @param color Original color
     * @return Transformed color
     */
    glm::vec4 applyColorBlindFilter(const glm::vec4& color) const;

    // === Callbacks ===

    /**
     * @brief Register settings change callback
     */
    void registerChangeCallback(AccessibilityChangeCallback callback);

    // === Reset ===

    /**
     * @brief Reset all settings to default
     */
    void resetToDefaults();

private:
    float textScale;
    float contrastMultiplier;
    float touchSensitivity;
    float cameraSensitivity;
    bool highContrastMode;
    ColorBlindMode colorBlindMode;

    std::vector<AccessibilityChangeCallback> changeCallbacks;

    void notifyChange();

    // Color vision conversion matrix
    glm::vec4 applyProtanopia(const glm::vec4& color) const;
    glm::vec4 applyDeuteranopia(const glm::vec4& color) const;
    glm::vec4 applyTritanopia(const glm::vec4& color) const;
};
