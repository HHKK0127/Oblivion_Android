#pragma once

#include "esm_reader.h"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <glm/glm.hpp>
#include <android/log.h>

#define LOG_TAG_WDLDR "WorldDataLoader"
#define LOGD_WDLDR(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_WDLDR, __VA_ARGS__)
#define LOGI_WDLDR(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_WDLDR, __VA_ARGS__)
#define LOGW_WDLDR(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_WDLDR, __VA_ARGS__)
#define LOGE_WDLDR(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_WDLDR, __VA_ARGS__)

// ============================================================================
// World Data Loader - ESM CELL/REFR/LAND/WRLD -> world chunk loading
//
// Handles: CELL record -> world chunk, REFR -> positioned objects,
//          LAND heightmap -> terrain mesh, WRLD world space management,
//          streaming loader (load nearby, unload distant)
// ============================================================================

// Oblivion cell size in world units (4096 units per cell)
constexpr float CELL_SIZE = 4096.0f;

// Terrain heightmap grid size (65x65 per cell)
constexpr int TERRAIN_GRID_SIZE = 65;

// Streaming: load cells within this radius (in cells)
constexpr int STREAM_LOAD_RADIUS = 3;

// Streaming: unload cells beyond this radius
constexpr int STREAM_UNLOAD_RADIUS = 5;

// Positioned object in the world
struct WorldObject {
    uint32_t refFormID = 0;         // REFR FormID
    uint32_t baseFormID = 0;        // Base object FormID (STAT, DOOR, NPC_, etc.)
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    float scale = 1.0f;
    std::string modelPath;          // Resolved from base form
    uint16_t flags = 0;
};

// Terrain chunk generated from LAND record
struct TerrainChunk {
    int32_t cellX = 0;
    int32_t cellY = 0;
    float heights[TERRAIN_GRID_SIZE][TERRAIN_GRID_SIZE];
    uint32_t vao = 0;
    uint32_t vbo = 0;
    uint32_t ebo = 0;
    uint32_t indexCount = 0;
    bool gpuUploaded = false;
};

// World cell chunk (CELL + its contents)
struct WorldChunk {
    uint32_t cellFormID = 0;
    int32_t gridX = 0;
    int32_t gridY = 0;
    std::string fullName;
    bool isInterior = false;
    bool loaded = false;

    std::vector<WorldObject> objects;
    std::unique_ptr<TerrainChunk> terrain;
};

// World space (WRLD)
struct WorldSpace {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    int32_t minX = 0, minY = 0;
    int32_t maxX = 0, maxY = 0;
    std::unordered_map<int64_t, std::unique_ptr<WorldChunk>> chunks;
};

// Callback types for streaming events
using ChunkLoadCallback = std::function<void(const WorldChunk&)>;
using ChunkUnloadCallback = std::function<void(uint32_t cellFormID)>;

class WorldDataLoader {
public:
    WorldDataLoader();
    ~WorldDataLoader();

    // Lifecycle
    bool initialize(const oblivion::ESMManager* esmManager);
    void cleanup();

    // ========================================================================
    // World Space Management
    // ========================================================================

    // Load a world space definition from ESM
    bool loadWorldSpace(uint32_t worldFormID);

    // Get loaded world space
    const WorldSpace* getWorldSpace(uint32_t worldFormID) const;

    // Get all loaded world space IDs
    std::vector<uint32_t> getLoadedWorldSpaceIDs() const;

    // ========================================================================
    // CELL -> World Chunk Loading
    // ========================================================================

    // Load a specific cell into a world chunk
    std::shared_ptr<WorldChunk> loadCell(uint32_t cellFormID);

    // Load cell by grid coordinates in a world space
    std::shared_ptr<WorldChunk> loadCellByGrid(uint32_t worldFormID,
                                                 int32_t gridX, int32_t gridY);

    // Get a loaded chunk (returns nullptr if not loaded)
    const WorldChunk* getChunk(uint32_t worldFormID,
                                int32_t gridX, int32_t gridY) const;

    // ========================================================================
    // REFR -> Positioned Objects
    // ========================================================================

    // Extract positioned objects from a cell's references
    std::vector<WorldObject> extractCellObjects(uint32_t cellFormID) const;

    // Resolve model path from base form ID
    std::string resolveModelPath(uint32_t baseFormID) const;

    // ========================================================================
    // LAND -> Terrain Mesh Generation
    // ========================================================================

    // Generate terrain chunk from LAND record
    std::unique_ptr<TerrainChunk> generateTerrain(uint32_t cellFormID);

    // Upload terrain chunk to GPU
    bool uploadTerrainGPU(TerrainChunk& chunk);

    // Free terrain GPU resources
    void freeTerrainGPU(TerrainChunk& chunk);

    // ========================================================================
    // Streaming Loader
    // ========================================================================

    // Update streaming based on player position
    void updateStreaming(const glm::vec3& playerWorldPos, uint32_t worldFormID);

    // Set streaming callbacks
    void setChunkLoadCallback(ChunkLoadCallback callback) {
        onLoadCallback = std::move(callback);
    }
    void setChunkUnloadCallback(ChunkUnloadCallback callback) {
        onUnloadCallback = std::move(callback);
    }

    // Force load/unload radius
    void setLoadRadius(int radius) { loadRadius = radius; }
    void setUnloadRadius(int radius) { unloadRadius = radius; }

    // Get loaded chunk count
    size_t getLoadedChunkCount() const;

    // ========================================================================
    // Interior Cells
    // ========================================================================

    // Load an interior cell
    std::shared_ptr<WorldChunk> loadInteriorCell(uint32_t cellFormID);

    // Check if a cell is interior
    bool isInteriorCell(uint32_t cellFormID) const;

private:
    // ESM manager reference
    const oblivion::ESMManager* esmManager = nullptr;
    bool initialized = false;

    // Loaded world spaces
    std::unordered_map<uint32_t, std::unique_ptr<WorldSpace>> worldSpaces;

    // Loaded interior cells
    std::unordered_map<uint32_t, std::shared_ptr<WorldChunk>> interiorCells;

    // Streaming state
    int32_t lastPlayerCellX = 0;
    int32_t lastPlayerCellY = 0;
    int loadRadius = STREAM_LOAD_RADIUS;
    int unloadRadius = STREAM_UNLOAD_RADIUS;

    // Streaming callbacks
    ChunkLoadCallback onLoadCallback;
    ChunkUnloadCallback onUnloadCallback;

    // Helper: cell grid coords -> key
    static int64_t cellKey(int32_t x, int32_t y) {
        return (static_cast<int64_t>(x) << 32) |
               static_cast<uint32_t>(y);
    }

    // Internal loading helpers
    bool loadCellObjects(WorldChunk& chunk, uint32_t cellFormID) const;
    bool loadCellTerrain(WorldChunk& chunk, uint32_t cellFormID);
    void unloadDistantChunks(WorldSpace& world, int32_t playerX, int32_t playerY);
};
