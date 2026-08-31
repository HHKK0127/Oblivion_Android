#include "debug_console.h"
#include "renderer.h"
#include "../world/world_manager.h"
#include "../game/npc_manager.h"
#include "../game/quest_manager.h"
#include "../game/combat_manager.h"
#include "../save_system/save_manager.h"
#include "../game/player_controller.h"
#include "../game/inventory_manager.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <cctype>
#include <deque>

// ============================================================================
// DebugConsole implementation
// ============================================================================

DebugConsole::DebugConsole() = default;

DebugConsole::~DebugConsole() {
    shutdown();
}

bool DebugConsole::initialize() {
    registerBuiltinCommands();
    addOutput("Debug console initialized. Type 'help' for commands.", ConsoleLine::Level::INFO);
    LOGI("DebugConsole initialized with %zu commands", commands_.size());
    return true;
}

void DebugConsole::shutdown() {
    commands_.clear();
    output_.clear();
    history_.clear();
    LOGI("DebugConsole shutdown");
}

void DebugConsole::setRenderer(Renderer* r) { renderer_ = r; }
void DebugConsole::setWorldManager(WorldManager* w) { worldManager_ = w; }
void DebugConsole::setNpcManager(NpcManager* n) { npcManager_ = n; }
void DebugConsole::setQuestManager(QuestManager* q) { questManager_ = q; }
void DebugConsole::setCombatManager(CombatManager* c) { combatManager_ = c; }
void DebugConsole::setSaveManager(SaveManager* s) { saveManager_ = s; }
void DebugConsole::setPlayerController(PlayerController* p) { playerController_ = p; }
void DebugConsole::setInventoryManager(InventoryManager* i) { inventoryManager_ = i; }

std::string DebugConsole::executeCommand(const std::string& input) {
    if (input.empty()) return "";

    // Add to history
    history_.push_back(input);
    if (history_.size() > MAX_HISTORY) {
        history_.pop_front();  // O(1) with deque
    }

    // Echo command
    addOutput("> " + input, ConsoleLine::Level::COMMAND);

    // Parse input
    auto args = parseInput(input);
    if (args.empty()) return "";

    std::string cmdName = args[0];
    // Convert to lowercase for case-insensitive matching
    std::transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::tolower);

    // Look up command
    auto it = commands_.find(cmdName);
    if (it == commands_.end()) {
        std::string error = "Unknown command: " + cmdName + ". Type 'help' for available commands.";
        addOutput(error, ConsoleLine::Level::ERROR);
        return error;
    }

    // Execute command
    std::vector<std::string> cmdArgs(args.begin() + 1, args.end());
    std::string result;
    try {
        result = it->second.handler(cmdArgs);
    } catch (const std::exception& e) {
        result = std::string("Error: ") + e.what();
        addOutput(result, ConsoleLine::Level::ERROR);
        return result;
    } catch (...) {
        result = "Error: Unknown exception";
        addOutput(result, ConsoleLine::Level::ERROR);
        return result;
    }

    if (!result.empty()) {
        addOutput(result, ConsoleLine::Level::INFO);
    }

    return result;
}

void DebugConsole::registerCommand(const std::string& name, const std::string& help,
                                    CommandHandler handler) {
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    commands_[lowerName] = {name, help, std::move(handler)};
}

void DebugConsole::clearOutput() {
    output_.clear();
}

// ============================================================================
// Built-in command registration
// ============================================================================

void DebugConsole::registerBuiltinCommands() {
    registerCommand("tp", "tp <x> <y> <z> - Teleport player to coordinates",
        [this](const std::vector<std::string>& args) { return cmdTeleport(args); });

    registerCommand("additem", "additem <id> [count] - Add item to inventory",
        [this](const std::vector<std::string>& args) { return cmdAddItem(args); });

    registerCommand("setstage", "setstage <quest> <stage> - Set quest stage",
        [this](const std::vector<std::string>& args) { return cmdSetStage(args); });

    registerCommand("tgm", "tgm - Toggle god mode",
        [this](const std::vector<std::string>& args) { return cmdGodMode(args); });

    registerCommand("tcl", "tcl - Toggle noclip mode",
        [this](const std::vector<std::string>& args) { return cmdNoClip(args); });

    registerCommand("fps", "fps - Toggle FPS display",
        [this](const std::vector<std::string>& args) { return cmdFPS(args); });

    registerCommand("save", "save [slot] - Save game",
        [this](const std::vector<std::string>& args) { return cmdSave(args); });

    registerCommand("load", "load [slot] - Load game",
        [this](const std::vector<std::string>& args) { return cmdLoad(args); });

    registerCommand("help", "help - Show available commands",
        [this](const std::vector<std::string>& args) { return cmdHelp(args); });

    registerCommand("clear", "clear - Clear console output",
        [this](const std::vector<std::string>& args) { return cmdClear(args); });
}

// ============================================================================
// Built-in command handlers
// ============================================================================

std::string DebugConsole::cmdAddItem(const std::vector<std::string>& args) {
    if (args.empty()) {
        return "Usage: additem <id> [count]";
    }

    uint32_t itemId = parseUint(args[0]);
    uint32_t count = args.size() > 1 ? parseUint(args[1], 1) : 1;

    if (inventoryManager_) {
        // Get item template and add to player inventory
        auto itemTemplate = inventoryManager_->getItemTemplate(itemId);
        if (!itemTemplate) {
            return "Error: Unknown item ID " + std::to_string(itemId);
        }
        if (inventoryManager_->playerAddItem(*itemTemplate, count)) {
            return "Added item " + std::to_string(itemId) + " x" + std::to_string(count);
        }
        return "Failed to add item " + std::to_string(itemId) + " (inventory full?)";
    }

    return "Error: InventoryManager not available";
}

