#pragma once

#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include <vector>

/**
 * @brief Placeholder asset generation utility
 *
 * Phase 9: Temporary asset generation for UI framework foundation
 * Uses parchment/antiquarian theme palette for
 * programmatic placeholder generation
 */
namespace PlaceholderAssets {

    // ========================================
    // Color palette definition (parchment style)
    // ========================================

    /// Oblivion UI theme color definitions
    namespace Colors {
        const glm::vec3 PARCHMENT_LIGHT(0.77f, 0.66f, 0.50f);   // #C4A97F - Light parchment
        const glm::vec3 PARCHMENT_DARK(0.55f, 0.45f, 0.33f);    // #8B7355 - Dark parchment
        const glm::vec3 BROWN_ACCENT(0.36f, 0.25f, 0.20f);      // #5C4033 - Brown accent
        const glm::vec3 GOLD_HIGHLIGHT(0.85f, 0.65f, 0.13f);    // #DAA520 - Gold highlight
        const glm::vec3 RED_HEALTH(0.80f, 0.10f, 0.10f);        // #CC1919 - HP (red)
        const glm::vec3 BLUE_MANA(0.20f, 0.40f, 0.80f);         // #3366CC - MP (blue)
        const glm::vec3 GREEN_STAMINA(0.40f, 0.70f, 0.30f);     // #66B319 - ST (green)
        const glm::vec3 DARK_GRAY(0.20f, 0.20f, 0.20f);         // #333333 - Dark gray
        const glm::vec3 LIGHT_GRAY(0.80f, 0.80f, 0.80f);        // #CCCCCC - Light gray
    }

    // ========================================
    // Drawing utility functions
    // ========================================

    /**
     * @brief Single color rectangle drawing
     * @param x Left X coordinate (screen coordinates)
     * @param y Top Y coordinate (screen coordinates)
     * @param width Width (pixels)
     * @param height Height (pixels)
     * @param color Drawing color（RGB）
     */
    void drawSolidRect(float x, float y, float width, float height, const glm::vec3& color);

    /**
     * @brief Single color rectangle drawing (with alpha)
     * @param x Left X coordinate
     * @param y Top Y coordinate
     * @param width Width
     * @param height Height
     * @param color Drawing color（RGB）
     * @param alpha Alpha (0.0-1.0)
     */
    void drawSolidRect(float x, float y, float width, float height,
                      const glm::vec3& color, float alpha);

    /**
     * @brief Panel drawing with border (parchment style)
     * @param x Left X coordinate
     * @param y Top Y coordinate
     * @param width Width
     * @param height Height
     */
    void drawPanel(float x, float y, float width, float height);

    /**
     * @brief Panel drawing (custom color)
     * @param x Left X coordinate
     * @param y Top Y coordinate
     * @param width Width
     * @param height Height
     * @param bgColor Background color
     * @param borderColor Border color
     */
    void drawPanel(float x, float y, float width, float height,
                  const glm::vec3& bgColor, const glm::vec3& borderColor);

    /**
     * @brief Status bar drawing (background + fill)
     * @param x Left X coordinate
     * @param y Top Y coordinate
     * @param width Width
     * @param height Height
     * @param fillRatio Fill ratio (0.0-1.0)
     * @param fillColor Fill color
     */
    void drawStatusBar(float x, float y, float width, float height,
                      float fillRatio, const glm::vec3& fillColor);

    /**
     * @brief HP bar drawing (red color)
     * @param x Left X coordinate
     * @param y Top Y coordinate
     * @param width Width
     * @param height Height
     * @param fillRatio Fill ratio (0.0-1.0)
     */
    inline void drawHPBar(float x, float y, float width, float height, float fillRatio) {
        drawStatusBar(x, y, width, height, fillRatio, Colors::RED_HEALTH);
    }

    /**
     * @brief MP bar drawing (blue color)
     * @param x Left X coordinate
     * @param y Top Y coordinate
     * @param width Width
     * @param height Height
     * @param fillRatio Fill ratio (0.0-1.0)
     */
    inline void drawMPBar(float x, float y, float width, float height, float fillRatio) {
        drawStatusBar(x, y, width, height, fillRatio, Colors::BLUE_MANA);
    }

    /**
     * @brief Stamina bar drawing (green color)
     * @param x Left X coordinate
     * @param y Top Y coordinate
     * @param width Width
     * @param height Height
     * @param fillRatio Fill ratio (0.0-1.0)
     */
    inline void drawStaminaBar(float x, float y, float width, float height, float fillRatio) {
        drawStatusBar(x, y, width, height, fillRatio, Colors::GREEN_STAMINA);
    }

    /**
     * @brief Icon frame drawing
     * @param x Left X coordinate
     * @param y Top Y coordinate
     * @param size Size (square)
     */
    void drawIconFrame(float x, float y, float size);

    /**
     * @brief Checkbox drawing (unchecked)
     * @param x Left X coordinate
     * @param y Top Y coordinate
     * @param size Size (square)
     */
    void drawCheckboxOff(float x, float y, float size);

    /**
     * @brief Checkbox drawing (checked)
     * @param x Left X coordinate
     * @param y Top Y coordinate
     * @param size Size (square)
     */
    void drawCheckboxOn(float x, float y, float size);

    /**
     * @brief Circle marker drawing
     * @param x Center X coordinate
     * @param y Center Y coordinate
     * @param radius Radius
     * @param color Drawing color
     */
    void drawCircleMarker(float x, float y, float radius, const glm::vec3& color);

    /**
     * @brief Triangle marker drawing (upward)
     * @param x Center X coordinate
     * @param y Center Y coordinate
     * @param size Size
     * @param color Drawing color
     */
    void drawTriangleMarker(float x, float y, float size, const glm::vec3& color);

    // ========================================
    // Initialization and cleanup
    // ========================================

    /**
     * @brief Initialize placeholder asset system
     * @return True on success
     */
    bool initialize();

    /**
     * @brief Cleanup placeholder assets
     */
    void cleanup();

} // namespace PlaceholderAssets
