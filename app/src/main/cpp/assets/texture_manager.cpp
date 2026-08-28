#include "texture_manager.h"
#include <GLES3/gl3.h>
#include <cstring>
#include <algorithm>
#include <cmath>

// ============================================================================
// AssetTextureManager Implementation
// ============================================================================

AssetTextureManager::AssetTextureManager() = default;

AssetTextureManager::~AssetTextureManager() {
    cleanup();
}

bool AssetTextureManager::initialize(BSArchive* arch) {
    if (initialized) {
        LOGW_ASTEX("Already initialized");
        return true;
    }
    archive = arch;
    initialized = true;
    LOGI_ASTEX("AssetTextureManager initialized");
    return true;
}

void AssetTextureManager::cleanup() {
    clearCache();
    for (auto& atlas : atlases) {
        if (atlas && atlas->glTextureId) {
            glDeleteTextures(1, &atlas->glTextureId);
        }
    }
    atlases.clear();
    initialized = false;
    LOGI_ASTEX("AssetTextureManager cleaned up");
}

// ============================================================================
// Texture Loading Pipeline
// ============================================================================

uint32_t AssetTextureManager::loadTexture(const std::string& texturePath) {
    if (!initialized || !archive) {
        LOGE_ASTEX("Not initialized");
        return 0;
    }

    // Check cache first
    auto it = cache.find(texturePath);
    if (it != cache.end()) {
        cacheHits++;
        updateLRU(texturePath);
        it->second.lastAccessTime = 0.0f; // Will be updated by caller
        return it->second.info.glTextureId;
    }
    cacheMisses++;

    // Load from BSA
    uint32_t texId = loadDDSFromBSA(texturePath);
    if (texId == 0) {
        LOGE_ASTEX("Failed to load texture: %s", texturePath.c_str());
        return 0;
    }

    return texId;
}

uint32_t AssetTextureManager::loadTextureFromData(const std::string& key,
                                                    const uint8_t* data,
                                                    size_t dataSize) {
    if (!initialized) return 0;

    auto it = cache.find(key);
    if (it != cache.end()) {
        cacheHits++;
        updateLRU(key);
        return it->second.info.glTextureId;
    }
    cacheMisses++;

    return loadDDSFromMemory(data, dataSize, key);
}

const TextureInfo* AssetTextureManager::getTextureInfo(
    const std::string& texturePath) const {
    auto it = cache.find(texturePath);
    if (it != cache.end()) {
        return &it->second.info;
    }
    return nullptr;
}

// ============================================================================
// DDS Loading from BSA
// ============================================================================

uint32_t AssetTextureManager::loadDDSFromBSA(const std::string& texturePath) {
    // Find file in BSA
    const BSAFileEntry* entry = archive->findFile(texturePath);
    if (!entry) {
        LOGW_ASTEX("Texture not found in BSA: %s", texturePath.c_str());
        return 0;
    }

    // Extract DDS data
    std::vector<uint8_t> ddsData;
    if (!archive->extractFileDecompressed(*entry, ddsData)) {
        LOGE_ASTEX("Failed to extract: %s", texturePath.c_str());
        return 0;
    }

    return loadDDSFromMemory(ddsData.data(), ddsData.size(), texturePath);
}

