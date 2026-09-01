#include "debug_hud.h"
#include "../audio/audio_manager.h"
#include "../engine/renderer.h"
#include <android/log.h>
#include <sstream>
#include <iomanip>
#include <sys/sysinfo.h>
#include <algorithm>
#include <cmath>

#undef LOG_TAG
#define LOG_TAG "DebugHUD"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

DebugHUD::DebugHUD()
    : textRenderer(nullptr), audioManager(nullptr), renderer(nullptr),
      visible(true), logVisible(false),
      currentPage(0),
      fps(0.0f), frameTimeMs(0.0f), avgFrameTimeMs(0.0f),
      minFrameTimeMs(std::numeric_limits<float>::max()), maxFrameTimeMs(0.0f),
      frameCount(0), timeSinceLastUpdate(0.0f), timeSinceLastReset(0.0f),
      fpsHistoryIndex(0),
      cpuTimeMs(0.0f), gpuTimeMs(0.0f), waitTimeMs(0.0f),
      drawCallCount(0), vertexCount(0), triangleCount(0),
      textureMemoryBytes(0), loadedTextureCount(0),
      nativeHeapBytes(0), javaHeapBytes(0) {
    fpsHistory.fill(0.0f);
    phaseTimesUs.fill(0.0f);
    LOGD("DebugHUD created");
}

DebugHUD::~DebugHUD() {
    cleanup();
}

bool DebugHUD::initialize(TextRenderer* textRend, AudioManager* audioMgr, Renderer* rend) {
    if (!textRend) {
        LOGD("Error: TextRenderer is null");
        return false;
    }
    textRenderer = textRend;
    audioManager = audioMgr;
    renderer = rend;
    LOGD("DebugHUD initialized");
    return true;
}

void DebugHUD::update(float deltaTime) {
    frameTimeMs = deltaTime * 1000.0f;

    // Accurate average: track sum and count for the current window
    // Using incremental average with frame counter (reset periodically)
    frameCount++;
    if (frameCount == 1) {
        avgFrameTimeMs = frameTimeMs;
    } else {
        avgFrameTimeMs = (avgFrameTimeMs * (frameCount - 1) + frameTimeMs) / frameCount;
    }

    // Track min/max
    minFrameTimeMs = std::min(minFrameTimeMs, frameTimeMs);
    maxFrameTimeMs = std::max(maxFrameTimeMs, frameTimeMs);

    timeSinceLastUpdate += deltaTime;
    timeSinceLastReset += deltaTime;

    // Periodic update for FPS display (every UPDATE_INTERVAL)
    if (timeSinceLastUpdate >= UPDATE_INTERVAL) {
        if (timeSinceLastUpdate > 0.0f) {
            // Use actual frame count over the time window for accurate FPS
            fps = static_cast<float>(frameCount) / timeSinceLastUpdate;
        }
        fpsHistory[fpsHistoryIndex % FPS_HISTORY_SIZE] = fps;
        fpsHistoryIndex++;
        timeSinceLastUpdate = 0.0f;
    }

    // Periodic reset of min/max/avg (every RESET_INTERVAL)
    if (timeSinceLastReset >= RESET_INTERVAL) {
        minFrameTimeMs = std::numeric_limits<float>::max();
        maxFrameTimeMs = 0.0f;
        avgFrameTimeMs = 0.0f;
        frameCount = 0;
        timeSinceLastReset = 0.0f;
    }
}

// --- Extended stats setters ---

void DebugHUD::setFrameTimeBreakdown(float cpu, float gpu, float wait) {
    cpuTimeMs = cpu;
    gpuTimeMs = gpu;
    waitTimeMs = wait;
}

void DebugHUD::setPhaseTime(int idx, float us) {
    if (idx >= 0 && idx < PHASE_COUNT) phaseTimesUs[idx] = us;
}

void DebugHUD::setDrawCallCount(int count) { drawCallCount = count; }
void DebugHUD::setVertexCount(int count) { vertexCount = count; }
void DebugHUD::setTriangleCount(int count) { triangleCount = count; }
void DebugHUD::setTextureMemory(long bytes) { textureMemoryBytes = bytes; }
void DebugHUD::setLoadedTextureCount(int count) { loadedTextureCount = count; }
void DebugHUD::setNativeHeap(long bytes) { nativeHeapBytes = bytes; }
void DebugHUD::setJavaHeap(long bytes) { javaHeapBytes = bytes; }

