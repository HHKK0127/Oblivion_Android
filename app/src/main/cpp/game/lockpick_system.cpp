#include "lockpick_system.h"
#include <algorithm>

namespace game {

void LockpickSystem::initialize() {
    if (initialized) {
        LOGW("LockpickSystem already initialized");
        return;
    }
    
    initialized = true;
    LOGI("LockpickSystem initialized");
}

void LockpickSystem::shutdown() {
    if (!initialized) return;
    
    lockpickActive = false;
    currentLock = LockState();
    
    initialized = false;
    LOGI("LockpickSystem shut down");
}

void LockpickSystem::update(float deltaTime) {
    if (!initialized || !lockpickActive) return;
    
    // Update tumbler animation
    updateTumblerAnimation(deltaTime);
}

bool LockpickSystem::startLockpick(uint32_t lockId, LockDifficulty difficulty) {
    if (!initialized) return false;
    
    // Check if we have lockpicks
    if (lockpickCount <= 0 && lockpickQuality != LockpickQuality::SKELETON_KEY) {
        LOGW("No lockpicks available");
        return false;
    }
    
    // Check if difficulty is impossible
    if (difficulty == LockDifficulty::IMPOSSIBLE) {
        LOGW("Lock is impossible to pick");
        return false;
    }
    
    // Initialize lock state
    currentLock.lockId = lockId;
    currentLock.difficulty = difficulty;
    currentLock.tumblerCount = getTumblerCount();
    currentLock.currentTumbler = 0;
    currentLock.isOpen = false;
    currentLock.isBroken = false;
    currentLock.attempts = 0;
    
    // Initialize tumblers
    initializeTumblers();
    
    lockpickActive = true;
    
    LOGI("Started lockpick: lockId=%u difficulty=%d tumblers=%u",
         lockId, static_cast<int>(difficulty), currentLock.tumblerCount);
    
    return true;
}

void LockpickSystem::cancelLockpick() {
    if (!lockpickActive) return;
    
    lockpickActive = false;
    currentLock = LockState();
    
    LOGI("Lockpick cancelled");
}

void LockpickSystem::pressTumbler() {
    if (!lockpickActive || currentLock.isOpen) return;
    
    Tumbler& tumbler = currentLock.tumblers[currentLock.currentTumbler];
    if (tumbler.isSet) return;
    
    currentLock.attempts++;
    
    // Check if tumbler is in sweet spot
    if (tumbler.isInSweetSpot()) {
        tumbler.isSet = true;
        tumbler.isMoving = false;
        
        LOGI("Tumbler %u set at position %.2f (sweet spot: %.2f-%.2f)",
             currentLock.currentTumbler, tumbler.position,
             tumbler.sweetSpotMin, tumbler.sweetSpotMax);
        
        if (onTumblerSet) {
            onTumblerSet(currentLock.currentTumbler, true);
        }
        
        // Check if all tumblers are set
        bool allSet = true;
        for (const auto& t : currentLock.tumblers) {
            if (!t.isSet) {
                allSet = false;
                break;
            }
        }
        
        if (allSet) {
            openLock();
        } else {
            // Move to next unset tumbler
            nextTumbler();
        }
    } else {
        // Failed - break lockpick
        breakLockpick();
    }
}

void LockpickSystem::releaseTumbler() {
    if (!lockpickActive) return;
    
    Tumbler& tumbler = currentLock.tumblers[currentLock.currentTumbler];
    if (tumbler.isSet) return;
    
    tumbler.isMoving = false;
}

void LockpickSystem::nextTumbler() {
    if (!lockpickActive) return;
    
    // Find next unset tumbler
    for (uint32_t i = 0; i < currentLock.tumblerCount; i++) {
        uint32_t next = (currentLock.currentTumbler + 1 + i) % currentLock.tumblerCount;
        if (!currentLock.tumblers[next].isSet) {
            currentLock.currentTumbler = next;
            return;
        }
    }
}

void LockpickSystem::prevTumbler() {
    if (!lockpickActive) return;
    
    // Find previous unset tumbler
    for (uint32_t i = 0; i < currentLock.tumblerCount; i++) {
        uint32_t prev = (currentLock.currentTumbler + currentLock.tumblerCount - 1 - i) % currentLock.tumblerCount;
        if (!currentLock.tumblers[prev].isSet) {
            currentLock.currentTumbler = prev;
            return;
        }
    }
}

bool LockpickSystem::isTumblerSet(uint32_t index) const {
    if (index >= currentLock.tumblerCount) return false;
    return currentLock.tumblers[index].isSet;
}

float LockpickSystem::getTumblerPosition(uint32_t index) const {
    if (index >= currentLock.tumblerCount) return 0.0f;
    return currentLock.tumblers[index].position;
}

void LockpickSystem::addLockpicks(uint32_t count) {
    lockpickCount += count;
    LOGI("Added %u lockpicks, total: %u", count, lockpickCount);
}

void LockpickSystem::setLockpickQuality(LockpickQuality quality) {
    lockpickQuality = quality;
}

float LockpickSystem::getAutoSuccessChance() const {
    if (securitySkill < 75) return 0.0f;
    
    // Higher skill = higher chance
    float baseChance = (securitySkill - 75) / 25.0f;  // 0.0 to 1.0
    return std::min(1.0f, baseChance * 0.5f);  // Max 50% chance
}

void LockpickSystem::saveState() const {
    // TODO: Implement save to file
    LOGI("LockpickSystem state saved");
}

void LockpickSystem::loadState() {
    // TODO: Implement load from file
    LOGI("LockpickSystem state loaded");
}

void LockpickSystem::initializeTumblers() {
    currentLock.tumblers.clear();
    currentLock.tumblers.resize(currentLock.tumblerCount);
    
    float sweetSpotSize = getSweetSpotSize();
    float tumblerSpeed = getTumblerSpeed();
    
    for (uint32_t i = 0; i < currentLock.tumblerCount; i++) {
        Tumbler& tumbler = currentLock.tumblers[i];
        
        // Randomize sweet spot position
        float sweetSpotCenter = dist(rng) * (1.0f - sweetSpotSize);
        tumbler.sweetSpotMin = sweetSpotCenter;
        tumbler.sweetSpotMax = sweetSpotCenter + sweetSpotSize;
        
        // Set speed (varies slightly per tumbler)
        tumbler.speed = tumblerSpeed * (0.8f + dist(rng) * 0.4f);
        
        // Start at random position
        tumbler.position = dist(rng);
        tumbler.isSet = false;
        tumbler.isMoving = true;
    }
}

void LockpickSystem::updateTumblerAnimation(float deltaTime) {
    tumblerAnimTimer += deltaTime;
    
    for (auto& tumbler : currentLock.tumblers) {
        if (tumbler.isSet || !tumbler.isMoving) continue;
        
        // Move tumbler up and down
        tumbler.position += tumbler.speed * deltaTime;
        if (tumbler.position >= 1.0f) {
            tumbler.position = 1.0f;
            tumbler.speed = -tumbler.speed;  // Reverse direction
        } else if (tumbler.position <= 0.0f) {
            tumbler.position = 0.0f;
            tumbler.speed = -tumbler.speed;  // Reverse direction
        }
    }
}

void LockpickSystem::breakLockpick() {
    currentLock.isBroken = true;
    
    // Don't break skeleton key
    if (lockpickQuality != LockpickQuality::SKELETON_KEY) {
        lockpickCount--;
        LOGI("Lockpick broken! Remaining: %u", lockpickCount);
    }
    
    // Reset tumblers
    initializeTumblers();
    currentLock.attempts = 0;
    
    if (lockpickCount <= 0 && lockpickQuality != LockpickQuality::SKELETON_KEY) {
        // Out of lockpicks
        lockpickActive = false;
        
        if (onLockpickResult) {
            onLockpickResult(false, currentLock.lockId);
        }
        
        LOGI("Out of lockpicks, lockpick failed");
    }
}

void LockpickSystem::openLock() {
    currentLock.isOpen = true;
    lockpickActive = false;
    
    if (onLockpickResult) {
        onLockpickResult(true, currentLock.lockId);
    }
    
    LOGI("Lock opened! lockId=%u attempts=%u", currentLock.lockId, currentLock.attempts);
}

float LockpickSystem::getSweetSpotSize() const {
    // Skill increases sweet spot size
    float skillBonus = securitySkill / 100.0f * 0.2f;  // Up to 20% bonus
    
    switch (currentLock.difficulty) {
        case LockDifficulty::VERY_EASY: return SWEET_SPOT_VERY_EASY + skillBonus;
        case LockDifficulty::EASY: return SWEET_SPOT_EASY + skillBonus;
        case LockDifficulty::MEDIUM: return SWEET_SPOT_MEDIUM + skillBonus;
        case LockDifficulty::HARD: return SWEET_SPOT_HARD + skillBonus;
        case LockDifficulty::VERY_HARD: return SWEET_SPOT_VERY_HARD + skillBonus;
        default: return 0.0f;
    }
}

float LockpickSystem::getTumblerSpeed() const {
    // Skill reduces tumbler speed
    float skillReduction = securitySkill / 100.0f * 0.5f;  // Up to 50% reduction
    
    switch (currentLock.difficulty) {
        case LockDifficulty::VERY_EASY: return TUMBLER_SPEED_VERY_EASY * (1.0f - skillReduction);
        case LockDifficulty::EASY: return TUMBLER_SPEED_EASY * (1.0f - skillReduction);
        case LockDifficulty::MEDIUM: return TUMBLER_SPEED_MEDIUM * (1.0f - skillReduction);
        case LockDifficulty::HARD: return TUMBLER_SPEED_HARD * (1.0f - skillReduction);
        case LockDifficulty::VERY_HARD: return TUMBLER_SPEED_VERY_HARD * (1.0f - skillReduction);
        default: return 0.0f;
    }
}

uint32_t LockpickSystem::getTumblerCount() const {
    switch (currentLock.difficulty) {
        case LockDifficulty::VERY_EASY: return TUMBLERS_VERY_EASY;
        case LockDifficulty::EASY: return TUMBLERS_EASY;
        case LockDifficulty::MEDIUM: return TUMBLERS_MEDIUM;
        case LockDifficulty::HARD: return TUMBLERS_HARD;
        case LockDifficulty::VERY_HARD: return TUMBLERS_VERY_HARD;
        default: return 0;
    }
}

} // namespace game
