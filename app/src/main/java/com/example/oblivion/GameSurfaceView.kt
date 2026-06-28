package com.example.oblivion

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.util.Log
import android.view.MotionEvent

class GameSurfaceView : GLSurfaceView {

    private var renderer: GameRenderer? = null
    private var lastX = 0f
    private var lastY = 0f
    private var renderThreadInitialized = false

    companion object {
        private const val TAG = "GameSurfaceView"
    }

    constructor(context: Context) : super(context) {
        Log.i(TAG, "Constructor(context) called")
        init()
    }

    constructor(context: Context, attrs: AttributeSet) : super(context, attrs) {
        Log.i(TAG, "Constructor(context, attrs) called")
        init()
    }

    private fun init() {
        Log.i(TAG, "=== init() called ===")
        try {
            Log.i(TAG, "Setting EGL context client version to 3")
            setEGLContextClientVersion(3)

            Log.i(TAG, "Creating GameRenderer")
            val newRenderer = GameRenderer(this)
            renderer = newRenderer
            Log.i(TAG, "Setting renderer")
            setRenderer(newRenderer)
            Log.i(TAG, "Renderer set successfully")

            Log.i(TAG, "Setting render mode to RENDERMODE_CONTINUOUSLY")
            renderMode = RENDERMODE_CONTINUOUSLY

            Log.i(TAG, "=== init() completed successfully ===")
        } catch (e: Exception) {
            Log.e(TAG, "Exception in init: ${e.message}", e)
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        val x = event.x
        val y = event.y

        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                lastX = x
                lastY = y
                renderer?.onTouchEvent(x, y)
            }
            MotionEvent.ACTION_UP -> {
                renderer?.onTouchEvent(x, y)
            }
            MotionEvent.ACTION_MOVE -> {
                val dx = x - lastX
                val dy = y - lastY
                renderer?.onTouchEvent(dx, dy)
            }
        }

        lastX = x
        lastY = y
        return true
    }

    override fun onResume() {
        Log.i(TAG, "onResume called")
        try {
            Log.i(TAG, "Resuming renderer")
            renderThreadInitialized = false
            super.onResume()
            Log.i(TAG, "onResume completed, render thread queued")
        } catch (e: Exception) {
            Log.e(TAG, "Exception in onResume: ${e.message}", e)
        }
    }

    override fun onPause() {
        Log.i(TAG, "onPause called")
        try {
            Log.i(TAG, "Pausing renderer")
            super.onPause()
            Log.i(TAG, "onPause completed")
        } catch (e: Exception) {
            Log.e(TAG, "Exception in onPause: ${e.message}", e)
        }
    }

    fun setRenderThreadInitialized(initialized: Boolean) {
        renderThreadInitialized = initialized
        Log.i(TAG, "Render thread initialized flag: $initialized")
    }

    fun getGameRenderer(): GameRenderer? {
        return renderer
    }
}
