#include "asset_manager.h"
#include <android/log.h>
#include <algorithm>
#include <fstream>
#include <sstream>

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

    // Update lastAccessTime for all cached entries
    for (auto& entry : meshCache) {
        entry.second.lastAccessTime += deltaTime;
    }
    for (auto& entry : textureCache) {
        entry.second.lastAccessTime += deltaTime;
    }

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

    closeAllArchives();

    if (ddsLoader) {
        ddsLoader->cleanup();
    }

    LOGD("AssetManager cleanup complete");
}

// ============================================================================
// BSA Archive Management
// ============================================================================

bool AssetManager::loadArchive(const std::string& bsaPath) {
    LOGD("Loading BSA archive: %s", bsaPath.c_str());

    auto archive = std::make_unique<BSArchive>();
    if (!archive->open(bsaPath)) {
        LOGE("Failed to open BSA archive: %s", bsaPath.c_str());
        return false;
    }

    LOGD("BSA archive loaded: %s (%u files, compressed=%s)",
         bsaPath.c_str(),
         archive->getFileCount(),
         archive->isCompressed() ? "yes" : "no");

    m_archives.push_back(std::move(archive));
    return true;
}

void AssetManager::closeAllArchives() {
    LOGD("Closing %zu BSA archives", m_archives.size());
    m_archives.clear();
}

// ============================================================================
// ESM Plugin Loading
// ============================================================================

bool AssetManager::loadEsm(const std::string& esmPath) {
    LOGD("Loading ESM/ESP plugin: %s", esmPath.c_str());
    return m_esmManager.loadPlugin(esmPath);
}

bool AssetManager::loadEsmFromArchive(const std::string& esmName) {
    LOGD("Loading ESM from BSA archive: %s", esmName.c_str());
    
    // The ESM is stored inside a BSA. Find the ESM file from the archives.
    std::string searchPath = esmName;
    std::replace(searchPath.begin(), searchPath.end(), '\\', '/');
    
    // Search all loaded BSA archives for the ESM file
    for (auto it = m_archives.rbegin(); it != m_archives.rend(); ++it) {
        const BSAFileEntry* entry = (*it)->findFile(searchPath);
        if (entry) {
            LOGD("Found ESM in BSA: %s (offset=%u, size=%u)", 
                 searchPath.c_str(), entry->offset, entry->size);
            
            std::vector<uint8_t> data;
            if ((*it)->extractFileDecompressed(*entry, data)) {
                LOGD("Extracted ESM data: %zu bytes", data.size());
                // Parse the ESM data directly
                return m_esmManager.loadPluginFromMemory(esmName, data.data(), data.size());
            }
        }
    }
    
    // Fallback to direct file
    if (!m_dataPath.empty()) {
        std::string fullPath = m_dataPath + "/" + searchPath;
        return m_esmManager.loadPlugin(fullPath);
    }
    
    LOGE("ESM file not found: %s", esmName.c_str());
    return false;
}

// ============================================================================
// BSA File Lookup
// ============================================================================

std::vector<uint8_t> AssetManager::loadFileData(const std::string& path) {
    std::string searchPath = path;
    std::replace(searchPath.begin(), searchPath.end(), '\\', '/');

    // Search BSA archives in reverse order (last loaded = highest priority)
    for (auto it = m_archives.rbegin(); it != m_archives.rend(); ++it) {
        const BSAFileEntry* entry = (*it)->findFile(searchPath);
        if (entry) {
            std::vector<uint8_t> data;
            if ((*it)->extractFileDecompressed(*entry, data)) {
                LOGD("Loaded file from BSA: %s (%zu bytes)", searchPath.c_str(), data.size());
                return data;
            }
        }
    }

    // Fallback to direct file access
    if (!m_dataPath.empty()) {
        std::string fullPath = m_dataPath + "/" + searchPath;
        std::ifstream file(fullPath, std::ios::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            std::vector<uint8_t> data(size);
            file.read(reinterpret_cast<char*>(data.data()), size);
            LOGD("Loaded file from disk: %s (%zu bytes)", fullPath.c_str(), data.size());
            return data;
        }
    }

    LOGE("File not found: %s", searchPath.c_str());
    return {};
}

