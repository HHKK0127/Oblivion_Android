#pragma once

#include <vector>
#include <cstdint>
#include <random>
#include "../assets/esm_reader.h"

namespace oblivion {

/**
 * @brief Loot generation system using ESM leveled lists
 *
 * Generates loot for containers, drops, and rewards based on
 * player level and leveled list data from ESM.
 */
class LootGenerator {
public:
    LootGenerator();
    ~LootGenerator();

    /**
     * @brief Initialize with ESM manager
     * @param esmMgr ESM manager with loaded data
     */
    void initialize(const ESMManager* esmMgr);

    /**
     * @brief Generate loot from a leveled list
     * @param listFormID FormID of the leveled list (LVLI/LVLC)
     * @param playerLevel Current player level
     * @return List of (formID, count) pairs for generated items
     */
    std::vector<std::pair<uint32_t, uint16_t>> generateLoot(
        uint32_t listFormID, uint32_t playerLevel) const;

    /**
     * @brief Generate random loot for a container
     * @param containerLevel Container's level (determines loot quality)
     * @param playerLevel Current player level
     * @return List of (formID, count) pairs for generated items
     */
    std::vector<std::pair<uint32_t, uint16_t>> generateContainerLoot(
        uint32_t containerLevel, uint32_t playerLevel) const;

    /**
     * @brief Generate loot drop from an enemy
     * @param enemyLevel Enemy's level
     * @param playerLevel Current player level
     * @return List of (formID, count) pairs for dropped items
     */
    std::vector<std::pair<uint32_t, uint16_t>> generateEnemyDrop(
        uint32_t enemyLevel, uint32_t playerLevel) const;

private:
    const ESMManager* esmManager = nullptr;
    mutable std::mt19937 rng;

    /**
     * @brief Get a random integer in range [min, max]
     */
    int randomInt(int min, int max) const;
};

} // namespace oblivion
