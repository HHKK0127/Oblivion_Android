package com.example.oblivion;

import android.app.Activity;
import android.os.Bundle;

public class MainActivity extends Activity {

    private GameSurfaceView gameSurfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // GameSurfaceView をメインビューとして設定
        gameSurfaceView = new GameSurfaceView(this);
        setContentView(gameSurfaceView);
    }

    @Override
    protected void onPause() {
        super.onPause();
        gameSurfaceView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        gameSurfaceView.onResume();
    }
}
