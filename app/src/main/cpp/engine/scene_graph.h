#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG_SG "SceneGraph"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_SG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_SG, __VA_ARGS__)
#else
#define LOGD_SG(...) do {} while(0)
#endif
#define LOGI_SG(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_SG, __VA_ARGS__)

// ============================================================================
// Scene Graph
// Phase 56: Hierarchical transform system for Gamebryo
// Parent-child relationships, dirty flag propagation, spatial queries
// ============================================================================

namespace engine {

// Minimal math types for scene graph
struct SGVec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    SGVec3() = default;
    SGVec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    SGVec3 operator+(const SGVec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    SGVec3 operator-(const SGVec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    SGVec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    SGVec3 normalized() const {
        float l = length();
        return l > 0.0001f ? SGVec3(x / l, y / l, z / l) : SGVec3(0, 0, 0);
    }
};

struct SGQuat {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
    SGQuat() = default;
    SGQuat(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    static SGQuat fromAxisAngle(const SGVec3& axis, float angleRad) {
        float half = angleRad * 0.5f;
        float s = std::sin(half);
        SGVec3 n = axis.normalized();
        return SGQuat(n.x * s, n.y * s, n.z * s, std::cos(half));
    }

    SGQuat operator*(const SGQuat& q) const {
        return SGQuat(
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w,
            w * q.w - x * q.x - y * q.y - z * q.z
        );
    }

    SGVec3 rotate(const SGVec3& v) const {
        SGVec3 qv(x, y, z);
        SGVec3 uv = cross(qv, v);
        SGVec3 uuv = cross(qv, uv);
        return v + (uv * w + uuv) * 2.0f;
    }

private:
    static SGVec3 cross(const SGVec3& a, const SGVec3& b) {
        return SGVec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }
};

// 4x4 matrix for transforms
struct SGMat4 {
    float m[16];

    SGMat4() {
        // Identity
        for (int i = 0; i < 16; i++) m[i] = 0.0f;
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static SGMat4 translation(const SGVec3& t) {
        SGMat4 mat;
        mat.m[12] = t.x; mat.m[13] = t.y; mat.m[14] = t.z;
        return mat;
    }

    static SGMat4 rotation(const SGQuat& q) {
        SGMat4 mat;
        float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
        float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
        float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;

        mat.m[0] = 1.0f - 2.0f * (yy + zz);
        mat.m[1] = 2.0f * (xy + wz);
        mat.m[2] = 2.0f * (xz - wy);
        mat.m[4] = 2.0f * (xy - wz);
        mat.m[5] = 1.0f - 2.0f * (xx + zz);
        mat.m[6] = 2.0f * (yz + wx);
        mat.m[8] = 2.0f * (xz + wy);
        mat.m[9] = 2.0f * (yz - wx);
        mat.m[10] = 1.0f - 2.0f * (xx + yy);
        return mat;
    }

    static SGMat4 scaling(const SGVec3& s) {
        SGMat4 mat;
        mat.m[0] = s.x; mat.m[5] = s.y; mat.m[10] = s.z;
        return mat;
    }

    SGMat4 operator*(const SGMat4& o) const {
        SGMat4 result;
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                result.m[col * 4 + row] =
                    m[0 * 4 + row] * o.m[col * 4 + 0] +
                    m[1 * 4 + row] * o.m[col * 4 + 1] +
                    m[2 * 4 + row] * o.m[col * 4 + 2] +
                    m[3 * 4 + row] * o.m[col * 4 + 3];
            }
        }
        return result;
    }

    SGVec3 transformPoint(const SGVec3& p) const {
        return SGVec3(
            m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12],
            m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
            m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14]
        );
    }
};

// Bounding box for spatial queries
struct AABB {
    SGVec3 min;
    SGVec3 max;

    AABB() : min(0, 0, 0), max(0, 0, 0) {}
    AABB(const SGVec3& min_, const SGVec3& max_) : min(min_), max(max_) {}

    void expand(const SGVec3& point) {
        min.x = std::min(min.x, point.x);
        min.y = std::min(min.y, point.y);
        min.z = std::min(min.z, point.z);
        max.x = std::max(max.x, point.x);
        max.y = std::max(max.y, point.y);
        max.z = std::max(max.z, point.z);
    }

