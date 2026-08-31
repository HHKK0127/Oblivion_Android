#include "nif_parser.h"
#include <android/log.h>
#include <algorithm>
#include <cstring>
#include <cmath>

#undef LOG_TAG
#undef LOGD
#undef LOGE
#define LOG_TAG "NIFParser"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

NIFParser::NIFParser() {
    memset(&header, 0, sizeof(NIFHeader));
}

NIFParser::~NIFParser() {
    if (fileStream.is_open()) {
        fileStream.close();
    }
}

bool NIFParser::parseFile(const std::string& filepath) {
    LOGD("=== Starting NIF parse: %s ===", filepath.c_str());

    // Open file
    fileStream.open(filepath, std::ios::binary);
    if (!fileStream.is_open()) {
        LOGE("Failed to open NIF file: %s", filepath.c_str());
        return false;
    }

    // Read header
    if (!readHeader()) {
        LOGE("Failed to read NIF header");
        fileStream.close();
        return false;
    }

    LOGD("NIF Header: version=%u, userVersion=%u, numObjects=%u",
         header.version, header.userVersion, header.numObjects);

    // Parse block type strings
    if (!parseBlockTypeStrings()) {
        LOGE("Failed to parse block type strings");
        fileStream.close();
        return false;
    }

    // Parse object array
    if (!parseObjectArray()) {
        LOGE("Failed to parse object array");
        fileStream.close();
        return false;
    }

    // Build node hierarchy
    if (!buildNodeHierarchy()) {
        LOGE("Failed to build node hierarchy");
        fileStream.close();
        return false;
    }

    fileStream.close();
    LOGD("=== NIF parse complete: %zu nodes found ===", nodes.size());
    return true;
}

bool NIFParser::readHeader() {
    LOGD("Reading NIF header...");

    // Read magic string until null terminator or newline (NIF headers are variable length)
    char magic[256];
    std::memset(magic, 0, sizeof(magic));
    size_t pos = 0;
    while (pos < sizeof(magic) - 1) {
        if (!readBytes(&magic[pos], 1)) {
            LOGE("Failed to read magic number");
            return false;
        }
        if (magic[pos] == '\n' || magic[pos] == '\0') {
            break;
        }
        pos++;
    }
    magic[pos] = '\0';

    strncpy(header.magic, magic, sizeof(header.magic) - 1);
    header.magic[sizeof(header.magic) - 1] = '\0';

    // Verify magic
    if (strstr(header.magic, "Gamebryo File Format") == nullptr) {
        LOGE("Invalid NIF magic: %s", header.magic);
        return false;
    }

    LOGD("NIF Magic: %s", header.magic);

    // Read version info
    header.version = readUInt32();
    header.userVersion = readUInt32();
    header.userVersion2 = readUInt32();
    header.numObjects = readUInt32();
    header.numStrings = readUInt32();
    header.maxStringLength = readUInt32();

    LOGD("Num objects: %u, Num strings: %u", header.numObjects, header.numStrings);

    return true;
}

bool NIFParser::parseBlockTypeStrings() {
    LOGD("Parsing block type strings: %u strings", header.numStrings);

    // String table comes after header
    // For now, we'll skip detailed parsing of string table
    // In a full implementation, we'd read all strings into a vector

    return true;
}

bool NIFParser::parseObjectArray() {
    LOGD("Parsing object array: %u objects", header.numObjects);

    nodes.resize(header.numObjects);

    for (uint32_t i = 0; i < header.numObjects; i++) {
        auto node = std::make_shared<NIFNode>();
        node->nodeIndex = i;
        node->parentIndex = -1;
        node->hasGeometry = false;

        // Read block type string
        readString(node->name);

        LOGD("Object %u: %s", i, node->name.c_str());

        // For now, just store basic node info
        // Full parsing of each block type would happen here
        nodes[i] = node;
    }

    return true;
}

bool NIFParser::buildNodeHierarchy() {
    LOGD("Building node hierarchy...");

    // Find root nodes (those without parents)
    for (const auto& node : nodes) {
        if (node && node->parentIndex == -1) {
            rootNodeIndices.push_back(node->nodeIndex);
            LOGD("Root node found: %s (index %u)", node->name.c_str(), node->nodeIndex);
        }
    }

    return true;
}

// Binary reading helpers
bool NIFParser::readBytes(char* buffer, size_t count) {
    fileStream.read(buffer, count);
    return fileStream.gcount() == (std::streamsize)count;
}

bool NIFParser::readString(std::string& str) {
    uint32_t length = readUInt32();
    if (length == 0) {
        str.clear();
        return true;  // Empty string is valid
    }
    if (length > 1024 * 1024) {  // 1MB upper limit to reject corrupt values
        LOGE("NIF string length too large: %u", length);
        return false;
    }
    std::vector<char> buffer(length);
    if (!readBytes(buffer.data(), length)) {
        return false;
    }
    str.assign(buffer.data(), length);
    return true;
}

bool NIFParser::readStringRef(std::string& str) {
    uint32_t index = readUInt32();
    // In full implementation, would look up from string table
    // For now, just set a placeholder
    str = "string_" + std::to_string(index);
    return true;
}

uint32_t NIFParser::readUInt32() {
    uint32_t value;
    fileStream.read(reinterpret_cast<char*>(&value), sizeof(uint32_t));
    return value;
}

uint16_t NIFParser::readUInt16() {
    uint16_t value;
    fileStream.read(reinterpret_cast<char*>(&value), sizeof(uint16_t));
    return value;
}

float NIFParser::readFloat() {
    float value;
    fileStream.read(reinterpret_cast<char*>(&value), sizeof(float));
    return value;
}

NIFVector3 NIFParser::readVector3() {
    float x = readFloat();
    float y = readFloat();
    float z = readFloat();
    return NIFVector3(x, y, z);
}

NIFVector4 NIFParser::readVector4() {
    float x = readFloat();
    float y = readFloat();
    float z = readFloat();
    float w = readFloat();
    return NIFVector4(x, y, z, w);
}

