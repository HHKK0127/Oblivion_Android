#include "audio_subscriber.h"

namespace audio {

void AudioSubscriber::init(weave::EventBus* bus, AudioManager* audio) {
    eventBus = bus;
    audioManager = audio;

    // Event → sound key mapping
    // Uses dedicated combat sounds. Equip sounds are used only for ANIM_EQUIP_START.
    soundMap = {
        {"ANIM_ATTACK_START",   "combat/attack_swing"},
        {"ANIM_ATTACK_HIT",     "combat/hit_generic"},
        {"COMBAT_ATTACK_HIT",   "combat/hit_generic"},
        {"COMBAT_CRITICAL_HIT", "combat/hit_critical"},
        {"COMBAT_BLOCK",        "combat/block_generic"},
        {"COMBAT_PARRY",        "combat/parry"},
        {"COMBAT_DODGE",        "combat/dodge"},
        {"ANIM_BLOCK_START",    "combat/block_generic"},
        {"ANIM_EQUIP_START",    "combat/blade_equip"},
        {"ANIM_DEATH_START",    "combat/death_generic"},
        {"COMBAT_DEATH",        "combat/death_generic"}
    };

    // Weapon-specific attack sounds (used by payload-based lookup in future)
    weaponAttackSounds = {
        {"blade",   "combat/attack_swing"},
        {"blunt",   "combat/hit_blunt"},
        {"axe",     "combat/hit_axe"},
        {"spear",   "combat/attack_swing"},
        {"bow",     "combat/bow_shoot"},
        {"staff",   "combat/attack_swing"},
        {"unarmed", "combat/hit_unarmed"}
    };

    LOGI_AUD("AudioSubscriber initialized");
}

void AudioSubscriber::subscribeToEvents() {
    if (!eventBus || !audioManager) return;

    eventBus->subscribe("ANIM_ATTACK_START", [this](const weave::Event& e) {
        playSoundForEvent("ANIM_ATTACK_START", e.targetId);
    });

    eventBus->subscribe("ANIM_ATTACK_HIT", [this](const weave::Event& e) {
        playSoundForEventWithPayload("ANIM_ATTACK_HIT", e.targetId, e.payload);
    });

    eventBus->subscribe("COMBAT_ATTACK_HIT", [this](const weave::Event& e) {
        playSoundForEventWithPayload("COMBAT_ATTACK_HIT", e.targetId, e.payload);
    });

    eventBus->subscribe("COMBAT_CRITICAL_HIT", [this](const weave::Event& e) {
        playSoundForEventWithPayload("COMBAT_CRITICAL_HIT", e.targetId, e.payload);
    });

    eventBus->subscribe("COMBAT_BLOCK", [this](const weave::Event& e) {
        playSoundForEvent("COMBAT_BLOCK", e.targetId);
    });

    eventBus->subscribe("COMBAT_PARRY", [this](const weave::Event& e) {
        playSoundForEvent("COMBAT_PARRY", e.targetId);
    });

    eventBus->subscribe("COMBAT_DODGE", [this](const weave::Event& e) {
        playSoundForEvent("COMBAT_DODGE", e.targetId);
    });

    eventBus->subscribe("ANIM_BLOCK_START", [this](const weave::Event& e) {
        playSoundForEvent("ANIM_BLOCK_START", e.targetId);
    });

    eventBus->subscribe("ANIM_DEATH_START", [this](const weave::Event& e) {
        playSoundForEvent("ANIM_DEATH_START", e.targetId);
    });

    eventBus->subscribe("COMBAT_DEATH", [this](const weave::Event& e) {
        playSoundForEvent("COMBAT_DEATH", e.targetId);
    });

    eventBus->subscribe("ANIM_EQUIP_START", [this](const weave::Event& e) {
        playSoundForEvent("ANIM_EQUIP_START", e.targetId);
    });

    LOGI_AUD("AudioSubscriber subscribed to events");
}

void AudioSubscriber::playSoundForEvent(const std::string& eventType, uint32_t targetId) {
    if (!audioManager) return;

    // For hit/attack events, route by weapon type from payload
    std::string soundKey;
    if (eventType == "COMBAT_ATTACK_HIT" || eventType == "ANIM_ATTACK_HIT") {
        // Try weapon-type-specific hit sound (payload set by CombatManager)
        soundKey = "combat/hit_generic"; // default
    } else if (eventType == "COMBAT_CRITICAL_HIT") {
        soundKey = "combat/hit_critical";
    } else {
        auto it = soundMap.find(eventType);
        if (it == soundMap.end()) {
            LOGD_AUD("No sound mapping for event: %s", eventType.c_str());
            return;
        }
        soundKey = it->second;
    }

    glm::vec3 pos = getSoundPosition(targetId);
    uint32_t sourceId = audioManager->playSound(soundKey, pos);

    if (sourceId == 0) {
        LOGD_AUD("Failed to play sound '%s' for event '%s'", soundKey.c_str(), eventType.c_str());
    } else {
        LOGD_AUD("Played sound '%s' for event '%s' (source=%u)", soundKey.c_str(), eventType.c_str(), sourceId);
    }
}

void AudioSubscriber::playSoundForEventWithPayload(const std::string& eventType,
                                                    uint32_t targetId,
                                                    const std::string& payload) {
    if (!audioManager) return;

    std::string soundKey;

    if (eventType == "COMBAT_ATTACK_HIT" || eventType == "ANIM_ATTACK_HIT") {
        // Parse weaponType from payload: {"weaponType":"blade",...}
        std::string weapType = extractPayloadField(payload, "weaponType");
        auto it = weaponAttackSounds.find(weapType);
        if (it != weaponAttackSounds.end()) {
            // Map weapon attack sounds to hit sounds
            if (weapType == "blade")   soundKey = "combat/hit_blade";
            else if (weapType == "blunt") soundKey = "combat/hit_blunt";
            else if (weapType == "axe")   soundKey = "combat/hit_axe";
            else if (weapType == "bow")   soundKey = "combat/bow_shoot";
            else if (weapType == "unarmed") soundKey = "combat/hit_unarmed";
            else soundKey = "combat/hit_generic";
        } else {
            soundKey = "combat/hit_generic";
        }
    } else if (eventType == "COMBAT_CRITICAL_HIT") {
        soundKey = "combat/hit_critical";
    } else {
        auto it = soundMap.find(eventType);
        if (it == soundMap.end()) return;
        soundKey = it->second;
    }

    glm::vec3 pos = getSoundPosition(targetId);
    audioManager->playSound(soundKey, pos);
    LOGD_AUD("Played '%s' for '%s' weaponType='%s'",
             soundKey.c_str(), eventType.c_str(),
             extractPayloadField(payload, "weaponType").c_str());
}

std::string AudioSubscriber::extractPayloadField(const std::string& payload,
                                                   const std::string& key) {
    // Simple JSON field extractor: finds "key":"value"
    std::string search = "\"" + key + "\":\"";
    auto pos = payload.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    auto end = payload.find('"', pos);
    if (end == std::string::npos) return "";
    return payload.substr(pos, end - pos);
}

glm::vec3 AudioSubscriber::getSoundPosition(uint32_t targetId) {
    // If target NPC position is available, use it for 3D audio
    if (targetId > 0 && npcPositionCallback) {
        return npcPositionCallback(targetId);
    }
    // Otherwise play at player position (2D-ish)
    return playerPosition;
}

} // namespace audio