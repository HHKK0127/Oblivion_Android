package com.example.oblivion

import android.content.Context
import android.opengl.GLSurfaceView
import android.util.AttributeSet
import android.util.Log
import android.view.MotionEvent

class GameSurfaceView : GLSurfaceView {

    private var renderer: GameRenderer? = null
    private var renderThreadInitialized = false

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
            setRenderer(newRenderer)
            renderMode = RENDERMODE_CONTINUOUSLY
        } catch (e: Exception) {
            Log.e(TAG, "Exception in init: ${e.message}", e)
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        if (renderer == null) return false

        val action = event.actionMasked
        val actionIndex = event.actionIndex
        
        when (action) {
            MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                val pointerId = event.getPointerId(actionIndex)
                val x = event.getX(actionIndex)
                val y = event.getY(actionIndex)
                renderer?.onTouchEvent(pointerId, x, y, 0) // 0 = DOWN
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP -> {
                val pointerId = event.getPointerId(actionIndex)
                val x = event.getX(actionIndex)
                val y = event.getY(actionIndex)
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
        }
        return true
    }

    override fun onResume() {
        renderThreadInitialized = false
        super.onResume()
    }

    override fun onPause() {
        super.onPause()
    }

    fun setRenderThreadInitialized(initialized: Boolean) {
        renderThreadInitialized = initialized
    }

    fun getGameRenderer(): GameRenderer? {
        return renderer
    }
}