    void merge(const AABB& other) {
        expand(other.min);
        expand(other.max);
    }

    SGVec3 center() const {
        return SGVec3((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f);
    }

    SGVec3 extent() const {
        return SGVec3(max.x - min.x, max.y - min.y, max.z - min.z);
    }

    bool intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }

    bool containsPoint(const SGVec3& p) const {
        return p.x >= min.x && p.x <= max.x &&
               p.y >= min.y && p.y <= max.y &&
               p.z >= min.z && p.z <= max.z;
    }
};

// ============================================================================
// SceneNode - single node in the scene graph
// ============================================================================

class SceneNode {
public:
    explicit SceneNode(const std::string& name = "")
        : name_(name) {}

    ~SceneNode() {
        children_.clear();
    }

    // --- Transform ---

    void setPosition(const SGVec3& pos) {
        localPosition_ = pos;
        markDirty();
    }

    void setRotation(const SGQuat& rot) {
        localRotation_ = rot;
        markDirty();
    }

    void setScale(const SGVec3& scale) {
        localScale_ = scale;
        markDirty();
    }

    const SGVec3& getLocalPosition() const { return localPosition_; }
    const SGQuat& getLocalRotation() const { return localRotation_; }
    const SGVec3& getLocalScale() const { return localScale_; }

    SGVec3 getWorldPosition() {
        updateWorldTransform();
        return worldPosition_;
    }

    const SGMat4& getWorldMatrix() {
        updateWorldTransform();
        return worldMatrix_;
    }

    // --- Hierarchy ---

    void addChild(std::shared_ptr<SceneNode> child) {
        if (child->parent_) {
            child->parent_->removeChild(child);
        }
        child->parent_ = this;
        child->markDirty();
        children_.push_back(child);
    }

    void removeChild(std::shared_ptr<SceneNode> child) {
        auto it = std::find(children_.begin(), children_.end(), child);
        if (it != children_.end()) {
            (*it)->parent_ = nullptr;
            children_.erase(it);
        }
    }

    SceneNode* getParent() const { return parent_; }
    const std::vector<std::shared_ptr<SceneNode>>& getChildren() const { return children_; }

    // --- Metadata ---

    void setName(const std::string& name) { name_ = name; }
    const std::string& getName() const { return name_; }

    void setTag(const std::string& tag) { tag_ = tag; }
    const std::string& getTag() const { return tag_; }

    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }

    void setUserData(void* data) { userData_ = data; }
    void* getUserData() const { return userData_; }

    // --- Bounding volume ---

    void setLocalBounds(const AABB& bounds) {
        localBounds_ = bounds;
        boundsDirty_ = true;
    }

    const AABB& getLocalBounds() const { return localBounds_; }

    AABB getWorldBounds() {
        updateWorldTransform();
        if (boundsDirty_) {
            // Transform local bounds to world space
            SGVec3 corners[8] = {
                {localBounds_.min.x, localBounds_.min.y, localBounds_.min.z},
                {localBounds_.max.x, localBounds_.min.y, localBounds_.min.z},
                {localBounds_.min.x, localBounds_.max.y, localBounds_.min.z},
                {localBounds_.max.x, localBounds_.max.y, localBounds_.min.z},
                {localBounds_.min.x, localBounds_.min.y, localBounds_.max.z},
                {localBounds_.max.x, localBounds_.min.y, localBounds_.max.z},
                {localBounds_.min.x, localBounds_.max.y, localBounds_.max.z},
                {localBounds_.max.x, localBounds_.max.y, localBounds_.max.z}
            };
            worldBounds_ = AABB();
            for (int i = 0; i < 8; i++) {
                worldBounds_.expand(worldMatrix_.transformPoint(corners[i]));
            }
            boundsDirty_ = false;
        }
        return worldBounds_;
    }

    // --- Dirty flag ---

    bool isDirty() const { return dirty_; }

