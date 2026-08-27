#pragma once

#include "../assets/esm_reader.h"
#include "../inventory/item_base.h"
#include <vector>
#include <random>
#include <memory>

namespace oblivion {

/// Alchemy system for potion crafting using ingredients
class AlchemySystem {
public:
    AlchemySystem();
    ~AlchemySystem();

    void initialize(const ESMManager* esmMgr);

    /// Craft a potion from two ingredients
    /// Returns the crafted potion as an inventory item, or nullptr if invalid
    std::unique_ptr<inventory::Item> craftPotion(uint32_t ingredient1FormID, uint32_t ingredient2FormID) const;

    /// Get all available ingredients
    std::vector<inventory::Item> getAllIngredients() const;

    /// Get all available potions
    std::vector<inventory::Item> getAllPotions() const;

    /// Get ingredient by formID
    const IngredientData* getIngredient(uint32_t formID) const;

    /// Get potion by formID
    const AlchemyData* getPotion(uint32_t formID) const;

    /// Check if two ingredients share any effects
    bool hasMatchingEffects(uint32_t ingredient1FormID, uint32_t ingredient2FormID) const;

    /// Get shared effects between two ingredients
    std::vector<uint32_t> getSharedEffects(uint32_t ingredient1FormID, uint32_t ingredient2FormID) const;

private:
    const ESMManager* esmManager = nullptr;

    /// Convert ingredient to inventory item
    inventory::Item convertIngredientToItem(const IngredientData& ingredient) const;

    /// Convert potion to inventory item
    inventory::Item convertPotionToItem(const AlchemyData& potion) const;

    /// Calculate potion magnitude from ingredient effects
    float calculateMagnitude(float mag1, float mag2) const;

    /// Calculate potion duration from ingredient effects
    uint32_t calculateDuration(uint32_t dur1, uint32_t dur2) const;
};

} // namespace oblivion