uint32_t AssetTextureManager::loadDDSFromMemory(const uint8_t* data,
                                                  size_t dataSize,
                                                  const std::string& key) {
    if (dataSize < sizeof(uint32_t) + sizeof(DDSHeader)) {
        LOGE_ASTEX("DDS data too small for key: %s", key.c_str());
        return 0;
    }

    // Verify DDS magic
    uint32_t magic = 0;
    std::memcpy(&magic, data, sizeof(uint32_t));
    if (magic != DDS_MAGIC) {
        LOGE_ASTEX("Invalid DDS magic for key: %s", key.c_str());
        return 0;
    }

    // Parse header
    DDSHeader ddsHeader;
    std::memcpy(&ddsHeader, data + sizeof(uint32_t), sizeof(DDSHeader));

    uint32_t width = ddsHeader.width;
    uint32_t height = ddsHeader.height;
    uint32_t mipCount = ddsHeader.mipmapCount;
    if (mipCount == 0) mipCount = 1;

    // Determine compression format
    TextureFormat format = TextureFormat::RGBA8;
    bool isCompressed = false;
    if (ddsHeader.pixelFormat.flags & DDPF_FOURCC) {
        uint32_t fourCC = ddsHeader.pixelFormat.fourCC;
        if (fourCC == FOURCC_DXT1) {
            format = TextureFormat::DXT1;
            isCompressed = true;
        } else if (fourCC == FOURCC_DXT3) {
            format = TextureFormat::DXT3;
            isCompressed = true;
        } else if (fourCC == FOURCC_DXT5) {
            format = TextureFormat::DXT5;
            isCompressed = true;
        }
    }

    // Calculate data offset (after magic + header)
    size_t headerSize = sizeof(uint32_t) + sizeof(DDSHeader);
    const uint8_t* pixelData = data + headerSize;
    size_t pixelDataSize = dataSize - headerSize;

    // Decompress to RGBA if needed
    std::vector<uint8_t> rgbaData;
    const uint8_t* uploadData = nullptr;
    int uploadWidth = static_cast<int>(width);
    int uploadHeight = static_cast<int>(height);

    if (isCompressed) {
        DDSTexture ddsTex;
        ddsTex.width = width;
        ddsTex.height = height;
        ddsTex.mipmapCount = mipCount;
        ddsTex.compressedData.assign(pixelData, pixelData + pixelDataSize);

        if (format == TextureFormat::DXT1)
            ddsTex.compressionFormat = DDSCompressionFormat::DXT1;
        else if (format == TextureFormat::DXT3)
            ddsTex.compressionFormat = DDSCompressionFormat::DXT3;
        else
            ddsTex.compressionFormat = DDSCompressionFormat::DXT5;

        if (!decompressDDSToRGBA(ddsTex, rgbaData)) {
            LOGE_ASTEX("Failed to decompress DDS: %s", key.c_str());
            return 0;
        }
        uploadData = rgbaData.data();
        format = TextureFormat::RGBA8;
    } else {
        // Uncompressed: copy to rgbaData so we have a consistent CPU-side buffer
        rgbaData.assign(pixelData, pixelData + pixelDataSize);
        uploadData = rgbaData.data();
    }

    // Upload to GPU with mipmaps
    uint32_t texId = uploadToGPU(uploadData, uploadWidth, uploadHeight,
                                  format, true);
    if (texId == 0) {
        LOGE_ASTEX("GPU upload failed: %s", key.c_str());
        return 0;
    }

    // Calculate memory usage
    size_t memBytes = static_cast<size_t>(width) * height * 4;

    // Store in cache
    CacheEntry entry;
    entry.info.glTextureId = texId;
    entry.info.width = width;
    entry.info.height = height;
    entry.info.mipmapCount = mipCount;
    entry.info.format = format;
    entry.info.memoryBytes = memBytes;
    entry.lastAccessTime = 0.0f;
    // Cache CPU-side pixel data for atlas generation (candidate textures only)
    if (width <= static_cast<uint32_t>(ATLAS_MAX_SUB_TEX_SIZE) &&
        height <= static_cast<uint32_t>(ATLAS_MAX_SUB_TEX_SIZE)) {
        entry.cpuPixelData.assign(rgbaData.begin(), rgbaData.end());
    }

    lruList.push_front(key);
    entry.lruIter = lruList.begin();
    cache[key] = std::move(entry);
    currentCacheSize += memBytes;

    evictIfNeeded();

    LOGD_ASTEX("Loaded texture: %s (%ux%u, %lu bytes)",
               key.c_str(), width, height,
               static_cast<unsigned long>(memBytes));
    return texId;
}

