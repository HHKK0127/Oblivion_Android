#include "mesh_loader.h"
#include <GLES3/gl3.h>
#include <cstring>
#include <cmath>
#include <algorithm>

// ============================================================================
// MeshLoader Implementation
// ============================================================================

MeshLoader::MeshLoader() = default;

MeshLoader::~MeshLoader() {
    cleanup();
}

bool MeshLoader::initialize() {
    LOGI_MESH("MeshLoader initialized");
    return true;
}

void MeshLoader::cleanup() {
    clearCache();
    LOGI_MESH("MeshLoader cleaned up");
}

// ============================================================================
// Mesh Loading (NIF -> VBO/VAO)
// ============================================================================

std::shared_ptr<LoadedMesh> MeshLoader::loadMesh(const std::string& nifPath) {
    // Check cache
    auto it = meshCache.find(nifPath);
    if (it != meshCache.end()) {
        return it->second;
    }

    // Parse NIF file
    if (!nifParser.parseFile(nifPath)) {
        LOGE_MESH("Failed to parse NIF: %s", nifPath.c_str());
        return nullptr;
    }

    // Extract all geometry
    std::vector<NIFGeometry> geometries = nifParser.extractAllGeometry();
    if (geometries.empty()) {
        LOGW_MESH("No geometry in NIF: %s", nifPath.c_str());
        return nullptr;
    }

    auto mesh = loadMeshFromGeometry(nifPath, geometries);
    if (mesh) {
        meshCache[nifPath] = mesh;
    }
    return mesh;
}

std::shared_ptr<LoadedMesh> MeshLoader::loadMeshFromData(
    const std::string& key, const uint8_t* data, size_t dataSize) {
    // Check cache
    auto it = meshCache.find(key);
    if (it != meshCache.end()) {
        return it->second;
    }

    // Write data to temp file for NIF parser (it expects file path)
    std::string tmpPath = "/tmp/oblivion_mesh_" + key + ".nif";
    FILE* tmpFile = fopen(tmpPath.c_str(), "wb");
    if (!tmpFile) {
        LOGE_MESH("Failed to create temp file for: %s", key.c_str());
        return nullptr;
    }
    fwrite(data, 1, dataSize, tmpFile);
    fclose(tmpFile);

    auto mesh = loadMesh(tmpPath);
    remove(tmpPath.c_str());

    // Re-cache under the key
    if (mesh) {
        meshCache.erase(key); // Remove temp path entry if any
        meshCache[key] = mesh;
    }
    return mesh;
}

std::shared_ptr<LoadedMesh> MeshLoader::loadMeshFromGeometry(
    const std::string& name,
    const std::vector<NIFGeometry>& geometries) {

    auto mesh = std::make_shared<LoadedMesh>();
    mesh->name = name;

    // Track bounding box
    float minB[3] = {1e30f, 1e30f, 1e30f};
    float maxB[3] = {-1e30f, -1e30f, -1e30f};

    std::vector<MeshMaterial> materials;

    for (const auto& geo : geometries) {
        if (geo.vertices.empty()) continue;

        SubMesh sub = createSubMesh(geo, materials);
        mesh->subMeshes.push_back(sub);

        // Update bounding box
        for (const auto& v : geo.vertices) {
            minB[0] = std::min(minB[0], v.x);
            minB[1] = std::min(minB[1], v.y);
            minB[2] = std::min(minB[2], v.z);
            maxB[0] = std::max(maxB[0], v.x);
            maxB[1] = std::max(maxB[1], v.y);
            maxB[2] = std::max(maxB[2], v.z);
        }
    }

    mesh->materials = std::move(materials);
    std::memcpy(mesh->boundingBoxMin, minB, sizeof(float) * 3);
    std::memcpy(mesh->boundingBoxMax, maxB, sizeof(float) * 3);

    // Calculate bounding radius
    float cx = (minB[0] + maxB[0]) * 0.5f;
    float cy = (minB[1] + maxB[1]) * 0.5f;
    float cz = (minB[2] + maxB[2]) * 0.5f;
    float dx = maxB[0] - cx;
    float dy = maxB[1] - cy;
    float dz = maxB[2] - cz;
    mesh->boundingRadius = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (mesh->subMeshes.empty()) {
        LOGW_MESH("No sub-meshes created for: %s", name.c_str());
        return nullptr;
    }

    LOGI_MESH("Loaded mesh: %s (%lu sub-meshes, %lu materials)",
              name.c_str(),
              static_cast<unsigned long>(mesh->subMeshes.size()),
              static_cast<unsigned long>(mesh->materials.size()));
    return mesh;
}

// ============================================================================
// Sub-Mesh Creation
// ============================================================================

