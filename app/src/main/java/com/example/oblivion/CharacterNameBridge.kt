package com.example.oblivion

import android.app.Activity
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.text.Editable
import android.text.TextWatcher
import android.util.Log
import android.view.View
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.widget.EditText

/**
 * Drives the character-creation NAME field from the Android soft keyboard.
 *
 * The original writes the name as it is typed rather than on submit (RE task
 * #123: built-in editor, realtime write to the player), so unlike
 * [ConsoleInputBridge] this bridge does not wait for a finished line. It keeps
 * the shared 1x1 input sink in sync with the native NAME tab and pushes every
 * edit straight back through [GameRenderer.nativeOnCharacterNameChanged].
 *
 * The sink is the same invisible EditText the console uses: it is 1x1 with
 * alpha 0 because the IME will not attach to a GONE view (see the comment in
 * activity_main.xml). The debug console therefore wins any overlap — it is the
 * debugging tool and it owns the field first — and this bridge stays idle while
 * [ConsoleInputBridge.isConsoleActive] is true.
 */
class CharacterNameBridge(
    private val activity: Activity,
    private val input: EditText,
    private val rendererProvider: () -> GameRenderer?,
    private val consoleActive: () -> Boolean
) {
    companion object {
        private const val TAG = "CharacterNameBridge"

        /** How often native NAME-tab ownership is polled, in milliseconds. */
        private const val POLL_INTERVAL_MS = 200L

        /** Retries for the IME request, which can race window focus. */
        private val IME_RETRY_DELAYS_MS = longArrayOf(150L, 400L)
    }

    private val handler = Handler(Looper.getMainLooper())
    private val inputMethodManager =
        activity.getSystemService(Context.INPUT_METHOD_SERVICE) as? InputMethodManager

    private var running = false

    /** True while this bridge, and not the console, has the sink shown. */
    private var shown = false

    /**
     * Public so MainActivity can keep the menu key forwarding from stealing
     * Enter/D-pad while the NAME field owns the keyboard — the same arbitration
     * the debug console already gets.
     */
    val isNameInputActive: Boolean
        get() = shown

    /** Set while this bridge is the one writing, so the watcher stays quiet. */
    private var seeding = false

    private val pollRunnable = object : Runnable {
        override fun run() {
            syncNameInput()
            if (running) {
                handler.postDelayed(this, POLL_INTERVAL_MS)
            }
        }
    }

    private val showImeRunnable = Runnable { requestIme() }

    private val nameWatcher = object : TextWatcher {
        override fun beforeTextChanged(s: CharSequence?, start: Int, count: Int, after: Int) = Unit

        override fun onTextChanged(s: CharSequence?, start: Int, before: Int, count: Int) = Unit

        override fun afterTextChanged(s: Editable?) {
            // Ownership matters here: the sink is shared with the debug console,
            // which also writes to it (its show/hide/submit all call setText).
            // Without this gate those writes would land in the name field, so
            // opening the console once would blank "Adventurer" and console
            // keystrokes would appear on the NAME tab in realtime.
            if (seeding || !running || !shown || consoleActive()) {
                return
            }
            rendererProvider()?.nativeOnCharacterNameChanged(s?.toString() ?: "")
        }
    }

    fun start() {
        if (running) {
            return
        }
        running = true
        input.addTextChangedListener(nameWatcher)
        handler.post(pollRunnable)
    }

    fun stop() {
        if (!running) {
            return
        }
        running = false
        handler.removeCallbacks(pollRunnable)
        handler.removeCallbacks(showImeRunnable)
        input.removeTextChangedListener(nameWatcher)
        hideNameInput()
    }

    /** Forces an immediate ownership check instead of waiting for the next poll. */
    fun refresh() {
        if (!running) {
            return
        }
        handler.removeCallbacks(pollRunnable)
        syncNameInput()
        handler.postDelayed(pollRunnable, POLL_INTERVAL_MS)
    }

    private fun syncNameInput() {
        val renderer = rendererProvider() ?: return
        val wantsName = try {
            renderer.nativeIsCharacterNameInputActive()
        } catch (e: Throwable) {
            Log.e(TAG, "nativeIsCharacterNameInputActive failed: ${e.message}")
            return
        }

        // The console owns the sink outright; release ours without touching the
        // view's visibility, which is the console's to set from here on.
        if (consoleActive()) {
            if (shown) {
                releaseIme()
                shown = false
            }
            return
        }

        if (wantsName == shown) {
            return
        }
        shown = wantsName
        Log.i(TAG, "Character name input ${if (wantsName) "opened" else "closed"}")
        if (wantsName) {
            showNameInput()
        } else {
            hideNameInput()
        }
    }

    private fun showNameInput() {
        // The sink is invisible (1x1, alpha 0), so it starts empty on purpose:
        // the first keystroke then replaces the default name rather than
        // appending to it, and leaving it untouched leaves the default alone.
        // The watcher is suppressed while the view is being reset.
        seeding = true
        try {
            input.setText("")
            input.visibility = View.VISIBLE
            input.requestFocus()
            requestIme()
            IME_RETRY_DELAYS_MS.forEach { handler.postDelayed(showImeRunnable, it) }
        } finally {
            seeding = false
        }
    }

    /**
     * The shared sink's editor action while the NAME field owns it (task #143).
     *
     * The window does not resize for the keyboard, so the keyboard covers the
     * creation buttons at the bottom of the panel, and the only controls the
     * user can still reach are the IME's own keys and Back. The sink already
     * asks for `imeOptions=actionDone` (activity_main.xml), so finishing the
     * name has to dismiss the keyboard - otherwise Create can never be tapped
     * without a Back key first. Returns true when the action was consumed here.
     */
    fun handleEditorAction(actionId: Int): Boolean {
        if (!running || !shown || consoleActive()) {
            return false
        }
        val finishes = actionId == EditorInfo.IME_ACTION_DONE ||
            actionId == EditorInfo.IME_ACTION_GO ||
            actionId == EditorInfo.IME_ACTION_SEARCH ||
            actionId == EditorInfo.IME_ACTION_UNSPECIFIED
        if (!finishes) {
            return false
        }
        releaseIme()
        Log.i(TAG, "Name input: keyboard closed by the editor action")
        return true
    }

    private fun hideNameInput() {
        handler.removeCallbacks(showImeRunnable)
        releaseIme()
        seeding = true
        try {
            input.setText("")
            input.clearFocus()
            input.visibility = View.GONE
        } finally {
            seeding = false
        }
        shown = false
    }

    private fun releaseIme() {
        inputMethodManager?.hideSoftInputFromWindow(input.windowToken, 0)
    }

    private fun requestIme() {
        if (!shown || !running || consoleActive()) {
            return
        }
        if (!input.hasFocus()) {
            input.requestFocus()
        }
        inputMethodManager?.showSoftInput(input, InputMethodManager.SHOW_IMPLICIT)
    }
}
