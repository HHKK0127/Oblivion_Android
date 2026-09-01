#pragma once

#include "ui_panel.h"
#include "../game/spell.h"
#include "../game/npc.h"
#include "text_renderer.h"
#include <vector>
#include <memory>

/**
 * @brief Spellbook UI
 *
 * Phase 10: Spell list and equipment management UI for learned spells
 * Displays by Oblivion magic school category, mana cost, and effects
 */
class UISpellbook : public UIPanel {
public:
    explicit UISpellbook(const std::string& title = "Spellbook");
    ~UISpellbook() override = default;

    // Initialize
    bool initialize(CharacterStatus* playerStatus, TextRenderer* textRenderer);

    // Spell data — caller pushes available spell definitions
    void setAvailableSpells(const std::vector<std::shared_ptr<Spell>>& spells);

    // Lifecycle
    void update(float deltaTime) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    void render() override;

    // Screen size
    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }

    // Filter by school (MagicSchool::ALTERATION … or -1 for all)
    void setSchoolFilter(int school);  // -1 = show all

private:
    CharacterStatus* characterStatus = nullptr;
    TextRenderer* textRenderer = nullptr;
    std::vector<std::shared_ptr<Spell>> allSpells;
    std::vector<std::shared_ptr<Spell>> filteredSpells;

    int selectedSpellIndex = -1;
    int schoolFilter = -1;  // -1 = all schools
    int scrollOffset = 0;

    int screenWidth  = 1080;
    int screenHeight = 1920;

    static constexpr float ROW_H   = 44.0f;
    static constexpr float ROW_GAP = 4.0f;
    static constexpr int   MAX_VISIBLE_ROWS = 8;

    void rebuildFilter();
    void renderSchoolTabs();
    void renderSpellList();
    void renderSpellDetail();
    void renderScrollButtons();

    int hitTestSchoolTab(float x, float y) const;
    int hitTestSpellRow(float x, float y) const;
    bool hitTestScrollUp(float x, float y) const;
    bool hitTestScrollDown(float x, float y) const;

    glm::vec3 schoolColor(MagicSchool school) const;
    const char* schoolLabel(int school) const;  // -1 → "All"
};
