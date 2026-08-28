#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <android/log.h>

#define LOG_TAG_MS "MaterialSystem"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_MS(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_MS, __VA_ARGS__)
#else
#define LOGD_MS(...) do {} while(0)
#endif
#define LOGI_MS(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_MS, __VA_ARGS__)

// ============================================================================
// Material System
// Phase 56: PBR-inspired material system for Gamebryo
// Oblivion uses a simplified Blinn-Phong model with texture layers
// ============================================================================

namespace engine {

// Material blend mode
enum class MaterialBlendMode : uint8_t {
    OPAQUE = 0,
    ALPHA_TEST,
    ALPHA_BLEND,
    ADDITIVE,
    MULTIPLY
};

// Texture slot types (Oblivion NiTexturingProperty)
enum class TextureSlot : uint8_t {
    BASE = 0,           // Main diffuse texture
    DARK,               // Dark map (multiply)
    DETAIL,             // Detail texture (close-up)
    GLOSS,              // Specular/gloss map
    GLOW,               // Self-illumination/emissive
    BUMP,               // Normal/bump map
    DECAL,              // Decal overlay
    ENVIRONMENT,        // Environment/cube map
    COUNT
};

// Alpha test function
enum class AlphaFunc : uint8_t {
    ALWAYS = 0,
    NEVER,
    LESS,
    EQUAL,
    LESS_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_EQUAL
};

// Texture reference
struct TextureRef {
    uint32_t textureId = 0;         // GPU texture handle
    std::string filePath;           // Original file path (for hot reload)
    float uvOffset[2] = {0.0f, 0.0f};
    float uvScale[2] = {1.0f, 1.0f};
    float uvRotation = 0.0f;
    uint32_t uvSet = 0;             // Which UV channel
};

// Material properties (Oblivion NiMaterialProperty)
struct MaterialProperties {
    float diffuseColor[3] = {0.8f, 0.8f, 0.8f};
    float specularColor[3] = {1.0f, 1.0f, 1.0f};
    float emissiveColor[3] = {0.0f, 0.0f, 0.0f};
    float ambientColor[3] = {0.2f, 0.2f, 0.2f};
    float glossiness = 30.0f;       // Specular exponent
    float alpha = 1.0f;
    float emissiveMultiplier = 1.0f;
};

// ============================================================================
// Material - complete material definition
// ============================================================================

class Material {
public:
    Material() = default;
    explicit Material(const std::string& name) : name_(name) {}

    // --- Properties ---

    void setProperties(const MaterialProperties& props) { properties_ = props; }
    const MaterialProperties& getProperties() const { return properties_; }

    void setDiffuseColor(float r, float g, float b) {
        properties_.diffuseColor[0] = r;
        properties_.diffuseColor[1] = g;
        properties_.diffuseColor[2] = b;
    }

    void setAlpha(float alpha) { properties_.alpha = alpha; }
    float getAlpha() const { return properties_.alpha; }

    void setGlossiness(float gloss) { properties_.glossiness = gloss; }

    // --- Textures ---

    void setTexture(TextureSlot slot, const TextureRef& tex) {
        textures_[static_cast<size_t>(slot)] = tex;
        textureMask_ |= (1 << static_cast<int>(slot));
    }

    bool hasTexture(TextureSlot slot) const {
        return (textureMask_ & (1 << static_cast<int>(slot))) != 0;
    }

    const TextureRef& getTexture(TextureSlot slot) const {
        return textures_[static_cast<size_t>(slot)];
    }

    uint32_t getTextureMask() const { return textureMask_; }

    // --- Blend mode ---

    void setBlendMode(MaterialBlendMode mode) { blendMode_ = mode; }
    MaterialBlendMode getBlendMode() const { return blendMode_; }

    // --- Alpha test ---

    void setAlphaTest(bool enabled, AlphaFunc func = AlphaFunc::GREATER, float threshold = 0.5f) {
        alphaTestEnabled_ = enabled;
        alphaTestFunc_ = func;
        alphaTestThreshold_ = threshold;
    }

    bool isAlphaTestEnabled() const { return alphaTestEnabled_; }
    AlphaFunc getAlphaTestFunc() const { return alphaTestFunc_; }
    float getAlphaTestThreshold() const { return alphaTestThreshold_; }

    // --- Two-sided ---

    void setTwoSided(bool twoSided) { twoSided_ = twoSided; }
    bool isTwoSided() const { return twoSided_; }

    // --- Metadata ---

    void setName(const std::string& name) { name_ = name; }
    const std::string& getName() const { return name_; }

