#include "game_console.h"
#include "text_renderer.h"
#include <GLES3/gl3.h>
#include <sstream>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG_CONSOLE "GameConsole"
#define LOGD_CONSOLE(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_CONSOLE, __VA_ARGS__)
#define LOGI_CONSOLE(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_CONSOLE, __VA_ARGS__)

GameConsole::GameConsole()
    : textRenderer(nullptr), visible(false), initialized(false),
      historyIndex(-1), historyMaxSize(50), scrollOffset(0.0f),
      cursorBlinkTimer(0.0f), cursorVisible(true),
      lastPlayerPos(0.0f, 0.0f, 0.0f), playerHealth(100.0f), playerMaxHealth(100.0f),
      godMode(false), noclip(false), screenWidth(1080), screenHeight(1920) {
}

GameConsole::~GameConsole() {
    cleanup();
}

bool GameConsole::initialize(TextRenderer* tr) {
    if (initialized) return true;
    textRenderer = tr;
    registerBuiltinCommands();
    initialized = true;
    LOGI_CONSOLE("GameConsole initialized");
    return true;
}

void GameConsole::cleanup() {
    commands.clear();
    outputBuffer.clear();
    commandHistory.clear();
    initialized = false;
}

void GameConsole::toggle() {
    visible = !visible;
    if (visible) {
        LOGI_CONSOLE("Console opened");
    }
}

void GameConsole::onTouchEvent(float x, float y, int action) {
    if (!visible) return;
    handleConsoleTouch(x, y, action);
}

void GameConsole::onKeyPress(int key) {
    if (!visible) return;
    handleConsoleKey(key);
}

void GameConsole::onTextInput(const char* text) {
    if (!visible || !text) return;
    currentInput += text;
}

void GameConsole::update(float deltaTime) {
    if (!visible) return;

    cursorBlinkTimer += deltaTime;
    if (cursorBlinkTimer >= 0.5f) {
        cursorBlinkTimer = 0.0f;
        cursorVisible = !cursorVisible;
    }
}

void GameConsole::render() {
    if (!visible || !textRenderer) return;

    // DPI-aware scaling
    float minDim = static_cast<float>(std::min(screenWidth, screenHeight));
    float scale = minDim / 1080.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    // Console background (semi-transparent dark)
    glClearColor(0.0f, 0.0f, 0.0f, 0.85f);
    glClear(GL_COLOR_BUFFER_BIT);

    float fontSize = 0.6f * scale;
    float lineH = fontSize * 28.0f;
    float margin = 20.0f * scale;
    float consoleHeight = screenHeight * 0.7f;

    // Console area definition
    consoleArea = {0.0f, 0.0f, static_cast<float>(screenWidth), consoleHeight};
    inputArea = {margin, consoleHeight - lineH * 2.0f, static_cast<float>(screenWidth) - margin * 2.0f, lineH * 1.5f};

    // Title
    glm::vec3 titleColor(0.3f, 0.9f, 0.4f);
    textRenderer->renderText("GAME CONSOLE", margin, margin + lineH, titleColor, 0.8f * scale);

    // Output buffer
    glm::vec3 outputColor(0.8f, 0.8f, 0.8f);
    float y = margin + lineH * 2.5f;
    int maxLines = static_cast<int>((consoleHeight - lineH * 5.0f) / lineH);
    int startLine = std::max(0, static_cast<int>(outputBuffer.size()) - maxLines);

    for (int i = startLine; i < static_cast<int>(outputBuffer.size()); ++i) {
        if (y > consoleHeight - lineH * 3.0f) break;
        textRenderer->renderText(outputBuffer[i].c_str(), margin, y, outputColor, fontSize);
        y += lineH;
    }

    // Input area background
    glm::vec4 inputBg(0.1f, 0.1f, 0.15f, 0.9f);
    // Draw input background quad (simplified - using text renderer)

    // Input prompt
    glm::vec3 promptColor(0.5f, 0.8f, 0.5f);
    textRenderer->renderText("> ", margin, inputArea.y + lineH * 0.3f, promptColor, fontSize);

    // Current input
    glm::vec3 inputColor(1.0f, 1.0f, 1.0f);
    textRenderer->renderText(currentInput.c_str(), margin + fontSize * 20.0f, inputArea.y + lineH * 0.3f, inputColor, fontSize);

    // Cursor
    if (cursorVisible) {
        float cursorX = margin + fontSize * 20.0f + textRenderer->getTextWidth(currentInput.c_str(), fontSize);
        textRenderer->renderText("_", cursorX, inputArea.y + lineH * 0.3f, inputColor, fontSize);
    }

    // Help hint
    glm::vec3 hintColor(0.4f, 0.4f, 0.4f);
    textRenderer->renderText("Type 'help' for commands. Tap outside to close.", margin, consoleHeight - lineH * 0.5f, hintColor, fontSize * 0.8f);
}

void GameConsole::executeCommand(const std::string& command) {
    if (command.empty()) return;

    addToHistory(command);
    appendOutput("> " + command);

    std::vector<std::string> args = tokenize(command);
    if (args.empty()) return;

    std::string cmdName = args[0];
    std::transform(cmdName.begin(), cmdName.end(), cmdName.begin(), ::tolower);

    // Find and execute command
    for (const auto& cmd : commands) {
        if (cmd.name == cmdName) {
            if (cmd.handler) {
                cmd.handler(args);
            }
            return;
        }
    }

    appendOutput("Unknown command: " + cmdName + ". Type 'help' for available commands.");
}

void GameConsole::print(const std::string& message) {
    appendOutput(message);
}

void GameConsole::registerCommand(const std::string& name, const std::string& description, CommandHandler handler) {
    CommandInfo info;
    info.name = name;
    info.description = description;
    info.handler = handler;
    commands.push_back(info);
}

