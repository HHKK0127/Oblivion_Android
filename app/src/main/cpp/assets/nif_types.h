#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>

// NIF file header constants
#define NIF_MAGIC "Gamebryo File Format, Version"
#define NIF_VERSION_OBLIVION "20.0.0.5"
#define NIF_VERSION_SKYRIM "20.2.0.7"

// NIF block type IDs
enum class NIFBlockType : uint32_t {
    NiNode = 0x001,
    NiTriShape = 0x30E,
    NiTriStrips = 0x30F,
    NiTexturingProperty = 0x307,
    NiMaterialProperty = 0x304,
    NiAlphaProperty = 0x305,
    NiVertexColorProperty = 0x306,
    NiZBufferProperty = 0x308,
    NiProperty = 0x300,
    Unknown = 0xFFFFFFFF
};

// NIF structures
struct NIFHeader {
    char magic[256];           // "Gamebryo File Format, Version X.X.X.X"
    uint32_t version;          // Version number
    uint32_t userVersion;      // User version
    uint32_t userVersion2;     // User version 2
    uint32_t numObjects;       // Number of objects in the file
    uint32_t numStrings;       // Number of strings in string table
    uint32_t maxStringLength;  // Maximum string length
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
    uint32_t nodeIndex;
    int32_t parentIndex;                    // -1 if root
    std::vector<int32_t> childIndices;
    
    // Node-specific data
    NIFTransform transform;
    bool hasGeometry;
    NIFGeometry geometry;
};
