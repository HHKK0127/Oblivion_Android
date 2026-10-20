#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

// Oblivion menu stackingtype values (RE 50.4, XML entities mapped to ints).
// These control how menus stack, overlap, and pass input.
enum class StackingType : int {
    ClickPast = 101,      // input passes through to menus behind
    NoClickPast = 102,    // blocks input behind (default for interactive menus)
    MixedMenu = 103,      // hybrid
    DoesNotStack = 6008,  // only one instance allowed
};

struct MenuStackEntry {
    int menuId;          // e.g. 0x3EB (pause), 0x3F4 (sleep), 0x3EA (inventory)
    StackingType type;   // from XML <stackingtype>
    std::string name;    // debug label
    uint64_t depthTimestamp; // set on open (RE 22.1 step 10)
    int layer;           // UISystem component layer
};

// MenuStack manages the active menu stack and input blocking rules.
// Menus register on open, unregister on close. The stack order determines
// input priority: topmost NoClickPast menu blocks input to menus behind it.
class MenuStack {
public:
    MenuStack();
    ~MenuStack();

    // Register a menu. Returns true if allowed (false for DoesNotStack dupes).
    // Sets depthTimestamp to current time for sort-on-register.
    bool push(int menuId, StackingType type, const std::string& name, int layer);

    // Unregister (close). Returns false if not found.
    bool pop(int menuId);

    // Check if a menu is currently open.
    bool isOpen(int menuId) const;

    // Find entry by id. Returns nullptr if not found.
    const MenuStackEntry* find(int menuId) const;

    // Get the active menu id (topmost NoClickPast or highest layer).
    int activeMenuId() const;

    // True when a menu blocks input from reaching component on given layer.
    bool isInputBlocked(int componentLayer) const;

    // The layer that blocks input (0 if none).
    int blockingLayer() const;

    // List for debug/tests.
    const std::vector<MenuStackEntry>& entries() const { return entries_; }
    size_t size() const { return entries_.size(); }

    // Clear all entries.
    void clear();

private:
    std::vector<MenuStackEntry> entries_;
    uint64_t timestampCounter_;

    uint64_t nextTimestamp();
};