void DebugHUD::addLogLine(const std::string& line) {
    logLines.push_back(line);
    if (logLines.size() > MAX_LOG_LINES) {
        logLines.erase(logLines.begin());
    }
}

void DebugHUD::exportLogs() {
    // In a real implementation, this would write to a file
    // For now, just log that export was requested
    addLogLine("Logs exported (simulated)");
}

std::string DebugHUD::getLogStats() const {
    return "Log lines: " + std::to_string(logLines.size()) + "/" + std::to_string(MAX_LOG_LINES)
           + " Level: " + logLevel
           + " Auto-scroll: " + (logAutoScroll ? "ON" : "OFF");
}

void DebugHUD::searchLogs(const std::string& pattern) {
    // In a real implementation, this would filter logs
    addLogLine("Searching for: " + pattern);
}

void DebugHUD::nextPage() { currentPage = (currentPage + 1) % totalPages; }
void DebugHUD::prevPage() { currentPage = (currentPage - 1 + totalPages) % totalPages; }

// --- Render ---

void DebugHUD::render() {
    if (!visible || !textRenderer) return;

    float xPos = 10.0f;
    float yPos = 10.0f;
    float lineHeight = 28.0f; // Increased from 18.0f for better readability on mobile
    float textScale = 1.4f;   // Increased from 1.0f for better readability on mobile

    // Limit rendering area to prevent overflow (use reasonable default)
    float maxY = 1800.0f; // Will be updated when screen size is available

    // Page indicator
    {
        std::stringstream ss;
        ss << "[Page " << (currentPage + 1) << "/" << totalPages << "]";
        textRenderer->renderText(ss.str(), xPos, yPos, glm::vec3(0.7f, 0.7f, 0.7f), 0.8f);
        yPos += lineHeight;
    }

    // Only render if within bounds
    if (yPos < maxY) {
        switch (currentPage) {
            case 0: renderOverviewPage(xPos, yPos, lineHeight, textScale); break;
            case 1: renderPerformancePage(xPos, yPos, lineHeight, textScale); break;
            case 2: renderMemoryPage(xPos, yPos, lineHeight, textScale); break;
            case 3: renderPhasePage(xPos, yPos, lineHeight, textScale); break;
        }
    }

    if (logVisible) {
        renderLogOverlay();
    }
}

void DebugHUD::renderOverviewPage(float& xPos, float& yPos, float lineHeight, float textScale) {
    glm::vec3 white(1.0f, 1.0f, 1.0f);
    glm::vec3 yellow(1.0f, 1.0f, 0.0f);
    glm::vec3 cyan(0.0f, 1.0f, 1.0f);
    glm::vec3 orange(1.0f, 0.5f, 0.0f);

    // FPS
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << "FPS: " << fps;
        glm::vec3 color = (fps >= 55.0f) ? glm::vec3(0.0f, 1.0f, 0.0f) :
                          (fps >= 30.0f) ? glm::vec3(1.0f, 1.0f, 0.0f) :
                                           glm::vec3(1.0f, 0.0f, 0.0f);
        textRenderer->renderText(ss.str(), xPos, yPos, color, textScale);
        yPos += lineHeight;
    }

    // Frame time
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << "Frame: " << frameTimeMs << " ms";
        textRenderer->renderText(ss.str(), xPos, yPos, white, textScale);
        yPos += lineHeight;
    }

    // Average
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << "Avg: " << avgFrameTimeMs << " ms";
        textRenderer->renderText(ss.str(), xPos, yPos, white, textScale);
        yPos += lineHeight;
    }

    // Memory
    {
        MemoryInfo memInfo = getMemoryInfo();
        std::stringstream ss;
        ss << "Mem: " << formatMemorySize(memInfo.usedMemory) << " / " << formatMemorySize(memInfo.totalMemory);
        textRenderer->renderText(ss.str(), xPos, yPos, white, textScale);
        yPos += lineHeight;
    }

    // Draw calls
    {
        std::stringstream ss;
        ss << "DrawCalls: " << drawCallCount << "  Verts: " << vertexCount << "  Tris: " << triangleCount;
        textRenderer->renderText(ss.str(), xPos, yPos, cyan, textScale);
        yPos += lineHeight;
    }

    // Debug mode
    textRenderer->renderText("DEBUG: ON", xPos, yPos, yellow, textScale);
    yPos += lineHeight;

    // Audio
    {
        std::string audioStatus = getAudioStatus();
        textRenderer->renderText(audioStatus, xPos, yPos, cyan, textScale);
        yPos += lineHeight;
    }

    // RetroFilter
    {
        std::string filterStatus = getRetroFilterStatus();
        if (!filterStatus.empty()) {
            textRenderer->renderText(filterStatus, xPos, yPos, orange, textScale);
            yPos += lineHeight;
        }
    }
}

