// MenuStack tests — verifies the menu stackingtype system:
//   push/pop lifecycle, DoesNotStack duplicate rejection,
//   NoClickPast input blocking, activeMenuId resolution.

#include "menu_stack_tests.h"

#include "../ui/ui_menu_stack.h"

#include <android/log.h>

#define LOG_TAG "MenuStackTest"
#define TEST_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define TEST_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void MenuStackTests::record(const std::string& name, bool passed,
                            const std::string& msg) {
    results.push_back({name, passed, msg});
    if (passed) {
        TEST_LOGI("[PASS] %s", name.c_str());
    } else {
        TEST_LOGE("[FAIL] %s: %s", name.c_str(), msg.c_str());
    }
}

int MenuStackTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int MenuStackTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string MenuStackTests::getSummary() const {
    return "MenuStack Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool MenuStackTests::runAllTests() {
    results.clear();

    // --- 1. Push/pop lifecycle ---
    {
        MenuStack stack;
        bool pushed = stack.push(1001, StackingType::NoClickPast, "pause", 5);
        record("push returns true", pushed);
        record("size is 1 after push", stack.size() == 1);
        record("isOpen true after push", stack.isOpen(1001));

        bool popped = stack.pop(1001);
        record("pop returns true", popped);
        record("size is 0 after pop", stack.size() == 0);
        record("isOpen false after pop", !stack.isOpen(1001));

        bool popMissing = stack.pop(9999);
        record("pop missing id returns false", !popMissing);
    }

    // --- 2. DoesNotStack duplicate rejection ---
    {
        MenuStack stack;
        bool first = stack.push(2001, StackingType::DoesNotStack, "hud_main", 10);
        record("first DoesNotStack push succeeds", first);

        bool second = stack.push(2001, StackingType::DoesNotStack, "hud_main", 10);
        record("duplicate DoesNotStack push rejected", !second);

        record("size remains 1 after rejected duplicate", stack.size() == 1);
    }

    // --- 3. Non-DoesNotStack allows duplicates ---
    {
        MenuStack stack;
        bool first = stack.push(3001, StackingType::ClickPast, "click_menu", 5);
        bool second = stack.push(3001, StackingType::ClickPast, "click_menu", 5);
        record("ClickPast duplicate allowed", first && second);
        record("size is 2 after duplicate ClickPast", stack.size() == 2);
    }

    // --- 4. isInputBlocked: empty stack blocks nothing ---
    {
        MenuStack stack;
        record("empty stack: layer 0 not blocked", !stack.isInputBlocked(0));
        record("empty stack: layer 5 not blocked", !stack.isInputBlocked(5));
        record("empty stack: layer 10 not blocked", !stack.isInputBlocked(10));
        record("empty stack: blockingLayer is 0", stack.blockingLayer() == 0);
        record("empty stack: activeMenuId is 0", stack.activeMenuId() == 0);
    }

    // --- 5. isInputBlocked: NoClickPast blocks lower layers ---
    {
        MenuStack stack;
        // Register a ClickPast menu at layer 3, then a NoClickPast at layer 5.
        stack.push(4001, StackingType::ClickPast, "background", 3);
        stack.push(4002, StackingType::NoClickPast, "pause_menu", 5);

        // Layer 5 (same as blocker) is NOT blocked.
        record("layer 5 not blocked", !stack.isInputBlocked(5));
        // Layer 4 is behind the blocker → blocked.
        record("layer 4 is blocked", stack.isInputBlocked(4));
        // Layer 3 is behind → blocked.
        record("layer 3 is blocked", stack.isInputBlocked(3));
        // Layer 0 is behind → blocked.
        record("layer 0 is blocked", stack.isInputBlocked(0));

        record("blockingLayer returns 5", stack.blockingLayer() == 5);
    }

    // --- 6. isInputBlocked: ClickPast does not block ---
    {
        MenuStack stack;
        stack.push(5001, StackingType::ClickPast, "click_menu", 5);
        record("ClickPast: layer 0 not blocked", !stack.isInputBlocked(0));
        record("ClickPast: layer 3 not blocked", !stack.isInputBlocked(3));
        record("ClickPast: layer 5 not blocked", !stack.isInputBlocked(5));
        record("ClickPast: blockingLayer is 0", stack.blockingLayer() == 0);
    }

    // --- 7. isInputBlocked: MixedMenu does not block ---
    {
        MenuStack stack;
        stack.push(6001, StackingType::MixedMenu, "mixed", 5);
        record("MixedMenu: layer 0 not blocked", !stack.isInputBlocked(0));
        record("MixedMenu: blockingLayer is 0", stack.blockingLayer() == 0);
    }

    // --- 8. activeMenuId: returns topmost NoClickPast ---
    {
        MenuStack stack;
        stack.push(7001, StackingType::ClickPast, "click", 3);
        record("activeMenuId with ClickPast only: topmost id",
               stack.activeMenuId() == 7001);

        stack.push(7002, StackingType::NoClickPast, "blocker1", 5);
        record("activeMenuId with NoClickPast on top: blocker1",
               stack.activeMenuId() == 7002);

        stack.push(7003, StackingType::NoClickPast, "blocker2", 7);
        record("activeMenuId with two NoClickPast: topmost blocker2",
               stack.activeMenuId() == 7003);

        stack.pop(7003);
        record("activeMenuId after pop: blocker1",
               stack.activeMenuId() == 7002);
    }

    // --- 9. Blocking survives menu order changes ---
    {
        MenuStack stack;
        // Add block at layer 5, then click-through at layer 7.
        stack.push(8001, StackingType::NoClickPast, "pause_menu", 5);
        stack.push(8002, StackingType::ClickPast, "click_overlay", 7);

        // ClickPast on top does NOT change the blocker.
        record("ClickPast above NoClickPast: layer 3 still blocked",
               stack.isInputBlocked(3));
        record("ClickPast above NoClickPast: layer 5 not blocked",
               !stack.isInputBlocked(5));
        record("ClickPast above NoClickPast: layer 7 not blocked",
               !stack.isInputBlocked(7));
        record("blockingLayer still 5", stack.blockingLayer() == 5);

        // activeMenuId should still be the NoClickPast.
        record("activeMenuId is pause_menu (NoClickPast) behind ClickPast",
               stack.activeMenuId() == 8001);
    }

    // --- 10. find() lookup ---
    {
        MenuStack stack;
        stack.push(9001, StackingType::NoClickPast, "pause", 5);
        stack.push(9002, StackingType::ClickPast, "hud", 10);

        const MenuStackEntry* foundPause = stack.find(9001);
        record("find exists for pause", foundPause != nullptr);
        record("find returns correct name", foundPause != nullptr && foundPause->name == "pause");

        const MenuStackEntry* foundMissing = stack.find(9999);
        record("find returns null for missing id", foundMissing == nullptr);
    }

    // --- 11. clear() resets state ---
    {
        MenuStack stack;
        stack.push(10001, StackingType::NoClickPast, "menu", 5);
        stack.push(10002, StackingType::ClickPast, "other", 3);
        record("size 2 before clear", stack.size() == 2);

        stack.clear();
        record("size 0 after clear", stack.size() == 0);
        record("no blocking after clear", !stack.isInputBlocked(0));
        record("activeMenuId 0 after clear", stack.activeMenuId() == 0);
    }

    // --- 12. Timestamps are monotonically increasing ---
    {
        MenuStack stack;
        stack.push(11001, StackingType::ClickPast, "first", 1);
        stack.push(11002, StackingType::NoClickPast, "second", 2);
        stack.push(11003, StackingType::MixedMenu, "third", 3);

        const auto& entries = stack.entries();
        bool increasing = entries.size() == 3
                       && entries[0].depthTimestamp < entries[1].depthTimestamp
                       && entries[1].depthTimestamp < entries[2].depthTimestamp;
        record("timestamps monotonically increase", increasing);
    }

    return getFailCount() == 0;
}