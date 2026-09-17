#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGW
#undef LOGE

#define LOG_TAG "CrimeSystem"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace game {

// Crime types matching Oblivion's crime system
enum class CrimeType : uint8_t {
    TRESPASS = 0,      // Entering restricted areas
    THEFT,             // Stealing items
    ASSAULT,           // Attacking NPCs
    MURDER,            // Killing NPCs
    PICKPOCKET,        // Pickpocketing
    ESCAPE,            // Escaping from jail
    WEREWOLF,          // Werewolf transformation (Shivering Isles)
    COUNT
};

// Bounty thresholds
enum class BountyLevel : uint8_t {
    NONE = 0,          // No bounty
    LOW,               // 1-999 gold
    MEDIUM,            // 1000-2999 gold
    HIGH,              // 3000+ gold
    COUNT
};

// Guard response based on bounty
enum class GuardResponse : uint8_t {
    IGNORE = 0,        // Ignore criminal
    WARNING,           // Warn criminal
    PURSUE,            // Chase criminal
    ATTACK,            // Attack on sight
    COUNT
};

// Reputation with factions
struct FactionReputation {
    int32_t reputation = 0;     // -100 to 100
    int32_t bounty = 0;         // Current bounty in gold
    bool isHostile = false;     // Faction is hostile to player
    bool isMember = false;      // Player is member of faction
    
    FactionReputation() = default;
    FactionReputation(int32_t rep) : reputation(rep) {}
};

// Crime record
struct CrimeRecord {
    CrimeType type;
    uint32_t victimId;          // NPC or faction ID
    uint32_t witnessCount;
    int32_t bounty;
    float timestamp;            // Game time when crime occurred
    bool witnessed;             // Was crime seen
    bool resolved;              // Has crime been resolved (paid bounty, served time, etc.)
    
    CrimeRecord() 
        : type(CrimeType::TRESPASS), victimId(0), witnessCount(0), 
          bounty(0), timestamp(0.0f), witnessed(false), resolved(false) {}
};

// Crime statistics
struct CrimeStats {
    uint32_t totalCrimes = 0;
    uint32_t timesArrested = 0;
    uint32_t timesEscaped = 0;
    uint32_t timesServedTime = 0;
    uint32_t totalBountyPaid = 0;
    uint32_t totalJailTime = 0;     // In game hours
    float lastCrimeTime = 0.0f;
};

// Callbacks
using CrimeCallback = std::function<void(const CrimeRecord&)>;
using BountyCallback = std::function<void(int32_t newBounty, BountyLevel level)>;
using ArrestCallback = std::function<void()>;

/**
 * @brief Crime & Reputation System
 * 
 * Manages player crimes, bounties, faction reputation, and guard behavior.
 * Based on Oblivion's crime mechanics.
 */
class CrimeReputationSystem {
public:
    static CrimeReputationSystem& getInstance() {
        static CrimeReputationSystem instance;
        return instance;
    }
    
    // Initialization
    void initialize();
    void shutdown();
    void update(float deltaTime);
    
    // Crime reporting
    void reportCrime(CrimeType type, uint32_t victimId, uint32_t witnessCount = 1);
    void reportCrimeWithBounty(CrimeType type, uint32_t victimId, int32_t bounty, uint32_t witnessCount = 1);
    
    // Bounty management
    int32_t getBounty() const { return currentBounty; }
    void setBounty(int32_t bounty);
    void clearBounty();
    BountyLevel getBountyLevel() const;
    GuardResponse getGuardResponse() const;
    
    // Faction reputation
    int32_t getReputation(uint32_t factionId) const;
    void setReputation(uint32_t factionId, int32_t reputation);
    void modifyReputation(uint32_t factionId, int32_t delta);
    bool isHostile(uint32_t factionId) const;
    void setHostile(uint32_t factionId, bool hostile);
    bool isMember(uint32_t factionId) const;
    void setMember(uint32_t factionId, bool member);
    
    // Jail system
    void arrestPlayer();
    void serveTime(uint32_t hours);
    void escapeJail();
    bool isInJail() const { return inJail; }
    uint32_t getJailTimeRemaining() const { return jailTimeRemaining; }
    
    // Crime history
    const std::vector<CrimeRecord>& getCrimeHistory() const { return crimeHistory; }
    const CrimeStats& getCrimeStats() const { return crimeStats; }
    void clearCrimeHistory();
    
    // Callbacks
    void setCrimeCallback(CrimeCallback callback) { onCrime = callback; }
    void setBountyCallback(BountyCallback callback) { onBountyChange = callback; }
    void setArrestCallback(ArrestCallback callback) { onArrest = callback; }
    
    // Save/Load
    void saveState() const;
    void loadState();
    
private:
    CrimeReputationSystem() = default;
    ~CrimeReputationSystem() = default;
    CrimeReputationSystem(const CrimeReputationSystem&) = delete;
    CrimeReputationSystem& operator=(const CrimeReputationSystem&) = delete;
    
    // State
    bool initialized = false;
    int32_t currentBounty = 0;
    bool inJail = false;
    uint32_t jailTimeRemaining = 0;
    float jailTimer = 0.0f;
    
    // Faction data
    std::unordered_map<uint32_t, FactionReputation> factionReputations;
    
    // Crime history
    std::vector<CrimeRecord> crimeHistory;
    CrimeStats crimeStats;
    
    // Bounty decay
    float bountyDecayTimer = 0.0f;
    static constexpr float BOUNTY_DECAY_INTERVAL = 3600.0f;  // 1 game hour
    static constexpr int32_t BOUNTY_DECAY_AMOUNT = 1;        // 1 gold per hour
    
    // Crime type bounty values
    static constexpr int32_t TRESPASS_BOUNTY = 5;
    static constexpr int32_t THEFT_BOUNTY = 25;
    static constexpr int32_t ASSAULT_BOUNTY = 40;
    static constexpr int32_t MURDER_BOUNTY = 1000;
    static constexpr int32_t PICKPOCKET_BOUNTY = 25;
    static constexpr int32_t ESCAPE_BOUNTY = 100;
    
    // Bounty level thresholds
    static constexpr int32_t LOW_BOUNTY_THRESHOLD = 1;
    static constexpr int32_t MEDIUM_BOUNTY_THRESHOLD = 1000;
    static constexpr int32_t HIGH_BOUNTY_THRESHOLD = 3000;
    
    // Callbacks
    CrimeCallback onCrime;
    BountyCallback onBountyChange;
    ArrestCallback onArrest;
    
    // Helper methods
    int32_t calculateBounty(CrimeType type) const;
    void updateBountyDecay(float deltaTime);
    void updateJailTime(float deltaTime);
    void notifyCrime(const CrimeRecord& record);
    void notifyBountyChange();
};

} // namespace game
