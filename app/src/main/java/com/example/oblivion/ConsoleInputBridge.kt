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

    /**
     * True while the console itself has the sink shown, tracked separately from
     * [consoleShown] because hideConsoleInput() also runs from stop(), where
     * consoleShown has already been reset. Clearing the field on a stop that
     * never opened the console would blank the shared sink out from under the
     * character NAME field through its TextWatcher.
     */
    private var ownsSink = false

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
            // The sink is shared with the NAME field, so the console's submit
            // must only fire while the console actually owns it — otherwise
            // finishing a character name would run it as a console command.
            if (!consoleShown) {
                false
            } else if (actionId == EditorInfo.IME_ACTION_DONE ||
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
                if (event.action == KeyEvent.ACTION_DOWN && event.repeatCount == 0 &&
                    consoleShown
                ) {
                    submit()
                }
                // Both edges must be swallowed. Leaving the key-up to the default
                // handling runs a focus search, which moves focus off the input
                // line and the keyboard stops reaching the console — or, while the
                // NAME field owns the sink, stops the name from being typed.
                true
            }
            KeyEvent.KEYCODE_ESCAPE -> {
                if (event.action == KeyEvent.ACTION_DOWN && consoleShown) {
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
        // On close the field is cleared while consoleShown is still true: the
        // character NAME bridge's TextWatcher blocks on consoleActive(), so
        // clearing after the flag drops would let a console clear through a
        // stale NAME ownership and blank the name (verifier R1). On open the
        // flag goes up first, as before. hideConsoleInput() does not read
        // consoleShown, so the reorder changes nothing else.
        if (visible) {
            consoleShown = true
            Log.i(TAG, "Debug console opened")
            showConsoleInput()
        } else {
            Log.i(TAG, "Debug console closed")
            hideConsoleInput()
            consoleShown = false
        }
    }

    private fun showConsoleInput() {
        ownsSink = true
        input.setText("")
        input.visibility = View.VISIBLE
        input.requestFocus()
        requestIme()
        IME_RETRY_DELAYS_MS.forEach { handler.postDelayed(showImeRunnable, it) }
    }

    private fun hideConsoleInput() {
        handler.removeCallbacks(showImeRunnable)
        inputMethodManager?.hideSoftInputFromWindow(input.windowToken, 0)
        // Clear only when the console actually held the sink. This runs from
        // stop() too, which MainActivity calls before the NAME bridge stops, so
        // an unconditional clear would wipe the character name the user typed
        // whenever the activity paused.
        if (ownsSink) {
            ownsSink = false
            input.setText("")
        }
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
        // Defence in depth: the listeners above already check consoleShown, and
        // this stops any future caller from running a NAME-field edit as a
        // console command through the shared sink.
        if (!consoleShown) {
            return
        }
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
