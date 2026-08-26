#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

// ============================================
// Phase 30 Step 10: Dynamic AABB Tree
// Broad-phase collision detection structure
// ============================================

struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    AABB() : min(0.0f, 0.0f, 0.0f), max(0.0f, 0.0f, 0.0f) {}
    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

    glm::vec3 getCenter() const { return (min + max) * 0.5f; }
    glm::vec3 getExtents() const { return (max - min) * 0.5f; }
    float getSurfaceArea() const {
        glm::vec3 d = max - min;
        return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
    }

    bool contains(const AABB& other) const {
        return min.x <= other.min.x && min.y <= other.min.y && min.z <= other.min.z &&
               max.x >= other.max.x && max.y >= other.max.y && max.z >= other.max.z;
    }

    bool intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }

    static AABB merge(const AABB& a, const AABB& b) {
        return AABB(
            glm::vec3(fminf(a.min.x, b.min.x), fminf(a.min.y, b.min.y), fminf(a.min.z, b.min.z)),
            glm::vec3(fmaxf(a.max.x, b.max.x), fmaxf(a.max.y, b.max.y), fmaxf(a.max.z, b.max.z))
        );
    }
};

struct AABBNode {
    AABB bounds;
    int32_t userData = -1;  // Index into external data array
    int32_t parent = -1;
    int32_t left = -1;
    int32_t right = -1;
    int32_t height = 0;    // For balancing

    bool isLeaf() const { return left == -1; }
};

class DynamicAABBTree {
public:
    DynamicAABBTree();
    ~DynamicAABBTree();

    // Insert a new AABB with associated user data
    // Returns the node index
    int32_t insert(const AABB& bounds, int32_t userData);

    // Remove a node by index
    void remove(int32_t nodeIndex);

    // Update a node's AABB (re-insert if it no longer fits)
    void update(int32_t nodeIndex, const AABB& newBounds);

    // Query for all AABBs that overlap with the given AABB
    void query(const AABB& bounds, std::vector<int32_t>& results) const;

    // Query for all AABBs that overlap with a ray
    void raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                 std::vector<int32_t>& results) const;

    // Get the AABB of a node
    AABB getNodeBounds(int32_t nodeIndex) const;

    // Get the user data of a node
    int32_t getNodeUserData(int32_t nodeIndex) const;

    // Get the number of nodes
    int32_t getNodeCount() const { return nodeCount; }

    // Validate tree integrity (debug)
    bool validate() const;

private:
    static constexpr int32_t NULL_NODE = -1;
    static constexpr float AABB_MARGIN = 0.1f;  // Fat AABB margin

    std::vector<AABBNode> nodes;
    int32_t root = NULL_NODE;
    int32_t freeList = NULL_NODE;
    int32_t nodeCount = 0;
    int32_t nodeCapacity = 0;

    // Allocate a new node
    int32_t allocateNode();

    // Free a node
    void freeNode(int32_t nodeIndex);

    // Insert a leaf node into the tree
    void insertLeaf(int32_t nodeIndex);

    // Remove a leaf node from the tree
    void removeLeaf(int32_t nodeIndex);

    // Balance the tree
    int32_t balance(int32_t nodeIndex);

    // Compute the height of a node
    int32_t computeHeight(int32_t nodeIndex) const;

    // Find the best sibling for a new leaf
    int32_t findBestSibling(const AABB& bounds) const;

    // Compute the cost of merging two AABBs
    static float mergeCost(const AABB& a, const AABB& b);

    // Fat AABB: expand bounds by margin
    static AABB fatten(const AABB& bounds, float margin);
};
