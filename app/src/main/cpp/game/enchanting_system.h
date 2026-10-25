#pragma once

#include "../inventory/item_base.h"
#include "../assets/esm_reader.h"
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGW
#undef LOGE

#define LOG_TAG "EnchantSystem"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace game {

// Enchantment types (matching Oblivion)
enum class EnchantmentType : uint8_t {
    CAST_ONCE = 0,      // One-time use
    CAST_ON_STRIKE,     // Triggers on hit
    CAST_ON_USE,        // Triggers on item use
    CONSTANT_EFFECT,    // Always active
    COUNT
};

// Magic effect families that can be enchanted.
//
// Oblivion identifies a magic effect with the 4-character editorID of its MGEF
// record (FIDG = Fire Damage). Several codes share one family because the game
// resolves the concrete target through the effect's actorValue instead of
// through the code itself: a single FOAT (Fortify Attribute) covers all eight
// attributes and a single DGAT covers all of them for damage. The commented
// code next to each member is one authoritative example measured in
// Oblivion.esm; every MGEF code in the game maps to exactly one member (see
// EnchantingSystem::magicEffectFromCode() in enchanting_system.cpp, whose
// coverage is checked by EnchantingSystemTests).
enum class MagicEffect : uint8_t {
    UNKNOWN = 0,                    // code absent from Oblivion.esm

    // Fortify (Restoration)
    FORTIFY_ATTRIBUTE,              // FOAT, actorValue selects the attribute
    FORTIFY_SKILL,                  // FOSK, actorValue selects the skill
    FORTIFY_HEALTH,                 // FOHE
    FORTIFY_MAGICKA,                // FOSP
    FORTIFY_FATIGUE,                // FOFA
    FORTIFY_MAGICKA_MULTIPLIER,     // FOMM

    // Restore and cure (Restoration)
    RESTORE_ATTRIBUTE,              // REAT
    RESTORE_HEALTH,                 // REHE
    RESTORE_MAGICKA,                // RESP
    RESTORE_FATIGUE,                // REFA
    CURE_DISEASE,                   // CUDI
    CURE_PARALYSIS,                 // CUPA
    CURE_POISON,                    // CUPO

    // Absorb (Restoration)
    ABSORB_ATTRIBUTE,               // ABAT
    ABSORB_SKILL,                   // ABSK
    ABSORB_HEALTH,                  // ABHE
    ABSORB_MAGICKA,                 // ABSP
    ABSORB_FATIGUE,                 // ABFA

    // Drain (Destruction)
    DRAIN_ATTRIBUTE,                // DRAT
    DRAIN_SKILL,                    // DRSK
    DRAIN_HEALTH,                   // DRHE
    DRAIN_MAGICKA,                  // DRSP
    DRAIN_FATIGUE,                  // DRFA

    // Damage (Destruction)
    DAMAGE_ATTRIBUTE,               // DGAT
    DAMAGE_HEALTH,                  // DGHE
    DAMAGE_MAGICKA,                 // DGSP
    DAMAGE_FATIGUE,                 // DGFA
    FIRE_DAMAGE,                    // FIDG
    FROST_DAMAGE,                   // FRDG
    SHOCK_DAMAGE,                   // SHDG
    SUN_DAMAGE,                     // SUDG
    DISINTEGRATE_ARMOR,             // DIAR
    DISINTEGRATE_WEAPON,            // DIWE

    // Shield (Alteration)
    SHIELD,                         // SHLD
    FIRE_SHIELD,                    // FISH
    FROST_SHIELD,                   // FRSH
    SHOCK_SHIELD,                   // LISH

    // Resist (Restoration)
    RESIST_FIRE,                    // RSFI
    RESIST_FROST,                   // RSFR
    RESIST_SHOCK,                   // RSSH
    RESIST_MAGIC,                   // RSMA
    RESIST_DISEASE,                 // RSDI
    RESIST_POISON,                  // RSPO
    RESIST_PARALYSIS,               // RSPA
    RESIST_NORMAL_WEAPONS,          // RSNW

    // Weakness (Destruction)
    WEAKNESS_FIRE,                  // WKFI
    WEAKNESS_FROST,                 // WKFR
    WEAKNESS_SHOCK,                 // WKSH
    WEAKNESS_MAGIC,                 // WKMA
    WEAKNESS_DISEASE,               // WKDI
    WEAKNESS_POISON,                // WKPO
    WEAKNESS_NORMAL_WEAPONS,        // WKNW

    // Mysticism
    DISPEL,                         // DSPL
    SPELL_ABSORPTION,               // SABS
    REFLECT_SPELL,                  // RFLC
    REFLECT_DAMAGE,                 // REDG
    SOUL_TRAP,                      // STRP
    TELEKINESIS,                    // TELE
    DETECT_LIFE,                    // DTCT

    // Illusion
    CALM,                           // CALM
    FRENZY,                         // FRNZ
    DEMORALIZE,                     // DEMO
    RALLY,                          // RALY
    CHARM,                          // CHRM
    COMMAND_CREATURE,               // COCR
    COMMAND_HUMANOID,               // COHU
    PARALYZE,                       // PARA
    SILENCE,                        // SLNC
    INVISIBILITY,                   // INVI
    CHAMELEON,                      // CHML
    NIGHT_EYE,                      // NEYE
    LIGHT,                          // LGHT

    // Alteration
    BURDEN,                         // BRDN
    FEATHER,                        // FTHR
    WATER_BREATHING,                // WABR
    WATER_WALKING,                  // WAWA
    OPEN,                           // OPEN
    LOCK,                           // LOCK
    STUNTED_MAGICKA,                // STMA

