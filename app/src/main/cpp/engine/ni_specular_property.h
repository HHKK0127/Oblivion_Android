#pragma once

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <android/log.h>

#define LOG_TAG "NiSpecularProperty"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// ============================================================================
// NiSpecularProperty - Gamebryo specular lighting property
// Controls specular intensity, glossiness, and color for materials
// ============================================================================

namespace gamebryo {

// Specular calculation mode
enum class SpecularMode : uint32_t {
    BLINN_PHONG = 0,      // Standard Blinn-Phong specular
    PHONG = 1,            // Classic Phong specular
    COOK_TORRANCE = 2,    // Cook-Torrance microfacet model
    WARD = 3,             // Ward anisotropic model
    GAUSSIAN = 4          // Gaussian distribution
};

// Specular property configuration
struct SpecularConfig {
    float specularPower = 32.0f;           // Glossiness (higher = tighter highlight)
    float specularIntensity = 1.0f;        // Overall specular strength
    float specularMultiplier = 1.0f;       // Material multiplier
    glm::vec3 specularColor = glm::vec3(1.0f, 1.0f, 1.0f);  // Specular tint
    
    SpecularMode mode = SpecularMode::BLINN_PHONG;
    
    // Fresnel effect
    bool useFresnel = false;
    float fresnelPower = 5.0f;
    float fresnelScale = 1.0f;
    glm::vec3 fresnelColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    // Anisotropy (for Ward model)
    float anisotropy = 0.0f;               // 0 = isotropic, 1 = full anisotropic
    glm::vec3 anisotropyDirection = glm::vec3(1.0f, 0.0f, 0.0f);
    
    // Environment reflection
    bool useEnvironmentMap = false;
    float environmentMapScale = 1.0f;
    
    bool operator==(const SpecularConfig& other) const {
        return specularPower == other.specularPower &&
               specularIntensity == other.specularIntensity &&
               specularMultiplier == other.specularMultiplier &&
               specularColor == other.specularColor &&
               mode == other.mode;
    }
    
    bool operator!=(const SpecularConfig& other) const {
        return !(*this == other);
    }
};

// Preset specular configurations
namespace SpecularPresets {
    // Metal presets
    inline SpecularConfig Metal() {
        SpecularConfig config;
        config.specularPower = 128.0f;
        config.specularIntensity = 2.0f;
        config.specularColor = glm::vec3(0.98f, 0.97f, 0.95f);
        config.useFresnel = true;
        config.fresnelPower = 3.0f;
        return config;
    }
    
    inline SpecularConfig Gold() {
        SpecularConfig config = Metal();
        config.specularColor = glm::vec3(1.0f, 0.85f, 0.45f);
        return config;
    }
    
    inline SpecularConfig Silver() {
        SpecularConfig config = Metal();
        config.specularColor = glm::vec3(0.97f, 0.97f, 1.0f);
        return config;
    }
    
    inline SpecularConfig Copper() {
        SpecularConfig config = Metal();
        config.specularColor = glm::vec3(0.95f, 0.70f, 0.55f);
        return config;
    }
    
    // Non-metal presets
    inline SpecularConfig Plastic() {
        SpecularConfig config;
        config.specularPower = 64.0f;
        config.specularIntensity = 0.5f;
        config.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
        return config;
    }
    
    inline SpecularConfig GlossyPlastic() {
        SpecularConfig config = Plastic();
        config.specularPower = 128.0f;
        config.specularIntensity = 0.8f;
        return config;
    }
    
    inline SpecularConfig Rubber() {
        SpecularConfig config;
        config.specularPower = 8.0f;
        config.specularIntensity = 0.2f;
        config.specularColor = glm::vec3(0.8f);
        return config;
    }
    
    // Organic presets
    inline SpecularConfig Skin() {
        SpecularConfig config;
        config.specularPower = 16.0f;
        config.specularIntensity = 0.3f;
        config.specularColor = glm::vec3(1.0f, 0.95f, 0.90f);
        config.useFresnel = true;
        config.fresnelPower = 2.0f;
        return config;
    }
    
    inline SpecularConfig WetSkin() {
        SpecularConfig config = Skin();
        config.specularIntensity = 0.8f;
        config.specularPower = 32.0f;
        return config;
    }
    
    inline SpecularConfig Hair() {
        SpecularConfig config;
        config.specularPower = 24.0f;
        config.specularIntensity = 0.4f;
        config.mode = SpecularMode::WARD;
        config.anisotropy = 0.8f;
        return config;
    }
    
