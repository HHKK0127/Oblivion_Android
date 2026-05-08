#include "performance_monitor.h"
#include <numeric>
#include <fstream>
#include <sstream>

// ============================================================================
// FrameTimer Implementation
// ============================================================================

FrameTimer::FrameTimer()
    : frameTime(0.0f), cpuTime(0.0f), renderTime(0.0f), updateTime(0.0f) {
    LOGD("FrameTimer created");
}

void FrameTimer::startFrame() {
    frameStart = std::chrono::high_resolution_clock::now();
    lastFrameTime = frameStart;
}

void FrameTimer::endFrame() {
    auto frameEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> elapsed = frameEnd - frameStart;
    frameTime = elapsed.count();
}

void FrameTimer::startMeasure(const std::string& label) {
    measureStart = std::chrono::high_resolution_clock::now();
}

void FrameTimer::endMeasure(const std::string& label) {
    auto measureEnd = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> elapsed = measureEnd - measureStart;

    if (label == "cpu") {
        cpuTime = elapsed.count();
    } else if (label == "render") {
        renderTime = elapsed.count();
    } else if (label == "update") {
        updateTime = elapsed.count();
    }
}

FrameMetrics FrameTimer::getMetrics() const {
    FrameMetrics metrics;
    metrics.frameTime = frameTime;
    metrics.cpuTime = cpuTime;
    metrics.renderTime = renderTime;
    metrics.updateTime = updateTime;
    metrics.fps = (frameTime > 0) ? 1000.0f / frameTime : 0.0f;
    metrics.droppedFrames = (frameTime > 33.33f) ? 1 : 0;  // 30 fps threshold
    return metrics;
}

// ============================================================================
// MemoryProfiler Implementation
// ============================================================================

MemoryProfiler::MemoryProfiler()
    : allocationCount(0) {
    LOGD("MemoryProfiler created");
    currentMetrics = {};
    peakMetrics = {};
}

void MemoryProfiler::updateMetrics() {
    readProcStats();
}

void MemoryProfiler::readProcStats() {
    // Read from /proc/self/status for memory information
    // This is Android-specific and requires reading proc filesystem

    std::ifstream statusFile("/proc/self/status");
    if (!statusFile.is_open()) {
        LOGW("Could not open /proc/self/status");
        return;
    }

    std::string line;
    size_t vmPeak = 0, vmHWM = 0, vmRSS = 0, vmSize = 0;

    while (std::getline(statusFile, line)) {
        if (line.find("VmSize:") == 0) {
            unsigned long value = 0;
            if (sscanf(line.c_str(), "VmSize: %lu", &value) == 1) {
                vmSize = value;
            }
        } else if (line.find("VmRSS:") == 0) {
            unsigned long value = 0;
            if (sscanf(line.c_str(), "VmRSS: %lu", &value) == 1) {
                vmRSS = value;
            }
        } else if (line.find("VmHWM:") == 0) {
            unsigned long value = 0;
            if (sscanf(line.c_str(), "VmHWM: %lu", &value) == 1) {
                vmHWM = value;
            }
        } else if (line.find("VmPeak:") == 0) {
            unsigned long value = 0;
            if (sscanf(line.c_str(), "VmPeak: %lu", &value) == 1) {
                vmPeak = value;
            }
        }
    }
    statusFile.close();

    // Convert KB to bytes
    currentMetrics.heapSize = vmSize * 1024;
    currentMetrics.heapUsed = vmRSS * 1024;
    currentMetrics.heapFree = (vmSize >= vmRSS) ? (vmSize - vmRSS) * 1024 : 0;

    if (currentMetrics.heapSize > 0) {
        currentMetrics.heapPercentage = (currentMetrics.heapUsed * 100.0f) / currentMetrics.heapSize;
    } else {
        currentMetrics.heapPercentage = 0.0f;
    }

    currentMetrics.peakMemory = vmHWM * 1024;
    currentMetrics.allocationCount = allocationCount;

    // Track peak metrics
    if (currentMetrics.heapUsed > peakMetrics.heapUsed) {
        peakMetrics = currentMetrics;
    }
}

size_t MemoryProfiler::getHeapSize() const {
    return currentMetrics.heapSize;
}

size_t MemoryProfiler::getHeapUsed() const {
    return currentMetrics.heapUsed;
}

float MemoryProfiler::getHeapPercentage() const {
    return currentMetrics.heapPercentage;
}

void MemoryProfiler::logMemoryStatus() const {
    LOGD("========== Memory Status ==========");
    LOGD("Heap Size: %zu KB", currentMetrics.heapSize / 1024);
    LOGD("Heap Used: %zu KB", currentMetrics.heapUsed / 1024);
    LOGD("Heap Free: %zu KB", currentMetrics.heapFree / 1024);
    LOGD("Usage: %.1f%%", currentMetrics.heapPercentage);
    LOGD("Peak Memory: %zu KB", currentMetrics.peakMemory / 1024);
    LOGD("===================================");
}

// ============================================================================
// PerformanceMonitor Implementation
// ============================================================================

PerformanceMonitor::PerformanceMonitor()
    : frameTimer(nullptr), memoryProfiler(nullptr),
      frameCount(0), isEnabled(true) {
    LOGD("PerformanceMonitor created");
    frameTimes.reserve(MAX_SAMPLES);
}

PerformanceMonitor::~PerformanceMonitor() {
    cleanup();
    LOGD("PerformanceMonitor destroyed");
}

