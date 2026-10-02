package com.example.oblivion

import android.app.Activity
import android.app.AlertDialog
import android.content.Intent
import android.media.AudioManager
import android.media.MediaPlayer
import android.media.SoundPool
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.RadioButton
import android.widget.RadioGroup
import android.widget.TextView
import androidx.documentfile.provider.DocumentFile
import java.io.IOException
import java.io.File

class MainActivity : Activity() {

    private var gameSurfaceView: GameSurfaceView? = null
    private var gameRenderer: GameRenderer? = null
    private var mediaPlayer: MediaPlayer? = null
    private var soundPool: SoundPool? = null
    private val loadedSounds = mutableMapOf<String, Int>() // filename → soundId
    private var spLoadListener: SoundPool.OnLoadCompleteListener? = null
    private var debugButtonPanel: LinearLayout? = null
    private var debugOverlayContainer: FrameLayout? = null
    private var isDebugPanelVisible = false
    private var isDebugMenuOpen = false

    // Tracks whether the debug panel ScrollView is scrolling (or has just
    // scrolled). Used to absorb taps that arrive right after a scroll so they
    // do not leak to the native engine or hit the wrong button.
    private var isScrollViewScrolling = false
    private var lastScrollTime = 0L
    private val scrollSettleDelayMs = 500L

    companion object {
        private const val TAG = "MainActivity"

        /** Intent extra that makes the activity run the native test suites and exit. */
        const val EXTRA_RUN_NATIVE_TESTS = "run_native_tests"

        private const val REQUEST_CODE_PICK_DATA_FOLDER = 1001
        private const val DATA_DIR_NAME = "data"

        // Game data source selection (APK bundled vs. Steam data copied via SAF)
        private const val PREF_NAME = "game_data"
        private const val PREF_DATA_SOURCE = "data_source"
        const val DATA_SOURCE_BUNDLED = "bundled"
        const val DATA_SOURCE_STEAM = "steam"

        @Volatile
        private var instance: MainActivity? = null

        fun getInstance(): MainActivity? = instance
    }

    private fun runNativeTestsAndFinish() {
        Thread {
            var allPassed = false
            try {
                val dataDir = File(filesDir, "data")
                val summary = GameRenderer().runAllNativeTests(dataDir.absolutePath)
                allPassed = !summary.contains("[FAIL]")
                Log.i(TAG, "=== NATIVE TEST RESULTS START ===\n$summary\n=== NATIVE TEST RESULTS END ===")
            } catch (t: Throwable) {
                Log.e(TAG, "Native test run failed", t)
            } finally {
                Log.i(
                    TAG,
                    if (allPassed) "NATIVE TESTS: ALL PASSED" else "NATIVE TESTS: SOME FAILED"
                )
                runOnUiThread { finish() }
            }
        }.start()
    }

    fun playBGM(filename: String) {
        runOnUiThread { playBGMInternal(filename) }
    }

    fun stopBGM() {
        runOnUiThread {
            mediaPlayer?.let {
                if (it.isPlaying) {
                    it.stop()
                    Log.i(TAG, "BGM stopped")
                }
            }
        }
    }

