#include "world_builder.h"
#include "world_manager.h"
#include "../assets/asset_manager.h"

// ============================================================================
// WorldBuilder Implementation
// ============================================================================

WorldBuilder::WorldBuilder()
    : worldManager(nullptr), assetManager(nullptr), isInitialized(false) {
    resetStats();
}

WorldBuilder::~WorldBuilder() {
    cleanup();
}

bool WorldBuilder::initialize(WorldManager* worldMgr, AssetManager* assetMgr) {
    worldManager = worldMgr;
    assetManager = assetMgr;

    if (!initializeSubSystems()) {
        LOGE_WB("Failed to initialize sub-systems");
        return false;
    }

    isInitialized = true;
    resetStats();

    LOGI_WB("WorldBuilder initialized");
    return true;
}

void WorldBuilder::cleanup() {
    if (landscapeRenderer) {
        landscapeRenderer->cleanup();
        landscapeRenderer = nullptr;
    }

    if (objectPlacer) {
        objectPlacer->cleanup();
        objectPlacer = nullptr;
    }

    if (textureManager) {
        textureManager->cleanup();
        textureManager = nullptr;
    }

    if (lodSystem) {
        lodSystem->cleanup();
        lodSystem = nullptr;
    }

    isInitialized = false;
    worldManager = nullptr;
    assetManager = nullptr;

    LOGI_WB("WorldBuilder cleaned up");
}

// ============================================================================
// World Building
// ============================================================================

bool WorldBuilder::buildWorld(const oblivion::ESMManager& esmMgr) {
    if (!isInitialized) {
        LOGE_WB("WorldBuilder not initialized");
        return false;
    }

    LOGI_WB("Building world from ESM data...");

    // Get all cells from ESM
    const auto& cells = esmMgr.getAllCells();
    LOGI_WB("Found %zu cells in ESM", cells.size());

    // Build each cell
    uint32_t builtCount = 0;
    for (const auto& cellData : cells) {
        // Create or get cell in WorldManager
        auto cell = worldManager->addCellFromESM(
            cellData.gridX, cellData.gridY,
            cellData.editorID, cellData.fullName,
            cellData.formID
        );

        if (cell && buildCell(cell, esmMgr)) {
            builtCount++;
        }
    }

    LOGI_WB("World built: %u cells processed", builtCount);
    return true;
}

bool WorldBuilder::buildCell(std::shared_ptr<Cell> cell,
                               const oblivion::ESMManager& esmMgr) {
    if (!cell) {
        LOGE_WB("Cannot build null cell");
        return false;
    }

    LOGD_WB("Building cell %u (%d, %d)", cell->cellId, cell->cellX, cell->cellY);

    // Build terrain
    if (!buildCellTerrain(cell, esmMgr)) {
        LOGW_WB("Failed to build terrain for cell %u", cell->cellId);
    }

    // Build objects
    if (!buildCellObjects(cell, esmMgr)) {
        LOGW_WB("Failed to build objects for cell %u", cell->cellId);
    }

    // Update statistics
    stats.cellsBuilt++;
    stats.totalObjectsPlaced += cell->staticObjects.size() + cell->dynamicObjects.size();
    stats.totalTerrainVertices += cell->heightData.size();

    // Calculate memory usage
    cell->memoryUsage = calculateCellMemory(cell);
    stats.totalMemoryUsage += cell->memoryUsage;

    LOGD_WB("Cell %u built: %zu static, %zu dynamic objects, %zu terrain vertices",
            cell->cellId, cell->staticObjects.size(),
            cell->dynamicObjects.size(), cell->heightData.size());

    return true;
}

bool WorldBuilder::rebuildCell(std::shared_ptr<Cell> cell,
                                 const oblivion::ESMManager& esmMgr) {
    if (!cell) return false;

    LOGI_WB("Rebuilding cell %u", cell->cellId);

    // Unload existing data
    onCellUnloaded(cell);

    // Rebuild
    return buildCell(cell, esmMgr);
}

// ============================================================================
// Cell Lifecycle
// ============================================================================

