#include "enchanting_system.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <ctime>

namespace game {

// Map a 4-character MGEF editorID code to our MagicEffect enum.
// Oblivion identifies magic effects by their editorID (e.g. FIDG = Fire Damage).
static MagicEffect mgefCodeToMagicEffect(const std::string& code) {
    if (code.size() != 4) return MagicEffect::FORTIFY_HEALTH;
    // Damage effects
    if (code == "FIDG") return MagicEffect::FIRE_DAMAGE;
    if (code == "FRDG") return MagicEffect::FROST_DAMAGE;
    if (code == "SHDG") return MagicEffect::SHOCK_DAMAGE;
    // Resist effects
    if (code == "RESF") return MagicEffect::RESIST_FIRE;
        if (code == "RESI") return MagicEffect::RESIST_FROST;
    if (code == "RESS") return MagicEffect::RESIST_SHOCK;
    if (code == "RESM") return MagicEffect::RESIST_MAGIC;
    if (code == "RESD") return MagicEffect::RESIST_DISEASE;
        if (code == "RESN") return MagicEffect::RESIST_POISON;
    // Fortify attribute
    if (code == "FOHE") return MagicEffect::FORTIFY_HEALTH;
    if (code == "FOSP") return MagicEffect::FORTIFY_MAGICKA;
    if (code == "FOST") return MagicEffect::FORTIFY_STAMINA;
        if (code == "FOAT") return MagicEffect::FORTIFY_STRENGTH;
    if (code == "FOIN") return MagicEffect::FORTIFY_INTELLIGENCE;
    if (code == "FOWI") return MagicEffect::FORTIFY_WILLPOWER;
    if (code == "FOAG") return MagicEffect::FORTIFY_AGILITY;
    if (code == "FOSD") return MagicEffect::FORTIFY_SPEED;
    if (code == "FOEN") return MagicEffect::FORTIFY_ENDURANCE;
    if (code == "FOPE") return MagicEffect::FORTIFY_PERSONALITY;
    if (code == "FOLU") return MagicEffect::FORTIFY_LUCK;
    if (code == "FOAR") return MagicEffect::FORTIFY_ARMOR;
    if (code == "FOAT") return MagicEffect::FORTIFY_ATTACK;
    // Utility
    if (code == "SOUL") return MagicEffect::SOUL_TRAP;
    if (code == "TELE") return MagicEffect::TELEKINESIS;
    if (code == "WABR") return MagicEffect::WATER_BREATHING;
    if (code == "WAWA") return MagicEffect::WATER_WALKING;
    if (code == "NIEY") return MagicEffect::NIGHT_EYE;
    if (code == "CHML") return MagicEffect::CHAMELEON;
    if (code == "INVI") return MagicEffect::INVISIBILITY;
    if (code == "LIGH") return MagicEffect::LIGHT;
    if (code == "FEAT") return MagicEffect::FEATHER;
    if (code == "DETE") return MagicEffect::DETECT_LIFE;
    return MagicEffect::FORTIFY_HEALTH;
}

// Map ESM enchantType to our EnchantmentType enum.
static EnchantmentType esmEnchantTypeToEnum(uint32_t et) {
    switch (et) {
        case 0: return EnchantmentType::CAST_ONCE;       // scroll
        case 1: return EnchantmentType::CAST_ON_USE;     // staff
        case 2: return EnchantmentType::CAST_ON_STRIKE;  // weapon
        case 3: return EnchantmentType::CONSTANT_EFFECT; // apparel
        default: return EnchantmentType::CAST_ONCE;
    }
}

void EnchantingSystem::initialize() {
    if (initialized) {
        LOGW("EnchantingSystem already initialized");
        return;
    }
    
    LOGI("=== EnchantingSystem Initializing ===");
    
    // Load saved state if exists
    loadState();
    
    initialized = true;
    LOGI("=== EnchantingSystem Initialized ===");
}

void EnchantingSystem::loadEnchantmentsFromESM(const oblivion::ESMManager& esmMgr) {
    const auto& esmEnchants = esmMgr.getAllEnchantments();
    size_t loaded = 0;

    for (const auto& e : esmEnchants) {
        if (e.formID == 0) continue;
        if (enchantments_.count(e.formID) != 0) continue;

        Enchantment ench;
        ench.id = e.formID;
        ench.name = e.fullName.empty() ? e.editorID : e.fullName;
        ench.type = esmEnchantTypeToEnum(e.enchantType);
        ench.charges = e.chargeAmount;
        ench.enchantCost = static_cast<float>(e.enchantCost);

        // Use the first effect's data
        if (!e.effectCodes.empty()) {
            ench.effect = mgefCodeToMagicEffect(e.effectCodes[0]);
        }
        if (!e.effectMagnitudes.empty()) {
            ench.magnitude = e.effectMagnitudes[0];
        }
        if (!e.effectDurations.empty()) {
            ench.duration = e.effectDurations[0];
        }

        ench.currentCharges = ench.charges;
        enchantments_[e.formID] = ench;
        ++loaded;

        if (loaded <= 10) {
            LOGI("  Enchant[%zu]: 0x%08X '%s' type=%u charges=%u cost=%u",
                 loaded, e.formID, ench.name.c_str(),
                 e.enchantType, e.chargeAmount, e.enchantCost);
        }
    }

    LOGI("EnchantingSystem: Loaded %zu enchantments from ESM data", loaded);
}

const Enchantment* EnchantingSystem::findEnchantment(uint32_t formID) const {
    auto it = enchantments_.find(formID);
    return (it != enchantments_.end()) ? &it->second : nullptr;
}

void EnchantingSystem::shutdown() {
    if (!initialized) return;
    
    LOGI("EnchantingSystem shutting down");
    
    // Save state
    saveState();
    
    initialized = false;
}

void EnchantingSystem::update(float deltaTime) {
    if (!initialized) return;
    
    // Nothing to update regularly
}

bool EnchantingSystem::canEnchant(const inventory::Item& item) const {
    if (!initialized) return false;
    
    // Check if item category supports enchanting
    if (item.category != inventory::ItemCategory::Weapon && 
        item.category != inventory::ItemCategory::Armor) {
        return false;
    }
    
    // Check if item already has enchantment
    if (item.enchantmentID != 0) {
        return false;
    }
    
    return true;
}

bool EnchantingSystem::enchantItem(inventory::Item& item, const Enchantment& enchant, const SoulGem& soulGem) {
    if (!initialized) {
        LOGE("EnchantingSystem not initialized");
        return false;
    }
    
    if (!canEnchant(item)) {
        LOGW("Item cannot be enchanted");
        return false;
    }
    
    if (!soulGem.hasSoul) {
        LOGW("Soul gem has no soul");
        return false;
    }
    
    // Calculate enchant cost
    float cost = calculateEnchantCost(enchant);
    float soulCapacity = getSoulGemCapacity(soulGem.size);
    
    // Check if soul gem has enough power
    if (cost > soulCapacity) {
        LOGW("Soul gem too weak for this enchantment (need %.1f, have %.1f)", cost, soulCapacity);
        return false;
    }
    
    // Apply skill bonus
    float skillFactor = 1.0f + (enchantSkill / 100.0f) * SKILL_MAGNITUDE_BONUS;
    
    // Create enchantment with skill bonuses
    Enchantment appliedEnchant = enchant;
    appliedEnchant.magnitude *= skillFactor;
    appliedEnchant.duration = static_cast<uint32_t>(appliedEnchant.duration * (1.0f + (enchantSkill / 100.0f) * SKILL_DURATION_BONUS));
    appliedEnchant.charges = static_cast<uint32_t>(soulCapacity / cost);
    appliedEnchant.currentCharges = appliedEnchant.charges;
    
    // Apply enchantment to item
    item.enchantmentID = enchant.id;
    item.enchantmentName = enchant.name;
    item.enchantmentEffect = static_cast<uint8_t>(enchant.effect);
    item.enchantmentMagnitude = appliedEnchant.magnitude;
    item.enchantmentDuration = appliedEnchant.duration;
    item.maxCharges = appliedEnchant.charges;
    item.currentCharges = appliedEnchant.currentCharges;
    item.enchantmentType = static_cast<uint8_t>(enchant.type);
    
    LOGI("Enchanted %s with %s (%.1f magnitude, %u duration, %u charges)",
         item.name.c_str(), enchant.name.c_str(), 
         appliedEnchant.magnitude, appliedEnchant.duration, appliedEnchant.charges);
    
    // Notify callback
    if (onEnchant) {
        onEnchant(true, appliedEnchant);
    }
    
    return true;
}

bool EnchantingSystem::rechargeItem(inventory::Item& item, const SoulGem& soulGem) {
    if (!initialized) return false;
    
    if (item.enchantmentID == 0) {
        LOGW("Item has no enchantment");
        return false;
    }
    
    if (!soulGem.hasSoul) {
        LOGW("Soul gem has no soul");
        return false;
    }
    
    uint32_t rechargeAmount = getRechargeAmount(soulGem);
    uint32_t newCharges = std::min(item.currentCharges + rechargeAmount, item.maxCharges);
    
    LOGI("Recharged %s: %u -> %u/%u charges", 
         item.name.c_str(), item.currentCharges, newCharges, item.maxCharges);
    
    item.currentCharges = newCharges;
    
    return true;
}

Enchantment EnchantingSystem::createEnchantment(MagicEffect effect, float magnitude, uint32_t duration, EnchantmentType type) const {
    Enchantment enchant;
    
    // Generate unique ID
    enchant.id = static_cast<uint32_t>(std::time(nullptr));
    
    // Set name based on effect
    switch (effect) {
        case MagicEffect::FORTIFY_HEALTH: enchant.name = "Fortify Health"; break;
        case MagicEffect::FORTIFY_MAGICKA: enchant.name = "Fortify Magicka"; break;
        case MagicEffect::FORTIFY_STAMINA: enchant.name = "Fortify Stamina"; break;
        case MagicEffect::FIRE_DAMAGE: enchant.name = "Fire Damage"; break;
        case MagicEffect::FROST_DAMAGE: enchant.name = "Frost Damage"; break;
        case MagicEffect::SHOCK_DAMAGE: enchant.name = "Shock Damage"; break;
        case MagicEffect::SOUL_TRAP: enchant.name = "Soul Trap"; break;
        case MagicEffect::RESIST_FIRE: enchant.name = "Resist Fire"; break;
        case MagicEffect::RESIST_FROST: enchant.name = "Resist Frost"; break;
        case MagicEffect::RESIST_SHOCK: enchant.name = "Resist Shock"; break;
        case MagicEffect::RESIST_MAGIC: enchant.name = "Resist Magic"; break;
        case MagicEffect::WATER_BREATHING: enchant.name = "Water Breathing"; break;
        case MagicEffect::WATER_WALKING: enchant.name = "Water Walking"; break;
        case MagicEffect::NIGHT_EYE: enchant.name = "Night Eye"; break;
        case MagicEffect::CHAMELEON: enchant.name = "Chameleon"; break;
        case MagicEffect::INVISIBILITY: enchant.name = "Invisibility"; break;
        case MagicEffect::LIGHT: enchant.name = "Light"; break;
        case MagicEffect::FEATHER: enchant.name = "Feather"; break;
        case MagicEffect::DETECT_LIFE: enchant.name = "Detect Life"; break;
        default: enchant.name = "Unknown Enchant"; break;
    }
    
    enchant.type = type;
    enchant.effect = effect;
    enchant.magnitude = magnitude;
    enchant.duration = duration;
    
    return enchant;
}

float EnchantingSystem::getEnchantCost(const Enchantment& enchant) const {
    return calculateEnchantCost(enchant);
}

uint32_t EnchantingSystem::getRechargeAmount(const SoulGem& soulGem) const {
    return static_cast<uint32_t>(getSoulGemCapacity(soulGem.size));
}

SoulGem EnchantingSystem::createSoulGem(SoulGemSize size, bool withSoul) const {
    SoulGem gem;
    
    // Generate unique ID
    gem.id = static_cast<uint32_t>(std::time(nullptr));
    
    // Set name based on size
    switch (size) {
        case SoulGemSize::PETTY: gem.name = "Petty Soul Gem"; break;
        case SoulGemSize::LESSER: gem.name = "Lesser Soul Gem"; break;
        case SoulGemSize::COMMON: gem.name = "Common Soul Gem"; break;
        case SoulGemSize::GREATER: gem.name = "Greater Soul Gem"; break;
        case SoulGemSize::GRAND: gem.name = "Grand Soul Gem"; break;
        default: gem.name = "Soul Gem"; break;
    }
    
    gem.size = size;
    gem.hasSoul = withSoul;
    gem.soulLevel = withSoul ? static_cast<uint32_t>(size) : 0;
    
    return gem;
}

bool EnchantingSystem::trapSoul(SoulGem& gem, uint32_t soulLevel) const {
    if (gem.hasSoul) {
        LOGW("Soul gem already contains a soul");
        return false;
    }
    
    // Check if soul fits
    if (soulLevel > static_cast<uint32_t>(gem.size)) {
        LOGW("Soul too powerful for this gem");
        return false;
    }
    
    gem.hasSoul = true;
    gem.soulLevel = soulLevel;
    
    LOGI("Trapped soul level %u in %s", soulLevel, gem.name.c_str());
    
    return true;
}

float EnchantingSystem::getSoulGemCapacity(SoulGemSize size) const {
    switch (size) {
        case SoulGemSize::PETTY: return SOUL_PETTY;
        case SoulGemSize::LESSER: return SOUL_LESSER;
        case SoulGemSize::COMMON: return SOUL_COMMON;
        case SoulGemSize::GREATER: return SOUL_GREATER;
        case SoulGemSize::GRAND: return SOUL_GRAND;
        default: return 0.0f;
    }
}

float EnchantingSystem::getEffectMagnitude(MagicEffect effect, float baseMagnitude, uint32_t skill) const {
    float skillBonus = 1.0f + (skill / 100.0f) * SKILL_MAGNITUDE_BONUS;
    
    switch (effect) {
        case MagicEffect::FORTIFY_HEALTH:
        case MagicEffect::FORTIFY_MAGICKA:
        case MagicEffect::FORTIFY_STAMINA:
        case MagicEffect::FORTIFY_STRENGTH:
        case MagicEffect::FORTIFY_INTELLIGENCE:
        case MagicEffect::FORTIFY_WILLPOWER:
        case MagicEffect::FORTIFY_AGILITY:
        case MagicEffect::FORTIFY_SPEED:
        case MagicEffect::FORTIFY_ENDURANCE:
        case MagicEffect::FORTIFY_PERSONALITY:
        case MagicEffect::FORTIFY_LUCK:
            return baseMagnitude * skillBonus;
            
        case MagicEffect::FIRE_DAMAGE:
        case MagicEffect::FROST_DAMAGE:
        case MagicEffect::SHOCK_DAMAGE:
            return baseMagnitude * skillBonus * 1.2f;  // Damage effects get 20% bonus
            
        case MagicEffect::RESIST_FIRE:
        case MagicEffect::RESIST_FROST:
        case MagicEffect::RESIST_SHOCK:
        case MagicEffect::RESIST_MAGIC:
        case MagicEffect::RESIST_DISEASE:
        case MagicEffect::RESIST_POISON:
            return std::min(baseMagnitude * skillBonus, 100.0f);  // Cap at 100%
            
        default:
            return baseMagnitude * skillBonus;
    }
}

uint32_t EnchantingSystem::getEffectDuration(MagicEffect effect, uint32_t baseDuration, uint32_t skill) const {
    float skillBonus = 1.0f + (skill / 100.0f) * SKILL_DURATION_BONUS;
    
    // Some effects have fixed durations
    switch (effect) {
        case MagicEffect::FORTIFY_HEALTH:
        case MagicEffect::FORTIFY_MAGICKA:
        case MagicEffect::FORTIFY_STAMINA:
            return static_cast<uint32_t>(baseDuration * skillBonus);
            
        case MagicEffect::WATER_BREATHING:
        case MagicEffect::WATER_WALKING:
        case MagicEffect::NIGHT_EYE:
        case MagicEffect::CHAMELEON:
        case MagicEffect::INVISIBILITY:
            return static_cast<uint32_t>(baseDuration * skillBonus * 1.5f);  // Utility effects get 50% bonus
            
        default:
            return static_cast<uint32_t>(baseDuration * skillBonus);
    }
}

float EnchantingSystem::calculateEnchantCost(const Enchantment& enchant) const {
    float cost = BASE_ENCHANT_COST;
    
    // Add magnitude cost
    cost += enchant.magnitude * getMagnitudeCostMultiplier(enchant.effect);
    
    // Add duration cost
    if (enchant.duration > 0) {
        cost += enchant.duration * getDurationCostMultiplier(enchant.effect);
    }
    
    // Apply type multiplier
    switch (enchant.type) {
        case EnchantmentType::CAST_ONCE:
            cost *= 0.5f;
            break;
        case EnchantmentType::CAST_ON_STRIKE:
            cost *= 1.0f;
            break;
        case EnchantmentType::CAST_ON_USE:
            cost *= 0.8f;
            break;
        case EnchantmentType::CONSTANT_EFFECT:
            cost *= 2.0f;
            break;
        default:
            break;
    }
    
    // Apply skill reduction
    float skillReduction = 1.0f - (enchantSkill / 100.0f) * SKILL_COST_REDUCTION;
    cost *= skillReduction;
    
    return std::max(cost, 1.0f);  // Minimum cost of 1
}

float EnchantingSystem::getMagnitudeCostMultiplier(MagicEffect effect) const {
    switch (effect) {
        case MagicEffect::FORTIFY_HEALTH:
        case MagicEffect::FORTIFY_MAGICKA:
        case MagicEffect::FORTIFY_STAMINA:
            return 1.0f;
            
        case MagicEffect::FORTIFY_STRENGTH:
        case MagicEffect::FORTIFY_INTELLIGENCE:
        case MagicEffect::FORTIFY_WILLPOWER:
        case MagicEffect::FORTIFY_AGILITY:
        case MagicEffect::FORTIFY_SPEED:
        case MagicEffect::FORTIFY_ENDURANCE:
        case MagicEffect::FORTIFY_PERSONALITY:
        case MagicEffect::FORTIFY_LUCK:
            return 1.5f;
            
        case MagicEffect::FIRE_DAMAGE:
        case MagicEffect::FROST_DAMAGE:
        case MagicEffect::SHOCK_DAMAGE:
            return 2.0f;
            
        case MagicEffect::SOUL_TRAP:
        case MagicEffect::TELEKINESIS:
            return 2.5f;
            
        case MagicEffect::INVISIBILITY:
        case MagicEffect::CHAMELEON:
            return 3.0f;
            
        default:
            return MAGNITUDE_COST_MULTIPLIER;
    }
}

float EnchantingSystem::getDurationCostMultiplier(MagicEffect effect) const {
    switch (effect) {
        case MagicEffect::FIRE_DAMAGE:
        case MagicEffect::FROST_DAMAGE:
        case MagicEffect::SHOCK_DAMAGE:
            return 0.5f;  // Damage over time is cheaper
            
        case MagicEffect::SOUL_TRAP:
            return 1.0f;
            
        case MagicEffect::INVISIBILITY:
        case MagicEffect::CHAMELEON:
            return 2.0f;  // Powerful effects cost more over time
            
        default:
            return DURATION_COST_MULTIPLIER;
    }
}

void EnchantingSystem::saveState() const {
    LOGD("EnchantingSystem state saved (skill=%u)", enchantSkill);
}

void EnchantingSystem::loadState() {
    LOGD("EnchantingSystem state loaded");
}

} // namespace game