NIFMatrix3x3 NIFParser::readMatrix3x3() {
    NIFMatrix3x3 matrix;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            matrix.m[i][j] = readFloat();
        }
    }
    return matrix;
}

NIFTransform NIFParser::readTransform() {
    NIFTransform transform;
    transform.rotation = readMatrix3x3();
    transform.translation = readVector3();
    transform.scale = readFloat();
    return transform;
}

NIFBlockType NIFParser::getBlockType(const std::string& blockName) {
    return NIFBlockTypeMap::fromString(blockName);
}

bool NIFParser::parseNiNode(std::shared_ptr<NIFNode>& node) {
    LOGD("Parsing NiNode: %s", node->name.c_str());
    node->transform = readTransform();
    return true;
}

bool NIFParser::parseNiTriShape(std::shared_ptr<NIFNode>& node) {
    LOGD("Parsing NiTriShape: %s", node->name.c_str());
    node->hasGeometry = true;
    node->geometry.name = node->name;
    node->geometry.transform = readTransform();
    return true;
}

bool NIFParser::parseNiTriStrips(std::shared_ptr<NIFNode>& node) {
    LOGD("Parsing NiTriStrips: %s", node->name.c_str());
    node->hasGeometry = true;
    node->geometry.name = node->name;
    node->geometry.transform = readTransform();
    return true;
}

bool NIFParser::parseMaterialProperty() {
    LOGD("Parsing NiMaterialProperty");
    return true;
}

bool NIFParser::parseTexturingProperty() {
    LOGD("Parsing NiTexturingProperty");
    return true;
}

std::shared_ptr<NIFNode> NIFParser::getNodeByName(const std::string& name) const {
    for (const auto& node : nodes) {
        if (node && node->name == name) {
            return node;
        }
    }
    return nullptr;
}

std::vector<NIFGeometry> NIFParser::extractAllGeometry() const {
    std::vector<NIFGeometry> geometries;
    for (const auto& node : nodes) {
        if (node && node->hasGeometry) {
            geometries.push_back(node->geometry);
        }
    }
    return geometries;
}

// ============================================
// Phase 30 Step 2: Skinning Parsing
// ============================================

bool NIFParser::parseNiSkinInstance(NIFSkinInstance& skin) {
    LOGD("Parsing NiSkinInstance...");

    // NiSkinInstance layout:
    // uint32_t skeletonRootIndex
    // uint32_t numBones
    // uint32_t[numBones] boneNodeIndices
    // uint32_t skinPartitionIndex
    // uint32_t skinDataIndex

    skin.skeletonRootIndex = readUInt32();
    uint32_t numBones = readUInt32();

    // Read bone node indices (we don't store them directly, but need to skip)
    for (uint32_t i = 0; i < numBones; i++) {
        readUInt32();  // bone node index
    }

    skin.skinPartitionIndex = readUInt32();
    skin.skinDataIndex = readUInt32();

    LOGD("NiSkinInstance: skeletonRoot=%u, numBones=%u, skinPartition=%u, skinData=%u",
         skin.skeletonRootIndex, numBones, skin.skinPartitionIndex, skin.skinDataIndex);

    return true;
}

bool NIFParser::parseNiSkinData(NIFSkinData& skinData) {
    LOGD("Parsing NiSkinData...");

    // NiSkinData layout:
    // NiTransform skinTransform (root transform)
    // uint32_t numBones
    // BoneData[numBones]:
    //   NiTransform skinTransform
    //   NIFVector3 boundingSphereOffset
    //   float boundingSphereRadius
    //   uint16_t numWeights
    //   VertexWeight[numWeights]: { uint16_t index, float weight }

    skinData.rootRotation = readMatrix3x3();
    skinData.rootTranslation = readVector3();
    skinData.rootScale = readFloat();

    // Re-read as a transform
    NIFTransform rootTransform;
    rootTransform.rotation = skinData.rootRotation;
    rootTransform.translation = skinData.rootTranslation;
    rootTransform.scale = skinData.rootScale;

    skinData.numBones = readUInt32();
    skinData.boneData.resize(skinData.numBones);

    for (uint32_t i = 0; i < skinData.numBones; i++) {
        NIFBoneData& bd = skinData.boneData[i];

        // Bone transform
        bd.skinTransform.rotation = readMatrix3x3();
        bd.skinTransform.translation = readVector3();
        bd.skinTransform.scale = readFloat();

        // Bounding sphere
        readVector3();  // offset
        readFloat();    // radius

        // Vertex weights
        uint16_t numWeights = readUInt16();
        bd.vertexWeights.resize(numWeights);
        for (uint16_t w = 0; w < numWeights; w++) {
            bd.vertexWeights[w].vertexIndex = readUInt16();
            bd.vertexWeights[w].weight = readFloat();
        }
    }

    LOGD("NiSkinData: numBones=%u", skinData.numBones);
    return true;
}

