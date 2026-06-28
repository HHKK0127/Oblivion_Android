#include "settings_ui.h"
#include "../engine/renderer.h"
#include "../engine/texture_loader.h"
#include <android/log.h>
#include <sstream>

SettingsUI::SettingsUI()
    : textRenderer(nullptr), settingsManager(nullptr), renderer(nullptr),
      visible(false), returnToMenu(false), selectedIndex(0) {
    LOGD("SettingsUI created");
}

SettingsUI::~SettingsUI() {
    if (panelTexture != 0) {
        TextureLoader::deleteTexture(panelTexture);
    }
    cleanup();
}

bool SettingsUI::initialize(TextRenderer* textRend, SettingsManager* settings, Renderer* rend) {
    if (!textRend || !settings) {
        LOGD("Error: TextRenderer or SettingsManager is null");
        return false;
    }

    textRenderer = textRend;
    settingsManager = settings;
    renderer = rend;

    // メニューアイテムを設定
    menuItems.push_back(SettingItem::DEBUG_MODE);
    menuItems.push_back(SettingItem::LANGUAGE);

    // Add RetroFilter options (Phase 6+)
    if (renderer) {
        menuItems.push_back(SettingItem::PIXELATION);
        menuItems.push_back(SettingItem::SCANLINES);
        menuItems.push_back(SettingItem::COLOR_REDUCTION);
        menuItems.push_back(SettingItem::CRT_DISTORTION);
        menuItems.push_back(SettingItem::FILM_GRAIN);
    }

    menuItems.push_back(SettingItem::BACK);

    updateMenuItems();
    buildGraphicalSettings();

    LOGD("SettingsUI initialized");
    return true;
}

void SettingsUI::buildGraphicalSettings() {
    settingsPanel = std::make_shared<UIPanel>("SettingsPanel");
    settingsPanel->initialize();
    settingsPanel->setTitle("Settings");
    settingsPanel->setTitleBarHeight(44.0f);
    settingsPanel->setTitleBarColor(glm::vec4(0.2f, 0.15f, 0.15f, 0.9f));
    settingsPanel->setCloseButtonVisible(true);
    settingsPanel->setDraggable(true);
    settingsPanel->setBackgroundColor(glm::vec4(0.08f, 0.06f, 0.06f, 0.3f));
    settingsPanel->setBorderWidth(2.0f);

    // Load panel background texture
    if (panelTexture == 0) {
        panelTexture = TextureLoader::loadTextureFromAsset("textures/ui/main_background.png");
        if (panelTexture != 0) {
            settingsPanel->setTexture(panelTexture);
            LOGI("SettingsUI: Panel background texture loaded: %u", panelTexture);
        } else {
            LOGW("SettingsUI: Failed to load panel background texture");
        }
    } else {
        settingsPanel->setTexture(panelTexture);
    }

    settingsPanel->setOnClose([this]() {
        returnToMenu = true;
        visible = false;
    });

    for (const auto& item : menuItems) {
        auto btn = std::make_shared<UIButton>("SettingBtn" + std::to_string(static_cast<int>(item)));
        btn->initialize();
        btn->setTextRenderer(textRenderer);
        btn->setSize(340.0f, 52.0f);
        btn->setLabelScale(1.0f);
        btn->setLabelColor(glm::vec3(1.0f, 1.0f, 1.0f));
        btn->setNormalColor(glm::vec4(0.2f, 0.15f, 0.15f, 0.25f));
        btn->setPressedColor(glm::vec4(0.5f, 0.35f, 0.35f, 0.8f));
        btn->setOnClick([this, item]() {
            selectItem(item);
            updateButtonLabels();
        });

        settingButtons.push_back(btn);
        settingsPanel->addChild(btn);
    }

    refreshLayout();
    updateButtonLabels();
}

void SettingsUI::refreshLayout() {
    if (!settingsPanel) return;
    settingsPanel->setScreenSize(screenWidth, screenHeight);

    float panelW = 420.0f;
    float panelH = static_cast<float>(settingButtons.size()) * 64.0f + 120.0f;
    float px = (screenWidth - panelW) * 0.5f;
    float py = (screenHeight - panelH) * 0.35f;
    settingsPanel->setPosition(px, py);
    settingsPanel->setSize(panelW, panelH);

    glm::vec2 contentPos = settingsPanel->getContentPosition();
    float btnW = 340.0f;
    float btnH = 52.0f;
    float bx = contentPos.x + (settingsPanel->getContentSize().x - btnW) * 0.5f;
    float startY = contentPos.y + 10.0f;

    for (size_t i = 0; i < settingButtons.size(); ++i) {
        float by = startY + static_cast<float>(i) * (btnH + 10.0f);
        settingButtons[i]->setPosition(bx - settingsPanel->getAbsolutePosition().x, by - settingsPanel->getAbsolutePosition().y);
        settingButtons[i]->setSize(btnW, btnH);
        settingButtons[i]->setScreenSize(screenWidth, screenHeight);
    }
}

