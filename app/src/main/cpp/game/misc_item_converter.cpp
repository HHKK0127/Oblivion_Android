#include "misc_item_converter.h"
#include <android/log.h>

#define LOG_TAG "MiscItemConverter"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {

MiscItemConverter::MiscItemConverter() {
}

MiscItemConverter::~MiscItemConverter() {
}

void MiscItemConverter::initialize(const ESMManager* esmMgr) {
    esmManager = esmMgr;
    LOGI("MiscItemConverter initialized");
}

inventory::Item MiscItemConverter::convertToItem(const MiscItemData& misc) const {
    inventory::Item item;

    item.id = misc.formID;
    item.name = misc.fullName;
    item.description = "A miscellaneous item";
    item.category = inventory::ItemCategory::Misc;
    item.rarity = inventory::ItemRarity::Common;
    item.equipSlot = inventory::EquipSlot::None;
    item.weight = misc.weight;
    item.value = misc.value;
    item.maxStack = 100;  // Misc items are stackable
    item.iconId = 0;

    item.stats.damage = 0;
    item.stats.defense = 0;

    LOGD("Converted misc item: %s (0x%08X)", misc.fullName.c_str(), misc.formID);
    return item;
}

std::vector<inventory::Item> MiscItemConverter::convertAllMiscItems() const {
    std::vector<inventory::Item> items;

    if (!esmManager) {
        LOGW("MiscItemConverter not initialized");
        return items;
    }

    const auto& miscList = esmManager->getAllMiscItems();
    items.reserve(miscList.size());

    for (const auto& misc : miscList) {
        items.push_back(convertToItem(misc));
    }

    LOGI("Converted %zu misc items", items.size());
    return items;
}

inventory::Item* MiscItemConverter::getMiscItemByFormID(uint32_t formID) const {
    if (!esmManager) return nullptr;

    const MiscItemData* misc = esmManager->findMiscItem(formID);
    if (!misc) return nullptr;

    return new inventory::Item(convertToItem(*misc));
}

} // namespace oblivion
