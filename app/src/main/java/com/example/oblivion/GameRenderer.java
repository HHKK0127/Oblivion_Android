package com.example.oblivion;

import android.opengl.GLSurfaceView;
import android.util.Log;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class GameRenderer implements GLSurfaceView.Renderer {

    private static final String TAG = "GameRenderer";

    private long nativeEngineHandle = 0;
    private GameSurfaceView gameSurfaceView;

    public GameRenderer() {
        // Default constructor
    }

    public GameRenderer(GameSurfaceView surfaceView) {
        this.gameSurfaceView = surfaceView;
        Log.i(TAG, "GameRenderer created with GameSurfaceView reference");
    }

    static {
        try {
            System.loadLibrary("native-lib");
            Log.i("GameRenderer", "Native library loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e("GameRenderer", "FATAL: Failed to load native library - " + e.getMessage(), e);
            throw e;
        }
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        Log.i(TAG, "=== onSurfaceCreated called ===");

        // Display OpenGL info immediately
        try {
            String vendor = gl.glGetString(GL10.GL_VENDOR);
            String renderer_name = gl.glGetString(GL10.GL_RENDERER);
            String version = gl.glGetString(GL10.GL_VERSION);

            Log.i(TAG, "OpenGL Info - Vendor: " + vendor);
            Log.i(TAG, "OpenGL Info - Renderer: " + renderer_name);
            Log.i(TAG, "OpenGL Info - Version: " + version);
        } catch (Exception e) {
            Log.e(TAG, "Error getting GL info: " + e.getMessage(), e);
        }

        try {
            // ネイティブエンジン初期化
            Log.i(TAG, "Calling nativeInitEngine()");
            nativeEngineHandle = nativeInitEngine();
            Log.i(TAG, "nativeInitEngine returned: handle=" + nativeEngineHandle);

            if (nativeEngineHandle == 0) {
                Log.e(TAG, "CRITICAL ERROR: nativeInitEngine returned 0 (native initialization failed)");
            } else {
                Log.i(TAG, "SUCCESS: Native engine initialized with valid handle");
            }

            // Signal that the render thread has successfully initialized
            if (gameSurfaceView != null) {
                gameSurfaceView.setRenderThreadInitialized(true);
                Log.i(TAG, "Signaled GameSurfaceView that render thread is initialized");
            }
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "FATAL JNI ERROR: Native library issue - " + e.getMessage(), e);
        } catch (Exception e) {
            Log.e(TAG, "Exception in onSurfaceCreated: " + e.getMessage(), e);
        }
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Log.i(TAG, "=== onSurfaceChanged: " + width + "x" + height + " ===");

        try {
            // ビューポート設定
            gl.glViewport(0, 0, width, height);
            Log.d(TAG, "Viewport set");

            // ネイティブレンダーにサイズ通知
            if (nativeEngineHandle != 0) {
                Log.d(TAG, "Calling nativeSetViewport");
                nativeSetViewport(nativeEngineHandle, width, height);
                Log.d(TAG, "nativeSetViewport completed");
            } else {
                Log.w(TAG, "onSurfaceChanged: nativeEngineHandle is 0");
            }
        } catch (Exception e) {
            Log.e(TAG, "Exception in onSurfaceChanged: " + e.getMessage(), e);
        }
    }

    private int frameCount = 0;
    private long lastLogTime = 0;

    @Override
    public void onDrawFrame(GL10 gl) {
        try {
            if (nativeEngineHandle != 0) {
                nativeRenderFrame(nativeEngineHandle);
                frameCount++;

                // Log FPS every second
                long currentTime = System.currentTimeMillis();
                if (currentTime - lastLogTime >= 1000) {
                    Log.d(TAG, "FPS: " + frameCount);
                    frameCount = 0;
                    lastLogTime = currentTime;
                }
            } else {
                if (frameCount == 0) {
                    Log.e(TAG, "CRITICAL: onDrawFrame called but nativeEngineHandle is 0 - native initialization never completed");
                }
                frameCount++;
                if (frameCount > 10) {
                    frameCount = 0; // Reset to avoid spam
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "Exception in onDrawFrame: " + e.getMessage(), e);
        }
    }

    public void onTouchEvent(float dx, float dy) {
        if (nativeEngineHandle != 0) {
            nativeOnTouchEvent(nativeEngineHandle, dx, dy);
        }
    }

    // JNI ネイティブメソッド
    private native long nativeInitEngine();
    private native void nativeSetViewport(long handle, int width, int height);
    private native void nativeRenderFrame(long handle);
    private native void nativeOnTouchEvent(long handle, float dx, float dy);
}
