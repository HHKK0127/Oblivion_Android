#include "clothing_converter.h"
#include <android/log.h>

#define LOG_TAG "ClothingConverter"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {

ClothingConverter::ClothingConverter() {
}

ClothingConverter::~ClothingConverter() {
}

void ClothingConverter::initialize(const ESMManager* esmMgr) {
    esmManager = esmMgr;
    LOGI("ClothingConverter initialized");
}

inventory::Item ClothingConverter::convertToItem(const ClothingData& clothing) const {
    inventory::Item item;

    item.id = clothing.formID;
    item.name = clothing.fullName;
    item.description = "";  // ClothingData doesn't have description
    item.category = inventory::ItemCategory::Armor;  // Use Armor category for clothing
    item.rarity = inventory::ItemRarity::Common;  // Default rarity
    item.equipSlot = determineEquipSlot(clothing);
    item.weight = clothing.weight;
    item.value = clothing.value;
    item.maxStack = 1;  // Clothing is not stackable
    item.iconId = 0;    // TODO: Map model path to icon

    // Clothing has no stats by default (enchantment would add stats)
    item.stats.damage = 0;
    item.stats.defense = 0;

    LOGD("Converted clothing: %s (0x%08X)", clothing.fullName.c_str(), clothing.formID);
    return item;
}

std::vector<inventory::Item> ClothingConverter::convertAllClothing() const {
    std::vector<inventory::Item> items;

    if (!esmManager) {
        LOGW("ClothingConverter not initialized");
        return items;
    }

    const auto& clothingList = esmManager->getAllClothing();
    items.reserve(clothingList.size());

    for (const auto& clothing : clothingList) {
        items.push_back(convertToItem(clothing));
    }

    LOGI("Converted %zu clothing items", items.size());
    return items;
}

std::unique_ptr<inventory::Item> ClothingConverter::getClothingByFormID(uint32_t formID) const {
    if (!esmManager) return nullptr;

    const ClothingData* clothing = esmManager->findClothing(formID);
    if (!clothing) return nullptr;

    // BUG FIX #72: Return unique_ptr instead of raw pointer to prevent memory leak
    return std::make_unique<inventory::Item>(convertToItem(*clothing));
}

inventory::EquipSlot ClothingConverter::determineEquipSlot(const ClothingData& clothing) const {
    // Determine slot based on editorID or model path
    // This is a simplified heuristic - in a full implementation,
    // we'd parse the clothing type from the ESM data

    std::string editorID = clothing.editorID;
    std::string modelPath = clothing.modelPath;

    // Convert to lowercase for case-insensitive comparison
    std::string lowerEditorID = editorID;
    std::string lowerModelPath = modelPath;
    for (auto& c : lowerEditorID) c = std::tolower(c);
    for (auto& c : lowerModelPath) c = std::tolower(c);

    // Check for head slot indicators
    if (lowerEditorID.find("hat") != std::string::npos ||
        lowerEditorID.find("hood") != std::string::npos ||
        lowerEditorID.find("helmet") != std::string::npos ||
        lowerEditorID.find("circlet") != std::string::npos ||
        lowerModelPath.find("hat") != std::string::npos ||
        lowerModelPath.find("hood") != std::string::npos) {
        return inventory::EquipSlot::Head;
    }

    // BUG FIX #73: Check for accessory slot BEFORE hands to prevent ring conflict
    // Rings should be Accessory, not Hands
    if (lowerEditorID.find("amulet") != std::string::npos ||
        lowerEditorID.find("necklace") != std::string::npos ||
        lowerEditorID.find("pendant") != std::string::npos ||
        lowerEditorID.find("ring") != std::string::npos) {
        return inventory::EquipSlot::Accessory;
    }

    // Check for hand slot indicators (gloves/gauntlets only, NOT rings)
    if (lowerEditorID.find("glove") != std::string::npos ||
        lowerEditorID.find("gauntlet") != std::string::npos ||
        lowerModelPath.find("glove") != std::string::npos) {
        return inventory::EquipSlot::Hands;
    }

    // Check for feet slot indicators
    if (lowerEditorID.find("boot") != std::string::npos ||
        lowerEditorID.find("shoe") != std::string::npos ||
        lowerModelPath.find("boot") != std::string::npos) {
        return inventory::EquipSlot::Feet;
    }

    // BUG FIX #74: Detect pants/greaves/skirts - map to Body since no Legs slot exists
    // In original Oblivion these have a separate Legs slot, but this implementation
    // uses Body for all torso/lower body clothing
    if (lowerEditorID.find("pant") != std::string::npos ||
        lowerEditorID.find("greave") != std::string::npos ||
        lowerEditorID.find("skirt") != std::string::npos ||
        lowerEditorID.find("trouser") != std::string::npos) {
        return inventory::EquipSlot::Body;
    }

    // Default to body slot for robes, shirts, etc.
    return inventory::EquipSlot::Body;
}

} // namespace oblivion
