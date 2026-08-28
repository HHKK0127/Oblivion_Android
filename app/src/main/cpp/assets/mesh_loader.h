#pragma once

#include "nif_parser.h"
#include "nif_types.h"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <android/log.h>

#define LOG_TAG_MESH "MeshLoader"
#define LOGD_MESH(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_MESH, __VA_ARGS__)
#define LOGI_MESH(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_MESH, __VA_ARGS__)
#define LOGW_MESH(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_MESH, __VA_ARGS__)
#define LOGE_MESH(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_MESH, __VA_ARGS__)

// ============================================================================
// Mesh Loader - NIF mesh -> OpenGL VBO/VAO conversion
//
// Handles: vertex data extraction, index buffer generation,
//          material/mesh group extraction, LOD mesh selection
// ============================================================================

// Vertex attribute layout for shaders
struct MeshVertex {
    float position[3];      // Location 0
    float normal[3];        // Location 1
    float texCoord[2];      // Location 2
    float boneWeights[4];   // Location 3 (skinning)
    uint8_t boneIndices[4]; // Location 4 (skinning)
};

// Material extracted from NIF
struct MeshMaterial {
    std::string name;
    float diffuseColor[3]  = {0.8f, 0.8f, 0.8f};
    float specularColor[3] = {1.0f, 1.0f, 1.0f};
    float ambientColor[3]  = {0.2f, 0.2f, 0.2f};
    float emissiveColor[3] = {0.0f, 0.0f, 0.0f};
    float specularExponent = 10.0f;
    float alpha = 1.0f;
    std::string diffuseTexture;
    std::string normalTexture;
};

// Sub-mesh (one draw call unit)
struct SubMesh {
    std::string name;
    uint32_t vao = 0;
    uint32_t vbo = 0;
    uint32_t ebo = 0;
    uint32_t indexCount = 0;
    uint32_t vertexCount = 0;
    int materialIndex = -1;
    bool hasSkinning = false;
};

// LOD level for a mesh
struct LODLevel {
    float distance = 0.0f;          // Switch distance
    std::vector<SubMesh> subMeshes;
};

// Complete loaded mesh
struct LoadedMesh {
    std::string name;
    std::vector<SubMesh> subMeshes;     // Default (highest detail)
    std::vector<LODLevel> lodLevels;    // LOD chain (optional)
    std::vector<MeshMaterial> materials;
    float boundingRadius = 0.0f;
    float boundingBoxMin[3] = {0.0f, 0.0f, 0.0f};
    float boundingBoxMax[3] = {0.0f, 0.0f, 0.0f};
};

class MeshLoader {
public:
    MeshLoader();
    ~MeshLoader();

    // Lifecycle
    bool initialize();
    void cleanup();

    // ========================================================================
    // Mesh Loading (NIF -> VBO/VAO)
    // ========================================================================

    // Load a NIF file and create GPU resources
    std::shared_ptr<LoadedMesh> loadMesh(const std::string& nifPath);

    // Load mesh from NIF data in memory
    std::shared_ptr<LoadedMesh> loadMeshFromData(const std::string& key,
                                                   const uint8_t* data,
                                                   size_t dataSize);

    // Load mesh from already-parsed NIF geometry
    std::shared_ptr<LoadedMesh> loadMeshFromGeometry(
        const std::string& name,
        const std::vector<NIFGeometry>& geometries);

    // ========================================================================
    // LOD Mesh Selection
    // ========================================================================

    // Load LOD meshes for a given base mesh
    bool loadLODMeshes(const std::string& basePath,
                        const std::vector<std::string>& lodPaths,
                        const std::vector<float>& switchDistances);

    // Select appropriate LOD level based on distance
    const LODLevel* selectLOD(const LoadedMesh& mesh, float distance) const;

    // ========================================================================
    // Mesh Cache
    // ========================================================================

    // Check if mesh is cached
    bool isCached(const std::string& key) const;

    // Get cached mesh
    std::shared_ptr<LoadedMesh> getCachedMesh(const std::string& key) const;

    // Unload a mesh and free GPU resources
    void unloadMesh(const std::string& key);

    // Clear all cached meshes
    void clearCache();

    // Get cache size
    size_t getCacheSize() const { return meshCache.size(); }

    // ========================================================================
    // Utility
    // ========================================================================

    // Calculate bounding sphere radius for a set of vertices
    static float calculateBoundingRadius(const std::vector<NIFVector3>& vertices);

    // Calculate AABB
    static void calculateAABB(const std::vector<NIFVector3>& vertices,
                               float outMin[3], float outMax[3]);

private:
    // NIF parser instance
    NIFParser nifParser;

    // Mesh cache
    std::unordered_map<std::string, std::shared_ptr<LoadedMesh>> meshCache;

    // Internal methods
    SubMesh createSubMesh(const NIFGeometry& geometry,
                           std::vector<MeshMaterial>& materials);
    std::vector<MeshVertex> buildVertexData(const NIFGeometry& geometry);
    std::vector<uint32_t> buildIndexData(const NIFGeometry& geometry);
    int findOrAddMaterial(std::vector<MeshMaterial>& materials,
                           const NIFGeometry& geometry);

    // GPU resource creation
    uint32_t createVAO();
    uint32_t createVBO(const void* data, size_t size);
    uint32_t createEBO(const void* data, size_t size);
    void setupVertexAttributes();
    void cleanupSubMesh(SubMesh& subMesh);
};
