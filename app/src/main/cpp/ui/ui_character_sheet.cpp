#include "ui_character_sheet.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cstdio>
#include <cmath>
#include <algorithm>

UICharacterSheet::UICharacterSheet(const std::string& title)
    : UIPanel(title) {
    // Oblivion parchment styling
    setBackgroundColor(glm::vec4(
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.r,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.g,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.b, 0.96f));
    setBorderColor(glm::vec4(
        PlaceholderAssets::Colors::BROWN_ACCENT.r,
        PlaceholderAssets::Colors::BROWN_ACCENT.g,
        PlaceholderAssets::Colors::BROWN_ACCENT.b, 1.0f));
    setBorderWidth(2.0f);
    setTitleBarColor(glm::vec4(
        PlaceholderAssets::Colors::BROWN_ACCENT.r,
        PlaceholderAssets::Colors::BROWN_ACCENT.g,
        PlaceholderAssets::Colors::BROWN_ACCENT.b, 0.95f));
    setTitle(title);
    setCloseButtonVisible(true);
    setDraggable(true);
}

bool UICharacterSheet::initialize(CharacterStatus* playerStatus, TextRenderer* tr) {
    if (!playerStatus || !tr) return false;
    characterStatus = playerStatus;
    textRenderer = tr;
    if (!UIPanel::initialize()) return false;
    return true;
}

void UICharacterSheet::setCharacter(CharacterStatus* charStatus) {
    characterStatus = charStatus;
}

void UICharacterSheet::setActiveTab(CharacterTab tab) {
    activeTab = tab;
}

void UICharacterSheet::update(float deltaTime) {
    UIPanel::update(deltaTime);
}

bool UICharacterSheet::onTouchDown(float x, float y, int pointerId) {
    if (!isVisible() || !isEnabled()) return false;

    // Title bar / close button
    if (isInsideCloseButton(x, y) || isInsideTitleBar(x, y)) {
        return UIPanel::onTouchDown(x, y, pointerId);
    }

    // Tab button hit test
    int tab = hitTestTabButton(x, y);
    if (tab >= 0) {
        setActiveTab(static_cast<CharacterTab>(tab));
        return true;
    }

    return UIPanel::onTouchDown(x, y, pointerId);
}

void UICharacterSheet::render() {
    if (!isVisible()) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    UIPanel::render();

    renderTabButtons();

    if (characterStatus) {
        switch (activeTab) {
            case CharacterTab::ATTRIBUTES: renderAttributesTab(); break;
            case CharacterTab::SKILLS:     renderSkillsTab();     break;
            case CharacterTab::EFFECTS:    renderEffectsTab();    break;
        }
    }

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

// ─── private rendering ────────────────────────────────────────────────────────

void UICharacterSheet::renderTabButtons() {
    glm::vec2 cp = getContentPosition();

    const char* tabLabels[] = { "Attributes", "Skills", "Effects" };
    const float tabW = 100.0f;
    const float tabH = 26.0f;
    const float gap  = 4.0f;

    for (int i = 0; i < 3; ++i) {
        float bx = cp.x + i * (tabW + gap);
        float by = cp.y;

        glm::vec3 bg = (i == static_cast<int>(activeTab))
            ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK)
            : glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.9f;
        glm::vec3 border = (i == static_cast<int>(activeTab))
            ? glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT)
            : glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT);

        PlaceholderAssets::drawPanel(bx, by, tabW, tabH, bg, border);

        if (textRenderer) {
            textRenderer->renderText(tabLabels[i], bx + 6.0f, by + 4.0f,
                glm::vec3(0.1f, 0.1f, 0.1f), 0.7f);
        }
    }
}

