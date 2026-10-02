package com.example.oblivion

import android.content.Context
import android.util.AttributeSet
import android.view.MotionEvent
import android.widget.ScrollView

/**
 * ScrollView used by the debug panel.
 *
 * While the content is still settling after a scroll (fling in progress or a
 * short grace period after the last scroll event), a tap can land on a button
 * that has shifted from where the user saw it, or leak through to the native
 * engine. Intercepting the ACTION_DOWN during that window absorbs the tap so
 * the user has to tap again once the content has settled.
 */
class DebugScrollView : ScrollView {

    /** True while the content is scrolling or settling after a scroll. */
    var isSettling: Boolean = false

    constructor(context: Context) : super(context)

    constructor(context: Context, attrs: AttributeSet) : super(context, attrs)

    constructor(context: Context, attrs: AttributeSet, defStyleAttr: Int) :
        super(context, attrs, defStyleAttr)

    override fun onInterceptTouchEvent(ev: MotionEvent): Boolean {
        // Absorb taps that arrive while the content is still settling so they
        // do not hit the wrong button at a shifted position. Dragging is still
        // allowed: the ScrollView takes over the gesture and scrolls normally.
        if (ev.actionMasked == MotionEvent.ACTION_DOWN && isSettling) {
            return true
        }
        return super.onInterceptTouchEvent(ev)
    }
}