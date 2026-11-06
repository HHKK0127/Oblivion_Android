// Fader slot array tests - see fader_slot_array_tests.h for the rationale.

#include "fader_slot_array_tests.h"

#include "../ui/fader_slot_array.h"

#include <cmath>
#include <utility>
#include <vector>

namespace {

constexpr float kEps = 1e-4f;

bool near(float a, float b) {
    return std::fabs(a - b) < kEps;
}

// Stand-in for the spawned texture object; only its identity is observed.
struct DummyTexture {
    int marker = 0x5730B0;  // the original spawn routine address, for flavor
};

}  // namespace

void FaderSlotArrayTests::record(const std::string& name, bool passed,
                                 const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int FaderSlotArrayTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int FaderSlotArrayTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string FaderSlotArrayTests::getSummary() const {
    return "Fader Slot Array Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool FaderSlotArrayTests::runAllTests() {
    results.clear();

    // --- 1. .data initials (array @0x00B12DC0, RE 48.2) ----------------------
    {
        FaderSlotArray f;
        record("slot 0: type 1 LetterBox.dds",
               f.slot(0).type_flag == 1 && f.slot(0).texture_name == "LetterBox.dds");
        record("slot 1: type 1 Black.dds",
               f.slot(1).type_flag == 1 && f.slot(1).texture_name == "Black.dds");
        record("slot 2: type 0 Black.dds",
               f.slot(2).type_flag == 0 && f.slot(2).texture_name == "Black.dds");
        bool rest_zero = true;
        for (int i = 0; i < FaderSlotArray::kSlotCount; ++i) {
            const auto& s = f.slot(i);
            if (s.enabled || s.duration != 0.0f ||
                s.texture_object != nullptr || s.current != 0.0f) {
                rest_zero = false;
            }
        }
        record("all slots: disabled, duration 0, no texture, current 0", rest_zero);
    }

    // --- 2. texture path template (0xA688F0) ----------------------------------
    {
        record("texture_path formats the faders directory",
               FaderSlotArray::texture_path("Black.dds") ==
                   "Data/Textures/Menus/Faders/Black.dds");
        record("texture_path handles an empty name",
               FaderSlotArray::texture_path("") == "Data/Textures/Menus/Faders/");
    }

    // --- 3. Out-of-range indexes are safe no-ops ------------------------------
    {
        FaderSlotArray f;
        DummyTexture d;
        record("current(-1)/current(3) read as 0",
               f.current(-1) == 0.0f && f.current(3) == 0.0f);
        record("is_active/is_complete false out of range",
               !f.is_active(-1) && !f.is_active(3) &&
               !f.is_complete(-2) && !f.is_complete(9));
        f.set_enabled(5, true);
        f.set_duration(-2, 3.0f);
        f.set_texture(3, &d);
        f.set_current(7, 1.0f);
        f.reset(-1, 1);
        f.reset(3, 1);
        bool untouched = true;
        for (int i = 0; i < FaderSlotArray::kSlotCount; ++i) {
            const auto& s = f.slot(i);
            if (s.enabled || s.duration != 0.0f || s.texture_object != nullptr) {
                untouched = false;
            }
        }
        record("setters/reset out of range leave the slots untouched", untouched);
    }

    // --- 4. is_active predicate (FUN_00572DF0) --------------------------------
    {
        FaderSlotArray f;
        f.set_enabled(0, true);
        record("active: enabled with current 0", f.is_active(0));
        f.set_current(0, 1.0f);
        record("active: enabled with current 1.0 (<= bound)", f.is_active(0));
        f.set_current(0, 1.5f);
        record("inactive: current above 1.0", !f.is_active(0));
        f.set_current(0, 0.5f);
        f.set_enabled(0, false);
        record("inactive: disabled slot", !f.is_active(0) && !f.is_active(1));
    }

    // --- 5. is_complete predicate (FUN_00572E30) ------------------------------
    {
        FaderSlotArray f;
        f.set_enabled(1, true);
        f.set_current(1, 1.0f);
        record("complete: enabled with current exactly 1.0", f.is_complete(1));
        f.set_current(1, 0.999f);
        record("not complete: current just below 1.0", !f.is_complete(1));
        f.set_current(1, 1.0f);
        f.set_enabled(1, false);
        record("not complete: disabled at 1.0", !f.is_complete(1));
    }

    // --- 6. reset with no texture loaded is a no-op ---------------------------
    {
        FaderSlotArray f;
        int releases = 0;
        int posts = 0;
        f.set_release_hook([&releases](int, void*) { releases++; });
        f.set_post_reset_hook([&posts]() { posts++; });
        f.set_enabled(0, true);
        f.reset(0, 1);
        record("tex-NULL reset: nothing fires, enabled untouched",
               releases == 0 && posts == 0 && f.slot(0).enabled);
    }

    // --- 7. reset(flag=0, latch off): disable only ----------------------------
    {
        FaderSlotArray f;
        DummyTexture d;
        int releases = 0;
        int posts = 0;
        f.set_release_hook([&releases](int, void*) { releases++; });
        f.set_post_reset_hook([&posts]() { posts++; });
        f.set_texture(0, &d);
        f.set_enabled(0, true);
        f.set_current(0, 0.5f);
        f.reset(0, 0);
        record("simple reset clears enabled only", !f.slot(0).enabled);
        record("simple reset leaves texture and current",
               f.slot(0).texture_object == &d && near(f.slot(0).current, 0.5f));
        record("simple reset fires no hooks", releases == 0 && posts == 0);
    }

    // --- 8. reset full path (flag=1) + 48.2 errata survival -------------------
    {
        FaderSlotArray f;
        DummyTexture d;
        std::vector<std::pair<int, void*>> releases;
        int posts = 0;
        f.set_release_hook(
            [&releases](int slot, void* tex) { releases.push_back({slot, tex}); });
        f.set_post_reset_hook([&posts]() { posts++; });
        f.set_texture(0, &d);
        f.set_enabled(0, true);
        f.set_current(0, 0.5f);
        f.reset(0, 1);
        record("full reset releases the texture object once",
               releases.size() == 1 && releases[0].first == 0 &&
                   releases[0].second == &d);
        record("full reset empties the slot",
               f.slot(0).texture_object == nullptr &&
               near(f.slot(0).current, 0.0f));
        record("full reset notifies the post-reset hook", posts == 1);
        record("errata: +0x00/+0x04 survive a full reset",
               f.slot(0).type_flag == 1 && f.slot(0).texture_name == "LetterBox.dds");
        record("full reset does not clear enabled", f.slot(0).enabled);
    }

    // --- 9. reset(flag=0) takes the full path while the menu latch is set -----
    {
        FaderSlotArray f;
        DummyTexture d;
        int releases = 0;
        f.set_release_hook([&releases](int, void*) { releases++; });
        f.set_menu_latch_query([]() { return true; });
        f.set_texture(2, &d);
        f.set_enabled(2, true);
        f.reset(2, 0);
        record("latched flag=0 resets fully",
               releases == 1 && f.slot(2).texture_object == nullptr);
        record("latched reset leaves enabled set (no simple disable)",
               f.slot(2).enabled);
    }

    // --- 10. enabled ramp-up toward 1.0 (FUN_00572F60) ------------------------
    {
        FaderSlotArray f;
        DummyTexture d;
        std::vector<std::pair<int, float>> draws;
        f.set_draw_hook(
            [&draws](int slot, void*, float alpha) { draws.push_back({slot, alpha}); });
        f.set_texture(0, &d);
        f.set_duration(0, 1.0f);
        f.set_enabled(0, true);
        f.update(0.5f, 0);
        record("ramp: half duration advances current to 0.5",
               near(f.current(0), 0.5f) && draws.size() == 1 &&
               near(draws[0].second, 0.5f));
        f.update(0.5f, 0);
        record("ramp: reaches 1.0 and draws the final alpha",
               near(f.current(0), 1.0f) && draws.size() == 2 &&
               near(draws[1].second, 1.0f) && f.is_complete(0));
        f.update(0.5f, 0);
        record("ramp: overshoot clamps at 1.0", near(f.current(0), 1.0f));
    }

    // --- 11. fully faded slot is left alone ------------------------------------
    {
        FaderSlotArray f;
        DummyTexture d;
        int draws = 0;
        f.set_draw_hook([&draws](int, void*, float) { draws++; });
        f.set_texture(1, &d);
        f.set_duration(1, 1.0f);
        f.set_enabled(1, true);
        f.set_current(1, 1.0f);
        f.update(0.5f, 0);
        record("current >= 1.0: no update, no draw",
               draws == 0 && near(f.current(1), 1.0f) &&
                   f.slot(1).texture_object == &d);
    }

    // --- 12. slot-2 request hook gating (0x5ADB40) ----------------------------
    {
        FaderSlotArray f;
        DummyTexture d0, d2;
        std::vector<int> requests;
        f.set_slot2_request_hook(
            [&requests](int slot) { requests.push_back(slot); });
        f.set_texture(0, &d0);
        f.set_duration(0, 1.0f);
        f.set_enabled(0, true);
        f.set_texture(2, &d2);
        f.set_duration(2, 1.0f);
        f.set_enabled(2, true);
        f.update(0.1f, 0);
        record("arg == 0: no request", requests.empty());
        f.update(0.1f, 1);
        record("arg != 0: slot 2 requests once, slot 0 never",
               requests.size() == 1 && requests[0] == 2);
        f.set_enabled(2, false);
        f.set_current(2, 0.5f);
        f.update(0.1f, 1);
        record("disabled slot 2: no request", requests.size() == 1);
    }

    // --- 13. slot-2 request fires even at current >= 1.0 ----------------------
    {
        FaderSlotArray f;
        DummyTexture d;
        int requests = 0;
        int draws = 0;
        f.set_slot2_request_hook([&requests](int) { requests++; });
        f.set_draw_hook([&draws](int, void*, float) { draws++; });
        f.set_texture(2, &d);
        f.set_duration(2, 1.0f);
        f.set_enabled(2, true);
        f.set_current(2, 1.0f);
        f.update(0.1f, 1);
        record("request precedes the >= 1.0 skip; still no draw",
               requests == 1 && draws == 0 && near(f.current(2), 1.0f));
    }

    // --- 14. disabled ramp-down, then release frame ----------------------------
    {
        FaderSlotArray f;
        DummyTexture d;
        std::vector<float> alphas;
        std::vector<std::pair<int, void*>> releases;
        f.set_draw_hook(
            [&alphas](int, void*, float a) { alphas.push_back(a); });
        f.set_release_hook(
            [&releases](int slot, void* tex) { releases.push_back({slot, tex}); });
        f.set_texture(1, &d);
        f.set_duration(1, 1.0f);
        f.set_current(1, 0.4f);  // enabled stays false
        f.update(0.2f, 0);
        record("ramp-down: current 0.2 with a draw",
               near(f.current(1), 0.2f) && alphas.size() == 1 &&
               near(alphas[0], 0.2f));
        f.update(0.2f, 0);
        record("ramp-down: reaches 0.0 and still draws that frame",
               near(f.current(1), 0.0f) && alphas.size() == 2 &&
               near(alphas[1], 0.0f));
        f.update(0.2f, 0);
        record("release frame: texture released, slot emptied, no draw",
               releases.size() == 1 && releases[0].first == 1 &&
                   releases[0].second == &d &&
                   f.slot(1).texture_object == nullptr &&
                   near(f.slot(1).current, 0.0f) && alphas.size() == 2);
    }

    // --- 15. unloaded slots are skipped ----------------------------------------
    {
        FaderSlotArray f;
        int draws = 0;
        int releases = 0;
        f.set_draw_hook([&draws](int, void*, float) { draws++; });
        f.set_release_hook([&releases](int, void*) { releases++; });
        f.set_enabled(0, true);
        f.set_duration(0, 1.0f);
        f.update(0.5f, 1);
        record("no texture: no draw, no release, no advance",
               draws == 0 && releases == 0 && near(f.current(0), 0.0f));
    }

    // --- 16. zero duration: instant fade / dt == 0 -----------------------------
    {
        FaderSlotArray f;
        DummyTexture d;
        std::vector<float> alphas;
        f.set_draw_hook(
            [&alphas](int, void*, float a) { alphas.push_back(a); });
        f.set_texture(0, &d);
        f.set_enabled(0, true);  // duration stays 0.0
        f.update(0.016f, 0);
        record("duration 0 with dt > 0: instant clamp to 1.0",
               near(f.current(0), 1.0f) && alphas.size() == 1 &&
               near(alphas[0], 1.0f));

        FaderSlotArray g;
        DummyTexture d2;
        int draws = 0;
        g.set_draw_hook([&draws](int, void*, float) { draws++; });
        g.set_texture(1, &d2);
        g.set_enabled(1, true);
        g.set_current(1, 0.3f);
        g.update(0.0f, 0);
        record("duration 0 with dt == 0: current untouched, still drawn",
               near(g.current(1), 0.3f) && draws == 1);
    }

    // --- 17. slots update independently ----------------------------------------
    {
        FaderSlotArray f;
        DummyTexture d0, d1;
        std::vector<int> released;
        f.set_release_hook(
            [&released](int slot, void*) { released.push_back(slot); });
        f.set_texture(0, &d0);
        f.set_duration(0, 1.0f);
        f.set_enabled(0, true);
        f.set_texture(1, &d1);  // disabled, current 0 -> released this frame
        f.update(0.5f, 0);
        record("slot 0 ramps while slot 1 releases",
               near(f.current(0), 0.5f) && f.slot(0).texture_object == &d0 &&
                   released.size() == 1 && released[0] == 1);
        record("slot 2 stays untouched",
               f.slot(2).texture_object == nullptr && near(f.current(2), 0.0f));
    }

    return getFailCount() == 0;
}