bool NIFParser::parseNiSkinPartition(NIFSkinPartition& partition) {
    LOGD("Parsing NiSkinPartition...");

    // NiSkinPartition layout:
    // uint32_t numPartitions
    // Partition[numPartitions]:
    //   uint16_t numVertices
    //   uint16_t numTriangles
    //   uint16_t numBones
    //   uint16_t numStrips
    //   uint16_t numWeightsPerVertex
    //   uint16_t[numBones] bones
    //   bool hasVertexMap
    //   uint16_t[numVertices] vertexMap (if hasVertexMap)
    //   bool hasVertexWeights
    //   float[numVertices * numWeightsPerVertex] vertexWeights (if hasVertexWeights)
    //   uint16_t[numStrips] stripLengths (if numStrips > 0)
    //   bool hasBoneIndices
    //   uint8_t[numVertices * numWeightsPerVertex] boneIndices (if hasBoneIndices)
    //   uint16_t[numTriangles * 3 or sum(stripLengths)] triangles

    uint32_t numPartitions = readUInt32();
    constexpr uint32_t MAX_PARTITIONS = 10000;
    if (numPartitions > MAX_PARTITIONS) {
        LOGE("NiSkinPartition: numPartitions %u exceeds limit", numPartitions);
        return false;
    }
    partition.partitions.resize(numPartitions);

    for (uint32_t p = 0; p < numPartitions; p++) {
        auto& part = partition.partitions[p];

        uint16_t numVertices = readUInt16();
        uint16_t numTriangles = readUInt16();
        uint16_t numBones = readUInt16();
        uint16_t numStrips = readUInt16();
        uint16_t numWeightsPerVertex = readUInt16();

        part.numVertices = numVertices;
        part.numTriangles = numTriangles;

        // Bone palette
        part.bonePalette.bones.resize(numBones);
        for (uint16_t b = 0; b < numBones; b++) {
            part.bonePalette.bones[b] = readUInt16();
        }

        // Vertex map
        bool hasVertexMap = (readUInt32() != 0);
        std::vector<uint16_t> vertexMap;
        if (hasVertexMap) {
            vertexMap.resize(numVertices);
            for (uint16_t v = 0; v < numVertices; v++) {
                vertexMap[v] = readUInt16();
            }
        }

        // Vertex weights
        bool hasVertexWeights = (readUInt32() != 0);
        if (hasVertexWeights) {
            part.packedWeights.resize(numVertices);
            for (uint16_t v = 0; v < numVertices; v++) {
                for (uint16_t w = 0; w < numWeightsPerVertex && w < 4; w++) {
                    part.packedWeights[v].weights[w] = readFloat();
                }
            }
        }

        // Strip lengths
        std::vector<uint16_t> stripLengths;
        if (numStrips > 0) {
            stripLengths.resize(numStrips);
            for (uint16_t s = 0; s < numStrips; s++) {
                stripLengths[s] = readUInt16();
            }
        }

        // Bone indices per vertex
        bool hasBoneIndices = (readUInt32() != 0);
        if (hasBoneIndices) {
            for (uint16_t v = 0; v < numVertices; v++) {
                for (uint16_t w = 0; w < numWeightsPerVertex && w < 4; w++) {
                    uint8_t idx = static_cast<uint8_t>(readUInt16() & 0xFF);
                    if (v < part.packedWeights.size()) {
                        part.packedWeights[v].boneIndices[w] = idx;
                    }
                }
            }
        }

        // Triangles
        uint32_t totalIndices;
        if (numStrips > 0) {
            totalIndices = 0;
            for (uint16_t s = 0; s < numStrips; s++) {
                totalIndices += stripLengths[s];
            }
        } else {
            totalIndices = numTriangles * 3;
        }

        part.indices.resize(totalIndices);
        for (uint32_t i = 0; i < totalIndices; i++) {
            part.indices[i] = readUInt16();
        }
    }

    LOGD("NiSkinPartition: numPartitions=%u", numPartitions);
    return true;
}

// ============================================
// Phase 30 Step 7: Animation Parsing
// ============================================

bool NIFParser::parseNiControllerManager(NIFControllerManager& manager) {
    LOGD("Parsing NiControllerManager...");

    // NiTimeController (parent of NiControllerManager)
    // uint32_t nextControllerIndex (usually UINT32_MAX)
    uint32_t nextController = readUInt32();
    // uint16_t flags
    uint16_t flags = readUInt16();
    // float frequency
    manager.lastTime = readFloat();
    // float phase
    float phase = readFloat();

    // NiControllerManager specific
    // uint32_t objectPaletteIndex
    manager.objectPaletteIndex = readUInt32();
    // uint32_t controllerSequenceCount
    manager.controllerSequenceCount = readUInt32();

    LOGD("  ControllerManager: %u sequences, palette=%u",
         manager.controllerSequenceCount, manager.objectPaletteIndex);

    // Read controller sequence indices (block references)
    std::vector<uint32_t> sequenceIndices(manager.controllerSequenceCount);
    for (uint32_t i = 0; i < manager.controllerSequenceCount; i++) {
        sequenceIndices[i] = readUInt32();
    }

    // Store indices for later resolution
    // Actual sequence data will be parsed when we encounter NiControllerSequence blocks
    manager.sequences.resize(manager.controllerSequenceCount);

    // Store block references for post-parse resolution
    for (uint32_t i = 0; i < manager.controllerSequenceCount; i++) {
        // We'll resolve these in resolveControllerReferences
        // For now, store the block index as metadata
        LOGD("  Sequence[%u] -> block %u", i, sequenceIndices[i]);
    }

    return true;
}

bool NIFParser::parseNiControllerSequence(NIFControllerSequence& sequence) {
    LOGD("Parsing NiControllerSequence...");

    // NiObject base (skip)
    // readString for name
    readStringRef(sequence.name);

    // uint32_t controllerManagerIndex (block ref)
    sequence.controllerManagerIndex = readUInt32();

    // readString for target name
    readStringRef(sequence.targetName);

    // float startTime, stopTime
    sequence.startTime = readFloat();
    sequence.stopTime = readFloat();

    // float phase
    sequence.phase = readFloat();

    // float frequency
    sequence.frequency = readFloat();

    // uint32_t cycleType (0=loop, 1=reverse, 2=clamp)
    uint32_t cycleType = readUInt32();
    sequence.loop = (cycleType == 0);

    // uint32_t textKeyCount
    uint32_t textKeyCount = readUInt32();
    constexpr uint32_t MAX_TEXT_KEYS = 100000;
    if (textKeyCount > MAX_TEXT_KEYS) {
        LOGE("NiControllerSequence: textKeyCount %u exceeds limit", textKeyCount);
        return false;
    }
    sequence.textKeys.resize(textKeyCount);

    // Read text keys
    for (uint32_t i = 0; i < textKeyCount; i++) {
        sequence.textKeys[i].time = readFloat();
        readStringRef(sequence.textKeys[i].value);
    }

    // uint32_t controlledBlockCount
    uint32_t blockCount = readUInt32();
    constexpr uint32_t MAX_CONTROLLED_BLOCKS = 10000;
    if (blockCount > MAX_CONTROLLED_BLOCKS) {
        LOGE("NiControllerSequence: blockCount %u exceeds limit", blockCount);
        return false;
    }
    sequence.controlledBlocks.resize(blockCount);

    // Read controlled blocks
    for (uint32_t i = 0; i < blockCount; i++) {
        auto& cb = sequence.controlledBlocks[i];
        // uint32_t nodeNameOffset (string table index)
        cb.targetNodeIndex = readUInt32();
        // uint32_t controllerType (string ref)
        uint32_t controllerTypeStr = readUInt32();
        // uint32_t controllerIndex (block ref)
        cb.keyframeDataIndex = readUInt32();
        // uint32_t nodeName (string ref)
        uint32_t nodeNameStr = readUInt32();
        // uint32_t propertyType (string ref)
        uint32_t propertyTypeStr = readUInt32();
        // uint32_t controllerType (string ref)
        uint32_t controllerType2 = readUInt32();
        // uint32_t controllerId (block ref)
        uint32_t controllerId = readUInt32();
        // uint32_t interpolator (block ref)
        uint32_t interpolator = readUInt32();

        cb.resolvedBoneIndex = -1;  // Will be resolved later
    }

    LOGD("  Sequence '%s': %.2f-%.2f, %u textKeys, %u blocks",
         sequence.name.c_str(), sequence.startTime, sequence.stopTime,
         textKeyCount, blockCount);

    return true;
}

