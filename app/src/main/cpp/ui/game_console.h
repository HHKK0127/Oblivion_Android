#pragma once

#include <vector>
#include <string>
#include <deque>
#include <functional>
#include <glm/glm.hpp>

class TextRenderer;

/**
 * @brief Game Console - Debug command input and execution system
 *
 * Supports commands like:
 * - teleport <x> <y> <z>
 * - spawn <npcId>
 * - god (toggle invincibility)
 * - kill (kill nearest NPC)
 * - heal (heal player)
 * - sethealth <value>
 * - setmana <value>
 * - noclip (toggle collision)
 * - fps (toggle FPS display)
 * - help (show commands)
 * - clear (clear console)
 */
class GameConsole {
public:
    GameConsole();
    ~GameConsole();

    bool initialize(TextRenderer* textRenderer);
    void cleanup();

    void toggle();
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

    void onTouchEvent(float x, float y, int action);
    void onKeyPress(int key);
    void onTextInput(const char* text);

    void update(float deltaTime);
    void render();

    // Execute a command string
    void executeCommand(const std::string& command);

    // Add output line
    void print(const std::string& message);

    // Get history for UI display
    const std::vector<std::string>& getOutputBuffer() const { return outputBuffer; }
    const std::string& getCurrentInput() const { return currentInput; }

    // Callback type for command execution
    using CommandHandler = std::function<void(const std::vector<std::string>& args)>;

    // Register custom command handler
    void registerCommand(const std::string& name, const std::string& description, CommandHandler handler);

    // Built-in handlers - called from external systems
    void setPlayerPosition(const glm::vec3& pos) { lastPlayerPos = pos; }
    void setPlayerHealth(float hp, float maxHp) { playerHealth = hp; playerMaxHealth = maxHp; }
    void setGodMode(bool enabled) { godMode = enabled; }
    bool isGodMode() const { return godMode; }
    bool isNoclip() const { return noclip; }

    // Game system references (set by Renderer)
    struct GameSystemRefs {
        // Player
        std::function<void(float, float, float)> setPlayerPos;
        std::function<glm::vec3()> getPlayerPos;
        std::function<void(float)> setHealth;
        std::function<void(float)> setMaxHealth;
        std::function<void(float)> setMana;
        std::function<void(float)> setStamina;
        std::function<void(int)> setLevel;
        std::function<void(float)> addExperience;
        std::function<void(const std::string&, int)> setSkill;
        std::function<void(const std::string&, int)> setAttribute;
        std::function<void()> maxAllSkills;
        std::function<void()> resetPlayerStats;
        std::function<std::string()> getPlayerStats;

        // Combat
        std::function<void()> attackNearest;
        std::function<void()> blockAction;
        std::function<void()> dodgeAction;
        std::function<void(uint32_t, float)> applyDamageToNpc;
        std::function<void(uint32_t)> killNpc;
        std::function<void(uint32_t)> resurrectNpc;
        std::function<void()> killAllNpcs;
        std::function<void()> toggleCombatDebug;

        // Inventory
        std::function<void(uint32_t, uint32_t)> addItem;
        std::function<void(uint32_t, uint32_t)> removeItem;
        std::function<void(uint32_t)> equipItem;
        std::function<void(uint32_t)> unequipItem;
        std::function<std::string()> listInventory;
        std::function<void()> clearInventory;
        std::function<void(float)> setCarryWeight;

        // Magic
        std::function<void(uint32_t)> learnSpell;
        std::function<void(uint32_t, uint32_t)> castSpellOnTarget;
        std::function<void(uint32_t)> equipSpell;
        std::function<std::string()> listSpells;
        std::function<void(const std::string&, float, float)> createSpell;

        // Quest
        std::function<void(uint32_t)> acceptQuest;
        std::function<void(uint32_t)> completeQuest;
        std::function<void(uint32_t)> failQuest;
        std::function<std::string()> listQuests;
        std::function<void(uint32_t, uint32_t, uint32_t)> updateObjective;

        // NPC
        std::function<uint32_t(const std::string&, float, float, float)> spawnNpcAt;
        std::function<void(uint32_t, const std::string&)> setNpcAiState;
        std::function<void(uint32_t, float)> setNpcAggression;
        std::function<void(uint32_t)> calmNpc;
        std::function<std::string()> listNpcs;
        std::function<std::string()> listNearbyNpcs;

