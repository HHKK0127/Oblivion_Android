#pragma once

#include <cstdint>

/**
 * @brief チートマネージャー（ダミー実装）
 *
 * 既存コードとの互換性のためのスタブ。
 * 実際のチート機能は現在未使用。
 */
class CheatManager {
public:
    enum class CheatType {
        REDUCED_SPELL_COST,
        NO_MAGICKA_DRAIN,
        CRITICAL_HIT_100,
        ONE_SHOT_KILL,
        ENEMY_WEAKNESS
    };

    bool isCheatActive(CheatType /*type*/) const { return false; }
};
