#include "material.h"

Material::Material()
    : ambient(0.2f, 0.2f, 0.2f),
      diffuse(0.8f, 0.8f, 0.8f),
      specular(1.0f, 1.0f, 1.0f),
      shininess(32.0f) {
}

Material::~Material() = default;

void Material::setAmbient(const glm::vec3& color) { ambient = color; }
void Material::setDiffuse(const glm::vec3& color) { diffuse = color; }
void Material::setSpecular(const glm::vec3& color) { specular = color; }
void Material::setShininess(float s) { shininess = s; }