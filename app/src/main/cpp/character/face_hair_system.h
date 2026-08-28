#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <cmath>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <glm/glm.hpp>
#include <android/log.h>

// ============================================================================
// Phase 55: Face Hair System
//
// Lightweight hair rendering for NPCs. Rather than full per-strand
// simulation, hair is represented as a small set of alpha-tested geometry
// strips ("cards") parameterized per style, colored from the NPC's
// FaceTexture hair color, and hidden automatically when a helmet covering
// the head is equipped. Two LOD tiers are supported: full strand strips up
// close, and a simplified low-strip-count version further away.
// ============================================================================

#define LOG_TAG_FHS "FaceHairSystem"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_FHS(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_FHS, __VA_ARGS__)
#else
#define LOGD_FHS(...) do {} while(0)
#endif
#define LOGI_FHS(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_FHS, __VA_ARGS__)
#define LOGW_FHS(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_FHS, __VA_ARGS__)
#define LOGE_FHS(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_FHS, __VA_ARGS__)

namespace facegen {

// ----------------------------------------------------------------------------
// Hair styles
// ----------------------------------------------------------------------------
enum class HairStyle : uint8_t {
    BALD = 0,
    SHORT,
    MEDIUM,
    LONG,
    PONYTAIL,
    BRAIDS,
    MOHAWK,
    COUNT
};

inline const char* HairStyleToString(HairStyle style) {
    switch (style) {
        case HairStyle::BALD:     return "BALD";
        case HairStyle::SHORT:    return "SHORT";
        case HairStyle::MEDIUM:   return "MEDIUM";
        case HairStyle::LONG:     return "LONG";
        case HairStyle::PONYTAIL: return "PONYTAIL";
        case HairStyle::BRAIDS:   return "BRAIDS";
        case HairStyle::MOHAWK:   return "MOHAWK";
        default:                  return "UNKNOWN";
    }
}

// Parameters describing how a hair style is generated as alpha-tested strips.
struct HairStyleParams {
    int strandCount = 0;     // number of geometry strips ("card" strands)
    float length = 0.0f;     // 0..1 normalized length (1 = shoulder length+)
    float curl = 0.0f;       // 0 = straight, 1 = tightly curled
    float thickness = 0.0f;  // strip width scale
    float coverage = 0.0f;   // 0..1 fraction of scalp covered (0 for bald)
};

class HairStyleTable {
public:
    static const HairStyleParams& Get(HairStyle style) {
        static const std::array<HairStyleParams, static_cast<size_t>(HairStyle::COUNT)> table = BuildTable();
        size_t idx = static_cast<size_t>(style);
        if (idx >= table.size()) idx = 0;
        return table[idx];
    }

private:
    static std::array<HairStyleParams, static_cast<size_t>(HairStyle::COUNT)> BuildTable() {
        std::array<HairStyleParams, static_cast<size_t>(HairStyle::COUNT)> t{};

        t[static_cast<size_t>(HairStyle::BALD)] = {0, 0.0f, 0.0f, 0.0f, 0.0f};
        t[static_cast<size_t>(HairStyle::SHORT)] = {24, 0.15f, 0.2f, 0.6f, 1.0f};
        t[static_cast<size_t>(HairStyle::MEDIUM)] = {40, 0.4f, 0.3f, 0.55f, 1.0f};
        t[static_cast<size_t>(HairStyle::LONG)] = {64, 0.8f, 0.25f, 0.5f, 1.0f};
        t[static_cast<size_t>(HairStyle::PONYTAIL)] = {56, 0.9f, 0.15f, 0.55f, 0.9f};
        t[static_cast<size_t>(HairStyle::BRAIDS)] = {48, 0.85f, 0.6f, 0.65f, 0.9f};
        t[static_cast<size_t>(HairStyle::MOHAWK)] = {18, 0.5f, 0.1f, 0.7f, 0.3f};

        return t;
    }
};

// ----------------------------------------------------------------------------
// A single hair strand strip: a thin quad-strip billboard/card, defined by a
// root position (scalp-relative, normalized [-1,1] on a UV-like disc), a
// growth direction, and per-strand length/curl jitter for visual variety.
// ----------------------------------------------------------------------------
struct HairStrand {
    glm::vec2 scalpRoot = glm::vec2(0.0f, 0.0f); // position on scalp parameterization
    glm::vec3 growthDir = glm::vec3(0.0f, 1.0f, 0.0f);
    float lengthScale = 1.0f;
    float curlPhase = 0.0f;
};

// ----------------------------------------------------------------------------
// Generated hair mesh data (CPU-side description consumed by the renderer).
// Kept intentionally simple: a flat list of strands plus shared style params.
// ----------------------------------------------------------------------------
struct HairMeshData {
    HairStyle style = HairStyle::BALD;
    std::vector<HairStrand> strands;
    bool isLowLod = false; // true if this is the simplified far-LOD version
};

// ----------------------------------------------------------------------------
// FaceHairSystem
//
// Generates and caches simplified hair "strip" geometry per style, resolves
// hair color from the owning face's hair color, and decides visibility based
// on equipped headgear + distance-based LOD.
// ----------------------------------------------------------------------------
class FaceHairSystem {
public:
    FaceHairSystem() = default;

