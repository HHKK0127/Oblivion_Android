#pragma once

#include <string>
#include <vector>
#include <functional>
#include <glm/glm.hpp>

class NpcManager;
class QuestManager;

namespace Game {

struct PlayerInfo {
    std::string name;
    int level = 1;
    float health = 100.0f;
    float maxHealth = 100.0f;
    float magicka = 100.0f;
    float maxMagicka = 100.0f;
    float stamina = 100.0f;
    float maxStamina = 100.0f;
    float position[3] = {0.0f, 0.0f, 0.0f};
    float rotation = 0.0f;
};

struct NpcInfo {
    uint32_t id = 0;
    std::string name;
    float health = 100.0f;
    float maxHealth = 100.0f;
    float position[3] = {0.0f, 0.0f, 0.0f};
    std::string state;
    bool isHostile = false;
};

struct QuestInfo {
    uint32_t id = 0;
    std::string name;
    std::string description;
    std::string status;
    int currentStage = 0;
    int totalStages = 0;
};

struct WorldInfo {
    std::string currentCell;
    float timeOfDay = 12.0f;
    float weatherCondition = 0.0f;
    int dayOfMonth = 1;
    std::string currentSeason;
};

class IGameBridge {
public:
    virtual ~IGameBridge() = default;

    virtual PlayerInfo getPlayerInfo() const = 0;
    virtual void setPlayerHealth(float health) = 0;
    virtual void setPlayerMagicka(float magicka) = 0;
    virtual void setPlayerStamina(float stamina) = 0;
    virtual void setPlayerPosition(float x, float y, float z) = 0;
    virtual void setPlayerLevel(int level) = 0;

    virtual std::vector<NpcInfo> getNearbyNpcs(float radius) const = 0;
    virtual NpcInfo getNpcInfo(uint32_t npcId) const = 0;
    virtual void setNpcHostile(uint32_t npcId, bool hostile) = 0;
    virtual void damageNpc(uint32_t npcId, float damage) = 0;

    virtual std::vector<QuestInfo> getActiveQuests() const = 0;
    virtual QuestInfo getQuestInfo(uint32_t questId) const = 0;
    virtual void advanceQuestStage(uint32_t questId) = 0;
    virtual void completeQuest(uint32_t questId) = 0;

    virtual WorldInfo getWorldInfo() const = 0;
    virtual void setTimeOfDay(float time) = 0;
    virtual void setWeather(float condition) = 0;
    virtual void changeCell(const std::string& cellName) = 0;

    virtual void executeCommand(const std::string& command) = 0;
    virtual std::string getLastError() const = 0;
};

class DebugGameBridge : public IGameBridge {
public:
    DebugGameBridge();
    ~DebugGameBridge() override;

    void initialize(::NpcManager* npcManager, ::QuestManager* questManager);
    void shutdown();

    PlayerInfo getPlayerInfo() const override;
    void setPlayerHealth(float health) override;
    void setPlayerMagicka(float magicka) override;
    void setPlayerStamina(float stamina) override;
    void setPlayerPosition(float x, float y, float z) override;
    void setPlayerLevel(int level) override;

    std::vector<NpcInfo> getNearbyNpcs(float radius) const override;
    NpcInfo getNpcInfo(uint32_t npcId) const override;
    void setNpcHostile(uint32_t npcId, bool hostile) override;
    void damageNpc(uint32_t npcId, float damage) override;

    std::vector<QuestInfo> getActiveQuests() const override;
    QuestInfo getQuestInfo(uint32_t questId) const override;
    void advanceQuestStage(uint32_t questId) override;
    void completeQuest(uint32_t questId) override;

    WorldInfo getWorldInfo() const override;
    void setTimeOfDay(float time) override;
    void setWeather(float condition) override;
    void changeCell(const std::string& cellName) override;

    void executeCommand(const std::string& command) override;
    std::string getLastError() const override;

private:
    ::NpcManager* m_npcManager;
    ::QuestManager* m_questManager;

    int m_playerLevel;
    float m_playerHealth;
    float m_playerMaxHealth;
    float m_playerMagicka;
    float m_playerMaxMagicka;
    float m_playerStamina;
    float m_playerMaxStamina;
    float m_playerPosition[3] = {0.0f, 0.0f, 0.0f};
    float m_playerRotation = 0.0f;

    std::string m_currentCell;
    float m_timeOfDay;
    float m_weatherCondition;
    int m_dayOfMonth;
    std::string m_currentSeason;

    std::string m_lastError;
};

} // namespace Game
