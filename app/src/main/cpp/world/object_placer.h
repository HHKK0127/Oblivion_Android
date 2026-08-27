#pragma once

#include "world_data.h"
#include "../assets/esm_reader.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <android/log.h>

// Forward declarations
class AssetManager;

#define LOG_TAG_OBJPLACE "ObjectPlacer"
#define LOGD_OBJ(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_OBJPLACE, __VA_ARGS__)
#define LOGI_OBJ(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_OBJPLACE, __VA_ARGS__)
#define LOGW_OBJ(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_OBJPLACE, __VA_ARGS__)
#define LOGE_OBJ(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_OBJPLACE, __VA_ARGS__)

// ============================================================================
// Object Placer - Places objects from ESM REFR records into cells
// ============================================================================

class ObjectPlacer {
public:
    ObjectPlacer();
    ~ObjectPlacer();

    // ========================================================================
    // Initialization
    // ========================================================================

    bool initialize(AssetManager* assetMgr);
    void cleanup();

    // ========================================================================
    // Object Placement
    // ========================================================================

    // Place all objects for a cell from ESM reference data
    bool placeObjectsForCell(std::shared_ptr<Cell> cell,
                              const oblivion::ESMManager& esmMgr);

    // Place a single reference object
    bool placeReference(std::shared_ptr<Cell> cell,
                        const oblivion::ReferenceData& ref,
                        const oblivion::ESMManager& esmMgr);

    // ========================================================================
    // Object Type Resolution
    // ========================================================================

    // Resolve base object type from FormID
    enum class ObjectType {
        UNKNOWN = 0,
        STATIC,
        ACTIVATOR,
        CONTAINER,
        DOOR,
        LIGHT,
        WEAPON,
        ARMOR,
        CLOTHING,
        BOOK,
        INGREDIENT,
        ALCHEMY,
        APPARATUS,
        TREE,
        FLORA,
        NPC,
        CREATURE,
        AMMO,
        MISC_ITEM
    };

    // Get object type from base FormID
    ObjectType resolveObjectType(uint32_t baseFormID,
                                  const oblivion::ESMManager& esmMgr);

    // Get model path for a base object
    std::string resolveModelPath(uint32_t baseFormID,
                                  ObjectType type,
                                  const oblivion::ESMManager& esmMgr);

    // ========================================================================
    // Statistics
    // ========================================================================

    struct PlacementStats {
        uint32_t totalPlaced = 0;
        uint32_t staticObjects = 0;
        uint32_t dynamicObjects = 0;
        uint32_t interactables = 0;
        uint32_t skipped = 0;
    };

    const PlacementStats& getStats() const { return stats; }
    void resetStats();

private:
    // ========================================================================
    // Member Variables
    // ========================================================================

    AssetManager* assetManager;
    bool isInitialized;
    PlacementStats stats;

    // ========================================================================
    // Private Methods
    // ========================================================================

    // Create WorldObject from reference data
    std::shared_ptr<WorldObject> createWorldObject(
        const oblivion::ReferenceData& ref,
        ObjectType type,
        const std::string& modelPath,
        const std::string& objectName);

    // Determine if object type is static
    bool isStaticType(ObjectType type) const;

    // Determine if object type is interactable
    bool isInteractableType(ObjectType type) const;

    // Get display name for object type
    std::string getObjectTypeName(ObjectType type) const;
};
