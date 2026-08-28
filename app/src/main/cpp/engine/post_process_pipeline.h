#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG_PP "PostProcess"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_PP(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_PP, __VA_ARGS__)
#else
#define LOGD_PP(...) do {} while(0)
#endif
#define LOGI_PP(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_PP, __VA_ARGS__)

// ============================================================================
// Post-Process Pipeline
// Phase 56: Full-screen effects for Oblivion's visual style
// Effects: bloom, tone mapping, fog, depth of field, vignette, color grading
// ============================================================================

namespace engine {

// Post-process effect types
enum class PostEffect : uint8_t {
    BLOOM = 0,
    TONE_MAPPING,
    FOG,
    DEPTH_OF_FIELD,
    VIGNETTE,
    COLOR_GRADING,
    MOTION_BLUR,
    FXAA,
    COUNT
};

// Fog mode (Oblivion uses distance fog heavily)
enum class FogMode : uint8_t {
    NONE = 0,
    LINEAR,
    EXP,
    EXP2,
    HEIGHT_BASED     // Oblivion's height fog
};

// Fog parameters
struct FogParams {
    FogMode mode = FogMode::LINEAR;
    float density = 0.01f;
    float start = 50.0f;
    float end = 500.0f;
    float color[3] = {0.7f, 0.8f, 0.9f};  // Sky blue
    float heightMin = -10.0f;
    float heightMax = 100.0f;
    float heightDensity = 0.1f;
};

// Bloom parameters
struct BloomParams {
    bool enabled = true;
    float threshold = 0.8f;         // Brightness threshold
    float intensity = 0.3f;         // Bloom strength
    float radius = 4.0f;            // Blur radius
    uint32_t iterations = 3;        // Blur passes
    float knee = 0.5f;              // Soft knee for threshold
};

// Tone mapping parameters
struct ToneMappingParams {
    bool enabled = true;
    float exposure = 1.0f;
    float contrast = 1.0f;
    float saturation = 1.0f;
    float whitePoint = 1.0f;
    // ACES filmic tone mapping
    float acesA = 2.51f;
    float acesB = 0.03f;
    float acesC = 2.43f;
    float acesD = 0.59f;
    float acesE = 0.14f;
};

// Depth of field parameters
struct DoFParams {
    bool enabled = false;
    float focalDistance = 10.0f;    // Focus distance
    float focalRange = 5.0f;        // Focus range
    float blurNear = 0.5f;          // Near blur strength
    float blurFar = 1.0f;           // Far blur strength
    float maxBlur = 8.0f;           // Max blur radius
};

// Vignette parameters
struct VignetteParams {
    bool enabled = true;
    float intensity = 0.3f;
    float smoothness = 0.5f;
    float roundness = 1.0f;
    float color[3] = {0.0f, 0.0f, 0.0f};
};

// Color grading LUT (Look-Up Table)
struct ColorGradingParams {
    bool enabled = false;
    float temperature = 0.0f;       // Warm/cool shift
    float tint = 0.0f;
    float shadows[3] = {0.0f, 0.0f, 0.0f};
    float midtones[3] = {0.0f, 0.0f, 0.0f};
    float highlights[3] = {0.0f, 0.0f, 0.0f};
    float lift = 0.0f;
    float gamma = 1.0f;
    float gain = 1.0f;
};

// Motion blur parameters
struct MotionBlurParams {
    bool enabled = false;
    float intensity = 0.5f;
    uint32_t samples = 8;
    float maxVelocity = 100.0f;
};

// FXAA parameters
struct FXAAParams {
    bool enabled = true;
    float qualitySubpixel = 0.75f;
    float qualityEdgeThreshold = 0.125f;
    float qualityEdgeThresholdMin = 0.0625f;
};

// Complete post-process configuration
struct PostProcessConfig {
    BloomParams bloom;
    ToneMappingParams toneMapping;
    FogParams fog;
    DoFParams dof;
    VignetteParams vignette;
    ColorGradingParams colorGrading;
    MotionBlurParams motionBlur;
    FXAAParams fxaa;
};

// ============================================================================
// PostProcessPipeline - manages all post-process effects
// ============================================================================

class PostProcessPipeline {
public:
    static PostProcessPipeline& instance() {
        static PostProcessPipeline inst;
        return inst;
    }

    void init() {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = PostProcessConfig{};
        enabledEffects_.resize(static_cast<size_t>(PostEffect::COUNT), true);
        initialized_ = true;
        LOGI_PP("PostProcessPipeline initialized");
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        initialized_ = false;
    }

    // --- Effect enable/disable ---

    void setEffectEnabled(PostEffect effect, bool enabled) {
        std::lock_guard<std::mutex> lock(mutex_);
        enabledEffects_[static_cast<size_t>(effect)] = enabled;
    }

    bool isEffectEnabled(PostEffect effect) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return enabledEffects_[static_cast<size_t>(effect)];
    }

