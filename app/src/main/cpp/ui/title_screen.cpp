#include "title_screen.h"
#include "text_renderer.h"
#include "../engine/texture_loader.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>

TitleScreen::TitleScreen()
    : state(TitleScreenState::LOGO_DISPLAY), displayTimer(0.0f),
      selectedIndex(0), gameStarted(false), settingsRequested(false),
      loadGameRequested(false), localizationManager(nullptr), textRenderer(nullptr) {
    LOGD("TitleScreen created");
}

TitleScreen::~TitleScreen() {
    TextureLoader::deleteTexture(bgTexture);
    TextureLoader::deleteTexture(logoTexture);
    TextureLoader::deleteTexture(btnNormalTex);
    TextureLoader::deleteTexture(btnHoverTex);
    TextureLoader::deleteTexture(btnPressedTex);
    TextureLoader::deleteTexture(menuPanelTexture);
    LOGD("TitleScreen destroyed");
}

void TitleScreen::initialize(LocalizationManager* lm, TextRenderer* tr) {
    localizationManager = lm;
    textRenderer = tr;

    // Initialize menu items
    menuItems.clear();
    menuItems.push_back("menu_start");
    menuItems.push_back("menu_load");
    menuItems.push_back("menu_settings");
    menuItems.push_back("menu_quit");

    displayTimer = 0.0f;
    state = TitleScreenState::LOGO_DISPLAY;
    selectedIndex = 0;
    gameStarted = false;

    buildGraphicalMenu();

    // Load textures
    if (!texturesLoaded) {
        bgTexture = TextureLoader::loadTextureFromAsset("textures/ui/main_background.png");
        logoTexture = TextureLoader::loadTextureFromAsset("textures/ui/oblivion_logo.png");
        texturesLoaded = true;
        LOGI("TitleScreen textures loaded: bg=%u logo=%u", bgTexture, logoTexture);
    }

    LOGI("TitleScreen initialized");
}

void TitleScreen::setScreenSize(int w, int h) {
    screenWidth = w;
    screenHeight = h;
    rebuildMenuLayout();
}

void TitleScreen::buildGraphicalMenu() {
    menuPanel = std::make_shared<UIPanel>("TitleMenuPanel");
    menuPanel->initialize();
    menuPanel->setTitle("");
    menuPanel->setTitleBarHeight(0.0f);
    menuPanel->setCloseButtonVisible(false);
    menuPanel->setDraggable(false);
    menuPanel->setBackgroundColor(glm::vec4(0.05f, 0.02f, 0.02f, 0.88f));
    menuPanel->setBorderColor(glm::vec4(0.6f, 0.2f, 0.2f, 0.9f));
    menuPanel->setBorderWidth(2.0f);

    // Load menu panel background texture
    if (menuPanelTexture == 0) {
        menuPanelTexture = TextureLoader::loadTextureFromAsset("textures/ui/main_background.png");
        LOGI("Menu panel texture loaded: %u", menuPanelTexture);
    }
    if (menuPanelTexture != 0) {
        menuPanel->setTexture(menuPanelTexture);
    }

    struct BtnInfo {
        int index;
        std::string labelKey;
    };
    BtnInfo infos[] = {
        {MENU_START,    "menu_start"},
        {MENU_LOAD,     "menu_load"},
        {MENU_SETTINGS, "menu_settings"},
        {MENU_QUIT,     "menu_quit"}
    };

    // Load button textures
    if (btnNormalTex == 0) {
        btnNormalTex = TextureLoader::loadTextureFromAsset("textures/ui/shared_button_long_off.png");
        btnHoverTex = TextureLoader::loadTextureFromAsset("textures/ui/shared_button_long_on.png");
        btnPressedTex = TextureLoader::loadTextureFromAsset("textures/ui/shared_button_long_on.png");
        LOGI("Button textures loaded: normal=%u hover=%u pressed=%u", btnNormalTex, btnHoverTex, btnPressedTex);
    }

    for (const auto& info : infos) {
        auto btn = std::make_shared<UIButton>("MenuBtn" + std::to_string(info.index));
        btn->initialize();
        std::string label = localizationManager ? localizationManager->getString(info.labelKey) : info.labelKey;
        btn->setLabel(label);
        btn->setTextRenderer(textRenderer);
        btn->setSize(300.0f, 60.0f);
        btn->setLabelScale(1.2f);
        btn->setLabelColor(glm::vec3(1.0f, 0.9f, 0.9f));
        btn->setNormalColor(glm::vec4(0.25f, 0.08f, 0.08f, 0.9f));
        btn->setHoverColor(glm::vec4(0.45f, 0.15f, 0.15f, 0.95f));
        btn->setPressedColor(glm::vec4(0.6f, 0.2f, 0.2f, 1.0f));

        // Set button textures
        if (btnNormalTex != 0) {
            btn->setNormalTexture(btnNormalTex);
            btn->setHoverTexture(btnHoverTex);
            btn->setPressedTexture(btnPressedTex);
        }

        int idx = info.index;
        btn->setOnClick([this, idx]() {
            selectedIndex = idx;
            handleMenuSelection();
        });

        menuButtons.push_back(btn);
        menuPanel->addChild(btn);
    }

    rebuildMenuLayout();
}

