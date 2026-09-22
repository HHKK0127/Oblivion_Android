#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <memory>

// Forward declarations
struct NPC;
class Mesh;
class Texture;
class Terrain;

// ============================================================================
// World Constants
// ============================================================================

// Cell dimensions. Oblivion exterior cells are 4096 game units across, which is
// also the unit used by ESM reference positions and by the renderer's cell math.
constexpr int32_t CELL_SIZE = 4096;             // Units per cell
constexpr int32_t TERRAIN_RESOLUTION = 33;      // Terrain grid resolution per cell (33x33)

// Load/Unload radius. These are compared against the horizontal distance from
// the player to the cell CENTRE, so one cell of separation is exactly CELL_SIZE.
constexpr float DEFAULT_CELL_LOAD_RADIUS = 6144.0f;     // 1.5 cells: covers the whole 3x3 block (diagonal = 5793)
constexpr float DEFAULT_CELL_UNLOAD_RADIUS = 8192.0f;   // 2 cells away (hysteresis against the load radius)

// Maximum concurrent loaded cells
constexpr int32_t MAX_ACTIVE_CELLS = 9;        // 3x3 grid of cells

// Memory constraints
constexpr size_t MAX_CELL_MEMORY_USAGE = 500 * 1024 * 1024;  // 500 MB total

// ============================================================================
// Enums & Types
// ============================================================================

enum class CellType : uint8_t {
    INTERIOR = 0,       // Dungeon, building interior
    EXTERIOR = 1        // Outdoor world
};

enum class CellLoadState : uint8_t {
    UNLOADED = 0,
    LOADING = 1,
    LOADED = 2,
    UNLOADING = 3
};

// ============================================================================
// World Objects - Base class for objects placed in cells
// ============================================================================

struct WorldObject {
    uint32_t objectId;
    std::string objectName;
    std::string modelPath;          // Path to NIF file

    glm::vec3 position;
    glm::vec3 rotation;
    float scale;

    std::shared_ptr<Mesh> mesh;     // Cached mesh
    bool isStatic;                  // Static vs dynamic objects
    bool isInteractable;            // Can player interact

    // Constructor
    WorldObject()
        : objectId(0), position(0.0f, 0.0f, 0.0f), rotation(0.0f, 0.0f, 0.0f), scale(1.0f),
          isStatic(true), isInteractable(false) {}
};

// ============================================================================
// Cell - Core unit of the world
// ============================================================================

struct Cell {
    // Identification
    uint32_t cellId;
    uint32_t tesFormID;             // TES4 FormID from ESM (for matching LAND/REFR data)
    uint32_t worldspaceFormID;      // Owning WRLD FormID (0 = interior / not from ESM)
    std::string cellName;
    std::string editorID;           // EDM editor ID (EDID subrecord)
    CellType cellType;
    int32_t cellX, cellY;           // Grid coordinates (exterior only)

    // Load state
    CellLoadState loadState;
    float distanceFromPlayer;

    // World elements
    std::vector<std::shared_ptr<NPC>> npcs;
    std::vector<std::shared_ptr<WorldObject>> staticObjects;
    std::vector<std::shared_ptr<WorldObject>> dynamicObjects;

    // Terrain
    std::shared_ptr<Terrain> terrain;
    std::vector<float> heightData;          // 33x33 height values in game units
    // LTEX formIDs for the four LAND quadrants, indexed 0=SW, 1=SE, 2=NW, 3=NE.
    uint32_t landscapeTextures[4] = {0, 0, 0, 0};

    // Resource management
    size_t memoryUsage;
    bool isDirty;                   // Needs update

    // Metadata
    std::string weatherType;        // "Clear", "Rain", "Snow"
    uint32_t ambientColor;          // RGBA format

    // Constructor
    Cell()
        : cellId(0), tesFormID(0), worldspaceFormID(0), cellType(CellType::EXTERIOR),
          cellX(0), cellY(0),
          loadState(CellLoadState::UNLOADED), distanceFromPlayer(FLT_MAX),
          memoryUsage(0), isDirty(true), weatherType("Clear"),
          ambientColor(0xFFFFFFFF) {}

    // Methods
    bool isActive() const { return loadState == CellLoadState::LOADED; }
    bool isLoading() const { return loadState == CellLoadState::LOADING; }
    bool isLoaded() const { return loadState == CellLoadState::LOADED; }
    float getMemoryUsageMB() const { return static_cast<float>(memoryUsage) / (1024.0f * 1024.0f); }

    // Get terrain height at local cell coordinates (0 .. CELL_SIZE)
    float getTerrainHeightAt(float localX, float localY) const {
        if (heightData.size() != static_cast<size_t>(TERRAIN_RESOLUTION * TERRAIN_RESOLUTION)) {
            return 0.0f;  // Default if no height data
        }

        const float step = static_cast<float>(CELL_SIZE) / (TERRAIN_RESOLUTION - 1);
        int gridX = static_cast<int>(std::min(static_cast<float>(TERRAIN_RESOLUTION - 1),
                                             std::max(0.0f, localX / step)));
        int gridY = static_cast<int>(std::min(static_cast<float>(TERRAIN_RESOLUTION - 1),
                                             std::max(0.0f, localY / step)));

        return heightData[gridY * TERRAIN_RESOLUTION + gridX];
    }
};

// ============================================================================
// Cell Grid - Spatial indexing
// ============================================================================

struct CellCoord {
    int32_t x, y;

    CellCoord() : x(0), y(0) {}
    CellCoord(int32_t cx, int32_t cy) : x(cx), y(cy) {}

    bool operator==(const CellCoord& other) const {
        return x == other.x && y == other.y;
    }

    uint64_t toKey() const {
        return (static_cast<uint64_t>(x) << 32) | static_cast<uint32_t>(y);
    }
};

// ============================================================================
// World State
// ============================================================================

struct WorldState {
    glm::vec3 playerPosition;
    glm::vec3 playerRotation;

    float timeOfDay;                // 0.0 - 24.0
    float weatherIntensity;         // 0.0 - 1.0
    std::string currentWeather;

    uint32_t dayCount;

    WorldState()
        : playerPosition(0.0f, 0.0f, 0.0f), playerRotation(0.0f, 0.0f, 0.0f),
          timeOfDay(12.0f), weatherIntensity(0.0f),
          currentWeather("Clear"), dayCount(0) {}
};
