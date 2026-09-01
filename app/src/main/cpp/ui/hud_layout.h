#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <android/log.h>

#define HUD_LAYOUT_LOG_TAG "HUDLayout"
#define HLAYOUT_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, HUD_LAYOUT_LOG_TAG, __VA_ARGS__)
#define HLAYOUT_LOGI(...) __android_log_print(ANDROID_LOG_INFO, HUD_LAYOUT_LOG_TAG, __VA_ARGS__)

class TextRenderer;

/**
 * @brief HUD element types
 */
enum class HUDElementType {
    HEALTH_BAR,
    MANA_BAR,
    STAMINA_BAR,
    QUICK_SLOTS,
    COMPASS,
    MINIMAP,
    TARGET_INFO,
    PLAYER_LEVEL,
    ACTIVE_EFFECTS,
    ACTION_PROMPT
};

/**
 * @brief HUD element layout information
 */
struct HUDElementLayout {
    HUDElementType type;
    glm::vec2 position;   // Pixel coordinates
    glm::vec2 size;       // Pixel size
    bool visible;
    float scale;          // Scaling factor

    HUDElementLayout()
        : type(HUDElementType::HEALTH_BAR), position(0.0f), size(0.0f),
          visible(true), scale(1.0f) {}
};

/**
 * @brief HUD layout management
 *
 * Phase 43: Manages HUD element placement and automatic adaptation to screen size.
 * - Health bar (top left)
 * - Mana bar (top right)
 * - Stamina bar (bottom center)
 * - Quick slots (bottom right)
 * - Compass (top center)
 * - Minimap (top left, below health bar)
 * - Target info (center)
 */
class HUDLayout {
public:
    HUDLayout();
    ~HUDLayout() = default;

    /**
     * @brief Initialize
     * @param screenWidth Screen width
     * @param screenHeight Screen height
     */
    void initialize(int screenWidth, int screenHeight);

    /**
     * @brief Recalculate layout when screen size changes
     */
    void recalculate(int screenWidth, int screenHeight);

    /**
     * @brief Get HUD element layout information
     * @param type Element type
     * @return Layout information
     */
    const HUDElementLayout& getElementLayout(HUDElementType type) const;

    /**
     * @brief Override HUD element position
     */
    void setElementPosition(HUDElementType type, const glm::vec2& position);

    /**
     * @brief Override HUD element size
     */
    void setElementSize(HUDElementType type, const glm::vec2& size);

    /**
     * @brief Set HUD element visibility
     */
    void setElementVisible(HUDElementType type, bool visible);

    /**
     * @brief Set HUD element scale
     */
    void setElementScale(HUDElementType type, float scale);

    /**
     * @brief Get list of all HUD elements
     */
    const std::vector<HUDElementLayout>& getAllElements() const { return elements; }

    /**
     * @brief Get UI scale factor (DPI-aware)
     */
    float getUIScale() const { return uiScale; }

    /**
     * @brief Set margin
     */
    void setMargin(float margin) { edgeMargin = margin; recalculate(screenWidth, screenHeight); }

    /**
     * @brief Set bar height
     */
    void setBarHeight(float height) { barHeight = height; recalculate(screenWidth, screenHeight); }

    /**
     * @brief Debug: log positions of all elements
     */
    void debugLogLayout() const;

private:
    int screenWidth;
    int screenHeight;
    float uiScale;
    float edgeMargin;
    float barHeight;
    float barWidth;
    float quickSlotSize;
    float minimapSize;
    float compassHeight;

    // Layout of all elements
    std::vector<HUDElementLayout> elements;

    // Element search helper
    HUDElementLayout* findElement(HUDElementType type);
    const HUDElementLayout* findElement(HUDElementType type) const;

    // Default layout calculation
    void calculateDefaultLayout();
};
