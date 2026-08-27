package com.example.oblivion

import android.app.Activity
import android.media.AudioManager
import android.media.MediaPlayer
import android.media.SoundPool
import android.os.Bundle
import android.util.Log
import java.io.IOException
import java.io.File

class MainActivity : Activity() {

    private var gameSurfaceView: GameSurfaceView? = null
    private var mediaPlayer: MediaPlayer? = null
    private var soundPool: SoundPool? = null
    private val loadedSounds = mutableMapOf<String, Int>() // filename → soundId
    private var spLoadListener: SoundPool.OnLoadCompleteListener? = null

    companion object {
        private const val TAG = "MainActivity"
        @Volatile
        private var instance: MainActivity? = null

        fun getInstance(): MainActivity? = instance
    }

    fun playBGM(filename: String) {
        runOnUiThread { playBGMInternal(filename) }
    }

    fun stopBGM() {
        runOnUiThread {
            mediaPlayer?.let {
                if (it.isPlaying) {
                    it.stop()
                    Log.i(TAG, "BGM stopped")
                }
            }
        }
    }

    fun playSE(filename: String) {
        runOnUiThread { playSEInternal(filename) }
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

                // データパスを設定（BSA/ESMファイルの検索パス）
                // nativeInitEngine() よりも前に呼ぶ必要がある。
                // Renderer.init() は onSurfaceCreated → nativeInitEngine() の中で呼ばれ、
                // その時点で BSA ロードを行うため、データパスは事前に登録必須。
                try {
                    val dataPath = filesDir.absolutePath + File.separator + "data"
                    val dataDir = java.io.File(dataPath)
                    if (!dataDir.exists()) {
                        dataDir.mkdirs()
                        Log.i(TAG, "Created data directory: $dataPath")
                    }
                    // 静的ファイル変数に保持（GameRenderer から参照される）
                    GameRenderer.dataPath = dataPath
                    // ネイティブ側にデータパスを登録（Engine 初期化前）
                    GameRenderer.nativeSetDataPath(dataPath)
                    Log.i(TAG, "BSA data path registered to native: $dataPath")
                } catch (e: Exception) {
                    Log.w(TAG, "Failed to set BSA data path: ${e.message}")
                }
            } catch (e: Exception) {
                Log.e(TAG, "Failed to initialize audio", e)
            }
        }

    override fun onPause() {
        super.onPause()
        Log.i(TAG, "onPause")

        gameSurfaceView?.onPause()
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
        if (instance === this) {
            instance = null
        }
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

            val assetPath = "audio/music/$filename"
            Log.i(TAG, "Loading BGM: $assetPath")

            val afd = assets.openFd(assetPath)
            mp.setDataSource(afd.fileDescriptor, afd.startOffset, afd.length)
            afd.close()
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

            // Cache hit → play immediately
            loadedSounds[filename]?.let { cachedId ->
                sp.play(cachedId, 1.0f, 1.0f, 0, 0, 1.0f)
                Log.i(TAG, "SE playing from cache: $filename")
                return
            }

            val assetPath = "audio/sounds/$filename"
            Log.i(TAG, "Loading SE: $assetPath")

            val afd = assets.openFd(assetPath)
            val soundId = sp.load(afd, 1)
            afd.close()
            loadedSounds[filename] = soundId

            // Set listener only once
            if (spLoadListener == null) {
                spLoadListener = SoundPool.OnLoadCompleteListener { pool, sampleId, status ->
                    if (status == 0) {
                        pool.play(sampleId, 1.0f, 1.0f, 0, 0, 1.0f)
                    } else {
                        Log.e(TAG, "Failed to load SE, status=$status")
                    }
                }
                sp.setOnLoadCompleteListener(spLoadListener)
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
            loadedSounds.clear()
            spLoadListener = null
            soundPool = null
        } catch (e: Exception) {
            Log.e(TAG, "Error during audio cleanup", e)
        }
    }
}
