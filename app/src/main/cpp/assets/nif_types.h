#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <glm/glm.hpp>

// NIF file header constants
#define NIF_MAGIC "Gamebryo File Format, Version"
#define NIF_VERSION_OBLIVION "20.0.0.5"
#define NIF_VERSION_SKYRIM "20.2.0.7"

// NIF block type IDs (string-based, mapped via NIFBlockTypeMap)
enum class NIFBlockType : uint32_t {
    Unknown = 0,
    // Core scene graph
    NiNode,
    NiTriShape,
    NiTriStrips,
    NiTexturingProperty,
    NiMaterialProperty,
    NiAlphaProperty,
    NiVertexColorProperty,
    NiZBufferProperty,
    NiProperty,
    // Skinning
    NiSkinInstance,
    NiSkinData,
    NiSkinPartition,
    // Animation
    NiControllerManager,
    NiControllerSequence,
    NiKeyframeController,
    NiKeyframeData,
    NiTextKeyExtraData,
    NiTransformController,
    // Physics (Havok)
    bhkCollisionObject,
    bhkRigidBody,
    bhkRigidBodyCInfo,
    bhkShape,
    bhkBoxShape,
    bhkSphereShape,
    bhkCapsuleShape,
    bhkConvexVerticesShape,
    bhkMeshShape,
    bhkPackedNiTriStripsShape,
    bhkMoppBvTreeShape,
    bhkCollisionFilter,
    NumTypes
};

// NIF structures
//
// Header layout verified against 322 meshes sampled from Oblivion - Meshes.bsa,
// covering every family present in the archive:
//
//   Gamebryo File Format, Version 20.0.0.4 / 20.0.0.5   (u8 endian byte = 1)
//   Gamebryo File Format, Version 10.2.0.0 / 10.1.0.106 (no endian byte)
//   NetImmerse File Format, Version 10.0.1.0            (compact legacy header)
//
// Gamebryo layout:
//
//   magic line            "Gamebryo File Format, Version X.X.X.X\n" (39..41 bytes)
//   u32  version          e.g. 0x0A01006A = 10.1.0.106, 0x14000004 = 20.0.0.4
//   u8   endian           present only when version >= 0x14000000 (always 1)
//   u32  userVersion      10 or 11
//   u32  numObjects       block count of the NiObject array
//   u32  unknownField     unidentified; 11 for 20.0.0.4, 5 for 10.1.0.106, 6 for 10.2.0.0
//   bzstring creator      "ccafasso" / "cmeister" / empty
//   bzstring processScript
//   bzstring exportScript
//   u16  numBlockTypes
//   (u32 len + chars) x numBlockTypes   block type name table
//   u16  blockTypeIndices[numObjects]   one type index per block, all < numBlockTypes
//   u32  numStrings       0 in every sampled mesh
//   u32  maxStringLength  width of the fixed-width inline block names, 0 when unused
//   <block 0 body>        begins with the inline object name, see below
//
// The inline object name of block 0 (and of every NiObjectNET-derived block) is
// encoded in one of two ways, decided by maxStringLength:
//
//   maxStringLength > 0   fixed width: exactly maxStringLength raw bytes
//   maxStringLength == 0  SizedString: u32 length followed by the bytes
//
// Gamebryo 20.0.0.4 meshes use the fixed-width form, while Gamebryo 10.1.0.106
// and legacy NetImmerse 10.0.1.0 meshes set maxStringLength to 0 and use the
// SizedString form. Both forms were verified against retail Oblivion meshes.
//
// NetImmerse legacy layout differs only in the preamble:
//
//   magic line            "NetImmerse File Format, Version 10.0.1.0\n"
//   u32  version          0x0A000100
//   u32  numObjects       block count, read straight after the version
//   u16  numBlockTypes    then the same type table, index array and string tail
struct NIFHeader {
    char magic[256] = {};      // "Gamebryo File Format, Version X.X.X.X"
    uint32_t version = 0;      // Version number
    uint8_t endian = 0;        // Endian byte, valid only when hasEndianByte is true
    bool hasEndianByte = false;// True for the 20.0.0.x family and later
    bool legacyNetImmerse = false; // True for the compact NetImmerse 10.0.1.0 header
    uint32_t userVersion = 0;  // User version
    uint32_t numObjects = 0;   // Number of blocks in the file
    uint32_t unknownField = 0; // Unidentified u32 between numObjects and the creator strings
    uint32_t numStrings = 0;   // Number of strings in the header string table
    uint32_t maxStringLength = 0; // Width of fixed-width inline block names, 0 = length-prefixed
    std::string creator;       // Creator string
    std::string processScript; // Process script string
    std::string exportScript;  // Export script string
    std::vector<std::string> blockTypeNames;  // One entry per distinct block type
    std::vector<uint16_t> blockTypeIndices;   // One entry per block
    size_t blockDataOffset = 0; // File offset at which block 0 begins
};

