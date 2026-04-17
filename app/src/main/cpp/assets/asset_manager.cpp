#include "asset_manager.h"
#include <android/log.h>
#include <chrono>
#include <algorithm>

#define LOG_TAG "AssetManager"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

AssetManager::AssetManager()
    : totalMemoryUsage(0), maxCacheSize(512 * 1024 * 1024) {  // 512 MB default
    LOGD("AssetManager created");
}

AssetManager::~AssetManager() {
    clearCache();
    LOGD("AssetManager destroyed");
}

bool AssetManager::initialize(const std::string& basePath) {
    this->basePath = basePath;
    LOGD("AssetManager initialized with base path: %s", basePath.c_str());
    return true;
}

std::shared_ptr<NifMeshData> AssetManager::loadMesh(const std::string& meshName) {
    LOGD("Loading mesh: %s", meshName.c_str());

    // Check if already in cache
    auto it = meshCache.find(meshName);
    if (it != meshCache.end()) {
        it->second.refCount++;
        it->second.lastAccessTime = std::chrono::system_clock::now().time_since_epoch().count();
        LOGD("Mesh found in cache. Ref count: %u", it->second.refCount);
        return it->second.meshData;
    }

    // Construct full path
    std::string fullPath = constructMeshPath(meshName);

    // Parse NIF file
    if (!parser.parseFile(fullPath)) {
        lastError = "Failed to parse NIF file: " + fullPath;
        LOGE("%s", lastError.c_str());
        return nullptr;
    }

    // Get the parsed meshes
    const auto& parsedMeshes = parser.getMeshes();
    if (parsedMeshes.empty()) {
        lastError = "No meshes found in NIF file: " + fullPath;
        LOGE("%s", lastError.c_str());
        return nullptr;
    }

    // Take the first mesh
    auto meshData = std::make_shared<NifMeshData>(parsedMeshes[0]);

    // Calculate memory size
    size_t meshSize = calculateMeshMemorySize(*meshData);

    // Check if we need to evict entries to make room
    while (totalMemoryUsage + meshSize > maxCacheSize && !meshCache.empty()) {
        evictLRUMesh();
    }

    // Add to cache
    CacheEntry entry;
    entry.meshData = meshData;
    entry.memorySize = meshSize;
    entry.refCount = 1;
    entry.lastAccessTime = std::chrono::system_clock::now().time_since_epoch().count();

    meshCache[meshName] = entry;
    totalMemoryUsage += meshSize;

    LOGD("Mesh loaded successfully. Cache size: %zu bytes, Cached meshes: %zu",
         totalMemoryUsage, meshCache.size());

    return meshData;
}

void AssetManager::unloadMesh(const std::string& meshName) {
    auto it = meshCache.find(meshName);
    if (it != meshCache.end()) {
        if (it->second.refCount > 0) {
            it->second.refCount--;
        }

        if (it->second.refCount == 0) {
            totalMemoryUsage -= it->second.memorySize;
            meshCache.erase(it);
            LOGD("Mesh unloaded: %s", meshName.c_str());
        } else {
            LOGD("Mesh ref count decreased to %u", it->second.refCount);
        }
    }
}

std::shared_ptr<NifMeshData> AssetManager::getMesh(const std::string& meshName) {
    auto it = meshCache.find(meshName);
    if (it != meshCache.end()) {
        it->second.lastAccessTime = std::chrono::system_clock::now().time_since_epoch().count();
        return it->second.meshData;
    }
    return nullptr;
}

void AssetManager::clearCache() {
    // Delete all GPU textures
    for (auto& entry : textureCache) {
        if (entry.second.textureId != 0) {
            glDeleteTextures(1, &entry.second.textureId);
        }
    }

    meshCache.clear();
    textureCache.clear();
    totalMemoryUsage = 0;
    LOGD("Asset cache cleared (meshes and textures)");
}

std::string AssetManager::constructMeshPath(const std::string& meshName) {
    // Construct path like: basePath/meshes/meshName.nif
    std::string path = basePath + "/meshes/" + meshName;

    // Add .nif extension if not already present
    if (path.substr(path.length() - 4) != ".nif") {
        path += ".nif";
    }

    return path;
}

size_t AssetManager::calculateMeshMemorySize(const NifMeshData& mesh) {
    size_t size = 0;

    // Approximate memory calculation
    size += mesh.vertices.size() * sizeof(NifVector3);
    size += mesh.colors.size() * sizeof(NifColor4);
    size += mesh.indices.size() * sizeof(uint16_t);
    size += mesh.normals.size() * sizeof(NifVector3);
    size += mesh.texCoords.size() * sizeof(std::pair<float, float>);
    size += mesh.name.capacity();
    size += mesh.texturePath.capacity();

    return size;
}

void AssetManager::evictLRUMesh() {
    // Find the mesh with the oldest access time and refCount of 0
    auto lruIt = meshCache.end();
    uint64_t oldestTime = UINT64_MAX;

    for (auto it = meshCache.begin(); it != meshCache.end(); ++it) {
        if (it->second.refCount == 0 && it->second.lastAccessTime < oldestTime) {
            oldestTime = it->second.lastAccessTime;
            lruIt = it;
        }
    }

    if (lruIt != meshCache.end()) {
        LOGD("Evicting mesh: %s (size: %zu bytes)", lruIt->first.c_str(), lruIt->second.memorySize);
        totalMemoryUsage -= lruIt->second.memorySize;
        meshCache.erase(lruIt);
    }
}

// ===== Texture Management Methods =====

