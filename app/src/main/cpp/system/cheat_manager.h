#pragma once

#include <cstdint>

/**
 * @brief Cheat manager (dummy implementation)
 *
 * Stub for compatibility with existing code.
 * Actual cheat functionality is currently unused.
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
