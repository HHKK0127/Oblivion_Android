#include "nif_parser.h"
#include <android/log.h>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <fstream>

#undef LOG_TAG
#undef LOGD
#undef LOGW
#undef LOGE
#define LOG_TAG "NIFParser"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {
// Upper bounds that keep a corrupt block header from turning into a huge
// allocation. Oblivion's largest shipped skeletons stay well below these.
constexpr uint32_t MAX_SKIN_BONES = 4096;
constexpr uint32_t MAX_SKIN_PARTITIONS = 4096;
constexpr uint32_t MAX_WEIGHTS_PER_VERTEX = 4;
constexpr uint32_t MAX_CONTROLLER_SEQUENCES = 4096;
constexpr uint32_t MAX_CONTROLLED_BLOCKS = 4096;
constexpr uint32_t MAX_RIGID_BODY_CONSTRAINTS = 4096;
constexpr uint32_t MAX_CONVEX_VERTICES = 65536;
constexpr uint32_t MAX_STRIPS_DATA = 4096;
constexpr uint32_t MAX_SHAPE_FILTERS = 4096;
constexpr uint32_t MAX_LIST_SUB_SHAPES = 4096;
constexpr uint32_t MAX_MOPP_DATA_SIZE = 1u << 20;
constexpr uint32_t MAX_SPLINE_CONTROL_POINTS = 1u << 20;
constexpr uint32_t MAX_TRAILER_VALUES = 64;

// Gamebryo version cut-offs used by the .kf sequence records. Oblivion's own
// animations are 20.0.0.4, but the shipped archives still carry a handful of
// legacy 10.1.0.106 / 10.2.0.0 creature idle files whose sequence and
// controlled block records differ in exactly these ranges.
constexpr uint32_t VERSION_10_1_0_104 = 0x0A010068;
constexpr uint32_t VERSION_10_1_0_106 = 0x0A01006A;
constexpr uint32_t VERSION_10_1_0_110 = 0x0A01006E;
constexpr uint32_t VERSION_10_1_0_113 = 0x0A010071;
constexpr uint32_t VERSION_10_2_0_0 = 0x0A020000;
constexpr uint32_t VERSION_10_4_0_1 = 0x0A040001;
constexpr uint32_t VERSION_20_0_0_5 = 0x14000005;
constexpr uint32_t VERSION_20_1_0_0 = 0x14010000;
}

NIFParser::NIFParser() {
    std::memset(header.magic, 0, sizeof(header.magic));
}

NIFParser::~NIFParser() {
}

bool NIFParser::parseFile(const std::string& filepath) {
    LOGD("=== Starting NIF parse: %s ===", filepath.c_str());

    nodes.clear();
    rootNodeIndices.clear();
    fileBuffer.clear();
    cursor = 0;
    readError = false;
    header = NIFHeader();

    // Block ranges from a previous file would otherwise stay visible through
    // locateBlockBody() even though the new file has not been walked yet.
    blockBodyOffsets.clear();
    blockBodyEnds.clear();
    blockPrefix = 0;
    blocksWalked = false;
    walkError.clear();

    // Read the whole file up front. Block bodies have no size table, so block
    // parsing continues after parseFile() returns and needs random access.
    std::ifstream in(filepath, std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        LOGE("Failed to open NIF file: %s", filepath.c_str());
        return false;
    }

    const std::streamoff fileSize = in.tellg();
    if (fileSize <= 0) {
        LOGE("Empty or unreadable NIF file: %s", filepath.c_str());
        in.close();
        return false;
    }

    fileBuffer.resize(static_cast<size_t>(fileSize));
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(fileBuffer.data()), fileSize);
    const bool shortRead = (in.gcount() != fileSize);
    in.close();
    if (shortRead) {
        LOGE("Short read on NIF file: %s", filepath.c_str());
        fileBuffer.clear();
        return false;
    }

    // Read header
    if (!readHeader()) {
        LOGE("Failed to read NIF header");
        fileBuffer.clear();
        return false;
    }

    LOGD("NIF Header: version=0x%08X, userVersion=%u, numBlocks=%u, blockTypes=%zu, blockDataOffset=%zu",
         header.version, header.userVersion, header.numObjects,
         header.blockTypeNames.size(), header.blockDataOffset);

    // Parse object array
    if (!parseObjectArray()) {
        LOGE("Failed to parse object array");
        fileBuffer.clear();
        return false;
    }

    // Build node hierarchy
    if (!buildNodeHierarchy()) {
        LOGE("Failed to build node hierarchy");
        fileBuffer.clear();
        return false;
    }

    // Record the byte range of every block body. This is best effort: a walk
    // failure only disables the block-anchored readers below, it does not make
    // the file unusable.
    if (walkAllBlocks()) {
        LOGD("NIF block walk: %zu blocks, prefix=%u",
             blockBodyOffsets.size(), blockPrefix);
    } else {
        LOGW("NIF block walk failed (%s); block-anchored readers unavailable",
             walkError.c_str());
    }

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

    // Verify magic. Retail Oblivion ships two families: the Gamebryo header used
    // by almost every mesh, and a compact NetImmerse header on legacy meshes.
    const bool legacyNetImmerse = (strstr(header.magic, "NetImmerse File Format") != nullptr);
    if (!legacyNetImmerse && strstr(header.magic, "Gamebryo File Format") == nullptr) {
        LOGE("Invalid NIF magic: %s", header.magic);
        return false;
    }
    header.legacyNetImmerse = legacyNetImmerse;

    LOGD("NIF Magic: %s", header.magic);

    // Version info
    header.version = readUInt32();

    // Meshes older than 5.0.0.1 have no block type table and no header tail at
    // all: nif.xml opens "User Version" at 10.0.1.8 and both "Num Block Types"
    // and "Block Type Index" at 5.0.0.1, so the block count is the last header
    // field and block 0 starts right after it. Each block body then carries its
    // own inline SizedString type name, which walkAllBlocks() reads.
    if (header.version < 0x05000001) {
        header.hasBlockTypeTable = false;
        header.numObjects = readUInt32();
        if (readError || header.numObjects == 0 || header.numObjects > 0x100000) {
            LOGE("Invalid block count: %u", header.numObjects);
            return false;
        }
        header.blockDataOffset = cursor;
        return true;
    }

    if (legacyNetImmerse) {
        // NetImmerse 10.0.1.0 meshes have no endian byte, no user version, no
        // unknown field and no creator strings: the block count follows the version.
        header.numObjects = readUInt32();

        // 10.0.1.2 is the single NetImmerse version that carries the Bethesda
        // stream header (nif.xml: BSStreamHeader cond is VER == 10.0.1.2): one
        // unknown uint followed by three byte strings. Verified against all 23
        // retail 10.0.1.2 meshes in the shipped BSA archives.
        if (header.version == 0x0A000102) {
            header.unknownField = readUInt32();
            if (!readByteString(header.creator) ||
                !readByteString(header.processScript) ||
                !readByteString(header.exportScript)) {
                LOGE("Failed to read header byte strings");
                return false;
            }
        }
    } else {
        // The 20.0.0.x family and later insert an endian byte after the version.
        if (header.version >= 0x14000000) {
            header.endian = readUInt8();
            header.hasEndianByte = true;
        }

        header.userVersion = readUInt32();
        header.numObjects = readUInt32();
        header.unknownField = readUInt32();

        if (!readByteString(header.creator) ||
            !readByteString(header.processScript) ||
            !readByteString(header.exportScript)) {
            LOGE("Failed to read header byte strings");
            return false;
        }
    }

    const uint16_t numBlockTypes = readUInt16();
    if (numBlockTypes == 0 || numBlockTypes > 4096) {
        LOGE("Invalid block type count: %u", numBlockTypes);
        return false;
    }

    header.blockTypeNames.reserve(numBlockTypes);
    for (uint16_t i = 0; i < numBlockTypes; i++) {
        std::string typeName;
        if (!readString(typeName)) {
            LOGE("Failed to read block type name %u", i);
            return false;
        }
        header.blockTypeNames.push_back(typeName);
    }

    if (header.numObjects == 0 || header.numObjects > 0x100000) {
        LOGE("Invalid block count: %u", header.numObjects);
        return false;
    }

    header.blockTypeIndices.resize(header.numObjects);
    for (uint32_t i = 0; i < header.numObjects; i++) {
        const uint16_t typeIndex = readUInt16();
        if (typeIndex >= numBlockTypes) {
            LOGE("Block %u has out-of-range type index %u (numBlockTypes=%u)",
                 i, typeIndex, numBlockTypes);
            return false;
        }
        header.blockTypeIndices[i] = typeIndex;
    }

    // Block groups. numGroups is 0 in every sampled retail mesh, and the group
    // array is not a string table: it sits between the block type index array
    // and block 0, so block 0 begins immediately after numGroups.
    header.numGroups = readUInt32();
    if (header.numGroups > 0x10000) {
        LOGE("Invalid block group count: %u", header.numGroups);
        return false;
    }
    for (uint32_t i = 0; i < header.numGroups; i++) {
        readUInt32();  // Group description index
    }

    header.blockDataOffset = cursor;
    return !readError;
}

bool NIFParser::parseObjectArray() {
    LOGD("Parsing object array: %u blocks, %zu block types",
         header.numObjects, header.blockTypeNames.size());

    nodes.resize(header.numObjects);

    // Pre-5.0.0.1 meshes carry no type table, so block 0's own inline type name
    // is the only block type known before the walk. The remaining entries are
    // filled in by walkAllBlocks(), which reads the type name of every block.
    if (!header.hasBlockTypeTable) {
        setCursor(header.blockDataOffset);
        std::string rootTypeName;
        if (!readString(rootTypeName)) {
            LOGE("Failed to read inline block type name at offset %zu", header.blockDataOffset);
            return false;
        }
        header.blockTypeNames.clear();
        header.blockTypeNames.push_back(rootTypeName);
        header.blockTypeIndices.assign(header.numObjects, 0);
        LOGD("Legacy block table: block 0 type is %s", rootTypeName.c_str());
    }

    for (uint32_t i = 0; i < header.numObjects; i++) {
        auto node = std::make_shared<NIFNode>();
        node->nodeIndex = i;
        node->parentIndex = -1;
        node->hasGeometry = false;
        node->blockTypeIndex = header.blockTypeIndices[i];
        node->blockTypeName = header.blockTypeNames[node->blockTypeIndex];
        nodes[i] = node;
    }

    // Block 0 is the scene root. Like every NiObjectNET-derived block it starts
    // with its inline object name, which is the only block name that can be read
    // without walking the variable-length block bodies.
    if (nodes.empty() || !isNamedBlockType(nodes[0]->blockTypeName)) {
        LOGD("Block 0 carries no inline object name");
        return true;
    }

    setCursor(header.blockDataOffset);
    if (!header.hasBlockTypeTable) {
        std::string inlineTypeName;
        if (!readString(inlineTypeName)) {  // Type name precedes the object name
            LOGE("Failed to skip inline block type name at offset %zu", header.blockDataOffset);
            return false;
        }
    }
    if (!readString(nodes[0]->name)) {
        LOGE("Failed to read root object name at offset %zu", header.blockDataOffset);
        return false;
    }

    LOGD("Block 0: %s (%s)", nodes[0]->name.c_str(), nodes[0]->blockTypeName.c_str());
    return true;
}

std::string NIFParser::getBlockTypeName(uint32_t index) const {
    if (index >= header.blockTypeIndices.size()) {
        return std::string();
    }
    const uint16_t typeIndex = header.blockTypeIndices[index];
    if (typeIndex >= header.blockTypeNames.size()) {
        return std::string();
    }
    return header.blockTypeNames[typeIndex];
}

bool NIFParser::hasBlockType(const std::string& typeName) const {
    for (uint32_t i = 0; i < header.blockTypeIndices.size(); i++) {
        if (getBlockTypeName(i) == typeName) {
            return true;
        }
    }
    return false;
}

// Types that derive from NiObjectNET and therefore start their block body with
// an inline object name. Only the scene-graph types are listed, which covers
// block 0 of every retail Oblivion mesh.
bool NIFParser::isNamedBlockType(const std::string& typeName) {
    static const char* kNamedTypes[] = {
        "NiNode", "NiTriShape", "NiTriStrips", "NiBSAnimationNode",
        "BSFadeNode", "NiBillboardNode", "NiLODNode", "NiSwitchNode",
        "NiSortAdjustNode", "NiBSBoneLODController"
    };
    for (const char* named : kNamedTypes) {
        if (typeName == named) {
            return true;
        }
    }
    return false;
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
    if (cursor + count > fileBuffer.size()) {
        readError = true;
        return false;
    }
    if (count > 0) {
        std::memcpy(buffer, fileBuffer.data() + cursor, count);
    }
    cursor += count;
    return true;
}

uint8_t NIFParser::readUInt8() {
    uint8_t value = 0;
    if (!readBytes(reinterpret_cast<char*>(&value), sizeof(uint8_t))) {
        return 0;
    }
    return value;
}