void SettingsUI::updateButtonLabels() {
    if (!settingsPanel) return;
    for (size_t i = 0; i < menuItems.size() && i < settingButtons.size(); ++i) {
        SettingItem item = menuItems[i];
        std::string label = getSettingLabel(item);
        std::string displayText;

        if (item == SettingItem::DEBUG_MODE) {
            bool debugEnabled = settingsManager->isDebugModeEnabled();
            displayText = label + ": " + (debugEnabled ? "ON" : "OFF");
        } else if (item == SettingItem::LANGUAGE) {
            std::string lang = settingsManager->getLanguage();
            displayText = label + ": " + (lang == "ja" ? "Japanese" : "English");
        } else if (renderer) {
            auto* retroSettings = renderer->getRetroSettings();
            if (item == SettingItem::PIXELATION) {
                displayText = label + ": " + (retroSettings->pixelation_enabled ? "ON" : "OFF");
            } else if (item == SettingItem::SCANLINES) {
                displayText = label + ": " + (retroSettings->scanlines_enabled ? "ON" : "OFF");
            } else if (item == SettingItem::COLOR_REDUCTION) {
                displayText = label + ": " + (retroSettings->color_reduction_enabled ? "ON" : "OFF");
            } else if (item == SettingItem::CRT_DISTORTION) {
                displayText = label + ": " + (retroSettings->crt_distortion_enabled ? "ON" : "OFF");
            } else if (item == SettingItem::FILM_GRAIN) {
                displayText = label + ": " + (retroSettings->grain_enabled ? "ON" : "OFF");
            } else {
                displayText = label;
            }
        } else {
            displayText = label;
        }

        settingButtons[i]->setLabel(displayText);
    }
}

void SettingsUI::setScreenSize(int w, int h) {
    screenWidth = w;
    screenHeight = h;
    refreshLayout();
}

void SettingsUI::toggle() {
    visible = !visible;
    selectedIndex = 0;
    if (visible && settingsPanel) {
        updateButtonLabels();
    }
    LOGD("SettingsUI toggled: %s", visible ? "ON" : "OFF");
}

void SettingsUI::render() {
    if (!visible) {
        return;
    }

    if (settingsPanel) {
        updateButtonLabels();
        settingsPanel->render();
    } else {
        // Fallback legacy rendering
        if (!textRenderer) return;
        glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
        glClear(GL_COLOR_BUFFER_BIT);
        glm::vec3 titleColor(1.0f, 1.0f, 0.0f);
        textRenderer->renderText("SETTINGS", 400.0f, 100.0f, titleColor, 1.5f);
    }
}

void SettingsUI::onTouchEvent(float x, float y) {
    if (!visible) {
        return;
    }

    if (settingsPanel && settingsPanel->onTouchDown(x, y, 0)) {
        return;
    }

    // Fallback legacy touch detection
    float yPos = 250.0f;
    for (size_t i = 0; i < menuItems.size(); i++) {
        if (y >= yPos && y < yPos + 60.0f) {
            selectedIndex = static_cast<int>(i);
            selectItem(menuItems[i]);
            break;
        }
        yPos += 80.0f;
    }
}

void SettingsUI::cleanup() {
    textRenderer = nullptr;
    settingsManager = nullptr;
    settingsPanel = nullptr;
    settingButtons.clear();
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
        case SettingItem::PIXELATION:
            return "[Opt] Pixelation";
        case SettingItem::SCANLINES:
            return "[Opt] Scanlines";
        case SettingItem::COLOR_REDUCTION:
            return "[Opt] Color Reduction";
        case SettingItem::CRT_DISTORTION:
            return "[Opt] CRT Distortion";
        case SettingItem::FILM_GRAIN:
            return "[Opt] Film Grain";
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
        // RetroFilter options (Phase 6+)
        case SettingItem::PIXELATION: {
            if (renderer) {
                auto& retroSettings = renderer->getRetroSettingsRef();
                retroSettings.pixelation_enabled = !retroSettings.pixelation_enabled;
                LOGD("Pixelation toggled to: %s", retroSettings.pixelation_enabled ? "ON" : "OFF");
            }
            break;
        }
        case SettingItem::SCANLINES: {
            if (renderer) {
                auto& retroSettings = renderer->getRetroSettingsRef();
                retroSettings.scanlines_enabled = !retroSettings.scanlines_enabled;
                LOGD("Scanlines toggled to: %s", retroSettings.scanlines_enabled ? "ON" : "OFF");
            }
            break;
        }
        case SettingItem::COLOR_REDUCTION: {
            if (renderer) {
                auto& retroSettings = renderer->getRetroSettingsRef();
                retroSettings.color_reduction_enabled = !retroSettings.color_reduction_enabled;
                LOGD("Color reduction toggled to: %s", retroSettings.color_reduction_enabled ? "ON" : "OFF");
            }
            break;
        }
        case SettingItem::CRT_DISTORTION: {
            if (renderer) {
                auto& retroSettings = renderer->getRetroSettingsRef();
                retroSettings.crt_distortion_enabled = !retroSettings.crt_distortion_enabled;
                LOGD("CRT distortion toggled to: %s", retroSettings.crt_distortion_enabled ? "ON" : "OFF");
            }
            break;
        }
        case SettingItem::FILM_GRAIN: {
            if (renderer) {
                auto& retroSettings = renderer->getRetroSettingsRef();
                retroSettings.grain_enabled = !retroSettings.grain_enabled;
                LOGD("Film grain toggled to: %s", retroSettings.grain_enabled ? "ON" : "OFF");
            }
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
