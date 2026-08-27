#pragma once

#include "save_format_version.h"
#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <android/log.h>

#define LOG_TAG_SLOT "SaveSlotManager"
#define SLOT_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_SLOT, __VA_ARGS__)
#define SLOT_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_SLOT, __VA_ARGS__)
#define SLOT_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_SLOT, __VA_ARGS__)

namespace fs = std::filesystem;

// ============================================================================
// Save Slot Metadata
// ============================================================================

struct SaveSlotInfo {
    uint32_t slotIndex = 0;
    std::string slotName;
    std::string displayName;
    uint64_t timestamp = 0;
    uint32_t playerLevel = 1;
    float gameTimeHours = 0.0f;
    std::string locationName;
    bool isAutoSave = false;
    bool isEmpty = true;
    uint32_t fileSize = 0;
};

// ============================================================================
// SaveSlotManager - Manages multiple save slots
// ============================================================================

class SaveSlotManager {
public:
    static constexpr uint32_t MAX_MANUAL_SLOTS = 10;
    static constexpr uint32_t AUTO_SAVE_SLOT = 0;
    static constexpr uint32_t QUICK_SAVE_SLOT = 11;

    SaveSlotManager() = default;
    ~SaveSlotManager() = default;

    // Initialize with base directory
    bool initialize(const std::string& baseDir);

    // Slot queries
    SaveSlotInfo getSlotInfo(uint32_t slotIndex) const;
    std::vector<SaveSlotInfo> getAllSlots() const;
    bool isSlotUsed(uint32_t slotIndex) const;
    uint32_t getUsedSlotCount() const;

    // Slot operations
    bool deleteSlot(uint32_t slotIndex);
    bool copySlot(uint32_t srcSlot, uint32_t dstSlot);
    bool renameSlot(uint32_t slotIndex, const std::string& newName);

    // Auto-save management
    std::string getAutoSavePath() const;
    std::string getQuickSavePath() const;
    std::string getSlotPath(uint32_t slotIndex) const;

    // Find latest save (for Continue)
    int findLatestSaveSlot() const;

    // Update slot metadata from header
    void updateSlotMetadata(uint32_t slotIndex, const save_format::SaveHeader& header);

private:
    std::string baseDir_;

    std::string slotIndexToName(uint32_t slotIndex) const;
    bool readHeader(const std::string& path, save_format::SaveHeader& header) const;
};
