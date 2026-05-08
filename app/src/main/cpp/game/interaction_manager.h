#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include "interactable.h"
#include "door.h"
#include "container.h"

// Forward declarations
struct Cell;
class WorldManager;

/**
 * Interaction Manager - Manages all interactable objects and player interactions
 * Handles proximity detection, interaction updates, and callbacks
 */
class InteractionManager {
public:
    InteractionManager();
    ~InteractionManager();

    // Initialization
    bool initialize(WorldManager* worldMgr);

    // Object registration
    std::shared_ptr<Door> createDoor(std::shared_ptr<WorldObject> worldObj);
    std::shared_ptr<Container> createContainer(std::shared_ptr<WorldObject> worldObj);
    void registerInteractable(std::shared_ptr<Interactable> interactable);
    void unregisterInteractable(uint32_t refId);

    // Update - should be called each frame
    void update(const glm::vec3& playerPos, float deltaTime);

    // Interaction handling
    bool interact(uint32_t interactableId, const glm::vec3& playerPos);

    // Query interactables in range
    std::shared_ptr<Interactable> getHighlightedInteractable() const { return highlightedInteractable; }
    const std::vector<std::shared_ptr<Interactable>>& getNearbyInteractables() const {
        return nearbyInteractables;
    }

    // Get interactable by reference ID
    std::shared_ptr<Interactable> getInteractable(uint32_t refId);

    // Statistics
    size_t getInteractableCount() const { return interactables.size(); }
    size_t getNearbyCount() const { return nearbyInteractables.size(); }

    // Interaction parameters
    float getInteractionDistance() const { return interactionDistance; }
    void setInteractionDistance(float distance) { interactionDistance = distance; }

    // Memory cleanup
    void clearInteractables();

private:
    WorldManager* worldManager;
    std::unordered_map<uint32_t, std::shared_ptr<Interactable>> interactables;
    std::vector<std::shared_ptr<Interactable>> nearbyInteractables;
    std::shared_ptr<Interactable> highlightedInteractable;  // Currently closest interactable

    float interactionDistance;  // Maximum distance for interaction
    glm::vec3 lastPlayerPos;
    float proximityCheckTimer;
    static constexpr float PROXIMITY_UPDATE_INTERVAL = 0.1f;  // Update proximity every 100ms

    // Helper methods
    void updateProximity(const glm::vec3& playerPos);
    void updateHighlightedObject(const glm::vec3& playerPos);
    std::shared_ptr<Interactable> findClosestInteractable(const glm::vec3& playerPos);
};