void GameConsole::registerBuiltinCommands() {
    // === Existing commands ===
    registerCommand("help", "Show available commands", [this](const std::vector<std::string>& args) { cmdHelp(args); });
    registerCommand("clear", "Clear console output", [this](const std::vector<std::string>& args) { cmdClear(args); });
    registerCommand("teleport", "Teleport to coordinates: teleport <x> <y> <z>", [this](const std::vector<std::string>& args) { cmdTeleport(args); });
    registerCommand("tp", "Alias for teleport", [this](const std::vector<std::string>& args) { cmdTeleport(args); });
    registerCommand("setpos", "Set player position: setpos <x> <y> <z>", [this](const std::vector<std::string>& args) { cmdSetPos(args); });
    registerCommand("spawn", "Spawn NPC: spawn <npcId>", [this](const std::vector<std::string>& args) { cmdSpawn(args); });
    registerCommand("god", "Toggle god mode", [this](const std::vector<std::string>& args) { cmdGod(args); });
    registerCommand("kill", "Kill nearest NPC", [this](const std::vector<std::string>& args) { cmdKill(args); });
    registerCommand("heal", "Heal player to full", [this](const std::vector<std::string>& args) { cmdHeal(args); });
    registerCommand("sethealth", "Set player health: sethealth <value>", [this](const std::vector<std::string>& args) { cmdSetHealth(args); });
    registerCommand("setmana", "Set player mana: setmana <value>", [this](const std::vector<std::string>& args) { cmdSetMana(args); });
    registerCommand("noclip", "Toggle noclip mode", [this](const std::vector<std::string>& args) { cmdNoclip(args); });
    registerCommand("fps", "Toggle FPS display", [this](const std::vector<std::string>& args) { cmdFPS(args); });
    registerCommand("time", "Show current game time", [this](const std::vector<std::string>& args) { cmdTime(args); });
    registerCommand("pos", "Show current position", [this](const std::vector<std::string>& args) { cmdPos(args); });
    registerCommand("stats", "Show player stats", [this](const std::vector<std::string>& args) { cmdStats(args); });

    // === Player commands ===
    registerCommand("setskill", "Set skill value: setskill <name> <value>", [this](const std::vector<std::string>& args) { cmdSetSkill(args); });
    registerCommand("setattr", "Set attribute value: setattr <name> <value>", [this](const std::vector<std::string>& args) { cmdSetAttribute(args); });
    registerCommand("addxp", "Add experience: addxp <amount>", [this](const std::vector<std::string>& args) { cmdAddXp(args); });
    registerCommand("levelup", "Level up player", [this](const std::vector<std::string>& args) { cmdLevelUp(args); });
    registerCommand("setlevel", "Set player level: setlevel <level>", [this](const std::vector<std::string>& args) { cmdSetLevel(args); });
    registerCommand("maxskills", "Max all skills to 100", [this](const std::vector<std::string>& args) { cmdMaxSkills(args); });
    registerCommand("resetstats", "Reset all stats to default", [this](const std::vector<std::string>& args) { cmdResetStats(args); });
    registerCommand("setstamina", "Set stamina: setstamina <value>", [this](const std::vector<std::string>& args) { cmdSetStamina(args); });

    // === Combat commands ===
    registerCommand("attack", "Attack nearest enemy", [this](const std::vector<std::string>& args) { cmdAttack(args); });
    registerCommand("block", "Block action", [this](const std::vector<std::string>& args) { cmdBlock(args); });
    registerCommand("dodge", "Dodge action", [this](const std::vector<std::string>& args) { cmdDodge(args); });
    registerCommand("damage", "Apply damage to NPC: damage <npcId> <amount>", [this](const std::vector<std::string>& args) { cmdDamage(args); });
    registerCommand("killall", "Kill all NPCs in range", [this](const std::vector<std::string>& args) { cmdKillAll(args); });
    registerCommand("resurrect", "Resurrect nearest NPC", [this](const std::vector<std::string>& args) { cmdResurrect(args); });
    registerCommand("combatdebug", "Toggle combat debug info", [this](const std::vector<std::string>& args) { cmdCombatDebug(args); });

    // === Inventory commands ===
    registerCommand("additem", "Add item: additem <itemId> [quantity]", [this](const std::vector<std::string>& args) { cmdAddItem(args); });
    registerCommand("removeitem", "Remove item: removeitem <itemId> [quantity]", [this](const std::vector<std::string>& args) { cmdRemoveItem(args); });
    registerCommand("equip", "Equip item: equip <itemId>", [this](const std::vector<std::string>& args) { cmdEquip(args); });
    registerCommand("unequip", "Unequip item: unequip <slotIndex>", [this](const std::vector<std::string>& args) { cmdUnequip(args); });
    registerCommand("listitems", "List all inventory items", [this](const std::vector<std::string>& args) { cmdListItems(args); });
    registerCommand("clearinv", "Clear inventory", [this](const std::vector<std::string>& args) { cmdClearInv(args); });
    registerCommand("setweight", "Set carry weight: setweight <value>", [this](const std::vector<std::string>& args) { cmdSetWeight(args); });

    // === Magic commands ===
    registerCommand("learnspell", "Learn spell: learnspell <spellId>", [this](const std::vector<std::string>& args) { cmdLearnSpell(args); });
    registerCommand("castspell", "Cast spell on target: castspell <spellId> <targetId>", [this](const std::vector<std::string>& args) { cmdCastSpell(args); });
    registerCommand("equipspell", "Equip spell: equipspell <spellId>", [this](const std::vector<std::string>& args) { cmdEquipSpell(args); });
    registerCommand("listspells", "List known spells", [this](const std::vector<std::string>& args) { cmdListSpells(args); });
    registerCommand("createspell", "Create spell: createspell <name> <damage> <manaCost>", [this](const std::vector<std::string>& args) { cmdCreateSpell(args); });

    // === Quest commands ===
    registerCommand("acceptquest", "Accept quest: acceptquest <questId>", [this](const std::vector<std::string>& args) { cmdAcceptQuest(args); });
    registerCommand("completequest", "Complete quest: completequest <questId>", [this](const std::vector<std::string>& args) { cmdCompleteQuest(args); });
    registerCommand("failquest", "Fail quest: failquest <questId>", [this](const std::vector<std::string>& args) { cmdFailQuest(args); });
    registerCommand("listquests", "List all quests", [this](const std::vector<std::string>& args) { cmdListQuests(args); });
    registerCommand("updateobj", "Update objective: updateobj <questId> <objId> <progress>", [this](const std::vector<std::string>& args) { cmdUpdateObjective(args); });

    // === NPC commands ===
    registerCommand("spawnat", "Spawn NPC at position: spawnat <name> <x> <y> <z>", [this](const std::vector<std::string>& args) { cmdSpawnAt(args); });
    registerCommand("setai", "Set NPC AI state: setai <npcId> <state>", [this](const std::vector<std::string>& args) { cmdSetAi(args); });
    registerCommand("aggro", "Make NPC aggressive: aggro <npcId>", [this](const std::vector<std::string>& args) { cmdAggro(args); });
    registerCommand("calm", "Calm NPC: calm <npcId>", [this](const std::vector<std::string>& args) { cmdCalm(args); });
    registerCommand("listnpcs", "List all NPCs", [this](const std::vector<std::string>& args) { cmdListNpcs(args); });
    registerCommand("nearby", "List nearby NPCs: nearby [radius]", [this](const std::vector<std::string>& args) { cmdNearby(args); });
    registerCommand("resurrectnpc", "Resurrect NPC: resurrectnpc <npcId>", [this](const std::vector<std::string>& args) { cmdResurrectNpc(args); });

    // === Dialogue commands ===
    registerCommand("talk", "Start dialogue with NPC: talk <npcId>", [this](const std::vector<std::string>& args) { cmdTalk(args); });
    registerCommand("selecttopic", "Select dialogue topic: selecttopic <index>", [this](const std::vector<std::string>& args) { cmdSelectTopic(args); });
    registerCommand("selectchoice", "Select dialogue choice: selectchoice <index>", [this](const std::vector<std::string>& args) { cmdSelectChoice(args); });
    registerCommand("endtalk", "End current dialogue", [this](const std::vector<std::string>& args) { cmdEndTalk(args); });

    // === World commands ===
    registerCommand("setweather", "Set weather: setweather <clear|rain|snow|fog|storm>", [this](const std::vector<std::string>& args) { cmdSetWeather(args); });
    registerCommand("settimescale", "Set time scale: settimescale <multiplier>", [this](const std::vector<std::string>& args) { cmdSetTimeScale(args); });
    registerCommand("settime", "Set time of day: settime <hour 0-24>", [this](const std::vector<std::string>& args) { cmdSetTime(args); });
    registerCommand("loadcell", "Load cell: loadcell <x> <y>", [this](const std::vector<std::string>& args) { cmdLoadCell(args); });
    registerCommand("worldinfo", "Show world info", [this](const std::vector<std::string>& args) { cmdWorldInfo(args); });

    // === Save/Load commands ===
    registerCommand("save", "Save game: save <slotIndex>", [this](const std::vector<std::string>& args) { cmdSave(args); });
    registerCommand("load", "Load game: load <slotIndex>", [this](const std::vector<std::string>& args) { cmdLoad(args); });
    registerCommand("quicksave", "Quick save", [this](const std::vector<std::string>& args) { cmdQuickSave(args); });
    registerCommand("quickload", "Quick load", [this](const std::vector<std::string>& args) { cmdQuickLoad(args); });
    registerCommand("listsaves", "List save slots", [this](const std::vector<std::string>& args) { cmdListSaves(args); });

    // === UI commands ===
    registerCommand("openmenu", "Open menu: openmenu <menuName>", [this](const std::vector<std::string>& args) { cmdOpenMenu(args); });
    registerCommand("closemenu", "Close current menu", [this](const std::vector<std::string>& args) { cmdCloseMenu(args); });
    registerCommand("debugmenu", "Toggle debug menu", [this](const std::vector<std::string>& args) { cmdDebugMenu(args); });

    // Phase 65: Extended Debug Commands
    registerCommand("wireframe", "Toggle wireframe rendering", [this](const std::vector<std::string>&) {
        if (gameRefs.toggleWireframe) gameRefs.toggleWireframe();
        print("Wireframe mode toggled");
    });
    registerCommand("aabb", "Toggle AABB visualization", [this](const std::vector<std::string>&) {
        if (gameRefs.toggleAabb) gameRefs.toggleAabb();
        print("AABB visualization toggled");
    });
    registerCommand("npcoverlay", "Toggle NPC info overlay", [this](const std::vector<std::string>&) {
        if (gameRefs.toggleNpcOverlay) gameRefs.toggleNpcOverlay();
        print("NPC overlay toggled");
    });
    registerCommand("touchtrail", "Toggle touch trail display", [this](const std::vector<std::string>&) {
        if (gameRefs.toggleTouchTrail) gameRefs.toggleTouchTrail();
        print("Touch trail toggled");
    });
    registerCommand("debugpage", "Switch debug HUD page: debugpage <1-4>", [this](const std::vector<std::string>& args) {
        if (args.empty()) {
            if (gameRefs.debugHudNextPage) gameRefs.debugHudNextPage();
            print("Debug HUD: next page");
        } else {
            int page = std::atoi(args[0].c_str());
            if (page >= 1 && page <= 4) {
                print("Debug HUD: page " + std::to_string(page));
            }
        }
    });
    registerCommand("debuglog", "Toggle in-game log overlay", [this](const std::vector<std::string>&) {
        if (gameRefs.toggleDebugLog) gameRefs.toggleDebugLog();
        print("Debug log overlay toggled");
    });
    registerCommand("perfstats", "Show detailed performance stats", [this](const std::vector<std::string>&) {
        print("=== Performance Stats ===");
        print("Use 'debugpage 2' for detailed view");
    });

    // === Sound commands ===
    registerCommand("playbgm", "Play background music", [this](const std::vector<std::string>&) {
        if (gameRefs.playBgm) gameRefs.playBgm();
        print("Playing BGM");
    });
    registerCommand("stopbgm", "Stop background music", [this](const std::vector<std::string>&) {
        if (gameRefs.stopBgm) gameRefs.stopBgm();
        print("BGM stopped");
    });
    registerCommand("playse", "Play sound effect", [this](const std::vector<std::string>&) {
        if (gameRefs.playSe) gameRefs.playSe();
        print("Playing SE");
    });
    registerCommand("stopallse", "Stop all sound effects", [this](const std::vector<std::string>&) {
        if (gameRefs.stopAllSe) gameRefs.stopAllSe();
        print("All SE stopped");
    });
    registerCommand("setvolume", "Set master volume: setvolume <0.0-1.0>", [this](const std::vector<std::string>& args) {
        if (args.empty()) {
            print("Usage: setvolume <0.0-1.0>");
            return;
        }
        float vol = std::stof(args[0]);
        if (gameRefs.setMasterVolume) gameRefs.setMasterVolume(vol);
        print("Volume set to " + std::to_string(vol));
    });
    registerCommand("mute", "Mute all audio", [this](const std::vector<std::string>&) {
        if (gameRefs.muteAll) gameRefs.muteAll();
        print("Audio muted");
    });
    registerCommand("unmute", "Unmute all audio", [this](const std::vector<std::string>&) {
        if (gameRefs.unmuteAll) gameRefs.unmuteAll();
        print("Audio unmuted");
    });
    registerCommand("listaudio", "List all audio files", [this](const std::vector<std::string>&) {
        if (gameRefs.listAudio) {
            print(gameRefs.listAudio());
        } else {
            print("Audio listing not available");
        }
    });
    registerCommand("audiostats", "Show audio statistics", [this](const std::vector<std::string>&) {
        if (gameRefs.getAudioStats) {
            print(gameRefs.getAudioStats());
        } else {
            print("Audio stats not available");
        }
    });

    // === Asset commands ===
    registerCommand("listtextures", "List all loaded textures", [this](const std::vector<std::string>&) {
        if (gameRefs.listTextures) {
            print(gameRefs.listTextures());
        } else {
            print("Texture listing not available");
        }
    });
    registerCommand("listmodels", "List all loaded models", [this](const std::vector<std::string>&) {
        if (gameRefs.listModels) {
            print(gameRefs.listModels());
        } else {
            print("Model listing not available");
        }
    });
    registerCommand("textureinfo", "Show texture info", [this](const std::vector<std::string>&) {
        if (gameRefs.getTextureInfo) {
            print(gameRefs.getTextureInfo());
        } else {
            print("Texture info not available");
        }
    });
    registerCommand("modelinfo", "Show model info", [this](const std::vector<std::string>&) {
        if (gameRefs.getModelInfo) {
            print(gameRefs.getModelInfo());
        } else {
            print("Model info not available");
        }
    });
    registerCommand("cachestats", "Show cache statistics", [this](const std::vector<std::string>&) {
        if (gameRefs.getCacheStats) {
            print(gameRefs.getCacheStats());
        } else {
            print("Cache stats not available");
        }
    });
    registerCommand("clearcache", "Clear asset cache", [this](const std::vector<std::string>&) {
        if (gameRefs.clearCache) gameRefs.clearCache();
        print("Cache cleared");
    });
    registerCommand("reloadassets", "Reload all assets", [this](const std::vector<std::string>&) {
        if (gameRefs.reloadAssets) gameRefs.reloadAssets();
        print("Assets reloaded");
    });
    registerCommand("memoryusage", "Show memory usage", [this](const std::vector<std::string>&) {
        if (gameRefs.getMemoryUsage) {
            print(gameRefs.getMemoryUsage());
        } else {
            print("Memory usage not available");
        }
    });
    registerCommand("assetstats", "Show asset statistics", [this](const std::vector<std::string>&) {
        if (gameRefs.getAssetStats) {
            print(gameRefs.getAssetStats());
        } else {
            print("Asset stats not available");
        }
    });

    // === Log commands ===
    registerCommand("loglevel", "Set log level: loglevel <all|debug|info|warn|error>", [this](const std::vector<std::string>& args) {
        if (args.empty()) {
            print("Usage: loglevel <all|debug|info|warn|error>");
            return;
        }
        if (gameRefs.setLogLevel) gameRefs.setLogLevel(args[0]);
        print("Log level set to " + args[0]);
    });
    registerCommand("clearlogs", "Clear log buffer", [this](const std::vector<std::string>&) {
        if (gameRefs.clearLogs) gameRefs.clearLogs();
        print("Logs cleared");
    });
    registerCommand("exportlogs", "Export logs to file", [this](const std::vector<std::string>&) {
        if (gameRefs.exportLogs) gameRefs.exportLogs();
        print("Logs exported");
    });
    registerCommand("logstats", "Show log statistics", [this](const std::vector<std::string>&) {
        if (gameRefs.getLogStats) {
            print(gameRefs.getLogStats());
        } else {
            print("Log stats not available");
        }
    });
    registerCommand("searchlog", "Search logs: searchlog <pattern>", [this](const std::vector<std::string>& args) {
        if (args.empty()) {
            print("Usage: searchlog <pattern>");
            return;
        }
        if (gameRefs.searchLogs) gameRefs.searchLogs(args[0]);
        print("Searching logs for: " + args[0]);
    });
    registerCommand("logautoscroll", "Toggle log auto-scroll", [this](const std::vector<std::string>&) {
        if (gameRefs.toggleLogAutoScroll) gameRefs.toggleLogAutoScroll();
        print("Log auto-scroll toggled");
    });
}

