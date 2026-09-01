#include "placeholder_assets.h"
#include <algorithm>
#include "ui_draw_helper.h"
#include <android/log.h>
#include <cmath>

#define LOG_TAG "PlaceholderAssets"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace PlaceholderAssets {

    // ========================================
    // Global state
    // ========================================

    static bool g_initialized = false;
    static int g_screenWidth = 1080;
    static int g_screenHeight = 1920;

    // ========================================
    // Initialization and cleanup
    // ========================================

    bool initialize() {
        if (g_initialized) {
            LOGW("PlaceholderAssets already initialized");
            return true;
        }

        UIDrawHelper::initialize();
        g_initialized = true;

        LOGI("PlaceholderAssets initialized");
        return true;
    }

    void cleanup() {
        if (!g_initialized) {
            return;
        }

        UIDrawHelper::cleanup();
        g_initialized = false;

        LOGI("PlaceholderAssets cleaned up");
    }

    // ========================================
    // Helper functions
    // ========================================

    static void updateScreenSize(int width, int height) {
        g_screenWidth = width;
        g_screenHeight = height;
    }

    // ========================================
    // Basic drawing functions
    // ========================================

    void drawSolidRect(float x, float y, float width, float height,
                      const glm::vec3& color) {
        drawSolidRect(x, y, width, height, color, 1.0f);
    }

    void drawSolidRect(float x, float y, float width, float height,
                      const glm::vec3& color, float alpha) {
        glm::vec4 colorWithAlpha(color.x, color.y, color.z, alpha);
        UIDrawHelper::drawColoredQuad(x, y, width, height, colorWithAlpha,
                                     g_screenWidth, g_screenHeight);
    }

    // ========================================
    // Panel drawing
    // ========================================

    void drawPanel(float x, float y, float width, float height) {
        drawPanel(x, y, width, height, Colors::PARCHMENT_LIGHT, Colors::BROWN_ACCENT);
    }

    void drawPanel(float x, float y, float width, float height,
                  const glm::vec3& bgColor, const glm::vec3& borderColor) {
        // Background
        drawSolidRect(x, y, width, height, bgColor, 0.95f);

        // Border (2 pixels thick)
        const float borderWidth = 2.0f;

        glm::vec4 borderColorWithAlpha(borderColor.x, borderColor.y, borderColor.z, 1.0f);
        UIDrawHelper::drawBorder(x, y, width, height, borderWidth,
                                borderColorWithAlpha, g_screenWidth, g_screenHeight);
    }

    // ========================================
    // Status bar drawing
    // ========================================

    void drawStatusBar(float x, float y, float width, float height,
                      float fillRatio, const glm::vec3& fillColor) {
        // Constraints
        fillRatio = std::clamp(fillRatio, 0.0f, 1.0f);

        // Background (dark gray)
        drawSolidRect(x, y, width, height, Colors::DARK_GRAY, 0.7f);

        // Fill (color)
        float fillWidth = width * fillRatio;
        if (fillWidth > 0.0f) {
            drawSolidRect(x, y, fillWidth, height, fillColor, 0.9f);
        }

        // Border (light gray)
        const float borderWidth = 1.0f;
        glm::vec4 borderColor(Colors::LIGHT_GRAY.x, Colors::LIGHT_GRAY.y,
                             Colors::LIGHT_GRAY.z, 0.8f);
        UIDrawHelper::drawBorder(x, y, width, height, borderWidth,
                                borderColor, g_screenWidth, g_screenHeight);
    }

    // ========================================
    // Icon/marker drawing
    // ========================================

    void drawIconFrame(float x, float y, float size) {
        // Background
        drawSolidRect(x, y, size, size, Colors::PARCHMENT_LIGHT, 0.7f);

        // Border
        const float borderWidth = 1.0f;
        glm::vec4 borderColor(Colors::BROWN_ACCENT.x, Colors::BROWN_ACCENT.y,
                             Colors::BROWN_ACCENT.z, 1.0f);
        UIDrawHelper::drawBorder(x, y, size, size, borderWidth,
                                borderColor, g_screenWidth, g_screenHeight);
    }

    // ========================================
    // Checkbox
    // ========================================

    void drawCheckboxOff(float x, float y, float size) {
        // Background (white)
        drawSolidRect(x, y, size, size, Colors::LIGHT_GRAY, 1.0f);

        // Border (dark gray)
        const float borderWidth = 1.0f;
        glm::vec4 borderColor(Colors::DARK_GRAY.x, Colors::DARK_GRAY.y,
                             Colors::DARK_GRAY.z, 1.0f);
        UIDrawHelper::drawBorder(x, y, size, size, borderWidth,
                                borderColor, g_screenWidth, g_screenHeight);
    }

    void drawCheckboxOn(float x, float y, float size) {
        // Background (white)
        drawSolidRect(x, y, size, size, Colors::LIGHT_GRAY, 1.0f);

        // Checkmark inner (gold)
        const float inset = size * 0.2f;
        drawSolidRect(x + inset, y + inset, size - 2 * inset, size - 2 * inset,
                     Colors::GOLD_HIGHLIGHT, 0.9f);

        // Border (dark gray)
        const float borderWidth = 1.0f;
        glm::vec4 borderColor(Colors::DARK_GRAY.x, Colors::DARK_GRAY.y,
                             Colors::DARK_GRAY.z, 1.0f);
        UIDrawHelper::drawBorder(x, y, size, size, borderWidth,
                                borderColor, g_screenWidth, g_screenHeight);
    }

    // ========================================
    // Marker drawing
    // ========================================

    void drawCircleMarker(float x, float y, float radius, const glm::vec3& color) {
        // Simple circle: approximated with multiple rectangles
        // Or draw with triangle fan (simplified)

        // Simplified: draw a square (pretend as circle)
        drawSolidRect(x - radius, y - radius, radius * 2.0f, radius * 2.0f,
                     color, 0.8f);

        // For more accurate circle rendering,
        // use multiple small rectangles or triangle fan
        // Using square here for simplicity
    }

    void drawTriangleMarker(float x, float y, float size, const glm::vec3& color) {
        // Triangles are complex, so draw upward diamond shape as simplified version
        // Originally should be implemented with triangle fan or GL_TRIANGLES

        // Upward peak
        float halfSize = size * 0.5f;

        // Approximate with diamond: represented by 4 triangle regions
        // Simplified: simply draw a rhombus

        // Center top
        drawSolidRect(x - size * 0.1f, y - halfSize, size * 0.2f, halfSize,
                     color, 0.8f);

        // Left side
        drawSolidRect(x - size * 0.4f, y, size * 0.3f, halfSize * 0.5f,
                     color, 0.6f);

        // Right side
        drawSolidRect(x + size * 0.1f, y, size * 0.3f, halfSize * 0.5f,
                     color, 0.6f);

        // Bottom
        drawSolidRect(x - size * 0.1f, y + halfSize * 0.5f, size * 0.2f,
                     halfSize * 0.5f, color, 0.8f);
    }

} // namespace PlaceholderAssets
