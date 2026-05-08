#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

enum class MagicSchool {
    ALTERATION,      // 変化の魔法
    CONJURATION,     // 召喚の魔法
    DESTRUCTION,     // 破壊の魔法
    ILLUSION,        // 幻覚の魔法
    MYSTICISM,       // 神秘の魔法
    RESTORATION      // 回復の魔法
};

enum class SpellEffectType {
    DAMAGE,          // ダメージを与える
    HEAL,            // HP回復
    RESTORE_MANA,    // マナ回復
    RESTORE_STAMINA, // スタミナ回復
    FORTIFY_ATTR,    // 属性強化
    PARALYZE,        // 麻痺
    INVISIBILITY,    // 姿を隠す
    SUMMON           // 召喚
};

struct SpellEffect {
    SpellEffectType type;
    float magnitude;      // ダメージ量、回復量など
    float duration;       // 効果時間（秒）
    std::string affectedAttribute;  // 影響する属性（Restoration用）

    SpellEffect(SpellEffectType t, float mag, float dur = 0.0f)
        : type(t), magnitude(mag), duration(dur) {}
};

struct Spell {
    uint32_t spellId;
    std::string name;
    std::string nameJa;          // 日本語名
    MagicSchool school;
    float manaCost;
    float baseDamage;
    uint32_t targetType;         // 0=自分、1=敵、2=味方

    std::vector<SpellEffect> effects;
    std::string description;
    std::string descriptionJa;   // 日本語説明

    // コンストラクタ
    Spell(uint32_t id, const std::string& n, const std::string& nJa,
          MagicSchool s, float cost, float damage)
        : spellId(id), name(n), nameJa(nJa), school(s),
          manaCost(cost), baseDamage(damage), targetType(1) {}

    // メソッド
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
