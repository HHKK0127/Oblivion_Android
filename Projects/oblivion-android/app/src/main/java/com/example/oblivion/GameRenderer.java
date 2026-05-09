package com.example.oblivion;

import android.opengl.GLSurfaceView;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class GameRenderer implements GLSurfaceView.Renderer {

    static {
        System.loadLibrary("native-lib");
    }

    // Native methods (linked to jni_bridge.cpp)
    public static native void nativeInit(int width, int height);
    public static native void nativeRender(float deltaTime);
    public static native void nativeCleanup();
    public static native void nativeSetLanguage(int language);
    public static native int nativeGetLanguage();
    public static native String nativeGetString(String key);
    public static native void nativeOnTouchEvent(float x, float y);
    public static native void nativeOnKeyPress(int key);
    public static native void nativeSetAssetManager(android.content.res.AssetManager assetManager);
    public static native void nativeSetTargetFPS(int fps);
    public static native int nativeGetTargetFPS();
    public static native void nativePlayBGM(String path);
    public static native void nativeStopBGM();

    private long lastTime;

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        lastTime = System.nanoTime();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        nativeInit(width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        long currentTime = System.nanoTime();
        float deltaTime = (currentTime - lastTime) / 1_000_000.0f; // milliseconds
        lastTime = currentTime;

        nativeRender(deltaTime);
    }
}
