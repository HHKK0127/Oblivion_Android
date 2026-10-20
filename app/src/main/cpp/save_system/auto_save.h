#pragma once

#include <functional>
#include <string>
#include <android/log.h>

#define LOG_TAG_AUTO "AutoSave"
#define AUTO_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_AUTO, __VA_ARGS__)
#define AUTO_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_AUTO, __VA_ARGS__)
#define AUTO_LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_AUTO, __VA_ARGS__)

/**
 * AutoSave — Event-driven automatic save system.
 *
 * Removed time-interval mode per project direction (autosave-event-only).
 * Saves are triggered only by explicit game events:
 *   - Fast travel arrival
 *   - Cell transition (interior/exterior switch)
 *   - Quest stage advancement
 *   - Rest/sleep
 *   - Level up
 *   - Menu close after significant action
 */
class AutoSave {
public:
    using SaveCallback = std::function<bool(uint32_t slotIndex)>;

    AutoSave() = default;
    ~AutoSave() = default;

    void initialize(SaveCallback saveCallback) {
        saveCallback_ = std::move(saveCallback);
        enabled_ = true;
        AUTO_LOGI("AutoSave initialized (event-only mode)");
    }

    // Trigger event-based auto-save
    bool triggerEventSave(const std::string& eventName) {
        if (!enabled_ || !saveCallback_) return false;
        AUTO_LOGI("Event-triggered auto-save: %s", eventName.c_str());
        bool success = saveCallback_(0); // Slot 0 = auto-save
        if (success) {
            autoSaveCount_++;
            AUTO_LOGI("Auto-save #%u successful", autoSaveCount_);
        } else {
            AUTO_LOGW("Auto-save failed for event: %s", eventName.c_str());
        }
        return success;
    }

    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

    // Stats
    uint32_t getAutoSaveCount() const { return autoSaveCount_; }

private:
    SaveCallback saveCallback_;
    bool enabled_ = false;
    uint32_t autoSaveCount_ = 0;
};
