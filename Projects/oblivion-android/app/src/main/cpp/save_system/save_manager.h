#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace SaveSystem {

// Forward declaration
class SaveValidator;

// ============================================================================
// Save Slot Information
// ============================================================================

struct SaveSlot {
    int slotId;
    std::string characterName;
    int playerLevel;
    std::string lastLocation;      // Current cell name/location
    std::string saveTime;          // Timestamp of save
    float playTime;                // Total playtime in hours
    bool isEmpty;
    std::string savePath;          // Full path to this slot's save file

    SaveSlot() :
        slotId(-1),
        playerLevel(0),
        playTime(0.0f),
        isEmpty(true) {}

    SaveSlot(int id) :
        slotId(id),
        playerLevel(0),
        playTime(0.0f),
        isEmpty(true) {}
};

// ============================================================================
// Save Manager Class
// ============================================================================

class SaveManager {
public:
    SaveManager();
    ~SaveManager();

    /**
     * Initialize the SaveManager and create save directories
     * @return true if initialization successful, false otherwise
     */
    bool initialize();

    /**
     * Save game state to a specific slot
     * @param slotId The save slot ID (0-4 recommended)
     * @param gameState The game state JSON to save
     * @return true if save successful, false otherwise
     */
    bool saveGame(int slotId, const json& gameState);

    /**
     * Load game state from a specific slot
     * @param slotId The save slot ID
     * @param outGameState Output parameter for the loaded game state
     * @return true if load successful, false otherwise
     */
    bool loadGame(int slotId, json& outGameState);

    /**
     * Get all available save slots
     * @return Vector of SaveSlot objects
     */
    std::vector<SaveSlot> getAllSaveSlots() const;

    /**
     * Get information about a specific save slot
     * @param slotId The save slot ID
     * @return SaveSlot object (isEmpty=true if slot empty)
     */
    SaveSlot getSaveSlot(int slotId) const;

    /**
     * Check if a save slot is empty
     * @param slotId The save slot ID
     * @return true if the slot is empty, false if it contains a save
     */
    bool isSaveSlotEmpty(int slotId) const;

    /**
     * Delete a save from a specific slot
     * @param slotId The save slot ID
     * @return true if deletion successful, false otherwise
     */
    bool deleteSaveSlot(int slotId);

    /**
     * Create an auto-save (typically used on cell transitions)
     * Auto-saves allow recovery if the game crashes
     * @param gameState The current game state
     * @return true if auto-save successful, false otherwise
     */
    bool createAutoSave(const json& gameState);

    /**
     * Load the most recent auto-save
     * @param outGameState Output parameter for the auto-save state
     * @return true if auto-save loaded, false if no valid auto-save exists
     */
    bool loadAutoSave(json& outGameState);

    /**
     * Check if an auto-save exists and is valid
     * @return true if a valid auto-save exists, false otherwise
     */
    bool hasAutoSave() const;

    /**
     * Get the base save directory
     * @return Full path to the saves directory
     */
    std::string getSaveDirectory() const;

    /**
     * Get the path to a specific save slot directory
     * @param slotId The save slot ID
     * @return Full path to the slot directory
     */
    std::string getSaveSlotPath(int slotId) const;

    /**
     * Get the path to the auto-save directory
     * @return Full path to the auto-save directory
     */
    std::string getAutoSavePath() const;

    /**
     * Update metadata for a save slot without reloading
     * @param slotId The save slot ID
     * @param slot The SaveSlot with updated metadata
     * @return true if update successful, false otherwise
     */
    bool updateSlotMetadata(int slotId, const SaveSlot& slot);

    /**
     * Get the maximum number of save slots
     * @return Maximum number of save slots (default: 5)
     */
    int getMaxSaveSlots() const { return MAX_SAVE_SLOTS; }

    /**
     * Clear all saves (for testing or new game)
     * WARNING: This is destructive and cannot be undone!
     * @return true if successful, false otherwise
     */
    bool clearAllSaves();

private:
    static constexpr int MAX_SAVE_SLOTS = 5;
    static constexpr const char* SAVE_DIR = "/data/data/com.example.oblivion/files/saves";
    static constexpr const char* AUTO_SAVE_DIR = "/data/data/com.example.oblivion/files/autosave";

    // Validator instance for consistency checking
    std::unique_ptr<SaveValidator> validator;

    // Helper functions
    /**
     * Create necessary directories for saving
     * @param path The directory path to create
     * @return true if successful or already exists, false otherwise
     */
    bool ensureDirectoryExists(const std::string& path) const;

    /**
     * Load metadata from a slot's metadata.json file
     * @param slotId The save slot ID
     * @return SaveSlot object (isEmpty=true if metadata doesn't exist)
     */
    SaveSlot loadSlotMetadata(int slotId) const;

    /**
     * Save metadata to a slot's metadata.json file
     * @param slotId The save slot ID
     * @param slot The SaveSlot metadata to save
     * @return true if successful, false otherwise
     */
    bool saveSlotMetadata(int slotId, const SaveSlot& slot) const;

    /**
     * Generate a timestamp string for saves
     * @return Current date/time as formatted string
     */
    std::string getCurrentTimestamp() const;

    /**
     * Validate a save file before loading
     * @param gameState The loaded game state JSON
     * @return true if valid, false otherwise
     */
    bool validateSaveFile(const json& gameState) const;
};

}  // namespace SaveSystem
