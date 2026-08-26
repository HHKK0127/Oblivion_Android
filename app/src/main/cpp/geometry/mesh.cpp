#include "mesh.h"
#include "../engine/shader.h"
#include "material.h"
#include <GLES3/gl3.h>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGE
#define LOG_TAG "Mesh"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

Mesh::Mesh() : vao(0), vbo(0), ebo(0), indexCount(0), material(nullptr) {
}

Mesh::~Mesh() {
    cleanup();
}

void Mesh::setVertices(const std::vector<Vertex>& verts) {
    vertices = verts;
}

void Mesh::setIndices(const std::vector<unsigned int>& idx) {
    indices = idx;
    indexCount = idx.size();
}

void Mesh::setMaterial(std::shared_ptr<Material> mat) {
    material = mat;
}

void Mesh::uploadToGPU() {
    if (vertices.empty() || indices.empty()) {
        LOGE("Cannot upload mesh: vertices or indices are empty");
        return;
    }

    LOGD("Uploading mesh to GPU: %zu vertices, %zu indices", vertices.size(), indices.size());

    // Generate VAO, VBO, EBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    // VBO: Vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // EBO: Index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Set up vertex attributes
    setupVertexAttributes();

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    LOGD("Mesh uploaded successfully: VAO=%u, VBO=%u, EBO=%u", vao, vbo, ebo);
}

void Mesh::setupVertexAttributes() {
    // Position (vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // Normal (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    // TexCoord (vec2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    // Color (vec3)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(3);
}

void Mesh::render(ShaderProgram& shader, const glm::mat4& modelMatrix) {
    if (!isReady()) {
        LOGE("Mesh not ready for rendering");
        return;
    }

    shader.use();
    shader.setUniform("model", modelMatrix);

    // Bind and draw
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Mesh::cleanup() {
    if (ebo != 0) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    indexCount = 0;
    LOGD("Mesh cleaned up");
}

// ============================================
// SkinnedMesh implementation
// ============================================

SkinnedMesh::SkinnedMesh() = default;

SkinnedMesh::~SkinnedMesh() {
    cleanup();
}

void SkinnedMesh::queryHardwareLimits() {
    GLint maxUBOSize = 0;
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUBOSize);
    maxBonesPerUBO = std::min(DEFAULT_MAX_BONES, maxUBOSize / static_cast<int>(sizeof(glm::mat4)));
    LOGD("Max UBO size: %d bytes, max bones per UBO: %d", maxUBOSize, maxBonesPerUBO);
}

void SkinnedMesh::setSkinVertices(const std::vector<SkinVertex>& verts,
                                   const std::vector<uint16_t>& idx) {
    skinVertices = verts;
    indices = idx;
    indexCount = static_cast<unsigned int>(idx.size());
}

void SkinnedMesh::setPartitions(const std::vector<NIFSkinPartition::Partition>& parts) {
    partitions = parts;
    buildPartitionRenderInfo();
}

void SkinnedMesh::buildPartitionRenderInfo() {
    partitionCounts.clear();
    partitionOffsets.clear();
    uint32_t offset = 0;
    for (const auto& p : partitions) {
        partitionCounts.push_back(static_cast<GLsizei>(p.numTriangles * 3));
        partitionOffsets.push_back(reinterpret_cast<const void*>(offset * sizeof(uint16_t)));
        offset += p.numTriangles * 3;
    }
}

void SkinnedMesh::uploadToGPU() {
    if (skinVertices.empty() || indices.empty()) {
        LOGE("Cannot upload skinned mesh: data empty");
        return;
    }

    queryHardwareLimits();

    LOGD("Uploading skinned mesh: %zu vertices, %zu indices, %zu partitions",
         skinVertices.size(), indices.size(), partitions.size());

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, skinVertices.size() * sizeof(SkinVertex),
                 skinVertices.data(), GL_STATIC_DRAW);

    // EBO (uint16)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint16_t),
                 indices.data(), GL_STATIC_DRAW);

    setupSkinVertexAttributes();
    createBoneUBO();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    LOGD("Skinned mesh uploaded: VAO=%u, VBO=%u, EBO=%u, UBO=%u", vao, vbo, ebo, boneUBO);
}

void SkinnedMesh::setupSkinVertexAttributes() {
    // Position (vec3) - offset 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkinVertex),
                          (void*)offsetof(SkinVertex, position));
    glEnableVertexAttribArray(0);

    // Normal (vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SkinVertex),
                          (void*)offsetof(SkinVertex, normal));
    glEnableVertexAttribArray(1);

    // TexCoord (vec2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SkinVertex),
                          (void*)offsetof(SkinVertex, texCoord));
    glEnableVertexAttribArray(2);

    // Color (vec3)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(SkinVertex),
                          (void*)offsetof(SkinVertex, color));
    glEnableVertexAttribArray(3);

    // Bone weights (vec4)
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(SkinVertex),
                          (void*)offsetof(SkinVertex, boneWeights));
    glEnableVertexAttribArray(4);

    // Bone indices (ivec4) - integer attribute
    glVertexAttribIPointer(5, 4, GL_INT, sizeof(SkinVertex),
                           (void*)offsetof(SkinVertex, boneIndices));
    glEnableVertexAttribArray(5);
}

void SkinnedMesh::createBoneUBO() {
    glGenBuffers(1, &boneUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, boneUBO);
    // Allocate UBO with max bones
    size_t uboSize = maxBonesPerUBO * sizeof(glm::mat4);
    glBufferData(GL_UNIFORM_BUFFER, uboSize, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SkinnedMesh::updateBoneMatrices(const std::vector<glm::mat4>& boneMatrices) {
    if (boneUBO == 0) return;

    int count = std::min(static_cast<int>(boneMatrices.size()), maxBonesPerUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, boneUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, count * sizeof(glm::mat4), boneMatrices.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void SkinnedMesh::render(ShaderProgram& shader, const glm::mat4& modelMatrix) {
    if (!isReady()) return;

    shader.use();
    shader.setUniform("uModel", modelMatrix);

    glBindVertexArray(vao);
    // Bind bone UBO to binding point 0
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, boneUBO);

    if (!partitions.empty() && !partitionCounts.empty()) {
        // Draw per-partition
        for (size_t i = 0; i < partitions.size(); ++i) {
            glDrawElements(GL_TRIANGLES, partitionCounts[i],
                           GL_UNSIGNED_SHORT, partitionOffsets[i]);
        }
    } else {
        // Draw all at once
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);
    }

    glBindVertexArray(0);
}

void SkinnedMesh::cleanup() {
    if (boneUBO != 0) {
        glDeleteBuffers(1, &boneUBO);
        boneUBO = 0;
    }
    if (ebo != 0) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    indexCount = 0;
    LOGD("SkinnedMesh cleaned up");
}
