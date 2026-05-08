#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <android/log.h>

#define LOG_TAG "PerformanceMonitor"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct FrameMetrics {
    float frameTime;        // ミリ秒
    float cpuTime;          // ミリ秒
    float renderTime;       // ミリ秒
    float updateTime;       // ミリ秒
    float fps;              // フレームレート
    int droppedFrames;      // フレームドロップ数
};

struct MemoryMetrics {
    size_t heapSize;        // 総メモリ
    size_t heapUsed;        // 使用メモリ
    size_t heapFree;        // 空きメモリ
    float heapPercentage;   // 使用率（%）
    size_t peakMemory;      // ピークメモリ
    size_t allocationCount; // 割り当て回数
};

struct PerformanceMetrics {
    FrameMetrics frameMetrics;
    MemoryMetrics memoryMetrics;
    float avgFrameTime;
    float minFrameTime;
    float maxFrameTime;
    float cpuUsagePercent;
    int totalFrames;
};

class FrameTimer {
public:
    FrameTimer();

    void startFrame();
    void endFrame();
    void startMeasure(const std::string& label);
    void endMeasure(const std::string& label);

    float getFrameTime() const { return frameTime; }
    float getCpuTime() const { return cpuTime; }
    float getRenderTime() const { return renderTime; }
    float getUpdateTime() const { return updateTime; }

    FrameMetrics getMetrics() const;

private:
    std::chrono::high_resolution_clock::time_point frameStart;
    std::chrono::high_resolution_clock::time_point measureStart;
    std::chrono::high_resolution_clock::time_point lastFrameTime;

    float frameTime;
    float cpuTime;
    float renderTime;
    float updateTime;
};

class MemoryProfiler {
public:
    MemoryProfiler();

    void updateMetrics();
    MemoryMetrics getMetrics() const { return currentMetrics; }

    size_t getHeapSize() const;
    size_t getHeapUsed() const;
    float getHeapPercentage() const;

    void logMemoryStatus() const;

private:
    MemoryMetrics currentMetrics;
    MemoryMetrics peakMetrics;
    size_t allocationCount;

    void readProcStats();
};

class PerformanceMonitor {
public:
    PerformanceMonitor();
    ~PerformanceMonitor();

    void initialize();
    void beginFrame();
    void endFrame();
    void recordCpuTime(float time);
    void recordRenderTime(float time);
    void recordUpdateTime(float time);

    PerformanceMetrics getMetrics() const;
    float getFPS() const;
    float getAverageFrameTime() const;

    void logPerformanceReport() const;
    void logDetailedMetrics() const;

private:
    std::unique_ptr<FrameTimer> frameTimer;
    std::unique_ptr<MemoryProfiler> memoryProfiler;

    std::vector<float> frameTimes;
    static constexpr int MAX_SAMPLES = 300;  // 5秒分（60 fps）
    int frameCount;
    bool isEnabled;

    float calculateAverageFrameTime() const;
    float calculateFPS() const;
};
