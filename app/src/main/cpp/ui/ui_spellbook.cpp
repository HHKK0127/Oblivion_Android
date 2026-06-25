#include "ui_spellbook.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cstdio>
#include <algorithm>

UISpellbook::UISpellbook(const std::string& title)
    : UIPanel(title) {
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
        PlaceholderAssets::Colors::BROWN_ACCENT.r * 0.8f,
        PlaceholderAssets::Colors::BROWN_ACCENT.g * 0.6f,
        PlaceholderAssets::Colors::BROWN_ACCENT.b * 0.8f, 0.95f));
    setTitle(title);
    setCloseButtonVisible(true);
    setDraggable(true);
}

bool UISpellbook::initialize(CharacterStatus* ps, TextRenderer* tr) {
    if (!ps || !tr) return false;
    characterStatus = ps;
    textRenderer = tr;
    return UIPanel::initialize();
}

void UISpellbook::setAvailableSpells(const std::vector<std::shared_ptr<Spell>>& spells) {
    allSpells = spells;
    rebuildFilter();
}

void UISpellbook::setSchoolFilter(int school) {
    schoolFilter = school;
    selectedSpellIndex = -1;
    scrollOffset = 0;
    rebuildFilter();
}

void UISpellbook::rebuildFilter() {
    filteredSpells.clear();
    for (const auto& sp : allSpells) {
        if (schoolFilter < 0 || static_cast<int>(sp->school) == schoolFilter) {
            filteredSpells.push_back(sp);
        }
    }
}

void UISpellbook::update(float deltaTime) {
    UIPanel::update(deltaTime);
}

bool UISpellbook::onTouchDown(float x, float y, int pointerId) {
    if (!isVisible() || !isEnabled()) return false;

    if (isInsideCloseButton(x, y) || isInsideTitleBar(x, y))
        return UIPanel::onTouchDown(x, y, pointerId);

    // School filter tab
    int tabIdx = hitTestSchoolTab(x, y);
    if (tabIdx >= -1) {   // includes -1 for "All"
        // hitTestSchoolTab returns -2 if miss
        if (tabIdx != -2) {
            setSchoolFilter(tabIdx);
            return true;
        }
    }

    // Scroll buttons
    if (hitTestScrollUp(x, y)) {
        if (scrollOffset > 0) --scrollOffset;
        return true;
    }
    if (hitTestScrollDown(x, y)) {
        int maxOffset = std::max(0, (int)filteredSpells.size() - MAX_VISIBLE_ROWS);
        if (scrollOffset < maxOffset) ++scrollOffset;
        return true;
    }

    // Spell row
    int row = hitTestSpellRow(x, y);
    if (row >= 0) {
        selectedSpellIndex = scrollOffset + row;
        return true;
    }

    return UIPanel::onTouchDown(x, y, pointerId);
}

void UISpellbook::render() {
    if (!isVisible()) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    UIPanel::render();

    renderSchoolTabs();
    renderSpellList();
    renderScrollButtons();
    if (selectedSpellIndex >= 0 &&
        selectedSpellIndex < static_cast<int>(filteredSpells.size())) {
        renderSpellDetail();
    }

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

// ─── tab bar ──────────────────────────────────────────────────────────────────

void UISpellbook::renderSchoolTabs() {
    glm::vec2 cp = getContentPosition();
    // "All" + 6 schools = 7 tabs
    const float tabW = 56.0f;
    const float tabH = 24.0f;
    const float gap  = 3.0f;
    float bx = cp.x;
    float by = cp.y;

    for (int i = -1; i <= 5; ++i) {
        glm::vec3 bg = (i == schoolFilter)
            ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK)
            : glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.85f;
        glm::vec3 border = (i == schoolFilter)
            ? schoolColor(static_cast<MagicSchool>(std::max(i, 0)))
            : glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT);

        PlaceholderAssets::drawPanel(bx, by, tabW, tabH, bg, border);
        if (textRenderer)
            textRenderer->renderText(schoolLabel(i), bx + 3.0f, by + 4.0f,
                glm::vec3(0.1f, 0.1f, 0.1f), 0.55f);
        bx += tabW + gap;
    }
}

// ─── spell list ───────────────────────────────────────────────────────────────