void TitleScreen::rebuildMenuLayout() {
    if (!menuPanel) return;
    menuPanel->setScreenSize(screenWidth, screenHeight);

    float panelW = 360.0f;
    float panelH = 360.0f;
    float px = (screenWidth - panelW) * 0.5f;
    float py = screenHeight * 0.35f;
    menuPanel->setPosition(px, py);
    menuPanel->setSize(panelW, panelH);

    float btnW = 300.0f;
    float btnH = 60.0f;
    float startY = 30.0f;
    float gap = 12.0f;
    for (size_t i = 0; i < menuButtons.size(); ++i) {
        float bx = (panelW - btnW) * 0.5f;
        float by = startY + static_cast<float>(i) * (btnH + gap);
        menuButtons[i]->setPosition(bx, by);
        menuButtons[i]->setSize(btnW, btnH);
        menuButtons[i]->setScreenSize(screenWidth, screenHeight);
    }
}

void TitleScreen::update(float deltaTime) {
    switch (state) {
        case TitleScreenState::LOGO_DISPLAY: {
            displayTimer += deltaTime;
            if (displayTimer >= LOGO_DISPLAY_DURATION) {
                state = TitleScreenState::MENU;
                LOGI("Logo display complete - transitioning to menu");
            }
            break;
        }

        case TitleScreenState::MENU: {
            updateMenu(deltaTime);
            break;
        }

        case TitleScreenState::LANGUAGE: {
            break;
        }

        case TitleScreenState::TRANSITIONING: {
            break;
        }

        default:
            break;
    }
}

void TitleScreen::render() {
    switch (state) {
        case TitleScreenState::LOGO_DISPLAY: {
            renderLogoDisplay();
            break;
        }

        case TitleScreenState::MENU: {
            renderMenu();
            break;
        }

        case TitleScreenState::LANGUAGE: {
            renderLanguageSelection();
            break;
        }

        case TitleScreenState::TRANSITIONING: {
            renderFadeOut();
            break;
        }

        default:
            break;
    }
}