bool NIFParser::parseNiKeyframeController(NIFKeyframeController& controller) {
    LOGD("Parsing NiControllerManager...");

    // NiTimeController base
    // uint32_t nextControllerIndex
    uint32_t nextController = readUInt32();
    // uint16_t flags
    uint16_t flags = readUInt16();
    // float frequency
    float frequency = readFloat();
    // float phase
    float phase = readFloat();

    // NiKeyframeController specific
    // uint32_t keyframeDataIndex (block ref)
    controller.keyframeDataIndex = readUInt32();

    // uint32_t targetNodeIndex (block ref)
    controller.targetNodeIndex = readUInt32();

    LOGD("  KeyframeController: target=%u, data=%u",
         controller.targetNodeIndex, controller.keyframeDataIndex);

    return true;
}

bool NIFParser::parseNiKeyframeData(NIFAnimationClip& clip) {
    LOGD("Parsing NiKeyframeData...");

    // uint32_t numRotationKeys
    uint32_t numRotKeys = readUInt32();
    constexpr uint32_t MAX_KEYS = 100000;
    if (numRotKeys > MAX_KEYS) {
        LOGE("NiKeyframeData: numRotKeys %u exceeds limit", numRotKeys);
        return false;
    }

    // Rotation keys
    // uint8_t rotationType (0=xyz, 1=constant, 2=linear, 3=quadratic)
    uint8_t rotType = readUInt32() & 0xFF;

    for (uint32_t i = 0; i < numRotKeys; i++) {
        NIFKeyframe kf;
        kf.time = readFloat();
        // Quaternion (x, y, z, w)
        kf.rotation = readVector4();
        clip.keyframes.push_back(kf);
    }

    // uint32_t numTranslateKeys
    uint32_t numTransKeys = readUInt32();
    if (numTransKeys > MAX_KEYS) {
        LOGE("NiKeyframeData: numTransKeys %u exceeds limit", numTransKeys);
        return false;
    }
    // uint8_t translateType
    uint8_t transType = readUInt32() & 0xFF;

    for (uint32_t i = 0; i < numTransKeys; i++) {
        float time = readFloat();
        NIFVector3 pos = readVector3();
        // Find or create keyframe at this time
        bool found = false;
        for (auto& kf : clip.keyframes) {
            if (std::abs(kf.time - time) < 0.001f) {
                kf.translation = pos;
                found = true;
                break;
            }
        }
        if (!found) {
            NIFKeyframe kf;
            kf.time = time;
            kf.translation = pos;
            clip.keyframes.push_back(kf);
        }
    }

    // uint32_t numScaleKeys
    uint32_t numScaleKeys = readUInt32();
    if (numScaleKeys > MAX_KEYS) {
        LOGE("NiKeyframeData: numScaleKeys %u exceeds limit", numScaleKeys);
        return false;
    }
    // uint8_t scaleType
    uint8_t scaleType = readUInt32() & 0xFF;

    for (uint32_t i = 0; i < numScaleKeys; i++) {
        float time = readFloat();
        float scale = readFloat();
        bool found = false;
        for (auto& kf : clip.keyframes) {
            if (std::abs(kf.time - time) < 0.001f) {
                kf.scale = scale;
                found = true;
                break;
            }
        }
        if (!found) {
            NIFKeyframe kf;
            kf.time = time;
            kf.scale = scale;
            clip.keyframes.push_back(kf);
        }
    }

    // Sort keyframes by time
    std::sort(clip.keyframes.begin(), clip.keyframes.end(),
              [](const NIFKeyframe& a, const NIFKeyframe& b) {
                  return a.time < b.time;
              });

    // Set duration
    if (!clip.keyframes.empty()) {
        clip.duration = clip.keyframes.back().time;
    }

    LOGD("  KeyframeData: %u rot, %u trans, %u scale, duration=%.2f",
         numRotKeys, numTransKeys, numScaleKeys, clip.duration);

    return true;
}

bool NIFParser::parseNiTextKeyExtraData(std::vector<NIFTextKey>& textKeys) {
    LOGD("Parsing NiTextKeyExtraData...");

    // NiExtraDataBase
    // uint32_t name (string ref)
    uint32_t nameRef = readUInt32();

    // uint32_t numTextKeys
    uint32_t numKeys = readUInt32();
    textKeys.resize(numKeys);

    for (uint32_t i = 0; i < numKeys; i++) {
        textKeys[i].time = readFloat();
        readStringRef(textKeys[i].value);
    }

    LOGD("  TextKeyExtraData: %u keys", numKeys);
    return true;
}

bool NIFParser::parseNiTransformController(NIFTransformController& controller) {
    LOGD("Parsing NiTransformController...");

    // NiTimeController base
    uint32_t nextController = readUInt32();
    uint16_t flags = readUInt16();
    float frequency = readFloat();
    float phase = readFloat();

    // NiTransformController specific
    controller.interpolatorIndex = readUInt32();

    LOGD("  TransformController: interpolator=%u", controller.interpolatorIndex);
    return true;
}

