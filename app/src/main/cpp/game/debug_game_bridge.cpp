#include "debug_game_bridge.h"
#include "npc_manager.h"
#include "quest_manager.h"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace Game {

DebugGameBridge::DebugGameBridge()
    : m_npcManager(nullptr)
    , m_questManager(nullptr)
    , m_playerLevel(1)
    , m_playerHealth(100.0f)
    , m_playerMaxHealth(100.0f)
    , m_playerMagicka(100.0f)
    , m_playerMaxMagicka(100.0f)
    , m_playerStamina(100.0f)
    , m_playerMaxStamina(100.0f)
    , m_timeOfDay(12.0f)
    , m_weatherCondition(0.0f)
    , m_dayOfMonth(1) {
    m_playerPosition[0] = 0.0f;
    m_playerPosition[1] = 0.0f;
    m_playerPosition[2] = 0.0f;
}

DebugGameBridge::~DebugGameBridge() {
}

void DebugGameBridge::initialize(::NpcManager* npcManager, ::QuestManager* questManager) {
    m_npcManager = npcManager;
    m_questManager = questManager;
}

void DebugGameBridge::shutdown() {
    m_npcManager = nullptr;
    m_questManager = nullptr;
}

PlayerInfo DebugGameBridge::getPlayerInfo() const {
    PlayerInfo info;
    info.name = "Player";
    info.level = m_playerLevel;
    info.health = m_playerHealth;
    info.maxHealth = m_playerMaxHealth;
    info.magicka = m_playerMagicka;
    info.maxMagicka = m_playerMaxMagicka;
    info.stamina = m_playerStamina;
    info.maxStamina = m_playerMaxStamina;
    info.position[0] = m_playerPosition[0];
    info.position[1] = m_playerPosition[1];
    info.position[2] = m_playerPosition[2];
    info.rotation = m_playerRotation;
    return info;
}

void DebugGameBridge::setPlayerHealth(float health) {
    m_playerHealth = std::max(0.0f, std::min(health, m_playerMaxHealth));
}

void DebugGameBridge::setPlayerMagicka(float magicka) {
    m_playerMagicka = std::max(0.0f, std::min(magicka, m_playerMaxMagicka));
}

void DebugGameBridge::setPlayerStamina(float stamina) {
    m_playerStamina = std::max(0.0f, std::min(stamina, m_playerMaxStamina));
}

void DebugGameBridge::setPlayerPosition(float x, float y, float z) {
    m_playerPosition[0] = x;
    m_playerPosition[1] = y;
    m_playerPosition[2] = z;
}

void DebugGameBridge::setPlayerLevel(int level) {
    m_playerLevel = std::max(1, level);
}

std::vector<NpcInfo> DebugGameBridge::getNearbyNpcs(float radius) const {
    std::vector<NpcInfo> result;
    if (!m_npcManager) return result;

    // Get all NPCs from manager
    auto allNpcs = m_npcManager->getAllNPCs();
    for (const auto& npc : allNpcs) {
        if (!npc) continue;

        float dx = npc->position.x - m_playerPosition[0];
        float dy = npc->position.y - m_playerPosition[1];
        float dz = npc->position.z - m_playerPosition[2];
        float dist = std::sqrt(dx*dx + dy*dy + dz*dz);

        if (dist <= radius) {
            NpcInfo info;
            info.id = npc->npcId;
            info.name = npc->name;
            info.health = npc->status.currentHealth;
            info.maxHealth = npc->status.maxHealth;
            info.position[0] = npc->position.x;
            info.position[1] = npc->position.y;
            info.position[2] = npc->position.z;
            info.state = npc->inCombat ? "Combat" : "Idle";
            info.isHostile = npc->inCombat;
            result.push_back(info);
        }
    }
    return result;
}

