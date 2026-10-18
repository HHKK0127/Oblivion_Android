#pragma once

/**
 * @brief Complex-scene update throttle (RE section 54.2, FUN_00889810).
 *
 * The original calls FUN_00889810 once per frame from PerFrameUpdate right
 * after Timer::Update (RE section 64.4 steps 6-7; sole caller 0x40D8C2) with
 * the frame's dt_game (Timer::fDeltaTime at [0xB33E9C], i.e. [0xB33E90]+0x0C)
 * and the SetSceneIsComplex latch ([0xB333B8], RE section 50.5: script cmd
 * 0x115D, only write 0x50E9D0, cleared every frame at 0x40DBB9-0x40DBCB).
 *
 * The latch selects the update interval: latch != 0 -> ecx = 2 (update every
 * 2nd frame), latch == 0 -> ecx = 3 (every 3rd frame). dt_game is accumulated
 * (EXE accumulator 0xB2E2E0) and the internal time advances only on the
 * interval frame; skipped frames expose 0.
 *
 * Section 54.2 additionally measures 0xB2E2E8 = 1/60, 0xB2E2EC = 0.032,
 * 0xB2E2F0 = 0.05, 0xA3D14C = 166.6667 and 0xA95CA8 = 1/120 as constants the
 * function references, but does not recover which sub-case uses each one
 * (166.6667 is the Timer::Update dt clamp of section 64.3, already mirrored
 * by kMaxDelta in jni_bridge.cpp). They are intentionally not reproduced
 * until their role is identified.
 *
 * The function writes three outputs (EXE 0xBA790C / 0xBA7914 / 0xBA7918)
 * whose consumer section 54.2 does not identify (section 64.4 labels the call
 * site "UI::Update"). No port consumer exists yet, so the values are exposed
 * via accessors for the future wiring.
 */
class SceneTimeThrottle {
public:
    static SceneTimeThrottle& instance() {
        static SceneTimeThrottle inst;
        return inst;
    }

    /**
     * Frame-loop entry (EXE caller 0x40D8C2). Accumulates dt_game, applies
     * the latch interval, advances internal time once every interval frames,
     * then clears the one-frame latch (EXE 0x40DBB9-0x40DBCB).
     */
    void frame_update(float dt_game) {
        const int interval = scene_complex_ ? 2 : 3;
        accumulator_ += dt_game;
        ++frames_since_update_;
        if (frames_since_update_ >= interval) {
            throttled_dt_ = accumulator_;
            elapsed_time_ += accumulator_;
            accumulator_ = 0.0f;
            frames_since_update_ = 0;
        } else {
            throttled_dt_ = 0.0f;
        }
        last_interval_ = interval;
        scene_complex_ = false;
    }

    /**
     * RE section 50.5: latch setter for script cmd 0x115D SetSceneIsComplex
     * (EXE writes the "arg1 != 0" result at 0x50E9D0).
     */
    static void set_scene_is_complex(bool is_complex) {
        instance().scene_complex_ = is_complex;
    }

    /** Throttled dt exposed this frame: accumulated dt on update frames, 0 otherwise. */
    float throttled_dt() const { return throttled_dt_; }

    /** Internal time advanced by the throttle so far. */
    float elapsed_time() const { return elapsed_time_; }

    /** Raw accumulator mirror of EXE 0xB2E2E0: pending dt between updates. */
    float accumulated_dt() const { return accumulator_; }

    /** Interval of the last processed frame: 2 when complex, 3 otherwise. */
    int update_interval() const { return last_interval_; }

private:
    SceneTimeThrottle() = default;

    bool scene_complex_ = false;   // EXE 0xB333B8 (cleared each frame)
    int last_interval_ = 3;        // ecx: latch == 0 -> 3 (RE section 54.2)
    int frames_since_update_ = 0;
    float accumulator_ = 0.0f;     // EXE 0xB2E2E0
    float throttled_dt_ = 0.0f;
    float elapsed_time_ = 0.0f;
};
