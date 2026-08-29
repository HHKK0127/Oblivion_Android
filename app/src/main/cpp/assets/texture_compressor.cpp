#include "texture_compressor.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>

// STB Image for loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace fs = std::filesystem;

namespace oblivion {

TextureCompressor::TextureCompressor() = default;
TextureCompressor::~TextureCompressor() = default;

CompressionResult TextureCompressor::compressTexture(
    const std::string& inputPath,
    const std::string& outputPath,
    TextureFormat format,
    CompressionQuality quality
) {
    CompressionResult result{};
    result.inputPath = inputPath;
    result.outputPath = outputPath;

    try {
        // Load source image
        int width, height, channels;
        unsigned char* pixels = stbi_load(inputPath.c_str(), &width, &height, &channels, 0);
        if (!pixels) {
            result.success = false;
            result.errorMessage = "Failed to load image: " + inputPath;
            return result;
        }

        result.originalSize = width * height * channels;

        // Create output directory if needed
        fs::path outPath(outputPath);
        if (outPath.has_parent_path()) {
            fs::create_directories(outPath.parent_path());
        }

        // Compress based on format
        std::vector<uint8_t> compressedData;
        bool success = false;

        switch (format) {
            case TextureFormat::ASTC_4x4:
            case TextureFormat::ASTC_6x6:
            case TextureFormat::ASTC_8x8: {
                uint8_t blockSize = (format == TextureFormat::ASTC_4x4) ? 4 :
                                   (format == TextureFormat::ASTC_6x6) ? 6 : 8;
                int qualityValue = (quality == CompressionQuality::HIGH) ? 0 :
                                  (quality == CompressionQuality::MEDIUM) ? 60 : 98;
                success = compressToASTC(pixels, width, height, channels, blockSize, qualityValue, compressedData);
                break;
            }
            case TextureFormat::ETC2_RGB:
            case TextureFormat::ETC2_RGBA: {
                int qualityValue = (quality == CompressionQuality::HIGH) ? 0 :
                                  (quality == CompressionQuality::MEDIUM) ? 60 : 98;
                success = compressToETC2(pixels, width, height, channels, qualityValue, compressedData);
                break;
            }
            case TextureFormat::RGBA8: {
                // No compression, just copy
                compressedData.assign(pixels, pixels + (width * height * channels));
                success = true;
                break;
            }
        }

        stbi_image_free(pixels);

        if (!success) {
            result.success = false;
            result.errorMessage = "Compression failed";
            return result;
        }

        // Write compressed data
        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile) {
            result.success = false;
            result.errorMessage = "Failed to open output file: " + outputPath;
            return result;
        }

        outFile.write(reinterpret_cast<const char*>(compressedData.data()), compressedData.size());
        outFile.close();

        result.compressedSize = compressedData.size();
        result.compressionRatio = static_cast<float>(result.compressedSize) / static_cast<float>(result.originalSize);
        result.success = true;

        return result;

    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
        return result;
    }
}

int TextureCompressor::compressDirectory(
    const std::string& inputDir,
    const std::string& outputDir,
    TextureFormat format,
    CompressionQuality quality,
    void (*progressCallback)(int, int)
) {
    if (!fs::exists(inputDir)) {
        return 0;
    }

    // Collect all PNG files
    std::vector<fs::path> pngFiles;
    for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".png") {
            pngFiles.push_back(entry.path());
        }
    }

    int totalFiles = static_cast<int>(pngFiles.size());
    int compressedCount = 0;

    for (int i = 0; i < totalFiles; ++i) {
        const auto& file = pngFiles[i];
        
        // Calculate relative path
        fs::path relativePath = fs::relative(file, inputDir);
        fs::path outputPath = fs::path(outputDir) / relativePath;
        outputPath.replace_extension(".astc");

        CompressionResult result = compressTexture(
            file.string(),
            outputPath.string(),
            format,
            quality
        );

        if (result.success) {
            compressedCount++;
        }

        if (progressCallback) {
            progressCallback(i + 1, totalFiles);
        }
    }

    return compressedCount;
}

float TextureCompressor::getCompressionRatio(TextureFormat format) const {
    switch (format) {
        case TextureFormat::ASTC_4x4: return 0.25f;
        case TextureFormat::ASTC_6x6: return 0.11f;
        case TextureFormat::ASTC_8x8: return 0.0625f;
        case TextureFormat::ETC2_RGB: return 0.25f;
        case TextureFormat::ETC2_RGBA: return 0.25f;
        case TextureFormat::RGBA8: return 1.0f;
        default: return 0.25f;
    }
}

