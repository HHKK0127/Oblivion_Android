#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include "game_state.h"

// Forward declarations
namespace SaveSystem { class SaveManager; struct SaveSlot; }
class WorldManager;
class NpcManager;
class QuestManager;
class InventoryManager;
class Player;  // May not exist yet, NPC serves as player

// ============================================================================
// Game Manager - Orchestrates Save/Load and System Integration
// ============================================================================

class GameManager {
public:
    GameManager();
    ~GameManager();

    // ========== Initialization ==========
    /**
     * Initialize all game systems
     * Must be called once at application startup
     */
    bool initialize();

    /**
     * Cleanup game manager and all systems
     */
    void cleanup();

    // ========== Game State Management ==========
    /**
     * Create a new game with default initial state
     */
    bool startNewGame(const std::string& characterName);

    /**
     * Save current game state to a specific slot
     * Gathers state from all systems and serializes to file
     */
    bool saveGame(int slotId, const std::string& saveName = "");

    /**
     * Load game state from a specific slot
     * Deserializes state and applies to all game systems
     */
    bool loadGame(int slotId);

    /**
     * Create an auto-save (called on cell transitions, etc.)
     */
    bool createAutoSave();

    /**
     * Load from auto-save (used for crash recovery)
     */
    bool loadAutoSave();

    // ========== Save Slot Management ==========
    /**
     * Get information about all save slots
     */
    std::vector<SaveSystem::SaveSlot> getAllSaveSlots() const;

    /**
     * Get information about a specific save slot
     */
    SaveSystem::SaveSlot getSaveSlot(int slotId) const;

    /**
     * Check if a save slot is empty
     */
    bool isSaveSlotEmpty(int slotId) const;

    /**
     * Delete a save slot
     */
    bool deleteSaveSlot(int slotId);

    /**
     * Check if an auto-save exists
     */
    bool hasAutoSave() const;

    // ========== Current Game State Access ==========
    /**
     * Get the current game state (for UI display, etc.)
     */
    const GameState& getCurrentGameState() const { return currentGameState; }

    /**
     * Get current game state for modification
     */
    GameState& getGameStateMutable() { return currentGameState; }

    /**
     * Get the save manager instance
     */
    SaveSystem::SaveManager* getSaveManager() { return saveManager.get(); }

    // ========== System References ==========
    /**
     * Get world manager for setting up world state
     */
    WorldManager* getWorldManager() { return worldManager; }

    /**
     * Get NPC manager for managing characters
     */
    NpcManager* getNpcManager() { return npcManager; }

    /**
     * Get quest manager for tracking quest state
     */
    QuestManager* getQuestManager() { return questManager; }

    /**
     * Get inventory manager (if exists)
     */
    InventoryManager* getInventoryManager() { return inventoryManager; }

private:
    // ========== Private Methods ==========
    /**
     * Gather current state from all game systems into GameState
     */
    bool gatherGameState();

    /**
     * Apply loaded GameState to all game systems
     */
    bool applyGameState(const GameState& state);

    /**
     * Initialize default game state for new game
     */
    GameState createDefaultGameState(const std::string& characterName);

    /**
     * Apply player state changes
     */
    void applyPlayerState(const PlayerState& state);

    /**
     * Apply world state changes
     */
    void applyWorldState(const WorldState& state);

    /**
     * Apply NPC states
     */
    void applyNpcStates(const std::map<uint32_t, NPCState>& states);

    /**
     * Apply quest states
     */
    void applyQuestStates(const std::map<uint32_t, QuestProgressState>& states);

    /**
     * Apply MOD states
     */
    void applyModStates(const GameState& state);

    /**
     * Gather player state from player/NPC
     */
    void gatherPlayerState();

    /**
     * Gather world state from world manager
     */
    void gatherWorldState();

    /**
     * Gather NPC states from NPC manager
     */
    void gatherNpcStates();

    /**
     * Gather quest states from quest manager
     */
    void gatherQuestStates();

    /**
     * Gather MOD states from various systems
     */
    void gatherModStates();

    // ========== Member Variables ==========
    std::unique_ptr<SaveSystem::SaveManager> saveManager;

    // System references (not owned by GameManager)
    WorldManager* worldManager;
    NpcManager* npcManager;
    QuestManager* questManager;
    InventoryManager* inventoryManager;

    // Current game state
    GameState currentGameState;

    // Initialization flag
    bool isInitialized;
};

