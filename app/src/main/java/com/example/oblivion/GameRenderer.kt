package com.example.oblivion

import android.content.res.AssetManager
import android.graphics.SurfaceTexture
import android.media.MediaPlayer
import android.opengl.GLES11Ext
import android.opengl.GLES20
import android.opengl.GLSurfaceView
import android.util.Log
import android.view.Surface
import javax.microedition.khronos.egl.EGLConfig
import javax.microedition.khronos.opengles.GL10

class GameRenderer : GLSurfaceView.Renderer {

    private var nativeEngineHandle: Long = 0
    private var gameSurfaceView: GameSurfaceView? = null
    private var appContext: android.content.Context? = null
    private var frameCount = 0
    private var lastLogTime: Long = 0
    private var onExitRequested: (() -> Unit)? = null

    // Title screen video background
    private var titleVideoPlayer: MediaPlayer? = null
    private var titleVideoSurfaceTexture: SurfaceTexture? = null
    private var titleVideoTextureId: Int = 0
    private var titleVideoReady: Boolean = false
    private var titleVideoInitAttempted: Boolean = false

    companion object {
        private const val TAG = "GameRenderer"
        var dataPath: String = ""

        init {
            try {
                System.loadLibrary("native-lib")
                Log.i(TAG, "Native library loaded successfully")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "FATAL: Failed to load native library - ${e.message}", e)
                throw e
            }
        }

        @JvmStatic
        external fun nativeInitAudioBridge(assetManager: AssetManager, mainActivity: Any)

        @JvmStatic
        external fun nativeSetDataPath(path: String)

        @JvmStatic
        external fun nativeInitBinkVideo(surface: android.view.Surface, videoBasePath: String): Boolean

        @JvmStatic
        external fun nativePlayVideo(clipId: String, loop: Boolean): Boolean

        @JvmStatic
        external fun nativeStopVideo(): Boolean

        @JvmStatic
        external fun nativeShutdownBinkVideo()

        @JvmStatic
        external fun nativeSetTitleVideoTexture(textureId: Int)

