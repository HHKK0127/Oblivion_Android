#include "settings_ui.h"
#include "text_renderer.h"
#include "../system/settings_manager.h"
#include "../engine/renderer.h"
#include "../engine/graphics/retro_filter.h"
#include <android/log.h>

#define LOG_TAG_SETTINGS "SettingsUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_SETTINGS, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_SETTINGS, __VA_ARGS__)

SettingsUI::SettingsUI()
    : textRenderer(nullptr), settingsManager(nullptr), renderer(nullptr),
      visible(false), returnToMenu(false), selectedIndex(0), scrollOffset(0.0f) {
}

SettingsUI::~SettingsUI() {
    cleanup();
}

bool SettingsUI::initialize(TextRenderer* textRenderer, SettingsManager* settingsManager, Renderer* renderer) {
    if (!textRenderer || !settingsManager) {
        LOGD("SettingsUI: null dependencies");
        return false;
    }
    this->textRenderer = textRenderer;
    this->settingsManager = settingsManager;
    this->renderer = renderer;
    visible = false;
    LOGI("SettingsUI initialized");
    return true;
}

void SettingsUI::cleanup() {
    textRenderer = nullptr;
    settingsManager = nullptr;
    renderer = nullptr;
}

void SettingsUI::update(float deltaTime) {
}

void SettingsUI::render() {
    if (!visible || !textRenderer) return;

    float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float yellow[4] = {1.0f, 1.0f, 0.0f, 1.0f};
    float gray[4] = {0.7f, 0.7f, 0.7f, 1.0f};

    textRenderer->renderText("Settings", 100.0f, 60.0f, 1.0f, white);
    textRenderer->renderText("Return to Menu", 100.0f, 110.0f, 0.75f, white);

    // Retro Filter Settings
    if (renderer) {
        float y = 180.0f;
        auto* retroSettings = renderer->getRetroSettings();

        textRenderer->renderText("=== RETRO FILTERS ===", 100.0f, y, 0.8f, yellow);
        y += 40.0f;

        std::string enabledStr = retroSettings->enabled ? "ON" : "OFF";
        textRenderer->renderText("Filter Enabled: " + enabledStr, 100.0f, y, 0.6f, white);
        y += 30.0f;

        if (retroSettings->enabled) {
            std::string scanStr = retroSettings->scanlines_enabled ? "ON" : "OFF";
            textRenderer->renderText("Scanlines: " + scanStr + " (Intensity: " + std::to_string(static_cast<int>(retroSettings->scanlines_intensity * 100.0f)) + "%)", 100.0f, y, 0.6f, gray);
            y += 25.0f;

            std::string pixStr = retroSettings->pixelation_enabled ? "ON" : "OFF";
            textRenderer->renderText("Pixelation: " + pixStr + " (Scale: " + std::to_string(static_cast<int>(retroSettings->pixelation_scale)) + ")", 100.0f, y, 0.6f, gray);
            y += 25.0f;

            std::string colStr = retroSettings->color_reduction_enabled ? "ON" : "OFF";
            textRenderer->renderText("Color Reduction: " + colStr + " (Bits: " + std::to_string(retroSettings->color_bits) + ")", 100.0f, y, 0.6f, gray);
            y += 25.0f;

            std::string crtStr = retroSettings->crt_distortion_enabled ? "ON" : "OFF";
            textRenderer->renderText("CRT Distortion: " + crtStr + " (Strength: " + std::to_string(static_cast<int>(retroSettings->distortion_strength * 100.0f)) + "%)", 100.0f, y, 0.6f, gray);
            y += 25.0f;

            std::string grainStr = retroSettings->grain_enabled ? "ON" : "OFF";
            textRenderer->renderText("Film Grain: " + grainStr + " (Strength: " + std::to_string(static_cast<int>(retroSettings->grain_strength * 100.0f)) + "%)", 100.0f, y, 0.6f, gray);
            y += 25.0f;
        }

        textRenderer->renderText("[Tap here to toggle filter]", 100.0f, y + 20.0f, 0.6f, yellow);
    }
}

void SettingsUI::toggle() {
    visible = !visible;
    LOGD("SettingsUI visibility toggled: %d", visible);
}

void SettingsUI::show() { visible = true; }
void SettingsUI::hide() { visible = false; }

void SettingsUI::onTouchEvent(float x, float y) {
    if (!visible) return;

    // Toggle retro filter
    if (renderer && y > 180.0f && y < 450.0f) {
        auto* retroSettings = renderer->getRetroSettings();
        if (y > 180.0f && y < 220.0f) {
            // Toggle main filter switch
            retroSettings->enabled = !retroSettings->enabled;
            LOGI("Retro filter toggled: %s", retroSettings->enabled ? "ON" : "OFF");
            return;
        } else if (retroSettings->enabled) {
            // Toggle individual effects
            if (y > 220.0f && y < 245.0f) {
                retroSettings->scanlines_enabled = !retroSettings->scanlines_enabled;
            } else if (y > 245.0f && y < 270.0f) {
                retroSettings->pixelation_enabled = !retroSettings->pixelation_enabled;
            } else if (y > 270.0f && y < 295.0f) {
                retroSettings->color_reduction_enabled = !retroSettings->color_reduction_enabled;
            } else if (y > 295.0f && y < 320.0f) {
                retroSettings->crt_distortion_enabled = !retroSettings->crt_distortion_enabled;
            } else if (y > 320.0f && y < 345.0f) {
                retroSettings->grain_enabled = !retroSettings->grain_enabled;
            }
            return;
        }
    }

    // Return to menu
    if (x > 80.0f && x < 500.0f && y > 110.0f && y < 150.0f) {
        returnToMenu = true;
        LOGD("SettingsUI: Return to menu requested");
    }
}

void SettingsUI::onKeyEvent(int keyCode, int action) {
}

void SettingsUI::resetReturnFlag() {
    returnToMenu = false;
}