#include "crime_reputation_system.h"
#include <algorithm>

namespace game {

// Faction IDs
static constexpr uint32_t FACTION_IMPERIAL_LEGION = 0x00001;
static constexpr uint32_t FACTION_FIGHTERS_GUILD = 0x00002;
static constexpr uint32_t FACTION_MAGES_GUILD = 0x00003;
static constexpr uint32_t FACTION_THIEVES_GUILD = 0x00004;
static constexpr uint32_t FACTION_DARK_BROTHERHOOD = 0x00005;
static constexpr uint32_t FACTION_BLADES = 0x00006;

void CrimeReputationSystem::initialize() {
    if (initialized) {
        LOGW("CrimeReputationSystem already initialized");
        return;
    }
    
    // Initialize default faction reputations
    factionReputations[FACTION_IMPERIAL_LEGION] = FactionReputation(0);
    factionReputations[FACTION_FIGHTERS_GUILD] = FactionReputation(0);
    factionReputations[FACTION_MAGES_GUILD] = FactionReputation(0);
    factionReputations[FACTION_THIEVES_GUILD] = FactionReputation(0);
    factionReputations[FACTION_DARK_BROTHERHOOD] = FactionReputation(0);
    factionReputations[FACTION_BLADES] = FactionReputation(0);
    
    initialized = true;
    LOGI("CrimeReputationSystem initialized");
}

void CrimeReputationSystem::shutdown() {
    if (!initialized) return;
    
    factionReputations.clear();
    crimeHistory.clear();
    crimeStats = CrimeStats();
    currentBounty = 0;
    inJail = false;
    jailTimeRemaining = 0;
    
    initialized = false;
    LOGI("CrimeReputationSystem shut down");
}

void CrimeReputationSystem::update(float deltaTime) {
    if (!initialized) return;
    
    // Update bounty decay
    updateBountyDecay(deltaTime);
    
    // Update jail time
    if (inJail) {
        updateJailTime(deltaTime);
    }
}

void CrimeReputationSystem::reportCrime(CrimeType type, uint32_t victimId, uint32_t witnessCount) {
    int32_t bounty = calculateBounty(type);
    reportCrimeWithBounty(type, victimId, bounty, witnessCount);
}

void CrimeReputationSystem::reportCrimeWithBounty(CrimeType type, uint32_t victimId, int32_t bounty, uint32_t witnessCount) {
    if (!initialized) return;
    
    CrimeRecord record;
    record.type = type;
    record.victimId = victimId;
    record.witnessCount = witnessCount;
    record.bounty = bounty;
    record.witnessed = (witnessCount > 0);
    record.resolved = false;
    
    // Add to history
    crimeHistory.push_back(record);
    crimeStats.totalCrimes++;
    crimeStats.lastCrimeTime = record.timestamp;
    
    // Only add bounty if witnessed
    if (record.witnessed) {
        currentBounty += bounty;
        notifyBountyChange();
    }
    
    // Modify faction reputation based on crime
    if (type == CrimeType::MURDER) {
        // Murder reduces reputation with all lawful factions
        for (auto& [factionId, rep] : factionReputations) {
            if (factionId != FACTION_THIEVES_GUILD && factionId != FACTION_DARK_BROTHERHOOD) {
                modifyReputation(factionId, -20);
            }
        }
    } else if (type == CrimeType::THEFT || type == CrimeType::PICKPOCKET) {
        // Theft reduces reputation with lawful factions
        for (auto& [factionId, rep] : factionReputations) {
            if (factionId != FACTION_THIEVES_GUILD && factionId != FACTION_DARK_BROTHERHOOD) {
                modifyReputation(factionId, -5);
            }
        }
    }
    
    notifyCrime(record);
    
    LOGI("Crime reported: type=%d victim=%u witnesses=%u bounty=%d total=%d",
         static_cast<int>(type), victimId, witnessCount, bounty, currentBounty);
}

void CrimeReputationSystem::setBounty(int32_t bounty) {
    currentBounty = std::max(0, bounty);
    notifyBountyChange();
}

void CrimeReputationSystem::clearBounty() {
    currentBounty = 0;
    notifyBountyChange();
}

BountyLevel CrimeReputationSystem::getBountyLevel() const {
    if (currentBounty <= 0) return BountyLevel::NONE;
    if (currentBounty < MEDIUM_BOUNTY_THRESHOLD) return BountyLevel::LOW;
    if (currentBounty < HIGH_BOUNTY_THRESHOLD) return BountyLevel::MEDIUM;
    return BountyLevel::HIGH;
}

GuardResponse CrimeReputationSystem::getGuardResponse() const {
    BountyLevel level = getBountyLevel();
    switch (level) {
        case BountyLevel::NONE: return GuardResponse::IGNORE;
        case BountyLevel::LOW: return GuardResponse::WARNING;
        case BountyLevel::MEDIUM: return GuardResponse::PURSUE;
        case BountyLevel::HIGH: return GuardResponse::ATTACK;
        default: return GuardResponse::IGNORE;
    }
}

int32_t CrimeReputationSystem::getReputation(uint32_t factionId) const {
    auto it = factionReputations.find(factionId);
    if (it != factionReputations.end()) {
        return it->second.reputation;
    }
    return 0;
}

void CrimeReputationSystem::setReputation(uint32_t factionId, int32_t reputation) {
    reputation = std::clamp(reputation, -100, 100);
    factionReputations[factionId].reputation = reputation;
}

void CrimeReputationSystem::modifyReputation(uint32_t factionId, int32_t delta) {
    int32_t current = getReputation(factionId);
    setReputation(factionId, current + delta);
}

bool CrimeReputationSystem::isHostile(uint32_t factionId) const {
    auto it = factionReputations.find(factionId);
    if (it != factionReputations.end()) {
        return it->second.isHostile;
    }
    return false;
}

void CrimeReputationSystem::setHostile(uint32_t factionId, bool hostile) {
    factionReputations[factionId].isHostile = hostile;
}

bool CrimeReputationSystem::isMember(uint32_t factionId) const {
    auto it = factionReputations.find(factionId);
    if (it != factionReputations.end()) {
        return it->second.isMember;
    }
    return false;
}

void CrimeReputationSystem::setMember(uint32_t factionId, bool member) {
    factionReputations[factionId].isMember = member;
}

void CrimeReputationSystem::arrestPlayer() {
    if (!initialized) return;
    
    inJail = true;
    jailTimeRemaining = currentBounty / 100;  // 1 hour per 100 gold bounty
    jailTimeRemaining = std::max(jailTimeRemaining, static_cast<uint32_t>(24));  // Minimum 24 hours
    
    crimeStats.timesArrested++;
    
    // Clear bounty after arrest
    currentBounty = 0;
    notifyBountyChange();
    
    if (onArrest) {
        onArrest();
    }
    
    LOGI("Player arrested. Jail time: %u hours", jailTimeRemaining);
}

void CrimeReputationSystem::serveTime(uint32_t hours) {
    if (!inJail) return;
    
    jailTimeRemaining = (hours >= jailTimeRemaining) ? 0 : jailTimeRemaining - hours;
    
    if (jailTimeRemaining == 0) {
        inJail = false;
        crimeStats.timesServedTime++;
        crimeStats.totalJailTime += hours;
        
        // Reduce reputation slightly after serving time
        for (auto& [factionId, rep] : factionReputations) {
            if (!rep.isMember) {
                modifyReputation(factionId, 5);  // Small reputation recovery
            }
        }
        
        LOGI("Player released from jail after %u hours", hours);
    }
}

void CrimeReputationSystem::escapeJail() {
    if (!inJail) return;
    
    inJail = false;
    jailTimeRemaining = 0;
    
    // Add escape bounty
    currentBounty += ESCAPE_BOUNTY;
    notifyBountyChange();
    
    crimeStats.timesEscaped++;
    
    // Reduce reputation with lawful factions
    for (auto& [factionId, rep] : factionReputations) {
        if (factionId != FACTION_THIEVES_GUILD && factionId != FACTION_DARK_BROTHERHOOD) {
            modifyReputation(factionId, -10);
        }
    }
    
    LOGI("Player escaped from jail. New bounty: %d", currentBounty);
}

void CrimeReputationSystem::clearCrimeHistory() {
    crimeHistory.clear();
    crimeStats = CrimeStats();
}

void CrimeReputationSystem::saveState() const {
    // TODO: Implement save to file
    LOGI("CrimeReputationSystem state saved");
}

void CrimeReputationSystem::loadState() {
    // TODO: Implement load from file
    LOGI("CrimeReputationSystem state loaded");
}

int32_t CrimeReputationSystem::calculateBounty(CrimeType type) const {
    switch (type) {
        case CrimeType::TRESPASS: return TRESPASS_BOUNTY;
        case CrimeType::THEFT: return THEFT_BOUNTY;
        case CrimeType::ASSAULT: return ASSAULT_BOUNTY;
        case CrimeType::MURDER: return MURDER_BOUNTY;
        case CrimeType::PICKPOCKET: return PICKPOCKET_BOUNTY;
        case CrimeType::ESCAPE: return ESCAPE_BOUNTY;
        default: return 0;
    }
}

void CrimeReputationSystem::updateBountyDecay(float deltaTime) {
    if (currentBounty <= 0) return;
    
    bountyDecayTimer += deltaTime;
    if (bountyDecayTimer >= BOUNTY_DECAY_INTERVAL) {
        bountyDecayTimer -= BOUNTY_DECAY_INTERVAL;
        currentBounty = std::max(0, currentBounty - BOUNTY_DECAY_AMOUNT);
        notifyBountyChange();
    }
}

void CrimeReputationSystem::updateJailTime(float deltaTime) {
    // Convert deltaTime to game hours (assuming 30x time scale)
    float gameHours = deltaTime * 30.0f / 3600.0f;
    jailTimer += gameHours;
    
    if (jailTimer >= 1.0f) {
        uint32_t hoursToServe = static_cast<uint32_t>(jailTimer);
        jailTimer -= hoursToServe;
        serveTime(hoursToServe);
    }
}

void CrimeReputationSystem::notifyCrime(const CrimeRecord& record) {
    if (onCrime) {
        onCrime(record);
    }
}

void CrimeReputationSystem::notifyBountyChange() {
    if (onBountyChange) {
        onBountyChange(currentBounty, getBountyLevel());
    }
}

} // namespace game
