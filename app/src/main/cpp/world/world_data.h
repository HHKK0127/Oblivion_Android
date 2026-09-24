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
// Terrain additive layers (LAND ATXT/VTXT)
// ============================================================================

// One ATXT texture group: a landscape texture (LTEX FormID) painted over one
// quadrant of the BTXT base textures, with per-vertex opacities from the
// following VTXT subrecords. The renderer keeps a dense 33x33 cell-local grid
// so terrain-mesh vertex lookups are O(1); the loader keeps only the sparse
// packed positions until the cell is actually assigned.
struct TerrainAdditiveLayer {
    uint32_t textureFormID = 0;       // LTEX FormID
    uint8_t quadrant = 0;             // 0=SW, 1=SE, 2=NW, 3=NE
    uint8_t layer = 0;                // 0-7 (BTXT is the lowest layer)
    // Dense 33x33 cell-local opacity grid, row-major (gridY*33+gridX), ready to
    // be sampled directly by the terrain mesh builder. Values 0.0-1.0. Empty
    // when the layer carries no painted weights.
    std::vector<float> opacityGrid;
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
    // Dense 33x33 height values in game units. This is a render/query CACHE, not
    // the source of truth: the LAND record in the ESM keeps the compact form
    // (1089 gradient bytes plus sparse layer weights), and only cells that are
    // actually rendered materialise the dense grids. Expanding every cell up
    // front cost ~64 MB of heights plus ~1.3 GB of layer opacity grids for the
    // 14,686 cells of Tamriel while only a handful are ever drawn.
    std::vector<float> heightData;
    // A LAND record exists for this cell (set from the ESM, survives eviction).
    bool hasTerrain = false;
    // heightData/additiveLayers currently hold the expanded dense form.
    bool terrainExpanded = false;
    // LTEX formIDs for the four LAND quadrants, indexed 0=SW, 1=SE, 2=NW, 3=NE.
    uint32_t landscapeTextures[4] = {0, 0, 0, 0};
    // ATXT/VTXT additive texture layers painted on top of the base quadrants.
    // Only populated for cells that actually carry VTXT data.
    std::vector<TerrainAdditiveLayer> additiveLayers;

    // Water
    // Explicit water surface height from the CELL XCLW subrecord (game units).
    // Negative XCLW values decode to NaN and hasWater=false: the cell is
    // treated as having no drawable water surface. No default water level is
    // derived from the parent worldspace (WRLD has no water-height field).
    // Interior cells never carry XCLW, so they default to 0 + hasWater=false.
    float waterLevel = 0.0f;
    bool hasWater = false;
    uint32_t waterTypeFormID = 0;   // WATR FormID resolved for this cell

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
    bool hasDenseTerrain() const {
        return heightData.size() ==
               static_cast<size_t>(TERRAIN_RESOLUTION) * static_cast<size_t>(TERRAIN_RESOLUTION);
    }

    // Drop the dense terrain cache. hasTerrain stays set, so the renderer can
    // rebuild it from the ESM record the next time the cell becomes visible.
    void releaseTerrainExpansion() {
        std::vector<float>().swap(heightData);
        std::vector<TerrainAdditiveLayer>().swap(additiveLayers);
        terrainExpanded = false;
    }

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
