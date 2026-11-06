// DDS loading is now delegated to the Rust `dds-ffi` C ABI (see
// oblivion-re-tools/crates/dds-ffi). This file is a thin C++ shim that keeps the
// original DDSLoader public API: it drives the Rust handle and materialises the
// cached DDSHeader / DDSTexture members that the getters hand out. Header
// parsing, DXT1/3/5/RXGB block decoding and uncompressed mask extraction live
// in Rust; on the C++ side they are FFI-backed, while the OpenGL upload and
// cleanup stay here because they need a GL context.

#include "dds_loader.h"
#include "dds_ffi.h"

#include <android/log.h>
#include <GLES3/gl3.h>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#undef LOG_TAG
#undef LOGD
#undef LOGE
#define LOG_TAG "DDSLoader"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// S3TC tokens are declared in GLES2/gl2ext.h, not in the GLES3 core header,
// so define them here when the platform header does not provide them.
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif
#ifndef GL_EXTENSIONS
#define GL_EXTENSIONS 0x1F03
#endif

namespace {

// Maps the Rust DDS_FORMAT_* code back to the C++ enum the public API exposes.
DDSCompressionFormat toCompressionFormat(int format) {
    switch (format) {
        case DDS_FORMAT_DXT1: return DDSCompressionFormat::DXT1;
        case DDS_FORMAT_DXT3: return DDSCompressionFormat::DXT3;
        case DDS_FORMAT_DXT5: return DDSCompressionFormat::DXT5;
        case DDS_FORMAT_RXGB: return DDSCompressionFormat::RXGB;
        case DDS_FORMAT_UNCOMPRESSED: return DDSCompressionFormat::UNCOMPRESSED;
        default: return DDSCompressionFormat::UNKNOWN;
    }
}

// Fetches the handle's last error message into a NUL-terminated buffer so it can
// be logged safely. `dds_last_error` writes into `out` and returns the needed
// length; passing a null out (as a bare integer) would be a bogus `%s` pointer.
std::string last_error_text(void* handle) {
    char buf[512];
    buf[0] = '\0';
    if (dds_last_error(handle, buf, sizeof(buf)) > 0) {
        return std::string(buf);
    }
    return std::string();
}

// GLES3 guarantees ETC2 but not S3TC, so the raw DXT payload can only go
// straight to the GPU when the driver advertises
// GL_EXT_texture_compression_s3tc. The query result is cached; -1 = not asked yet.
int g_s3tc_support = -1;

bool supports_s3tc() {
    if (g_s3tc_support < 0) {
        const unsigned char* exts = glGetString(GL_EXTENSIONS);
        g_s3tc_support =
            (exts && std::strstr(reinterpret_cast<const char*>(exts),
                                 "GL_EXT_texture_compression_s3tc"))
                ? 1 : 0;
    }
    return g_s3tc_support == 1;
}

uint32_t s3tc_internal_format(DDSCompressionFormat format) {
    switch (format) {
        case DDSCompressionFormat::DXT1: return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        case DDSCompressionFormat::DXT3: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        case DDSCompressionFormat::DXT5: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        default: return 0;
    }
}

size_t s3tc_block_bytes(DDSCompressionFormat format) {
    switch (format) {
        case DDSCompressionFormat::DXT1: return 8;
        case DDSCompressionFormat::DXT3:
        case DDSCompressionFormat::DXT5: return 16;
        default: return 0;
    }
}

// Bytes of one mip level of a block-compressed 2D texture: ceil(w/4) x
// ceil(h/4) blocks. Levels below 4 px clamp to a single block row/column,
// matching the GL spec's compressed-image sizing rule.
size_t compressed_level_bytes(uint32_t width, uint32_t height, size_t blockBytes) {
    const uint32_t blocksX = (width + 3) / 4;
    const uint32_t blocksY = (height + 3) / 4;
    return static_cast<size_t>(blocksX) * static_cast<size_t>(blocksY) * blockBytes;
}

}  // namespace

DDSLoader::DDSLoader() : header(), texture(), m_handle(dds_create()) {
    texture.textureId = 0;
    std::memset(&header, 0, sizeof(DDSHeader));
}

DDSLoader::~DDSLoader() {
    cleanup();
    if (m_handle) {
        dds_destroy(m_handle);
        m_handle = nullptr;
    }
}

