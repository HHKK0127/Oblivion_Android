#include "world_entity.h"
#include "../assets/asset_manager.h"
#include "../assets/nif_parser.h"
#include "../geometry/skin_partition_packer.h"
#include <android/log.h>
#include <cmath>

#define LOG_TAG_WL "WorldLoader"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_WL(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_WL, __VA_ARGS__)
#else
#define LOGD_WL(...) do {} while(0)
#endif
#define LOGI_WL(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_WL, __VA_ARGS__)
#define LOGW_WL(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_WL, __VA_ARGS__)
#define LOGE_WL(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_WL, __VA_ARGS__)

// ============================================================================
// WorldEntity
// ============================================================================

glm::mat4 WorldEntity::getModelMatrix() const {
    // Build TRS matrix: Translate * RotY * RotX * RotZ * Scale
    glm::mat4 T;
    T[3][0] = position.x;
    T[3][1] = position.y;
    T[3][2] = position.z;

    float cy = std::cos(rotation.y), sy = std::sin(rotation.y);
    float cx = std::cos(rotation.x), sx = std::sin(rotation.x);
    float cz = std::cos(rotation.z), sz = std::sin(rotation.z);

    glm::mat4 R;
    // Y * X * Z rotation order (Euler)
    R[0][0] = cy * cz + sy * sx * sz;
    R[0][1] = cx * sz;
    R[0][2] = -sy * cz + cy * sx * sz;
    R[1][0] = cy * (-sz) + sy * sx * cz;
    R[1][1] = cx * cz;
    R[1][2] = -sy * (-sz) + cy * sx * cz;
    R[2][0] = sy * cx;
    R[2][1] = -sx;
    R[2][2] = cy * cx;

    glm::mat4 S;
    S[0][0] = scale.x;
    S[1][1] = scale.y;
    S[2][2] = scale.z;

    // Multiply T * R * S manually (no glm::operator* for mat4)
    glm::mat4 TR;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            TR[i][j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                TR[i][j] += T[i][k] * R[k][j];
            }
        }
    }
    glm::mat4 TRS;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            TRS[i][j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                TRS[i][j] += TR[i][k] * S[k][j];
            }
        }
    }
    return TRS;
}

// ============================================================================
// WorldLoader
// ============================================================================

WorldLoader::WorldLoader() = default;
WorldLoader::~WorldLoader() = default;

void WorldLoader::init(AssetManager* assets, CollisionWorld* collision) {
    assetManager = assets;
    collisionWorld = collision;
    LOGI_WL("WorldLoader initialized");
}

// ----------------------------------------------------------------------------
// NIF Cache
// ----------------------------------------------------------------------------

std::shared_ptr<NIFCache> WorldLoader::getOrParseNIF(const std::string& nifPath) {
    auto it = nifCache.find(nifPath);
    if (it != nifCache.end()) {
        return it->second;
    }

    auto cache = std::make_shared<NIFCache>();
    cache->filePath = nifPath;

    // Parse NIF file
    NIFParser parser;
    if (!parser.parseFile(nifPath)) {
        LOGE_WL("Failed to parse NIF: %s", nifPath.c_str());
        nifCache[nifPath] = cache;
        return cache;
    }

    // Cache nodes
    cache->nodes = parser.getNodes();
    // Dereference shared_ptr for Skeleton::buildFromNIF
    cache->derefNodes.reserve(cache->nodes.size());
    for (auto& nodePtr : cache->nodes) {
        if (nodePtr) {
            cache->derefNodes.push_back(*nodePtr);
        }
    }

    // Cache geometry
    cache->geometries = parser.extractAllGeometry();

    // Try to parse skinning data
    cache->hasSkin = parser.parseNiSkinInstance(cache->skinInstance);
    if (cache->hasSkin) {
        parser.parseNiSkinData(cache->skinData);
        parser.parseNiSkinPartition(cache->skinPartition);
        LOGD_WL("NIF has skin: %u bones", cache->skinData.numBones);
    }

    // Try to parse animation data
    cache->hasAnimation = parser.parseNiControllerManager(cache->controllerManager);
    if (cache->hasAnimation) {
        LOGD_WL("NIF has animation: %u sequences",
                cache->controllerManager.controllerSequenceCount);
    }

    // Try to parse collision data
    cache->hasCollision = parser.parseBhkCollisionObject(cache->collisionObject);
    if (cache->hasCollision) {
        LOGD_WL("NIF has collision: shape type %d",
                static_cast<int>(cache->collisionObject.shape.type));
    }

    cache->valid = true;
    nifCache[nifPath] = cache;

    LOGI_WL("Cached NIF: %s (nodes=%zu, skin=%d, anim=%d, collision=%d)",
            nifPath.c_str(), cache->nodes.size(),
            cache->hasSkin ? 1 : 0,
            cache->hasAnimation ? 1 : 0,
            cache->hasCollision ? 1 : 0);

    return cache;
}

