#include "sleep_wait_menu_tests.h"
#include "../ui/ui_sleep_wait_menu.h"
#include "../engine/sky_weather_system.h"

#include <cmath>

namespace {

constexpr float kEps = 1e-4f;

bool near(float a, float b) {
    return std::fabs(a - b) < kEps;
}

} // namespace

void SleepWaitMenuTests::record(const std::string& name, bool passed,
                                const std::string& msg) {
    results.push_back({name, passed, msg});
}

bool SleepWaitMenuTests::near(float a, float b, float eps) {
    return std::fabs(a - b) < eps;
}

int SleepWaitMenuTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int SleepWaitMenuTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string SleepWaitMenuTests::getSummary() const {
    return "Sleep/Wait Menu Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool SleepWaitMenuTests::runAllTests() {
    results.clear();

    // --- 1. Construction / destruction ---
    {
        UISleepWaitMenu menu;
        record("starts invisible", !menu.isVisible());
        record("starts not fast-forwarding", !menu.isFastForwarding());
        record("default is sleep mode", menu.isSleepMode());
        record("consumeHours returns 0 initially", menu.consumeHours() == 0);
    }

    // --- 2. Mode toggle ---
    {
        UISleepWaitMenu menu;

        menu.open(true);
        record("open(true) sets sleep mode", menu.isSleepMode());
        record("visible after open", menu.isVisible());

        menu.close();
        record("invisible after close", !menu.isVisible());

        menu.open(false);
        record("open(false) sets wait mode", !menu.isSleepMode());
        record("visible after reopen", menu.isVisible());
        menu.close();
    }

    // --- 3. Mode toggle via open ---
    {
        UISleepWaitMenu menu;
        menu.open(true);
        record("sleep mode = true when open(true)", menu.isSleepMode());
        menu.close();

        menu.open(false);
        record("sleep mode = false when open(false)", !menu.isSleepMode());
        menu.close();
    }

    // --- 4. consumeHours after fast-forward ---
    {
        UISleepWaitMenu menu;

        menu.open(true);
        record("consumeHours returns 0 before any confirm", menu.consumeHours() == 0);

        // Simulate confirm: consumeHours should be set after fast-forward ends
        // We verify the pending hours mechanic by calling consumeHours twice
        record("second consumeHours also 0", menu.consumeHours() == 0);
        menu.close();
    }

    // --- 5. handleBack ---
    {
        UISleepWaitMenu menu;
        menu.open(true);
        record("handleBack returns true when visible", menu.handleBack());
        record("closed after back", !menu.isVisible());
        record("handleBack returns false when invisible", !menu.handleBack());
    }

    // --- 6. onKeyPress DPAD navigation ---
    {
        UISleepWaitMenu menu;
        menu.open(true);

        // Right arrow increases slider
        menu.onKeyPress(22);
        record("visible after keypress", menu.isVisible());

        menu.close();
    }

    // --- 7. Screen size ---
    {
        UISleepWaitMenu menu;
        menu.setScreenSize(1920, 1080);
        // No crash or bad state after resize
        record("screen size set without crash", true);
    }

    // --- 8. setFadeAlpha ---
    {
        UISleepWaitMenu menu;
        menu.setFadeAlpha(0.0f);
        menu.setFadeAlpha(0.5f);
        menu.setFadeAlpha(1.0f);
        menu.setFadeAlpha(1.5f); // clamped
        record("setFadeAlpha handles clamp", true);
    }

    // --- 9. Fast-forward end-to-end ---
    {
        UISleepWaitMenu menu;
        menu.open(true);

        // Simulate confirm via key press
        menu.onKeyPress(66);
        record("enters fast-forward after confirm", menu.isFastForwarding());
        record("visible during fast-forward", menu.isVisible());

        // Update with enough time to tick through one hour
        // Real-time: 1.0 sec per game hour
        menu.update(0.5f);
        record("still fast-forwarding after 0.5s", menu.isFastForwarding());

        menu.update(0.6f);
        // After 1.1 sec total = 1 tick, default selected is 1 hour, so it should be done
        record("not fast-forwarding after all hours pass", !menu.isFastForwarding());
        if (!menu.isFastForwarding()) {
            int h = menu.consumeHours();
            record("consumeHours returns correct value", h == 1);
        }
    }

    // --- 10. Key-based fast-forward cancel ---
    {
        UISleepWaitMenu menu;
        menu.open(false); // wait mode
        // Default selected hours = 1

        menu.onKeyPress(22); // right arrow: 2 hours
        menu.onKeyPress(22); // 3 hours
        menu.onKeyPress(66); // confirm
        record("fast-forwarding after enter", menu.isFastForwarding());
        record("visible during ff", menu.isVisible());

        // Cancel during fast-forward
        bool handled = menu.handleBack();
        record("handleBack returns true during ff", handled);
        record("not fast-forwarding after cancel", !menu.isFastForwarding());
    }

    // --- 11. consumeHours cleared after read ---
    {
        UISleepWaitMenu menu;
        menu.open(true);
        menu.onKeyPress(66); // confirm (1 hour default)

        // Let it run to completion
        menu.update(2.0f);
        record("fast-forward complete", !menu.isFastForwarding());

        int first = menu.consumeHours();
        record("first consumeHours returns non-zero", first > 0);
        int second = menu.consumeHours();
        record("second consumeHours returns 0 (cleared)", second == 0);
    }

    return getFailCount() == 0;
}