    // Stone/terrain presets
    inline SpecularConfig Stone() {
        SpecularConfig config;
        config.specularPower = 4.0f;
        config.specularIntensity = 0.1f;
        config.specularColor = glm::vec3(0.9f);
        return config;
    }
    
    inline SpecularConfig WetStone() {
        SpecularConfig config = Stone();
        config.specularPower = 16.0f;
        config.specularIntensity = 0.3f;
        return config;
    }
    
    // Fabric presets
    inline SpecularConfig Fabric() {
        SpecularConfig config;
        config.specularPower = 2.0f;
        config.specularIntensity = 0.05f;
        config.specularColor = glm::vec3(0.85f);
        return config;
    }
    
    inline SpecularConfig Silk() {
        SpecularConfig config;
        config.specularPower = 48.0f;
        config.specularIntensity = 0.6f;
        config.mode = SpecularMode::WARD;
        config.anisotropy = 0.5f;
        return config;
    }
    
    // Special presets
    inline SpecularConfig Gem() {
        SpecularConfig config;
        config.specularPower = 256.0f;
        config.specularIntensity = 1.5f;
        config.specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
        config.useEnvironmentMap = true;
        config.environmentMapScale = 1.5f;
        return config;
    }
    
    inline SpecularConfig Glass() {
        SpecularConfig config;
        config.specularPower = 512.0f;
        config.specularIntensity = 1.0f;
        config.useFresnel = true;
        config.fresnelPower = 2.0f;
        config.useEnvironmentMap = true;
        return config;
    }
    
    // Default
    inline SpecularConfig Default() {
        return SpecularConfig();
    }
}

// NiSpecularProperty - Main specular property class
class NiSpecularProperty {
public:
    NiSpecularProperty();
    ~NiSpecularProperty();
    
    // Initialization
    bool initialize(const SpecularConfig& config);
    void shutdown();
    
    // Configuration
    void setConfig(const SpecularConfig& config);
    const SpecularConfig& getConfig() const { return config_; }
    
    // Parameter setters
    void setSpecularPower(float power);
    void setSpecularIntensity(float intensity);
    void setSpecularMultiplier(float multiplier);
    void setSpecularColor(const glm::vec3& color);
    void setSpecularMode(SpecularMode mode);
    void setFresnelEnabled(bool enabled);
    void setFresnelParams(float power, float scale, const glm::vec3& color);
    void setAnisotropy(float anisotropy, const glm::vec3& direction);
    void setEnvironmentMapEnabled(bool enabled, float scale = 1.0f);
    
    // Getters
    float getSpecularPower() const { return config_.specularPower; }
    float getSpecularIntensity() const { return config_.specularIntensity; }
    const glm::vec3& getSpecularColor() const { return config_.specularColor; }
    
    // Apply to shader uniforms
    void applyUniforms(uint32_t program, const std::string& uniformPrefix = "u_specular") const;
    
    // Generate GLSL snippet for shader
    std::string generateGLSL(const std::string& variablePrefix = "specular") const;
    
    // Preset application
    void applyPreset(const SpecularConfig& preset);
    void applyMetalPreset();
    void applyPlasticPreset();
    void applySkinPreset();
    void applyStonePreset();
    void applyFabricPreset();
    void applyGemPreset();
    void applyGlassPreset();
    
    // Calculate specular at a point (CPU fallback)
    float calculateSpecular(const glm::vec3& normal,
                            const glm::vec3& viewDir,
                            const glm::vec3& lightDir) const;
    
    // Material name
    void setName(const std::string& name) { name_ = name; }
    const std::string& getName() const { return name_; }

private:
    SpecularConfig config_;
    std::string name_;
    bool initialized_ = false;
    
    // Internal calculation methods
    float calculateBlinnPhong(const glm::vec3& normal,
                              const glm::vec3& viewDir,
                              const glm::vec3& lightDir) const;
    float calculatePhong(const glm::vec3& normal,
                         const glm::vec3& viewDir,
                         const glm::vec3& lightDir) const;
    float calculateFresnel(float cosTheta) const;
};

// SpecularPropertyManager - Global specular property management
class SpecularPropertyManager {
public:
    static SpecularPropertyManager& getInstance();
    
    // Property registration
    void registerProperty(const std::string& name, const NiSpecularProperty& prop);
    void unregisterProperty(const std::string& name);
    
    // Property queries
    NiSpecularProperty* getProperty(const std::string& name);
    const NiSpecularProperty* getProperty(const std::string& name) const;
    bool hasProperty(const std::string& name) const;
    
    // Bulk operations
    std::vector<std::string> getPropertyNames() const;
    void clear();
    
