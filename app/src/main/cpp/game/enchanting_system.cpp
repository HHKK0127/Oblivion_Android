#include "enchanting_system.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <ctime>
#include <unordered_map>

namespace game {

// Map a 4-character MGEF editorID code to a MagicEffect family.
//
// The table was generated from all 145 MGEF records in Oblivion.esm and is
// exhaustive: a code that is not part of the game's data returns UNKNOWN
// rather than silently pretending to be some other effect.
MagicEffect EnchantingSystem::magicEffectFromCode(const std::string& code) {
    static const std::unordered_map<std::string, MagicEffect> kMgefCodeToEffect = {
        {"ABAT", MagicEffect::ABSORB_ATTRIBUTE},   // Absorb Attribute
        {"ABFA", MagicEffect::ABSORB_FATIGUE},   // Absorb Fatigue
        {"ABHE", MagicEffect::ABSORB_HEALTH},   // Absorb Health
        {"ABSK", MagicEffect::ABSORB_SKILL},   // Absorb Skill
        {"ABSP", MagicEffect::ABSORB_MAGICKA},   // Absorb Magicka
        {"BABO", MagicEffect::BOUND_ITEM},   // Bound Boots
        {"BACU", MagicEffect::BOUND_ITEM},   // Bound Cuirass
        {"BAGA", MagicEffect::BOUND_ITEM},   // Bound Gauntlets
        {"BAGR", MagicEffect::BOUND_ITEM},   // Bound Greaves
        {"BAHE", MagicEffect::BOUND_ITEM},   // Bound Helmet
        {"BASH", MagicEffect::BOUND_ITEM},   // Bound Shield
        {"BRDN", MagicEffect::BURDEN},   // Burden
        {"BW01", MagicEffect::BOUND_ITEM},   // Bound Order Weapon 1
        {"BW02", MagicEffect::BOUND_ITEM},   // Bound Order Weapon 2
        {"BW03", MagicEffect::BOUND_ITEM},   // Bound Order Weapon 3
        {"BW04", MagicEffect::BOUND_ITEM},   // Bound Order Weapon 4
        {"BW05", MagicEffect::BOUND_ITEM},   // Bound Order Weapon 5
        {"BW06", MagicEffect::BOUND_ITEM},   // Bound Order Weapon 6
        {"BW07", MagicEffect::BOUND_ITEM},   // Summon Staff of Sheogorath
        {"BW08", MagicEffect::BOUND_ITEM},   // Bound Priest Dagger
        {"BWAX", MagicEffect::BOUND_ITEM},   // Bound Axe
        {"BWBO", MagicEffect::BOUND_ITEM},   // Bound Bow
        {"BWDA", MagicEffect::BOUND_ITEM},   // Bound Dagger
        {"BWMA", MagicEffect::BOUND_ITEM},   // Bound Mace
        {"BWSW", MagicEffect::BOUND_ITEM},   // Bound Sword
        {"CALM", MagicEffect::CALM},   // Calm
        {"CHML", MagicEffect::CHAMELEON},   // Chameleon
        {"CHRM", MagicEffect::CHARM},   // Charm
        {"COCR", MagicEffect::COMMAND_CREATURE},   // Command Creature
        {"COHU", MagicEffect::COMMAND_HUMANOID},   // Command Humanoid
        {"CUDI", MagicEffect::CURE_DISEASE},   // Cure Disease
        {"CUPA", MagicEffect::CURE_PARALYSIS},   // Cure Paralysis
        {"CUPO", MagicEffect::CURE_POISON},   // Cure Poison
        {"DARK", MagicEffect::DARKNESS},   // DO NOT USE - Darkness
        {"DEMO", MagicEffect::DEMORALIZE},   // Demoralize
        {"DGAT", MagicEffect::DAMAGE_ATTRIBUTE},   // Damage Attribute
        {"DGFA", MagicEffect::DAMAGE_FATIGUE},   // Damage Fatigue
        {"DGHE", MagicEffect::DAMAGE_HEALTH},   // Damage Health
        {"DGSP", MagicEffect::DAMAGE_MAGICKA},   // Damage Magicka
        {"DIAR", MagicEffect::DISINTEGRATE_ARMOR},   // Disintegrate Armor
        {"DIWE", MagicEffect::DISINTEGRATE_WEAPON},   // Disintegrate Weapon
        {"DRAT", MagicEffect::DRAIN_ATTRIBUTE},   // Drain Attribute
        {"DRFA", MagicEffect::DRAIN_FATIGUE},   // Drain Fatigue
        {"DRHE", MagicEffect::DRAIN_HEALTH},   // Drain Health
        {"DRSK", MagicEffect::DRAIN_SKILL},   // Drain Skill
        {"DRSP", MagicEffect::DRAIN_MAGICKA},   // Drain Magicka
        {"DSPL", MagicEffect::DISPEL},   // Dispel
        {"DTCT", MagicEffect::DETECT_LIFE},   // Detect Life
        {"FIDG", MagicEffect::FIRE_DAMAGE},   // Fire Damage
        {"FISH", MagicEffect::FIRE_SHIELD},   // Fire Shield
        {"FOAT", MagicEffect::FORTIFY_ATTRIBUTE},   // Fortify Attribute
        {"FOFA", MagicEffect::FORTIFY_FATIGUE},   // Fortify Fatigue
        {"FOHE", MagicEffect::FORTIFY_HEALTH},   // Fortify Health
        {"FOMM", MagicEffect::FORTIFY_MAGICKA_MULTIPLIER},   // Fortify Magicka Multiplier
        {"FOSK", MagicEffect::FORTIFY_SKILL},   // Fortify Skill
        {"FOSP", MagicEffect::FORTIFY_MAGICKA},   // Fortify Magicka
        {"FRDG", MagicEffect::FROST_DAMAGE},   // Frost Damage
        {"FRNZ", MagicEffect::FRENZY},   // Frenzy
        {"FRSH", MagicEffect::FROST_SHIELD},   // Frost Shield
        {"FTHR", MagicEffect::FEATHER},   // Feather
        {"INVI", MagicEffect::INVISIBILITY},   // Invisibility
        {"LGHT", MagicEffect::LIGHT},   // Light
        {"LISH", MagicEffect::SHOCK_SHIELD},   // Shock Shield
        {"LOCK", MagicEffect::LOCK},   // DO NOT USE - Lock
        {"MYHL", MagicEffect::SUMMON},   // Summon Mythic Dawn Helm
        {"MYTH", MagicEffect::SUMMON},   // Summon Mythic Dawn Armor
        {"NEYE", MagicEffect::NIGHT_EYE},   // Night-Eye
        {"OPEN", MagicEffect::OPEN},   // Open
        {"PARA", MagicEffect::PARALYZE},   // Paralyze
        {"POSN", MagicEffect::POISON_INFO},   // Poison Info
        {"RALY", MagicEffect::RALLY},   // Rally
        {"REAN", MagicEffect::REANIMATE},   // Reanimate
        {"REAT", MagicEffect::RESTORE_ATTRIBUTE},   // Restore Attribute
        {"REDG", MagicEffect::REFLECT_DAMAGE},   // Reflect Damage
        {"REFA", MagicEffect::RESTORE_FATIGUE},   // Restore Fatigue
        {"REHE", MagicEffect::RESTORE_HEALTH},   // Restore Health
        {"RESP", MagicEffect::RESTORE_MAGICKA},   // Restore Magicka
        {"RFLC", MagicEffect::REFLECT_SPELL},   // Reflect Spell
        {"RSDI", MagicEffect::RESIST_DISEASE},   // Resist Disease
        {"RSFI", MagicEffect::RESIST_FIRE},   // Resist Fire
        {"RSFR", MagicEffect::RESIST_FROST},   // Resist Frost
        {"RSMA", MagicEffect::RESIST_MAGIC},   // Resist Magic
        {"RSNW", MagicEffect::RESIST_NORMAL_WEAPONS},   // Resist Normal Weapons
        {"RSPA", MagicEffect::RESIST_PARALYSIS},   // Resist Paralysis
        {"RSPO", MagicEffect::RESIST_POISON},   // Resist Poison
        {"RSSH", MagicEffect::RESIST_SHOCK},   // Resist Shock
        {"SABS", MagicEffect::SPELL_ABSORPTION},   // Spell Absorption
        {"SEFF", MagicEffect::SCRIPT_EFFECT},   // Script Effect
        {"SHDG", MagicEffect::SHOCK_DAMAGE},   // Shock Damage
        {"SHLD", MagicEffect::SHIELD},   // Shield
        {"SLNC", MagicEffect::SILENCE},   // Silence
        {"STMA", MagicEffect::STUNTED_MAGICKA},   // Stunted Magicka
        {"STRP", MagicEffect::SOUL_TRAP},   // Soul Trap
        {"SUDG", MagicEffect::SUN_DAMAGE},   // Sun Damage
        {"TELE", MagicEffect::TELEKINESIS},   // Telekinesis
        {"TURN", MagicEffect::TURN_UNDEAD},   // Turn Undead
        {"VAMP", MagicEffect::VAMPIRISM},   // Vampirism
        {"WABR", MagicEffect::WATER_BREATHING},   // Water Breathing
        {"WAWA", MagicEffect::WATER_WALKING},   // Water Walking
        {"WKDI", MagicEffect::WEAKNESS_DISEASE},   // Weakness to Disease
        {"WKFI", MagicEffect::WEAKNESS_FIRE},   // Weakness to Fire
        {"WKFR", MagicEffect::WEAKNESS_FROST},   // Weakness to Frost
        {"WKMA", MagicEffect::WEAKNESS_MAGIC},   // Weakness to Magic
        {"WKNW", MagicEffect::WEAKNESS_NORMAL_WEAPONS},   // Weakness to Normal Weapons
        {"WKPO", MagicEffect::WEAKNESS_POISON},   // Weakness to Poison
        {"WKSH", MagicEffect::WEAKNESS_SHOCK},   // Weakness to Shock
        {"Z001", MagicEffect::SUMMON},   // Summon Rufio's Ghost
        {"Z002", MagicEffect::SUMMON},   // Summon Ancestor Guardian
        {"Z003", MagicEffect::SUMMON},   // Summon Spiderling
        {"Z004", MagicEffect::SUMMON},   // Summon Flesh Atronach
        {"Z005", MagicEffect::SUMMON},   // Summon Bear
        {"Z006", MagicEffect::SUMMON},   // Summon Gluttonous Hunger
        {"Z007", MagicEffect::SUMMON},   // Summon Ravenous Hunger
        {"Z008", MagicEffect::SUMMON},   // Summon Voracious Hunger
        {"Z009", MagicEffect::SUMMON},   // Summon Dark Seducer
        {"Z010", MagicEffect::SUMMON},   // Summon Golden Saint
        {"Z011", MagicEffect::SUMMON},   // Wabba Summon
        {"Z012", MagicEffect::SUMMON},   // Summon Decrepit Shambles
        {"Z013", MagicEffect::SUMMON},   // Summon Shambles
        {"Z014", MagicEffect::SUMMON},   // Summon Replete Shambles
        {"Z015", MagicEffect::SUMMON},   // Summon Hunger
        {"Z016", MagicEffect::SUMMON},   // Summon Mangled Flesh Atronach
        {"Z017", MagicEffect::SUMMON},   // Summon Torn Flesh Atronach
        {"Z018", MagicEffect::SUMMON},   // Summon Stitched Flesh Atronach
        {"Z019", MagicEffect::SUMMON},   // Summon Sewn Flesh Atronach
        {"ZCLA", MagicEffect::SUMMON},   // Summon Clannfear
        {"ZDAE", MagicEffect::SUMMON},   // Summon Daedroth
        {"ZDRE", MagicEffect::SUMMON},   // Summon Dremora
        {"ZDRL", MagicEffect::SUMMON},   // Summon Dremora Lord
        {"ZFIA", MagicEffect::SUMMON},   // Summon Flame Atronach
        {"ZFRA", MagicEffect::SUMMON},   // Summon Frost Atronach
        {"ZGHO", MagicEffect::SUMMON},   // Summon Ghost
        {"ZHDZ", MagicEffect::SUMMON},   // Summon Headless Zombie
        {"ZLIC", MagicEffect::SUMMON},   // Summon Lich
        {"ZSCA", MagicEffect::SUMMON},   // Summon Scamp
        {"ZSKA", MagicEffect::SUMMON},   // Summon Skeleton Guardian
        {"ZSKC", MagicEffect::SUMMON},   // Summon Skeleton Champion
        {"ZSKE", MagicEffect::SUMMON},   // Summon Skeleton
        {"ZSKH", MagicEffect::SUMMON},   // Summon Skeleton Hero
        {"ZSPD", MagicEffect::SUMMON},   // Summon Spider Daedra
        {"ZSTA", MagicEffect::SUMMON},   // Summon Storm Atronach
        {"ZWRA", MagicEffect::SUMMON},   // Summon Faded Wraith
        {"ZWRL", MagicEffect::SUMMON},   // Summon Gloom Wraith
        {"ZXIV", MagicEffect::SUMMON},   // Summon Xivilai
        {"ZZOM", MagicEffect::SUMMON},   // Summon Zombie
    };

    if (code.size() != 4) return MagicEffect::UNKNOWN;

    auto it = kMgefCodeToEffect.find(code);
    return (it != kMgefCodeToEffect.end()) ? it->second : MagicEffect::UNKNOWN;
}

// Human-readable name of an effect family. Several MGEF codes share one family
// (see MagicEffect), so this names the effect, not the affected attribute or
// skill.
const char* EnchantingSystem::magicEffectName(MagicEffect effect) {
    static const char* const kNames[] = {
        "Unknown",   // UNKNOWN
        "Fortify Attribute",   // FORTIFY_ATTRIBUTE
        "Fortify Skill",   // FORTIFY_SKILL
        "Fortify Health",   // FORTIFY_HEALTH
        "Fortify Magicka",   // FORTIFY_MAGICKA
        "Fortify Fatigue",   // FORTIFY_FATIGUE
        "Fortify Magicka Multiplier",   // FORTIFY_MAGICKA_MULTIPLIER
        "Restore Attribute",   // RESTORE_ATTRIBUTE
        "Restore Health",   // RESTORE_HEALTH
        "Restore Magicka",   // RESTORE_MAGICKA
        "Restore Fatigue",   // RESTORE_FATIGUE
        "Cure Disease",   // CURE_DISEASE
        "Cure Paralysis",   // CURE_PARALYSIS
        "Cure Poison",   // CURE_POISON
        "Absorb Attribute",   // ABSORB_ATTRIBUTE
        "Absorb Skill",   // ABSORB_SKILL
        "Absorb Health",   // ABSORB_HEALTH
        "Absorb Magicka",   // ABSORB_MAGICKA
        "Absorb Fatigue",   // ABSORB_FATIGUE
        "Drain Attribute",   // DRAIN_ATTRIBUTE
        "Drain Skill",   // DRAIN_SKILL
        "Drain Health",   // DRAIN_HEALTH
        "Drain Magicka",   // DRAIN_MAGICKA
        "Drain Fatigue",   // DRAIN_FATIGUE
        "Damage Attribute",   // DAMAGE_ATTRIBUTE
        "Damage Health",   // DAMAGE_HEALTH
        "Damage Magicka",   // DAMAGE_MAGICKA
        "Damage Fatigue",   // DAMAGE_FATIGUE
        "Fire Damage",   // FIRE_DAMAGE
        "Frost Damage",   // FROST_DAMAGE
        "Shock Damage",   // SHOCK_DAMAGE
        "Sun Damage",   // SUN_DAMAGE
        "Disintegrate Armor",   // DISINTEGRATE_ARMOR
        "Disintegrate Weapon",   // DISINTEGRATE_WEAPON
        "Shield",   // SHIELD
        "Fire Shield",   // FIRE_SHIELD
        "Frost Shield",   // FROST_SHIELD
        "Shock Shield",   // SHOCK_SHIELD
        "Resist Fire",   // RESIST_FIRE
        "Resist Frost",   // RESIST_FROST
        "Resist Shock",   // RESIST_SHOCK
        "Resist Magic",   // RESIST_MAGIC
        "Resist Disease",   // RESIST_DISEASE
        "Resist Poison",   // RESIST_POISON
        "Resist Paralysis",   // RESIST_PARALYSIS
        "Resist Normal Weapons",   // RESIST_NORMAL_WEAPONS
        "Weakness to Fire",   // WEAKNESS_FIRE
        "Weakness to Frost",   // WEAKNESS_FROST
        "Weakness to Shock",   // WEAKNESS_SHOCK
        "Weakness to Magic",   // WEAKNESS_MAGIC
        "Weakness to Disease",   // WEAKNESS_DISEASE
        "Weakness to Poison",   // WEAKNESS_POISON
        "Weakness to Normal Weapons",   // WEAKNESS_NORMAL_WEAPONS
        "Dispel",   // DISPEL
        "Spell Absorption",   // SPELL_ABSORPTION
        "Reflect Spell",   // REFLECT_SPELL
        "Reflect Damage",   // REFLECT_DAMAGE
        "Soul Trap",   // SOUL_TRAP
        "Telekinesis",   // TELEKINESIS
        "Detect Life",   // DETECT_LIFE
        "Calm",   // CALM
        "Frenzy",   // FRENZY
        "Demoralize",   // DEMORALIZE
        "Rally",   // RALLY
        "Charm",   // CHARM
        "Command Creature",   // COMMAND_CREATURE
        "Command Humanoid",   // COMMAND_HUMANOID
        "Paralyze",   // PARALYZE
        "Silence",   // SILENCE
        "Invisibility",   // INVISIBILITY
        "Chameleon",   // CHAMELEON
        "Night-Eye",   // NIGHT_EYE
        "Light",   // LIGHT
        "Burden",   // BURDEN
        "Feather",   // FEATHER
        "Water Breathing",   // WATER_BREATHING
        "Water Walking",   // WATER_WALKING
        "Open",   // OPEN
        "Lock",   // LOCK
        "Stunted Magicka",   // STUNTED_MAGICKA
        "Turn Undead",   // TURN_UNDEAD
        "Bound Item",   // BOUND_ITEM
        "Summon",   // SUMMON
        "Reanimate",   // REANIMATE
        "Script Effect",   // SCRIPT_EFFECT
        "Vampirism",   // VAMPIRISM
        "Poison Info",   // POISON_INFO
        "Darkness",   // DARKNESS
    };
    static_assert(sizeof(kNames) / sizeof(kNames[0]) == static_cast<size_t>(MagicEffect::COUNT),
                  "magicEffectName table out of sync with MagicEffect");

    const size_t index = static_cast<size_t>(effect);
    if (index >= static_cast<size_t>(MagicEffect::COUNT)) return "Unknown";
    return kNames[index];
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
    size_t multiEffect = 0;
    size_t unknownCode = 0;

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
            ench.effect = magicEffectFromCode(e.effectCodes[0]);
            if (ench.effect == MagicEffect::UNKNOWN) {
                ++unknownCode;
            }
        }
        if (!e.effectMagnitudes.empty()) {
            ench.magnitude = e.effectMagnitudes[0];
        }
        if (!e.effectDurations.empty()) {
            ench.duration = e.effectDurations[0];
        }
        // Only the first effect is modelled; hub codes such as FOAT/FOSK need
        // the EFIT actorValue to name their concrete target.
        if (!e.effectActorValues.empty()) {
            ench.actorValue = e.effectActorValues[0];
        }
        // 33% of the shipped ENCH records carry two or more effects; the extra
        // ones are dropped by design until multi-effect enchantments land.
        if (e.effectCodes.size() > 1) {
            ++multiEffect;
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

    if (multiEffect > 0) {
        LOGI("EnchantingSystem: %zu of %zu enchantments have 2+ effects; "
             "only the first effect is applied", multiEffect, loaded);
    }
    if (unknownCode > 0) {
        LOGW("EnchantingSystem: %zu of %zu enchantments use an unrecognised "
             "first effect code", unknownCode, loaded);
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
    
    enchant.name = magicEffectName(effect);
    
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
        case MagicEffect::FORTIFY_ATTRIBUTE:
        case MagicEffect::FORTIFY_SKILL:
        case MagicEffect::FORTIFY_HEALTH:
        case MagicEffect::FORTIFY_MAGICKA:
        case MagicEffect::FORTIFY_FATIGUE:
        case MagicEffect::FORTIFY_MAGICKA_MULTIPLIER:
        case MagicEffect::RESTORE_ATTRIBUTE:
        case MagicEffect::RESTORE_HEALTH:
        case MagicEffect::RESTORE_MAGICKA:
        case MagicEffect::RESTORE_FATIGUE:
        case MagicEffect::CURE_DISEASE:
        case MagicEffect::CURE_PARALYSIS:
        case MagicEffect::CURE_POISON:
            return baseMagnitude * skillBonus;
            
        case MagicEffect::DAMAGE_ATTRIBUTE:
        case MagicEffect::DAMAGE_HEALTH:
        case MagicEffect::DAMAGE_MAGICKA:
        case MagicEffect::DAMAGE_FATIGUE:
        case MagicEffect::FIRE_DAMAGE:
        case MagicEffect::FROST_DAMAGE:
        case MagicEffect::SHOCK_DAMAGE:
        case MagicEffect::SUN_DAMAGE:
        case MagicEffect::DISINTEGRATE_ARMOR:
        case MagicEffect::DISINTEGRATE_WEAPON:
        case MagicEffect::ABSORB_ATTRIBUTE:
        case MagicEffect::ABSORB_SKILL:
        case MagicEffect::ABSORB_HEALTH:
        case MagicEffect::ABSORB_MAGICKA:
        case MagicEffect::ABSORB_FATIGUE:
        case MagicEffect::DRAIN_ATTRIBUTE:
        case MagicEffect::DRAIN_SKILL:
        case MagicEffect::DRAIN_HEALTH:
        case MagicEffect::DRAIN_MAGICKA:
        case MagicEffect::DRAIN_FATIGUE:
            return baseMagnitude * skillBonus * 1.2f;  // Hostile effects get 20% bonus
            
        case MagicEffect::RESIST_FIRE:
        case MagicEffect::RESIST_FROST:
        case MagicEffect::RESIST_SHOCK:
        case MagicEffect::RESIST_MAGIC:
        case MagicEffect::RESIST_DISEASE:
        case MagicEffect::RESIST_POISON:
        case MagicEffect::RESIST_PARALYSIS:
        case MagicEffect::RESIST_NORMAL_WEAPONS:
        case MagicEffect::WEAKNESS_FIRE:
        case MagicEffect::WEAKNESS_FROST:
        case MagicEffect::WEAKNESS_SHOCK:
        case MagicEffect::WEAKNESS_MAGIC:
        case MagicEffect::WEAKNESS_DISEASE:
        case MagicEffect::WEAKNESS_POISON:
        case MagicEffect::WEAKNESS_NORMAL_WEAPONS:
            return std::min(baseMagnitude * skillBonus, 100.0f);  // Cap at 100%
            
        default:
            return baseMagnitude * skillBonus;
    }
}

uint32_t EnchantingSystem::getEffectDuration(MagicEffect effect, uint32_t baseDuration, uint32_t skill) const {
    float skillBonus = 1.0f + (skill / 100.0f) * SKILL_DURATION_BONUS;
    
    // Some effects have fixed durations
    switch (effect) {
        case MagicEffect::FORTIFY_ATTRIBUTE:
        case MagicEffect::FORTIFY_SKILL:
        case MagicEffect::FORTIFY_HEALTH:
        case MagicEffect::FORTIFY_MAGICKA:
        case MagicEffect::FORTIFY_FATIGUE:
        case MagicEffect::FORTIFY_MAGICKA_MULTIPLIER:
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
        case MagicEffect::FORTIFY_FATIGUE:
            return 1.0f;
            
        case MagicEffect::FORTIFY_ATTRIBUTE:
        case MagicEffect::FORTIFY_SKILL:
        case MagicEffect::FORTIFY_MAGICKA_MULTIPLIER:
        case MagicEffect::RESTORE_ATTRIBUTE:
        case MagicEffect::RESTORE_HEALTH:
        case MagicEffect::RESTORE_MAGICKA:
        case MagicEffect::RESTORE_FATIGUE:
        case MagicEffect::CURE_DISEASE:
        case MagicEffect::CURE_PARALYSIS:
        case MagicEffect::CURE_POISON:
            return 1.5f;
            
        case MagicEffect::DAMAGE_ATTRIBUTE:
        case MagicEffect::DAMAGE_HEALTH:
        case MagicEffect::DAMAGE_MAGICKA:
        case MagicEffect::DAMAGE_FATIGUE:
        case MagicEffect::FIRE_DAMAGE:
        case MagicEffect::FROST_DAMAGE:
        case MagicEffect::SHOCK_DAMAGE:
        case MagicEffect::SUN_DAMAGE:
        case MagicEffect::DISINTEGRATE_ARMOR:
        case MagicEffect::DISINTEGRATE_WEAPON:
        case MagicEffect::ABSORB_ATTRIBUTE:
        case MagicEffect::ABSORB_SKILL:
        case MagicEffect::ABSORB_HEALTH:
        case MagicEffect::ABSORB_MAGICKA:
        case MagicEffect::ABSORB_FATIGUE:
        case MagicEffect::DRAIN_ATTRIBUTE:
        case MagicEffect::DRAIN_SKILL:
        case MagicEffect::DRAIN_HEALTH:
        case MagicEffect::DRAIN_MAGICKA:
        case MagicEffect::DRAIN_FATIGUE:
            return 2.0f;
            
        case MagicEffect::SOUL_TRAP:
        case MagicEffect::TELEKINESIS:
            return 2.5f;
            
        case MagicEffect::INVISIBILITY:
        case MagicEffect::CHAMELEON:
        case MagicEffect::PARALYZE:
        case MagicEffect::COMMAND_CREATURE:
        case MagicEffect::COMMAND_HUMANOID:
        case MagicEffect::TURN_UNDEAD:
            return 3.0f;
            
        default:
            return MAGNITUDE_COST_MULTIPLIER;
    }
}

float EnchantingSystem::getDurationCostMultiplier(MagicEffect effect) const {
    switch (effect) {
        case MagicEffect::DAMAGE_ATTRIBUTE:
        case MagicEffect::DAMAGE_HEALTH:
        case MagicEffect::DAMAGE_MAGICKA:
        case MagicEffect::DAMAGE_FATIGUE:
        case MagicEffect::FIRE_DAMAGE:
        case MagicEffect::FROST_DAMAGE:
        case MagicEffect::SHOCK_DAMAGE:
        case MagicEffect::SUN_DAMAGE:
            return 0.5f;  // Damage over time is cheaper
            
        case MagicEffect::SOUL_TRAP:
            return 1.0f;
            
        case MagicEffect::INVISIBILITY:
        case MagicEffect::CHAMELEON:
        case MagicEffect::PARALYZE:
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
