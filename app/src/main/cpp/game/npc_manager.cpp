#include "npc_manager.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>

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

// ============================================================
// ESM Data Integration
// ============================================================

std::shared_ptr<NPC> NpcManager::createNPCFromESM(uint32_t formID, const glm::vec3& position) {
    if (!m_esm) {
        LOGW("createNPCFromESM: ESMManager not set, falling back to createNPC");
        return createNPC("Unknown", position);
    }

    // Try creature first
    const oblivion::CreatureData* creature = m_esm->findCreature(formID);
    if (creature) {
        uint32_t npcId = nextNpcId++;
        auto npc = std::make_shared<NPC>(npcId, creature->fullName.empty() ? creature->editorID : creature->fullName);
        npc->position = position;
        npc->race = "Creature";
        npc->class_ = "Creature";

        // Apply ESM creature stats
        npc->status.maxHealth = static_cast<float>(creature->health);
        npc->status.currentHealth = npc->status.maxHealth;
        npc->status.maxMana = 0.0f;
        npc->status.currentMana = 0.0f;
        npc->status.maxStamina = static_cast<float>(creature->combat + creature->stealth);
        npc->status.stamina = npc->status.maxStamina;
        npc->status.level = creature->level;
        npc->status.weaponDamage = static_cast<float>(creature->attackDamage);

        // Set mesh path from ESM model
        npc->meshAssetPath = creature->modelPath;

        npcs[npcId] = npc;
        LOGI("Creature spawned from ESM: %s (formID=0x%08X, ID=%u, HP=%.0f, ATK=%u, LVL=%u)",
             npc->name.c_str(), formID, npcId, npc->status.maxHealth,
             creature->attackDamage, creature->level);
        return npc;
    }

    // Fallback: generic NPC
    LOGW("createNPCFromESM: formID 0x%08X not found in CREA records", formID);
    return createNPC("Unknown", position);
}

std::shared_ptr<NPC> NpcManager::spawnFromLeveledList(uint32_t leveledListFormID,
                                                       uint32_t playerLevel,
                                                       const glm::vec3& position) {
    if (!m_esm) {
        LOGW("spawnFromLeveledList: ESMManager not set");
        return nullptr;
    }

    const oblivion::LeveledListData* lvlc = m_esm->findLeveledList(leveledListFormID);
    if (!lvlc) {
        LOGW("spawnFromLeveledList: LeveledList 0x%08X not found", leveledListFormID);
        return nullptr;
    }

    // ChanceNone: probability (0-100) that nothing spawns
    if (lvlc->chanceNone > 0) {
        int roll = std::rand() % 100;
        if (roll < lvlc->chanceNone) {
            LOGD("spawnFromLeveledList: ChanceNone=%u, roll=%d — nothing spawned", lvlc->chanceNone, roll);
            return nullptr;
        }
    }

    // Collect entries whose level <= playerLevel
    std::vector<const oblivion::LeveledListEntry*> eligible;
    for (const auto& entry : lvlc->entries) {
        if (entry.level <= playerLevel) {
            eligible.push_back(&entry);
        }
    }

    if (eligible.empty()) {
        LOGW("spawnFromLeveledList: No eligible entries for playerLevel=%u in list 0x%08X",
             playerLevel, leveledListFormID);
        return nullptr;
    }

    // Pick a random eligible entry
    const oblivion::LeveledListEntry* chosen = eligible[std::rand() % eligible.size()];

    // If the referenced formID is itself a leveled list, recurse
    const oblivion::LeveledListData* nestedList = m_esm->findLeveledList(chosen->referencedFormID);
    if (nestedList) {
        return spawnFromLeveledList(chosen->referencedFormID, playerLevel, position);
    }

    // Otherwise spawn as creature
    auto npc = createNPCFromESM(chosen->referencedFormID, position);
    if (npc) {
        LOGI("spawnFromLeveledList: Spawned %s from list 0x%08X (level %u, entry level %u)",
             npc->name.c_str(), leveledListFormID, playerLevel, chosen->level);
    }
    return npc;
}

