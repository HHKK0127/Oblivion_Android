#pragma once
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <cmath>
#include <cstdint>
#include <android/log.h>

#define OBLIVION_PMAT_LOG_TAG "OblivionPhysicsMaterial"
#define OBLIVION_PMAT_LOGI(...) __android_log_print(ANDROID_LOG_INFO, OBLIVION_PMAT_LOG_TAG, __VA_ARGS__)
#define OBLIVION_PMAT_LOGW(...) __android_log_print(ANDROID_LOG_WARN, OBLIVION_PMAT_LOG_TAG, __VA_ARGS__)

namespace oblivion {

// Oblivion ESM MATT-like material categories
enum class MaterialType : uint8_t {
    STONE = 0,
    DIRT,
    GRASS,
    METAL,
    WOOD,
    ORGANIC,   // flesh / creatures
    CLOTH,
    WATER,
    SAND,
    SNOW,
    GLASS,
    SKIN_LIGHT_ARMOR,
    SKIN_HEAVY_ARMOR,
    NUM_MATERIALS
};

enum class SurfaceSoundEvent : uint8_t {
    FOOTSTEP = 0,
    IMPACT_LIGHT,
    IMPACT_HEAVY,
    BREAK_SOUND,
    NUM_EVENTS
};

// Breakable object metadata attached to a material.
struct BreakableProperties {
    bool breakable = false;
    float healthThreshold = 0.0f;   // damage required to break
    std::string debrisType;         // maps to DestructionSystem debris presets
    int debrisCount = 0;
};

// Core physics material definition, roughly equivalent to an ESM MATT record.
class PhysicsMaterial {
public:
    PhysicsMaterial() = default;
    PhysicsMaterial(MaterialType type, const std::string& name,
                     float friction, float restitution, float density)
        : type(type), name(name), friction(friction),
          restitution(restitution), density(density) {}

    MaterialType type = MaterialType::STONE;
    std::string name;

    float friction = 0.6f;      // static/dynamic combined friction coefficient
    float restitution = 0.1f;   // bounciness
    float density = 1000.0f;    // kg/m^3, used for auto-mass computation

    // Sound identifiers per event, resolved by the audio system (string keys
    // so this header stays independent from any audio manager include).
    std::unordered_map<SurfaceSoundEvent, std::string> sounds;

    BreakableProperties breakable;

    void setSound(SurfaceSoundEvent event, const std::string& soundId) {
        sounds[event] = soundId;
    }

    std::string getSound(SurfaceSoundEvent event) const {
        auto it = sounds.find(event);
        return it != sounds.end() ? it->second : std::string();
    }

    // Compute mass for a given volume (m^3) using this material's density.
    float computeMass(float volumeM3) const {
        return density * volumeM3;
    }
};

// Result of combining two materials at a contact point.
struct CombinedMaterialResponse {
    float friction = 0.5f;
    float restitution = 0.0f;
};

// Central material database. Loaded from ESM MATT records at startup and
// referenced by body user data throughout the physics simulation.
class PhysicsMaterialDatabase {
public:
    static PhysicsMaterialDatabase& getInstance() {
        static PhysicsMaterialDatabase instance;
        return instance;
    }

    // Populates the database with Oblivion's default material set. Should be
    // called once during physics initialization before ESM overrides load.
    void initDefaults() {
        materials.clear();

        addMaterial(MaterialType::STONE, "Stone", 0.9f, 0.05f, 2600.0f);
        addMaterial(MaterialType::DIRT, "Dirt", 0.7f, 0.02f, 1500.0f);
        addMaterial(MaterialType::GRASS, "Grass", 0.55f, 0.02f, 900.0f);
        addMaterial(MaterialType::METAL, "Metal", 0.4f, 0.25f, 7800.0f);
        addMaterial(MaterialType::WOOD, "Wood", 0.6f, 0.15f, 700.0f);
        addMaterial(MaterialType::ORGANIC, "Organic", 0.65f, 0.1f, 985.0f);
        addMaterial(MaterialType::CLOTH, "Cloth", 0.8f, 0.0f, 150.0f);
        addMaterial(MaterialType::WATER, "Water", 0.05f, 0.0f, 1000.0f);
        addMaterial(MaterialType::SAND, "Sand", 0.75f, 0.01f, 1600.0f);
        addMaterial(MaterialType::SNOW, "Snow", 0.5f, 0.01f, 400.0f);
        addMaterial(MaterialType::GLASS, "Glass", 0.3f, 0.3f, 2500.0f);
        addMaterial(MaterialType::SKIN_LIGHT_ARMOR, "SkinLightArmor", 0.5f, 0.1f, 2700.0f);
        addMaterial(MaterialType::SKIN_HEAVY_ARMOR, "SkinHeavyArmor", 0.45f, 0.2f, 7800.0f);

        setDefaultSounds();
        setDefaultBreakable();

        OBLIVION_PMAT_LOGI("Initialized %zu default physics materials", materials.size());
    }