    fun playSE(filename: String) {
        runOnUiThread { playSEInternal(filename) }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.i(TAG, "=== onCreate called ===")

        // Dismiss keyguard and turn screen on
        @Suppress("DEPRECATION")
        window.addFlags(
            android.view.WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED or
            android.view.WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON or
            android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON or
            android.view.WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD
        )

        instance = this

        if (intent?.getBooleanExtra(EXTRA_RUN_NATIVE_TESTS, false) == true) {
            Log.i(TAG, "=== Native test mode requested ===")
            runNativeTestsAndFinish()
            return
        }

        try {
            // Initialize game immediately - asset extraction is optional
            Log.i(TAG, "Initializing game (asset extraction deferred)")
            initializeGame()

            // Extract assets in background if needed (non-blocking)
            Thread {
                try {
                    val assetExtractor = AssetExtractor(this@MainActivity)
                    if (assetExtractor.needsExtraction()) {
                        Log.i(TAG, "Background: Extracting assets to external storage")
                        assetExtractor.extractAssets { current, total ->
                            Log.d(TAG, "Extracting: $current/$total")
                        }
                        Log.i(TAG, "Background: Asset extraction complete")
                    } else {
                        Log.i(TAG, "Background: Assets already extracted")
                    }
                } catch (e: Exception) {
                    Log.w(TAG, "Background asset extraction failed (non-fatal): ${e.message}")
                }
                // Prepare game data based on the selected data source.
                // "bundled": extract .bsa/.esm bundled in APK assets/data into filesDir/data.
                // "steam":  data was already copied via SAF, nothing to do here.
                try {
                    if (getDataSource() == DATA_SOURCE_BUNDLED) {
                        val assetExtractor = AssetExtractor(this@MainActivity)
                        val bundled = assetExtractor.listBundledGameData()
                        if (bundled.isNotEmpty()) {
                            Log.i(TAG, "Background: Extracting ${bundled.size} bundled game data file(s)")
                            assetExtractor.extractBundledGameData { current, total ->
                                Log.d(TAG, "Bundled data: $current/$total")
                            }
                        }
                    }
                } catch (e: Exception) {
                    Log.w(TAG, "Background bundled data extraction failed (non-fatal): ${e.message}")
                }
            }.start()
        } catch (e: Exception) {
            Log.e(TAG, "Exception in onCreate: ${e.message}", e)
        }
    }

