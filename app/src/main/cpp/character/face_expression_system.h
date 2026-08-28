#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <algorithm>
#include <glm/glm.hpp>
#include <android/log.h>

// ============================================================================
// Phase 55: Face Expression System
//
// Facial expression animation for NPCs: preset expressions, smooth blending,
// priority-based overrides (combat vs idle), a layered expression stack, and
// a lip sync / viseme pipeline for dialogue playback.
// ============================================================================

#define LOG_TAG_FES "FaceExpressionSystem"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_FES(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_FES, __VA_ARGS__)
#else
#define LOGD_FES(...) do {} while(0)
#endif
#define LOGI_FES(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_FES, __VA_ARGS__)
#define LOGW_FES(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_FES, __VA_ARGS__)
#define LOGE_FES(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_FES, __VA_ARGS__)

namespace facegen {

// ----------------------------------------------------------------------------
// Expression presets
// ----------------------------------------------------------------------------
enum class ExpressionType : uint8_t {
    NEUTRAL = 0,
    HAPPY,
    SAD,
    ANGRY,
    FEARFUL,
    DISGUSTED,
    SURPRISED,
    COUNT
};

inline const char* ExpressionTypeToString(ExpressionType type) {
    switch (type) {
        case ExpressionType::NEUTRAL:   return "NEUTRAL";
        case ExpressionType::HAPPY:     return "HAPPY";
        case ExpressionType::SAD:       return "SAD";
        case ExpressionType::ANGRY:     return "ANGRY";
        case ExpressionType::FEARFUL:   return "FEARFUL";
        case ExpressionType::DISGUSTED: return "DISGUSTED";
        case ExpressionType::SURPRISED: return "SURPRISED";
        default:                        return "UNKNOWN";
    }
}

// Priority used when arbitrating between concurrently requested expressions.
// Higher value wins (e.g. combat pain/rage overrides idle chatter).
enum class ExpressionPriority : uint8_t {
    IDLE = 0,
    DIALOGUE = 1,
    EMOTION = 2,
    COMBAT = 3,
    SCRIPTED = 4, // cinematic / quest-forced expression, highest priority
};

// Number of morph target channels driven by the expression system. This is
// intentionally decoupled from FaceGenMorpher's full morph target list; only
// the face-muscle-relevant subset is driven here (brows, eyes, mouth, jaw).
static constexpr int kExpressionMorphChannelCount = 24;

enum ExpressionMorphChannel : int {
    MORPH_BROW_INNER_UP = 0,
    MORPH_BROW_OUTER_UP,
    MORPH_BROW_DOWN,
    MORPH_BROW_SQUEEZE,
    MORPH_EYE_WIDE,
    MORPH_EYE_SQUINT,
    MORPH_EYE_CLOSE,
    MORPH_CHEEK_RAISE,
    MORPH_CHEEK_PUFF,
    MORPH_NOSE_SNEER,
    MORPH_NOSE_FLARE,
    MORPH_MOUTH_SMILE,
    MORPH_MOUTH_FROWN,
    MORPH_MOUTH_OPEN,
    MORPH_MOUTH_WIDE,
    MORPH_MOUTH_PUCKER,
    MORPH_MOUTH_PRESS,
    MORPH_LIP_UPPER_UP,
    MORPH_LIP_LOWER_DOWN,
    MORPH_LIP_CORNER_UP,
    MORPH_LIP_CORNER_DOWN,
    MORPH_JAW_OPEN,
    MORPH_JAW_LEFT,
    MORPH_JAW_RIGHT,
};

using MorphWeights = std::array<float, kExpressionMorphChannelCount>;

inline MorphWeights ZeroWeights() {
    MorphWeights w{};
    w.fill(0.0f);
    return w;
}

// ----------------------------------------------------------------------------
// Expression preset table: each entry defines the target morph weights for a
// fully-activated (weight = 1.0) expression.
// ----------------------------------------------------------------------------
class ExpressionPresetTable {
public:
    static const MorphWeights& Get(ExpressionType type) {
        static const std::array<MorphWeights, static_cast<size_t>(ExpressionType::COUNT)> table = BuildTable();
        size_t idx = static_cast<size_t>(type);
        if (idx >= table.size()) {
            idx = 0;
        }
        return table[idx];
    }

private:
    static std::array<MorphWeights, static_cast<size_t>(ExpressionType::COUNT)> BuildTable() {
        std::array<MorphWeights, static_cast<size_t>(ExpressionType::COUNT)> t;
        for (auto& w : t) {
            w = ZeroWeights();
        }

        // NEUTRAL: all zero.

        // HAPPY
        {
            auto& w = t[static_cast<size_t>(ExpressionType::HAPPY)];
            w[MORPH_MOUTH_SMILE] = 0.9f;
            w[MORPH_LIP_CORNER_UP] = 0.8f;
            w[MORPH_CHEEK_RAISE] = 0.6f;
            w[MORPH_EYE_SQUINT] = 0.3f;
        }

        // SAD
        {
            auto& w = t[static_cast<size_t>(ExpressionType::SAD)];
            w[MORPH_MOUTH_FROWN] = 0.7f;
            w[MORPH_LIP_CORNER_DOWN] = 0.8f;
            w[MORPH_BROW_INNER_UP] = 0.6f;
            w[MORPH_EYE_CLOSE] = 0.2f;
        }

        // ANGRY
        {
            auto& w = t[static_cast<size_t>(ExpressionType::ANGRY)];
            w[MORPH_BROW_DOWN] = 0.9f;
            w[MORPH_BROW_SQUEEZE] = 0.8f;
            w[MORPH_NOSE_SNEER] = 0.4f;
            w[MORPH_MOUTH_PRESS] = 0.5f;
            w[MORPH_EYE_SQUINT] = 0.4f;
        }

        // FEARFUL
        {
            auto& w = t[static_cast<size_t>(ExpressionType::FEARFUL)];
            w[MORPH_BROW_INNER_UP] = 0.8f;
            w[MORPH_BROW_OUTER_UP] = 0.5f;
            w[MORPH_EYE_WIDE] = 0.9f;
            w[MORPH_MOUTH_OPEN] = 0.3f;
            w[MORPH_MOUTH_WIDE] = 0.4f;
        }

        // DISGUSTED
        {
            auto& w = t[static_cast<size_t>(ExpressionType::DISGUSTED)];
            w[MORPH_NOSE_SNEER] = 0.9f;
            w[MORPH_NOSE_FLARE] = 0.5f;
            w[MORPH_LIP_UPPER_UP] = 0.6f;
            w[MORPH_CHEEK_RAISE] = 0.3f;
            w[MORPH_EYE_SQUINT] = 0.3f;
        }

        // SURPRISED
        {
            auto& w = t[static_cast<size_t>(ExpressionType::SURPRISED)];
            w[MORPH_BROW_INNER_UP] = 0.9f;
            w[MORPH_BROW_OUTER_UP] = 0.9f;
            w[MORPH_EYE_WIDE] = 0.8f;
            w[MORPH_JAW_OPEN] = 0.5f;
            w[MORPH_MOUTH_OPEN] = 0.6f;
        }

        return t;
    }
};

// ----------------------------------------------------------------------------
// Phoneme set for lip sync. Grouped by shared mouth shape where reasonable
// (e.g. M/B/P share the same closed-lip viseme).
// ----------------------------------------------------------------------------
enum class Phoneme : uint8_t {
    SIL = 0, // silence / rest
    AA,
    AE,
    AH,
    AO,
    EH,
    ER,
    IH,
    IY,
    OH,
    OU,
    W,
    FV,   // F, V
    L,
    MBP,  // M, B, P
    SZ,   // S, Z
    TDN,  // T, D, N
    COUNT
};

// A viseme is the visual (mouth-shape) target for a phoneme, expressed as
// weights over the mouth/jaw/lip morph channels.
struct Viseme {
    MorphWeights weights = ZeroWeights();
};

class VisemeTable {
public:
    static const MorphWeights& Get(Phoneme p) {
        static const std::array<MorphWeights, static_cast<size_t>(Phoneme::COUNT)> table = BuildTable();
        size_t idx = static_cast<size_t>(p);
        if (idx >= table.size()) {
            idx = 0;
        }
        return table[idx];
    }

private:
    static std::array<MorphWeights, static_cast<size_t>(Phoneme::COUNT)> BuildTable() {
        std::array<MorphWeights, static_cast<size_t>(Phoneme::COUNT)> t;
        for (auto& w : t) {
            w = ZeroWeights();
        }

        auto set = [&](Phoneme p, std::initializer_list<std::pair<int, float>> vals) {
            auto& w = t[static_cast<size_t>(p)];
            for (auto& kv : vals) {
                w[kv.first] = kv.second;
            }
        };

        set(Phoneme::AA, {{MORPH_JAW_OPEN, 0.7f}, {MORPH_MOUTH_OPEN, 0.8f}});
        set(Phoneme::AE, {{MORPH_JAW_OPEN, 0.5f}, {MORPH_MOUTH_WIDE, 0.5f}});
        set(Phoneme::AH, {{MORPH_JAW_OPEN, 0.4f}, {MORPH_MOUTH_OPEN, 0.4f}});
        set(Phoneme::AO, {{MORPH_JAW_OPEN, 0.6f}, {MORPH_MOUTH_PUCKER, 0.4f}});
        set(Phoneme::EH, {{MORPH_JAW_OPEN, 0.3f}, {MORPH_MOUTH_WIDE, 0.3f}});
        set(Phoneme::ER, {{MORPH_JAW_OPEN, 0.25f}, {MORPH_MOUTH_PUCKER, 0.2f}});
        set(Phoneme::IH, {{MORPH_MOUTH_WIDE, 0.3f}, {MORPH_JAW_OPEN, 0.15f}});
        set(Phoneme::IY, {{MORPH_MOUTH_WIDE, 0.6f}, {MORPH_LIP_CORNER_UP, 0.2f}});
        set(Phoneme::OH, {{MORPH_MOUTH_PUCKER, 0.7f}, {MORPH_JAW_OPEN, 0.3f}});
        set(Phoneme::OU, {{MORPH_MOUTH_PUCKER, 0.9f}});
        set(Phoneme::W,  {{MORPH_MOUTH_PUCKER, 0.8f}});
        set(Phoneme::FV, {{MORPH_LIP_UPPER_UP, 0.2f}, {MORPH_LIP_LOWER_DOWN, 0.2f}});
        set(Phoneme::L,  {{MORPH_JAW_OPEN, 0.3f}, {MORPH_MOUTH_WIDE, 0.2f}});
        set(Phoneme::MBP, {{MORPH_MOUTH_PRESS, 0.9f}});
        set(Phoneme::SZ, {{MORPH_MOUTH_WIDE, 0.2f}, {MORPH_JAW_OPEN, 0.05f}});
        set(Phoneme::TDN, {{MORPH_JAW_OPEN, 0.15f}, {MORPH_MOUTH_WIDE, 0.1f}});
        // SIL stays all-zero.

        return t;
    }
};

// A single timed entry in a viseme timeline for dialogue playback.
struct VisemeKey {
    float timeSeconds = 0.0f;
    Phoneme phoneme = Phoneme::SIL;
};

class VisemeTimeline {
public:
    void Clear() { keys_.clear(); duration_ = 0.0f; }