void GameConsole::handleConsoleTouch(float x, float y, int action) {
    // Close console if tapping outside console area
    if (action == 0 && !consoleArea.contains(x, y)) {
        visible = false;
        return;
    }

    // Focus input area on tap
    if (action == 0 && inputArea.contains(x, y)) {
        // Input area focused - keyboard should appear
        // On Android, we'd request soft keyboard here
    }
}

void GameConsole::handleConsoleKey(int key) {
    // Handle special keys
    switch (key) {
        case 66: // Enter
            processInput();
            break;
        case 67: // Backspace
            if (!currentInput.empty()) {
                currentInput.pop_back();
            }
            break;
        case 111: // Escape
            visible = false;
            break;
        case 19: // Up arrow - history
            if (!commandHistory.empty()) {
                if (historyIndex < static_cast<int>(commandHistory.size()) - 1) {
                    historyIndex++;
                }
                currentInput = commandHistory[commandHistory.size() - 1 - historyIndex];
            }
            break;
        case 20: // Down arrow - history
            if (historyIndex > 0) {
                historyIndex--;
                currentInput = commandHistory[commandHistory.size() - 1 - historyIndex];
            } else {
                historyIndex = -1;
                currentInput.clear();
            }
            break;
    }
}

void GameConsole::processInput() {
    if (currentInput.empty()) return;
    executeCommand(currentInput);
    currentInput.clear();
    historyIndex = -1;
}

