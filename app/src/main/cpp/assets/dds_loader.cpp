#include "dds_loader.h"
#include <fstream>
#include <android/log.h>
#include <GLES3/gl3.h>
#include <cstring>

#undef LOG_TAG
#undef LOGD
#undef LOGE
#define LOG_TAG "DDSLoader"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

DDSLoader::DDSLoader() : texture() {
    texture.textureId = 0;
    memset(&header, 0, sizeof(DDSHeader));
}

DDSLoader::~DDSLoader() {
    cleanup();
}

bool DDSLoader::loadFile(const std::string& filepath) {
    LOGD("=== Loading DDS file: %s ===", filepath.c_str());

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to open DDS file: %s", filepath.c_str());
        return false;
    }

    // Read magic number
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(uint32_t));

    if (magic != DDS_MAGIC) {
        LOGE("Invalid DDS magic number: 0x%08X (expected 0x%08X)", magic, DDS_MAGIC);
        file.close();
        return false;
    }

    LOGD("Valid DDS magic detected");

    // Read header
    if (!readHeader(file)) {
        LOGE("Failed to read DDS header");
        file.close();
        return false;
    }

    // Extract texture info
    texture.width = header.width;
    texture.height = header.height;
    texture.mipmapCount = header.mipmapCount > 0 ? header.mipmapCount : 1;
    texture.compressionFormat = getCompressionFormat(header.pixelFormat.fourCC);

    LOGD("DDS Info: %ux%u, %u mipmaps, format: %u",
         texture.width, texture.height, texture.mipmapCount, 
         static_cast<uint32_t>(texture.compressionFormat));

    // Read compressed data
    uint32_t dataSize = file.seekg(0, std::ios::end).tellg();
    dataSize -= file.tellg();
    file.seekg(sizeof(uint32_t) + sizeof(DDSHeader), std::ios::beg);

    texture.compressedData.resize(dataSize);
    file.read(reinterpret_cast<char*>(texture.compressedData.data()), dataSize);

    if (!file.good()) {
        LOGE("Failed to read DDS texture data");
        file.close();
        return false;
    }

    file.close();
    LOGD("DDS file loaded successfully (%zu bytes)", texture.compressedData.size());

    return true;
}

bool DDSLoader::readHeader(std::ifstream& file) {
    file.read(reinterpret_cast<char*>(&header), sizeof(DDSHeader));

    if (!file.good()) {
        LOGE("Failed to read DDS header");
        return false;
    }

    // Validate header
    if (header.size != 124) {
        LOGD("Warning: DDS header size is %u (expected 124)", header.size);
    }

    // Check required flags
    if (!(header.flags & DDSD_WIDTH) || !(header.flags & DDSD_HEIGHT)) {
        LOGE("DDS header missing width or height flag");
        return false;
    }

    return true;
}

DDSCompressionFormat DDSLoader::getCompressionFormat(uint32_t fourCC) {
    switch (fourCC) {
        case FOURCC_DXT1:
            return DDSCompressionFormat::DXT1;
        case FOURCC_DXT3:
            return DDSCompressionFormat::DXT3;
        case FOURCC_DXT5:
            return DDSCompressionFormat::DXT5;
        case FOURCC_RXGB:
            return DDSCompressionFormat::RXGB;
        default:
            return DDSCompressionFormat::UNKNOWN;
    }
}

bool DDSLoader::decompressTexture() {
    LOGD("Decompressing DDS texture: format=%u", 
         static_cast<uint32_t>(texture.compressionFormat));

    // Calculate decompressed size (RGBA)
    uint32_t decompSize = texture.width * texture.height * 4;
    texture.decompressedData.resize(decompSize);

    // TODO: Implement DXT decompression using libsquish
    // For now, just log that decompression would happen here
    
    switch (texture.compressionFormat) {
        case DDSCompressionFormat::DXT1:
            LOGD("DXT1 decompression (not yet implemented)");
            return decompressDXT1();
        case DDSCompressionFormat::DXT3:
            LOGD("DXT3 decompression (not yet implemented)");
            return decompressDXT3();
        case DDSCompressionFormat::DXT5:
            LOGD("DXT5 decompression (not yet implemented)");
            return decompressDXT5();
        default:
            LOGE("Unsupported DDS compression format");
            return false;
    }
}

bool DDSLoader::decompressDXT1() {
    // TODO: Implement DXT1 decompression
    // libsquish example:
    // squish::DecompressImage(
    //     texture.decompressedData.data(),
    //     texture.width,
    //     texture.height,
    //     texture.compressedData.data(),
    //     squish::kDxt1
    // );
    
    LOGD("DXT1 decompression placeholder");
    return true;
}

bool DDSLoader::decompressDXT3() {
    // TODO: Implement DXT3 decompression
    LOGD("DXT3 decompression placeholder");
    return true;
}

bool DDSLoader::decompressDXT5() {
    // TODO: Implement DXT5 decompression
    LOGD("DXT5 decompression placeholder");
    return true;
}

unsigned int DDSLoader::uploadToGPU() {
    LOGD("Uploading DDS texture to GPU: %ux%u", texture.width, texture.height);

    if (texture.decompressedData.empty()) {
        LOGE("No decompressed texture data");
        return 0;
    }

    // Generate OpenGL texture
    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data (assuming RGBA format)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 texture.width, texture.height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE,
                 texture.decompressedData.data());

    // Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    texture.textureId = texId;
    LOGD("Texture uploaded successfully: ID=%u", texId);

    return texId;
}

void DDSLoader::cleanup() {
    if (texture.textureId != 0) {
        glDeleteTextures(1, &texture.textureId);
        texture.textureId = 0;
    }

    texture.compressedData.clear();
    texture.decompressedData.clear();

    LOGD("DDS texture cleaned up");
}
