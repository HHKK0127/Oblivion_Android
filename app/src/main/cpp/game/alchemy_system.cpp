#include "alchemy_system.h"
#include <android/log.h>
#include <algorithm>

#define LOG_TAG "AlchemySystem"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {

AlchemySystem::AlchemySystem() {
}

AlchemySystem::~AlchemySystem() {
}

void AlchemySystem::initialize(const ESMManager* esmMgr) {
    esmManager = esmMgr;
    LOGI("AlchemySystem initialized");
}

std::unique_ptr<inventory::Item> AlchemySystem::craftPotion(uint32_t ingredient1FormID, uint32_t ingredient2FormID) const {
    if (!esmManager) {
        LOGW("AlchemySystem not initialized");
        return nullptr;
    }

    // Get ingredients
    const IngredientData* ing1 = esmManager->findIngredient(ingredient1FormID);
    const IngredientData* ing2 = esmManager->findIngredient(ingredient2FormID);

    if (!ing1 || !ing2) {
        LOGW("Invalid ingredient formID(s)");
        return nullptr;
    }

    // Find shared effects
    std::vector<uint32_t> sharedEffects = getSharedEffects(ingredient1FormID, ingredient2FormID);

    if (sharedEffects.empty()) {
        LOGD("No matching effects between ingredients");
        return nullptr;
    }

    // Create a new potion
    auto potion = std::make_unique<inventory::Item>();

    // Generate dynamic ID from ingredient formIDs (avoid fixed 0x00000000)
    potion->id = (ingredient1FormID ^ ingredient2FormID) | 0xFF000000;
    potion->name = "Crafted Potion";
    potion->description = "A potion crafted from " + ing1->fullName + " and " + ing2->fullName;
    potion->category = inventory::ItemCategory::Consumable;
    potion->rarity = inventory::ItemRarity::Common;
    potion->equipSlot = inventory::EquipSlot::None;
    potion->weight = 0.5f;
    potion->value = 10;  // Base value
    potion->maxStack = 10;
    potion->iconId = 0;

    // Process ALL shared effects, not just the first one
    for (size_t i = 0; i < sharedEffects.size(); ++i) {
        uint32_t effectFormID = sharedEffects[i];

        // Find the effect indices in both ingredients
        int idx1 = -1, idx2 = -1;
        for (size_t j = 0; j < ing1->effectFormIDs.size(); ++j) {
            if (ing1->effectFormIDs[j] == effectFormID) { idx1 = static_cast<int>(j); break; }
        }
        for (size_t j = 0; j < ing2->effectFormIDs.size(); ++j) {
            if (ing2->effectFormIDs[j] == effectFormID) { idx2 = static_cast<int>(j); break; }
        }

        if (idx1 < 0 || idx2 < 0) continue;

        float mag1 = (idx1 < static_cast<int>(ing1->effectMagnitudes.size())) ? ing1->effectMagnitudes[idx1] : 0.0f;
        float mag2 = (idx2 < static_cast<int>(ing2->effectMagnitudes.size())) ? ing2->effectMagnitudes[idx2] : 0.0f;
        float magnitude = calculateMagnitude(mag1, mag2);

        // Look up the magic effect to determine type
        const MagicEffectData* mgef = esmManager->findMagicEffect(effectFormID);
        if (mgef) {
            // effectType: 0=other, 1=fire, 2=frost, 3=shock, 4=drain, 5=absorb, 6=disintegrate
            // actorValue determines what attribute is affected
            // For healing effects (Restoration school), use healAmount
            // For damage/drain effects, use stats.damage
            // For fortify effects, use appropriate stat
            if (mgef->school == 5) {
                // Restoration - likely healing
                potion->healAmount += static_cast<int32_t>(magnitude);
            } else if (mgef->effectType == 4 || mgef->effectType == 1 || mgef->effectType == 2 || mgef->effectType == 3) {
                // Drain/Fire/Frost/Shock - damage effects
                potion->stats.damage += static_cast<int>(magnitude);
            } else {
                // Other effects (fortify, etc.) - store as healAmount as fallback
                potion->healAmount += static_cast<int32_t>(magnitude);
            }
        } else {
            // Unknown effect - store as healAmount
            potion->healAmount += static_cast<int32_t>(magnitude);
        }

        LOGD("Added effect 0x%08X: magnitude=%.1f", effectFormID, magnitude);
    }

    LOGI("Crafted potion from %s and %s (%zu effects)",
         ing1->fullName.c_str(), ing2->fullName.c_str(), sharedEffects.size());

    return potion;
}