std::string TextureCompressor::getFormatName(TextureFormat format) const {
    switch (format) {
        case TextureFormat::ASTC_4x4: return "ASTC 4x4";
        case TextureFormat::ASTC_6x6: return "ASTC 6x6";
        case TextureFormat::ASTC_8x8: return "ASTC 8x8";
        case TextureFormat::ETC2_RGB: return "ETC2 RGB";
        case TextureFormat::ETC2_RGBA: return "ETC2 RGBA";
        case TextureFormat::RGBA8: return "RGBA8";
        default: return "Unknown";
    }
}

bool TextureCompressor::isASTCSupported() {
    // Check for ASTC extension in OpenGL ES
    // This would need to be called after GL context is created
    return true; // Assume supported on modern devices
}

bool TextureCompressor::isETC2Supported() {
    // ETC2 is mandatory in OpenGL ES 3.0
    return true;
}

bool TextureCompressor::compressToASTC(
    const uint8_t* pixels,
    int width,
    int height,
    int channels,
    uint8_t blockSize,
    int quality,
    std::vector<uint8_t>& output
) {
    // Calculate block counts
    int blocksX = (width + blockSize - 1) / blockSize;
    int blocksY = (height + blockSize - 1) / blockSize;
    int totalBlocks = blocksX * blocksY;

    // ASTC block size is always 128 bits (16 bytes)
    const int ASTC_BLOCK_SIZE = 16;
    
    // Reserve space for header + data
    output.clear();
    output.reserve(16 + (totalBlocks * ASTC_BLOCK_SIZE));

    // Write ASTC header
    writeASTCHeader(output, width, height, blockSize, blockSize);

    // Compress each block
    for (int by = 0; by < blocksY; ++by) {
        for (int bx = 0; bx < blocksX; ++bx) {
            // Extract block pixels
            std::vector<uint8_t> blockPixels(blockSize * blockSize * channels, 0);
            
            for (int y = 0; y < blockSize; ++y) {
                for (int x = 0; x < blockSize; ++x) {
                    int srcX = bx * blockSize + x;
                    int srcY = by * blockSize + y;
                    
                    if (srcX < width && srcY < height) {
                        int srcIdx = (srcY * width + srcX) * channels;
                        int dstIdx = (y * blockSize + x) * channels;
                        
                        for (int c = 0; c < channels; ++c) {
                            blockPixels[dstIdx + c] = pixels[srcIdx + c];
                        }
                    }
                }
            }

            // Simple compression: average color + error
            // Real ASTC would use proper encoding
            uint8_t astcBlock[ASTC_BLOCK_SIZE] = {0};
            
            // Calculate average color
            uint32_t avgR = 0, avgG = 0, avgB = 0, avgA = 0;
            int pixelCount = blockSize * blockSize;
            
            for (int i = 0; i < pixelCount; ++i) {
                avgR += blockPixels[i * channels];
                avgG += blockPixels[i * channels + 1];
                avgB += blockPixels[i * channels + 2];
                if (channels > 3) {
                    avgA += blockPixels[i * channels + 3];
                }
            }
            
            astcBlock[0] = static_cast<uint8_t>(avgR / pixelCount);
            astcBlock[1] = static_cast<uint8_t>(avgG / pixelCount);
            astcBlock[2] = static_cast<uint8_t>(avgB / pixelCount);
            astcBlock[3] = static_cast<uint8_t>(channels > 3 ? avgA / pixelCount : 255);
            
            // Store block
            output.insert(output.end(), astcBlock, astcBlock + ASTC_BLOCK_SIZE);
        }
    }

    return true;
}

