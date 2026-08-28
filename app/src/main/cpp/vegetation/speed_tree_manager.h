#pragma once

// ============================================================================
// SpeedTreeManager - Billboard-based vegetation rendering system
// Phase 51: SpeedTree alternative using billboard + wind shader approach
// ============================================================================

#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>
#include <memory>
#include <glm/glm.hpp>
#include <GLES3/gl3.h>

class Renderer;
class ShaderProgram;

namespace vegetation {

// LOD levels for tree rendering
enum class TreeLodLevel : int {
    NEAR = 0,       // Full mesh + detailed shader (~500 polys)
    MID = 1,        // Simplified mesh (~100 polys)
    FAR = 2,        // Minimal mesh (~20 polys)
    BILLBOARD = 3   // 4-vertex cross billboard + texture
};

// Wind parameters for tree animation
struct WindParams {
    glm::vec3 direction = glm::vec3(1.0f, 0.0f, 0.0f);
    float strength = 0.3f;
    float swayPeriod = 2.0f;
    float vertexAmplitude = 0.15f;
    float gustFrequency = 0.5f;
    float gustStrength = 0.2f;
};

// Tree type definition (shared across instances of same species)
struct TreeType {
    uint32_t typeId = 0;
    std::string meshPath;
    std::string texturePath;
    std::string billboardTexturePath;
    float minHeight = 3.0f;
    float maxHeight = 12.0f;
    glm::vec2 leafUvMin = glm::vec2(0.0f, 0.0f);
    glm::vec2 leafUvMax = glm::vec2(1.0f, 1.0f);
    glm::vec2 trunkUvMin = glm::vec2(0.0f, 0.0f);
    glm::vec2 trunkUvMax = glm::vec2(0.5f, 1.0f);
    float billboardWidth = 4.0f;
    float billboardHeight = 8.0f;
    GLuint textureId = 0;
    GLuint billboardTextureId = 0;
};

// Single tree instance in the world
struct TreeInstance {
    uint32_t instanceId = 0;
    uint32_t typeId = 0;
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    float rotation = 0.0f;
    float scale = 1.0f;
    TreeLodLevel currentLod = TreeLodLevel::BILLBOARD;
    float lodBlend = 0.0f;  // 0.0 = current LOD, 1.0 = next LOD (for fade)
    bool visible = true;
};

// Per-type instance buffer for instanced rendering
struct InstanceBuffer {
    GLuint vao = 0;
    GLuint instanceVbo = 0;
    GLuint meshVbo = 0;
    GLuint ebo = 0;
    size_t instanceCount = 0;
    size_t maxInstances = 0;
    bool dirty = true;
};

// ============================================================================
// SpeedTreeManager - main vegetation manager (singleton)
// ============================================================================

class SpeedTreeManager {
public:
    static SpeedTreeManager& instance();

    // Initialization
    bool initialize(Renderer* renderer);
    void shutdown();

    // Tree type registration
    bool registerTreeType(uint32_t typeId, const TreeType& type);
    const TreeType* getTreeType(uint32_t typeId) const;

    // Instance management
    uint32_t spawnTree(uint32_t typeId, const glm::vec3& pos, float rotation, float scale);
    bool removeTree(uint32_t instanceId);
    TreeInstance* getTreeInstance(uint32_t instanceId);

    // Bulk spawning for world areas
    size_t spawnForest(const std::string& worldspaceId,
                       const std::vector<TreeInstance>& trees);
    void unloadForest(const std::string& worldspaceId);

    // LOD selection based on camera distance
    TreeLodLevel selectLod(const glm::vec3& treePos,
                           const glm::vec3& cameraPos) const;

    // Per-frame update (wind animation, LOD transitions)
    void update(float deltaTime, const glm::vec3& windDir);

    // Render all visible trees
    void render(Renderer* renderer,
                const glm::mat4& viewProj,
                const glm::vec3& cameraPos);

    // Statistics
    size_t getTreeCount() const { return instances_.size(); }
    size_t getTypeCount() const { return treeTypes_.size(); }
    size_t getVisibleCount() const { return visibleCount_; }

    // Configuration
    void setMaxDrawDistance(float dist) { maxDrawDistance_ = dist; }
    float getMaxDrawDistance() const { return maxDrawDistance_; }
    void setLodDistances(float nearDist, float midDist, float farDist);
    void setWindParams(const WindParams& params) { windParams_ = params; }
    const WindParams& getWindParams() const { return windParams_; }

private:
    SpeedTreeManager();
    ~SpeedTreeManager();
    SpeedTreeManager(const SpeedTreeManager&) = delete;
    SpeedTreeManager& operator=(const SpeedTreeManager&) = delete;

    // Internal rendering helpers
    void renderNearLod(const std::vector<TreeInstance*>& trees,
                       const glm::mat4& viewProj,
                       const glm::vec3& cameraPos);
    void renderMidLod(const std::vector<TreeInstance*>& trees,
                      const glm::mat4& viewProj,
                      const glm::vec3& cameraPos);
    void renderFarLod(const std::vector<TreeInstance*>& trees,
                      const glm::mat4& viewProj,
                      const glm::vec3& cameraPos);
    void renderBillboards(const std::vector<TreeInstance*>& trees,
                          const glm::mat4& viewProj,
                          const glm::vec3& cameraPos);

    // Frustum culling
    bool isInFrustum(const glm::vec3& pos, float radius,
                     const glm::mat4& viewProj) const;

    // Billboard mesh generation
    void generateBillboardMesh();
    void generateNearMesh();
    void generateMidMesh();
    void generateFarMesh();

    // Shader initialization
    bool initShaders();

    // State
    bool initialized_ = false;
    Renderer* renderer_ = nullptr;

    // Tree types and instances
    std::unordered_map<uint32_t, TreeType> treeTypes_;
    std::vector<TreeInstance> instances_;
    std::unordered_map<std::string, std::vector<uint32_t>> forestRegions_;
    uint32_t nextInstanceId_ = 1;

    // Instance buffers per type
    std::unordered_map<uint32_t, InstanceBuffer> instanceBuffers_;

    // LOD meshes
    GLuint billboardVao_ = 0;
    GLuint billboardVbo_ = 0;
    GLuint nearVao_ = 0;
    GLuint nearVbo_ = 0;
    GLuint nearEbo_ = 0;
    GLuint midVao_ = 0;
    GLuint midVbo_ = 0;
    GLuint midEbo_ = 0;
    GLuint farVao_ = 0;
    GLuint farVbo_ = 0;
    GLuint farEbo_ = 0;

    // Shaders
    std::unique_ptr<ShaderProgram> billboardShader_;
    std::unique_ptr<ShaderProgram> meshShader_;

    // LOD distances
    float lodNearDist_ = 20.0f;
    float lodMidDist_ = 50.0f;
    float lodFarDist_ = 100.0f;
    float maxDrawDistance_ = 200.0f;

    // Wind
    WindParams windParams_;
    float windTime_ = 0.0f;

    // Statistics
    size_t visibleCount_ = 0;
    size_t drawCalls_ = 0;
};

} // namespace vegetation
