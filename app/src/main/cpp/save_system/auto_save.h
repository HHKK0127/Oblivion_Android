#pragma once

#include <functional>
#include <string>
#include <android/log.h>

#define LOG_TAG_AUTO "AutoSave"
#define AUTO_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_AUTO, __VA_ARGS__)
#define AUTO_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_AUTO, __VA_ARGS__)
#define AUTO_LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_AUTO, __VA_ARGS__)

// ============================================================================
// AutoSave - Automatic save system
// Triggers saves at intervals or on important events
// ============================================================================

class AutoSave {
public:
    // Default auto-save interval (5 minutes)
    static constexpr float DEFAULT_INTERVAL = 300.0f;

    // Minimum time between auto-saves (prevent spam)
    static constexpr float MIN_INTERVAL = 30.0f;

    AutoSave() = default;
    ~AutoSave() = default;

    // Initialize with save callback
    using SaveCallback = std::function<bool(uint32_t slotIndex)>;

    void initialize(SaveCallback saveCallback) {
        saveCallback_ = std::move(saveCallback);
        enabled_ = true;
        timer_ = 0.0f;
        AUTO_LOGI("AutoSave initialized (interval: %.0fs)", interval_);
    }

    // Update (call every frame)
    void update(float deltaTime) {
        if (!enabled_ || !saveCallback_) return;

        timer_ += deltaTime;
        if (timer_ >= interval_) {
            triggerAutoSave();
            timer_ = 0.0f;
        }
    }

    // Trigger an auto-save immediately
    bool triggerAutoSave() {
        if (!saveCallback_) return false;

        float timeSinceLastSave = timer_;
        if (timeSinceLastSave < MIN_INTERVAL && lastSaveTime_ > 0.0f) {
            AUTO_LOGD("Auto-save skipped (too soon since last save)");
            return false;
        }

        AUTO_LOGI("Triggering auto-save...");
        bool success = saveCallback_(0);  // Slot 0 = auto-save

        if (success) {
            lastSaveTime_ = totalGameTime_;
            autoSaveCount_++;
            AUTO_LOGI("Auto-save #%u successful", autoSaveCount_);
        } else {
            AUTO_LOGW("Auto-save failed");
        }

        return success;
    }

    // Trigger event-based auto-save (e.g., entering new area, completing quest)
    bool triggerEventSave(const std::string& eventName) {
        AUTO_LOGI("Event-triggered save: %s", eventName.c_str());
        return triggerAutoSave();
    }

    // Configuration
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

    void setInterval(float seconds) {
        interval_ = (seconds < MIN_INTERVAL) ? MIN_INTERVAL : seconds;
    }
    float getInterval() const { return interval_; }

    // Stats
    uint32_t getAutoSaveCount() const { return autoSaveCount_; }
    float getTimeSinceLastSave() const { return totalGameTime_ - lastSaveTime_; }

    // Set total game time (for metadata)
    void setTotalGameTime(float time) { totalGameTime_ = time; }

private:
    SaveCallback saveCallback_;
    bool enabled_ = false;
    float interval_ = DEFAULT_INTERVAL;
    float timer_ = 0.0f;
    float lastSaveTime_ = 0.0f;
    float totalGameTime_ = 0.0f;
    uint32_t autoSaveCount_ = 0;
};
