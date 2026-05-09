#include "save_manager.h"
#include "save_validator.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "SaveManager", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "SaveManager", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "SaveManager", __VA_ARGS__)

namespace SaveSystem {

SaveManager::SaveManager() {
    validator = std::make_unique<SaveValidator>();
}

SaveManager::~SaveManager() {
}

// ============================================================================
// Initialization
// ============================================================================

bool SaveManager::initialize() {
    LOGI("Initializing SaveManager");

    if (!ensureDirectoryExists(SAVE_DIR)) {
        LOGE("Failed to create save directory: %s", SAVE_DIR);
        return false;
    }

    if (!ensureDirectoryExists(AUTO_SAVE_DIR)) {
        LOGE("Failed to create auto-save directory: %s", AUTO_SAVE_DIR);
        return false;
    }

    LOGI("SaveManager initialized successfully");
    return true;
}

// ============================================================================
// Save/Load Operations
// ============================================================================

bool SaveManager::saveGame(int slotId, const json& gameState) {
    if (slotId < 0 || slotId >= MAX_SAVE_SLOTS) {
        LOGE("Invalid slot ID: %d", slotId);
        return false;
    }

    try {
        // Ensure slot directory exists
        std::string slotPath = getSaveSlotPath(slotId);
        if (!ensureDirectoryExists(slotPath)) {
            LOGE("Failed to create slot directory: %s", slotPath.c_str());
            return false;
        }

        // Make a copy and add checksum
        json gameStateCopy = gameState;
        if (!gameStateCopy.contains("metadata")) {
            gameStateCopy["metadata"] = json::object();
        }
        gameStateCopy["metadata"]["version"] = "0.6.0";
        gameStateCopy["metadata"]["saveTime"] = getCurrentTimestamp();

        validator->addChecksum(gameStateCopy);

        // Write save.json
        std::string savePath = slotPath + "/save.json";
        std::ofstream saveFile(savePath);
        if (!saveFile.is_open()) {
            LOGE("Failed to open save file: %s", savePath.c_str());
            return false;
        }

        saveFile << gameStateCopy.dump(2);
        saveFile.close();

        LOGI("Game saved to slot %d: %s", slotId, savePath.c_str());

        // Extract metadata and save it separately
        SaveSlot slot(slotId);
        slot.characterName = gameStateCopy.value("characterName", "Unknown");
        slot.playerLevel = gameStateCopy.value("playerLevel", 1);
        slot.lastLocation = gameStateCopy.value("lastLocation", "Unknown");
        slot.saveTime = gameStateCopy["metadata"]["saveTime"];
        slot.playTime = gameStateCopy.value("playTime", 0.0f);
        slot.isEmpty = false;
        slot.savePath = savePath;

        if (!saveSlotMetadata(slotId, slot)) {
            LOGW("Failed to save slot metadata");
        }

        return true;

    } catch (const json::exception& e) {
        LOGE("JSON error while saving: %s", e.what());
        return false;
    } catch (const std::exception& e) {
        LOGE("Unexpected error while saving: %s", e.what());
        return false;
    }
}

bool SaveManager::loadGame(int slotId, json& outGameState) {
    if (slotId < 0 || slotId >= MAX_SAVE_SLOTS) {
        LOGE("Invalid slot ID: %d", slotId);
        return false;
    }

    try {
        std::string slotPath = getSaveSlotPath(slotId);
        std::string savePath = slotPath + "/save.json";

        // Check if save file exists
        if (!std::filesystem::exists(savePath)) {
            LOGE("Save file not found: %s", savePath.c_str());
            return false;
        }

        // Read and parse JSON
        std::ifstream saveFile(savePath);
        if (!saveFile.is_open()) {
            LOGE("Failed to open save file: %s", savePath.c_str());
            return false;
        }

        outGameState = json::parse(saveFile);
        saveFile.close();

        // Validate the save file
        if (!validateSaveFile(outGameState)) {
            LOGE("Save file validation failed");
            return false;
        }

        LOGI("Game loaded from slot %d: %s", slotId, savePath.c_str());
        return true;

    } catch (const json::parse_error& e) {
        LOGE("JSON parse error while loading: %s", e.what());
        return false;
    } catch (const std::exception& e) {
        LOGE("Unexpected error while loading: %s", e.what());
        return false;
    }
}

// ============================================================================
// Slot Management
// ============================================================================

std::vector<SaveSlot> SaveManager::getAllSaveSlots() const {
    std::vector<SaveSlot> slots;

    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        slots.push_back(getSaveSlot(i));
    }