SubMesh MeshLoader::createSubMesh(const NIFGeometry& geometry,
                                    std::vector<MeshMaterial>& materials) {
    SubMesh sub;
    sub.name = geometry.name;
    sub.materialIndex = findOrAddMaterial(materials, geometry);

    // Build vertex data
    std::vector<MeshVertex> vertices = buildVertexData(geometry);
    std::vector<uint32_t> indices = buildIndexData(geometry);

    sub.vertexCount = static_cast<uint32_t>(vertices.size());
    sub.indexCount = static_cast<uint32_t>(indices.size());

    if (vertices.empty() || indices.empty()) {
        return sub;
    }

    // Create GPU resources
    sub.vao = createVAO();
    sub.vbo = createVBO(vertices.data(),
                         vertices.size() * sizeof(MeshVertex));
    sub.ebo = createEBO(indices.data(),
                         indices.size() * sizeof(uint32_t));

    // Bind VAO and set up vertex attributes
    glBindVertexArray(sub.vao);

    glBindBuffer(GL_ARRAY_BUFFER, sub.vbo);

    // Position (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                           reinterpret_cast<void*>(offsetof(MeshVertex, position)));

    // Normal (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                           reinterpret_cast<void*>(offsetof(MeshVertex, normal)));

    // TexCoord (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                           reinterpret_cast<void*>(offsetof(MeshVertex, texCoord)));

    // Bone weights (location 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                           reinterpret_cast<void*>(offsetof(MeshVertex, boneWeights)));

    // Bone indices (location 4)
    glEnableVertexAttribArray(4);
    glVertexAttribIPointer(4, 4, GL_UNSIGNED_BYTE, sizeof(MeshVertex),
                            reinterpret_cast<void*>(offsetof(MeshVertex, boneIndices)));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sub.ebo);
    glBindVertexArray(0);

    return sub;
}

std::vector<MeshVertex> MeshLoader::buildVertexData(
    const NIFGeometry& geometry) {
    size_t vertCount = geometry.vertices.size();
    std::vector<MeshVertex> vertices(vertCount);

    for (size_t i = 0; i < vertCount; i++) {
        MeshVertex& v = vertices[i];

        // Position
        v.position[0] = geometry.vertices[i].x;
        v.position[1] = geometry.vertices[i].y;
        v.position[2] = geometry.vertices[i].z;

        // Normal
        if (i < geometry.normals.size()) {
            v.normal[0] = geometry.normals[i].x;
            v.normal[1] = geometry.normals[i].y;
            v.normal[2] = geometry.normals[i].z;
        } else {
            v.normal[0] = 0.0f;
            v.normal[1] = 1.0f;
            v.normal[2] = 0.0f;
        }

        // UV
        if (i < geometry.texCoords.size()) {
            v.texCoord[0] = geometry.texCoords[i].x;
            v.texCoord[1] = geometry.texCoords[i].y;
        } else {
            v.texCoord[0] = 0.0f;
            v.texCoord[1] = 0.0f;
        }

        // Bone weights (default: no skinning)
        v.boneWeights[0] = 1.0f;
        v.boneWeights[1] = 0.0f;
        v.boneWeights[2] = 0.0f;
        v.boneWeights[3] = 0.0f;
        v.boneIndices[0] = 0;
        v.boneIndices[1] = 0;
        v.boneIndices[2] = 0;
        v.boneIndices[3] = 0;
    }

    return vertices;
}

std::vector<uint32_t> MeshLoader::buildIndexData(
    const NIFGeometry& geometry) {
    std::vector<uint32_t> indices;
    indices.reserve(geometry.triangles.size() * 3);

    for (const auto& tri : geometry.triangles) {
        indices.push_back(static_cast<uint32_t>(tri.v0));
        indices.push_back(static_cast<uint32_t>(tri.v1));
        indices.push_back(static_cast<uint32_t>(tri.v2));
    }

    return indices;
}

int MeshLoader::findOrAddMaterial(std::vector<MeshMaterial>& materials,
                                    const NIFGeometry& geometry) {
    // Check if material with same diffuse texture already exists
    for (size_t i = 0; i < materials.size(); i++) {
        if (materials[i].diffuseTexture == geometry.diffuseTexture) {
            return static_cast<int>(i);
        }
    }

    // Create new material
    MeshMaterial mat;
    mat.name = geometry.name;
    mat.diffuseTexture = geometry.diffuseTexture;
    mat.normalTexture = geometry.normalTexture;
    materials.push_back(mat);
    return static_cast<int>(materials.size()) - 1;
}

// ============================================================================
// LOD Mesh Selection
// ============================================================================

