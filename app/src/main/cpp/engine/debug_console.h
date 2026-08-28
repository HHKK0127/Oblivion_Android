#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <sstream>
#include <android/log.h>

#define LOG_TAG "DebugConsole"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward declarations
class Renderer;
class WorldManager;
class NpcManager;
class QuestManager;
class CombatManager;
class SaveManager;
class PlayerController;
class InventoryManager;

// ============================================================================
// DebugConsole - In-game debug command system
// Phase 42: Full game loop integration
// ============================================================================

// Console output line
struct ConsoleLine {
    std::string text;
    enum class Level : uint8_t { INFO, WARNING, ERROR, COMMAND } level;
};

// Command handler: takes parsed arguments, returns output string
using CommandHandler = std::function<std::string(const std::vector<std::string>&)>;

class DebugConsole {
public:
    DebugConsole();
    ~DebugConsole();

    bool initialize();
    void shutdown();

    // Execute a command string
    std::string executeCommand(const std::string& input);

    // Register custom commands
    void registerCommand(const std::string& name, const std::string& help, CommandHandler handler);

    // System references for built-in commands
    void setRenderer(Renderer* renderer);
    void setWorldManager(WorldManager* world);
    void setNpcManager(NpcManager* npc);
    void setQuestManager(QuestManager* quest);
    void setCombatManager(CombatManager* combat);
    void setSaveManager(SaveManager* save);
    void setPlayerController(PlayerController* player);
    void setInventoryManager(InventoryManager* inventory);

    // Console output
    const std::vector<ConsoleLine>& getOutput() const { return output_; }
    void clearOutput();
    size_t getMaxOutputLines() const { return MAX_OUTPUT_LINES; }

    // Command history
    const std::vector<std::string>& getHistory() const { return history_; }

    // Console visibility
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    void toggle() { visible_ = !visible_; }

    // God mode / noclip state
    bool isGodMode() const { return godMode_; }
    bool isNoClip() const { return noClip_; }
    bool isFPSVisible() const { return showFPS_; }

private:
    // System pointers (non-owning)
    Renderer* renderer_ = nullptr;
    WorldManager* worldManager_ = nullptr;
    NpcManager* npcManager_ = nullptr;
    QuestManager* questManager_ = nullptr;
    CombatManager* combatManager_ = nullptr;
    SaveManager* saveManager_ = nullptr;
    PlayerController* playerController_ = nullptr;
    InventoryManager* inventoryManager_ = nullptr;

    // Console state
    bool visible_ = false;
    bool godMode_ = false;
    bool noClip_ = false;
    bool showFPS_ = false;

    // Output buffer
    std::vector<ConsoleLine> output_;
    static constexpr size_t MAX_OUTPUT_LINES = 256;

    // Command history
    std::vector<std::string> history_;
    static constexpr size_t MAX_HISTORY = 64;

    // Registered commands
    struct CommandEntry {
        std::string name;
        std::string help;
        CommandHandler handler;
    };
    std::unordered_map<std::string, CommandEntry> commands_;

    // Built-in command registration
    void registerBuiltinCommands();

    // Built-in command handlers
    std::string cmdTeleport(const std::vector<std::string>& args);
    std::string cmdAddItem(const std::vector<std::string>& args);
    std::string cmdSetStage(const std::vector<std::string>& args);
    std::string cmdGodMode(const std::vector<std::string>& args);
    std::string cmdNoClip(const std::vector<std::string>& args);
    std::string cmdFPS(const std::vector<std::string>& args);
    std::string cmdSave(const std::vector<std::string>& args);
    std::string cmdLoad(const std::vector<std::string>& args);
    std::string cmdHelp(const std::vector<std::string>& args);
    std::string cmdClear(const std::vector<std::string>& args);

    // Utility
    void addOutput(const std::string& text, ConsoleLine::Level level);
    std::vector<std::string> parseInput(const std::string& input);
    float parseFloat(const std::string& s, float defaultVal = 0.0f);
    uint32_t parseUint(const std::string& s, uint32_t defaultVal = 0);
};