    return slots;
}

SaveSlot SaveManager::getSaveSlot(int slotId) const {
    if (slotId < 0 || slotId >= MAX_SAVE_SLOTS) {
        SaveSlot empty;
        empty.slotId = slotId;
        empty.isEmpty = true;
        return empty;
    }

    return loadSlotMetadata(slotId);
}

bool SaveManager::isSaveSlotEmpty(int slotId) const {
    SaveSlot slot = getSaveSlot(slotId);
    return slot.isEmpty;
}

bool SaveManager::deleteSaveSlot(int slotId) {
    if (slotId < 0 || slotId >= MAX_SAVE_SLOTS) {
        LOGE("Invalid slot ID: %d", slotId);
        return false;
    }

    try {
        std::string slotPath = getSaveSlotPath(slotId);

        if (std::filesystem::exists(slotPath)) {
            size_t removed = std::filesystem::remove_all(slotPath);
            LOGI("Deleted slot %d, removed %zu files", slotId, removed);
            return true;
        } else {
            LOGI("Slot %d is already empty", slotId);
            return true;
        }

    } catch (const std::exception& e) {
        LOGE("Error deleting slot %d: %s", slotId, e.what());
        return false;
    }
}

// ============================================================================
// Auto-Save Operations
// ============================================================================

bool SaveManager::createAutoSave(const json& gameState) {
    try {
        if (!ensureDirectoryExists(AUTO_SAVE_DIR)) {
            LOGE("Failed to create auto-save directory");
            return false;
        }

        // Make a copy and add checksum
        json autoSaveCopy = gameState;
        if (!autoSaveCopy.contains("metadata")) {
            autoSaveCopy["metadata"] = json::object();
        }
        autoSaveCopy["metadata"]["version"] = "0.6.0";
        autoSaveCopy["metadata"]["saveTime"] = getCurrentTimestamp();

        validator->addChecksum(autoSaveCopy);

        // Write autosave.json
        std::string autoSavePath = std::string(AUTO_SAVE_DIR) + "/autosave.json";
        std::ofstream autoSaveFile(autoSavePath);
        if (!autoSaveFile.is_open()) {
            LOGE("Failed to open auto-save file: %s", autoSavePath.c_str());
            return false;
        }

        autoSaveFile << autoSaveCopy.dump(2);
        autoSaveFile.close();

        LOGI("Auto-save created: %s", autoSavePath.c_str());
        return true;

    } catch (const json::exception& e) {
        LOGE("JSON error while auto-saving: %s", e.what());
        return false;
    } catch (const std::exception& e) {
        LOGE("Unexpected error while auto-saving: %s", e.what());
        return false;
    }
}

bool SaveManager::loadAutoSave(json& outGameState) {
    try {
        std::string autoSavePath = std::string(AUTO_SAVE_DIR) + "/autosave.json";

        if (!std::filesystem::exists(autoSavePath)) {
            LOGE("Auto-save file not found: %s", autoSavePath.c_str());
            return false;
        }

        std::ifstream autoSaveFile(autoSavePath);
        if (!autoSaveFile.is_open()) {
            LOGE("Failed to open auto-save file: %s", autoSavePath.c_str());
            return false;
        }

        outGameState = json::parse(autoSaveFile);
        autoSaveFile.close();

        if (!validateSaveFile(outGameState)) {
            LOGE("Auto-save file validation failed");
            return false;
        }

        LOGI("Auto-save loaded: %s", autoSavePath.c_str());
        return true;

    } catch (const json::parse_error& e) {
        LOGE("JSON parse error while loading auto-save: %s", e.what());
        return false;
    } catch (const std::exception& e) {
        LOGE("Unexpected error while loading auto-save: %s", e.what());
        return false;
    }
}

