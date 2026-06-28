#include "hud_renderer.h"
#include "placeholder_assets.h"
#include "text_renderer.h"
#include "ui_system.h"
#include "ui_draw_helper.h"
#include <android/log.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

#define LOG_TAG "HUDRenderer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

HUDRenderer::HUDRenderer() {
    LOGD("HUDRenderer created");
}

HUDRenderer::~HUDRenderer() {
    cleanup();
}

bool HUDRenderer::initialize(TextRenderer* textRenderer, UISystem* uiSystem,
                            int screenWidth, int screenHeight) {
    if (initialized) {
        LOGW("HUDRenderer already initialized");
        return true;
    }

    if (!textRenderer) {
        LOGE("TextRenderer is null");
        return false;
    }

    this->textRenderer = textRenderer;
    this->uiSystem = uiSystem;
    this->screenWidth = screenWidth;
    this->screenHeight = screenHeight;

    // PlaceholderAssets を初期化
    if (!PlaceholderAssets::initialize()) {
        LOGE("Failed to initialize PlaceholderAssets");
        return false;
    }

    // デフォルトレイアウトを計算
    statusBarY = screenHeight - 150.0f;
    minimapX = screenWidth - 150.0f;
    quickSlotX = (screenWidth - (NUM_QUICK_SLOTS * 50.0f)) / 2.0f;
    quickSlotY = screenHeight - 70.0f;

    initialized = true;
    LOGI("HUDRenderer initialized (screen: %dx%d)", screenWidth, screenHeight);

    return true;
}

void HUDRenderer::update(float deltaTime) {
    // 更新ロジック（必要に応じて）
}

void HUDRenderer::render() {
    if (!visible || !initialized) {
        return;
    }

    // OpenGL 状態を設定
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 各HUD要素を描画
    renderStatusBars();
    renderMinimap();
    renderQuickSlots();
    renderPlayerLevel();

    // OpenGL 状態を復元
    glEnable(GL_DEPTH_TEST);
}

void HUDRenderer::cleanup() {
    if (initialized) {
        PlaceholderAssets::cleanup();
        initialized = false;
        LOGI("HUDRenderer cleaned up");
    }
}

void HUDRenderer::onScreenResize(int width, int height) {
    screenWidth = width;
    screenHeight = height;

    // レイアウトを再計算
    statusBarY = screenHeight - 150.0f;
    minimapX = screenWidth - 150.0f;
    quickSlotX = (screenWidth - (NUM_QUICK_SLOTS * 50.0f)) / 2.0f;
    quickSlotY = screenHeight - 70.0f;

    LOGI("HUDRenderer resized to %dx%d", width, height);
}

// === セッター ===

void HUDRenderer::setPlayerHealth(float current, float max) {
    healthCurrent = std::clamp(current, 0.0f, max);
    healthMax = max;
}

void HUDRenderer::setPlayerMana(float current, float max) {
    manaCurrent = std::clamp(current, 0.0f, max);
    manaMax = max;
}

void HUDRenderer::setPlayerStamina(float current, float max) {
    staminaCurrent = std::clamp(current, 0.0f, max);
    staminaMax = max;
}

void HUDRenderer::setQuickSlotItem(int slotIndex, const std::string& itemName) {
    if (slotIndex >= 0 && slotIndex < NUM_QUICK_SLOTS) {
        quickSlotItems[slotIndex] = itemName;
    }
}

void HUDRenderer::clearQuickSlots() {
    for (int i = 0; i < NUM_QUICK_SLOTS; i++) {
        quickSlotItems[i].clear();
    }
}

void HUDRenderer::setStatusBarPosition(float x, float y) {
    statusBarX = x;
    statusBarY = y;
}

void HUDRenderer::setMinimapPosition(float x, float y) {
    minimapX = x;
    minimapY = y;
}

void HUDRenderer::setQuickSlotPosition(float x, float y) {
    quickSlotX = x;
    quickSlotY = y;
}

// === ヘルパー描画関数 ===

