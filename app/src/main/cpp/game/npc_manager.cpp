#include "npc_manager.h"
#include <algorithm>
#include <cmath>

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
    cellNpcs.clear();
    npcToCell.clear();
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
    result.reserve(npcs.size());  // Pre-allocate space for all NPCs
    for (const auto& pair : npcs) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<std::shared_ptr<NPC>> NpcManager::getNPCsInArea(const glm::vec3& center, float radius) const {
    std::vector<std::shared_ptr<NPC>> result;
    result.reserve(10);  // Pre-allocate space for typical nearby NPC count
    float radiusSq = radius * radius;  // Avoid sqrt by comparing squared distances

    for (const auto& pair : npcs) {
        if (pair.second) {
            glm::vec3 diff = pair.second->position - center;
            float distanceSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
            if (distanceSq <= radiusSq) {
                result.push_back(pair.second);
            }
        }
    }
    return result;
}

// Cell Integration Methods (NEW)
std::vector<std::shared_ptr<NPC>> NpcManager::getNpcsForCell(uint32_t cellId) const {
    std::vector<std::shared_ptr<NPC>> result;
    auto it = cellNpcs.find(cellId);
    if (it != cellNpcs.end()) {
        result.reserve(it->second.size());
        for (uint32_t npcId : it->second) {
            auto npc = getNPC(npcId);
            if (npc) {
                result.push_back(npc);
            }
        }
    }
    LOGD("getNpcsForCell: Cell=%u returned %zu NPCs", cellId, result.size());
    return result;
}

void NpcManager::registerNpcToCell(uint32_t npcId, uint32_t cellId) {
    // Check if NPC is already in this cell
    auto npcCellIt = npcToCell.find(npcId);
    if (npcCellIt != npcToCell.end() && npcCellIt->second == cellId) {
        // Already in this cell, nothing to do
        return;
    }

    // If NPC is in a different cell, remove from old cell first
    if (npcCellIt != npcToCell.end()) {
        uint32_t oldCellId = npcCellIt->second;
        auto oldCellNpcsIt = cellNpcs.find(oldCellId);
        if (oldCellNpcsIt != cellNpcs.end()) {
            auto& npcList = oldCellNpcsIt->second;
            npcList.erase(std::remove(npcList.begin(), npcList.end(), npcId), npcList.end());
            LOGD("NPC moved from cell %u to cell %u: ID=%u", oldCellId, cellId, npcId);
        }
    }

    // Add NPC to new cell
    cellNpcs[cellId].push_back(npcId);
    npcToCell[npcId] = cellId;

    LOGD("NPC registered to cell: NPC=%u, Cell=%u", npcId, cellId);
}

void NpcManager::unregisterNpcFromCell(uint32_t npcId) {
    auto cellIt = npcToCell.find(npcId);
    if (cellIt == npcToCell.end()) {
        LOGW("NPC not registered in any cell: ID=%u", npcId);
        return;
    }

    uint32_t cellId = cellIt->second;

    // Remove NPC from cell's NPC list
    auto npcsIt = cellNpcs.find(cellId);
    if (npcsIt != cellNpcs.end()) {
        auto& npcList = npcsIt->second;
        npcList.erase(std::remove(npcList.begin(), npcList.end(), npcId), npcList.end());
    }

    // Remove NPC to cell mapping
    npcToCell.erase(cellIt);

    LOGD("NPC unregistered from cell: NPC=%u, Cell=%u", npcId, cellId);
}

uint32_t NpcManager::getNpcCell(uint32_t npcId) const {
    auto it = npcToCell.find(npcId);
    if (it == npcToCell.end()) {
        return UINT32_MAX;  // Invalid cell ID (no cell assigned)
    }
    return it->second;
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
