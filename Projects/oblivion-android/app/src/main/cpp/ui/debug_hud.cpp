#include "debug_hud.h"
#include "text_renderer.h"
#include <android/log.h>

#define LOG_TAG_HUD "DebugHUD"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_HUD, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_HUD, __VA_ARGS__)

DebugHUD::DebugHUD()
    : textRenderer(nullptr), currentFPS(0.0f), drawCalls(0),
      triangleCount(0), memoryUsageBytes(0), initialized(false) {
}

DebugHUD::~DebugHUD() {
    cleanup();
}

bool DebugHUD::initialize(TextRenderer* textRenderer) {
    if (!textRenderer) {
        LOGD("DebugHUD: null TextRenderer");
        return false;
    }
    this->textRenderer = textRenderer;
    initialized = true;
    LOGI("DebugHUD initialized");
    return true;
}

void DebugHUD::cleanup() {
    textRenderer = nullptr;
    initialized = false;
}

void DebugHUD::update(float deltaTime) {
}

void DebugHUD::render() {
    if (!initialized || !textRenderer) return;

    float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    textRenderer->renderText("FPS: " + std::to_string(static_cast<int>(currentFPS)),
                              10.0f, 10.0f, 0.5f, white);

    textRenderer->renderText("Draws: " + std::to_string(drawCalls),
                              10.0f, 30.0f, 0.5f, white);

    float memMB = static_cast<float>(memoryUsageBytes) / (1024.0f * 1024.0f);
    textRenderer->renderText("Mem: " + std::to_string(static_cast<int>(memMB)) + " MB",
                              10.0f, 50.0f, 0.5f, white);
}

void DebugHUD::setFPS(float fps) { currentFPS = fps; }
void DebugHUD::setDrawCalls(int calls) { drawCalls = calls; }
void DebugHUD::setTriangleCount(int count) { triangleCount = count; }
void DebugHUD::setMemoryUsage(size_t bytes) { memoryUsageBytes = bytes; }