    // Apply preset by name
    bool applyPreset(const std::string& propertyName, const std::string& presetName);

private:
    SpecularPropertyManager() = default;
    ~SpecularPropertyManager() = default;
    SpecularPropertyManager(const SpecularPropertyManager&) = delete;
    SpecularPropertyManager& operator=(const SpecularPropertyManager&) = delete;
    
    std::unordered_map<std::string, NiSpecularProperty> properties_;
};

// ============================================================================
// Implementation
// ============================================================================

inline NiSpecularProperty::NiSpecularProperty() = default;
inline NiSpecularProperty::~NiSpecularProperty() { shutdown(); }

inline bool NiSpecularProperty::initialize(const SpecularConfig& config) {
    config_ = config;
    initialized_ = true;
    LOGI("NiSpecularProperty initialized (power: %.1f, intensity: %.2f)",
         config_.specularPower, config_.specularIntensity);
    return true;
}

inline void NiSpecularProperty::shutdown() {
    initialized_ = false;
}

inline void NiSpecularProperty::setConfig(const SpecularConfig& config) {
    config_ = config;
}

inline void NiSpecularProperty::setSpecularPower(float power) {
    config_.specularPower = glm::max(1.0f, power);
}

inline void NiSpecularProperty::setSpecularIntensity(float intensity) {
    config_.specularIntensity = glm::max(0.0f, intensity);
}

inline void NiSpecularProperty::setSpecularMultiplier(float multiplier) {
    config_.specularMultiplier = glm::max(0.0f, multiplier);
}

inline void NiSpecularProperty::setSpecularColor(const glm::vec3& color) {
    config_.specularColor = glm::clamp(color, glm::vec3(), glm::vec3(1.0f, 1.0f, 1.0f));
}

inline void NiSpecularProperty::setSpecularMode(SpecularMode mode) {
    config_.mode = mode;
}

inline void NiSpecularProperty::setFresnelEnabled(bool enabled) {
    config_.useFresnel = enabled;
}

inline void NiSpecularProperty::setFresnelParams(float power, float scale, const glm::vec3& color) {
    config_.fresnelPower = glm::max(0.1f, power);
    config_.fresnelScale = scale;
    config_.fresnelColor = glm::clamp(color, glm::vec3(), glm::vec3(1.0f, 1.0f, 1.0f));
}

inline void NiSpecularProperty::setAnisotropy(float anisotropy, const glm::vec3& direction) {
    config_.anisotropy = glm::clamp(anisotropy, 0.0f, 1.0f);
    config_.anisotropyDirection = glm::normalize(direction);
}

inline void NiSpecularProperty::setEnvironmentMapEnabled(bool enabled, float scale) {
    config_.useEnvironmentMap = enabled;
    config_.environmentMapScale = scale;
}

inline void NiSpecularProperty::applyUniforms(uint32_t program, const std::string& uniformPrefix) const {
    if (!initialized_) return;
    
    GLint loc;
    std::string prefix = uniformPrefix + ".";
    
    loc = glGetUniformLocation(program, (prefix + "power").c_str());
    if (loc >= 0) glUniform1f(loc, config_.specularPower);
    
    loc = glGetUniformLocation(program, (prefix + "intensity").c_str());
    if (loc >= 0) glUniform1f(loc, config_.specularIntensity * config_.specularMultiplier);
    
    loc = glGetUniformLocation(program, (prefix + "color").c_str());
    if (loc >= 0) glUniform3fv(loc, 1, &config_.specularColor[0]);
    
    loc = glGetUniformLocation(program, (prefix + "mode").c_str());
    if (loc >= 0) glUniform1i(loc, static_cast<int>(config_.mode));
    
    loc = glGetUniformLocation(program, (prefix + "useFresnel").c_str());
    if (loc >= 0) glUniform1i(loc, config_.useFresnel ? 1 : 0);
    
    if (config_.useFresnel) {
        loc = glGetUniformLocation(program, (prefix + "fresnelPower").c_str());
        if (loc >= 0) glUniform1f(loc, config_.fresnelPower);
        
        loc = glGetUniformLocation(program, (prefix + "fresnelScale").c_str());
        if (loc >= 0) glUniform1f(loc, config_.fresnelScale);
        
        loc = glGetUniformLocation(program, (prefix + "fresnelColor").c_str());
        if (loc >= 0) glUniform3fv(loc, 1, &config_.fresnelColor[0]);
    }
}

inline std::string NiSpecularProperty::generateGLSL(const std::string& variablePrefix) const {
    std::string vp = variablePrefix;
    std::string glsl = R"(
// Specular calculation
float )" + vp + R"(_calc(vec3 N, vec3 V, vec3 L, float power) {
    vec3 H = normalize(V + L);
    float NdotH = max(dot(N, H), 0.0);
    return pow(NdotH, power);
}
)";
    
