package com.example.oblivion

import android.app.Activity
import android.content.Intent
import android.media.MediaPlayer
import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.WindowManager
import android.widget.FrameLayout
import java.io.File

/**
 * IntroVideoActivity - Plays the Oblivion intro video before launching the game.
 * Uses a simple SurfaceView + MediaPlayer approach for reliable playback.
 * After video completes or user taps to skip, launches MainActivity.
 */
class IntroVideoActivity : Activity(), SurfaceHolder.Callback {

    companion object {
        private const val TAG = "IntroVideoActivity"
    }

    private var mediaPlayer: MediaPlayer? = null
    private var surfaceView: SurfaceView? = null
    private var surfaceReady = false
    private var videoPrepared = false
    private var videoCompleted = false
    private var videoStarted = false
    private val handler = android.os.Handler(android.os.Looper.getMainLooper())

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Fullscreen immersive
        window.setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        )
        window.setFlags(
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON,
            WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON
        )

        // Hide system UI
        window.decorView.systemUiVisibility = (
            android.view.View.SYSTEM_UI_FLAG_FULLSCREEN
            or android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            or android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        )

        // Create a black background with a SurfaceView
        val frameLayout = FrameLayout(this)
        frameLayout.setBackgroundColor(android.graphics.Color.BLACK)

        surfaceView = SurfaceView(this).apply {
            holder.addCallback(this@IntroVideoActivity)
        }
        frameLayout.addView(surfaceView)
        setContentView(frameLayout)

        Log.i(TAG, "IntroVideoActivity created")
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        surfaceReady = true
        Log.i(TAG, "Surface created")
        tryStartPlayback()
        
        // Timeout: if video doesn't start within 3 seconds, launch game
        handler.postDelayed({
            if (!videoStarted && !videoCompleted) {
                Log.w(TAG, "Video start timeout, launching game")
                launchGame()
            }
        }, 3000)
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        Log.i(TAG, "Surface changed: ${width}x${height}")
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        surfaceReady = false
        releasePlayer()
    }

    private fun tryStartPlayback() {
        if (videoCompleted || videoStarted) return
        
        // Take local copy to avoid race condition
        val s = surfaceView?.holder?.surface
        val sv = surfaceView
        if (s == null || !surfaceReady || sv == null) return

        // Find the video file: try external storage first, then assets
        val videoFile = findVideoFile()
        if (videoFile == null) {
            Log.w(TAG, "Video file not found, skipping to game")
            launchGame()
            return
        }

        try {
            Log.i(TAG, "Starting video playback: $videoFile")
            videoStarted = true
            mediaPlayer = MediaPlayer().apply {
                setDataSource(videoFile)
                setDisplay(sv.holder)
                setOnPreparedListener {
                    Log.i(TAG, "Video prepared: ${videoWidth}x${videoHeight}")
                    videoPrepared = true
                    // Scale surface to fit video aspect ratio
                    adjustSurfaceSize(videoWidth, videoHeight)
                    start()
                }
                setOnCompletionListener {
                    Log.i(TAG, "Video playback completed")
                    videoCompleted = true
                    releasePlayer()
                    launchGame()
                }
                setOnErrorListener { _, what, extra ->
                    Log.e(TAG, "Video error: what=$what, extra=$extra")
                    videoCompleted = true
                    releasePlayer()
                    launchGame()
                    true
                }
                setOnVideoSizeChangedListener { mp, width, height ->
                    try {
                        adjustSurfaceSize(width, height)
                    } catch (e: Exception) {
                        Log.e(TAG, "onVideoSizeChanged error", e)
                    }
                }
                prepareAsync()
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to start video: ${e.message}")
            videoStarted = false
            launchGame()
        }
    }

    private fun findVideoFile(): String? {
        // 1. Try external storage (extracted from assets)
        val extDir = getExternalFilesDir(null)
        if (extDir != null) {
            val extVideo = File(extDir, "oblivion_assets/videos/oblivion_intro.mp4")
            if (extVideo.exists() && extVideo.length() > 0) {
                Log.i(TAG, "Found video at external: ${extVideo.absolutePath}")
                return extVideo.absolutePath
            }
        }

        // 2. Try filesDir
        val filesVideo = File(filesDir, "oblivion_assets/videos/oblivion_intro.mp4")
        if (filesVideo.exists() && filesVideo.length() > 0) {
            Log.i(TAG, "Found video at filesDir: ${filesVideo.absolutePath}")
            return filesVideo.absolutePath
        }

        // 3. Copy from assets to cache and use that
        try {
            val cacheVideo = File(cacheDir, "oblivion_intro.mp4")
            if (cacheVideo.exists() && cacheVideo.length() > 0) {
                Log.i(TAG, "Found video at cache: ${cacheVideo.absolutePath}")
                return cacheVideo.absolutePath
            }
            assets.open("videos/oblivion_intro.mp4").use { input ->
                cacheVideo.outputStream().use { output ->
                    input.copyTo(output)
                }
            }
            if (cacheVideo.exists() && cacheVideo.length() > 0) {
                Log.i(TAG, "Copied video to cache: ${cacheVideo.absolutePath}")
                return cacheVideo.absolutePath
            }
        } catch (e: Exception) {
            Log.w(TAG, "Could not copy video from assets: ${e.message}")
        }

        return null
    }

    private fun adjustSurfaceSize(videoWidth: Int, videoHeight: Int) {
        val sv = surfaceView ?: return
        val parent = sv.parent as? FrameLayout ?: return
        val displayMetrics = resources.displayMetrics
        val screenW = displayMetrics.widthPixels
        val screenH = displayMetrics.heightPixels

        if (videoWidth <= 0 || videoHeight <= 0) return

        val videoAspect = videoWidth.toFloat() / videoHeight.toFloat()
        val screenAspect = screenW.toFloat() / screenH.toFloat()

        val newW: Int
        val newH: Int
        if (videoAspect > screenAspect) {
            // Video is wider than screen - fit to width
            newW = screenW
            newH = (screenW / videoAspect).toInt()
        } else {
            // Video is taller than screen - fit to height
            newH = screenH
            newW = (screenH * videoAspect).toInt()
        }

        val params = sv.layoutParams as FrameLayout.LayoutParams
        params.width = newW
        params.height = newH
        params.gravity = android.view.Gravity.CENTER
        sv.layoutParams = params

        Log.i(TAG, "Surface adjusted: ${newW}x${newH} (video: ${videoWidth}x${videoHeight})")
    }

    private fun releasePlayer() {
        try {
            mediaPlayer?.let {
                if (it.isPlaying) it.stop()
                it.release()
            }
        } catch (e: Exception) {
            Log.w(TAG, "Error releasing player: ${e.message}")
        }
        mediaPlayer = null
        videoPrepared = false
    }

    private fun launchGame() {
        if (isFinishing) return
        Log.i(TAG, "Launching MainActivity")
        val intent = Intent(this, MainActivity::class.java)
        startActivity(intent)
        finish()
    }

    override fun onTouchEvent(event: android.view.MotionEvent): Boolean {
        if (event.action == android.view.MotionEvent.ACTION_DOWN) {
            if (mediaPlayer?.isPlaying == true) {
                Log.i(TAG, "User tapped to skip video")
                videoCompleted = true
                releasePlayer()
                launchGame()
                return true
            }
        }
        return super.onTouchEvent(event)
    }

    override fun onPause() {
        super.onPause()
        if (mediaPlayer?.isPlaying == true) {
            mediaPlayer?.pause()
        }
    }

    override fun onResume() {
        super.onResume()
        if (videoPrepared && !videoCompleted) {
            mediaPlayer?.start()
        }
    }

    override fun onDestroy() {
        releasePlayer()
        super.onDestroy()
    }
}
