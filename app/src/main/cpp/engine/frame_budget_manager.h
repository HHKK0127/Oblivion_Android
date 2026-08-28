#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG_FBM "FrameBudgetManager"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_FBM(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_FBM, __VA_ARGS__)
#else
#define LOGD_FBM(...) do {} while(0)
#endif
#define LOGI_FBM(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_FBM, __VA_ARGS__)
#define LOGW_FBM(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_FBM, __VA_ARGS__)

// ============================================================================
// Frame Budget Manager
// Phase 55: Dynamic quality scaling based on frame time budget
// Integrates with Imperial Weave v4.0 15-phase pipeline
// ============================================================================

namespace engine {

// Quality levels from highest to lowest
enum class QualityLevel : uint8_t {
    ULTRA = 0,
    HIGH = 1,
    MEDIUM = 2,
    LOW = 3,
    POTATO = 4,
    COUNT = 5
};

// Phase priority determines which phases get cut first
enum class PhasePriority : uint8_t {
    CRITICAL = 0,   // Never skip: physics, render submit, event process
    HIGH = 1,       // Skip only at POTATO: AI, animation, player
    MEDIUM = 2,     // Skip at LOW: vegetation, facegen, combat
    LOW = 3         // Skip at MEDIUM: video, audio, script
};

// Per-phase timing record
struct PhaseRecord {
    std::string name;
    PhasePriority priority;
    float lastMs = 0.0f;
    float avgMs = 0.0f;
    float minMs = 999999.0f;
    float maxMs = 0.0f;
    uint32_t sampleCount = 0;
    bool enabled = true;