struct NIFVector3 {
    float x, y, z;
    
    NIFVector3() : x(0), y(0), z(0) {}
    NIFVector3(float vx, float vy, float vz) : x(vx), y(vy), z(vz) {}
    
    glm::vec3 toGLM() const {
        return glm::vec3(x, y, z);
    }
};

struct NIFVector4 {
    float x, y, z, w;
    
    NIFVector4() : x(0), y(0), z(0), w(1) {}
    NIFVector4(float vx, float vy, float vz, float vw) : x(vx), y(vy), z(vz), w(vw) {}
};

struct NIFMatrix3x3 {
    float m[3][3];
    
    NIFMatrix3x3() {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                m[i][j] = (i == j) ? 1.0f : 0.0f;
            }
        }
    }
};

// NIF Block Header (for each block in the file)
struct NIFBlockHeader {
    std::string blockType;     // Type name (e.g., "NiNode", "NiTriShape")
    uint32_t blockSize;        // Size of the block data
    uint32_t blockIndex;       // Index in the object array
};

// NIF Transformation Matrix
struct NIFTransform {
    NIFMatrix3x3 rotation;
    NIFVector3 translation;
    float scale;
    
    NIFTransform() : scale(1.0f) {}
    
    glm::mat4 toGLMMatrix4() const {
        // Create identity matrix by translating at origin
        glm::mat4 result = glm::translate(glm::mat4(), glm::vec3(0.0f, 0.0f, 0.0f));

        // Set rotation (3x3 upper-left, with scale)
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                result[i][j] = rotation.m[i][j] * scale;
            }
        }

        // Set translation (last row, first 3 columns)
        result[3][0] = translation.x;
        result[3][1] = translation.y;
        result[3][2] = translation.z;

        return result;
    }
    
    NIFMatrix3x3 m;
};

// NIF Triangle Data
struct NIFTriangle {
    uint16_t v0, v1, v2;  // Vertex indices
};

// NIF Geometry Data (used by NiTriShape and NiTriStrips)
struct NIFGeometry {
    std::string name;
    NIFTransform transform;
    std::vector<NIFVector3> vertices;
    std::vector<NIFVector3> normals;
    std::vector<NIFVector4> colors;
    std::vector<glm::vec2> texCoords;
    std::vector<NIFTriangle> triangles;
    
    // Material/Texture references
    std::string diffuseTexture;
    std::string normalTexture;
    uint32_t materialPropertyIndex;
    uint32_t texturingPropertyIndex;
};

// NIF Node (base structure)
struct NIFNode {
    std::string name;
    uint32_t nodeIndex = 0;
    int32_t parentIndex = -1;               // -1 if root
    std::vector<int32_t> childIndices;
    uint16_t blockTypeIndex = 0;            // Index into NIFHeader::blockTypeNames
    std::string blockTypeName;              // Resolved block type, e.g. "NiNode"

    // Node-specific data
    NIFTransform transform;
    bool hasGeometry = false;
    NIFGeometry geometry;
};

// ============================================
// Phase 30: Collision (bhkCollisionObject)
// ============================================

enum class CollisionShapeType : uint32_t {
    None = 0,
    Box,
    Sphere,
    Capsule,
    ConvexHull,
    TriMesh,
    MoppBvTree
};

struct CollisionShape {
    CollisionShapeType type = CollisionShapeType::None;
    NIFVector3 center;
    NIFVector3 halfExtents;
    float radius = 0.0f;
    float height = 0.0f;
    std::vector<NIFVector3> vertices;
    std::vector<NIFTriangle> triangles;
    std::vector<uint8_t> moppData;
    uint32_t moppDataSize = 0;
};