// ============================================================================
// GPU Upload
// ============================================================================

uint32_t AssetTextureManager::uploadToGPU(const uint8_t* data, int width,
                                            int height, TextureFormat format,
                                            bool generateMips) {
    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    generateMips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLenum internalFmt = GL_RGBA8;
    GLenum fmt = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;

    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, width, height, 0,
                 fmt, type, data);

    if (generateMips) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    return texId;
}

// ============================================================================
// DDS Decompression (DXT -> RGBA)
// ============================================================================

bool AssetTextureManager::decompressDDSToRGBA(const DDSTexture& dds,
                                                std::vector<uint8_t>& rgbaOut) {
    int w = static_cast<int>(dds.width);
    int h = static_cast<int>(dds.height);
    rgbaOut.resize(static_cast<size_t>(w) * h * 4, 0);

    const uint8_t* src = dds.compressedData.data();

    switch (dds.compressionFormat) {
        case DDSCompressionFormat::DXT1:
            return decompressDXT1(src, rgbaOut.data(), w, h);
        case DDSCompressionFormat::DXT3:
            return decompressDXT3(src, rgbaOut.data(), w, h);
        case DDSCompressionFormat::DXT5:
            return decompressDXT5(src, rgbaOut.data(), w, h);
        default:
            LOGE_ASTEX("Unsupported compression format");
            return false;
    }
}

uint32_t AssetTextureManager::expand565(uint16_t color) {
    uint32_t r = (color >> 11) & 0x1F;
    uint32_t g = (color >> 5) & 0x3F;
    uint32_t b = color & 0x1F;
    r = (r << 3) | (r >> 2);
    g = (g << 2) | (g >> 4);
    b = (b << 3) | (b >> 2);
    return (0xFF << 24) | (b << 16) | (g << 8) | r;
}

uint32_t AssetTextureManager::interpolateColor(uint32_t c0, uint32_t c1,
                                                  int idx, bool isDXT1) {
    uint32_t r0 = c0 & 0xFF, g0 = (c0 >> 8) & 0xFF, b0 = (c0 >> 16) & 0xFF;
    uint32_t r1 = c1 & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = (c1 >> 16) & 0xFF;

    if (idx == 0) return c0;
    if (idx == 1) return c1;

    if (idx == 2) {
        if (isDXT1 && c0 <= c1) {
            // 1-bit alpha: transparent black
            return 0;
        }
        uint32_t r = (2 * r0 + r1 + 1) / 3;
        uint32_t g = (2 * g0 + g1 + 1) / 3;
        uint32_t b = (2 * b0 + b1 + 1) / 3;
        return (0xFF << 24) | (b << 16) | (g << 8) | r;
    }

    // idx == 3
    if (isDXT1 && c0 <= c1) {
        // 1-bit alpha: fully transparent
        return 0;
    }
    uint32_t r = (r0 + 2 * r1 + 1) / 3;
    uint32_t g = (g0 + 2 * g1 + 1) / 3;
    uint32_t b = (b0 + 2 * b1 + 1) / 3;
    return (0xFF << 24) | (b << 16) | (g << 8) | r;
}

void AssetTextureManager::decodeDXTBlock(const uint8_t* block,
                                           uint8_t* outPixels,
                                           bool isDXT1) {
    uint16_t c0, c1;
    std::memcpy(&c0, block, 2);
    std::memcpy(&c1, block + 2, 2);

    uint32_t colors[4];
    colors[0] = expand565(c0);
    colors[1] = expand565(c1);
    colors[2] = interpolateColor(colors[0], colors[1], 2, isDXT1);
    colors[3] = interpolateColor(colors[0], colors[1], 3, isDXT1);

    uint32_t indices = 0;
    std::memcpy(&indices, block + 4, 4);

    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            int idx = (indices >> (2 * (4 * row + col))) & 0x3;
            uint32_t c = colors[idx];
            int pixIdx = (row * 4 + col) * 4;
            outPixels[pixIdx + 0] = c & 0xFF;
            outPixels[pixIdx + 1] = (c >> 8) & 0xFF;
            outPixels[pixIdx + 2] = (c >> 16) & 0xFF;
            outPixels[pixIdx + 3] = (c >> 24) & 0xFF;
        }
    }
}