        // Dialogue
        std::function<void(uint32_t)> startDialogueWith;
        std::function<void(int)> selectDialogueTopic;
        std::function<void(int)> selectDialogueChoice;
        std::function<void()> endDialogue;

        // World
        std::function<void(const std::string&)> setWeather;
        std::function<void(float)> setTimeScale;
        std::function<void(float)> setTimeOfDay;
        std::function<void(int32_t, int32_t)> loadCell;
        std::function<std::string()> getWorldInfo;

        // Save/Load
        std::function<void(uint32_t)> saveGameSlot;
        std::function<void(uint32_t)> loadGameSlot;
        std::function<void()> quickSave;
        std::function<void()> quickLoad;
        std::function<std::string()> listSaveSlots;

        // UI
        std::function<void(const std::string&)> openMenu;
        std::function<void()> closeMenu;
        std::function<void()> toggleDebugMenu;

        // Phase 65: Extended Debug
        std::function<void()> toggleWireframe;
        std::function<void()> toggleAabb;
        std::function<void()> toggleNpcOverlay;
        std::function<void()> toggleTouchTrail;
        std::function<void()> debugHudNextPage;
        std::function<void()> debugHudPrevPage;
        std::function<void()> toggleDebugLog;

        // Sound
        std::function<void()> playBgm;
        std::function<void()> stopBgm;
        std::function<void()> playSe;
        std::function<void()> stopAllSe;
        std::function<void(float)> setMasterVolume;
        std::function<void()> muteAll;
        std::function<void()> unmuteAll;
        std::function<std::string()> listAudio;
        std::function<std::string()> getAudioStats;
        // Phase 66: BGM browsing
        std::function<std::string()> listBgmTracks;
        std::function<void(const std::string&)> playBgmTrack;
        std::function<void(float)> setBgmVolume;
        std::function<std::string()> getCurrentBgmInfo;

        // Assets
        std::function<std::string()> listTextures;
        std::function<std::string()> listModels;
        std::function<std::string()> getTextureInfo;
        std::function<std::string()> getModelInfo;
        std::function<std::string()> getCacheStats;
        std::function<void()> clearCache;
        std::function<void()> reloadAssets;
        std::function<std::string()> getMemoryUsage;
        std::function<std::string()> getAssetStats;
        // Phase 66: Texture browsing
        std::function<std::string()> listTexturesDetailed;
        std::function<std::string(const std::string&)> getTextureDetail;

        // Phase 66: Map debug
        std::function<void(float, float, float)> teleportTo;
        std::function<std::string()> getPlayerPosition;
        std::function<void(float, float)> movePlayerRelative;
        std::function<std::string()> listNearbyCells;
        std::function<void(int32_t, int32_t)> teleportToCell;

        // Phase 67: Performance monitoring
        std::function<std::string()> getPerformanceStats;
        std::function<std::string()> getDetailedPerformance;
        std::function<void()> resetPerformanceStats;
        std::function<std::string()> getMemoryStats;
        std::function<std::string()> getDrawCallStats;

        // Phase 68: NPC debug
        std::function<std::string()> listAllNpcs;
        std::function<std::string(uint32_t)> getNpcInfo;
        std::function<void(const std::string&, float, float, float)> spawnNpc;
        std::function<void(bool)> toggleNpcAi;
        std::function<std::string()> getNpcCount;
        std::function<void(float)> setNpcSpeed;

        // Phase 69: Combat debug
        std::function<std::string()> getCombatStats;
        std::function<std::string()> getActiveCombats;
        std::function<void()> attackNearestEnemy;
        std::function<void(bool)> toggleCombatOverlay;
        std::function<void(float)> setDamageMultiplier;
        std::function<void(bool)> toggleInvincibility;
        std::function<void(float, float)> setPlayerDamage;

        // Phase 70: Magic debug
        std::function<std::string()> listAllSpells;
        std::function<std::string(uint32_t)> getSpellInfo;
        std::function<void(uint32_t)> castSpellAtNearest;
        std::function<void(float)> setSpellDamageMultiplier;
        std::function<void(bool)> toggleInfiniteMana;
        std::function<std::string()> getPlayerSpells;
        std::function<void(uint32_t)> teachSpellToPlayer;

        // Phase 71: Inventory debug
        std::function<std::string()> listPlayerInventory;
        std::function<std::string(uint32_t)> getItemInfo;
        std::function<void(uint32_t, uint32_t)> addItemToPlayer;
        std::function<void(uint32_t, uint32_t)> removeItemFromPlayer;
        std::function<std::string()> getInventoryWeight;
        std::function<void(float)> setCarryCapacity;

