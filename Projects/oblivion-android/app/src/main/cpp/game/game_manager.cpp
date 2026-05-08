#include "game_manager.h"
#include "../save_system/save_manager.h"
#include "../save_system/save_system.h"
#include "npc.h"
#include "quest.h"
#include "quest_manager.h"
#include "../world/world_manager.h"
#include "npc_manager.h"
#include <android/log.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;
using SaveSystem::SaveManager;

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "GameManager", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "GameManager", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "GameManager", __VA_ARGS__)

GameManager::GameManager()
    : worldManager(nullptr),
      npcManager(nullptr),
      questManager(nullptr),
      inventoryManager(nullptr),
      isInitialized(false) {
    saveManager = std::make_unique<SaveManager>();
    LOGI("GameManager created");
}

GameManager::~GameManager() {
    cleanup();
    LOGI("GameManager destroyed");
}

// ============================================================================
// Initialization
// ============================================================================

bool GameManager::initialize() {
    if (isInitialized) {
        LOGW("GameManager already initialized");
        return true;
    }

    // Initialize save manager
    if (!saveManager->initialize()) {
        LOGE("Failed to initialize SaveManager");
        return false;
    }

    LOGI("GameManager initialized successfully");
    isInitialized = true;
    return true;
}

void GameManager::cleanup() {
    if (!isInitialized) return;

    saveManager.reset();
    isInitialized = false;
    LOGI("GameManager cleanup complete");
}

// ============================================================================
// Game State Management
// ============================================================================

bool GameManager::startNewGame(const std::string& characterName) {
    LOGI("Starting new game: %s", characterName.c_str());

    // Create default game state
    currentGameState = createDefaultGameState(characterName);

    // Apply to all systems
    if (!applyGameState(currentGameState)) {
        LOGE("Failed to apply new game state");
        return false;
    }

    LOGI("New game started successfully");
    return true;
}

bool GameManager::saveGame(int slotId, const std::string& saveName) {
    LOGI("Saving game to slot %d", slotId);

    // Gather current state from all systems
    if (!gatherGameState()) {
        LOGE("Failed to gather game state");
        return false;
    }

    // Update metadata
    if (!saveName.empty()) {
        currentGameState.saveName = saveName;
    }
    currentGameState.saveTimestamp = getCurrentTimestamp();

    // Serialize and save to file
    json gameStateJson = SaveSystem::serializeGameState(currentGameState);
    if (!saveManager->saveGame(slotId, gameStateJson)) {
        LOGE("Failed to save game to slot %d", slotId);
        return false;
    }

    LOGI("Game saved to slot %d successfully", slotId);
    return true;
}

bool GameManager::loadGame(int slotId) {
    LOGI("Loading game from slot %d", slotId);

    json gameStateJson;
    if (!saveManager->loadGame(slotId, gameStateJson)) {
        LOGW("Save slot %d corrupted or missing, attempting auto-save recovery", slotId);
        if (saveManager->loadAutoSave(gameStateJson)) {
            LOGI("Auto-save recovery successful for slot %d", slotId);
        } else {
            LOGE("Failed to load game from slot %d and no valid auto-save available", slotId);
            return false;
        }
    }

    GameState loadedState = SaveSystem::deserializeGameState(gameStateJson);

    // Apply loaded state to all systems
    if (!applyGameState(loadedState)) {
        LOGE("Failed to apply loaded game state");
        return false;
    }

    currentGameState = loadedState;

    LOGI("Game loaded from slot %d successfully", slotId);
    return true;
}

bool GameManager::createAutoSave() {
    LOGI("Creating auto-save");

    // Gather current state
    if (!gatherGameState()) {
        LOGE("Failed to gather game state for auto-save");
        return false;
    }

    // Update metadata
    currentGameState.saveName = "[AUTO-SAVE]";
    currentGameState.saveTimestamp = getCurrentTimestamp();

    // Serialize and save auto-save
    json gameStateJson = SaveSystem::serializeGameState(currentGameState);
    if (!saveManager->createAutoSave(gameStateJson)) {
        LOGE("Failed to create auto-save");
        return false;
    }

    LOGI("Auto-save created successfully");
    return true;
}

