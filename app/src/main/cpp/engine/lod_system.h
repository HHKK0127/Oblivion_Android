#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace oblivion {

/**
 * LOD (Level of Detail) configuration
 */
struct LODConfig {
    float distances[4] = {10.0f, 25.0f, 50.0f, 100.0f};  // Distance thresholds for each LOD level
    float qualityFactors[4] = {1.0f, 0.75f, 0.5f, 0.25f}; // Quality reduction factors
    bool enabled = true;
    float hysteresis = 2.0f;  // Prevent LOD flickering
};

/**
 * LOD level for a mesh
 */
struct LODLevel {
    int vertexCount;
    int indexCount;
    float screenSize;  // Minimum screen size to use this LOD
    void* meshData;    // Pointer to mesh data
};

/**
 * LOD node in the scene
 */
struct LODNode {
    uint32_t id;
    float boundingRadius;
    std::vector<LODLevel> levels;
    int currentLevel;
    float lastSwitchDistance;
};

/**
 * LOD system for managing mesh detail levels
 * Reduces polygon count for distant objects
 */
class LODSystem {
public:
    LODSystem();
    ~LODSystem();

    /**
     * Initialize LOD system with configuration
     */
    void initialize(const LODConfig& config);

    /**
     * Update LOD levels based on camera position
     * @param cameraX Camera X position
     * @param cameraY Camera Y position
     * @param cameraZ Camera Z position
     * @param fov Field of view in degrees
     * @param screenHeight Screen height in pixels
     */
    void update(float cameraX, float cameraY, float cameraZ, 
                float fov, float screenHeight);

    /**
     * Register a mesh for LOD management
     * @param id Unique identifier for the mesh
     * @param boundingRadius Bounding sphere radius
     * @param levels LOD levels (from highest to lowest detail)
     * @return true if registration succeeded
     */
    bool registerMesh(uint32_t id, float boundingRadius, 
                      const std::vector<LODLevel>& levels);

    /**
     * Unregister a mesh from LOD management
     */
    void unregisterMesh(uint32_t id);

    /**
     * Get current LOD level for a mesh
     * @param id Mesh identifier
     * @return LOD level index (0 = highest detail), or -1 if not found
     */
    int getCurrentLOD(uint32_t id) const;

    /**
     * Get LOD level for a mesh at a specific distance
     */
    int getLODAtDistance(uint32_t id, float distance) const;

    /**
     * Force a specific LOD level for a mesh
     */
    void forceLOD(uint32_t id, int level);

    /**
     * Reset forced LOD level
     */
    void resetForcedLOD(uint32_t id);

    /**
     * Enable or disable LOD system
     */
    void setEnabled(bool enabled);

    /**
     * Check if LOD system is enabled
     */
    bool isEnabled() const;

    /**
     * Get LOD configuration
     */
    const LODConfig& getConfig() const;

    /**
     * Update LOD configuration
     */
    void setConfig(const LODConfig& config);

    /**
     * Get statistics
     */
    struct Statistics {
        int totalMeshes;
        int meshesAtLOD0;
        int meshesAtLOD1;
        int meshesAtLOD2;
        int meshesAtLOD3;
        float averageLOD;
    };
    Statistics getStatistics() const;

    /**
     * Clear all registered meshes
     */
    void clear();

private:
    /**
     * Calculate screen-space size of an object
     */
    float calculateScreenSize(float distance, float boundingRadius, 
                              float fov, float screenHeight) const;

    /**
     * Determine LOD level based on screen size
     */
    int determineLODLevel(float screenSize, const std::vector<LODLevel>& levels) const;

    LODConfig config_;
    std::unordered_map<uint32_t, LODNode> nodes_;
    std::unordered_map<uint32_t, int> forcedLODs_;
    bool enabled_;
    
    // Camera state
    float cameraX_, cameraY_, cameraZ_;
    float fov_;
    float screenHeight_;
};

} // namespace oblivion