bool DDSLoader::loadFile(const std::string& filepath) {
    LOGD("=== Loading DDS file: %s ===", filepath.c_str());

    std::ifstream in(filepath, std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        LOGE("Failed to open DDS file: %s", filepath.c_str());
        return false;
    }

    const std::streamoff fileSize = in.tellg();
    if (fileSize <= 0) {
        LOGE("Empty or unreadable DDS file: %s", filepath.c_str());
        in.close();
        return false;
    }

    std::vector<uint8_t> data(static_cast<size_t>(fileSize));
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(data.data()), fileSize);
    const bool shortRead = (in.gcount() != fileSize);
    in.close();
    if (shortRead) {
        LOGE("Short read on DDS file: %s", filepath.c_str());
        return false;
    }

    if (!m_handle) {
        LOGE("DDS handle is null");
        return false;
    }

    const int rc = dds_parse_memory(m_handle, data.data(), data.size());
    if (rc != DDS_OK) {
        LOGE("Rust DDS parse failed: %d (%s)", rc, last_error_text(m_handle).c_str());
        return false;
    }

    // Rebuild the cached header and texture view from the Rust parse result.
    const uint8_t* rawHeader = nullptr;
    size_t rawHeaderLen = 0;
    if (dds_raw_header(m_handle, &rawHeader, &rawHeaderLen) != DDS_OK ||
        rawHeaderLen != sizeof(DDSHeader) || rawHeader == nullptr) {
        LOGE("dds_raw_header failed: %s", last_error_text(m_handle).c_str());
        return false;
    }
    std::memcpy(&header, rawHeader, sizeof(DDSHeader));

    texture.width = static_cast<uint32_t>(dds_width(m_handle));
    texture.height = static_cast<uint32_t>(dds_height(m_handle));
    const int mipmapCount = dds_mipmap_count(m_handle);
    texture.mipmapCount = mipmapCount > 0 ? static_cast<uint32_t>(mipmapCount) : 1u;
    texture.compressionFormat = toCompressionFormat(dds_compression_format(m_handle));

    // Mirror the compressed payload so the C++ view matches what was parsed.
    const uint8_t* payload = nullptr;
    size_t payloadLen = 0;
    if (dds_compressed_data(m_handle, &payload, &payloadLen) == DDS_OK && payload != nullptr) {
        texture.compressedData.assign(payload, payload + payloadLen);
    } else {
        texture.compressedData.clear();
    }
    texture.decompressedData.clear();

    LOGD("DDS Info: %ux%u, %u mipmaps, format: %u",
         texture.width, texture.height, texture.mipmapCount,
         static_cast<uint32_t>(texture.compressionFormat));
    LOGD("DDS file loaded successfully (%zu bytes)", texture.compressedData.size());

    return true;
}

bool DDSLoader::decompressTexture() {
    LOGD("Decompressing DDS texture: format=%u",
         static_cast<uint32_t>(texture.compressionFormat));

    if (!m_handle) {
        LOGE("DDS handle is null");
        return false;
    }

    const int rc = dds_decompress(m_handle);
    if (rc != DDS_OK) {
        LOGE("Rust DDS decompress failed: %d (%s)", rc, last_error_text(m_handle).c_str());
        return false;
    }

    const uint8_t* rgba = nullptr;
    size_t rgbaLen = 0;
    if (dds_decompressed_data(m_handle, &rgba, &rgbaLen) != DDS_OK || rgba == nullptr) {
        LOGE("dds_decompressed_data failed: %s", last_error_text(m_handle).c_str());
        return false;
    }

    texture.decompressedData.assign(rgba, rgba + rgbaLen);

    LOGD("DDS decompressed: %ux%u (%zu bytes)",
         texture.width, texture.height, texture.decompressedData.size());
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

unsigned int DDSLoader::uploadCompressedToGPU() {
    const uint32_t internalFormat = s3tc_internal_format(texture.compressionFormat);
    if (internalFormat == 0) {
        return 0;  // Not an S3TC format; caller uses the RGBA8 path.
    }
    if (texture.compressedData.empty() || texture.width == 0 || texture.height == 0) {
        return 0;
    }
    if (!supports_s3tc()) {
        LOGD("S3TC not supported by driver; using decompress path");
        return 0;
    }

    const size_t blockBytes = s3tc_block_bytes(texture.compressionFormat);

    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload every mip level the payload actually contains. Some BSA DDS files
    // store only level 0 despite a larger mipmapCount, so stop at the payload
    // end and let GL generate the remaining chain.
    size_t offset = 0;
    uint32_t levels = 0;
    for (uint32_t level = 0; level < texture.mipmapCount; ++level) {
        const uint32_t lw = (texture.width >> level) ? (texture.width >> level) : 1u;
        const uint32_t lh = (texture.height >> level) ? (texture.height >> level) : 1u;
        const size_t levelBytes = compressed_level_bytes(lw, lh, blockBytes);
        if (offset + levelBytes > texture.compressedData.size()) {
            break;
        }
        glCompressedTexImage2D(GL_TEXTURE_2D, static_cast<GLint>(level),
                               static_cast<GLenum>(internalFormat),
                               static_cast<GLsizei>(lw), static_cast<GLsizei>(lh), 0,
                               static_cast<GLsizei>(levelBytes),
                               texture.compressedData.data() + offset);
        offset += levelBytes;
        ++levels;
    }

    if (levels == 0) {
        glBindTexture(GL_TEXTURE_2D, 0);
        LOGE("Compressed payload too small for %ux%u", texture.width, texture.height);
        return 0;
    }
    if (levels < texture.mipmapCount) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    texture.textureId = texId;
    LOGD("Compressed DDS uploaded: ID=%u fmt=0x%X levels=%u bytes=%zu",
         texId, internalFormat, levels, offset);

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