void WorldLoader::clearCache() {
    nifCache.clear();
    LOGI_WL("NIF cache cleared");
}

// ----------------------------------------------------------------------------
// Load Static (architecture, furniture)
// ----------------------------------------------------------------------------

WorldEntity WorldLoader::loadStatic(const std::string& nifPath, const glm::vec3& pos,
                                     const glm::vec3& rot, const glm::vec3& scl) {
    WorldEntity entity;
    entity.entityId = nextEntityId++;
    entity.nifPath = nifPath;
    entity.type = WorldEntityType::STATIC;
    entity.position = pos;
    entity.rotation = rot;
    entity.scale = scl;

    auto cache = getOrParseNIF(nifPath);
    if (!cache->valid) {
        LOGW_WL("Invalid NIF for static entity: %s", nifPath.c_str());
        return entity;
    }

    // Build visual mesh
    if (!cache->geometries.empty()) {
        entity.mesh = buildMesh(cache->geometries);
    }

    // Build collision body
    if (cache->hasCollision && collisionWorld) {
        entity.collisionBodyId = convertCollision(
            cache->collisionObject, pos, rot, scl);
    }

    LOGD_WL("Loaded static entity #%u: %s (mesh=%d, body=%d)",
            entity.entityId, nifPath.c_str(),
            entity.mesh ? 1 : 0, entity.collisionBodyId >= 0 ? 1 : 0);

    return entity;
}

// ----------------------------------------------------------------------------
// Load Dynamic (doors, containers)
// ----------------------------------------------------------------------------

WorldEntity WorldLoader::loadDynamic(const std::string& nifPath, const glm::vec3& pos,
                                      const glm::vec3& rot, const glm::vec3& scl) {
    WorldEntity entity;
    entity.entityId = nextEntityId++;
    entity.nifPath = nifPath;
    entity.type = WorldEntityType::DYNAMIC;
    entity.position = pos;
    entity.rotation = rot;
    entity.scale = scl;

    auto cache = getOrParseNIF(nifPath);
    if (!cache->valid) {
        LOGW_WL("Invalid NIF for dynamic entity: %s", nifPath.c_str());
        return entity;
    }

    // Build visual mesh
    if (!cache->geometries.empty()) {
        entity.mesh = buildMesh(cache->geometries);
    }

    // Build collision body
    if (cache->hasCollision && collisionWorld) {
        entity.collisionBodyId = convertCollision(
            cache->collisionObject, pos, rot, scl);
    }

    // Build skeleton + animation if available
    if (cache->hasSkin) {
        entity.skeleton = buildSkeleton(*cache);
        if (entity.skeleton && cache->hasAnimation) {
            entity.animator = buildAnimator(*cache, entity.skeleton.get());
        }
    }

    LOGD_WL("Loaded dynamic entity #%u: %s (mesh=%d, body=%d, skel=%d, anim=%d)",
            entity.entityId, nifPath.c_str(),
            entity.mesh ? 1 : 0, entity.collisionBodyId >= 0 ? 1 : 0,
            entity.skeleton ? 1 : 0, entity.animator ? 1 : 0);

    return entity;
}

// ----------------------------------------------------------------------------
// Load Actor (NPCs, creatures)
// ----------------------------------------------------------------------------

