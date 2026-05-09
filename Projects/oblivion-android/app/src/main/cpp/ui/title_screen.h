#pragma once

#include <vector>
#include <string>
#include <memory>
#include <android/log.h>
#include "../localization/localization_manager.h"
#include "text_renderer.h"
#include "image_renderer.h"

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
    LocalizationManager* localizationManager;
    TextRenderer* textRenderer;
    AAssetManager* assetManager;

    // Background images
    std::unique_ptr<ImageRenderer> backgroundImage;
    std::unique_ptr<ImageRenderer> logoImage;
    bool imagesLoaded;

    static constexpr float LOGO_DISPLAY_DURATION = 3.0f;

public:
    TitleScreen();
    ~TitleScreen();

    void initialize(LocalizationManager* lm);
    void update(float deltaTime);
    void render(TextRenderer* textRenderer);
    void onTouchEvent(float x, float y);
    void onKeyPress(int key);
    void setTextRenderer(TextRenderer* tr);
    void setAssetManager(AAssetManager* mgr);
    void loadImages();
    void playTitleBGM();
    void stopTitleBGM();

    bool isGameStarted() const { return gameStarted; }
    TitleScreenState getState() const { return state; }

    bool isSettingsRequested() const { return settingsRequested; }
    void resetSettingsRequest() { settingsRequested = false; }

    bool isBGMPlayRequested(std::string& outPath);
    bool isBGMStopRequested();
    void resetBGMRequests();

private:
    void transitionToMenu();
    bool settingsRequested;
    bool bgmPlayRequested;
    bool bgmStopRequested;
    std::string bgmPath;

    void transitionToLanguageMenu();
    void updateMenu(float deltaTime);
    void handleMenuSelection();
    void startGame();
};
