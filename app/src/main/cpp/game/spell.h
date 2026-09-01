#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

enum class MagicSchool {
    ALTERATION,      // Alteration magic
    CONJURATION,     // Conjuration magic
    DESTRUCTION,     // Destruction magic
    ILLUSION,        // Illusion magic
    MYSTICISM,       // Mysticism magic
    RESTORATION      // Restoration magic
};

enum class SpellEffectType {
    DAMAGE,          // Deal damage
    HEAL,            // Restore HP
    RESTORE_MANA,    // Restore mana
    RESTORE_STAMINA, // Restore stamina
    FORTIFY_ATTR,    // Fortify attribute
    PARALYZE,        // Paralyze
    INVISIBILITY,    // Invisibility
    SUMMON           // Summon
};

struct SpellEffect {
    SpellEffectType type;
    float magnitude;      // Damage amount, recovery amount, etc.
    float duration;       // Effect duration (seconds)
    std::string affectedAttribute;  // Affected attribute (for Restoration)

    SpellEffect(SpellEffectType t, float mag, float dur = 0.0f)
        : type(t), magnitude(mag), duration(dur) {}
};

struct Spell {
    uint32_t spellId;
    std::string name;
    std::string nameJa;          // Japanese name
    MagicSchool school;
    float manaCost;
    float baseDamage;
    uint32_t targetType;         // 0=self, 1=enemy, 2=ally

    std::vector<SpellEffect> effects;
    std::string description;
    std::string descriptionJa;   // Japanese description

    // Constructor
    Spell(uint32_t id, const std::string& n, const std::string& nJa,
          MagicSchool s, float cost, float damage)
        : spellId(id), name(n), nameJa(nJa), school(s),
          manaCost(cost), baseDamage(damage), targetType(1) {}

    // Methods
    bool isAvailable(float currentMana) const {
        return currentMana >= manaCost;
    }

    std::string getSchoolName() const {
        switch (school) {
            case MagicSchool::ALTERATION: return "Alteration";
            case MagicSchool::CONJURATION: return "Conjuration";
            case MagicSchool::DESTRUCTION: return "Destruction";
            case MagicSchool::ILLUSION: return "Illusion";
            case MagicSchool::MYSTICISM: return "Mysticism";
            case MagicSchool::RESTORATION: return "Restoration";
            default: return "Unknown";
        }
    }

    std::string getSchoolNameJa() const {
        switch (school) {
            case MagicSchool::ALTERATION: return "変化の魔法";
            case MagicSchool::CONJURATION: return "召喚の魔法";
            case MagicSchool::DESTRUCTION: return "破壊の魔法";
            case MagicSchool::ILLUSION: return "幻覚の魔法";
            case MagicSchool::MYSTICISM: return "神秘の魔法";
            case MagicSchool::RESTORATION: return "回復の魔法";
            default: return "不明";
        }
    }
};
