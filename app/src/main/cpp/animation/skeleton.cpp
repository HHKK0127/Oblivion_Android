#include "skeleton.h"
#include <queue>
#include <algorithm>
#include <cstring>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#define LOG_TAG "Skeleton"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

glm::mat4 Skeleton::localToMatrix(const NIFTransform& t) const {
    glm::mat4 m;
    // Initialize to identity first
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            m[i][j] = (i == j) ? 1.0f : 0.0f;
    // Rotation (3x3 upper-left, scaled)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            m[i][j] = t.rotation.m[i][j] * t.scale;
        }
    }
    // Translation
    m[3][0] = t.translation.x;
    m[3][1] = t.translation.y;
    m[3][2] = t.translation.z;
    return m;
}

bool Skeleton::buildFromNIF(const std::vector<NIFNode>& nodes,
                            const NIFSkinInstance& skinInstance,
                            const NIFSkinData& skinData) {
    uint32_t numBones = skinData.numBones > 0 ? skinData.numBones : static_cast<uint32_t>(skinData.boneData.size());
    if (numBones == 0) {
        LOGD("No bones in skin data");
        return false;
    }

    bones.resize(numBones);
    skinningMatrices.resize(numBones);
    nameToIndex.clear();
    rootIndices.clear();

    // Build bones from skin data
    for (uint32_t i = 0; i < numBones; ++i) {
        Bone& bone = bones[i];
        bone.localTransform = localToMatrix(skinData.boneData[i].skinTransform);
        bone.worldTransform = glm::mat4();
        bone.inverseBindMatrix = glm::inverse(bone.localTransform);
        bone.skinningMatrix = glm::mat4();
        bone.parentIndex = -1;
        bone.firstChildIndex = -1;
        bone.childCount = 0;

        // Try to find matching NiNode by index
        if (i < nodes.size()) {
            bone.name = nodes[i].name;
            nameToIndex[bone.name] = static_cast<int>(i);

            // Resolve parent from NiNode hierarchy
            if (nodes[i].parentIndex >= 0 && nodes[i].parentIndex < static_cast<int32_t>(numBones)) {
                bone.parentIndex = nodes[i].parentIndex;
            }
        } else {
            bone.name = "bone_" + std::to_string(i);
            nameToIndex[bone.name] = static_cast<int>(i);
        }
    }

    // Find root bones
    for (int i = 0; i < static_cast<int>(numBones); ++i) {
        if (bones[i].parentIndex < 0) {
            rootIndices.push_back(i);
        }
    }

    rebuildChildInfo();
    buildUpdateOrder();
    setBindPose();

    LOGD("Skeleton built: %u bones, %zu roots, %zu update order",
         numBones, rootIndices.size(), updateOrder.size());
    return true;
}

void Skeleton::rebuildChildInfo() {
    // Reset child info
    for (auto& b : bones) {
        b.firstChildIndex = -1;
        b.childCount = 0;
    }
    // Count children
    for (size_t i = 0; i < bones.size(); ++i) {
        if (bones[i].parentIndex >= 0) {
            bones[bones[i].parentIndex].childCount++;
        }
    }
    // Set first child index (sequential layout)
    int offset = 0;
    for (auto& b : bones) {
        if (b.childCount > 0) {
            b.firstChildIndex = offset;
        }
        offset += b.childCount;
    }
}

void Skeleton::buildUpdateOrder() {
    // BFS from roots to get topological order (parent before child)
    updateOrder.clear();
    updateOrder.reserve(bones.size());

    std::queue<int> queue;
    for (int root : rootIndices) {
        queue.push(root);
    }

    while (!queue.empty()) {
        int idx = queue.front();
        queue.pop();
        updateOrder.push_back(idx);

        // Enqueue children
        for (size_t i = 0; i < bones.size(); ++i) {
            if (bones[i].parentIndex == idx) {
                queue.push(static_cast<int>(i));
            }
        }
    }

    // Safety: add any bones not reached (orphaned)
    if (updateOrder.size() < bones.size()) {
        std::vector<bool> visited(bones.size(), false);
        for (int idx : updateOrder) visited[idx] = true;
        for (size_t i = 0; i < bones.size(); ++i) {
            if (!visited[i]) {
                updateOrder.push_back(static_cast<int>(i));
            }
        }
    }
}

void Skeleton::setBindPose() {
    // Compute bind pose world transforms
    for (int idx : updateOrder) {
        Bone& bone = bones[idx];
        if (bone.parentIndex >= 0) {
            bone.worldTransform = bones[bone.parentIndex].worldTransform * bone.localTransform;
        } else {
            bone.worldTransform = bone.localTransform;
        }
        // Store inverse bind matrix
        bone.inverseBindMatrix = glm::inverse(bone.worldTransform);
        bone.skinningMatrix = glm::mat4(); // identity at bind pose
    }
}

void Skeleton::setBoneLocalTransform(int index, const glm::mat4& localTransform) {
    if (index >= 0 && index < static_cast<int>(bones.size())) {
        bones[index].localTransform = localTransform;
    }
}

void Skeleton::setBoneLocalTransformByName(const std::string& name, const glm::mat4& transform) {
    auto it = nameToIndex.find(name);
    if (it != nameToIndex.end()) {
        bones[it->second].localTransform = transform;
    }
}

void Skeleton::update() {
    // Iterative topological update (no recursion)
    for (int idx : updateOrder) {
        Bone& bone = bones[idx];
        if (bone.parentIndex >= 0) {
            bone.worldTransform = bones[bone.parentIndex].worldTransform * bone.localTransform;
        } else {
            bone.worldTransform = bone.localTransform;
        }
        bone.skinningMatrix = bone.worldTransform * bone.inverseBindMatrix;
        skinningMatrices[idx] = bone.skinningMatrix;
    }
}

int Skeleton::getBoneIndex(const std::string& name) const {
    auto it = nameToIndex.find(name);
    return (it != nameToIndex.end()) ? it->second : -1;
}

const Bone& Skeleton::getBone(int index) const {
    static Bone emptyBone{};
    if (index < 0 || index >= static_cast<int>(bones.size())) {
        LOGD("getBone: index %d out of range (0-%zu)", index, bones.size() - 1);
        return emptyBone;
    }
    return bones[index];
}

Bone& Skeleton::getBone(int index) {
    static Bone emptyBone{};
    if (index < 0 || index >= static_cast<int>(bones.size())) {
        LOGD("getBone: index %d out of range (0-%zu)", index, bones.size() - 1);
        return emptyBone;
    }
    return bones[index];
}