    void setShaderName(const std::string& shader) { shaderName_ = shader; }
    const std::string& getShaderName() const { return shaderName_; }

    // --- Shader key generation (for batching) ---

    uint64_t getSortKey() const {
        uint64_t key = 0;
        key |= static_cast<uint64_t>(blendMode_) & 0x7;
        key |= (static_cast<uint64_t>(textureMask_) & 0xFF) << 3;
        key |= (alphaTestEnabled_ ? 1ULL : 0ULL) << 11;
        key |= (twoSided_ ? 1ULL : 0ULL) << 12;
        // Include first texture ID for sorting
        if (hasTexture(TextureSlot::BASE)) {
            key |= (static_cast<uint64_t>(textures_[0].textureId) & 0xFFFFF) << 13;
        }
        return key;
    }

    // Generate GLSL fragment shader snippet for this material
    std::string generateShaderSnippet() const {
        std::string src;

        // Uniforms
        src += "uniform vec3 uDiffuseColor;\n";
        src += "uniform vec3 uSpecularColor;\n";
        src += "uniform vec3 uEmissiveColor;\n";
        src += "uniform float uGlossiness;\n";
        src += "uniform float uAlpha;\n";

        if (hasTexture(TextureSlot::BASE)) {
            src += "uniform sampler2D uBaseTexture;\n";
        }
        if (hasTexture(TextureSlot::BUMP)) {
            src += "uniform sampler2D uNormalMap;\n";
        }
        if (hasTexture(TextureSlot::GLOSS)) {
            src += "uniform sampler2D uSpecularMap;\n";
        }
        if (hasTexture(TextureSlot::GLOW)) {
            src += "uniform sampler2D uEmissiveMap;\n";
        }
        if (hasTexture(TextureSlot::DETAIL)) {
            src += "uniform sampler2D uDetailTexture;\n";
        }

        src += "\nvec4 computeMaterial(vec2 uv, vec3 normal, vec3 lightDir, vec3 viewDir) {\n";
        src += "    vec3 diff = uDiffuseColor;\n";
        src += "    vec3 spec = uSpecularColor;\n";
        src += "    float alpha = uAlpha;\n";

        // Base texture
        if (hasTexture(TextureSlot::BASE)) {
            src += "    vec4 baseTex = texture(uBaseTexture, uv);\n";
            src += "    diff *= baseTex.rgb;\n";
            src += "    alpha *= baseTex.a;\n";
        }

        // Detail texture
        if (hasTexture(TextureSlot::DETAIL)) {
            src += "    vec4 detailTex = texture(uDetailTexture, uv * 8.0);\n";
            src += "    diff *= detailTex.rgb * 2.0;\n";
        }

        // Normal mapping
        if (hasTexture(TextureSlot::BUMP)) {
            src += "    vec3 bumpNormal = texture(uNormalMap, uv).rgb * 2.0 - 1.0;\n";
            src += "    normal = normalize(normal + bumpNormal * 0.5);\n";
        }

        // Lighting (Blinn-Phong)
        src += "    float NdotL = max(dot(normal, lightDir), 0.0);\n";
        src += "    vec3 halfDir = normalize(lightDir + viewDir);\n";
        src += "    float NdotH = max(dot(normal, halfDir), 0.0);\n";

        // Specular
        if (hasTexture(TextureSlot::GLOSS)) {
            src += "    vec3 glossTex = texture(uSpecularMap, uv).rgb;\n";
            src += "    spec *= glossTex;\n";
        }
        src += "    float specPow = pow(NdotH, uGlossiness);\n";

        // Emissive
        src += "    vec3 emissive = uEmissiveColor;\n";
        if (hasTexture(TextureSlot::GLOW)) {
            src += "    emissive *= texture(uEmissiveMap, uv).rgb;\n";
        }

        // Final color
        src += "    vec3 color = diff * NdotL + spec * specPow + emissive;\n";

        // Alpha test
        if (alphaTestEnabled_) {
            src += "    if (alpha < " + std::to_string(alphaTestThreshold_) + ") discard;\n";
        }

        src += "    return vec4(color, alpha);\n";
        src += "}\n";

        return src;
    }

private:
    std::string name_;
    std::string shaderName_;
    MaterialProperties properties_;
    std::array<TextureRef, static_cast<size_t>(TextureSlot::COUNT)> textures_;
    uint32_t textureMask_ = 0;
    MaterialBlendMode blendMode_ = MaterialBlendMode::OPAQUE;
    bool alphaTestEnabled_ = false;
    AlphaFunc alphaTestFunc_ = AlphaFunc::GREATER;
    float alphaTestThreshold_ = 0.5f;
    bool twoSided_ = false;
};

// ============================================================================
// MaterialSystem - manages all materials
// ============================================================================

class MaterialSystem {
public:
    static MaterialSystem& instance() {
        static MaterialSystem inst;
        return inst;
    }