bool MeshLoader::loadLODMeshes(const std::string& basePath,
                                 const std::vector<std::string>& lodPaths,
                                 const std::vector<float>& switchDistances) {
    auto baseIt = meshCache.find(basePath);
    if (baseIt == meshCache.end()) {
        LOGE_MESH("Base mesh not loaded: %s", basePath.c_str());
        return false;
    }

    auto& mesh = baseIt->second;
    mesh->lodLevels.clear();

    // LOD 0 = full detail (already loaded as default sub-meshes)
    LODLevel lod0;
    lod0.distance = 0.0f;
    lod0.subMeshes = mesh->subMeshes;
    mesh->lodLevels.push_back(lod0);

    // Load additional LOD levels
    for (size_t i = 0; i < lodPaths.size() && i < switchDistances.size(); i++) {
        auto lodMesh = loadMesh(lodPaths[i]);
        if (!lodMesh) {
            LOGW_MESH("Failed to load LOD %lu: %s",
                       static_cast<unsigned long>(i), lodPaths[i].c_str());
            continue;
        }

        LODLevel lod;
        lod.distance = switchDistances[i];
        lod.subMeshes = lodMesh->subMeshes;
        mesh->lodLevels.push_back(lod);
    }

    // Sort by distance
    std::sort(mesh->lodLevels.begin(), mesh->lodLevels.end(),
              [](const LODLevel& a, const LODLevel& b) {
                  return a.distance < b.distance;
              });

    LOGI_MESH("Loaded %lu LOD levels for: %s",
              static_cast<unsigned long>(mesh->lodLevels.size()),
              basePath.c_str());
    return true;
}

const LODLevel* MeshLoader::selectLOD(const LoadedMesh& mesh,
                                        float distance) const {
    if (mesh.lodLevels.empty()) return nullptr;

    // Find the highest-detail LOD whose switch distance <= distance
    const LODLevel* best = &mesh.lodLevels[0];
    for (const auto& lod : mesh.lodLevels) {
        if (distance >= lod.distance) {
            best = &lod;
        } else {
            break;
        }
    }
    return best;
}

// ============================================================================
// GPU Resource Helpers
// ============================================================================

uint32_t MeshLoader::createVAO() {
    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    return vao;
}

uint32_t MeshLoader::createVBO(const void* data, size_t size) {
    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size),
                 data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vbo;
}

uint32_t MeshLoader::createEBO(const void* data, size_t size) {
    GLuint ebo = 0;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(size),
                 data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return ebo;
}

void MeshLoader::cleanupSubMesh(SubMesh& subMesh) {
    if (subMesh.vao) {
        glDeleteVertexArrays(1, &subMesh.vao);
        subMesh.vao = 0;
    }
    if (subMesh.vbo) {
        glDeleteBuffers(1, &subMesh.vbo);
        subMesh.vbo = 0;
    }
    if (subMesh.ebo) {
        glDeleteBuffers(1, &subMesh.ebo);
        subMesh.ebo = 0;
    }
}

// ============================================================================
// Cache Management
// ============================================================================

bool MeshLoader::isCached(const std::string& key) const {
    return meshCache.find(key) != meshCache.end();
}

std::shared_ptr<LoadedMesh> MeshLoader::getCachedMesh(
    const std::string& key) const {
    auto it = meshCache.find(key);
    return (it != meshCache.end()) ? it->second : nullptr;
}

void MeshLoader::unloadMesh(const std::string& key) {
    auto it = meshCache.find(key);
    if (it == meshCache.end()) return;

    auto& mesh = it->second;
    for (auto& sub : mesh->subMeshes) {
        cleanupSubMesh(sub);
    }
    for (auto& lod : mesh->lodLevels) {
        for (auto& sub : lod.subMeshes) {
            cleanupSubMesh(sub);
        }
    }
    meshCache.erase(it);
}

void MeshLoader::clearCache() {
    for (auto& pair : meshCache) {
        auto& mesh = pair.second;
        for (auto& sub : mesh->subMeshes) {
            cleanupSubMesh(sub);
        }
        for (auto& lod : mesh->lodLevels) {
            for (auto& sub : lod.subMeshes) {
                cleanupSubMesh(sub);
            }
        }
    }
    meshCache.clear();
}

// ============================================================================
// Utility
// ============================================================================

float MeshLoader::calculateBoundingRadius(
    const std::vector<NIFVector3>& vertices) {
    if (vertices.empty()) return 0.0f;

    // Find center
    float cx = 0, cy = 0, cz = 0;
    for (const auto& v : vertices) {
        cx += v.x; cy += v.y; cz += v.z;
    }
    float n = static_cast<float>(vertices.size());
    cx /= n; cy /= n; cz /= n;

    // Find max distance from center
    float maxDist = 0.0f;
    for (const auto& v : vertices) {
        float dx = v.x - cx;
        float dy = v.y - cy;
        float dz = v.z - cz;
        float dist = dx * dx + dy * dy + dz * dz;
        if (dist > maxDist) maxDist = dist;
    }
    return std::sqrt(maxDist);
}

void MeshLoader::calculateAABB(const std::vector<NIFVector3>& vertices,
                                 float outMin[3], float outMax[3]) {
    outMin[0] = outMin[1] = outMin[2] = 1e30f;
    outMax[0] = outMax[1] = outMax[2] = -1e30f;

    for (const auto& v : vertices) {
        outMin[0] = std::min(outMin[0], v.x);
        outMin[1] = std::min(outMin[1], v.y);
        outMin[2] = std::min(outMin[2], v.z);
        outMax[0] = std::max(outMax[0], v.x);
        outMax[1] = std::max(outMax[1], v.y);
        outMax[2] = std::max(outMax[2], v.z);
    }
}