    private fun initializeGame() {
        try {
            Log.i(TAG, "Initializing audio system")
            initializeAudio()

            Log.i(TAG, "Setting content view with debug overlay")
            setContentView(R.layout.activity_main)

            // Get the GLSurfaceView from layout and setup renderer
            val glSurfaceView = findViewById<android.opengl.GLSurfaceView>(R.id.gl_surface_view)
            if (glSurfaceView != null) {
                // Create GameRenderer with default constructor
                gameRenderer = GameRenderer()
                gameRenderer!!.setContext(this)
                gameRenderer!!.setOnExitRequestedListener {
                    Log.i(TAG, "Exit requested - finishing activity")
                    runOnUiThread { finish() }
                }
                glSurfaceView.setEGLContextClientVersion(3)
                glSurfaceView.setRenderer(gameRenderer!!)
                glSurfaceView.renderMode = android.opengl.GLSurfaceView.RENDERMODE_CONTINUOUSLY

                // Setup touch event forwarding
                glSurfaceView.setOnTouchListener { view, event ->
                    val actionMasked = event.actionMasked
                    val rawX = event.rawX
                    val rawY = event.rawY

                    if (actionMasked == android.view.MotionEvent.ACTION_DOWN) {
                        Log.d(TAG, "GLSurfaceView touch DOWN at ($rawX, $rawY)")
                    }

                    // When native DebugMenu is visible, forward all touches to native
                    // so the in-game DebugMenu UI (tabs, buttons) can react.
                    val isNativeMenuVisible = gameRenderer?.nativeIsDebugMenuVisible() ?: false
                    if (actionMasked == android.view.MotionEvent.ACTION_DOWN) {
                        Log.d(TAG, "isNativeMenuVisible=$isNativeMenuVisible")
                    }

                    val shouldForwardToNative = !isDebugPanelVisible || isNativeMenuVisible
                    if (actionMasked == android.view.MotionEvent.ACTION_DOWN) {
                        Log.d(TAG, "shouldForwardToNative=$shouldForwardToNative isDebugPanelVisible=$isDebugPanelVisible isNativeMenuVisible=$isNativeMenuVisible isScrollViewScrolling=$isScrollViewScrolling")
                    }
                    if (shouldForwardToNative) {
                        // Forward touch to native (game or DebugMenu)
                        val actionIndex = event.actionIndex
                        val pointerId: Int
                        val x: Float
                        val y: Float
                        val action: Int

                        when (actionMasked) {
                            android.view.MotionEvent.ACTION_DOWN -> {
                                pointerId = event.getPointerId(0)
                                x = event.getX(0)
                                y = event.getY(0)
                                action = 0
                            }
                            android.view.MotionEvent.ACTION_UP -> {
                                pointerId = event.getPointerId(0)
                                x = event.getX(0)
                                y = event.getY(0)
                                action = 1
                            }
                            android.view.MotionEvent.ACTION_MOVE -> {
                                pointerId = event.getPointerId(0)
                                x = event.getX(0)
                                y = event.getY(0)
                                action = 2
                            }
                            android.view.MotionEvent.ACTION_POINTER_DOWN -> {
                                pointerId = event.getPointerId(actionIndex)
                                x = event.getX(actionIndex)
                                y = event.getY(actionIndex)
                                action = 5
                            }
                            android.view.MotionEvent.ACTION_POINTER_UP -> {
                                pointerId = event.getPointerId(actionIndex)
                                x = event.getX(actionIndex)
                                y = event.getY(actionIndex)
                                action = 6
                            }
                            else -> {
                                pointerId = event.getPointerId(0)
                                x = event.getX(0)
                                y = event.getY(0)
                                action = 3
                            }
                        }
                        try {
                            gameRenderer?.onTouchEvent(pointerId, x, y, action)
                        } catch (e: Exception) {
                            Log.e(TAG, "onTouchEvent JNI exception: ${e.message}")
                        }
                        if (actionMasked == android.view.MotionEvent.ACTION_DOWN) {
                            Log.d(TAG, "Forwarded touch to native: ($x, $y) action=$action")
                        }
                        return@setOnTouchListener true
                    } else if (isDebugPanelVisible && actionMasked == android.view.MotionEvent.ACTION_DOWN) {
                        // Check if touch is in debug UI area
                        val container = debugOverlayContainer
                        if (container != null && container.visibility == View.VISIBLE) {
                            val loc = IntArray(2)
                            container.getLocationOnScreen(loc)
                            val containerLeft = loc[0].toFloat()
                            val containerTop = loc[1].toFloat()
                            val containerRight = containerLeft + container.width.toFloat()
                            val containerBottom = containerTop + container.height.toFloat()

                            val toggleBtn = findViewById<Button>(R.id.btn_debug_toggle)
                            val toggleLoc = IntArray(2)
                            toggleBtn.getLocationOnScreen(toggleLoc)
                            val toggleLeft = toggleLoc[0].toFloat()
                            val toggleTop = toggleLoc[1].toFloat()
                            val toggleRight = toggleLeft + toggleBtn.width.toFloat()
                            val toggleBottom = toggleTop + toggleBtn.height.toFloat()

                            // Check if touch is in debug UI
                            val inDebugArea = (rawX >= containerLeft && rawX <= containerRight &&
                                              rawY >= containerTop && rawY <= containerBottom) ||
                                             (rawX >= toggleLeft && rawX <= toggleRight &&
                                              rawY >= toggleTop && rawY <= toggleBottom)

                            if (inDebugArea) {
                                // Absorb taps that arrive right after a scroll so
                                // they do not leak to the native engine or hit the
                                // wrong button while the content is still settling.
                                if (isScrollViewScrolling) {
                                    Log.d(TAG, "Touch in debug area absorbed (scroll settling)")
                                    return@setOnTouchListener true
                                }
                                Log.d(TAG, "Touch in debug area, consuming it (Android handles it)")
                                // Return true so GameSurfaceView.onTouchEvent is not
                                // invoked; returning false would fall through to the
                                // surface view and leak the touch to the native engine.
                                return@setOnTouchListener true
                            }
                        }
                    }

                    // If we get here, don't forward to native
                    true
                }

                Log.i(TAG, "GLSurfaceView setup complete")
            } else {
                Log.e(TAG, "GLSurfaceView not found in layout")
                // Fallback to creating GameSurfaceView directly
                gameSurfaceView = GameSurfaceView(this)
                setContentView(gameSurfaceView)
            }

            // Setup debug buttons
            setupDebugButtons()

            // Setup game data transfer buttons
            setupDataButtons()

            Log.i(TAG, "ContentView set successfully")
        } catch (e: Exception) {
            Log.e(TAG, "Exception in initializeGame: ${e.message}", e)
        }
    }

