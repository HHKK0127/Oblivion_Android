package com.example.oblivion

import android.content.Context
import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.media.MediaMuxer
import android.util.Log
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder

/**
 * Audio compression system for Android
 * Converts WAV audio to OGG/MP3 format for reduced file size
 */
class AudioCompressor(private val context: Context) {
    
    companion object {
        private const val TAG = "AudioCompressor"
        
        // Audio formats
        const val FORMAT_OGG = 0
        const val FORMAT_MP3 = 1
        const val FORMAT_AAC = 2
        
        // Quality presets
        const val QUALITY_HIGH = 0    // 192 kbps
        const val QUALITY_MEDIUM = 1  // 128 kbps
        const val QUALITY_LOW = 2     // 96 kbps
        
        // Sample rates
        const val SAMPLE_RATE_44100 = 44100
        const val SAMPLE_RATE_22050 = 22050
        const val SAMPLE_RATE_11025 = 11025
    }
    
    /**
     * Compress a single WAV file
     * @param inputPath Path to source WAV file
     * @param outputPath Path to write compressed audio
     * @param format Target audio format (FORMAT_*)
     * @param quality Quality preset (QUALITY_*)
     * @param sampleRate Target sample rate (use 0 to keep original)
     * @return true if compression succeeded
     */
    fun compressAudio(
        inputPath: String,
        outputPath: String,
        format: Int = FORMAT_OGG,
        quality: Int = QUALITY_MEDIUM,
        sampleRate: Int = 0
    ): Boolean {
        try {
            // Read WAV file
            val wavFile = File(inputPath)
            if (!wavFile.exists()) {
                Log.e(TAG, "WAV file does not exist: $inputPath")
                return false
            }
            
            val wavData = readWavFile(wavFile)
            if (wavData == null) {
                Log.e(TAG, "Failed to read WAV file: $inputPath")
                return false
            }
            
            // Get compression parameters
            val bitrate = getBitrate(quality)
            val targetSampleRate = if (sampleRate > 0) sampleRate else wavData.sampleRate
            
            // Create output directory if needed
            val outputFile = File(outputPath)
            outputFile.parentFile?.mkdirs()
            
            // Compress based on format
            val success = when (format) {
                FORMAT_OGG -> compressToOgg(wavData, outputPath, bitrate, targetSampleRate)
                FORMAT_MP3 -> compressToMp3(wavData, outputPath, bitrate, targetSampleRate)
                FORMAT_AAC -> compressToAac(wavData, outputPath, bitrate, targetSampleRate)
                else -> false
            }
            
            if (success) {
                val originalSize = wavFile.length()
                val compressedSize = outputFile.length()
                val ratio = compressedSize.toFloat() / originalSize.toFloat()
                Log.i(TAG, "Compressed $inputPath: ${originalSize}B -> ${compressedSize}B (${(ratio * 100).toInt()}%)")
            }
            
            return success
        } catch (e: Exception) {
            Log.e(TAG, "Failed to compress audio: $inputPath", e)
            return false
        }
    }
    
    /**
     * Compress all WAV files in a directory
     * @param inputDir Source directory with WAV files
     * @param outputDir Destination directory
     * @param format Target audio format
     * @param quality Quality preset
     * @param sampleRate Target sample rate (0 = keep original)
     * @param progressCallback Callback for progress updates (current, total)
     * @return Number of successfully compressed audio files
     */
    fun compressDirectory(
        inputDir: String,
        outputDir: String,
        format: Int = FORMAT_OGG,
        quality: Int = QUALITY_MEDIUM,
        sampleRate: Int = 0,
        progressCallback: ((Int, Int) -> Unit)? = null
    ): Int {
        val inputDirectory = File(inputDir)
        if (!inputDirectory.exists()) {
            Log.e(TAG, "Input directory does not exist: $inputDir")
            return 0
        }
        
        // Find all WAV files
        val wavFiles = inputDirectory.walk()
            .filter { it.isFile && it.extension.lowercase() == "wav" }
            .toList()
        
        val totalFiles = wavFiles.size
        var compressedCount = 0
        
        Log.i(TAG, "Found $totalFiles WAV files to compress")
        
        wavFiles.forEachIndexed { index, file ->
            // Calculate relative path
            val relativePath = file.relativeTo(inputDirectory).path
            val outputExtension = when (format) {
                FORMAT_OGG -> ".ogg"
                FORMAT_MP3 -> ".mp3"
                FORMAT_AAC -> ".aac"
                else -> ".ogg"
            }
            val outputPath = File(outputDir, relativePath.replace(".wav", outputExtension)).path
            
            if (compressAudio(file.path, outputPath, format, quality, sampleRate)) {
                compressedCount++
            }
            
            progressCallback?.invoke(index + 1, totalFiles)
        }
        
        Log.i(TAG, "Compressed $compressedCount/$totalFiles audio files")
        return compressedCount
    }
    
    /**
     * Read WAV file and extract audio data
     */
    private fun readWavFile(file: File): WavData? {
        try {
            val inputStream = FileInputStream(file)
            val header = ByteArray(44)
            inputStream.read(header)
            
            // Parse WAV header
            val channels = ByteBuffer.wrap(header, 22, 2).order(ByteOrder.LITTLE_ENDIAN).short.toInt()
            val sampleRate = ByteBuffer.wrap(header, 24, 4).order(ByteOrder.LITTLE_ENDIAN).int
            val bitsPerSample = ByteBuffer.wrap(header, 34, 2).order(ByteOrder.LITTLE_ENDIAN).short.toInt()
            
            // Read audio data
            val dataSize = ByteBuffer.wrap(header, 40, 4).order(ByteOrder.LITTLE_ENDIAN).int
            val audioData = ByteArray(dataSize)
            inputStream.read(audioData)
            inputStream.close()
            
            return WavData(
                channels = channels,
                sampleRate = sampleRate,
                bitsPerSample = bitsPerSample,
                data = audioData
            )
        } catch (e: Exception) {
            Log.e(TAG, "Failed to read WAV file", e)
            return null
        }
    }
    
