#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <glm/glm.hpp>
#include <android/log.h>

// ============================================================================
// Phase 55: Face Race Presets
//
// Race-specific face generation presets for all playable/NPC races in
// Oblivion: base shape offsets, skin/hair/eye color palettes, gender
// modifiers, age curves, and bounded random variation. Also provides an
// integration point for loading overrides from ESM RACE records.
// ============================================================================

#define LOG_TAG_FRP "FaceRacePresets"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_FRP(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_FRP, __VA_ARGS__)
#else
#define LOGD_FRP(...) do {} while(0)
#endif
#define LOGI_FRP(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_FRP, __VA_ARGS__)
#define LOGW_FRP(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_FRP, __VA_ARGS__)
#define LOGE_FRP(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_FRP, __VA_ARGS__)

namespace facegen {

// ----------------------------------------------------------------------------
// Race identifiers
// ----------------------------------------------------------------------------
enum class RaceType : uint8_t {
    IMPERIAL = 0,
    BRETON,
    NORD,
    REDGUARD,
    DARK_ELF,
    HIGH_ELF,
    WOOD_ELF,
    ORC,
    KHAJIIT,
    ARGONIAN,
    COUNT
};

inline const char* RaceTypeToString(RaceType r) {
    switch (r) {
        case RaceType::IMPERIAL:  return "Imperial";
        case RaceType::BRETON:    return "Breton";
        case RaceType::NORD:      return "Nord";
        case RaceType::REDGUARD:  return "Redguard";
        case RaceType::DARK_ELF:  return "DarkElf";
        case RaceType::HIGH_ELF:  return "HighElf";
        case RaceType::WOOD_ELF:  return "WoodElf";
        case RaceType::ORC:       return "Orc";
        case RaceType::KHAJIIT:   return "Khajiit";
        case RaceType::ARGONIAN:  return "Argonian";
        default:                  return "Unknown";
    }
}

enum class Gender : uint8_t { MALE = 0, FEMALE = 1 };

// ----------------------------------------------------------------------------
// Minimal shape descriptor used by presets. This intentionally mirrors only
// the subset of FaceGenMorpher::FaceShape fields relevant to race presets so
// this header stays self-contained (no include of face_gen_morpher.h). Any
// consumer can copy these fields onto the real FaceShape struct field-by-field.
// ----------------------------------------------------------------------------
struct RaceFaceShape {
    float headWidth = 0.5f;
    float headHeight = 0.5f;
    float headDepth = 0.5f;

    float noseWidth = 0.5f;
    float noseHeight = 0.5f;
    float noseLength = 0.5f;

    float eyeWidth = 0.5f;
    float eyeHeight = 0.5f;
    float eyeSeparation = 0.5f;

    float mouthWidth = 0.5f;
    float jawWidth = 0.5f;
    float jawAngle = 0.5f;

    float cheekboneHeight = 0.5f;
    float chinHeight = 0.5f;
    float chinWidth = 0.5f;