    private fun setupDebugButtons() {
        try {
            debugOverlayContainer = findViewById<FrameLayout>(R.id.debug_overlay_container)
            debugButtonPanel = findViewById<LinearLayout>(R.id.debug_button_panel)
            val debugToggleBtn = findViewById<Button>(R.id.btn_debug_toggle)
            val closeDebugBtn = findViewById<Button>(R.id.btn_close_debug)

            // Track ScrollView scroll state so taps arriving right after a
            // scroll can be absorbed (prevents leaks to the native engine and
            // taps landing on the wrong button while the content is settling).
            val debugScrollView = findViewById<DebugScrollView>(R.id.debug_scroll_view)
            debugScrollView?.setOnScrollChangeListener { _, _, _, _, _ ->
                isScrollViewScrolling = true
                debugScrollView.isSettling = true
                lastScrollTime = System.currentTimeMillis()
                debugScrollView.postDelayed({
                    if (System.currentTimeMillis() - lastScrollTime >= scrollSettleDelayMs) {
                        isScrollViewScrolling = false
                        debugScrollView.isSettling = false
                    }
                }, scrollSettleDelayMs)
            }

            // Toggle debug panel visibility
            debugToggleBtn.setOnClickListener {
                // Close native DebugMenu if it's open
                if (isDebugMenuOpen) {
                    gameRenderer?.nativeToggleDebugMenu()
                    isDebugMenuOpen = false
                }
                isDebugPanelVisible = !isDebugPanelVisible
                debugOverlayContainer?.visibility = if (isDebugPanelVisible) View.VISIBLE else View.GONE
                Log.d(TAG, "Debug panel ${if (isDebugPanelVisible) "shown" else "hidden"}")
            }

            // Close button
            closeDebugBtn.setOnClickListener {
                isDebugPanelVisible = false
                debugOverlayContainer?.visibility = View.GONE
                Log.d(TAG, "Debug panel closed")
            }

            // Debug Console toggle
            findViewById<Button>(R.id.btn_debug_console)?.setOnClickListener {
                gameRenderer?.nativeToggleDebugConsole()
                Log.d(TAG, "Toggled debug console")
            }

            // NPC Debug toggle
            findViewById<Button>(R.id.btn_debug_npc)?.setOnClickListener {
                gameRenderer?.nativeToggleNpcDebug()
                Log.d(TAG, "Toggled NPC debug")
            }

            // World Debug toggle
            findViewById<Button>(R.id.btn_debug_world)?.setOnClickListener {
                gameRenderer?.nativeToggleWorldDebug()
                Log.d(TAG, "Toggled world debug")
            }

            // Performance Graph toggle
            findViewById<Button>(R.id.btn_debug_perf)?.setOnClickListener {
                gameRenderer?.nativeTogglePerfGraph()
                Log.d(TAG, "Toggled performance graph")
            }

            // All Debug toggle
            findViewById<Button>(R.id.btn_debug_all)?.setOnClickListener {
                gameRenderer?.nativeToggleAllDebug()
                Log.d(TAG, "Toggled all debug systems")
            }

            // Debug Menu toggle - opens Assets tab and3D Viewer directly
            val menuBtn = findViewById<Button>(R.id.btn_debug_menu)
            menuBtn?.setOnClickListener {
                // Open native DebugMenu if not already open
                if (!(gameRenderer?.nativeIsDebugMenuVisible() ?: false)) {
                    gameRenderer?.nativeToggleDebugMenu()
                }
                // Select Assets tab (index 11) and toggle 3D Viewer
                gameRenderer?.nativeDebugMenuSelectTab(11)
                gameRenderer?.nativeToggle3DViewer()
                // Hide Android debug panel
                debugOverlayContainer?.visibility = View.GONE
                isDebugPanelVisible = false
                isDebugMenuOpen = true
                Log.d(TAG, "Opened3D Viewer via Assets tab")
            }

            // Open Assets Tab directly
            val assetsBtn = findViewById<Button>(R.id.btn_debug_assets)
            assetsBtn?.setOnClickListener {
                // Ensure native DebugMenu is visible
                if (!(gameRenderer?.nativeIsDebugMenuVisible() ?: false)) {
                    gameRenderer?.nativeToggleDebugMenu()
                }
                // Switch to Assets tab (index 11)
                gameRenderer?.nativeDebugMenuSelectTab(11)
                // Hide Android debug panel
                debugOverlayContainer?.visibility = View.GONE
                isDebugPanelVisible = false
                isDebugMenuOpen = true
                Log.d(TAG, "Opened Assets tab via JNI")
            }

            // Quick Actions
            findViewById<Button>(R.id.btn_debug_heal)?.setOnClickListener {
                gameRenderer?.nativeExecuteConsoleCommand("heal")
                Log.d(TAG, "Executed: heal")
            }

            findViewById<Button>(R.id.btn_debug_killall)?.setOnClickListener {
                gameRenderer?.nativeExecuteConsoleCommand("killall")
                Log.d(TAG, "Executed: killall")
            }

            findViewById<Button>(R.id.btn_debug_fly)?.setOnClickListener {
                gameRenderer?.nativeExecuteConsoleCommand("tgm")
                Log.d(TAG, "Executed: tgm (god mode)")
            }

            findViewById<Button>(R.id.btn_debug_noclip)?.setOnClickListener {
                gameRenderer?.nativeExecuteConsoleCommand("noclip")
                Log.d(TAG, "Executed: noclip")
            }

            findViewById<Button>(R.id.btn_debug_interior)?.setOnClickListener {
                gameRenderer?.nativeExecuteConsoleCommand("teleportinterior")
                Log.d(TAG, "Executed: teleportinterior")
            }

            findViewById<Button>(R.id.btn_debug_exterior)?.setOnClickListener {
                gameRenderer?.nativeExecuteConsoleCommand("teleportexterior")
                Log.d(TAG, "Executed: teleportexterior")
            }

            findViewById<Button>(R.id.btn_debug_usedoor)?.setOnClickListener {
                gameRenderer?.nativeExecuteConsoleCommand("usedoor")
                Log.d(TAG, "Executed: usedoor")
            }

            findViewById<Button>(R.id.btn_debug_gamestate)?.setOnClickListener {
                gameRenderer?.nativeExecuteConsoleCommand("gamestate")
                Log.d(TAG, "Executed: gamestate")
            }

            // Lighting A/B: the same viewpoint at noon and at midnight is what proves the
            // sun and ambient curves are actually connected (P19).
            findViewById<Button>(R.id.btn_debug_time_noon)?.setOnClickListener {
                gameRenderer?.nativeExecuteConsoleCommand("settime 12")
                Log.d(TAG, "Executed: settime 12")
            }

            findViewById<Button>(R.id.btn_debug_time_midnight)?.setOnClickListener {
                gameRenderer?.nativeExecuteConsoleCommand("settime 0")
                Log.d(TAG, "Executed: settime 0")
            }

            findViewById<Button>(R.id.btn_debug_weather_clear)?.setOnClickListener {
                gameRenderer?.nativeExecuteConsoleCommand("setweather clear")
                Log.d(TAG, "Executed: setweather clear")
            }

            findViewById<Button>(R.id.btn_debug_weather_storm)?.setOnClickListener {
                gameRenderer?.nativeExecuteConsoleCommand("setweather storm")
                Log.d(TAG, "Executed: setweather storm")
            }

            Log.i(TAG, "Debug buttons setup complete")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to setup debug buttons: ${e.message}", e)
        }
    }