bool NIFParser::resolveControllerReferences(NIFControllerManager& manager,
                                             const std::vector<NIFKeyframeData>& keyframeDataArray) {
    LOGD("Resolving controller references...");

    // Resolve controlled block references
    for (auto& seq : manager.sequences) {
        for (auto& cb : seq.controlledBlocks) {
            // Resolve target node name
            if (cb.targetNodeIndex < nodes.size() && nodes[cb.targetNodeIndex]) {
                // Store resolved bone index for AnimationPlayer
                cb.resolvedBoneIndex = static_cast<int32_t>(cb.targetNodeIndex);
            }

            // Resolve keyframe data reference
            if (cb.keyframeDataIndex < keyframeDataArray.size()) {
                // Copy keyframe data into the controlled block's clip
                const auto& kfd = keyframeDataArray[cb.keyframeDataIndex];
                cb.clip = kfd.clip;
            }
        }
    }

    LOGD("Resolved %zu sequences", manager.sequences.size());
    return true;
}

// ============================================
// Phase 30 Step 9: bhkCollisionObject parsing
// ============================================

bool NIFParser::parseBhkCollisionObject(CollisionObject& obj) {
    // bhkCollisionObject:
    //   uint32_t target (parent node index)
    //   uint32_t flags (short)
    //   uint32_t body (bhkRigidBody index)
    //   uint32_t numBodyFilters
    //   uint32_t[] bodyFilters

    uint32_t targetIndex = 0;
    if (!readBytes(reinterpret_cast<char*>(&targetIndex), 4)) return false;
    obj.nodeIndex = targetIndex;

    uint16_t flags = 0;
    if (!readBytes(reinterpret_cast<char*>(&flags), 2)) return false;

    uint32_t bodyRef = 0;
    if (!readBytes(reinterpret_cast<char*>(&bodyRef), 4)) return false;
    obj.rigidBodyIndex = bodyRef;

    uint32_t numBodyFilters = 0;
    if (!readBytes(reinterpret_cast<char*>(&numBodyFilters), 4)) return false;
    // Skip body filters
    for (uint32_t i = 0; i < numBodyFilters; i++) {
        uint32_t filter = 0;
        if (!readBytes(reinterpret_cast<char*>(&filter), 4)) return false;
    }

    LOGD("bhkCollisionObject: target=%u, body=%u, flags=%u",
         targetIndex, bodyRef, flags);
    return true;
}

bool NIFParser::parseBhkRigidBody(RigidBodyInfo& info) {
    // bhkRigidBody (Oblivion NIF ver 20.x):
    //   Havok material (uint32_t)
    //   collisionFilterInfo (uint32_t)
    //   unknown (uint8_t[5])
    //   collisionResponse (uint8_t)
    //   unknown2 (uint8_t)
    //   processContactCallbackDelay (uint16_t)
    //   unknown3 (uint16_t)
    //   collisionFilterCopyInfo (uint32_t)
    //   unknown4 (uint8_t[4])
    //   mass (float)
    //   linearDamping (float)
    //   angularDamping (float)
    //   friction (float)
    //   restitution (float)
    //   maxLinearVelocity (float)
    //   maxAngularVelocity (float)
    //   penetrationDepth (float)
    //   motionSystem (uint8_t)
    //   deactivatorType (uint8_t)
    //   solverDeactivation (uint8_t)
    //   qualityType (uint8_t)
    //   autoRemoveLevel (int8_t)
    //   respondableMask (uint8_t[4])
    //   unknown5 (uint8_t[8])
    //   translation (hkVector4: 4 floats)
    //   rotation (hkQuaternion: 4 floats)
    //   linearVelocity (hkVector4: 4 floats)
    //   angularVelocity (hkVector4: 4 floats)
    //   inertiaMatrix (hkMatrix3: 9 floats)
    //   center (hkVector4: 4 floats)
    //   mass2 (float)
    //   linearDamping2 (float)
    //   angularDamping2 (float)
    //   friction2 (float)
    //   restitution2 (float)
    //   maxLinearVelocity2 (float)
    //   maxAngularVelocity2 (float)
    //   penetrationDepth2 (float)
    //   motionSystem2 (uint8_t)
    //   deactivatorType2 (uint8_t)
    //   solverDeactivation2 (uint8_t)
    //   qualityType2 (uint8_t)
    //   autoRemoveLevel2 (int8_t)
    //   respondableMask2 (uint8_t[4])
    //   unknown6 (uint8_t[8])
    //   translation2 (hkVector4)
    //   rotation2 (hkQuaternion)
    //   linearVelocity2 (hkVector4)
    //   angularVelocity2 (hkQuaternion)
    //   inertiaMatrix2 (hkMatrix3)
    //   center2 (hkVector4)
    //   numConstraints (uint32_t)
    //   constraints[] (uint32_t refs)
    //   unknown7 (uint8_t[4])

    // Skip Havok material
    uint32_t havokMaterial = 0;
    if (!readBytes(reinterpret_cast<char*>(&havokMaterial), 4)) return false;

    uint32_t collisionFilterInfo = 0;
    if (!readBytes(reinterpret_cast<char*>(&collisionFilterInfo), 4)) return false;
    info.collisionFilter = collisionFilterInfo;

    // Skip unknown bytes (5 + 1 + 1 + 2 + 2 + 4 + 4 = 19 bytes)
    char skipBuf[19];
    if (!readBytes(skipBuf, 19)) return false;

    // Mass
    if (!readBytes(reinterpret_cast<char*>(&info.mass), 4)) return false;

    // Linear damping
    float linearDamping = 0;
    if (!readBytes(reinterpret_cast<char*>(&linearDamping), 4)) return false;

    // Angular damping
    float angularDamping = 0;
    if (!readBytes(reinterpret_cast<char*>(&angularDamping), 4)) return false;

    // Friction
    if (!readBytes(reinterpret_cast<char*>(&info.friction), 4)) return false;

    // Restitution
    if (!readBytes(reinterpret_cast<char*>(&info.restitution), 4)) return false;

    // Max linear velocity, max angular velocity, penetration depth
    char skipBuf2[12];
    if (!readBytes(skipBuf2, 12)) return false;

    // Motion system, deactivator type, solver deactivation, quality type, auto remove level
    char skipBuf3[5];
    if (!readBytes(skipBuf3, 5)) return false;

    // Respondable mask (4 bytes)
    char skipBuf4[4];
    if (!readBytes(skipBuf4, 4)) return false;

    // Unknown (8 bytes)
    char skipBuf5[8];
    if (!readBytes(skipBuf5, 8)) return false;

    // Translation (hkVector4: x, y, z, w)
    float tx, ty, tz, tw;
    if (!readBytes(reinterpret_cast<char*>(&tx), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&ty), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&tz), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&tw), 4)) return false;
    info.transform.translation = {tx, ty, tz};

    // Rotation (hkQuaternion: x, y, z, w)
    float qx, qy, qz, qw;
    if (!readBytes(reinterpret_cast<char*>(&qx), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&qy), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&qz), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&qw), 4)) return false;

    // Linear velocity (hkVector4)
    float lvx, lvy, lvz, lvw;
    if (!readBytes(reinterpret_cast<char*>(&lvx), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&lvy), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&lvz), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&lvw), 4)) return false;
    info.linearVelocity = {lvx, lvy, lvz};

    // Angular velocity (hkVector4)
    float avx, avy, avz, avw;
    if (!readBytes(reinterpret_cast<char*>(&avx), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&avy), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&avz), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&avw), 4)) return false;
    info.angularVelocity = {avx, avy, avz};

    // Inertia matrix (9 floats) + center (4 floats) = 52 bytes
    char skipBuf6[52];
    if (!readBytes(skipBuf6, 52)) return false;

    // Second copy of properties (mass through center) = same structure
    // mass, linearDamping, angularDamping, friction, restitution
    // maxLinearVelocity, maxAngularVelocity, penetrationDepth
    // motionSystem, deactivatorType, solverDeactivation, qualityType, autoRemoveLevel
    // respondableMask, unknown
    // translation, rotation, linearVelocity, angularVelocity
    // inertiaMatrix, center
    // Total: 4*7 + 5 + 4 + 8 + 4*16 + 4*9 + 4*4 = 28 + 17 + 64 + 36 + 16 = 161 bytes
    // Simplified: skip the entire second copy
    size_t secondCopySize = 4*7 + 5 + 4 + 8 + 4*16 + 4*9 + 4*4;
    std::vector<char> skipBuf7(secondCopySize);
    if (!readBytes(skipBuf7.data(), secondCopySize)) return false;

    // Number of constraints
    uint32_t numConstraints = 0;
    if (!readBytes(reinterpret_cast<char*>(&numConstraints), 4)) return false;

    // Constraint references
    for (uint32_t i = 0; i < numConstraints; i++) {
        uint32_t constraintRef = 0;
        if (!readBytes(reinterpret_cast<char*>(&constraintRef), 4)) return false;
    }

    // Unknown (4 bytes)
    char skipBuf8[4];
    if (!readBytes(skipBuf8, 4)) return false;

    LOGD("bhkRigidBody: mass=%.2f, friction=%.2f, restitution=%.2f, constraints=%u",
         info.mass, info.friction, info.restitution, numConstraints);
    return true;
}

