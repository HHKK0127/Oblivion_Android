#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <glm/glm.hpp>
#include "../game/npc.h"
#include "../game/quest.h"
#include "../game/item.h"
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
class Container;
namespace oblivion { namespace script { class ScriptManager; } }

/**
 * @brief Phase 69: container contents captured by a loadGame() call.
 *
 * Containers are materialised lazily the first time the player opens them, so
 * a load cannot restore them directly. Instead loadGame() records the saved
 * contents here and applyPendingContainerState() hands them over when the
 * container is finally created.
 */
struct PendingContainerState {
    bool locked = false;
    std::vector<InventoryItem> items;
};

/**
 * @brief Inventory slot data - Serializable item information
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
 * @brief Equipment slot data - Serializable equipment information
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
 * SaveManager: Manage saving and restoring game progress
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

    // Accumulated play time of the active session, in seconds. Restored from the
    // save header on load and written back out on save so the slot list can show
    // the original's "%02i:%02i:%02i" play time (RE_EXE_REPORT §27).
    uint32_t getPlayTimeSeconds() const { return playTimeSeconds_; }
    void setPlayTimeSeconds(uint32_t seconds) { playTimeSeconds_ = seconds; }

    // Slot management
    SaveSlotManager& getSlotManager() { return slotManager_; }
    bool deleteSave(uint32_t slotIndex);
    std::vector<SaveSlotInfo> getSaveSlots() const;
    bool hasSave(uint32_t slotIndex) const;

    // Game state snapshot (for legacy compatibility)
    bool captureGameState(GameState& state);
    bool restoreGameState(const GameState& state);

    // Phase 69: hand the contents captured for this container by the most recent
    // loadGame() to the freshly materialised container. Returns false when the
    // container was not part of that save, so the caller populates from the ESM.
    bool applyPendingContainerState(const std::shared_ptr<Container>& container);

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

    // Format version of the last file read by readFromFile(). Load paths use it
    // to decide whether optional, version-gated fields exist in the payload.
    uint32_t loadedFormatVersion_ = save_format::CURRENT_VERSION;

    // Accumulated play time of the active session, in seconds.
    uint32_t playTimeSeconds_ = 0;

    // Fractional carry so sub-second frame steps are not lost.
    float playTimeAccumulator_ = 0.0f;

    // Phase 69: container contents read by the last loadGame(), keyed by the
    // container's persistence FormID. Consumed by applyPendingContainerState().
    std::unordered_map<uint32_t, PendingContainerState> pendingContainers_;

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
 * GameState: Game state snapshot (legacy compatibility)
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
