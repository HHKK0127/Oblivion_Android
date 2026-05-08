#pragma once

#include <vector>
#include <string>
#include <memory>
#include <android/log.h>
#include "../localization/localization_manager.h"
#include "settings_ui.h"

#define LOG_TAG "TitleScreen"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

enum class TitleScreenState {
    LOGO_DISPLAY,  // Show logo for 3 seconds
    MENU,          // Show menu
    LANGUAGE,      // Language selection
    TRANSITIONING  // Transition to game
};

class TitleScreen {
private:
    TitleScreenState state;
    float displayTimer;
    std::vector<std::string> menuItems;
    int selectedIndex;
    bool gameStarted;
    bool settingsRequested;
    LocalizationManager* localizationManager;
    std::unique_ptr<SettingsUI> settingsUI;

    static constexpr float LOGO_DISPLAY_DURATION = 3.0f;
    static constexpr int MENU_START = 0;      // "Start Game"
    static constexpr int MENU_SETTINGS = 1;   // "Settings"
    static constexpr int MENU_QUIT = 2;       // "Quit"

public:
    TitleScreen();
    ~TitleScreen();

    void initialize(LocalizationManager* lm);
    void update(float deltaTime);
    void render();
    void onTouchEvent(float x, float y);
    void onKeyPress(int key);

    bool isGameStarted() const { return gameStarted; }
    bool isSettingsRequested() const { return settingsRequested; }
    void resetSettingsRequest() { settingsRequested = false; }
    TitleScreenState getState() const { return state; }

private:
    void transitionToMenu();
    void transitionToLanguageMenu();
    void updateMenu(float deltaTime);
    void handleMenuSelection();
    void startGame();

    // Oblivion風レンダリング関数
    void renderLogoDisplay();
    void renderMenu();
    void renderLanguageSelection();
    void renderFadeOut();
    void renderOblivionLogo(float alpha);
    void renderPressToStartMessage(float alpha);
    void renderMenuTitle();
    void renderMenuItems();
};
