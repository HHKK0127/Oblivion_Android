#include "text_renderer.h"
#include <GLES3/gl3.h>
#include <android/log.h>

#define LOG_TAG_TEXT "TextRenderer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_TEXT, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_TEXT, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_TEXT, __VA_ARGS__)

TextRenderer::TextRenderer()
    : screenWidth(0), screenHeight(0), initialized(false) {
}

TextRenderer::~TextRenderer() {
    cleanup();
}

bool TextRenderer::initialize() {
    LOGI("TextRenderer initialized");
    initialized = true;
    return true;
}

void TextRenderer::cleanup() {
    initialized = false;
    LOGD("TextRenderer cleaned up");
}

void TextRenderer::setScreenSize(unsigned int width, unsigned int height) {
    screenWidth = width;
    screenHeight = height;
    LOGD("TextRenderer screen size set: %ux%u", screenWidth, screenHeight);
}

void TextRenderer::renderText(const std::string& text, float x, float y,
                                float scale, const float color[4]) {
    if (!initialized) return;
    // TODO: Implement OpenGL text rendering with font atlas
}

void TextRenderer::renderText(const std::string& text, float x, float y,
                                float scale, float r, float g, float b, float a) {
    float color[4] = {r, g, b, a};
    renderText(text, x, y, scale, color);
}

float TextRenderer::getTextWidth(const std::string& text, float scale) const {
    return text.size() * scale * 0.5f;
}

float TextRenderer::getTextHeight(float scale) const {
    return scale * 1.0f;
}