void DebugHUD::renderPerformancePage(float& xPos, float& yPos, float lineHeight, float textScale) {
    glm::vec3 white(1.0f, 1.0f, 1.0f);
    glm::vec3 green(0.0f, 1.0f, 0.0f);
    glm::vec3 red(1.0f, 0.3f, 0.3f);
    glm::vec3 blue(0.3f, 0.5f, 1.0f);
    glm::vec3 gray(0.5f, 0.5f, 0.5f);

    // FPS
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << "FPS: " << fps;
        glm::vec3 color = (fps >= 55.0f) ? green : (fps >= 30.0f) ? glm::vec3(1.0f, 1.0f, 0.0f) : red;
        textRenderer->renderText(ss.str(), xPos, yPos, color, textScale);
        yPos += lineHeight;
    }

    // Frame time range
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2)
           << "Min: " << minFrameTimeMs << "  Max: " << maxFrameTimeMs << " ms";
        textRenderer->renderText(ss.str(), xPos, yPos, white, textScale);
        yPos += lineHeight;
    }

    // Frame time breakdown
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2)
           << "CPU: " << cpuTimeMs << "  GPU: " << gpuTimeMs << "  Wait: " << waitTimeMs << " ms";
        textRenderer->renderText(ss.str(), xPos, yPos, white, textScale);
        yPos += lineHeight;
    }

    // Frame time bar
    {
        float barX = xPos;
        float barY = yPos;
        float barW = 200.0f;
        float barH = 12.0f;
        renderFrameTimeBar(barX, barY, barW, barH);
        yPos += barH + 4.0f;
        textRenderer->renderText("CPU", xPos, yPos, red, textScale * 0.7f);
        textRenderer->renderText("GPU", xPos + 40.0f, yPos, green, textScale * 0.7f);
        textRenderer->renderText("Wait", xPos + 80.0f, yPos, gray, textScale * 0.7f);
        yPos += lineHeight;
    }

    // FPS Graph
    {
        textRenderer->renderText("FPS History:", xPos, yPos, white, textScale * 0.8f);
        yPos += lineHeight;
        float graphW = 200.0f;
        float graphH = 60.0f;
        renderFpsGraph(xPos, yPos, graphW, graphH);
        yPos += graphH + 4.0f;
    }

    // Rendering stats
    {
        std::stringstream ss;
        ss << "Draw Calls: " << drawCallCount;
        textRenderer->renderText(ss.str(), xPos, yPos, blue, textScale);
        yPos += lineHeight;
    }
    {
        std::stringstream ss;
        ss << "Vertices: " << vertexCount << "  Tris: " << triangleCount;
        textRenderer->renderText(ss.str(), xPos, yPos, blue, textScale);
        yPos += lineHeight;
    }
    {
        std::stringstream ss;
        ss << "Textures: " << loadedTextureCount << " (" << formatMemorySize(textureMemoryBytes) << ")";
        textRenderer->renderText(ss.str(), xPos, yPos, blue, textScale);
        yPos += lineHeight;
    }
}

