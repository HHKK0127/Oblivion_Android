package com.example.oblivion;

import android.opengl.GLSurfaceView;
import android.util.Log;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class GameRenderer implements GLSurfaceView.Renderer {

    private static final String TAG = "GameRenderer";

    private long nativeEngineHandle = 0;

    static {
        System.loadLibrary("native-lib");
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        Log.d(TAG, "onSurfaceCreated called");

        // ネイティブエンジン初期化
        nativeEngineHandle = nativeInitEngine();
        Log.d(TAG, "Native engine initialized: " + nativeEngineHandle);

        // バージョン情報ログ出力
        String vendor = gl.glGetString(GL10.GL_VENDOR);
        String renderer = gl.glGetString(GL10.GL_RENDERER);
        String version = gl.glGetString(GL10.GL_VERSION);
        String extensions = gl.glGetString(GL10.GL_EXTENSIONS);

        Log.d(TAG, "GL Vendor: " + vendor);
        Log.d(TAG, "GL Renderer: " + renderer);
        Log.d(TAG, "GL Version: " + version);
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Log.d(TAG, "onSurfaceChanged: " + width + "x" + height);

        // ビューポート設定
        gl.glViewport(0, 0, width, height);

        // ネイティブレンダーにサイズ通知
        nativeSetViewport(nativeEngineHandle, width, height);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        // フレームレンダリング
        nativeRenderFrame(nativeEngineHandle);
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
