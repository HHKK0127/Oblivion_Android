#pragma once

#include <cstdint>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <android/log.h>

#define LOG_TAG "PerformanceProfiler"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// PerformanceProfiler - Runtime performance monitoring
// Phase 42: Full game loop integration
// ============================================================================

// Per-system timing sample
struct SystemTiming {
    std::string name;
    float lastMs = 0.0f;
    float avgMs = 0.0f;
    float minMs = 999999.0f;
    float maxMs = 0.0f;
    uint32_t sampleCount = 0;

    void record(float ms) {
        lastMs = ms;
        if (ms < minMs) minMs = ms;
        if (ms > maxMs) maxMs = ms;
        // Running average
        avgMs = (avgMs * sampleCount + ms) / (sampleCount + 1);
        sampleCount++;
    }

    void reset() {
        lastMs = 0.0f;
        avgMs = 0.0f;
        minMs = 999999.0f;
        maxMs = 0.0f;
        sampleCount = 0;
    }
};

// Memory info (Android-specific)
struct MemoryInfo {
    size_t totalPss = 0;        // Total PSS in KB
    size_t nativeHeap = 0;      // Native heap in KB
    size_t javaHeap = 0;        // Java heap in KB
    size_t graphics = 0;        // Graphics memory in KB
};

// FPS statistics
struct FPSStats {
    float current = 0.0f;
    float average = 0.0f;
    float min = 999.0f;
    float max = 0.0f;
    uint32_t frameCount = 0;
    uint32_t droppedFrames = 0;
};

class PerformanceProfiler {
public:
    PerformanceProfiler();
    ~PerformanceProfiler();

    bool initialize();
    void shutdown();

    // Frame lifecycle
    void beginFrame();
    void endFrame();

    // System timing
    void beginSystemTiming(const std::string& systemName);
    void endSystemTiming(const std::string& systemName);

    // FPS
    const FPSStats& getFPSStats() const { return fpsStats_; }
    float getCurrentFPS() const { return fpsStats_.current; }

    // Memory
    void updateMemoryInfo();
    const MemoryInfo& getMemoryInfo() const { return memoryInfo_; }

    // System timings
    const std::unordered_map<std::string, SystemTiming>& getSystemTimings() const {
        return systemTimings_;
    }

    // Draw call tracking
    void recordDrawCall() { drawCallCount_++; }
    uint32_t getDrawCallCount() const { return drawCallCount_; }

    // Report generation
    std::string generateReport() const;

    // Reset statistics
    void resetStats();

    // Enable/disable
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

private:
    bool enabled_ = true;

    // Frame timing
    std::chrono::high_resolution_clock::time_point frameStart_;
    float frameTimeMs_ = 0.0f;

    // FPS tracking
    FPSStats fpsStats_;
    float fpsAccumulator_ = 0.0f;
    uint32_t fpsFrameCount_ = 0;
    static constexpr float FPS_UPDATE_INTERVAL = 1.0f; // Update FPS every second

    // System timings
    std::unordered_map<std::string, SystemTiming> systemTimings_;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> activeTimers_;

    // Memory
    MemoryInfo memoryInfo_;

    // Draw calls
    uint32_t drawCallCount_ = 0;

    // Android memory reading
    void readAndroidMemory();
};