bool NIFParser::parseBhkShape(CollisionShape& shape) {
    // bhkShape is abstract - read the block type to determine actual shape
    // This is called from the main block parser when a bhkShape block is encountered
    // The actual shape type is determined by the block type string
    LOGD("parseBhkShape: dispatching based on block type");
    return true;
}

bool NIFParser::parseBhkBoxShape(CollisionShape& shape) {
    // bhkBoxShape:
    //   material (uint32_t)
    //   radius (float)
    //   dimensions (hkVector4: x, y, z, w) - half extents
    //   unknown (uint8_t[8])

    shape.type = CollisionShapeType::Box;

    uint32_t material = 0;
    if (!readBytes(reinterpret_cast<char*>(&material), 4)) return false;

    float radius = 0;
    if (!readBytes(reinterpret_cast<char*>(&radius), 4)) return false;
    shape.radius = radius;

    float dx, dy, dz, dw;
    if (!readBytes(reinterpret_cast<char*>(&dx), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&dy), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&dz), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&dw), 4)) return false;
    shape.halfExtents = {dx, dy, dz};

    char unknown[8];
    if (!readBytes(unknown, 8)) return false;

    LOGD("bhkBoxShape: halfExtents=(%.2f, %.2f, %.2f), radius=%.2f",
         dx, dy, dz, radius);
    return true;
}

bool NIFParser::parseBhkSphereShape(CollisionShape& shape) {
    // bhkSphereShape:
    //   material (uint32_t)
    //   radius (float)

    shape.type = CollisionShapeType::Sphere;

    uint32_t material = 0;
    if (!readBytes(reinterpret_cast<char*>(&material), 4)) return false;

    if (!readBytes(reinterpret_cast<char*>(&shape.radius), 4)) return false;

    LOGD("bhkSphereShape: radius=%.2f", shape.radius);
    return true;
}

