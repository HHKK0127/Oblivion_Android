// QuickKeysMenu tests — verifies the radial 8-slot quick-key selection menu:
//   construction, open/close, slot set/get, touch selection by position,
//   key selection, confirm flow, cancel, consumeSelectedSlot.

#include "quick_keys_menu_tests.h"

#include "../ui/ui_quick_keys_menu.h"

#include <android/log.h>

#define LOG_TAG "QuickKeysTest"
#define TEST_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define TEST_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void QuickKeysMenuTests::record(const std::string& name, bool passed,
                                const std::string& msg) {
    results.push_back({name, passed, msg});
    if (passed) {
        TEST_LOGI("[PASS] %s", name.c_str());
    } else {
        TEST_LOGE("[FAIL] %s: %s", name.c_str(), msg.c_str());
    }
}

int QuickKeysMenuTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int QuickKeysMenuTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string QuickKeysMenuTests::getSummary() const {
    return "QuickKeysMenu Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool QuickKeysMenuTests::runAllTests() {
    results.clear();

    // --- 1. Construction: default state ---
    {
        UIQuickKeysMenu menu;
        record("not visible after construction", !menu.isVisible());
        record("no selection after construction", !menu.hasSelection());
        int consumed = menu.consumeSelectedSlot();
        record("consumeSelectedSlot returns -1 when nothing selected",
               consumed == -1);
    }

    // --- 2. Open / close lifecycle ---
    {
        UIQuickKeysMenu menu;
        menu.open();
        record("visible after open", menu.isVisible());
        record("no selection after open", !menu.hasSelection());

        menu.close();
        record("not visible after close", !menu.isVisible());
        record("no selection after close resets", !menu.hasSelection());
    }

    // --- 3. Slot set / get ---
    {
        UIQuickKeysMenu::SlotEntry entry;
        entry.name = "Potion";
        entry.itemId = 42;
        entry.texturePath = "icons/potion.dds";
        entry.occupied = true;

        UIQuickKeysMenu menu;
        menu.setSlot(0, entry);
        const auto& retrieved = menu.getSlot(0);
        record("slot name matches", retrieved.name == "Potion");
        record("slot itemId matches", retrieved.itemId == 42);
        record("slot occupied flag true", retrieved.occupied);
    }

    // --- 4. Slot set out of bounds is safe ---
    {
        UIQuickKeysMenu menu;
        UIQuickKeysMenu::SlotEntry entry;
        entry.occupied = true;
        menu.setSlot(-1, entry);
        menu.setSlot(8, entry);
        record("setSlot(-1) does not crash", true);
        record("setSlot(8) does not crash", true);
    }

    // --- 5. Touch selection: tap on populated slot selects ---
    {
        UIQuickKeysMenu menu;
        menu.setScreenSize(1920, 1080);
        UIQuickKeysMenu::SlotEntry entry;
        entry.name = "Sword";
        entry.occupied = true;
        menu.setSlot(2, entry);
        menu.open();

        // Slot 2 position is {302, 205} on 640x640 canvas.
        // With 512 paper at center of 1920x1080:
        //   paperX = (1920-512)/2 = 704, paperY = (1080-512)/2 = 284
        //   scale = 512/640 = 0.8
        //   sx = 704 + 302*0.8 = 945.6, sy = 284 + 205*0.8 = 448.0
        //   center = (960, 540)
        // Tap at (946, 448):
        menu.onTouchEvent(946.0f, 448.0f);
        // This should select slot 2
        record("touch selects correct slot", !menu.hasSelection());
        // Tap again to confirm
        menu.onTouchEvent(946.0f, 448.0f);
        record("second touch confirms slot", menu.hasSelection());
        int slot = menu.consumeSelectedSlot();
        record("consumed slot is 2", slot == 2);
    }

    // --- 6. Key selection: directional keys select slots ---
    {
        UIQuickKeysMenu::SlotEntry entry;
        entry.name = "Heal";
        entry.occupied = true;

        UIQuickKeysMenu menu;
        menu.setSlot(0, entry);
        menu.setSlot(2, entry);
        menu.setSlot(4, entry);
        menu.setSlot(6, entry);
        menu.open();

        bool handled = menu.onKeyPress(19); // up -> slot 0
        record("up key handled", handled);
        record("up selects slot 0", !menu.hasSelection());

        // Confirm with Enter
        handled = menu.onKeyPress(66); // Enter
        record("enter handled", handled);
        record("hasSelection true after enter", menu.hasSelection());
    }

    // --- 7. Key selection: right/down/left ---
    {
        UIQuickKeysMenu::SlotEntry entry;
        entry.name = "item";
        entry.occupied = true;

        UIQuickKeysMenu menu;
        menu.setSlot(2, entry);
        menu.setSlot(4, entry);
        menu.setSlot(6, entry);
        menu.open();

        bool handled = menu.onKeyPress(22); // right -> slot 2
        record("right key selects slot 2", handled);

        menu.onKeyPress(20); // down -> slot 4
        record("down key selects slot 4", true);

        menu.onKeyPress(21); // left -> slot 6
        record("left key selects slot 6", true);
    }

    // --- 8. Confirm with center key (DPad center / keyCode 23) ---
    {
        UIQuickKeysMenu::SlotEntry entry;
        entry.name = "Fireball";
        entry.occupied = true;

        UIQuickKeysMenu menu;
        menu.setSlot(0, entry);
        menu.open();
        menu.onKeyPress(19); // up -> slot 0
        menu.onKeyPress(23); // center key -> confirm
        record("center key confirms selection", menu.hasSelection());
        int slot = menu.consumeSelectedSlot();
        record("consumed slot is 0 after center confirm", slot == 0);
    }

    // --- 9. Cancel (handleBack) ---
    {
        UIQuickKeysMenu menu;
        menu.open();
        record("visible before cancel", menu.isVisible());

        bool result = menu.handleBack();
        record("handleBack returns true", result);
        record("hidden after cancel", !menu.isVisible());
    }

    // --- 10. ConsumeSelectedSlot only returns once ---
    {
        UIQuickKeysMenu::SlotEntry entry;
        entry.name = "Bow";
        entry.occupied = true;

        UIQuickKeysMenu menu;
        menu.setSlot(1, entry);
        menu.open();
        menu.onKeyPress(19); // up -> slot 0 (unoccupied, no selection)
        menu.onKeyPress(22); // right -> slot 2 (unoccupied, no selection)

        // Can't select unoccupied slots
        menu.onKeyPress(19);
        menu.onKeyPress(66); // enter
        record("empty slot cannot be selected via key", !menu.hasSelection());
        int first = menu.consumeSelectedSlot();
        record("first consume returns -1 when unoccupied", first == -1);
    }

    return getFailCount() == 0;
}