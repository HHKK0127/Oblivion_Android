#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

class Material {
public:
    Material();
    ~Material();

    // Properties
    void setColor(const glm::vec3& color);
    void setAmbient(const glm::vec3& ambient);
    void setDiffuse(const glm::vec3& diffuse);
    void setSpecular(const glm::vec3& specular);
    void setShininess(float shininess);

    // Texture
    void setTexture(unsigned int textureId);
    void setTextureFromFile(const std::string& filepath);

    // Getters
    glm::vec3 getColor() const { return color; }
    glm::vec3 getAmbient() const { return ambient; }
    glm::vec3 getDiffuse() const { return diffuse; }
    glm::vec3 getSpecular() const { return specular; }
    float getShininess() const { return shininess; }
    unsigned int getTextureId() const { return textureId; }
    bool hasTexture() const { return textureId != 0; }

    // Cleanup
    void cleanup();

private:
    glm::vec3 color;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    unsigned int textureId;
};