    float earSize = 0.5f;
    float earAngle = 0.5f;
};

inline RaceFaceShape LerpShape(const RaceFaceShape& a, const RaceFaceShape& b, float t) {
    RaceFaceShape out;
    out.headWidth = a.headWidth + (b.headWidth - a.headWidth) * t;
    out.headHeight = a.headHeight + (b.headHeight - a.headHeight) * t;
    out.headDepth = a.headDepth + (b.headDepth - a.headDepth) * t;
    out.noseWidth = a.noseWidth + (b.noseWidth - a.noseWidth) * t;
    out.noseHeight = a.noseHeight + (b.noseHeight - a.noseHeight) * t;
    out.noseLength = a.noseLength + (b.noseLength - a.noseLength) * t;
    out.eyeWidth = a.eyeWidth + (b.eyeWidth - a.eyeWidth) * t;
    out.eyeHeight = a.eyeHeight + (b.eyeHeight - a.eyeHeight) * t;
    out.eyeSeparation = a.eyeSeparation + (b.eyeSeparation - a.eyeSeparation) * t;
    out.mouthWidth = a.mouthWidth + (b.mouthWidth - a.mouthWidth) * t;
    out.jawWidth = a.jawWidth + (b.jawWidth - a.jawWidth) * t;
    out.jawAngle = a.jawAngle + (b.jawAngle - a.jawAngle) * t;
    out.cheekboneHeight = a.cheekboneHeight + (b.cheekboneHeight - a.cheekboneHeight) * t;
    out.chinHeight = a.chinHeight + (b.chinHeight - a.chinHeight) * t;
    out.chinWidth = a.chinWidth + (b.chinWidth - a.chinWidth) * t;
    out.earSize = a.earSize + (b.earSize - a.earSize) * t;
    out.earAngle = a.earAngle + (b.earAngle - a.earAngle) * t;
    return out;
}

// ----------------------------------------------------------------------------
// Color range: inclusive [min, max] per channel, used for skin/hair/eye color
// palettes. Sampling picks a uniformly random point within the range.
// ----------------------------------------------------------------------------
struct ColorRange {
    glm::vec3 minColor = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 maxColor = glm::vec3(1.0f, 1.0f, 1.0f);
};

// A palette is a small set of representative color ranges (e.g. distinct
// hair color "swatches" rather than one huge continuous range).
using ColorPalette = std::vector<ColorRange>;

// ----------------------------------------------------------------------------
// Age curve: describes how a shape drifts from young adult to elderly.
// Sampled by normalized age t in [0,1] where 0 = young adult, 1 = elderly.
// ----------------------------------------------------------------------------
struct AgeCurvePoint {
    float t = 0.0f; // 0 = young adult, 0.5 = middle age, 1 = elderly
    float skinSag = 0.0f;        // wrinkle/sag morph weight
    float cheekHollow = 0.0f;    // cheek hollowing weight
    float browDroop = 0.0f;      // brow lowering weight
    float grayness = 0.0f;       // 0 = natural hair color, 1 = fully gray
};

class AgeCurve {
public:
    AgeCurve() {
        points_ = {
            {0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.5f, 0.15f, 0.1f, 0.05f, 0.15f},
            {0.75f, 0.4f, 0.3f, 0.2f, 0.55f},
            {1.0f, 0.75f, 0.6f, 0.4f, 0.9f},
        };
    }

    AgeCurvePoint Sample(float t) const {
        t = std::clamp(t, 0.0f, 1.0f);
        for (size_t i = 0; i + 1 < points_.size(); ++i) {
            const auto& a = points_[i];
            const auto& b = points_[i + 1];
            if (t >= a.t && t <= b.t) {
                float span = b.t - a.t;
                float localT = span > 1e-5f ? (t - a.t) / span : 0.0f;
                AgeCurvePoint out;
                out.t = t;
                out.skinSag = a.skinSag + (b.skinSag - a.skinSag) * localT;
                out.cheekHollow = a.cheekHollow + (b.cheekHollow - a.cheekHollow) * localT;
                out.browDroop = a.browDroop + (b.browDroop - a.browDroop) * localT;
                out.grayness = a.grayness + (b.grayness - a.grayness) * localT;
                return out;
            }
        }
        return points_.back();
    }

private:
    std::vector<AgeCurvePoint> points_;
};

// ----------------------------------------------------------------------------
// Gender modifier: additive offsets applied on top of the race base shape.
// ----------------------------------------------------------------------------
struct GenderModifier {
    float jawWidthDelta = 0.0f;
    float jawAngleDelta = 0.0f;
    float cheekboneHeightDelta = 0.0f;
    float browRidgeDelta = 0.0f; // reuses headHeight as a stand-in brow ridge proxy
    float chinWidthDelta = 0.0f;
    float noseWidthDelta = 0.0f;
};

// ----------------------------------------------------------------------------
// Full race preset definition.
// ----------------------------------------------------------------------------
struct RacePreset {
    RaceType race = RaceType::IMPERIAL;
    std::string name;

    RaceFaceShape baseShapeMale;
    RaceFaceShape baseShapeFemale;

    GenderModifier maleModifier;
    GenderModifier femaleModifier;

    ColorPalette skinTones;
    ColorPalette hairColors;
    ColorPalette eyeColors;

