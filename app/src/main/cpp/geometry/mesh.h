#pragma once

#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texCoord;

    Vertex() = default;
    Vertex(const glm::vec3& pos, const glm::vec3& col, const glm::vec2& tex)
        : position(pos), color(col), texCoord(tex) {}
};

class Mesh {
public:
    Mesh();
    ~Mesh();

    // Initialize mesh with vertex and index data
    bool init(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices);

    // Set texture for the mesh
    void setTexture(GLuint textureId) { this->textureId = textureId; }
    GLuint getTexture() const { return textureId; }
    bool hasTexture() const { return textureId != 0; }

    // Render the mesh with transformation matrices
    void render(const glm::mat4& model, const glm::mat4& view,
                const glm::mat4& projection, GLuint shaderProgram) const;

    // Clean up GPU resources
    void cleanup();

    // Get mesh properties
    size_t getVertexCount() const { return vertexCount; }
    size_t getIndexCount() const { return indexCount; }
    GLuint getVAO() const { return VAO; }

private:
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    GLuint textureId;
    size_t vertexCount;
    size_t indexCount;

    void setupVertexAttributes();
};
