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
    // グローバル状態
    // ========================================

    static bool g_initialized = false;
    static int g_screenWidth = 1080;
    static int g_screenHeight = 1920;

    // ========================================
    // 初期化・クリーンアップ
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
    // ヘルパー関数
    // ========================================

    static void updateScreenSize(int width, int height) {
        g_screenWidth = width;
        g_screenHeight = height;
    }

    // ========================================
    // 基本描画関数
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
    // パネル描画
    // ========================================

    void drawPanel(float x, float y, float width, float height) {
        drawPanel(x, y, width, height, Colors::PARCHMENT_LIGHT, Colors::BROWN_ACCENT);
    }

    void drawPanel(float x, float y, float width, float height,
                  const glm::vec3& bgColor, const glm::vec3& borderColor) {
        // 背景
        drawSolidRect(x, y, width, height, bgColor, 0.95f);

        // 枠線（太さ2ピクセル）
        const float borderWidth = 2.0f;

        glm::vec4 borderColorWithAlpha(borderColor.x, borderColor.y, borderColor.z, 1.0f);
        UIDrawHelper::drawBorder(x, y, width, height, borderWidth,
                                borderColorWithAlpha, g_screenWidth, g_screenHeight);
    }

    // ========================================
    // ステータスバー描画
    // ========================================

    void drawStatusBar(float x, float y, float width, float height,
                      float fillRatio, const glm::vec3& fillColor) {
        // 制約
        fillRatio = std::clamp(fillRatio, 0.0f, 1.0f);

        // 背景（暗いグレー）
        drawSolidRect(x, y, width, height, Colors::DARK_GRAY, 0.7f);

        // フィル（カラー）
        float fillWidth = width * fillRatio;
        if (fillWidth > 0.0f) {
            drawSolidRect(x, y, fillWidth, height, fillColor, 0.9f);
        }

        // 枠線（淡いグレー）
        const float borderWidth = 1.0f;
        glm::vec4 borderColor(Colors::LIGHT_GRAY.x, Colors::LIGHT_GRAY.y,
                             Colors::LIGHT_GRAY.z, 0.8f);
        UIDrawHelper::drawBorder(x, y, width, height, borderWidth,
                                borderColor, g_screenWidth, g_screenHeight);
    }

    // ========================================
    // アイコン・マーカー描画
    // ========================================

    void drawIconFrame(float x, float y, float size) {
        // 背景
        drawSolidRect(x, y, size, size, Colors::PARCHMENT_LIGHT, 0.7f);

        // 枠線
        const float borderWidth = 1.0f;
        glm::vec4 borderColor(Colors::BROWN_ACCENT.x, Colors::BROWN_ACCENT.y,
                             Colors::BROWN_ACCENT.z, 1.0f);
        UIDrawHelper::drawBorder(x, y, size, size, borderWidth,
                                borderColor, g_screenWidth, g_screenHeight);
    }

    // ========================================
    // チェックボックス
    // ========================================

    void drawCheckboxOff(float x, float y, float size) {
        // 背景（白）
        drawSolidRect(x, y, size, size, Colors::LIGHT_GRAY, 1.0f);

        // 枠線（暗いグレー）
        const float borderWidth = 1.0f;
        glm::vec4 borderColor(Colors::DARK_GRAY.x, Colors::DARK_GRAY.y,
                             Colors::DARK_GRAY.z, 1.0f);
        UIDrawHelper::drawBorder(x, y, size, size, borderWidth,
                                borderColor, g_screenWidth, g_screenHeight);
    }

    void drawCheckboxOn(float x, float y, float size) {
        // 背景（白）
        drawSolidRect(x, y, size, size, Colors::LIGHT_GRAY, 1.0f);

        // チェックマーク内側（金色）
        const float inset = size * 0.2f;
        drawSolidRect(x + inset, y + inset, size - 2 * inset, size - 2 * inset,
                     Colors::GOLD_HIGHLIGHT, 0.9f);

        // 枠線（暗いグレー）
        const float borderWidth = 1.0f;
        glm::vec4 borderColor(Colors::DARK_GRAY.x, Colors::DARK_GRAY.y,
                             Colors::DARK_GRAY.z, 1.0f);
        UIDrawHelper::drawBorder(x, y, size, size, borderWidth,
                                borderColor, g_screenWidth, g_screenHeight);
    }

    // ========================================
    // マーカー描画
    // ========================================

    void drawCircleMarker(float x, float y, float radius, const glm::vec3& color) {
        // 単純な円：複数の四角形で近似
        // または、三角形ファンで描画（簡略版）

        // 簡略版：正方形を描画（円に見立てる）
        drawSolidRect(x - radius, y - radius, radius * 2.0f, radius * 2.0f,
                     color, 0.8f);

        // より正確な円を描画する場合は、
        // 複数の小さな四角形または三角形ファンを使用する必要があります
        // ここでは簡略化のため正方形を使用
    }

    void drawTriangleMarker(float x, float y, float size, const glm::vec3& color) {
        // 三角形は複雑なため、簡略版として上向きのダイアモンド型を描画
        // 本来は三角形ファンまたはGL_TRIANGLESで実装

        // 上向きのピーク
        float halfSize = size * 0.5f;

        // ダイアモンド型で近似：4つの三角形領域で表現
        // 簡略版：単純に菱形を描画

        // 中央上部
        drawSolidRect(x - size * 0.1f, y - halfSize, size * 0.2f, halfSize,
                     color, 0.8f);

        // 左側
        drawSolidRect(x - size * 0.4f, y, size * 0.3f, halfSize * 0.5f,
                     color, 0.6f);

        // 右側
        drawSolidRect(x + size * 0.1f, y, size * 0.3f, halfSize * 0.5f,
                     color, 0.6f);

        // 下部
        drawSolidRect(x - size * 0.1f, y + halfSize * 0.5f, size * 0.2f,
                     halfSize * 0.5f, color, 0.8f);
    }

} // namespace PlaceholderAssets
