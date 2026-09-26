#pragma once

#include "world_data.h"
#include "world_item.h"
#include "cell.h"
#include <unordered_map>
#include <queue>
#include <vector>
#include <memory>
#include <cmath>
#include <android/log.h>

// Forward declarations
class NpcManager;
class AssetManager;
class CellLoader;
class CellTransitionManager;
class DoorManager;

#define LOG_TAG_WORLD "WorldManager"
#define LOGD_WORLD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_WORLD, __VA_ARGS__)
#define LOGI_WORLD(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_WORLD, __VA_ARGS__)
#define LOGW_WORLD(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_WORLD, __VA_ARGS__)
#define LOGE_WORLD(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_WORLD, __VA_ARGS__)

// ============================================================================
// World Manager - Core world system
// ============================================================================

class WorldManager {
public:
    WorldManager();
    ~WorldManager();

    // ========================================================================
    // Lifecycle Methods
    // ========================================================================

    bool initialize(NpcManager* npcMgr, AssetManager* assetMgr);
    void update(float deltaTime);
    void render();
    void cleanup();

    // ========================================================================
    // Cell Management
    // ========================================================================

    // Load/unload specific cell
    bool loadCell(uint32_t cellId);
    bool loadCell(int32_t cellX, int32_t cellY);
    void unloadCell(uint32_t cellId);
    void unloadCell(int32_t cellX, int32_t cellY);

    // Automatic load/unload based on player position
    void updateActiveCells();

    // ESM-based cell creation
    std::shared_ptr<Cell> addCellFromESM(int32_t cellX, int32_t cellY, 
                                          const std::string& editorID,
                                          const std::string& fullName,
                                          uint32_t tesFormID = 0,
                                          bool isExterior = false,
                                          uint32_t worldspaceFormID = 0);

    // Drop every cell so ESM data can be loaded into a clean world.
    // Only safe while nothing is loaded yet (called during world build).
    void clearAllCells();

    // FormID of the worldspace the player currently inhabits (0 = none).
    void setCurrentWorldspace(uint32_t worldspaceFormID) { currentWorldspaceFormID = worldspaceFormID; }
    uint32_t getCurrentWorldspace() const { return currentWorldspaceFormID; }

    // Place the player in the exterior cell closest to the worldspace origin
    // that actually owns a heightmap, so terrain is visible on first frame.
    bool spawnPlayerAtNearestTerrainCell();

    // Number of exterior cells that own real height data.
    size_t countExteriorCellsWithTerrain() const;

    // ========================================================================
    // Player Position Tracking
    // ========================================================================

    void setPlayerPosition(const glm::vec3& pos);
    glm::vec3 getPlayerPosition() const { return worldState.playerPosition; }

    void setPlayerRotation(const glm::vec3& rot);
    glm::vec3 getPlayerRotation() const { return worldState.playerRotation; }

    // Camera access (for audio system and rendering)
    glm::vec3 getCameraPosition() const { return worldState.playerPosition; }
    glm::vec3 getCameraForward() const {
        // Calculate forward vector from rotation (yaw)
        float yaw = worldState.playerRotation.y * 3.14159f / 180.0f;
        return glm::vec3(std::sin(yaw), 0.0f, std::cos(yaw));
    }

    // ========================================================================
    // Cell Queries
    // ========================================================================

    std::shared_ptr<Cell> getCurrentCell() const { return currentCell; }

    // Phase 66 P15: true when the player is inside an interior cell. Interior
    // cells have no grid coordinate, so getCellAt(playerPosition) cannot find
    // them; the renderer must ask this instead to suppress sky, water and
    // terrain fog while indoors.
    bool isPlayerIndoors() const {
        return currentCell && currentCell->cellType != CellType::EXTERIOR;
    }

    // Phase 66 P15: leave the current interior cell and resume exterior cell
    // streaming at the player's world position. Returns true when an interior
    // was actually left.
    bool leaveInteriorCell();

    std::shared_ptr<Cell> getCellAt(const glm::vec3& worldPos);
    std::shared_ptr<Cell> getCellById(uint32_t cellId);
    std::shared_ptr<Cell> getCellByCoord(int32_t cellX, int32_t cellY);
    std::shared_ptr<Cell> getCell(int32_t cellX, int32_t cellY) { return getCellByCoord(cellX, cellY); }

    const std::vector<std::shared_ptr<Cell>>& getActiveCells() const { return activeCells; }

    // Get all cells (for ESM-based world building)
    const std::unordered_map<uint32_t, std::shared_ptr<Cell>>& getAllCellsMap() const { return cells; }

    // Find cell by TES FormID (ESM formID)
    std::shared_ptr<Cell> getCellByFormID(uint32_t tesFormID) const;

