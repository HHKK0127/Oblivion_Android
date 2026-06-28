package com.example.oblivion

import android.app.Activity
import android.media.AudioManager
import android.media.MediaPlayer
import android.media.SoundPool
import android.os.Bundle
import android.util.Log
import java.io.IOException

class MainActivity : Activity() {

    private var gameSurfaceView: GameSurfaceView? = null
    private var mediaPlayer: MediaPlayer? = null
    private var soundPool: SoundPool? = null

    companion object {
        private const val TAG = "MainActivity"
        private var instance: MainActivity? = null

        @JvmStatic
        fun playBGM(filename: String) {
            instance?.playBGMInternal(filename) ?: Log.w(TAG, "MainActivity instance not available")
        }

        @JvmStatic
        fun stopBGM() {
            instance?.mediaPlayer?.let {
                if (it.isPlaying) {
                    it.stop()
                    Log.i(TAG, "BGM stopped")
                }
            }
        }

        @JvmStatic
        fun playSE(filename: String) {
            instance?.playSEInternal(filename) ?: Log.w(TAG, "MainActivity instance not available")
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.i(TAG, "=== onCreate called ===")

        instance = this

        try {
            Log.i(TAG, "Initializing audio system")
            initializeAudio()

            Log.i(TAG, "Creating GameSurfaceView")
            gameSurfaceView = GameSurfaceView(this)
            Log.i(TAG, "Setting GameSurfaceView as content view")
            setContentView(gameSurfaceView)
            Log.i(TAG, "ContentView set successfully")
        } catch (e: Exception) {
            Log.e(TAG, "Exception in onCreate: ${e.message}", e)
        }
    }

    private fun initializeAudio() {
        try {
            mediaPlayer = MediaPlayer()
            volumeControlStream = AudioManager.STREAM_MUSIC
            Log.i(TAG, "MediaPlayer initialized for BGM")

            soundPool = SoundPool.Builder().setMaxStreams(5).build()
            Log.i(TAG, "SoundPool initialized for SE (max 5 sounds)")

            try {
                GameRenderer.nativeInitAudioBridge(assets, this)
                Log.i(TAG, "Audio bridge initialized with AssetManager and MainActivity")
            } catch (e: Exception) {
                Log.w(TAG, "Failed to initialize audio bridge: ${e.message}")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to initialize audio", e)
        }
    }

    override fun onPause() {
        super.onPause()
        Log.i(TAG, "onPause")

        gameSurfaceView?.let { view ->
            if (view.getGameRenderer() != null) {
                Log.i(TAG, "Pausing game surface view")
                view.onPause()
            } else {
                Log.i(TAG, "GameRenderer not yet created, deferring pause")
            }
        }

        mediaPlayer?.let {
            if (it.isPlaying) {
                it.pause()
                Log.i(TAG, "BGM paused")
            }
        }
    }

    override fun onResume() {
        super.onResume()
        Log.i(TAG, "onResume")
        gameSurfaceView?.onResume()
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.i(TAG, "onDestroy - cleaning up audio")
        cleanupAudio()
        instance = null
    }

    private fun playBGMInternal(filename: String) {
        try {
            val mp = mediaPlayer
            if (mp == null) {
                Log.e(TAG, "MediaPlayer not initialized")
                return
            }

            if (mp.isPlaying) {
                mp.stop()
            }
            mp.reset()

            val assetPath = "file:///android_asset/audio/music/$filename"
            Log.i(TAG, "Loading BGM: $assetPath")

            mp.setDataSource(assetPath)
            mp.prepare()
            mp.isLooping = true
            mp.start()

            Log.i(TAG, "BGM playing: $filename")
        } catch (e: IOException) {
            Log.e(TAG, "Failed to play BGM: $filename", e)
        }
    }

    private fun playSEInternal(filename: String) {
        try {
            val sp = soundPool
            if (sp == null) {
                Log.e(TAG, "SoundPool not initialized")
                return
            }

            val assetPath = "audio/sounds/$filename"
            Log.i(TAG, "Loading SE: $assetPath")

            // For raw loading from assets
            val afd = assets.openFd(assetPath)
            val soundId = sp.load(afd, 1)
            
            // To play immediately, we typically need to wait for load completion in SoundPool.
            // For this port keeping logic similar to original.
            sp.setOnLoadCompleteListener { pool, sampleId, status ->
                if (status == 0) {
                    pool.play(sampleId, 1.0f, 1.0f, 0, 0, 1.0f)
                }
            }

            Log.i(TAG, "SE playing queued: $filename")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to play SE: $filename", e)
        }
    }

    private fun cleanupAudio() {
        try {
            mediaPlayer?.let {
                if (it.isPlaying) {
                    it.stop()
                }
                it.release()
                Log.i(TAG, "MediaPlayer released")
            }
            mediaPlayer = null

            soundPool?.let {
                it.release()
                Log.i(TAG, "SoundPool released")
            }
            soundPool = null
        } catch (e: Exception) {
            Log.e(TAG, "Error during audio cleanup", e)
        }
    }
}
