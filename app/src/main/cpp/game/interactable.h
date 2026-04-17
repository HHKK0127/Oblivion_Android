#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Forward declarations
class WorldObject;

/**
 * Base class for interactable world objects (doors, containers, furniture, etc.)
 * Handles proximity detection, interaction state, and callbacks
 */
class Interactable {
public:
    enum class ObjectType {
        DOOR,
        CONTAINER,
        ACTIVATOR,
        FURNITURE,
        UNKNOWN
    };

    enum class InteractionState {
        IDLE,
        HIGHLIGHTED,     // Player is in range
        INTERACTING,     // Interaction in progress
        ACTIVATED        // Completed interaction
    };

    Interactable(ObjectType type, std::shared_ptr<WorldObject> worldObj);
    virtual ~Interactable() = default;

    // Getters
    ObjectType getType() const { return type; }
    InteractionState getState() const { return state; }
    std::shared_ptr<WorldObject> getWorldObject() const { return worldObject; }
    const std::string& getInteractionText() const { return interactionText; }

    // Proximity detection
    virtual bool isInRange(const glm::vec3& playerPos, float interactionRadius);
    float getInteractionRadius() const { return interactionRadius; }
    void setInteractionRadius(float radius) { interactionRadius = radius; }

    // Interaction state management
    virtual void onPlayerEnterRange();
    virtual void onPlayerExitRange();
    virtual bool onInteract(const glm::vec3& playerPos);
    virtual void update(float deltaTime);

    // Enable/disable interactability
    bool isEnabled() const { return enabled; }
    void setEnabled(bool enable) { enabled = enable; }

protected:
    ObjectType type;
    InteractionState state;
    std::shared_ptr<WorldObject> worldObject;
    std::string interactionText;
    float interactionRadius;
    bool enabled;
    float highlightIntensity;

    // State transition helpers
    void setInteractionState(InteractionState newState);

private:
    float highlightTimer;
};