    void AddKey(float timeSeconds, Phoneme phoneme) {
        keys_.push_back({timeSeconds, phoneme});
        duration_ = std::max(duration_, timeSeconds);
    }

    void Finalize() {
        std::sort(keys_.begin(), keys_.end(),
                  [](const VisemeKey& a, const VisemeKey& b) { return a.timeSeconds < b.timeSeconds; });
    }

    float GetDuration() const { return duration_; }
    bool Empty() const { return keys_.empty(); }

    // Samples the timeline at a given playback time, linearly blending
    // between the two closest keys so lip movement is smooth.
    MorphWeights Sample(float timeSeconds) const {
        if (keys_.empty()) {
            return ZeroWeights();
        }
        if (timeSeconds <= keys_.front().timeSeconds) {
            return VisemeTable::Get(keys_.front().phoneme);
        }
        if (timeSeconds >= keys_.back().timeSeconds) {
            return VisemeTable::Get(keys_.back().phoneme);
        }

        for (size_t i = 0; i + 1 < keys_.size(); ++i) {
            const VisemeKey& a = keys_[i];
            const VisemeKey& b = keys_[i + 1];
            if (timeSeconds >= a.timeSeconds && timeSeconds <= b.timeSeconds) {
                float span = b.timeSeconds - a.timeSeconds;
                float t = span > 1e-5f ? (timeSeconds - a.timeSeconds) / span : 0.0f;
                const MorphWeights& wa = VisemeTable::Get(a.phoneme);
                const MorphWeights& wb = VisemeTable::Get(b.phoneme);
                MorphWeights out = ZeroWeights();
                for (int c = 0; c < kExpressionMorphChannelCount; ++c) {
                    out[c] = wa[c] + (wb[c] - wa[c]) * t;
                }
                return out;
            }
        }
        return ZeroWeights();
    }

private:
    std::vector<VisemeKey> keys_;
    float duration_ = 0.0f;
};

// ----------------------------------------------------------------------------
// A single expression layer request (used for the expression stack).
// Layers are composited additively on top of the base expression, in order
// of priority; higher-priority layers can also fully override the base.
// ----------------------------------------------------------------------------
struct ExpressionLayer {
    std::string name;
    ExpressionType type = ExpressionType::NEUTRAL;
    ExpressionPriority priority = ExpressionPriority::IDLE;
    float intensity = 1.0f;  // 0..1 scale applied to the preset weights
    bool additive = true;    // additive layer vs full override
    bool active = true;
};

// ----------------------------------------------------------------------------
// FaceExpressionSystem
//
// Owns per-NPC expression state: current blended morph weights, the target
// expression, the layer stack, and (optionally) an active viseme timeline for
// dialogue lip sync. Call Update() once per frame with deltaTime.
// ----------------------------------------------------------------------------
class FaceExpressionSystem {
public:
    FaceExpressionSystem() {
        currentWeights_ = ZeroWeights();
        targetWeights_ = ZeroWeights();
    }