    private fun initializeAudio() {
            try {
                mediaPlayer = MediaPlayer()
                volumeControlStream = AudioManager.STREAM_MUSIC
                Log.i(TAG, "MediaPlayer initialized for BGM")

                soundPool = SoundPool.Builder().setMaxStreams(5).build()
                Log.i(TAG, "SoundPool initialized for SE (max 5 sounds)")

                try {
                    GameRenderer.nativeInitAudioBridge(assets, this)
                    Log.i(TAG, "Audio bridge initialized with AssetManager and MainActivity")
                } catch (e: Exception) {
                    Log.w(TAG, "Failed to initialize audio bridge: ${e.message}")
                }

                // Set data path for BSA/ESM file lookup
                // Note: nativeSetDataPath will be called after nativeInitEngine creates the renderer
                // Store for later use in onSurfaceCreated callback
                try {
                    val dataPath = filesDir.absolutePath + File.separator + "data"
                    val dataDir = java.io.File(dataPath)
                    if (!dataDir.exists()) {
                        dataDir.mkdirs()
                        Log.i(TAG, "Created data directory: $dataPath")
                    }
                    // Store path for use after engine initialization
                    GameRenderer.dataPath = dataPath
                    Log.i(TAG, "BSA data path stored: $dataPath (will be set on native after engine init)")
                } catch (e: Exception) {
                    Log.w(TAG, "Failed to set BSA data path: ${e.message}")
                }
            } catch (e: Exception) {
                Log.e(TAG, "Failed to initialize audio", e)
            }
        }