bool WorldBuilder::onCellLoaded(std::shared_ptr<Cell> cell,
                                  const oblivion::ESMManager& esmMgr) {
    if (!cell) return false;

    LOGD_WB("Cell %u loaded, building world data", cell->cellId);
    return buildCell(cell, esmMgr);
}

void WorldBuilder::onCellUnloaded(std::shared_ptr<Cell> cell) {
    if (!cell) return;

    LOGD_WB("Cell %u unloading, cleaning up", cell->cellId);

    // Clear terrain cache for this cell
    if (landscapeRenderer) {
        landscapeRenderer->clearCache();
    }

    // Clear cell objects
    cell->staticObjects.clear();
    cell->dynamicObjects.clear();
    cell->heightData.clear();
    cell->memoryUsage = 0;
}

// ============================================================================
// Update & Render
// ============================================================================

void WorldBuilder::update(float deltaTime, const glm::vec3& cameraPos) {
    if (!isInitialized) return;

    // Update LOD system frustum
    // Note: Would need view-projection matrix from renderer
    // For now, just update LOD based on distance

    // Update texture manager cache
    if (textureManager) {
        // Periodic cache maintenance could go here
    }
}

void WorldBuilder::render(const glm::mat4& viewProj) {
    if (!isInitialized) return;

    // Render terrain for all active cells
    if (landscapeRenderer && worldManager) {
        const auto& activeCells = worldManager->getActiveCells();
        for (auto& cell : activeCells) {
            if (cell && cell->isLoaded()) {
                landscapeRenderer->renderCell(cell, viewProj, nullptr);
            }
        }
    }
}

// ============================================================================
// Statistics
// ============================================================================

void WorldBuilder::resetStats() {
    stats = WorldStats();
}

// ============================================================================
// Private Methods
// ============================================================================

bool WorldBuilder::initializeSubSystems() {
    // Initialize LandscapeRenderer
    landscapeRenderer = std::make_unique<LandscapeRenderer>();
    if (!landscapeRenderer->initialize(assetManager)) {
        LOGE_WB("Failed to initialize LandscapeRenderer");
        return false;
    }
    LOGI_WB("LandscapeRenderer initialized");

    // Initialize ObjectPlacer
    objectPlacer = std::make_unique<ObjectPlacer>();
    if (!objectPlacer->initialize(assetManager)) {
        LOGE_WB("Failed to initialize ObjectPlacer");
        return false;
    }
    LOGI_WB("ObjectPlacer initialized");

    // Initialize TextureManager
    textureManager = std::make_unique<TextureManager>();
    if (!textureManager->initialize(assetManager)) {
        LOGE_WB("Failed to initialize TextureManager");
        return false;
    }
    LOGI_WB("TextureManager initialized");

    // Initialize LODSystem
    lodSystem = std::make_unique<LODSystem>();
    if (!lodSystem->initialize()) {
        LOGE_WB("Failed to initialize LODSystem");
        return false;
    }
    LOGI_WB("LODSystem initialized");

    return true;
}

bool WorldBuilder::buildCellTerrain(std::shared_ptr<Cell> cell,
                                      const oblivion::ESMManager& esmMgr) {
    if (!cell || !landscapeRenderer) return false;

    return landscapeRenderer->generateCellTerrain(cell, esmMgr);
}

bool WorldBuilder::buildCellObjects(std::shared_ptr<Cell> cell,
                                      const oblivion::ESMManager& esmMgr) {
    if (!cell || !objectPlacer) return false;

    return objectPlacer->placeObjectsForCell(cell, esmMgr);
}

size_t WorldBuilder::calculateCellMemory(std::shared_ptr<Cell> cell) const {
    if (!cell) return 0;

    size_t total = 0;

    // Terrain height data
    total += cell->heightData.size() * sizeof(float);

    // Static objects
    total += cell->staticObjects.size() * sizeof(WorldObject);

    // Dynamic objects
    total += cell->dynamicObjects.size() * sizeof(WorldObject);

    // NPC data
    total += cell->npcs.size() * sizeof(void*);  // Shared pointers

    return total;
}
