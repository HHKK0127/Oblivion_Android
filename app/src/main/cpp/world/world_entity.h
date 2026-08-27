#pragma once

// Phase 31: World Entity
// Represents a loaded world object with mesh, collision, skeleton, and animation

#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "../geometry/mesh.h"
#include "../collision/collision_world.h"
#include "../animation/skeleton.h"
#include "../animation/animation_player.h"
#include "../assets/nif_types.h"
#include "../engine/imperial_weave.h"

// Forward declarations
class AssetManager;

// NIF parse cache to avoid re-parsing the same file
struct NIFCache {
    std::string filePath;
    bool valid = false;

    // Parsed data
    std::vector<std::shared_ptr<NIFNode>> nodes;
    std::vector<NIFNode> derefNodes;  // Dereferenced for Skeleton::buildFromNIF
    NIFSkinInstance skinInstance;
    NIFSkinData skinData;
    NIFSkinPartition skinPartition;
    bool hasSkin = false;

    NIFControllerManager controllerManager;
    bool hasAnimation = false;

    CollisionObject collisionObject;
    bool hasCollision = false;

    // Geometry
    std::vector<NIFGeometry> geometries;
};

// Entity type classification
enum class WorldEntityType : uint8_t {
    STATIC,     // Architecture, furniture (mesh + collision, no animation)
    DYNAMIC,    // Doors, containers (mesh + collision + simple animation)
    ACTOR,      // NPCs, creatures (skinned mesh + skeleton + animation + collision)
    TRIGGER     // Collision only, no visual mesh
};

// World entity: a single object in the world
struct WorldEntity {
    uint32_t entityId = 0;
    uint32_t npcId = 0;  // Associated NPC ID (0 if not an NPC)
    std::string nifPath;
    WorldEntityType type = WorldEntityType::STATIC;

    // Transform
    glm::vec3 position;
    glm::vec3 rotation;  // Euler angles (radians)
    glm::vec3 scale;

    // Visual
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<SkinnedMesh> skinnedMesh;

    // Physics
    int32_t collisionBodyId = -1;  // Index into CollisionWorld::bodies

    // Skeleton & Animation (nullptr for static objects)
    std::unique_ptr<Skeleton> skeleton;
    std::unique_ptr<animation::AnimationPlayer> animator;

    // State
    bool isActive = true;
    bool isVisible = true;

    WorldEntity() : position(0.0f, 0.0f, 0.0f),
                    rotation(0.0f, 0.0f, 0.0f),
                    scale(1.0f, 1.0f, 1.0f) {}

    // Compute model matrix from transform
    glm::mat4 getModelMatrix() const;
};

// WorldLoader: loads NIF files into WorldEntity instances
class WorldLoader {
public:
    WorldLoader();
    ~WorldLoader();

    void init(AssetManager* assets, CollisionWorld* collisionWorld);

    // Load entities by type
    WorldEntity loadStatic(const std::string& nifPath, const glm::vec3& pos,
                           const glm::vec3& rot = glm::vec3(0.0f, 0.0f, 0.0f),
                           const glm::vec3& scl = glm::vec3(1.0f, 1.0f, 1.0f));

    WorldEntity loadDynamic(const std::string& nifPath, const glm::vec3& pos,
                            const glm::vec3& rot = glm::vec3(0.0f, 0.0f, 0.0f),
                            const glm::vec3& scl = glm::vec3(1.0f, 1.0f, 1.0f));

    WorldEntity loadActor(const std::string& nifPath, const glm::vec3& pos,
                          const glm::vec3& rot = glm::vec3(0.0f, 0.0f, 0.0f),
                          const glm::vec3& scl = glm::vec3(1.0f, 1.0f, 1.0f));

    // Load actor for specific NPC (with npcId mapping)
    WorldEntity loadActorForNpc(const std::string& nifPath, const glm::vec3& pos,
                                uint32_t npcId,
                                const glm::vec3& rot = glm::vec3(0.0f, 0.0f, 0.0f),
                                const glm::vec3& scl = glm::vec3(1.0f, 1.0f, 1.0f));

    // Unload
    void unload(WorldEntity& entity);

    // NIF cache management
    void clearCache();
    size_t getCacheSize() const { return nifCache.size(); }

    // Entity ID counter
    uint32_t getNextEntityId() { return nextEntityId++; }

    // Imperial Weave EventBus integration
    void setEventBus(weave::EventBus* bus) { eventBus = bus; }

    // Get WorldEntity by NPC ID
    WorldEntity* getEntityByNpcId(uint32_t npcId);

private:
    AssetManager* assetManager = nullptr;
    CollisionWorld* collisionWorld = nullptr;
    weave::EventBus* eventBus = nullptr;
    uint32_t nextEntityId = 1;

    // NIF parse cache (path → cached data)
    std::unordered_map<std::string, std::shared_ptr<NIFCache>> nifCache;

    // WorldEntity storage (entityId → WorldEntity)
    std::unordered_map<uint32_t, std::unique_ptr<WorldEntity>> entities;

    // NPC ID to entityId mapping
    std::unordered_map<uint32_t, uint32_t> npcToEntityMap;

    // Get or parse NIF file
    std::shared_ptr<NIFCache> getOrParseNIF(const std::string& nifPath);

    // Convert NIF collision to CollisionBody
    int32_t convertCollision(const CollisionObject& obj,
                             const glm::vec3& worldPos,
                             const glm::vec3& worldRot,
                             const glm::vec3& worldScale);

    // Build mesh from NIF geometry
    std::shared_ptr<Mesh> buildMesh(const std::vector<NIFGeometry>& geometries);

    // Build skinned mesh from NIF data
    std::shared_ptr<SkinnedMesh> buildSkinnedMesh(const NIFCache& cache);

    // Build skeleton from NIF data
    std::unique_ptr<Skeleton> buildSkeleton(const NIFCache& cache);

    // Build animation player from NIF data
    std::unique_ptr<animation::AnimationPlayer> buildAnimator(
        const NIFCache& cache, Skeleton* skeleton);
};