    override fun onPause() {
        super.onPause()
        Log.i(TAG, "onPause")
        // Unregister predictive back gesture callback (Android 13+)
        if (android.os.Build.VERSION.SDK_INT >= 33) {
            backCallback?.let { onBackInvokedDispatcher.unregisterOnBackInvokedCallback(it) }
        }
        // Do NOT call gameSurfaceView.onPause() - keep GL thread running
        // GLSurfaceView.onPause() kills the render thread, which prevents onSurfaceCreated
        mediaPlayer?.let {
            if (it.isPlaying) {
                it.pause()
                Log.i(TAG, "BGM paused")
            }
        }
    }

    override fun onResume() {
        super.onResume()
        Log.i(TAG, "onResume")
        gameSurfaceView?.onResume()
        // Register predictive back gesture callback (Android 13+)
        if (android.os.Build.VERSION.SDK_INT >= 33) {
            backCallback?.let {
                onBackInvokedDispatcher.registerOnBackInvokedCallback(
                    android.window.OnBackInvokedDispatcher.PRIORITY_DEFAULT,
                    it
                )
            }
        }
        // Note: startRenderer() is called inside GameSurfaceView.onResume() if needed
    }

    override fun onDestroy() {
        super.onDestroy()
        Log.i(TAG, "onDestroy - cleaning up audio")
        cleanupAudio()
        if (instance === this) {
            instance = null
        }
    }

    @Suppress("MissingSuperCall", "DEPRECATION")
    override fun onBackPressed() {
        Log.i(TAG, "onBackPressed - forwarding to native engine")
        val consumed = gameRenderer?.nativeOnBackKey() ?: false
        if (consumed) {
            Log.d(TAG, "Back key consumed by native")
        } else {
            // Title/Launcher screen: confirm exit
            Log.i(TAG, "On Title/Launcher - showing exit confirmation")
            AlertDialog.Builder(this)
                .setTitle("Exit Oblivion?")
                .setMessage("終了しますか？")
                .setPositiveButton("Exit") { _, _ -> finish() }
                .setNegativeButton("Cancel", null)
                .show()
        }
    }

    // Android 13+ predictive back gesture support via OnBackInvokedDispatcher
    private val backCallback = if (android.os.Build.VERSION.SDK_INT >= 33) {
        object : android.window.OnBackInvokedCallback {
            override fun onBackInvoked() {
                Log.i(TAG, "onBackInvoked - predictive back gesture")
                handleBack()
            }
        }
    } else null

    // Unified back handler (Android 11+ OnBackInvokedDispatcher + legacy onBackPressed)
    private fun handleBack() {
        Log.i(TAG, "handleBack - forwarding to native engine")
        val consumed = gameRenderer?.nativeOnBackKey() ?: false
        if (consumed) {
            Log.d(TAG, "Back key consumed by native")
        } else {
            // Title/Launcher screen: confirm exit
            Log.i(TAG, "On Title/Launcher - showing exit confirmation")
            AlertDialog.Builder(this)
                .setTitle("Exit Oblivion?")
                .setMessage("終了しますか？")
                .setPositiveButton("Exit") { _, _ -> finish() }
                .setNegativeButton("Cancel", null)
                .show()
        }
    }

    private fun playBGMInternal(filename: String) {
        try {
            val mp = mediaPlayer
            if (mp == null) {
                Log.e(TAG, "MediaPlayer not initialized")
                return
            }

            if (mp.isPlaying) {
                mp.stop()
            }
            mp.reset()

            val assetPath = "audio/music/$filename"
            Log.i(TAG, "Loading BGM: $assetPath")

            val afd = assets.openFd(assetPath)
            mp.setDataSource(afd.fileDescriptor, afd.startOffset, afd.length)
            afd.close()
            mp.prepare()
            mp.isLooping = true
            mp.start()

            Log.i(TAG, "BGM playing: $filename")
        } catch (e: IOException) {
            Log.e(TAG, "Failed to play BGM: $filename", e)
        }
    }

