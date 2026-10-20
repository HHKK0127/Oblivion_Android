#include "ui_menu_stack.h"

#include <algorithm>
#include <android/log.h>

#define LOG_TAG "MenuStack"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

MenuStack::MenuStack() : timestampCounter_(0) {}

MenuStack::~MenuStack() {}

uint64_t MenuStack::nextTimestamp() {
    return ++timestampCounter_;
}

bool MenuStack::push(int menuId, StackingType type, const std::string& name, int layer) {
    if (type == StackingType::DoesNotStack && isOpen(menuId)) {
        LOGW("DoesNotStack menu '%s' (id=%d) already open — rejected", name.c_str(), menuId);
        return false;
    }

    MenuStackEntry entry;
    entry.menuId = menuId;
    entry.type = type;
    entry.name = name;
    entry.depthTimestamp = nextTimestamp();
    entry.layer = layer;

    entries_.push_back(entry);
    LOGD("MenuStack push: '%s' (id=%d, type=%d, layer=%d) — %zu entries",
         name.c_str(), menuId, static_cast<int>(type), layer, entries_.size());
    return true;
}

bool MenuStack::pop(int menuId) {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->menuId == menuId) {
            LOGD("MenuStack pop: '%s' (id=%d)", it->name.c_str(), menuId);
            entries_.erase(it);
            return true;
        }
    }
    LOGW("MenuStack pop: menu id=%d not found", menuId);
    return false;
}

bool MenuStack::isOpen(int menuId) const {
    for (const auto& e : entries_) {
        if (e.menuId == menuId)
            return true;
    }
    return false;
}

const MenuStackEntry* MenuStack::find(int menuId) const {
    for (const auto& e : entries_) {
        if (e.menuId == menuId)
            return &e;
    }
    return nullptr;
}

int MenuStack::activeMenuId() const {
    // Scan from top (back) to bottom (front) — first NoClickPast wins.
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
        if (it->type == StackingType::NoClickPast)
            return it->menuId;
    }
    // No blocking menu — return topmost entry or 0.
    if (!entries_.empty())
        return entries_.back().menuId;
    return 0;
}

int MenuStack::blockingLayer() const {
    // Scan from top (back) to bottom (front); first NoClickPast is the blocker.
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
        if (it->type == StackingType::NoClickPast)
            return it->layer;
    }
    return 0;
}

bool MenuStack::isInputBlocked(int componentLayer) const {
    const int blocker = blockingLayer();
    return blocker > 0 && componentLayer < blocker;
}

void MenuStack::clear() {
    entries_.clear();
    timestampCounter_ = 0;
}