    // Requests a base expression change. If the requested priority is lower
    // than the currently active base priority and the base hasn't expired,
    // the request is ignored (combat/scripted expressions win over idle).
    void SetBaseExpression(ExpressionType type, ExpressionPriority priority, float blendSpeed = 4.0f) {
        if (priority < basePriority_ && baseHoldRemaining_ > 0.0f) {
            LOGD_FES("SetBaseExpression ignored: %s (priority too low)",
                     ExpressionTypeToString(type));
            return;
        }
        baseType_ = type;
        basePriority_ = priority;
        blendSpeed_ = std::max(0.01f, blendSpeed);
        baseHoldRemaining_ = 0.0f; // no auto-expiry unless HoldBaseExpression is used
    }

    // Holds the current base expression at the given priority for a duration,
    // preventing lower-priority requests from interrupting it (e.g. a combat
    // grimace holds for the duration of a hit reaction).
    void HoldBaseExpression(float seconds) {
        baseHoldRemaining_ = std::max(baseHoldRemaining_, seconds);
    }

    // Adds or updates a named modifier layer (e.g. "squint_sunlight",
    // "pain_flinch"). Layers persist until explicitly removed.
    void SetLayer(const std::string& name, ExpressionType type, ExpressionPriority priority,
                  float intensity = 1.0f, bool additive = true) {
        for (auto& layer : layers_) {
            if (layer.name == name) {
                layer.type = type;
                layer.priority = priority;
                layer.intensity = intensity;
                layer.additive = additive;
                layer.active = true;
                return;
            }
        }
        ExpressionLayer layer;
        layer.name = name;
        layer.type = type;
        layer.priority = priority;
        layer.intensity = intensity;
        layer.additive = additive;
        layer.active = true;
        layers_.push_back(layer);
    }

