#include "loading_screen.h"
#include "text_renderer.h"
#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"
#include "../localization/localization_manager.h"
#include <android/log.h>
#include <GLES3/gl3.h>
#include <cmath>

#define LOG_TAG "LoadingScreen"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

LoadingScreen::LoadingScreen() {
    LOGD("LoadingScreen created");
}

LoadingScreen::~LoadingScreen() {
    cleanup();
    LOGD("LoadingScreen destroyed");
}

void LoadingScreen::initialize(LocalizationManager* lm, TextRenderer* tr) {
    localizationManager = lm;
    textRenderer = tr;
    visible = false;
    progress = 0.0f;
    symbolRotation = 0.0f;
    fadeAlpha = 0.0f;
    messageText = "Loading";
    loadTextures();
    LOGI("LoadingScreen initialized");
}

void LoadingScreen::loadTextures() {
    if (texturesLoaded) return;

    // Original Oblivion loading screen assets (extracted from BSA).
    bgTexture = TextureLoader::loadTextureFromAsset("textures/ui/loading_background.png");
    symbolTexture = TextureLoader::loadTextureFromAsset("textures/ui/loading_symbol.png");
    loadDefaultTexture = TextureLoader::loadTextureFromAsset("textures/ui/load_in_game_default.png");

    if (bgTexture == 0) {
        LOGW("loading_background.png not found - loading screen will use solid colour");
    }
    if (symbolTexture == 0) {
        LOGW("loading_symbol.png not found - loading screen will skip symbol animation");
    }
    if (loadDefaultTexture == 0) {
        LOGW("load_in_game_default.png not found - loading screen will skip default overlay");
    }

    texturesLoaded = true;
    LOGI("LoadingScreen textures loaded (bg=%u, symbol=%u, default=%u)",
         bgTexture, symbolTexture, loadDefaultTexture);
}

void LoadingScreen::cleanup() {
    TextureLoader::deleteTexture(bgTexture);
    TextureLoader::deleteTexture(symbolTexture);
    TextureLoader::deleteTexture(loadDefaultTexture);
    bgTexture = 0;
    symbolTexture = 0;
    loadDefaultTexture = 0;
    texturesLoaded = false;
    LOGD("LoadingScreen textures cleaned up");
}

void LoadingScreen::setScreenSize(int w, int h) {
    screenWidth = w;
    screenHeight = h;
}

void LoadingScreen::show(const std::string& message) {
    visible = true;
    messageText = message;
    progress = 0.0f;
    fadeAlpha = 0.0f;
    symbolRotation = 0.0f;
    LOGD("LoadingScreen shown: %s", message.c_str());
}

void LoadingScreen::hide() {
    visible = false;
    LOGD("LoadingScreen hidden");
}

void LoadingScreen::setProgress(float p) {
    progress = (p < 0.0f) ? 0.0f : (p > 1.0f) ? 1.0f : p;
}

void LoadingScreen::setMessage(const std::string& message) {
    messageText = message;
}

void LoadingScreen::update(float deltaTime) {
    if (!visible) return;

    // Fade in
    if (fadeAlpha < 1.0f) {
        fadeAlpha += deltaTime * kFadeSpeed;
        if (fadeAlpha > 1.0f) fadeAlpha = 1.0f;
    }

    // Subtle symbol rotation (like the original's spinning loading icon)
    symbolRotation += deltaTime * 90.0f;  // 90 degrees per second
    if (symbolRotation >= 360.0f) {
        symbolRotation -= 360.0f;
    }
}

