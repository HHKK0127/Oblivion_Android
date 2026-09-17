#pragma once

#include "../inventory/item_base.h"
#include <cstdint>
#include <string>
#include <vector>
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

// Magic effects that can be enchanted
enum class MagicEffect : uint8_t {
    FORTIFY_HEALTH = 0,
    FORTIFY_MAGICKA,
    FORTIFY_STAMINA,
    FORTIFY_STRENGTH,
    FORTIFY_INTELLIGENCE,
    FORTIFY_WILLPOWER,
    FORTIFY_AGILITY,
    FORTIFY_SPEED,
    FORTIFY_ENDURANCE,
    FORTIFY_PERSONALITY,
    FORTIFY_LUCK,
    FORTIFY_ARMOR,
    FORTIFY_ATTACK,
    RESIST_FIRE,
    RESIST_FROST,
    RESIST_SHOCK,
    RESIST_MAGIC,
    RESIST_DISEASE,
    RESIST_POISON,
    FIRE_DAMAGE,
    FROST_DAMAGE,
    SHOCK_DAMAGE,
    SOUL_TRAP,
    TELEKINESIS,
    WATER_BREATHING,
    WATER_WALKING,
    NIGHT_EYE,
    CHAMELEON,
    INVISIBILITY,
    LIGHT,
    FEATHER,
    DETECT_LIFE,
    COUNT
};

// Enchantment data
struct Enchantment {
    uint32_t id = 0;
    std::string name;
    EnchantmentType type = EnchantmentType::CAST_ONCE;
    MagicEffect effect = MagicEffect::FORTIFY_HEALTH;
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
    
    // Enchanting
    bool canEnchant(const inventory::Item& item) const;
    bool enchantItem(inventory::Item& item, const Enchantment& enchant, const SoulGem& soulGem);
    bool rechargeItem(inventory::Item& item, const SoulGem& soulGem);
    
    // Enchantment queries
    Enchantment createEnchantment(MagicEffect effect, float magnitude, uint32_t duration, EnchantmentType type) const;
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