    // Distance (meters) beyond which the simplified/low-strand-count strip
    // set is used instead of the full style strand count.
    static constexpr float kFullLodMaxDistance = 10.0f;

    // Generates (or returns a cached copy of) the strand layout for a style,
    // at either full or reduced density depending on distance to camera.
    const HairMeshData& GetOrGenerate(HairStyle style, float distanceToCamera) {
        bool lowLod = distanceToCamera > kFullLodMaxDistance;
        uint64_t key = MakeCacheKey(style, lowLod);

        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }

        HairMeshData data = Generate(style, lowLod);
        auto insertedIt = cache_.emplace(key, std::move(data)).first;
        return insertedIt->second;
    }

    // Determines whether hair should be rendered at all for this NPC this
    // frame, considering equipped headgear and the style's bald-ness.
    bool ShouldRenderHair(HairStyle style, bool helmetEquipped, bool helmetCoversHair) const {
        if (style == HairStyle::BALD) {
            return false;
        }
        if (helmetEquipped && helmetCoversHair) {
            return false;
        }
        return true;
    }

    // Resolves the final hair color to use for shading, applying a small
    // graying blend if requested (e.g. driven by FaceRacePresets age curve).
    static glm::vec3 ResolveHairColor(const glm::vec3& baseHairColor, float graynessFactor) {
        graynessFactor = std::clamp(graynessFactor, 0.0f, 1.0f);
        static const glm::vec3 grayColor = glm::vec3(0.75f, 0.75f, 0.75f);
        return baseHairColor + (grayColor - baseHairColor) * graynessFactor;
    }

    void ClearCache() { cache_.clear(); }
    size_t GetCacheSize() const { return cache_.size(); }

private:
    static uint64_t MakeCacheKey(HairStyle style, bool lowLod) {
        return (static_cast<uint64_t>(style) << 1) | (lowLod ? 1u : 0u);
    }

    HairMeshData Generate(HairStyle style, bool lowLod) const {
        HairMeshData data;
        data.style = style;
        data.isLowLod = lowLod;

        const HairStyleParams& params = HairStyleTable::Get(style);
        int strandCount = params.strandCount;
        if (lowLod) {
            // Reduce strand count for distant rendering; alpha-tested strips
            // widen slightly (handled by the shader/thickness scalar) to
            // compensate for coverage loss.
            strandCount = std::max(4, strandCount / 4);
        }

        data.strands.reserve(static_cast<size_t>(strandCount));
        for (int i = 0; i < strandCount; ++i) {
            HairStrand strand = GenerateStrand(style, params, i, strandCount);
            data.strands.push_back(strand);
        }

        LOGD_FHS("Generated %s hair mesh: style=%s strands=%d",
                 lowLod ? "LOW-LOD" : "FULL", HairStyleToString(style), strandCount);

        return data;
    }

    HairStrand GenerateStrand(HairStyle style, const HairStyleParams& params,
                               int index, int total) const {
        HairStrand strand;

        // Deterministic pseudo-random distribution across the scalp so hair
        // is stable frame-to-frame without needing to store an RNG per NPC.
        float angle = (static_cast<float>(index) / std::max(1, total)) * 6.2831853f;
        float radius = 0.3f + 0.6f * Fract(std::sin(static_cast<float>(index) * 12.9898f) * 43758.5453f);

        switch (style) {
            case HairStyle::MOHAWK:
                // Concentrate strands along the central scalp line only.
                strand.scalpRoot = glm::vec2(0.0f, radius - 0.3f);
                break;
            case HairStyle::PONYTAIL:
                // Bias roots toward the back of the scalp, then the growth
                // direction sweeps down/back to form the tail shape.
                strand.scalpRoot = glm::vec2(std::cos(angle) * radius * 0.5f, -0.4f + radius * 0.2f);
                strand.growthDir = glm::vec3(0.0f, -0.6f, -0.8f);
                break;
            default:
                strand.scalpRoot = glm::vec2(std::cos(angle) * radius, std::sin(angle) * radius);
                break;
        }

        if (style != HairStyle::PONYTAIL) {
            strand.growthDir = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        strand.lengthScale = params.length * (0.85f + 0.3f * Fract(static_cast<float>(index) * 0.618f));
        strand.curlPhase = params.curl * static_cast<float>(index);

        return strand;
    }

    static float Fract(float v) {
        return v - std::floor(v);
    }

    std::unordered_map<uint64_t, HairMeshData> cache_;
};

} // namespace facegen
