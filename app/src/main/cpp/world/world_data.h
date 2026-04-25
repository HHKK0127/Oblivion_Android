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

// Cell dimensions
constexpr int32_t CELL_SIZE = 128;              // Units per cell
constexpr int32_t TERRAIN_RESOLUTION = 65;     // Terrain grid resolution per cell (65x65)

// Load/Unload radius
constexpr float DEFAULT_CELL_LOAD_RADIUS = 256.0f;      // 2 cells away
constexpr float DEFAULT_CELL_UNLOAD_RADIUS = 512.0f;    // 4 cells away

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
    std::string cellName;
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
    std::vector<float> heightData;          // 65x65 height values

    // Resource management
    size_t memoryUsage;
    bool isDirty;                   // Needs update

    // Metadata
    std::string weatherType;        // "Clear", "Rain", "Snow"
    uint32_t ambientColor;          // RGBA format

    // Constructor
    Cell()
        : cellId(0), cellType(CellType::EXTERIOR), cellX(0), cellY(0),
          loadState(CellLoadState::UNLOADED), distanceFromPlayer(FLT_MAX),
          memoryUsage(0), isDirty(true), weatherType("Clear"),
          ambientColor(0xFFFFFFFF) {}

    // Methods
    bool isActive() const { return loadState == CellLoadState::LOADED; }
    bool isLoading() const { return loadState == CellLoadState::LOADING; }
    bool isLoaded() const { return loadState == CellLoadState::LOADED; }
    float getMemoryUsageMB() const { return static_cast<float>(memoryUsage) / (1024.0f * 1024.0f); }

    // Get terrain height at local cell coordinates (0-128)
    float getTerrainHeightAt(float localX, float localY) const {
        // Clamp to cell bounds (65x65 heightmap for 128x128 cell)
        int gridX = static_cast<int>(std::min(64.0f, std::max(0.0f, localX / 2.0f)));
        int gridY = static_cast<int>(std::min(64.0f, std::max(0.0f, localY / 2.0f)));

        if (heightData.empty()) {
            return 0.0f;  // Default if no height data
        }

        // Linear interpolation between heightmap samples
        int index = gridY * 65 + gridX;
        if (index < 0 || index >= static_cast<int>(heightData.size())) {
            return 0.0f;
        }

        return heightData[index];
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
