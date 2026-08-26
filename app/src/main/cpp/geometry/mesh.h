#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <glm/glm.hpp>
#include <GLES3/gl3.h>

// Forward declaration
class ShaderProgram;
class Material;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 color;

    Vertex() : position(0.0f, 0.0f, 0.0f), normal(0.0f, 0.0f, 1.0f), texCoord(0.0f, 0.0f), color(1.0f, 1.0f, 1.0f) {}
    Vertex(const glm::vec3& pos, const glm::vec3& norm = glm::vec3(0.0f, 0.0f, 1.0f),
           const glm::vec2& uv = glm::vec2(0.0f, 0.0f), const glm::vec3& col = glm::vec3(1.0f, 1.0f, 1.0f))
        : position(pos), normal(norm), texCoord(uv), color(col) {}
};

class Mesh {
public:
    Mesh();
    ~Mesh();

    // Data setup
    void setVertices(const std::vector<Vertex>& vertices);
    void setIndices(const std::vector<unsigned int>& indices);
    void setMaterial(std::shared_ptr<Material> material);

    // GPU resource management
    void uploadToGPU();
    void cleanup();

    // Rendering
    void render(ShaderProgram& shader, const glm::mat4& modelMatrix);

    // Getters
    unsigned int getVAO() const { return vao; }
    unsigned int getIndexCount() const { return indexCount; }
    bool isReady() const { return vao != 0 && indexCount > 0; }

private:
    // GPU resources
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;

    // Mesh data
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int indexCount;

    // Material
    std::shared_ptr<Material> material;

    // Helper
    void setupVertexAttributes();
};

// ============================================
// Phase 30: Skinned Mesh (bone-weighted)
// ============================================

#include "nif_types.h"

class SkinnedMesh {
public:
    struct SkinVertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoord;
        glm::vec3 color;
        glm::vec4 boneWeights;    // max 4 bones (sum = 1.0)
        int32_t boneIndices[4];   // max 4 bone indices (no ivec4 in custom GLM)
    };

    SkinnedMesh();
    ~SkinnedMesh();

    // Data setup
    void setSkinVertices(const std::vector<SkinVertex>& vertices,
                         const std::vector<uint16_t>& indices);
    void setPartitions(const std::vector<NIFSkinPartition::Partition>& partitions);

    // GPU resource management
    void uploadToGPU();
    void cleanup();

    // Per-frame bone matrix update
    void updateBoneMatrices(const std::vector<glm::mat4>& boneMatrices);

    // Rendering
    void render(ShaderProgram& shader, const glm::mat4& modelMatrix);

    // Hardware query
    void queryHardwareLimits();

    bool isReady() const { return vao != 0 && indexCount > 0; }

private:
    static constexpr int DEFAULT_MAX_BONES = 64;

    // GPU resources
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    unsigned int boneUBO = 0;

    // Mesh data
    std::vector<SkinVertex> skinVertices;
    std::vector<uint16_t> indices;
    unsigned int indexCount = 0;

    // Partitions
    std::vector<NIFSkinPartition::Partition> partitions;
    std::vector<GLsizei> partitionCounts;    // triangle counts per partition
    std::vector<const void*> partitionOffsets; // byte offsets per partition

    // UBO config
    int maxBonesPerUBO = DEFAULT_MAX_BONES;
    bool supportsMultiDraw = false;

    void setupSkinVertexAttributes();
    void createBoneUBO();
    void buildPartitionRenderInfo();
};
