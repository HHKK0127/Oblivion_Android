#pragma once
#include <glm/glm.hpp>
#include <functional>

namespace oblivion {

enum class PhysicsLayer : uint8_t {
    NON_MOVING = 0,  // 地形、建物
    MOVING = 1,      // プレイヤー、NPC、動的オブジェクト
    TRIGGER = 2,     // ドア、チェスト（将来拡張用）
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
