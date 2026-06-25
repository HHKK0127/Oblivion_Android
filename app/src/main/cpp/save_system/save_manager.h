#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <glm/glm.hpp>
#include "../game/npc.h"
#include "../game/quest.h"

#define LOG_TAG "SaveManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward declarations
struct GameState;

/**
 * @brief インベントリスロットデータ - シリアライザブルなアイテム情報
 *
 * Phase 9B Week 4: ゲーム状態保存時のインベントリシリアライズ用
 */
struct InventorySlotData {
    uint32_t itemId = 0;
    uint32_t quantity = 0;
    std::string itemName;

    InventorySlotData() = default;
    InventorySlotData(uint32_t id, uint32_t qty, const std::string& name)
        : itemId(id), quantity(qty), itemName(name) {}
};

/**
 * @brief 装備スロットデータ - シリアライザブルな装備情報
 */
struct EquippedItemData {
    uint32_t slotIndex = 0;  // EquipSlot enum as uint32_t
    uint32_t itemId = 0;
    std::string itemName;

    EquippedItemData() = default;
    EquippedItemData(uint32_t slot, uint32_t id, const std::string& name)
        : slotIndex(slot), itemId(id), itemName(name) {}
};

/**
 * SaveManager: ゲーム進行状況の保存・復元を管理
 */
class SaveManager {
public:
    SaveManager() = default;
    ~SaveManager() = default;

    // Initialization
    bool initialize();

    // Save/Load operations
    bool saveGame(const std::string& slotName, const GameState& state);
    bool loadGame(const std::string& slotName, GameState& outState);

    // Slot management
    bool deleteSave(const std::string& slotName);
    std::vector<std::string> getSaveSlots() const;
    bool hasSave(const std::string& slotName) const;
    std::string getLatestSave() const;

private:
    std::string getSavePath(const std::string& slotName) const;
    std::string getBaseDir() const;

    // Serialization helpers
    std::string serializeGameState(const GameState& state) const;
    bool deserializeGameState(const std::string& json, GameState& outState);
};

/**
 * GameState: ゲーム状態のスナップショット
 */
struct GameState {
    // Metadata
    std::string saveName;
    uint64_t saveTimestamp = 0;
    std::string version = "0.6.0";

    // Player state
    glm::vec3 playerPosition = glm::vec3(0, 0, 0);
    CharacterStatus playerStatus;

    // World state
    std::vector<uint32_t> loadedCells;

    // NPC states
    std::map<uint32_t, std::pair<glm::vec3, CharacterStatus>> npcStates;  // npcId -> (pos, status)

    // Quest states
    std::map<uint32_t, int> questStates;  // questId -> state enum (0=pending, 1=accepted, etc)

    // Inventory state (Phase 9B Week 4)
    std::vector<InventorySlotData> inventorySlots;    // Player inventory grid (60 slots)
    std::vector<EquippedItemData> equippedItems;       // Currently equipped items
    float playerInventoryWeight = 0.0f;                // Current inventory weight

    GameState() {
        // Initialize default player status
        playerStatus.initialize(100.0f, 120.0f, 1);
    }
};

#include <android/log.h>