WorldEntity WorldLoader::loadActor(const std::string& nifPath, const glm::vec3& pos,
                                    const glm::vec3& rot, const glm::vec3& scl) {
    WorldEntity entity;
    entity.entityId = nextEntityId++;
    entity.nifPath = nifPath;
    entity.type = WorldEntityType::ACTOR;
    entity.position = pos;
    entity.rotation = rot;
    entity.scale = scl;

    auto cache = getOrParseNIF(nifPath);
    if (!cache->valid) {
        LOGW_WL("Invalid NIF for actor entity: %s", nifPath.c_str());
        return entity;
    }

    // Build skinned mesh (preferred) or static mesh
    if (cache->hasSkin) {
        entity.skinnedMesh = buildSkinnedMesh(*cache);
        entity.skeleton = buildSkeleton(*cache);

        if (entity.skeleton && cache->hasAnimation) {
            entity.animator = buildAnimator(*cache, entity.skeleton.get());
        }
    } else if (!cache->geometries.empty()) {
        // Fallback to static mesh if no skin data
        entity.mesh = buildMesh(cache->geometries);
        LOGW_WL("Actor has no skin data, using static mesh: %s", nifPath.c_str());
    }

    // Build collision body
    if (cache->hasCollision && collisionWorld) {
        entity.collisionBodyId = convertCollision(
            cache->collisionObject, pos, rot, scl);
    }

    // Store entity
    entities[entity.entityId] = std::make_unique<WorldEntity>(std::move(entity));

    LOGD_WL("Loaded actor entity #%u: %s (skinned=%d, skel=%d, anim=%d, body=%d)",
            entity.entityId, nifPath.c_str(),
            entity.skinnedMesh ? 1 : 0,
            entity.skeleton ? 1 : 0,
            entity.animator ? 1 : 0,
            entity.collisionBodyId >= 0 ? 1 : 0);

    return entity;
}

// ============================================================================
// Load actor for specific NPC (with npcId mapping)
// ============================================================================
WorldEntity WorldLoader::loadActorForNpc(const std::string& nifPath, const glm::vec3& pos,
                                         uint32_t npcId,
                                         const glm::vec3& rot, const glm::vec3& scl) {
    WorldEntity entity = loadActor(nifPath, pos, rot, scl);
    entity.npcId = npcId;

    // Update mapping
    npcToEntityMap[npcId] = entity.entityId;
    entities[entity.entityId] = std::make_unique<WorldEntity>(std::move(entity));

    LOGD_WL("Loaded actor for NPC: npcId=%u, entityId=%u", npcId, entity.entityId);
    return entity;
}

// ----------------------------------------------------------------------------
// Unload
// ----------------------------------------------------------------------------

void WorldLoader::unload(WorldEntity& entity) {
    // Remove collision body
    if (entity.collisionBodyId >= 0 && collisionWorld) {
        collisionWorld->removeBody(entity.collisionBodyId);
        entity.collisionBodyId = -1;
    }

    // Cleanup GPU resources
    if (entity.mesh) {
        entity.mesh->cleanup();
        entity.mesh.reset();
    }

    // Remove from entity storage
    if (entity.entityId > 0) {
        entities.erase(entity.entityId);
        if (entity.npcId > 0) {
            npcToEntityMap.erase(entity.npcId);
        }
    }
}

// ============================================================================
// Get WorldEntity by NPC ID
// ============================================================================
WorldEntity* WorldLoader::getEntityByNpcId(uint32_t npcId) {
    auto it = npcToEntityMap.find(npcId);
    if (it != npcToEntityMap.end()) {
        auto entityIt = entities.find(it->second);
        if (entityIt != entities.end()) {
            return entityIt->second.get();
        }
    }
    return nullptr;
}

// ----------------------------------------------------------------------------
// Collision Conversion
// ----------------------------------------------------------------------------

