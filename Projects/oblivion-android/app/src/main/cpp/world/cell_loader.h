#pragma once

#include "world_data.h"
#include <string>
#include <memory>
#include <android/log.h>

// Forward declarations
class AssetManager;

#define LOG_TAG_LOADER "CellLoader"
#define LOGD_LOADER(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_LOADER, __VA_ARGS__)
#define LOGI_LOADER(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_LOADER, __VA_ARGS__)
#define LOGW_LOADER(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_LOADER, __VA_ARGS__)
#define LOGE_LOADER(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_LOADER, __VA_ARGS__)

// ============================================================================
// Cell Loader - Handles loading cell data from ESM and generating content
// ============================================================================

class CellLoader {
public:
    CellLoader();
    ~CellLoader();

    // ========================================================================
    // Initialization
    // ========================================================================

    bool initialize(const std::string& gameDataPath, AssetManager* assetMgr);
    void cleanup();

    // ========================================================================
    // Cell Data Loading
    // ========================================================================

    // Load cell from ESM (full implementation in Phase 4+)
    // Currently returns test data
    bool loadCellData(std::shared_ptr<Cell> cell);

    // Load NPCs for a cell
    bool loadNpcsForCell(std::shared_ptr<Cell> cell);

    // Load objects for a cell
    bool loadObjectsForCell(std::shared_ptr<Cell> cell);

    // Generate terrain mesh
    bool generateTerrain(std::shared_ptr<Cell> cell);

    // ========================================================================
    // Terrain Generation
    // ========================================================================

    // Generate flat terrain with optional randomization
    std::vector<float> generateFlatTerrain();

    // Generate heightmap from ESM data
    std::vector<float> loadHeightmapFromESM(int32_t cellX, int32_t cellY);

    // ========================================================================
    // Helper Methods
    // ========================================================================

    // Get terrain height at local position
    float getTerrainHeight(const std::vector<float>& heightData,
                          float localX, float localZ);

    // Check if ESM file exists
    bool esmExists() const { return !gameDataPath.empty(); }

private:
    // ========================================================================
    // Member Variables
    // ========================================================================

    std::string gameDataPath;       // Path to Oblivion Data folder
    AssetManager* assetManager;     // For loading meshes/textures
    bool isInitialized;

    // ========================================================================
    // Private Methods - ESM Parsing (Phase 4+)
    // ========================================================================

    // Parse ESM header
    bool parseEsmHeader();

    // Parse CELL records
    bool parseCellRecords();

    // Parse NPC/creature instances
    bool parseNpcRecords();

    // Parse object instances
    bool parseObjectRecords();
};