    void RemoveLayer(const std::string& name) {
        layers_.erase(std::remove_if(layers_.begin(), layers_.end(),
                                      [&](const ExpressionLayer& l) { return l.name == name; }),
                      layers_.end());
    }

    void ClearLayers() { layers_.clear(); }

    // Starts dialogue lip sync playback using the given viseme timeline.
    // The lip-sync-driven mouth channels take priority over any expression's
    // mouth weights while playing, but brow/eye channels remain unaffected.
    void PlayViseme(std::shared_ptr<VisemeTimeline> timeline) {
        std::lock_guard<std::mutex> lock(mutex_);
        activeTimeline_ = std::move(timeline);
        visemeTime_ = 0.0f;
        visemePlaying_ = (activeTimeline_ != nullptr) && !activeTimeline_->Empty();
    }

    void StopViseme() {
        std::lock_guard<std::mutex> lock(mutex_);
        visemePlaying_ = false;
        activeTimeline_.reset();
    }

    bool IsVisemePlaying() const { return visemePlaying_; }

    // Advances blending and viseme playback. Returns the composed morph
    // weights to feed into the mesh morph pass.
    const MorphWeights& Update(float deltaTime) {
        if (baseHoldRemaining_ > 0.0f) {
            baseHoldRemaining_ = std::max(0.0f, baseHoldRemaining_ - deltaTime);
        }

        // 1. Compose target weights: base expression + active additive layers,
        //    with any full-override layer of highest priority taking over.
        MorphWeights composed = ExpressionPresetTable::Get(baseType_);

        const ExpressionLayer* overrideLayer = nullptr;
        for (const auto& layer : layers_) {
            if (!layer.active) continue;
            if (!layer.additive) {
                if (overrideLayer == nullptr || layer.priority > overrideLayer->priority) {
                    overrideLayer = &layer;
                }
            }
        }

        if (overrideLayer != nullptr) {
            const MorphWeights& ow = ExpressionPresetTable::Get(overrideLayer->type);
            for (int c = 0; c < kExpressionMorphChannelCount; ++c) {
                composed[c] = ow[c] * overrideLayer->intensity;
            }
        }

        for (const auto& layer : layers_) {
            if (!layer.active || !layer.additive) continue;
            const MorphWeights& lw = ExpressionPresetTable::Get(layer.type);
            for (int c = 0; c < kExpressionMorphChannelCount; ++c) {
                composed[c] = std::min(1.0f, composed[c] + lw[c] * layer.intensity);
            }
        }

        targetWeights_ = composed;

        // 2. Smoothly blend current -> target.
        float alpha = std::min(1.0f, blendSpeed_ * deltaTime);
        for (int c = 0; c < kExpressionMorphChannelCount; ++c) {
            currentWeights_[c] += (targetWeights_[c] - currentWeights_[c]) * alpha;
        }

        // 3. Overlay lip sync viseme onto mouth/jaw channels only, so
        //    dialogue speech doesn't fight with brow/eye emotion channels.
        if (visemePlaying_) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (activeTimeline_) {
                visemeTime_ += deltaTime;
                MorphWeights visemeWeights = activeTimeline_->Sample(visemeTime_);
                static const int mouthChannels[] = {
                    MORPH_MOUTH_OPEN, MORPH_MOUTH_WIDE, MORPH_MOUTH_PUCKER, MORPH_MOUTH_PRESS,
                    MORPH_LIP_UPPER_UP, MORPH_LIP_LOWER_DOWN, MORPH_JAW_OPEN,
                };
                for (int ch : mouthChannels) {
                    currentWeights_[ch] = visemeWeights[ch];
                }
                if (visemeTime_ >= activeTimeline_->GetDuration()) {
                    visemePlaying_ = false;
                }
            }
        }

        return currentWeights_;
    }

    const MorphWeights& GetCurrentWeights() const { return currentWeights_; }
    ExpressionType GetBaseExpression() const { return baseType_; }

private:
    MorphWeights currentWeights_;
    MorphWeights targetWeights_;

    ExpressionType baseType_ = ExpressionType::NEUTRAL;
    ExpressionPriority basePriority_ = ExpressionPriority::IDLE;
    float baseHoldRemaining_ = 0.0f;
    float blendSpeed_ = 4.0f;

    std::vector<ExpressionLayer> layers_;

    std::shared_ptr<VisemeTimeline> activeTimeline_;
    float visemeTime_ = 0.0f;
    bool visemePlaying_ = false;

    std::mutex mutex_;
};

} // namespace facegen
