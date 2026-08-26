#include "loot_generator.h"
#include <android/log.h>

#define LOG_TAG "LootGenerator"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {

LootGenerator::LootGenerator() : rng(std::random_device{}()) {
}

LootGenerator::~LootGenerator() {
}

void LootGenerator::initialize(const ESMManager* esmMgr) {
    esmManager = esmMgr;
    LOGI("LootGenerator initialized");
}

std::vector<std::pair<uint32_t, uint16_t>> LootGenerator::generateLoot(
    uint32_t listFormID, uint32_t playerLevel) const {
    if (!esmManager) {
        LOGW("LootGenerator not initialized");
        return {};
    }

    return esmManager->resolveLeveledList(listFormID, playerLevel);
}

std::vector<std::pair<uint32_t, uint16_t>> LootGenerator::generateContainerLoot(
    uint32_t containerLevel, uint32_t playerLevel) const {
    if (!esmManager) {
        LOGW("LootGenerator not initialized");
        return {};
    }

    std::vector<std::pair<uint32_t, uint16_t>> loot;

    // Get all leveled lists
    const auto& lists = esmManager->getAllLeveledLists();
    if (lists.empty()) {
        return loot;
    }

    // Select a random subset of leveled lists based on container level
    int numLists = randomInt(1, 3);  // 1-3 items per container
    for (int i = 0; i < numLists; ++i) {
        // Pick a random leveled list
        int idx = randomInt(0, static_cast<int>(lists.size()) - 1);
        const auto& list = lists[idx];

        // Resolve the list
        auto items = esmManager->resolveLeveledList(list.formID, playerLevel);
        loot.insert(loot.end(), items.begin(), items.end());
    }

    LOGD("Generated %zu items for container (level %u)", loot.size(), containerLevel);
    return loot;
}

std::vector<std::pair<uint32_t, uint16_t>> LootGenerator::generateEnemyDrop(
    uint32_t enemyLevel, uint32_t playerLevel) const {
    if (!esmManager) {
        LOGW("LootGenerator not initialized");
        return {};
    }

    std::vector<std::pair<uint32_t, uint16_t>> loot;

    // Get all leveled lists
    const auto& lists = esmManager->getAllLeveledLists();
    if (lists.empty()) {
        return loot;
    }

    // Enemies drop 0-2 items based on level
    int numItems = randomInt(0, 2);
    for (int i = 0; i < numItems; ++i) {
        // Pick a random leveled list
        int idx = randomInt(0, static_cast<int>(lists.size()) - 1);
        const auto& list = lists[idx];

        // Resolve the list
        auto items = esmManager->resolveLeveledList(list.formID, playerLevel);
        loot.insert(loot.end(), items.begin(), items.end());
    }

    LOGD("Generated %zu items for enemy drop (level %u)", loot.size(), enemyLevel);
    return loot;
}

int LootGenerator::randomInt(int min, int max) const {
    if (min >= max) return min;
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

} // namespace oblivion
