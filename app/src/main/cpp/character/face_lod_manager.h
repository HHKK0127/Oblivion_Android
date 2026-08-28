#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <glm/glm.hpp>
#include <android/log.h>

// ============================================================================
// Phase 55: Face LOD Manager
//
// Manages level-of-detail for NPC faces based on camera distance: full
// morphed mesh + blended texture up close, a simplified mesh/base texture at
// medium range, a cheap billboard impostor further out, and nothing rendered
// beyond the culling distance. Includes cross-fade transitions between
// levels, per-frame batch processing limits, and a simple memory budget.
// ============================================================================

#define LOG_TAG_FLM "FaceLodManager"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_FLM(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_FLM, __VA_ARGS__)
#else
#define LOGD_FLM(...) do {} while(0)
#endif
#define LOGI_FLM(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_FLM, __VA_ARGS__)
#define LOGW_FLM(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_FLM, __VA_ARGS__)
#define LOGE_FLM(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_FLM, __VA_ARGS__)

namespace facegen {

// ----------------------------------------------------------------------------
// LOD levels
// ----------------------------------------------------------------------------
enum class FaceLodLevel : uint8_t {
    FULL = 0,   // morphed mesh + fully blended texture
    MEDIUM,     // simplified mesh + base texture only
    LOW,        // billboard impostor
    HIDDEN,     // not rendered at all
};

inline const char* FaceLodLevelToString(FaceLodLevel level) {
    switch (level) {
        case FaceLodLevel::FULL:   return "FULL";
        case FaceLodLevel::MEDIUM: return "MEDIUM";
        case FaceLodLevel::LOW:    return "LOW";
        case FaceLodLevel::HIDDEN: return "HIDDEN";
        default:                   return "UNKNOWN";
    }
}

// Distance thresholds (meters) at which the LOD level downgrades.
struct FaceLodThresholds {
    float fullMaxDistance = 5.0f;
    float mediumMaxDistance = 20.0f;
    float lowMaxDistance = 50.0f;
    // beyond lowMaxDistance -> HIDDEN

    FaceLodLevel LevelForDistance(float distance) const {
        if (distance < fullMaxDistance) return FaceLodLevel::FULL;
        if (distance < mediumMaxDistance) return FaceLodLevel::MEDIUM;
        if (distance < lowMaxDistance) return FaceLodLevel::LOW;
        return FaceLodLevel::HIDDEN;
    }
};

// ----------------------------------------------------------------------------
// Impostor: a small render-to-texture capture of the face from a fixed set of
// angles, used at LOW LOD instead of the real mesh.
// ----------------------------------------------------------------------------
static constexpr int kImpostorAngleCount = 8;
static constexpr int kImpostorTextureSize = 64;

struct FaceImpostor {
    uint32_t textureHandles[kImpostorAngleCount] = {0};
    bool anglesCaptured[kImpostorAngleCount] = {false};
    bool fullyCaptured = false;

    int AngleIndexForYaw(float yawDegrees) const {
        // Normalize to [0, 360) then bucket into 8 45-degree slices.
        float normalized = std::fmod(yawDegrees, 360.0f);
        if (normalized < 0.0f) normalized += 360.0f;
        int idx = static_cast<int>((normalized + 22.5f) / 45.0f) % kImpostorAngleCount;
        return idx;
    }

    void MarkCaptured(int angleIndex, uint32_t textureHandle) {
        if (angleIndex < 0 || angleIndex >= kImpostorAngleCount) return;
        textureHandles[angleIndex] = textureHandle;
        anglesCaptured[angleIndex] = true;
        fullyCaptured = std::all_of(std::begin(anglesCaptured), std::end(anglesCaptured),
                                     [](bool b) { return b; });
    }

