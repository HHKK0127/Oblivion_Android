#pragma once

// Audio Subscriber
// Listens for game events via Imperial Weave EventBus
// and plays appropriate sound effects via AudioManager

#include "../engine/imperial_weave.h"
#include "audio_manager.h"
#include <string>
#include <unordered_map>
#include <android/log.h>

#define LOG_TAG "AudioSubscriber"
#define LOGD_AUD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI_AUD(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace audio {

class AudioSubscriber {
public:
    AudioSubscriber() = default;
    ~AudioSubscriber() = default;

    // Initialize with EventBus and AudioManager
    void init(weave::EventBus* bus, AudioManager* audio);

    // Subscribe to game events
    void subscribeToEvents();

    // Set player position for 3D audio
    void setPlayerPosition(const glm::vec3& pos) { playerPosition = pos; }

    // Set NPC position lookup (optional, for 3D positional audio)
    void setNpcPositionCallback(std::function<glm::vec3(uint32_t)> callback) {
        npcPositionCallback = callback;
    }

private:
    weave::EventBus* eventBus = nullptr;
    AudioManager* audioManager = nullptr;

    glm::vec3 playerPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    std::function<glm::vec3(uint32_t)> npcPositionCallback;

    // Event type → sound key mapping
    std::unordered_map<std::string, std::string> soundMap;

    // Weapon type → attack sound key mapping
    std::unordered_map<std::string, std::string> weaponAttackSounds;

    void playSoundForEvent(const std::string& eventType, uint32_t targetId = 0);
    void playSoundForEventWithPayload(const std::string& eventType,
                                      uint32_t targetId,
                                      const std::string& payload);
    glm::vec3 getSoundPosition(uint32_t targetId);
    static std::string extractPayloadField(const std::string& payload, const std::string& key);
};

} // namespace audio