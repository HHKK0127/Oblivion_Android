#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace oblivion {

/**
 * Texture compression formats for mobile GPU
 */
enum class TextureFormat {
    ASTC_4x4,    // Best quality, moderate compression
    ASTC_6x6,    // Balanced quality/compression
    ASTC_8x8,    // Maximum compression, lower quality
    ETC2_RGB,    // Good for opaque textures
    ETC2_RGBA,   // Good for textures with alpha
    RGBA8        // Uncompressed fallback
};

/**
 * Compression quality presets
 */
enum class CompressionQuality {
    HIGH,    // Best visual quality
    MEDIUM,  // Balanced
    LOW      // Maximum compression
};

/**
 * Texture compression result
 */
struct CompressionResult {
    bool success;
    std::string outputPath;
    size_t originalSize;
    size_t compressedSize;
    float compressionRatio;
    std::string errorMessage;
};

/**
 * Texture compressor for mobile GPU formats
 * Supports ASTC and ETC2 compression
 */
class TextureCompressor {
public:
    TextureCompressor();
    ~TextureCompressor();

    /**
     * Compress a single texture file
     * @param inputPath Path to source PNG file
     * @param outputPath Path to write compressed texture
     * @param format Target compression format
     * @param quality Compression quality
     * @return Compression result with details
     */
    CompressionResult compressTexture(
        const std::string& inputPath,
        const std::string& outputPath,
        TextureFormat format = TextureFormat::ASTC_4x4,
        CompressionQuality quality = CompressionQuality::MEDIUM
    );

    /**
     * Compress all textures in a directory
     * @param inputDir Source directory with PNG files
     * @param outputDir Destination directory
     * @param format Target compression format
     * @param quality Compression quality
     * @param progressCallback Optional progress callback
     * @return Number of successfully compressed textures
     */
    int compressDirectory(
        const std::string& inputDir,
        const std::string& outputDir,
        TextureFormat format = TextureFormat::ASTC_4x4,
        CompressionQuality quality = CompressionQuality::MEDIUM,
        void (*progressCallback)(int current, int total) = nullptr
    );

    /**
     * Get estimated compression ratio for format
     */
    float getCompressionRatio(TextureFormat format) const;

    /**
     * Get format name string
     */
    std::string getFormatName(TextureFormat format) const;

    /**
     * Check if ASTC compression is supported on this device
     */
    static bool isASTCSupported();

    /**
     * Check if ETC2 compression is supported on this device
     */
    static bool isETC2Supported();

private:
    /**
     * Compress bitmap data to ASTC format
     */
    bool compressToASTC(
        const uint8_t* pixels,
        int width,
        int height,
        int channels,
        uint8_t blockSize,
        int quality,
        std::vector<uint8_t>& output
    );

    /**
     * Compress bitmap data to ETC2 format
     */
    bool compressToETC2(
        const uint8_t* pixels,
        int width,
        int height,
        int channels,
        int quality,
        std::vector<uint8_t>& output
    );

    /**
     * Write ASTC file header
     */
    void writeASTCHeader(
        std::vector<uint8_t>& output,
        int width,
        int height,
        uint8_t blockSizeX,
        uint8_t blockSizeY
    );

    /**
     * Write KTX file header for ETC2
     */
    void writeKTXHeader(
        std::vector<uint8_t>& output,
        int width,
        int height,
        bool hasAlpha
    );
};

} // namespace oblivion
