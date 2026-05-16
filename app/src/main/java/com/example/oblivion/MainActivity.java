package com.example.oblivion;

import android.app.Activity;
import android.content.Context;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.SoundPool;
import android.os.Bundle;
import android.util.Log;
import java.io.IOException;

public class MainActivity extends Activity {

    private static final String TAG = "MainActivity";
    private GameSurfaceView gameSurfaceView;

    // Audio components
    private MediaPlayer mediaPlayer;
    private SoundPool soundPool;
    private static MainActivity instance;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.i(TAG, "=== onCreate called ===");

        // Store singleton instance for JNI callback
        instance = this;

        try {
            // Initialize audio system
            Log.i(TAG, "Initializing audio system");
            initializeAudio();

            // GameSurfaceView をメインビューとして設定
            Log.i(TAG, "Creating GameSurfaceView");
            gameSurfaceView = new GameSurfaceView(this);
            Log.i(TAG, "Setting GameSurfaceView as content view");
            setContentView(gameSurfaceView);
            Log.i(TAG, "ContentView set successfully");
        } catch (Exception e) {
            Log.e(TAG, "Exception in onCreate: " + e.getMessage(), e);
        }
    }

    /**
     * Initialize audio components (MediaPlayer + SoundPool)
     */
    private void initializeAudio() {
        try {
            // MediaPlayer for BGM (background music)
            mediaPlayer = new MediaPlayer();
            setVolumeControlStream(AudioManager.STREAM_MUSIC);
            Log.i(TAG, "MediaPlayer initialized for BGM");

            // SoundPool for SE (sound effects) - max 5 simultaneous sounds
            soundPool = new SoundPool(5, AudioManager.STREAM_MUSIC, 0);
            Log.i(TAG, "SoundPool initialized for SE (max 5 sounds)");

            // Initialize audio bridge with AssetManager for WAV loading (Phase 8+)
            try {
                android.content.res.AssetManager assetManager = getAssets();
                GameRenderer.nativeInitAudioBridge(assetManager, this);
                Log.i(TAG, "Audio bridge initialized with AssetManager and MainActivity");
            } catch (Exception e) {
                Log.w(TAG, "Failed to initialize audio bridge: " + e.getMessage());
            }
        } catch (Exception e) {
            Log.e(TAG, "Failed to initialize audio", e);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.i(TAG, "onPause");

        // Only pause the surface view if the render thread has initialized
        // This prevents killing the render thread before it can start on Amazon Fire devices
        if (gameSurfaceView != null) {
            GameRenderer renderer = gameSurfaceView.getGameRenderer();
            if (renderer != null) {
                Log.i(TAG, "Pausing game surface view");
                gameSurfaceView.onPause();
            } else {
                Log.i(TAG, "GameRenderer not yet created, deferring pause");
            }
        }

        if (mediaPlayer != null && mediaPlayer.isPlaying()) {
            mediaPlayer.pause();
            Log.i(TAG, "BGM paused");
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.i(TAG, "onResume");
        gameSurfaceView.onResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "onDestroy - cleaning up audio");
        cleanupAudio();
    }

    /**
     * Play BGM (background music)
     * Called from native code (C++)
     */
    public static void playBGM(String filename) {
        if (instance == null) {
            Log.w(TAG, "MainActivity instance not available");
            return;
        }

        instance.playBGMInternal(filename);
    }

    private void playBGMInternal(String filename) {
        try {
            if (mediaPlayer == null) {
                Log.e(TAG, "MediaPlayer not initialized");
                return;
            }

            // Stop current BGM if playing
            if (mediaPlayer.isPlaying()) {
                mediaPlayer.stop();
            }
            mediaPlayer.reset();

            // Load MP3 from assets
            String assetPath = "file:///android_asset/audio/music/" + filename;
            Log.i(TAG, "Loading BGM: " + assetPath);

            mediaPlayer.setDataSource(assetPath);
            mediaPlayer.prepare();
            mediaPlayer.setLooping(true);
            mediaPlayer.start();

            Log.i(TAG, "BGM playing: " + filename);
        } catch (IOException e) {
            Log.e(TAG, "Failed to play BGM: " + filename, e);
        }
    }

    /**
     * Stop BGM
     * Called from native code (C++)
     */
    public static void stopBGM() {
        if (instance == null) return;

        if (instance.mediaPlayer != null && instance.mediaPlayer.isPlaying()) {
            instance.mediaPlayer.stop();
            Log.i(TAG, "BGM stopped");
        }
    }

    /**
     * Play SE (sound effect)
     * Called from native code (C++)
     */
    public static void playSE(String filename) {
        if (instance == null) {
            Log.w(TAG, "MainActivity instance not available");
            return;
        }

        instance.playSEInternal(filename);
    }

    private void playSEInternal(String filename) {
        try {
            if (soundPool == null) {
                Log.e(TAG, "SoundPool not initialized");
                return;
            }

            // Load SE from assets
            String assetPath = "audio/sounds/" + filename;
            Log.i(TAG, "Loading SE: " + assetPath);

            // For simplicity, load from raw resources
            // In production, you would load directly from assets
            int soundId = soundPool.load(assetPath, 1);
            soundPool.play(soundId, 1.0f, 1.0f, 0, 0, 1.0f);

            Log.i(TAG, "SE playing: " + filename);
        } catch (Exception e) {
            Log.e(TAG, "Failed to play SE: " + filename, e);
        }
    }

    /**
     * Cleanup audio resources
     */
    private void cleanupAudio() {
        try {
            if (mediaPlayer != null) {
                if (mediaPlayer.isPlaying()) {
                    mediaPlayer.stop();
                }
                mediaPlayer.release();
                mediaPlayer = null;
                Log.i(TAG, "MediaPlayer released");
            }

            if (soundPool != null) {
                soundPool.release();
                soundPool = null;
                Log.i(TAG, "SoundPool released");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error during audio cleanup", e);
        }
    }
}
