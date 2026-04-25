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
    if (blockName == "NiNode") return NIFBlockType::NiNode;
    if (blockName == "NiTriShape") return NIFBlockType::NiTriShape;
    if (blockName == "NiTriStrips") return NIFBlockType::NiTriStrips;
    if (blockName == "NiTexturingProperty") return NIFBlockType::NiTexturingProperty;
    if (blockName == "NiMaterialProperty") return NIFBlockType::NiMaterialProperty;

    return NIFBlockType::Unknown;
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
