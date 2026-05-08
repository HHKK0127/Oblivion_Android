#include "mesh.h"
#include <GLES3/gl3.h>
#include <android/log.h>

#define LOG_TAG_MESH "Mesh"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_MESH, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_MESH, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_MESH, __VA_ARGS__)

Mesh::Mesh()
    : vao(0), vbo(0), ebo(0), uploaded(false) {
}

Mesh::~Mesh() {
    cleanup();
}

bool Mesh::initialize() {
    LOGD("Mesh initialized");
    return true;
}

void Mesh::cleanup() {
    if (uploaded) {
        if (ebo) glDeleteBuffers(1, &ebo);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (vao) glDeleteVertexArrays(1, &vao);
        vao = vbo = ebo = 0;
        uploaded = false;
    }
    vertices.clear();
    normals.clear();
    texCoords.clear();
    indices.clear();
}

void Mesh::setVertices(const std::vector<glm::vec3>& verts) {
    vertices = verts;
}

void Mesh::setNormals(const std::vector<glm::vec3>& norms) {
    normals = norms;
}

void Mesh::setTexCoords(const std::vector<glm::vec2>& uvs) {
    texCoords = uvs;
}

void Mesh::setIndices(const std::vector<uint32_t>& idx) {
    indices = idx;
}

bool Mesh::uploadToGPU() {
    if (vertices.empty()) {
        LOGE("Cannot upload mesh with no vertices");
        return false;
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(glm::vec3),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    if (!indices.empty()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     indices.size() * sizeof(uint32_t),
                     indices.data(),
                     GL_STATIC_DRAW);
    }

    if (!normals.empty()) {
        GLuint normalVbo;
        glGenBuffers(1, &normalVbo);
        glBindBuffer(GL_ARRAY_BUFFER, normalVbo);
        glBufferData(GL_ARRAY_BUFFER,
                     normals.size() * sizeof(glm::vec3),
                     normals.data(),
                     GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    }

    glBindVertexArray(0);

    uploaded = true;
    LOGI("Mesh uploaded to GPU: %zu vertices, %zu indices",
         vertices.size(), indices.size());
    return true;
}

void Mesh::render(ShaderProgram* shader, const glm::mat4& modelMatrix) {
    if (!uploaded) return;

    glBindVertexArray(vao);

    if (!indices.empty()) {
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
    }

    glBindVertexArray(0);
}