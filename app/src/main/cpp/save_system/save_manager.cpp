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
    // Serialize NPC states
    ss << "  \"npcStates\": [\n";
    {
        bool first = true;
        for (const auto& pair : state.npcStates) {
            if (!first) ss << ",\n";
            first = false;
            const auto& pos = pair.second.first;
            const auto& status = pair.second.second;
            ss << "    {\"npcId\": " << pair.first
               << ", \"pos\": [" << pos.x << ", " << pos.y << ", " << pos.z << "]"
               << ", \"health\": " << status.currentHealth
               << ", \"mana\": " << status.currentMana
               << ", \"stamina\": " << status.stamina
               << ", \"level\": " << status.level << "}";
        }
    }
    ss << "\n  ],\n";

    // Serialize quest states
    ss << "  \"questStates\": [\n";
    {
        bool first = true;
        for (const auto& pair : state.questStates) {
            if (!first) ss << ",\n";
            first = false;
            ss << "    {\"questId\": " << pair.first
               << ", \"state\": " << pair.second << "}";
        }
    }
    ss << "\n  ]\n";
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
            // Parse [x, y, z] array
            size_t arr_start = json.find("[", pos);
            size_t arr_end = json.find("]", arr_start);
            if (arr_start != std::string::npos && arr_end != std::string::npos) {
                std::string arr_str = json.substr(arr_start + 1, arr_end - arr_start - 1);
                std::stringstream ss(arr_str);
                std::string token;
                int coord_idx = 0;
                while (std::getline(ss, token, ',') && coord_idx < 3) {
                    // Trim whitespace
                    size_t start = token.find_first_not_of(" \t\n\r");
                    size_t end = token.find_last_not_of(" \t\n\r");
                    if (start != std::string::npos) {
                        token = token.substr(start, end - start + 1);
                    }
                    float val = std::stof(token);
                    switch (coord_idx) {
                        case 0: outState.playerPosition.x = val; break;
                        case 1: outState.playerPosition.y = val; break;
                        case 2: outState.playerPosition.z = val; break;
                    }
                    coord_idx++;
                }
                LOGD("Parsed player position from save: (%.2f, %.2f, %.2f)",
                     outState.playerPosition.x, outState.playerPosition.y, outState.playerPosition.z);
            }
        }

        // Extract health
        pos = json.find("playerHealth");
        if (pos != std::string::npos) {
            size_t val_start = json.find(":", pos) + 1;
            size_t val_end = json.find(",", val_start);
            std::string val_str = json.substr(val_start, val_end - val_start);
            outState.playerStatus.currentHealth = std::stof(val_str);
        }

        // Extract mana
        pos = json.find("playerMana");
        if (pos != std::string::npos) {
            size_t val_start = json.find(":", pos) + 1;
            size_t val_end = json.find(",", val_start);
            if (val_end == std::string::npos) val_end = json.find("}", val_start);
            std::string val_str = json.substr(val_start, val_end - val_start);
            outState.playerStatus.currentMana = std::stof(val_str);
        }

        // Extract NPC states
        pos = json.find("\"npcStates\"");
        if (pos != std::string::npos) {
            size_t arr_start = json.find("[", pos);
            size_t arr_end = json.find("]", arr_start);
            if (arr_start != std::string::npos && arr_end != std::string::npos) {
                std::string arr_str = json.substr(arr_start + 1, arr_end - arr_start - 1);
                // Parse each NPC entry: {"npcId": N, "pos": [x,y,z], "health": H, ...}
                size_t search_pos = 0;
                while (search_pos < arr_str.size()) {
                    size_t npc_start = arr_str.find("{", search_pos);
                    if (npc_start == std::string::npos) break;
                    size_t npc_end = arr_str.find("}", npc_start);
                    if (npc_end == std::string::npos) break;
                    std::string npc_str = arr_str.substr(npc_start, npc_end - npc_start + 1);

                    // Parse npcId
                    size_t id_pos = npc_str.find("\"npcId\"");
                    if (id_pos != std::string::npos) {
                        size_t id_val_start = npc_str.find(":", id_pos) + 1;
                        size_t id_val_end = npc_str.find(",", id_val_start);
                        uint32_t npcId = static_cast<uint32_t>(std::stoul(
                            npc_str.substr(id_val_start, id_val_end - id_val_start)));

                        // Parse position
                        glm::vec3 pos_vec(0.0f, 0.0f, 0.0f);
                        size_t pos_arr = npc_str.find("\"pos\"");
                        if (pos_arr != std::string::npos) {
                            size_t p_start = npc_str.find("[", pos_arr);
                            size_t p_end = npc_str.find("]", p_start);
                            if (p_start != std::string::npos && p_end != std::string::npos) {
                                std::string p_str = npc_str.substr(p_start + 1, p_end - p_start - 1);
                                std::stringstream pss(p_str);
                                std::string token;
                                int idx = 0;
                                while (std::getline(pss, token, ',') && idx < 3) {
                                    size_t s = token.find_first_not_of(" \t\n\r");
                                    size_t e = token.find_last_not_of(" \t\n\r");
                                    if (s != std::string::npos) token = token.substr(s, e - s + 1);
                                    float v = std::stof(token);
                                    if (idx == 0) pos_vec.x = v;
                                    else if (idx == 1) pos_vec.y = v;
                                    else if (idx == 2) pos_vec.z = v;
                                    idx++;
                                }
                            }
                        }

                        // Parse status fields
                        CharacterStatus status;
                        auto parseFloat = [&](const std::string& key, float& out) {
                            size_t kp = npc_str.find(key);
                            if (kp != std::string::npos) {
                                size_t vs = npc_str.find(":", kp) + 1;
                                size_t ve = npc_str.find(",", vs);
                                if (ve == std::string::npos) ve = npc_str.find("}", vs);
                                out = std::stof(npc_str.substr(vs, ve - vs));
                            }
                        };
                        auto parseUint = [&](const std::string& key, uint32_t& out) {
                            size_t kp = npc_str.find(key);
                            if (kp != std::string::npos) {
                                size_t vs = npc_str.find(":", kp) + 1;
                                size_t ve = npc_str.find(",", vs);
                                if (ve == std::string::npos) ve = npc_str.find("}", vs);
                                out = static_cast<uint32_t>(std::stoul(npc_str.substr(vs, ve - vs)));
                            }
                        };
                        parseFloat("\"health\"", status.currentHealth);
                        parseFloat("\"mana\"", status.currentMana);
                        parseFloat("\"stamina\"", status.stamina);
                        parseUint("\"level\"", status.level);

                        outState.npcStates[npcId] = std::make_pair(pos_vec, status);
                        LOGD("Parsed NPC %u: pos=(%.2f, %.2f, %.2f) hp=%.1f",
                             npcId, pos_vec.x, pos_vec.y, pos_vec.z, status.currentHealth);
                    }
                    search_pos = npc_end + 1;
                }
            }
        }

        // Extract quest states
        pos = json.find("\"questStates\"");
        if (pos != std::string::npos) {
            size_t arr_start = json.find("[", pos);
            size_t arr_end = json.find("]", arr_start);
            if (arr_start != std::string::npos && arr_end != std::string::npos) {
                std::string arr_str = json.substr(arr_start + 1, arr_end - arr_start - 1);
                size_t search_pos = 0;
                while (search_pos < arr_str.size()) {
                    size_t q_start = arr_str.find("{", search_pos);
                    if (q_start == std::string::npos) break;
                    size_t q_end = arr_str.find("}", q_start);
                    if (q_end == std::string::npos) break;
                    std::string q_str = arr_str.substr(q_start, q_end - q_start + 1);

                    size_t id_pos = q_str.find("\"questId\"");
                    size_t st_pos = q_str.find("\"state\"");
                    if (id_pos != std::string::npos && st_pos != std::string::npos) {
                        size_t id_vs = q_str.find(":", id_pos) + 1;
                        size_t id_ve = q_str.find(",", id_vs);
                        uint32_t questId = static_cast<uint32_t>(std::stoul(q_str.substr(id_vs, id_ve - id_vs)));

                        size_t st_vs = q_str.find(":", st_pos) + 1;
                        size_t st_ve = q_str.find(",", st_vs);
                        if (st_ve == std::string::npos) st_ve = q_str.find("}", st_vs);
                        int state = std::stoi(q_str.substr(st_vs, st_ve - st_vs));

                        outState.questStates[questId] = state;
                        LOGD("Parsed quest %u: state=%d", questId, state);
                    }
                    search_pos = q_end + 1;
                }
            }
        }

        LOGD("GameState deserialization complete");
        return true;
    } catch (const std::exception& e) {
        LOGE("Deserialization failed: %s", e.what());
        return false;
    }
}