bool AssetTextureManager::decompressDXT1(const uint8_t* src, uint8_t* dst,
                                           int width, int height) {
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    const uint8_t* srcPtr = src;

    for (int by = 0; by < blocksY; by++) {
        for (int bx = 0; bx < blocksX; bx++) {
            uint8_t blockPixels[64];
            decodeDXTBlock(srcPtr, blockPixels, true);
            srcPtr += 8;

            for (int row = 0; row < 4; row++) {
                int py = by * 4 + row;
                if (py >= height) continue;
                for (int col = 0; col < 4; col++) {
                    int px = bx * 4 + col;
                    if (px >= width) continue;
                    int dstIdx = (py * width + px) * 4;
                    int srcIdx = (row * 4 + col) * 4;
                    dst[dstIdx + 0] = blockPixels[srcIdx + 0];
                    dst[dstIdx + 1] = blockPixels[srcIdx + 1];
                    dst[dstIdx + 2] = blockPixels[srcIdx + 2];
                    dst[dstIdx + 3] = blockPixels[srcIdx + 3];
                }
            }
        }
    }
    return true;
}

bool AssetTextureManager::decompressDXT3(const uint8_t* src, uint8_t* dst,
                                           int width, int height) {
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    const uint8_t* srcPtr = src;

    for (int by = 0; by < blocksY; by++) {
        for (int bx = 0; bx < blocksX; bx++) {
            // First 8 bytes: explicit alpha (4 bits per pixel)
            uint8_t alphaBlock[16];
            for (int i = 0; i < 4; i++) {
                uint16_t row;
                std::memcpy(&row, srcPtr + i * 2, 2);
                for (int j = 0; j < 4; j++) {
                    uint8_t a4 = (row >> (j * 4)) & 0xF;
                    alphaBlock[i * 4 + j] = (a4 << 4) | a4;
                }
            }
            srcPtr += 8;

            // Next 8 bytes: DXT color block
            uint8_t blockPixels[64];
            decodeDXTBlock(srcPtr, blockPixels, false);
            srcPtr += 8;

            // Apply explicit alpha
            for (int row = 0; row < 4; row++) {
                int py = by * 4 + row;
                if (py >= height) continue;
                for (int col = 0; col < 4; col++) {
                    int px = bx * 4 + col;
                    if (px >= width) continue;
                    int dstIdx = (py * width + px) * 4;
                    int srcIdx = (row * 4 + col) * 4;
                    dst[dstIdx + 0] = blockPixels[srcIdx + 0];
                    dst[dstIdx + 1] = blockPixels[srcIdx + 1];
                    dst[dstIdx + 2] = blockPixels[srcIdx + 2];
                    dst[dstIdx + 3] = alphaBlock[row * 4 + col];
                }
            }
        }
    }
    return true;
}

