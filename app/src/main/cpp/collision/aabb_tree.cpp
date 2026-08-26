#include "aabb_tree.h"
#include <cstring>
#include <cmath>
#include <algorithm>

// ============================================
// Phase 30 Step 10: Dynamic AABB Tree
// ============================================

DynamicAABBTree::DynamicAABBTree() {
    root = NULL_NODE;
    freeList = NULL_NODE;
    nodeCount = 0;
    nodeCapacity = 16;
    nodes.resize(nodeCapacity);

    // Initialize free list
    for (int32_t i = 0; i < nodeCapacity - 1; i++) {
        nodes[i].parent = i + 1;
    }
    nodes[nodeCapacity - 1].parent = NULL_NODE;
    freeList = 0;
}

DynamicAABBTree::~DynamicAABBTree() {
}

int32_t DynamicAABBTree::allocateNode() {
    if (freeList == NULL_NODE) {
        // Expand capacity
        int32_t oldCapacity = nodeCapacity;
        nodeCapacity *= 2;
        nodes.resize(nodeCapacity);

        // Initialize new free list
        for (int32_t i = oldCapacity; i < nodeCapacity - 1; i++) {
            nodes[i].parent = i + 1;
        }
        nodes[nodeCapacity - 1].parent = NULL_NODE;
        freeList = oldCapacity;
    }

    int32_t nodeIndex = freeList;
    freeList = nodes[nodeIndex].parent;

    nodes[nodeIndex].parent = NULL_NODE;
    nodes[nodeIndex].left = NULL_NODE;
    nodes[nodeIndex].right = NULL_NODE;
    nodes[nodeIndex].height = 0;
    nodes[nodeIndex].userData = -1;

    nodeCount++;
    return nodeIndex;
}

void DynamicAABBTree::freeNode(int32_t nodeIndex) {
    nodes[nodeIndex].parent = freeList;
    nodes[nodeIndex].height = -1;
    freeList = nodeIndex;
    nodeCount--;
}

int32_t DynamicAABBTree::insert(const AABB& bounds, int32_t userData) {
    int32_t nodeIndex = allocateNode();

    // Fat AABB to avoid frequent updates
    nodes[nodeIndex].bounds = fatten(bounds, AABB_MARGIN);
    nodes[nodeIndex].userData = userData;

    insertLeaf(nodeIndex);

    return nodeIndex;
}

void DynamicAABBTree::remove(int32_t nodeIndex) {
    if (nodeIndex < 0 || nodeIndex >= nodeCapacity) return;
    if (nodes[nodeIndex].height == -1) return;  // Already freed

    removeLeaf(nodeIndex);
    freeNode(nodeIndex);
}

void DynamicAABBTree::update(int32_t nodeIndex, const AABB& newBounds) {
    if (nodeIndex < 0 || nodeIndex >= nodeCapacity) return;
    if (nodes[nodeIndex].height == -1) return;

    removeLeaf(nodeIndex);

    nodes[nodeIndex].bounds = fatten(newBounds, AABB_MARGIN);

    insertLeaf(nodeIndex);
}

void DynamicAABBTree::insertLeaf(int32_t leaf) {
    if (root == NULL_NODE) {
        root = leaf;
        nodes[root].parent = NULL_NODE;
        return;
    }

    // Find the best sibling
    int32_t sibling = findBestSibling(nodes[leaf].bounds);

    // Create a new parent
    int32_t oldParent = nodes[sibling].parent;
    int32_t newParent = allocateNode();
    nodes[newParent].parent = oldParent;
    nodes[newParent].bounds = AABB::merge(nodes[leaf].bounds, nodes[sibling].bounds);
    nodes[newParent].height = nodes[sibling].height + 1;

    if (oldParent != NULL_NODE) {
        // Sibling is not the root
        if (nodes[oldParent].left == sibling) {
            nodes[oldParent].left = newParent;
        } else {
            nodes[oldParent].right = newParent;
        }
    } else {
        // Sibling was the root
        root = newParent;
    }

    nodes[newParent].left = sibling;
    nodes[newParent].right = leaf;
    nodes[sibling].parent = newParent;
    nodes[leaf].parent = newParent;

    // Walk back up the tree refitting AABBs and balancing
    int32_t index = nodes[leaf].parent;
    while (index != NULL_NODE) {
        index = balance(index);

        int32_t left = nodes[index].left;
        int32_t right = nodes[index].right;

        nodes[index].height = 1 + std::max(nodes[left].height, nodes[right].height);
        nodes[index].bounds = AABB::merge(nodes[left].bounds, nodes[right].bounds);

        index = nodes[index].parent;
    }
}

