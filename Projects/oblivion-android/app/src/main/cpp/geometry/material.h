#pragma once

#include <glm/glm.hpp>
#include <string>

class Material {
public:
    Material();
    ~Material();

    void setAmbient(const glm::vec3& color);
    void setDiffuse(const glm::vec3& color);
    void setSpecular(const glm::vec3& color);
    void setShininess(float shininess);

    const glm::vec3& getAmbient() const { return ambient; }
    const glm::vec3& getDiffuse() const { return diffuse; }
    const glm::vec3& getSpecular() const { return specular; }
    float getShininess() const { return shininess; }

private:
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};