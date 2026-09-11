#include "material.h"
#include <GLES3/gl3.h>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGE
#define LOG_TAG "Material"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

Material::Material()
    : color(1.0f, 1.0f, 1.0f),
      ambient(0.2f, 0.2f, 0.2f),
      diffuse(0.8f, 0.8f, 0.8f),
      specular(1.0f, 1.0f, 1.0f),
      shininess(32.0f),
      textureId(0),
      normalMapId(0),
      specularMapId(0) {
    LOGD("Material created with default properties");
}

Material::~Material() {
    cleanup();
}

void Material::setColor(const glm::vec3& col) {
    color = col;
}

void Material::setAmbient(const glm::vec3& amb) {
    ambient = amb;
}

void Material::setDiffuse(const glm::vec3& diff) {
    diffuse = diff;
}

void Material::setSpecular(const glm::vec3& spec) {
    specular = spec;
}

void Material::setShininess(float shine) {
    shininess = shine;
}

void Material::setTexture(unsigned int texId) {
    textureId = texId;
    LOGD("Texture set: %u", textureId);
}

void Material::setTextureFromFile(const std::string& filepath) {
    // TODO: Implement texture loading from file
    // This will be done in Phase 2 with DDS loader
    LOGD("Texture loading from file: %s (not yet implemented)", filepath.c_str());
}

// Normal Map
void Material::setNormalMap(unsigned int texId) {
    normalMapId = texId;
    LOGD("Normal map set: %u", normalMapId);
}

void Material::setNormalMapFromFile(const std::string& filepath) {
    // TODO: Implement normal map loading from file
    LOGD("Normal map loading from file: %s (not yet implemented)", filepath.c_str());
}

// Specular Map
void Material::setSpecularMap(unsigned int texId) {
    specularMapId = texId;
    LOGD("Specular map set: %u", specularMapId);
}

void Material::setSpecularMapFromFile(const std::string& filepath) {
    // TODO: Implement specular map loading from file
    LOGD("Specular map loading from file: %s (not yet implemented)", filepath.c_str());
}

void Material::cleanup() {
    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
        textureId = 0;
    }
    if (normalMapId != 0) {
        glDeleteTextures(1, &normalMapId);
        normalMapId = 0;
    }
    if (specularMapId != 0) {
        glDeleteTextures(1, &specularMapId);
        specularMapId = 0;
    }
}
