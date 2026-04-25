#include "cell_loader.h"
#include "../assets/asset_manager.h"
#include <cstring>

// ============================================================================
// CellLoader Implementation
// ============================================================================

CellLoader::CellLoader()
    : assetManager(nullptr), isInitialized(false) {
}

CellLoader::~CellLoader() {
    cleanup();
}

bool CellLoader::initialize(const std::string& dataPath, AssetManager* assetMgr) {
    gameDataPath = dataPath;
    assetManager = assetMgr;
    isInitialized = true;

    LOGI_LOADER("CellLoader initialized with data path: %s", gameDataPath.c_str());
    return true;
}

void CellLoader::cleanup() {
    isInitialized = false;
    gameDataPath.clear();
    assetManager = nullptr;
}

// ============================================================================
// Cell Data Loading
// ============================================================================

bool CellLoader::loadCellData(std::shared_ptr<Cell> cell) {
    if (!cell) {
        LOGE_LOADER("Cannot load null cell");
        return false;
    }

    LOGD_LOADER("Loading cell data: %s", cell->cellName.c_str());

    // For Phase 3: Generate test data
    // In Phase 4+: Load from ESM file

    // Generate terrain
    cell->heightData = generateFlatTerrain();

    // Load NPCs and objects
    loadNpcsForCell(cell);
    loadObjectsForCell(cell);

    LOGD_LOADER("Cell data loaded: %s", cell->cellName.c_str());
    return true;
}

bool CellLoader::loadNpcsForCell(std::shared_ptr<Cell> cell) {
    if (!cell) return false;

    // Phase 3: No NPCs loaded yet
    // Phase 4+: Load NPC instances from ESM

    LOGD_LOADER("Loaded %zu NPCs for cell %u", cell->npcs.size(), cell->cellId);
    return true;
}

bool CellLoader::loadObjectsForCell(std::shared_ptr<Cell> cell) {
    if (!cell) return false;

    // Phase 3: No objects loaded yet
    // Phase 4+: Load object instances from ESM

    LOGD_LOADER("Loaded %zu objects for cell %u",
                cell->staticObjects.size() + cell->dynamicObjects.size(),
                cell->cellId);
    return true;
}

bool CellLoader::generateTerrain(std::shared_ptr<Cell> cell) {
    if (!cell) return false;

    // Generate flat terrain for now
    cell->heightData = generateFlatTerrain();

    LOGD_LOADER("Generated terrain for cell %u", cell->cellId);
    return true;
}

// ============================================================================
// Terrain Generation
// ============================================================================

std::vector<float> CellLoader::generateFlatTerrain() {
    // Create flat terrain (height = 0)
    int32_t gridSize = TERRAIN_RESOLUTION;
    std::vector<float> heightData(gridSize * gridSize, 0.0f);

    return heightData;
}

std::vector<float> CellLoader::loadHeightmapFromESM(int32_t cellX, int32_t cellY) {
    // Phase 4+: Load heightmap from ESM file
    // For now, return flat terrain

    LOGD_LOADER("Loading heightmap for cell (%d, %d)", cellX, cellY);

    return generateFlatTerrain();
}

// ============================================================================
// Helper Methods
// ============================================================================

float CellLoader::getTerrainHeight(const std::vector<float>& heightData,
                                 float localX, float localZ) {
    // Clamp to valid range
    if (localX < 0.0f) localX = 0.0f;
    if (localX >= CELL_SIZE) localX = CELL_SIZE - 1.0f;
    if (localZ < 0.0f) localZ = 0.0f;
    if (localZ >= CELL_SIZE) localZ = CELL_SIZE - 1.0f;

    // Convert to grid coordinates
    float gridX = (localX / CELL_SIZE) * (TERRAIN_RESOLUTION - 1);
    float gridZ = (localZ / CELL_SIZE) * (TERRAIN_RESOLUTION - 1);

    int32_t gridXi = static_cast<int32_t>(gridX);
    int32_t gridZi = static_cast<int32_t>(gridZ);

    // Clamp to grid bounds
    if (gridXi >= TERRAIN_RESOLUTION - 1) gridXi = TERRAIN_RESOLUTION - 2;
    if (gridZi >= TERRAIN_RESOLUTION - 1) gridZi = TERRAIN_RESOLUTION - 2;

    // Get height at index (for now, return flat terrain)
    if (heightData.empty()) return 0.0f;

    int32_t idx = gridZi * TERRAIN_RESOLUTION + gridXi;
    if (idx >= 0 && idx < static_cast<int32_t>(heightData.size())) {
        return heightData[idx];
    }

    return 0.0f;
}

// ============================================================================
// Private Methods - ESM Parsing (Phase 4+)
// ============================================================================

bool CellLoader::parseEsmHeader() {
    // Phase 4+: Implement ESM header parsing
    LOGD_LOADER("ESM header parsing not yet implemented");
    return false;
}

bool CellLoader::parseCellRecords() {
    // Phase 4+: Implement cell record parsing
    LOGD_LOADER("Cell record parsing not yet implemented");
    return false;
}

bool CellLoader::parseNpcRecords() {
    // Phase 4+: Implement NPC record parsing
    LOGD_LOADER("NPC record parsing not yet implemented");
    return false;
}

bool CellLoader::parseObjectRecords() {
    // Phase 4+: Implement object record parsing
    LOGD_LOADER("Object record parsing not yet implemented");
    return false;
}
