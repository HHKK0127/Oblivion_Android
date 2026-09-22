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

    // Read compressed data (everything after the 4-byte magic and the 124-byte header)
    file.seekg(0, std::ios::end);
    const std::streamoff fileSize = file.tellg();
    const std::streamoff dataOffset =
        static_cast<std::streamoff>(sizeof(uint32_t) + sizeof(DDSHeader));
    if (fileSize < dataOffset) {
        LOGE("DDS file too small: %lld bytes", static_cast<long long>(fileSize));
        file.close();
        return false;
    }
    const size_t dataSize = static_cast<size_t>(fileSize - dataOffset);

    file.seekg(dataOffset, std::ios::beg);
    texture.compressedData.resize(dataSize);
    file.read(reinterpret_cast<char*>(texture.compressedData.data()),
              static_cast<std::streamsize>(dataSize));

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

namespace {

// Expands a packed RGB565 colour to 8-bit RGB, replicating the high bits into
// the low ones so that 0xFFFF maps to pure white.
inline void decode_rgb565(uint16_t packed, uint8_t out_rgb[3]) {
    const uint8_t r5 = static_cast<uint8_t>((packed >> 11) & 0x1F);
    const uint8_t g6 = static_cast<uint8_t>((packed >> 5) & 0x3F);
    const uint8_t b5 = static_cast<uint8_t>(packed & 0x1F);
    out_rgb[0] = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
    out_rgb[1] = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
    out_rgb[2] = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
}

// Builds the four-entry colour palette of a BC1 colour block. The two-colour
// interpolated mode (with a transparent index) is only reachable for DXT1;
// DXT3/DXT5 always use the three-colour interpolation.
inline void build_colour_palette(const uint8_t* block, bool allow_two_colour_mode,
                                 uint8_t palette[4][4]) {
    const uint16_t c0 = static_cast<uint16_t>(block[0] | (block[1] << 8));
    const uint16_t c1 = static_cast<uint16_t>(block[2] | (block[3] << 8));

    decode_rgb565(c0, palette[0]);
    decode_rgb565(c1, palette[1]);
    palette[0][3] = 255;
    palette[1][3] = 255;

    if (c0 > c1 || !allow_two_colour_mode) {
        for (int channel = 0; channel < 3; ++channel) {
            palette[2][channel] = static_cast<uint8_t>(
                (2 * palette[0][channel] + palette[1][channel]) / 3);
            palette[3][channel] = static_cast<uint8_t>(
                (palette[0][channel] + 2 * palette[1][channel]) / 3);
        }
        palette[2][3] = 255;
        palette[3][3] = 255;
    } else {
        for (int channel = 0; channel < 3; ++channel) {
            palette[2][channel] = static_cast<uint8_t>(
                (palette[0][channel] + palette[1][channel]) / 2);
            palette[3][channel] = 0;
        }
        palette[2][3] = 255;
        palette[3][3] = 0;
    }
}

// Builds the eight-entry interpolated alpha palette of a BC3 alpha block.
inline void build_alpha_palette(const uint8_t* block, uint8_t palette[8]) {
    const uint8_t a0 = block[0];
    const uint8_t a1 = block[1];
    palette[0] = a0;
    palette[1] = a1;

    if (a0 > a1) {
        for (int step = 1; step <= 6; ++step) {
            palette[1 + step] = static_cast<uint8_t>(
                ((7 - step) * a0 + step * a1) / 7);
        }
    } else {
        for (int step = 1; step <= 4; ++step) {
            palette[1 + step] = static_cast<uint8_t>(
                ((5 - step) * a0 + step * a1) / 5);
        }
        palette[6] = 0;
        palette[7] = 255;
    }
}

// Writes one texel, discarding writes that fall outside the texture so that
// dimensions which are not a multiple of four stay in bounds.
inline void write_texel(uint8_t* destination, uint32_t width, uint32_t height,
                        uint32_t x, uint32_t y, const uint8_t rgba[4]) {
    if (x >= width || y >= height) return;
    uint8_t* out = destination + (static_cast<size_t>(y) * width + x) * 4;
    out[0] = rgba[0];
    out[1] = rgba[1];
    out[2] = rgba[2];
    out[3] = rgba[3];
}

// DXT1 (BC1): 8-byte blocks holding two RGB565 endpoints and sixteen 2-bit
// indices. An endpoint pair with c0 <= c1 enables the 1-bit alpha mode.
void decode_bc1(const uint8_t* source, size_t source_size, uint8_t* destination,
                uint32_t width, uint32_t height) {
    const uint32_t blocks_x = (width + 3) / 4;
    const uint32_t blocks_y = (height + 3) / 4;

    for (uint32_t block_y = 0; block_y < blocks_y; ++block_y) {
        for (uint32_t block_x = 0; block_x < blocks_x; ++block_x) {
            const size_t offset = (static_cast<size_t>(block_y) * blocks_x + block_x) * 8;
            if (offset + 8 > source_size) return;
            const uint8_t* block = source + offset;

            uint8_t palette[4][4];
            build_colour_palette(block, true, palette);

            uint32_t indices = 0;
            for (int byte = 0; byte < 4; ++byte) {
                indices |= static_cast<uint32_t>(block[4 + byte]) << (8 * byte);
            }

            for (int texel = 0; texel < 16; ++texel) {
                const uint8_t* colour = palette[(indices >> (2 * texel)) & 0x03];
                write_texel(destination, width, height,
                            block_x * 4 + static_cast<uint32_t>(texel & 3),
                            block_y * 4 + static_cast<uint32_t>(texel >> 2), colour);
            }
        }
    }
}

// DXT3 (BC2): 8 bytes of explicit 4-bit alpha followed by a BC1 colour block
// that always uses the three-colour interpolation.
void decode_bc2(const uint8_t* source, size_t source_size, uint8_t* destination,
                uint32_t width, uint32_t height) {
    const uint32_t blocks_x = (width + 3) / 4;
    const uint32_t blocks_y = (height + 3) / 4;

    for (uint32_t block_y = 0; block_y < blocks_y; ++block_y) {
        for (uint32_t block_x = 0; block_x < blocks_x; ++block_x) {
            const size_t offset = (static_cast<size_t>(block_y) * blocks_x + block_x) * 16;
            if (offset + 16 > source_size) return;
            const uint8_t* block = source + offset;

            uint8_t palette[4][4];
            build_colour_palette(block + 8, false, palette);

            uint32_t indices = 0;
            for (int byte = 0; byte < 4; ++byte) {
                indices |= static_cast<uint32_t>(block[12 + byte]) << (8 * byte);
            }

            for (int texel = 0; texel < 16; ++texel) {
                const uint8_t* colour = palette[(indices >> (2 * texel)) & 0x03];
                uint8_t rgba[4];
                rgba[0] = colour[0];
                rgba[1] = colour[1];
                rgba[2] = colour[2];
                rgba[3] = static_cast<uint8_t>(
                    ((block[texel >> 1] >> (4 * (texel & 1))) & 0x0F) * 17);
                write_texel(destination, width, height,
                            block_x * 4 + static_cast<uint32_t>(texel & 3),
                            block_y * 4 + static_cast<uint32_t>(texel >> 2), rgba);
            }
        }
    }
}

// DXT5 (BC3) and RXGB: 8 bytes of interpolated alpha followed by a BC1 colour
// block. RXGB is a BC3 variant used for normal maps where the X component is
// stored in the alpha channel, so the red and alpha channels are swapped.
void decode_bc3(const uint8_t* source, size_t source_size, uint8_t* destination,
                uint32_t width, uint32_t height, bool swap_red_alpha) {
    const uint32_t blocks_x = (width + 3) / 4;
    const uint32_t blocks_y = (height + 3) / 4;

    for (uint32_t block_y = 0; block_y < blocks_y; ++block_y) {
        for (uint32_t block_x = 0; block_x < blocks_x; ++block_x) {
            const size_t offset = (static_cast<size_t>(block_y) * blocks_x + block_x) * 16;
            if (offset + 16 > source_size) return;
            const uint8_t* block = source + offset;

            uint8_t alpha_palette[8];
            build_alpha_palette(block, alpha_palette);

            uint64_t alpha_indices = 0;
            for (int byte = 0; byte < 6; ++byte) {
                alpha_indices |= static_cast<uint64_t>(block[2 + byte]) << (8 * byte);
            }

            uint8_t palette[4][4];
            build_colour_palette(block + 8, false, palette);

            uint32_t colour_indices = 0;
            for (int byte = 0; byte < 4; ++byte) {
                colour_indices |= static_cast<uint32_t>(block[12 + byte]) << (8 * byte);
            }

            for (int texel = 0; texel < 16; ++texel) {
                const uint8_t* colour = palette[(colour_indices >> (2 * texel)) & 0x03];
                uint8_t rgba[4];
                rgba[0] = colour[0];
                rgba[1] = colour[1];
                rgba[2] = colour[2];
                rgba[3] = alpha_palette[(alpha_indices >> (3 * texel)) & 0x07];
                if (swap_red_alpha) {
                    const uint8_t red = rgba[0];
                    rgba[0] = rgba[3];
                    rgba[3] = red;
                }
                write_texel(destination, width, height,
                            block_x * 4 + static_cast<uint32_t>(texel & 3),
                            block_y * 4 + static_cast<uint32_t>(texel >> 2), rgba);
            }
        }
    }
}

}  // namespace