    private fun playSEInternal(filename: String) {
        try {
            val sp = soundPool
            if (sp == null) {
                Log.e(TAG, "SoundPool not initialized")
                return
            }

            // Cache hit → play immediately
            loadedSounds[filename]?.let { cachedId ->
                sp.play(cachedId, 1.0f, 1.0f, 0, 0, 1.0f)
                Log.i(TAG, "SE playing from cache: $filename")
                return
            }

            val assetPath = "audio/sounds/$filename"
            Log.i(TAG, "Loading SE: $assetPath")

            val afd = assets.openFd(assetPath)
            val soundId = sp.load(afd, 1)
            afd.close()
            loadedSounds[filename] = soundId

            // Set listener only once
            if (spLoadListener == null) {
                spLoadListener = SoundPool.OnLoadCompleteListener { pool, sampleId, status ->
                    if (status == 0) {
                        pool.play(sampleId, 1.0f, 1.0f, 0, 0, 1.0f)
                    } else {
                        Log.e(TAG, "Failed to load SE, status=$status")
                    }
                }
                sp.setOnLoadCompleteListener(spLoadListener)
            }

            Log.i(TAG, "SE playing queued: $filename")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to play SE: $filename", e)
        }
    }

    private fun cleanupAudio() {
        try {
            mediaPlayer?.let {
                if (it.isPlaying) {
                    it.stop()
                }
                it.release()
                Log.i(TAG, "MediaPlayer released")
            }
            mediaPlayer = null

            soundPool?.let {
                it.release()
                Log.i(TAG, "SoundPool released")
            }
            loadedSounds.clear()
            spLoadListener = null
            soundPool = null
        } catch (e: Exception) {
            Log.e(TAG, "Error during audio cleanup", e)
        }
    }

    // ------------------------------------------------------------------
    // Steam game data transfer (SAF folder picker + copy to filesDir/data)
    // ------------------------------------------------------------------

