#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <chrono>
#include <mutex>
#include <android/log.h>

#define LOG_TAG_PROFILER "ProfilerDashboard"
#define LOGD_PD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_PROFILER, __VA_ARGS__)
#define LOGI_PD(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_PROFILER, __VA_ARGS__)
#define LOGW_PD(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_PROFILER, __VA_ARGS__)
#define LOGE_PD(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_PROFILER, __VA_ARGS__)

// Forward declarations
class TextRenderer;
class MemoryPoolManager;
class RenderOptimizer;
class AsyncTaskManager;
class CacheManager;

// ============================================================================
// Profiler Dashboard - Real-time performance monitoring with graphs
// ============================================================================

class ProfilerDashboard {
public:
    // ========================================================================
    // Graph data ring buffer
    // ========================================================================

    static constexpr int GRAPH_HISTORY_SIZE = 120;  // 2 seconds at 60fps

    struct GraphData {
        std::array<float, GRAPH_HISTORY_SIZE> values = {};
        int writeIndex = 0;
        float minValue = 0.0f;
        float maxValue = 100.0f;
        float currentValue = 0.0f;

        void push(float value) {
            currentValue = value;
            values[writeIndex] = value;
            writeIndex = (writeIndex + 1) % GRAPH_HISTORY_SIZE;
        }

        float getAverage() const {
            float sum = 0.0f;
            for (int i = 0; i < GRAPH_HISTORY_SIZE; ++i) {
                sum += values[i];
            }
            return sum / static_cast<float>(GRAPH_HISTORY_SIZE);
        }

        float getMax() const {
            float maxVal = values[0];
            for (int i = 1; i < GRAPH_HISTORY_SIZE; ++i) {
                if (values[i] > maxVal) maxVal = values[i];
            }
            return maxVal;
        }
    };

    // ========================================================================
    // System timing data
    // ========================================================================

    struct SystemTiming {
        float worldUpdateMs = 0.0f;
        float renderMs = 0.0f;
        float physicsMs = 0.0f;
        float aiUpdateMs = 0.0f;
        float uiUpdateMs = 0.0f;
        float audioMs = 0.0f;
        float totalMs = 0.0f;
    };

    // ========================================================================
    // Dashboard configuration
    // ========================================================================

    struct Config {
        bool showFPSGraph = true;
        bool showMemoryGraph = true;
        bool showDrawCalls = true;
        bool showSystemTimings = true;
        bool showCacheStats = true;
        bool showPoolStats = true;
        bool showTaskStats = true;
        float updateIntervalSec = 0.25f;
        int graphWidth = 120;
        int graphHeight = 30;
    };

    ProfilerDashboard();
    ~ProfilerDashboard();

    // ========================================================================
    // Lifecycle
    // ========================================================================

    bool initialize(TextRenderer* textRenderer);
    void cleanup();

    // ========================================================================
    // System references (optional, for detailed stats)
    // ========================================================================

    void setMemoryPoolManager(MemoryPoolManager* pool) { memoryPool_ = pool; }
    void setRenderOptimizer(RenderOptimizer* optimizer) { renderOptimizer_ = optimizer; }
    void setAsyncTaskManager(AsyncTaskManager* taskMgr) { asyncTaskMgr_ = taskMgr; }
    void setCacheManager(CacheManager* cacheMgr) { cacheMgr_ = cacheMgr; }

    // ========================================================================
    // Data input
    // ========================================================================

    // Record frame timing
    void recordFrame(float frameTimeMs);

    // Record system timing breakdown
    void recordSystemTiming(const SystemTiming& timing);

    // Record draw call count
    void recordDrawCalls(uint32_t count);

    // Record memory usage
    void recordMemoryUsage(size_t usedBytes, size_t totalBytes);

    // ========================================================================
    // Update and render
    // ========================================================================

    void update(float deltaTime);
    void render();

    // ========================================================================
    // Visibility
    // ========================================================================

    void toggle() { visible_ = !visible_; }
    bool isVisible() const { return visible_; }
    void setVisible(bool v) { visible_ = v; }

    // ========================================================================
    // Configuration
    // ========================================================================

    void setConfig(const Config& config) { config_ = config; }
    Config& getConfig() { return config_; }

    // ========================================================================
    // Getters for external access
    // ========================================================================

    float getCurrentFPS() const { return fpsGraph_.currentValue; }
    float getAverageFPS() const { return fpsGraph_.getAverage(); }
    const SystemTiming& getLastTiming() const { return lastTiming_; }

private:
    // ========================================================================
    // Member variables
    // ========================================================================

    TextRenderer* textRenderer_ = nullptr;
    MemoryPoolManager* memoryPool_ = nullptr;
    RenderOptimizer* renderOptimizer_ = nullptr;
    AsyncTaskManager* asyncTaskMgr_ = nullptr;
    CacheManager* cacheMgr_ = nullptr;

    bool visible_ = false;
    bool initialized_ = false;
    Config config_;

    // Graph data
    GraphData fpsGraph_;
    GraphData frameTimeGraph_;
    GraphData memoryGraph_;
    GraphData drawCallGraph_;

    // System timing
    SystemTiming lastTiming_;
    SystemTiming avgTiming_;
    int timingSampleCount_ = 0;

    // Draw calls
    uint32_t currentDrawCalls_ = 0;
    uint32_t peakDrawCalls_ = 0;

    // Memory
    size_t currentMemoryUsed_ = 0;
    size_t currentMemoryTotal_ = 0;
    size_t peakMemoryUsed_ = 0;

    // Update timing
    float timeSinceUpdate_ = 0.0f;

    // Frame counting for FPS
    int frameCount_ = 0;
    float fpsAccumulator_ = 0.0f;

    // ========================================================================
    // Rendering helpers
    // ========================================================================

    void renderGraph(const GraphData& graph, int x, int y, int width, int height,
                     const std::string& label, const std::string& unit);

    void renderText(int x, int y, const std::string& text, float scale = 1.0f);

    void renderBar(int x, int y, int width, int height, float fillRatio,
                   uint32_t color);

    // ========================================================================
    // Data collection
    // ========================================================================

    void collectPoolStats();
    void collectCacheStats();
    void collectTaskStats();
    void updateAverages();
};
