#include "lod_system.h"
#include <cmath>
#include <algorithm>

namespace oblivion {

LODSystem::LODSystem() 
    : enabled_(true)
    , cameraX_(0), cameraY_(0), cameraZ_(0)
    , fov_(60.0f)
    , screenHeight_(1080.0f) {
}

LODSystem::~LODSystem() = default;

void LODSystem::initialize(const LODConfig& config) {
    config_ = config;
    enabled_ = config.enabled;
}

void LODSystem::update(float cameraX, float cameraY, float cameraZ,
                       float fov, float screenHeight) {
    if (!enabled_) return;

    cameraX_ = cameraX;
    cameraY_ = cameraY;
    cameraZ_ = cameraZ;
    fov_ = fov;
    screenHeight_ = screenHeight;

    // Update LOD levels for all registered meshes
    for (auto& [id, node] : nodes_) {
        // Check if LOD is forced
        auto forcedIt = forcedLODs_.find(id);
        if (forcedIt != forcedLODs_.end()) {
            node.currentLevel = forcedIt->second;
            continue;
        }

        // Calculate distance to camera
        // Note: In a real implementation, you would use the actual object position
        // For now, we use a placeholder distance
        float distance = 50.0f; // Placeholder

        // Calculate screen size
        float screenSize = calculateScreenSize(distance, node.boundingRadius, fov_, screenHeight_);

        // Determine LOD level with hysteresis
        int newLevel = determineLODLevel(screenSize, node.levels);
        
        // Apply hysteresis to prevent flickering
        if (newLevel != node.currentLevel) {
            float hysteresisDistance = config_.hysteresis;
            if (std::abs(distance - node.lastSwitchDistance) < hysteresisDistance) {
                // Keep current level if within hysteresis range
                newLevel = node.currentLevel;
            } else {
                node.lastSwitchDistance = distance;
            }
        }

        node.currentLevel = newLevel;
    }
}

bool LODSystem::registerMesh(uint32_t id, float boundingRadius,
                             const std::vector<LODLevel>& levels) {
    if (levels.empty()) {
        return false;
    }

    LODNode node;
    node.id = id;
    node.boundingRadius = boundingRadius;
    node.levels = levels;
    node.currentLevel = 0;
    node.lastSwitchDistance = 0.0f;

    nodes_[id] = node;
    return true;
}

void LODSystem::unregisterMesh(uint32_t id) {
    nodes_.erase(id);
    forcedLODs_.erase(id);
}

int LODSystem::getCurrentLOD(uint32_t id) const {
    auto it = nodes_.find(id);
    if (it != nodes_.end()) {
        return it->second.currentLevel;
    }
    return -1;
}

int LODSystem::getLODAtDistance(uint32_t id, float distance) const {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) {
        return -1;
    }

    float screenSize = calculateScreenSize(distance, it->second.boundingRadius, fov_, screenHeight_);
    return determineLODLevel(screenSize, it->second.levels);
}

void LODSystem::forceLOD(uint32_t id, int level) {
    auto it = nodes_.find(id);
    if (it != nodes_.end() && level >= 0 && level < static_cast<int>(it->second.levels.size())) {
        forcedLODs_[id] = level;
    }
}

void LODSystem::resetForcedLOD(uint32_t id) {
    forcedLODs_.erase(id);
}

void LODSystem::setEnabled(bool enabled) {
    enabled_ = enabled;
}

bool LODSystem::isEnabled() const {
    return enabled_;
}

const LODConfig& LODSystem::getConfig() const {
    return config_;
}

void LODSystem::setConfig(const LODConfig& config) {
    config_ = config;
    enabled_ = config.enabled;
}

LODSystem::Statistics LODSystem::getStatistics() const {
    Statistics stats{};
    stats.totalMeshes = static_cast<int>(nodes_.size());
    stats.meshesAtLOD0 = 0;
    stats.meshesAtLOD1 = 0;
    stats.meshesAtLOD2 = 0;
    stats.meshesAtLOD3 = 0;
    stats.averageLOD = 0.0f;

    if (nodes_.empty()) {
        return stats;
    }

    float totalLOD = 0.0f;
    for (const auto& [id, node] : nodes_) {
        switch (node.currentLevel) {
            case 0: stats.meshesAtLOD0++; break;
            case 1: stats.meshesAtLOD1++; break;
            case 2: stats.meshesAtLOD2++; break;
            case 3: stats.meshesAtLOD3++; break;
        }
        totalLOD += node.currentLevel;
    }

    stats.averageLOD = totalLOD / stats.totalMeshes;
    return stats;
}

void LODSystem::clear() {
    nodes_.clear();
    forcedLODs_.clear();
}

float LODSystem::calculateScreenSize(float distance, float boundingRadius,
                                     float fov, float screenHeight) const {
    if (distance <= 0.0f) {
        return screenHeight;  // Object is at camera position
    }

    // Calculate angular size in radians
    float angularSize = 2.0f * std::atan(boundingRadius / distance);
    
    // Convert to screen pixels
    float fovRadians = fov * 3.14159f / 180.0f;
    float screenSize = (angularSize / fovRadians) * screenHeight;

    return screenSize;
}

int LODSystem::determineLODLevel(float screenSize, const std::vector<LODLevel>& levels) const {
    if (levels.empty()) {
        return 0;
    }

    // Find the appropriate LOD level based on screen size
    for (int i = 0; i < static_cast<int>(levels.size()); ++i) {
        if (screenSize >= levels[i].screenSize) {
            return i;
        }
    }

    // Return lowest detail level
    return static_cast<int>(levels.size()) - 1;
}

} // namespace oblivion