    void init() {
        std::lock_guard<std::mutex> lock(mutex_);
        materials_.clear();
        registerDefaults();
        initialized_ = true;
        LOGI_MS("MaterialSystem initialized with %zu default materials", materials_.size());
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        materials_.clear();
        initialized_ = false;
    }

    // Create material
    std::shared_ptr<Material> createMaterial(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto mat = std::make_shared<Material>(name);
        materials_[name] = mat;
        return mat;
    }

    // Find material by name
    std::shared_ptr<Material> findMaterial(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = materials_.find(name);
        if (it != materials_.end()) {
            return it->second;
        }
        return nullptr;
    }

    // Get or create (for ESM loading)
    std::shared_ptr<Material> getOrCreate(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = materials_.find(name);
        if (it != materials_.end()) {
            return it->second;
        }
        auto mat = std::make_shared<Material>(name);
        materials_[name] = mat;
        return mat;
    }

    // Get all materials sorted by sort key (for batching)
    std::vector<std::shared_ptr<Material>> getSortedMaterials() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::shared_ptr<Material>> sorted;
        for (const auto& [name, mat] : materials_) {
            sorted.push_back(mat);
        }
        std::sort(sorted.begin(), sorted.end(),
            [](const std::shared_ptr<Material>& a, const std::shared_ptr<Material>& b) {
                return a->getSortKey() < b->getSortKey();
            });
        return sorted;
    }

    size_t getMaterialCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return materials_.size();
    }

private:
    MaterialSystem() = default;

    bool initialized_ = false;
    std::unordered_map<std::string, std::shared_ptr<Material>> materials_;
    mutable std::mutex mutex_;

    void registerDefaults() {
        // Default opaque material
        auto defaultMat = std::make_shared<Material>("default");
        materials_["default"] = defaultMat;

        // Terrain material
        auto terrain = std::make_shared<Material>("terrain");
        terrain->setBlendMode(MaterialBlendMode::OPAQUE);
        terrain->setGlossiness(10.0f);
        terrain->setTwoSided(false);
        materials_["terrain"] = terrain;

        // Water material
        auto water = std::make_shared<Material>("water");
        water->setBlendMode(MaterialBlendMode::ALPHA_BLEND);
        water->setAlpha(0.7f);
        water->setGlossiness(128.0f);
        water->setTwoSided(true);
        materials_["water"] = water;

        // Vegetation (alpha tested)
        auto vegetation = std::make_shared<Material>("vegetation");
        vegetation->setBlendMode(MaterialBlendMode::ALPHA_TEST);
        vegetation->setAlphaTest(true, AlphaFunc::GREATER, 0.3f);
        vegetation->setTwoSided(true);
        vegetation->setGlossiness(5.0f);
        materials_["vegetation"] = vegetation;

        // Character skin
        auto skin = std::make_shared<Material>("skin");
        skin->setBlendMode(MaterialBlendMode::OPAQUE);
        skin->setGlossiness(20.0f);
        skin->setProperties({{0.8f, 0.6f, 0.5f}, {0.3f, 0.3f, 0.3f}, {0.0f, 0.0f, 0.0f},
                             {0.2f, 0.15f, 0.1f}, 20.0f, 1.0f, 0.0f});
        materials_["skin"] = skin;

        // Metal
        auto metal = std::make_shared<Material>("metal");
        metal->setBlendMode(MaterialBlendMode::OPAQUE);
        metal->setGlossiness(80.0f);
        metal->setProperties({{0.5f, 0.5f, 0.5f}, {0.9f, 0.9f, 0.9f}, {0.0f, 0.0f, 0.0f},
                              {0.1f, 0.1f, 0.1f}, 80.0f, 1.0f, 0.0f});
        materials_["metal"] = metal;

        // Glass (alpha blend)
        auto glass = std::make_shared<Material>("glass");
        glass->setBlendMode(MaterialBlendMode::ALPHA_BLEND);
        glass->setAlpha(0.3f);
        glass->setGlossiness(256.0f);
        glass->setTwoSided(true);
        materials_["glass"] = glass;

        // Emissive/magic glow
        auto glow = std::make_shared<Material>("glow");
        glow->setBlendMode(MaterialBlendMode::ADDITIVE);
        glow->setProperties({{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 1.0f},
                             {0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 2.0f});
        materials_["glow"] = glow;
    }
};

} // namespace engine
