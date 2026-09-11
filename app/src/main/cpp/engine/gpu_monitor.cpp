#include "gpu_monitor.h"
#include <GLES3/gl3.h>
#include <android/log.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <dirent.h>
#include <sys/stat.h>

#define LOG_TAG "GPUMonitor"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

GPUMonitor& GPUMonitor::getInstance() {
    static GPUMonitor instance;
    return instance;
}

bool GPUMonitor::initialize() {
    if (initialized_) return true;
    
    if (!detectGPU()) {
        LOGE("Failed to detect GPU");
        // Fallback: generic mode
        vendor_ = GPUVendor::Unknown;
        supportsTemp_ = false;
        supportsClock_ = false;
        supportsMem_ = false;
    }
    
    // Clear history
    history_.clear();
    
    initialized_ = true;
    LOGD("GPUMonitor initialized: %s", getVendorString().c_str());
    return true;
}

void GPUMonitor::shutdown() {
    stopMonitoring();
    initialized_ = false;
}

bool GPUMonitor::detectGPU() {
    // Detect from renderer string
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    if (!renderer) return false;
    
    std::string rend = renderer;
    gpuModel_ = rend;
    
    if (rend.find("Adreno") != std::string::npos) {
        vendor_ = GPUVendor::Adreno;
        return initAdreno();
    }
    else if (rend.find("Mali") != std::string::npos) {
        vendor_ = GPUVendor::Mali;
        return initMali();
    }
    else if (rend.find("PowerVR") != std::string::npos) {
        vendor_ = GPUVendor::PowerVR;
        return initGeneric();
    }
    else if (rend.find("NVIDIA") != std::string::npos) {
        vendor_ = GPUVendor::NVIDIA;
        return initGeneric();
    }
    
    return false;
}

bool GPUMonitor::initAdreno() {
    // Find Adreno sysfs path
    const std::vector<std::string> paths = {
        "/sys/class/kgsl/kgsl-3d0",
        "/sys/class/misc/kgsl-3d0",
        "/sys/devices/soc.0/1c00000.qcom,kgsl-3d0/kgsl/kgsl-3d0"
    };
    
    for (const auto& path : paths) {
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            sysPath_ = path;
            break;
        }
    }
    
    if (sysPath_.empty()) {
        LOGE("Adreno sysfs not found");
        return false;
    }
    
    // Feature check
    int64_t testVal;
    supportsClock_ = readFileInt(sysPath_ + "/clock_mhz", testVal);
    supportsTemp_ = readFileInt(sysPath_ + "/temp", testVal);
    supportsMem_ = readFileInt(sysPath_ + "/gpuclk", testVal); // memory clock
    
    LOGD("Adreno initialized: %s", sysPath_.c_str());
    LOGD("  Clock: %s", supportsClock_ ? "yes" : "no");
    LOGD("  Temp: %s", supportsTemp_ ? "yes" : "no");
    LOGD("  Mem: %s", supportsMem_ ? "yes" : "no");
    
    return true;
}

bool GPUMonitor::initMali() {
    // Find Mali sysfs path
    const std::vector<std::string> paths = {
        "/sys/class/misc/mali0",
        "/sys/class/gpufreq/mali",
        "/sys/devices/11800000.mali"
    };
    
    for (const auto& path : paths) {
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            sysPath_ = path;
            break;
        }
    }
    
    if (sysPath_.empty()) {
        // Mali can work without path found
        LOGD("Mali sysfs not found, using estimation");
    }
    
    // Mali usually supports temperature and clock
    supportsClock_ = true;
    supportsTemp_ = true;
    supportsMem_ = false; // Mali often doesn't expose memory stats
    
    return true;
}

bool GPUMonitor::initGeneric() {
    // Generic: timing-based estimation
    LOGD("Using generic GPU estimation");
    supportsClock_ = false;
    supportsTemp_ = false;
    supportsMem_ = false;
    return true;
}

void GPUMonitor::startMonitoring(int intervalMs) {
    if (monitoring_) return;
    
    monitorIntervalMs_ = intervalMs;
    monitoring_ = true;
    monitorThread_ = std::thread(&GPUMonitor::monitoringThread, this);
    
    LOGD("Monitoring started (%dms interval)", intervalMs);
}

void GPUMonitor::stopMonitoring() {
    monitoring_ = false;
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }
    LOGD("Monitoring stopped");
}

