package com.example.oblivion

import android.util.Log
import android.view.Surface

class OblivionEngine {
    companion object {
        const val TAG = "OblivionEngine"

        const val SUCCESS = 0
        const val FAILURE = -1
        const val PENDING = 1

        init {
            System.loadLibrary("oblivion_engine")
        }
    }

    private var nativeHandle: Long = 0
    private var isInitialized = false
    private val initLock = Object()

    fun initialize(surface: Surface, enableValidation: Boolean): Boolean {
        synchronized(initLock) {
            if (isInitialized) {
                throw IllegalStateException("Engine already initialized")
            }

            return try {
                Log.i(TAG, "Starting native initialization...")
                nativeHandle = nativeInitialize(surface, enableValidation)
                isInitialized = nativeHandle != 0L

                if (isInitialized) {
                    Log.i(TAG, "Engine initialized successfully (handle: $nativeHandle)")
                } else {
                    Log.e(TAG, "Engine initialization failed (nativeInitialize returned 0)")
                }
                isInitialized
            } catch (e: Exception) {
                Log.e(TAG, "Exception during initialization: ${e.message}", e)
                isInitialized = false
                false
            }
        }
    }

    fun onSurfaceCreated() {
        if (isInitialized) {
            nativeOnSurfaceCreated(nativeHandle)
        }
    }

    fun onSurfaceChanged(width: Int, height: Int) {
        if (isInitialized) {
            nativeOnSurfaceChanged(nativeHandle, width, height)
        }
    }

    fun onSurfaceDestroyed() {
        if (isInitialized) {
            nativeOnSurfaceDestroyed(nativeHandle)
        }
    }

    fun pause() {
        if (isInitialized) {
            nativePause(nativeHandle)
        }
    }

    fun resume() {
        if (isInitialized) {
            nativeResume(nativeHandle)
        }
    }

    fun destroy() {
        synchronized(initLock) {
            if (isInitialized && nativeHandle != 0L) {
                Log.i(TAG, "Destroying engine (handle: $nativeHandle)")
                nativeDestroy(nativeHandle)
                nativeHandle = 0
                isInitialized = false
            }
        }
    }

    fun onTouchEvent(pointerId: Int, x: Float, y: Float, action: Int) {
        if (isInitialized) {
            nativeOnTouchEvent(nativeHandle, pointerId, x, y, action)
        }
    }

    private external fun nativeInitialize(surface: Surface, enableValidation: Boolean): Long
    private external fun nativeOnSurfaceCreated(handle: Long)
    private external fun nativeOnSurfaceChanged(handle: Long, width: Int, height: Int)
    private external fun nativeOnSurfaceDestroyed(handle: Long)
    private external fun nativePause(handle: Long)
    private external fun nativeResume(handle: Long)
    private external fun nativeDestroy(handle: Long)
    private external fun nativeOnTouchEvent(handle: Long, pointerId: Int, x: Float, y: Float, action: Int)
}