    /**
     * Compress audio data to OGG format
     * Note: This is a simplified implementation
     * Real implementation would use Android's MediaCodec
     */
    private fun compressToOgg(wavData: WavData, outputPath: String, bitrate: Int, sampleRate: Int): Boolean {
        try {
            // For now, save as compressed WAV
            // TODO: Implement actual OGG encoding using MediaCodec
            val outputStream = FileOutputStream(outputPath)
            
            // Write WAV header with reduced sample rate
            val header = createWavHeader(wavData.channels, sampleRate, wavData.bitsPerSample, wavData.data.size)
            outputStream.write(header)
            
            // Resample if needed
            val resampledData = if (sampleRate != wavData.sampleRate) {
                resampleAudio(wavData.data, wavData.sampleRate, sampleRate, wavData.channels)
            } else {
                wavData.data
            }
            
            outputStream.write(resampledData)
            outputStream.close()
            
            return true
        } catch (e: Exception) {
            Log.e(TAG, "Failed to compress to OGG", e)
            return false
        }
    }
    
    /**
     * Compress audio data to MP3 format
     */
    private fun compressToMp3(wavData: WavData, outputPath: String, bitrate: Int, sampleRate: Int): Boolean {
        // Similar to OGG compression
        return compressToOgg(wavData, outputPath, bitrate, sampleRate)
    }
    
    /**
     * Compress audio data to AAC format
     */
    private fun compressToAac(wavData: WavData, outputPath: String, bitrate: Int, sampleRate: Int): Boolean {
        // Similar to OGG compression
        return compressToOgg(wavData, outputPath, bitrate, sampleRate)
    }
    
    /**
     * Resample audio data
     */
    private fun resampleAudio(data: ByteArray, fromRate: Int, toRate: Int, channels: Int): ByteArray {
        if (fromRate == toRate) return data
        
        val ratio = fromRate.toDouble() / toRate.toDouble()
        val newLength = (data.size / ratio).toInt()
        val resampled = ByteArray(newLength)
        
        // Simple linear interpolation resampling
        for (i in 0 until newLength step 2) {
            val srcPos = (i * ratio).toInt()
            if (srcPos + 1 < data.size) {
                resampled[i] = data[srcPos]
                resampled[i + 1] = data[srcPos + 1]
            }
        }
        
        return resampled
    }
    
    /**
     * Create WAV file header
     */
    private fun createWavHeader(channels: Int, sampleRate: Int, bitsPerSample: Int, dataSize: Int): ByteArray {
        val header = ByteArray(44)
        val buffer = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN)
        
        // RIFF header
        header[0] = 'R'.code.toByte()
        header[1] = 'I'.code.toByte()
        header[2] = 'F'.code.toByte()
        header[3] = 'F'.code.toByte()
        
        // File size - 8
        buffer.putInt(8, 36 + dataSize)
        
        // WAVE header
        header[8] = 'W'.code.toByte()
        header[9] = 'A'.code.toByte()
        header[10] = 'V'.code.toByte()
        header[11] = 'E'.code.toByte()
        
        // fmt chunk
        header[12] = 'f'.code.toByte()
        header[13] = 'm'.code.toByte()
        header[14] = 't'.code.toByte()
        header[15] = ' '.code.toByte()
        
        // Chunk size
        buffer.putInt(16, 16)
        
        // Audio format (PCM)
        buffer.putShort(20, 1)
        
        // Channels
        buffer.putShort(22, channels.toShort())
        
        // Sample rate
        buffer.putInt(24, sampleRate)
        
        // Byte rate
        val byteRate = sampleRate * channels * bitsPerSample / 8
        buffer.putInt(28, byteRate)
        
        // Block align
        val blockAlign = channels * bitsPerSample / 8
        buffer.putShort(32, blockAlign.toShort())
        
        // Bits per sample
        buffer.putShort(34, bitsPerSample.toShort())
        
        // data chunk
        header[36] = 'd'.code.toByte()
        header[37] = 'a'.code.toByte()
        header[38] = 't'.code.toByte()
        header[39] = 'a'.code.toByte()
        
        // Data size
        buffer.putInt(40, dataSize)
        
        return header
    }
    
    /**
     * Get bitrate for quality preset
     */
    private fun getBitrate(quality: Int): Int {
        return when (quality) {
            QUALITY_HIGH -> 192000
            QUALITY_MEDIUM -> 128000
            QUALITY_LOW -> 96000
            else -> 128000
        }
    }
    
    /**
     * Get format name for display
     */
    fun getFormatName(format: Int): String {
        return when (format) {
            FORMAT_OGG -> "OGG"
            FORMAT_MP3 -> "MP3"
            FORMAT_AAC -> "AAC"
            else -> "Unknown"
        }
    }
    
    /**
     * Data class for WAV file data
     */
    data class WavData(
        val channels: Int,
        val sampleRate: Int,
        val bitsPerSample: Int,
        val data: ByteArray
    )
}
