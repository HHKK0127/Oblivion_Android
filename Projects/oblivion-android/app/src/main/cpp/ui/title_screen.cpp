#include "title_screen.h"

TitleScreen::TitleScreen()
    : state(TitleScreenState::LOGO_DISPLAY), displayTimer(0.0f),
      selectedIndex(0), gameStarted(false), settingsRequested(false),
      bgmPlayRequested(false), bgmStopRequested(false),
      textRenderer(nullptr), localizationManager(nullptr), assetManager(nullptr), imagesLoaded(false) {
    LOGD("TitleScreen created");
}

TitleScreen::~TitleScreen() {
    LOGD("TitleScreen destroyed");
}

void TitleScreen::initialize(LocalizationManager* lm) {
    localizationManager = lm;

    // Initialize menu items
    menuItems.clear();
    menuItems.push_back("menu_start");
    menuItems.push_back("menu_settings");
    menuItems.push_back("menu_quit");

    displayTimer = 0.0f;
    state = TitleScreenState::LOGO_DISPLAY;
    selectedIndex = 0;
    gameStarted = false;

    // Start BGM when title screen initializes
    playTitleBGM();

    LOGI("TitleScreen initialized");
}

void TitleScreen::update(float deltaTime) {
    switch (state) {
        case TitleScreenState::LOGO_DISPLAY: {
            displayTimer += deltaTime;
            if (displayTimer >= LOGO_DISPLAY_DURATION) {
                state = TitleScreenState::MENU;
                displayTimer = 0.0f;
                playTitleBGM();
                LOGI("Logo display complete - transitioning to menu");
            }
            break;
        }

        case TitleScreenState::MENU: {
            updateMenu(deltaTime);
            break;
        }

        case TitleScreenState::LANGUAGE: {
            // Language menu update
            break;
        }

        case TitleScreenState::TRANSITIONING: {
            // Game started - nothing to do
            break;
        }

        default:
            break;
    }
}

void TitleScreen::setTextRenderer(TextRenderer* tr) {
    textRenderer = tr;
}

void TitleScreen::setAssetManager(AAssetManager* mgr) {
    assetManager = mgr;
}

void TitleScreen::loadImages() {
    if (imagesLoaded || !assetManager) return;

    backgroundImage = std::make_unique<ImageRenderer>();
    if (!backgroundImage->loadFromAssetManager(assetManager, "textures/main_background.png")) {
        LOGW("Failed to load background image from assets");
    }

    logoImage = std::make_unique<ImageRenderer>();
    if (!logoImage->loadFromAssetManager(assetManager, "textures/oblivion_logo.png")) {
        LOGW("Failed to load logo image from assets");
    }

    imagesLoaded = true;
}

void TitleScreen::playTitleBGM() {
    LOGI("Requesting title BGM");
    bgmPlayRequested = true;
    bgmStopRequested = false;
    bgmPath = "music/tes4title.mp3";
}

void TitleScreen::stopTitleBGM() {
    LOGI("Requesting stop BGM");
    bgmStopRequested = true;
    bgmPlayRequested = false;
}

bool TitleScreen::isBGMPlayRequested(std::string& outPath) {
    if (bgmPlayRequested) {
        outPath = bgmPath;
        bgmPlayRequested = false;
        return true;
    }
    return false;
}

bool TitleScreen::isBGMStopRequested() {
    if (bgmStopRequested) {
        bgmStopRequested = false;
        return true;
    }
    return false;
}

void TitleScreen::resetBGMRequests() {
    bgmPlayRequested = false;
    bgmStopRequested = false;
}

