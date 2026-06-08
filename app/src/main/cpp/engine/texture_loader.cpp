#include "texture_loader.h"
#include <android/asset_manager.h>
#include <android/log.h>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

extern "C" AAssetManager* jni_audio_get_asset_manager();

#define LOG_TAG "TextureLoader"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

std::unordered_map<GLuint, std::pair<int, int>> TextureLoader::s_textureSizes;

GLuint TextureLoader::loadTextureFromAsset(const std::string& filename) {
    LOGD("Loading texture: %s", filename.c_str());

    AAssetManager* mgr = jni_audio_get_asset_manager();
    if (!mgr) {
        LOGE("AAssetManager not initialized");
        return 0;
    }

    AAsset* asset = AAssetManager_open(mgr, filename.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("Failed to open asset: %s", filename.c_str());
        return 0;
    }

    off_t len = AAsset_getLength(asset);
    std::vector<unsigned char> buffer(len);
    AAsset_read(asset, buffer.data(), len);
    AAsset_close(asset);

    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(buffer.data(), static_cast<int>(len),
                                                   &width, &height, &channels, 4);
    if (!pixels) {
        LOGE("Failed to decode PNG: %s", filename.c_str());
        return 0;
    }

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    if (textureId == 0) {
        LOGE("glGenTextures failed");
        stbi_image_free(pixels);
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);

    s_textureSizes[textureId] = {width, height};

    LOGI("Texture loaded: %s (%dx%d, %d channels) -> id=%u",
         filename.c_str(), width, height, channels, textureId);
    return textureId;
}

void TextureLoader::deleteTexture(GLuint textureId) {
    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
        s_textureSizes.erase(textureId);
        LOGD("Texture deleted: id=%u", textureId);
    }
}

bool TextureLoader::getTextureSize(GLuint textureId, int& width, int& height) {
    if (textureId == 0) return false;
    auto it = s_textureSizes.find(textureId);
    if (it != s_textureSizes.end()) {
        width = it->second.first;
        height = it->second.second;
        return true;
    }
    return false;
}