bool GameManager::loadAutoSave() {
    LOGI("Loading auto-save");

    json gameStateJson;
    if (!saveManager->loadAutoSave(gameStateJson)) {
        LOGE("Failed to load auto-save");
        return false;
    }

    GameState loadedState = SaveSystem::deserializeGameState(gameStateJson);

    // Apply loaded state
    if (!applyGameState(loadedState)) {
        LOGE("Failed to apply auto-save state");
        return false;
    }

    currentGameState = loadedState;

    LOGI("Auto-save loaded successfully");
    return true;
}

// ============================================================================
// Save Slot Management
// ============================================================================

std::vector<SaveSlot> GameManager::getAllSaveSlots() const {
    return saveManager->getAllSaveSlots();
}

SaveSlot GameManager::getSaveSlot(int slotId) const {
    return saveManager->getSaveSlot(slotId);
}

bool GameManager::isSaveSlotEmpty(int slotId) const {
    return saveManager->isSaveSlotEmpty(slotId);
}

bool GameManager::deleteSaveSlot(int slotId) {
    LOGI("Deleting save slot %d", slotId);
    return saveManager->deleteSaveSlot(slotId);
}

bool GameManager::hasAutoSave() const {
    return saveManager->hasAutoSave();
}

// ============================================================================
// Private: Game State Gathering
// ============================================================================

bool GameManager::gatherGameState() {
    LOGI("Gathering game state from all systems");

    try {
        gatherPlayerState();
        gatherWorldState();
        gatherNpcStates();
        gatherQuestStates();
        gatherModStates();

        LOGI("Game state gathered successfully");
        return true;
    } catch (const std::exception& e) {
        LOGE("Exception while gathering game state: %s", e.what());
        return false;
    }
}

void GameManager::gatherPlayerState() {
    // This would require a Player class or accessing the player NPC
    // For now, we store minimal player data
    // TODO: Implement when Player class is available
    LOGI("Player state gathered");
}

void GameManager::gatherWorldState() {
    if (!worldManager) {
        LOGW("WorldManager not available");
        return;
    }

    // Gather world time, weather, loaded cells
    // TODO: Implement when WorldManager has getter methods for this data
    LOGI("World state gathered");
}

void GameManager::gatherNpcStates() {
    if (!npcManager) {
        LOGW("NpcManager not available");
        return;
    }

    currentGameState.npcStates.clear();

    auto allNpcs = npcManager->getAllNPCs();
    for (const auto& npc : allNpcs) {
        if (npc) {
            NPCState npcState;
            npcState.npcId = npc->npcId;
            npcState.position = npc->position;
            npcState.rotation = npc->rotation;
            npcState.status = npc->status;
            npcState.aiState = npc->aiState;
            npcState.wanderRadius = npc->wanderRadius;
            npcState.currentCell = 0;
            currentGameState.npcStates[npc->npcId] = npcState;
        }
    }

    LOGI("NPC states gathered: %zu NPCs", currentGameState.npcStates.size());
}

void GameManager::gatherQuestStates() {
    if (!questManager) {
        LOGW("QuestManager not available");
        return;
    }

    currentGameState.questStates.clear();

    auto activeQuests = questManager->getActiveQuests();
    for (const auto& quest : activeQuests) {
        if (quest) {
            QuestProgressState qps;
            qps.questId = quest->questId;
            qps.giverNpcId = quest->giverNpcId;
            qps.state = quest->state;
            currentGameState.questStates[quest->questId] = qps;
        }
    }

    LOGI("Quest states gathered: %zu quests", currentGameState.questStates.size());
}

void GameManager::gatherModStates() {
    // Gather MOD-specific state
    // - Active MODs list
    // - Companion states
    // - Pet states
    // - NPC relationships (CSR)
    // - Game balance settings

    LOGI("MOD states gathered");
}

