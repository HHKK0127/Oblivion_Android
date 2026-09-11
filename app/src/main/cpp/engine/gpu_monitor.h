#pragma once

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <deque>
#include <functional>

// GPU vendor detection
enum class GPUVendor {
    Unknown,
    Adreno,     // Qualcomm
    Mali,       // ARM
    PowerVR,    // Imagination
    NVIDIA      // NVIDIA (Shield, etc.)
};

// GPU statistics
struct GPUStats {
    uint64_t timestamp;
    
    // Usage (%)
    float usagePercent;
    
    // Clock frequency (MHz)
    float clockFrequency;
    
    // Temperature (C) - when available
    float temperature;
    
    // Memory usage (MB)
    size_t memoryUsed;
    size_t memoryTotal;
    
    // Performance counters
    uint64_t frameCount;
    uint64_t drawCalls;
    uint64_t verticesSubmitted;
    uint64_t primitivesRendered;
    
    // Bandwidth (MB/s) - estimated
    float bandwidthRead;
    float bandwidthWrite;
    
    // Shader statistics
    uint32_t shaderSwitches;
    uint32_t textureBinds;
    
    bool valid;
    
    GPUStats() : timestamp(0), usagePercent(0), clockFrequency(0),
                 temperature(0), memoryUsed(0), memoryTotal(0),
                 frameCount(0), drawCalls(0), verticesSubmitted(0),
                 primitivesRendered(0), bandwidthRead(0), bandwidthWrite(0),
                 shaderSwitches(0), textureBinds(0), valid(false) {}
};

// GPU monitoring class
class GPUMonitor {
public:
    static GPUMonitor& getInstance();
    
    // Lifecycle
    bool initialize();
    void shutdown();
    
    // Monitoring
    void startMonitoring(int intervalMs = 100);
    void stopMonitoring();
    
    // Data access
    GPUStats getCurrentStats() const;
    std::deque<GPUStats> getHistory(size_t count = 60) const;
    
    // Statistics
    float getAverageUsage(size_t frames = 60) const;
    float getPeakUsage(size_t frames = 60) const;
    float getMinUsage(size_t frames = 60) const;
    
    // Warning thresholds
    void setWarningThreshold(float percent);
    void setCriticalThreshold(float percent);
    bool isWarning() const;
    bool isCritical() const;
    
    // Callbacks
    using WarningCallback = std::function<void(const GPUStats&)>;
    void setWarningCallback(WarningCallback cb);
    void setCriticalCallback(WarningCallback cb);
    
    // Frame markers (called from app)
    void markFrameStart();
    void markFrameEnd();
    void incrementDrawCall(int vertices = 0);
    void incrementTextureBind();
    void incrementShaderSwitch();
    
    // Vendor info
    GPUVendor getVendor() const;
    std::string getVendorString() const;
    std::string getGPUModel() const;
    
    // Available features
    bool supportsTemperature() const;
    bool supportsClockFrequency() const;
    bool supportsMemoryStats() const;
    
private:
    GPUMonitor() = default;
    ~GPUMonitor() = default;
    
    // Monitoring thread
    void monitoringThread();
    
    // Vendor-specific implementations
    bool detectGPU();
    bool initAdreno();
    bool initMali();
    bool initGeneric();
    
    GPUStats readAdrenoStats();
    GPUStats readMaliStats();
    GPUStats readGenericStats();
    GPUStats estimateFromTiming();
    
    // File reading helpers
    bool readFileInt(const std::string& path, int64_t& value);
    bool readFileFloat(const std::string& path, float& value);
    bool readFileString(const std::string& path, std::string& value);
    
    // State
    std::atomic<bool> initialized_{false};
    std::atomic<bool> monitoring_{false};
    std::thread monitorThread_;
    
    // Data
    mutable std::mutex statsMutex_;
    GPUStats currentStats_;
    std::deque<GPUStats> history_;
    static constexpr size_t MAX_HISTORY = 300; // 5 min @ 100ms
    
    // Thresholds
    float warningThreshold_ = 70.0f;
    float criticalThreshold_ = 90.0f;
    std::atomic<bool> warningActive_{false};
    std::atomic<bool> criticalActive_{false};
    
    // Callbacks
    WarningCallback warningCallback_;
    WarningCallback criticalCallback_;
    
    // GPU info
    GPUVendor vendor_ = GPUVendor::Unknown;
    std::string gpuModel_;
    std::string sysPath_; // /sys/class/kgsl/kgsl-3d0 etc.
    
    // Feature flags
    bool supportsTemp_ = false;
    bool supportsClock_ = false;
    bool supportsMem_ = false;
    
    // Frame metrics
    struct FrameMetrics {
        std::atomic<uint64_t> frameCount{0};
        std::atomic<uint64_t> drawCalls{0};
        std::atomic<uint64_t> vertices{0};
        std::atomic<uint64_t> textures{0};
        std::atomic<uint64_t> shaders{0};
        
        uint64_t lastFrameCount = 0;
        uint64_t lastDrawCalls = 0;
        uint64_t lastVertices = 0;
        
        uint64_t frameStartTime = 0;
    } metrics_;
    
    // Estimation
    uint64_t lastFrameTime_ = 0;
    float cpuTime_ = 0;
};
