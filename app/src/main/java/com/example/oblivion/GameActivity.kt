package com.example.oblivion

import android.app.Activity
import android.os.Bundle
import android.util.Log
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

        // バックグラウンドスレッドで初期化（ANR 防止）
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
}
