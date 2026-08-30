#include "world_debug_info.h"
#include "text_renderer.h"
#include "../world/world_manager.h"
#include <GLES3/gl3.h>
#include <android/log.h>

#define LOG_TAG_WORLD_DBG "WorldDebugInfo"
#define LOGD_WORLD_DBG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_WORLD_DBG, __VA_ARGS__)

WorldDebugInfo::WorldDebugInfo()
    : textRenderer(nullptr), worldManager(nullptr), visible(false), initialized(false),
      screenWidth(1080), screenHeight(1920),
      updateInterval(0.2f), timeSinceLastUpdate(0.0f) {
    cachedInfo = {};
}

WorldDebugInfo::~WorldDebugInfo() {
    cleanup();
}

bool WorldDebugInfo::initialize(TextRenderer* tr, WorldManager* wm) {
    if (initialized) return true;
    textRenderer = tr;
    worldManager = wm;
    initialized = true;
    LOGD_WORLD_DBG("WorldDebugInfo initialized");
    return true;
}

void WorldDebugInfo::cleanup() {
    initialized = false;
}

void WorldDebugInfo::toggle() {
    visible = !visible;
}

void WorldDebugInfo::update(float deltaTime) {
    if (!visible) return;

    timeSinceLastUpdate += deltaTime;
    if (timeSinceLastUpdate >= updateInterval) {
        timeSinceLastUpdate = 0.0f;
        updateCachedInfo();
    }
}

void WorldDebugInfo::updateCachedInfo() {
    if (!worldManager) return;

    cachedInfo.playerPos = worldManager->getPlayerPosition();
    cachedInfo.cameraPos = worldManager->getCameraPosition();
    cachedInfo.cameraForward = worldManager->getCameraForward();

    // Calculate cell coordinates (Oblivion uses 4096-unit cells)
    const float CELL_SIZE = 4096.0f;
    cachedInfo.currentCellX = static_cast<int32_t>(floor(cachedInfo.playerPos.x / CELL_SIZE));
    cachedInfo.currentCellY = static_cast<int32_t>(floor(cachedInfo.playerPos.z / CELL_SIZE));

    // Use getActiveCells().size() for loaded cell count
    cachedInfo.loadedCells = static_cast<int>(worldManager->getActiveCells().size());
}

void WorldDebugInfo::render() {
    if (!visible || !textRenderer) return;

    // DPI-aware scaling
    float minDim = static_cast<float>(std::min(screenWidth, screenHeight));
    float scale = minDim / 1080.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    float fontSize = 0.45f * scale;
    float lineH = fontSize * 22.0f;
    float x = 10.0f * scale;
    float y = lineH * 2.0f;

    // Header
    glm::vec3 headerColor(0.3f, 0.7f, 0.9f);
    textRenderer->renderText("WORLD INFO", x, y, headerColor, 0.6f * scale);
    y += lineH * 1.8f;

    // Info lines
    glm::vec3 labelColor(0.7f, 0.7f, 0.8f);
    glm::vec3 valueColor(0.9f, 0.9f, 0.9f);

    // Player Position
    textRenderer->renderText("Player:", x, y, labelColor, fontSize);
    textRenderer->renderText(formatVector(cachedInfo.playerPos), x + 80.0f * scale, y, valueColor, fontSize);
    y += lineH;

    // Camera Position
    textRenderer->renderText("Camera:", x, y, labelColor, fontSize);
    textRenderer->renderText(formatVector(cachedInfo.cameraPos), x + 80.0f * scale, y, valueColor, fontSize);
    y += lineH;

    // Camera Forward
    textRenderer->renderText("Forward:", x, y, labelColor, fontSize);
    textRenderer->renderText(formatVector(cachedInfo.cameraForward), x + 80.0f * scale, y, valueColor, fontSize);
    y += lineH;

    // Cell Coordinates
    char cellBuf[64];
    snprintf(cellBuf, sizeof(cellBuf), "(%d, %d)", cachedInfo.currentCellX, cachedInfo.currentCellY);
    textRenderer->renderText("Cell:", x, y, labelColor, fontSize);
    textRenderer->renderText(cellBuf, x + 80.0f * scale, y, valueColor, fontSize);
    y += lineH;

    // Loaded Cells
    char loadedBuf[32];
    snprintf(loadedBuf, sizeof(loadedBuf), "%d", cachedInfo.loadedCells);
    textRenderer->renderText("Loaded:", x, y, labelColor, fontSize);
    textRenderer->renderText(loadedBuf, x + 80.0f * scale, y, valueColor, fontSize);
}

std::string WorldDebugInfo::formatVector(const glm::vec3& v) const {
    char buf[64];
    snprintf(buf, sizeof(buf), "(%.1f, %.1f, %.1f)", v.x, v.y, v.z);
    return std::string(buf);
}