void DebugHUD::renderMemoryPage(float& xPos, float& yPos, float lineHeight, float textScale) {
    glm::vec3 white(1.0f, 1.0f, 1.0f);
    glm::vec3 green(0.0f, 1.0f, 0.0f);
    glm::vec3 yellow(1.0f, 1.0f, 0.0f);
    glm::vec3 cyan(0.0f, 1.0f, 1.0f);

    textRenderer->renderText("=== Memory Details ===", xPos, yPos, yellow, textScale);
    yPos += lineHeight;

    {
        MemoryInfo memInfo = getMemoryInfo();
        textRenderer->renderText("System:", xPos, yPos, white, textScale * 0.9f);
        yPos += lineHeight;
        std::stringstream ss;
        ss << "  Total: " << formatMemorySize(memInfo.totalMemory);
        textRenderer->renderText(ss.str(), xPos, yPos, white, textScale * 0.8f);
        yPos += lineHeight;
        ss.str("");
        ss << "  Used:  " << formatMemorySize(memInfo.usedMemory);
        textRenderer->renderText(ss.str(), xPos, yPos, green, textScale * 0.8f);
        yPos += lineHeight;
        ss.str("");
        ss << "  Free:  " << formatMemorySize(memInfo.freeMemory);
        textRenderer->renderText(ss.str(), xPos, yPos, cyan, textScale * 0.8f);
        yPos += lineHeight;
    }

    {
        std::stringstream ss;
        ss << "Native Heap: " << formatMemorySize(nativeHeapBytes);
        textRenderer->renderText(ss.str(), xPos, yPos, white, textScale * 0.9f);
        yPos += lineHeight;
    }

    {
        std::stringstream ss;
        ss << "Java Heap:   " << formatMemorySize(javaHeapBytes);
        textRenderer->renderText(ss.str(), xPos, yPos, white, textScale * 0.9f);
        yPos += lineHeight;
    }

    {
        std::stringstream ss;
        ss << "Textures:    " << formatMemorySize(textureMemoryBytes)
           << " (" << loadedTextureCount << " loaded)";
        textRenderer->renderText(ss.str(), xPos, yPos, white, textScale * 0.9f);
        yPos += lineHeight;
    }

    {
        size_t clipCount = 0;
        if (audioManager) {
            clipCount = audioManager->getLoadedClipsCount();
        }
        std::stringstream ss;
        ss << "Audio:       " << clipCount << " clips loaded";
        textRenderer->renderText(ss.str(), xPos, yPos, white, textScale * 0.9f);
        yPos += lineHeight;
    }
}

void DebugHUD::renderPhasePage(float& xPos, float& yPos, float lineHeight, float textScale) {
    glm::vec3 white(1.0f, 1.0f, 1.0f);
    glm::vec3 yellow(1.0f, 1.0f, 0.0f);
    glm::vec3 green(0.0f, 1.0f, 0.0f);
    glm::vec3 red(1.0f, 0.3f, 0.3f);

    textRenderer->renderText("=== Imperial Weave Phases ===", xPos, yPos, yellow, textScale);
    yPos += lineHeight;

    static const char* phaseNames[] = {
        "PreUpdate", "EventProcess", "World", "AI", "Player",
        "Inventory", "Spell", "Animation", "Physics", "Combat",
        "Quest", "Audio", "RenderSubmit", "PostRender", "Cleanup"
    };

    float totalUs = 0.0f;
    for (int i = 0; i < PHASE_COUNT; i++) {
        totalUs += phaseTimesUs[i];
    }

    for (int i = 0; i < PHASE_COUNT; i++) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1);
        ss << phaseNames[i] << ": " << phaseTimesUs[i] << " us";

        glm::vec3 color = green;
        if (phaseTimesUs[i] > 5000.0f) color = red;
        else if (phaseTimesUs[i] > 1000.0f) color = yellow;

        textRenderer->renderText(ss.str(), xPos, yPos, color, textScale * 0.8f);
        yPos += lineHeight * 0.9f;
    }

    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << "Total: " << totalUs << " us ("
           << (totalUs / 1000.0f) << " ms)";
        textRenderer->renderText(ss.str(), xPos, yPos, yellow, textScale * 0.9f);
        yPos += lineHeight;
    }
}

void DebugHUD::renderFpsGraph(float x, float y, float width, float height) {
    // Text-based FPS graph (OpenGL ES 3.0 compatible)
    int count = std::min(fpsHistoryIndex, FPS_HISTORY_SIZE);
    if (count < 2) return;

    // Build a simple bar graph using text characters
    std::string graph;
    for (int i = 0; i < count && i < 30; i++) {
        int idx = (fpsHistoryIndex - count + i) % FPS_HISTORY_SIZE;
        float val = fpsHistory[idx];
        if (val >= 60.0f) graph += "#";
        else if (val >= 45.0f) graph += "=";
        else if (val >= 30.0f) graph += "-";
        else graph += ".";
    }

    textRenderer->renderText(graph, x, y, glm::vec3(0.0f, 1.0f, 0.0f), 0.8f);
}

