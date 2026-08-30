#include "debug_menu.h"
#include "game_console.h"
#include "text_renderer.h"
#include <GLES3/gl3.h>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG_DEBUG "DebugMenu"
#define LOGI_DEBUG(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_DEBUG, __VA_ARGS__)
#define LOGE_DEBUG(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_DEBUG, __VA_ARGS__)

DebugMenu::DebugMenu()
    : textRenderer(nullptr), console(nullptr),
      visible(false), initialized(false),
      screenWidth(1080), screenHeight(1920),
      currentTab(Tab::PLAYER), tabScrollOffset(0.0f) {
}

DebugMenu::~DebugMenu() {
    cleanup();
}

bool DebugMenu::initialize(TextRenderer* tr, GameConsole* c) {
    if (initialized) return true;
    textRenderer = tr;
    console = c;

    createTabButtons();
    createPlayerButtons();
    createCombatButtons();
    createInventoryButtons();
    createMagicButtons();
    createQuestButtons();
    createNpcButtons();
    createDialogueButtons();
    createWorldButtons();
    createSaveButtons();

    initialized = true;
    LOGI_DEBUG("DebugMenu initialized");
    return true;
}

void DebugMenu::cleanup() {
    tabButtons.clear();
    tabContents.clear();
    initialized = false;
}

void DebugMenu::toggle() {
    visible = !visible;
    LOGI_DEBUG("DebugMenu %s", visible ? "opened" : "closed");
}

void DebugMenu::onTouchEvent(float x, float y, int action) {
    if (!visible) return;
    if (action == 0) { // DOWN
        handleTabTap(x, y);
        handleContentTap(x, y);
    }
}

void DebugMenu::update(float deltaTime) {
    if (!visible) return;
}

