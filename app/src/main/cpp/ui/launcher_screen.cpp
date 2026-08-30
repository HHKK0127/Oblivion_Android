#include "launcher_screen.h"
#include "text_renderer.h"
#include "../engine/texture_loader.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cmath>

LauncherScreen::LauncherScreen()
    : state(LauncherState::MAIN),
      localizationManager(nullptr), textRenderer(nullptr), selectedIndex(0) {
    LOGD("LauncherScreen created (Oblivion Authentic Launcher)");
}

LauncherScreen::~LauncherScreen() {
    TextureLoader::deleteTexture(bgTexture);
    TextureLoader::deleteTexture(logoTexture);
    TextureLoader::deleteTexture(buttonBgTex);
    TextureLoader::deleteTexture(buttonHoverTex);
    LOGD("LauncherScreen destroyed");
}

void LauncherScreen::initialize(LocalizationManager* lm, TextRenderer* tr) {
    localizationManager = lm;
    textRenderer = tr;

    selectedIndex = 0;
    fadeInAlpha = 0.0f;
    displayTimer = 0.0f;
    state = LauncherState::MAIN;

    buildMainMenu();

    if (!texturesLoaded) {
        // Original: dark stone/metal background
        bgTexture = TextureLoader::loadTextureFromAsset("textures/ui/launcher_bg.png");
        // Right side Oblivion logo (large)
        logoTexture = TextureLoader::loadTextureFromAsset("textures/ui/oblivion_logo_large.png");
        // Button background (stone style)
        buttonBgTex = TextureLoader::loadTextureFromAsset("textures/ui/btn_stone.png");
        // Hover state (bright stone style)
        buttonHoverTex = TextureLoader::loadTextureFromAsset("textures/ui/btn_stone_hover.png");

        texturesLoaded = true;
        LOGI("Launcher textures loaded");
    }

    LOGI("LauncherScreen initialized");
}

void LauncherScreen::setScreenSize(int w, int h) {
    screenWidth = w;
    screenHeight = h;
    rebuildLayout();
}

void LauncherScreen::buildMainMenu() {
    mainPanel = std::make_shared<UIPanel>("LauncherMainPanel");
    mainPanel->initialize();
    mainPanel->setTitle("");
    mainPanel->setTitleBarHeight(0.0f);
    mainPanel->setCloseButtonVisible(false);
    mainPanel->setDraggable(false);
    mainPanel->setBackgroundColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    mainPanel->setBorderColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    mainPanel->setBorderWidth(0.0f);

    struct BtnInfo { int index; std::string labelKey; };
    BtnInfo infos[] = {
        {BTN_PLAY,       "launcher_play"},
        {BTN_OPTIONS,    "launcher_options"},
        {BTN_DATA_FILES, "launcher_data_files"},
        {BTN_SUPPORT,    "launcher_support"},
        {BTN_EXIT,       "launcher_exit"}
    };

    for (const auto& info : infos) {
        auto btn = std::make_shared<UIButton>("LauncherBtn" + std::to_string(info.index));
        btn->initialize();
        std::string label = localizationManager ? localizationManager->getString(info.labelKey) : info.labelKey;
        btn->setLabel(label);
        btn->setTextRenderer(textRenderer);

        // Original: large buttons (TV/console friendly)
        btn->setSize(380.0f, 72.0f);
        btn->setLabelScale(1.6f);

        // Label color: dark gold (unselected)
        btn->setLabelColor(COLOR_GOLD_DIM);

        // Button background: stone texture or dark semi-transparent
        if (buttonBgTex != 0) {
            btn->setNormalTexture(buttonBgTex);
            btn->setHoverTexture(buttonHoverTex);
        } else {
            btn->setNormalColor(glm::vec4(0.12f, 0.10f, 0.08f, 0.85f));
            btn->setHoverColor(glm::vec4(0.20f, 0.17f, 0.12f, 0.90f));
            btn->setPressedColor(glm::vec4(0.08f, 0.07f, 0.05f, 0.95f));
        }

        int idx = info.index;
        btn->setOnClick([this, idx]() {
            selectedIndex = idx;
            handleSelection();
        });

        menuButtons.push_back(btn);
        mainPanel->addChild(btn);
    }

    rebuildLayout();
}