bool NIFParser::readString(std::string& str) {
    uint32_t length = readUInt32();
    if (readError) {
        return false;
    }
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

// bzstring: a u8 length that includes the terminating NUL, followed by the body.
bool NIFParser::readByteString(std::string& str) {
    const uint8_t length = readUInt8();
    if (readError) {
        return false;
    }
    if (length == 0) {
        str.clear();
        return true;
    }
    std::vector<char> buffer(length);
    if (!readBytes(buffer.data(), length)) {
        return false;
    }
    size_t textLength = length;
    while (textLength > 0 && buffer[textLength - 1] == '\0') {
        textLength--;
    }
    str.assign(buffer.data(), textLength);
    return true;
}

bool NIFParser::readStringRef(std::string& str) {
    uint32_t index = readUInt32();
    if (readError) {
        return false;
    }
    // In full implementation, would look up from string table
    // For now, just set a placeholder
    str = "string_" + std::to_string(index);
    return true;
}

uint32_t NIFParser::readUInt32() {
    uint32_t value = 0;
    if (!readBytes(reinterpret_cast<char*>(&value), sizeof(uint32_t))) {
        return 0;
    }
    return value;
}

uint16_t NIFParser::readUInt16() {
    uint16_t value = 0;
    if (!readBytes(reinterpret_cast<char*>(&value), sizeof(uint16_t))) {
        return 0;
    }
    return value;
}

float NIFParser::readFloat() {
    float value = 0.0f;
    if (!readBytes(reinterpret_cast<char*>(&value), sizeof(float))) {
        return 0.0f;
    }
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

// Havok stores 3-component values padded to 16 bytes; the padding is dropped.
NIFVector3 NIFParser::readHKVector3() {
    float x = readFloat();
    float y = readFloat();
    float z = readFloat();
    readFloat();                                // w padding
    return NIFVector3(x, y, z);
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

// ---------------------------------------------------------------------------
// Block body walker
// ---------------------------------------------------------------------------
// Retail NIF files store no per-block size table, so the only way to reach a
// given block body is to measure every preceding body in order. The helpers
// below reproduce the nif.xml field lists for the 80 block types that appear in
// the shipped Oblivion meshes (20.0.0.4, user version 2817, BS version 11).
// They only advance the cursor; the typed readers (parseNiSkinData and friends)
// re-read the bodies afterwards. Any skip that runs past the end of the buffer
// leaves readError set, which every helper checks before doing more work.

bool NIFParser::skipBytes(size_t count) {
    if (readError) {
        return false;
    }
    if (cursor + count > fileBuffer.size()) {
        readError = true;
        return false;
    }
    cursor += count;
    return true;
}

void NIFParser::skipFixed(size_t count) {
    skipBytes(count);
}

// nif.xml: bool is "32-bit up to and including 4.0.0.2, 8-bit from 4.1.0.1 on".
// The pre-5.0.0.1 meshes that this walker now reaches all sit below that cut, so
// their flags occupy a full uint32.
uint32_t NIFParser::readBoolField() {
    if (header.version <= 0x04000002) {
        return readUInt32();
    }
    return readUInt8();
}

void NIFParser::skipRefArray(uint32_t count) {
    skipBytes(static_cast<size_t>(count) * 4);
}

void NIFParser::skipPtrArray(uint32_t count) {
    skipBytes(static_cast<size_t>(count) * 4);
}

void NIFParser::skipVector3Array(uint32_t count) {
    skipBytes(static_cast<size_t>(count) * 12);
}

void NIFParser::skipVector4Array(uint32_t count) {
    skipBytes(static_cast<size_t>(count) * 16);
}

void NIFParser::skipColor4Array(uint32_t count) {
    skipBytes(static_cast<size_t>(count) * 16);
}

void NIFParser::skipFloatArray(uint32_t count) {
    skipBytes(static_cast<size_t>(count) * 4);
}

void NIFParser::skipU16Array(uint32_t count) {
    skipBytes(static_cast<size_t>(count) * 2);
}

// SizedString / ByteArray / FilePath all share the u32 length prefix layout.
void NIFParser::skipStringArray(uint32_t count) {
    for (uint32_t i = 0; i < count && !readError; i++) {
        const uint32_t length = readUInt32();
        if (readError) {
            return;
        }
        skipBytes(length);
    }
}

// NiObjectNET: Name | Extra Data List | Controller
void NIFParser::skipNiObjectNET() {
    skipStringArray(1);
    if (!header.hasBlockTypeTable) {
        // Pre-5.0.0.1 meshes store Extra Data as a single ref with no list
        // count, which only arrived with the 10.0.1.0 layout.
        skipBytes(4);                           // Extra Data
        skipBytes(4);                           // Controller
        return;
    }
    const uint32_t extraDataCount = readUInt32();
    if (readError) {
        return;
    }
    skipRefArray(extraDataCount);
    skipBytes(4);
}

// NiAVObject: NiObjectNET | Flags | Translation | Rotation | Scale | Properties
// | Collision Object
void NIFParser::skipNiAVObject() {
    skipNiObjectNET();
    if (readError) {
        return;
    }
    skipBytes(2);
    skipBytes(12);
    skipBytes(36);
    skipBytes(4);
    if (!header.hasBlockTypeTable) {
        skipBytes(12);                          // Velocity (until 4.2.2.0)
    }
    const uint32_t propertyCount = readUInt32();
    if (readError) {
        return;
    }
    skipRefArray(propertyCount);
    if (!header.hasBlockTypeTable) {
        // Has Bounding Volume is version-sized per nif.xml, and the collision
        // object only exists from 10.0.1.0 on.
        if (readBoolField() != 0) {
            skipBytes(16);                      // Bounding Volume
        }
        return;
    }
    skipBytes(4);
}

// NiGeometry: NiAVObject | Data | Skin Instance | Material Data
void NIFParser::skipNiGeometry() {
    skipNiAVObject();
    if (readError) {
        return;
    }
    skipBytes(4);
    if (header.version >= 0x0303000D) {
        skipBytes(4);                           // Skin Instance (since 3.3.0.13)
    }
    if (header.version >= 0x0A000100) {
        skipMaterialData();     // Material Data (since 10.0.1.0)
    }
}

// MaterialData on these meshes is Has Shader plus its optional pair.
void NIFParser::skipMaterialData() {
    const uint8_t hasShader = readUInt8();
    if (readError) {
        return;
    }
    if (hasShader != 0) {
        skipStringArray(1);   // Shader Name
        skipBytes(4);         // Shader Extra Data
    }
}

// NiTimeController: Next Controller | Flags | Frequency | Phase | Start Time
// | Stop Time | Target
void NIFParser::skipNiTimeController() {
    skipBytes(4 + 2 + 4 + 4 + 4 + 4 + 4);
}

// NiInterpController adds Manager Controlled inside the 10.1.0.104..10.1.0.108
// window (inclusive), so only the 10.1.0.106 meshes carry the extra byte.
void NIFParser::skipNiInterpController() {
    skipNiTimeController();
    if (header.version >= 0x0A010068 && header.version <= 0x0A01006C) {
        skipBytes(1);   // Manager Controlled
    }
}

// nif.xml #BSVER#: the header user version carries the Bethesda stream version.
// The BS header carries the block stream version separately from the user
// version: the 20.0.0.4 meshes report 11, the 10.1.0.106 menus 5 and the
// 10.2.0.0 meshes 6 or 9. The legacy NetImmerse files have no BS header, which
// is the zero that nif.xml means when it says that the #BSVER# #LT# conditions
// also apply to the NI streams.
uint32_t NIFParser::bsVersion() const {
    return header.unknownField;
}

// NiPSysModifier: Name | Order | Target | Active
void NIFParser::skipNiPSysModifier() {
    skipStringArray(1);
    skipBytes(4 + 4 + 1);
}

// NiPSysEmitter: Speed | Speed Variation | Declination | Declination Variation
// | Planar Angle | Planar Angle Variation | Initial Color | Initial Radius
// | Radius Variation | Life Span | Life Span Variation [| Emitter Object]
// Radius Variation only exists from 10.4.0.1 on. The Oblivion meshes carry
// neither it nor the Unknown QQSpeed pair of nif.xml, which is contradicted by
// the exact block boundaries of miscfirefly01.nif (10.2.0.0), so the payload
// stays 52 bytes without an emitter object and 56 bytes with one.
void NIFParser::skipNiPSysEmitterBase(bool hasEmitterObject) {
    skipBytes(6 * 4);
    skipBytes(16);          // Initial Color (Color4)
    skipBytes(4);           // Initial Radius
    if (header.version >= 0x0A040001) {
        skipBytes(4);       // Radius Variation (since 10.4.0.1)
    }
    skipBytes(4 + 4);       // Life Span, Life Span Variation
    if (hasEmitterObject && header.version >= 0x0A010000) {
        skipBytes(4);       // Emitter Object (since 10.1.0.0)
    }
}

// NiDynamicEffect: Switch State | Num Affected Nodes | Affected Nodes
// The affected node list only exists from 10.1.0.0 on, and the switch state
// only from 10.1.0.106 on, so the 10.0.1.0 lights carry neither field.
void NIFParser::skipNiDynamicEffect() {
    if (header.version < 0x0A010000) {
        return;
    }
    if (header.version >= 0x0A01006A) {
        skipBytes(1);       // Switch State (since 10.1.0.106)
    }
    const uint32_t numAffectedNodes = readUInt32();
    if (readError) {
        return;
    }
    skipPtrArray(numAffectedNodes);
}

// NiLight: Dimmer | Ambient Color | Diffuse Color | Specular Color
void NIFParser::skipNiLight() {
    skipNiDynamicEffect();
    if (readError) {
        return;
    }
    skipBytes(4 + 12 + 12 + 12);
}

// TexDesc: Source | Clamp Mode | Filter Mode | UV Set | PS2 L | PS2 K
// | Has Texture Transform plus the transform block when present.
// PS2 L / PS2 K only exist up to 10.4.0.1.
void NIFParser::skipTexDesc() {
    skipBytes(4 + 4 + 4 + 4);
    if (header.version <= 0x0A040001) {
        skipBytes(2 + 2);               // PS2 L, PS2 K
    }
    if (header.version >= 0x0A010000) {
        const uint8_t hasTransform = readUInt8();
        if (readError) {
            return;
        }
        if (hasTransform != 0) {
            skipBytes(8 + 8 + 4 + 4 + 8);   // Translation, Scale, Rotation, Method, Center
        }
    }
}

void NIFParser::skipShaderTexDescs(uint32_t count) {
    for (uint32_t i = 0; i < count && !readError; i++) {
        const uint8_t hasMap = readUInt8();
        if (readError) {
            return;
        }
        if (hasMap != 0) {
            skipTexDesc();
            skipBytes(4);   // Map ID
        }
    }
}

int NIFParser::keyValueSize(const char* valueType) {
    if (std::strcmp(valueType, "float") == 0) return 4;
    if (std::strcmp(valueType, "byte") == 0) return 1;
    if (std::strcmp(valueType, "Color4") == 0) return 16;
    if (std::strcmp(valueType, "Vector3") == 0) return 12;
    if (std::strcmp(valueType, "Quaternion") == 0) return 16;
    return 0;   // "string" and anything else are length prefixed
}

// Key: Time | Value [| Forward | Backward] [| TBC]. The arg carries the
// interpolation of the enclosing KeyGroup: 2 adds a tangent pair, 3 a TBC triple.
void NIFParser::skipKeys(uint32_t count, const char* valueType, int arg) {
    const bool isString = (std::strcmp(valueType, "string") == 0);
    const int valueBytes = isString ? 0 : keyValueSize(valueType);
    const int tangentBytes = (arg == 2) ? 2 * valueBytes : 0;
    const int tbcBytes = (arg == 3) ? 12 : 0;
    for (uint32_t i = 0; i < count && !readError; i++) {
        skipBytes(4);                     // Time
        if (isString) {
            skipStringArray(1);           // Value
        } else {
            skipBytes(valueBytes);        // Value
        }
        if (tangentBytes > 0) {
            skipBytes(tangentBytes);
        }
        if (tbcBytes > 0) {
            skipBytes(tbcBytes);
        }
    }
}

// QuatKey: Time | Value, plus TBC for rotation type 3. Rotation type 4 switches
// the whole array over to XYZ Euler KeyGroups and reads no quaternion at all.
void NIFParser::skipQuatKeys(uint32_t count, int rotationType) {
    if (rotationType == 4) {
        return;
    }
    for (uint32_t i = 0; i < count && !readError; i++) {
        skipBytes(4);       // Time
        skipBytes(16);      // Value (Quaternion)
        if (rotationType == 3) {
            skipBytes(12);  // TBC
        }
    }
}

// KeyGroup: Num Keys | Interpolation | Keys[Num Keys]. Num Keys == 0 leaves the
// interpolation unread, which matches the zero length array that follows.
void NIFParser::skipKeyGroup(const char* valueType) {
    const uint32_t numKeys = readUInt32();
    if (readError) {
        return;
    }
    int interpolation = 0;
    if (numKeys != 0) {
        interpolation = static_cast<int>(readUInt32());
        if (readError) {
            return;
        }
    }
    skipKeys(numKeys, valueType, interpolation);
}

// NiKeyframeData: rotation keys (quaternion or XYZ KeyGroups) | Translations
// | Scales.
void NIFParser::skipKeyframeData() {
    const uint32_t numRotationKeys = readUInt32();
    if (readError) {
        return;
    }
    int rotationType = 0;
    if (numRotationKeys != 0) {
        rotationType = static_cast<int>(readUInt32());
        if (readError) {
            return;
        }
    }
    skipQuatKeys(numRotationKeys, rotationType);
    if (rotationType == 4) {
        if (header.version <= 0x0A010000) {
            skipBytes(4);               // Order
        }
        for (int axis = 0; axis < 3 && !readError; axis++) {
            skipKeyGroup("float");      // XYZ Rotations
        }
    }
    skipKeyGroup("Vector3");            // Translations
    skipKeyGroup("float");              // Scales
}

// NiBSplineInterpolator family. Every member opens with the same four fields
// and only the tail differs, so the block type name picks the tail. Oblivion's
// .kf files use NiBSplineCompTransformInterpolator for the human and horse
// skeletons and NiTransformInterpolator for the creature skeletons.
void NIFParser::skipBSplineInterpolator(const std::string& typeName) {
    skipBytes(4 + 4 + 4 + 4);               // Start Time, Stop Time, Spline Data, Basis Data
    if (typeName == "NiBSplineCompTransformInterpolator" ||
        typeName == "NiBSplineTransformInterpolator") {
        skipBytes(32);                      // Transform
        skipBytes(4 + 4 + 4);               // Translation, Rotation, Scale Handle
        if (typeName == "NiBSplineCompTransformInterpolator") {
            skipBytes(4 * 6);               // Offset / Half Range for translation, rotation, scale
        }
    } else if (typeName == "NiBSplineCompPoint3Interpolator") {
        skipBytes(12 + 4);                  // Value, Handle
        skipBytes(4 + 4);                   // Position Offset, Position Half Range
    } else if (typeName == "NiBSplinePoint3Interpolator") {
        skipBytes(12 + 4);                  // Value, Handle
    } else if (typeName == "NiBSplineCompFloatInterpolator") {
        skipBytes(4 + 4);                   // Value, Handle
        skipBytes(4 + 4);                   // Float Offset, Float Half Range
    } else if (typeName == "NiBSplineFloatInterpolator") {
        skipBytes(4 + 4);                   // Value, Handle
    }
}

// NiBSplineData: Num Float Control Points | Float Control Points |
// Num Compact Control Points | Compact Control Points. The compact control
// points are 16 bit values that the interpolator's offset and half range
// expand back to the original floats.
void NIFParser::skipBSplineData() {
    const uint32_t numFloatControlPoints = readUInt32();
    if (readError) {
        return;
    }
    if (numFloatControlPoints > MAX_SPLINE_CONTROL_POINTS) {
        readError = true;
        return;
    }
    skipFloatArray(numFloatControlPoints);
    const uint32_t numCompactControlPoints = readUInt32();
    if (readError) {
        return;
    }
    if (numCompactControlPoints > MAX_SPLINE_CONTROL_POINTS) {
        readError = true;
        return;
    }
    skipU16Array(numCompactControlPoints);
}

// InterpBlendItem: Interpolator | Weight | Normalized Weight | Priority | Ease
// Spinner. The priority field narrows from an int to a byte at 10.1.0.110, so the
// item stride is 20 bytes below that cut-off and 17 from 10.1.0.110 onwards.
void NIFParser::skipInterpBlendItems(uint32_t count) {
    const size_t stride = (header.version <= 0x0A01006D) ? 20 : 17;
    skipBytes(static_cast<size_t>(count) * stride);
}

// NiBlendInterpolator. Three record layouts ship in the shipped archives: the
// 10.1.0.106/107 records keep 16 bit array and count fields, 10.1.0.108/109 add
// the single interpolator shortcut, 10.1.0.110/111 narrow those fields to bytes,
// and 10.1.0.112 leads with a flags byte and drops the array grow by field.
void NIFParser::skipBlendInterpolator(size_t valueSize) {
    const uint32_t version = header.version;
    if (version >= 0x0A010070) {        // 10.1.0.112 and later
        const uint8_t flags = readUInt8();
        const uint32_t arraySize = readUInt8();
        if (readError) {
            return;
        }
        skipBytes(4);                   // Weight Threshold
        if ((flags & 1) == 0) {
            skipBytes(1 + 1 + 1 + 1 + 4 + 4 + 4 + 4);
            skipInterpBlendItems(arraySize);
        }
        skipBytes(valueSize);           // Value
        return;
    }
    const uint32_t arraySize = (version >= 0x0A01006E) ? readUInt8() : readUInt16();
    if (version <= 0x0A01006D) {
        skipBytes(2);                   // Array Grow By
    }
    if (readError) {
        return;
    }
    skipInterpBlendItems(arraySize);
    skipBytes(1 + 4 + 1);               // Manager Controlled, Weight Threshold, Only Use Highest Weight
    if (version >= 0x0A01006E) {        // 10.1.0.110, 10.1.0.111
        skipBytes(1 + 1);               // Interp Count, Single Index
        skipBytes(4 + 4);               // Single Interpolator, Single Time
        skipBytes(1 + 1);               // High Priority, Next High Priority
    } else {
        skipBytes(2 + 2);               // Interp Count, Single Index
        if (version >= 0x0A01006C) {    // 10.1.0.108 and later
            skipBytes(4 + 4);           // Single Interpolator, Single Time
        }
        skipBytes(4 + 4);               // High Priority, Next High Priority
    }
    skipBytes(valueSize);               // Value
}

// NodeSet: Num Nodes | Nodes
void NIFParser::skipNodeSet() {
    const uint32_t numNodes = readUInt32();
    if (readError) {
        return;
    }
    skipPtrArray(numNodes);
}

// AVObject: Name | AV Object
void NIFParser::skipAVObjectArray(uint32_t count) {
    for (uint32_t i = 0; i < count && !readError; i++) {
        skipStringArray(1);
        skipBytes(4);
    }
}

// MatchGroup: Num Vertices | Vertex Indices
void NIFParser::skipMatchGroups(uint32_t count) {
    for (uint32_t i = 0; i < count && !readError; i++) {
        const uint32_t numVertices = readUInt16();
        if (readError) {
            return;
        }
        skipU16Array(numVertices);
    }
}

// FurniturePosition: Offset | Orientation | Position Ref 1 | Position Ref 2
void NIFParser::skipFurniturePositions(uint32_t count) {
    skipBytes(static_cast<size_t>(count) * 16);
}

// NiGeometryData: the shared vertex payload followed by the per-type tail.
void NIFParser::skipNiGeometryData(const std::string& typeName) {
    // Pre-5.0.0.1 meshes predate Group ID, Keep/Compress Flags and Consistency
    // Flags, order the flags as Bound -> Vertex Colors -> Data Flags -> Has UV,
    // and store every flag as a full uint32. Verified byte-exact against
    // marker_arrow.nif (4.0.0.2) and marker_radius.nif (3.3.0.13).
    if (!header.hasBlockTypeTable) {
        const uint32_t numVertices = readUInt16();
        if (readError) {
            return;
        }
        if (readBoolField() != 0) {
            skipVector3Array(numVertices);      // Vertices
        }
        if (readError) {
            return;
        }
        if (readBoolField() != 0) {
            skipVector3Array(numVertices);      // Normals
        }
        if (readError) {
            return;
        }
        skipBytes(16);                          // Bounding Sphere
        if (readBoolField() != 0) {
            skipColor4Array(numVertices);       // Vertex Colors
        }
        if (readError) {
            return;
        }
        const uint32_t legacyDataFlags = readUInt16();
        // Has UV is an explicit bool only up to and including 4.0.0.2. From
        // 4.1.0.1 the set count lives in Data Flags instead.
        if (header.version <= 0x04000002) {
            if (readBoolField() != 0) {
                skipBytes(static_cast<size_t>(numVertices) * 8);
            }
        } else {
            const uint32_t uvSets = legacyDataFlags & 63;
            if (uvSets != 0) {
                skipBytes(static_cast<size_t>(uvSets) * numVertices * 8);
            }
        }
        if (readError) {
            return;
        }
        if (typeName == "NiTriShapeData") {
            const uint32_t numTriangles = readUInt16();
            skipBytes(4);                       // Num Triangle Points
            skipBytes(static_cast<size_t>(numTriangles) * 6);
            const uint32_t numMatchGroups = readUInt16();
            if (readError) {
                return;
            }
            skipMatchGroups(numMatchGroups);
            return;
        }
        if (typeName == "NiTriStripsData") {
            skipBytes(2);                       // Num Triangles
            const uint32_t numStrips = readUInt16();
            if (readError) {
                return;
            }
            size_t totalPoints = 0;
            for (uint32_t i = 0; i < numStrips && !readError; i++) {
                totalPoints += readUInt16();
            }
            if (readError) {
                return;
            }
            skipBytes(totalPoints * 2);
        }
        return;
    }

    if (header.version >= 0x0A010072) {
        skipBytes(4);                               // Group ID (since 10.1.0.114)
    }
    const uint32_t numVertices = readUInt16();
    if (readError) {
        return;
    }
    if (header.version >= 0x0A010000) {
        skipBytes(1);                               // Keep Flags (since 10.1.0.0)
        skipBytes(1);                               // Compress Flags (since 10.1.0.0)
    }
    const uint8_t hasVertices = readUInt8();
    if (readError) {
        return;
    }
    if (hasVertices != 0) {
        skipVector3Array(numVertices);
    }

    const uint16_t dataFlags = readUInt16();
    const uint8_t hasNormals = readUInt8();
    if (readError) {
        return;
    }
    if (hasNormals != 0) {
        skipVector3Array(numVertices);              // Normals
        // BS Data Flags only exists on the 20.2+ streams, which these meshes
        // predate, so the tangent bit comes from Data Flags alone.
        if (header.version >= 0x0A010000 && (dataFlags & 0x1000) != 0) {
            skipVector3Array(numVertices);          // Tangents
            skipVector3Array(numVertices);          // Bitangents
        }
    }

    skipBytes(16);                                  // Bounding Sphere
    const uint8_t hasVertexColors = readUInt8();
    if (readError) {
        return;
    }
    if (hasVertexColors != 0) {
        skipColor4Array(numVertices);
    }
    skipBytes(static_cast<size_t>(dataFlags & 63) * numVertices * 8);   // UV Sets
    skipBytes(2);                                   // Consistency Flags
    if (header.version >= 0x14000004) {
        skipBytes(4);                               // Additional Data (since 20.0.0.4)
    }

    if (typeName == "NiTriShapeData") {
        const uint32_t numTriangles = readUInt16();
        skipBytes(4);                               // Num Triangle Points
        bool hasTriangles = true;                   // Triangles are unconditional until 10.0.1.2
        if (header.version >= 0x0A010000) {
            hasTriangles = readUInt8() != 0;        // Has Triangles (since 10.1.0.0)
            if (readError) {
                return;
            }
        }
        if (hasTriangles) {
            skipBytes(static_cast<size_t>(numTriangles) * 6);
        }
        const uint32_t numMatchGroups = readUInt16();
        if (readError) {
            return;
        }
        skipMatchGroups(numMatchGroups);
        return;
    }

    if (typeName == "NiTriStripsData") {
        skipBytes(2);                               // Num Triangles
        const uint32_t numStrips = readUInt16();
        if (readError) {
            return;
        }
        size_t totalPoints = 0;
        for (uint32_t i = 0; i < numStrips && !readError; i++) {
            totalPoints += readUInt16();
        }
        if (readError) {
            return;
        }
        const uint8_t hasPoints = header.version >= 0x0A010003 ? readUInt8() : 1;
        if (readError) {
            return;
        }
        if (hasPoints != 0) {
            skipBytes(totalPoints * 2);
        }
        return;
    }

    // NiPSysData
    uint8_t hasRadii = 0;
    if (header.version >= 0x0A010000) {
        hasRadii = readUInt8();                     // Has Radii (since 10.1.0.0)
        if (readError) {
            return;
        }
    } else {
        skipBytes(4);                               // Particle Radius (until 10.0.1.0)
    }
    if (hasRadii != 0) {
        skipFloatArray(numVertices);
    }
    skipBytes(2);                                   // Num Active
    const uint8_t hasSizes = readUInt8();
    if (readError) {
        return;
    }
    if (hasSizes != 0) {
        skipFloatArray(numVertices);
    }
    uint8_t hasRotations = 0;
    if (header.version >= 0x0A000100) {
        hasRotations = readUInt8();                 // Has Rotations (since 10.0.1.0)
        if (readError) {
            return;
        }
    }
    if (hasRotations != 0) {
        skipBytes(static_cast<size_t>(numVertices) * 16);   // Quaternion
    }
    // Has Rotation Angles and Has Rotation Axes only exist since 20.0.0.4, so
    // Oblivion meshes never carry them.
    if (header.version >= 0x14000004) {
        const uint8_t hasRotationAngles = readUInt8();
        if (readError) {
            return;
        }
        if (hasRotationAngles != 0) {
            skipFloatArray(numVertices);
        }
        const uint8_t hasRotationAxes = readUInt8();
        if (readError) {
            return;
        }
        if (hasRotationAxes != 0) {
            skipVector3Array(numVertices);
        }
    }
    // NiParticleInfo is 40 bytes until 10.4.0.1 because of Rotation Axis, and
    // 28 bytes afterwards.
    const size_t particleInfoStride = header.version <= 0x0A040001 ? 40 : 28;
    skipBytes(static_cast<size_t>(numVertices) * particleInfoStride);   // Particle Info
    if (header.version >= 0x14000002) {
        const uint8_t hasRotationSpeeds = readUInt8();   // Has Rotation Speeds (since 20.0.0.2)
        if (readError) {
            return;
        }
        if (hasRotationSpeeds != 0) {
            skipFloatArray(numVertices);
        }
    }
    skipBytes(2 + 2);                               // Num Added Particles, Base

    if (typeName == "NiMeshPSysData") {
        skipBytes(4);                               // Default Pool Size
        skipBytes(1);                               // Fill Pools On Load
        const uint32_t numGenerations = readUInt32();
        if (readError) {
            return;
        }
        skipBytes(static_cast<size_t>(numGenerations) * 4);   // Generations
        skipBytes(4);                               // Particle Meshes
    }
}

// SkinPartition: the vertex payload is sparse, so every optional block is
// gated by its own flag byte. Below 10.1.0.0 there are no flag bytes at all --
// the vertex map, the weight table and the face data are unconditional there
// (nif.xml SkinPartition).
void NIFParser::skipSkinPartition() {
    const uint32_t numVertices = readUInt16();
    const uint32_t numTriangles = readUInt16();
    const uint32_t numBones = readUInt16();
    const uint32_t numStrips = readUInt16();
    const uint32_t numWeightsPerVertex = readUInt16();
    if (readError) {
        return;
    }
    skipU16Array(numBones);

    if (header.version < 0x0A010000) {
        skipU16Array(numVertices);              // Vertex Map
        skipBytes(static_cast<size_t>(numVertices) * numWeightsPerVertex * 4);
        std::vector<uint32_t> legacyStripLengths(numStrips);
        for (uint32_t i = 0; i < numStrips && !readError; i++) {
            legacyStripLengths[i] = readUInt16();
        }
        if (readError) {
            return;
        }
        if (numStrips != 0) {
            for (uint32_t i = 0; i < numStrips && !readError; i++) {
                skipBytes(static_cast<size_t>(legacyStripLengths[i]) * 2);
            }
        } else {
            skipBytes(static_cast<size_t>(numTriangles) * 6);
        }
        const uint8_t legacyHasBoneIndices = readUInt8();
        if (readError) {
            return;
        }
        if (legacyHasBoneIndices != 0) {
            skipBytes(static_cast<size_t>(numVertices) * numWeightsPerVertex);
        }
        return;
    }

    const uint8_t hasVertexMap = readUInt8();
    if (readError) {
        return;
    }
    if (hasVertexMap != 0) {
        skipU16Array(numVertices);
    }
    const uint8_t hasVertexWeights = readUInt8();
    if (readError) {
        return;
    }
    if (hasVertexWeights != 0) {
        skipBytes(static_cast<size_t>(numVertices) * numWeightsPerVertex * 4);
    }
    std::vector<uint32_t> stripLengths(numStrips);
    for (uint32_t i = 0; i < numStrips && !readError; i++) {
        stripLengths[i] = readUInt16();
    }
    if (readError) {
        return;
    }
    const uint8_t hasFaces = readUInt8();
    if (readError) {
        return;
    }
    if (hasFaces != 0) {
        if (numStrips != 0) {
            for (uint32_t i = 0; i < numStrips && !readError; i++) {
                skipBytes(static_cast<size_t>(stripLengths[i]) * 2);
            }
        } else {
            skipBytes(static_cast<size_t>(numTriangles) * 6);
        }
    }
    const uint8_t hasBoneIndices = readUInt8();
    if (readError) {
        return;
    }
    if (hasBoneIndices != 0) {
        skipBytes(static_cast<size_t>(numVertices) * numWeightsPerVertex);
    }
}

// ControlledBlock. Records below 10.1.0.104 hold the target name and the
// controller reference only, 10.1.0.104/105 add the blend interpolator and
// index, and the controller ID strings run from 10.1.0.104 through 10.1.0.113.
// The name-in-palette offsets replace those strings between 10.2.0.0 and
// 20.1.0.0.
void NIFParser::skipControlledBlock() {
    const uint32_t version = header.version;
    if (version < VERSION_10_1_0_104) {
        skipStringArray(1);                     // Target Name
        skipBytes(4);                           // Controller
        return;
    }
    if (version < VERSION_10_1_0_106) {
        skipBytes(4);                           // Controller
    } else {
        skipBytes(4 + 4);                       // Interpolator, Controller
    }
    if (version <= VERSION_10_1_0_110) {
        skipBytes(4);                           // Blend Interpolator
        skipBytes(2);                           // Blend Index
    }
    if (version >= VERSION_10_1_0_106) {
        skipBytes(1);                           // Priority
    }
    if (version <= VERSION_10_1_0_113) {
        skipStringArray(5);                     // Node Name .. Interpolator ID
    } else if (version <= VERSION_20_1_0_0) {
        skipBytes(4 + 20);                      // String Palette, five name offsets
    } else {
        skipStringArray(5);                     // Node Name .. Interpolator ID
    }
}

// Morph: Frame Name (10.1.0.106 and later) | Num Keys, Interpolation and Keys
// (before 10.1.0.0) | Legacy Weight (10.1.0.104-20.1.0.2) | Vectors. The legacy
// weight follows the stream version, not the user version: the 10.1.0.106
// menus report BS version 5 and keep the weight, while the 20.0.0.4 meshes
// report 11 and go straight from the frame name to the vectors.
void NIFParser::skipMorph(uint32_t numVertices) {
    const uint32_t version = header.version;
    if (version >= VERSION_10_1_0_106) {
        skipStringArray(1);                     // Frame Name
    } else if (version < 0x0A010000) {
        const uint32_t numKeys = readUInt32();
        if (readError) {
            return;
        }
        int interpolation = 0;
        if (numKeys != 0) {
            interpolation = static_cast<int>(readUInt32());
            if (readError) {
                return;
            }
        }
        skipKeys(numKeys, "float", interpolation);
    }
    if (version >= VERSION_10_1_0_104 && bsVersion() < 10) {
        skipBytes(4);                           // Legacy Weight
    }
    skipVector3Array(numVertices);
}

// Measures one block body. Every branch consumes exactly the bytes nif.xml
// describes for version 20.0.0.4 with BS version 11.
bool NIFParser::walkBlockBody(const std::string& typeName) {
    // --- NiExtraData roots (Name only, no extra data list or controller) ----
    if (typeName == "NiStringExtraData") {
        skipStringArray(1);                     // Name
        skipStringArray(1);                     // String Data
    } else if (typeName == "NiBinaryExtraData") {
        skipStringArray(1);                     // Name
        skipStringArray(1);                     // Binary Data (ByteArray)
    } else if (typeName == "NiBooleanExtraData") {
        skipStringArray(1);                     // Name
        skipBytes(1);                           // Boolean Data
    } else if (typeName == "NiIntegerExtraData") {
        skipStringArray(1);                     // Name
        skipBytes(4);                           // Integer Data
    } else if (typeName == "BSXFlags") {
        skipStringArray(1);                     // Name
        skipBytes(4);                           // Integer Data
    } else if (typeName == "BSBound") {
        skipStringArray(1);                     // Name
        skipBytes(12 + 12);                     // Center, Dimensions
    } else if (typeName == "BSFurnitureMarker") {
        skipStringArray(1);                     // Name
        const uint32_t numPositions = readUInt32();
        if (readError) {
            return false;
        }
        skipFurniturePositions(numPositions);
    } else if (typeName == "NiTextKeyExtraData") {
        skipStringArray(1);                     // Name
        const uint32_t numTextKeys = readUInt32();
        if (readError) {
            return false;
        }
        skipKeys(numTextKeys, "string", 1);

    // --- NiObjectNET roots -------------------------------------------------
    } else if (typeName == "NiMaterialProperty") {
        skipNiObjectNET();
        if (header.version <= 0x0A000102) {
            skipBytes(2);                       // Flags (until 10.0.1.2)
        }
        skipBytes(4 * 12 + 4 + 4);              // Four colors, Glossiness, Alpha
    } else if (typeName == "NiAlphaProperty") {
        skipNiObjectNET();
        skipBytes(2 + 1);                       // Flags, Threshold
    } else if (typeName == "NiVertexColorProperty") {
        skipNiObjectNET();
        skipBytes(2 + 4 + 4);                   // Flags, Vertex Mode, Lighting Mode
    } else if (typeName == "NiZBufferProperty") {
        skipNiObjectNET();
        skipBytes(2);                           // Flags
        if (header.version >= 0x0401000C) {
            skipBytes(4);                       // Function (since 4.1.0.12)
        }
    } else if (typeName == "NiSpecularProperty") {
        skipNiObjectNET();
        skipBytes(2);                           // Flags
    } else if (typeName == "NiWireframeProperty") {
        skipNiObjectNET();
        skipBytes(2);                           // Flags
    } else if (typeName == "NiDitherProperty") {
        skipNiObjectNET();
        skipBytes(2);                           // Flags
    } else if (typeName == "NiFogProperty") {
        skipNiObjectNET();
        skipBytes(2 + 4 + 12);                  // Flags, Fog Depth, Fog Color
    } else if (typeName == "NiStencilProperty") {
        skipNiObjectNET();
        if (header.version <= 0x0A000102) {
            skipBytes(2);                       // Flags (until 10.0.1.2)
        }
        skipBytes(1 + 4 + 4 + 4 + 4 + 4 + 4 + 4);
    } else if (typeName == "NiSourceTexture") {
        skipNiObjectNET();
        const uint8_t useExternal = readUInt8();
        if (readError) {
            return false;
        }
        if (header.version < 0x0A010000) {
            uint8_t useInternal = 0;
            if (useExternal == 0) {
                useInternal = readUInt8();      // Use Internal (until 10.0.1.3)
                if (readError) {
                    return false;
                }
            } else {
                skipStringArray(1);             // File Name
            }
            if (useExternal == 0 && useInternal != 0) {
                skipBytes(4);                   // Pixel Data
            }
        } else {
            skipStringArray(1);                 // File Name
            skipBytes(4);                       // Pixel Data
        }
        skipBytes(12);                          // Format Prefs
        skipBytes(1);                           // Is Static
        if (header.version >= 0x0A010067) {
            skipBytes(1);                       // Direct Render (since 10.1.0.103)
        }
    } else if (typeName == "NiStringPalette") {
        skipStringArray(1);                     // Palette
        skipBytes(4);                           // Length
    } else if (typeName == "NiTexturingProperty") {
        skipNiObjectNET();
        if (header.version <= 0x0A000102) {
            skipBytes(2);                       // Flags (until 10.0.1.2)
        }
        skipBytes(4);                           // Apply Mode
        const uint32_t textureCount = readUInt32();
        if (readError) {
            return false;
        }
        for (int slot = 0; slot < 5 && !readError; slot++) {
            const uint8_t hasTexture = readUInt8();
            if (readError) {
                return false;
            }
            if (hasTexture != 0) {
                skipTexDesc();
            }
        }
        if (textureCount > 5) {
            const uint8_t hasBumpMap = readUInt8();
            if (readError) {
                return false;
            }
            if (hasBumpMap != 0) {
                skipTexDesc();
                skipBytes(4 + 4 + 16);          // Luma Scale, Luma Offset, Matrix22
            }
        }
        for (uint32_t slot = 6; slot <= 9 && !readError; slot++) {
            if (textureCount <= slot) {
                break;
            }
            const uint8_t hasDecal = readUInt8();
            if (readError) {
                return false;
            }
            if (hasDecal != 0) {
                skipTexDesc();
            }
        }
        const uint32_t numShaderTextures = readUInt32();
        if (readError) {
            return false;
        }
        skipShaderTexDescs(numShaderTextures);

    // --- NiAVObject roots --------------------------------------------------
    } else if (typeName == "NiNode" || typeName == "NiBillboardNode") {
        skipNiAVObject();
        const uint32_t childCount = readUInt32();
        if (readError) {
            return false;
        }
        skipRefArray(childCount);
        const uint32_t effectCount = readUInt32();
        if (readError) {
            return false;
        }
        skipRefArray(effectCount);
        if (typeName == "NiBillboardNode") {
            skipBytes(2);                       // Billboard Mode
        }
    } else if (typeName == "NiAmbientLight" || typeName == "NiDirectionalLight") {
        skipNiAVObject();
        skipNiLight();
    } else if (typeName == "NiPointLight") {
        skipNiAVObject();
        skipNiLight();
        skipBytes(4 * 3);                       // Constant, Linear, Quadratic
    } else if (typeName == "NiSpotLight") {
        skipNiAVObject();
        skipNiLight();
        skipBytes(4 * 3 + 4 + 4);               // Point light pair, Angle, Exponent
    } else if (typeName == "NiCamera") {
        skipNiAVObject();
        skipBytes(2);                           // Camera Flags
        skipBytes(6 * 4);                       // Frustum
        skipBytes(1);                           // Use Orthographic Projection
        skipBytes(4 * 4);                       // Viewport
        skipBytes(4 + 4 + 4 + 4);               // LOD Adjust, Scene, Screen counts
    } else if (typeName == "NiTriShape" || typeName == "NiTriStrips") {
        skipNiGeometry();
    } else if (typeName == "NiParticleSystem" || typeName == "NiMeshParticleSystem") {
        skipNiGeometry();
        skipBytes(1);                           // World Space
        const uint32_t numModifiers = readUInt32();
        if (readError) {
            return false;
        }
        skipRefArray(numModifiers);

    // --- Geometry data -----------------------------------------------------
    } else if (typeName == "NiTriShapeData" || typeName == "NiTriStripsData" ||
               typeName == "NiPSysData" || typeName == "NiMeshPSysData") {
        skipNiGeometryData(typeName);

    // --- NiTimeController roots --------------------------------------------
    } else if (typeName == "NiPSysUpdateCtlr") {
        skipNiTimeController();
    } else if (typeName == "NiKeyframeController") {
        // The interpolator link starts at 10.1.0.104 while the keyframe data
        // link stops at 10.1.0.103, so a file carries one, the other, or (in
        // the 10.1.0.104..10.1.0.108 window) both.
        skipNiInterpController();
        if (header.version >= 0x0A010068) {
            skipBytes(4);                       // Interpolator
        }
        if (header.version <= 0x0A010067) {
            skipBytes(4);                       // Data
        }
    } else if (typeName == "BSKeyframeController") {
        // NiKeyframeController plus a second keyframe data link (nif.xml).
        skipNiInterpController();
        if (header.version >= 0x0A010068) {
            skipBytes(4);                       // Interpolator
        }
        if (header.version <= 0x0A010067) {
            skipBytes(4);                       // Data
        }
        skipBytes(4);                           // Data 2
    } else if (typeName == "NiAlphaController" || typeName == "NiVisController" ||
               typeName == "NiTransformController") {
        skipNiInterpController();
        skipBytes(4);                           // Interpolator
    } else if (typeName == "NiPSysEmitterSpeedCtlr" ||
               typeName == "NiPSysEmitterDeclinationCtlr" ||
               typeName == "NiPSysEmitterInitialRadiusCtlr" ||
               typeName == "NiPSysModifierActiveCtlr" ||
               typeName == "NiPSysGravityStrengthCtlr" ||
               typeName == "NiPSysEmitterLifeSpanCtlr") {
        skipNiInterpController();
        skipBytes(4);                           // Interpolator
        skipStringArray(1);                     // Modifier Name
    } else if (typeName == "NiPSysResetOnLoopCtlr") {
        skipNiTimeController();
    } else if (typeName == "NiMaterialColorController") {
        skipNiInterpController();
        skipBytes(4 + 2);                       // Interpolator, Target Color (MaterialColor)
    } else if (typeName == "NiTextureTransformController") {
        skipNiInterpController();
        skipBytes(4 + 1 + 4 + 4);               // Interpolator, Shader Map, Slot, Operation
    } else if (typeName == "NiFlipController") {
        skipNiInterpController();
        skipBytes(4 + 4);                       // Interpolator, Texture Slot
        const uint32_t numSources = readUInt32();
        if (readError) {
            return false;
        }
        skipRefArray(numSources);
    } else if (typeName == "NiGeomMorpherController") {
        skipNiInterpController();
        skipBytes(2);                           // Morpher Flags
        skipBytes(4);                           // Data
        skipBytes(1);                           // Always Update
        const uint32_t numInterpolators = readUInt32();
        if (readError) {
            return false;
        }
        skipRefArray(numInterpolators);
        // The int list that 10.2.0.0 appends after the interpolator refs is
        // guarded by a BS version above 9. The 10.1.0.106 menus report 5 and
        // the 10.2.0.0 meshes 6 or 9, so only the 20.0.0.4 meshes carry it.
        if (header.version >= VERSION_10_2_0_0 && header.version < VERSION_20_0_0_5 &&
            bsVersion() > 9) {
            const uint32_t numUnknownInts = readUInt32();
            if (readError) {
                return false;
            }
            skipBytes(numUnknownInts * 4);
        }
    } else if (typeName == "NiMultiTargetTransformController") {
        skipNiInterpController();
        const uint32_t numExtraTargets = readUInt16();
        if (readError) {
            return false;
        }
        skipPtrArray(numExtraTargets);
    } else if (typeName == "NiBSBoneLODController") {
        skipNiTimeController();
        skipBytes(4);                           // LOD
        const uint32_t numLods = readUInt32();
        skipBytes(4);                           // Num Node Groups
        if (readError) {
            return false;
        }
        // Node Groups is sized by Num LODs rather than Num Node Groups.
        for (uint32_t i = 0; i < numLods && !readError; i++) {
            skipNodeSet();
        }
    } else if (typeName == "NiPSysEmitterCtlr") {
        skipNiInterpController();
        skipBytes(4);                           // Interpolator
        skipStringArray(1);                     // Modifier Name
        skipBytes(4);                           // Visibility Interpolator
    } else if (typeName == "NiControllerManager") {
        skipNiTimeController();
        skipBytes(1);                           // Cumulative
        const uint32_t numSequences = readUInt32();
        if (readError) {
            return false;
        }
        skipRefArray(numSequences);
        skipBytes(4);                           // Object Palette
    } else if (typeName == "bhkBlendController") {
        skipNiTimeController();
        skipBytes(4);                           // Keys

    // --- Interpolators -----------------------------------------------------
    } else if (typeName == "NiTransformInterpolator") {
        skipBytes(32);                          // Translation, Rotation, Scale
        if (header.version <= 0x0A01006D) {
            skipBytes(3);                       // TRS Valid: bool[3] (until 10.1.0.109)
        }
        skipBytes(4);                           // Data
    } else if (typeName == "NiFloatInterpolator") {
        skipBytes(4 + 4);                       // Value, Data
    } else if (typeName == "NiBoolInterpolator") {
        skipBytes(1 + 4);                       // Value, Data
    } else if (typeName == "NiPoint3Interpolator") {
        skipBytes(12 + 4);                      // Value, Data
    } else if (typeName == "NiBlendFloatInterpolator") {
        skipBlendInterpolator(4);
    } else if (typeName == "NiBlendBoolInterpolator") {
        skipBlendInterpolator(1);
    } else if (typeName == "NiBlendTransformInterpolator") {
        // The stored NiQuatTransform disappears at 10.1.0.110.
        skipBlendInterpolator(header.version <= 0x0A01006D ? 35 : 0);
    } else if (typeName == "NiBlendPoint3Interpolator") {
        skipBlendInterpolator(12);
    } else if (typeName == "NiBlendColorInterpolator") {
        skipBlendInterpolator(16);
    } else if (typeName == "NiBlendQuaternionInterpolator") {
        skipBlendInterpolator(16);
    } else if (typeName == "NiBoolTimelineInterpolator") {
        skipBytes(1 + 4);                       // Value, Data
    } else if (typeName == "NiPathInterpolator") {
        skipBytes(2 + 4 + 4 + 4 + 2 + 4 + 4);   // Flags, Bank, Angle, Smoothing, Axis, Path, Percent
    } else if (typeName == "NiBSplineInterpolator" ||
               typeName == "NiBSplineTransformInterpolator" ||
               typeName == "NiBSplineCompTransformInterpolator" ||
               typeName == "NiBSplinePoint3Interpolator" ||
               typeName == "NiBSplineCompPoint3Interpolator" ||
               typeName == "NiBSplineFloatInterpolator" ||
               typeName == "NiBSplineCompFloatInterpolator") {
        skipBSplineInterpolator(typeName);

    // --- Key data ----------------------------------------------------------
    } else if (typeName == "NiFloatData") {
        skipKeyGroup("float");
    } else if (typeName == "NiBoolData") {
        skipKeyGroup("byte");
    } else if (typeName == "NiColorData") {
        skipKeyGroup("Color4");
    } else if (typeName == "NiPosData") {
        skipKeyGroup("Vector3");
    } else if (typeName == "NiTransformData" || typeName == "NiKeyframeData") {
        skipKeyframeData();

    // --- Spline data -------------------------------------------------------
    } else if (typeName == "NiBSplineData") {
        skipBSplineData();
    } else if (typeName == "NiBSplineBasisData") {
        skipBytes(4);                           // Num Control Points

    // --- Animation payload -------------------------------------------------
    } else if (typeName == "NiMorphData") {
        const uint32_t numMorphs = readUInt32();
        const uint32_t numVertices = readUInt32();
        skipBytes(1);                           // Relative Targets
        if (readError) {
            return false;
        }
        for (uint32_t i = 0; i < numMorphs && !readError; i++) {
            skipMorph(numVertices);
        }
    } else if (typeName == "NiControllerSequence") {
        skipStringArray(1);                     // Name
        if (header.version < VERSION_10_1_0_106) {
            // Legacy NiSequence: the accum root name and the text keys replace
            // the weight, timing and manager block that 10.1.0.106 introduced,
            // and the controlled block array has no grow by field.
            skipStringArray(1);                 // Accum Root Name
            skipBytes(4);                       // Text Keys
            const uint32_t numLegacyBlocks = readUInt32();
            if (readError) {
                return false;
            }
            for (uint32_t i = 0; i < numLegacyBlocks && !readError; i++) {
                skipControlledBlock();
            }
        } else {
            const uint32_t numControlledBlocks = readUInt32();
            skipBytes(4);                       // Array Grow By
            if (readError) {
                return false;
            }
            for (uint32_t i = 0; i < numControlledBlocks && !readError; i++) {
                skipControlledBlock();
            }
            skipBytes(4 + 4 + 4 + 4);           // Weight, Text Keys, Cycle Type, Frequency
            if (header.version >= VERSION_10_1_0_106 && header.version <= VERSION_10_4_0_1) {
                skipBytes(4);                   // Phase
            }
            skipBytes(4 + 4);                   // Start Time, Stop Time
            if (header.version == VERSION_10_1_0_106) {
                skipBytes(1);                   // Play Backwards
            }
            skipBytes(4);                       // Manager
            skipStringArray(1);                 // Accum Root Name
            if (header.version >= VERSION_10_1_0_113 && header.version <= VERSION_20_1_0_0) {
                skipBytes(4);                   // String Palette
            }
        }
    } else if (typeName == "NiDefaultAVObjectPalette") {
        skipBytes(4);                           // Scene
        const uint32_t numObjs = readUInt32();
        if (readError) {
            return false;
        }
        skipAVObjectArray(numObjs);
    } else if (typeName == "NiSkinInstance") {
        // The partition link sits between Data and Skeleton Root, and only
        // exists since 10.1.0.101 (nif.xml). Below that the two references are
        // adjacent, so reading a partition link there shifts every following
        // field by four bytes.
        skipBytes(4);                           // Data
        if (header.version >= 0x0A010065) {
            skipBytes(4);                       // Skin Partition
        }
        skipBytes(4);                           // Skeleton Root
        const uint32_t numBones = readUInt32();
        if (readError) {
            return false;
        }
        skipPtrArray(numBones);
    } else if (typeName == "NiSkinData") {
        skipBytes(52);                          // Skin Transform
        const uint32_t numBones = readUInt32();
        // The partition link lives on this block only below 10.1.0.0, where the
        // skin instance has no partition of its own (nif.xml: since 4.0.0.2).
        if (header.version >= 0x04000002 && header.version < 0x0A010000) {
            skipBytes(4);                       // Skin Partition
        }
        // Has Vertex Weights defaults to true below 4.2.1.0, where the byte is
        // absent from the stream.
        uint8_t hasVertexWeights = 1;
        if (header.version >= 0x04020100) {
            hasVertexWeights = readUInt8();
        }
        if (readError) {
            return false;
        }
        for (uint32_t i = 0; i < numBones && !readError; i++) {
            skipBytes(52 + 16);                 // Bone skin transform, Bounding Sphere
            const uint32_t numVertices = readUInt16();
            if (readError) {
                return false;
            }
            if (hasVertexWeights != 0) {
                skipBytes(static_cast<size_t>(numVertices) * 6);   // BoneVertData
            }
        }
    } else if (typeName == "NiSkinPartition") {
        const uint32_t numPartitions = readUInt32();
        if (readError) {
            return false;
        }
        for (uint32_t i = 0; i < numPartitions && !readError; i++) {
            skipSkinPartition();
        }

    // --- Particle system modifiers -----------------------------------------
    } else if (typeName == "NiPSysAgeDeathModifier") {
        skipNiPSysModifier();
        skipBytes(1 + 4);                       // Spawn on Death, Spawn Modifier
    } else if (typeName == "NiPSysBoundUpdateModifier") {
        skipNiPSysModifier();
        skipBytes(2);                           // Update Skip
    } else if (typeName == "NiPSysGrowFadeModifier") {
        skipNiPSysModifier();
        skipBytes(4 + 2 + 4 + 2);               // Grow Time, Grow Generation, Fade pair
    } else if (typeName == "NiPSysPositionModifier") {
        skipNiPSysModifier();
    } else if (typeName == "NiPSysSpawnModifier") {
        skipNiPSysModifier();
        skipBytes(2 + 4 + 2 + 2 + 4 + 4 + 4 + 4);
    } else if (typeName == "NiPSysColorModifier") {
        skipNiPSysModifier();
        skipBytes(4);                           // Data
    } else if (typeName == "NiPSysColliderManager") {
        skipNiPSysModifier();
        skipBytes(4);                           // Collider
    } else if (typeName == "NiPSysGravityModifier") {
        skipNiPSysModifier();
        skipBytes(4 + 12 + 4 + 4 + 4 + 4 + 4);  // Object, Axis, Decay, Strength,
                                                // Force Type, Turbulence pair
    } else if (typeName == "NiPSysBombModifier") {
        skipNiPSysModifier();
        skipBytes(4 + 12 + 4 + 4 + 4 + 4);      // Object, Axis, Decay, Delta V,
                                                // Decay Type, Symmetry Type
    } else if (typeName == "NiPSysDragModifier") {
        skipNiPSysModifier();
        skipBytes(4 + 12 + 4 + 4 + 4);          // Object, Axis, Percentage, Range pair
    } else if (typeName == "NiPSysMeshUpdateModifier") {
        skipNiPSysModifier();
        const uint32_t numMeshes = readUInt32();
        if (readError) {
            return false;
        }
        skipPtrArray(numMeshes);
    } else if (typeName == "BSParentVelocityModifier") {
        skipNiPSysModifier();
        skipBytes(4);                           // Damping
    } else if (typeName == "NiPSysSphereEmitter") {
        skipNiPSysModifier();
        skipNiPSysEmitterBase(true);
        skipBytes(4);                           // Radius
    } else if (typeName == "NiPSysCylinderEmitter") {
        skipNiPSysModifier();
        skipNiPSysEmitterBase(true);
        skipBytes(4 + 4);                       // Radius, Height
    } else if (typeName == "BSPSysArrayEmitter") {
        skipNiPSysModifier();
        skipNiPSysEmitterBase(true);
    } else if (typeName == "NiPSysBoxEmitter") {
        skipNiPSysModifier();
        skipNiPSysEmitterBase(true);
        skipBytes(4 + 4 + 4);                   // Width, Height, Depth
    } else if (typeName == "NiPSysMeshEmitter") {
        skipNiPSysModifier();
        skipNiPSysEmitterBase(false);           // No emitter object on this one
        const uint32_t numEmitterMeshes = readUInt32();
        if (readError) {
            return false;
        }
        skipPtrArray(numEmitterMeshes);
        skipBytes(4 + 4 + 12);                  // Velocity Type, Emission Type, Axis
    } else if (typeName == "NiPSysRotationModifier") {
        skipNiPSysModifier();
        skipBytes(4 + 4 + 4 + 4 + 1 + 1 + 12);
    } else if (typeName == "NiPSysPlanarCollider") {
        // Derives from NiPSysCollider, not NiPSysModifier.
        skipBytes(4 + 1 + 1 + 4 + 4 + 4 + 4);   // Bounce .. Collider Object
        skipBytes(4 + 4 + 12 + 12);             // Width, Height, X Axis, Y Axis

    // --- Havok collision ---------------------------------------------------
    } else if (typeName == "bhkCollisionObject") {
        skipBytes(4 + 2 + 4);                   // Target, Flags, Body
    } else if (typeName == "bhkBlendCollisionObject") {
        skipBytes(4 + 2 + 4 + 4 + 4);           // plus Heir Gain, Vel Gain
        if (bsVersion() < 9) {
            skipBytes(4 + 4);                   // Unknown Float 1, Unknown Float 2
        }
    } else if (typeName == "bhkRigidBody" || typeName == "bhkRigidBodyT") {
        skipBytes(4);                           // Shape
        skipBytes(4);                           // Havok Filter
        skipBytes(4 + 1 + 3 + 12);              // World Object Info
        skipBytes(1 + 1 + 2);                   // Entity Info
        // The leading six fields and the three Max fields of the info struct
        // only exist from 10.1.0.0 on.
        skipBytes(header.version >= 0x0A010000 ? 196 : 168);   // bhkRigidBodyCInfo550_660
        const uint32_t numConstraints = readUInt32();
        if (readError) {
            return false;
        }
        skipRefArray(numConstraints);
        skipBytes(4);                           // Body Flags
    } else if (typeName == "bhkSPCollisionObject") {
        skipBytes(4 + 2 + 4);                   // Target, Flags, Body
    } else if (typeName == "bhkTransformShape" || typeName == "bhkConvexTransformShape") {
        skipBytes(4 + 4 + 4 + 8);               // Shape, Material, Radius, Unused 01
        skipBytes(64);                          // Transform
    } else if (typeName == "bhkConvexSweepShape") {
        skipBytes(4 + 4 + 4 + 12);              // Shape, Material, Radius, Unknown
    } else if (typeName == "bhkMultiSphereShape") {
        skipBytes(4 + 12);                      // Material, Shape Property
        const uint32_t numSpheres = readUInt32();
        if (readError) {
            return false;
        }
        skipBytes(static_cast<size_t>(numSpheres) * 16);   // NiBound
    } else if (typeName == "bhkSimpleShapePhantom") {
        skipBytes(4 + 4 + 20);                  // Shape, Havok Filter, World Object Info
        skipBytes(8);                           // Unused 01
        skipBytes(64);                          // Transform
    } else if (typeName == "bhkSphereShape") {
        skipBytes(4 + 4);                       // Material, Radius
    } else if (typeName == "bhkBoxShape") {
        skipBytes(4 + 4 + 8 + 12 + 4);
    } else if (typeName == "bhkCapsuleShape") {
        skipBytes(4 + 4 + 8 + 12 + 4 + 12 + 4);
    } else if (typeName == "bhkConvexVerticesShape") {
        skipBytes(4 + 4 + 12 + 12);             // Material, Radius, two properties
        const uint32_t numVertices = readUInt32();
        if (readError) {
            return false;
        }
        skipVector4Array(numVertices);
        const uint32_t numNormals = readUInt32();
        if (readError) {
            return false;
        }
        skipVector4Array(numNormals);
    } else if (typeName == "bhkNiTriStripsShape") {
        skipBytes(4 + 4 + 20 + 4);              // Material, Radius, Unused 01, Grow By
        if (header.version >= 0x0A010000) {
            skipBytes(16);                      // Scale (since 10.1.0.0)
        }
        const uint32_t numStripsData = readUInt32();
        if (readError) {
            return false;
        }
        skipRefArray(numStripsData);
        const uint32_t numFilters = readUInt32();
        if (readError) {
            return false;
        }
        skipBytes(static_cast<size_t>(numFilters) * 4);   // HavokFilter
    } else if (typeName == "bhkListShape") {
        const uint32_t numSubShapes = readUInt32();
        if (readError) {
            return false;
        }
        skipRefArray(numSubShapes);
        // A HavokMaterial leads with an extra word up to and including
        // 10.0.1.2: oar01 measures the sub-shape list as 56 bytes with eight
        // bytes of material, the 20.0.0.4 meshes only four.
        skipBytes((header.version <= 0x0A000102 ? 8 : 4) + 12 + 12);
        const uint32_t numFilters = readUInt32();
        if (readError) {
            return false;
        }
        skipBytes(static_cast<size_t>(numFilters) * 4);
    } else if (typeName == "bhkMeshShape") {
        // 10.0.1.0 only. ungrdltraphingedoor measures 204 bytes: eight bytes of
        // unknown words, the radius, two more, a scale, the shape property
        // array, three trailing words and a single strip data reference.
        skipBytes(8 + 4 + 8 + 16);
        const uint32_t numShapeProperties = readUInt32();
        if (readError) {
            return false;
        }
        skipBytes(static_cast<size_t>(numShapeProperties) * 12);
        skipBytes(12);                          // Unknown 03
        const uint32_t numStripsData = readUInt32();
        if (readError) {
            return false;
        }
        skipRefArray(numStripsData);
    } else if (typeName == "bhkMoppBvTreeShape") {
        // Below 10.1.0.0 the shape ref lives in the per-block prefix and the
        // MOPP code offset does not exist yet.
        if (header.version >= 0x0A010000) {
            skipBytes(4);                       // Shape
        }
        skipBytes(12 + 4);                      // Unused, Scale
        const uint32_t codeSize = readUInt32();
        if (readError) {
            return false;
        }
        if (header.version >= 0x0A010000) {
            skipBytes(16);                      // MOPP code offset
        }
        skipBytes(codeSize);                    // MOPP code data
    } else if (typeName == "bhkLimitedHingeConstraint") {
        skipBytes(16);                          // bhkConstraintCInfo
        skipBytes(124);                         // 7 x Vector4, Min/Max Angle, Friction
    } else if (typeName == "bhkHingeConstraint") {
        skipBytes(16);                          // bhkConstraintCInfo
        skipBytes(80);                          // 5 x Vector4
    } else if (typeName == "bhkPrismaticConstraint") {
        skipBytes(16);                          // bhkConstraintCInfo
        skipBytes(140);                         // 8 x Vector4, Min/Max, Friction
    } else if (typeName == "bhkStiffSpringConstraint") {
        skipBytes(16);                          // bhkConstraintCInfo
        skipBytes(36);                          // Pivot pair, Length
    } else if (typeName == "bhkRagdollConstraint") {
        skipBytes(16);                          // bhkConstraintCInfo
        skipBytes(120);                         // 6 x Vector4, 5 angles, Friction
    } else if (typeName == "bhkMalleableConstraint") {
        skipBytes(16);                          // bhkConstraintCInfo
        const uint32_t constraintType = readUInt32();
        if (readError) {
            return false;
        }
        skipBytes(16);                          // nested bhkConstraintCInfo
        switch (constraintType) {
            case 0: skipBytes(32); break;       // Ball and socket
            case 1: skipBytes(80); break;       // Hinge
            case 2: skipBytes(124); break;      // Limited hinge
            case 6: skipBytes(140); break;      // Prismatic
            case 7: skipBytes(120); break;      // Ragdoll
            case 8: skipBytes(36); break;       // Stiff spring
            default: break;
        }
        skipBytes(4 + 4);                       // Tau, Damping
    } else {
        return false;
    }

    return !readError;
}

// Leading word in front of a block body. Every 10.0.1.x and 10.1.0.x sample
// carries one except the NiCollisionObject subclasses, which start straight at
// their Target reference: floorplane01 measures bhkCollisionObject as ten bytes
// with the reference at the body start, and four extra bytes there derail the
// whole walk. The 10.2+ and 20.x meshes drop the prefix everywhere.
//
// Two shape types read their own leading word, so counting it as a prefix and
// again as body content would double it: bhkListShape starts at its Num Sub
// Shapes count (measured 8 in handscythe01 and 2 in oar01, matching each file's
// sub-shape groups) and bhkMeshShape at the first of its eight leading words.
uint32_t NIFParser::blockBodyPrefix(const std::string& typeName) const {
    if (typeName == "bhkCollisionObject" || typeName == "bhkBlendCollisionObject" ||
        typeName == "bhkSPCollisionObject") {
        return 0;
    }
    if (header.version <= 0x0A000102 &&
        (typeName == "bhkListShape" || typeName == "bhkMeshShape")) {
        return 0;
    }
    return blockPrefix;
}

// Walks every block body in order and records its byte range. Retail files have
// no block size table, so this is the only way to seek to a specific body. The
// walk ends by consuming the file trailer, so a caller can verify it by
// checking that the cursor landed exactly on the end of the file.
bool NIFParser::walkAllBlocks() {
    blockBodyOffsets.clear();
    blockBodyEnds.clear();
    blockPrefix = 0;
    blocksWalked = false;
    walkError.clear();

    const uint32_t blockCount = getBlockCount();
    if (blockCount == 0) {
        walkError = "no blocks in block table";
        return false;
    }

    // Meshes from the 10.0.1.x and 10.1.0.x lines prefix each block body with a
    // uint32 that reads as zero in every sample of the shipped corpus, so the
    // first field of the body sits four bytes after the recorded offset. The
    // 10.2+ and 20.x meshes drop the prefix.
    if (header.version >= 0x0A000100 && header.version <= 0x0A01006A) {
        blockPrefix = 4;
    }

    readError = false;
    blockBodyOffsets.reserve(blockCount);
    blockBodyEnds.reserve(blockCount);
    cursor = header.blockDataOffset;

    if (!header.hasBlockTypeTable) {
        header.blockTypeNames.clear();
        header.blockTypeIndices.clear();
    }

    for (uint32_t index = 0; index < blockCount; index++) {
        const size_t bodyStart = cursor;
        std::string typeName;
        if (header.hasBlockTypeTable) {
            typeName = getBlockTypeName(index);
        } else {
            // Every block of a pre-5.0.0.1 mesh opens with its own type name.
            if (!readString(typeName)) {
                walkError = "cannot read inline type name of block " + std::to_string(index);
                break;
            }
            header.blockTypeNames.push_back(typeName);
            header.blockTypeIndices.push_back(static_cast<uint16_t>(header.blockTypeNames.size() - 1));
            if (index < nodes.size()) {
                nodes[index]->blockTypeName = typeName;
                nodes[index]->blockTypeIndex = header.blockTypeIndices.back();
            }
        }
        if (!skipBytes(blockBodyPrefix(typeName))) {
            break;
        }
        if (!walkBlockBody(typeName)) {
            walkError = "cannot measure block " + std::to_string(index) + " (" +
                        (typeName.empty() ? "unknown" : typeName) + ")";
            break;
        }
        blockBodyOffsets.push_back(bodyStart);
        blockBodyEnds.push_back(cursor);
    }

    if (blockBodyOffsets.size() != blockCount) {
        if (walkError.empty()) {
            walkError = "walk stopped after " + std::to_string(blockBodyOffsets.size()) +
                        " of " + std::to_string(blockCount) + " blocks";
        }
        readError = true;
        return false;
    }

    if (!readTrailer()) {
        walkError = "unrecognised file trailer at offset " + std::to_string(cursor);
        readError = true;
        return false;
    }

    blocksWalked = true;
    return true;
}

// Every shipped mesh ends with a trailer: a uint32 value count followed by that
// many uint32 values. 8,030 of the 8,032 corpus meshes store one zero and the
// two magiceffects meshes store two values. Consuming it leaves the cursor
// exactly on the end of the file, which is how callers validate the walk.
//
// Eight landscape LOD tiles carry tool data between the trailer and a repeated
// trailer at the very end. The block table never references that data, so it is
// measured and skipped rather than parsed.
bool NIFParser::readTrailer() {
    trailerValueCount = 0;
    trailingDataSize = 0;

    const uint32_t valueCount = readUInt32();
    if (readError || valueCount > MAX_TRAILER_VALUES) {
        return false;
    }
    trailerValueCount = valueCount;
    if (!skipBytes(static_cast<size_t>(valueCount) * 4)) {
        return false;
    }
    if (cursor == fileBuffer.size()) {
        return true;
    }

    // Extra data is only accepted when the file still ends with a trailer that
    // repeats the same value count, so a truncated mesh keeps failing the walk.
    if (fileBuffer.size() - cursor < 8) {
        return false;
    }
    uint32_t tailCount = 0;
    uint32_t tailTerminator = 0;
    std::memcpy(&tailCount, fileBuffer.data() + fileBuffer.size() - 8, sizeof(uint32_t));
    std::memcpy(&tailTerminator, fileBuffer.data() + fileBuffer.size() - 4, sizeof(uint32_t));
    if (tailCount != valueCount || tailTerminator != 0) {
        return false;
    }

    trailingDataSize = fileBuffer.size() - cursor;
    cursor = fileBuffer.size();
    return true;
}

size_t NIFParser::getBlockBodyOffset(uint32_t index) const {
    if (index >= blockBodyOffsets.size()) {
        return 0;
    }
    return blockBodyOffsets[index];
}

size_t NIFParser::getBlockBodyEnd(uint32_t index) const {
    if (index >= blockBodyEnds.size()) {
        return 0;
    }
    return blockBodyEnds[index];
}

// Points at the first field of a block body, past the 10.1.0.x size prefix.
bool NIFParser::locateBlockBody(uint32_t index, size_t& offset) const {
    if (!blocksWalked || index >= blockBodyOffsets.size()) {
        return false;
    }
    offset = blockBodyOffsets[index] + blockBodyPrefix(getBlockTypeName(index));
    return offset <= blockBodyEnds[index];
}

bool NIFParser::findBlocksOfType(const std::string& typeName, std::vector<uint32_t>& out) const {
    out.clear();
    if (!blocksWalked) {
        return false;
    }
    for (uint32_t index = 0; index < getBlockCount(); index++) {
        if (getBlockTypeName(index) == typeName) {
            out.push_back(index);
        }
    }
    return !out.empty();
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

// Positions the cursor on the first field of the first block of a given type.
// Returns false when the file was not walked or holds no such block.
bool NIFParser::seekToBlockOfType(const std::string& typeName, uint32_t& blockIndex) {
    std::vector<uint32_t> blocks;
    if (!findBlocksOfType(typeName, blocks)) {
        LOGD("%s: no block of this type in file", typeName.c_str());
        return false;
    }
    blockIndex = blocks.front();

    size_t offset = 0;
    if (!locateBlockBody(blockIndex, offset)) {
        LOGE("%s: cannot locate block %u", typeName.c_str(), blockIndex);
        return false;
    }
    cursor = offset;
    return true;
}

bool NIFParser::seekToAnyBlockOfType(const std::vector<std::string>& typeNames,
                                     uint32_t& blockIndex) {
    bool found = false;
    for (const std::string& typeName : typeNames) {
        std::vector<uint32_t> blocks;
        if (!findBlocksOfType(typeName, blocks)) {
            continue;
        }
        if (!found || blocks.front() < blockIndex) {
            blockIndex = blocks.front();
            found = true;
        }
    }
    if (!found) {
        LOGD("no block of the requested types in file");
        return false;
    }

    size_t offset = 0;
    if (!locateBlockBody(blockIndex, offset)) {
        LOGE("cannot locate block %u", blockIndex);
        return false;
    }
    cursor = offset;
    return true;
}

bool NIFParser::parseNiSkinInstance(NIFSkinInstance& skin) {
    LOGD("Parsing NiSkinInstance...");

    // NiSkinInstance layout (nif.xml):
    // Data:Ref(4) | Skin Partition:Ref(4, since 10.1.0.101) | Skeleton Root:Ptr(4)
    // | Num Bones:uint(4) | Bones:Ptr[Num Bones](4 each)
    uint32_t blockIndex = 0;
    if (!seekToBlockOfType("NiSkinInstance", blockIndex)) {
        return false;
    }

    skin.skinDataIndex = readUInt32();
    if (header.version >= 0x0A010065) {
        skin.skinPartitionIndex = readUInt32();
    } else {
        skin.skinPartitionIndex = 0xFFFFFFFF;
    }
    skin.skeletonRootIndex = readUInt32();
    const uint32_t numBones = readUInt32();
    if (readError) {
        return false;
    }
    if (numBones > MAX_SKIN_BONES) {
        LOGE("NiSkinInstance: numBones %u exceeds limit", numBones);
        return false;
    }

    skin.boneNodeIndices.resize(numBones);
    for (uint32_t i = 0; i < numBones; i++) {
        skin.boneNodeIndices[i] = readUInt32();
    }
    if (readError) {
        return false;
    }

    LOGD("NiSkinInstance: skeletonRoot=%u, numBones=%u, skinPartition=%u, skinData=%u",
         skin.skeletonRootIndex, numBones, skin.skinPartitionIndex, skin.skinDataIndex);

    return true;
}

bool NIFParser::parseNiSkinData(NIFSkinData& skinData) {
    LOGD("Parsing NiSkinData...");

    uint32_t blockIndex = 0;
    if (!seekToBlockOfType("NiSkinData", blockIndex)) {
        return false;
    }
    return parseNiSkinDataAt(blockIndex, skinData);
}

bool NIFParser::parseNiSkinDataAt(uint32_t blockIndex, NIFSkinData& skinData) {
    size_t offset = 0;
    if (!locateBlockBody(blockIndex, offset)) {
        return false;
    }
    cursor = offset;

    // NiSkinData layout (nif.xml):
    // Skin Transform:NiTransform(52) | Skin Partition:Ref(4, 4.0.0.2..10.1.0.0)
    // | Num Bones:uint(4) | Has Vertex Weights:bool(1, since 4.2.1.0)
    // | Bone List:BoneData[Num Bones]
    // BoneData: NiTransform(52) | Bounding Sphere:NiBound(16) | Num Vertices:ushort(2)
    //           | Vertex Weights:BoneVertData[Num Vertices] (only when Has Vertex Weights)
    // BoneVertData: Index:ushort(2) | Weight:float(4)
    skinData.rootRotation = readMatrix3x3();
    skinData.rootTranslation = readVector3();
    skinData.rootScale = readFloat();

    // The partition link lives on this block only below 10.1.0.0; from there on
    // the skin instance owns it instead.
    if (header.version >= 0x04000002 && header.version < 0x0A010000) {
        skipBytes(4);                           // Skin Partition
    }

    skinData.numBones = readUInt32();
    if (readError) {
        return false;
    }
    if (skinData.numBones == 0 || skinData.numBones > MAX_SKIN_BONES) {
        LOGE("NiSkinData: numBones %u out of range", skinData.numBones);
        return false;
    }

    // Without this flag the bone list holds transforms only and every Num
    // Vertices field is followed directly by the next bone. The byte itself is
    // absent below 4.2.1.0, where the flag defaults to true.
    uint8_t hasVertexWeightsByte = 1;
    if (header.version >= 0x04020100) {
        hasVertexWeightsByte = readUInt8();
    }
    const bool hasVertexWeights = (hasVertexWeightsByte != 0);
    if (readError) {
        return false;
    }

    const size_t blockEnd = getBlockBodyEnd(blockIndex);
    skinData.boneData.clear();
    skinData.boneData.resize(skinData.numBones);

    for (uint32_t i = 0; i < skinData.numBones; i++) {
        NIFBoneData& bone = skinData.boneData[i];

        bone.skinTransform.rotation = readMatrix3x3();
        bone.skinTransform.translation = readVector3();
        bone.skinTransform.scale = readFloat();

        skipBytes(16);                          // Bounding Sphere: Center + Radius

        const uint16_t numVertices = readUInt16();
        if (readError) {
            return false;
        }

        if (!hasVertexWeights) {
            continue;
        }
        if (cursor + static_cast<size_t>(numVertices) * 6 > blockEnd) {
            LOGE("NiSkinData: bone %u claims %u vertices past block end", i, numVertices);
            return false;
        }

        bone.vertexWeights.resize(numVertices);
        for (uint16_t v = 0; v < numVertices; v++) {
            bone.vertexWeights[v].vertexIndex = readUInt16();
            bone.vertexWeights[v].weight = readFloat();
        }
        if (readError) {
            return false;
        }
    }

    LOGD("NiSkinData: numBones=%u, vertexWeights=%s, %zu/%zu bytes consumed",
         skinData.numBones, hasVertexWeights ? "yes" : "no",
         cursor - offset, blockEnd - offset);
    return true;
}

bool NIFParser::parseNiSkinPartition(NIFSkinPartition& partition) {
    LOGD("Parsing NiSkinPartition...");

    uint32_t blockIndex = 0;
    if (!seekToBlockOfType("NiSkinPartition", blockIndex)) {
        return false;
    }
    return parseNiSkinPartitionAt(blockIndex, partition);
}

bool NIFParser::parseNiSkinPartitionAt(uint32_t blockIndex, NIFSkinPartition& partition) {
    size_t offset = 0;
    if (!locateBlockBody(blockIndex, offset)) {
        return false;
    }
    cursor = offset;

    // NiSkinPartition: Num Partitions:uint(4) | Partitions:SkinPartition[N]
    // SkinPartition (nif.xml):
    //   Num Vertices:ushort | Num Triangles:ushort | Num Bones:ushort
    //   | Num Strips:ushort | Num Weights Per Vertex:ushort
    //   | Bones:ushort[Num Bones]
    //   | Has Vertex Map:bool(1) | Vertex Map:ushort[Num Vertices] (cond)
    //   | Has Vertex Weights:bool(1) | Vertex Weights:float[NV][NWPV] (cond)
    //   | Strip Lengths:ushort[Num Strips]                <- unconditional
    //   | Has Faces:bool(1)
    //   | Strips:ushort[Num Strips][Strip Lengths] (cond: Has Faces && Num Strips != 0)
    //   | Triangles:Triangle[Num Triangles]        (cond: Has Faces && Num Strips == 0)
    //   | Has Bone Indices:bool(1) | Bone Indices:byte[NV][NWPV] (cond)
    const uint32_t numPartitions = readUInt32();
    if (readError) {
        return false;
    }
    if (numPartitions > MAX_SKIN_PARTITIONS) {
        LOGE("NiSkinPartition: numPartitions %u exceeds limit", numPartitions);
        return false;
    }

    const size_t blockEnd = getBlockBodyEnd(blockIndex);
    partition.partitions.clear();
    partition.partitions.resize(numPartitions);

    uint32_t maxBones = 0;
    uint32_t maxWeights = 0;

    for (uint32_t p = 0; p < numPartitions; p++) {
        auto& part = partition.partitions[p];

        const uint16_t numVertices = readUInt16();
        const uint16_t numTriangles = readUInt16();
        const uint16_t numBones = readUInt16();
        const uint16_t numStrips = readUInt16();
        const uint16_t numWeightsPerVertex = readUInt16();
        if (readError) {
            return false;
        }
        if (numBones > MAX_SKIN_BONES || numWeightsPerVertex > MAX_WEIGHTS_PER_VERTEX) {
            LOGE("NiSkinPartition[%u]: %u bones / %u weights per vertex out of range",
                 p, numBones, numWeightsPerVertex);
            return false;
        }

        part.numVertices = numVertices;
        part.numTriangles = numTriangles;
        maxBones = std::max<uint32_t>(maxBones, numBones);
        maxWeights = std::max<uint32_t>(maxWeights, numWeightsPerVertex);

        part.bonePalette.bones.resize(numBones);
        for (uint16_t b = 0; b < numBones; b++) {
            part.bonePalette.bones[b] = readUInt16();
        }

        // Vertex map is dropped: the packed weights already cover every vertex.
        if (readUInt8() != 0) {
            if (cursor + static_cast<size_t>(numVertices) * 2 > blockEnd) {
                LOGE("NiSkinPartition[%u]: vertex map runs past block end", p);
                return false;
            }
            skipU16Array(numVertices);
        }

        if (readUInt8() != 0) {
            if (cursor + static_cast<size_t>(numVertices) * numWeightsPerVertex * 4 > blockEnd) {
                LOGE("NiSkinPartition[%u]: vertex weights run past block end", p);
                return false;
            }
            part.packedWeights.resize(numVertices);
            for (uint16_t v = 0; v < numVertices; v++) {
                for (uint16_t w = 0; w < numWeightsPerVertex; w++) {
                    const float weight = readFloat();
                    if (w < MAX_WEIGHTS_PER_VERTEX) {
                        part.packedWeights[v].weights[w] = weight;
                    }
                }
            }
        }

        // Strip Lengths is present for every partition, even when Num Strips is 0.
        std::vector<uint16_t> stripLengths(numStrips);
        for (uint16_t s = 0; s < numStrips; s++) {
            stripLengths[s] = readUInt16();
        }
        if (readError) {
            return false;
        }

        const bool hasFaces = (readUInt8() != 0);
        if (readError) {
            return false;
        }

        uint32_t totalIndices = 0;
        if (hasFaces) {
            if (numStrips != 0) {
                for (uint16_t s = 0; s < numStrips; s++) {
                    totalIndices += stripLengths[s];
                }
            } else {
                totalIndices = static_cast<uint32_t>(numTriangles) * 3;
            }
        }

        if (cursor + static_cast<size_t>(totalIndices) * 2 > blockEnd) {
            LOGE("NiSkinPartition[%u]: %u indices run past block end", p, totalIndices);
            return false;
        }
        part.indices.resize(totalIndices);
        for (uint32_t i = 0; i < totalIndices; i++) {
            part.indices[i] = readUInt16();
        }

        if (readUInt8() != 0) {
            if (cursor + static_cast<size_t>(numVertices) * numWeightsPerVertex > blockEnd) {
                LOGE("NiSkinPartition[%u]: bone indices run past block end", p);
                return false;
            }
            if (part.packedWeights.size() < numVertices) {
                part.packedWeights.resize(numVertices);
            }
            for (uint16_t v = 0; v < numVertices; v++) {
                for (uint16_t w = 0; w < numWeightsPerVertex; w++) {
                    const uint8_t boneIndex = readUInt8();
                    if (w < MAX_WEIGHTS_PER_VERTEX) {
                        part.packedWeights[v].boneIndices[w] = boneIndex;
                    }
                }
            }
        }
        if (readError) {
            return false;
        }
    }

    partition.maxBonesPerPartition = maxBones;
    partition.maxBonesPerVertex = maxWeights;

    LOGD("NiSkinPartition: numPartitions=%u, maxBones=%u, maxWeightsPerVertex=%u, %zu/%zu bytes consumed",
         numPartitions, maxBones, maxWeights, cursor - offset, blockEnd - offset);
    return true;
}

// ============================================
// Phase 30 Step 7: Animation Parsing
// ============================================

bool NIFParser::parseNiControllerManager(NIFControllerManager& manager) {
    LOGD("Parsing NiControllerManager...");

    // NiControllerManager layout (version 20.0.0.4):
    //   NiTimeController: Next Controller:Ref(4) | Flags:ushort(2) | Frequency:float(4)
    //                     | Phase:float(4) | Start Time:float(4) | Stop Time:float(4)
    //                     | Target:Ptr(4)
    //   Cumulative:bool(1)
    //   Num Controller Sequences:uint(4)
    //   Controller Sequences:Ref[N](4 each)
    //   Object Palette:Ref(4)              <- last field
    uint32_t blockIndex = 0;
    if (!seekToBlockOfType("NiControllerManager", blockIndex)) {
        return false;
    }

    const uint32_t nextController = readUInt32();
    const uint16_t flags = readUInt16();
    const float frequency = readFloat();
    const float phase = readFloat();
    const float startTime = readFloat();
    const float stopTime = readFloat();
    const uint32_t target = readUInt32();
    const bool cumulative = (readUInt8() != 0);
    manager.controllerSequenceCount = readUInt32();
    if (readError) {
        return false;
    }
    if (manager.controllerSequenceCount > MAX_CONTROLLER_SEQUENCES) {
        LOGE("NiControllerManager: %u sequences exceeds limit",
             manager.controllerSequenceCount);
        return false;
    }

    std::vector<uint32_t> sequenceRefs(manager.controllerSequenceCount);
    for (uint32_t i = 0; i < manager.controllerSequenceCount; i++) {
        sequenceRefs[i] = readUInt32();
    }
    manager.objectPaletteIndex = readUInt32();
    if (readError) {
        return false;
    }

    LOGD("  ControllerManager: next=%u flags=0x%04X freq=%.2f phase=%.2f start=%.2f stop=%.2f"
         " target=%u cumulative=%d sequences=%u palette=%u",
         nextController, flags, frequency, phase, startTime, stopTime,
         target, cumulative ? 1 : 0, manager.controllerSequenceCount,
         manager.objectPaletteIndex);

    // lastTime has no counterpart in this block; keep it as the sequence clock
    // origin so consumers do not read a stale value.
    manager.lastTime = 0.0f;

    // Each sequence is its own block, so resolve every reference by index.
    manager.sequences.clear();
    manager.sequences.reserve(manager.controllerSequenceCount);
    for (uint32_t i = 0; i < manager.controllerSequenceCount; i++) {
        NIFControllerSequence sequence;
        if (parseNiControllerSequenceAt(sequenceRefs[i], sequence)) {
            manager.sequences.push_back(std::move(sequence));
        } else {
            LOGW("NiControllerManager: sequence %u (block %u) failed to parse",
                 i, sequenceRefs[i]);
        }
    }

    LOGD("  ControllerManager: %zu/%u sequences parsed",
         manager.sequences.size(), manager.controllerSequenceCount);
    return true;
}

bool NIFParser::parseNiControllerSequence(NIFControllerSequence& sequence) {
    LOGD("Parsing NiControllerSequence...");

    uint32_t blockIndex = 0;
    if (!seekToBlockOfType("NiControllerSequence", blockIndex)) {
        return false;
    }
    return parseNiControllerSequenceAt(blockIndex, sequence);
}

bool NIFParser::parseNiControllerSequenceAt(uint32_t blockIndex, NIFControllerSequence& sequence) {
    size_t offset = 0;
    if (!locateBlockBody(blockIndex, offset)) {
        return false;
    }
    cursor = offset;
    const size_t blockEnd = getBlockBodyEnd(blockIndex);

    // NiControllerSequence layout (version 20.0.0.4):
    //   Name:SizedString(u32 length + bytes)
    //   Num Controlled Blocks:uint(4) | Array Grow By:uint(4)
    //   Controlled Blocks:ControlledBlock[N]
    //   Weight:float(4) | Text Keys:Ref(4) | Cycle Type:CycleType(4)
    //   Frequency:float(4) | Start Time:float(4) | Stop Time:float(4)
    //   Manager:Ptr(4) | Accum Root Name:string
    //   String Palette:Ref(4)
    // ControlledBlock:
    //   Interpolator:Ref(4) | Controller:Ref(4) | Priority:byte(1) | String Palette:Ref(4)
    //   | Node Name Offset(4) | Property Type Offset(4) | Controller Type Offset(4)
    //   | Controller ID Offset(4) | Interpolator ID Offset(4)
    // Play Backwards:bool only exists in version 10.1.0.106 and Phase only up to
    // 10.4.0.1, so neither is present here.
    if (!readString(sequence.name)) {
        return false;
    }
    const uint32_t numControlledBlocks = readUInt32();
    skipBytes(4);                               // Array Grow By
    if (readError) {
        return false;
    }
    if (numControlledBlocks > MAX_CONTROLLED_BLOCKS) {
        LOGE("NiControllerSequence: %u controlled blocks exceeds limit",
             numControlledBlocks);
        return false;
    }

    sequence.controlledBlocks.clear();
    sequence.controlledBlocks.resize(numControlledBlocks);
    for (uint32_t i = 0; i < numControlledBlocks; i++) {
        auto& cb = sequence.controlledBlocks[i];
        cb.interpolatorIndex = readUInt32();
        cb.controllerIndex = readUInt32();
        cb.priority = readUInt8();
        cb.stringPaletteIndex = readUInt32();
        cb.nodeNameOffset = readUInt32();
        cb.propertyTypeOffset = readUInt32();
        cb.controllerTypeOffset = readUInt32();
        cb.controllerIdOffset = readUInt32();
        cb.interpolatorIdOffset = readUInt32();

        // AnimationPlayer consumes keyframeDataIndex as an index into the
        // keyframe data array, which resolveControllerReferences() fills in.
        cb.keyframeDataIndex = cb.controllerIndex;
        cb.resolvedBoneIndex = -1;
    }

    skipBytes(4);                               // Weight
    const uint32_t textKeyBlock = readUInt32();
    const uint32_t cycleType = readUInt32();
    sequence.frequency = readFloat();
    sequence.startTime = readFloat();
    sequence.stopTime = readFloat();
    sequence.controllerManagerIndex = readUInt32();
    if (!readString(sequence.targetName)) {
        return false;
    }
    const uint32_t stringPalette = readUInt32();
    if (readError) {
        return false;
    }

    sequence.loop = (cycleType == 0);
    sequence.duration = sequence.stopTime - sequence.startTime;
    sequence.phase = 0.0f;                      // Phase is absent in 20.0.0.4

    // Text Keys is a reference to a NiTextKeyExtraData block, not inline data.
    // Reading it moves the cursor, so the sequence position is restored after.
    sequence.textKeys.clear();
    if (textKeyBlock != 0xFFFFFFFF && textKeyBlock != 0) {
        size_t textKeyOffset = 0;
        if (locateBlockBody(textKeyBlock, textKeyOffset)) {
            const size_t savedCursor = cursor;
            cursor = textKeyOffset;
            if (!parseNiTextKeyExtraData(sequence.textKeys)) {
                LOGW("NiControllerSequence '%s': text keys (block %u) failed to parse",
                     sequence.name.c_str(), textKeyBlock);
                sequence.textKeys.clear();
            }
            cursor = savedCursor;
        }
    }

    LOGD("  Sequence '%s': %.3f-%.3f (%.3f), cycle=%u, %u blocks, %zu textKeys,"
         " manager=%u, accumRoot='%s', palette=%u, %zu/%zu bytes",
         sequence.name.c_str(), sequence.startTime, sequence.stopTime, sequence.duration,
         cycleType, numControlledBlocks, sequence.textKeys.size(),
         sequence.controllerManagerIndex, sequence.targetName.c_str(), stringPalette,
         cursor - offset, blockEnd - offset);
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
    LOGD("Parsing bhkCollisionObject...");

    // bhkCollisionObject layout (version 20.0.0.4):
    //   Target:Ptr(4) | Flags:bhkCOFlags(2) | Body:Ref(4)
    // bhkBlendCollisionObject derives from it and appends:
    //   Heir Gain:float(4) | Vel Gain:float(4)
    // There is no body-filter array on this block; reading one used to consume
    // the first four bytes of the next block.
    uint32_t blockIndex = 0;
    if (!seekToAnyBlockOfType({"bhkCollisionObject", "bhkBlendCollisionObject"}, blockIndex)) {
        return false;
    }
    const bool isBlend = getBlockTypeName(blockIndex) == "bhkBlendCollisionObject";

    obj.nodeIndex = readUInt32();
    const uint16_t flags = readUInt16();
    obj.rigidBodyIndex = readUInt32();
    if (readError) {
        return false;
    }
    if (isBlend) {
        skipBytes(8);                           // Heir Gain + Vel Gain
        if (readError) {
            return false;
        }
    }

    // Resolve the referenced rigid body and shape so callers get usable
    // collision geometry without having to walk the block table themselves.
    uint32_t shapeIndex = 0;
    if (!parseBhkRigidBodyAt(obj.rigidBodyIndex, obj.bodyInfo, shapeIndex)) {
        LOGW("bhkCollisionObject: body block %u could not be read", obj.rigidBodyIndex);
    } else if (shapeIndex != 0xFFFFFFFF && !parseBhkShapeAt(shapeIndex, obj.shape)) {
        LOGW("bhkCollisionObject: shape block %u could not be read", shapeIndex);
    }

    // The Target is a block index; surface the node name when it is in range.
    const std::vector<std::shared_ptr<NIFNode>>& nodes = getNodes();
    if (obj.nodeIndex < nodes.size() && nodes[obj.nodeIndex]) {
        obj.targetName = nodes[obj.nodeIndex]->name;
    }

    LOGD("%s: target=%u (%s), flags=0x%04X, body=%u, shape=%u, shapeType=%d",
         isBlend ? "bhkBlendCollisionObject" : "bhkCollisionObject",
         obj.nodeIndex, obj.targetName.c_str(), flags, obj.rigidBodyIndex, shapeIndex,
         static_cast<int>(obj.shape.type));
    return true;
}

bool NIFParser::parseBhkRigidBody(RigidBodyInfo& info) {
    LOGD("Parsing bhkRigidBody...");

    // bhkRigidBodyT derives from bhkRigidBody without adding a single field.
    uint32_t blockIndex = 0;
    if (!seekToAnyBlockOfType({"bhkRigidBody", "bhkRigidBodyT"}, blockIndex)) {
        return false;
    }

    uint32_t shapeIndex = 0;
    return parseBhkRigidBodyAt(blockIndex, info, shapeIndex);
}

bool NIFParser::parseBhkRigidBodyAt(uint32_t blockIndex, RigidBodyInfo& info,
                                    uint32_t& shapeIndex) {
    size_t offset = 0;
    if (!locateBlockBody(blockIndex, offset)) {
        return false;
    }
    cursor = offset;
    const size_t blockEnd = getBlockBodyEnd(blockIndex);

    // bhkRigidBody layout (version 20.0.0.4):
    //   Shape:Ref(4)
    //   Havok Filter:HavokFilter(4)
    //   World Object Info:bhkWorldObjectCInfo(20)
    //     Unused01:uint(4) | BroadPhaseType:enum(1) | Unused02:byte[3]
    //     | Property:bhkWorldObjCInfoProperty(12)
    //   Entity Info:bhkEntityCInfo(4)
    //     Collision Response:enum(1) | Unused01:byte(1) | Process Contact Callback Delay:ushort(2)
    //   Rigid Body Info:bhkRigidBodyCInfo550_660(196)
    //   Num Constraints:uint(4) | Constraints:Ref[N](4 each) | Body Flags:uint(4)
    shapeIndex = readUInt32();

    // Havok Filter at the bhkWorldObject level carries the layer/group bits the
    // rest of the engine uses for collision filtering.
    const uint8_t layer = readUInt8();
    const uint8_t filterFlags = readUInt8();
    const uint16_t group = readUInt16();
    info.collisionGroup = layer;
    info.collisionFilter = group;
    info.isTrigger = (filterFlags & 0x80) != 0;

    skipBytes(20);                              // World Object Info
    skipBytes(4);                               // Entity Info

    // bhkRigidBodyCInfo550_660 (196 bytes)
    skipBytes(4);                               // Unused 01
    skipBytes(4);                               // Havok Filter (duplicate)
    skipBytes(4);                               // Unused 02
    const uint8_t collisionResponse = readUInt8();
    skipBytes(1);                               // Unused 03
    skipBytes(2);                               // Process Contact Callback Delay
    skipBytes(4);                               // Unused 04

    const NIFVector3 translation = readHKVector3();
    const NIFVector3 rotation = readHKVector3();          // hkQuaternion
    info.linearVelocity = readHKVector3();
    info.angularVelocity = readHKVector3();
    skipBytes(48);                              // Inertia Tensor (hkMatrix3)
    skipBytes(16);                              // Center (hkVector4)

    info.mass = readFloat();
    const float linearDamping = readFloat();
    const float angularDamping = readFloat();
    info.friction = readFloat();
    info.restitution = readFloat();
    const float maxLinearVelocity = readFloat();
    const float maxAngularVelocity = readFloat();
    const float penetrationDepth = readFloat();
    const uint8_t motionSystem = readUInt8();
    const uint8_t deactivatorType = readUInt8();
    const uint8_t solverDeactivation = readUInt8();
    const uint8_t qualityType = readUInt8();
    skipBytes(12);                              // Unused 05
    if (readError) {
        return false;
    }

    info.transform.translation = translation;
    (void)rotation;                             // Quaternion order needs the Havok convention

    const uint32_t numConstraints = readUInt32();
    if (readError) {
        return false;
    }
    if (numConstraints > MAX_RIGID_BODY_CONSTRAINTS) {
        LOGE("bhkRigidBody: %u constraints exceeds limit", numConstraints);
        return false;
    }
    skipRefArray(numConstraints);
    skipBytes(4);                               // Body Flags
    if (readError) {
        return false;
    }

    LOGD("bhkRigidBody: shape=%u, layer=%u, group=%u, mass=%.2f, friction=%.2f,"
         " restitution=%.2f, response=%u, motion=%u/%u/%u/%u, damping=%.3f/%.3f,"
         " vel=(%.2f, %.2f, %.2f), constraints=%u, %zu/%zu bytes",
         shapeIndex, layer, group, info.mass, info.friction, info.restitution,
         collisionResponse, motionSystem, deactivatorType, solverDeactivation, qualityType,
         linearDamping, angularDamping,
         info.linearVelocity.x, info.linearVelocity.y, info.linearVelocity.z,
         numConstraints, cursor - offset, blockEnd - offset);
    return true;
}

bool NIFParser::parseBhkShape(CollisionShape& shape) {
    LOGD("Parsing bhkShape...");

    // bhkShape is abstract. A bhkShape block can only be reached by block index,
    // and the concrete reader is chosen from the block's own type name.
    uint32_t blockIndex = 0;
    if (!seekToAnyBlockOfType({"bhkSphereShape", "bhkBoxShape", "bhkCapsuleShape",
                               "bhkConvexVerticesShape", "bhkNiTriStripsShape",
                               "bhkMoppBvTreeShape", "bhkListShape", "bhkMeshShape"},
                              blockIndex)) {
        LOGE("parseBhkShape: no bhkShape-derived block in file");
        return false;
    }
    return parseBhkShapeAt(blockIndex, shape);
}

bool NIFParser::parseBhkShapeAt(uint32_t blockIndex, CollisionShape& shape) {
    const std::string typeName = getBlockTypeName(blockIndex);

    size_t offset = 0;
    if (!locateBlockBody(blockIndex, offset)) {
        return false;
    }
    cursor = offset;
    const size_t blockEnd = getBlockBodyEnd(blockIndex);

    // Every bhk*Shape starts with Material:HavokMaterial(4) + Radius:float(4),
    // except bhkListShape, whose material follows its sub-shape array.
    bool ok = false;
    if (typeName == "bhkSphereShape") {
        ok = parseBhkSphereShape(shape);
    } else if (typeName == "bhkBoxShape") {
        ok = parseBhkBoxShape(shape);
    } else if (typeName == "bhkCapsuleShape") {
        ok = parseBhkCapsuleShape(shape);
    } else if (typeName == "bhkConvexVerticesShape") {
        ok = parseBhkConvexVerticesShape(shape);
    } else if (typeName == "bhkNiTriStripsShape") {
        ok = parseBhkNiTriStripsShape(shape);
    } else if (typeName == "bhkMoppBvTreeShape") {
        ok = parseBhkMoppBvTreeShape(shape);
    } else if (typeName == "bhkListShape") {
        ok = parseBhkListShape(shape);
    } else if (typeName == "bhkMeshShape") {
        ok = parseBhkMeshShape(shape);
    } else {
        LOGE("parseBhkShape: unsupported shape block type '%s'", typeName.c_str());
        return false;
    }

    if (!ok) {
        LOGE("parseBhkShape: '%s' reader failed", typeName.c_str());
        return false;
    }

    LOGD("bhkShape '%s': %zu/%zu bytes consumed",
         typeName.c_str(), cursor - offset, blockEnd - offset);
    return true;
}

bool NIFParser::parseBhkBoxShape(CollisionShape& shape) {
    LOGD("Parsing bhkBoxShape...");

    // bhkBoxShape layout:
    //   Material:HavokMaterial(4) | Radius:float(4) | Unused01:byte[8]
    //   | Dimensions:hkVector4(16) | Unused Float(4)
    shape.type = CollisionShapeType::Box;

    skipBytes(4);                               // Material
    shape.radius = readFloat();
    skipBytes(8);                               // Unused 01
    const NIFVector3 dimensions = readHKVector3();
    skipBytes(4);                               // Unused Float
    if (readError) {
        return false;
    }
    shape.halfExtents = dimensions;

    LOGD("bhkBoxShape: halfExtents=(%.2f, %.2f, %.2f), radius=%.2f",
         dimensions.x, dimensions.y, dimensions.z, shape.radius);
    return true;
}

bool NIFParser::parseBhkSphereShape(CollisionShape& shape) {
    LOGD("Parsing bhkSphereShape...");

    // bhkSphereShape layout: Material:HavokMaterial(4) | Radius:float(4)
    shape.type = CollisionShapeType::Sphere;

    skipBytes(4);                               // Material
    shape.radius = readFloat();
    if (readError) {
        return false;
    }

    LOGD("bhkSphereShape: radius=%.2f", shape.radius);
    return true;
}

bool NIFParser::parseBhkCapsuleShape(CollisionShape& shape) {
    LOGD("Parsing bhkCapsuleShape...");

    // bhkCapsuleShape layout:
    //   Material:HavokMaterial(4) | Radius:float(4) | Unused01:byte[8]
    //   | First Point:hkVector4(16) | Radius1:float(4)
    //   | Second Point:hkVector4(16) | Radius2:float(4)
    shape.type = CollisionShapeType::Capsule;

    skipBytes(4);                               // Material
    shape.radius = readFloat();
    skipBytes(8);                               // Unused 01
    const NIFVector3 firstPoint = readHKVector3();
    skipBytes(4);                               // Radius 1
    const NIFVector3 secondPoint = readHKVector3();
    skipBytes(4);                               // Radius 2
    if (readError) {
        return false;
    }

    const float dx = secondPoint.x - firstPoint.x;
    const float dy = secondPoint.y - firstPoint.y;
    const float dz = secondPoint.z - firstPoint.z;
    shape.height = sqrtf(dx * dx + dy * dy + dz * dz);
    shape.center = {(firstPoint.x + secondPoint.x) * 0.5f,
                    (firstPoint.y + secondPoint.y) * 0.5f,
                    (firstPoint.z + secondPoint.z) * 0.5f};

    LOGD("bhkCapsuleShape: radius=%.2f, height=%.2f", shape.radius, shape.height);
    return true;
}

bool NIFParser::parseBhkConvexVerticesShape(CollisionShape& shape) {
    LOGD("Parsing bhkConvexVerticesShape...");

    // bhkConvexVerticesShape layout:
    //   Material:HavokMaterial(4) | Radius:float(4)
    //   | Vertices Property:bhkWorldObjCInfoProperty(12)
    //   | Normals Property:bhkWorldObjCInfoProperty(12)
    //   | Num Vertices:uint(4) | Vertices:hkVector4[N](16 each)
    //   | Num Normals:uint(4) | Normals:hkVector4[N](16 each)
    shape.type = CollisionShapeType::ConvexHull;

    skipBytes(4);                               // Material
    shape.radius = readFloat();
    skipBytes(24);                              // Vertices + Normals properties

    const uint32_t numVertices = readUInt32();
    if (readError) {
        return false;
    }
    if (numVertices > MAX_CONVEX_VERTICES) {
        LOGE("bhkConvexVerticesShape: %u vertices exceeds limit", numVertices);
        return false;
    }

    shape.vertices.clear();
    shape.vertices.reserve(numVertices);
    for (uint32_t i = 0; i < numVertices; i++) {
        shape.vertices.push_back(readHKVector3());
    }
    if (readError) {
        return false;
    }

    const uint32_t numNormals = readUInt32();
    if (readError) {
        return false;
    }
    if (numNormals > MAX_CONVEX_VERTICES) {
        LOGE("bhkConvexVerticesShape: %u normals exceeds limit", numNormals);
        return false;
    }
    skipBytes(static_cast<size_t>(numNormals) * 16);
    if (readError) {
        return false;
    }

    LOGD("bhkConvexVerticesShape: %u vertices, %u normals, radius=%.2f",
         numVertices, numNormals, shape.radius);
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

bool NIFParser::parseBhkNiTriStripsShape(CollisionShape& shape) {
    LOGD("Parsing bhkNiTriStripsShape...");

    // bhkNiTriStripsShape layout:
    //   Material:HavokMaterial(4) | Radius:float(4) | Unused01:byte[20]
    //   | Grow By:uint(4) | Scale:hkVector4(16)
    //   | Num Strips Data:uint(4) | Strips Data:Ref[N](4 each)
    //   | Num Filters:uint(4) | Filters:HavokFilter[N](4 each)
    // The strip vertices live in separate bhkNiTriStripsData blocks; the strip
    // refs are recorded so a caller can resolve them through the block table.
    shape.type = CollisionShapeType::TriMesh;

    skipBytes(4);                               // Material
    shape.radius = readFloat();
    skipBytes(20);                              // Unused 01
    skipBytes(4);                               // Grow By
    skipBytes(16);                              // Scale

    const uint32_t numStripsData = readUInt32();
    if (readError) {
        return false;
    }
    if (numStripsData > MAX_STRIPS_DATA) {
        LOGE("bhkNiTriStripsShape: %u strip blocks exceeds limit", numStripsData);
        return false;
    }
    shape.stripsDataRefs.clear();
    shape.stripsDataRefs.reserve(numStripsData);
    for (uint32_t i = 0; i < numStripsData; i++) {
        shape.stripsDataRefs.push_back(readUInt32());
    }

    const uint32_t numFilters = readUInt32();
    if (readError) {
        return false;
    }
    if (numFilters > MAX_SHAPE_FILTERS) {
        LOGE("bhkNiTriStripsShape: %u filters exceeds limit", numFilters);
        return false;
    }
    skipBytes(static_cast<size_t>(numFilters) * 4);
    if (readError) {
        return false;
    }

    LOGD("bhkNiTriStripsShape: radius=%.2f, strips=%u, filters=%u",
         shape.radius, numStripsData, numFilters);
    return true;
}

bool NIFParser::parseBhkListShape(CollisionShape& shape) {
    LOGD("Parsing bhkListShape...");

    // bhkListShape layout:
    //   Num Sub Shapes:uint(4) | Sub Shapes:Ref[N](4 each) | Material:HavokMaterial(4)
    //   | Child Shape Property:bhkWorldObjCInfoProperty(12)
    //   | Child Filter Property:bhkWorldObjCInfoProperty(12)
    //   | Num Filters:uint(4) | Filters:HavokFilter[N](4 each)
    shape.type = CollisionShapeType::List;

    const uint32_t numSubShapes = readUInt32();
    if (readError) {
        return false;
    }
    if (numSubShapes > MAX_LIST_SUB_SHAPES) {
        LOGE("bhkListShape: %u sub shapes exceeds limit", numSubShapes);
        return false;
    }
    shape.subShapeRefs.clear();
    shape.subShapeRefs.reserve(numSubShapes);
    for (uint32_t i = 0; i < numSubShapes; i++) {
        shape.subShapeRefs.push_back(readUInt32());
    }

    skipBytes(4);                               // Material
    skipBytes(12);                              // Child Shape Property
    skipBytes(12);                              // Child Filter Property

    const uint32_t numFilters = readUInt32();
    if (readError) {
        return false;
    }
    if (numFilters > MAX_SHAPE_FILTERS) {
        LOGE("bhkListShape: %u filters exceeds limit", numFilters);
        return false;
    }
    skipBytes(static_cast<size_t>(numFilters) * 4);
    if (readError) {
        return false;
    }

    LOGD("bhkListShape: %u sub shapes, %u filters", numSubShapes, numFilters);
    return true;
}

bool NIFParser::parseBhkMoppBvTreeShape(CollisionShape& shape) {
    LOGD("Parsing bhkMoppBvTreeShape...");

    // bhkMoppBvTreeShape layout:
    //   Shape:Ref(4) | Unused01:byte[12] | Scale:float(4)
    //   | MOPP Code:hkpMoppCode { Data Size:uint(4) | Offset:hkVector4(16)
    //                            | Data:byte[Data Size] }
    shape.type = CollisionShapeType::MoppBvTree;

    shape.childShapeRef = readUInt32();
    skipBytes(12);                              // Unused 01
    const float scale = readFloat();
    const uint32_t moppDataSize = readUInt32();
    skipBytes(16);                              // Offset (hkVector4)
    if (readError) {
        return false;
    }
    if (moppDataSize > MAX_MOPP_DATA_SIZE) {
        LOGE("bhkMoppBvTreeShape: %u byte MOPP code exceeds limit", moppDataSize);
        return false;
    }

    shape.moppDataSize = moppDataSize;
    shape.moppData.resize(moppDataSize);
    if (moppDataSize > 0 && !readBytes(reinterpret_cast<char*>(shape.moppData.data()),
                                       moppDataSize)) {
        return false;
    }

    LOGD("bhkMoppBvTreeShape: child=%u, scale=%.3f, moppDataSize=%u",
         shape.childShapeRef, scale, moppDataSize);
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