// ============================================================================
// Private: Game State Application
// ============================================================================

bool GameManager::applyGameState(const GameState& state) {
    LOGI("Applying game state to all systems");

    try {
        applyPlayerState(state.playerState);
        applyWorldState(state.worldState);
        applyNpcStates(state.npcStates);
        applyQuestStates(state.questStates);
        applyModStates(state);

        LOGI("Game state applied successfully");
        return true;
    } catch (const std::exception& e) {
        LOGE("Exception while applying game state: %s", e.what());
        return false;
    }
}

void GameManager::applyPlayerState(const PlayerState& state) {
    // Apply player position, inventory, status
    // TODO: Implement when Player class is available
    LOGI("Player state applied");
}

void GameManager::applyWorldState(const WorldState& state) {
    if (!worldManager) {
        LOGW("WorldManager not available");
        return;
    }

    // Apply world time, weather, load cells
    // TODO: Implement when WorldManager has setter methods
    LOGI("World state applied");
}

void GameManager::applyNpcStates(const std::map<uint32_t, NPCState>& states) {
    if (!npcManager) {
        LOGW("NpcManager not available");
        return;
    }

    for (const auto& [npcId, npcState] : states) {
        auto npc = npcManager->getNPC(npcId);
        if (npc) {
            npc->position = npcState.position;
            npc->status = npcState.status;
            npc->aiState = npcState.aiState;
        }
    }
    LOGI("NPC states applied: %zu NPCs", states.size());
}

void GameManager::applyQuestStates(const std::map<uint32_t, QuestProgressState>& states) {
    if (!questManager) {
        LOGW("QuestManager not available");
        return;
    }

    for (const auto& [questId, qps] : states) {
        auto quest = questManager->getQuest(questId);
        if (quest) {
            quest->state = qps.state;
        }
    }
    LOGI("Quest states applied: %zu quests", states.size());
}

void GameManager::applyModStates(const GameState& state) {
    LOGI("Applying MOD states");

    // Apply active MODs list
    LOGI("Active MODs: %zu", state.activeMods.size());

    // Apply companion states
    LOGI("Companions: %zu", state.companionStates.size());

    // Apply pet states
    LOGI("Pets: %zu", state.petStates.size());

    // Apply NPC relationships (CSR)
    LOGI("NPC Relationships: %zu", state.npcRelationships.size());

    // Apply game balance settings
    LOGI("Game balance: capacity x%.2f, damage x%.2f",
         state.gameBalance.carryCapacityMultiplier,
         state.gameBalance.damageMultiplier);

    LOGI("MOD states applied");
}

// ============================================================================
// Private: Game State Initialization
// ============================================================================

GameState GameManager::createDefaultGameState(const std::string& characterName) {
    GameState state;

    // Metadata
    state.saveName = characterName;
    state.saveTimestamp = getCurrentTimestamp();
    state.version = "0.6.0";
    state.checksum = 0;

    // Player State
    state.playerState.position = glm::vec3(0.0f, 0.0f, 0.0f);
    state.playerState.currentCell = 0;
    state.playerState.playerLevel = 1;
    state.playerState.experiencePoints = 0.0f;

    // Character Status initialization
    state.playerState.status.currentHealth = 100.0f;
    state.playerState.status.maxHealth = 100.0f;
    state.playerState.status.currentMana = 50.0f;
    state.playerState.status.maxMana = 50.0f;
    state.playerState.status.stamina = 100.0f;
    state.playerState.status.maxStamina = 100.0f;

    // World State
    state.worldState.timeOfDay = 12.0f;
    state.worldState.dayCount = 0;
    state.worldState.currentWeather = "clear";

    // MOD States - Initialize empty, will be populated when MODs are active
    state.activeMods.clear();
    state.modVersions.clear();
    state.companionStates.clear();
    state.petStates.clear();
    state.learnedModSpells.clear();
    state.npcRelationships.clear();

    LOGI("Default game state created for player: %s", characterName.c_str());
    return state;
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string getCurrentTimestamp() {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

