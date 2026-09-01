#pragma once

#include <string>
#include <array>
#include <vector>
#include <glm/glm.hpp>
#include "text_renderer.h"

class AudioManager;
class Renderer;

/**
 * @brief Extended Debug HUD with graphs, breakdowns, and detailed stats
 *
 * Features:
 * - FPS graph (last 60 frames)
 * - Frame time breakdown (CPU/GPU/Wait)
 * - Memory details (Native/Java/Texture/Audio)
 * - Imperial Weave phase times
 * - Draw call / vertex / triangle counts
 * - In-game log display
 */
class DebugHUD {
public:
    DebugHUD();
    ~DebugHUD();

    bool initialize(TextRenderer* textRenderer, AudioManager* audioManager = nullptr, Renderer* renderer = nullptr);
    void update(float deltaTime);
    void render();
    void toggle();
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }
    void setAudioManager(AudioManager* audioMgr) { audioManager = audioMgr; }
    void cleanup();

    // --- Extended stats setters (called from Renderer/ImperialWeave) ---

    // Frame time breakdown (milliseconds)
    void setFrameTimeBreakdown(float cpuMs, float gpuMs, float waitMs);

    // Imperial Weave phase times (microseconds)
    static constexpr int PHASE_COUNT = 15;
    void setPhaseTime(int phaseIndex, float microseconds);

    // Rendering stats
    void setDrawCallCount(int count);
    void setVertexCount(int count);
    void setTriangleCount(int count);
    void setTextureMemory(long bytes);
    void setLoadedTextureCount(int count);

    // Memory breakdown
    void setNativeHeap(long bytes);
    void setJavaHeap(long bytes);

    // Log display
    void addLogLine(const std::string& line);
    void setLogVisible(bool v) { logVisible = v; }
    bool isLogVisible() const { return logVisible; }

    // Log management (for debug menu)
    void setLogLevel(const std::string& level) { logLevel = level; }
    void clearLogs() { logLines.clear(); }
    void exportLogs();
    std::string getLogStats() const;
    void searchLogs(const std::string& pattern);
    void toggleLogAutoScroll() { logAutoScroll = !logAutoScroll; }

    // Debug page navigation (multiple pages of info)
    void nextPage();
    void prevPage();
    int getCurrentPage() const { return currentPage; }
    int getTotalPages() const { return totalPages; }

private:
    TextRenderer* textRenderer;
    AudioManager* audioManager;
    Renderer* renderer;
    bool visible;
    bool logVisible;

    // Page system
    int currentPage;
    static constexpr int totalPages = 4;  // 0=Overview, 1=Performance, 2=Memory, 3=Phases

    // Basic stats
    float fps;
    float frameTimeMs;
    float avgFrameTimeMs;
    float minFrameTimeMs;
    float maxFrameTimeMs;

        uint64_t frameCount;  // Changed from int to prevent overflow (~584 years @ 60fps)
    float timeSinceLastUpdate;
        float timeSinceLastReset;  // For periodic min/max/avg reset
        static constexpr float UPDATE_INTERVAL = 0.5f;
        static constexpr float RESET_INTERVAL = 5.0f;  // Reset min/max/avg every 5 seconds

    // FPS graph (circular buffer)
    static constexpr int FPS_HISTORY_SIZE = 60;
    std::array<float, FPS_HISTORY_SIZE> fpsHistory;
    int fpsHistoryIndex;

    // Frame time breakdown
    float cpuTimeMs;
    float gpuTimeMs;
    float waitTimeMs;

    // Imperial Weave phase times
    std::array<float, PHASE_COUNT> phaseTimesUs;  // microseconds

    // Rendering stats
    int drawCallCount;
    int vertexCount;
    int triangleCount;
    long textureMemoryBytes;
    int loadedTextureCount;

    // Memory breakdown
    long nativeHeapBytes;
    long javaHeapBytes;

    // In-game log
    static constexpr int MAX_LOG_LINES = 20;
    std::vector<std::string> logLines;
    std::string logLevel = "all";
    bool logAutoScroll = true;

    // Memory info
    struct MemoryInfo {
        long totalMemory;
        long usedMemory;
        long freeMemory;
    };

    MemoryInfo getMemoryInfo() const;
    std::string formatMemorySize(long bytes) const;
    std::string getAudioStatus() const;
    std::string getRetroFilterStatus() const;

    // Render helpers for each page
    void renderOverviewPage(float& xPos, float& yPos, float lineHeight, float textScale);
    void renderPerformancePage(float& xPos, float& yPos, float lineHeight, float textScale);
    void renderMemoryPage(float& xPos, float& yPos, float lineHeight, float textScale);
    void renderPhasePage(float& xPos, float& yPos, float lineHeight, float textScale);
    void renderFpsGraph(float x, float y, float width, float height);
    void renderFrameTimeBar(float x, float y, float width, float height);
    void renderLogOverlay();
};