void TitleScreen::render(TextRenderer* textRenderer) {
    if (!textRenderer) return;

    // Load images on first render if available
    if (!imagesLoaded && assetManager) {
        loadImages();
    }

    switch (state) {
        case TitleScreenState::LOGO_DISPLAY: {
            // Draw background image if loaded
            if (backgroundImage && backgroundImage->isLoaded()) {
                backgroundImage->drawFullscreen();
            }

            // Draw logo image if loaded, otherwise fallback text
            if (logoImage && logoImage->isLoaded()) {
                float lw = logoImage->getWidth();
                float lh = logoImage->getHeight();
                float scale = (textRenderer->getScreenWidth() * 0.7f) / lw;
                float logoW = lw * scale;
                float logoH = lh * scale;
                float lx = (textRenderer->getScreenWidth() - logoW) * 0.5f;
                float ly = textRenderer->getScreenHeight() * 0.25f;
                logoImage->draw(lx, ly, logoW, logoH);
            } else {
                const float color[4] = {1.0f, 0.85f, 0.5f, 1.0f}; // Gold
                float tw = textRenderer->getTextWidth("OBLIVION", 2.0f);
                float tx = (textRenderer->getScreenWidth() - tw) * 0.5f;
                float ty = textRenderer->getScreenHeight() * 0.35f;
                textRenderer->renderText("OBLIVION", tx, ty, 2.0f, color);
            }
            break;
        }

        case TitleScreenState::MENU: {
            // Draw background image if loaded
            if (backgroundImage && backgroundImage->isLoaded()) {
                backgroundImage->drawFullscreen();
            }

            // Draw logo at top
            if (logoImage && logoImage->isLoaded()) {
                float lw = logoImage->getWidth();
                float lh = logoImage->getHeight();
                float scale = (textRenderer->getScreenWidth() * 0.5f) / lw;
                float logoW = lw * scale;
                float logoH = lh * scale;
                float lx = (textRenderer->getScreenWidth() - logoW) * 0.5f;
                float ly = textRenderer->getScreenHeight() * 0.15f;
                logoImage->draw(lx, ly, logoW, logoH);
            }

            // Draw menu items centered on screen
            float scale = 1.0f;
            float itemHeight = textRenderer->getTextHeight(scale) * 1.5f;
            float totalHeight = menuItems.size() * itemHeight;
            float startY = (textRenderer->getScreenHeight() - totalHeight) * 0.5f + itemHeight * 1.5f;

            for (size_t i = 0; i < menuItems.size(); ++i) {
                bool isSelected = (i == selectedIndex);
                std::string displayText = localizationManager ?
                    localizationManager->getString(menuItems[i]) :
                    menuItems[i];

                if (isSelected) {
                    displayText = "> " + displayText;
                    const float selectedColor[4] = {1.0f, 1.0f, 0.3f, 1.0f}; // Yellow
                    float tw = textRenderer->getTextWidth(displayText, scale);
                    float tx = (textRenderer->getScreenWidth() - tw) * 0.5f;
                    textRenderer->renderText(displayText, tx, startY + i * itemHeight, scale, selectedColor);
                } else {
                    const float normalColor[4] = {0.9f, 0.9f, 0.9f, 1.0f}; // White-ish
                    float tw = textRenderer->getTextWidth(displayText, scale);
                    float tx = (textRenderer->getScreenWidth() - tw) * 0.5f;
                    textRenderer->renderText(displayText, tx, startY + i * itemHeight, scale, normalColor);
                }
            }
            break;
        }

        case TitleScreenState::LANGUAGE: {
            const float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            textRenderer->renderText("Select Language", 400.0f, 400.0f, 1.5f, color);
            break;
        }

        case TitleScreenState::TRANSITIONING: {
            // Fade out / nothing to render
            break;
        }

        default:
            break;
    }
}

void TitleScreen::onTouchEvent(float x, float y) {
    if (state == TitleScreenState::MENU) {
        // Simple menu item detection based on Y coordinate
        float menuItemHeight = 100.0f;
        float menuStartY = 100.0f;

        int selectedItem = -1;
        for (size_t i = 0; i < menuItems.size(); ++i) {
            float itemY = menuStartY + (i * menuItemHeight);
            if (y >= itemY && y < itemY + menuItemHeight) {
                selectedItem = i;
                break;
            }
        }

        if (selectedItem >= 0) {
            selectedIndex = selectedItem;
            handleMenuSelection();
        }
    } else if (state == TitleScreenState::LOGO_DISPLAY) {
        // Skip logo by tapping
        displayTimer = LOGO_DISPLAY_DURATION;
    }
}

void TitleScreen::onKeyPress(int key) {
    if (state == TitleScreenState::MENU) {
        // Arrow key navigation (simplified)
        if (key == 19) {  // Up arrow
            selectedIndex = (selectedIndex - 1 + menuItems.size()) % menuItems.size();
        } else if (key == 20) {  // Down arrow
            selectedIndex = (selectedIndex + 1) % menuItems.size();
        } else if (key == 23 || key == 66) {  // Select (ENTER or TAP)
            handleMenuSelection();
        }
    }
}

void TitleScreen::transitionToMenu() {
    state = TitleScreenState::MENU;
    displayTimer = 0.0f;
    selectedIndex = 0;  // Highlight "Start" by default
    LOGD("Transitioned to menu state");
}

void TitleScreen::transitionToLanguageMenu() {
    state = TitleScreenState::LANGUAGE;
    selectedIndex = 0;
    LOGD("Transitioned to language selection state");
}

void TitleScreen::updateMenu(float deltaTime) {
    // Menu update logic (animations, etc.)
}

void TitleScreen::handleMenuSelection() {
    if (selectedIndex >= menuItems.size()) {
        return;
    }

    const std::string& selected = menuItems[selectedIndex];

    if (selected == "menu_start") {
        state = TitleScreenState::TRANSITIONING;
        gameStarted = true;
        LOGI("Menu selection: Start Game");
    } else if (selected == "menu_settings") {
        settingsRequested = true;
        LOGD("Menu selection: Settings (requested)");
    } else if (selected == "menu_quit") {
        LOGD("Menu selection: Quit");
        // Would call Android activity finish() in real implementation
    }
}

void TitleScreen::startGame() {
    gameStarted = true;
    LOGI("Game started - transitioning to main game");
}