GLuint AssetManager::loadTexture(const std::string& textureName) {
    LOGD("Loading texture: %s", textureName.c_str());

    // Check if already in cache
    auto it = textureCache.find(textureName);
    if (it != textureCache.end()) {
        it->second.refCount++;
        it->second.lastAccessTime = std::chrono::system_clock::now().time_since_epoch().count();
        LOGD("Texture found in cache. Ref count: %u", it->second.refCount);
        return it->second.textureId;
    }

    // For now, fallback to procedural texture
    LOGD("Texture file not found, using procedural texture");
    return loadTestTexture(256, 256);
}

GLuint AssetManager::loadTestTexture(int width, int height) {
    LOGD("Creating test texture (%dx%d)", width, height);

    GLuint textureId = createProceduralTexture(width, height);
    if (textureId != 0) {
        TextureCacheEntry entry;
        entry.textureId = textureId;
        entry.memorySize = calculateTextureMemorySize(width, height);
        entry.refCount = 1;
        entry.lastAccessTime = std::chrono::system_clock::now().time_since_epoch().count();

        textureCache["test_texture"] = entry;
        totalMemoryUsage += entry.memorySize;

        LOGD("Test texture created: ID=%u, Size=%zu bytes", textureId, entry.memorySize);
    }
    return textureId;
}

void AssetManager::unloadTexture(const std::string& textureName) {
    auto it = textureCache.find(textureName);
    if (it != textureCache.end()) {
        if (it->second.refCount > 0) {
            it->second.refCount--;
        }

        if (it->second.refCount == 0) {
            glDeleteTextures(1, &it->second.textureId);
            totalMemoryUsage -= it->second.memorySize;
            textureCache.erase(it);
            LOGD("Texture unloaded: %s", textureName.c_str());
        } else {
            LOGD("Texture ref count decreased to %u", it->second.refCount);
        }
    }
}

GLuint AssetManager::getTexture(const std::string& textureName) {
    auto it = textureCache.find(textureName);
    if (it != textureCache.end()) {
        it->second.lastAccessTime = std::chrono::system_clock::now().time_since_epoch().count();
        return it->second.textureId;
    }
    return 0;
}

std::string AssetManager::constructTexturePath(const std::string& textureName) {
    // Construct path like: basePath/textures/textureName.dds
    std::string path = basePath + "/textures/" + textureName;

    // Add .dds extension if not already present
    if (path.length() < 4 || path.substr(path.length() - 4) != ".dds") {
        path += ".dds";
    }

    return path;
}

size_t AssetManager::calculateTextureMemorySize(uint32_t width, uint32_t height) {
    // Approximate GPU memory for RGBA texture
    return width * height * 4;
}

void AssetManager::evictLRUTexture() {
    // Find the texture with the oldest access time and refCount of 0
    auto lruIt = textureCache.end();
    uint64_t oldestTime = UINT64_MAX;

    for (auto it = textureCache.begin(); it != textureCache.end(); ++it) {
        if (it->second.refCount == 0 && it->second.lastAccessTime < oldestTime) {
            oldestTime = it->second.lastAccessTime;
            lruIt = it;
        }
    }

    if (lruIt != textureCache.end()) {
        LOGD("Evicting texture: %s (size: %zu bytes)", lruIt->first.c_str(), lruIt->second.memorySize);
        glDeleteTextures(1, &lruIt->second.textureId);
        totalMemoryUsage -= lruIt->second.memorySize;
        textureCache.erase(lruIt);
    }
}

GLuint AssetManager::createProceduralTexture(int width, int height) {
    LOGD("Creating procedural checkerboard texture: %dx%d", width, height);

    // Create a simple checkerboard pattern
    std::vector<uint32_t> pixels(width * height);

    int checkSize = 16;  // 16x16 pixel blocks
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int checkX = (x / checkSize) % 2;
            int checkY = (y / checkSize) % 2;
            bool white = (checkX == checkY);

            // Create RGBA color (little-endian on most systems)
            uint32_t color = white ? 0xFFFFFFFF : 0xFF808080;  // White or Gray
            pixels[y * width + x] = color;
        }
    }

    // Create OpenGL texture
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    glBindTexture(GL_TEXTURE_2D, 0);

    LOGD("Procedural texture created: ID=%u", textureId);
    return textureId;
}

// ===== Cache Statistics =====

void AssetManager::logCacheStats() const {
    LOGD("===== Asset Manager Cache Statistics =====");
    LOGD("Total Memory Usage: %zu / %zu bytes (%.2f%%)",
         totalMemoryUsage, maxCacheSize,
         (float)totalMemoryUsage / maxCacheSize * 100.0f);
    LOGD("Cached Meshes: %zu", meshCache.size());
    LOGD("Cached Textures: %zu", textureCache.size());

    if (!meshCache.empty()) {
        LOGD("--- Mesh Cache Details ---");
        for (const auto& entry : meshCache) {
            LOGD("  %s: %zu bytes, RefCount=%u, LastAccess=%llu",
                 entry.first.c_str(), entry.second.memorySize,
                 entry.second.refCount, entry.second.lastAccessTime);
        }
    }

    if (!textureCache.empty()) {
        LOGD("--- Texture Cache Details ---");
        for (const auto& entry : textureCache) {
            LOGD("  %s: ID=%u, %zu bytes, RefCount=%u, LastAccess=%llu",
                 entry.first.c_str(), entry.second.textureId, entry.second.memorySize,
                 entry.second.refCount, entry.second.lastAccessTime);
        }
    }
    LOGD("======================================");
}