    // --- Bloom ---

    void setBloom(const BloomParams& params) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.bloom = params;
    }

    const BloomParams& getBloom() const { return config_.bloom; }

    // --- Tone Mapping ---

    void setToneMapping(const ToneMappingParams& params) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.toneMapping = params;
    }

    void setExposure(float exposure) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.toneMapping.exposure = exposure;
    }

    const ToneMappingParams& getToneMapping() const { return config_.toneMapping; }

    // --- Fog ---

    void setFog(const FogParams& params) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.fog = params;
    }

    void setFogMode(FogMode mode) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.fog.mode = mode;
    }

    void setFogDensity(float density) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.fog.density = density;
    }

    void setFogColor(float r, float g, float b) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.fog.color[0] = r;
        config_.fog.color[1] = g;
        config_.fog.color[2] = b;
    }

    const FogParams& getFog() const { return config_.fog; }

    // --- Depth of Field ---

    void setDoF(const DoFParams& params) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.dof = params;
    }

    const DoFParams& getDoF() const { return config_.dof; }

    // --- Vignette ---

    void setVignette(const VignetteParams& params) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.vignette = params;
    }

    const VignetteParams& getVignette() const { return config_.vignette; }

    // --- Color Grading ---

    void setColorGrading(const ColorGradingParams& params) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.colorGrading = params;
    }

    const ColorGradingParams& getColorGrading() const { return config_.colorGrading; }

    // --- Motion Blur ---

    void setMotionBlur(const MotionBlurParams& params) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.motionBlur = params;
    }

    const MotionBlurParams& getMotionBlur() const { return config_.motionBlur; }

    // --- FXAA ---

    void setFXAA(const FXAAParams& params) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_.fxaa = params;
    }

    const FXAAParams& getFXAA() const { return config_.fxaa; }

    // --- Presets ---

    void applyPreset(const std::string& presetName) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (presetName == "oblivion_default") {
            // Oblivion's characteristic look
            config_.fog.mode = FogMode::LINEAR;
            config_.fog.start = 50.0f;
            config_.fog.end = 500.0f;
            config_.fog.color[0] = 0.7f;
            config_.fog.color[1] = 0.8f;
            config_.fog.color[2] = 0.9f;
            config_.toneMapping.exposure = 1.1f;
            config_.toneMapping.contrast = 1.05f;
            config_.toneMapping.saturation = 0.95f;
            config_.bloom.enabled = true;
            config_.bloom.threshold = 0.85f;
            config_.bloom.intensity = 0.25f;
            config_.vignette.enabled = true;
            config_.vignette.intensity = 0.2f;
            config_.fxaa.enabled = true;
        } else if (presetName == "dungeon") {
            // Dark dungeon look
            config_.fog.mode = FogMode::EXP2;
            config_.fog.density = 0.02f;
            config_.fog.color[0] = 0.1f;
            config_.fog.color[1] = 0.08f;
            config_.fog.color[2] = 0.05f;
            config_.toneMapping.exposure = 0.8f;
            config_.toneMapping.contrast = 1.2f;
            config_.vignette.intensity = 0.5f;
            config_.bloom.threshold = 0.9f;
            config_.bloom.intensity = 0.15f;
        } else if (presetName == "night") {
            // Night scene
            config_.fog.mode = FogMode::LINEAR;
            config_.fog.start = 30.0f;
            config_.fog.end = 300.0f;
            config_.fog.color[0] = 0.05f;
            config_.fog.color[1] = 0.05f;
            config_.fog.color[2] = 0.15f;
            config_.toneMapping.exposure = 0.6f;
            config_.toneMapping.saturation = 0.7f;
            config_.bloom.enabled = true;
            config_.bloom.threshold = 0.7f;
            config_.bloom.intensity = 0.4f;
        } else if (presetName == "oblivion_gate") {
            // Oblivion realm - red and hellish
            config_.fog.mode = FogMode::EXP;
            config_.fog.density = 0.005f;
            config_.fog.color[0] = 0.8f;
            config_.fog.color[1] = 0.2f;
            config_.fog.color[2] = 0.1f;
            config_.toneMapping.exposure = 1.3f;
            config_.toneMapping.contrast = 1.3f;
            config_.toneMapping.saturation = 1.2f;
            config_.colorGrading.shadows[0] = 0.3f;
            config_.colorGrading.shadows[1] = -0.1f;
            config_.colorGrading.shadows[2] = -0.2f;
            config_.bloom.enabled = true;
            config_.bloom.threshold = 0.6f;
            config_.bloom.intensity = 0.5f;
        } else if (presetName == "performance") {
            // Minimal effects for low-end devices
            config_.bloom.enabled = false;
            config_.dof.enabled = false;
            config_.motionBlur.enabled = false;
            config_.colorGrading.enabled = false;
            config_.vignette.enabled = false;
            config_.fxaa.enabled = true;
            config_.fog.mode = FogMode::LINEAR;
            config_.fog.start = 100.0f;
            config_.fog.end = 400.0f;
        }
    }

    // --- Full configuration access ---

    void setConfig(const PostProcessConfig& config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = config;
    }

    const PostProcessConfig& getConfig() const { return config_; }

    // Generate GLSL fragment shader source for the combined post-process pass
    std::string generateShaderSource() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string src;

        src += "#version 300 es\n";
        src += "precision highp float;\n";
        src += "in vec2 vTexCoord;\n";
        src += "out vec4 fragColor;\n";
        src += "uniform sampler2D uSceneTexture;\n";
        src += "uniform sampler2D uDepthTexture;\n";
        src += "uniform sampler2D uBloomTexture;\n";
        src += "uniform float uTime;\n";
        src += "uniform vec2 uResolution;\n";

        // Fog uniforms
        if (config_.fog.mode != FogMode::NONE) {
            src += "uniform vec3 uFogColor;\n";
            src += "uniform float uFogDensity;\n";
            src += "uniform float uFogStart;\n";
            src += "uniform float uFogEnd;\n";
        }

        src += "\nvoid main() {\n";
        src += "    vec4 color = texture(uSceneTexture, vTexCoord);\n";
        src += "    float depth = texture(uDepthTexture, vTexCoord).r;\n";

        // Fog application
        if (config_.fog.mode != FogMode::NONE && enabledEffects_[static_cast<size_t>(PostEffect::FOG)]) {
            src += "    // Fog\n";
            src += "    float fogFactor = 0.0;\n";
            if (config_.fog.mode == FogMode::LINEAR) {
                src += "    fogFactor = clamp((uFogEnd - depth) / (uFogEnd - uFogStart), 0.0, 1.0);\n";
            } else if (config_.fog.mode == FogMode::EXP) {
                src += "    fogFactor = exp(-uFogDensity * depth);\n";
            } else if (config_.fog.mode == FogMode::EXP2) {
                src += "    fogFactor = exp(-uFogDensity * uFogDensity * depth * depth);\n";
            }
            src += "    color.rgb = mix(uFogColor, color.rgb, fogFactor);\n";
        }

        // Bloom
        if (config_.bloom.enabled && enabledEffects_[static_cast<size_t>(PostEffect::BLOOM)]) {
            src += "    // Bloom\n";
            src += "    vec4 bloom = texture(uBloomTexture, vTexCoord);\n";
            src += "    color.rgb += bloom.rgb * " +
                   std::to_string(config_.bloom.intensity) + ";\n";
        }

        // Tone mapping (ACES filmic)
        if (config_.toneMapping.enabled && enabledEffects_[static_cast<size_t>(PostEffect::TONE_MAPPING)]) {
            src += "    // ACES Tone Mapping\n";
            src += "    color.rgb *= " + std::to_string(config_.toneMapping.exposure) + ";\n";
            src += "    vec3 a = color.rgb * (" + std::to_string(config_.toneMapping.acesA) +
                   " * color.rgb + " + std::to_string(config_.toneMapping.acesB) + ");\n";
            src += "    vec3 b = color.rgb * (" + std::to_string(config_.toneMapping.acesC) +
                   " * color.rgb + " + std::to_string(config_.toneMapping.acesD) + ") + " +
                   std::to_string(config_.toneMapping.acesE) + ";\n";
            src += "    color.rgb = clamp(a / b, 0.0, 1.0);\n";

            // Saturation
            if (std::abs(config_.toneMapping.saturation - 1.0f) > 0.01f) {
                src += "    float lum = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));\n";
                src += "    color.rgb = mix(vec3(lum), color.rgb, " +
                       std::to_string(config_.toneMapping.saturation) + ");\n";
            }
        }

        // Vignette
        if (config_.vignette.enabled && enabledEffects_[static_cast<size_t>(PostEffect::VIGNETTE)]) {
            src += "    // Vignette\n";
            src += "    vec2 uv = vTexCoord * 2.0 - 1.0;\n";
            src += "    float dist = length(uv);\n";
            src += "    float vig = smoothstep(" +
                   std::to_string(config_.vignette.roundness) + ", " +
                   std::to_string(config_.vignette.roundness - config_.vignette.smoothness) +
                   ", dist);\n";
            src += "    color.rgb = mix(vec3(" +
                   std::to_string(config_.vignette.color[0]) + ", " +
                   std::to_string(config_.vignette.color[1]) + ", " +
                   std::to_string(config_.vignette.color[2]) + "), color.rgb, " +
                   "mix(1.0, vig, " + std::to_string(config_.vignette.intensity) + "));\n";
        }

        src += "    fragColor = color;\n";
        src += "}\n";

        return src;
    }

private:
    PostProcessPipeline() = default;

    bool initialized_ = false;
    PostProcessConfig config_;
    std::vector<bool> enabledEffects_;
    mutable std::mutex mutex_;
};

} // namespace engine
