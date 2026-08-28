#include "sound_effect_manager.h"
#include "audio_manager.h"
#include <algorithm>
#include <cmath>

namespace audio {

SoundEffectManager::SoundEffectManager()
    : audioManager(nullptr),
      nextInstanceId(1),
      sfxVolume(1.0f),
      refDistance(1.0f),
      maxDistance(50.0f),
      rolloffFactor(1.0f),
      listenerPosition(0.0f, 0.0f, 0.0f) {
}

SoundEffectManager::~SoundEffectManager() {
    cleanup();
}

void SoundEffectManager::initialize(AudioManager* manager) {
    audioManager = manager;
    activeInstances.reserve(MAX_VOICES);
    LOGI("SoundEffectManager initialized (max voices=%u)", MAX_VOICES);
}

void SoundEffectManager::update(float deltaTime) {
    // Update instance ages and clean up finished ones
    std::vector<uint32_t> finishedIds;

    for (auto& inst : activeInstances) {
        if (!inst.active) {
            continue;
        }

        inst.age += deltaTime;

        // Check if the source is still playing
        if (audioManager) {
            auto* sourcePtr = reinterpret_cast<void*>(
                static_cast<uintptr_t>(inst.sourceId));
            // If source was cleaned up by AudioManager, mark as inactive
            if (audioManager->getActiveSourcesesCount() == 0) {
                inst.active = false;
                finishedIds.push_back(inst.instanceId);
            }
        }
    }

    // Remove finished instances
    for (uint32_t id : finishedIds) {
        activeInstances.erase(
            std::remove_if(activeInstances.begin(), activeInstances.end(),
                           [id](const SfxInstance& i) { return i.instanceId == id; }),
            activeInstances.end());
    }

    // Update volumes based on distance
    updateInstanceVolumes();
}

void SoundEffectManager::cleanup() {
    stopAll();
    sfxDefs.clear();
    activeInstances.clear();
    LOGI("SoundEffectManager cleaned up");
}

// ========== SFX Registration ==========

void SoundEffectManager::registerSfx(const SfxDef& def) {
    sfxDefs[def.key] = def;
    LOGD("SFX registered: %s -> %s (priority=%u, 3D=%s)",
         def.key.c_str(), def.filePath.c_str(),
         static_cast<unsigned int>(def.priority),
         def.is3D ? "true" : "false");
}

void SoundEffectManager::registerSfx(const std::string& key, const std::string& filePath,
                                      float volume, SfxPriority priority) {
    SfxDef def;
    def.key = key;
    def.filePath = filePath;
    def.baseVolume = volume;
    def.priority = priority;
    def.is3D = true;
    def.preload = false;
    registerSfx(def);
}

void SoundEffectManager::preloadAll() {
    if (!audioManager) {
        LOGE("SoundEffectManager not initialized");
        return;
    }

    uint32_t count = 0;
    for (auto& pair : sfxDefs) {
        SfxDef& def = pair.second;
        if (def.preload && def.clipId == 0) {
            def.clipId = audioManager->loadClip(def.filePath, 1, false);
            if (def.clipId != 0) {
                count++;
            }
        }
    }

    LOGI("Preloaded %u SFX clips", count);
}

bool SoundEffectManager::hasSfx(const std::string& key) const {
    return sfxDefs.find(key) != sfxDefs.end();
}

// ========== Playback ==========

uint32_t SoundEffectManager::play(const std::string& key,
                                    const glm::vec3& position,
                                    float volumeScale) {
    if (!audioManager) {
        LOGE("SoundEffectManager not initialized");
        return 0;
    }

    auto it = sfxDefs.find(key);
    if (it == sfxDefs.end()) {
        LOGW("SFX not found: %s", key.c_str());
        return 0;
    }

    SfxDef& def = it->second;

    // Load clip if not preloaded
    if (def.clipId == 0) {
        def.clipId = audioManager->loadClip(def.filePath, 1, false);
        if (def.clipId == 0) {
            LOGE("Failed to load SFX: %s", def.filePath.c_str());
            return 0;
        }
    }

    // Find a free voice or evict a lower-priority one
    uint32_t voiceSlot = findFreeVoice();
    if (voiceSlot == UINT32_MAX) {
        voiceSlot = evictLowestPriorityVoice(def.priority);
        if (voiceSlot == UINT32_MAX) {
            LOGW("No available voice for SFX: %s", key.c_str());
            return 0;
        }
    }

    // Calculate distance-based volume
    float distVolume = 1.0f;
    if (def.is3D) {
        distVolume = calculateAttenuation(position, listenerPosition,
                                           def.minDistance, def.maxDistance, rolloffFactor);
    }

    float effectiveVolume = def.baseVolume * sfxVolume * volumeScale * distVolume;

    // Play via AudioManager
    uint32_t sourceId = audioManager->playSE(def.clipId, position, effectiveVolume);
    if (sourceId == 0) {
        return 0;
    }

    // Create instance
    uint32_t instanceId = nextInstanceId++;
    SfxInstance inst;
    inst.instanceId = instanceId;
    inst.sourceId = sourceId;
    inst.key = key;
    inst.position = position;
    inst.volume = effectiveVolume;
    inst.priority = def.priority;
    inst.age = 0.0f;
    inst.active = true;

    activeInstances.push_back(inst);

    LOGD("SFX playing: %s instance=%u source=%u vol=%.2f dist_vol=%.2f",
         key.c_str(), instanceId, sourceId, effectiveVolume, distVolume);

    return instanceId;
}

uint32_t SoundEffectManager::play2D(const std::string& key, float volumeScale) {
    // Play at listener position (effectively 2D)
    return play(key, listenerPosition, volumeScale);
}

void SoundEffectManager::stop(uint32_t instanceId) {
    for (auto& inst : activeInstances) {
        if (inst.instanceId == instanceId && inst.active) {
            if (audioManager) {
                audioManager->stopSE(inst.sourceId);
            }
            inst.active = false;
            LOGD("SFX stopped: instance=%u", instanceId);
            return;
        }
    }
}

void SoundEffectManager::stopAll() {
    for (auto& inst : activeInstances) {
        if (inst.active && audioManager) {
            audioManager->stopSE(inst.sourceId);
        }
        inst.active = false;
    }
    activeInstances.clear();
    LOGD("All SFX stopped");
}

// ========== Volume ==========

void SoundEffectManager::setVolume(float volume) {
    sfxVolume = std::max(0.0f, std::min(1.0f, volume));
    updateInstanceVolumes();
}

// ========== Distance Model ==========

void SoundEffectManager::setDistanceModel(float rDist, float mDist, float rolloff) {
    refDistance = std::max(0.1f, rDist);
    maxDistance = std::max(1.0f, mDist);
    rolloffFactor = std::max(0.0f, rolloff);
    LOGD("SFX distance model: ref=%.1f max=%.1f rolloff=%.2f",
         refDistance, maxDistance, rolloffFactor);
}

float SoundEffectManager::calculateAttenuation(const glm::vec3& sourcePos,
                                                 const glm::vec3& listenerPos,
                                                 float rDist, float mDist, float rolloff) {
    float dx = sourcePos.x - listenerPos.x;
    float dy = sourcePos.y - listenerPos.y;
    float dz = sourcePos.z - listenerPos.z;
    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (distance >= mDist) {
        return 0.0f;
    }

    if (distance <= rDist) {
        return 1.0f;
    }

    // Inverse distance model with rolloff
    float attenuation = rDist / (rDist + rolloff * (distance - rDist));
    return std::max(0.0f, std::min(1.0f, attenuation));
}

// ========== Doppler Effect (Stub) ==========

void SoundEffectManager::setDopplerConfig(const DopplerConfig& config) {
    dopplerConfig = config;
    LOGD("Doppler config: factor=%.2f speed=%.1f",
         config.factor, config.speedOfSound);
}

float SoundEffectManager::calculateDoppler(const glm::vec3& sourcePos,
                                             const glm::vec3& sourceVel,
                                             const glm::vec3& listenerPos,
                                             const glm::vec3& listenerVel) const {
    if (dopplerConfig.factor <= 0.0f) {
        return 1.0f;  // No Doppler effect
    }

    // Calculate relative velocity along the source-listener axis
    float dx = sourcePos.x - listenerPos.x;
    float dy = sourcePos.y - listenerPos.y;
    float dz = sourcePos.z - listenerPos.z;
    float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    if (distance < 0.001f) {
        return 1.0f;
    }

    // Unit vector from listener to source
    float invDist = 1.0f / distance;
    float ux = dx * invDist;
    float uy = dy * invDist;
    float uz = dz * invDist;

    // Project velocities onto the axis
    float vListener = listenerVel.x * ux + listenerVel.y * uy + listenerVel.z * uz;
    float vSource = sourceVel.x * ux + sourceVel.y * uy + sourceVel.z * uz;

    // Doppler formula: f' = f * (c + vListener) / (c + vSource)
    float c = dopplerConfig.speedOfSound;
    float numerator = c + vListener * dopplerConfig.factor;
    float denominator = c + vSource * dopplerConfig.factor;

    if (std::abs(denominator) < 0.001f) {
        return 1.0f;
    }

    float pitch = numerator / denominator;

    // Clamp to reasonable range
    return std::max(0.5f, std::min(2.0f, pitch));
}

// ========== Environmental Reverb ==========

void SoundEffectManager::setReverbPreset(ReverbPreset preset) {
    currentReverb = ReverbConfig::getPreset(preset);
    LOGI("Reverb preset set: %u (room=%.2f wet=%.2f decay=%.1fs)",
         static_cast<unsigned int>(preset),
         currentReverb.roomSize, currentReverb.wetLevel,
         currentReverb.decayTime);
}

uint32_t SoundEffectManager::getActiveVoiceCount() const {
    uint32_t count = 0;
    for (const auto& inst : activeInstances) {
        if (inst.active) {
            count++;
        }
    }
    return count;
}

// ========== Internal Methods ==========

uint32_t SoundEffectManager::findFreeVoice() {
    // Look for an inactive slot
    for (auto& inst : activeInstances) {
        if (!inst.active) {
            return inst.instanceId;
        }
    }

    // If under max voices, we can add a new one
    if (activeInstances.size() < MAX_VOICES) {
        return UINT32_MAX;  // Signal to create new instance
    }

    return UINT32_MAX;  // No free voice
}

uint32_t SoundEffectManager::evictLowestPriorityVoice(SfxPriority requiredPriority) {
    // Find the lowest priority active instance that is lower than required
    uint32_t bestId = UINT32_MAX;
    SfxPriority lowestPriority = requiredPriority;
    float oldestAge = 0.0f;

    for (auto& inst : activeInstances) {
        if (!inst.active) {
            return inst.instanceId;  // Use inactive slot first
        }

        if (inst.priority < lowestPriority ||
            (inst.priority == lowestPriority && inst.age > oldestAge)) {
            lowestPriority = inst.priority;
            oldestAge = inst.age;
            bestId = inst.instanceId;
        }
    }

    if (bestId != UINT32_MAX) {
        // Evict the found instance
        stop(bestId);
        LOGD("Evicted SFX instance %u (priority=%u, age=%.1fs)",
             bestId, static_cast<unsigned int>(lowestPriority), oldestAge);
    }

    return bestId;
}

void SoundEffectManager::updateInstanceVolumes() {
    for (auto& inst : activeInstances) {
        if (!inst.active) {
            continue;
        }

        auto it = sfxDefs.find(inst.key);
        if (it == sfxDefs.end()) {
            continue;
        }

        const SfxDef& def = it->second;
        float newVolume = computeInstanceVolume(inst, def);

        // Update volume if changed significantly
        if (std::abs(newVolume - inst.volume) > 0.01f) {
            inst.volume = newVolume;
            // Volume update is handled by AudioManager's 3D system
        }
    }
}

float SoundEffectManager::computeInstanceVolume(const SfxInstance& inst,
                                                  const SfxDef& def) const {
    float distVolume = 1.0f;
    if (def.is3D) {
        distVolume = calculateAttenuation(inst.position, listenerPosition,
                                           def.minDistance, def.maxDistance, rolloffFactor);
    }
    return def.baseVolume * sfxVolume * distVolume;
}

} // namespace audio