        @JvmStatic
        external fun nativeUpdateTitleVideoTexture()
    }

    constructor()

    constructor(surfaceView: GameSurfaceView) {
        this.gameSurfaceView = surfaceView
        Log.i(TAG, "GameRenderer created with GameSurfaceView reference")
    }

    fun setContext(context: android.content.Context) {
        this.appContext = context.applicationContext
        Log.i(TAG, "Application context set")
    }

    fun setOnExitRequestedListener(listener: () -> Unit) {
        onExitRequested = listener
    }

    override fun onSurfaceCreated(gl: GL10, config: EGLConfig) {
        android.util.Log.wtf(TAG, "===== onSurfaceCreated CALLED - THIS SHOULD APPEAR IN LOGS =====")
        Log.i(TAG, "=== onSurfaceCreated called ===")

        try {
            val vendor = gl.glGetString(GL10.GL_VENDOR)
            val rendererName = gl.glGetString(GL10.GL_RENDERER)
            val version = gl.glGetString(GL10.GL_VERSION)

            Log.i(TAG, "OpenGL Info - Vendor: $vendor")
            Log.i(TAG, "OpenGL Info - Renderer: $rendererName")
            Log.i(TAG, "OpenGL Info - Version: $version")
        } catch (e: Exception) {
            Log.e(TAG, "Error getting GL info: ${e.message}", e)
        }

        try {
            Log.i(TAG, "Calling nativeInitEngine()")
            nativeEngineHandle = nativeInitEngine()
            Log.i(TAG, "nativeInitEngine returned: handle=$nativeEngineHandle")

            if (nativeEngineHandle == 0L) {
                            Log.e(TAG, "CRITICAL ERROR: nativeInitEngine returned 0 (native initialization failed)")
                        } else {
                            Log.i(TAG, "SUCCESS: Native engine initialized with valid handle")
                            // Set data path AFTER engine is created (g_renderer must exist in native)
                            if (dataPath.isNotEmpty()) {
                                nativeSetDataPath(dataPath)
                                Log.i(TAG, "BSA data path set on native: $dataPath")
                            }
                        }

            gameSurfaceView?.let {
                it.setRenderThreadInitialized(true)
                Log.i(TAG, "Signaled GameSurfaceView that render thread is initialized")
            }
        } catch (e: UnsatisfiedLinkError) {
            Log.e(TAG, "FATAL JNI ERROR: Native library issue - ${e.message}", e)
        } catch (e: Exception) {
            Log.e(TAG, "Exception in onSurfaceCreated: ${e.message}", e)
        }
    }

    override fun onSurfaceChanged(gl: GL10, width: Int, height: Int) {
        Log.i(TAG, "=== onSurfaceChanged: ${width}x${height} ===")

        try {
            gl.glViewport(0, 0, width, height)
            Log.d(TAG, "Viewport set")

            // Notify GameSurfaceView of GL viewport size for touch coordinate scaling
            gameSurfaceView?.setGLViewportSize(width, height)

            if (nativeEngineHandle != 0L) {
                Log.d(TAG, "Calling nativeSetViewport")
                nativeSetViewport(nativeEngineHandle, width, height)
                Log.d(TAG, "nativeSetViewport completed")

                // Send view (physical) size for touch coordinate conversion
                val viewW = gameSurfaceView?.width ?: width
                val viewH = gameSurfaceView?.height ?: height
                nativeSetViewSize(nativeEngineHandle, viewW.toFloat(), viewH.toFloat())
                Log.d(TAG, "View size set: ${viewW}x${viewH}")
            } else {
                Log.w(TAG, "onSurfaceChanged: nativeEngineHandle is 0")
            }

            // Initialize BinkVideoPlayer for intro video playback
            // Uses the GL surface for video frame rendering
            initBinkVideo()

            // Initialize title screen video background (Map loop)
            // Note: gameSurfaceView may be null when using XML GLSurfaceView,
            // so we use appContext set via setContext()
            val context = appContext
            if (context != null && !titleVideoInitAttempted) {
                titleVideoInitAttempted = true
                initTitleVideo(context)
            }

        } catch (e: Exception) {
            Log.e(TAG, "Exception in onSurfaceChanged: ${e.message}", e)
        }
    }

    private var binkVideoInitialized = false
    private var binkVideoInitFailed = false

    // Title screen video background
    private fun initTitleVideo(context: android.content.Context) {
        try {
            // Find map_loop.mp4
            val extDir = context.getExternalFilesDir(null)
            val videoFile = java.io.File(extDir, "videos/map_loop.mp4")
            if (!videoFile.exists()) {
                Log.w(TAG, "map_loop.mp4 not found at ${videoFile.absolutePath}")
                return
            }

            // Create OES texture
            val textures = IntArray(1)
            GLES20.glGenTextures(1, textures, 0)
            titleVideoTextureId = textures[0]
            GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, titleVideoTextureId)
            GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_LINEAR)
            GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR)
            GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE)
            GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE)
            GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0)

            // Create SurfaceTexture from OES texture
            titleVideoSurfaceTexture = SurfaceTexture(titleVideoTextureId)
            titleVideoSurfaceTexture!!.setOnFrameAvailableListener {
                titleVideoReady = true
            }

            // Create MediaPlayer
            titleVideoPlayer = MediaPlayer().apply {
                setSurface(Surface(titleVideoSurfaceTexture))
                setDataSource(videoFile.absolutePath)
                isLooping = true
                setVolume(0f, 0f) // Muted - game has its own music
                prepare()
                start()
            }

            Log.i(TAG, "Title video initialized: textureId=$titleVideoTextureId, file=${videoFile.absolutePath}")

            // Notify native of texture ID
            nativeSetTitleVideoTexture(titleVideoTextureId)

        } catch (e: Exception) {
            Log.e(TAG, "Failed to init title video: ${e.message}", e)
            titleVideoPlayer?.release()
            titleVideoPlayer = null
            titleVideoSurfaceTexture?.release()
            titleVideoSurfaceTexture = null
        }
    }

    private fun releaseTitleVideo() {
        try {
            titleVideoPlayer?.stop()
            titleVideoPlayer?.release()
            titleVideoPlayer = null
            titleVideoSurfaceTexture?.release()
            titleVideoSurfaceTexture = null
            if (titleVideoTextureId != 0) {
                val textures = intArrayOf(titleVideoTextureId)
                GLES20.glDeleteTextures(1, textures, 0)
                titleVideoTextureId = 0
            }
            titleVideoReady = false
        } catch (e: Exception) {
            Log.e(TAG, "Error releasing title video: ${e.message}")
        }
    }

    private fun initBinkVideo() {
        if (binkVideoInitialized || binkVideoInitFailed) return

        try {
            // In GLSurfaceView's renderer thread, we need to get the Surface differently.
            // The holder.surface might be null during GL rendering. Use the view's holder directly.
            val view = gameSurfaceView
            if (view == null) {
                // gameSurfaceView is null when using default constructor (GLSurfaceView from XML)
                // BinkVideoPlayer requires a Surface, so we cannot initialize it in this case.
                // The intro video was already played by IntroVideoActivity, so this is OK.
                Log.d(TAG, "initBinkVideo: gameSurfaceView is null (using XML GLSurfaceView), skipping BinkVideo init")
                binkVideoInitFailed = true
                return
            }

            // Try to get surface from the view's holder
            var surface: android.view.Surface? = null
            try {
                surface = view.holder.surface
            } catch (e: Exception) {
                Log.w(TAG, "initBinkVideo: exception getting surface: ${e.message}")
            }

            if (surface == null) {
                // On GLSurfaceView, the surface is managed by EGL. 
                // We need to get it from the EGL context instead.
                // For now, mark as failed since BinkVideoPlayer won't work without a valid surface.
                Log.w(TAG, "initBinkVideo: surface not available (GLSurfaceView EGL managed)")
                binkVideoInitFailed = true
                return
            }

            // Resolve video base path: external storage oblivion_assets/videos
            val videoBasePath = view.context?.let { ctx ->
                val extDir = ctx.getExternalFilesDir(null)
                if (extDir != null) {
                    val videoDir = java.io.File(extDir, "oblivion_assets/videos")
                    if (videoDir.exists()) {
                        videoDir.absolutePath
                    } else {
                        // Fallback: try filesDir/data
                        val dataVideoDir = java.io.File(ctx.filesDir, "data/videos")
                        if (dataVideoDir.exists()) dataVideoDir.absolutePath else ""
                    }
                } else ""
            } ?: ""

            Log.i(TAG, "Initializing BinkVideoPlayer with videoBasePath: $videoBasePath")
            val result = nativeInitBinkVideo(surface, videoBasePath)
            if (result) {
                Log.i(TAG, "BinkVideoPlayer initialized successfully")
                binkVideoInitialized = true
            } else {
                Log.w(TAG, "BinkVideoPlayer initialization failed (will use fallback intro)")
                binkVideoInitFailed = true
            }
        } catch (e: Exception) {
            Log.w(TAG, "initBinkVideo exception (will use fallback intro): ${e.message}")
            binkVideoInitFailed = true
        }
    }

    override fun onDrawFrame(gl: GL10) {
        try {
            if (nativeEngineHandle != 0L) {
                // Check for exit request before rendering
                if (nativeIsExitRequested()) {
                    Log.i(TAG, "Exit requested by native engine")
                    onExitRequested?.invoke()
                    return
                }

                // Update title video texture if new frame available
                if (titleVideoReady && titleVideoSurfaceTexture != null) {
                    try {
                        titleVideoSurfaceTexture!!.updateTexImage()
                        nativeUpdateTitleVideoTexture()
                        titleVideoReady = false
                    } catch (e: Exception) {
                        Log.w(TAG, "Error updating title video texture: ${e.message}")
                    }
                }

                nativeRenderFrame(nativeEngineHandle)
                frameCount++

                // Retry BinkVideoPlayer init if it failed in onSurfaceChanged (with rate limiting)
                            if (!binkVideoInitialized && !binkVideoInitFailed) {
                    initBinkVideo()
                }

                val currentTime = System.currentTimeMillis()
                if (currentTime - lastLogTime >= 1000) {
                    Log.d(TAG, "FPS: $frameCount")
                    frameCount = 0
                    lastLogTime = currentTime
                }
            } else {
                if (frameCount == 0) {
                    Log.e(TAG, "CRITICAL: onDrawFrame called but nativeEngineHandle is 0 - native initialization never completed")
                }
                frameCount++
                if (frameCount > 10) {
                    frameCount = 0
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Exception in onDrawFrame: ${e.message}", e)
        }
    }

    fun onTouchEvent(pointerId: Int, x: Float, y: Float, action: Int) {
        if (nativeEngineHandle != 0L) {
            nativeOnTouchEvent(nativeEngineHandle, pointerId, x, y, action)
        }
    }

    private external fun nativeInitEngine(): Long
    private external fun nativeSetViewport(handle: Long, width: Int, height: Int)
    private external fun nativeSetViewSize(handle: Long, width: Float, height: Float)
    private external fun nativeRenderFrame(handle: Long)
    private external fun nativeOnTouchEvent(handle: Long, pointerId: Int, x: Float, y: Float, action: Int)

    // Phase 30 Step 13: Integration test
    external fun nativeRunPhase30Test(assetPath: String): String

    // Debug System Toggles
    external fun nativeToggleDebugConsole()
    external fun nativeToggleNpcDebug()
    external fun nativeToggleWorldDebug()
    external fun nativeTogglePerfGraph()
    external fun nativeToggleAllDebug()
    external fun nativeToggleDebugMenu()
    external fun nativeIsDebugMenuVisible(): Boolean
    external fun nativeDebugMenuSelectTab(tabIndex: Int)
    external fun nativeToggle3DViewer()
    external fun nativeStartGame()
        external fun nativeOnBackKey(): Boolean
        external fun nativeExecuteConsoleCommand(command: String)

    // Exit request check
    external fun nativeIsExitRequested(): Boolean
}
