#include "nif_parser.h"
#include <android/log.h>
#include <algorithm>
#include <cstring>

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

    char magic[256];
    if (!readBytes(magic, 256)) {
        LOGE("Failed to read magic number");
        return false;
    }

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
    if (length > 0 && length < 256) {
        char buffer[256];
        if (!readBytes(buffer, length)) {
            return false;
        }
        str = std::string(buffer, length);
        return true;
    }
    return false;
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
    sequence.textKeys.resize(textKeyCount);

    // Read text keys
    for (uint32_t i = 0; i < textKeyCount; i++) {
        sequence.textKeys[i].time = readFloat();
        readStringRef(sequence.textKeys[i].value);
    }

    // uint32_t controlledBlockCount
    uint32_t blockCount = readUInt32();
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
