#include "save_slot_manager.h"
#include <fstream>
#include <algorithm>
#include <cstring>

bool SaveSlotManager::initialize(const std::string& baseDir) {
    baseDir_ = baseDir;
    if (!fs::exists(baseDir_)) {
        try {
            fs::create_directories(baseDir_);
            SLOT_LOGI("Created save directory: %s", baseDir_.c_str());
        } catch (const std::exception& e) {
            SLOT_LOGE("Failed to create save directory: %s", e.what());
            return false;
        }
    }
    SLOT_LOGI("SaveSlotManager initialized with %u used slots", getUsedSlotCount());
    return true;
}

std::string SaveSlotManager::slotIndexToName(uint32_t slotIndex) const {
    if (slotIndex == AUTO_SAVE_SLOT) return "autosave";
    if (slotIndex == QUICK_SAVE_SLOT) return "quicksave";
    return "slot_" + std::to_string(slotIndex);
}

std::string SaveSlotManager::getSlotPath(uint32_t slotIndex) const {
    return baseDir_ + slotIndexToName(slotIndex) + ".sav";
}

std::string SaveSlotManager::getAutoSavePath() const {
    return getSlotPath(AUTO_SAVE_SLOT);
}

std::string SaveSlotManager::getQuickSavePath() const {
    return getSlotPath(QUICK_SAVE_SLOT);
}

bool SaveSlotManager::readHeader(const std::string& path, save_format::SaveHeader& header) const {
    if (!fs::exists(path)) return false;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    return file.good();
}

SaveSlotInfo SaveSlotManager::getSlotInfo(uint32_t slotIndex) const {
    SaveSlotInfo info;
    info.slotIndex = slotIndex;
    info.slotName = slotIndexToName(slotIndex);
    info.isEmpty = true;

    std::string path = getSlotPath(slotIndex);
    if (!fs::exists(path)) return info;

    save_format::SaveHeader header;
    if (readHeader(path, header)) {
        auto validation = save_format::validateHeader(header);
        if (validation == save_format::ValidationResult::OK) {
            info.isEmpty = false;
            info.displayName = std::string(header.slotName);
            info.timestamp = header.timestamp;
            info.isAutoSave = (slotIndex == AUTO_SAVE_SLOT);
            info.fileSize = static_cast<uint32_t>(fs::file_size(path));
        }
    }

    return info;
}

std::vector<SaveSlotInfo> SaveSlotManager::getAllSlots() const {
    std::vector<SaveSlotInfo> slots;
    // Auto-save slot
    slots.push_back(getSlotInfo(AUTO_SAVE_SLOT));
    // Manual slots 1-10
    for (uint32_t i = 1; i <= MAX_MANUAL_SLOTS; ++i) {
        slots.push_back(getSlotInfo(i));
    }
    // Quick-save slot
    slots.push_back(getSlotInfo(QUICK_SAVE_SLOT));
    return slots;
}

bool SaveSlotManager::isSlotUsed(uint32_t slotIndex) const {
    return fs::exists(getSlotPath(slotIndex));
}

uint32_t SaveSlotManager::getUsedSlotCount() const {
    uint32_t count = 0;
    for (uint32_t i = 0; i <= QUICK_SAVE_SLOT; ++i) {
        if (isSlotUsed(i)) ++count;
    }
    return count;
}

bool SaveSlotManager::deleteSlot(uint32_t slotIndex) {
    std::string path = getSlotPath(slotIndex);
    if (!fs::exists(path)) {
        SLOT_LOGE("Slot %u does not exist", slotIndex);
        return false;
    }

    try {
        fs::remove(path);
        SLOT_LOGI("Deleted save slot %u", slotIndex);
        return true;
    } catch (const std::exception& e) {
        SLOT_LOGE("Failed to delete slot %u: %s", slotIndex, e.what());
        return false;
    }
}

bool SaveSlotManager::copySlot(uint32_t srcSlot, uint32_t dstSlot) {
    std::string srcPath = getSlotPath(srcSlot);
    std::string dstPath = getSlotPath(dstSlot);

    if (!fs::exists(srcPath)) {
        SLOT_LOGE("Source slot %u does not exist", srcSlot);
        return false;
    }

    try {
        fs::copy_file(srcPath, dstPath, fs::copy_options::overwrite_existing);
        SLOT_LOGI("Copied slot %u to slot %u", srcSlot, dstSlot);
        return true;
    } catch (const std::exception& e) {
        SLOT_LOGE("Failed to copy slot: %s", e.what());
        return false;
    }
}

bool SaveSlotManager::renameSlot(uint32_t slotIndex, const std::string& newName) {
    std::string path = getSlotPath(slotIndex);
    if (!fs::exists(path)) {
        SLOT_LOGE("Slot %u does not exist", slotIndex);
        return false;
    }

    // Read, modify header, write back
    std::ifstream inFile(path, std::ios::binary);
    if (!inFile.is_open()) return false;

    std::vector<uint8_t> fileData((std::istreambuf_iterator<char>(inFile)),
                                   std::istreambuf_iterator<char>());
    inFile.close();

    if (fileData.size() < sizeof(save_format::SaveHeader)) return false;

    save_format::SaveHeader* header = reinterpret_cast<save_format::SaveHeader*>(fileData.data());
    std::memset(header->slotName, 0, sizeof(header->slotName));
    size_t copyLen = std::min(newName.size(), sizeof(header->slotName) - 1);
    std::memcpy(header->slotName, newName.c_str(), copyLen);

    std::ofstream outFile(path, std::ios::binary | std::ios::trunc);
    if (!outFile.is_open()) return false;

    outFile.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
    outFile.close();

    SLOT_LOGI("Renamed slot %u to '%s'", slotIndex, newName.c_str());
    return true;
}

int SaveSlotManager::findLatestSaveSlot() const {
    uint64_t latestTime = 0;
    int latestSlot = -1;

    // Check manual slots first, then auto-save
    for (uint32_t i = 1; i <= MAX_MANUAL_SLOTS; ++i) {
        SaveSlotInfo info = getSlotInfo(i);
        if (!info.isEmpty && info.timestamp > latestTime) {
            latestTime = info.timestamp;
            latestSlot = static_cast<int>(i);
        }
    }

    // Also check quick-save
    SaveSlotInfo qsInfo = getSlotInfo(QUICK_SAVE_SLOT);
    if (!qsInfo.isEmpty && qsInfo.timestamp > latestTime) {
        latestTime = qsInfo.timestamp;
        latestSlot = static_cast<int>(QUICK_SAVE_SLOT);
    }

    return latestSlot;
}

void SaveSlotManager::updateSlotMetadata(uint32_t slotIndex, const save_format::SaveHeader& header) {
    SLOT_LOGD("Updated metadata for slot %u: '%s'", slotIndex, header.slotName);
}
