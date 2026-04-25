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
