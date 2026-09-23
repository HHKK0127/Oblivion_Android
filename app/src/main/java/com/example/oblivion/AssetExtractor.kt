package com.example.oblivion

import android.content.Context
import android.util.Log
import java.io.*

/**
 * Asset extraction system for Oblivion Android.
 * Moves large game assets from APK to app-specific external storage (no permission needed).
 */
class AssetExtractor(private val context: Context) {
    
    companion object {
        private const val TAG = "AssetExtractor"
        private const val ASSET_DIR = "oblivion_assets"
        private const val VERSION_FILE = "version.txt"
        private const val CURRENT_VERSION = 6  // v4: intro OP clips; v5: added oblivion_iv_logo; v6: added credits_menu (1% title easter egg)
        private const val EXTRACTION_MARKER = ".extraction_complete"
    }
    
    private val externalDir: File
        get() = File(
            context.getExternalFilesDir(null),
            ASSET_DIR
        )
    
    /**
     * Check if assets need extraction.
     */
    fun needsExtraction(): Boolean {
        val marker = File(externalDir, EXTRACTION_MARKER)
        if (!marker.exists()) return true
        
        val versionFile = File(externalDir, VERSION_FILE)
        if (!versionFile.exists()) return true
        
        return try {
            val version = versionFile.readText().trim().toInt()
            version < CURRENT_VERSION
        } catch (e: Exception) {
            true
        }
    }
    
    /**
     * Extract assets to external storage.
     * @param progressCallback Called with (current, total) progress
     */
    fun extractAssets(progressCallback: (Int, Int) -> Unit): Boolean {
        return try {
            // Create directory
            externalDir.mkdirs()
            
            // Extract videos first (large files, separate step)
            extractVideos(progressCallback)
            
            // Get list of other assets to extract
            val assets = listAssets("").filter { !it.startsWith("videos/") }
            val total = assets.size
            
            Log.i(TAG, "Extracting $total non-video assets to ${externalDir.absolutePath}")
            
            // Extract each asset
            assets.forEachIndexed { index, assetPath ->
                extractAsset(assetPath)
                progressCallback(index + 1, total)
            }
            
            // Write version file and marker atomically
            val versionDir = externalDir
            val marker = File(versionDir, EXTRACTION_MARKER)
            val tempMarker = File(versionDir, ".extraction_in_progress")
            tempMarker.writeText(CURRENT_VERSION.toString())
            
            File(versionDir, VERSION_FILE).writeText(CURRENT_VERSION.toString())
            tempMarker.renameTo(marker)
            
            Log.i(TAG, "Asset extraction complete")
            true
        } catch (e: Exception) {
            Log.e(TAG, "Asset extraction failed", e)
            false
        }
    }
    
    /**
     * Extract video files from assets to external storage.
     * Uses atomic extraction: writes to temp directory first, then renames on success.
     */
    private fun extractVideos(progressCallback: (Int, Int) -> Unit) {
        // List video assets
        val videoAssets = try {
            context.assets.list("videos") ?: emptyArray()
        } catch (e: Exception) {
            Log.w(TAG, "No videos directory in assets")
            emptyArray()
        }
        
        if (videoAssets.isEmpty()) {
            Log.i(TAG, "No video assets to extract")
            return
        }
        
        // Check if videos already extracted
        val videoDir = File(externalDir, "videos")
        val allExist = videoAssets.all { fileName ->
            val f = File(videoDir, fileName)
            f.exists() && f.length() > 0
        }
        if (allExist) {
            Log.i(TAG, "Videos already extracted")
            return
        }
        
        Log.i(TAG, "Extracting ${videoAssets.size} video files...")
        
        // Use temp directory for atomic extraction
        val tempDir = File(externalDir, ".videos_temp")
        tempDir.mkdirs()
        
        try {
            videoAssets.forEachIndexed { index, fileName ->
                val outputFile = File(tempDir, fileName)
                if (outputFile.exists() && outputFile.length() > 0) {
                    Log.d(TAG, "Video already in temp: $fileName")
                    progressCallback(index + 1, videoAssets.size)
                    return@forEachIndexed
                }
                
                context.assets.open("videos/$fileName").use { input ->
                    FileOutputStream(outputFile).use { output ->
                        input.copyTo(output)
                    }
                }
                Log.i(TAG, "Extracted video to temp: $fileName (${outputFile.length()} bytes)")
                progressCallback(index + 1, videoAssets.size)
            }
            
            // All videos extracted successfully, move to final location
            videoDir.mkdirs()
            tempDir.listFiles()?.forEach { file ->
                val target = File(videoDir, file.name)
                file.renameTo(target)
            }
            tempDir.delete()
            
            Log.i(TAG, "Video extraction complete")
        } catch (e: Exception) {
            Log.e(TAG, "Video extraction failed, cleaning up temp", e)
            tempDir.deleteRecursively()
            throw e
        }
    }
    
    /**
     * Get the path to an asset, preferring external storage.
     */
    fun getAssetPath(assetPath: String): String {
        val externalFile = File(externalDir, assetPath)
        return if (externalFile.exists()) {
            externalFile.absolutePath
        } else {
            // Fall back to APK assets
            "assets/$assetPath"
        }
    }
    
    /**
     * Check if an asset exists in external storage.
     */
    fun hasExternalAsset(assetPath: String): Boolean {
        return File(externalDir, assetPath).exists()
    }
    
    /**
     * List all assets in the APK.
     */
    private fun listAssets(path: String): List<String> {
        val result = mutableListOf<String>()
        val assets = context.assets.list(path) ?: return emptyList()
        
        for (asset in assets) {
            val fullPath = if (path.isEmpty()) asset else "$path/$asset"
            val subAssets = context.assets.list(fullPath)
            
            if (subAssets == null || subAssets.isEmpty()) {
                // It's a file
                result.add(fullPath)
            } else {
                // It's a directory
                result.addAll(listAssets(fullPath))
            }
        }
        
        return result
    }
    
    /**
     * Extract a single asset to external storage.
     */
    private fun extractAsset(assetPath: String) {
        val outputFile = File(externalDir, assetPath)
        
        // Create parent directories
        outputFile.parentFile?.mkdirs()
        
        // Copy asset
        context.assets.open(assetPath).use { input ->
            FileOutputStream(outputFile).use { output ->
                input.copyTo(output)
            }
        }
    }
    
    /**
     * Get total size of assets to extract.
     */
    fun getAssetSize(): Long {
        var totalSize = 0L
        
        listAssets("").forEach { assetPath ->
            try {
                context.assets.open(assetPath).use { input ->
                    totalSize += input.available()
                }
            } catch (e: Exception) {
                // Skip errors
            }
        }
        
        return totalSize
    }
    
    /**
     * Clean up extracted assets.
     */
    fun cleanup() {
        externalDir.deleteRecursively()
        Log.i(TAG, "Cleaned up extracted assets")
    }
}
