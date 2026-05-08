#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <glm/glm.hpp>

// Forward declarations
class WorldManager;
class WorldObject;

// ============================================================================
// Interactable Base Class
// ============================================================================

class Interactable {
public:
    virtual ~Interactable() = default;

    virtual bool onInteract(const glm::vec3& playerPos) = 0;
    virtual void update(float deltaTime) = 0;
    virtual bool isEnabled() const { return enabled; }
    virtual void setEnabled(bool e) { enabled = e; }
    virtual bool isInRange(const glm::vec3& playerPos, float range) const = 0;
    virtual void onPlayerEnterRange() {}
    virtual void onPlayerExitRange() {}

    std::shared_ptr<WorldObject> getWorldObject() const { return worldObject; }

protected:
    std::shared_ptr<WorldObject> worldObject;
    bool enabled = true;
};

// ============================================================================
// Door
// ============================================================================

class Door : public Interactable {
public:
    Door(std::shared_ptr<WorldObject> obj);
    ~Door() override = default;

    bool onInteract(const glm::vec3& playerPos) override;
    void update(float deltaTime) override;
    bool isInRange(const glm::vec3& playerPos, float range) const override;
};

// ============================================================================
// Container
// ============================================================================

class Container : public Interactable {
public:
    Container(std::shared_ptr<WorldObject> obj);
    ~Container() override = default;

    bool onInteract(const glm::vec3& playerPos) override;
    void update(float deltaTime) override;
    bool isInRange(const glm::vec3& playerPos, float range) const override;
};

// ============================================================================
// Interaction Manager
// ============================================================================

#define PROXIMITY_UPDATE_INTERVAL 0.5f

class InteractionManager {
public:
    InteractionManager();
    ~InteractionManager();

    bool initialize(WorldManager* worldMgr);

    std::shared_ptr<Door> createDoor(std::shared_ptr<WorldObject> worldObj);
    std::shared_ptr<Container> createContainer(std::shared_ptr<WorldObject> worldObj);

    void registerInteractable(std::shared_ptr<Interactable> interactable);
    void unregisterInteractable(uint32_t refId);

    void update(const glm::vec3& playerPos, float deltaTime);
    void updateProximity(const glm::vec3& playerPos);
    void updateHighlightedObject(const glm::vec3& playerPos);

    std::shared_ptr<Interactable> findClosestInteractable(const glm::vec3& playerPos);
    bool interact(uint32_t interactableId, const glm::vec3& playerPos);

    std::shared_ptr<Interactable> getInteractable(uint32_t refId);

    void clearInteractables();

private:
    WorldManager* worldManager;
    float interactionDistance;
    glm::vec3 lastPlayerPos;
    float proximityCheckTimer;

    std::unordered_map<uint32_t, std::shared_ptr<Interactable>> interactables;
    std::vector<std::shared_ptr<Interactable>> nearbyInteractables;
    std::shared_ptr<Interactable> highlightedInteractable;
};