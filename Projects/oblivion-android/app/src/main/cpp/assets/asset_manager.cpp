#include "asset_manager.h"
#include "../geometry/mesh.h"
#include <GLES3/gl3.h>

AssetManager::AssetManager()
    : dataPath("/sdcard/Oblivion/Data") {
}

AssetManager::~AssetManager() {
    cleanup();
}

bool AssetManager::initialize() {
    LOGI("AssetManager initialized with data path: %s", dataPath.c_str());
    return true;
}

void AssetManager::cleanup() {
    meshCache.clear();
    textureCache.clear();
    mountedBSAs.clear();
    LOGD("AssetManager cleaned up");
}

std::shared_ptr<Mesh> AssetManager::loadNifMesh(const std::string& path) {
    auto it = meshCache.find(path);
    if (it != meshCache.end()) {
        return it->second;
    }

    LOGD("Loading NIF mesh: %s", path.c_str());

    auto mesh = std::make_shared<Mesh>();
    mesh->initialize();

    meshCache[path] = mesh;
    return mesh;
}

std::shared_ptr<Mesh> AssetManager::getMesh(const std::string& path) {
    auto it = meshCache.find(path);
    if (it != meshCache.end()) {
        return it->second;
    }
    return nullptr;
}

bool AssetManager::loadDDSTexture(const std::string& path) {
    LOGD("Loading DDS texture: %s", path.c_str());
    return true;
}

bool AssetManager::mountBSA(const std::string& bsaPath) {
    LOGI("Mounting BSA: %s", bsaPath.c_str());
    mountedBSAs.push_back(bsaPath);
    return true;
}

bool AssetManager::unmountBSA(const std::string& bsaPath) {
    auto it = std::find(mountedBSAs.begin(), mountedBSAs.end(), bsaPath);
    if (it != mountedBSAs.end()) {
        mountedBSAs.erase(it);
        return true;
    }
    return false;
}

bool AssetManager::hasMesh(const std::string& path) const {
    return meshCache.find(path) != meshCache.end();
}

bool AssetManager::hasTexture(const std::string& path) const {
    return textureCache.find(path) != textureCache.end();
}

void AssetManager::clearCache() {
    meshCache.clear();
    textureCache.clear();
    LOGI("Asset cache cleared");
}