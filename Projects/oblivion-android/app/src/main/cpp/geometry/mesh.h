#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <glm/glm.hpp>

class ShaderProgram;

class Mesh {
public:
    Mesh();
    ~Mesh();

    bool initialize();
    void cleanup();

    void setVertices(const std::vector<glm::vec3>& verts);
    void setNormals(const std::vector<glm::vec3>& norms);
    void setTexCoords(const std::vector<glm::vec2>& uvs);
    void setIndices(const std::vector<uint32_t>& idx);

    bool uploadToGPU();
    void render(ShaderProgram* shader, const glm::mat4& modelMatrix);

    const std::vector<glm::vec3>& getVertices() const { return vertices; }
    const std::vector<uint32_t>& getIndices() const { return indices; }

    bool isUploaded() const { return uploaded; }

private:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<uint32_t> indices;

    uint32_t vao;
    uint32_t vbo;
    uint32_t ebo;
    bool uploaded;
};