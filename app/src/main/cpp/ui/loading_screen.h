#pragma once

#include <string>
#include <functional>
#include <GLES3/gl3.h>

class TextRenderer;
class LocalizationManager;

/**
 * @brief Loading screen displayed during game data / world transitions.
 *
 * Uses original Oblivion assets: loading_background.png, load_in_game_default.png,
 * loading_symbol.png.  The progress bar is a simple coloured rectangle until the
 * original stat_skill_level_ribbon textures are available.
 *
 * Lifecycle:  initialize() -> update()/render() -> cleanup()
 */
class LoadingScreen {
public:
    LoadingScreen();
    ~LoadingScreen();

    void initialize(LocalizationManager* lm, TextRenderer* tr);
    void update(float deltaTime);
    void render();
    void cleanup();

    void setScreenSize(int w, int h);

    /** Show the loading screen with a status message. */
    void show(const std::string& message = "Loading");

    /** Hide the loading screen. */
    void hide();

    /** Update the progress bar (0.0 – 1.0). */
    void setProgress(float progress);

    /** Update the status message. */
    void setMessage(const std::string& message);

    bool isVisible() const { return visible; }
    float getProgress() const { return progress; }

private:
    void loadTextures();
    void renderBackground();
    void renderSymbol();
    void renderProgressBar();
    void renderMessage();

    // State
    bool visible = false;
    bool texturesLoaded = false;
    int screenWidth = 1920;
    int screenHeight = 1080;
    float progress = 0.0f;
    float symbolRotation = 0.0f;  // subtle rotation animation
    float fadeAlpha = 0.0f;       // fade-in effect

    // Dependencies
    LocalizationManager* localizationManager = nullptr;
    TextRenderer* textRenderer = nullptr;

    // Textures
    GLuint bgTexture = 0;          // loading_background.png
    GLuint symbolTexture = 0;      // loading_symbol.png
    GLuint loadDefaultTexture = 0; // load_in_game_default.png

    // Status message
    std::string messageText;

    // Layout constants (1280x960 canvas, scaled to screen)
    static constexpr float kCanvasWidth = 1280.0f;
    static constexpr float kCanvasHeight = 960.0f;
    static constexpr float kProgressBarWidth = 300.0f;
    static constexpr float kProgressBarHeight = 20.0f;
    static constexpr float kProgressBarY = 120.0f;
    static constexpr float kSymbolSize = 64.0f;
    static constexpr float kSymbolY = 200.0f;
    static constexpr float kFadeSpeed = 2.0f;  // alpha units per second
    };