private:
    std::string name_;
    std::string tag_;
    bool visible_ = true;
    void* userData_ = nullptr;

    // Local transform
    SGVec3 localPosition_;
    SGQuat localRotation_;
    SGVec3 localScale_ = SGVec3(1.0f, 1.0f, 1.0f);

    // World transform (cached)
    SGMat4 worldMatrix_;
    SGVec3 worldPosition_;
    bool dirty_ = true;

    // Bounding volume
    AABB localBounds_;
    AABB worldBounds_;
    bool boundsDirty_ = true;

    // Hierarchy
    SceneNode* parent_ = nullptr;
    std::vector<std::shared_ptr<SceneNode>> children_;

    void markDirty() {
        dirty_ = true;
        boundsDirty_ = true;
        for (auto& child : children_) {
            child->markDirty();
        }
    }

    void updateWorldTransform() {
        if (!dirty_) return;

        SGMat4 T = SGMat4::translation(localPosition_);
        SGMat4 R = SGMat4::rotation(localRotation_);
        SGMat4 S = SGMat4::scaling(localScale_);
        SGMat4 local = T * R * S;

        if (parent_) {
            parent_->updateWorldTransform();
            worldMatrix_ = parent_->worldMatrix_ * local;
        } else {
            worldMatrix_ = local;
        }

        worldPosition_ = SGVec3(worldMatrix_.m[12], worldMatrix_.m[13], worldMatrix_.m[14]);
        dirty_ = false;
    }
};

// ============================================================================
// SceneGraph - root manager for the scene hierarchy
// ============================================================================

class SceneGraph {
public:
    static SceneGraph& instance() {
        static SceneGraph inst;
        return inst;
    }

    void init() {
        std::lock_guard<std::mutex> lock(mutex_);
        rootNode_ = std::make_shared<SceneNode>("root");
        nodeMap_["root"] = rootNode_;
        initialized_ = true;
        LOGI_SG("SceneGraph initialized");
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        rootNode_.reset();
        nodeMap_.clear();
        initialized_ = false;
    }

    // Create a named node
    std::shared_ptr<SceneNode> createNode(const std::string& name,
                                           const std::string& parentName = "root") {
        std::lock_guard<std::mutex> lock(mutex_);
        auto node = std::make_shared<SceneNode>(name);
        nodeMap_[name] = node;

        auto parentIt = nodeMap_.find(parentName);
        if (parentIt != nodeMap_.end()) {
            parentIt->second->addChild(node);
        } else {
            rootNode_->addChild(node);
        }

        return node;
    }

    // Find node by name
    std::shared_ptr<SceneNode> findNode(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(name);
        if (it != nodeMap_.end()) {
            return it->second;
        }
        return nullptr;
    }

    // Remove node
    void removeNode(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodeMap_.find(name);
        if (it != nodeMap_.end()) {
            if (it->second->getParent()) {
                it->second->getParent()->removeChild(it->second);
            }
            nodeMap_.erase(it);
        }
    }

    // Get root
    std::shared_ptr<SceneNode> getRoot() {
        std::lock_guard<std::mutex> lock(mutex_);
        return rootNode_;
    }

    // Spatial query: find all nodes whose bounds intersect the query AABB
    std::vector<std::shared_ptr<SceneNode>> queryAABB(const AABB& bounds) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::shared_ptr<SceneNode>> results;
        queryAABBRecursive(rootNode_, bounds, results);
        return results;
    }

    // Get node count
    size_t getNodeCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return nodeMap_.size();
    }

private:
    SceneGraph() = default;

    bool initialized_ = false;
    std::shared_ptr<SceneNode> rootNode_;
    std::unordered_map<std::string, std::shared_ptr<SceneNode>> nodeMap_;
    mutable std::mutex mutex_;

    void queryAABBRecursive(std::shared_ptr<SceneNode> node,
                            const AABB& bounds,
                            std::vector<std::shared_ptr<SceneNode>>& results) {
        if (!node || !node->isVisible()) return;

        AABB worldBounds = node->getWorldBounds();
        if (worldBounds.intersects(bounds)) {
            results.push_back(node);
        }

        for (auto& child : node->getChildren()) {
            queryAABBRecursive(child, bounds, results);
        }
    }
};

} // namespace engine
