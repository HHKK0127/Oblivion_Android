package com.example.oblivion;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.MotionEvent;

public class GameSurfaceView extends GLSurfaceView {

    private GameRenderer renderer;
    private float lastX = 0;
    private float lastY = 0;

    public GameSurfaceView(Context context) {
        super(context);
        init();
    }

    public GameSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        // OpenGL ES 3.0 を使用
        setEGLContextClientVersion(3);

        // GameRenderer を設定
        renderer = new GameRenderer();
        setRenderer(renderer);
        setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
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

    public GameRenderer getGameRenderer() {
        return renderer;
    }
}
