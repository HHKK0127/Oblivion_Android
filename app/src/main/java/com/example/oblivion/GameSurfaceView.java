package com.example.oblivion;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.MotionEvent;

public class GameSurfaceView extends GLSurfaceView {

    private static final String TAG = "GameSurfaceView";
    private GameRenderer renderer;
    private float lastX = 0;
    private float lastY = 0;
    private boolean renderThreadInitialized = false;

    public GameSurfaceView(Context context) {
        super(context);
        Log.i(TAG, "Constructor(context) called");
        init();
    }

    public GameSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        Log.i(TAG, "Constructor(context, attrs) called");
        init();
    }

    private void init() {
        Log.i(TAG, "=== init() called ===");
        try {
            // OpenGL ES 3.0 を使用
            Log.i(TAG, "Setting EGL context client version to 3");
            setEGLContextClientVersion(3);

            // GameRenderer を設定
            Log.i(TAG, "Creating GameRenderer");
            renderer = new GameRenderer(this);
            Log.i(TAG, "Setting renderer");
            setRenderer(renderer);
            Log.i(TAG, "Renderer set successfully");

            // フレームレート制御: 連続レンダリングを使用し、native側で frame timing を実装
            // Continuous rendering allows native code to control frame rate precisely
            Log.i(TAG, "Setting render mode to RENDERMODE_CONTINUOUSLY");
            setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

            // OPTIMIZATION: Disabled setPreserveEGLContextOnPause for Amazon Fire compatibility
            // This was causing render thread initialization failures on older devices
            // Log.i(TAG, "Setting preserve EGL context on pause");
            // setPreserveEGLContextOnPause(true);

            Log.i(TAG, "=== init() completed successfully ===");
        } catch (Exception e) {
            Log.e(TAG, "Exception in init: " + e.getMessage(), e);
            Log.e(TAG, "Stack trace: ", e);
            // Continue anyway - don't fail initialization
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        float x = event.getX();
        float y = event.getY();

        switch (event.getAction()) {
            case MotionEvent.ACTION_MOVE:
                float dx = x - lastX;
                float dy = y - lastY;

                // カメラ制御にタッチイベントを伝達
                if (renderer != null) {
                    renderer.onTouchEvent(dx, dy);
                }
                break;
        }

        lastX = x;
        lastY = y;
        return true;
    }

    @Override
    public void onResume() {
        Log.i(TAG, "onResume called");
        try {
            Log.i(TAG, "Resuming renderer");
            renderThreadInitialized = false;
            super.onResume();
            Log.i(TAG, "onResume completed, render thread queued");
        } catch (Exception e) {
            Log.e(TAG, "Exception in onResume: " + e.getMessage(), e);
        }
    }

    @Override
    public void onPause() {
        Log.i(TAG, "onPause called");
        try {
            Log.i(TAG, "Pausing renderer");
            super.onPause();
            Log.i(TAG, "onPause completed");
        } catch (Exception e) {
            Log.e(TAG, "Exception in onPause: " + e.getMessage(), e);
        }
    }

    public void setRenderThreadInitialized(boolean initialized) {
        renderThreadInitialized = initialized;
        Log.i(TAG, "Render thread initialized flag: " + initialized);
    }

    public GameRenderer getGameRenderer() {
        return renderer;
    }
}