NpcInfo DebugGameBridge::getNpcInfo(uint32_t npcId) const {
    NpcInfo info;
    if (!m_npcManager) return info;

    auto* npc = m_npcManager->getNPC(npcId).get();
    if (npc) {
        info.id = npc->npcId;
        info.name = npc->name;
        info.health = npc->status.currentHealth;
        info.maxHealth = npc->status.maxHealth;
        info.position[0] = npc->position.x;
        info.position[1] = npc->position.y;
        info.position[2] = npc->position.z;
        info.state = npc->inCombat ? "Combat" : "Idle";
        info.isHostile = npc->inCombat;
    }
    return info;
}

void DebugGameBridge::setNpcHostile(uint32_t npcId, bool hostile) {
    // Placeholder - would need to implement hostility system
}

void DebugGameBridge::damageNpc(uint32_t npcId, float damage) {
    if (!m_npcManager) return;
    auto* npc = m_npcManager->getNPC(npcId).get();
    if (npc) {
        npc->takeDamage(damage);
    }
}

std::vector<QuestInfo> DebugGameBridge::getActiveQuests() const {
    std::vector<QuestInfo> result;
    if (!m_questManager) return result;

    // Placeholder - would need to implement quest retrieval
    return result;
}

QuestInfo DebugGameBridge::getQuestInfo(uint32_t questId) const {
    QuestInfo info;
    // Placeholder - would need to implement quest info retrieval
    return info;
}

void DebugGameBridge::advanceQuestStage(uint32_t questId) {
    // Placeholder - would need to implement quest advancement
}

void DebugGameBridge::completeQuest(uint32_t questId) {
    // Placeholder - would need to implement quest completion
}

WorldInfo DebugGameBridge::getWorldInfo() const {
    WorldInfo info;
    info.currentCell = m_currentCell;
    info.timeOfDay = m_timeOfDay;
    info.weatherCondition = m_weatherCondition;
    info.dayOfMonth = m_dayOfMonth;
    info.currentSeason = m_currentSeason;
    return info;
}

void DebugGameBridge::setTimeOfDay(float time) {
    m_timeOfDay = std::fmod(time, 24.0f);
    if (m_timeOfDay < 0.0f) m_timeOfDay += 24.0f;
}

void DebugGameBridge::setWeather(float condition) {
    m_weatherCondition = std::max(0.0f, std::min(condition, 1.0f));
}

void DebugGameBridge::changeCell(const std::string& cellName) {
    m_currentCell = cellName;
}

void DebugGameBridge::executeCommand(const std::string& command) {
    m_lastError = "";
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;

    if (cmd == "sethealth") {
        float value;
        if (iss >> value) {
            setPlayerHealth(value);
        } else {
            m_lastError = "Usage: sethealth <value>";
        }
    } else if (cmd == "setmagicka") {
        float value;
        if (iss >> value) {
            setPlayerMagicka(value);
        } else {
            m_lastError = "Usage: setmagicka <value>";
        }
    } else if (cmd == "setstamina") {
        float value;
        if (iss >> value) {
            setPlayerStamina(value);
        } else {
            m_lastError = "Usage: setstamina <value>";
        }
    } else if (cmd == "setlevel") {
        int value;
        if (iss >> value) {
            setPlayerLevel(value);
        } else {
            m_lastError = "Usage: setlevel <value>";
        }
    } else if (cmd == "settime") {
        float value;
        if (iss >> value) {
            setTimeOfDay(value);
        } else {
            m_lastError = "Usage: settime <0-24>";
        }
    } else if (cmd == "setweather") {
        float value;
        if (iss >> value) {
            setWeather(value);
        } else {
            m_lastError = "Usage: setweather <0-1>";
        }
    } else if (cmd == "help") {
        m_lastError = "Commands: sethealth, setmagicka, setstamina, setlevel, settime, setweather";
    } else {
        m_lastError = "Unknown command: " + cmd;
    }
}

std::string DebugGameBridge::getLastError() const {
    return m_lastError;
}

} // namespace Game
