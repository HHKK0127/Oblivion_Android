#pragma once

#include "world_data.h"
#include "landscape_renderer.h"
#include "object_placer.h"
#include "texture_manager.h"
#include "lod_system.h"
#include "../assets/esm_reader.h"
#include <memory>
#include <vector>
#include <android/log.h>

// Forward declarations
class WorldManager;
class AssetManager;

#define LOG_TAG_WORLDBUILDER "WorldBuilder"
#define LOGD_WB(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_WORLDBUILDER, __VA_ARGS__)
#define LOGI_WB(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_WORLDBUILDER, __VA_ARGS__)
#define LOGW_WB(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_WORLDBUILDER, __VA_ARGS__)
#define LOGE_WB(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_WORLDBUILDER, __VA_ARGS__)

// ============================================================================
// World Builder - Integrates all world systems
// ============================================================================

class WorldBuilder {
public:
    WorldBuilder();
    ~WorldBuilder();

    // ========================================================================
    // Initialization
    // ========================================================================

    bool initialize(WorldManager* worldMgr, AssetManager* assetMgr);
    void cleanup();

    // ========================================================================
    // World Building
    // ========================================================================

    // Build world from ESM data
    bool buildWorld(const oblivion::ESMManager& esmMgr);

    // Build a single cell from ESM data
    bool buildCell(std::shared_ptr<Cell> cell,
                    const oblivion::ESMManager& esmMgr);

    // Rebuild cell (unload and reload)
    bool rebuildCell(std::shared_ptr<Cell> cell,
                      const oblivion::ESMManager& esmMgr);

    // ========================================================================
    // Cell Lifecycle
    // ========================================================================

    // Called when a cell is loaded
    bool onCellLoaded(std::shared_ptr<Cell> cell,
                       const oblivion::ESMManager& esmMgr);

    // Called when a cell is unloaded
    void onCellUnloaded(std::shared_ptr<Cell> cell);

    // ========================================================================
    // Update & Render
    // ========================================================================

    // Update world systems (LOD, culling, etc.)
    void update(float deltaTime, const glm::vec3& cameraPos);

    // Render world
    void render(const glm::mat4& viewProj);

    // ========================================================================
    // System Access
    // ========================================================================

    LandscapeRenderer* getLandscapeRenderer() { return landscapeRenderer.get(); }
    ObjectPlacer* getObjectPlacer() { return objectPlacer.get(); }
    TextureManager* getTextureManager() { return textureManager.get(); }
    LODSystem* getLODSystem() { return lodSystem.get(); }

    // ========================================================================
    // Statistics
    // ========================================================================

    struct WorldStats {
        uint32_t cellsBuilt = 0;
        uint32_t totalObjectsPlaced = 0;
        uint32_t totalTerrainVertices = 0;
        size_t totalMemoryUsage = 0;
    };

    const WorldStats& getStats() const { return stats; }
    void resetStats();

private:
    // ========================================================================
    // Member Variables
    // ========================================================================

    WorldManager* worldManager;
    AssetManager* assetManager;
    bool isInitialized;

    // Sub-systems
    std::unique_ptr<LandscapeRenderer> landscapeRenderer;
    std::unique_ptr<ObjectPlacer> objectPlacer;
    std::unique_ptr<TextureManager> textureManager;
    std::unique_ptr<LODSystem> lodSystem;

    // Statistics
    WorldStats stats;

    // ========================================================================
    // Private Methods
    // ========================================================================

    // Initialize sub-systems
    bool initializeSubSystems();

    // Build terrain for a cell
    bool buildCellTerrain(std::shared_ptr<Cell> cell,
                           const oblivion::ESMManager& esmMgr);

    // Build objects for a cell
    bool buildCellObjects(std::shared_ptr<Cell> cell,
                           const oblivion::ESMManager& esmMgr);

    // Calculate cell memory usage
    size_t calculateCellMemory(std::shared_ptr<Cell> cell) const;
};
