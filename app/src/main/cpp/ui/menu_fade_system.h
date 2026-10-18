#pragma once

#include <functional>
#include <string>
#include <vector>

/**
 * @brief Per-menu open/close fade, ported from the original MenuManager fade list.
 *
 * RE 48 (docs/RE_EXE_REPORT.md 48.3 / 49.6): the original keeps one fade node
 * per animating menu in a list at MenuManager+0x130. A node holds
 * source (menu id), current, duration and prev/next. The open path
 * (FUN_00584740, phase 2) and the close path (FUN_00584390, phase 8) both
 * create the node through FUN_00584300, which first unlinks any node for the
 * same source and clamps duration to a 0.001 s floor (49.6.1).
 *
 * FUN_00583C30 / FUN_00583f40 advance every node once per frame:
 *     current += dt;  progress = current / duration
 * and unlink the node in the same frame progress reaches 1.0 (49.6.4).
 * Playback is linear and one-shot: a close never rewinds an open node, it
 * appends a fresh one (49.6.5). The duration is the `menufade` trait
 * (0.25 s default) with `explorefade` as the fallback when menufade is 0
 * (49.6.2 / 49.6.3).
 *
 * This class is renderer-agnostic: callers poll getAlpha() per frame while the
 * node animates and pass a callback to beginClose() when they need to act once
 * the close fade has fully played out.
 */
class MenuFadeSystem {
public:
    enum class Phase {
        OPENING,  // original [+0x24] == 2
        CLOSING,  // original [+0x24] == 8
    };

    struct Node {
        std::string id;  // original "source" menu id
        Phase phase;
        float current = 0.0f;    // elapsed fade time
        float duration = 0.25f;  // clamped to kMinDuration
        std::function<void()> onCloseComplete;
    };

    // XML defaults: menufade 0.25 s; the node constructor clamps to 0.001 s.
    static constexpr float kDefaultDuration = 0.25f;
    static constexpr float kMinDuration = 0.001f;

    // Starts (or restarts) the open fade for id. Replacing a node in the other
    // phase is the original's cross-fade: the menu keeps its visible state and
    // the new fade starts from 0.
    void beginOpen(const std::string& id, float duration);
    void beginOpen(const std::string& id) { beginOpen(id, defaultDuration_); }

    // Starts (or restarts) the close fade for id. onCloseComplete runs once,
    // after the node has been removed, so the callback may safely start new
    // fades. Replacing a pending close drops its callback unfired.
    void beginClose(const std::string& id, float duration,
                    std::function<void()> onCloseComplete = nullptr);
    void beginClose(const std::string& id,
                    std::function<void()> onCloseComplete = nullptr) {
        beginClose(id, defaultDuration_, std::move(onCloseComplete));
    }

    // Advances every node by deltaTime, removes the finished ones and then
    // fires their close callbacks.
    void update(float deltaTime);

    // Fade alpha for id: OPENING -> progress (0..1), CLOSING -> 1 - progress.
    // With no node the steady state wins: visible -> 1, hidden -> 0.
    float getAlpha(const std::string& id, bool steadyVisible) const;

    bool isTransitioning(const std::string& id) const;
    bool isAnyTransitioning() const { return !nodes_.empty(); }
    bool isClosing(const std::string& id) const;

    float defaultDuration() const { return defaultDuration_; }
    void setDefaultDuration(float seconds);

    void clear() { nodes_.clear(); }

    const std::vector<Node>& nodes() const { return nodes_; }

private:
    Node* find(const std::string& id);
    const Node* find(const std::string& id) const;
    void begin(const std::string& id, Phase phase, float duration,
               std::function<void()> onCloseComplete);
    static float progressOf(const Node& node);

    std::vector<Node> nodes_;
    float defaultDuration_ = kDefaultDuration;
};
