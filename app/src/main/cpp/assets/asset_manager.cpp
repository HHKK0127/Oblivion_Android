#include "asset_manager.h"
#include <android/log.h>
#include <algorithm>

#undef LOG_TAG
#undef LOGD
#undef LOGE
#define LOG_TAG "AssetManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

AssetManager::AssetManager()
    : maxCacheSize(500 * 1024 * 1024),  // 500 MB default
      currentCacheSize(0),
      cacheCheckInterval(30.0f),
      timeSinceLastCheck(0.0f) {
}

AssetManager::~AssetManager() {
    cleanup();
}

bool AssetManager::initialize() {
    LOGD("=== AssetManager initialization ===");

    // Initialize NIF parser
    nifParser = std::make_shared<NIFParser>();
    if (!nifParser) {
        LOGE("Failed to create NIFParser");
        return false;
    }

    // Initialize DDS loader
    ddsLoader = std::make_shared<DDSLoader>();
    if (!ddsLoader) {
        LOGE("Failed to create DDSLoader");
        return false;
    }

    LOGD("AssetManager initialized successfully");
    LOGD("Cache limit: %zu MB", maxCacheSize / (1024 * 1024));

    return true;
}

void AssetManager::update(float deltaTime) {
    timeSinceLastCheck += deltaTime;

    // Periodically check cache size
    if (timeSinceLastCheck >= cacheCheckInterval) {
        timeSinceLastCheck = 0.0f;

        if (currentCacheSize > maxCacheSize) {
            LOGD("Cache size (%zu MB) exceeds limit (%zu MB), pruning...",
                 currentCacheSize / (1024 * 1024),
                 maxCacheSize / (1024 * 1024));
            pruneCache();
        }
    }
}

void AssetManager::cleanup() {
    LOGD("Cleaning up AssetManager...");

    meshCache.clear();
    textureCache.clear();
    currentCacheSize = 0;

    if (ddsLoader) {
        ddsLoader->cleanup();
    }

    LOGD("AssetManager cleanup complete");
}

std::shared_ptr<Mesh> AssetManager::loadNifMesh(const std::string& nifPath) {
    LOGD("Loading NIF mesh: %s", nifPath.c_str());

    // Check cache first
    if (meshCache.find(nifPath) != meshCache.end()) {
        LOGD("Mesh found in cache: %s", nifPath.c_str());
        meshCache[nifPath].lastAccessTime = 0.0f;
        return std::static_pointer_cast<Mesh>(meshCache[nifPath].asset);
    }

    // Load NIF file
    if (!nifParser->parseFile(nifPath)) {
        LOGE("Failed to parse NIF file: %s", nifPath.c_str());
        return nullptr;
    }

    // Create mesh from NIF geometry
    auto mesh = std::make_shared<Mesh>();

    // Extract geometry from parsed NIF
    auto geometries = nifParser->extractAllGeometry();
    if (!geometries.empty()) {
        // Use first geometry for now
        // TODO: Handle multiple geometries per model
        const auto& geom = geometries[0];

        // Convert NIF vectors to Vertex format
        std::vector<Vertex> vertices;
        for (size_t i = 0; i < geom.vertices.size(); i++) {
            Vertex v;
            v.position = geom.vertices[i].toGLM();

            if (i < geom.normals.size()) {
                v.normal = geom.normals[i].toGLM();
            }

            if (i < geom.texCoords.size()) {
                v.texCoord = geom.texCoords[i];
            }

            if (i < geom.colors.size()) {
                v.color = glm::vec3(geom.colors[i].x, geom.colors[i].y, geom.colors[i].z);
            }

            vertices.push_back(v);
        }

        // Convert indices
        std::vector<unsigned int> indices;
        for (const auto& tri : geom.triangles) {
            indices.push_back(tri.v0);
            indices.push_back(tri.v1);
            indices.push_back(tri.v2);
        }

        mesh->setVertices(vertices);
        mesh->setIndices(indices);
        mesh->uploadToGPU();

        LOGD("Mesh created: %zu vertices, %zu indices", vertices.size(), indices.size());
    }

    // Cache the mesh
    CacheEntry entry;
    entry.asset = mesh;
    entry.sizeBytes = sizeof(Mesh) + (mesh->getIndexCount() * sizeof(unsigned int));
    entry.lastAccessTime = 0.0f;

    meshCache[nifPath] = entry;
    currentCacheSize += entry.sizeBytes;

    return mesh;
}

