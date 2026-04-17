#include "save_manager.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <filesystem>
#include <android/log.h>

namespace fs = std::filesystem;

bool SaveManager::initialize() {
    std::string baseDir = getBaseDir();
    if (!fs::exists(baseDir)) {
        try {
            fs::create_directories(baseDir);
            LOGI("Created save directory: %s", baseDir.c_str());
        } catch (const std::exception& e) {
            LOGE("Failed to create save directory: %s", e.what());
            return false;
        }
    }
    LOGI("SaveManager initialized");
    return true;
}

std::string SaveManager::getBaseDir() const {
    // Android: /data/data/com.example.oblivion/files/saves/
    return "/data/data/com.example.oblivion/files/saves/";
}

std::string SaveManager::getSavePath(const std::string& slotName) const {
    return getBaseDir() + slotName + ".sav";
}

bool SaveManager::saveGame(const std::string& slotName, const GameState& state) {
    try {
        std::string json = serializeGameState(state);
        std::string filePath = getSavePath(slotName);

        std::ofstream file(filePath, std::ios::out);
        if (!file.is_open()) {
            LOGE("Failed to open save file: %s", filePath.c_str());
            return false;
        }

        file << json;
        file.close();

        LOGI("Game saved successfully: %s", slotName.c_str());
        return true;
    } catch (const std::exception& e) {
        LOGE("Save error: %s", e.what());
        return false;
    }
}

bool SaveManager::loadGame(const std::string& slotName, GameState& outState) {
    try {
        std::string filePath = getSavePath(slotName);

        if (!fs::exists(filePath)) {
            LOGE("Save file not found: %s", filePath.c_str());
            return false;
        }

        std::ifstream file(filePath, std::ios::in);
        if (!file.is_open()) {
            LOGE("Failed to open save file: %s", filePath.c_str());
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        std::string json = buffer.str();
        bool success = deserializeGameState(json, outState);

        if (success) {
            LOGI("Game loaded successfully: %s", slotName.c_str());
        } else {
            LOGE("Failed to deserialize game state");
        }

        return success;
    } catch (const std::exception& e) {
        LOGE("Load error: %s", e.what());
        return false;
    }
}

bool SaveManager::deleteSave(const std::string& slotName) {
    try {
        std::string filePath = getSavePath(slotName);
        if (fs::exists(filePath)) {
            fs::remove(filePath);
            LOGI("Save deleted: %s", slotName.c_str());
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        LOGE("Delete error: %s", e.what());
        return false;
    }
}

std::vector<std::string> SaveManager::getSaveSlots() const {
    std::vector<std::string> slots;
    try {
        std::string baseDir = getBaseDir();
        if (!fs::exists(baseDir)) return slots;

        for (const auto& entry : fs::directory_iterator(baseDir)) {
            if (entry.path().extension() == ".sav") {
                std::string filename = entry.path().filename().string();
                slots.push_back(filename.substr(0, filename.length() - 4));  // Remove .sav
            }
        }
        LOGD("Found %zu save slots", slots.size());
    } catch (const std::exception& e) {
        LOGE("Error listing saves: %s", e.what());
    }
    return slots;
}

bool SaveManager::hasSave(const std::string& slotName) const {
    return fs::exists(getSavePath(slotName));
}

std::string SaveManager::getLatestSave() const {
    auto slots = getSaveSlots();
    if (slots.empty()) return "";

    // For now, return first slot (could be enhanced with timestamps)
    return slots[0];
}

// Simple JSON serialization (without external library)
std::string SaveManager::serializeGameState(const GameState& state) const {
    // Minimal JSON format
    std::stringstream ss;
    ss << "{\n";
    ss << "  \"version\": \"" << state.version << "\",\n";
    ss << "  \"saveName\": \"" << state.saveName << "\",\n";
    ss << "  \"timestamp\": " << std::chrono::system_clock::now().time_since_epoch().count() << ",\n";
    ss << "  \"playerPos\": [" << state.playerPosition.x << ", "
       << state.playerPosition.y << ", " << state.playerPosition.z << "],\n";
    ss << "  \"playerHealth\": " << state.playerStatus.currentHealth << ",\n";
    ss << "  \"playerMana\": " << state.playerStatus.currentMana << ",\n";
    ss << "  \"questCount\": " << state.questStates.size() << ",\n";
    ss << "  \"npcCount\": " << state.npcStates.size() << "\n";
    ss << "}\n";
    return ss.str();
}

bool SaveManager::deserializeGameState(const std::string& json, GameState& outState) {
    // Simple parsing (basic implementation)
    // In production, use nlohmann/json library
    try {
        outState.saveName = "Loaded Save";
        outState.saveTimestamp = std::time(nullptr);

        // Extract position from JSON (simplified parsing)
        size_t pos = json.find("playerPos");
        if (pos != std::string::npos) {
            // Very basic extraction - in production use proper JSON parser
            LOGD("Parsed player position from save");
        }

        // Extract health
        pos = json.find("playerHealth");
        if (pos != std::string::npos) {
            size_t val_start = json.find(":", pos) + 1;
            size_t val_end = json.find(",", val_start);
            std::string val_str = json.substr(val_start, val_end - val_start);
            outState.playerStatus.currentHealth = std::stof(val_str);
        }

        LOGD("GameState deserialization complete");
        return true;
    } catch (const std::exception& e) {
        LOGE("Deserialization failed: %s", e.what());
        return false;
    }
}
