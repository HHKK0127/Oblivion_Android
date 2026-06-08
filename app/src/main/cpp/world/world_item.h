#pragma once

#include <string>
#include <glm/glm.hpp>
#include <cmath>

/**
 * @brief ワールドに配置される拾えるアイテム
 */
class WorldItem {
public:
    uint32_t worldItemId;
    uint32_t itemId;
    std::string itemName;
    std::string itemNameJa;
    glm::vec3 position;
    bool isPickedUp;

    WorldItem(uint32_t wid, uint32_t iid, const std::string& name,
              const std::string& nameJa, const glm::vec3& pos)
        : worldItemId(wid), itemId(iid), itemName(name), itemNameJa(nameJa),
          position(pos), isPickedUp(false) {}

    bool isInPickupRange(const glm::vec3& playerPos, float range) const {
        glm::vec3 diff = position - playerPos;
        float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        return distSq <= range * range;
    }

    float getDistanceTo(const glm::vec3& playerPos) const {
        glm::vec3 diff = position - playerPos;
        return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    }
};