bool AssetManager::fileExists(const std::string& path) const {
    std::string searchPath = path;
    std::replace(searchPath.begin(), searchPath.end(), '\\', '/');

    for (const auto& archive : m_archives) {
        if (archive->findFile(searchPath)) {
            return true;
        }
    }

    if (!m_dataPath.empty()) {
        std::string fullPath = m_dataPath + "/" + searchPath;
        std::ifstream file(fullPath);
        return file.good();
    }

    return false;
}

std::vector<std::string> AssetManager::findFiles(const std::string& prefix) const {
    std::vector<std::string> results;
    std::string searchPrefix = prefix;
    std::replace(searchPrefix.begin(), searchPrefix.end(), '\\', '/');

    for (const auto& archive : m_archives) {
        auto entries = archive->findFilesByPrefix(searchPrefix);
        for (const auto* entry : entries) {
            results.push_back(entry->fullPath);
        }
    }
    return results;
}

std::vector<std::string> AssetManager::findFilesByExtension(const std::string& ext) const {
    std::vector<std::string> results;
    for (const auto& archive : m_archives) {
        auto entries = archive->findFilesByExtension(ext);
        for (const auto* entry : entries) {
            results.push_back(entry->fullPath);
        }
    }
    return results;
}

// ============================================================================
// Asset Loading
// ============================================================================

