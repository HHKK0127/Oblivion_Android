#include "performance_profiler.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <android/log.h>

// ============================================================================
// PerformanceProfiler implementation
// ============================================================================

PerformanceProfiler::PerformanceProfiler() = default;

PerformanceProfiler::~PerformanceProfiler() {
    shutdown();
}

bool PerformanceProfiler::initialize() {
    resetStats();
    LOGI("PerformanceProfiler initialized");
    return true;
}

void PerformanceProfiler::shutdown() {
    systemTimings_.clear();
    activeTimers_.clear();
    LOGI("PerformanceProfiler shutdown");
}

void PerformanceProfiler::beginFrame() {
    if (!enabled_) return;
    frameStart_ = std::chrono::high_resolution_clock::now();
    drawCallCount_ = 0;
}

void PerformanceProfiler::endFrame() {
    if (!enabled_) return;

    auto frameEnd = std::chrono::high_resolution_clock::now();
    frameTimeMs_ = std::chrono::duration<float, std::milli>(frameEnd - frameStart_).count();

    // Update FPS
    fpsAccumulator_ += frameTimeMs_;
    fpsFrameCount_++;

    if (fpsAccumulator_ >= FPS_UPDATE_INTERVAL * 1000.0f) {
        fpsStats_.current = static_cast<float>(fpsFrameCount_) / (fpsAccumulator_ / 1000.0f);
        fpsStats_.frameCount += fpsFrameCount_;

        if (fpsStats_.current < fpsStats_.min) fpsStats_.min = fpsStats_.current;
        if (fpsStats_.current > fpsStats_.max) fpsStats_.max = fpsStats_.current;

        // Running average
        float totalFrames = static_cast<float>(fpsStats_.frameCount);
        fpsStats_.average = (fpsStats_.average * (totalFrames - fpsFrameCount_) +
                            fpsStats_.current * fpsFrameCount_) / totalFrames;

        // Count dropped frames (below 30 FPS threshold)
        if (fpsStats_.current < 30.0f) {
            fpsStats_.droppedFrames++;
        }

        fpsAccumulator_ = 0.0f;
        fpsFrameCount_ = 0;
    }
}

void PerformanceProfiler::beginSystemTiming(const std::string& systemName) {
    if (!enabled_) return;
    activeTimers_[systemName] = std::chrono::high_resolution_clock::now();
}

void PerformanceProfiler::endSystemTiming(const std::string& systemName) {
    if (!enabled_) return;

    auto it = activeTimers_.find(systemName);
    if (it == activeTimers_.end()) return;

    auto end = std::chrono::high_resolution_clock::now();
    float ms = std::chrono::duration<float, std::milli>(end - it->second).count();

    systemTimings_[systemName].name = systemName;
    systemTimings_[systemName].record(ms);

    activeTimers_.erase(it);
}

void PerformanceProfiler::updateMemoryInfo() {
    readAndroidMemory();
}

std::string PerformanceProfiler::generateReport() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    oss << "=== Performance Report ===\n";
    oss << "FPS: " << fpsStats_.current
        << " (avg: " << fpsStats_.average
        << " min: " << fpsStats_.min
        << " max: " << fpsStats_.max << ")\n";
    oss << "Frame time: " << frameTimeMs_ << " ms\n";
    oss << "Dropped frames: " << fpsStats_.droppedFrames << "\n";
    oss << "Draw calls: " << drawCallCount_ << "\n";

    oss << "\n--- Memory ---\n";
    oss << "Total PSS: " << (memoryInfo_.totalPss / 1024) << " MB\n";
    oss << "Native Heap: " << (memoryInfo_.nativeHeap / 1024) << " MB\n";
    oss << "Java Heap: " << (memoryInfo_.javaHeap / 1024) << " MB\n";
    oss << "Graphics: " << (memoryInfo_.graphics / 1024) << " MB\n";

    oss << "\n--- System Timings ---\n";
    for (const auto& pair : systemTimings_) {
        const auto& t = pair.second;
        oss << std::setw(16) << std::left << t.name
            << ": last=" << std::setw(8) << t.lastMs
            << " avg=" << std::setw(8) << t.avgMs
            << " min=" << std::setw(8) << t.minMs
            << " max=" << std::setw(8) << t.maxMs
            << " ms\n";
    }

    return oss.str();
}

void PerformanceProfiler::resetStats() {
    fpsStats_ = FPSStats{};
    fpsAccumulator_ = 0.0f;
    fpsFrameCount_ = 0;
    frameTimeMs_ = 0.0f;
    drawCallCount_ = 0;

    for (auto& pair : systemTimings_) {
        pair.second.reset();
    }
}

void PerformanceProfiler::readAndroidMemory() {
    // Read /proc/self/status for memory info on Android
    std::ifstream statusFile("/proc/self/status");
    if (!statusFile.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(statusFile, line)) {
        if (line.find("VmRSS:") == 0) {
            // VmRSS gives resident set size in KB
            std::istringstream iss(line);
            std::string label;
            size_t value;
            iss >> label >> value;
            memoryInfo_.nativeHeap = value;
        }
    }

    // Try to read smaps for PSS
    std::ifstream smapsFile("/proc/self/smaps_rollup");
    if (smapsFile.is_open()) {
        std::string line;
        while (std::getline(smapsFile, line)) {
            if (line.find("Pss:") == 0) {
                std::istringstream iss(line);
                std::string label;
                size_t value;
                iss >> label >> value;
                memoryInfo_.totalPss = value;
                break;
            }
        }
    }
}