bool NIFParser::parseBhkCapsuleShape(CollisionShape& shape) {
    // bhkCapsuleShape:
    //   material (uint32_t)
    //   radius (float)
    //   unknown1 (uint8_t[4])
    //   firstPoint (hkVector4: x, y, z, w)
    //   unknown2 (uint8_t[4])
    //   secondPoint (hkVector4: x, y, z, w)
    //   unknown3 (uint8_t[4])

    shape.type = CollisionShapeType::Capsule;

    uint32_t material = 0;
    if (!readBytes(reinterpret_cast<char*>(&material), 4)) return false;

    if (!readBytes(reinterpret_cast<char*>(&shape.radius), 4)) return false;

    char unknown1[4];
    if (!readBytes(unknown1, 4)) return false;

    float p1x, p1y, p1z, p1w;
    if (!readBytes(reinterpret_cast<char*>(&p1x), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&p1y), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&p1z), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&p1w), 4)) return false;

    char unknown2[4];
    if (!readBytes(unknown2, 4)) return false;

    float p2x, p2y, p2z, p2w;
    if (!readBytes(reinterpret_cast<char*>(&p2x), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&p2y), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&p2z), 4)) return false;
    if (!readBytes(reinterpret_cast<char*>(&p2w), 4)) return false;

    char unknown3[4];
    if (!readBytes(unknown3, 4)) return false;

    // Height = distance between two points
    float dx = p2x - p1x, dy = p2y - p1y, dz = p2z - p1z;
    shape.height = sqrtf(dx*dx + dy*dy + dz*dz);
    shape.center = {(p1x + p2x) * 0.5f, (p1y + p2y) * 0.5f, (p1z + p2z) * 0.5f};

    LOGD("bhkCapsuleShape: radius=%.2f, height=%.2f", shape.radius, shape.height);
    return true;
}

bool NIFParser::parseBhkConvexVerticesShape(CollisionShape& shape) {
    // bhkConvexVerticesShape:
    //   material (uint32_t)
    //   radius (float)
    //   unknown1 (uint8_t[16]) - hkAabb
    //   numVertices (uint32_t)
    //   vertices[] (hkVector4: x, y, z, w per vertex)
    //   numNormals (uint32_t)
    //   normals[] (hkVector4: x, y, z, w per normal)

    shape.type = CollisionShapeType::ConvexHull;

    uint32_t material = 0;
    if (!readBytes(reinterpret_cast<char*>(&material), 4)) return false;

    if (!readBytes(reinterpret_cast<char*>(&shape.radius), 4)) return false;

    // Skip unknown (16 bytes - hkAabb min/max)
    char unknown1[16];
    if (!readBytes(unknown1, 16)) return false;

    uint32_t numVertices = 0;
    if (!readBytes(reinterpret_cast<char*>(&numVertices), 4)) return false;

    shape.vertices.reserve(numVertices);
    for (uint32_t i = 0; i < numVertices; i++) {
        float vx, vy, vz, vw;
        if (!readBytes(reinterpret_cast<char*>(&vx), 4)) return false;
        if (!readBytes(reinterpret_cast<char*>(&vy), 4)) return false;
        if (!readBytes(reinterpret_cast<char*>(&vz), 4)) return false;
        if (!readBytes(reinterpret_cast<char*>(&vw), 4)) return false;
        shape.vertices.push_back({vx, vy, vz});
    }

    uint32_t numNormals = 0;
    if (!readBytes(reinterpret_cast<char*>(&numNormals), 4)) return false;
    // Skip normals (not needed for collision detection)
    for (uint32_t i = 0; i < numNormals; i++) {
        char normalData[16];
        if (!readBytes(normalData, 16)) return false;
    }

    LOGD("bhkConvexVerticesShape: %u vertices, %u normals", numVertices, numNormals);
    return true;
}

bool NIFParser::parseBhkMeshShape(CollisionShape& shape) {
    // bhkMeshShape:
    //   material (uint32_t)
    //   radius (float)
    //   unknown1 (uint8_t[8])
    //   unknown2 (uint8_t[4])
    //   unknown3 (uint8_t[4])
    //   numSubShapes (uint32_t)
    //   subShapes[] (uint32_t refs)
    //   unknown4 (uint8_t[4])
    //   unknown5 (uint8_t[4])
    //   numVertices (uint32_t)
    //   vertices[] (hkVector4: x, y, z, w per vertex)
    //   numTriangles (uint32_t)
    //   triangles[] (3 x uint16_t + uint16_t material per triangle)

    shape.type = CollisionShapeType::TriMesh;

    uint32_t material = 0;
    if (!readBytes(reinterpret_cast<char*>(&material), 4)) return false;

    if (!readBytes(reinterpret_cast<char*>(&shape.radius), 4)) return false;

    char unknown1[8];
    if (!readBytes(unknown1, 8)) return false;

    char unknown2[4];
    if (!readBytes(unknown2, 4)) return false;

    char unknown3[4];
    if (!readBytes(unknown3, 4)) return false;

    uint32_t numSubShapes = 0;
    if (!readBytes(reinterpret_cast<char*>(&numSubShapes), 4)) return false;

    for (uint32_t i = 0; i < numSubShapes; i++) {
        uint32_t subShapeRef = 0;
        if (!readBytes(reinterpret_cast<char*>(&subShapeRef), 4)) return false;
    }

    char unknown4[4];
    if (!readBytes(unknown4, 4)) return false;

    char unknown5[4];
    if (!readBytes(unknown5, 4)) return false;

    uint32_t numVertices = 0;
    if (!readBytes(reinterpret_cast<char*>(&numVertices), 4)) return false;

    shape.vertices.reserve(numVertices);
    for (uint32_t i = 0; i < numVertices; i++) {
        float vx, vy, vz, vw;
        if (!readBytes(reinterpret_cast<char*>(&vx), 4)) return false;
        if (!readBytes(reinterpret_cast<char*>(&vy), 4)) return false;
        if (!readBytes(reinterpret_cast<char*>(&vz), 4)) return false;
        if (!readBytes(reinterpret_cast<char*>(&vw), 4)) return false;
        shape.vertices.push_back({vx, vy, vz});
    }

    uint32_t numTriangles = 0;
    if (!readBytes(reinterpret_cast<char*>(&numTriangles), 4)) return false;

    shape.triangles.reserve(numTriangles);
    for (uint32_t i = 0; i < numTriangles; i++) {
        uint16_t v0, v1, v2, triMaterial;
        if (!readBytes(reinterpret_cast<char*>(&v0), 2)) return false;
        if (!readBytes(reinterpret_cast<char*>(&v1), 2)) return false;
        if (!readBytes(reinterpret_cast<char*>(&v2), 2)) return false;
        if (!readBytes(reinterpret_cast<char*>(&triMaterial), 2)) return false;
        shape.triangles.push_back({v0, v1, v2});
    }

    LOGD("bhkMeshShape: %u vertices, %u triangles", numVertices, numTriangles);
    return true;
}