void LauncherScreen::rebuildLayout() {
    if (!mainPanel) return;
    mainPanel->setScreenSize(screenWidth, screenHeight);

    // Original: panel on left, logo on right
    float panelW = 450.0f;
    float panelH = 520.0f;
    float px = screenWidth * 0.06f;   // Left-aligned
    float py = screenHeight * 0.18f;  // Slightly upper
    mainPanel->setPosition(px, py);
    mainPanel->setSize(panelW, panelH);

    float btnW = 400.0f;
    float btnH = 72.0f;
    float startY = 20.0f;
    float gap = 18.0f;

    for (size_t i = 0; i < menuButtons.size(); ++i) {
        float bx = 25.0f;
        float by = startY + static_cast<float>(i) * (btnH + gap);
        menuButtons[i]->setPosition(bx, by);
        menuButtons[i]->setSize(btnW, btnH);
        menuButtons[i]->setScreenSize(screenWidth, screenHeight);
    }
}

void LauncherScreen::update(float deltaTime) {
    displayTimer += deltaTime;
    glowPhase += deltaTime * 2.2f;

    // Fade in
    if (fadeInAlpha < 1.0f) {
        fadeInAlpha += deltaTime * 1.5f;
        if (fadeInAlpha > 1.0f) fadeInAlpha = 1.0f;
    }

    // Selection highlight animation
    for (size_t i = 0; i < menuButtons.size(); ++i) {
        bool isSelected = (static_cast<int>(i) == selectedIndex);
        if (isSelected) {
            float glow = 0.5f + 0.5f * sin(glowPhase);
            glm::vec3 c(COLOR_GOLD_DIM.x + (COLOR_GOLD_BRIGHT.x - COLOR_GOLD_DIM.x) * glow,
                        COLOR_GOLD_DIM.y + (COLOR_GOLD_BRIGHT.y - COLOR_GOLD_DIM.y) * glow,
                        COLOR_GOLD_DIM.z + (COLOR_GOLD_BRIGHT.z - COLOR_GOLD_DIM.z) * glow);
            menuButtons[i]->setLabelColor(c);
        } else {
            glm::vec3 dimmed(COLOR_GOLD_DIM.x * 0.6f, COLOR_GOLD_DIM.y * 0.6f, COLOR_GOLD_DIM.z * 0.6f);
            menuButtons[i]->setLabelColor(dimmed);
        }
    }
}

void LauncherScreen::render() {
    switch (state) {
        case LauncherState::MAIN:
            renderMain();
            break;
        case LauncherState::OPTIONS:
            renderOptions();
            break;
        case LauncherState::DATA_FILES:
            renderDataFiles();
            break;
        case LauncherState::SUPPORT:
            renderSupport();
            break;
        case LauncherState::TRANSITIONING:
            renderFadeToGame();
            break;
    }
}

// ============================================================================
// Main launcher screen (original: left buttons / right logo)
// ============================================================================
void LauncherScreen::renderMain() {
    glClearColor(COLOR_DARK_BG.x, COLOR_DARK_BG.y, COLOR_DARK_BG.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    renderBackground();
    renderLogo();

    if (mainPanel) {
        mainPanel->render();
    }

    // Fade in overlay
    if (fadeInAlpha < 1.0f) {
        UIDrawHelper::drawColoredQuad(
            0.0f, 0.0f,
            static_cast<float>(screenWidth), static_cast<float>(screenHeight),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f - fadeInAlpha),
            screenWidth, screenHeight);
    }
}

// ============================================================================
// Background (dark stone/metal)
// ============================================================================
void LauncherScreen::renderBackground() {
    if (bgTexture != 0) {
        UIDrawHelper::drawTexturedQuad(
            0.0f, 0.0f,
            static_cast<float>(screenWidth), static_cast<float>(screenHeight),
            bgTexture, glm::vec4(1.0f, 1.0f, 1.0f, 0.4f),
            screenWidth, screenHeight);
    } else {
        // Fallback: dark gradient
        UIDrawHelper::drawColoredQuad(
            0.0f, 0.0f,
            static_cast<float>(screenWidth), static_cast<float>(screenHeight),
            glm::vec4(0.06f, 0.05f, 0.04f, 1.0f),
            screenWidth, screenHeight);
    }

    // Original: thin panel background on left side (button area)
    float panelX = screenWidth * 0.04f;
    float panelY = screenHeight * 0.12f;
    float panelW = 480.0f;
    float panelH = 560.0f;
    UIDrawHelper::drawColoredQuad(
        panelX, panelY, panelW, panelH,
        glm::vec4(0.04f, 0.03f, 0.02f, 0.65f),
        screenWidth, screenHeight);

    // Border (thin gold)
    UIDrawHelper::drawBorder(
        panelX, panelY, panelW, panelH, 2.0f,
        glm::vec4(COLOR_GOLD_DIM.x, COLOR_GOLD_DIM.y, COLOR_GOLD_DIM.z, 0.3f),
        screenWidth, screenHeight);
}

