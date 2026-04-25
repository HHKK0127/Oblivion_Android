#include "settings_ui.h"
#include <android/log.h>
#include <sstream>

SettingsUI::SettingsUI()
    : textRenderer(nullptr), settingsManager(nullptr),
      visible(false), returnToMenu(false), selectedIndex(0) {
    LOGD("SettingsUI created");
}

SettingsUI::~SettingsUI() {
    cleanup();
}

bool SettingsUI::initialize(TextRenderer* renderer, SettingsManager* settings) {
    if (!renderer || !settings) {
        LOGD("Error: TextRenderer or SettingsManager is null");
        return false;
    }

    textRenderer = renderer;
    settingsManager = settings;

    // メニューアイテムを設定
    menuItems.push_back(SettingItem::DEBUG_MODE);
    menuItems.push_back(SettingItem::LANGUAGE);
    menuItems.push_back(SettingItem::BACK);

    updateMenuItems();

    LOGD("SettingsUI initialized");
    return true;
}

void SettingsUI::toggle() {
    visible = !visible;
    selectedIndex = 0;
    LOGD("SettingsUI toggled: %s", visible ? "ON" : "OFF");
}

void SettingsUI::render() {
    if (!visible || !textRenderer) {
        return;
    }

    // 背景を暗くする（半透明効果）
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);

    // タイトル
    glm::vec3 titleColor(1.0f, 1.0f, 0.0f);  // 黄色
    textRenderer->renderText("SETTINGS", 400.0f, 100.0f, titleColor, 1.5f);

    // メニューアイテムを描画
    glm::vec3 normalColor(1.0f, 1.0f, 1.0f);  // 白
    glm::vec3 selectedColor(1.0f, 0.0f, 0.0f);  // 赤

    float yPos = 250.0f;
    for (int i = 0; i < menuItems.size(); i++) {
        std::string label = getSettingLabel(menuItems[i]);
        glm::vec3 color = (i == selectedIndex) ? selectedColor : normalColor;

        std::string displayText;
        if (menuItems[i] == SettingItem::DEBUG_MODE) {
            bool debugEnabled = settingsManager->isDebugModeEnabled();
            displayText = label + ": " + (debugEnabled ? "ON" : "OFF");
        } else if (menuItems[i] == SettingItem::LANGUAGE) {
            std::string lang = settingsManager->getLanguage();
            displayText = label + ": " + (lang == "ja" ? "Japanese" : "English");
        } else {
            displayText = label;
        }

        // 選択されている場合は「> 」を追加
        if (i == selectedIndex) {
            displayText = "> " + displayText;
        }

        textRenderer->renderText(displayText, 300.0f, yPos, color, 1.2f);
        yPos += 80.0f;
    }

    // ヘルプテキスト
    glm::vec3 helpColor(0.7f, 0.7f, 0.7f);
    textRenderer->renderText("Tap item to select", 350.0f, 600.0f, helpColor, 0.8f);
}

void SettingsUI::onTouchEvent(float x, float y) {
    if (!visible) {
        return;
    }

    // タッチ位置からメニューアイテムを判定
    float yPos = 250.0f;
    for (int i = 0; i < menuItems.size(); i++) {
        if (y >= yPos && y < yPos + 60.0f) {
            selectedIndex = i;
            selectItem(menuItems[i]);
            break;
        }
        yPos += 80.0f;
    }
}

void SettingsUI::cleanup() {
    textRenderer = nullptr;
    settingsManager = nullptr;
    LOGD("SettingsUI cleaned up");
}

void SettingsUI::updateMenuItems() {
    renderItems.clear();

    float yPos = 250.0f;
    for (const auto& item : menuItems) {
        MenuItem mi;
        mi.label = getSettingLabel(item);
        mi.x = 300.0f;
        mi.y = yPos;
        mi.width = 400.0f;
        mi.height = 60.0f;

        renderItems.push_back(mi);
        yPos += 80.0f;
    }
}

std::string SettingsUI::getSettingLabel(SettingItem item) const {
    switch (item) {
        case SettingItem::DEBUG_MODE:
            return "Debug Mode";
        case SettingItem::LANGUAGE:
            return "Language";
        case SettingItem::BACK:
            return "Back to Menu";
        default:
            return "Unknown";
    }
}

void SettingsUI::selectItem(SettingItem item) {
    LOGD("SettingsUI item selected: %d", static_cast<int>(item));

    switch (item) {
        case SettingItem::DEBUG_MODE: {
            // デバッグモードをトグル
            bool current = settingsManager->isDebugModeEnabled();
            settingsManager->setDebugMode(!current);
            LOGD("Debug mode toggled to: %s", !current ? "ON" : "OFF");
            break;
        }
        case SettingItem::LANGUAGE: {
            // 言語をトグル
            std::string current = settingsManager->getLanguage();
            std::string newLang = (current == "ja") ? "en" : "ja";
            settingsManager->setLanguage(newLang);
            LOGD("Language changed to: %s", newLang.c_str());
            break;
        }
        case SettingItem::BACK: {
            // メニューに戻る
            returnToMenu = true;
            visible = false;
            LOGD("Returning to main menu");
            break;
        }
    }
}
