#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>
#include <glm/glm.hpp>
#include "../game/npc.h"
#include "../game/quest.h"
#include "serializable.h"
#include "save_format_version.h"
#include "save_slot_manager.h"
#include "auto_save.h"

#define LOG_TAG "SaveManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward declarations
struct GameState;
class PlayerController;
class InventoryManager;
class SpellManager;
class QuestManager;
class WorldManager;
namespace oblivion { namespace script { class ScriptManager; } }

/**
 * @brief インベントリスロットデータ - シリアライザブルなアイテム情報
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
    uint32_t slotIndex = 0;
    uint32_t itemId = 0;
    std::string itemName;

    EquippedItemData() = default;
    EquippedItemData(uint32_t slot, uint32_t id, const std::string& name)
        : slotIndex(slot), itemId(id), itemName(name) {}
};

/**
 * SaveManager: ゲーム進行状況の保存・復元を管理
 * Phase 41: Binary format with full system serialization
 */
class SaveManager {
public:
    SaveManager() = default;
    ~SaveManager() = default;

    // Initialization
    bool initialize();

    // System registration (Phase 41)
    void setPlayerController(PlayerController* pc) { playerController_ = pc; }
    void setInventoryManager(InventoryManager* im) { inventoryManager_ = im; }
    void setSpellManager(SpellManager* sm) { spellManager_ = sm; }
    void setQuestManager(QuestManager* qm) { questManager_ = qm; }
    void setWorldManager(WorldManager* wm) { worldManager_ = wm; }
    void setScriptManager(oblivion::script::ScriptManager* sm) { scriptManager_ = sm; }

    // Save/Load operations (binary format)
    bool saveGame(uint32_t slotIndex, const std::string& slotName = "");
    bool loadGame(uint32_t slotIndex);

    // Legacy JSON save/load (backward compatibility)
    bool saveGameLegacy(const std::string& slotName, const GameState& state);
    bool loadGameLegacy(const std::string& slotName, GameState& outState);

    // Quick save/load
    bool quickSave();
    bool quickLoad();

    // Auto-save integration
    void update(float deltaTime);
    AutoSave& getAutoSave() { return autoSave_; }

    // Slot management
    SaveSlotManager& getSlotManager() { return slotManager_; }
    bool deleteSave(uint32_t slotIndex);
    std::vector<SaveSlotInfo> getSaveSlots() const;
    bool hasSave(uint32_t slotIndex) const;

    // Game state snapshot (for legacy compatibility)
    bool captureGameState(GameState& state);
    bool restoreGameState(const GameState& state);

private:
    // System pointers (non-owning)
    PlayerController* playerController_ = nullptr;
    InventoryManager* inventoryManager_ = nullptr;
    SpellManager* spellManager_ = nullptr;
    QuestManager* questManager_ = nullptr;
    WorldManager* worldManager_ = nullptr;
    oblivion::script::ScriptManager* scriptManager_ = nullptr;

    // Sub-systems
    SaveSlotManager slotManager_;
    AutoSave autoSave_;

    // Base directory
    std::string getBaseDir() const;

    // Binary serialization
    bool writeToFile(uint32_t slotIndex, const std::string& slotName,
                     const std::vector<uint8_t>& payload);
    bool readFromFile(uint32_t slotIndex, std::vector<uint8_t>& payload);

    // Legacy helpers
    std::string getSavePathLegacy(const std::string& slotName) const;
    std::string serializeGameState(const GameState& state) const;
    bool deserializeGameState(const std::string& json, GameState& outState);
};

/**
 * GameState: ゲーム状態のスナップショット (legacy compatibility)
 */
struct GameState {
    std::string saveName;
    uint64_t saveTimestamp = 0;
    std::string version = "0.7.0";

    glm::vec3 playerPosition = glm::vec3(0, 0, 0);
    glm::vec3 playerRotation = glm::vec3(0, 0, 0);
    CharacterStatus playerStatus;

    std::vector<uint32_t> loadedCells;

    std::map<uint32_t, std::pair<glm::vec3, CharacterStatus>> npcStates;
    std::map<uint32_t, int> questStates;

    std::vector<InventorySlotData> inventorySlots;
    std::vector<EquippedItemData> equippedItems;
    float playerInventoryWeight = 0.0f;

    // Phase 41 additions
    uint32_t playerLevel = 1;
    float playerExperience = 0.0f;
    float gameTimeHours = 0.0f;
    float timeOfDay = 12.0f;
    uint32_t dayCount = 0;

    GameState() {
        playerStatus.initialize(100.0f, 120.0f, 1);
    }
};

#include <android/log.h>
