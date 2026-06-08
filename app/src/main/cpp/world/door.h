#pragma once

#include <glm/glm.hpp>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <android/log.h>

// ============================================================================
// Door System - Cell-to-Cell Transitions
// ============================================================================

#define LOG_TAG_DOOR "DoorManager"
#define LOGD_DOOR(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_DOOR, __VA_ARGS__)
#define LOGI_DOOR(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_DOOR, __VA_ARGS__)
#define LOGW_DOOR(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_DOOR, __VA_ARGS__)
#define LOGE_DOOR(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_DOOR, __VA_ARGS__)

// Forward declarations
class WorldManager;

// ============================================================================
// Door Data Structure
// ============================================================================

struct Door {
    uint32_t doorId;
    glm::vec3 position;           // Door position in source cell
    std::string modelPath;        // NIF file for 3D model
    glm::vec3 rotation;           // Door rotation

    // Destination information
    uint32_t destinationCell;     // Target cell ID
    glm::vec3 destinationPos;     // Spawn position in destination cell
    glm::vec3 destinationRotation;// Camera rotation at destination

    // Metadata
    std::string name;             // Display name ("Small House Door", etc.)
    std::string nameJa;           // Japanese name
    float interactionRadius;      // How close player needs to be (default 2.0m)

    // Constructor
    Door() : doorId(0), position(0.0f, 0.0f, 0.0f), rotation(0.0f, 0.0f, 0.0f),
             destinationCell(0), destinationPos(0.0f, 0.0f, 0.0f), destinationRotation(0.0f, 0.0f, 0.0f),
             interactionRadius(2.0f) {}

    Door(uint32_t id, const glm::vec3& pos, const std::string& nameEn,
         const std::string& nameJa_, uint32_t destCell, const glm::vec3& destPos)
        : doorId(id), position(pos), modelPath(""), rotation(0.0f, 0.0f, 0.0f),
          destinationCell(destCell), destinationPos(destPos), destinationRotation(0.0f, 0.0f, 0.0f),
          name(nameEn), nameJa(nameJa_), interactionRadius(2.0f) {}
};

// ============================================================================
// Door Manager
// ============================================================================

class DoorManager {
public:
    DoorManager();
    ~DoorManager();

    // Initialization
    bool initialize(WorldManager* worldMgr);
    void cleanup();

    // Door Registration
    void registerDoor(const Door& door);
    void registerDoor(uint32_t doorId, const glm::vec3& position,
                     const std::string& nameEn, const std::string& nameJa,
                     uint32_t destinationCell, const glm::vec3& destinationPos);

    // Door Queries
    const Door* getDoor(uint32_t doorId) const;
    const Door* getDoorAtPosition(const glm::vec3& position, float radius = 2.0f) const;
    const Door* getNearestDoor(const glm::vec3& position) const;

    // Door Interaction
    bool useDoor(uint32_t doorId);

    // Cell-specific queries
    std::vector<const Door*> getDoorsInCell(uint32_t cellId) const;

    // Statistics
    size_t getDoorCount() const { return doors.size(); }
    void logDoorStatus() const;

private:
    // Manager reference
    WorldManager* worldManager;

    // Door storage
    std::unordered_map<uint32_t, Door> doors;  // doorId → Door

    // Counter
    uint32_t nextDoorId;

    // Helper methods
    bool performCellTransition(const Door& door);
    bool validateDoorData(const Door& door) const;
};