void UISpellbook::renderSpellList() {
    glm::vec2 cp = getContentPosition();
    float x = cp.x;
    float y = cp.y + 30.0f;  // below school tabs
    float rowW = getContentSize().x - 160.0f;  // leave room for detail panel

    int count = static_cast<int>(filteredSpells.size());
    int visEnd = std::min(scrollOffset + MAX_VISIBLE_ROWS, count);

    for (int i = scrollOffset; i < visEnd; ++i) {
        const auto& sp = filteredSpells[i];
        bool selected = (i == selectedSpellIndex);

        glm::vec3 bg = selected
            ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK)
            : glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.9f;
        glm::vec3 border = selected
            ? glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT)
            : glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT) * 0.7f;

        float ry = y + (i - scrollOffset) * (ROW_H + ROW_GAP);
        PlaceholderAssets::drawPanel(x, ry, rowW, ROW_H, bg, border);

        // School color strip on left
        PlaceholderAssets::drawSolidRect(x, ry, 6.0f, ROW_H,
            schoolColor(sp->school), 1.0f);

        if (textRenderer) {
            // Spell name
            textRenderer->renderText(sp->name,
                x + 12.0f, ry + 4.0f, glm::vec3(0.1f, 0.1f, 0.1f), 0.8f);

            // School name (small)
            textRenderer->renderText(sp->getSchoolName(),
                x + 12.0f, ry + 22.0f,
                glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT), 0.6f);

            // Mana cost (right-aligned)
            char buf[16];
            snprintf(buf, sizeof(buf), "%.0f MP", sp->manaCost);
            bool affordable = characterStatus &&
                              characterStatus->currentMana >= sp->manaCost;
            glm::vec3 costCol = affordable
                ? glm::vec3(PlaceholderAssets::Colors::BLUE_MANA)
                : glm::vec3(0.7f, 0.2f, 0.2f);
            textRenderer->renderText(buf,
                x + rowW - 56.0f, ry + 12.0f, costCol, 0.65f);
        }
    }

    // Empty state
    if (filteredSpells.empty() && textRenderer) {
        PlaceholderAssets::drawPanel(x, y, rowW, 50.0f,
            glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.95f,
            PlaceholderAssets::Colors::BROWN_ACCENT);
        textRenderer->renderText("No spells known",
            x + 10.0f, y + 16.0f, glm::vec3(0.4f, 0.3f, 0.2f), 0.8f);
    }
}

// ─── detail panel ─────────────────────────────────────────────────────────────

void UISpellbook::renderSpellDetail() {
    if (selectedSpellIndex < 0 ||
        selectedSpellIndex >= static_cast<int>(filteredSpells.size())) return;

    const auto& sp = filteredSpells[selectedSpellIndex];
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();

    float detailW = 150.0f;
    float detailX = cp.x + cs.x - detailW - 2.0f;
    float detailY = cp.y + 30.0f;
    float detailH = cs.y - 30.0f;

    // Panel background
    PlaceholderAssets::drawPanel(detailX, detailY, detailW, detailH,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT),
        schoolColor(sp->school));

    if (!textRenderer) return;

    float tx = detailX + 6.0f;
    float ty = detailY + 6.0f;

    // School colour header bar
    PlaceholderAssets::drawPanel(detailX, detailY, detailW, 18.0f,
        schoolColor(sp->school), PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText(sp->getSchoolName(),
        tx, ty, glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.65f);
    ty += 22.0f;

    // Spell name
    textRenderer->renderText(sp->name, tx, ty, glm::vec3(0.1f, 0.1f, 0.1f), 0.75f);
    ty += 18.0f;
    if (!sp->nameJa.empty())
        textRenderer->renderText(sp->nameJa, tx, ty,
            glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT), 0.6f);
    ty += 16.0f;

    // Divider
    UIDrawHelper::drawColoredQuad(detailX + 4.0f, ty, detailW - 8.0f, 1.0f,
        glm::vec4(PlaceholderAssets::Colors::BROWN_ACCENT, 1.0f),
        screenWidth, screenHeight);
    ty += 6.0f;

    // Mana cost
    char buf[32];
    snprintf(buf, sizeof(buf), "Cost: %.0f MP", sp->manaCost);
    bool affordable = characterStatus && characterStatus->currentMana >= sp->manaCost;
    textRenderer->renderText(buf, tx, ty,
        affordable ? glm::vec3(PlaceholderAssets::Colors::BLUE_MANA)
                   : glm::vec3(0.8f, 0.2f, 0.2f), 0.65f);
    ty += 14.0f;

    // Base damage
    if (sp->baseDamage > 0.0f) {
        snprintf(buf, sizeof(buf), "Dmg: %.0f", sp->baseDamage);
        textRenderer->renderText(buf, tx, ty,
            glm::vec3(PlaceholderAssets::Colors::RED_HEALTH), 0.65f);
        ty += 14.0f;
    }

    // Effects list
    ty += 4.0f;
    for (const auto& eff : sp->effects) {
        const char* effName = "Unknown";
        switch (eff.type) {
            case SpellEffectType::DAMAGE:          effName = "Damage"; break;
            case SpellEffectType::HEAL:             effName = "Heal"; break;
            case SpellEffectType::RESTORE_MANA:    effName = "Restore MP"; break;
            case SpellEffectType::RESTORE_STAMINA: effName = "Restore ST"; break;
            case SpellEffectType::FORTIFY_ATTR:    effName = "Fortify"; break;
            case SpellEffectType::PARALYZE:        effName = "Paralyze"; break;
            case SpellEffectType::INVISIBILITY:    effName = "Invisible"; break;
            case SpellEffectType::SUMMON:          effName = "Summon"; break;
        }
        snprintf(buf, sizeof(buf), "%s %.0f", effName, eff.magnitude);
        textRenderer->renderText(buf, tx, ty,
            glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT), 0.6f);
        ty += 12.0f;
    }
}

