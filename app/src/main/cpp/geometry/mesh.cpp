#include "mesh.h"
#include <android/log.h>

#define LOG_TAG "Mesh"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

Mesh::Mesh() : VAO(0), VBO(0), EBO(0), textureId(0), vertexCount(0), indexCount(0) {}

Mesh::~Mesh() {
    cleanup();
}

bool Mesh::init(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices) {
    if (vertices.empty() || indices.empty()) {
        LOGE("Cannot create mesh with empty vertices or indices");
        return false;
    }

    vertexCount = vertices.size();
    indexCount = indices.size();

    LOGD("Initializing mesh: %zu vertices, %zu indices", vertexCount, indexCount);

    // Generate and setup VAO, VBO, EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Bind VAO
    glBindVertexArray(VAO);

    // Setup VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    // Setup EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint16_t), indices.data(), GL_STATIC_DRAW);

    // Setup vertex attributes
    setupVertexAttributes();

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    LOGD("Mesh initialized successfully");
    return true;
}

void Mesh::setupVertexAttributes() {
    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // Color attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    // TexCoord attribute (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);
}

void Mesh::render(const glm::mat4& model, const glm::mat4& view,
                  const glm::mat4& projection, GLuint shaderProgram) const {
    static int renderCount = 0;
    if (renderCount++ % 60 == 0) {  // Log every 60 frames
        LOGD("Mesh::render called - VAO=%u, VBO=%u, EBO=%u, indexCount=%zu, textureId=%u",
             VAO, VBO, EBO, indexCount, textureId);
    }

    glUseProgram(shaderProgram);

    // Set uniform matrices
    GLint modelLoc = glGetUniformLocation(shaderProgram, "uModel");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "uView");
    GLint projLoc = glGetUniformLocation(shaderProgram, "uProjection");

    // Create non-const copies for value_ptr()
    glm::mat4 modelCopy = model;
    glm::mat4 viewCopy = view;
    glm::mat4 projectionCopy = projection;

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, modelCopy.value_ptr());
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewCopy.value_ptr());
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionCopy.value_ptr());

    // Bind texture if available
    if (textureId != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        GLint texLoc = glGetUniformLocation(shaderProgram, "uTexture");
        glUniform1i(texLoc, 0);
    }

    // Render
    if (VAO == 0) {
        LOGE("ERROR: Mesh VAO is 0!");
        return;
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), GL_UNSIGNED_SHORT, 0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        LOGE("GL Error after glDrawElements: 0x%x", err);
    }

    glBindVertexArray(0);

    // Unbind texture
    if (textureId != 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void Mesh::cleanup() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (EBO != 0) {
        glDeleteBuffers(1, &EBO);
        EBO = 0;
    }
    LOGD("Mesh cleaned up");
}
