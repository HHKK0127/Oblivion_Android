// Menu fade machine implementation - see menu_fade_system.h for the RE 48
// mapping onto the original MenuManager fade node list.

#include "menu_fade_system.h"

#include <algorithm>
#include <android/log.h>

#undef LOG_TAG
#undef LOGI
#undef LOGD
#define LOG_TAG "MenuFade"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace {
const char* phaseName(MenuFadeSystem::Phase phase) {
    return phase == MenuFadeSystem::Phase::OPENING ? "open" : "close";
}
}  // namespace

MenuFadeSystem::Node* MenuFadeSystem::find(const std::string& id) {
    for (auto& node : nodes_) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

const MenuFadeSystem::Node* MenuFadeSystem::find(const std::string& id) const {
    for (const auto& node : nodes_) {
        if (node.id == id) {
            return &node;
        }
    }
    return nullptr;
}

void MenuFadeSystem::begin(const std::string& id, Phase phase, float duration,
                           std::function<void()> onCloseComplete) {
    // FUN_00584300 unlinks an existing node for the same source first; a
    // pending close callback dies with the replaced node.
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
        if (it->id == id) {
            LOGD("Fade replace: id=%s (%s dropped)", id.c_str(), phaseName(it->phase));
            nodes_.erase(it);
            break;
        }
    }

    Node node;
    node.id = id;
    node.phase = phase;
    node.current = 0.0f;
    node.duration = std::max(duration, kMinDuration);
    node.onCloseComplete = std::move(onCloseComplete);
    nodes_.push_back(std::move(node));

    LOGI("Fade begin: id=%s phase=%s duration=%.3f s", id.c_str(), phaseName(phase),
         std::max(duration, kMinDuration));
}

void MenuFadeSystem::beginOpen(const std::string& id, float duration) {
    begin(id, Phase::OPENING, duration, nullptr);
}

void MenuFadeSystem::beginClose(const std::string& id, float duration,
                                std::function<void()> onCloseComplete) {
    begin(id, Phase::CLOSING, duration, std::move(onCloseComplete));
}

float MenuFadeSystem::progressOf(const Node& node) {
    return std::min(node.current / node.duration, 1.0f);
}

void MenuFadeSystem::update(float deltaTime) {
    if (nodes_.empty() || deltaTime <= 0.0f) {
        return;
    }

    // Callbacks are collected and run after the sweep: the original fires the
    // phase transition from the same update, and re-entrancy while iterating
    // would invalidate the node pointers.
    std::vector<std::function<void()>> completions;
    for (auto it = nodes_.begin(); it != nodes_.end();) {
        it->current += deltaTime;
        if (progressOf(*it) >= 1.0f) {
            LOGI("Fade complete: id=%s phase=%s", it->id.c_str(), phaseName(it->phase));
            if (it->phase == Phase::CLOSING && it->onCloseComplete) {
                completions.push_back(std::move(it->onCloseComplete));
            }
            it = nodes_.erase(it);
        } else {
            ++it;
        }
    }

    for (auto& completion : completions) {
        completion();
    }
}

float MenuFadeSystem::getAlpha(const std::string& id, bool steadyVisible) const {
    const Node* node = find(id);
    if (!node) {
        return steadyVisible ? 1.0f : 0.0f;
    }
    const float progress = progressOf(*node);
    return node->phase == Phase::OPENING ? progress : 1.0f - progress;
}

bool MenuFadeSystem::isTransitioning(const std::string& id) const {
    return find(id) != nullptr;
}

bool MenuFadeSystem::isClosing(const std::string& id) const {
    const Node* node = find(id);
    return node != nullptr && node->phase == Phase::CLOSING;
}

void MenuFadeSystem::setDefaultDuration(float seconds) {
    defaultDuration_ = std::max(seconds, kMinDuration);
}
