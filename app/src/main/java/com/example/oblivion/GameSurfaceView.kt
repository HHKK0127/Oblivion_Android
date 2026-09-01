package com.example.oblivion

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.util.Log
import android.view.MotionEvent

class GameSurfaceView : GLSurfaceView {

    private var renderer: GameRenderer? = null
    private var renderThreadInitialized = false
    private var rendererSet = false

    // GL viewport dimensions (set by renderer after surface creation)
    private var glViewportWidth = 0
    private var glViewportHeight = 0

    // Display density for touch coordinate scaling
    // MotionEvent.getX()/getY() returns coordinates in display logical pixels,
    // but the UI (GL viewport) uses physical pixels. We need to scale.
    private var displayDensity = 1.0f

    companion object {
        private const val TAG = "GameSurfaceView"
    }

    constructor(context: Context) : super(context) {
        init()
    }

    constructor(context: Context, attrs: AttributeSet) : super(context, attrs) {
        init()
    }

    private fun init() {
        try {
            setEGLContextClientVersion(3)
            val newRenderer = GameRenderer(this)
            renderer = newRenderer
            // Defer setRenderer to onResume to ensure GL thread starts after activity is visible
            Log.i(TAG, "GameSurfaceView initialized (renderer deferred)")

            // Get display density for touch coordinate scaling
            displayDensity = context.resources.displayMetrics.density
            Log.i(TAG, "Display density: $displayDensity")
        } catch (e: Exception) {
            Log.e(TAG, "Exception in init: ${e.message}", e)
        }
    }

    /**
     * Call this after setContentView and onResume to start the GL render thread.
     */
    fun startRenderer() {
        if (!rendererSet && renderer != null) {
            Log.i(TAG, "Starting GL renderer thread")
            setRenderer(renderer)
            renderMode = RENDERMODE_CONTINUOUSLY
            rendererSet = true
        }
    }

    override fun dispatchTouchEvent(event: MotionEvent): Boolean {
        Log.d(TAG, "dispatchTouchEvent: action=${event.actionMasked}")
        return super.dispatchTouchEvent(event)
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (renderer == null) {
            Log.w(TAG, "onTouchEvent: renderer is null!")
            return false
        }

        // MotionEvent.getX()/getY() returns coordinates in the view's physical pixel space.
        // For a match_parent GLSurfaceView, this already matches the GL viewport (physical pixels).
        // No density scaling needed - coordinates are already in the correct space.
        val action = event.actionMasked
        val actionIndex = event.actionIndex
        
        when (action) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                val pointerId = event.getPointerId(actionIndex)
                val x = event.getX(actionIndex)
                val y = event.getY(actionIndex)
                Log.d(TAG, "Touch DOWN: pointerId=$pointerId, pos=($x, $y)")
                renderer?.onTouchEvent(pointerId, x, y, 0) // 0 = DOWN
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> {
                val pointerId = event.getPointerId(actionIndex)
                val x = event.getX(actionIndex)
                val y = event.getY(actionIndex)
                Log.d(TAG, "Touch UP: pointerId=$pointerId, pos=($x, $y)")
                renderer?.onTouchEvent(pointerId, x, y, 1) // 1 = UP
            }
            MotionEvent.ACTION_MOVE -> {
                for (i in 0 until event.pointerCount) {
                    val pointerId = event.getPointerId(i)
                    val x = event.getX(i)
                    val y = event.getY(i)
                    renderer?.onTouchEvent(pointerId, x, y, 2) // 2 = MOVE
                }
            }
            MotionEvent.ACTION_CANCEL -> {
                // Release all active pointers on cancel
                for (i in 0 until event.pointerCount) {
                    val pointerId = event.getPointerId(i)
                    val x = event.getX(i)
                    val y = event.getY(i)
                    renderer?.onTouchEvent(pointerId, x, y, 1) // 1 = UP
                }
                Log.d(TAG, "Touch CANCEL: released all pointers")
            }
        }
        return true
    }

    override fun onResume() {
        // If renderer not yet set, start it now (before super.onResume)
        if (!rendererSet && renderer != null) {
            startRenderer()
        }
        renderThreadInitialized = false
        super.onResume()
    }

    override fun onPause() {
        super.onPause()
    }

    fun setRenderThreadInitialized(initialized: Boolean) {
        renderThreadInitialized = initialized
    }

    /**
     * Called by the renderer after surface creation to inform the view
     * of the actual GL viewport dimensions. Used for touch coordinate scaling.
     */
    fun setGLViewportSize(width: Int, height: Int) {
        glViewportWidth = width
        glViewportHeight = height
        Log.i(TAG, "GL viewport size set: ${width}x${height}, view size: ${this.width}x${this.height}")
    }

    fun getGameRenderer(): GameRenderer? {
        return renderer
    }
}
