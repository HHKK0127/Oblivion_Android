#pragma once

#include "interactable.h"
#include <glm/glm.hpp>

/**
 * Door - An interactable door that can be opened/closed
 * Supports rotation animation and locked state
 */
class Door : public Interactable {
public:
    enum class DoorState {
        CLOSED,
        OPENING,
        OPEN,
        CLOSING
    };

    Door(std::shared_ptr<WorldObject> worldObj);
    ~Door() override = default;

    // Door-specific state
    DoorState getDoorState() const { return doorState; }
    bool isLocked() const { return locked; }
    void setLocked(bool lock) { locked = lock; }

    // Interaction
    bool onInteract(const glm::vec3& playerPos) override;
    void update(float deltaTime) override;

    // Door-specific properties
    float getOpenAngle() const { return openAngle; }
    void setOpenAngle(float angle) { openAngle = angle; }

    float getAnimationDuration() const { return animationDuration; }
    void setAnimationDuration(float duration) { animationDuration = duration; }

    // Get current rotation based on animation state
    glm::vec3 getCurrentRotation() const { return currentRotation; }

    // Teleportation to paired door (for interior doors)
    void setTeleportDoor(std::shared_ptr<Door> paired) { pairedDoor = paired; }
    std::shared_ptr<Door> getPairedDoor() const { return pairedDoor; }

private:
    DoorState doorState;
    bool locked;
    float openAngle;
    float animationDuration;
    float animationProgress;  // 0.0 to 1.0
    glm::vec3 originalRotation;
    glm::vec3 currentRotation;
    std::shared_ptr<Door> pairedDoor;  // For paired doors (entrance/exit)

    void updateAnimation(float deltaTime);
    void updateInteractionText();
};
