#include "scene_renderer.h"
#include "renderer.h"
#include <chrono>

// ============================================================================
// SceneRenderer implementation
// ============================================================================

SceneRenderer::SceneRenderer() = default;

SceneRenderer::~SceneRenderer() {
    shutdown();
}

bool SceneRenderer::initialize(Renderer* renderer) {
    renderer_ = renderer;
    if (!renderer_) {
        LOGE("SceneRenderer: null renderer");
        return false;
    }

    resetStats();
    LOGI("SceneRenderer initialized");
    return true;
}

void SceneRenderer::shutdown() {
    renderer_ = nullptr;
    LOGI("SceneRenderer shutdown");
}

void SceneRenderer::render(float deltaTime) {
    if (!renderer_) return;

    auto totalStart = std::chrono::high_resolution_clock::now();
    resetStats();

    // Pass 1: 3D World
    if (worldEnabled_) {
        renderWorld(deltaTime);
    }

    // Pass 2: NPCs and entities
    if (entityEnabled_) {
        renderEntities(deltaTime);
    }

    // Pass 3: Effects (particles, spells, weather)
    if (effectsEnabled_) {
        renderEffects(deltaTime);
    }

    // Pass 4: 2D UI / HUD
    if (uiEnabled_) {
        renderUI(deltaTime);
    }

    // Pass 5: Post-processing (RetroFilter)
    if (postProcessEnabled_) {
        renderPostProcess(deltaTime);
    }

    auto totalEnd = std::chrono::high_resolution_clock::now();
    stats_.totalRenderMs = std::chrono::duration<float, std::milli>(totalEnd - totalStart).count();
}

void SceneRenderer::renderWorld(float deltaTime) {
    startPassTimer();

    // World rendering is handled by Renderer::render which calls
    // WorldManager and landscape rendering internally.
    // The Renderer already manages the full 3D scene pipeline.
    if (renderer_) {
        renderer_->render(deltaTime);
    }

    stats_.worldRenderMs = endPassTimer();
}

void SceneRenderer::renderEntities(float deltaTime) {
    (void)deltaTime;
    startPassTimer();

    // Entity rendering (NPCs, objects) is integrated into Renderer::render
    // via NpcManager. This pass is for any additional entity rendering
    // that might be needed outside the main render call.

    stats_.entityRenderMs = endPassTimer();
}

void SceneRenderer::renderEffects(float deltaTime) {
    (void)deltaTime;
    startPassTimer();

    // Effects rendering (particles, spell effects, weather)
    // Currently integrated into the main render pass.
    // This is a placeholder for future dedicated effects rendering.

    stats_.effectsRenderMs = endPassTimer();
}

void SceneRenderer::renderUI(float deltaTime) {
    (void)deltaTime;
    startPassTimer();

    // UI/HUD rendering is handled by UIManager within Renderer::render
    // This pass is for any additional 2D overlay rendering.

    stats_.uiRenderMs = endPassTimer();
}

void SceneRenderer::renderPostProcess(float deltaTime) {
    (void)deltaTime;
    startPassTimer();

    // Post-processing (RetroFilter) is applied within Renderer::render
    // This pass is for any additional post-processing effects.

    stats_.postProcessMs = endPassTimer();
}

void SceneRenderer::resetStats() {
    stats_ = RenderStats{};
}

void SceneRenderer::startPassTimer() {
    passStart_ = std::chrono::high_resolution_clock::now();
}

float SceneRenderer::endPassTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float, std::milli>(end - passStart_).count();
}
