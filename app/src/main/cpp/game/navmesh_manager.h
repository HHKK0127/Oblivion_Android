#pragma once

#include <vector>
#include <unordered_map>
#include <queue>
#include <cmath>
#include <glm/glm.hpp>
#include "../assets/esm_reader.h"

#define LOG_TAG "NavMeshManager"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {

/**
 * @brief NavMesh-based pathfinding manager
 *
 * Loads NavMesh data from ESM and provides A* pathfinding
 * for NPC AI movement.
 */
class NavMeshManager {
public:
    NavMeshManager();
    ~NavMeshManager();

    /**
     * @brief Load NavMesh data from ESM manager
     * @param esmMgr ESM manager with loaded data
     */
    void loadFromESM(const ESMManager& esmMgr);

    /**
     * @brief Find path between two world positions
     * @param start Start position (world coordinates)
     * @param end End position (world coordinates)
     * @param outPath Output path as list of waypoints
     * @return true if path found
     */
    bool findPath(const glm::vec3& start, const glm::vec3& end,
                  std::vector<glm::vec3>& outPath) const;

    /**
     * @brief Check if a position is on the navmesh
     * @param position World position to check
     * @return true if position is walkable
     */
    bool isWalkable(const glm::vec3& position) const;

    /**
     * @brief Get the nearest point on the navmesh
     * @param position World position
     * @return Nearest walkable position
     */
    glm::vec3 getNearestPoint(const glm::vec3& position) const;

    /**
     * @brief Get number of loaded navmeshes
     */
    size_t getNavMeshCount() const { return m_navMeshes.size(); }

    /**
     * @brief Clear all loaded navmesh data
     */
    void clear();

private:
    // Loaded NavMesh data
    std::vector<NavMeshData> m_navMeshes;

    // Spatial lookup: cellFormID -> index into m_navMeshes
    std::unordered_map<uint32_t, size_t> m_cellToNavMesh;

    /**
     * @brief Find which triangle contains a position
     * @param navMesh NavMesh to search
     * @param position World position
     * @return Triangle index, or -1 if not found
     */
    int findTriangle(const NavMeshData& navMesh, const glm::vec3& position) const;

    /**
     * @brief Get centroid of a triangle
     * @param navMesh NavMesh data
     * @param triIdx Triangle index
     * @return Centroid position
     */
    glm::vec3 getTriangleCentroid(const NavMeshData& navMesh, int triIdx) const;

    /**
     * @brief Calculate distance between two points (2D, ignoring Y)
     */
    float distance2D(const glm::vec3& a, const glm::vec3& b) const;

    /**
     * @brief A* pathfinding on a single NavMesh
     * @param navMesh NavMesh data
     * @param startTri Starting triangle index
     * @param endTri Ending triangle index
     * @param outPath Output waypoints
     * @return true if path found
     */
    bool astar(const NavMeshData& navMesh, int startTri, int endTri,
               std::vector<glm::vec3>& outPath) const;
};

} // namespace oblivion