void GPUMonitor::monitoringThread() {
    while (monitoring_) {
        GPUStats stats;
        
        switch (vendor_) {
            case GPUVendor::Adreno:
                stats = readAdrenoStats();
                break;
            case GPUVendor::Mali:
                stats = readMaliStats();
                break;
            default:
                stats = readGenericStats();
                break;
        }
        
        // Add frame metrics
        stats.frameCount = metrics_.frameCount.load();
        stats.drawCalls = metrics_.drawCalls.load() - metrics_.lastDrawCalls;
        stats.verticesSubmitted = metrics_.vertices.load();
        stats.shaderSwitches = metrics_.shaders.load();
        stats.textureBinds = metrics_.textures.load();
        
        // Update deltas
        metrics_.lastDrawCalls = metrics_.drawCalls.load();
        
        // Store
        {
            std::lock_guard<std::mutex> lock(statsMutex_);
            currentStats_ = stats;
            history_.push_back(stats);
            if (history_.size() > MAX_HISTORY) {
                history_.pop_front();
            }
        }
        
        // Warning check
        if (stats.valid) {
            if (stats.usagePercent >= criticalThreshold_) {
                if (!criticalActive_) {
                    criticalActive_ = true;
                    if (criticalCallback_) criticalCallback_(stats);
                }
            } else if (stats.usagePercent >= warningThreshold_) {
                if (!warningActive_) {
                    warningActive_ = true;
                    if (warningCallback_) warningCallback_(stats);
                }
                criticalActive_ = false;
            } else {
                warningActive_ = false;
                criticalActive_ = false;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(monitorIntervalMs_));
    }
}

GPUStats GPUMonitor::readAdrenoStats() {
    GPUStats stats;
    stats.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Clock frequency
    int64_t clk = 0;
    if (supportsClock_ && readFileInt(sysPath_ + "/clock_mhz", clk)) {
        stats.clockFrequency = (float)clk;
    }
    
    // Usage calculation (Adreno specific)
    // Use /sys/class/kgsl/kgsl-3d0/gpu_busy_percentage if available
    int64_t busy = 0;
    if (readFileInt(sysPath_ + "/gpu_busy_percentage", busy)) {
        stats.usagePercent = (float)busy;
    } else {
        // Fallback: estimation
        stats = estimateFromTiming();
    }
    
    // Temperature
    int64_t temp = 0;
    if (supportsTemp_ && readFileInt(sysPath_ + "/temp", temp)) {
        stats.temperature = (float)temp / 1000.0f; // mC -> C
    }
    
    // Memory (estimated)
    if (supportsMem_) {
        GLint mem = 0;
        glGetIntegerv(0x9049 /* GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX */, &mem);
        if (mem > 0) {
            stats.memoryTotal = 1024; // estimated
            stats.memoryUsed = stats.memoryTotal - mem;
        }
    }
    
    stats.valid = true;
    return stats;
}

GPUStats GPUMonitor::readMaliStats() {
    GPUStats stats;
    stats.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Mali often doesn't expose direct usage
    // Estimate from clock frequency and load
    
    int64_t clk = 0;
    if (readFileInt(sysPath_ + "/clock", clk)) {
        stats.clockFrequency = (float)clk / 1000000.0f; // Hz -> MHz
    }
    
    // Get usage estimation
    GPUStats estimated = estimateFromTiming();
    stats.usagePercent = estimated.usagePercent;
    
    // Temperature
    int64_t temp = 0;
    if (readFileInt("/sys/class/thermal/thermal_zone0/temp", temp)) {
        stats.temperature = (float)temp / 1000.0f;
    }
    
    stats.valid = true;
    return stats;
}

GPUStats GPUMonitor::readGenericStats() {
    // Generic: estimate from frame timing
    return estimateFromTiming();
}

GPUStats GPUMonitor::estimateFromTiming() {
    GPUStats stats;
    stats.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Estimate GPU time
    // Estimate GPU load as part of frame time
    uint64_t now = stats.timestamp;
    if (lastFrameTime_ > 0) {
        float frameTime = (now - lastFrameTime_) / 1000.0f; // ms
        
        // Simple estimation: estimate GPU load from draw calls and vertices
        float estimatedLoad = 0.0f;
        
        uint64_t draws = metrics_.drawCalls.load() - metrics_.lastDrawCalls;
        uint64_t verts = metrics_.vertices.load();
        
        // Heuristic: 100 draw calls/frame ~ 30% load
        estimatedLoad += draws * 0.3f;
        // 1 million vertices/frame ~ 20% load
        estimatedLoad += (verts / 1000000.0f) * 20.0f;
        
        // Also estimate from frame time (16.6ms = 60fps baseline)
        if (frameTime > 0) {
            float fps = 1000.0f / frameTime;
            float loadFromFps = (60.0f - std::min(fps, 60.0f)) / 60.0f * 100.0f;
            estimatedLoad = std::max(estimatedLoad, loadFromFps);
        }
        
        stats.usagePercent = std::min(estimatedLoad, 100.0f);
    }
    lastFrameTime_ = now;
    
    // Estimated values, so mark as valid but approximate
    stats.valid = true;
    return stats;
}

// File reading helpers
bool GPUMonitor::readFileInt(const std::string& path, int64_t& value) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    file >> value;
    return !file.fail();
}