void DynamicAABBTree::removeLeaf(int32_t leaf) {
    if (leaf == root) {
        root = NULL_NODE;
        return;
    }

    int32_t parent = nodes[leaf].parent;
    int32_t grandParent = nodes[parent].parent;
    int32_t sibling;

    if (nodes[parent].left == leaf) {
        sibling = nodes[parent].right;
    } else {
        sibling = nodes[parent].left;
    }

    if (grandParent != NULL_NODE) {
        // Destroy parent and connect sibling to grandParent
        if (nodes[grandParent].left == parent) {
            nodes[grandParent].left = sibling;
        } else {
            nodes[grandParent].right = sibling;
        }
        nodes[sibling].parent = grandParent;
        freeNode(parent);

        // Adjust ancestor bounds
        int32_t index = grandParent;
        while (index != NULL_NODE) {
            index = balance(index);

            int32_t left = nodes[index].left;
            int32_t right = nodes[index].right;

            nodes[index].height = 1 + std::max(nodes[left].height, nodes[right].height);
            nodes[index].bounds = AABB::merge(nodes[left].bounds, nodes[right].bounds);

            index = nodes[index].parent;
        }
    } else {
        root = sibling;
        nodes[sibling].parent = NULL_NODE;
        freeNode(parent);
    }
}

int32_t DynamicAABBTree::balance(int32_t nodeIndex) {
    if (nodes[nodeIndex].isLeaf() || nodes[nodeIndex].height < 2) {
        return nodeIndex;
    }

    int32_t left = nodes[nodeIndex].left;
    int32_t right = nodes[nodeIndex].right;

    int32_t balanceFactor = nodes[right].height - nodes[left].height;

    // Rotate right
    if (balanceFactor > 1) {
        int32_t rightLeft = nodes[right].left;
        int32_t rightRight = nodes[right].right;

        // Swap node and right
        nodes[right].left = nodeIndex;
        nodes[right].parent = nodes[nodeIndex].parent;
        nodes[nodeIndex].parent = right;

        if (nodes[right].parent != NULL_NODE) {
            if (nodes[nodes[right].parent].left == nodeIndex) {
                nodes[nodes[right].parent].left = right;
            } else {
                nodes[nodes[right].parent].right = right;
            }
        } else {
            root = right;
        }

        if (nodes[rightLeft].height > nodes[rightRight].height) {
            nodes[right].right = rightLeft;
            nodes[nodeIndex].right = rightRight;
            nodes[rightRight].parent = nodeIndex;
            nodes[nodeIndex].bounds = AABB::merge(nodes[left].bounds, nodes[rightRight].bounds);
            nodes[right].bounds = AABB::merge(nodes[nodeIndex].bounds, nodes[rightLeft].bounds);

            nodes[nodeIndex].height = 1 + std::max(nodes[left].height, nodes[rightRight].height);
            nodes[right].height = 1 + std::max(nodes[nodeIndex].height, nodes[rightLeft].height);
        } else {
            nodes[right].right = rightRight;
            nodes[nodeIndex].right = rightLeft;
            nodes[rightLeft].parent = nodeIndex;
            nodes[nodeIndex].bounds = AABB::merge(nodes[left].bounds, nodes[rightLeft].bounds);
            nodes[right].bounds = AABB::merge(nodes[nodeIndex].bounds, nodes[rightRight].bounds);

            nodes[nodeIndex].height = 1 + std::max(nodes[left].height, nodes[rightLeft].height);
            nodes[right].height = 1 + std::max(nodes[nodeIndex].height, nodes[rightRight].height);
        }

        return right;
    }

    // Rotate left
    if (balanceFactor < -1) {
        int32_t leftLeft = nodes[left].left;
        int32_t leftRight = nodes[left].right;

        // Swap node and left
        nodes[left].left = nodeIndex;
        nodes[left].parent = nodes[nodeIndex].parent;
        nodes[nodeIndex].parent = left;

        if (nodes[left].parent != NULL_NODE) {
            if (nodes[nodes[left].parent].left == nodeIndex) {
                nodes[nodes[left].parent].left = left;
            } else {
                nodes[nodes[left].parent].right = left;
            }
        } else {
            root = left;
        }

        if (nodes[leftLeft].height > nodes[leftRight].height) {
            nodes[left].right = leftLeft;
            nodes[nodeIndex].left = leftRight;
            nodes[leftRight].parent = nodeIndex;
            nodes[nodeIndex].bounds = AABB::merge(nodes[right].bounds, nodes[leftRight].bounds);
            nodes[left].bounds = AABB::merge(nodes[nodeIndex].bounds, nodes[leftLeft].bounds);

            nodes[nodeIndex].height = 1 + std::max(nodes[right].height, nodes[leftRight].height);
            nodes[left].height = 1 + std::max(nodes[nodeIndex].height, nodes[leftLeft].height);
        } else {
            nodes[left].right = leftRight;
            nodes[nodeIndex].left = leftLeft;
            nodes[leftLeft].parent = nodeIndex;
            nodes[nodeIndex].bounds = AABB::merge(nodes[right].bounds, nodes[leftLeft].bounds);
            nodes[left].bounds = AABB::merge(nodes[nodeIndex].bounds, nodes[leftRight].bounds);

            nodes[nodeIndex].height = 1 + std::max(nodes[right].height, nodes[leftLeft].height);
            nodes[left].height = 1 + std::max(nodes[nodeIndex].height, nodes[leftRight].height);
        }

        return left;
    }

    return nodeIndex;
}