std::shared_ptr<Material> AssetManager::loadDDSTexture(const std::string& ddsPath) {
    LOGD("Loading DDS texture: %s", ddsPath.c_str());

    // Check cache first
    if (textureCache.find(ddsPath) != textureCache.end()) {
        LOGD("Texture found in cache: %s", ddsPath.c_str());
        textureCache[ddsPath].lastAccessTime = 0.0f;
        return std::static_pointer_cast<Material>(textureCache[ddsPath].asset);
    }

    // Load DDS file
    if (!ddsLoader->loadFile(ddsPath)) {
        LOGE("Failed to load DDS file: %s", ddsPath.c_str());
        return nullptr;
    }

    // Decompress texture
    if (!ddsLoader->decompressTexture()) {
        LOGE("Failed to decompress DDS texture: %s", ddsPath.c_str());
        return nullptr;
    }

    // Create material with texture
    auto material = std::make_shared<Material>();

    // Upload to GPU and get texture ID
    unsigned int texId = ddsLoader->uploadToGPU();
    if (texId != 0) {
        material->setTexture(texId);
        LOGD("Texture uploaded: ID=%u", texId);
    }

    // Cache the material
    CacheEntry entry;
    entry.asset = material;
    const auto& ddsTexture = ddsLoader->getTexture();
    entry.sizeBytes = ddsTexture.width * ddsTexture.height * 4;  // RGBA
    entry.lastAccessTime = 0.0f;

    textureCache[ddsPath] = entry;
    currentCacheSize += entry.sizeBytes;

    return material;
}

bool AssetManager::cacheExists(const std::string& key) const {
    return meshCache.find(key) != meshCache.end() || 
           textureCache.find(key) != textureCache.end();
}

void AssetManager::cacheEvict(const std::string& key) {
    auto meshIt = meshCache.find(key);
    if (meshIt != meshCache.end()) {
        currentCacheSize -= meshIt->second.sizeBytes;
        meshCache.erase(meshIt);
        LOGD("Evicted mesh from cache: %s", key.c_str());
    }

    auto texIt = textureCache.find(key);
    if (texIt != textureCache.end()) {
        currentCacheSize -= texIt->second.sizeBytes;
        textureCache.erase(texIt);
        LOGD("Evicted texture from cache: %s", key.c_str());
    }
}

size_t AssetManager::getCacheSize() const {
    return currentCacheSize;
}

void AssetManager::setCacheLimit(size_t bytes) {
    maxCacheSize = bytes;
    LOGD("Cache limit set to %zu MB", bytes / (1024 * 1024));
}

void AssetManager::pruneCache() {
    // Remove least recently used entries until under limit
    while (currentCacheSize > maxCacheSize) {
        evictLRU();
    }
}

void AssetManager::evictLRU() {
    std::string lruKey;
    float maxTime = -1.0f;

    // Find LRU mesh
    for (auto& entry : meshCache) {
        if (entry.second.lastAccessTime > maxTime) {
            maxTime = entry.second.lastAccessTime;
            lruKey = entry.first;
        }
    }

    // Find LRU texture
    for (auto& entry : textureCache) {
        if (entry.second.lastAccessTime > maxTime) {
            maxTime = entry.second.lastAccessTime;
            lruKey = entry.first;
        }
    }

    if (!lruKey.empty()) {
        cacheEvict(lruKey);
    }
}
