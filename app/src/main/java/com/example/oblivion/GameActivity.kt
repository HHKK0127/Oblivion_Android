package com.example.oblivion

import android.app.Activity
import android.os.Bundle
import android.util.Log
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import kotlinx.coroutines.*

class GameActivity : Activity(), SurfaceHolder.Callback {

    private lateinit var surfaceView: SurfaceView
    private val engine = OblivionEngine()
    private val scope = CoroutineScope(Dispatchers.Main + Job())
    private var isInitializing = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        surfaceView = SurfaceView(this).apply {
            holder.addCallback(this@GameActivity)
        }
        setContentView(surfaceView)
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        if (isInitializing) return
        isInitializing = true

        // Initialize on background thread (prevent ANR)
        scope.launch(Dispatchers.IO) {
            try {
                Log.i("GameActivity", "Starting engine initialization on background thread")
                engine.initialize(holder.surface, enableValidation = true)

                withContext(Dispatchers.Main) {
                    Log.i("GameActivity", "Engine initialization completed, calling onSurfaceCreated")
                    engine.onSurfaceCreated()
                }
            } catch (e: Exception) {
                Log.e("GameActivity", "Engine initialization failed", e)
                withContext(Dispatchers.Main) {
                    isInitializing = false
                }
            }
        }
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
        scope.launch(Dispatchers.IO) {
            engine.onSurfaceChanged(width, height)
        }
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        scope.launch(Dispatchers.IO) {
            engine.onSurfaceDestroyed()
        }
    }

    override fun onTouchEvent(event: MotionEvent): Boolean {
        val action = event.actionMasked
        val pointerIndex = event.actionIndex
        val pointerId = event.getPointerId(pointerIndex)

        engine.onTouchEvent(
            pointerId,
            event.getX(pointerIndex),
            event.getY(pointerIndex),
            action
        )
        return true
    }

    override fun onPause() {
        super.onPause()
        scope.launch(Dispatchers.IO) {
            engine.pause()
        }
    }

    override fun onResume() {
        super.onResume()
        scope.launch(Dispatchers.IO) {
            engine.resume()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        scope.launch(Dispatchers.IO) {
            engine.destroy()
        }
        scope.cancel()
        super.onDestroy()
    }

    // =========================================================================
    // Gamepad / Controller input handling
    // =========================================================================

    override fun dispatchGenericMotionEvent(event: MotionEvent): Boolean {
        // Check if this is a gamepad/joystick motion event
        if (event.source and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK) {
            val deviceId = event.deviceId
            handleGamepadMotionEvent(deviceId, event)
            return true
        }
        return super.dispatchGenericMotionEvent(event)
    }

    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        // Check if this is a gamepad key event
        val source = event.device?.sources ?: 0
        if (source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD ||
            source and InputDevice.SOURCE_DPAD == InputDevice.SOURCE_DPAD) {
            val deviceId = event.deviceId
            val pressed = event.action == KeyEvent.ACTION_DOWN
            engine.onGamepadKeyEvent(deviceId, event.keyCode, pressed)
            return true
        }
        return super.dispatchKeyEvent(event)
    }

    private fun handleGamepadMotionEvent(deviceId: Int, event: MotionEvent) {
        // Left stick
        val leftX = event.getAxisValue(MotionEvent.AXIS_X)
        val leftY = event.getAxisValue(MotionEvent.AXIS_Y)
        if (Math.abs(leftX) > 0.01f || Math.abs(leftY) > 0.01f) {
            engine.onGamepadAxisEvent(deviceId, 0, leftX)  // LEFT_X
            engine.onGamepadAxisEvent(deviceId, 1, leftY)  // LEFT_Y
        }

        // Right stick
        val rightX = event.getAxisValue(MotionEvent.AXIS_Z)
        val rightY = event.getAxisValue(MotionEvent.AXIS_RZ)
        if (Math.abs(rightX) > 0.01f || Math.abs(rightY) > 0.01f) {
            engine.onGamepadAxisEvent(deviceId, 11, rightX) // RIGHT_X
            engine.onGamepadAxisEvent(deviceId, 14, rightY) // RIGHT_Y
        }

        // Triggers
        val leftTrigger = event.getAxisValue(MotionEvent.AXIS_LTRIGGER)
        val rightTrigger = event.getAxisValue(MotionEvent.AXIS_RTRIGGER)
        if (leftTrigger > 0.01f) {
            engine.onGamepadAxisEvent(deviceId, 17, leftTrigger) // TRIGGER_LEFT
        }
        if (rightTrigger > 0.01f) {
            engine.onGamepadAxisEvent(deviceId, 18, rightTrigger) // TRIGGER_RIGHT
        }

        // D-pad (HAT)
        val hatX = event.getAxisValue(MotionEvent.AXIS_HAT_X)
        val hatY = event.getAxisValue(MotionEvent.AXIS_HAT_Y)
        if (Math.abs(hatX) > 0.01f || Math.abs(hatY) > 0.01f) {
            engine.onGamepadAxisEvent(deviceId, 15, hatX) // HAT_X
            engine.onGamepadAxisEvent(deviceId, 16, hatY) // HAT_Y
        }
    }
}