bool NIFParser::parseBhkPackedNiTriStripsShape(CollisionShape& shape) {
    // bhkPackedNiTriStripsShape:
    //   material (uint32_t)
    //   radius (float)
    //   unknown1 (uint8_t[8])
    //   unknown2 (uint8_t[4])
    //   unknown3 (uint8_t[4])
    //   numSubShapes (uint32_t)
    //   subShapes[] (each: numVertices(uint16_t), unknown(uint16_t), material(uint32_t))
    //   unknown4 (uint8_t[4])
    //   unknown5 (uint8_t[4])
    //   numVertices (uint32_t)
    //   vertices[] (hkVector4: x, y, z, w per vertex)
    //   numTriangles (uint32_t)
    //   triangles[] (3 x uint16_t + uint16_t weldingInfo per triangle)
    //   numUnknown (uint32_t)
    //   unknown6[] (uint8_t[4] per entry)

    shape.type = CollisionShapeType::TriMesh;

    uint32_t material = 0;
    if (!readBytes(reinterpret_cast<char*>(&material), 4)) return false;

    if (!readBytes(reinterpret_cast<char*>(&shape.radius), 4)) return false;

    char unknown1[8];
    if (!readBytes(unknown1, 8)) return false;

    char unknown2[4];
    if (!readBytes(unknown2, 4)) return false;

    char unknown3[4];
    if (!readBytes(unknown3, 4)) return false;

    uint32_t numSubShapes = 0;
    if (!readBytes(reinterpret_cast<char*>(&numSubShapes), 4)) return false;

    for (uint32_t i = 0; i < numSubShapes; i++) {
        uint16_t numVerts, unknown;
        uint32_t subMaterial;
        if (!readBytes(reinterpret_cast<char*>(&numVerts), 2)) return false;
        if (!readBytes(reinterpret_cast<char*>(&unknown), 2)) return false;
        if (!readBytes(reinterpret_cast<char*>(&subMaterial), 4)) return false;
    }

    char unknown4[4];
    if (!readBytes(unknown4, 4)) return false;

    char unknown5[4];
    if (!readBytes(unknown5, 4)) return false;

    uint32_t numVertices = 0;
    if (!readBytes(reinterpret_cast<char*>(&numVertices), 4)) return false;

    shape.vertices.reserve(numVertices);
    for (uint32_t i = 0; i < numVertices; i++) {
        float vx, vy, vz, vw;
        if (!readBytes(reinterpret_cast<char*>(&vx), 4)) return false;
        if (!readBytes(reinterpret_cast<char*>(&vy), 4)) return false;
        if (!readBytes(reinterpret_cast<char*>(&vz), 4)) return false;
        if (!readBytes(reinterpret_cast<char*>(&vw), 4)) return false;
        shape.vertices.push_back({vx, vy, vz});
    }

    uint32_t numTriangles = 0;
    if (!readBytes(reinterpret_cast<char*>(&numTriangles), 4)) return false;

    shape.triangles.reserve(numTriangles);
    for (uint32_t i = 0; i < numTriangles; i++) {
        uint16_t v0, v1, v2, weldingInfo;
        if (!readBytes(reinterpret_cast<char*>(&v0), 2)) return false;
        if (!readBytes(reinterpret_cast<char*>(&v1), 2)) return false;
        if (!readBytes(reinterpret_cast<char*>(&v2), 2)) return false;
        if (!readBytes(reinterpret_cast<char*>(&weldingInfo), 2)) return false;
        shape.triangles.push_back({v0, v1, v2});
    }

    uint32_t numUnknown = 0;
    if (!readBytes(reinterpret_cast<char*>(&numUnknown), 4)) return false;
    for (uint32_t i = 0; i < numUnknown; i++) {
        char unknownData[4];
        if (!readBytes(unknownData, 4)) return false;
    }

    LOGD("bhkPackedNiTriStripsShape: %u vertices, %u triangles", numVertices, numTriangles);
    return true;
}

bool NIFParser::parseBhkMoppBvTreeShape(CollisionShape& shape) {
    // bhkMoppBvTreeShape:
    //   material (uint32_t)
    //   radius (float)
    //   unknown1 (uint8_t[8])
    //   child (uint32_t ref - bhkShape)
    //   unknown2 (uint8_t[4])
    //   unknown3 (uint8_t[4])
    //   moppDataSize (uint32_t)
    //   moppData[] (uint8_t)
    //   unknown4 (uint8_t[16]) - scale/offset

    shape.type = CollisionShapeType::MoppBvTree;

    uint32_t material = 0;
    if (!readBytes(reinterpret_cast<char*>(&material), 4)) return false;

    if (!readBytes(reinterpret_cast<char*>(&shape.radius), 4)) return false;

    char unknown1[8];
    if (!readBytes(unknown1, 8)) return false;

    uint32_t childRef = 0;
    if (!readBytes(reinterpret_cast<char*>(&childRef), 4)) return false;

    char unknown2[4];
    if (!readBytes(unknown2, 4)) return false;

    char unknown3[4];
    if (!readBytes(unknown3, 4)) return false;

    uint32_t moppDataSize = 0;
    if (!readBytes(reinterpret_cast<char*>(&moppDataSize), 4)) return false;
    shape.moppDataSize = moppDataSize;

    shape.moppData.resize(moppDataSize);
    if (!readBytes(reinterpret_cast<char*>(shape.moppData.data()), moppDataSize)) return false;

    char unknown4[16];
    if (!readBytes(unknown4, 16)) return false;

    LOGD("bhkMoppBvTreeShape: child=%u, moppDataSize=%u", childRef, moppDataSize);
    return true;
}

bool NIFParser::parseBhkCollisionFilter(uint32_t& group, uint32_t& filter) {
    // bhkCollisionFilter is not a separate block in Oblivion NIF
    // The filter info is embedded in bhkRigidBody
    // This is a placeholder for future use
    group = 0;
    filter = 0;
    return true;
}