    AgeCurve ageCurve;

    // Per-parameter random variation bound, e.g. 0.10 = +/-10%.
    float variationBound = 0.10f;
};

// ----------------------------------------------------------------------------
// FaceRacePresets singleton
// ----------------------------------------------------------------------------
class FaceRacePresets {
public:
    static FaceRacePresets& Instance() {
        static FaceRacePresets instance;
        return instance;
    }

    const RacePreset& GetPreset(RaceType race) const {
        size_t idx = static_cast<size_t>(race);
        if (idx >= presets_.size()) {
            idx = 0;
        }
        return presets_[idx];
    }

    // Applies race + gender + normalized age [0,1] to produce a base shape,
    // without random variation. Callers wanting variation should call
    // GenerateRandomShape instead.
    RaceFaceShape BuildBaseShape(RaceType race, Gender gender, float normalizedAge) const {
        const RacePreset& preset = GetPreset(race);
        RaceFaceShape shape = (gender == Gender::MALE) ? preset.baseShapeMale : preset.baseShapeFemale;
        const GenderModifier& mod = (gender == Gender::MALE) ? preset.maleModifier : preset.femaleModifier;

        shape.jawWidth = std::clamp(shape.jawWidth + mod.jawWidthDelta, 0.0f, 1.0f);
        shape.jawAngle = std::clamp(shape.jawAngle + mod.jawAngleDelta, 0.0f, 1.0f);
        shape.cheekboneHeight = std::clamp(shape.cheekboneHeight + mod.cheekboneHeightDelta, 0.0f, 1.0f);
        shape.headHeight = std::clamp(shape.headHeight + mod.browRidgeDelta, 0.0f, 1.0f);
        shape.chinWidth = std::clamp(shape.chinWidth + mod.chinWidthDelta, 0.0f, 1.0f);
        shape.noseWidth = std::clamp(shape.noseWidth + mod.noseWidthDelta, 0.0f, 1.0f);

        AgeCurvePoint agePoint = preset.ageCurve.Sample(normalizedAge);
        // Aging hollows cheeks and drops brow height slightly.
        shape.cheekboneHeight = std::clamp(shape.cheekboneHeight - agePoint.cheekHollow * 0.3f, 0.0f, 1.0f);
        shape.headHeight = std::clamp(shape.headHeight - agePoint.browDroop * 0.2f, 0.0f, 1.0f);

        return shape;
    }

    // Generates a randomized shape within +/- preset.variationBound of the
    // base shape for the given race/gender/age, using the provided RNG so
    // callers can control determinism (e.g. seed from NPC form ID).
    RaceFaceShape GenerateRandomShape(RaceType race, Gender gender, float normalizedAge,
                                       std::mt19937& rng) const {
        const RacePreset& preset = GetPreset(race);
        RaceFaceShape shape = BuildBaseShape(race, gender, normalizedAge);
        std::uniform_real_distribution<float> dist(-preset.variationBound, preset.variationBound);

        shape.headWidth = Vary(shape.headWidth, dist(rng));
        shape.headHeight = Vary(shape.headHeight, dist(rng));
        shape.headDepth = Vary(shape.headDepth, dist(rng));
        shape.noseWidth = Vary(shape.noseWidth, dist(rng));
        shape.noseHeight = Vary(shape.noseHeight, dist(rng));
        shape.noseLength = Vary(shape.noseLength, dist(rng));
        shape.eyeWidth = Vary(shape.eyeWidth, dist(rng));
        shape.eyeHeight = Vary(shape.eyeHeight, dist(rng));
        shape.eyeSeparation = Vary(shape.eyeSeparation, dist(rng));
        shape.mouthWidth = Vary(shape.mouthWidth, dist(rng));
        shape.jawWidth = Vary(shape.jawWidth, dist(rng));
        shape.jawAngle = Vary(shape.jawAngle, dist(rng));
        shape.cheekboneHeight = Vary(shape.cheekboneHeight, dist(rng));
        shape.chinHeight = Vary(shape.chinHeight, dist(rng));
        shape.chinWidth = Vary(shape.chinWidth, dist(rng));
        shape.earSize = Vary(shape.earSize, dist(rng));
        shape.earAngle = Vary(shape.earAngle, dist(rng));

        return shape;
    }