int32_t DynamicAABBTree::computeHeight(int32_t nodeIndex) const {
    if (nodeIndex == NULL_NODE) return 0;
    if (nodes[nodeIndex].isLeaf()) return 0;

    int32_t leftHeight = computeHeight(nodes[nodeIndex].left);
    int32_t rightHeight = computeHeight(nodes[nodeIndex].right);
    return 1 + std::max(leftHeight, rightHeight);
}

int32_t DynamicAABBTree::findBestSibling(const AABB& bounds) const {
    int32_t bestSibling = root;
    float bestCost = mergeCost(nodes[root].bounds, bounds);

    // Iterative traversal with cost-based selection
    std::vector<int32_t> stack;
    stack.push_back(root);

    while (!stack.empty()) {
        int32_t current = stack.back();
        stack.pop_back();

        if (nodes[current].isLeaf()) {
            float cost = mergeCost(nodes[current].bounds, bounds);
            if (cost < bestCost) {
                bestCost = cost;
                bestSibling = current;
            }
            continue;
        }

        // Check children
        int32_t left = nodes[current].left;
        int32_t right = nodes[current].right;

        float leftCost = mergeCost(nodes[left].bounds, bounds);
        float rightCost = mergeCost(nodes[right].bounds, bounds);

        // Prune: only explore if potentially better
        if (leftCost < bestCost) {
            bestCost = leftCost;
            bestSibling = left;
            if (!nodes[left].isLeaf()) {
                stack.push_back(left);
            }
        }

        if (rightCost < bestCost) {
            bestCost = rightCost;
            bestSibling = right;
            if (!nodes[right].isLeaf()) {
                stack.push_back(right);
            }
        }
    }

    return bestSibling;
}

float DynamicAABBTree::mergeCost(const AABB& a, const AABB& b) {
    return AABB::merge(a, b).getSurfaceArea();
}

AABB DynamicAABBTree::fatten(const AABB& bounds, float margin) {
    return AABB(
        bounds.min - glm::vec3(margin, margin, margin),
        bounds.max + glm::vec3(margin, margin, margin)
    );
}

void DynamicAABBTree::query(const AABB& bounds, std::vector<int32_t>& results) const {
    if (root == NULL_NODE) return;

    std::vector<int32_t> stack;
    stack.push_back(root);

    while (!stack.empty()) {
        int32_t nodeIndex = stack.back();
        stack.pop_back();

        if (nodeIndex == NULL_NODE) continue;

        const AABBNode& node = nodes[nodeIndex];

        if (node.bounds.intersects(bounds)) {
            if (node.isLeaf()) {
                results.push_back(node.userData);
            } else {
                stack.push_back(node.left);
                stack.push_back(node.right);
            }
        }
    }
}

void DynamicAABBTree::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
                               std::vector<int32_t>& results) const {
    if (root == NULL_NODE) return;

    // Simple AABB-ray intersection test
    std::vector<int32_t> stack;
    stack.push_back(root);

    while (!stack.empty()) {
        int32_t nodeIndex = stack.back();
        stack.pop_back();

        if (nodeIndex == NULL_NODE) continue;

        const AABBNode& node = nodes[nodeIndex];

        // AABB-ray intersection
        float tmin = 0.0f;
        float tmax = maxDistance;

        for (int axis = 0; axis < 3; axis++) {
            float invD = 1.0f / ((&direction.x)[axis] + 1e-10f);
            float t0 = ((&node.bounds.min.x)[axis] - (&origin.x)[axis]) * invD;
            float t1 = ((&node.bounds.max.x)[axis] - (&origin.x)[axis]) * invD;

            if (invD < 0.0f) std::swap(t0, t1);

            tmin = std::max(tmin, t0);
            tmax = std::min(tmax, t1);

            if (tmin > tmax) break;
        }

        if (tmin <= tmax) {
            if (node.isLeaf()) {
                results.push_back(node.userData);
            } else {
                stack.push_back(node.left);
                stack.push_back(node.right);
            }
        }
    }
}

AABB DynamicAABBTree::getNodeBounds(int32_t nodeIndex) const {
    if (nodeIndex < 0 || nodeIndex >= nodeCapacity) return AABB();
    return nodes[nodeIndex].bounds;
}

int32_t DynamicAABBTree::getNodeUserData(int32_t nodeIndex) const {
    if (nodeIndex < 0 || nodeIndex >= nodeCapacity) return -1;
    return nodes[nodeIndex].userData;
}

bool DynamicAABBTree::validate() const {
    if (root == NULL_NODE) return nodeCount == 0;

    // Check that all nodes are reachable
    int32_t reachable = 0;
    std::vector<int32_t> stack;
    stack.push_back(root);

    while (!stack.empty()) {
        int32_t nodeIndex = stack.back();
        stack.pop_back();

        if (nodeIndex == NULL_NODE) continue;

        reachable++;

        if (!nodes[nodeIndex].isLeaf()) {
            stack.push_back(nodes[nodeIndex].left);
            stack.push_back(nodes[nodeIndex].right);
        }
    }

    return reachable == nodeCount;
}
