#include "npc_manager.h"
#include <algorithm>
#include <glm/geometric.hpp>

NpcManager::NpcManager() : nextNpcId(1000) {
    LOGD("NpcManager created");
}

NpcManager::~NpcManager() {
    cleanup();
    LOGD("NpcManager destroyed");
}

bool NpcManager::initialize() {
    LOGI("NpcManager initialized");
    return true;
}

void NpcManager::cleanup() {
    npcs.clear();
    LOGD("NpcManager cleaned up");
}

void NpcManager::update(float deltaTime) {
    for (auto& pair : npcs) {
        if (pair.second) {
            pair.second->update(deltaTime);
        }
    }
}

std::shared_ptr<NPC> NpcManager::createNPC(const std::string& name, const glm::vec3& position) {
    uint32_t npcId = nextNpcId++;
    auto npc = std::make_shared<NPC>(npcId, name);
    npc->position = position;
    npcs[npcId] = npc;

    LOGD("NPC created: ID=%u, Name=%s, Pos=(%.1f, %.1f, %.1f)",
         npcId, name.c_str(), position.x, position.y, position.z);
    return npc;
}

std::shared_ptr<NPC> NpcManager::getNPC(uint32_t npcId) const {
    auto it = npcs.find(npcId);
    if (it == npcs.end()) {
        return nullptr;
    }
    return it->second;
}

void NpcManager::removeNPC(uint32_t npcId) {
    auto it = npcs.find(npcId);
    if (it != npcs.end()) {
        LOGD("NPC removed: ID=%u", npcId);
        npcs.erase(it);
    }
}

std::vector<std::shared_ptr<NPC>> NpcManager::getAllNPCs() const {
    std::vector<std::shared_ptr<NPC>> result;
    for (const auto& pair : npcs) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<std::shared_ptr<NPC>> NpcManager::getNPCsInArea(const glm::vec3& center, float radius) const {
    std::vector<std::shared_ptr<NPC>> result;
    for (const auto& pair : npcs) {
        if (pair.second) {
            float distance = glm::distance(pair.second->position, center);
            if (distance <= radius) {
                result.push_back(pair.second);
            }
        }
    }
    return result;
}

void NpcManager::logNpcStatus() const {
    LOGD("========== NPC Manager Status ==========");
    LOGD("Total NPCs: %zu", npcs.size());
    for (const auto& pair : npcs) {
        if (pair.second) {
            LOGD("  NPC: %s (ID=%u, HP=%.1f/%.1f)",
                 pair.second->name.c_str(), pair.second->npcId,
                 pair.second->status.currentHealth, pair.second->status.maxHealth);
        }
    }
    LOGD("=======================================");
}

// Phase 1: Movement AI Setup Methods

void NpcManager::addPatrolWaypoint(uint32_t npcId, const glm::vec3& waypoint) {
    auto npc = getNPC(npcId);
    if (!npc) {
        LOGW("NPC %u not found for patrol waypoint addition", npcId);
        return;
    }

    npc->patrolWaypoints.push_back(waypoint);
    LOGD("Added patrol waypoint to NPC %u: (%.1f, %.1f, %.1f) [total: %zu]",
         npcId, waypoint.x, waypoint.y, waypoint.z, npc->patrolWaypoints.size());
}

void NpcManager::setNPCWanderRadius(uint32_t npcId, float radius) {
    auto npc = getNPC(npcId);
    if (!npc) {
        LOGW("NPC %u not found for wander radius setting", npcId);
        return;
    }

    if (radius < 0.0f) {
        LOGW("Invalid wander radius %.1f for NPC %u, ignoring", radius, npcId);
        return;
    }

    npc->wanderRadius = radius;
    LOGD("Set wander radius for NPC %u to %.1f", npcId, radius);
}

void NpcManager::setNPCAIState(uint32_t npcId, AIState state) {
    auto npc = getNPC(npcId);
    if (!npc) {
        LOGW("NPC %u not found for AI state change", npcId);
        return;
    }

    npc->setAIState(state);
}