std::vector<std::string> GameConsole::tokenize(const std::string& str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

void GameConsole::addToHistory(const std::string& cmd) {
    commandHistory.push_back(cmd);
    if (static_cast<int>(commandHistory.size()) > historyMaxSize) {
        commandHistory.erase(commandHistory.begin());
    }
}

void GameConsole::appendOutput(const std::string& line) {
    outputBuffer.push_back(line);
    // Keep buffer manageable
    if (outputBuffer.size() > 200) {
        outputBuffer.erase(outputBuffer.begin());
    }
}

// ============================================================
// Built-in Command Implementations
// ============================================================

void GameConsole::cmdHelp(const std::vector<std::string>& args) {
    appendOutput("=== Available Commands ===");
    for (const auto& cmd : commands) {
        appendOutput("  " + cmd.name + " - " + cmd.description);
    }
}

void GameConsole::cmdClear(const std::vector<std::string>& args) {
    outputBuffer.clear();
}

void GameConsole::cmdTeleport(const std::vector<std::string>& args) {
    if (args.size() < 4) {
        appendOutput("Usage: teleport <x> <y> <z>");
        return;
    }
    try {
        float x = std::stof(args[1]);
        float y = std::stof(args[2]);
        float z = std::stof(args[3]);
        lastPlayerPos = glm::vec3(x, y, z);
        appendOutput("Teleported to (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ")");
        LOGI_CONSOLE("Teleport command: (%f, %f, %f)", x, y, z);
    } catch (...) {
        appendOutput("Invalid coordinates. Use: teleport <x> <y> <z>");
    }
}

void GameConsole::cmdSetPos(const std::vector<std::string>& args) {
    cmdTeleport(args); // Same as teleport
}

void GameConsole::cmdSpawn(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: spawn <npcId>");
        return;
    }
    try {
        int npcId = std::stoi(args[1]);
        appendOutput("Spawn request for NPC ID=" + std::to_string(npcId));
        LOGI_CONSOLE("Spawn command: NPC ID=%d", npcId);
        // Actual spawn would be handled by callback
    } catch (...) {
        appendOutput("Invalid NPC ID. Use: spawn <npcId>");
    }
}