struct RigidBodyInfo {
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.3f;
    NIFVector3 linearVelocity;
    NIFVector3 angularVelocity;
    NIFTransform transform;
    uint32_t collisionGroup = 0;
    uint32_t collisionFilter = 0;
    bool isTrigger = false;
};

struct CollisionObject {
    uint32_t nodeIndex = 0;
    uint32_t rigidBodyIndex = 0;
    CollisionShape shape;
    RigidBodyInfo bodyInfo;
    std::string targetName;
};

// ============================================
// Phase 30: Skinning (NiSkinInstance/Partition)
// ============================================

struct NIFVertexWeight {
    uint16_t vertexIndex = 0;
    float weight = 0.0f;
};

struct NIFBoneData {
    NIFTransform skinTransform;
    std::vector<NIFVertexWeight> vertexWeights;
};

struct NIFSkinData {
    std::string name;
    uint32_t numBones = 0;
    std::vector<NIFBoneData> boneData;
    NIFMatrix3x3 rootRotation;
    NIFVector3 rootTranslation;
    float rootScale = 1.0f;
};

// GPU packed data (4 bones per vertex)
struct GPUVertexWeight {
    uint16_t boneIndices[4] = {0, 0, 0, 0};
    float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct GPUBonePalette {
    std::vector<uint16_t> bones;
};

struct NIFSkinPartition {
    struct Partition {
        GPUBonePalette bonePalette;
        std::vector<GPUVertexWeight> packedWeights;
        std::vector<uint16_t> indices;
        uint32_t numVertices = 0;
        uint32_t numTriangles = 0;
        std::vector<NIFTriangle> triangles;
    };
    std::vector<Partition> partitions;
    uint32_t maxBonesPerPartition = 4;
    uint32_t maxBonesPerVertex = 4;
};

struct NIFSkinInstance {
    uint32_t skinDataIndex = 0;
    uint32_t skinPartitionIndex = 0;
    uint32_t skeletonRootIndex = 0;
    std::string name;
};

struct NIFBone {
    std::string name;
    int32_t boneIndex = 0;
    int32_t parentBoneIndex = -1;
    NIFTransform localTransform;
    glm::mat4 inverseBindMatrix;  // default = identity
};

struct BoneState {
    glm::mat4 localTransform;     // default = identity
    glm::mat4 worldTransform;     // default = identity
    glm::mat4 skinningMatrix;     // default = identity
};

// ============================================
// Phase 30: Animation (NiControllerManager/Sequence)
// ============================================

enum class InterpolationType : uint32_t {
    Constant = 0,
    Linear = 1,
    Quadratic = 2,
    TBC = 3,
    XYZ = 4
};

struct NIFKeyframe {
    float time = 0.0f;
    NIFVector4 rotation;
    NIFVector3 translation;
    float scale = 1.0f;
    NIFVector4 forwardTangent;
    NIFVector4 backwardTangent;
};

struct NIFTextKey {
    float time = 0.0f;
    std::string value;
};

struct NIFAnimationClip {
    std::string name;
    float duration = 0.0f;
    float frequency = 1.0f;
    std::vector<NIFKeyframe> keyframes;
};

// NiKeyframeData block (resolved from NIF file)
struct NIFKeyframeData {
    uint32_t index = 0;
    NIFAnimationClip clip;
};

struct NIFKeyframeController {
    uint32_t targetNodeIndex = 0;
    uint32_t keyframeDataIndex = 0;
    NIFAnimationClip clip;
    // Phase 30: resolved bone index for fast lookup
    int32_t resolvedBoneIndex = -1;
};

struct NIFControllerSequence {
    std::string name;
    uint32_t controllerManagerIndex = 0;
    std::string targetName;
    float duration = 0.0f;
    float frequency = 1.0f;
    float phase = 0.0f;
    float startTime = 0.0f;
    float stopTime = 0.0f;
    bool loop = false;
    std::vector<NIFKeyframeController> controlledBlocks;
    std::vector<NIFTextKey> textKeys;
};

struct NIFControllerManager {
    std::string name;
    uint32_t controllerSequenceCount = 0;
    std::vector<NIFControllerSequence> sequences;
    uint32_t objectPaletteIndex = 0;
    float lastTime = 0.0f;
};

struct NIFTransformController {
    uint32_t targetNodeIndex = 0;
    uint32_t interpolatorIndex = 0;
};
