#pragma once

#include "quest_record.h"
#include "../game/player.h"
#include "../game/inventory_manager.h"
#include <vector>
#include <string>
#include <cstdint>
#include <functional>
#include <android/log.h>

#define LOG_TAG "QuestRewards"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// Quest Rewards
// Phase 39: Handles quest completion rewards (XP, gold, items, skills)
// ============================================================================

// ============================================================================
// Reward Type
// ============================================================================
enum class RewardType : uint8_t {
    EXPERIENCE = 0,
    GOLD       = 1,
    ITEM       = 2,
    SKILL_UP   = 3,
    REPUTATION = 4,
    SPELL      = 5
};

// ============================================================================
// Reward Entry
// ============================================================================
struct RewardEntry {
    RewardType type = RewardType::EXPERIENCE;
    uint32_t value = 0;              // Amount (XP, gold, quantity)
    uint32_t targetFormID = 0;       // Item/spell FormID
    std::string skillName;           // For SKILL_UP rewards
    int32_t skillAmount = 0;         // Skill increase amount
    std::string description;         // Display text

    // Factory methods
    static RewardEntry experience(uint32_t xp) {
        RewardEntry r;
        r.type = RewardType::EXPERIENCE;
        r.value = xp;
        r.description = std::to_string(xp) + " XP";
        return r;
    }

    static RewardEntry gold(uint32_t amount) {
        RewardEntry r;
        r.type = RewardType::GOLD;
        r.value = amount;
        r.description = std::to_string(amount) + " Gold";
        return r;
    }

    static RewardEntry item(uint32_t itemFormID, uint32_t quantity = 1) {
        RewardEntry r;
        r.type = RewardType::ITEM;
        r.value = quantity;
        r.targetFormID = itemFormID;
        return r;
    }

    static RewardEntry skillUp(const std::string& skill, int32_t amount) {
        RewardEntry r;
        r.type = RewardType::SKILL_UP;
        r.skillName = skill;
        r.skillAmount = amount;
        r.description = skill + " +" + std::to_string(amount);
        return r;
    }
};

// ============================================================================
// Quest Reward Package
// ============================================================================
struct QuestRewardPackage {
    uint32_t questFormID = 0;
    std::vector<RewardEntry> rewards;
    bool hasBeenGranted = false;

    void addExperience(uint32_t xp) {
        rewards.push_back(RewardEntry::experience(xp));
    }
    void addGold(uint32_t amount) {
        rewards.push_back(RewardEntry::gold(amount));
    }
    void addItem(uint32_t itemFormID, uint32_t quantity = 1) {
        rewards.push_back(RewardEntry::item(itemFormID, quantity));
    }
    void addSkillUp(const std::string& skill, int32_t amount) {
        rewards.push_back(RewardEntry::skillUp(skill, amount));
    }

    uint32_t getTotalExperience() const {
        uint32_t total = 0;
        for (const auto& r : rewards) {
            if (r.type == RewardType::EXPERIENCE) total += r.value;
        }
        return total;
    }

    uint32_t getTotalGold() const {
        uint32_t total = 0;
        for (const auto& r : rewards) {
            if (r.type == RewardType::GOLD) total += r.value;
        }
        return total;
    }
};

// ============================================================================
// Reward Callback
// ============================================================================
using RewardGrantedCallback = std::function<void(uint32_t questFormID,
                                                   const QuestRewardPackage& rewards)>;

// ============================================================================
// Quest Reward Manager
// ============================================================================
class QuestRewardManager {
public:
    QuestRewardManager();
    ~QuestRewardManager();

    // Initialize with game systems
    bool initialize(Player* player, InventoryManager* invMgr);

    void cleanup();

    // Register rewards for a quest
    void registerReward(uint32_t questFormID, const QuestRewardPackage& reward);

    // Grant rewards for quest completion
    bool grantRewards(uint32_t questFormID);

    // Query
    const QuestRewardPackage* getRewardPackage(uint32_t questFormID) const;
    bool hasRewards(uint32_t questFormID) const;
    bool hasBeenGranted(uint32_t questFormID) const;

    // Callbacks
    void setRewardGrantedCallback(RewardGrantedCallback callback) {
        rewardCallback_ = std::move(callback);
    }

    // Save/Load support
    std::vector<uint32_t> exportGrantedQuests() const;
    void importGrantedQuests(const std::vector<uint32_t>& questFormIDs);

private:
    // Reward storage (questFormID -> reward package)
    std::unordered_map<uint32_t, QuestRewardPackage> rewards_;

    // Game system pointers
    Player* player_ = nullptr;
    InventoryManager* inventoryManager_ = nullptr;

    // Callback
    RewardGrantedCallback rewardCallback_;

    // Internal reward application
    void applyExperience(uint32_t xp);
    void applyGold(uint32_t amount);
    void applyItem(uint32_t itemFormID, uint32_t quantity);
    void applySkillUp(const std::string& skill, int32_t amount);
};
