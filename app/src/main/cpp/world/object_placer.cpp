#include "object_placer.h"
#include "../assets/asset_manager.h"

// ============================================================================
// ObjectPlacer Implementation
// ============================================================================

ObjectPlacer::ObjectPlacer()
    : assetManager(nullptr), isInitialized(false) {
    resetStats();
}

ObjectPlacer::~ObjectPlacer() {
    cleanup();
}

bool ObjectPlacer::initialize(AssetManager* assetMgr) {
    assetManager = assetMgr;
    isInitialized = true;
    resetStats();

    LOGI_OBJ("ObjectPlacer initialized");
    return true;
}

void ObjectPlacer::cleanup() {
    isInitialized = false;
    assetManager = nullptr;

    LOGI_OBJ("ObjectPlacer cleaned up");
}

// ============================================================================
// Object Placement
// ============================================================================

bool ObjectPlacer::placeObjectsForCell(std::shared_ptr<Cell> cell,
                                        const oblivion::ESMManager& esmMgr) {
    if (!cell) {
        LOGE_OBJ("Cannot place objects for null cell");
        return false;
    }

    LOGD_OBJ("Placing objects for cell %u (0x%08X)", cell->cellId, cell->tesFormID);

    const auto& refs = esmMgr.getAllReferences();
    size_t placed = 0;

    for (const auto& ref : refs) {
        // Only process references belonging to this cell
        if (ref.cellFormID != cell->tesFormID) continue;

        if (placeReference(cell, ref, esmMgr)) {
            ++placed;
        }
    }

    LOGI_OBJ("Cell %u: placed %zu objects from REFR data", cell->cellId, placed);
    return true;
}

bool ObjectPlacer::placeReference(std::shared_ptr<Cell> cell,
                                   const oblivion::ReferenceData& ref,
                                   const oblivion::ESMManager& esmMgr) {
    if (!cell) return false;

    // Resolve object type
    ObjectType type = resolveObjectType(ref.baseFormID, esmMgr);
    if (type == ObjectType::UNKNOWN) {
        LOGD_OBJ("Unknown object type for baseFormID 0x%08X, skipping", ref.baseFormID);
        stats.skipped++;
        return false;
    }

    // Resolve model path
    std::string modelPath = resolveModelPath(ref.baseFormID, type, esmMgr);

    // Get object type name
    std::string objectName = getObjectTypeName(type);

    // Create WorldObject
    auto obj = createWorldObject(ref, type, modelPath, objectName);
    if (!obj) {
        LOGE_OBJ("Failed to create WorldObject for ref 0x%08X", ref.formID);
        return false;
    }

    // Add to cell
    if (obj->isStatic) {
        cell->staticObjects.push_back(obj);
    } else {
        cell->dynamicObjects.push_back(obj);
    }

    // Update stats
    stats.totalPlaced++;
    if (obj->isStatic) {
        stats.staticObjects++;
    } else {
        stats.dynamicObjects++;
    }
    if (obj->isInteractable) {
        stats.interactables++;
    }

    return true;
}

// ============================================================================
// Object Type Resolution
// ============================================================================

ObjectPlacer::ObjectType ObjectPlacer::resolveObjectType(
    uint32_t baseFormID, const oblivion::ESMManager& esmMgr) {

    // Check each object type in order of likelihood
    if (esmMgr.findStatic(baseFormID)) return ObjectType::STATIC;
    if (esmMgr.findActivator(baseFormID)) return ObjectType::ACTIVATOR;
    if (esmMgr.findContainer(baseFormID)) return ObjectType::CONTAINER;
    if (esmMgr.findLight(baseFormID)) return ObjectType::LIGHT;
    if (esmMgr.findTree(baseFormID)) return ObjectType::TREE;
    if (esmMgr.findFlora(baseFormID)) return ObjectType::FLORA;
    if (esmMgr.findWeapon(baseFormID)) return ObjectType::WEAPON;
    if (esmMgr.findArmor(baseFormID)) return ObjectType::ARMOR;
    if (esmMgr.findClothing(baseFormID)) return ObjectType::CLOTHING;
    if (esmMgr.findBook(baseFormID)) return ObjectType::BOOK;
    if (esmMgr.findIngredient(baseFormID)) return ObjectType::INGREDIENT;
    if (esmMgr.findAlchemy(baseFormID)) return ObjectType::ALCHEMY;
    if (esmMgr.findApparatus(baseFormID)) return ObjectType::APPARATUS;
    if (esmMgr.findMiscItem(baseFormID)) return ObjectType::MISC_ITEM;
    if (esmMgr.findNPC(baseFormID)) return ObjectType::NPC;
    if (esmMgr.findCreature(baseFormID)) return ObjectType::CREATURE;

    return ObjectType::UNKNOWN;
}