void GameConsole::cmdGod(const std::vector<std::string>& args) {
    godMode = !godMode;
    appendOutput("God mode: " + std::string(godMode ? "ON" : "OFF"));
}

void GameConsole::cmdKill(const std::vector<std::string>& args) {
    appendOutput("Kill nearest NPC (not implemented - requires NPC manager callback)");
}

void GameConsole::cmdHeal(const std::vector<std::string>& args) {
    playerHealth = playerMaxHealth;
    appendOutput("Player healed to " + std::to_string(static_cast<int>(playerHealth)) + " HP");
}

void GameConsole::cmdSetHealth(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: sethealth <value>");
        return;
    }
    try {
        float hp = std::stof(args[1]);
        playerHealth = std::min(hp, playerMaxHealth);
        appendOutput("Health set to " + std::to_string(static_cast<int>(playerHealth)));
    } catch (...) {
        appendOutput("Invalid value. Use: sethealth <number>");
    }
}

void GameConsole::cmdSetMana(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: setmana <value>");
        return;
    }
    appendOutput("Mana set to " + args[1] + " (requires player mana callback)");
}

void GameConsole::cmdNoclip(const std::vector<std::string>& args) {
    noclip = !noclip;
    appendOutput("Noclip: " + std::string(noclip ? "ON" : "OFF"));
}

void GameConsole::cmdFPS(const std::vector<std::string>& args) {
    appendOutput("FPS display toggled (handled by renderer)");
}

void GameConsole::cmdTime(const std::vector<std::string>& args) {
    appendOutput("Game time: (requires time system callback)");
}

void GameConsole::cmdPos(const std::vector<std::string>& args) {
    appendOutput("Position: (" + std::to_string(static_cast<int>(lastPlayerPos.x)) + ", " +
                 std::to_string(static_cast<int>(lastPlayerPos.y)) + ", " +
                 std::to_string(static_cast<int>(lastPlayerPos.z)) + ")");
}

void GameConsole::cmdStats(const std::vector<std::string>& args) {
    if (gameRefs.getPlayerStats) {
        appendOutput(gameRefs.getPlayerStats());
    } else {
        appendOutput("=== Player Stats ===");
        appendOutput("Health: " + std::to_string(static_cast<int>(playerHealth)) + "/" + std::to_string(static_cast<int>(playerMaxHealth)));
        appendOutput("God Mode: " + std::string(godMode ? "ON" : "OFF"));
        appendOutput("Noclip: " + std::string(noclip ? "ON" : "OFF"));
    }
}

// ============================================================
// Player Commands
// ============================================================

void GameConsole::cmdSetSkill(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        appendOutput("Usage: setskill <name> <value>");
        appendOutput("Skills: Blade, Blunt, Block, Restoration, Destruction, Alteration,");
        appendOutput("        Conjuration, Illusion, Mysticism, Marksman, Athletics, Acrobatics");
        return;
    }
    try {
        int value = std::stoi(args[2]);
        if (value < 0 || value > 100) {
            appendOutput("Skill value must be 0-100");
            return;
        }
        if (gameRefs.setSkill) {
            gameRefs.setSkill(args[1], value);
            appendOutput("Skill " + args[1] + " set to " + std::to_string(value));
        } else {
            appendOutput("Player system not connected");
        }
    } catch (...) {
        appendOutput("Invalid value. Use: setskill <name> <0-100>");
    }
}

void GameConsole::cmdSetAttribute(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        appendOutput("Usage: setattr <name> <value>");
        appendOutput("Attributes: Strength, Intelligence, Willpower, Agility, Speed, Endurance, Personality, Luck");
        return;
    }
    try {
        int value = std::stoi(args[2]);
        if (value < 1 || value > 255) {
            appendOutput("Attribute value must be 1-255");
            return;
        }
        if (gameRefs.setAttribute) {
            gameRefs.setAttribute(args[1], value);
            appendOutput("Attribute " + args[1] + " set to " + std::to_string(value));
        } else {
            appendOutput("Player system not connected");
        }
    } catch (...) {
        appendOutput("Invalid value. Use: setattr <name> <1-255>");
    }
}

void GameConsole::cmdAddXp(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: addxp <amount>");
        return;
    }
    try {
        float amount = std::stof(args[1]);
        if (gameRefs.addExperience) {
            gameRefs.addExperience(amount);
            appendOutput("Added " + std::to_string(static_cast<int>(amount)) + " XP");
        } else {
            appendOutput("Player system not connected");
        }
    } catch (...) {
        appendOutput("Invalid amount. Use: addxp <number>");
    }
}

void GameConsole::cmdLevelUp(const std::vector<std::string>& args) {
    if (gameRefs.setLevel) {
        // Get current level and increment
        appendOutput("Level up triggered");
    } else {
        appendOutput("Player system not connected");
    }
}

void GameConsole::cmdSetLevel(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: setlevel <level>");
        return;
    }
    try {
        int level = std::stoi(args[1]);
        if (level < 1 || level > 50) {
            appendOutput("Level must be 1-50");
            return;
        }
        if (gameRefs.setLevel) {
            gameRefs.setLevel(level);
            appendOutput("Player level set to " + std::to_string(level));
        } else {
            appendOutput("Player system not connected");
        }
    } catch (...) {
        appendOutput("Invalid level. Use: setlevel <1-50>");
    }
}

void GameConsole::cmdMaxSkills(const std::vector<std::string>& args) {
    if (gameRefs.maxAllSkills) {
        gameRefs.maxAllSkills();
        appendOutput("All skills set to 100");
    } else {
        appendOutput("Player system not connected");
    }
}

void GameConsole::cmdResetStats(const std::vector<std::string>& args) {
    if (gameRefs.resetPlayerStats) {
        gameRefs.resetPlayerStats();
        appendOutput("All stats reset to default");
    } else {
        appendOutput("Player system not connected");
    }
}

void GameConsole::cmdSetStamina(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: setstamina <value>");
        return;
    }
    try {
        float value = std::stof(args[1]);
        if (gameRefs.setStamina) {
            gameRefs.setStamina(value);
            appendOutput("Stamina set to " + std::to_string(static_cast<int>(value)));
        } else {
            appendOutput("Player system not connected");
        }
    } catch (...) {
        appendOutput("Invalid value. Use: setstamina <number>");
    }
}

// ============================================================
// Combat Commands
// ============================================================

void GameConsole::cmdAttack(const std::vector<std::string>& args) {
    if (gameRefs.attackNearest) {
        gameRefs.attackNearest();
        appendOutput("Attacking nearest enemy");
    } else {
        appendOutput("Combat system not connected");
    }
}

