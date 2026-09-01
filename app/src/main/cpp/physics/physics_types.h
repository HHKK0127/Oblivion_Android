#pragma once
#include <glm/glm.hpp>
#include <functional>

namespace oblivion {

enum class PhysicsLayer : uint8_t {
    NON_MOVING = 0,  // Terrain, buildings
    MOVING = 1,      // Player, NPC, dynamic objects
    TRIGGER = 2,     // Door, chest (for future expansion)
    NUM_LAYERS
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    float maxDistance = 1000.0f;
};

struct RaycastHit {
    glm::vec3 point{0.0f, 0.0f, 0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    float distance = 0.0f;
    bool hit = false;
};

using TriggerCallback = std::function<void(uint32_t bodyId, bool entered)>;

} // namespace oblivion