bool AssetTextureManager::decompressDXT5(const uint8_t* src, uint8_t* dst,
                                           int width, int height) {
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    const uint8_t* srcPtr = src;

    for (int by = 0; by < blocksY; by++) {
        for (int bx = 0; bx < blocksX; bx++) {
            // First 8 bytes: interpolated alpha
            uint8_t a0 = srcPtr[0];
            uint8_t a1 = srcPtr[1];

            uint64_t bits = 0;
            for (int i = 0; i < 6; i++) {
                bits |= static_cast<uint64_t>(srcPtr[2 + i]) << (8 * i);
            }

            uint8_t alphaTable[8];
            alphaTable[0] = a0;
            alphaTable[1] = a1;
            if (a0 > a1) {
                for (int i = 1; i <= 5; i++)
                    alphaTable[1 + i] = static_cast<uint8_t>(
                        ((6 - i) * a0 + (i) * a1 + 3) / 7);
                alphaTable[7] = 0;
            } else {
                for (int i = 1; i <= 3; i++)
                    alphaTable[1 + i] = static_cast<uint8_t>(
                        ((4 - i) * a0 + (i) * a1 + 2) / 5);
                alphaTable[5] = 0;
                alphaTable[6] = 255;
                alphaTable[7] = 255;
            }

            uint8_t alphaIndices[16];
            for (int i = 0; i < 16; i++) {
                alphaIndices[i] = alphaTable[(bits >> (3 * i)) & 0x7];
            }
            srcPtr += 8;

            // Next 8 bytes: DXT color block
            uint8_t blockPixels[64];
            decodeDXTBlock(srcPtr, blockPixels, false);
            srcPtr += 8;

            // Apply interpolated alpha
            for (int row = 0; row < 4; row++) {
                int py = by * 4 + row;
                if (py >= height) continue;
                for (int col = 0; col < 4; col++) {
                    int px = bx * 4 + col;
                    if (px >= width) continue;
                    int dstIdx = (py * width + px) * 4;
                    int srcIdx = (row * 4 + col) * 4;
                    dst[dstIdx + 0] = blockPixels[srcIdx + 0];
                    dst[dstIdx + 1] = blockPixels[srcIdx + 1];
                    dst[dstIdx + 2] = blockPixels[srcIdx + 2];
                    dst[dstIdx + 3] = alphaIndices[row * 4 + col];
                }
            }
        }
    }
    return true;
}

// ============================================================================
// Mipmap Generation
// ============================================================================

bool AssetTextureManager::generateMipmaps(const std::string& texturePath) {
    auto it = cache.find(texturePath);
    if (it == cache.end()) return false;

    GLuint texId = it->second.info.glTextureId;
    glBindTexture(GL_TEXTURE_2D, texId);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Update mipmap count estimate
    int w = static_cast<int>(it->second.info.width);
    int h = static_cast<int>(it->second.info.height);
    it->second.info.mipmapCount = 1 + static_cast<uint32_t>(
        std::floor(std::log2(std::max(w, h))));

    return true;
}