// ============================================================================
// Right side logo (large)
// ============================================================================
void LauncherScreen::renderLogo() {
    if (logoTexture == 0) return;

    // Right side center placement
    float scaleFactor = (screenWidth > screenHeight) ? 0.45f : 0.75f;
    float logoW = static_cast<float>(screenWidth) * scaleFactor;
    float logoH = logoW * 0.22f;
    float logoX = static_cast<float>(screenWidth) * 0.58f;
    float logoY = (static_cast<float>(screenHeight) - logoH) * 0.45f;

    UIDrawHelper::drawTexturedQuad(
        logoX, logoY, logoW, logoH,
        logoTexture, glm::vec4(1.0f, 1.0f, 1.0f, fadeInAlpha),
        screenWidth, screenHeight);
}

// ============================================================================
// Options screen (quality settings etc.)
// ============================================================================
void LauncherScreen::renderOptions() {
    glClearColor(COLOR_DARK_BG.x, COLOR_DARK_BG.y, COLOR_DARK_BG.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    renderBackground();

    if (!optionsPanel) {
        optionsPanel = std::make_shared<UIPanel>("OptionsPanel");
        optionsPanel->initialize();
        optionsPanel->setTitle(localizationManager ? localizationManager->getString("launcher_options") : "Options");
        optionsPanel->setScreenSize(screenWidth, screenHeight);
        optionsPanel->setPosition(screenWidth * 0.15f, screenHeight * 0.1f);
        optionsPanel->setSize(screenWidth * 0.7f, screenHeight * 0.8f);
        optionsPanel->setBackgroundColor(glm::vec4(0.08f, 0.07f, 0.05f, 0.92f));
        optionsPanel->setBorderColor(glm::vec4(0.65f, 0.55f, 0.30f, 0.5f));
        optionsPanel->setBorderWidth(2.0f);

        // Back button
        auto backBtn = std::make_shared<UIButton>("OptionsBackBtn");
        backBtn->initialize();
        backBtn->setLabel("Back");
        backBtn->setTextRenderer(textRenderer);
        backBtn->setSize(160.0f, 50.0f);
        backBtn->setLabelScale(1.2f);
        backBtn->setLabelColor(COLOR_GOLD_DIM);
        backBtn->setNormalColor(glm::vec4(0.15f, 0.13f, 0.10f, 0.9f));
        backBtn->setHoverColor(glm::vec4(0.25f, 0.22f, 0.15f, 0.95f));
        backBtn->setPosition(optionsPanel->getSize().x - 180.0f, optionsPanel->getSize().y - 70.0f);
        backBtn->setOnClick([this]() {
            state = LauncherState::MAIN;
        });
        optionsPanel->addChild(backBtn);
    }

    if (optionsPanel) optionsPanel->render();
}

// ============================================================================
// Data Files screen (plugin management)
// ============================================================================
void LauncherScreen::renderDataFiles() {
    glClearColor(COLOR_DARK_BG.x, COLOR_DARK_BG.y, COLOR_DARK_BG.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    renderBackground();

    if (!dataFilesPanel) {
        dataFilesPanel = std::make_shared<UIPanel>("DataFilesPanel");
        dataFilesPanel->initialize();
        dataFilesPanel->setTitle(localizationManager ? localizationManager->getString("launcher_data_files") : "Data Files");
        dataFilesPanel->setScreenSize(screenWidth, screenHeight);
        dataFilesPanel->setPosition(screenWidth * 0.1f, screenHeight * 0.1f);
        dataFilesPanel->setSize(screenWidth * 0.8f, screenHeight * 0.8f);
        dataFilesPanel->setBackgroundColor(glm::vec4(0.08f, 0.07f, 0.05f, 0.92f));
        dataFilesPanel->setBorderColor(glm::vec4(0.65f, 0.55f, 0.30f, 0.5f));
        dataFilesPanel->setBorderWidth(2.0f);

        // Back button
        auto backBtn = std::make_shared<UIButton>("DataFilesBackBtn");
        backBtn->initialize();
        backBtn->setLabel("Back");
        backBtn->setTextRenderer(textRenderer);
        backBtn->setSize(160.0f, 50.0f);
        backBtn->setLabelScale(1.2f);
        backBtn->setLabelColor(COLOR_GOLD_DIM);
        backBtn->setNormalColor(glm::vec4(0.15f, 0.13f, 0.10f, 0.9f));
        backBtn->setHoverColor(glm::vec4(0.25f, 0.22f, 0.15f, 0.95f));
        backBtn->setPosition(dataFilesPanel->getSize().x - 180.0f, dataFilesPanel->getSize().y - 70.0f);
        backBtn->setOnClick([this]() {
            state = LauncherState::MAIN;
        });
        dataFilesPanel->addChild(backBtn);
    }

    if (dataFilesPanel) dataFilesPanel->render();
}

// ============================================================================
// Support screen
// ============================================================================
void LauncherScreen::renderSupport() {
    glClearColor(COLOR_DARK_BG.x, COLOR_DARK_BG.y, COLOR_DARK_BG.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    renderBackground();

    // TODO: Support info rendering (text-based)
}

// ============================================================================
// Game launch fade
// ============================================================================
void LauncherScreen::renderFadeToGame() {
    float fade = displayTimer * 1.5f;
    if (fade > 1.0f) fade = 1.0f;
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

// ============================================================================
// Input handling
// ============================================================================
void LauncherScreen::onTouchEvent(float x, float y) {
    if (state == LauncherState::MAIN) {
        if (mainPanel && mainPanel->onTouchDown(x, y, 0)) return;

        // Fallback
        float menuTop = screenHeight * 0.18f;
        float menuLeft = screenWidth * 0.06f;
        float menuWidth = 450.0f;
        float itemH = 90.0f;
        int numButtons = static_cast<int>(menuButtons.size());
        for (int i = 0; i < numButtons; ++i) {
            float itemY = menuTop + static_cast<float>(i) * itemH;
            if (x >= menuLeft && x <= menuLeft + menuWidth &&
                y >= itemY && y < itemY + itemH) {
                selectedIndex = i;
                handleSelection();
                return;
            }
        }
    } else if (state == LauncherState::OPTIONS) {
        if (optionsPanel && optionsPanel->onTouchDown(x, y, 0)) return;
    } else if (state == LauncherState::DATA_FILES) {
        if (dataFilesPanel && dataFilesPanel->onTouchDown(x, y, 0)) return;
    }
}

void LauncherScreen::onKeyPress(int key) {
    if (state == LauncherState::MAIN) {
        int numButtons = static_cast<int>(menuButtons.size());
        if (numButtons == 0) numButtons = 5; // Safety fallback

        switch (key) {
            case 19: // UP
                selectedIndex = (selectedIndex - 1 + numButtons) % numButtons;
                break;
            case 20: // DOWN
                selectedIndex = (selectedIndex + 1) % numButtons;
                break;
            case 23: // ENTER
            case 66: // DPAD_CENTER
                handleSelection();
                break;
        }
    } else {
        // Sub-screens: ESC/Back to return
        if (key == 4) { // BACK
            state = LauncherState::MAIN;
        }
    }
}

void LauncherScreen::handleSelection() {
    switch (selectedIndex) {
        case BTN_PLAY: {
            state = LauncherState::TRANSITIONING;
            displayTimer = 0.0f;
            if (onPlayCallback) onPlayCallback();
            LOGI("Launcher: Play selected");
            break;
        }
        case BTN_OPTIONS: {
            state = LauncherState::OPTIONS;
            LOGI("Launcher: Options selected");
            break;
        }
        case BTN_DATA_FILES: {
            state = LauncherState::DATA_FILES;
            LOGI("Launcher: Data Files selected");
            break;
        }
        case BTN_SUPPORT: {
            state = LauncherState::SUPPORT;
            LOGI("Launcher: Support selected");
            break;
        }
        case BTN_EXIT: {
            if (onExitCallback) onExitCallback();
            LOGI("Launcher: Exit selected");
            break;
        }
    }
}
