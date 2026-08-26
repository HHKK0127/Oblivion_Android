#include "navmesh_manager.h"
#include <algorithm>
#include <limits>
#include <unordered_set>
#include <android/log.h>

namespace oblivion {

NavMeshManager::NavMeshManager() {
    LOGD("NavMeshManager created");
}

NavMeshManager::~NavMeshManager() {
    clear();
    LOGD("NavMeshManager destroyed");
}

void NavMeshManager::clear() {
    m_navMeshes.clear();
    m_cellToNavMesh.clear();
}

void NavMeshManager::loadFromESM(const ESMManager& esmMgr) {
    clear();

    const auto& navMeshes = esmMgr.getAllNavMeshes();
    m_navMeshes.reserve(navMeshes.size());

    for (const auto& nm : navMeshes) {
        m_navMeshes.push_back(nm);
        if (nm.cellFormID != 0) {
            m_cellToNavMesh[nm.cellFormID] = m_navMeshes.size() - 1;
        }
    }

    LOGI("Loaded %zu NavMeshes from ESM data", m_navMeshes.size());
    size_t totalVerts = 0, totalTris = 0;
    for (const auto& nm : m_navMeshes) {
        totalVerts += nm.vertices.size();
        totalTris += nm.triangles.size();
    }
    LOGI("  Total: %zu vertices, %zu triangles", totalVerts, totalTris);
}

float NavMeshManager::distance2D(const glm::vec3& a, const glm::vec3& b) const {
    float dx = a.x - b.x;
    float dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}

glm::vec3 NavMeshManager::getTriangleCentroid(const NavMeshData& navMesh, int triIdx) const {
    if (triIdx < 0 || triIdx >= static_cast<int>(navMesh.triangles.size())) {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }
    const auto& tri = navMesh.triangles[triIdx];
    glm::vec3 centroid(0.0f, 0.0f, 0.0f);
    int count = 0;
    for (int i = 0; i < 3; i++) {
        if (tri.vertex[i] < navMesh.vertices.size()) {
            centroid.x += navMesh.vertices[tri.vertex[i]].x;
            centroid.y += navMesh.vertices[tri.vertex[i]].y;
            centroid.z += navMesh.vertices[tri.vertex[i]].z;
            count++;
        }
    }
    if (count > 0) {
        centroid.x /= static_cast<float>(count);
        centroid.y /= static_cast<float>(count);
        centroid.z /= static_cast<float>(count);
    }
    return centroid;
}

int NavMeshManager::findTriangle(const NavMeshData& navMesh, const glm::vec3& position) const {
    // Find the triangle whose centroid is closest to the position
    int bestTri = -1;
    float bestDist = std::numeric_limits<float>::max();

    for (size_t i = 0; i < navMesh.triangles.size(); i++) {
        glm::vec3 centroid = getTriangleCentroid(navMesh, static_cast<int>(i));
        float dist = distance2D(centroid, position);
        if (dist < bestDist) {
            bestDist = dist;
            bestTri = static_cast<int>(i);
        }
    }

    return bestTri;
}

bool NavMeshManager::astar(const NavMeshData& navMesh, int startTri, int endTri,
                           std::vector<glm::vec3>& outPath) const {
    if (startTri < 0 || endTri < 0 ||
        startTri >= static_cast<int>(navMesh.triangles.size()) ||
        endTri >= static_cast<int>(navMesh.triangles.size())) {
        return false;
    }

    if (startTri == endTri) {
        outPath.push_back(getTriangleCentroid(navMesh, endTri));
        return true;
    }

    // A* data structures
    struct Node {
        int triIdx;
        float g;  // Cost from start
        float f;  // g + heuristic
        int parent;

        bool operator>(const Node& other) const { return f > other.f; }
    };

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
    std::unordered_map<int, float> gScore;
    std::unordered_map<int, int> cameFrom;
    std::unordered_set<int> closedSet;

    glm::vec3 endCentroid = getTriangleCentroid(navMesh, endTri);

    // Initialize with start node
    gScore[startTri] = 0.0f;
    float h = distance2D(getTriangleCentroid(navMesh, startTri), endCentroid);
    openSet.push({startTri, 0.0f, h, -1});

    while (!openSet.empty()) {
        Node current = openSet.top();
        openSet.pop();

        if (current.triIdx == endTri) {
            // Reconstruct path
            std::vector<int> path;
            int idx = endTri;
            while (idx != -1) {
                path.push_back(idx);
                auto it = cameFrom.find(idx);
                if (it != cameFrom.end()) {
                    idx = it->second;
                } else {
                    break;
                }
            }
            std::reverse(path.begin(), path.end());

            // Convert to waypoints
            outPath.clear();
            outPath.reserve(path.size());
            for (int triIdx : path) {
                outPath.push_back(getTriangleCentroid(navMesh, triIdx));
            }
            return true;
        }

        if (closedSet.count(current.triIdx)) continue;
        closedSet.insert(current.triIdx);

        // Explore neighbors via adjacent edges
        const auto& tri = navMesh.triangles[current.triIdx];
        for (int edge = 0; edge < 3; edge++) {
            int neighborTri = tri.adjacentEdge[edge];
            if (neighborTri < 0 || neighborTri >= static_cast<int>(navMesh.triangles.size())) {
                continue;
            }
            if (closedSet.count(neighborTri)) continue;

            glm::vec3 neighborCentroid = getTriangleCentroid(navMesh, neighborTri);
            float tentativeG = current.g + distance2D(
                getTriangleCentroid(navMesh, current.triIdx), neighborCentroid);

            auto it = gScore.find(neighborTri);
            if (it == gScore.end() || tentativeG < it->second) {
                gScore[neighborTri] = tentativeG;
                cameFrom[neighborTri] = current.triIdx;
                float f = tentativeG + distance2D(neighborCentroid, endCentroid);
                openSet.push({neighborTri, tentativeG, f, current.triIdx});
            }
        }
    }

    return false;  // No path found
}

bool NavMeshManager::findPath(const glm::vec3& start, const glm::vec3& end,
                              std::vector<glm::vec3>& outPath) const {
    outPath.clear();

    if (m_navMeshes.empty()) {
        LOGW("No NavMesh data loaded");
        return false;
    }

    // Find the best NavMesh (closest to start position)
    const NavMeshData* bestMesh = nullptr;
    float bestDist = std::numeric_limits<float>::max();

    for (const auto& nm : m_navMeshes) {
        if (nm.vertices.empty() || nm.triangles.empty()) continue;
        glm::vec3 center = getTriangleCentroid(nm, 0);
        float dist = distance2D(center, start);
        if (dist < bestDist) {
            bestDist = dist;
            bestMesh = &nm;
        }
    }

    if (!bestMesh) {
        LOGW("No suitable NavMesh found for pathfinding");
        return false;
    }

    int startTri = findTriangle(*bestMesh, start);
    int endTri = findTriangle(*bestMesh, end);

    if (startTri < 0 || endTri < 0) {
        LOGW("Could not find start/end triangle on NavMesh");
        return false;
    }

    bool found = astar(*bestMesh, startTri, endTri, outPath);
    if (found) {
        LOGD("Path found: %zu waypoints", outPath.size());
    } else {
        LOGW("No path found between triangles %d and %d", startTri, endTri);
    }

    return found;
}

bool NavMeshManager::isWalkable(const glm::vec3& position) const {
    for (const auto& nm : m_navMeshes) {
        if (nm.vertices.empty() || nm.triangles.empty()) continue;
        int tri = findTriangle(nm, position);
        if (tri >= 0) {
            glm::vec3 centroid = getTriangleCentroid(nm, tri);
            float dist = distance2D(centroid, position);
            if (dist < 5.0f) {  // Within reasonable distance
                return true;
            }
        }
    }
    return false;
}

glm::vec3 NavMeshManager::getNearestPoint(const glm::vec3& position) const {
    glm::vec3 nearest = position;
    float bestDist = std::numeric_limits<float>::max();

    for (const auto& nm : m_navMeshes) {
        if (nm.vertices.empty()) continue;
        for (const auto& vert : nm.vertices) {
            float dist = distance2D(vert, position);
            if (dist < bestDist) {
                bestDist = dist;
                nearest = vert;
            }
        }
    }

    return nearest;
}

} // namespace oblivion