    // Samples a random color from the given palette using the provided RNG.
    static glm::vec3 SampleColor(const ColorPalette& palette, std::mt19937& rng) {
        if (palette.empty()) {
            return glm::vec3(0.5f, 0.5f, 0.5f);
        }
        std::uniform_int_distribution<size_t> swatchDist(0, palette.size() - 1);
        const ColorRange& range = palette[swatchDist(rng)];
        std::uniform_real_distribution<float> tDist(0.0f, 1.0f);
        float tr = tDist(rng);
        float tg = tDist(rng);
        float tb = tDist(rng);
        return glm::vec3(
            range.minColor.x + (range.maxColor.x - range.minColor.x) * tr,
            range.minColor.y + (range.maxColor.y - range.minColor.y) * tg,
            range.minColor.z + (range.maxColor.z - range.minColor.z) * tb
        );
    }

    // ------------------------------------------------------------------
    // ESM RACE record integration point. In the full pipeline this would
    // parse the RACE record's FNAM/DESC/skin & body-part subrecords and
    // override the corresponding preset fields (name, skin tones, etc).
    // This function performs a best-effort merge and logs what changed;
    // fields not present in the ESM data retain their built-in defaults.
    // ------------------------------------------------------------------
    struct EsmRaceRecordData {
        std::string editorId;
        std::string fullName;
        bool hasSkinTones = false;
        ColorPalette skinTones;
        bool hasHairColors = false;
        ColorPalette hairColors;
        bool hasEyeColors = false;
        ColorPalette eyeColors;
    };

    bool ApplyEsmRaceRecord(RaceType race, const EsmRaceRecordData& esmData) {
        size_t idx = static_cast<size_t>(race);
        if (idx >= presets_.size()) {
            LOGW_FRP("ApplyEsmRaceRecord: invalid race index %zu", idx);
            return false;
        }
        RacePreset& preset = presets_[idx];
        if (!esmData.fullName.empty()) {
            preset.name = esmData.fullName;
        }
        if (esmData.hasSkinTones) {
            preset.skinTones = esmData.skinTones;
        }
        if (esmData.hasHairColors) {
            preset.hairColors = esmData.hairColors;
        }
        if (esmData.hasEyeColors) {
            preset.eyeColors = esmData.eyeColors;
        }
        LOGI_FRP("Applied ESM RACE record '%s' to preset %s",
                 esmData.editorId.c_str(), RaceTypeToString(race));
        return true;
    }

private:
    FaceRacePresets() {
        presets_.resize(static_cast<size_t>(RaceType::COUNT));
        BuildImperial();
        BuildBreton();
        BuildNord();
        BuildRedguard();
        BuildDarkElf();
        BuildHighElf();
        BuildWoodElf();
        BuildOrc();
        BuildKhajiit();
        BuildArgonian();
    }

    static float Vary(float base, float delta) {
        return std::clamp(base * (1.0f + delta), 0.0f, 1.0f);
    }

    RacePreset& At(RaceType r) { return presets_[static_cast<size_t>(r)]; }

    // Shared helper: builds a symmetric-ish male/female pair from a single
    // "average" shape plus small default gender deltas, so each race builder
    // below only needs to specify the race-distinguishing parameters.
    static void ApplyDefaultGenderSplit(RacePreset& preset, const RaceFaceShape& average) {
        preset.baseShapeMale = average;
        preset.baseShapeFemale = average;

        preset.maleModifier.jawWidthDelta = 0.06f;
        preset.maleModifier.jawAngleDelta = 0.04f;
        preset.maleModifier.cheekboneHeightDelta = -0.03f;
        preset.maleModifier.browRidgeDelta = 0.03f;
        preset.maleModifier.chinWidthDelta = 0.05f;
        preset.maleModifier.noseWidthDelta = 0.03f;

        preset.femaleModifier.jawWidthDelta = -0.05f;
        preset.femaleModifier.jawAngleDelta = -0.03f;
        preset.femaleModifier.cheekboneHeightDelta = 0.05f;
        preset.femaleModifier.browRidgeDelta = -0.02f;
        preset.femaleModifier.chinWidthDelta = -0.04f;
        preset.femaleModifier.noseWidthDelta = -0.02f;
    }