int32_t WorldLoader::convertCollision(const CollisionObject& obj,
                                       const glm::vec3& worldPos,
                                       const glm::vec3& worldRot,
                                       const glm::vec3& worldScale) {
    if (!collisionWorld) return -1;

    CollisionBody body;
    body.position = worldPos;
    body.rotation = worldRot;
    body.scale = worldScale;
    body.isStatic = true;
    body.isTrigger = obj.bodyInfo.isTrigger;
    body.mass = obj.bodyInfo.mass;
    body.friction = obj.bodyInfo.friction;
    body.restitution = obj.bodyInfo.restitution;
    body.collisionGroup = obj.bodyInfo.collisionGroup;
    body.collisionFilter = obj.bodyInfo.collisionFilter;

    // Map NIF collision shape to CollisionWorld shape type
    switch (obj.shape.type) {
        case CollisionShapeType::Box:
            body.shapeType = ShapeType::BOX;
            body.halfExtents = obj.shape.halfExtents.toGLM();
            break;

        case CollisionShapeType::Sphere:
            body.shapeType = ShapeType::SPHERE;
            body.radius = obj.shape.radius;
            break;

        case CollisionShapeType::Capsule:
            body.shapeType = ShapeType::CAPSULE;
            body.radius = obj.shape.radius;
            body.height = obj.shape.height;
            break;

        case CollisionShapeType::ConvexHull:
            body.shapeType = ShapeType::CONVEX;
            // Convex hull vertices are stored in shape data
            // For now, approximate as box from vertex bounds
            if (!obj.shape.vertices.empty()) {
                glm::vec3 minV(1e9f, 1e9f, 1e9f), maxV(-1e9f, -1e9f, -1e9f);
                for (const auto& v : obj.shape.vertices) {
                    glm::vec3 gv = v.toGLM();
                    minV.x = std::min(minV.x, gv.x);
                    minV.y = std::min(minV.y, gv.y);
                    minV.z = std::min(minV.z, gv.z);
                    maxV.x = std::max(maxV.x, gv.x);
                    maxV.y = std::max(maxV.y, gv.y);
                    maxV.z = std::max(maxV.z, gv.z);
                }
                body.halfExtents = (maxV - minV) * 0.5f;
                body.shapeType = ShapeType::BOX;  // Approximate
            }
            break;

        case CollisionShapeType::TriMesh:
        case CollisionShapeType::MoppBvTree:
            body.shapeType = ShapeType::MESH;
            // TriMesh collision is complex; for now use as static mesh
            break;

        default:
            LOGW_WL("Unsupported collision shape type: %d",
                    static_cast<int>(obj.shape.type));
            return -1;
    }

    int32_t bodyId = collisionWorld->addBody(body);
    if (bodyId < 0) {
        LOGE_WL("Failed to add collision body to world");
    }
    return bodyId;
}

// ----------------------------------------------------------------------------
// Mesh Building
// ----------------------------------------------------------------------------

std::shared_ptr<Mesh> WorldLoader::buildMesh(const std::vector<NIFGeometry>& geometries) {
    if (geometries.empty()) return nullptr;

    auto mesh = std::make_shared<Mesh>();

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // Merge all geometries into one mesh
    for (const auto& geo : geometries) {
        unsigned int baseIndex = static_cast<unsigned int>(vertices.size());

        // Add vertices
        for (size_t i = 0; i < geo.vertices.size(); i++) {
            Vertex v;
            v.position = geo.vertices[i].toGLM();

            if (i < geo.normals.size()) {
                v.normal = geo.normals[i].toGLM();
            }
            if (i < geo.texCoords.size()) {
                v.texCoord = geo.texCoords[i];
            }
            if (i < geo.colors.size()) {
                v.color = glm::vec3(geo.colors[i].x, geo.colors[i].y, geo.colors[i].z);
            }

            vertices.push_back(v);
        }

        // Add indices
        for (const auto& tri : geo.triangles) {
            indices.push_back(baseIndex + tri.v0);
            indices.push_back(baseIndex + tri.v1);
            indices.push_back(baseIndex + tri.v2);
        }
    }

    if (vertices.empty() || indices.empty()) return nullptr;

    mesh->setVertices(vertices);
    mesh->setIndices(indices);
    mesh->uploadToGPU();

    return mesh;
}

// ----------------------------------------------------------------------------
// Skinned Mesh Building
// ----------------------------------------------------------------------------

