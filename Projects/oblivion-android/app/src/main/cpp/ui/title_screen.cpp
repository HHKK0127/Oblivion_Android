#include "title_screen.h"

TitleScreen::TitleScreen()
    : state(TitleScreenState::LOGO_DISPLAY), displayTimer(0.0f),
      selectedIndex(0), gameStarted(false), settingsRequested(false),
      localizationManager(nullptr) {
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

    LOGI("TitleScreen initialized");
}

void TitleScreen::update(float deltaTime) {
    switch (state) {
        case TitleScreenState::LOGO_DISPLAY: {
            displayTimer += deltaTime;
            if (displayTimer >= LOGO_DISPLAY_DURATION) {
                // Auto-transition to game (skip menu for now)
                state = TitleScreenState::TRANSITIONING;
                startGame();
                LOGI("Logo display complete - starting game automatically");
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

void TitleScreen::render() {
    // Placeholder for UI rendering
    // In a real implementation, would use OpenGL text rendering
    switch (state) {
        case TitleScreenState::LOGO_DISPLAY: {
            LOGD("Rendering logo (%.1f/%.1f)", displayTimer, LOGO_DISPLAY_DURATION);
            break;
        }

        case TitleScreenState::MENU: {
            LOGD("Rendering menu, selected: %d", selectedIndex);
            for (size_t i = 0; i < menuItems.size(); ++i) {
                bool isSelected = (i == selectedIndex);
                const char* marker = isSelected ? "> " : "  ";
                std::string displayText = localizationManager ?
                    localizationManager->getString(menuItems[i]) :
                    menuItems[i];
                LOGD("%s%s", marker, displayText.c_str());
            }
            break;
        }

        case TitleScreenState::LANGUAGE: {
            LOGD("Rendering language selection");
            break;
        }

        case TitleScreenState::TRANSITIONING: {
            LOGD("Rendering fade-out effect");
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
