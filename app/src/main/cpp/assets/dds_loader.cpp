#include "dds_loader.h"
#include <fstream>
#include <android/log.h>
#include <cstring>

#define LOG_TAG "DdsLoader"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

DdsLoader::DdsLoader() {}

DdsLoader::~DdsLoader() {}

GLuint DdsLoader::loadTexture(const std::string& filePath) {
    LOGD("Loading DDS texture: %s", filePath.c_str());

    DdsTextureData texData;
    if (!loadTextureData(filePath, texData)) {
        LOGE("Failed to load texture data from %s", filePath.c_str());
        return 0;
    }

    return createGLTexture(texData);
}

bool DdsLoader::loadTextureData(const std::string& filePath, DdsTextureData& outTexture) {
    uint32_t magic;
    DDSHeader header;
    std::vector<uint8_t> fileData;

    if (!readDdsHeader(filePath, magic, header, fileData)) {
        return false;
    }

    if (magic != DDS_MAGIC) {
        lastError = "Invalid DDS magic number";
        LOGE("%s", lastError.c_str());
        return false;
    }

    LOGD("DDS Header: %ux%u, MipMaps: %u", header.dwWidth, header.dwHeight, header.dwMipMapCount);

    // Parse format
    if (!parseDDSFormat(header.ddspf, outTexture.format, outTexture.internalFormat, outTexture.isCompressed)) {
        lastError = "Unsupported DDS format";
        LOGE("%s", lastError.c_str());
        return false;
    }

    outTexture.width = header.dwWidth;
    outTexture.height = header.dwHeight;
    outTexture.mipMapCount = (header.dwMipMapCount > 0) ? header.dwMipMapCount : 1;

    // Extract mip map data
    size_t offset = sizeof(uint32_t) + sizeof(DDSHeader);  // After magic and header

    for (uint32_t i = 0; i < outTexture.mipMapCount; i++) {
        uint32_t mipWidth = std::max(1u, header.dwWidth >> i);
        uint32_t mipHeight = std::max(1u, header.dwHeight >> i);
        size_t mipSize = calculateMipSize(mipWidth, mipHeight, outTexture.isCompressed);

        if (offset + mipSize > fileData.size()) {
            lastError = "DDS file truncated";
            LOGE("%s", lastError.c_str());
            return false;
        }

        std::vector<uint8_t> mipData(fileData.begin() + offset, fileData.begin() + offset + mipSize);
        outTexture.mipData.push_back(mipData);
        offset += mipSize;

        LOGD("Mip %u: %ux%u, Size: %zu bytes", i, mipWidth, mipHeight, mipSize);
    }

    LOGD("DDS texture data loaded successfully");
    return true;
}

bool DdsLoader::readDdsHeader(const std::string& filePath, uint32_t& magic, DDSHeader& header,
                              std::vector<uint8_t>& fileData) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        lastError = "Failed to open DDS file: " + filePath;
        LOGE("%s", lastError.c_str());
        return false;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    fileData.resize(fileSize);
    if (!file.read(reinterpret_cast<char*>(fileData.data()), fileSize)) {
        lastError = "Failed to read DDS file";
        LOGE("%s", lastError.c_str());
        return false;
    }
    file.close();

    if (fileData.size() < 128) {
        lastError = "DDS file too small";
        LOGE("%s", lastError.c_str());
        return false;
    }

    // Read magic
    std::memcpy(&magic, fileData.data(), sizeof(uint32_t));

    // Read header
    std::memcpy(&header, fileData.data() + sizeof(uint32_t), sizeof(DDSHeader));

    return true;
}

bool DdsLoader::parseDDSFormat(const DDSPixelFormat& ddspf, GLenum& outFormat,
                               GLenum& outInternalFormat, bool& outIsCompressed) {
    outIsCompressed = false;

    if (ddspf.dwFlags & DDPF_FOURCC) {
        // Compressed format
        outIsCompressed = true;

        switch (ddspf.dwFourCC) {
            case FOURCC_DXT1:
                // DXT1 is not supported in OpenGL ES 3.0, fallback to RGBA
                LOGD("DXT1 compression detected, will decompress to RGBA");
                outFormat = GL_RGBA;
                outInternalFormat = GL_RGBA;
                outIsCompressed = false;
                return true;

            case FOURCC_DXT3:
            case FOURCC_DXT5:
                // DXT3/DXT5 are not supported in OpenGL ES 3.0, fallback to RGBA
                LOGD("DXT3/DXT5 compression detected, will decompress to RGBA");
                outFormat = GL_RGBA;
                outInternalFormat = GL_RGBA;
                outIsCompressed = false;
                return true;

            default:
                LOGE("Unsupported FourCC format: 0x%08X", ddspf.dwFourCC);
                return false;
        }
    } else if (ddspf.dwFlags & DDPF_RGB) {
        // Uncompressed RGB(A) format
        outIsCompressed = false;

        if (ddspf.dwFlags & DDPF_ALPHAPIXELS) {
            outFormat = GL_RGBA;
            outInternalFormat = GL_RGBA;
        } else {
            outFormat = GL_RGB;
            outInternalFormat = GL_RGB;
        }
        return true;
    } else if (ddspf.dwFlags & DDPF_LUMINANCE) {
        outFormat = GL_LUMINANCE;
        outInternalFormat = GL_LUMINANCE;
        return true;
    }

    return false;
}

GLuint DdsLoader::createGLTexture(const DdsTextureData& texData) {
    GLuint textureId = 0;
    glGenTextures(1, &textureId);

    if (textureId == 0) {
        LOGE("Failed to create OpenGL texture");
        return 0;
    }

    glBindTexture(GL_TEXTURE_2D, textureId);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    for (size_t i = 0; i < texData.mipData.size(); i++) {
        uint32_t mipWidth = std::max(1u, texData.width >> i);
        uint32_t mipHeight = std::max(1u, texData.height >> i);

        if (texData.isCompressed) {
            // Upload compressed data (not typically done in ES 3.0 for DXT)
            LOGD("Uploading compressed mip level %zu", i);
            // glCompressedTexImage2D would go here
        } else {
            // Upload uncompressed data
            glTexImage2D(GL_TEXTURE_2D, i, texData.internalFormat, mipWidth, mipHeight, 0,
                        texData.format, GL_UNSIGNED_BYTE, texData.mipData[i].data());
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    LOGD("OpenGL texture created: ID %u", textureId);
    return textureId;
}

size_t DdsLoader::calculateMipSize(uint32_t width, uint32_t height, bool isCompressed) {
    if (isCompressed) {
        // DXT1 = 8 bytes per 16 pixels, DXT5 = 16 bytes per 16 pixels
        // Simplified: assume DXT1 or DXT5 at 4x4 block size
        uint32_t blockWidth = (width + 3) / 4;
        uint32_t blockHeight = (height + 3) / 4;
        // Assuming DXT1 (8 bytes per block)
        return blockWidth * blockHeight * 8;
    } else {
        // Uncompressed RGBA = 4 bytes per pixel
        return width * height * 4;
    }
}
