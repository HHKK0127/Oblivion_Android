package com.example.oblivion

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.Log
import java.io.File
import java.io.FileOutputStream

/**
 * Texture compression system for Android GPU
 * Converts PNG textures to compressed formats (ASTC/ETC2)
 * for reduced memory usage and faster loading
 */
class TextureCompressor(private val context: Context) {
    
    companion object {
        private const val TAG = "TextureCompressor"
        
        // Compression formats
        const val FORMAT_ASTC_4x4 = 0
        const val FORMAT_ASTC_6x6 = 1
        const val FORMAT_ASTC_8x8 = 2
        const val FORMAT_ETC2_RGB = 3
        const val FORMAT_ETC2_RGBA = 4
        
        // Quality presets
        const val QUALITY_HIGH = 0
        const val QUALITY_MEDIUM = 1
        const val QUALITY_LOW = 2
    }
    
    /**
     * Compress a single texture file
     * @param inputPath Path to source PNG file
     * @param outputPath Path to write compressed texture
     * @param format Compression format (FORMAT_*)
     * @param quality Quality preset (QUALITY_*)
     * @return true if compression succeeded
     */
    fun compressTexture(
        inputPath: String,
        outputPath: String,
        format: Int = FORMAT_ASTC_4x4,
        quality: Int = QUALITY_MEDIUM
    ): Boolean {
        try {
            // Load source image
            val options = BitmapFactory.Options().apply {
                inPreferredConfig = Bitmap.Config.ARGB_8888
            }
            val bitmap = BitmapFactory.decodeFile(inputPath, options) ?: return false
            
            // Get compression parameters
            val (blockSize, qualityLevel) = getCompressionParams(format, quality)
            
            // Create output directory if needed
            val outputFile = File(outputPath)
            outputFile.parentFile?.mkdirs()
            
            // Compress and save
            val success = compressAndSave(bitmap, outputPath, blockSize, qualityLevel)
            
            bitmap.recycle()
            return success
        } catch (e: Exception) {
            Log.e(TAG, "Failed to compress texture: $inputPath", e)
            return false
        }
    }
    
    /**
     * Compress all textures in a directory
     * @param inputDir Source directory with PNG files
     * @param outputDir Destination directory for compressed files
     * @param format Compression format
     * @param quality Quality preset
     * @param progressCallback Callback for progress updates (current, total)
     * @return Number of successfully compressed textures
     */
    fun compressDirectory(
        inputDir: String,
        outputDir: String,
        format: Int = FORMAT_ASTC_4x4,
        quality: Int = QUALITY_MEDIUM,
        progressCallback: ((Int, Int) -> Unit)? = null
    ): Int {
        val inputDirectory = File(inputDir)
        if (!inputDirectory.exists()) {
            Log.e(TAG, "Input directory does not exist: $inputDir")
            return 0
        }
        
        // Find all PNG files
        val pngFiles = inputDirectory.walk()
            .filter { it.isFile && it.extension.lowercase() == "png" }
            .toList()
        
        val totalFiles = pngFiles.size
        var compressedCount = 0
        
        Log.i(TAG, "Found $totalFiles PNG files to compress")
        
        pngFiles.forEachIndexed { index, file ->
            // Calculate relative path
            val relativePath = file.relativeTo(inputDirectory).path
            val outputPath = File(outputDir, relativePath.replace(".png", ".astc")).path
            
            if (compressTexture(file.path, outputPath, format, quality)) {
                compressedCount++
            }
            
            progressCallback?.invoke(index + 1, totalFiles)
        }
        
        Log.i(TAG, "Compressed $compressedCount/$totalFiles textures")
        return compressedCount
    }
    
    /**
     * Get compression parameters for format and quality
     */
    private fun getCompressionParams(format: Int, quality: Int): Pair<Int, Int> {
        return when (format) {
            FORMAT_ASTC_4x4 -> {
                val q = when (quality) {
                    QUALITY_HIGH -> 0
                    QUALITY_MEDIUM -> 60
                    QUALITY_LOW -> 98
                    else -> 60
                }
                Pair(4, q)
            }
            FORMAT_ASTC_6x6 -> {
                val q = when (quality) {
                    QUALITY_HIGH -> 0
                    QUALITY_MEDIUM -> 60
                    QUALITY_LOW -> 98
                    else -> 60
                }
                Pair(6, q)
            }
            FORMAT_ASTC_8x8 -> {
                val q = when (quality) {
                    QUALITY_HIGH -> 0
                    QUALITY_MEDIUM -> 60
                    QUALITY_LOW -> 98
                    else -> 60
                }
                Pair(8, q)
            }
            FORMAT_ETC2_RGB -> {
                val q = when (quality) {
                    QUALITY_HIGH -> 0
                    QUALITY_MEDIUM -> 60
                    QUALITY_LOW -> 98
                    else -> 60
                }
                Pair(4, q)
            }
            FORMAT_ETC2_RGBA -> {
                val q = when (quality) {
                    QUALITY_HIGH -> 0
                    QUALITY_MEDIUM -> 60
                    QUALITY_LOW -> 98
                    else -> 60
                }
                Pair(4, q)
            }
            else -> Pair(4, 60)
        }
    }
    
    /**
     * Compress bitmap and save to file
     * Note: This is a simplified implementation
     * Real implementation would use ASTC/ETC2 encoder
     */
    private fun compressAndSave(
        bitmap: Bitmap,
        outputPath: String,
        blockSize: Int,
        quality: Int
    ): Boolean {
        try {
            // For now, save as compressed PNG
            // TODO: Implement actual ASTC/ETC2 encoding
            val outputStream = FileOutputStream(outputPath)
            val success = bitmap.compress(Bitmap.CompressFormat.PNG, quality, outputStream)
            outputStream.close()
            return success
        } catch (e: Exception) {
            Log.e(TAG, "Failed to save compressed texture", e)
            return false
        }
    }
    
    /**
     * Get estimated compression ratio for format
     */
    fun getCompressionRatio(format: Int): Float {
        return when (format) {
            FORMAT_ASTC_4x4 -> 0.25f  // 4:1 compression
            FORMAT_ASTC_6x6 -> 0.11f  // 9:1 compression
            FORMAT_ASTC_8x8 -> 0.0625f // 16:1 compression
            FORMAT_ETC2_RGB -> 0.25f  // 4:1 compression
            FORMAT_ETC2_RGBA -> 0.25f // 4:1 compression
            else -> 0.25f
        }
    }
    
    /**
     * Get format name for display
     */
    fun getFormatName(format: Int): String {
        return when (format) {
            FORMAT_ASTC_4x4 -> "ASTC 4x4"
            FORMAT_ASTC_6x6 -> "ASTC 6x6"
            FORMAT_ASTC_8x8 -> "ASTC 8x8"
            FORMAT_ETC2_RGB -> "ETC2 RGB"
            FORMAT_ETC2_RGBA -> "ETC2 RGBA"
            else -> "Unknown"
        }
    }
}