void LoadingScreen::render() {
    if (!visible) return;

    // Scale from the 1280x960 canvas to the actual screen
    const float scaleX = static_cast<float>(screenWidth) / kCanvasWidth;
    const float scaleY = static_cast<float>(screenHeight) / kCanvasHeight;
    const float scale = std::min(scaleX, scaleY);

    // Centre the canvas on screen
    const float canvasW = kCanvasWidth * scale;
    const float canvasH = kCanvasHeight * scale;
    const float offsetX = (screenWidth - canvasW) * 0.5f;
    const float offsetY = (screenHeight - canvasH) * 0.5f;

    // Background: solid dark fill
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw the loading_background if available
    if (bgTexture != 0) {
        UIDrawHelper::drawTexturedQuad(
            0.0f, 0.0f,
            static_cast<float>(screenWidth), static_cast<float>(screenHeight),
            bgTexture,
            glm::vec4(1.0f, 1.0f, 1.0f, fadeAlpha),
            screenWidth, screenHeight);
    }

    // Draw the load_in_game_default overlay (centred, scaled to fit)
    if (loadDefaultTexture != 0) {
        // The original is a 16:9 image; fit it within the canvas
        float imgW = canvasW;
        float imgH = canvasH;
        float imgX = offsetX;
        float imgY = offsetY;
        UIDrawHelper::drawTexturedQuad(
            imgX, imgY, imgW, imgH,
            loadDefaultTexture,
            glm::vec4(1.0f, 1.0f, 1.0f, fadeAlpha * 0.6f),
            screenWidth, screenHeight);
    }

    // Draw the loading symbol (spinning icon)
    renderSymbol();

    // Draw the progress bar
    renderProgressBar();

    // Draw the status message
    renderMessage();
}

void LoadingScreen::renderSymbol() {
    if (symbolTexture == 0) return;

    const float scaleX = static_cast<float>(screenWidth) / kCanvasWidth;
    const float scaleY = static_cast<float>(screenHeight) / kCanvasHeight;
    const float scale = std::min(scaleX, scaleY);

    const float symbolSize = kSymbolSize * scale;
    const float symbolX = (screenWidth - symbolSize) * 0.5f;
    const float symbolY = (screenHeight * 0.5f) - (kSymbolY * scale) - symbolSize;

    // Draw the symbol with rotation via UV trick: the symbol is a simple
    // circular icon, so we draw it centred above the progress bar.
    UIDrawHelper::drawTexturedQuad(
        symbolX, symbolY, symbolSize, symbolSize,
        symbolTexture,
        glm::vec4(1.0f, 1.0f, 1.0f, fadeAlpha),
        screenWidth, screenHeight);
}

void LoadingScreen::renderProgressBar() {
    const float scaleX = static_cast<float>(screenWidth) / kCanvasWidth;
    const float scaleY = static_cast<float>(screenHeight) / kCanvasHeight;
    const float scale = std::min(scaleX, scaleY);

    const float barW = kProgressBarWidth * scale;
    const float barH = kProgressBarHeight * scale;
    const float barX = (screenWidth - barW) * 0.5f;
    const float barY = screenHeight * 0.5f + (kProgressBarY * scale);

    // Background track (dark)
    UIDrawHelper::drawColoredQuad(
        barX, barY, barW, barH,
        glm::vec4(0.15f, 0.12f, 0.08f, fadeAlpha * 0.8f),
        screenWidth, screenHeight);

    // Progress fill (gold, matching Oblivion's UI accent)
    if (progress > 0.0f) {
        const float fillW = barW * progress;
        UIDrawHelper::drawColoredQuad(
            barX, barY, fillW, barH,
            glm::vec4(0.75f, 0.60f, 0.25f, fadeAlpha * 0.9f),
            screenWidth, screenHeight);
    }

    // Border
    UIDrawHelper::drawBorder(
        barX, barY, barW, barH, 1.0f,
        glm::vec4(0.5f, 0.4f, 0.2f, fadeAlpha * 0.7f),
        screenWidth, screenHeight);
}

void LoadingScreen::renderMessage() {
    if (!textRenderer || messageText.empty()) return;

    const float scaleX = static_cast<float>(screenWidth) / kCanvasWidth;
    const float scaleY = static_cast<float>(screenHeight) / kCanvasHeight;
    const float scale = std::min(scaleX, scaleY);

    const float textY = screenHeight * 0.5f + (kProgressBarY * scale) + kProgressBarHeight * scale + 30.0f * scale;

    // Render the loading message centred below the progress bar
    // Use the Kingthings font (font index 1) matching the original loading_menu.xml
        // Manually centre by computing text width
        const float textWidth = textRenderer->getTextWidth(messageText, 1.2f * scale);
        const float textX = (screenWidth - textWidth) * 0.5f;
        textRenderer->renderText(
            messageText,
            textX, textY,
            glm::vec3(0.75f, 0.65f, 0.40f),  // Gold dim
            1.2f * scale);
}