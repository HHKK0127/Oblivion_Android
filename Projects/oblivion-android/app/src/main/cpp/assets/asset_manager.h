#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <android/log.h>

class Mesh;

#define LOG_TAG_ASSET "AssetManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_ASSET, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_ASSET, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_ASSET, __VA_ARGS__)

class AssetManager {
public:
    AssetManager();
    ~AssetManager();

    bool initialize();
    void cleanup();

    // Mesh loading
    std::shared_ptr<Mesh> loadNifMesh(const std::string& path);
    std::shared_ptr<Mesh> getMesh(const std::string& path);

    // Texture loading
    bool loadDDSTexture(const std::string& path);

    // BSA management
    bool mountBSA(const std::string& bsaPath);
    bool unmountBSA(const std::string& bsaPath);

    // Asset queries
    bool hasMesh(const std::string& path) const;
    bool hasTexture(const std::string& path) const;

    void clearCache();

private:
    std::unordered_map<std::string, std::shared_ptr<Mesh>> meshCache;
    std::unordered_map<std::string, uint32_t> textureCache;
    std::vector<std::string> mountedBSAs;
    std::string dataPath;
};