void TitleScreen::renderLogoDisplay() {
    float fadeAlpha = displayTimer / LOGO_DISPLAY_DURATION;
    fadeAlpha = fadeAlpha > 1.0f ? 1.0f : fadeAlpha;

    // Draw background texture if available
    if (bgTexture != 0) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        UIDrawHelper::drawTexturedQuad(0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight),
                                       bgTexture, glm::vec4(1.0f, 1.0f, 1.0f, fadeAlpha), screenWidth, screenHeight);
    } else {
        float bgBrightness = 0.0f + (0.2f * fadeAlpha);
        glClearColor(bgBrightness, bgBrightness, bgBrightness, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    renderOblivionLogo(fadeAlpha);
    renderPressToStartMessage(fadeAlpha);
}

void TitleScreen::renderMenu() {
    // Draw background texture if available
    if (bgTexture != 0) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        UIDrawHelper::drawTexturedQuad(0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight),
                                       bgTexture, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), screenWidth, screenHeight);
    } else {
        glClearColor(0.12f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    if (menuPanel) {
        menuPanel->render();
    } else {
        renderMenuTitle();
        renderMenuItems();
    }
}

void TitleScreen::renderLanguageSelection() {
    glClearColor(0.1f, 0.02f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    LOGD("Language Selection rendered");
}

void TitleScreen::renderFadeOut() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    LOGD("Fade-out effect");
}

void TitleScreen::renderOblivionLogo(float alpha) {
    if (logoTexture != 0) {
        float logoW = static_cast<float>(screenWidth) * 0.8f;
        float logoH = logoW * 0.25f;  // aspect ratio approx
        float logoX = (static_cast<float>(screenWidth) - logoW) * 0.5f;
        float logoY = static_cast<float>(screenHeight) * 0.15f;
        UIDrawHelper::drawTexturedQuad(logoX, logoY, logoW, logoH,
                                       logoTexture, glm::vec4(1.0f, 1.0f, 1.0f, alpha), screenWidth, screenHeight);
    } else {
        glDisable(GL_DEPTH_TEST);
        float brightness = alpha * 0.8f;
        glClearColor(brightness, brightness, brightness, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    LOGD("  Rendering Oblivion logo (alpha: %.2f)", alpha);
}

void TitleScreen::renderPressToStartMessage(float alpha) {
    LOGD("  Rendering 'Press to Start' message (alpha: %.2f)", alpha);
}

void TitleScreen::renderMenuTitle() {
    LOGD("  Rendering menu title");
}

void TitleScreen::renderMenuItems() {
    for (size_t i = 0; i < menuItems.size(); ++i) {
        bool isSelected = (static_cast<int>(i) == selectedIndex);
        float colorIntensity = isSelected ? 1.0f : 0.5f;
        LOGD("  Menu item[%zu]: %s (selected: %s, color: %.2f)",
             i, menuItems[i].c_str(), isSelected ? "yes" : "no", colorIntensity);
    }
}

void TitleScreen::onTouchEvent(float x, float y) {
    if (state == TitleScreenState::MENU) {
        if (menuPanel && menuPanel->onTouchDown(x, y, 0)) {
            return;
        }
        // Fallback to legacy Y-based detection if graphical menu not ready
        float menuItemHeight = 100.0f;
        float menuStartY = 100.0f;
        int selectedItem = -1;
        for (size_t i = 0; i < menuItems.size(); ++i) {
            float itemY = menuStartY + (i * menuItemHeight);
            if (y >= itemY && y < itemY + menuItemHeight) {
                selectedItem = static_cast<int>(i);
                break;
            }
        }
        if (selectedItem >= 0) {
            selectedIndex = selectedItem;
            handleMenuSelection();
        }
    } else if (state == TitleScreenState::LOGO_DISPLAY) {
        displayTimer = LOGO_DISPLAY_DURATION;
    }
}

void TitleScreen::onKeyPress(int key) {
    if (state == TitleScreenState::MENU) {
        if (key == 19) {
            selectedIndex = (selectedIndex - 1 + menuItems.size()) % menuItems.size();
        } else if (key == 20) {
            selectedIndex = (selectedIndex + 1) % menuItems.size();
        } else if (key == 23 || key == 66) {
            handleMenuSelection();
        }
    }
}

void TitleScreen::transitionToMenu() {
    state = TitleScreenState::MENU;
    displayTimer = 0.0f;
    selectedIndex = 0;
    LOGD("Transitioned to menu state");
}

void TitleScreen::transitionToLanguageMenu() {
    state = TitleScreenState::LANGUAGE;
    selectedIndex = 0;
    LOGD("Transitioned to language selection state");
}

void TitleScreen::updateMenu(float deltaTime) {
    (void)deltaTime;
}

void TitleScreen::handleMenuSelection() {
    if (selectedIndex >= static_cast<int>(menuItems.size())) {
        return;
    }

    const std::string& selected = menuItems[selectedIndex];

    if (selected == "menu_start") {
        state = TitleScreenState::TRANSITIONING;
        gameStarted = true;
        LOGI("Menu selection: Start Game");
    } else if (selected == "menu_load") {
        loadGameRequested = true;
        LOGD("Menu selection: Load Game - requesting save/load UI");
    } else if (selected == "menu_settings") {
        settingsRequested = true;
        LOGD("Menu selection: Settings - requesting settings UI");
    } else if (selected == "menu_quit") {
        LOGD("Menu selection: Quit");
    }
}

void TitleScreen::startGame() {
    gameStarted = true;
    LOGI("Game started - transitioning to main game");
}