void NpcManager::initializePlayerFromESM(NPC& player, uint32_t raceFormID,
                                          uint32_t classFormID, uint32_t birthsignFormID) {
    if (!m_esm) {
        LOGW("initializePlayerFromESM: ESMManager not set");
        return;
    }

    // --- Apply RACE data ---
    const oblivion::RaceData* race = m_esm->findRace(raceFormID);
    if (race) {
        player.race = race->fullName.empty() ? race->editorID : race->fullName;

        // Apply racial attribute bonuses
        auto setAttr = [&](const std::string& name, uint8_t bonus) {
            if (bonus > 0) {
                auto it = player.status.attributes.find(name);
                if (it != player.status.attributes.end()) {
                    it->second += static_cast<float>(bonus);
                } else {
                    player.status.attributes[name] = static_cast<float>(bonus);
                }
            }
        };
        setAttr("Strength", race->attrStrength);
        setAttr("Intelligence", race->attrIntelligence);
        setAttr("Willpower", race->attrWillpower);
        setAttr("Agility", race->attrAgility);
        setAttr("Speed", race->attrSpeed);
        setAttr("Endurance", race->attrEndurance);
        setAttr("Personality", race->attrPersonality);

        // Apply racial starting health
        if (race->startingHealth > 0) {
            player.status.maxHealth = static_cast<float>(race->startingHealth);
            player.status.currentHealth = player.status.maxHealth;
        }

        // Apply racial spells (e.g., Resist Disease, Water Breathing)
        for (uint32_t spellFormID : race->spellFormIDs) {
            player.status.knownSpells.push_back(spellFormID);
        }

        // Apply skill bonuses
        for (const auto& bonus : race->skillBonuses) {
            // bonus.first = skillFormID (we store as attribute name string for now)
            // In a full implementation, we'd resolve the MGEF/SKIL formID to a skill name
            LOGD("Racial skill bonus: skillFormID=0x%08X, bonus=%d", bonus.first, bonus.second);
        }

        // Set mesh path
        player.meshAssetPath = race->maleModelPath;  // TODO: gender selection

        LOGI("Race applied: %s (HP=%.0f, spells=%zu)",
             player.race.c_str(), player.status.maxHealth, race->spellFormIDs.size());
    } else {
        LOGW("initializePlayerFromESM: Race 0x%08X not found", raceFormID);
    }

    // --- Apply CLASS data ---
    const oblivion::ClassData* cls = m_esm->findClass(classFormID);
    if (cls) {
        player.class_ = cls->fullName.empty() ? cls->editorID : cls->fullName;

        // Apply primary attribute bonuses (+10 each)
        auto boostAttr = [&](uint32_t attrFormID) {
            // Oblivion attribute FormIDs are well-known constants
            // For now, apply a generic boost based on the primary attributes
            LOGD("Class primary attribute: formID=0x%08X", attrFormID);
        };
        boostAttr(cls->primaryAttribute1);
        boostAttr(cls->primaryAttribute2);

        LOGI("Class applied: %s (specialization=%u)", player.class_.c_str(), cls->specialization);
    } else {
        LOGW("initializePlayerFromESM: Class 0x%08X not found", classFormID);
    }

    // --- Apply BIRTHSIGN data ---
    const oblivion::BirthsignData* bsgn = m_esm->findBirthsign(birthsignFormID);
    if (bsgn) {
        // Grant birthsign power spells
        for (uint32_t spellFormID : bsgn->spellFormIDs) {
            player.status.knownSpells.push_back(spellFormID);
            LOGD("Birthsign power granted: spellFormID=0x%08X", spellFormID);
        }
        LOGI("Birthsign applied: %s (powers=%zu)",
             bsgn->fullName.c_str(), bsgn->spellFormIDs.size());
    } else {
        LOGW("initializePlayerFromESM: Birthsign 0x%08X not found", birthsignFormID);
    }

    LOGI("Player initialized from ESM: race=%s, class=%s, HP=%.0f, MP=%.0f, spells=%zu",
         player.race.c_str(), player.class_.c_str(),
         player.status.maxHealth, player.status.maxMana,
         player.status.knownSpells.size());
}

// ============================================================
// Status Effect System
// ============================================================

void NpcManager::addStatusEffect(NPC& npc, SpellEffectType type, float duration, float magnitude) {
    ActiveStatusEffect effect;
    effect.type = type;
    effect.remaining = duration;
    effect.magnitude = magnitude;
    m_statusEffects[npc.npcId].push_back(effect);

    // Apply immediate effects
    switch (type) {
        case SpellEffectType::PARALYZE:
            npc.setAIState(AIState::IDLE);
            LOGI("NPC %s paralyzed for %.1fs", npc.name.c_str(), duration);
            break;
        case SpellEffectType::INVISIBILITY:
            LOGI("NPC %s invisible for %.1fs", npc.name.c_str(), duration);
            break;
        case SpellEffectType::FORTIFY_ATTR:
            LOGI("NPC %s attribute fortified by %.0f for %.1fs", npc.name.c_str(), magnitude, duration);
            break;
        case SpellEffectType::SUMMON:
            LOGI("NPC %s summoned for %.1fs", npc.name.c_str(), duration);
            break;
        default:
            break;
    }
}

void NpcManager::updateStatusEffects(NPC& npc, float deltaTime) {
    auto it = m_statusEffects.find(npc.npcId);
    if (it == m_statusEffects.end()) return;

    auto& effects = it->second;
    for (auto effIt = effects.begin(); effIt != effects.end(); ) {
        effIt->remaining -= deltaTime;
        if (effIt->remaining <= 0.0f) {
            // Effect expired — remove side effects
            switch (effIt->type) {
                case SpellEffectType::PARALYZE:
                    npc.setAIState(AIState::IDLE);
                    LOGI("NPC %s paralysis wore off", npc.name.c_str());
                    break;
                case SpellEffectType::INVISIBILITY:
                    LOGI("NPC %s invisibility wore off", npc.name.c_str());
                    break;
                default:
                    break;
            }
            effIt = effects.erase(effIt);
        } else {
            ++effIt;
        }
    }

    if (effects.empty()) {
        m_statusEffects.erase(it);
    }
}

bool NpcManager::hasStatusEffect(const NPC& npc, SpellEffectType type) const {
    auto it = m_statusEffects.find(npc.npcId);
    if (it == m_statusEffects.end()) return false;

    for (const auto& effect : it->second) {
        if (effect.type == type && effect.remaining > 0.0f) {
            return true;
        }
    }
    return false;
}
