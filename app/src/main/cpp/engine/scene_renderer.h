#pragma once

#include <cstdint>
#include <vector>
#include <android/log.h>

#define LOG_TAG "SceneRenderer"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward declarations
class Renderer;
class WorldManager;
class NpcManager;

namespace oblivion {
class PhysicsManager;
}

// ============================================================================
// SceneRenderer - 3D scene -> UI -> HUD draw pipeline
// Phase 42: Full game loop integration
// ============================================================================

// Render pass types
enum class RenderPass : uint8_t {
    WORLD,          // 3D world geometry (terrain, buildings, trees)
    NPC_ENTITY,     // NPCs and interactive objects
    EFFECTS,        // Particle effects, spells, weather
    UI_HUD,         // 2D UI overlay, HUD elements
    POST_PROCESS    // Retro filter, screen effects
};

// Render statistics
struct RenderStats {
    uint32_t drawCalls = 0;
    uint32_t triangles = 0;
    uint32_t vertices = 0;
    float worldRenderMs = 0.0f;
    float entityRenderMs = 0.0f;
    float effectsRenderMs = 0.0f;
    float uiRenderMs = 0.0f;
    float postProcessMs = 0.0f;
    float totalRenderMs = 0.0f;
};

class SceneRenderer {
public:
    SceneRenderer();
    ~SceneRenderer();

    bool initialize(Renderer* renderer);
    void shutdown();

    // Main render call - executes all passes in order
    void render(float deltaTime);

    // Individual pass control
    void renderWorld(float deltaTime);
    void renderEntities(float deltaTime);
    void renderEffects(float deltaTime);
    void renderUI(float deltaTime);
    void renderPostProcess(float deltaTime);

    // Render statistics
    const RenderStats& getStats() const { return stats_; }
    void resetStats();

    // Configuration
    void setWorldRenderingEnabled(bool enabled) { worldEnabled_ = enabled; }
    void setEntityRenderingEnabled(bool enabled) { entityEnabled_ = enabled; }
    void setEffectsRenderingEnabled(bool enabled) { effectsEnabled_ = enabled; }
    void setUIRenderingEnabled(bool enabled) { uiEnabled_ = enabled; }
    void setPostProcessEnabled(bool enabled) { postProcessEnabled_ = enabled; }

    // Retro filter control
    void setRetroFilterEnabled(bool enabled) { retroFilterEnabled_ = enabled; }
    bool isRetroFilterEnabled() const { return retroFilterEnabled_; }

private:
    Renderer* renderer_ = nullptr;

    // Pass enable flags
    bool worldEnabled_ = true;
    bool entityEnabled_ = true;
    bool effectsEnabled_ = true;
    bool uiEnabled_ = true;
    bool postProcessEnabled_ = true;
    bool retroFilterEnabled_ = true;

    // Statistics
    RenderStats stats_;

    // Timing helpers
    std::chrono::high_resolution_clock::time_point passStart_;
    void startPassTimer();
    float endPassTimer();
};
