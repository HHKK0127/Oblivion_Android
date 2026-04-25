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
      textureId(0) {
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

void Material::cleanup() {
    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
        textureId = 0;
    }
}