    uint32_t GetTextureForYaw(float yawDegrees) const {
        int idx = AngleIndexForYaw(yawDegrees);
        return anglesCaptured[idx] ? textureHandles[idx] : 0;
    }
};

// ----------------------------------------------------------------------------
// Per-NPC LOD state
// ----------------------------------------------------------------------------
struct FaceLodState {
    uint32_t npcId = 0;
    FaceLodLevel currentLevel = FaceLodLevel::HIDDEN;
    FaceLodLevel targetLevel = FaceLodLevel::HIDDEN;

    // Cross-fade blend factor: 0 = fully at currentLevel's visual, 1 = fully
    // at targetLevel's visual. Once it reaches 1, currentLevel snaps to
    // targetLevel and the factor resets to 0.
    float transitionBlend = 0.0f;
    float transitionSpeed = 2.0f; // blend units per second

    float lastDistance = 0.0f;
    bool hasImpostor = false;
    std::shared_ptr<FaceImpostor> impostor;

    // Resource residency flags, used to enforce the memory budget.
    bool fullResResident = false;
    bool mediumResResident = false;
};

// ----------------------------------------------------------------------------
// Memory budget tracking
// ----------------------------------------------------------------------------
struct FaceLodBudget {
    int maxFullRes = 50;
    int maxMediumRes = 200;
    // Low-res impostors are cheap (64x64) and considered unlimited.

    int currentFullRes = 0;
    int currentMediumRes = 0;

    bool HasFullResRoom() const { return currentFullRes < maxFullRes; }
    bool HasMediumResRoom() const { return currentMediumRes < maxMediumRes; }
};

// ----------------------------------------------------------------------------
// FaceLodManager
//
// Owns LOD state for all currently tracked NPC faces. Call UpdateDistances()
// once per frame (or on a throttled cadence) with the camera position, then
// ProcessBatch() to advance a limited number of transitions/resource loads
// per frame to avoid frame spikes.
// ----------------------------------------------------------------------------
class FaceLodManager {
public:
    explicit FaceLodManager(const FaceLodThresholds& thresholds = FaceLodThresholds())
        : thresholds_(thresholds) {}

    void RegisterNpc(uint32_t npcId) {
        if (states_.find(npcId) != states_.end()) {
            return;
        }
        FaceLodState state;
        state.npcId = npcId;
        states_.emplace(npcId, state);
        pendingUpdate_.push_back(npcId);
    }

    void UnregisterNpc(uint32_t npcId) {
        auto it = states_.find(npcId);
        if (it == states_.end()) return;
        ReleaseResources(it->second);
        states_.erase(it);
    }

    // Recomputes each NPC's target LOD level from distance to camera. This is
    // cheap (just math) so it can run for every registered NPC every frame;
    // the expensive resource transitions are throttled in ProcessBatch().
    void UpdateDistances(const glm::vec3& cameraPos,
                         const std::unordered_map<uint32_t, glm::vec3>& npcPositions) {
        for (auto& kv : states_) {
            FaceLodState& state = kv.second;
            auto posIt = npcPositions.find(state.npcId);
            if (posIt == npcPositions.end()) {
                continue;
            }
            float distance = glm::length(posIt->second - cameraPos);
            state.lastDistance = distance;

            FaceLodLevel newTarget = thresholds_.LevelForDistance(distance);
            if (newTarget != state.targetLevel) {
                state.targetLevel = newTarget;
                if (std::find(pendingUpdate_.begin(), pendingUpdate_.end(), state.npcId) ==
                    pendingUpdate_.end()) {
                    pendingUpdate_.push_back(state.npcId);
                }
            }
        }
    }

    // Advances at most `maxPerFrame` (default 5) pending NPC face LOD
    // transitions, respecting the memory budget. Call once per frame.
    void ProcessBatch(int maxPerFrame = 5) {
        int processed = 0;
        while (processed < maxPerFrame && !pendingUpdate_.empty()) {
            uint32_t npcId = pendingUpdate_.front();
            pendingUpdate_.erase(pendingUpdate_.begin());

            auto it = states_.find(npcId);
            if (it == states_.end()) {
                continue;
            }
            TransitionOne(it->second);
            ++processed;
        }
    }