bool GPUMonitor::readFileFloat(const std::string& path, float& value) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    file >> value;
    return !file.fail();
}

bool GPUMonitor::readFileString(const std::string& path, std::string& value) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    
    std::getline(file, value);
    return !file.fail();
}

// Public API
GPUStats GPUMonitor::getCurrentStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return currentStats_;
}

std::deque<GPUStats> GPUMonitor::getHistory(size_t count) const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    std::deque<GPUStats> result;
    size_t start = history_.size() > count ? history_.size() - count : 0;
    
    for (size_t i = start; i < history_.size(); i++) {
        result.push_back(history_[i]);
    }
    return result;
}

float GPUMonitor::getAverageUsage(size_t frames) const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    if (history_.empty()) return 0.0f;
    
    size_t count = std::min(frames, history_.size());
    float sum = 0.0f;
    
    auto it = history_.rbegin();
    for (size_t i = 0; i < count && it != history_.rend(); i++, ++it) {
        sum += it->usagePercent;
    }
    
    return sum / count;
}

float GPUMonitor::getPeakUsage(size_t frames) const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    if (history_.empty()) return 0.0f;
    
    size_t count = std::min(frames, history_.size());
    float peak = 0.0f;
    
    auto it = history_.rbegin();
    for (size_t i = 0; i < count && it != history_.rend(); i++, ++it) {
        peak = std::max(peak, it->usagePercent);
    }
    
    return peak;
}

float GPUMonitor::getMinUsage(size_t frames) const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    if (history_.empty()) return 100.0f;
    
    size_t count = std::min(frames, history_.size());
    float minVal = 100.0f;
    
    auto it = history_.rbegin();
    for (size_t i = 0; i < count && it != history_.rend(); i++, ++it) {
        minVal = std::min(minVal, it->usagePercent);
    }
    
    return minVal;
}

void GPUMonitor::setWarningThreshold(float percent) {
    warningThreshold_ = percent;
}

void GPUMonitor::setCriticalThreshold(float percent) {
    criticalThreshold_ = percent;
}

bool GPUMonitor::isWarning() const {
    return warningActive_;
}

bool GPUMonitor::isCritical() const {
    return criticalActive_;
}

void GPUMonitor::setWarningCallback(WarningCallback cb) {
    warningCallback_ = cb;
}

void GPUMonitor::setCriticalCallback(WarningCallback cb) {
    criticalCallback_ = cb;
}

// Frame markers
void GPUMonitor::markFrameStart() {
    metrics_.frameStartTime = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void GPUMonitor::markFrameEnd() {
    metrics_.frameCount++;
}

void GPUMonitor::incrementDrawCall(int vertices) {
    metrics_.drawCalls++;
    metrics_.vertices += vertices;
}

void GPUMonitor::incrementTextureBind() {
    metrics_.textures++;
}

void GPUMonitor::incrementShaderSwitch() {
    metrics_.shaders++;
}

// Info access
GPUVendor GPUMonitor::getVendor() const {
    return vendor_;
}

std::string GPUMonitor::getVendorString() const {
    switch (vendor_) {
        case GPUVendor::Adreno: return "Adreno";
        case GPUVendor::Mali: return "Mali";
        case GPUVendor::PowerVR: return "PowerVR";
        case GPUVendor::NVIDIA: return "NVIDIA";
        default: return "Unknown";
    }
}

std::string GPUMonitor::getGPUModel() const {
    return gpuModel_;
}

bool GPUMonitor::supportsTemperature() const {
    return supportsTemp_;
}

bool GPUMonitor::supportsClockFrequency() const {
    return supportsClock_;
}

bool GPUMonitor::supportsMemoryStats() const {
    return supportsMem_;
}