void PerformanceMonitor::initialize() {
    LOGI("PerformanceMonitor initializing");

    frameTimer = std::make_unique<FrameTimer>();
    memoryProfiler = std::make_unique<MemoryProfiler>();

    if (!frameTimer || !memoryProfiler) {
        LOGE("Failed to initialize PerformanceMonitor components");
        isEnabled = false;
        return;
    }

    frameCount = 0;
    frameTimes.clear();
    isEnabled = true;

    LOGI("PerformanceMonitor initialized successfully");
}

void PerformanceMonitor::cleanup() {
    LOGI("PerformanceMonitor cleaning up");

    if (memoryProfiler) {
        memoryProfiler.reset();
    }
    if (frameTimer) {
        frameTimer.reset();
    }

    frameTimes.clear();
    frameCount = 0;
}

void PerformanceMonitor::beginFrame() {
    if (!isEnabled || !frameTimer) return;

    frameTimer->startFrame();
    frameTimer->startMeasure("cpu");
}

void PerformanceMonitor::endFrame() {
    if (!isEnabled || !frameTimer) return;

    frameTimer->endMeasure("cpu");
    frameTimer->endFrame();

    // Track frame time
    float currentFrameTime = frameTimer->getFrameTime();
    if (frameTimes.size() < MAX_SAMPLES) {
        frameTimes.push_back(currentFrameTime);
    } else {
        frameTimes[frameCount % MAX_SAMPLES] = currentFrameTime;
    }

    frameCount++;
}

void PerformanceMonitor::recordCpuTime(float time) {
    if (!isEnabled) return;
    // CPU time already recorded by FrameTimer
}

void PerformanceMonitor::recordRenderTime(float time) {
    if (!isEnabled || !frameTimer) return;
    frameTimer->endMeasure("render");
}

void PerformanceMonitor::recordUpdateTime(float time) {
    if (!isEnabled || !frameTimer) return;
    frameTimer->endMeasure("update");
}

PerformanceMetrics PerformanceMonitor::getMetrics() const {
    PerformanceMetrics metrics = {};

    if (!frameTimer || !memoryProfiler || !isEnabled) {
        return metrics;
    }

    // Frame metrics
    metrics.frameMetrics = frameTimer->getMetrics();

    // Memory metrics
    metrics.memoryMetrics = memoryProfiler->getMetrics();

    // Aggregate frame timing
    if (!frameTimes.empty()) {
        metrics.totalFrames = frameCount;
        metrics.avgFrameTime = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f) / frameTimes.size();
        metrics.minFrameTime = *std::min_element(frameTimes.begin(), frameTimes.end());
        metrics.maxFrameTime = *std::max_element(frameTimes.begin(), frameTimes.end());
    }

    // CPU usage (approximation based on frame time)
    metrics.cpuUsagePercent = (metrics.avgFrameTime / 16.67f) * 100.0f;  // 60 fps baseline

    return metrics;
}

float PerformanceMonitor::getFPS() const {
    if (!frameTimer || !isEnabled) return 0.0f;
    return frameTimer->getMetrics().fps;
}

float PerformanceMonitor::getAverageFrameTime() const {
    if (frameTimes.empty()) return 0.0f;
    return std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f) / frameTimes.size();
}

void PerformanceMonitor::logPerformanceReport() const {
    if (!isEnabled) return;

    LOGD("========== Performance Report ==========");
    LOGD("Total Frames: %d", frameCount);
    LOGD("Current FPS: %.1f", getFPS());
    LOGD("Average Frame Time: %.2f ms", getAverageFrameTime());

    if (!frameTimes.empty()) {
        LOGD("Min Frame Time: %.2f ms", *std::min_element(frameTimes.begin(), frameTimes.end()));
        LOGD("Max Frame Time: %.2f ms", *std::max_element(frameTimes.begin(), frameTimes.end()));
    }

    if (memoryProfiler) {
        LOGD("Memory Usage: %.1f%%", memoryProfiler->getHeapPercentage());
        LOGD("=========================================");
    }
}

void PerformanceMonitor::logDetailedMetrics() const {
    if (!isEnabled) return;

    PerformanceMetrics metrics = getMetrics();

    LOGD("========== Detailed Metrics ==========");
    LOGD("[Frame Metrics]");
    LOGD("  Current FPS: %.1f", metrics.frameMetrics.fps);
    LOGD("  Frame Time: %.2f ms", metrics.frameMetrics.frameTime);
    LOGD("  CPU Time: %.2f ms", metrics.frameMetrics.cpuTime);
    LOGD("  Render Time: %.2f ms", metrics.frameMetrics.renderTime);
    LOGD("  Update Time: %.2f ms", metrics.frameMetrics.updateTime);
    LOGD("  Dropped Frames: %d", metrics.frameMetrics.droppedFrames);

    LOGD("[Memory Metrics]");
    LOGD("  Heap Size: %zu KB", metrics.memoryMetrics.heapSize / 1024);
    LOGD("  Heap Used: %zu KB", metrics.memoryMetrics.heapUsed / 1024);
    LOGD("  Usage: %.1f%%", metrics.memoryMetrics.heapPercentage);
    LOGD("  Peak Memory: %zu KB", metrics.memoryMetrics.peakMemory / 1024);

    LOGD("[Aggregate Metrics]");
    LOGD("  Avg Frame Time: %.2f ms", metrics.avgFrameTime);
    LOGD("  Min Frame Time: %.2f ms", metrics.minFrameTime);
    LOGD("  Max Frame Time: %.2f ms", metrics.maxFrameTime);
    LOGD("  CPU Usage: %.1f%%", metrics.cpuUsagePercent);
    LOGD("  Total Frames: %d", metrics.totalFrames);
    LOGD("=====================================");
}