void UICharacterSheet::renderAttributesTab() {
    if (!characterStatus || !textRenderer) return;

    glm::vec2 cp = getContentPosition();
    float x  = cp.x + 8.0f;
    float y  = cp.y + 34.0f;  // below tabs

    // Vitals section header
    PlaceholderAssets::drawPanel(x, y, 280.0f, 18.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("VITALS", x + 6.0f, y + 2.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.7f);
    y += 22.0f;

    // HP / MP / Stamina bars
    renderStatDisplay(x, y, "Health",  characterStatus->currentHealth, characterStatus->maxHealth);  y += 28.0f;
    renderStatDisplay(x, y, "Mana",    characterStatus->currentMana,   characterStatus->maxMana);    y += 28.0f;
    renderStatDisplay(x, y, "Stamina", characterStatus->stamina,        characterStatus->maxStamina); y += 36.0f;

    // Attributes section header
    PlaceholderAssets::drawPanel(x, y, 280.0f, 18.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("ATTRIBUTES", x + 6.0f, y + 2.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.7f);
    y += 22.0f;

    // Oblivion 8 core attributes in order
    const char* attrOrder[] = {
        "Strength","Intelligence","Willpower",
        "Agility","Speed","Endurance","Personality","Luck"
    };
    for (const char* attr : attrOrder) {
        auto it = characterStatus->attributes.find(attr);
        float val = (it != characterStatus->attributes.end()) ? it->second : 0.0f;
        renderAttributeRow(x, y, attr, val);
        y += 20.0f;
    }

    // Combat stats
    y += 6.0f;
    PlaceholderAssets::drawPanel(x, y, 280.0f, 18.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("COMBAT", x + 6.0f, y + 2.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.7f);
    y += 22.0f;

    renderAttributeRow(x, y, "Weapon Damage", characterStatus->weaponDamage); y += 20.0f;
    renderAttributeRow(x, y, "Armor Rating",  characterStatus->armorRating);
}

void UICharacterSheet::renderSkillsTab() {
    if (!characterStatus || !textRenderer) return;

    glm::vec2 cp = getContentPosition();
    float x = cp.x + 8.0f;
    float y = cp.y + 34.0f;

    // Section header
    PlaceholderAssets::drawPanel(x, y, 280.0f, 18.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("SKILLS", x + 6.0f, y + 2.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.7f);
    y += 22.0f;

    // Oblivion skill set
    const char* skillOrder[] = {
        "Blade","Blunt","Hand-to-Hand","Block","Armorer","Athletics",
        "Heavy Armor","Destruction","Conjuration","Mysticism",
        "Restoration","Illusion","Alteration","Alchemy",
        "Light Armor","Marksman","Mercantile","Security","Sneak","Acrobatics"
    };

    for (const char* skill : skillOrder) {
        auto it = characterStatus->skills.find(skill);
        float val = (it != characterStatus->skills.end()) ? it->second : 0.0f;
        renderSkillRow(x, y, skill, val, 100.0f);
        y += 18.0f;
    }
}

void UICharacterSheet::renderEffectsTab() {
    glm::vec2 cp = getContentPosition();
    float x = cp.x + 8.0f;
    float y = cp.y + 34.0f;

    PlaceholderAssets::drawPanel(x, y, 280.0f, 18.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    if (textRenderer)
        textRenderer->renderText("ACTIVE EFFECTS", x + 6.0f, y + 2.0f,
            glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.7f);
    y += 28.0f;

    // Placeholder: no active effects system yet
    PlaceholderAssets::drawPanel(x, y, 280.0f, 40.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.95f,
        PlaceholderAssets::Colors::BROWN_ACCENT);
    if (textRenderer)
        textRenderer->renderText("No active effects",
            x + 8.0f, y + 12.0f, glm::vec3(0.4f, 0.3f, 0.2f), 0.8f);
}

void UICharacterSheet::renderAttributeRow(float x, float y, const std::string& attrName, float value) {
    if (!textRenderer) return;

    PlaceholderAssets::drawPanel(x, y, 280.0f, 16.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.9f,
        PlaceholderAssets::Colors::BROWN_ACCENT * 0.7f);

    textRenderer->renderText(attrName, x + 4.0f, y + 2.0f,
        glm::vec3(0.1f, 0.1f, 0.1f), 0.65f);

    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f", value);
    textRenderer->renderText(buf, x + 240.0f, y + 2.0f,
        glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT), 0.65f);
}

void UICharacterSheet::renderSkillRow(float x, float y, const std::string& skillName,
                                      float value, float maxValue) {
    if (!textRenderer) return;

    PlaceholderAssets::drawPanel(x, y, 280.0f, 14.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.88f,
        PlaceholderAssets::Colors::BROWN_ACCENT * 0.5f);

    textRenderer->renderText(skillName, x + 4.0f, y + 1.0f,
        glm::vec3(0.1f, 0.1f, 0.1f), 0.6f);

    // Skill bar (right side, 60px wide)
    float barX = x + 210.0f;
    float fillRatio = (maxValue > 0.0f) ? (value / maxValue) : 0.0f;
    fillRatio = std::min(fillRatio, 1.0f);
    PlaceholderAssets::drawStatusBar(barX, y + 2.0f, 60.0f, 10.0f,
        fillRatio, PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
}

void UICharacterSheet::renderStatDisplay(float x, float y,
                                         const std::string& label,
                                         float current, float max) {
    if (!textRenderer) return;

    textRenderer->renderText(label, x, y,
        glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT), 0.7f);

    float barX = x + 70.0f;
    float ratio = (max > 0.0f) ? std::min(current / max, 1.0f) : 0.0f;

    glm::vec3 barColor;
    if (label == "Health")       barColor = PlaceholderAssets::Colors::RED_HEALTH;
    else if (label == "Mana")    barColor = PlaceholderAssets::Colors::BLUE_MANA;
    else                          barColor = PlaceholderAssets::Colors::GREEN_STAMINA;

    PlaceholderAssets::drawStatusBar(barX, y + 2.0f, 160.0f, 12.0f, ratio, barColor);

    char buf[32];
    snprintf(buf, sizeof(buf), "%.0f / %.0f", current, max);
    textRenderer->renderText(buf, barX + 165.0f, y + 2.0f,
        glm::vec3(0.2f, 0.2f, 0.2f), 0.6f);
}

int UICharacterSheet::hitTestTabButton(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    const float tabW = 100.0f;
    const float tabH = 26.0f;
    const float gap  = 4.0f;

    for (int i = 0; i < 3; ++i) {
        float bx = cp.x + i * (tabW + gap);
        float by = cp.y;
        if (x >= bx && x <= bx + tabW && y >= by && y <= by + tabH)
            return i;
    }
    return -1;
}
