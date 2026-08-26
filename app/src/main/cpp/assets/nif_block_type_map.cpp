#include "nif_block_type_map.h"

const std::unordered_map<std::string, NIFBlockType> NIFBlockTypeMap::stringToType = {
    // Core scene graph
    {"NiNode", NIFBlockType::NiNode},
    {"NiTriShape", NIFBlockType::NiTriShape},
    {"NiTriStrips", NIFBlockType::NiTriStrips},
    {"NiTexturingProperty", NIFBlockType::NiTexturingProperty},
    {"NiMaterialProperty", NIFBlockType::NiMaterialProperty},
    {"NiAlphaProperty", NIFBlockType::NiAlphaProperty},
    {"NiVertexColorProperty", NIFBlockType::NiVertexColorProperty},
    {"NiZBufferProperty", NIFBlockType::NiZBufferProperty},
    {"NiProperty", NIFBlockType::NiProperty},
    // Skinning
    {"NiSkinInstance", NIFBlockType::NiSkinInstance},
    {"NiSkinData", NIFBlockType::NiSkinData},
    {"NiSkinPartition", NIFBlockType::NiSkinPartition},
    // Animation
    {"NiControllerManager", NIFBlockType::NiControllerManager},
    {"NiControllerSequence", NIFBlockType::NiControllerSequence},
    {"NiKeyframeController", NIFBlockType::NiKeyframeController},
    {"NiKeyframeData", NIFBlockType::NiKeyframeData},
    {"NiTextKeyExtraData", NIFBlockType::NiTextKeyExtraData},
    {"NiTransformController", NIFBlockType::NiTransformController},
    // Physics (Havok)
    {"bhkCollisionObject", NIFBlockType::bhkCollisionObject},
    {"bhkRigidBody", NIFBlockType::bhkRigidBody},
    {"bhkRigidBodyCInfo", NIFBlockType::bhkRigidBodyCInfo},
    {"bhkShape", NIFBlockType::bhkShape},
    {"bhkBoxShape", NIFBlockType::bhkBoxShape},
    {"bhkSphereShape", NIFBlockType::bhkSphereShape},
    {"bhkCapsuleShape", NIFBlockType::bhkCapsuleShape},
    {"bhkConvexVerticesShape", NIFBlockType::bhkConvexVerticesShape},
    {"bhkMeshShape", NIFBlockType::bhkMeshShape},
    {"bhkPackedNiTriStripsShape", NIFBlockType::bhkPackedNiTriStripsShape},
    {"bhkMoppBvTreeShape", NIFBlockType::bhkMoppBvTreeShape},
    {"bhkCollisionFilter", NIFBlockType::bhkCollisionFilter},
};

const std::array<const char*, static_cast<size_t>(NIFBlockType::NumTypes)> NIFBlockTypeMap::typeToString = {
    "Unknown",
    // Core
    "NiNode", "NiTriShape", "NiTriStrips",
    "NiTexturingProperty", "NiMaterialProperty", "NiAlphaProperty",
    "NiVertexColorProperty", "NiZBufferProperty", "NiProperty",
    // Skinning
    "NiSkinInstance", "NiSkinData", "NiSkinPartition",
    // Animation
    "NiControllerManager", "NiControllerSequence", "NiKeyframeController",
    "NiKeyframeData", "NiTextKeyExtraData", "NiTransformController",
    // Physics
    "bhkCollisionObject", "bhkRigidBody", "bhkRigidBodyCInfo", "bhkShape",
    "bhkBoxShape", "bhkSphereShape", "bhkCapsuleShape", "bhkConvexVerticesShape",
    "bhkMeshShape", "bhkPackedNiTriStripsShape", "bhkMoppBvTreeShape", "bhkCollisionFilter"
};

NIFBlockType NIFBlockTypeMap::fromString(const std::string& name) {
    auto it = stringToType.find(name);
    return (it != stringToType.end()) ? it->second : NIFBlockType::Unknown;
}

const char* NIFBlockTypeMap::toString(NIFBlockType type) {
    size_t idx = static_cast<size_t>(type);
    return (idx < typeToString.size()) ? typeToString[idx] : "Unknown";
}