        // Phase 72: Quest debug enhanced
        std::function<std::string()> getActiveQuestList;
        std::function<std::string(uint32_t)> getQuestDetails;
        std::function<void(uint32_t)> resetQuest;
        std::function<std::string(uint32_t)> getQuestRewardInfo;
        std::function<void(uint32_t)> completeAllObjectives;

        // Phase 73: Dialogue debug enhanced
        std::function<std::string()> getDialogueState;
        std::function<std::string()> getDialogueTopics;
        std::function<std::string()> getDialogueChoices;
        std::function<std::string()> getDialogueHistory;
        std::function<void()> resetDialogue;

        // Phase 74: World debug enhanced
        std::function<std::string()> getWorldInfoDetailed;
        std::function<std::string(int32_t, int32_t)> getCellDetails;
        std::function<std::string()> getActiveCellsList;
        std::function<std::string()> getWorldItemsList;
        std::function<std::string()> getDoorInfo;

        // Logs
        std::function<void(const std::string&)> setLogLevel;
        std::function<void()> clearLogs;
        std::function<void()> exportLogs;
        std::function<std::string()> getLogStats;
        std::function<void(const std::string&)> searchLogs;
        std::function<void()> toggleLogAutoScroll;
    };

    void setGameSystemRefs(const GameSystemRefs& refs) { gameRefs = refs; }

private:
    TextRenderer* textRenderer;
    bool visible;
    bool initialized;

    std::string currentInput;
    std::vector<std::string> outputBuffer;
    std::vector<std::string> commandHistory;
    int historyIndex;
    int historyMaxSize;

    float scrollOffset;
    float cursorBlinkTimer;
    bool cursorVisible;

    // Game system references
    GameSystemRefs gameRefs;

    // Game state for queries
    glm::vec3 lastPlayerPos;
    float playerHealth;
    float playerMaxHealth;
    bool godMode;
    bool noclip;

    int screenWidth;
    int screenHeight;

    // Console UI areas (for touch handling)
    struct Rect {
        float x, y, w, h;
        bool contains(float px, float py) const {
            return px >= x && px <= x + w && py >= y && py <= y + h;
        }
    };
    Rect consoleArea;
    Rect inputArea;

    // Command registry
    struct CommandInfo {
        std::string name;
        std::string description;
        CommandHandler handler;
    };
    std::vector<CommandInfo> commands;

    void registerBuiltinCommands();
    void handleConsoleTouch(float x, float y, int action);
    void handleConsoleKey(int key);
    void processInput();
    std::vector<std::string> tokenize(const std::string& str);
    void addToHistory(const std::string& cmd);
    void appendOutput(const std::string& line);

    // Built-in command implementations
    void cmdHelp(const std::vector<std::string>& args);
    void cmdClear(const std::vector<std::string>& args);
    void cmdTeleport(const std::vector<std::string>& args);
    void cmdSetPos(const std::vector<std::string>& args);
    void cmdSpawn(const std::vector<std::string>& args);
    void cmdGod(const std::vector<std::string>& args);
    void cmdKill(const std::vector<std::string>& args);
    void cmdHeal(const std::vector<std::string>& args);
    void cmdSetHealth(const std::vector<std::string>& args);
    void cmdSetMana(const std::vector<std::string>& args);
    void cmdNoclip(const std::vector<std::string>& args);
    void cmdFPS(const std::vector<std::string>& args);
    void cmdTime(const std::vector<std::string>& args);
    void cmdPos(const std::vector<std::string>& args);
    void cmdStats(const std::vector<std::string>& args);

    // Player commands
    void cmdSetSkill(const std::vector<std::string>& args);
    void cmdSetAttribute(const std::vector<std::string>& args);
    void cmdAddXp(const std::vector<std::string>& args);
    void cmdLevelUp(const std::vector<std::string>& args);
    void cmdSetLevel(const std::vector<std::string>& args);
    void cmdMaxSkills(const std::vector<std::string>& args);
    void cmdResetStats(const std::vector<std::string>& args);
    void cmdSetStamina(const std::vector<std::string>& args);

    // Combat commands
    void cmdAttack(const std::vector<std::string>& args);
    void cmdBlock(const std::vector<std::string>& args);
    void cmdDodge(const std::vector<std::string>& args);
    void cmdDamage(const std::vector<std::string>& args);
    void cmdKillAll(const std::vector<std::string>& args);
    void cmdResurrect(const std::vector<std::string>& args);
    void cmdCombatDebug(const std::vector<std::string>& args);

