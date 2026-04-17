#include "nif_parser.h"
#include <fstream>
#include <cstring>
#include <android/log.h>

#define LOG_TAG "NifParser"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

NifParser::NifParser() {}

NifParser::~NifParser() {
    meshes.clear();
}

bool NifParser::parseFile(const std::string& filePath) {
    LOGD("Parsing NIF file: %s", filePath.c_str());

    std::vector<uint8_t> fileData;
    if (!readFileToMemory(filePath, fileData)) {
        lastError = "Failed to read file: " + filePath;
        LOGE("%s", lastError.c_str());
        return false;
    }

    LOGD("File size: %zu bytes", fileData.size());

    size_t offset = 0;

    // Parse header
    NifFileHeader header;
    if (!parseHeader(fileData, header, offset)) {
        lastError = "Failed to parse NIF header";
        LOGE("%s", lastError.c_str());
        return false;
    }

    LOGD("NIF Header Version: 0x%08X, User Version: %u", header.headerVersion, header.userVersion);

    // Parse object array
    if (!parseObjectArray(fileData, offset)) {
        lastError = "Failed to parse NIF objects";
        LOGE("%s", lastError.c_str());
        return false;
    }

    LOGD("Successfully parsed NIF file. Extracted %zu meshes", meshes.size());
    return true;
}

bool NifParser::readFileToMemory(const std::string& filePath, std::vector<uint8_t>& data) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOGE("Cannot open file: %s", filePath.c_str());
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    data.resize(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        LOGE("Failed to read file data");
        return false;
    }

    file.close();
    return true;
}

uint32_t NifParser::readUInt32(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 4 > data.size()) {
        LOGE("Buffer overflow: trying to read uint32 at offset %zu", offset);
        return 0;
    }

    uint32_t value = 0;
    value |= (static_cast<uint32_t>(data[offset + 0]) << 0);
    value |= (static_cast<uint32_t>(data[offset + 1]) << 8);
    value |= (static_cast<uint32_t>(data[offset + 2]) << 16);
    value |= (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;
    return value;
}

uint16_t NifParser::readUInt16(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 2 > data.size()) {
        LOGE("Buffer overflow: trying to read uint16 at offset %zu", offset);
        return 0;
    }

    uint16_t value = 0;
    value |= (static_cast<uint16_t>(data[offset + 0]) << 0);
    value |= (static_cast<uint16_t>(data[offset + 1]) << 8);
    offset += 2;
    return value;
}

uint8_t NifParser::readUInt8(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 1 > data.size()) {
        LOGE("Buffer overflow: trying to read uint8 at offset %zu", offset);
        return 0;
    }

    uint8_t value = data[offset];
    offset += 1;
    return value;
}

float NifParser::readFloat(const std::vector<uint8_t>& data, size_t& offset) {
    if (offset + 4 > data.size()) {
        LOGE("Buffer overflow: trying to read float at offset %zu", offset);
        return 0.0f;
    }

    uint32_t intValue = 0;
    intValue |= (static_cast<uint32_t>(data[offset + 0]) << 0);
    intValue |= (static_cast<uint32_t>(data[offset + 1]) << 8);
    intValue |= (static_cast<uint32_t>(data[offset + 2]) << 16);
    intValue |= (static_cast<uint32_t>(data[offset + 3]) << 24);
    offset += 4;

    float value;
    std::memcpy(&value, &intValue, sizeof(float));
    return value;
}

NifVector3 NifParser::readVector3(const std::vector<uint8_t>& data, size_t& offset) {
    return NifVector3{
        readFloat(data, offset),
        readFloat(data, offset),
        readFloat(data, offset)
    };
}

NifColor4 NifParser::readColor4(const std::vector<uint8_t>& data, size_t& offset) {
    return NifColor4{
        readFloat(data, offset),
        readFloat(data, offset),
        readFloat(data, offset),
        readFloat(data, offset)
    };
}

NifString NifParser::readString(const std::vector<uint8_t>& data, size_t& offset) {
    NifString result;
    result.length = readUInt32(data, offset);

    if (result.length == 0) {
        return result;
    }

    if (offset + result.length > data.size()) {
        LOGE("Buffer overflow: trying to read string of length %u at offset %zu", result.length, offset);
        result.length = 0;
        return result;
    }

    result.value.assign(data.begin() + offset, data.begin() + offset + result.length);
    offset += result.length;
    return result;
}

bool NifParser::parseHeader(const std::vector<uint8_t>& data, NifFileHeader& header, size_t& offset) {
    if (data.size() < 30) {
        LOGE("File too small for NIF header");
        return false;
    }

    // Read magic string (30 bytes)
    std::memcpy(header.magic, &data[offset], 30);
    offset += 30;

    // Verify magic number
    if (std::strncmp(header.magic, NIF_MAGIC_NUMBER, 30) != 0) {
        LOGE("Invalid NIF magic number");
        return false;
    }

    // Read version info
    header.headerVersion = readUInt32(data, offset);
    header.userVersion = readUInt32(data, offset);
    header.userVersion2 = readUInt32(data, offset);

    LOGD("Header parsed successfully. Version: 0x%08X", header.headerVersion);
    return true;
}

bool NifParser::parseObjectArray(const std::vector<uint8_t>& data, size_t& offset) {
    // Read number of objects (in simplified version, we'll create a default mesh)
    // In a full implementation, this would parse the entire object tree

    // Create a simple placeholder mesh
    NifMeshData placeholderMesh;
    placeholderMesh.name = "NIF_Mesh_0";

    // Add simple cube geometry as placeholder
    placeholderMesh.vertices = {
        {-1, -1, 1},   // 0
        {1, -1, 1},    // 1
        {1, 1, 1},     // 2
        {-1, 1, 1},    // 3
        {-1, -1, -1},  // 4
        {1, -1, -1},   // 5
        {1, 1, -1},    // 6
        {-1, 1, -1}    // 7
    };

    placeholderMesh.indices = {
        0, 1, 2, 2, 3, 0,  // Front
        4, 6, 5, 6, 4, 7,  // Back
        8, 9, 10, 10, 11, 8,  // This is placeholder
        12, 14, 13, 14, 12, 15,  // More placeholder
        0, 1, 2, 2, 3, 0,  // Repeat for now
        4, 5, 6, 6, 7, 4   // Repeat for now
    };

    placeholderMesh.colors.resize(placeholderMesh.vertices.size(), {1.0f, 1.0f, 1.0f, 1.0f});
    placeholderMesh.texturePath = "";

    meshes.push_back(placeholderMesh);

    LOGD("Object array parsing completed. Created placeholder mesh");
    return true;
}