std::vector<inventory::Item> AlchemySystem::getAllIngredients() const {
    std::vector<inventory::Item> items;

    if (!esmManager) {
        LOGW("AlchemySystem not initialized");
        return items;
    }

    const auto& ingredientList = esmManager->getAllIngredients();
    items.reserve(ingredientList.size());

    for (const auto& ingredient : ingredientList) {
        items.push_back(convertIngredientToItem(ingredient));
    }

    LOGI("Converted %zu ingredients", items.size());
    return items;
}

std::vector<inventory::Item> AlchemySystem::getAllPotions() const {
    std::vector<inventory::Item> items;

    if (!esmManager) {
        LOGW("AlchemySystem not initialized");
        return items;
    }

    const auto& potionList = esmManager->getAllAlchemy();
    items.reserve(potionList.size());

    for (const auto& potion : potionList) {
        items.push_back(convertPotionToItem(potion));
    }

    LOGI("Converted %zu potions", items.size());
    return items;
}

const IngredientData* AlchemySystem::getIngredient(uint32_t formID) const {
    if (!esmManager) return nullptr;
    return esmManager->findIngredient(formID);
}

const AlchemyData* AlchemySystem::getPotion(uint32_t formID) const {
    if (!esmManager) return nullptr;
    return esmManager->findAlchemy(formID);
}

bool AlchemySystem::hasMatchingEffects(uint32_t ingredient1FormID, uint32_t ingredient2FormID) const {
    return !getSharedEffects(ingredient1FormID, ingredient2FormID).empty();
}

std::vector<uint32_t> AlchemySystem::getSharedEffects(uint32_t ingredient1FormID, uint32_t ingredient2FormID) const {
    std::vector<uint32_t> sharedEffects;

    if (!esmManager) return sharedEffects;

    const IngredientData* ing1 = esmManager->findIngredient(ingredient1FormID);
    const IngredientData* ing2 = esmManager->findIngredient(ingredient2FormID);

    if (!ing1 || !ing2) return sharedEffects;

    // Find common effect formIDs
    for (uint32_t effect1 : ing1->effectFormIDs) {
        for (uint32_t effect2 : ing2->effectFormIDs) {
            if (effect1 == effect2) {
                sharedEffects.push_back(effect1);
                break;
            }
        }
    }

    return sharedEffects;
}

inventory::Item AlchemySystem::convertIngredientToItem(const IngredientData& ingredient) const {
    inventory::Item item;

    item.id = ingredient.formID;
    item.name = ingredient.fullName;
    item.description = "An alchemical ingredient";
    item.category = inventory::ItemCategory::Material;
    item.rarity = inventory::ItemRarity::Common;
    item.equipSlot = inventory::EquipSlot::None;
    item.weight = ingredient.weight;
    item.value = ingredient.value;
    item.maxStack = 100;  // Ingredients are stackable
    item.iconId = 0;

    item.stats.damage = 0;
    item.stats.defense = 0;

    return item;
}

inventory::Item AlchemySystem::convertPotionToItem(const AlchemyData& potion) const {
    inventory::Item item;

    item.id = potion.formID;
    item.name = potion.fullName;
    item.description = "A potion";
    item.category = inventory::ItemCategory::Consumable;
    item.rarity = inventory::ItemRarity::Common;
    item.equipSlot = inventory::EquipSlot::None;
    item.weight = potion.weight;
    item.value = potion.value;
    item.maxStack = 10;  // Potions are stackable
    item.iconId = 0;

    // Use first effect magnitude as damage stat
    if (!potion.effectMagnitudes.empty()) {
        item.stats.damage = static_cast<int>(potion.effectMagnitudes[0]);
    } else {
        item.stats.damage = 0;
    }
    item.stats.defense = 0;

    return item;
}

float AlchemySystem::calculateMagnitude(float mag1, float mag2) const {
    // Average of both ingredients' magnitudes
    return (mag1 + mag2) / 2.0f;
}

uint32_t AlchemySystem::calculateDuration(uint32_t dur1, uint32_t dur2) const {
    // Average of both ingredients' durations
    return (dur1 + dur2) / 2;
}

} // namespace oblivion