std::string ObjectPlacer::resolveModelPath(uint32_t baseFormID,
                                             ObjectType type,
                                             const oblivion::ESMManager& esmMgr) {
    // Try to get model path from the appropriate data structure
    switch (type) {
        case ObjectType::STATIC: {
            auto* data = esmMgr.findStatic(baseFormID);
            return data ? data->modelPath : "";
        }
        case ObjectType::ACTIVATOR: {
            auto* data = esmMgr.findActivator(baseFormID);
            return data ? data->modelPath : "";
        }
        case ObjectType::CONTAINER: {
            auto* data = esmMgr.findContainer(baseFormID);
            return data ? data->modelPath : "";
        }
        case ObjectType::LIGHT: {
            auto* data = esmMgr.findLight(baseFormID);
            return data ? data->modelPath : "";
        }
        case ObjectType::TREE: {
            auto* data = esmMgr.findTree(baseFormID);
            return data ? data->modelPath : "";
        }
        case ObjectType::FLORA: {
            auto* data = esmMgr.findFlora(baseFormID);
            return data ? data->modelPath : "";
        }
        case ObjectType::WEAPON: {
            // Weapons don't have modelPath in WeaponData, use generic path
            return "items/weapon";
        }
        case ObjectType::ARMOR: {
            auto* data = esmMgr.findArmor(baseFormID);
            return data ? data->modelPath : "items/armor";
        }
        case ObjectType::CLOTHING: {
            auto* data = esmMgr.findClothing(baseFormID);
            return data ? data->modelPath : "items/clothing";
        }
        case ObjectType::BOOK: {
            auto* data = esmMgr.findBook(baseFormID);
            return data ? data->modelPath : "items/book";
        }
        case ObjectType::INGREDIENT: {
            auto* data = esmMgr.findIngredient(baseFormID);
            return data ? data->modelPath : "items/ingredient";
        }
        case ObjectType::ALCHEMY: {
            auto* data = esmMgr.findAlchemy(baseFormID);
            return data ? data->modelPath : "items/potion";
        }
        case ObjectType::APPARATUS: {
            auto* data = esmMgr.findApparatus(baseFormID);
            return data ? data->modelPath : "items/apparatus";
        }
        case ObjectType::MISC_ITEM: {
            auto* data = esmMgr.findMiscItem(baseFormID);
            return data ? data->modelPath : "items/misc";
        }
        case ObjectType::NPC:
        case ObjectType::CREATURE:
            return "characters/base";  // Generic character model
        default:
            return "";
    }
}

// ============================================================================
// Private Methods
// ============================================================================

std::shared_ptr<WorldObject> ObjectPlacer::createWorldObject(
    const oblivion::ReferenceData& ref,
    ObjectType type,
    const std::string& modelPath,
    const std::string& objectName) {

    auto obj = std::make_shared<WorldObject>();
    obj->objectId = ref.formID;
    obj->objectName = objectName;
    obj->modelPath = modelPath;
    obj->position = ref.position;
    obj->rotation = ref.rotation;
    obj->scale = ref.scale;
    obj->isStatic = isStaticType(type);
    obj->isInteractable = isInteractableType(type);

    return obj;
}

bool ObjectPlacer::isStaticType(ObjectType type) const {
    switch (type) {
        case ObjectType::STATIC:
        case ObjectType::ACTIVATOR:
        case ObjectType::CONTAINER:
        case ObjectType::LIGHT:
        case ObjectType::TREE:
        case ObjectType::FLORA:
            return true;
        default:
            return false;
    }
}

bool ObjectPlacer::isInteractableType(ObjectType type) const {
    switch (type) {
        case ObjectType::ACTIVATOR:
        case ObjectType::CONTAINER:
        case ObjectType::DOOR:
        case ObjectType::WEAPON:
        case ObjectType::ARMOR:
        case ObjectType::CLOTHING:
        case ObjectType::BOOK:
        case ObjectType::INGREDIENT:
        case ObjectType::ALCHEMY:
        case ObjectType::APPARATUS:
        case ObjectType::FLORA:
        case ObjectType::MISC_ITEM:
        case ObjectType::NPC:
        case ObjectType::CREATURE:
            return true;
        default:
            return false;
    }
}

std::string ObjectPlacer::getObjectTypeName(ObjectType type) const {
    switch (type) {
        case ObjectType::STATIC: return "Static";
        case ObjectType::ACTIVATOR: return "Activator";
        case ObjectType::CONTAINER: return "Container";
        case ObjectType::DOOR: return "Door";
        case ObjectType::LIGHT: return "Light";
        case ObjectType::WEAPON: return "Weapon";
        case ObjectType::ARMOR: return "Armor";
        case ObjectType::CLOTHING: return "Clothing";
        case ObjectType::BOOK: return "Book";
        case ObjectType::INGREDIENT: return "Ingredient";
        case ObjectType::ALCHEMY: return "Alchemy";
        case ObjectType::APPARATUS: return "Apparatus";
        case ObjectType::TREE: return "Tree";
        case ObjectType::FLORA: return "Flora";
        case ObjectType::NPC: return "NPC";
        case ObjectType::CREATURE: return "Creature";
        case ObjectType::AMMO: return "Ammo";
        case ObjectType::MISC_ITEM: return "MiscItem";
        default: return "Unknown";
    }
}

void ObjectPlacer::resetStats() {
    stats = PlacementStats();
}