void HUDRenderer::renderStatusBars() {
    const float barWidth = 200.0f;
    const float barHeight = 16.0f;
    const float spacing = 8.0f;
    const float labelWidth = 50.0f;

    float yPos = statusBarY;

    // HP ラベル
    if (textRenderer) {
        textRenderer->renderText("HP:", statusBarX, yPos,
                               glm::vec3(1.0f, 1.0f, 1.0f), 1.0f);
    }

    // HP バー
    float hpRatio = healthMax > 0.0f ? healthCurrent / healthMax : 0.0f;
    PlaceholderAssets::drawStatusBar(statusBarX + labelWidth, yPos,
                                    barWidth, barHeight, hpRatio,
                                    PlaceholderAssets::Colors::RED_HEALTH);

    // HP 数値
    if (textRenderer) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0)
           << healthCurrent << "/" << healthMax;
        textRenderer->renderText(ss.str(), statusBarX + labelWidth + barWidth + spacing,
                               yPos, glm::vec3(1.0f, 1.0f, 1.0f), 0.8f);
    }

    yPos += barHeight + spacing;

    // MP ラベル
    if (textRenderer) {
        textRenderer->renderText("MP:", statusBarX, yPos,
                               glm::vec3(1.0f, 1.0f, 1.0f), 1.0f);
    }

    // MP バー
    float manaRatio = manaMax > 0.0f ? manaCurrent / manaMax : 0.0f;
    PlaceholderAssets::drawStatusBar(statusBarX + labelWidth, yPos,
                                    barWidth, barHeight, manaRatio,
                                    PlaceholderAssets::Colors::BLUE_MANA);

    // MP 数値
    if (textRenderer) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0)
           << manaCurrent << "/" << manaMax;
        textRenderer->renderText(ss.str(), statusBarX + labelWidth + barWidth + spacing,
                               yPos, glm::vec3(1.0f, 1.0f, 1.0f), 0.8f);
    }

    yPos += barHeight + spacing;

    // スタミナ ラベル
    if (textRenderer) {
        textRenderer->renderText("ST:", statusBarX, yPos,
                               glm::vec3(1.0f, 1.0f, 1.0f), 1.0f);
    }

    // スタミナ バー
    float staminaRatio = staminaMax > 0.0f ? staminaCurrent / staminaMax : 0.0f;
    PlaceholderAssets::drawStatusBar(statusBarX + labelWidth, yPos,
                                    barWidth, barHeight, staminaRatio,
                                    PlaceholderAssets::Colors::GREEN_STAMINA);

    // スタミナ 数値
    if (textRenderer) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(0)
           << staminaCurrent << "/" << staminaMax;
        textRenderer->renderText(ss.str(), statusBarX + labelWidth + barWidth + spacing,
                               yPos, glm::vec3(1.0f, 1.0f, 1.0f), 0.8f);
    }
}

void HUDRenderer::renderMinimap() {
    const float minimapSize = 120.0f;
    const float padding = 10.0f;

    // ミニマップ背景パネル
    PlaceholderAssets::drawPanel(minimapX - minimapSize - padding,
                                 minimapY, minimapSize + padding * 2.0f,
                                 minimapSize + padding * 2.0f);

    // ミニマップ枠
    PlaceholderAssets::drawIconFrame(minimapX - minimapSize, minimapY + padding,
                                     minimapSize);

    // プレイヤーマーカー（中央に小さな三角形）
    const float markerSize = 10.0f;
    PlaceholderAssets::drawTriangleMarker(
        minimapX - minimapSize / 2.0f,
        minimapY + padding + minimapSize / 2.0f,
        markerSize, PlaceholderAssets::Colors::GOLD_HIGHLIGHT);

    // ラベル
    if (textRenderer) {
        textRenderer->renderText("MAP", minimapX - minimapSize / 2.0f - 15.0f,
                               minimapY - 10.0f,
                               PlaceholderAssets::Colors::BROWN_ACCENT, 0.8f);
    }
}

void HUDRenderer::renderQuickSlots() {
    const float slotSize = 48.0f;
    const float slotSpacing = 2.0f;

    for (int i = 0; i < NUM_QUICK_SLOTS; i++) {
        float x = quickSlotX + i * (slotSize + slotSpacing);
        float y = quickSlotY;

        // スロット背景
        if (quickSlotItems[i].empty()) {
            PlaceholderAssets::drawIconFrame(x, y, slotSize);
        } else {
            // アイテムが入っているスロット（金色強調）
            PlaceholderAssets::drawPanel(x, y, slotSize, slotSize,
                                        PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
        }

        // スロット番号ラベル
        if (textRenderer) {
            textRenderer->renderText(std::to_string(i),
                                   x + slotSize - 15.0f,
                                   y + slotSize - 15.0f,
                                   PlaceholderAssets::Colors::BROWN_ACCENT, 0.6f);
        }
    }
}

void HUDRenderer::renderPlayerLevel() {
    const float levelX = 20.0f;
    const float levelY = 20.0f;

    // レベルパネル
    PlaceholderAssets::drawPanel(levelX, levelY, 80.0f, 60.0f);

    // テキスト
    if (textRenderer) {
        textRenderer->renderText("LEVEL", levelX + 10.0f, levelY + 10.0f,
                               PlaceholderAssets::Colors::BROWN_ACCENT, 0.8f);
        textRenderer->renderText(std::to_string(playerLevel),
                               levelX + 25.0f, levelY + 35.0f,
                               PlaceholderAssets::Colors::GOLD_HIGHLIGHT, 1.2f);
    }
}