    void BuildImperial() {
        RacePreset& p = At(RaceType::IMPERIAL);
        p.race = RaceType::IMPERIAL;
        p.name = "Imperial";
        RaceFaceShape avg;
        avg.headWidth = 0.5f; avg.headHeight = 0.5f; avg.headDepth = 0.5f;
        avg.noseWidth = 0.48f; avg.noseHeight = 0.5f; avg.noseLength = 0.5f;
        avg.jawWidth = 0.5f; avg.jawAngle = 0.5f; avg.chinHeight = 0.5f; avg.chinWidth = 0.5f;
        avg.cheekboneHeight = 0.5f; avg.earSize = 0.5f; avg.earAngle = 0.5f;
        avg.eyeWidth = 0.5f; avg.eyeHeight = 0.5f; avg.eyeSeparation = 0.5f; avg.mouthWidth = 0.5f;
        ApplyDefaultGenderSplit(p, avg);
        p.skinTones = { {glm::vec3(0.75f, 0.55f, 0.42f), glm::vec3(0.92f, 0.75f, 0.62f)} };
        p.hairColors = { {glm::vec3(0.05f, 0.03f, 0.02f), glm::vec3(0.15f, 0.1f, 0.06f)},
                          {glm::vec3(0.35f, 0.2f, 0.1f), glm::vec3(0.55f, 0.35f, 0.2f)} };
        p.eyeColors = { {glm::vec3(0.2f, 0.3f, 0.5f), glm::vec3(0.35f, 0.5f, 0.65f)},
                         {glm::vec3(0.25f, 0.4f, 0.2f), glm::vec3(0.4f, 0.55f, 0.3f)} };
        p.variationBound = 0.10f;
    }

    void BuildBreton() {
        RacePreset& p = At(RaceType::BRETON);
        p.race = RaceType::BRETON;
        p.name = "Breton";
        RaceFaceShape avg;
        avg.headWidth = 0.47f; avg.headHeight = 0.5f; avg.headDepth = 0.48f;
        avg.noseWidth = 0.45f; avg.noseHeight = 0.48f; avg.noseLength = 0.47f;
        avg.jawWidth = 0.45f; avg.jawAngle = 0.48f; avg.chinHeight = 0.48f; avg.chinWidth = 0.46f;
        avg.cheekboneHeight = 0.52f; avg.earSize = 0.45f; avg.earAngle = 0.5f;
        avg.eyeWidth = 0.52f; avg.eyeHeight = 0.52f; avg.eyeSeparation = 0.5f; avg.mouthWidth = 0.48f;
        ApplyDefaultGenderSplit(p, avg);
        p.skinTones = { {glm::vec3(0.82f, 0.65f, 0.55f), glm::vec3(0.95f, 0.82f, 0.72f)} };
        p.hairColors = { {glm::vec3(0.5f, 0.35f, 0.2f), glm::vec3(0.75f, 0.55f, 0.35f)},
                          {glm::vec3(0.05f, 0.03f, 0.02f), glm::vec3(0.2f, 0.13f, 0.08f)} };
        p.eyeColors = { {glm::vec3(0.15f, 0.25f, 0.45f), glm::vec3(0.3f, 0.45f, 0.6f)} };
        p.variationBound = 0.10f;
    }