    // Advances cross-fade blend factors for all NPCs currently mid-transition.
    // Cheap enough to run for all of them every frame.
    void UpdateTransitions(float deltaTime) {
        for (auto& kv : states_) {
            FaceLodState& state = kv.second;
            if (state.currentLevel == state.targetLevel) {
                continue;
            }
            state.transitionBlend = std::min(1.0f, state.transitionBlend + state.transitionSpeed * deltaTime);
            if (state.transitionBlend >= 1.0f) {
                state.currentLevel = state.targetLevel;
                state.transitionBlend = 0.0f;
            }
        }
    }

    const FaceLodState* GetState(uint32_t npcId) const {
        auto it = states_.find(npcId);
        return (it != states_.end()) ? &it->second : nullptr;
    }

    const FaceLodBudget& GetBudget() const { return budget_; }

    size_t GetTrackedNpcCount() const { return states_.size(); }
    size_t GetPendingCount() const { return pendingUpdate_.size(); }

private:
    void TransitionOne(FaceLodState& state) {
        if (state.currentLevel == state.targetLevel) {
            return;
        }

        LOGD_FLM("NPC %u transitioning %s -> %s (dist=%.1f)",
                 state.npcId,
                 FaceLodLevelToString(state.currentLevel),
                 FaceLodLevelToString(state.targetLevel),
                 state.lastDistance);

        // Acquire resources for the target level if budget allows; otherwise
        // fall back to a lower-cost level to stay within budget.
        FaceLodLevel achievable = state.targetLevel;

        if (achievable == FaceLodLevel::FULL && !budget_.HasFullResRoom()) {
            LOGW_FLM("Full-res budget exhausted (%d/%d), downgrading NPC %u to MEDIUM",
                     budget_.currentFullRes, budget_.maxFullRes, state.npcId);
            achievable = FaceLodLevel::MEDIUM;
        }
        if (achievable == FaceLodLevel::MEDIUM && !budget_.HasMediumResRoom()) {
            LOGW_FLM("Medium-res budget exhausted (%d/%d), downgrading NPC %u to LOW",
                     budget_.currentMediumRes, budget_.maxMediumRes, state.npcId);
            achievable = FaceLodLevel::LOW;
        }

        ReleaseResources(state);
        AcquireResources(state, achievable);

        state.targetLevel = achievable;
        state.transitionBlend = 0.0f;
        // currentLevel is updated incrementally by UpdateTransitions(); we
        // just kicked off acquisition of the new target's resources here.
    }

    void AcquireResources(FaceLodState& state, FaceLodLevel level) {
        switch (level) {
            case FaceLodLevel::FULL:
                state.fullResResident = true;
                budget_.currentFullRes++;
                break;
            case FaceLodLevel::MEDIUM:
                state.mediumResResident = true;
                budget_.currentMediumRes++;
                break;
            case FaceLodLevel::LOW:
                if (!state.impostor) {
                    state.impostor = std::make_shared<FaceImpostor>();
                }
                state.hasImpostor = true;
                break;
            case FaceLodLevel::HIDDEN:
            default:
                break;
        }
    }

    void ReleaseResources(FaceLodState& state) {
        if (state.fullResResident) {
            state.fullResResident = false;
            budget_.currentFullRes = std::max(0, budget_.currentFullRes - 1);
        }
        if (state.mediumResResident) {
            state.mediumResResident = false;
            budget_.currentMediumRes = std::max(0, budget_.currentMediumRes - 1);
        }
        // Impostors are cheap; keep them cached across transitions rather
        // than freeing eagerly, so repeated in/out-of-range NPCs don't
        // re-render all 8 angles every time.
    }

    FaceLodThresholds thresholds_;
    FaceLodBudget budget_;
    std::unordered_map<uint32_t, FaceLodState> states_;
    std::vector<uint32_t> pendingUpdate_;
};

} // namespace facegen