void GameConsole::cmdBlock(const std::vector<std::string>& args) {
    if (gameRefs.blockAction) {
        gameRefs.blockAction();
        appendOutput("Block action triggered");
    } else {
        appendOutput("Combat system not connected");
    }
}

void GameConsole::cmdDodge(const std::vector<std::string>& args) {
    if (gameRefs.dodgeAction) {
        gameRefs.dodgeAction();
        appendOutput("Dodge action triggered");
    } else {
        appendOutput("Combat system not connected");
    }
}

void GameConsole::cmdDamage(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        appendOutput("Usage: damage <npcId> <amount>");
        return;
    }
    try {
        uint32_t npcId = static_cast<uint32_t>(std::stoi(args[1]));
        float amount = std::stof(args[2]);
        if (gameRefs.applyDamageToNpc) {
            gameRefs.applyDamageToNpc(npcId, amount);
            appendOutput("Applied " + std::to_string(static_cast<int>(amount)) + " damage to NPC " + std::to_string(npcId));
        } else {
            appendOutput("Combat system not connected");
        }
    } catch (...) {
        appendOutput("Invalid args. Use: damage <npcId> <amount>");
    }
}

void GameConsole::cmdKillAll(const std::vector<std::string>& args) {
    if (gameRefs.killAllNpcs) {
        gameRefs.killAllNpcs();
        appendOutput("All NPCs killed");
    } else {
        appendOutput("Combat system not connected");
    }
}

void GameConsole::cmdResurrect(const std::vector<std::string>& args) {
    if (gameRefs.resurrectNpc) {
        gameRefs.resurrectNpc(0); // 0 = nearest
        appendOutput("Resurrected nearest NPC");
    } else {
        appendOutput("Combat system not connected");
    }
}

void GameConsole::cmdCombatDebug(const std::vector<std::string>& args) {
    if (gameRefs.toggleCombatDebug) {
        gameRefs.toggleCombatDebug();
        appendOutput("Combat debug toggled");
    } else {
        appendOutput("Combat system not connected");
    }
}

// ============================================================
// Inventory Commands
// ============================================================

void GameConsole::cmdAddItem(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: additem <itemId> [quantity]");
        return;
    }
    try {
        uint32_t itemId = static_cast<uint32_t>(std::stoi(args[1]));
        uint32_t quantity = (args.size() >= 3) ? static_cast<uint32_t>(std::stoi(args[2])) : 1;
        if (gameRefs.addItem) {
            gameRefs.addItem(itemId, quantity);
            appendOutput("Added " + std::to_string(quantity) + "x item " + std::to_string(itemId));
        } else {
            appendOutput("Inventory system not connected");
        }
    } catch (...) {
        appendOutput("Invalid args. Use: additem <itemId> [quantity]");
    }
}

void GameConsole::cmdRemoveItem(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: removeitem <itemId> [quantity]");
        return;
    }
    try {
        uint32_t itemId = static_cast<uint32_t>(std::stoi(args[1]));
        uint32_t quantity = (args.size() >= 3) ? static_cast<uint32_t>(std::stoi(args[2])) : 1;
        if (gameRefs.removeItem) {
            gameRefs.removeItem(itemId, quantity);
            appendOutput("Removed " + std::to_string(quantity) + "x item " + std::to_string(itemId));
        } else {
            appendOutput("Inventory system not connected");
        }
    } catch (...) {
        appendOutput("Invalid args. Use: removeitem <itemId> [quantity]");
    }
}

void GameConsole::cmdEquip(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: equip <itemId>");
        return;
    }
    try {
        uint32_t itemId = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.equipItem) {
            gameRefs.equipItem(itemId);
            appendOutput("Equipped item " + std::to_string(itemId));
        } else {
            appendOutput("Inventory system not connected");
        }
    } catch (...) {
        appendOutput("Invalid item ID. Use: equip <itemId>");
    }
}

void GameConsole::cmdUnequip(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: unequip <slotIndex>");
        return;
    }
    try {
        uint32_t slot = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.unequipItem) {
            gameRefs.unequipItem(slot);
            appendOutput("Unequipped slot " + std::to_string(slot));
        } else {
            appendOutput("Inventory system not connected");
        }
    } catch (...) {
        appendOutput("Invalid slot. Use: unequip <slotIndex>");
    }
}

void GameConsole::cmdListItems(const std::vector<std::string>& args) {
    if (gameRefs.listInventory) {
        appendOutput(gameRefs.listInventory());
    } else {
        appendOutput("Inventory system not connected");
    }
}

void GameConsole::cmdClearInv(const std::vector<std::string>& args) {
    if (gameRefs.clearInventory) {
        gameRefs.clearInventory();
        appendOutput("Inventory cleared");
    } else {
        appendOutput("Inventory system not connected");
    }
}

void GameConsole::cmdSetWeight(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: setweight <value>");
        return;
    }
    try {
        float weight = std::stof(args[1]);
        if (gameRefs.setCarryWeight) {
            gameRefs.setCarryWeight(weight);
            appendOutput("Carry weight set to " + std::to_string(static_cast<int>(weight)));
        } else {
            appendOutput("Inventory system not connected");
        }
    } catch (...) {
        appendOutput("Invalid value. Use: setweight <number>");
    }
}

// ============================================================
// Magic Commands
// ============================================================

void GameConsole::cmdLearnSpell(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: learnspell <spellId>");
        return;
    }
    try {
        uint32_t spellId = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.learnSpell) {
            gameRefs.learnSpell(spellId);
            appendOutput("Learned spell " + std::to_string(spellId));
        } else {
            appendOutput("Magic system not connected");
        }
    } catch (...) {
        appendOutput("Invalid spell ID. Use: learnspell <spellId>");
    }
}

void GameConsole::cmdCastSpell(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        appendOutput("Usage: castspell <spellId> <targetId>");
        return;
    }
    try {
        uint32_t spellId = static_cast<uint32_t>(std::stoi(args[1]));
        uint32_t targetId = static_cast<uint32_t>(std::stoi(args[2]));
        if (gameRefs.castSpellOnTarget) {
            gameRefs.castSpellOnTarget(spellId, targetId);
            appendOutput("Cast spell " + std::to_string(spellId) + " on target " + std::to_string(targetId));
        } else {
            appendOutput("Magic system not connected");
        }
    } catch (...) {
        appendOutput("Invalid args. Use: castspell <spellId> <targetId>");
    }
}

void GameConsole::cmdEquipSpell(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: equipspell <spellId>");
        return;
    }
    try {
        uint32_t spellId = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.equipSpell) {
            gameRefs.equipSpell(spellId);
            appendOutput("Equipped spell " + std::to_string(spellId));
        } else {
            appendOutput("Magic system not connected");
        }
    } catch (...) {
        appendOutput("Invalid spell ID. Use: equipspell <spellId>");
    }
}