    void BuildNord() {
        RacePreset& p = At(RaceType::NORD);
        p.race = RaceType::NORD;
        p.name = "Nord";
        RaceFaceShape avg;
        avg.headWidth = 0.55f; avg.headHeight = 0.52f; avg.headDepth = 0.53f;
        avg.noseWidth = 0.5f; avg.noseHeight = 0.52f; avg.noseLength = 0.53f;
        avg.jawWidth = 0.58f; avg.jawAngle = 0.55f; avg.chinHeight = 0.52f; avg.chinWidth = 0.55f;
        avg.cheekboneHeight = 0.5f; avg.earSize = 0.5f; avg.earAngle = 0.48f;
        avg.eyeWidth = 0.48f; avg.eyeHeight = 0.48f; avg.eyeSeparation = 0.52f; avg.mouthWidth = 0.5f;
        ApplyDefaultGenderSplit(p, avg);
        p.skinTones = { {glm::vec3(0.85f, 0.68f, 0.55f), glm::vec3(0.98f, 0.85f, 0.72f)} };
        p.hairColors = { {glm::vec3(0.7f, 0.55f, 0.3f), glm::vec3(0.95f, 0.85f, 0.6f)},
                          {glm::vec3(0.4f, 0.3f, 0.15f), glm::vec3(0.6f, 0.45f, 0.25f)} };
        p.eyeColors = { {glm::vec3(0.3f, 0.5f, 0.6f), glm::vec3(0.45f, 0.65f, 0.75f)},
                         {glm::vec3(0.3f, 0.45f, 0.3f), glm::vec3(0.45f, 0.6f, 0.4f)} };
        p.variationBound = 0.10f;
    }

    void BuildRedguard() {
        RacePreset& p = At(RaceType::REDGUARD);
        p.race = RaceType::REDGUARD;
        p.name = "Redguard";
        RaceFaceShape avg;
        avg.headWidth = 0.52f; avg.headHeight = 0.5f; avg.headDepth = 0.5f;
        avg.noseWidth = 0.53f; avg.noseHeight = 0.5f; avg.noseLength = 0.5f;
        avg.jawWidth = 0.53f; avg.jawAngle = 0.52f; avg.chinHeight = 0.5f; avg.chinWidth = 0.52f;
        avg.cheekboneHeight = 0.5f; avg.earSize = 0.5f; avg.earAngle = 0.5f;
        avg.eyeWidth = 0.5f; avg.eyeHeight = 0.5f; avg.eyeSeparation = 0.5f; avg.mouthWidth = 0.53f;
        ApplyDefaultGenderSplit(p, avg);
        p.skinTones = { {glm::vec3(0.35f, 0.22f, 0.15f), glm::vec3(0.55f, 0.38f, 0.26f)} };
        p.hairColors = { {glm::vec3(0.02f, 0.01f, 0.01f), glm::vec3(0.1f, 0.06f, 0.04f)} };
        p.eyeColors = { {glm::vec3(0.15f, 0.1f, 0.05f), glm::vec3(0.3f, 0.2f, 0.1f)} };
        p.variationBound = 0.10f;
    }

    void BuildDarkElf() {
        RacePreset& p = At(RaceType::DARK_ELF);
        p.race = RaceType::DARK_ELF;
        p.name = "DarkElf";
        RaceFaceShape avg;
        avg.headWidth = 0.44f; avg.headHeight = 0.5f; avg.headDepth = 0.45f;
        avg.noseWidth = 0.42f; avg.noseHeight = 0.5f; avg.noseLength = 0.55f;
        avg.jawWidth = 0.4f; avg.jawAngle = 0.45f; avg.chinHeight = 0.5f; avg.chinWidth = 0.4f;
        avg.cheekboneHeight = 0.58f; avg.earSize = 0.6f; avg.earAngle = 0.65f;
        avg.eyeWidth = 0.55f; avg.eyeHeight = 0.48f; avg.eyeSeparation = 0.48f; avg.mouthWidth = 0.44f;
        ApplyDefaultGenderSplit(p, avg);
        p.skinTones = { {glm::vec3(0.3f, 0.3f, 0.35f), glm::vec3(0.5f, 0.5f, 0.55f)} }; // ashen gray
        p.hairColors = { {glm::vec3(0.02f, 0.02f, 0.02f), glm::vec3(0.08f, 0.08f, 0.08f)},
                          {glm::vec3(0.5f, 0.5f, 0.55f), glm::vec3(0.8f, 0.8f, 0.85f)} };
        p.eyeColors = { {glm::vec3(0.7f, 0.1f, 0.1f), glm::vec3(0.9f, 0.25f, 0.2f)} }; // red eyes
        p.variationBound = 0.09f;
    }