    private fun setupDataButtons() {
        findViewById<Button>(R.id.btn_data_pick)?.setOnClickListener {
            startSteamDataPicker()
        }
        findViewById<Button>(R.id.btn_data_restart)?.setOnClickListener {
            Log.i(TAG, "Restarting app to reload game data")
            val intent = Intent(this, MainActivity::class.java)
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK)
            finish()
            startActivity(intent)
        }
        setupDataSourceSelection()
        refreshDataStatus()
    }

    /** Restore the persisted data-source choice and wire up the radio buttons. */
    private fun setupDataSourceSelection() {
        val radioGroup = findViewById<RadioGroup>(R.id.radio_data_source) ?: return
        val bundled = findViewById<RadioButton>(R.id.radio_data_bundled) ?: return
        val steam = findViewById<RadioButton>(R.id.radio_data_steam) ?: return
        val source = getDataSource()
        bundled.isChecked = source == DATA_SOURCE_BUNDLED
        steam.isChecked = source == DATA_SOURCE_STEAM
        radioGroup.setOnCheckedChangeListener { _, checkedId ->
            val newSource =
                if (checkedId == R.id.radio_data_bundled) DATA_SOURCE_BUNDLED else DATA_SOURCE_STEAM
            if (newSource != getDataSource()) {
                setDataSource(newSource)
                Log.i(TAG, "Data source changed to: $newSource (restart to apply)")
                refreshDataStatus()
                showDataSourceRestartHint()
            }
        }
    }

    /** Tell the user the data-source change only applies on next launch. */
    private fun showDataSourceRestartHint() {
        findViewById<TextView>(R.id.txt_data_restart_hint)?.visibility = View.VISIBLE
    }

    private fun getDataSource(): String {
        return getSharedPreferences(PREF_NAME, MODE_PRIVATE)
            .getString(PREF_DATA_SOURCE, DATA_SOURCE_BUNDLED) ?: DATA_SOURCE_BUNDLED
    }

    private fun setDataSource(source: String) {
        getSharedPreferences(PREF_NAME, MODE_PRIVATE)
            .edit()
            .putString(PREF_DATA_SOURCE, source)
            .apply()
    }

    private fun refreshDataStatus() {
        val statusText = findViewById<TextView>(R.id.txt_data_status) ?: return
        val dataDir = File(filesDir, DATA_DIR_NAME)
        val files = dataDir.listFiles()?.filter { it.isFile } ?: emptyList()
        val bsaCount = files.count { it.extension.equals("bsa", ignoreCase = true) }
        val esmCount = files.count { it.extension.equals("esm", ignoreCase = true) }
        val totalBytes = files.sumOf { it.length() }

        val bundled = AssetExtractor(this).listBundledGameData()
        val sourceLabel = if (getDataSource() == DATA_SOURCE_BUNDLED) "APK bundled" else "Steam (copied)"
        val bundledLabel = if (bundled.isEmpty()) "none in APK" else "${bundled.size} file(s) in APK"
        statusText.text =
            "Source: $sourceLabel | Bundled: $bundledLabel\n" +
            "Data folder: $DATA_DIR_NAME\n" +
            "BSA: $bsaCount, ESM: $esmCount (${formatSize(totalBytes)})"
    }

    private fun formatSize(bytes: Long): String {
        if (bytes < 1024) return "$bytes B"
        val kb = bytes / 1024.0
        if (kb < 1024) return String.format("%.1f KB", kb)
        val mb = kb / 1024.0
        if (mb < 1024) return String.format("%.1f MB", mb)
        return String.format("%.2f GB", mb / 1024.0)
    }

    private fun startSteamDataPicker() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT_TREE)
        intent.addFlags(
            Intent.FLAG_GRANT_READ_URI_PERMISSION or
            Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION or
            Intent.FLAG_GRANT_WRITE_URI_PERMISSION
        )
        startActivityForResult(intent, REQUEST_CODE_PICK_DATA_FOLDER)
    }

    @Suppress("DEPRECATION")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == REQUEST_CODE_PICK_DATA_FOLDER && resultCode == Activity.RESULT_OK) {
            val uri = data?.data ?: return
            try {
                contentResolver.takePersistableUriPermission(
                    uri,
                    Intent.FLAG_GRANT_READ_URI_PERMISSION
                )
            } catch (e: Exception) {
                Log.w(TAG, "takePersistableUriPermission failed: ${e.message}")
            }
            startSteamDataCopy(uri)
        }
    }

    private fun startSteamDataCopy(treeUri: Uri) {
        val progressText = findViewById<TextView>(R.id.txt_data_progress)
        val pickButton = findViewById<Button>(R.id.btn_data_pick)
        pickButton.isEnabled = false
        progressText.text = "Scanning folder..."

        Thread {
            try {
                val rootDoc = DocumentFile.fromTreeUri(this, treeUri)
                if (rootDoc == null) {
                    runOnUiThread {
                        progressText.text = "Cannot access the selected folder"
                        pickButton.isEnabled = true
                    }
                    return@Thread
                }
                val dataFiles = mutableListOf<DocumentFile>()
                collectDataFiles(rootDoc, dataFiles)
                if (dataFiles.isEmpty()) {
                    runOnUiThread {
                        progressText.text = "No .bsa or .esm files found. Select the Oblivion Data folder."
                        pickButton.isEnabled = true
                    }
                    return@Thread
                }
                val dataDir = File(filesDir, DATA_DIR_NAME)
                if (!dataDir.exists()) dataDir.mkdirs()

                var copied = 0
                for (doc in dataFiles) {
                    val fileName = doc.name ?: continue
                    runOnUiThread {
                        progressText.text = "Copying ($copied/${dataFiles.size}): $fileName"
                    }
                    copyDocumentToFile(doc, File(dataDir, fileName))
                    copied++
                }
                runOnUiThread {
                    progressText.text = "Copy complete: $copied file(s). Restart the app to load game data."
                    refreshDataStatus()
                    pickButton.isEnabled = true
                }
            } catch (e: Exception) {
                Log.e(TAG, "Steam data copy failed: ${e.message}", e)
                runOnUiThread {
                    progressText.text = "Copy failed: ${e.message}"
                    pickButton.isEnabled = true
                }
            }
        }.start()
    }

    private fun collectDataFiles(doc: DocumentFile, out: MutableList<DocumentFile>) {
        if (!doc.isDirectory) return
        for (child in doc.listFiles()) {
            if (child.isDirectory) {
                collectDataFiles(child, out)
            } else {
                val name = child.name ?: continue
                val ext = name.substringAfterLast('.', "").lowercase()
                if (ext == "bsa" || ext == "esm") {
                    out.add(child)
                }
            }
        }
    }

    private fun copyDocumentToFile(doc: DocumentFile, target: File) {
        val input = contentResolver.openInputStream(doc.uri) ?: return
        try {
            target.outputStream().use { out ->
                input.copyTo(out, bufferSize = 1 shl 20)
            }
        } finally {
            input.close()
        }
    }
}