    // Inventory commands
    void cmdAddItem(const std::vector<std::string>& args);
    void cmdRemoveItem(const std::vector<std::string>& args);
    void cmdEquip(const std::vector<std::string>& args);
    void cmdUnequip(const std::vector<std::string>& args);
    void cmdListItems(const std::vector<std::string>& args);
    void cmdClearInv(const std::vector<std::string>& args);
    void cmdSetWeight(const std::vector<std::string>& args);

    // Magic commands
    void cmdLearnSpell(const std::vector<std::string>& args);
    void cmdCastSpell(const std::vector<std::string>& args);
    void cmdEquipSpell(const std::vector<std::string>& args);
    void cmdListSpells(const std::vector<std::string>& args);
    void cmdCreateSpell(const std::vector<std::string>& args);

    // Quest commands
    void cmdAcceptQuest(const std::vector<std::string>& args);
    void cmdCompleteQuest(const std::vector<std::string>& args);
    void cmdFailQuest(const std::vector<std::string>& args);
    void cmdListQuests(const std::vector<std::string>& args);
    void cmdUpdateObjective(const std::vector<std::string>& args);

    // NPC commands
    void cmdSpawnAt(const std::vector<std::string>& args);
    void cmdSetAi(const std::vector<std::string>& args);
    void cmdAggro(const std::vector<std::string>& args);
    void cmdCalm(const std::vector<std::string>& args);
    void cmdListNpcs(const std::vector<std::string>& args);
    void cmdNearby(const std::vector<std::string>& args);
    void cmdResurrectNpc(const std::vector<std::string>& args);

    // Dialogue commands
    void cmdTalk(const std::vector<std::string>& args);
    void cmdSelectTopic(const std::vector<std::string>& args);
    void cmdSelectChoice(const std::vector<std::string>& args);
    void cmdEndTalk(const std::vector<std::string>& args);

    // World commands
    void cmdSetWeather(const std::vector<std::string>& args);
    void cmdSetTimeScale(const std::vector<std::string>& args);
    void cmdSetTime(const std::vector<std::string>& args);
    void cmdLoadCell(const std::vector<std::string>& args);
    void cmdWorldInfo(const std::vector<std::string>& args);

    // Save/Load commands
    void cmdSave(const std::vector<std::string>& args);
    void cmdLoad(const std::vector<std::string>& args);
    void cmdQuickSave(const std::vector<std::string>& args);
    void cmdQuickLoad(const std::vector<std::string>& args);
    void cmdListSaves(const std::vector<std::string>& args);

    // UI commands
    void cmdOpenMenu(const std::vector<std::string>& args);
    void cmdCloseMenu(const std::vector<std::string>& args);
    void cmdDebugMenu(const std::vector<std::string>& args);

    // Sound commands
    void cmdPlayBgm(const std::vector<std::string>& args);
    void cmdStopBgm(const std::vector<std::string>& args);
    void cmdPlaySe(const std::vector<std::string>& args);
    void cmdStopAllSe(const std::vector<std::string>& args);
    void cmdSetVolume(const std::vector<std::string>& args);
    void cmdMute(const std::vector<std::string>& args);
    void cmdUnmute(const std::vector<std::string>& args);
    void cmdListAudio(const std::vector<std::string>& args);
    void cmdAudioStats(const std::vector<std::string>& args);

    // Asset commands
    void cmdListTextures(const std::vector<std::string>& args);
    void cmdListModels(const std::vector<std::string>& args);
    void cmdTextureInfo(const std::vector<std::string>& args);
    void cmdModelInfo(const std::vector<std::string>& args);
    void cmdCacheStats(const std::vector<std::string>& args);
    void cmdClearCache(const std::vector<std::string>& args);
    void cmdReloadAssets(const std::vector<std::string>& args);
    void cmdMemoryUsage(const std::vector<std::string>& args);
    void cmdAssetStats(const std::vector<std::string>& args);

    // Log commands
    void cmdLogLevel(const std::vector<std::string>& args);
    void cmdClearLogs(const std::vector<std::string>& args);
    void cmdExportLogs(const std::vector<std::string>& args);
    void cmdLogStats(const std::vector<std::string>& args);
    void cmdSearchLog(const std::vector<std::string>& args);
    void cmdLogAutoScroll(const std::vector<std::string>& args);
};
