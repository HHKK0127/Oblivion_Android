#pragma once

#include <cstdint>
#include <functional>
#include <random>
#include <vector>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGW
#undef LOGE

#define LOG_TAG "LockpickSystem"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace game {

// Lock difficulty levels (matching Oblivion)
enum class LockDifficulty : uint8_t {
    VERY_EASY = 0,   // Skill 0-25
    EASY,            // Skill 25-50
    MEDIUM,          // Skill 50-75
    HARD,            // Skill 75-100
    VERY_HARD,       // Skill 100+ (master)
    IMPOSSIBLE,      // Cannot be picked
    COUNT
};

// Lockpick quality
enum class LockpickQuality : uint8_t {
    STANDARD = 0,    // Normal lockpick
    SKELETON_KEY,    // Never breaks (Thieves Guild reward)
    COUNT
};

// Tumbler state (Oblivion's lockpick minigame)
struct Tumbler {
    float position = 0.0f;      // Current position (0.0 to 1.0)
    float sweetSpotMin = 0.0f;  // Sweet spot start
    float sweetSpotMax = 0.0f;  // Sweet spot end
    float speed = 0.0f;         // Movement speed
    bool isSet = false;         // Successfully set
    bool isMoving = false;      // Currently moving
    
    bool isInSweetSpot() const {
        return position >= sweetSpotMin && position <= sweetSpotMax;
    }
};

// Lock state
struct LockState {
    uint32_t lockId = 0;
    LockDifficulty difficulty = LockDifficulty::VERY_EASY;
    uint32_t tumblerCount = 0;      // Number of tumblers (1-5)
    std::vector<Tumbler> tumblers;
    uint32_t currentTumbler = 0;    // Currently active tumbler
    bool isOpen = false;
    bool isBroken = false;          // Lockpick broke
    uint32_t attempts = 0;
    
    LockState() = default;
};

// Callbacks
using LockpickCallback = std::function<void(bool success, uint32_t lockId)>;
using TumblerCallback = std::function<void(uint32_t tumblerIndex, bool set)>;

/**
 * @brief Lockpicking Minigame System
 * 
 * Implements Oblivion's lockpicking minigame where players must
 * time button presses to set tumblers in their sweet spots.
 */
class LockpickSystem {
public:
    static LockpickSystem& getInstance() {
        static LockpickSystem instance;
        return instance;
    }
    
    // Initialization
    void initialize();
    void shutdown();
    void update(float deltaTime);
    
    // Lock interaction
    bool startLockpick(uint32_t lockId, LockDifficulty difficulty);
    void cancelLockpick();
    bool isLockpicking() const { return lockpickActive; }
    
    // Tumbler control
    void pressTumbler();            // Try to set current tumbler
    void releaseTumbler();          // Release current tumbler
    void nextTumbler();             // Move to next tumbler
    void prevTumbler();             // Move to previous tumbler
    
    // State queries
    const LockState& getLockState() const { return currentLock; }
    uint32_t getCurrentTumbler() const { return currentLock.currentTumbler; }
    bool isTumblerSet(uint32_t index) const;
    float getTumblerPosition(uint32_t index) const;
    bool isLockOpen() const { return currentLock.isOpen; }
    
    // Lockpick inventory
    uint32_t getLockpickCount() const { return lockpickCount; }
    void addLockpicks(uint32_t count);
    void setLockpickQuality(LockpickQuality quality);
    LockpickQuality getLockpickQuality() const { return lockpickQuality; }
    
    // Skill integration
    void setSecuritySkill(uint32_t skill) { securitySkill = skill; }
    uint32_t getSecuritySkill() const { return securitySkill; }
    
    // Auto-attempt (for high skill)
    bool canAutoAttempt() const { return securitySkill >= 75; }
    float getAutoSuccessChance() const;
    
    // Callbacks
    void setLockpickCallback(LockpickCallback callback) { onLockpickResult = callback; }
    void setTumblerCallback(TumblerCallback callback) { onTumblerSet = callback; }
    
    // Save/Load
    void saveState() const;
    void loadState();
    
private:
    LockpickSystem() = default;
    ~LockpickSystem() = default;
    LockpickSystem(const LockpickSystem&) = delete;
    LockpickSystem& operator=(const LockpickSystem&) = delete;
    
    // State
    bool initialized = false;
    bool lockpickActive = false;
    LockState currentLock;
    
    // Inventory
    uint32_t lockpickCount = 10;
    LockpickQuality lockpickQuality = LockpickQuality::STANDARD;
    
    // Skill
    uint32_t securitySkill = 0;
    
    // Tumbler animation
    float tumblerAnimTimer = 0.0f;
    static constexpr float TUMBLER_ANIM_SPEED = 2.0f;  // Oscillations per second
    
    // Sweet spot size based on difficulty and skill
    static constexpr float SWEET_SPOT_VERY_EASY = 0.4f;
    static constexpr float SWEET_SPOT_EASY = 0.3f;
    static constexpr float SWEET_SPOT_MEDIUM = 0.2f;
    static constexpr float SWEET_SPOT_HARD = 0.1f;
    static constexpr float SWEET_SPOT_VERY_HARD = 0.05f;
    
    // Tumbler speed based on difficulty
    static constexpr float TUMBLER_SPEED_VERY_EASY = 1.0f;
    static constexpr float TUMBLER_SPEED_EASY = 1.5f;
    static constexpr float TUMBLER_SPEED_MEDIUM = 2.0f;
    static constexpr float TUMBLER_SPEED_HARD = 3.0f;
    static constexpr float TUMBLER_SPEED_VERY_HARD = 4.0f;
    
    // Tumbler counts based on difficulty
    static constexpr uint32_t TUMBLERS_VERY_EASY = 1;
    static constexpr uint32_t TUMBLERS_EASY = 2;
    static constexpr uint32_t TUMBLERS_MEDIUM = 3;
    static constexpr uint32_t TUMBLERS_HARD = 4;
    static constexpr uint32_t TUMBLERS_VERY_HARD = 5;
    
    // Helper methods
    void initializeTumblers();
    void updateTumblerAnimation(float deltaTime);
    void breakLockpick();
    void openLock();
    float getSweetSpotSize() const;
    float getTumblerSpeed() const;
    uint32_t getTumblerCount() const;
    
    // Random number generation
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist{0.0f, 1.0f};
    
    // Callbacks
    LockpickCallback onLockpickResult;
    TumblerCallback onTumblerSet;
};

} // namespace game
