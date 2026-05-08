#include "settings_ui.h"
#include "text_renderer.h"
#include "../system/settings_manager.h"
#include <android/log.h>

#define LOG_TAG_SETTINGS "SettingsUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_SETTINGS, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_SETTINGS, __VA_ARGS__)

SettingsUI::SettingsUI()
    : textRenderer(nullptr), settingsManager(nullptr),
      visible(false), returnToMenu(false), selectedIndex(0), scrollOffset(0.0f) {
}

SettingsUI::~SettingsUI() {
    cleanup();
}

bool SettingsUI::initialize(TextRenderer* textRenderer, SettingsManager* settingsManager) {
    if (!textRenderer || !settingsManager) {
        LOGD("SettingsUI: null dependencies");
        return false;
    }
    this->textRenderer = textRenderer;
    this->settingsManager = settingsManager;
    visible = false;
    LOGI("SettingsUI initialized");
    return true;
}

void SettingsUI::cleanup() {
    textRenderer = nullptr;
    settingsManager = nullptr;
}

void SettingsUI::update(float deltaTime) {
}

void SettingsUI::render() {
    if (!visible || !textRenderer) return;

    float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    textRenderer->renderText("Settings", 100.0f, 100.0f, 1.0f, white);
    textRenderer->renderText("Return to Menu", 100.0f, 150.0f, 0.75f, white);
}

void SettingsUI::toggle() {
    visible = !visible;
    LOGD("SettingsUI visibility toggled: %d", visible);
}

void SettingsUI::show() { visible = true; }
void SettingsUI::hide() { visible = false; }

void SettingsUI::onTouchEvent(float x, float y) {
    if (!visible) return;

    // Simple touch detection for "Return to Menu" option
    if (x > 80.0f && x < 500.0f && y > 130.0f && y < 180.0f) {
        returnToMenu = true;
        LOGD("SettingsUI: Return to menu requested");
    }
}

void SettingsUI::onKeyEvent(int keyCode, int action) {
}

void SettingsUI::resetReturnFlag() {
    returnToMenu = false;
}