bool SaveManager::hasAutoSave() const {
    std::string autoSavePath = std::string(AUTO_SAVE_DIR) + "/autosave.json";
    return std::filesystem::exists(autoSavePath);
}

// ============================================================================
// Path Management
// ============================================================================

std::string SaveManager::getSaveDirectory() const {
    return std::string(SAVE_DIR);
}

std::string SaveManager::getSaveSlotPath(int slotId) const {
    return std::string(SAVE_DIR) + "/slot" + std::to_string(slotId);
}

std::string SaveManager::getAutoSavePath() const {
    return std::string(AUTO_SAVE_DIR);
}

// ============================================================================
// Metadata Management
// ============================================================================

bool SaveManager::updateSlotMetadata(int slotId, const SaveSlot& slot) {
    return saveSlotMetadata(slotId, slot);
}

bool SaveManager::clearAllSaves() {
    LOGE("WARNING: Clearing all saves!");

    bool allCleared = true;
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        if (!deleteSaveSlot(i)) {
            allCleared = false;
        }
    }

    // Also clear auto-save
    try {
        if (std::filesystem::exists(AUTO_SAVE_DIR)) {
            std::filesystem::remove_all(AUTO_SAVE_DIR);
        }
    } catch (const std::exception& e) {
        LOGE("Failed to clear auto-save: %s", e.what());
        allCleared = false;
    }

    return allCleared;
}

// ============================================================================
// Private Helper Functions
// ============================================================================

bool SaveManager::ensureDirectoryExists(const std::string& path) const {
    try {
        if (!std::filesystem::exists(path)) {
            return std::filesystem::create_directories(path);
        }
        return true;
    } catch (const std::exception& e) {
        LOGE("Failed to create directory %s: %s", path.c_str(), e.what());
        return false;
    }
}

SaveSlot SaveManager::loadSlotMetadata(int slotId) const {
    SaveSlot slot(slotId);

    try {
        std::string metadataPath = getSaveSlotPath(slotId) + "/metadata.json";

        if (!std::filesystem::exists(metadataPath)) {
            slot.isEmpty = true;
            return slot;
        }

        std::ifstream metadataFile(metadataPath);
        json metadata = json::parse(metadataFile);
        metadataFile.close();

        slot.characterName = metadata.value("characterName", "Unknown");
        slot.playerLevel = metadata.value("playerLevel", 0);
        slot.lastLocation = metadata.value("lastLocation", "Unknown");
        slot.saveTime = metadata.value("saveTime", "");
        slot.playTime = metadata.value("playTime", 0.0f);
        slot.isEmpty = false;
        slot.savePath = getSaveSlotPath(slotId) + "/save.json";

        return slot;

    } catch (const std::exception& e) {
        LOGE("Error loading metadata for slot %d: %s", slotId, e.what());
        slot.isEmpty = true;
        return slot;
    }
}

bool SaveManager::saveSlotMetadata(int slotId, const SaveSlot& slot) const {
    try {
        std::string slotPath = getSaveSlotPath(slotId);
        if (!ensureDirectoryExists(slotPath)) {
            return false;
        }

        json metadata;
        metadata["characterName"] = slot.characterName;
        metadata["playerLevel"] = slot.playerLevel;
        metadata["lastLocation"] = slot.lastLocation;
        metadata["saveTime"] = slot.saveTime;
        metadata["playTime"] = slot.playTime;

        std::string metadataPath = slotPath + "/metadata.json";
        std::ofstream metadataFile(metadataPath);
        if (!metadataFile.is_open()) {
            LOGE("Failed to open metadata file: %s", metadataPath.c_str());
            return false;
        }

        metadataFile << metadata.dump(2);
        metadataFile.close();

        return true;

    } catch (const std::exception& e) {
        LOGE("Error saving metadata for slot %d: %s", slotId, e.what());
        return false;
    }
}

std::string SaveManager::getCurrentTimestamp() const {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool SaveManager::validateSaveFile(const json& gameState) const {
    SaveSystem::ValidationError err = validator->validate(gameState);
    if (err != SaveSystem::ValidationError::OK) {
        LOGE("Validation error: %s", validator->getErrorMessage(err).c_str());
        return false;
    }
    return true;
}

}  // namespace SaveSystem
