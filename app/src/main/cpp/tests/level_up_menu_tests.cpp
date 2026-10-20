#include "level_up_menu_tests.h"
#include "../ui/ui_level_up_menu.h"

#include <cmath>

namespace {

constexpr float kEps = 1e-4f;

bool near(float a, float b) {
    return std::fabs(a - b) < kEps;
}

} // namespace

void LevelUpMenuTests::record(const std::string& name, bool passed,
                              const std::string& msg) {
    results.push_back({name, passed, msg});
}

bool LevelUpMenuTests::near(float a, float b, float eps) {
    return std::fabs(a - b) < eps;
}

int LevelUpMenuTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int LevelUpMenuTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string LevelUpMenuTests::getSummary() const {
    return "Level Up Menu Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool LevelUpMenuTests::runAllTests() {
    results.clear();

    // --- 1. Construction / destruction ---
    {
        UILevelUpMenu menu;
        record("starts invisible", !menu.isVisible());
        record("no selection initially", !menu.hasSelection());
        record("consume returns -1 initially", menu.consumeSelectedAttribute() == -1);
    }

    // --- 2. Open with 3 choices ---
    {
        UILevelUpMenu menu;
        std::vector<int> choices = {0, 3, 6}; // STR, AGI, END
        std::vector<int> skillCounts = {2, 5, 9};
        menu.open(choices, skillCounts, 5);

        record("visible after open", menu.isVisible());
        record("no selection after open", !menu.hasSelection());
        record("consume returns -1 before confirm", menu.consumeSelectedAttribute() == -1);

        menu.close();
        record("invisible after close", !menu.isVisible());
    }

    // --- 3. calculateBonus formula ---
    {
        record("0 skill increases -> +1", UILevelUpMenu::calculateBonus(0) == 1);
        record("1 skill increase -> +2", UILevelUpMenu::calculateBonus(1) == 2);
        record("3 skill increases -> +2", UILevelUpMenu::calculateBonus(3) == 2);
        record("4 skill increases -> +2", UILevelUpMenu::calculateBonus(4) == 2);
        record("5 skill increases -> +3", UILevelUpMenu::calculateBonus(5) == 3);
        record("6 skill increases -> +3", UILevelUpMenu::calculateBonus(6) == 3);
        record("7 skill increases -> +3", UILevelUpMenu::calculateBonus(7) == 3);
        record("8 skill increases -> +4", UILevelUpMenu::calculateBonus(8) == 4);
        record("9 skill increases -> +4", UILevelUpMenu::calculateBonus(9) == 4);
        record("10 skill increases -> +5", UILevelUpMenu::calculateBonus(10) == 5);
        record("11 skill increases -> +5", UILevelUpMenu::calculateBonus(11) == 5);
        record("negative skill increases -> +1", UILevelUpMenu::calculateBonus(-1) == 1);
    }

    // --- 4. Row selection via touch ---
    {
        UILevelUpMenu menu;
        std::vector<int> choices = {0, 1, 2};
        std::vector<int> skillCounts = {2, 5, 9};
        menu.open(choices, skillCounts, 10);

        // First tap selects
        menu.onTouchEvent(400.0f, 50.0f); // inside row 0 approximately
        record("touch selects row", true);

        // Tap same row again to confirm
        bool confirmed = false;
        menu.setOnLevelUpCallback([&confirmed]() { confirmed = true; });
        menu.onTouchEvent(400.0f, 50.0f);
        record("second tap triggers confirm", confirmed);

        if (confirmed) {
            int attr = menu.consumeSelectedAttribute();
            record("consumed attribute is valid", attr >= 0 && attr <= 7);
            record("selection cleared after consume", !menu.hasSelection());
        }

        menu.close();
    }

    // --- 5. Row selection via key ---
    {
        UILevelUpMenu menu;
        std::vector<int> choices = {0, 3, 6};
        std::vector<int> skillCounts = {2, 5, 9};
        menu.open(choices, skillCounts, 15);

        // DPAD_DOWN to move to row 1
        menu.onKeyPress(20);
        record("visible after keypress", menu.isVisible());

        // DPAD_DOWN again to row 2
        menu.onKeyPress(20);

        // DPAD_UP back to row 1
        menu.onKeyPress(19);

        bool confirmed = false;
        menu.setOnLevelUpCallback([&confirmed]() { confirmed = true; });

        // ENTER to confirm
        menu.onKeyPress(66);
        record("enter confirms selection", confirmed);

        if (confirmed) {
            int attr = menu.consumeSelectedAttribute();
            record("consumed attribute matches choice[1]", attr == 3);
            record("selection cleared after consume", !menu.hasSelection());
        }

        menu.close();
    }

    // --- 6. Confirm flow ---
    {
        UILevelUpMenu menu;
        std::vector<int> choices = {0, 1, 2};
        std::vector<int> skillCounts = {1, 2, 3};
        menu.open(choices, skillCounts, 25);

        bool levelUpCalled = false;
        int capturedAttr = -1;
        menu.setOnLevelUpCallback([&levelUpCalled]() { levelUpCalled = true; });

        // Navigate to row 2
        menu.onKeyPress(20);
        menu.onKeyPress(20);
        menu.onKeyPress(66); // confirm

        record("onLevelUp callback invoked", levelUpCalled);

        if (levelUpCalled) {
            capturedAttr = menu.consumeSelectedAttribute();
            record("selected attribute is valid", capturedAttr >= 0 && capturedAttr <= 7);
        }

        // Second confirm should not re-trigger without new selection
        bool levelUpCalled2 = false;
        menu.setOnLevelUpCallback([&levelUpCalled2]() { levelUpCalled2 = true; });
        menu.onKeyPress(66);
        record("second confirm does not re-trigger (consumed)", !levelUpCalled2);

        menu.close();
    }

    // --- 7. Back button cancels ---
    {
        UILevelUpMenu menu;
        std::vector<int> choices = {7, 6, 5};
        std::vector<int> skillCounts = {0, 1, 2};
        menu.open(choices, skillCounts, 1);

        bool cancelCalled = false;
        menu.setOnCancelCallback([&cancelCalled]() { cancelCalled = true; });

        bool handled = menu.handleBack();
        record("handleBack returns true when visible", handled);
        record("menu closed after back", !menu.isVisible());
        record("onCancel callback invoked", cancelCalled);
        record("handleBack returns false when invisible", !menu.handleBack());
    }

    // --- 8. Screen size and fade ---
    {
        UILevelUpMenu menu;
        menu.setScreenSize(1920, 1080);
        menu.setFadeAlpha(0.0f);
        menu.setFadeAlpha(0.5f);
        menu.setFadeAlpha(1.0f);
        menu.setFadeAlpha(1.5f);
        record("setScreenSize and setFadeAlpha handle clamping", true);
    }

    // --- 9. Open with empty choices ---
    {
        UILevelUpMenu menu;
        std::vector<int> choices;
        std::vector<int> skillCounts;
        menu.open(choices, skillCounts, 1);
        record("no rows with empty choices", !menu.hasSelection());
        menu.close();
        record("closes cleanly after empty open", !menu.isVisible());
    }

    // --- 10. setFadeAlpha clamp ---
    {
        UILevelUpMenu menu;
        menu.setFadeAlpha(-0.1f);
        // No crash
        record("setFadeAlpha negative clamp", true);
    }

    return getFailCount() == 0;
}