std::shared_ptr<SkinnedMesh> WorldLoader::buildSkinnedMesh(const NIFCache& cache) {
    if (!cache.hasSkin || cache.geometries.empty()) return nullptr;

    auto skinnedMesh = std::make_shared<SkinnedMesh>();

    // Build skin vertices from geometry + skin data
    std::vector<SkinnedMesh::SkinVertex> skinVertices;
    std::vector<uint16_t> skinIndices;

    // Use first geometry (most common case for skinned meshes)
    const auto& geo = cache.geometries[0];

    // Build vertex weights from NIFSkinData
    // Map: vertexIndex → [(boneIndex, weight), ...]
    std::unordered_map<uint16_t, std::vector<std::pair<uint16_t, float>>> vertexWeightMap;
    for (size_t boneIdx = 0; boneIdx < cache.skinData.boneData.size(); boneIdx++) {
        const auto& boneData = cache.skinData.boneData[boneIdx];
        for (const auto& vw : boneData.vertexWeights) {
            vertexWeightMap[vw.vertexIndex].push_back(
                {static_cast<uint16_t>(boneIdx), vw.weight});
        }
    }

    // Build skin vertices
    for (size_t i = 0; i < geo.vertices.size(); i++) {
        SkinnedMesh::SkinVertex sv;
        sv.position = geo.vertices[i].toGLM();

        if (i < geo.normals.size()) {
            sv.normal = geo.normals[i].toGLM();
        }
        if (i < geo.texCoords.size()) {
            sv.texCoord = geo.texCoords[i];
        }
        if (i < geo.colors.size()) {
            sv.color = glm::vec3(geo.colors[i].x, geo.colors[i].y, geo.colors[i].z);
        }

        // Apply bone weights (up to 4)
        auto wit = vertexWeightMap.find(static_cast<uint16_t>(i));
        if (wit != vertexWeightMap.end()) {
            auto& weights = wit->second;
            // Sort by weight descending
            std::sort(weights.begin(), weights.end(),
                      [](const auto& a, const auto& b) { return a.second > b.second; });

            float totalWeight = 0.0f;
            float* wptr = &sv.boneWeights.x;
            for (size_t w = 0; w < 4 && w < weights.size(); w++) {
                sv.boneIndices[w] = weights[w].first;
                wptr[w] = weights[w].second;
                totalWeight += weights[w].second;
            }
            // Normalize weights
            if (totalWeight > 0.0f) {
                for (int w = 0; w < 4; w++) {
                    wptr[w] /= totalWeight;
                }
            }
        } else {
            // No weights: default to bone 0
            sv.boneIndices[0] = 0;
            sv.boneWeights.x = 1.0f;
        }

        skinVertices.push_back(sv);
    }

    // Build indices
    for (const auto& tri : geo.triangles) {
        skinIndices.push_back(tri.v0);
        skinIndices.push_back(tri.v1);
        skinIndices.push_back(tri.v2);
    }

    if (skinVertices.empty() || skinIndices.empty()) return nullptr;

    skinnedMesh->setSkinVertices(skinVertices, skinIndices);

    // Apply skin partitions if available
    if (!cache.skinPartition.partitions.empty()) {
        skinnedMesh->setPartitions(cache.skinPartition.partitions);
    }

    skinnedMesh->uploadToGPU();
    skinnedMesh->queryHardwareLimits();

    return skinnedMesh;
}

// ----------------------------------------------------------------------------
// Skeleton Building
// ----------------------------------------------------------------------------

std::unique_ptr<Skeleton> WorldLoader::buildSkeleton(const NIFCache& cache) {
    if (!cache.hasSkin) return nullptr;

    auto skeleton = std::make_unique<Skeleton>();

    if (!skeleton->buildFromNIF(cache.derefNodes, cache.skinInstance, cache.skinData)) {
        LOGE_WL("Failed to build skeleton from NIF data");
        return nullptr;
    }

    skeleton->setBindPose();
    skeleton->update();

    LOGD_WL("Built skeleton with %d bones", skeleton->getBoneCount());
    return skeleton;
}

// ----------------------------------------------------------------------------
// Animation Player Building
// ----------------------------------------------------------------------------

std::unique_ptr<animation::AnimationPlayer> WorldLoader::buildAnimator(
    const NIFCache& cache, Skeleton* skeleton) {
    if (!cache.hasAnimation || !skeleton) return nullptr;

    auto animator = std::make_unique<animation::AnimationPlayer>();

    animator->initialize(skeleton, &cache.controllerManager.sequences);

    // Connect to Imperial Weave EventBus if available
    if (eventBus) {
        animator->setEventBus(eventBus);
    }

    // Auto-play first sequence (idle) if available
    if (!cache.controllerManager.sequences.empty()) {
        animator->play(0, true, 1.0f);
        LOGD_WL("Auto-playing animation sequence 0: %s",
                cache.controllerManager.sequences[0].name.c_str());
    }

    return animator;
}