std::string DebugConsole::cmdTeleport(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        return "Usage: tp <x> <y> <z>";
    }

    float x = parseFloat(args[0]);
    float y = parseFloat(args[1]);
    float z = parseFloat(args[2]);

    if (playerController_) {
        playerController_->setPosition(glm::vec3(x, y, z));
        return "Teleported to " + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z);
    }

    return "Error: PlayerController not available";
}

std::string DebugConsole::cmdSetStage(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return "Usage: setstage <questId> <stage>\n"
               "  Stage 0 = PENDING (available)\n"
               "  Stage 1 = ACCEPTED\n"
               "  Stage 2 = IN_PROGRESS\n"
               "  Stage 3 = COMPLETED\n"
               "  Stage 4 = FAILED";
    }

    uint32_t questId = parseUint(args[0]);
    uint32_t stage = parseUint(args[1]);

    if (questManager_) {
        auto quest = questManager_->getQuest(questId);
        if (!quest) {
            return "Error: Unknown quest ID " + std::to_string(questId);
        }

        bool success = false;
        switch (stage) {
            case 0: // PENDING - not directly settable, would need to fail/abandon first
                return "Cannot set stage to PENDING directly. Use failquest or abandon.";
            case 1: // ACCEPTED
                success = questManager_->acceptQuest(questId);
                break;
            case 2: // IN_PROGRESS - auto when accepted with objectives
                success = questManager_->acceptQuest(questId);
                break;
            case 3: // COMPLETED
                success = questManager_->completeQuest(questId);
                break;
            case 4: // FAILED
                success = questManager_->failQuest(questId);
                break;
            default:
                return "Error: Invalid stage " + std::to_string(stage) + " (0-4)";
        }

        if (success) {
            return "Set quest " + std::to_string(questId) + " to stage " + std::to_string(stage);
        }
        return "Failed to set quest stage (check quest state)";
    }

    return "Error: QuestManager not available";
}

std::string DebugConsole::cmdGodMode(const std::vector<std::string>& args) {
    (void)args;
    godMode_ = !godMode_;
    return godMode_ ? "God mode ON" : "God mode OFF";
}

std::string DebugConsole::cmdNoClip(const std::vector<std::string>& args) {
    (void)args;
    noClip_ = !noClip_;
    return noClip_ ? "Noclip ON" : "Noclip OFF";
}

std::string DebugConsole::cmdFPS(const std::vector<std::string>& args) {
    (void)args;
    showFPS_ = !showFPS_;
    return showFPS_ ? "FPS display ON" : "FPS display OFF";
}

std::string DebugConsole::cmdSave(const std::vector<std::string>& args) {
    uint32_t slot = args.empty() ? 0 : parseUint(args[0]);

    if (saveManager_) {
        bool success = saveManager_->saveGame(slot);
        return success ? "Saved to slot " + std::to_string(slot)
                       : "Failed to save to slot " + std::to_string(slot);
    }

    return "Error: SaveManager not available";
}

std::string DebugConsole::cmdLoad(const std::vector<std::string>& args) {
    uint32_t slot = args.empty() ? 0 : parseUint(args[0]);

    if (saveManager_) {
        bool success = saveManager_->loadGame(slot);
        return success ? "Loaded from slot " + std::to_string(slot)
                       : "Failed to load from slot " + std::to_string(slot);
    }

    return "Error: SaveManager not available";
}

std::string DebugConsole::cmdHelp(const std::vector<std::string>& args) {
    (void)args;
    std::string help = "Available commands:\n";
    // Sort commands alphabetically for consistent display
    std::vector<std::pair<std::string, CommandEntry>> sortedCommands;
    sortedCommands.reserve(commands_.size());
    for (const auto& pair : commands_) {
        sortedCommands.emplace_back(pair.first, pair.second);
    }
    std::sort(sortedCommands.begin(), sortedCommands.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });
    for (const auto& pair : sortedCommands) {
        help += "  " + pair.second.help + "\n";
    }
    return help;
}

std::string DebugConsole::cmdClear(const std::vector<std::string>& args) {
    (void)args;
    clearOutput();
    return "";
}

// ============================================================================
// Utility methods
// ============================================================================

void DebugConsole::addOutput(const std::string& text, ConsoleLine::Level level) {
    output_.push_back({text, level});
    if (output_.size() > MAX_OUTPUT_LINES) {
        output_.pop_front();  // O(1) with deque
    }
}

std::vector<std::string> DebugConsole::parseInput(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

float DebugConsole::parseFloat(const std::string& s, float defaultVal) {
    try {
        return std::stof(s);
    } catch (...) {
        return defaultVal;
    }
}

uint32_t DebugConsole::parseUint(const std::string& s, uint32_t defaultVal) {
    try {
        unsigned long val = std::stoul(s);
        if (val > UINT32_MAX) {
            addOutput("Warning: Value exceeds 32-bit range, truncating", ConsoleLine::Level::WARNING);
            return UINT32_MAX;
        }
        return static_cast<uint32_t>(val);
    } catch (...) {
        return defaultVal;
    }
}
