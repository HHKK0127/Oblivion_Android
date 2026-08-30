#include "performance_graph.h"
#include "text_renderer.h"
#include <GLES3/gl3.h>
#include <algorithm>
#include <numeric>
#include <android/log.h>

#define LOG_TAG_PERF "PerformanceGraph"
#define LOGD_PERF(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_PERF, __VA_ARGS__)

PerformanceGraph::PerformanceGraph()
    : textRenderer(nullptr), visible(false), initialized(false),
      screenWidth(1080), screenHeight(1920),
      historyIndex(0), currentFPS(0.0f), currentFrameTime(0.0f),
      avgFPS(0.0f), minFPS(999.0f), maxFPS(0.0f),
      drawCalls(0), memoryMB(0.0f) {
    frameTimeHistory.resize(HISTORY_SIZE, 0.0f);
    fpsHistory.resize(HISTORY_SIZE, 0.0f);
}

PerformanceGraph::~PerformanceGraph() {
    cleanup();
}

bool PerformanceGraph::initialize(TextRenderer* tr) {
    if (initialized) return true;
    textRenderer = tr;
    initialized = true;
    LOGD_PERF("PerformanceGraph initialized");
    return true;
}

void PerformanceGraph::cleanup() {
    initialized = false;
}

void PerformanceGraph::toggle() {
    visible = !visible;
}

void PerformanceGraph::update(float deltaTime) {
    if (!visible) return;

    // Calculate FPS from delta time
    if (deltaTime > 0.0f) {
        currentFPS = 1.0f / deltaTime;
        currentFrameTime = deltaTime * 1000.0f; // Convert to ms

        // Add to history
        fpsHistory[historyIndex] = currentFPS;
        frameTimeHistory[historyIndex] = currentFrameTime;
        historyIndex = (historyIndex + 1) % HISTORY_SIZE;

        // Calculate stats
        avgFPS = calculateAverage(fpsHistory);
        minFPS = calculateMin(fpsHistory);
        maxFPS = calculateMax(fpsHistory);
    }
}

void PerformanceGraph::addFrameTime(float ms) {
    currentFrameTime = ms;
    if (ms > 0.0f) {
        currentFPS = 1000.0f / ms;
    }
}

void PerformanceGraph::render() {
    if (!visible || !textRenderer) return;

    // DPI-aware scaling
    float minDim = static_cast<float>(std::min(screenWidth, screenHeight));
    float scale = minDim / 1080.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    float graphW = 200.0f * scale;
    float graphH = 60.0f * scale;
    float x = screenWidth - graphW - 10.0f * scale;
    float y = 10.0f * scale;

    // FPS Graph
    glm::vec3 fpsColor(0.2f, 0.8f, 0.3f);
    renderGraph(x, y, graphW, graphH, fpsHistory, 60.0f, fpsColor, "FPS");
    y += graphH + 5.0f * scale;

    // Frame Time Graph
    glm::vec3 ftColor(0.3f, 0.6f, 0.9f);
    renderGraph(x, y, graphW, graphH, frameTimeHistory, 33.3f, ftColor, "Frame Time (ms)");
    y += graphH + 10.0f * scale;

    // Stats text
    renderStats(x, y, scale);
}

void PerformanceGraph::renderGraph(float x, float y, float width, float height,
                                     const std::vector<float>& data, float maxVal,
                                     const glm::vec3& color, const std::string& label) {
    // Background
    glm::vec4 bgColor(0.1f, 0.1f, 0.1f, 0.7f);

    // Draw graph background using GL
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Simple quad for background
    GLfloat vertices[] = {
        x, y,
        x + width, y,
        x + width, y + height,
        x, y + height
    };

    // We'll use text-based graph for now since we don't have a shape renderer
    // Draw label
    float fontSize = 0.4f;
    textRenderer->renderText(label, x, y - 5.0f, color, fontSize);

    // Draw graph bars (simplified)
    float barWidth = width / HISTORY_SIZE;
    for (int i = 0; i < HISTORY_SIZE; i++) {
        int idx = (historyIndex + i) % HISTORY_SIZE;
        float val = data[idx];
        if (val <= 0.0f) continue;

        float normalizedHeight = (val / maxVal) * height;
        if (normalizedHeight > height) normalizedHeight = height;

        // Draw a simple character to represent the bar
        float barX = x + i * barWidth;
        float barY = y + height - normalizedHeight;

        // Use a block character to draw the bar
        int blocks = static_cast<int>(normalizedHeight / 4.0f);
        for (int b = 0; b < blocks; b++) {
            textRenderer->renderText("|", barX, y + height - b * 4.0f, color, 0.3f);
        }
    }
}

void PerformanceGraph::renderStats(float x, float y, float scale) {
    float fontSize = 0.4f * scale;
    float lineH = fontSize * 20.0f;

    glm::vec3 labelColor(0.7f, 0.7f, 0.8f);
    glm::vec3 valueColor(0.9f, 0.9f, 0.9f);
    glm::vec3 goodColor(0.3f, 0.9f, 0.3f);
    glm::vec3 warnColor(0.9f, 0.9f, 0.3f);
    glm::vec3 badColor(0.9f, 0.3f, 0.3f);

    // Current FPS
    glm::vec3 fpsColor = currentFPS >= 55.0f ? goodColor : (currentFPS >= 30.0f ? warnColor : badColor);
    char fpsBuf[32];
    snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f", currentFPS);
    textRenderer->renderText(fpsBuf, x, y, fpsColor, fontSize);
    y += lineH;

    // Average FPS
    char avgBuf[32];
    snprintf(avgBuf, sizeof(avgBuf), "Avg: %.1f", avgFPS);
    textRenderer->renderText(avgBuf, x, y, labelColor, fontSize);
    y += lineH;

    // Min/Max FPS
    char minMaxBuf[64];
    snprintf(minMaxBuf, sizeof(minMaxBuf), "%.1f - %.1f", minFPS, maxFPS);
    textRenderer->renderText(minMaxBuf, x, y, labelColor, fontSize);
    y += lineH;

    // Frame Time
    char ftBuf[32];
    snprintf(ftBuf, sizeof(ftBuf), "FT: %.1fms", currentFrameTime);
    textRenderer->renderText(ftBuf, x, y, labelColor, fontSize);
    y += lineH;

    // Draw Calls
    if (drawCalls > 0) {
        char dcBuf[32];
        snprintf(dcBuf, sizeof(dcBuf), "DC: %d", drawCalls);
        textRenderer->renderText(dcBuf, x, y, labelColor, fontSize);
        y += lineH;
    }

    // Memory
    if (memoryMB > 0.0f) {
        char memBuf[32];
        snprintf(memBuf, sizeof(memBuf), "Mem: %.1fMB", memoryMB);
        textRenderer->renderText(memBuf, x, y, labelColor, fontSize);
    }
}

float PerformanceGraph::calculateAverage(const std::vector<float>& data) const {
    float sum = 0.0f;
    int count = 0;
    for (float val : data) {
        if (val > 0.0f) {
            sum += val;
            count++;
        }
    }
    return count > 0 ? sum / count : 0.0f;
}

float PerformanceGraph::calculateMin(const std::vector<float>& data) const {
    float minVal = 999.0f;
    for (float val : data) {
        if (val > 0.0f && val < minVal) {
            minVal = val;
        }
    }
    return minVal;
}

float PerformanceGraph::calculateMax(const std::vector<float>& data) const {
    float maxVal = 0.0f;
    for (float val : data) {
        if (val > maxVal) {
            maxVal = val;
        }
    }
    return maxVal;
}
