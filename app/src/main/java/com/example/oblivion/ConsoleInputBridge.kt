package com.example.oblivion

import android.app.Activity
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.view.KeyEvent
import android.view.View
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.widget.EditText

/**
 * Drives the native debug console from the Android soft keyboard.
 *
 * The native GameConsole keeps its own input buffer, so instead of mirroring
 * keystrokes this bridge keeps the command line in a real EditText and hands the
 * finished line to [GameRenderer.nativeExecuteConsoleCommand] when the user
 * submits. IME composition, paste and deletion therefore stay entirely on the
 * Android side and there is only ever one buffer to keep consistent.
 *
 * Console visibility is polled rather than pushed because the console can also be
 * closed from native code (tap outside the panel, Escape, a console command).
 */
class ConsoleInputBridge(
    private val activity: Activity,
    private val input: EditText,
    private val rendererProvider: () -> GameRenderer?
) {
    companion object {
        private const val TAG = "ConsoleInputBridge"

        /** How often native console visibility is polled, in milliseconds. */
        private const val POLL_INTERVAL_MS = 200L

        /**
         * Extra delays used to retry the IME request. The first attempt can race
         * with the window regaining focus, so a couple of retries make the
         * keyboard reliable without polling it forever.
         */
        private val IME_RETRY_DELAYS_MS = longArrayOf(150L, 400L)
    }

    private val handler = Handler(Looper.getMainLooper())
    private val inputMethodManager =
        activity.getSystemService(Context.INPUT_METHOD_SERVICE) as? InputMethodManager

    private var running = false
    private var consoleShown = false

    /** True while the debug console is open and the input line owns the keyboard. */
    val isConsoleActive: Boolean
        get() = consoleShown

    private val pollRunnable = object : Runnable {
        override fun run() {
            syncConsoleVisibility()
            if (running) {
                handler.postDelayed(this, POLL_INTERVAL_MS)
            }
        }
    }

    private val showImeRunnable = Runnable { requestIme() }

    /** Forces an immediate visibility check instead of waiting for the next poll. */
    fun refresh() {
        if (!running) {
            return
        }
        handler.removeCallbacks(pollRunnable)
        syncConsoleVisibility()
        handler.postDelayed(pollRunnable, POLL_INTERVAL_MS)
    }

    private val editorActionListener =
        android.widget.TextView.OnEditorActionListener { _, actionId, _ ->
            if (actionId == EditorInfo.IME_ACTION_DONE ||
                actionId == EditorInfo.IME_ACTION_UNSPECIFIED
            ) {
                submit()
                true
            } else {
                false
            }
        }

    private val keyListener = View.OnKeyListener { _, keyCode, event ->
        when (keyCode) {
            KeyEvent.KEYCODE_ENTER, KeyEvent.KEYCODE_NUMPAD_ENTER -> {
                if (event.action == KeyEvent.ACTION_DOWN && event.repeatCount == 0) {
                    submit()
                }
                // Both edges must be swallowed. Leaving the key-up to the default
                // handling runs a focus search, which moves focus off the input
                // line and the keyboard stops reaching the console.
                true
            }
            KeyEvent.KEYCODE_ESCAPE -> {
                if (event.action == KeyEvent.ACTION_DOWN) {
                    closeConsole()
                }
                true
            }
            else -> false
        }
    }

    fun start() {
        if (running) {
            return
        }
        running = true
        input.setOnEditorActionListener(editorActionListener)
        input.setOnKeyListener(keyListener)
        handler.post(pollRunnable)
    }

    fun stop() {
        if (!running) {
            return
        }
        running = false
        handler.removeCallbacks(pollRunnable)
        handler.removeCallbacks(showImeRunnable)
        input.setOnEditorActionListener(null)
        input.setOnKeyListener(null)
        hideConsoleInput()
        consoleShown = false
    }

    private fun syncConsoleVisibility() {
        val renderer = rendererProvider() ?: return
        val visible = try {
            renderer.nativeIsGameConsoleVisible()
        } catch (e: Throwable) {
            Log.e(TAG, "nativeIsGameConsoleVisible failed: ${e.message}")
            return
        }
        if (visible == consoleShown) {
            return
        }
        consoleShown = visible
        Log.i(TAG, "Debug console ${if (visible) "opened" else "closed"}")
        if (visible) {
            showConsoleInput()
        } else {
            hideConsoleInput()
        }
    }

    private fun showConsoleInput() {
        input.setText("")
        input.visibility = View.VISIBLE
        input.requestFocus()
        requestIme()
        IME_RETRY_DELAYS_MS.forEach { handler.postDelayed(showImeRunnable, it) }
    }

    private fun hideConsoleInput() {
        handler.removeCallbacks(showImeRunnable)
        inputMethodManager?.hideSoftInputFromWindow(input.windowToken, 0)
        input.setText("")
        input.clearFocus()
        input.visibility = View.GONE
    }

    private fun requestIme() {
        if (!consoleShown || !running) {
            return
        }
        if (!input.hasFocus()) {
            input.requestFocus()
        }
        inputMethodManager?.showSoftInput(input, InputMethodManager.SHOW_IMPLICIT)
    }

    private fun submit() {
        val command = input.text.toString().trim()
        input.setText("")
        if (command.isEmpty()) {
            return
        }
        Log.i(TAG, "Console command: $command")
        rendererProvider()?.nativeExecuteConsoleCommand(command)
    }

    private fun closeConsole() {
        rendererProvider()?.nativeOnConsoleKeyPress(KeyEvent.KEYCODE_ESCAPE)
    }
}