    if (config_.useFresnel) {
        glsl += R"(
float )" + vp + R"(_fresnel(float cosTheta, float power) {
    return pow(1.0 - cosTheta, power);
}
)";
    }
    
    return glsl;
}

inline void NiSpecularProperty::applyPreset(const SpecularConfig& preset) {
    config_ = preset;
    LOGD("Applied specular preset (power: %.1f, intensity: %.2f)",
         config_.specularPower, config_.specularIntensity);
}

inline void NiSpecularProperty::applyMetalPreset() { applyPreset(SpecularPresets::Metal()); }
inline void NiSpecularProperty::applyPlasticPreset() { applyPreset(SpecularPresets::Plastic()); }
inline void NiSpecularProperty::applySkinPreset() { applyPreset(SpecularPresets::Skin()); }
inline void NiSpecularProperty::applyStonePreset() { applyPreset(SpecularPresets::Stone()); }
inline void NiSpecularProperty::applyFabricPreset() { applyPreset(SpecularPresets::Fabric()); }
inline void NiSpecularProperty::applyGemPreset() { applyPreset(SpecularPresets::Gem()); }
inline void NiSpecularProperty::applyGlassPreset() { applyPreset(SpecularPresets::Glass()); }

inline float NiSpecularProperty::calculateSpecular(const glm::vec3& normal,
                                                    const glm::vec3& viewDir,
                                                    const glm::vec3& lightDir) const {
    switch (config_.mode) {
        case SpecularMode::BLINN_PHONG:
            return calculateBlinnPhong(normal, viewDir, lightDir);
        case SpecularMode::PHONG:
            return calculatePhong(normal, viewDir, lightDir);
        default:
            return calculateBlinnPhong(normal, viewDir, lightDir);
    }
}

inline float NiSpecularProperty::calculateBlinnPhong(const glm::vec3& normal,
                                                      const glm::vec3& viewDir,
                                                      const glm::vec3& lightDir) const {
    glm::vec3 halfDir = glm::normalize(viewDir + lightDir);
    float NdotH = glm::max(glm::dot(normal, halfDir), 0.0f);
    float specular = powf(NdotH, config_.specularPower);
    
    if (config_.useFresnel) {
        float cosTheta = glm::max(glm::dot(viewDir, normal), 0.0f);
        specular *= calculateFresnel(cosTheta);
    }
    
    return specular * config_.specularIntensity * config_.specularMultiplier;
}

inline float NiSpecularProperty::calculatePhong(const glm::vec3& normal,
                                                 const glm::vec3& viewDir,
                                                 const glm::vec3& lightDir) const {
    glm::vec3 reflectDir = glm::reflect(-lightDir, normal);
    float RdotV = glm::max(glm::dot(reflectDir, viewDir), 0.0f);
    return powf(RdotV, config_.specularPower) * config_.specularIntensity * config_.specularMultiplier;
}

inline float NiSpecularProperty::calculateFresnel(float cosTheta) const {
    return config_.fresnelScale * powf(1.0f - cosTheta, config_.fresnelPower);
}

// SpecularPropertyManager implementation
inline SpecularPropertyManager& SpecularPropertyManager::getInstance() {
    static SpecularPropertyManager instance;
    return instance;
}

inline void SpecularPropertyManager::registerProperty(const std::string& name, const NiSpecularProperty& prop) {
    properties_[name] = prop;
    LOGD("Registered specular property '%s'", name.c_str());
}

inline void SpecularPropertyManager::unregisterProperty(const std::string& name) {
    properties_.erase(name);
}

inline NiSpecularProperty* SpecularPropertyManager::getProperty(const std::string& name) {
    auto it = properties_.find(name);
    if (it != properties_.end()) return &it->second;
    return nullptr;
}

inline const NiSpecularProperty* SpecularPropertyManager::getProperty(const std::string& name) const {
    auto it = properties_.find(name);
    if (it != properties_.end()) return &it->second;
    return nullptr;
}

inline bool SpecularPropertyManager::hasProperty(const std::string& name) const {
    return properties_.find(name) != properties_.end();
}

inline std::vector<std::string> SpecularPropertyManager::getPropertyNames() const {
    std::vector<std::string> names;
    for (const auto& [name, prop] : properties_) {
        names.push_back(name);
    }
    return names;
}

inline void SpecularPropertyManager::clear() {
    properties_.clear();
}

} // namespace gamebryo
