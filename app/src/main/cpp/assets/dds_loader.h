#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

// DDS File Format Constants
#define DDS_MAGIC 0x20534444  // "DDS " in little-endian

// DDS Pixel Format Flags
#define DDPF_ALPHAPIXELS  0x00000001
#define DDPF_ALPHA        0x00000002
#define DDPF_FOURCC       0x00000004
#define DDPF_RGB          0x00000040
#define DDPF_YUV          0x00000200
#define DDPF_LUMINANCE    0x00020000

// DDS Compression Formats (FourCC)
#define FOURCC_DXT1 0x31545844  // "DXT1"
#define FOURCC_DXT3 0x33545844  // "DXT3"
#define FOURCC_DXT5 0x35545844  // "DXT5"
#define FOURCC_RXGB 0x42475852  // "RXGB" (ATI)

// DDS Header Flags
#define DDSD_CAPS         0x00000001
#define DDSD_HEIGHT       0x00000002
#define DDSD_WIDTH        0x00000004
#define DDSD_PITCH        0x00000008
#define DDSD_PIXELFORMAT  0x00001000
#define DDSD_MIPMAPCOUNT  0x00020000
#define DDSD_LINEARSIZE   0x00080000
#define DDSD_DEPTH        0x00800000

// Texture Types
enum class DDSTextureType {
    UNKNOWN = 0,
    TEXTURE2D = 1,
    TEXTURE3D = 2,
    CUBEMAP = 3
};

// Compression Format
enum class DDSCompressionFormat {
    UNCOMPRESSED = 0,
    DXT1 = 1,
    DXT3 = 2,
    DXT5 = 3,
    RXGB = 4,
    UNKNOWN = 5
};

// DDS Pixel Format
struct DDSPixelFormat {
    uint32_t size;
    uint32_t flags;
    uint32_t fourCC;
    uint32_t bitCount;
    uint32_t redMask;
    uint32_t greenMask;
    uint32_t blueMask;
    uint32_t alphaMask;
};

// DDS Header
struct DDSHeader {
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipmapCount;
    uint32_t reserved1[11];
    DDSPixelFormat pixelFormat;
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
    uint32_t reserved2;
};

// DDS Texture Data
struct DDSTexture {
    uint32_t width;
    uint32_t height;
    uint32_t mipmapCount;
    DDSCompressionFormat compressionFormat;
    DDSTextureType textureType;
    
    std::vector<uint8_t> compressedData;
    std::vector<uint8_t> decompressedData;
    
    // OpenGL texture handle (set after upload)
    unsigned int textureId;
};

// DDS Loader Class
class DDSLoader {
public:
    DDSLoader();
    ~DDSLoader();

    // Load and parse DDS file
    bool loadFile(const std::string& filepath);

    // Get texture data
    const DDSTexture& getTexture() const { return texture; }
    const DDSHeader& getHeader() const { return header; }

    // Decompress DDS data (convert DXT to RGB)
    bool decompressTexture();

    // Upload to OpenGL
    unsigned int uploadToGPU();

    // Cleanup
    void cleanup();

private:
    DDSHeader header;
    DDSTexture texture;

    // Helper methods
    bool readHeader(std::ifstream& file);
    DDSCompressionFormat getCompressionFormat(uint32_t fourCC);
    bool decompressDXT1();
    bool decompressDXT3();
    bool decompressDXT5();
};