    void BuildHighElf() {
        RacePreset& p = At(RaceType::HIGH_ELF);
        p.race = RaceType::HIGH_ELF;
        p.name = "HighElf";
        RaceFaceShape avg;
        avg.headWidth = 0.42f; avg.headHeight = 0.53f; avg.headDepth = 0.44f;
        avg.noseWidth = 0.4f; avg.noseHeight = 0.5f; avg.noseLength = 0.55f;
        avg.jawWidth = 0.4f; avg.jawAngle = 0.42f; avg.chinHeight = 0.52f; avg.chinWidth = 0.4f;
        avg.cheekboneHeight = 0.6f; avg.earSize = 0.62f; avg.earAngle = 0.68f;
        avg.eyeWidth = 0.55f; avg.eyeHeight = 0.5f; avg.eyeSeparation = 0.46f; avg.mouthWidth = 0.42f;
        ApplyDefaultGenderSplit(p, avg);
        p.skinTones = { {glm::vec3(0.88f, 0.8f, 0.68f), glm::vec3(0.98f, 0.92f, 0.82f)} };
        p.hairColors = { {glm::vec3(0.85f, 0.8f, 0.65f), glm::vec3(1.0f, 0.95f, 0.85f)},
                          {glm::vec3(0.5f, 0.45f, 0.3f), glm::vec3(0.7f, 0.65f, 0.45f)} };
        p.eyeColors = { {glm::vec3(0.4f, 0.6f, 0.7f), glm::vec3(0.55f, 0.75f, 0.85f)},
                         {glm::vec3(0.3f, 0.5f, 0.35f), glm::vec3(0.45f, 0.65f, 0.5f)} };
        p.variationBound = 0.08f;
    }

    void BuildWoodElf() {
        RacePreset& p = At(RaceType::WOOD_ELF);
        p.race = RaceType::WOOD_ELF;
        p.name = "WoodElf";
        RaceFaceShape avg;
        avg.headWidth = 0.4f; avg.headHeight = 0.46f; avg.headDepth = 0.42f;
        avg.noseWidth = 0.4f; avg.noseHeight = 0.46f; avg.noseLength = 0.48f;
        avg.jawWidth = 0.38f; avg.jawAngle = 0.4f; avg.chinHeight = 0.46f; avg.chinWidth = 0.38f;
        avg.cheekboneHeight = 0.55f; avg.earSize = 0.65f; avg.earAngle = 0.7f;
        avg.eyeWidth = 0.56f; avg.eyeHeight = 0.5f; avg.eyeSeparation = 0.46f; avg.mouthWidth = 0.42f;
        ApplyDefaultGenderSplit(p, avg);
        p.skinTones = { {glm::vec3(0.55f, 0.45f, 0.3f), glm::vec3(0.75f, 0.62f, 0.42f)} };
        p.hairColors = { {glm::vec3(0.15f, 0.3f, 0.1f), glm::vec3(0.3f, 0.45f, 0.2f)},
                          {glm::vec3(0.3f, 0.2f, 0.1f), glm::vec3(0.5f, 0.35f, 0.2f)} };
        p.eyeColors = { {glm::vec3(0.3f, 0.5f, 0.25f), glm::vec3(0.45f, 0.65f, 0.35f)},
                         {glm::vec3(0.55f, 0.45f, 0.2f), glm::vec3(0.7f, 0.6f, 0.3f)} };
        p.variationBound = 0.10f;
    }