void DebugHUD::renderFrameTimeBar(float x, float y, float width, float height) {
    // Text-based frame time breakdown (OpenGL ES 3.0 compatible)
    float total = cpuTimeMs + gpuTimeMs + waitTimeMs;
    if (total <= 0.0f) return;

    int cpuPct = static_cast<int>((cpuTimeMs / total) * 100.0f);
    int gpuPct = static_cast<int>((gpuTimeMs / total) * 100.0f);
    int waitPct = 100 - cpuPct - gpuPct;

    std::stringstream ss;
    ss << "CPU:" << cpuPct << "% GPU:" << gpuPct << "% Wait:" << waitPct << "%";
    textRenderer->renderText(ss.str(), x, y, glm::vec3(1.0f, 1.0f, 1.0f), 0.8f);
}

void DebugHUD::renderLogOverlay() {
    if (logLines.empty()) return;

    float xPos = 10.0f;
    float yPos = 200.0f;
    float lineHeight = 14.0f;
    glm::vec3 logColor(0.8f, 0.8f, 0.8f);

    // Header
    textRenderer->renderText("--- Log ---", xPos, yPos, glm::vec3(1.0f, 1.0f, 0.0f), 0.7f);
    yPos += lineHeight;

    for (const auto& line : logLines) {
        textRenderer->renderText(line, xPos, yPos, logColor, 0.7f);
        yPos += lineHeight;
    }
}

// --- Utility ---

void DebugHUD::toggle() {
    visible = !visible;
    LOGD("Debug HUD toggled: %s", visible ? "ON" : "OFF");
}

void DebugHUD::cleanup() {
    textRenderer = nullptr;
    LOGD("DebugHUD cleaned up");
}

DebugHUD::MemoryInfo DebugHUD::getMemoryInfo() const {
    MemoryInfo info = {0, 0, 0};
    FILE* memFile = fopen("/proc/meminfo", "r");
    if (memFile) {
        char line[256];
        long memFree = 0;
        long buffers = 0;
        long cached = 0;
        while (fgets(line, sizeof(line), memFile)) {
            if (sscanf(line, "MemTotal: %ld kB", &info.totalMemory) == 1) {
                info.totalMemory *= 1024;
            } else if (sscanf(line, "MemAvailable: %ld kB", &info.freeMemory) == 1) {
                info.freeMemory *= 1024;
            } else if (sscanf(line, "MemFree: %ld kB", &memFree) == 1) {
                memFree *= 1024;
            } else if (sscanf(line, "Buffers: %ld kB", &buffers) == 1) {
                buffers *= 1024;
            } else if (sscanf(line, "Cached: %ld kB", &cached) == 1) {
                cached *= 1024;
            }
        }
        fclose(memFile);
        // Fallback: MemAvailable not available on older kernels (pre-3.14)
        if (info.freeMemory == 0 && (memFree > 0 || buffers > 0 || cached > 0)) {
            info.freeMemory = memFree + buffers + cached;
        }
        info.usedMemory = info.totalMemory - info.freeMemory;
    }
    return info;
}

std::string DebugHUD::formatMemorySize(long bytes) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);
    if (bytes < 1024) {
        ss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        ss << bytes / 1024.0f << " KB";
    } else if (bytes < 1024L * 1024 * 1024) {
        ss << bytes / (1024.0f * 1024.0f) << " MB";
    } else {
        ss << bytes / (1024.0f * 1024.0f * 1024.0f) << " GB";
    }
    return ss.str();
}

std::string DebugHUD::getAudioStatus() const {
    std::stringstream ss;
    if (!audioManager) {
        ss << "Audio: [disabled]";
        return ss.str();
    }
    ss << "Audio: [not available]";
    return ss.str();
}

std::string DebugHUD::getRetroFilterStatus() const {
    std::stringstream ss;
    if (!renderer) return "";
    auto settings = renderer->getRetroSettings();
    if (!settings) return "";

    std::string activeEffects;
    if (settings->scanlines_enabled) activeEffects += "S";
    if (settings->pixelation_enabled) activeEffects += "P";
    if (settings->color_reduction_enabled) activeEffects += "C";
    if (settings->crt_distortion_enabled) activeEffects += "D";
    if (settings->grain_enabled) activeEffects += "G";

    if (activeEffects.empty()) {
        ss << "Filters: [none active]";
    } else {
        ss << "Filters: " << activeEffects;
    }
    return ss.str();
}