    void record(float ms) {
        lastMs = ms;
        if (ms < minMs) minMs = ms;
        if (ms > maxMs) maxMs = ms;
        avgMs = (avgMs * sampleCount + ms) / static_cast<float>(sampleCount + 1);
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

// Quality scaling parameters per level
struct QualityProfile {
    float vegetationDensity = 1.0f;     // 0.0-1.0 tree/grass density multiplier
    float shadowResolution = 1.0f;      // 0.25-1.0 shadow map resolution scale
    float textureQuality = 1.0f;        // 0.5-1.0 mipmap bias
    int maxFaceLodLevel = 0;            // 0=FULL, 1=MEDIUM, 2=LOW
    bool enablePostProcessing = true;
    bool enableVideoPlayback = true;
    bool enableParticleEffects = true;
    int maxDrawCalls = 2000;
    float lodBias = 1.0f;               // Multiplier for LOD distance thresholds
};

// ============================================================================
// FrameBudgetManager - singleton frame budget and quality management
// ============================================================================

class FrameBudgetManager {
public:
    static FrameBudgetManager& instance() {
        static FrameBudgetManager inst;
        return inst;
    }

    // Initialize with target frame time (default 16.6ms = 60fps)
    void init(float targetFrameTimeMs = 16.6f) {
        targetFrameTimeMs_ = targetFrameTimeMs;
        currentQuality_ = QualityLevel::HIGH;
        consecutiveOverBudget_ = 0;
        consecutiveUnderBudget_ = 0;
        initQualityProfiles();
        registerDefaultPhases();
        initialized_ = true;
        LOGI_FBM("Initialized with target %.1f ms (%.0f fps)",
                 targetFrameTimeMs, 1000.0f / targetFrameTimeMs);
    }

    void shutdown() {
        phases_.clear();
        initialized_ = false;
    }

    // --- Frame lifecycle ---

    void beginFrame() {
        frameStart_ = std::chrono::high_resolution_clock::now();
    }

    void endFrame() {
        auto now = std::chrono::high_resolution_clock::now();
        lastFrameTimeMs_ = std::chrono::duration<float, std::milli>(
            now - frameStart_).count();

        // Rolling average of frame times
        frameTimeAccum_ += lastFrameTimeMs_;
        frameCount_++;
        if (frameCount_ >= FPS_WINDOW) {
            avgFrameTimeMs_ = frameTimeAccum_ / static_cast<float>(frameCount_);
            frameTimeAccum_ = 0.0f;
            frameCount_ = 0;
        }

        // Quality scaling decision
        evaluateQualityScaling();
    }

    // --- Phase timing ---

    void beginPhase(const std::string& name) {
        phaseTimers_[name] = std::chrono::high_resolution_clock::now();
    }

    void endPhase(const std::string& name) {
        auto it = phaseTimers_.find(name);
        if (it == phaseTimers_.end()) return;

        auto now = std::chrono::high_resolution_clock::now();
        float ms = std::chrono::duration<float, std::milli>(
            now - it->second).count();

        auto phaseIt = phases_.find(name);
        if (phaseIt != phases_.end()) {
            phaseIt->second.record(ms);
        }
    }

    // --- Phase registration ---

    void registerPhase(const std::string& name, PhasePriority priority) {
        PhaseRecord rec;
        rec.name = name;
        rec.priority = priority;
        phases_[name] = rec;
    }

    // --- Query ---

    bool shouldSkipPhase(const std::string& name) const {
        auto it = phases_.find(name);
        if (it == phases_.end()) return false;

        const PhaseRecord& rec = it->second;
        if (!rec.enabled) return true;

        // Never skip CRITICAL phases
        if (rec.priority == PhasePriority::CRITICAL) return false;

        // Skip based on quality level
        int priorityLevel = static_cast<int>(rec.priority);
        int qualityLevel = static_cast<int>(currentQuality_);

        // At POTATO quality, skip LOW and MEDIUM
        // At LOW quality, skip LOW only
        // At MEDIUM, skip nothing extra
        if (qualityLevel >= 4 && priorityLevel >= 2) return true;  // POTATO: skip MEDIUM+LOW
        if (qualityLevel >= 3 && priorityLevel >= 3) return true;  // LOW: skip LOW

        return false;
    }

    bool isFrameOverBudget() const {
        return lastFrameTimeMs_ > targetFrameTimeMs_;
    }

    bool withinBudget(std::chrono::high_resolution_clock::time_point frameStart) const {
        auto now = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration<float, std::milli>(now - frameStart).count();
        return elapsed < targetFrameTimeMs_;
    }

    // --- Quality accessors ---

    QualityLevel getQualityLevel() const { return currentQuality_; }
    void setQualityLevel(QualityLevel level) { currentQuality_ = level; }

    const QualityProfile& getQualityProfile() const {
        return qualityProfiles_[static_cast<int>(currentQuality_)];
    }

    float getTargetFrameTimeMs() const { return targetFrameTimeMs_; }
    float getLastFrameTimeMs() const { return lastFrameTimeMs_; }
    float getAvgFrameTimeMs() const { return avgFrameTimeMs_; }
    float getFrameTimeRatio() const { return lastFrameTimeMs_ / targetFrameTimeMs_; }

    // --- Debug ---

    std::string getPhaseTimings() const {
        std::string result = "=== Frame Budget Report ===\n";
        result += "Quality: " + qualityLevelName(currentQuality_) + "\n";
        result += "Frame: " + std::to_string(lastFrameTimeMs_) + "ms / "
                + std::to_string(targetFrameTimeMs_) + "ms\n";
        for (const auto& pair : phases_) {
            const PhaseRecord& rec = pair.second;
            result += "  " + rec.name + ": " + std::to_string(rec.lastMs) + "ms"
                    + " (avg " + std::to_string(rec.avgMs) + "ms)"
                    + (rec.enabled ? "" : " [DISABLED]")
                    + "\n";
        }
        return result;
    }

    const std::unordered_map<std::string, PhaseRecord>& getPhases() const {
        return phases_;
    }

private:
    FrameBudgetManager() = default;

    bool initialized_ = false;
    float targetFrameTimeMs_ = 16.6f;
    float lastFrameTimeMs_ = 0.0f;
    float avgFrameTimeMs_ = 0.0f;
    float frameTimeAccum_ = 0.0f;
    uint32_t frameCount_ = 0;

    static constexpr uint32_t FPS_WINDOW = 60;
    static constexpr uint32_t OVER_BUDGET_THRESHOLD = 10;
    static constexpr uint32_t UNDER_BUDGET_THRESHOLD = 60;
    static constexpr float UNDER_BUDGET_RATIO = 0.7f;

    QualityLevel currentQuality_ = QualityLevel::HIGH;
    uint32_t consecutiveOverBudget_ = 0;
    uint32_t consecutiveUnderBudget_ = 0;

    std::chrono::high_resolution_clock::time_point frameStart_;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> phaseTimers_;
    std::unordered_map<std::string, PhaseRecord> phases_;
    QualityProfile qualityProfiles_[static_cast<int>(QualityLevel::COUNT)];

    void initQualityProfiles() {
        // ULTRA
        qualityProfiles_[0] = {1.0f, 1.0f, 1.0f, 0, true, true, true, 3000, 1.0f};
        // HIGH
        qualityProfiles_[1] = {0.8f, 0.75f, 1.0f, 0, true, true, true, 2000, 1.0f};
        // MEDIUM
        qualityProfiles_[2] = {0.5f, 0.5f, 0.75f, 1, true, true, false, 1500, 1.2f};
        // LOW
        qualityProfiles_[3] = {0.3f, 0.25f, 0.5f, 2, false, false, false, 1000, 1.5f};
        // POTATO
        qualityProfiles_[4] = {0.1f, 0.0f, 0.5f, 2, false, false, false, 500, 2.0f};
    }

    void registerDefaultPhases() {
        registerPhase("PreUpdate", PhasePriority::CRITICAL);
        registerPhase("EventProcess", PhasePriority::CRITICAL);
        registerPhase("WorldUpdate", PhasePriority::HIGH);
        registerPhase("AiUpdate", PhasePriority::HIGH);
        registerPhase("PlayerUpdate", PhasePriority::CRITICAL);
        registerPhase("InventoryUpdate", PhasePriority::HIGH);
        registerPhase("SpellUpdate", PhasePriority::HIGH);
        registerPhase("AnimationUpdate", PhasePriority::HIGH);
        registerPhase("JoltPhysics", PhasePriority::CRITICAL);
        registerPhase("PhysicsSync", PhasePriority::CRITICAL);
        registerPhase("CombatUpdate", PhasePriority::HIGH);
        registerPhase("QuestUpdate", PhasePriority::MEDIUM);
        registerPhase("ScriptUpdate", PhasePriority::MEDIUM);
        registerPhase("VegetationUpdate", PhasePriority::MEDIUM);
        registerPhase("FaceGenUpdate", PhasePriority::MEDIUM);
        registerPhase("VideoUpdate", PhasePriority::LOW);
        registerPhase("AudioUpdate", PhasePriority::LOW);
        registerPhase("RenderSubmit", PhasePriority::CRITICAL);
    }

    void evaluateQualityScaling() {
        if (lastFrameTimeMs_ > targetFrameTimeMs_) {
            consecutiveOverBudget_++;
            consecutiveUnderBudget_ = 0;

            if (consecutiveOverBudget_ >= OVER_BUDGET_THRESHOLD) {
                // Downgrade quality
                int q = static_cast<int>(currentQuality_);
                if (q < static_cast<int>(QualityLevel::POTATO)) {
                    currentQuality_ = static_cast<QualityLevel>(q + 1);
                    LOGW_FBM("Quality downgraded to %s (over budget for %u frames)",
                             qualityLevelName(currentQuality_).c_str(),
                             consecutiveOverBudget_);
                }
                consecutiveOverBudget_ = 0;
            }
        } else if (lastFrameTimeMs_ < targetFrameTimeMs_ * UNDER_BUDGET_RATIO) {
            consecutiveUnderBudget_++;
            consecutiveOverBudget_ = 0;

            if (consecutiveUnderBudget_ >= UNDER_BUDGET_THRESHOLD) {
                // Upgrade quality
                int q = static_cast<int>(currentQuality_);
                if (q > static_cast<int>(QualityLevel::ULTRA)) {
                    currentQuality_ = static_cast<QualityLevel>(q - 1);
                    LOGI_FBM("Quality upgraded to %s (under budget for %u frames)",
                             qualityLevelName(currentQuality_).c_str(),
                             consecutiveUnderBudget_);
                }
                consecutiveUnderBudget_ = 0;
            }
        } else {
            consecutiveOverBudget_ = 0;
            consecutiveUnderBudget_ = 0;
        }
    }

    static std::string qualityLevelName(QualityLevel level) {
        switch (level) {
            case QualityLevel::ULTRA:  return "ULTRA";
            case QualityLevel::HIGH:   return "HIGH";
            case QualityLevel::MEDIUM: return "MEDIUM";
            case QualityLevel::LOW:    return "LOW";
            case QualityLevel::POTATO: return "POTATO";
            default: return "UNKNOWN";
        }
    }
};

} // namespace engine
