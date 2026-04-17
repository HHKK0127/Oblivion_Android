#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

// NIF File Format Constants
#define NIF_MAGIC_NUMBER "Gamebryo File Format, Version"

// NIF Object Types (simplified - only mesh-relevant types)
enum class NifObjectType {
    NiNode = 0,
    NiTriShape = 1,
    NiTriStrips = 2,
    NiGeometry = 3,
    NiProperty = 4,
    NiTexturingProperty = 5,
    NiMaterialProperty = 6,
    Unknown = 255
};

// Data structures for NIF file format
struct NifFileHeader {
    char magic[30];           // "Gamebryo File Format, Version"
    uint32_t headerVersion;   // E.g., 0x14020001 for NIF 12.0
    uint32_t userVersion;     // Game-specific (0 for Oblivion base)
    uint32_t userVersion2;    // Additional version info
};

struct NifVector3 {
    float x, y, z;
};

struct NifColor3 {
    float r, g, b;
};

struct NifColor4 {
    float r, g, b, a;
};

struct NifString {
    uint32_t length;
    std::string value;
};

// Simple mesh data extracted from NIF
struct NifMeshData {
    std::string name;
    std::vector<NifVector3> vertices;
    std::vector<NifColor4> colors;
    std::vector<uint16_t> indices;
    std::vector<NifVector3> normals;
    std::vector<std::pair<float, float>> texCoords;  // UV coordinates
    std::string texturePath;

    NifMeshData() = default;
    ~NifMeshData() = default;
};

class NifParser {
public:
    NifParser();
    ~NifParser();

    // Parse NIF file and extract mesh data
    bool parseFile(const std::string& filePath);

    // Get parsed mesh data
    const std::vector<NifMeshData>& getMeshes() const { return meshes; }

    // Get error message if parsing failed
    const std::string& getLastError() const { return lastError; }

private:
    std::vector<NifMeshData> meshes;
    std::string lastError;

    // Helper functions for binary reading
    uint32_t readUInt32(const std::vector<uint8_t>& data, size_t& offset);
    uint16_t readUInt16(const std::vector<uint8_t>& data, size_t& offset);
    uint8_t readUInt8(const std::vector<uint8_t>& data, size_t& offset);
    float readFloat(const std::vector<uint8_t>& data, size_t& offset);
    NifVector3 readVector3(const std::vector<uint8_t>& data, size_t& offset);
    NifColor4 readColor4(const std::vector<uint8_t>& data, size_t& offset);
    NifString readString(const std::vector<uint8_t>& data, size_t& offset);

    // Parse NIF structure
    bool parseHeader(const std::vector<uint8_t>& data, NifFileHeader& header, size_t& offset);
    bool parseObjectArray(const std::vector<uint8_t>& data, size_t& offset);

    // Read entire file into memory
    bool readFileToMemory(const std::string& filePath, std::vector<uint8_t>& data);
};