    // Phase 66 P15: register an interior cell and make it the player's current
    // cell. Interior cells have no grid coordinate, so they are keyed by their
    // TES FormID instead of a (cellX, cellY) pair. This is what makes the
    // renderer's interior sky/water suppression paths reachable in game; the
    // normal world build deliberately skips interiors.
    std::shared_ptr<Cell> enterInteriorCell(uint32_t tesFormID,
                                            const std::string& editorID,
                                            const std::string& fullName);

    // Get all NPCs in a specific cell
    std::vector<NPC*> getNpcsInCell(uint32_t cellId);
    std::vector<NPC*> getNpcsInCell(std::shared_ptr<Cell> cell);

    // ========================================================================
    // World State
    // ========================================================================

    const WorldState& getWorldState() const { return worldState; }
    void setWorldState(const WorldState& state) { worldState = state; }

    // Time management
    void advanceTime(float deltaTime);
    float getTimeOfDay() const { return worldState.timeOfDay; }
    uint32_t getDayCount() const { return worldState.dayCount; }

    // ========================================================================
    // Memory Management
    // ========================================================================

    size_t getTotalMemoryUsage() const;
    size_t getCacheSize() const { return cells.size(); }
    void logWorldStatus() const;

    // ========================================================================
    // Manager Access
    // ========================================================================

    NpcManager* getNpcManager() { return npcManager; }
    AssetManager* getAssetManager() { return assetManager; }
    DoorManager* getDoorManager() { return doorManager.get(); }

    // ========================================================================
    // Settings
    // ========================================================================

    void setCellLoadRadius(float radius) { cellLoadRadius = radius; }
    void setCellUnloadRadius(float radius) { cellUnloadRadius = radius; }

    float getCellLoadRadius() const { return cellLoadRadius; }
    float getCellUnloadRadius() const { return cellUnloadRadius; }

    // ========================================================================
    // World Items (Pickupable Objects)
    // ========================================================================

    /**
     * @brief Spawn item in world
     */
    uint32_t spawnWorldItem(uint32_t itemId, const std::string& itemName,
                           const std::string& itemNameJa, const glm::vec3& position);

    /**
     * @brief Get pickupable items around player
     */
    std::shared_ptr<WorldItem> getNearbyPickupItem(const glm::vec3& playerPos,
                                                    float pickupRange = 3.0f) const;

    /**
     * @brief Get nearest item
     */
    std::shared_ptr<WorldItem> getNearestWorldItem(const glm::vec3& playerPos) const;

    /**
     * @brief Pick up item (remove from world)
     */
    bool pickupWorldItem(uint32_t worldItemId);

    /**
     * @brief Get all world items
     */
    const std::vector<std::shared_ptr<WorldItem>>& getWorldItems() const {
        return worldItems;
    }

private:
    // ========================================================================
    // Member Variables
    // ========================================================================

    // Cell storage
    std::unordered_map<uint32_t, std::shared_ptr<Cell>> cells;  // cellId → Cell
    std::unordered_map<uint64_t, uint32_t> coordToId;          // (x,y) → cellId
    std::unordered_map<uint32_t, uint32_t> formIdToCellId;     // TES FormID → cellId

    // Active cells
    std::shared_ptr<Cell> currentCell;
    std::vector<std::shared_ptr<Cell>> activeCells;  // Currently visible cells

    // Loading queue
    std::queue<uint32_t> loadingQueue;
    std::queue<uint32_t> unloadingQueue;

    // Managers
    NpcManager* npcManager;
    AssetManager* assetManager;
    CellManager cellManager;
    std::unique_ptr<CellTransitionManager> cellTransitionManager;
    std::unique_ptr<DoorManager> doorManager;  // NEW: Door system (Task 2)

    // World state
    WorldState worldState;

    // Load/Unload settings
    float cellLoadRadius;
    float cellUnloadRadius;
    float loadUpdateTimer;                  // Batch update every N seconds

    // Counters
    uint32_t nextCellId;
    uint32_t nextWorldItemId;  // For unique WorldItem IDs
    uint32_t cellsLoaded;
    uint32_t cellsUnloaded;

    // Worldspace the player is currently in (0 = interior / unset)
    uint32_t currentWorldspaceFormID;

    // World Items
    std::vector<std::shared_ptr<WorldItem>> worldItems;

    // ========================================================================
    // Private Methods
    // ========================================================================

    // Cell initialization
    void initializeTestCells();
    std::shared_ptr<Cell> createCell(uint32_t cellId, int32_t cellX, int32_t cellY,
                                     const std::string& name, CellType type);

    // Update methods
    void updateCellLoadingQueues(float deltaTime);
    void checkCellDistances();
    void unloadDistantCells();
    void loadNearbyCells();

    // Helper methods
    CellCoord getPlayerCellCoord() const;
    void updateCellDistance(const std::shared_ptr<Cell>& cell, float halfCell);
    bool shouldLoadCell(std::shared_ptr<Cell> cell) const;
    bool shouldUnloadCell(std::shared_ptr<Cell> cell) const;
    uint32_t getOrCreateCellId(int32_t cellX, int32_t cellY);
};
