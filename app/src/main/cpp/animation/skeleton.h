#pragma once

#include "nif_types.h"
#include <vector>
#include <string>
#include <unordered_map>

// Unified bone structure (cache-friendly single struct)
struct Bone {
    std::string name;
    int parentIndex = -1;
    int firstChildIndex = -1;
    int childCount = 0;

    glm::mat4 localTransform;       // parent-relative
    glm::mat4 worldTransform;       // root-relative
    glm::mat4 inverseBindMatrix;    // (bind pose world)^-1
    glm::mat4 skinningMatrix;       // world × inverseBind
};

class Skeleton {
public:
    // Build from NIF data
    bool buildFromNIF(const std::vector<NIFNode>& nodes,
                      const NIFSkinInstance& skinInstance,
                      const NIFSkinData& skinData);

    // Bone access
    int getBoneIndex(const std::string& name) const;
    const Bone& getBone(int index) const;
    Bone& getBone(int index);
    const std::vector<Bone>& getBones() const { return bones; }
    int getBoneCount() const { return static_cast<int>(bones.size()); }
    bool isBuilt() const { return !bones.empty(); }

    // Pose setting
    void setBindPose();
    void setBoneLocalTransform(int index, const glm::mat4& localTransform);
    void setBoneLocalTransformByName(const std::string& name, const glm::mat4& transform);

    // Iterative update (topological order, no recursion)
    void update();

    // Skinning matrices output
    const std::vector<glm::mat4>& getSkinningMatrices() const { return skinningMatrices; }

private:
    std::vector<Bone> bones;
    std::vector<glm::mat4> skinningMatrices;
    std::unordered_map<std::string, int> nameToIndex;
    std::vector<int> rootIndices;

    // Topological update order (parent always before child)
    std::vector<int> updateOrder;

    void buildUpdateOrder();
    void rebuildChildInfo();
    glm::mat4 localToMatrix(const NIFTransform& t) const;
};
