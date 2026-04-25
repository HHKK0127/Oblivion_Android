#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>

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
