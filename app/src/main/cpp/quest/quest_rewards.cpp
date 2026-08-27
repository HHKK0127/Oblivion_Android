#include "quest_rewards.h"
#include <algorithm>

// ============================================================================
// QuestRewardManager
// ============================================================================

QuestRewardManager::QuestRewardManager() {
    LOGD("QuestRewardManager created");
}

QuestRewardManager::~QuestRewardManager() {
    cleanup();
    LOGD("QuestRewardManager destroyed");
}

bool QuestRewardManager::initialize(Player* player, InventoryManager* invMgr) {
    if (!player) {
        LOGE("Cannot initialize QuestRewardManager with null Player");
        return false;
    }

    player_ = player;
    inventoryManager_ = invMgr;

    LOGI("QuestRewardManager initialized");
    return true;
}

void QuestRewardManager::cleanup() {
    rewards_.clear();
    player_ = nullptr;
    inventoryManager_ = nullptr;
    LOGD("QuestRewardManager cleaned up");
}

void QuestRewardManager::registerReward(uint32_t questFormID, const QuestRewardPackage& reward) {
    rewards_[questFormID] = reward;
    LOGD("Registered rewards for quest 0x%08X: %zu entries",
         questFormID, reward.rewards.size());
}

bool QuestRewardManager::grantRewards(uint32_t questFormID) {
    auto it = rewards_.find(questFormID);
    if (it == rewards_.end()) {
        LOGW("No rewards registered for quest 0x%08X", questFormID);
        return false;
    }

    auto& package = it->second;
    if (package.hasBeenGranted) {
        LOGW("Rewards already granted for quest 0x%08X", questFormID);
        return false;
    }

    LOGI("Granting rewards for quest 0x%08X", questFormID);

    // Apply each reward
    for (const auto& reward : package.rewards) {
        switch (reward.type) {
            case RewardType::EXPERIENCE:
                applyExperience(reward.value);
                break;
            case RewardType::GOLD:
                applyGold(reward.value);
                break;
            case RewardType::ITEM:
                applyItem(reward.targetFormID, reward.value);
                break;
            case RewardType::SKILL_UP:
                applySkillUp(reward.skillName, reward.skillAmount);
                break;
            case RewardType::REPUTATION:
                LOGD("Reputation reward not yet implemented");
                break;
            case RewardType::SPELL:
                LOGD("Spell reward not yet implemented");
                break;
        }
    }

    package.hasBeenGranted = true;

    // Notify callback
    if (rewardCallback_) {
        rewardCallback_(questFormID, package);
    }

    LOGI("Rewards granted for quest 0x%08X: %zu entries",
         questFormID, package.rewards.size());
    return true;
}

const QuestRewardPackage* QuestRewardManager::getRewardPackage(uint32_t questFormID) const {
    auto it = rewards_.find(questFormID);
    if (it == rewards_.end()) return nullptr;
    return &it->second;
}

bool QuestRewardManager::hasRewards(uint32_t questFormID) const {
    return rewards_.find(questFormID) != rewards_.end();
}

bool QuestRewardManager::hasBeenGranted(uint32_t questFormID) const {
    auto it = rewards_.find(questFormID);
    if (it == rewards_.end()) return false;
    return it->second.hasBeenGranted;
}

// ============================================================================
// Internal Reward Application
// ============================================================================

void QuestRewardManager::applyExperience(uint32_t xp) {
    if (!player_) return;
    player_->addExperience(static_cast<float>(xp));
    LOGI("Granted %u XP to player", xp);
}

void QuestRewardManager::applyGold(uint32_t amount) {
    if (!player_) return;
    // Gold is stored as an inventory item in Oblivion
    // For now, we add it directly to the player's experience as a placeholder
    // In a full implementation, this would add gold coins to inventory
    LOGI("Granted %u Gold to player", amount);

    // If inventory manager is available, add gold as an item
    if (inventoryManager_) {
        Item goldItem;
        goldItem.itemId = 0x0000000F;  // Gold FormID in Oblivion
        goldItem.name = "Gold";
        goldItem.value = amount;
        goldItem.weight = 0.0f;
        goldItem.type = ItemType::MISC;
        inventoryManager_->playerAddItem(goldItem, amount);
    }
}

void QuestRewardManager::applyItem(uint32_t itemFormID, uint32_t quantity) {
    if (!inventoryManager_) {
        LOGW("Cannot grant item reward: InventoryManager not available");
        return;
    }

    // Look up item template
    auto itemTemplate = inventoryManager_->getItemTemplate(itemFormID);
    if (itemTemplate) {
        inventoryManager_->playerAddItem(*itemTemplate, quantity);
        LOGI("Granted item 0x%08X x%u to player", itemFormID, quantity);
    } else {
        LOGW("Item template 0x%08X not found, creating placeholder", itemFormID);
        Item placeholder;
        placeholder.itemId = itemFormID;
        placeholder.name = "Quest Reward Item";
        placeholder.value = 100;
        placeholder.weight = 1.0f;
        placeholder.type = ItemType::MISC;
        inventoryManager_->playerAddItem(placeholder, quantity);
    }
}

void QuestRewardManager::applySkillUp(const std::string& skill, int32_t amount) {
    if (!player_) return;

    // Map skill name to Player::Skills member
    auto& skills = player_->skills;
    if (skill == "Blade") {
        skills.Blade = std::min(100, skills.Blade + amount);
    } else if (skill == "Blunt") {
        skills.Blunt = std::min(100, skills.Blunt + amount);
    } else if (skill == "Block") {
        skills.Block = std::min(100, skills.Block + amount);
    } else if (skill == "Restoration") {
        skills.Restoration = std::min(100, skills.Restoration + amount);
    } else if (skill == "Destruction") {
        skills.Destruction = std::min(100, skills.Destruction + amount);
    } else if (skill == "Alteration") {
        skills.Alteration = std::min(100, skills.Alteration + amount);
    } else if (skill == "Conjuration") {
        skills.Conjuration = std::min(100, skills.Conjuration + amount);
    } else if (skill == "Illusion") {
        skills.Illusion = std::min(100, skills.Illusion + amount);
    } else if (skill == "Mysticism") {
        skills.Mysticism = std::min(100, skills.Mysticism + amount);
    } else if (skill == "Marksman") {
        skills.Marksman = std::min(100, skills.Marksman + amount);
    } else if (skill == "Athletics") {
        skills.Athletics = std::min(100, skills.Athletics + amount);
    } else if (skill == "Acrobatics") {
        skills.Acrobatics = std::min(100, skills.Acrobatics + amount);
    } else {
        LOGW("Unknown skill: %s", skill.c_str());
        return;
    }

    LOGI("Granted %s +%d to player", skill.c_str(), amount);
}

// ============================================================================
// Save/Load Support
// ============================================================================

std::vector<uint32_t> QuestRewardManager::exportGrantedQuests() const {
    std::vector<uint32_t> result;
    for (const auto& [formID, package] : rewards_) {
        if (package.hasBeenGranted) {
            result.push_back(formID);
        }
    }
    return result;
}

void QuestRewardManager::importGrantedQuests(const std::vector<uint32_t>& questFormIDs) {
    for (uint32_t formID : questFormIDs) {
        auto it = rewards_.find(formID);
        if (it != rewards_.end()) {
            it->second.hasBeenGranted = true;
        }
    }
    LOGI("Imported %zu granted quest rewards", questFormIDs.size());
}