void DebugMenu::render() {
    if (!visible || !textRenderer) return;

    float scale = getScale();

    // Background overlay
    glClearColor(0.0f, 0.0f, 0.0f, 0.75f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Tab bar at top
    renderTabBar();

    // Content area below tabs
    renderContent();
}

void DebugMenu::renderTabBar() {
    float scale = getScale();
    float tabH = TAB_HEIGHT * scale;
    float margin = BUTTON_MARGIN * scale;
    float tabW = (static_cast<float>(screenWidth) - margin * (static_cast<float>(tabButtons.size()) + 1)) / static_cast<float>(tabButtons.size());

    // Reposition tab buttons
    for (size_t i = 0; i < tabButtons.size(); ++i) {
        tabButtons[i].x = margin + (tabW + margin) * static_cast<float>(i);
        tabButtons[i].y = margin;
        tabButtons[i].w = tabW;
        tabButtons[i].h = tabH;

        // Background color (active tab brighter)
        glm::vec3 color = (i == static_cast<size_t>(currentTab)) ?
            glm::vec3(0.4f, 0.7f, 0.4f) : glm::vec3(0.2f, 0.2f, 0.3f);

        // Draw button background as text highlight
        std::string label = (i == static_cast<size_t>(currentTab)) ? "[" + tabButtons[i].label + "]" : tabButtons[i].label;
        textRenderer->renderText(label.c_str(),
                                  tabButtons[i].x + 5.0f * scale,
                                  tabButtons[i].y + tabH * 0.3f,
                                  color, 0.4f * scale);
    }
}

void DebugMenu::renderContent() {
    float scale = getScale();
    float tabH = TAB_HEIGHT * scale;
    float margin = BUTTON_MARGIN * scale;
    float contentY = tabH + margin * 2.0f;
    float contentH = static_cast<float>(screenHeight) - contentY - margin;

    size_t tabIdx = static_cast<size_t>(currentTab);
    if (tabIdx >= tabContents.size()) return;
    TabContent& content = tabContents[tabIdx];

    float btnH = BUTTON_HEIGHT * scale;
    float btnW = (static_cast<float>(screenWidth) - margin * 3.0f) * 0.5f;

    // Two-column layout
    for (size_t i = 0; i < content.buttons.size(); ++i) {
        int col = static_cast<int>(i % 2);
        int row = static_cast<int>(i / 2);
        float bx = margin + (btnW + margin) * static_cast<float>(col);
        float by = contentY + (btnH + margin) * static_cast<float>(row) - content.scrollOffset;

        // Skip if outside visible area
        if (by + btnH < contentY || by > static_cast<float>(screenHeight)) continue;

        content.buttons[i].x = bx;
        content.buttons[i].y = by;
        content.buttons[i].w = btnW;
        content.buttons[i].h = btnH;

        glm::vec3 color = content.buttons[i].pressed ?
            glm::vec3(0.3f, 0.6f, 0.3f) : glm::vec3(0.4f, 0.5f, 0.6f);
        textRenderer->renderText(content.buttons[i].label.c_str(),
                                  bx + 8.0f * scale,
                                  by + btnH * 0.3f,
                                  color, 0.45f * scale);
    }
}

bool DebugMenu::hitTest(const Button& btn, float x, float y) const {
    return x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h;
}

void DebugMenu::handleTabTap(float x, float y) {
    for (size_t i = 0; i < tabButtons.size(); ++i) {
        if (hitTest(tabButtons[i], x, y)) {
            currentTab = static_cast<Tab>(i);
            LOGI_DEBUG("Tab changed to %zu", i);
            return;
        }
    }
}

void DebugMenu::handleContentTap(float x, float y) {
    size_t tabIdx = static_cast<size_t>(currentTab);
    if (tabIdx >= tabContents.size()) return;

    for (auto& btn : tabContents[tabIdx].buttons) {
        if (hitTest(btn, x, y)) {
            if (console && !btn.command.empty()) {
                LOGI_DEBUG("Executing: %s", btn.command.c_str());
                console->executeCommand(btn.command);
            }
            return;
        }
    }
}

float DebugMenu::getScale() const {
    float minDim = static_cast<float>(std::min(screenWidth, screenHeight));
    float scale = minDim / 1080.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;
    return scale;
}

std::string DebugMenu::getTabName(Tab tab) const {
    switch (tab) {
        case Tab::PLAYER: return "Player";
        case Tab::COMBAT: return "Combat";
        case Tab::INVENTORY: return "Items";
        case Tab::MAGIC: return "Magic";
        case Tab::QUEST: return "Quest";
        case Tab::NPC: return "NPC";
        case Tab::DIALOGUE: return "Talk";
        case Tab::WORLD: return "World";
        case Tab::SAVE: return "Save";
        default: return "?";
    }
}

// ============= Tab creation =============

void DebugMenu::createTabButtons() {
    for (int i = 0; i < static_cast<int>(Tab::COUNT); ++i) {
        Button btn;
        btn.label = getTabName(static_cast<Tab>(i));
        btn.command = "";
        btn.color = glm::vec3(0.4f, 0.7f, 0.4f);
        btn.pressed = false;
        tabButtons.push_back(btn);
    }
}

void DebugMenu::createPlayerButtons() {
    TabContent content;
    std::vector<std::pair<std::string, std::string>> items = {
        {"Heal", "heal"},
        {"God Mode", "god"},
        {"Set HP 100", "sethealth 100"},
        {"Set MP 100", "setmana 100"},
        {"Set Stamina 100", "setstamina 100"},
        {"Set Level 50", "setlevel 50"},
        {"Add XP 1000", "addxp 1000"},
        {"Max Skills", "maxskills"},
        {"Reset Stats", "resetstats"},
        {"Set Blade 100", "setskill Blade 100"},
        {"Set Dest 100", "setskill Destruction 100"},
        {"Set Speed 100", "setattr Speed 100"},
        {"Show Stats", "stats"},
    };
    for (const auto& item : items) {
        Button btn;
        btn.label = item.first;
        btn.command = item.second;
        btn.color = glm::vec3(0.3f, 0.5f, 0.7f);
        btn.pressed = false;
        content.buttons.push_back(btn);
    }
    tabContents.push_back(content);
}

void DebugMenu::createCombatButtons() {
    TabContent content;
    std::vector<std::pair<std::string, std::string>> items = {
        {"Attack", "attack"},
        {"Block", "block"},
        {"Dodge", "dodge"},
        {"Kill Nearest", "kill"},
        {"Kill All", "killall"},
        {"Resurrect", "resurrect"},
        {"Damage 10", "damage 0 10"},
        {"Damage 100", "damage 0 100"},
        {"Combat Debug", "combatdebug"},
    };
    for (const auto& item : items) {
        Button btn;
        btn.label = item.first;
        btn.command = item.second;
        btn.color = glm::vec3(0.7f, 0.3f, 0.3f);
        btn.pressed = false;
        content.buttons.push_back(btn);
    }
    tabContents.push_back(content);
}

void DebugMenu::createInventoryButtons() {
    TabContent content;
    std::vector<std::pair<std::string, std::string>> items = {
        {"Add Gold x100", "additem 0 100"},
        {"Add Apple x5", "additem 1 5"},
        {"Add Health Pot", "additem 10 5"},
        {"Add Magicka Pot", "additem 11 5"},
        {"Remove Apple", "removeitem 1 1"},
        {"Clear Inv", "clearinv"},
        {"List Items", "listitems"},
        {"Max Weight", "setweight 9999"},
    };
    for (const auto& item : items) {
        Button btn;
        btn.label = item.first;
        btn.command = item.second;
        btn.color = glm::vec3(0.6f, 0.5f, 0.2f);
        btn.pressed = false;
        content.buttons.push_back(btn);
    }
    tabContents.push_back(content);
}

void DebugMenu::createMagicButtons() {
    TabContent content;
    std::vector<std::pair<std::string, std::string>> items = {
        {"Learn Fire", "learnspell 1"},
        {"Learn Heal", "learnspell 2"},
        {"Learn Light", "learnspell 3"},
        {"Equip Fire", "equipspell 1"},
        {"Cast Fire", "castspell 1 0"},
        {"Cast Heal", "castspell 2 0"},
        {"List Spells", "listspells"},
        {"Set MP 100", "setmana 100"},
    };
    for (const auto& item : items) {
        Button btn;
        btn.label = item.first;
        btn.command = item.second;
        btn.color = glm::vec3(0.5f, 0.2f, 0.7f);
        btn.pressed = false;
        content.buttons.push_back(btn);
    }
    tabContents.push_back(content);
}

void DebugMenu::createQuestButtons() {
    TabContent content;
    std::vector<std::pair<std::string, std::string>> items = {
        {"List Quests", "listquests"},
        {"Accept Main", "acceptquest 1"},
        {"Accept Side", "acceptquest 2"},
        {"Complete Q1", "completequest 1"},
        {"Fail Q1", "failquest 1"},
        {"Update Obj", "updateobj 1 1 5"},
    };
    for (const auto& item : items) {
        Button btn;
        btn.label = item.first;
        btn.command = item.second;
        btn.color = glm::vec3(0.7f, 0.5f, 0.2f);
        btn.pressed = false;
        content.buttons.push_back(btn);
    }
    tabContents.push_back(content);
}

void DebugMenu::createNpcButtons() {
    TabContent content;
    std::vector<std::pair<std::string, std::string>> items = {
        {"List NPCs", "listnpcs"},
        {"Nearby", "nearby"},
        {"Spawn Guard", "spawnat Guard 0 0 0"},
        {"Spawn Mage", "spawnat Mage 5 0 5"},
        {"Spawn Bandit", "spawnat Bandit -5 0 5"},
        {"Aggro NPC", "aggro 0"},
        {"Calm NPC", "calm 0"},
        {"Set AI Combat", "setai 0 combat"},
        {"Set AI Idle", "setai 0 idle"},
        {"Resurrect", "resurrectnpc 0"},
    };
    for (const auto& item : items) {
        Button btn;
        btn.label = item.first;
        btn.command = item.second;
        btn.color = glm::vec3(0.4f, 0.5f, 0.3f);
        btn.pressed = false;
        content.buttons.push_back(btn);
    }
    tabContents.push_back(content);
}

void DebugMenu::createDialogueButtons() {
    TabContent content;
    std::vector<std::pair<std::string, std::string>> items = {
        {"Talk NPC", "talk 0"},
        {"Topic 0", "selecttopic 0"},
        {"Topic 1", "selecttopic 1"},
        {"Topic 2", "selecttopic 2"},
        {"Choice 0", "selectchoice 0"},
        {"Choice 1", "selectchoice 1"},
        {"End Talk", "endtalk"},
    };
    for (const auto& item : items) {
        Button btn;
        btn.label = item.first;
        btn.command = item.second;
        btn.color = glm::vec3(0.6f, 0.4f, 0.6f);
        btn.pressed = false;
        content.buttons.push_back(btn);
    }
    tabContents.push_back(content);
}

void DebugMenu::createWorldButtons() {
    TabContent content;
    std::vector<std::pair<std::string, std::string>> items = {
        {"Weather Clear", "setweather clear"},
        {"Weather Rain", "setweather rain"},
        {"Weather Snow", "setweather snow"},
        {"Weather Fog", "setweather fog"},
        {"Weather Storm", "setweather storm"},
        {"Time Dawn (6)", "settime 6"},
        {"Time Noon (12)", "settime 12"},
        {"Time Dusk (18)", "settime 18"},
        {"Time Midnight", "settime 0"},
        {"Time x30", "settimescale 30"},
        {"Time x1", "settimescale 1"},
        {"Time x0 (Pause)", "settimescale 0"},
        {"Load Cell 0,0", "loadcell 0 0"},
        {"World Info", "worldinfo"},
    };
    for (const auto& item : items) {
        Button btn;
        btn.label = item.first;
        btn.command = item.second;
        btn.color = glm::vec3(0.3f, 0.6f, 0.6f);
        btn.pressed = false;
        content.buttons.push_back(btn);
    }
    tabContents.push_back(content);
}

void DebugMenu::createSaveButtons() {
    TabContent content;
    std::vector<std::pair<std::string, std::string>> items = {
        {"Quick Save", "quicksave"},
        {"Quick Load", "quickload"},
        {"Save Slot 0", "save 0"},
        {"Save Slot 1", "save 1"},
        {"Save Slot 2", "save 2"},
        {"Load Slot 0", "load 0"},
        {"Load Slot 1", "load 1"},
        {"Load Slot 2", "load 2"},
        {"List Saves", "listsaves"},
    };
    for (const auto& item : items) {
        Button btn;
        btn.label = item.first;
        btn.command = item.second;
        btn.color = glm::vec3(0.5f, 0.5f, 0.3f);
        btn.pressed = false;
        content.buttons.push_back(btn);
    }
    tabContents.push_back(content);
}