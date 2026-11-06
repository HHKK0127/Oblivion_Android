#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>

/**
 * @brief Global 3-slot full-screen fader overlay array, ported from the
 * original .data array at 0x00B12DC0 (stride 0x18, three entries).
 *
 * RE 48.2 (docs/RE_EXE_REPORT.md 48.2): each slot mirrors one 24-byte record:
 *   +0x00 slot type flag (0 = normal fade, 1 = letterbox-type)
 *   +0x04 texture name, sprintf'ed into "Data\Textures\Menus\Faders\%s"
 *         (template 0xA688F0)
 *   +0x08 enabled
 *   +0x0C duration in seconds (the per-frame step is dt / duration)
 *   +0x10 spawned texture object (NULL = not loaded)
 *   +0x14 current progress, used directly as the full-screen quad alpha
 * The .data initials are slot 0 "LetterBox.dds" (type 1), slot 1 "Black.dds"
 * (type 1) and slot 2 "Black.dds" (type 0); all disabled, duration 0,
 * no texture, current 0.
 *
 * FUN_00572F60 runs once per frame from the main loop (raw disassembly
 * tmp/re/f572f60.txt): slots with no texture are skipped; an enabled slot
 * ramps current toward 1.0 (step dt / duration, clamped) and draws through
 * FUN_004A2A90; once current reaches 1.0 the slot is left alone (no update,
 * no draw). A disabled slot with current > 0 ramps down to 0.0 and draws;
 * once current is 0.0 the texture object is released (owner vtable+0x88 +
 * atomic refcount) and the slot empties, with no draw on the release frame.
 *
 * FUN_00572EC0(slot, flag) resets one slot (cdecl, ret 8; the decoded body
 * supersedes the 48.2 prose, which wrongly claims +0x00/+0x04 are cleared):
 *   - no texture loaded: no-op;
 *   - flag == 0 and the menu latch [0x00B3A6D4] == 0: clear enabled only,
 *     letting the following frames fade the texture out;
 *   - otherwise: release the texture object, zero current, drop the pointer
 *     and notify FUN_00440AF0. +0x00 and +0x04 are never touched.
 * The main loop calls reset(2, 1) every frame, so slot 2 only survives while
 * something re-spawns its texture each frame.
 *
 * The class is renderer-agnostic: the draw/release hooks are injected
 * callbacks (FUN_004A2A90 and the refcount release have no port-side
 * equivalent yet) and default to no-ops, as do the slot-2 request hook
 * (0x5ADB40, unidentified) and the post-reset hook (FUN_00440AF0). The
 * texture spawn routine at 0x5730B0 is out of scope; set_texture() is the
 * injection point. Out-of-range slot indexes are no-ops; the original has
 * no bounds check.
 */
class FaderSlotArray {
public:
    static constexpr int kSlotCount = 3;

    struct Slot {
        int type_flag = 0;              // +0x00
        std::string texture_name;       // +0x04
        bool enabled = false;           // +0x08
        float duration = 0.0f;          // +0x0C seconds
        void* texture_object = nullptr; // +0x10
        float current = 0.0f;           // +0x14 progress / alpha
    };

    FaderSlotArray();

    // Per-frame advance + draw (FUN_00572F60). dt is the frame time the
    // original reads from the global DAT_00B33E9C. arg gates the slot-2
    // request hook exactly like the original (hook only when arg != 0).
    void update(float dt, uint32_t arg);

    // Reset one slot (FUN_00572EC0). See the class comment for the paths.
    void reset(int slot, int flag);

    // FUN_00572EA0: the +0x14 getter (0.0 for out-of-range slots).
    float current(int slot) const;

    // FUN_00572DF0: enabled && current <= 1.0.
    bool is_active(int slot) const;

    // FUN_00572E30: enabled && current == 1.0.
    bool is_complete(int slot) const;

    const Slot& slot(int index) const { return slots_[index]; }

    void set_enabled(int slot, bool enabled);
    void set_duration(int slot, float seconds);
    void set_texture(int slot, void* texture_object);
    void set_current(int slot, float current);

    // Original template 0xA688F0: "Data\Textures\Menus\Faders\%s". The port
    // uses forward slashes for the asset loader.
    static std::string texture_path(const std::string& name);

    // Injection points (all optional; unset = no-op).
    void set_draw_hook(
        std::function<void(int slot, void* texture, float alpha)> hook) {
        draw_hook_ = std::move(hook);
    }
    void set_release_hook(std::function<void(int slot, void* texture)> hook) {
        release_hook_ = std::move(hook);
    }
    void set_menu_latch_query(std::function<bool()> query) {
        menu_latch_query_ = std::move(query);
    }
    void set_slot2_request_hook(std::function<void(int slot)> hook) {
        slot2_request_hook_ = std::move(hook);
    }
    void set_post_reset_hook(std::function<void()> hook) {
        post_reset_hook_ = std::move(hook);
    }

private:
    static bool valid(int slot) { return slot >= 0 && slot < kSlotCount; }

    // duration == 0 makes the original divide by zero (1/0 = +inf, an
    // instant clamp for dt > 0; 0 * inf is NaN). The port returns an
    // infinite step when dt > 0 and 0 when dt == 0; negative durations
    // keep the original's dt / duration arithmetic.
    static float step(float dt, float duration);

    std::array<Slot, kSlotCount> slots_;
    std::function<void(int, void*, float)> draw_hook_;
    std::function<void(int, void*)> release_hook_;
    std::function<bool()> menu_latch_query_;
    std::function<void(int)> slot2_request_hook_;
    std::function<void()> post_reset_hook_;
};