bool DDSLoader::decompressTexture() {
    LOGD("Decompressing DDS texture: format=%u", 
         static_cast<uint32_t>(texture.compressionFormat));

    // Calculate decompressed size (RGBA)
    uint32_t decompSize = texture.width * texture.height * 4;
    texture.decompressedData.resize(decompSize);

    switch (texture.compressionFormat) {
        case DDSCompressionFormat::DXT1:
            return decompressDXT1();
        case DDSCompressionFormat::DXT3:
            return decompressDXT3();
        case DDSCompressionFormat::DXT5:
            return decompressDXT5();
        case DDSCompressionFormat::RXGB:
            return decompressRXGB();
        case DDSCompressionFormat::UNCOMPRESSED:
        case DDSCompressionFormat::UNKNOWN:
            // A missing FourCC with the RGB flag set means an uncompressed
            // pixel format described by the channel masks.
            if (!(header.pixelFormat.flags & DDPF_FOURCC) &&
                (header.pixelFormat.flags & DDPF_RGB)) {
                return decompressUncompressed();
            }
            LOGE("Unsupported DDS compression format");
            return false;
        default:
            LOGE("Unsupported DDS compression format");
            return false;
    }
}

bool DDSLoader::decompressDXT1() {
    if (texture.compressedData.empty()) {
        LOGE("No compressed DDS data to decompress");
        return false;
    }
    decode_bc1(texture.compressedData.data(), texture.compressedData.size(),
               texture.decompressedData.data(), texture.width, texture.height);
    LOGD("DXT1 decompressed: %ux%u", texture.width, texture.height);
    return true;
}