    // Conjuration
    TURN_UNDEAD,                    // TURN
    BOUND_ITEM,                     // BABO..BASH, BWAX..BWSW, BW01..BW08
    SUMMON,                         // MYTH, MYHL, Z001..Z019, ZCLA..ZZOM
    REANIMATE,                      // REAN

    // Other
    SCRIPT_EFFECT,                  // SEFF
    VAMPIRISM,                      // VAMP
    POISON_INFO,                    // POSN (internal, never a cast effect)
    DARKNESS,                       // DARK (DO NOT USE - Darkness, never cast)

    COUNT
};

// Enchantment data
struct Enchantment {
    uint32_t id = 0;
    std::string name;
    EnchantmentType type = EnchantmentType::CAST_ONCE;
    MagicEffect effect = MagicEffect::UNKNOWN;
    // EFIT actorValue of the effect above. Codes such as FOAT (Fortify
    // Attribute) and FOSK (Fortify Skill) carry the concrete attribute/skill
    // index here, so the target cannot be recovered from `effect` alone.
    uint32_t actorValue = 0;
    float magnitude = 0.0f;
    uint32_t duration = 0;          // 0 for constant effect
    uint32_t charges = 0;           // Max charges
    uint32_t currentCharges = 0;    // Current charges
    float enchantCost = 0.0f;       // Magicka cost to use
    
    Enchantment() = default;
};

// Soul gem for recharging
enum class SoulGemSize : uint8_t {
    PETTY = 0,
    LESSER,
    COMMON,
    GREATER,
    GRAND,
    COUNT
};

struct SoulGem {
    uint32_t id = 0;
    std::string name;
    SoulGemSize size = SoulGemSize::PETTY;
    bool hasSoul = false;
    uint32_t soulLevel = 0;         // 0-5 (petty to grand)
    
    SoulGem() = default;
};

// Callbacks
using EnchantCallback = std::function<void(bool success, const Enchantment& enchant)>;

/**
 * @brief Enchanting System
 * 
 * Allows players to enchant items with magic effects using soul gems.
 * Based on Oblivion's enchanting mechanics.
 */
class EnchantingSystem {
public:
    // Initialization
    void initialize();
    void shutdown();
    void update(float deltaTime);
    
        // Load enchantments from ESM data
        void loadEnchantmentsFromESM(const oblivion::ESMManager& esmMgr);
    
        // Enchanting
        bool canEnchant(const inventory::Item& item) const;
        bool enchantItem(inventory::Item& item, const Enchantment& enchant, const SoulGem& soulGem);
        bool rechargeItem(inventory::Item& item, const SoulGem& soulGem);
    
        // Enchantment queries
        Enchantment createEnchantment(MagicEffect effect, float magnitude, uint32_t duration, EnchantmentType type) const;
        const Enchantment* findEnchantment(uint32_t formID) const;
        const std::unordered_map<uint32_t, Enchantment>& getAllEnchantments() const { return enchantments_; }
        float getEnchantCost(const Enchantment& enchant) const;
        uint32_t getRechargeAmount(const SoulGem& soulGem) const;
    
    // Soul gem management
    SoulGem createSoulGem(SoulGemSize size, bool withSoul = false) const;
    bool trapSoul(SoulGem& gem, uint32_t soulLevel) const;
    float getSoulGemCapacity(SoulGemSize size) const;
    
    // Skill integration
    void setEnchantSkill(uint32_t skill) { enchantSkill = skill; }
    uint32_t getEnchantSkill() const { return enchantSkill; }
    
    // Effects
    float getEffectMagnitude(MagicEffect effect, float baseMagnitude, uint32_t skill) const;
    uint32_t getEffectDuration(MagicEffect effect, uint32_t baseDuration, uint32_t skill) const;

    // Effect code translation. Static so that callers holding only a raw MGEF
    // editorID code (and the host tests) can resolve it without an instance.
    static MagicEffect magicEffectFromCode(const std::string& code);
    static const char* magicEffectName(MagicEffect effect);
    
    // Callbacks
    void setEnchantCallback(EnchantCallback callback) { onEnchant = callback; }
    
    // Save/Load
    void saveState() const;
    void loadState();
    
    // Constructor/Destructor
    EnchantingSystem() = default;
    ~EnchantingSystem() = default;
    
private:
    EnchantingSystem(const EnchantingSystem&) = delete;
    EnchantingSystem& operator=(const EnchantingSystem&) = delete;
    
    // State
    bool initialized = false;
    uint32_t enchantSkill = 0;
    
        // ESM-loaded enchantments (formID -> Enchantment)
        std::unordered_map<uint32_t, Enchantment> enchantments_;
    
    // Enchantment costs
    static constexpr float BASE_ENCHANT_COST = 10.0f;
    static constexpr float MAGNITUDE_COST_MULTIPLIER = 2.0f;
    static constexpr float DURATION_COST_MULTIPLIER = 1.5f;
    
    // Soul gem capacities
    static constexpr float SOUL_PETTY = 10.0f;
    static constexpr float SOUL_LESSER = 25.0f;
    static constexpr float SOUL_COMMON = 50.0f;
    static constexpr float SOUL_GREATER = 100.0f;
    static constexpr float SOUL_GRAND = 200.0f;
    
    // Skill bonuses
    static constexpr float SKILL_MAGNITUDE_BONUS = 0.5f;   // 50% more magnitude at skill 100
    static constexpr float SKILL_DURATION_BONUS = 0.5f;    // 50% more duration at skill 100
    static constexpr float SKILL_COST_REDUCTION = 0.5f;    // 50% less cost at skill 100
    
    // Helper methods
    float calculateEnchantCost(const Enchantment& enchant) const;
    float getMagnitudeCostMultiplier(MagicEffect effect) const;
    float getDurationCostMultiplier(MagicEffect effect) const;
    
    // Callbacks
    EnchantCallback onEnchant;
};

} // namespace game
