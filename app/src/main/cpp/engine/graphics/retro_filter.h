#pragma once

#include <memory>
#include <chrono>
#include <android/log.h>
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include "../shader.h"

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGW
#undef LOGE

#define LOG_TAG "RetroFilter"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * RetroFilter - Post-processing filter system for retro/vintage CRT monitor effects
 * Applies chain of effects: pixelation, color reduction, CRT distortion, scanlines, and film grain
 * All rendering is done to offscreen framebuffers with optional resolution scaling
 */
class RetroFilter {
public:
    /**
     * Resolution scale options for rendering the scene
     * Lower resolutions enhance pixelation effect
     */
    enum ResolutionScale {
        FULL_RES = 0,       // Native 1280x720 (or screen resolution)
        HALF_RES = 1,       // Half resolution (640x360)
        QUARTER_RES = 2     // Quarter resolution (320x180)
    };

    /**
     * Settings structure for all retro filter effects
     * All effects can be toggled independently
     */
    struct Settings {
        // Scanlines - CRT走査線効果
        bool scanlines_enabled = true;
        float scanlines_intensity = 0.5f;    // 0.0 (invisible) - 1.0 (opaque)

        // Pixelation - ピクセルブロック化
        bool pixelation_enabled = true;
        float pixelation_scale = 8.0f;       // 2.0 (small) - 16.0 (large blocks)

        // Color Reduction - 発色数制限
        bool color_reduction_enabled = true;
        int color_bits = 8;                  // 4 (16 colors) - 8 (256 colors)

        // CRT Distortion - 画面の湾曲歪み
        bool crt_distortion_enabled = true;
        float distortion_strength = 0.1f;    // 0.0 (none) - 0.3 (strong)

        // Film Grain - 時間ベースノイズ
        bool grain_enabled = true;
        float grain_strength = 0.3f;         // 0.0 (none) - 1.0 (heavy)

        // Resolution scaling
        ResolutionScale resolution_scale = FULL_RES;
    };

    /**
     * Constructor
     */
    RetroFilter();

    /**
     * Destructor - cleans up GPU resources
     */
    ~RetroFilter();

    /**
     * Initialize the retro filter system
     * @param width Screen width in pixels
     * @param height Screen height in pixels
     * @return true if initialization successful, false otherwise
     */
    bool initialize(unsigned int width, unsigned int height);

    /**
     * Bind the scene framebuffer for rendering the main game content
     * Call this before rendering world/UI, then call apply() and renderToScreen() after
     */
    void bindSceneFramebuffer();

    /**
     * Apply all active retro filter effects in correct order
     * @param settings RetroFilter settings specifying which effects to apply
     */
    void apply(const Settings& settings);

    /**
     * Render the final processed image to the screen
     * Handles upscaling if scene was rendered at lower resolution
     */
    void renderToScreen();

    /**
     * Clean up all GPU resources
     */
    void cleanup();

    /**
     * Update screen dimensions (call on screen resize/orientation change)
     * @param width New screen width
     * @param height New screen height
     */
    void setNativeResolution(unsigned int width, unsigned int height);

    /**
     * Get current scene rendering width
     */
    unsigned int getSceneWidth() const { return sceneWidth; }

    /**
     * Get current scene rendering height
     */
    unsigned int getSceneHeight() const { return sceneHeight; }

private:
    // ========== GPU Resource Management ==========

    // Screen dimensions
    unsigned int screenWidth = 0;
    unsigned int screenHeight = 0;

    // Scene rendering FBO (may be at reduced resolution)
    unsigned int sceneWidth = 0;
    unsigned int sceneHeight = 0;
    GLuint sceneFBO = 0;
    GLuint sceneTex = 0;

    // Work framebuffers for effect ping-ponging
    GLuint workFBO1 = 0;
    GLuint workTex1 = 0;
    GLuint workFBO2 = 0;
    GLuint workTex2 = 0;

    // Current output texture after effect chain
    GLuint finalResultTex = 0;

    // Last applied resolution scale (to detect changes)
    ResolutionScale lastScale = FULL_RES;

    // ========== Shader Programs ==========

    // Embedded GLSL shaders - one per effect type
    ShaderProgram* scanlineShader = nullptr;
    ShaderProgram* pixelationShader = nullptr;
    ShaderProgram* colorReductionShader = nullptr;
    ShaderProgram* distortionShader = nullptr;
    ShaderProgram* grainShader = nullptr;
    ShaderProgram* compositeShader = nullptr;  // For upscaling if needed

    // ========== Geometry ==========

    // Full-screen quad for rendering effects
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;

    // ========== Timing ==========

    // Start time for time-based grain animation
    std::chrono::steady_clock::time_point startTime;

    // ========== Performance Optimization: Cached Values ==========

    // Grain time caching - update only every N frames instead of every frame
    float cachedGrainTime = 0.0f;
    int framesSinceLastUpdate = 0;
    static constexpr int GRAIN_TIME_UPDATE_INTERVAL = 60;  // Update every 60 frames

    // Color reduction pre-calculation cache
    float cachedColorLevels = 256.0f;
    int lastColorBits = 8;

    // Pre-computed scene resolution uniform
    glm::vec2 cachedSceneResolution = glm::vec2(0.0f, 0.0f);
    bool sceneResolutionChanged = true;

    // ========== Private Methods ==========

    /**
     * Create all required framebuffer objects
     * Creates FBOs for scene rendering and effect processing at current resolution
     */
    void createFramebuffers();

    /**
     * Create fullscreen quad mesh for effect rendering
     * Quad spans from -1 to 1 in NDC space
     */
    void createQuadMesh();

    /**
     * Create and compile all shader programs
     * Shaders contain embedded GLSL code
     */
    void createShaders();

    /**
     * Render a fullscreen quad with specified input texture
     * Binds quad VAO and draws with bound shader
     * @param inputTexture The texture to render (should be bound to texture unit 0)
     */
    void renderScreenQuad(GLuint inputTexture);

    /**
     * Compute scene dimensions based on resolution scale setting
     * @param scale Resolution scale mode (FULL/HALF/QUARTER)
     */
    void computeSceneResolution(ResolutionScale scale);

    /**
     * Helper to check OpenGL errors for debugging
     */
    void checkGLError(const char* operation);
};