// ─── scroll buttons ───────────────────────────────────────────────────────────

void UISpellbook::renderScrollButtons() {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float rowW = cs.x - 160.0f;

    // ▲ Up
    float upX = cp.x + rowW + 4.0f;
    float upY = cp.y + 30.0f;
    PlaceholderAssets::drawPanel(upX, upY, 20.0f, 20.0f,
        PlaceholderAssets::Colors::PARCHMENT_DARK,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    if (textRenderer)
        textRenderer->renderText("^", upX + 6.0f, upY + 2.0f,
            glm::vec3(0.1f, 0.1f, 0.1f), 0.7f);

    // ▼ Down
    float dnY = upY + 26.0f;
    PlaceholderAssets::drawPanel(upX, dnY, 20.0f, 20.0f,
        PlaceholderAssets::Colors::PARCHMENT_DARK,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    if (textRenderer)
        textRenderer->renderText("v", upX + 6.0f, dnY + 2.0f,
            glm::vec3(0.1f, 0.1f, 0.1f), 0.7f);
}

// ─── hit testing ──────────────────────────────────────────────────────────────

int UISpellbook::hitTestSchoolTab(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    const float tabW = 56.0f;
    const float tabH = 24.0f;
    const float gap  = 3.0f;

    for (int i = -1; i <= 5; ++i) {
        float bx = cp.x + (i + 1) * (tabW + gap);
        if (x >= bx && x <= bx + tabW && y >= cp.y && y <= cp.y + tabH)
            return i;
    }
    return -2;  // miss
}

int UISpellbook::hitTestSpellRow(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float rowW = getContentSize().x - 160.0f;
    float startY = cp.y + 30.0f;

    for (int i = 0; i < MAX_VISIBLE_ROWS; ++i) {
        float ry = startY + i * (ROW_H + ROW_GAP);
        if (x >= cp.x && x <= cp.x + rowW &&
            y >= ry && y <= ry + ROW_H)
            return i;
    }
    return -1;
}

bool UISpellbook::hitTestScrollUp(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float rowW = cs.x - 160.0f;
    float upX = cp.x + rowW + 4.0f;
    float upY = cp.y + 30.0f;
    return x >= upX && x <= upX + 20.0f && y >= upY && y <= upY + 20.0f;
}

bool UISpellbook::hitTestScrollDown(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float rowW = cs.x - 160.0f;
    float upX = cp.x + rowW + 4.0f;
    float dnY = cp.y + 30.0f + 26.0f;
    return x >= upX && x <= upX + 20.0f && y >= dnY && y <= dnY + 20.0f;
}

// ─── helpers ──────────────────────────────────────────────────────────────────

glm::vec3 UISpellbook::schoolColor(MagicSchool school) const {
    switch (school) {
        case MagicSchool::DESTRUCTION:  return glm::vec3(0.80f, 0.20f, 0.15f);
        case MagicSchool::RESTORATION:  return glm::vec3(0.20f, 0.65f, 0.25f);
        case MagicSchool::CONJURATION:  return glm::vec3(0.50f, 0.20f, 0.70f);
        case MagicSchool::ILLUSION:     return glm::vec3(0.15f, 0.45f, 0.75f);
        case MagicSchool::ALTERATION:   return glm::vec3(0.70f, 0.55f, 0.15f);
        case MagicSchool::MYSTICISM:    return glm::vec3(0.30f, 0.55f, 0.65f);
        default:                        return glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK);
    }
}

const char* UISpellbook::schoolLabel(int school) const {
    if (school < 0) return "All";
    switch (static_cast<MagicSchool>(school)) {
        case MagicSchool::ALTERATION:  return "Alter";
        case MagicSchool::CONJURATION: return "Conj";
        case MagicSchool::DESTRUCTION: return "Destr";
        case MagicSchool::ILLUSION:    return "Illus";
        case MagicSchool::MYSTICISM:   return "Myst";
        case MagicSchool::RESTORATION: return "Rest";
        default: return "?";
    }
}
