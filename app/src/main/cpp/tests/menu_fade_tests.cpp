// Menu fade machine tests - see menu_fade_tests.h for the rationale.

#include "menu_fade_tests.h"

#include "../ui/menu_fade_system.h"

#include <cmath>

namespace {

constexpr float kEps = 1e-4f;

bool near(float a, float b) {
    return std::fabs(a - b) < kEps;
}

}  // namespace

void MenuFadeTests::record(const std::string& name, bool passed,
                           const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int MenuFadeTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int MenuFadeTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string MenuFadeTests::getSummary() const {
    return "Menu Fade Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool MenuFadeTests::runAllTests() {
    results.clear();

    // --- 1. Static lookup with no active fade (steady-state alpha) ------------
    {
        MenuFadeSystem fade;
        record("no node: hidden menu alpha 0", near(fade.getAlpha("pause", false), 0.0f));
        record("no node: visible menu alpha 1", near(fade.getAlpha("pause", true), 1.0f));
        record("default duration is 0.25 s", near(fade.defaultDuration(), 0.25f));
        record("no node: not transitioning",
               !fade.isTransitioning("pause") && !fade.isAnyTransitioning());
    }

    // --- 2. Linear open fade (phase 2, FUN_00584740) --------------------------
    {
        MenuFadeSystem fade;
        fade.beginOpen("pause", 0.25f);
        record("open: alpha starts at 0", near(fade.getAlpha("pause", false), 0.0f));
        record("open: transitioning, not closing",
               fade.isTransitioning("pause") && !fade.isClosing("pause"));
        fade.update(0.125f);
        record("open: half way alpha 0.5", near(fade.getAlpha("pause", false), 0.5f));
        fade.update(0.0625f);
        record("open: three quarters alpha 0.75", near(fade.getAlpha("pause", false), 0.75f));
        fade.update(0.0625f);
        record("open: node removed at completion", !fade.isTransitioning("pause"));
        record("open: no node left", fade.nodes().empty());
        record("open: steady visible alpha 1", near(fade.getAlpha("pause", true), 1.0f));
    }

    // --- 3. Close fade and completion callback (phase 8, FUN_00584390) --------
    {
        MenuFadeSystem fade;
        int completions = 0;
        fade.beginClose("pause", 0.25f, [&completions]() { completions++; });
        record("close: alpha starts at 1", near(fade.getAlpha("pause", true), 1.0f));
        record("close: isClosing true",
               fade.isClosing("pause") && fade.isTransitioning("pause"));
        fade.update(0.1f);
        record("close: alpha drops linearly (1 -> 0.6)", near(fade.getAlpha("pause", true), 0.6f));
        record("close: callback not fired early", completions == 0);
        fade.update(0.15f);
        record("close: callback fired once at completion", completions == 1);
        record("close: node removed", !fade.isTransitioning("pause"));
        record("close: steady hidden alpha 0", near(fade.getAlpha("pause", false), 0.0f));
        fade.update(0.5f);
        record("close: callback not repeated", completions == 1);
    }

    // --- 4. Overshoot in a single update --------------------------------------
    {
        MenuFadeSystem fade;
        int completions = 0;
        fade.beginClose("pause", 0.25f, [&completions]() { completions++; });
        fade.update(10.0f);
        record("overshoot: completes in one frame",
               !fade.isTransitioning("pause") && completions == 1);
    }

    // --- 5. Same-id replacement: unlink then create (FUN_00584300) ------------
    {
        MenuFadeSystem fade;
        int first = 0;
        int second = 0;
        fade.beginClose("pause", 0.25f, [&first]() { first++; });
        fade.update(0.1f);
        fade.beginClose("pause", 0.25f, [&second]() { second++; });
        record("replace: still one node for the id", fade.nodes().size() == 1);
        record("replace: restarted from 0", near(fade.getAlpha("pause", true), 1.0f));
        fade.update(0.25f);
        record("replace: dropped callback never fired", first == 0);
        record("replace: new callback fired", second == 1);
    }

    // --- 6. Cross-fade: close interrupted by an open (no rewind) --------------
    {
        MenuFadeSystem fade;
        int completions = 0;
        fade.beginOpen("pause", 0.25f);
        fade.update(0.25f);
        fade.beginClose("pause", 0.25f, [&completions]() { completions++; });
        fade.update(0.2f);
        fade.beginOpen("pause", 0.25f);
        record("cross: close callback dropped on interrupt", completions == 0);
        record("cross: open restarts from 0, no rewind of close node",
               near(fade.getAlpha("pause", true), 0.0f) && !fade.isClosing("pause"));
        fade.update(0.25f);
        record("cross: ends steady visible", !fade.isTransitioning("pause"));
    }

    // --- 7. Duration clamp: 0.001 s floor (49.6.1) ----------------------------
    {
        MenuFadeSystem fade;
        fade.beginOpen("pause", 0.0f);
        record("clamp: zero duration floored, node alive", fade.isTransitioning("pause"));
        fade.update(0.001f);
        record("clamp: zero-duration open completes after 1 ms",
               !fade.isTransitioning("pause"));
        fade.beginClose("pause", -5.0f, []() {});
        record("clamp: negative duration floored, node alive", fade.isTransitioning("pause"));
        fade.update(0.001f);
        record("clamp: negative-duration close completes after 1 ms",
               !fade.isTransitioning("pause"));
    }

    // --- 8. Default duration drives the convenience overloads -----------------
    {
        MenuFadeSystem fade;
        fade.setDefaultDuration(0.5f);
        record("default duration setter", near(fade.defaultDuration(), 0.5f));
        fade.beginOpen("pause");  // no explicit duration; uses the default
        fade.update(0.25f);
        record("default duration drives beginOpen(id)",
               near(fade.getAlpha("pause", false), 0.5f));
        fade.setDefaultDuration(0.0f);
        record("default duration floored at 0.001", near(fade.defaultDuration(), 0.001f));
    }

    // --- 9. Independent nodes per menu id --------------------------------------
    {
        MenuFadeSystem fade;
        fade.beginOpen("pause", 0.25f);
        fade.beginClose("options", 0.25f);
        fade.update(0.125f);
        record("multi: per-id alphas (open 0.5 / close 0.5)",
               near(fade.getAlpha("pause", false), 0.5f) &&
               near(fade.getAlpha("options", true), 0.5f));
        record("multi: both transitioning", fade.isTransitioning("pause") &&
               fade.isTransitioning("options") && fade.nodes().size() == 2);
        fade.update(0.125f);
        record("multi: both nodes removed on completion", fade.nodes().empty());
        record("multi: unrelated id stays at steady state",
               near(fade.getAlpha("save", true), 1.0f));
    }

    // --- 10. Reopen from a close-completion callback ---------------------------
    {
        MenuFadeSystem fade;
        bool reopened = false;
        fade.beginClose("pause", 0.25f, [&fade, &reopened]() {
            reopened = true;
            fade.beginOpen("pause", 0.25f);
        });
        fade.update(0.25f);
        record("reopen: callback ran and started a new node",
               reopened && fade.isTransitioning("pause") && !fade.isClosing("pause"));
        fade.update(0.25f);
        record("reopen: fade settles visible", fade.nodes().empty());
    }

    return getFailCount() == 0;
}