void GameConsole::cmdListSpells(const std::vector<std::string>& args) {
    if (gameRefs.listSpells) {
        appendOutput(gameRefs.listSpells());
    } else {
        appendOutput("Magic system not connected");
    }
}

void GameConsole::cmdCreateSpell(const std::vector<std::string>& args) {
    if (args.size() < 4) {
        appendOutput("Usage: createspell <name> <damage> <manaCost>");
        return;
    }
    try {
        float damage = std::stof(args[2]);
        float manaCost = std::stof(args[3]);
        if (gameRefs.createSpell) {
            gameRefs.createSpell(args[1], damage, manaCost);
            appendOutput("Created spell '" + args[1] + "' (damage=" + std::to_string(static_cast<int>(damage)) + ", mana=" + std::to_string(static_cast<int>(manaCost)) + ")");
        } else {
            appendOutput("Magic system not connected");
        }
    } catch (...) {
        appendOutput("Invalid args. Use: createspell <name> <damage> <manaCost>");
    }
}

// ============================================================
// Quest Commands
// ============================================================

void GameConsole::cmdAcceptQuest(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: acceptquest <questId>");
        return;
    }
    try {
        uint32_t questId = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.acceptQuest) {
            gameRefs.acceptQuest(questId);
            appendOutput("Accepted quest " + std::to_string(questId));
        } else {
            appendOutput("Quest system not connected");
        }
    } catch (...) {
        appendOutput("Invalid quest ID. Use: acceptquest <questId>");
    }
}

void GameConsole::cmdCompleteQuest(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: completequest <questId>");
        return;
    }
    try {
        uint32_t questId = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.completeQuest) {
            gameRefs.completeQuest(questId);
            appendOutput("Completed quest " + std::to_string(questId));
        } else {
            appendOutput("Quest system not connected");
        }
    } catch (...) {
        appendOutput("Invalid quest ID. Use: completequest <questId>");
    }
}

void GameConsole::cmdFailQuest(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: failquest <questId>");
        return;
    }
    try {
        uint32_t questId = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.failQuest) {
            gameRefs.failQuest(questId);
            appendOutput("Failed quest " + std::to_string(questId));
        } else {
            appendOutput("Quest system not connected");
        }
    } catch (...) {
        appendOutput("Invalid quest ID. Use: failquest <questId>");
    }
}

void GameConsole::cmdListQuests(const std::vector<std::string>& args) {
    if (gameRefs.listQuests) {
        appendOutput(gameRefs.listQuests());
    } else {
        appendOutput("Quest system not connected");
    }
}

void GameConsole::cmdUpdateObjective(const std::vector<std::string>& args) {
    if (args.size() < 4) {
        appendOutput("Usage: updateobj <questId> <objId> <progress>");
        return;
    }
    try {
        uint32_t questId = static_cast<uint32_t>(std::stoi(args[1]));
        uint32_t objId = static_cast<uint32_t>(std::stoi(args[2]));
        uint32_t progress = static_cast<uint32_t>(std::stoi(args[3]));
        if (gameRefs.updateObjective) {
            gameRefs.updateObjective(questId, objId, progress);
            appendOutput("Updated objective " + std::to_string(objId) + " progress to " + std::to_string(progress));
        } else {
            appendOutput("Quest system not connected");
        }
    } catch (...) {
        appendOutput("Invalid args. Use: updateobj <questId> <objId> <progress>");
    }
}

// ============================================================
// NPC Commands
// ============================================================

void GameConsole::cmdSpawnAt(const std::vector<std::string>& args) {
    if (args.size() < 5) {
        appendOutput("Usage: spawnat <name> <x> <y> <z>");
        return;
    }
    try {
        float x = std::stof(args[2]);
        float y = std::stof(args[3]);
        float z = std::stof(args[4]);
        if (gameRefs.spawnNpcAt) {
            uint32_t id = gameRefs.spawnNpcAt(args[1], x, y, z);
            appendOutput("Spawned NPC '" + args[1] + "' at (" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + ") ID=" + std::to_string(id));
        } else {
            appendOutput("NPC system not connected");
        }
    } catch (...) {
        appendOutput("Invalid args. Use: spawnat <name> <x> <y> <z>");
    }
}

void GameConsole::cmdSetAi(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        appendOutput("Usage: setai <npcId> <state>");
        appendOutput("States: idle, patrol, follow, flee, combat, sleep");
        return;
    }
    try {
        uint32_t npcId = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.setNpcAiState) {
            gameRefs.setNpcAiState(npcId, args[2]);
            appendOutput("NPC " + std::to_string(npcId) + " AI set to " + args[2]);
        } else {
            appendOutput("NPC system not connected");
        }
    } catch (...) {
        appendOutput("Invalid args. Use: setai <npcId> <state>");
    }
}

void GameConsole::cmdAggro(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: aggro <npcId>");
        return;
    }
    try {
        uint32_t npcId = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.setNpcAggression) {
            gameRefs.setNpcAggression(npcId, 100.0f);
            appendOutput("NPC " + std::to_string(npcId) + " is now aggressive");
        } else {
            appendOutput("NPC system not connected");
        }
    } catch (...) {
        appendOutput("Invalid NPC ID. Use: aggro <npcId>");
    }
}

void GameConsole::cmdCalm(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: calm <npcId>");
        return;
    }
    try {
        uint32_t npcId = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.calmNpc) {
            gameRefs.calmNpc(npcId);
            appendOutput("NPC " + std::to_string(npcId) + " calmed");
        } else {
            appendOutput("NPC system not connected");
        }
    } catch (...) {
        appendOutput("Invalid NPC ID. Use: calm <npcId>");
    }
}

void GameConsole::cmdListNpcs(const std::vector<std::string>& args) {
    if (gameRefs.listNpcs) {
        appendOutput(gameRefs.listNpcs());
    } else {
        appendOutput("NPC system not connected");
    }
}

void GameConsole::cmdNearby(const std::vector<std::string>& args) {
    if (gameRefs.listNearbyNpcs) {
        appendOutput(gameRefs.listNearbyNpcs());
    } else {
        appendOutput("NPC system not connected");
    }
}

void GameConsole::cmdResurrectNpc(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: resurrectnpc <npcId>");
        return;
    }
    try {
        uint32_t npcId = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.resurrectNpc) {
            gameRefs.resurrectNpc(npcId);
            appendOutput("Resurrected NPC " + std::to_string(npcId));
        } else {
            appendOutput("NPC system not connected");
        }
    } catch (...) {
        appendOutput("Invalid NPC ID. Use: resurrectnpc <npcId>");
    }
}