bool DDSLoader::decompressDXT3() {
    if (texture.compressedData.empty()) {
        LOGE("No compressed DDS data to decompress");
        return false;
    }
    decode_bc2(texture.compressedData.data(), texture.compressedData.size(),
               texture.decompressedData.data(), texture.width, texture.height);
    LOGD("DXT3 decompressed: %ux%u", texture.width, texture.height);
    return true;
}

bool DDSLoader::decompressDXT5() {
    if (texture.compressedData.empty()) {
        LOGE("No compressed DDS data to decompress");
        return false;
    }
    decode_bc3(texture.compressedData.data(), texture.compressedData.size(),
               texture.decompressedData.data(), texture.width, texture.height, false);
    LOGD("DXT5 decompressed: %ux%u", texture.width, texture.height);
    return true;
}

bool DDSLoader::decompressRXGB() {
    if (texture.compressedData.empty()) {
        LOGE("No compressed DDS data to decompress");
        return false;
    }
    decode_bc3(texture.compressedData.data(), texture.compressedData.size(),
               texture.decompressedData.data(), texture.width, texture.height, true);
    LOGD("RXGB decompressed: %ux%u", texture.width, texture.height);
    return true;
}

bool DDSLoader::decompressUncompressed() {
    const DDSPixelFormat& format = header.pixelFormat;
    const uint32_t bytesPerPixel = format.bitCount / 8;
    if (bytesPerPixel != 3 && bytesPerPixel != 4) {
        LOGE("Unsupported uncompressed DDS bit count: %u", format.bitCount);
        return false;
    }

    const size_t required =
        static_cast<size_t>(texture.width) * texture.height * bytesPerPixel;
    if (texture.compressedData.size() < required) {
        LOGE("Uncompressed DDS data truncated: %zu < %zu",
             texture.compressedData.size(), required);
        return false;
    }

    // Spreads a masked channel across the full 8-bit range.
    const auto extract = [](uint32_t value, uint32_t mask) -> uint8_t {
        if (mask == 0) return 255;
        uint32_t shift = 0;
        while (shift < 32 && ((mask >> shift) & 1u) == 0) ++shift;
        const uint32_t field = (value & mask) >> shift;
        const uint32_t maxValue = mask >> shift;
        if (maxValue == 0) return 255;
        return static_cast<uint8_t>((field * 255 + maxValue / 2) / maxValue);
    };

    const bool hasAlpha = (format.flags & DDPF_ALPHAPIXELS) != 0 && format.alphaMask != 0;
    const bool hasMasks = (format.redMask | format.greenMask | format.blueMask) != 0;

    const uint8_t* source = texture.compressedData.data();
    uint8_t* destination = texture.decompressedData.data();
    const size_t texelCount = static_cast<size_t>(texture.width) * texture.height;

    for (size_t texel = 0; texel < texelCount; ++texel) {
        const uint8_t* in = source + texel * bytesPerPixel;
        uint8_t* out = destination + texel * 4;

        if (hasMasks) {
            uint32_t packed = 0;
            for (uint32_t byte = 0; byte < bytesPerPixel; ++byte) {
                packed |= static_cast<uint32_t>(in[byte]) << (8 * byte);
            }
            out[0] = extract(packed, format.redMask);
            out[1] = extract(packed, format.greenMask);
            out[2] = extract(packed, format.blueMask);
            out[3] = hasAlpha ? extract(packed, format.alphaMask) : 255;
        } else if (bytesPerPixel == 4) {
            // Undocumented masks: assume the common little-endian BGRA layout.
            out[0] = in[2];
            out[1] = in[1];
            out[2] = in[0];
            out[3] = in[3];
        } else {
            out[0] = in[2];
            out[1] = in[1];
            out[2] = in[0];
            out[3] = 255;
        }
    }

    LOGD("Uncompressed DDS decompressed: %ux%u (%u bpp)",
         texture.width, texture.height, format.bitCount);
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