    void BuildOrc() {
        RacePreset& p = At(RaceType::ORC);
        p.race = RaceType::ORC;
        p.name = "Orc";
        RaceFaceShape avg;
        avg.headWidth = 0.62f; avg.headHeight = 0.55f; avg.headDepth = 0.58f;
        avg.noseWidth = 0.6f; avg.noseHeight = 0.55f; avg.noseLength = 0.5f;
        avg.jawWidth = 0.68f; avg.jawAngle = 0.65f; avg.chinHeight = 0.5f; avg.chinWidth = 0.62f;
        avg.cheekboneHeight = 0.45f; avg.earSize = 0.55f; avg.earAngle = 0.4f;
        avg.eyeWidth = 0.42f; avg.eyeHeight = 0.4f; avg.eyeSeparation = 0.55f; avg.mouthWidth = 0.6f;
        ApplyDefaultGenderSplit(p, avg);
        p.skinTones = { {glm::vec3(0.25f, 0.35f, 0.2f), glm::vec3(0.4f, 0.5f, 0.3f)} }; // green
        p.hairColors = { {glm::vec3(0.02f, 0.02f, 0.02f), glm::vec3(0.1f, 0.08f, 0.06f)} };
        p.eyeColors = { {glm::vec3(0.6f, 0.4f, 0.05f), glm::vec3(0.8f, 0.55f, 0.1f)} };
        p.variationBound = 0.12f;
    }

    void BuildKhajiit() {
        RacePreset& p = At(RaceType::KHAJIIT);
        p.race = RaceType::KHAJIIT;
        p.name = "Khajiit";
        RaceFaceShape avg;
        avg.headWidth = 0.45f; avg.headHeight = 0.48f; avg.headDepth = 0.55f; // muzzle depth
        avg.noseWidth = 0.5f; avg.noseHeight = 0.4f; avg.noseLength = 0.35f;
        avg.jawWidth = 0.5f; avg.jawAngle = 0.5f; avg.chinHeight = 0.35f; avg.chinWidth = 0.45f;
        avg.cheekboneHeight = 0.5f; avg.earSize = 0.7f; avg.earAngle = 0.55f;
        avg.eyeWidth = 0.5f; avg.eyeHeight = 0.55f; avg.eyeSeparation = 0.52f; avg.mouthWidth = 0.4f;
        ApplyDefaultGenderSplit(p, avg);
        p.skinTones = { {glm::vec3(0.4f, 0.3f, 0.2f), glm::vec3(0.7f, 0.55f, 0.35f)},
                         {glm::vec3(0.15f, 0.1f, 0.08f), glm::vec3(0.3f, 0.22f, 0.15f)} }; // fur tones
        p.hairColors = p.skinTones; // fur color doubles as "hair"
        p.eyeColors = { {glm::vec3(0.75f, 0.65f, 0.1f), glm::vec3(0.9f, 0.8f, 0.2f)},
                         {glm::vec3(0.2f, 0.45f, 0.15f), glm::vec3(0.35f, 0.6f, 0.25f)} };
        p.variationBound = 0.12f;
    }

    void BuildArgonian() {
        RacePreset& p = At(RaceType::ARGONIAN);
        p.race = RaceType::ARGONIAN;
        p.name = "Argonian";
        RaceFaceShape avg;
        avg.headWidth = 0.42f; avg.headHeight = 0.45f; avg.headDepth = 0.6f; // snout
        avg.noseWidth = 0.35f; avg.noseHeight = 0.35f; avg.noseLength = 0.3f;
        avg.jawWidth = 0.45f; avg.jawAngle = 0.5f; avg.chinHeight = 0.3f; avg.chinWidth = 0.4f;
        avg.cheekboneHeight = 0.45f; avg.earSize = 0.2f; avg.earAngle = 0.3f; // minimal ears
        avg.eyeWidth = 0.45f; avg.eyeHeight = 0.4f; avg.eyeSeparation = 0.55f; avg.mouthWidth = 0.45f;
        ApplyDefaultGenderSplit(p, avg);
        p.skinTones = { {glm::vec3(0.1f, 0.3f, 0.2f), glm::vec3(0.25f, 0.45f, 0.3f)},
                         {glm::vec3(0.2f, 0.25f, 0.35f), glm::vec3(0.35f, 0.4f, 0.5f)} }; // scale tones
        p.hairColors = {}; // no hair, quill/frill color could go here later
        p.eyeColors = { {glm::vec3(0.7f, 0.6f, 0.1f), glm::vec3(0.9f, 0.75f, 0.2f)},
                         {glm::vec3(0.15f, 0.4f, 0.4f), glm::vec3(0.3f, 0.55f, 0.55f)} };
        p.variationBound = 0.12f;
    }

    std::vector<RacePreset> presets_;
};

} // namespace facegen
