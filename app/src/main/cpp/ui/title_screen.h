#pragma once

#include <vector>
#include <string>
#include <memory>
#include <android/log.h>
#include "../localization/localization_manager.h"
#include "settings_ui.h"
#include "ui_panel.h"
#include "ui_button.h"

#define LOG_TAG "TitleScreen"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

class TextRenderer;

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
    bool loadGameRequested;
    LocalizationManager* localizationManager;
    std::unique_ptr<SettingsUI> settingsUI;
    TextRenderer* textRenderer = nullptr;

    // Phase 9: Graphical UI components
    std::shared_ptr<UIPanel> menuPanel;
    std::vector<std::shared_ptr<UIButton>> menuButtons;
    int screenWidth = 1080;
    int screenHeight = 1920;

    // Phase 9: Textures
    GLuint bgTexture = 0;
    GLuint logoTexture = 0;
    bool texturesLoaded = false;

    // Button textures
    GLuint btnNormalTex = 0;
    GLuint btnHoverTex = 0;
    GLuint btnPressedTex = 0;

    // Menu panel texture
    GLuint menuPanelTexture = 0;

    static constexpr float LOGO_DISPLAY_DURATION = 3.0f;
    static constexpr int MENU_START = 0;      // "Start Game"
    static constexpr int MENU_LOAD = 1;       // "Load Game"
    static constexpr int MENU_SETTINGS = 2;   // "Settings"
    static constexpr int MENU_QUIT = 3;       // "Quit"

public:
    TitleScreen();
    ~TitleScreen();

    void initialize(LocalizationManager* lm, TextRenderer* tr);
    void update(float deltaTime);
    void render();
    void onTouchEvent(float x, float y);
    void onKeyPress(int key);

    bool isGameStarted() const { return gameStarted; }
    bool isSettingsRequested() const { return settingsRequested; }
    void resetSettingsRequest() { settingsRequested = false; }
    bool isLoadGameRequested() const { return loadGameRequested; }
    void resetLoadGameRequest() { loadGameRequested = false; }
    TitleScreenState getState() const { return state; }

    void setScreenSize(int w, int h);

private:
    void transitionToMenu();
    void transitionToLanguageMenu();
    void updateMenu(float deltaTime);
    void handleMenuSelection();
    void startGame();
    void buildGraphicalMenu();
    void rebuildMenuLayout();

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
