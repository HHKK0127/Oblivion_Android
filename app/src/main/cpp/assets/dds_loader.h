#pragma once

#include <GLES3/gl3.h>
#include <string>
#include <vector>
#include <cstdint>

// DDS file format constants
#define DDS_MAGIC 0x20534444  // "DDS "

// DDS Pixel Format Flags
#define DDPF_ALPHAPIXELS 0x00000001
#define DDPF_ALPHA       0x00000002
#define DDPF_FOURCC      0x00000004
#define DDPF_RGB         0x00000040
#define DDPF_YUV         0x00000200
#define DDPF_LUMINANCE   0x00020000

// Compression formats (FourCC)
#define FOURCC_DXT1 0x31545844  // "DXT1"
#define FOURCC_DXT3 0x33545844  // "DXT3"
#define FOURCC_DXT5 0x35545844  // "DXT5"
#define FOURCC_ATI1 0x31495441  // "ATI1"
#define FOURCC_ATI2 0x32495441  // "ATI2"

// DDS file structures
struct DDSPixelFormat {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
};

struct DDSHeader {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    DDSPixelFormat ddspf;
    uint32_t dwCaps;
    uint32_t dwCaps2;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
};

struct DdsTextureData {
    GLuint textureId;
    uint32_t width;
    uint32_t height;
    uint32_t mipMapCount;
    GLenum format;           // GL_RGBA, GL_RGB, etc.
    GLenum internalFormat;   // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, etc.
    bool isCompressed;
    std::vector<std::vector<uint8_t>> mipData;  // Mip level data

    DdsTextureData() : textureId(0), width(0), height(0), mipMapCount(0),
                      format(GL_RGBA), internalFormat(GL_RGBA), isCompressed(false) {}
};

class DdsLoader {
public:
    DdsLoader();
    ~DdsLoader();

    // Load DDS file and create OpenGL texture
    GLuint loadTexture(const std::string& filePath);

    // Load DDS file and get texture data (without uploading to GPU)
    bool loadTextureData(const std::string& filePath, DdsTextureData& outTexture);

    // Get last error message
    const std::string& getLastError() const { return lastError; }

private:
    std::string lastError;

    // Helper functions
    bool readDdsHeader(const std::string& filePath, uint32_t& magic, DDSHeader& header,
                      std::vector<uint8_t>& fileData);
    bool parseDDSFormat(const DDSPixelFormat& ddspf, GLenum& outFormat,
                       GLenum& outInternalFormat, bool& outIsCompressed);
    GLuint createGLTexture(const DdsTextureData& texData);
    size_t calculateMipSize(uint32_t width, uint32_t height, bool isCompressed);
};