    void addMaterial(MaterialType type, const std::string& name,
                      float friction, float restitution, float density) {
        materials[type] = PhysicsMaterial(type, name, friction, restitution, density);
    }

    PhysicsMaterial* get(MaterialType type) {
        auto it = materials.find(type);
        return it != materials.end() ? &it->second : nullptr;
    }

    const PhysicsMaterial* get(MaterialType type) const {
        auto it = materials.find(type);
        return it != materials.end() ? &it->second : nullptr;
    }

    // Combine two materials into effective friction/restitution for a
    // contact using standard PhysX/Jolt-style combining rules.
    static CombinedMaterialResponse combine(const PhysicsMaterial& a, const PhysicsMaterial& b) {
        CombinedMaterialResponse response;
        response.friction = std::sqrt(std::max(a.friction, 0.0f) * std::max(b.friction, 0.0f));
        response.restitution = std::max(a.restitution, b.restitution);
        return response;
    }

    CombinedMaterialResponse combine(MaterialType a, MaterialType b) const {
        const PhysicsMaterial* matA = get(a);
        const PhysicsMaterial* matB = get(b);
        if (!matA || !matB) {
            return CombinedMaterialResponse{};
        }
        return combine(*matA, *matB);
    }

    // Placeholder for loading MATT records from an ESM file. The actual ESM
    // parsing lives in the data layer; this hook lets it push overrides in.
    void applyEsmOverride(const std::string& esmMaterialName, MaterialType mappedType,
                           float friction, float restitution) {
        PhysicsMaterial* mat = get(mappedType);
        if (!mat) {
            OBLIVION_PMAT_LOGW("applyEsmOverride: unknown material type for %s", esmMaterialName.c_str());
            return;
        }
        mat->friction = friction;
        mat->restitution = restitution;
        OBLIVION_PMAT_LOGI("ESM override applied to %s from record %s", mat->name.c_str(), esmMaterialName.c_str());
    }

private:
    PhysicsMaterialDatabase() = default;

    void setDefaultSounds() {
        struct SoundSet {
            MaterialType type;
            const char* footstep;
            const char* impactLight;
            const char* impactHeavy;
            const char* breakSound;
        };
        static const SoundSet kSets[] = {
            {MaterialType::STONE, "footstep_stone", "impact_stone_light", "impact_stone_heavy", "break_stone"},
            {MaterialType::DIRT, "footstep_dirt", "impact_dirt_light", "impact_dirt_heavy", "break_dirt"},
            {MaterialType::GRASS, "footstep_grass", "impact_grass_light", "impact_grass_heavy", "break_grass"},
            {MaterialType::METAL, "footstep_metal", "impact_metal_light", "impact_metal_heavy", "break_metal"},
            {MaterialType::WOOD, "footstep_wood", "impact_wood_light", "impact_wood_heavy", "break_wood"},
            {MaterialType::ORGANIC, "footstep_organic", "impact_organic_light", "impact_organic_heavy", "break_organic"},
            {MaterialType::CLOTH, "footstep_cloth", "impact_cloth_light", "impact_cloth_heavy", "break_cloth"},
            {MaterialType::WATER, "footstep_water", "impact_water_light", "impact_water_heavy", "break_water"},
            {MaterialType::SAND, "footstep_sand", "impact_sand_light", "impact_sand_heavy", "break_sand"},
            {MaterialType::SNOW, "footstep_snow", "impact_snow_light", "impact_snow_heavy", "break_snow"},
            {MaterialType::GLASS, "footstep_glass", "impact_glass_light", "impact_glass_heavy", "break_glass"},
        };
        for (const auto& s : kSets) {
            PhysicsMaterial* mat = get(s.type);
            if (!mat) continue;
            mat->setSound(SurfaceSoundEvent::FOOTSTEP, s.footstep);
            mat->setSound(SurfaceSoundEvent::IMPACT_LIGHT, s.impactLight);
            mat->setSound(SurfaceSoundEvent::IMPACT_HEAVY, s.impactHeavy);
            mat->setSound(SurfaceSoundEvent::BREAK_SOUND, s.breakSound);
        }
    }

    void setDefaultBreakable() {
        if (PhysicsMaterial* glass = get(MaterialType::GLASS)) {
            glass->breakable.breakable = true;
            glass->breakable.healthThreshold = 5.0f;
            glass->breakable.debrisType = "glass_shards";
            glass->breakable.debrisCount = 8;
        }
        if (PhysicsMaterial* wood = get(MaterialType::WOOD)) {
            wood->breakable.breakable = true;
            wood->breakable.healthThreshold = 25.0f;
            wood->breakable.debrisType = "wood_splinters";
            wood->breakable.debrisCount = 6;
        }
    }

    std::unordered_map<MaterialType, PhysicsMaterial> materials;
};

} // namespace oblivion