bool TextureCompressor::compressToETC2(
    const uint8_t* pixels,
    int width,
    int height,
    int channels,
    int quality,
    std::vector<uint8_t>& output
) {
    // ETC2 compresses 4x4 blocks into 8 bytes (RGB) or 16 bytes (RGBA)
    int blocksX = (width + 3) / 4;
    int blocksY = (height + 3) / 4;
    int totalBlocks = blocksX * blocksY;
    
    bool hasAlpha = (channels > 3);
    int blockSize = hasAlpha ? 16 : 8;
    
    // Reserve space
    output.clear();
    output.reserve(64 + (totalBlocks * blockSize)); // 64 bytes for KTX header

    // Write KTX header
    writeKTXHeader(output, width, height, hasAlpha);

    // Compress each block
    for (int by = 0; by < blocksY; ++by) {
        for (int bx = 0; bx < blocksX; ++bx) {
            // Extract 4x4 block
            uint8_t blockPixels[4 * 4 * 4]; // Max 4 channels
            memset(blockPixels, 0, sizeof(blockPixels));
            
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 4; ++x) {
                    int srcX = bx * 4 + x;
                    int srcY = by * 4 + y;
                    
                    if (srcX < width && srcY < height) {
                        int srcIdx = (srcY * width + srcX) * channels;
                        int dstIdx = (y * 4 + x) * 4;
                        
                        for (int c = 0; c < channels; ++c) {
                            blockPixels[dstIdx + c] = pixels[srcIdx + c];
                        }
                        if (channels < 4) {
                            blockPixels[dstIdx + 3] = 255;
                        }
                    }
                }
            }

            // Simple ETC2 encoding: average color
            // Real ETC2 would use proper encoding with tables
            uint8_t etcBlock[16] = {0};
            
            uint32_t avgR = 0, avgG = 0, avgB = 0, avgA = 0;
            int pixelCount = 16;
            
            for (int i = 0; i < pixelCount; ++i) {
                avgR += blockPixels[i * 4];
                avgG += blockPixels[i * 4 + 1];
                avgB += blockPixels[i * 4 + 2];
                avgA += blockPixels[i * 4 + 3];
            }
            
            etcBlock[0] = static_cast<uint8_t>(avgR / pixelCount);
            etcBlock[1] = static_cast<uint8_t>(avgG / pixelCount);
            etcBlock[2] = static_cast<uint8_t>(avgB / pixelCount);
            
            if (hasAlpha) {
                etcBlock[3] = static_cast<uint8_t>(avgA / pixelCount);
                output.insert(output.end(), etcBlock, etcBlock + 16);
            } else {
                output.insert(output.end(), etcBlock, etcBlock + 8);
            }
        }
    }

    return true;
}

void TextureCompressor::writeASTCHeader(
    std::vector<uint8_t>& output,
    int width,
    int height,
    uint8_t blockSizeX,
    uint8_t blockSizeY
) {
    // ASTC file header (16 bytes)
    uint8_t header[16] = {0};
    
    // Magic number
    header[0] = 0x13;
    header[1] = 0xAB;
    header[2] = 0xA1;
    header[3] = 0x5C;
    
    // Block size
    header[4] = blockSizeX;
    header[5] = blockSizeY;
    header[6] = 1; // block size Z
    
    // Image size (little-endian)
    header[7] = width & 0xFF;
    header[8] = (width >> 8) & 0xFF;
    header[9] = (width >> 16) & 0xFF;
    
    header[10] = height & 0xFF;
    header[11] = (height >> 8) & 0xFF;
    header[12] = (height >> 16) & 0xFF;
    
    header[13] = 1; // depth
    header[14] = 1; // array size
    header[15] = 1; // face count
    
    output.insert(output.end(), header, header + 16);
}

void TextureCompressor::writeKTXHeader(
    std::vector<uint8_t>& output,
    int width,
    int height,
    bool hasAlpha
) {
    // KTX1 header (64 bytes)
    uint8_t header[64] = {0};
    
    // Identifier
    const uint8_t identifier[12] = {
        0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB,
        0x0D, 0x0A, 0x1A, 0x0A
    };
    memcpy(header, identifier, 12);
    
    // Endianness (little-endian)
    header[12] = 0x04;
    header[13] = 0x03;
    header[14] = 0x02;
    header[15] = 0x01;
    
    // GL type (UNSIGNED_BYTE)
    header[16] = 0x14;
    header[17] = 0x01;
    
    // GL format
    uint32_t glFormat = hasAlpha ? 0x1908 : 0x1907; // GL_RGBA or GL_RGB
    memcpy(header + 20, &glFormat, 4);
    
    // GL internal format (ETC2)
    uint32_t internalFormat = hasAlpha ? 0x9278 : 0x9274; // GL_COMPRESSED_RGBA8_ETC2_EAC or GL_COMPRESSED_RGB8_ETC2
    memcpy(header + 24, &internalFormat, 4);
    
    // Width and height
    uint32_t w = width;
    uint32_t h = height;
    memcpy(header + 36, &w, 4);
    memcpy(header + 40, &h, 4);
    
    output.insert(output.end(), header, header + 64);
}

} // namespace oblivion