std::shared_ptr<Mesh> AssetManager::loadNifMesh(const std::string& nifPath) {
    LOGD("Loading NIF mesh: %s", nifPath.c_str());

    // Check cache first
    if (meshCache.find(nifPath) != meshCache.end()) {
        LOGD("Mesh found in cache: %s", nifPath.c_str());
        meshCache[nifPath].lastAccessTime = 0.0f;
        return std::static_pointer_cast<Mesh>(meshCache[nifPath].asset);
    }

    // Try loading from BSA archives first
    std::vector<uint8_t> fileData = loadFileData(nifPath);
    if (!fileData.empty()) {
        auto mesh = loadNifFromData(nifPath, fileData.data(), fileData.size());
        if (mesh) {
            CacheEntry entry;
            entry.asset = mesh;
            entry.sizeBytes = sizeof(Mesh) + (mesh->getIndexCount() * sizeof(unsigned int));
            entry.lastAccessTime = 0.0f;
            meshCache[nifPath] = entry;
            currentCacheSize += entry.sizeBytes;
            LOGD("Mesh loaded from BSA: %s", nifPath.c_str());
            return mesh;
        }
    }

    // Fallback: try direct file path (legacy)
    // Normalize path separators for Android
    std::string normalizedPath = nifPath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    if (!nifParser->parseFile(normalizedPath)) {
        LOGE("Failed to parse NIF file: %s", nifPath.c_str());
        return nullptr;
    }

    // Create mesh from NIF geometry
    auto mesh = std::make_shared<Mesh>();

    auto geometries = nifParser->extractAllGeometry();
    if (!geometries.empty()) {
        const auto& geom = geometries[0];

        std::vector<Vertex> vertices;
        for (size_t i = 0; i < geom.vertices.size(); i++) {
            Vertex v;
            v.position = geom.vertices[i].toGLM();
            if (i < geom.normals.size()) v.normal = geom.normals[i].toGLM();
            if (i < geom.texCoords.size()) v.texCoord = geom.texCoords[i];
            if (i < geom.colors.size()) v.color = glm::vec3(geom.colors[i].x, geom.colors[i].y, geom.colors[i].z);
            vertices.push_back(v);
        }

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

    CacheEntry entry;
    entry.asset = mesh;
    entry.sizeBytes = sizeof(Mesh) + (mesh->getIndexCount() * sizeof(unsigned int));
    entry.lastAccessTime = 0.0f;
    meshCache[nifPath] = entry;
    currentCacheSize += entry.sizeBytes;

    return mesh;
}

std::shared_ptr<Mesh> AssetManager::loadNifFromData(const std::string& path,
                                                     const uint8_t* data, size_t dataSize) {
    // Generate a unique temp file path for the NIF data
    // Hash the path to avoid collisions
    std::string tempFile = "/data/data/com.example.oblivion/cache/bsa_nif_" +
                           std::to_string(std::hash<std::string>{}(path)) + ".nif";

    std::ofstream outFile(tempFile, std::ios::binary);
    if (!outFile.is_open()) {
        LOGE("Failed to create temp file for NIF: %s", path.c_str());
        return nullptr;
    }
    outFile.write(reinterpret_cast<const char*>(data), dataSize);
    outFile.close();

    // Parse directly from temp file (avoid recursion through loadNifMesh)
    if (!nifParser->parseFile(tempFile)) {
        LOGE("Failed to parse NIF from temp file: %s", path.c_str());
        std::remove(tempFile.c_str());
        return nullptr;
    }

    auto mesh = std::make_shared<Mesh>();
    auto geometries = nifParser->extractAllGeometry();
    if (!geometries.empty()) {
        const auto& geom = geometries[0];

        std::vector<Vertex> vertices;
        for (size_t i = 0; i < geom.vertices.size(); i++) {
            Vertex v;
            v.position = geom.vertices[i].toGLM();
            if (i < geom.normals.size()) v.normal = geom.normals[i].toGLM();
            if (i < geom.texCoords.size()) v.texCoord = geom.texCoords[i];
            if (i < geom.colors.size()) v.color = glm::vec3(geom.colors[i].x, geom.colors[i].y, geom.colors[i].z);
            vertices.push_back(v);
        }

        std::vector<unsigned int> indices;
        for (const auto& tri : geom.triangles) {
            indices.push_back(tri.v0);
            indices.push_back(tri.v1);
            indices.push_back(tri.v2);
        }

        mesh->setVertices(vertices);
        mesh->setIndices(indices);
        mesh->uploadToGPU();
    }

    // Clean up temp file
    std::remove(tempFile.c_str());
    return mesh;
}

std::shared_ptr<Material> AssetManager::loadDDSTexture(const std::string& ddsPath) {
    LOGD("Loading DDS texture: %s", ddsPath.c_str());

    if (textureCache.find(ddsPath) != textureCache.end()) {
        LOGD("Texture found in cache: %s", ddsPath.c_str());
        textureCache[ddsPath].lastAccessTime = 0.0f;
        return std::static_pointer_cast<Material>(textureCache[ddsPath].asset);
    }

    // Try loading from BSA archives
    std::string loadPath = ddsPath;
    bool usedTempFile = false;
    std::vector<uint8_t> fileData = loadFileData(ddsPath);
    if (!fileData.empty()) {
        std::string tempFile = "/data/data/com.example.oblivion/cache/bsa_dds_" +
                               std::to_string(std::hash<std::string>{}(ddsPath)) + ".dds";
        std::ofstream outFile(tempFile, std::ios::binary);
        if (outFile.is_open()) {
            outFile.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
            outFile.close();
            loadPath = tempFile;
            usedTempFile = true;
        }
    }

    if (!ddsLoader->loadFile(loadPath)) {
        LOGE("Failed to load DDS file: %s", ddsPath.c_str());
        if (usedTempFile) std::remove(loadPath.c_str());
        return nullptr;
    }

    if (!ddsLoader->decompressTexture()) {
        LOGE("Failed to decompress DDS texture: %s", ddsPath.c_str());
        if (usedTempFile) std::remove(loadPath.c_str());
        return nullptr;
    }

    auto material = std::make_shared<Material>();
    unsigned int texId = ddsLoader->uploadToGPU();
    if (texId != 0) {
        material->setTexture(texId);
        LOGD("Texture uploaded: ID=%u", texId);
    }

    // Clean up temp file
    if (usedTempFile) {
        std::remove(loadPath.c_str());
    }

    CacheEntry entry;
    entry.asset = material;
    const auto& ddsTexture = ddsLoader->getTexture();
    entry.sizeBytes = ddsTexture.width * ddsTexture.height * 4;
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
    while (currentCacheSize > maxCacheSize) {
        evictLRU();
    }
}

void AssetManager::evictLRU() {
    std::string lruKey;
    float maxTime = -1.0f;

    for (auto& entry : meshCache) {
        if (entry.second.lastAccessTime > maxTime) {
            maxTime = entry.second.lastAccessTime;
            lruKey = entry.first;
        }
    }

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