void AssetTextureManager::generateAllMipmaps() {
    for (auto& pair : cache) {
        GLuint texId = pair.second.info.glTextureId;
        if (texId) {
            glBindTexture(GL_TEXTURE_2D, texId);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================================
// Texture Atlas Generation
// ============================================================================

int AssetTextureManager::generateAtlases() {
    // Collect small textures that are candidates for atlasing
    struct AtlasCandidate {
        std::string key;
        int width, height;
    };
    std::vector<AtlasCandidate> candidates;

    for (auto& pair : cache) {
        auto& info = pair.second.info;
        if (!info.inAtlas &&
            info.width <= ATLAS_MAX_SUB_TEX_SIZE &&
            info.height <= ATLAS_MAX_SUB_TEX_SIZE &&
            info.glTextureId != 0) {
            candidates.push_back({pair.first,
                                   static_cast<int>(info.width),
                                   static_cast<int>(info.height)});
        }
    }

    if (candidates.empty()) return 0;

    // Sort by height descending for better packing
    std::sort(candidates.begin(), candidates.end(),
              [](const AtlasCandidate& a, const AtlasCandidate& b) {
                  return a.height > b.height;
              });

    int atlasesBefore = static_cast<int>(atlases.size());

    for (auto& cand : candidates) {
        // Use cached CPU-side pixel data (OpenGL ES has no glGetTexImage)
        auto it = cache.find(cand.key);
        if (it == cache.end()) continue;
        if (it->second.cpuPixelData.empty()) continue;

        const uint8_t* pixels = it->second.cpuPixelData.data();
        size_t expectedSize = static_cast<size_t>(cand.width) * cand.height * 4;
        if (it->second.cpuPixelData.size() != expectedSize) {
            LOGW_ASTEX("Cached pixel data size mismatch for: %s", cand.key.c_str());
            continue;
        }

        AtlasRegion region;
        if (packIntoAtlas(cand.key, pixels, cand.width, cand.height, region)) {
            it->second.info.inAtlas = true;
            it->second.info.atlasRegion = region;

            // Free the individual texture
            GLuint srcTex = it->second.info.glTextureId;
            if (srcTex) {
                glDeleteTextures(1, &srcTex);
            }
            it->second.info.glTextureId = 0;
            it->second.cpuPixelData.clear();
            it->second.cpuPixelData.shrink_to_fit();
            currentCacheSize -= it->second.info.memoryBytes;
        }
    }

    // Upload atlas textures to GPU
    for (auto& atlas : atlases) {
        if (atlas && atlas->glTextureId == 0 && !atlas->pixelData.empty()) {
            GLuint texId = 0;
            glGenTextures(1, &texId);
            glBindTexture(GL_TEXTURE_2D, texId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlas->size, atlas->size,
                         0, GL_RGBA, GL_UNSIGNED_BYTE, atlas->pixelData.data());
            glBindTexture(GL_TEXTURE_2D, 0);
            atlas->glTextureId = texId;
            atlas->pixelData.clear(); // Free CPU-side data
        }
    }

    int newAtlases = static_cast<int>(atlases.size()) - atlasesBefore;
    LOGI_ASTEX("Generated %d new atlases (%lu total)",
               newAtlases, static_cast<unsigned long>(atlases.size()));
    return newAtlases;
}

bool AssetTextureManager::packIntoAtlas(const std::string& key,
                                          const uint8_t* rgbaData,
                                          int width, int height,
                                          AtlasRegion& regionOut) {
    // Try to fit into existing atlases
    for (int i = 0; i < static_cast<int>(atlases.size()); i++) {
        auto& atlas = atlases[i];
        if (atlas->glTextureId != 0) continue; // Already uploaded, skip

        // Simple row-packing: try to fit in current row
        if (atlas->nextY + height <= atlas->size) {
            int x = 0;
            // Find x position from existing entries in this row
            for (auto& e : atlas->entries) {
                if (e.y == atlas->nextY || 
                    (e.y < atlas->nextY && e.y + e.height > atlas->nextY)) {
                    x = std::max(x, e.x + e.width);
                }
            }

            if (x + width <= atlas->size) {
                // Copy pixel data into atlas
                for (int row = 0; row < height; row++) {
                    int dstOffset = ((atlas->nextY + row) * atlas->size + x) * 4;
                    int srcOffset = row * width * 4;
                    std::memcpy(atlas->pixelData.data() + dstOffset,
                                rgbaData + srcOffset,
                                static_cast<size_t>(width) * 4);
                }

                AtlasEntry entry;
                entry.x = x;
                entry.y = atlas->nextY;
                entry.width = width;
                entry.height = height;
                entry.used = true;
                atlas->entries.push_back(entry);

                regionOut.atlasIndex = i;
                regionOut.x = x;
                regionOut.y = atlas->nextY;
                regionOut.width = width;
                regionOut.height = height;
                regionOut.u0 = static_cast<float>(x) / atlas->size;
                regionOut.v0 = static_cast<float>(atlas->nextY) / atlas->size;
                regionOut.u1 = static_cast<float>(x + width) / atlas->size;
                regionOut.v1 = static_cast<float>(atlas->nextY + height) / atlas->size;

                // Advance row cursor if this row is full
                atlas->nextY += height;
                return true;
            }
        }
    }

    // Create new atlas
    int idx = createNewAtlas();
    auto& atlas = atlases[idx];

    // Place at top-left
    for (int row = 0; row < height; row++) {
        int dstOffset = row * atlas->size * 4;
        int srcOffset = row * width * 4;
        std::memcpy(atlas->pixelData.data() + dstOffset,
                    rgbaData + srcOffset,
                    static_cast<size_t>(width) * 4);
    }

    AtlasEntry entry;
    entry.x = 0;
    entry.y = 0;
    entry.width = width;
    entry.height = height;
    entry.used = true;
    atlas->entries.push_back(entry);
    atlas->nextY = height;

    regionOut.atlasIndex = idx;
    regionOut.x = 0;
    regionOut.y = 0;
    regionOut.width = width;
    regionOut.height = height;
    regionOut.u0 = 0.0f;
    regionOut.v0 = 0.0f;
    regionOut.u1 = static_cast<float>(width) / atlas->size;
    regionOut.v1 = static_cast<float>(height) / atlas->size;
    return true;
}

int AssetTextureManager::createNewAtlas() {
    auto atlas = std::make_unique<Atlas>();
    atlas->size = ATLAS_SIZE;
    atlas->pixelData.resize(
        static_cast<size_t>(ATLAS_SIZE) * ATLAS_SIZE * 4, 0);
    int idx = static_cast<int>(atlases.size());
    atlases.push_back(std::move(atlas));
    return idx;
}

uint32_t AssetTextureManager::getAtlasTextureId(int atlasIndex) const {
    if (atlasIndex < 0 || atlasIndex >= static_cast<int>(atlases.size()))
        return 0;
    return atlases[atlasIndex]->glTextureId;
}

const AtlasRegion* AssetTextureManager::getAtlasRegion(
    const std::string& texturePath) const {
    auto it = cache.find(texturePath);
    if (it != cache.end() && it->second.info.inAtlas) {
        return &it->second.info.atlasRegion;
    }
    return nullptr;
}

// ============================================================================
// LRU Cache Management
// ============================================================================

void AssetTextureManager::updateLRU(const std::string& key) {
    auto it = cache.find(key);
    if (it == cache.end()) return;

    lruList.erase(it->second.lruIter);
    lruList.push_front(key);
    it->second.lruIter = lruList.begin();
}

void AssetTextureManager::evictIfNeeded() {
    while (currentCacheSize > maxCacheSize && !lruList.empty()) {
        const std::string& victim = lruList.back();
        auto it = cache.find(victim);
        if (it != cache.end()) {
            if (it->second.info.glTextureId) {
                GLuint texId = it->second.info.glTextureId;
                glDeleteTextures(1, &texId);
            }
            currentCacheSize -= it->second.info.memoryBytes;
            cache.erase(it);
        }
        lruList.pop_back();
    }
}

void AssetTextureManager::evictLRU(size_t targetBytes) {
    while (currentCacheSize > targetBytes && !lruList.empty()) {
        const std::string& victim = lruList.back();
        auto it = cache.find(victim);
        if (it != cache.end()) {
            if (it->second.info.glTextureId) {
                GLuint texId = it->second.info.glTextureId;
                glDeleteTextures(1, &texId);
            }
            currentCacheSize -= it->second.info.memoryBytes;
            LOGD_ASTEX("Evicted texture: %s", victim.c_str());
            cache.erase(it);
        }
        lruList.pop_back();
    }
}

void AssetTextureManager::unloadTexture(const std::string& texturePath) {
    auto it = cache.find(texturePath);
    if (it == cache.end()) return;

    if (it->second.info.glTextureId) {
        GLuint texId = it->second.info.glTextureId;
        glDeleteTextures(1, &texId);
    }
    currentCacheSize -= it->second.info.memoryBytes;
    lruList.erase(it->second.lruIter);
    cache.erase(it);
}

void AssetTextureManager::clearCache() {
    for (auto& pair : cache) {
        if (pair.second.info.glTextureId) {
            GLuint texId = pair.second.info.glTextureId;
            glDeleteTextures(1, &texId);
        }
    }
    cache.clear();
    lruList.clear();
    currentCacheSize = 0;
    cacheHits = 0;
    cacheMisses = 0;
}
