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

void DDSLoader::cleanup() {
    if (texture.textureId != 0) {
        glDeleteTextures(1, &texture.textureId);
        texture.textureId = 0;
    }

    texture.compressedData.clear();
    texture.decompressedData.clear();

    LOGD("DDS texture cleaned up");
}