// ============================================================
// Dialogue Commands
// ============================================================

void GameConsole::cmdTalk(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: talk <npcId>");
        return;
    }
    try {
        uint32_t npcId = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.startDialogueWith) {
            gameRefs.startDialogueWith(npcId);
            appendOutput("Started dialogue with NPC " + std::to_string(npcId));
        } else {
            appendOutput("Dialogue system not connected");
        }
    } catch (...) {
        appendOutput("Invalid NPC ID. Use: talk <npcId>");
    }
}

void GameConsole::cmdSelectTopic(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: selecttopic <index>");
        return;
    }
    try {
        int index = std::stoi(args[1]);
        if (gameRefs.selectDialogueTopic) {
            gameRefs.selectDialogueTopic(index);
            appendOutput("Selected topic " + std::to_string(index));
        } else {
            appendOutput("Dialogue system not connected");
        }
    } catch (...) {
        appendOutput("Invalid index. Use: selecttopic <index>");
    }
}

void GameConsole::cmdSelectChoice(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: selectchoice <index>");
        return;
    }
    try {
        int index = std::stoi(args[1]);
        if (gameRefs.selectDialogueChoice) {
            gameRefs.selectDialogueChoice(index);
            appendOutput("Selected choice " + std::to_string(index));
        } else {
            appendOutput("Dialogue system not connected");
        }
    } catch (...) {
        appendOutput("Invalid index. Use: selectchoice <index>");
    }
}

void GameConsole::cmdEndTalk(const std::vector<std::string>& args) {
    if (gameRefs.endDialogue) {
        gameRefs.endDialogue();
        appendOutput("Dialogue ended");
    } else {
        appendOutput("Dialogue system not connected");
    }
}

// ============================================================
// World Commands
// ============================================================

void GameConsole::cmdSetWeather(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: setweather <clear|rain|snow|fog|storm>");
        return;
    }
    if (gameRefs.setWeather) {
        gameRefs.setWeather(args[1]);
        appendOutput("Weather set to " + args[1]);
    } else {
        appendOutput("World system not connected");
    }
}

void GameConsole::cmdSetTimeScale(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: settimescale <multiplier>");
        appendOutput("  1 = real-time, 30 = default Oblivion, 0 = paused");
        return;
    }
    try {
        float scale = std::stof(args[1]);
        if (gameRefs.setTimeScale) {
            gameRefs.setTimeScale(scale);
            appendOutput("Time scale set to " + std::to_string(scale) + "x");
        } else {
            appendOutput("World system not connected");
        }
    } catch (...) {
        appendOutput("Invalid value. Use: settimescale <number>");
    }
}

void GameConsole::cmdSetTime(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: settime <hour 0-24>");
        appendOutput("  0=midnight, 6=dawn, 12=noon, 18=dusk");
        return;
    }
    try {
        float hour = std::stof(args[1]);
        if (hour < 0.0f || hour > 24.0f) {
            appendOutput("Hour must be 0-24");
            return;
        }
        if (gameRefs.setTimeOfDay) {
            gameRefs.setTimeOfDay(hour);
            appendOutput("Time set to " + std::to_string(hour) + ":00");
        } else {
            appendOutput("World system not connected");
        }
    } catch (...) {
        appendOutput("Invalid hour. Use: settime <0-24>");
    }
}

void GameConsole::cmdLoadCell(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        appendOutput("Usage: loadcell <x> <y>");
        return;
    }
    try {
        int32_t x = std::stoi(args[1]);
        int32_t y = std::stoi(args[2]);
        if (gameRefs.loadCell) {
            gameRefs.loadCell(x, y);
            appendOutput("Loading cell (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        } else {
            appendOutput("World system not connected");
        }
    } catch (...) {
        appendOutput("Invalid coords. Use: loadcell <x> <y>");
    }
}

void GameConsole::cmdWorldInfo(const std::vector<std::string>& args) {
    if (gameRefs.getWorldInfo) {
        appendOutput(gameRefs.getWorldInfo());
    } else {
        appendOutput("World system not connected");
    }
}

// ============================================================
// Save/Load Commands
// ============================================================

void GameConsole::cmdSave(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: save <slotIndex 0-9>");
        return;
    }
    try {
        uint32_t slot = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.saveGameSlot) {
            gameRefs.saveGameSlot(slot);
            appendOutput("Saved to slot " + std::to_string(slot));
        } else {
            appendOutput("Save system not connected");
        }
    } catch (...) {
        appendOutput("Invalid slot. Use: save <0-9>");
    }
}

void GameConsole::cmdLoad(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: load <slotIndex 0-9>");
        return;
    }
    try {
        uint32_t slot = static_cast<uint32_t>(std::stoi(args[1]));
        if (gameRefs.loadGameSlot) {
            gameRefs.loadGameSlot(slot);
            appendOutput("Loaded from slot " + std::to_string(slot));
        } else {
            appendOutput("Save system not connected");
        }
    } catch (...) {
        appendOutput("Invalid slot. Use: load <0-9>");
    }
}

void GameConsole::cmdQuickSave(const std::vector<std::string>& args) {
    if (gameRefs.quickSave) {
        gameRefs.quickSave();
        appendOutput("Quick save complete");
    } else {
        appendOutput("Save system not connected");
    }
}

void GameConsole::cmdQuickLoad(const std::vector<std::string>& args) {
    if (gameRefs.quickLoad) {
        gameRefs.quickLoad();
        appendOutput("Quick load complete");
    } else {
        appendOutput("Save system not connected");
    }
}

void GameConsole::cmdListSaves(const std::vector<std::string>& args) {
    if (gameRefs.listSaveSlots) {
        appendOutput(gameRefs.listSaveSlots());
    } else {
        appendOutput("Save system not connected");
    }
}

// ============================================================
// UI Commands
// ============================================================

void GameConsole::cmdOpenMenu(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        appendOutput("Usage: openmenu <menuName>");
        appendOutput("Menus: inventory, magic, map, stats, settings, console");
        return;
    }
    if (gameRefs.openMenu) {
        gameRefs.openMenu(args[1]);
        appendOutput("Opened menu: " + args[1]);
    } else {
        appendOutput("UI system not connected");
    }
}

void GameConsole::cmdCloseMenu(const std::vector<std::string>& args) {
    if (gameRefs.closeMenu) {
        gameRefs.closeMenu();
        appendOutput("Menu closed");
    } else {
        appendOutput("UI system not connected");
    }
}

void GameConsole::cmdDebugMenu(const std::vector<std::string>& args) {
    if (gameRefs.toggleDebugMenu) {
        gameRefs.toggleDebugMenu();
        appendOutput("Debug menu toggled");
    } else {
        appendOutput("Debug menu not connected");
    }
}
