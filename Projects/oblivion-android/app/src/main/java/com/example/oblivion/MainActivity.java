package com.example.oblivion;

import android.app.Activity;
import android.media.MediaPlayer;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import java.io.IOException;

public class MainActivity extends Activity {
    private GLSurfaceView glSurfaceView;
    private static MediaPlayer mediaPlayer;
    private static MainActivity instance;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        instance = this;

        // Fullscreen
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        );

        // Pass AssetManager to native code before GL init
        GameRenderer.nativeSetAssetManager(getAssets());

        // Create GLSurfaceView with OpenGL ES 3.0
        glSurfaceView = new GLSurfaceView(this);
        glSurfaceView.setEGLContextClientVersion(3);
        glSurfaceView.setRenderer(new GameRenderer());
        glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

        // Touch handling
        glSurfaceView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                if (event.getAction() == MotionEvent.ACTION_DOWN ||
                    event.getAction() == MotionEvent.ACTION_MOVE) {
                    float x = event.getX();
                    float y = event.getY();
                    GameRenderer.nativeOnTouchEvent(x, y);
                }
                return true;
            }
        });

        setContentView(glSurfaceView);
    }

    @Override
    protected void onPause() {
        super.onPause();
        glSurfaceView.onPause();
        if (mediaPlayer != null && mediaPlayer.isPlaying()) {
            mediaPlayer.pause();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        glSurfaceView.onResume();
        if (mediaPlayer != null) {
            mediaPlayer.start();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        GameRenderer.nativeCleanup();
        if (mediaPlayer != null) {
            mediaPlayer.stop();
            mediaPlayer.release();
            mediaPlayer = null;
        }
    }

    // Static methods called from JNI for BGM playback
    public static void playBGM(String path) {
        try {
            if (instance == null) {
                android.util.Log.w("MainActivity", "playBGM called but no instance available");
                return;
            }
            if (mediaPlayer != null) {
                mediaPlayer.stop();
                mediaPlayer.release();
            }
            mediaPlayer = new MediaPlayer();
            android.content.res.AssetFileDescriptor afd = instance.getAssets().openFd(path);
            mediaPlayer.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
            afd.close();
            mediaPlayer.setLooping(true);
            mediaPlayer.prepare();
            mediaPlayer.start();
            android.util.Log.i("MainActivity", "playBGM started: " + path);
        } catch (Exception e) {
            android.util.Log.e("MainActivity", "playBGM error: " + e.getMessage());
        }
    }

    public static void stopBGM() {
        if (mediaPlayer != null) {
            mediaPlayer.stop();
            mediaPlayer.release();
            mediaPlayer = null;
        }
        android.util.Log.i("MainActivity", "stopBGM called");
    }
}
