#include "ui_inventory_panel.h"

#include "text_renderer.h"
#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"
#include "../inventory/item_factory.h"

#include <GLES3/gl3.h>
#include <android/log.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#undef LOG_TAG
#undef LOGD
#undef LOGW
#define LOG_TAG "UIInventoryPanel"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace ui {

namespace {

// ---------------------------------------------------------------------------
// Layout constants, straight from Data\Menus\Main\inventory_menu.xml.
//
// Every value is in the XML's own coordinate space: the menu canvas is 1280x960
// and widgets are laid out in raw pixels that map 1:1 onto texels (the 596x160
// border widget reads [0,596]x[0,160] of a 1024x256 DDS). layoutScale is applied
// once, at draw time, to the whole tree.
// ---------------------------------------------------------------------------

constexpr float kCanvasWidth = 1280.0f;
constexpr float kCanvasHeight = 960.0f;

// inv_background (shared_main_background_1.dds) - the menu sheet.
constexpr float kBackgroundWidth = 745.0f;
constexpr float kBackgroundHeight = 792.0f;

// inv_window / inv_pane.
constexpr float kWindowX = 40.0f;
constexpr float kWindowY = 90.0f;
constexpr float kWindowWidth = 645.0f;
constexpr float kWindowHeight = 486.0f;
constexpr float kPaneOffsetY = -8.0f;

// inv_scroll_bar: x=690, y=inv_window.y, height=inv_window.height.
constexpr float kScrollBarX = 690.0f;
constexpr float kScrollBarWidth = 20.0f;
constexpr float kScrollBarCapHeight = 32.0f;
constexpr float kScrollBarMarkerHeight = 42.0f;
constexpr float kScrollBarBottomCropY = 98.0f;

// Rows: inv_item is 81 tall and inv_scroll_bar.user8 says six fit at a time.
constexpr int kVisibleRows = 6;
constexpr float kRowHeight = 81.0f;

// In-list header (inv_*_header) and sort header (inv_headers_top) both sit at
// x=102 inside inv_window / inv_background, so both land 142 from the sheet.
constexpr float kHeaderX = kWindowX + 102.0f;
constexpr float kHeaderTextX = 130.0f;
constexpr float kHeaderTextY = 35.0f;
constexpr float kHeaderIconY = 30.0f;
constexpr float kHeaderDecorDy = 5.0f;
constexpr float kDecorWidth = 35.0f;
constexpr float kDecorHeight = 17.0f;

// Sort header only: inv_headers_top is at y=-50 and its children at y=100,
// which lands 5px higher than the in-list header decoration.
constexpr float kSortHeaderY = -50.0f + 100.0f;
constexpr float kSortTextY = 5.0f;
constexpr float kSortDecorDy = 6.0f;

// inv_item children (relative to inv_window's x, i.e. the row origin).
constexpr float kItemIconX = 5.0f;
constexpr float kItemIconY = 7.0f;
constexpr float kItemIconSize = 64.0f;
constexpr float kItemMarkerX = 7.0f;
constexpr float kItemMarkerY = 7.0f;
constexpr float kItemMarkerWidth = 635.0f;
constexpr float kItemMarkerHeight = 64.0f;
constexpr float kItemNameX = 80.0f;
constexpr float kItemCountX = 70.0f;
constexpr float kItemColumnX[4] = {418.0f, 491.0f, 551.0f, 612.0f};
constexpr float kHeaderIconX[4] = {300.0f, 375.0f, 435.0f, 495.0f};
constexpr float kColumnIconSize = 32.0f;

// inv_tabs / inv_tabs_pN.
constexpr float kTabsY = 594.0f;
constexpr float kTabY = 37.0f;
constexpr float kTabX[5] = {28.0f, 165.0f, 304.0f, 443.0f, 578.0f};
constexpr float kTabWidth[5] = {135.0f, 135.0f, 134.0f, 135.0f, 135.0f};
constexpr float kTabHeight = 85.0f;
constexpr float kTabIconX = 37.0f;
constexpr float kTabIconY = 8.0f;
constexpr float kTabIconSize = 64.0f;
constexpr float kTabPlateY = 24.0f;
constexpr float kTabPlateWidth = 136.0f;
constexpr float kTabPlateHeight = 36.0f;

// inv_extra_data: x = 640 - (460 - moneyTextWidth/2) + 40, y = 580, 640x32.
constexpr float kExtraDataWidth = 640.0f;
constexpr float kExtraDataY = 580.0f;
constexpr float kExtraDataHeight = 32.0f;
constexpr float kExtraTextX[3] = {40.0f, 250.0f, 460.0f};
constexpr float kExtraIconX[3] = {0.0f, 210.0f, 420.0f};
constexpr float kExtraIconCropX[3] = {32.0f, 128.0f, 224.0f};

// Menu pages, in tab order (inv_p1..inv_p5).
constexpr int kPageAll = 1;
constexpr int kPageWeapons = 2;
constexpr int kPageApparel = 3;
constexpr int kPageAlchemy = 4;
constexpr int kPageMisc = 5;

struct SectionDef {
    int headerIndex;   // 1..9, matches inv_*_header order and ItemSubcategory
    int columnCount;   // value columns shown for this section
    const char* label;
};

// Section order follows the in-list header declarations in the XML.
constexpr SectionDef kSections[9] = {
    {1, 4, "Weapons"},
    {2, 4, "Apparel"},
    {3, 3, "Apparatuses"},
    {4, 2, "Potions"},
    {5, 2, "Ingredients"},
    {6, 2, "Books"},
    {7, 2, "Keys"},
    {8, 2, "Stones"},
    {9, 2, "Misc"},
};

// Per-page section lists. The XML hides a section by giving it the same header
// position as the next one, so an empty section simply never appears.
constexpr int kPageSections[5][9] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8},  // 1: All
    {0, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 3, 4, -1, -1, -1, -1, -1, -1},
    {5, 6, 7, 8, -1, -1, -1, -1, -1},
};

// Column icons, carved from all_small_icons.dds by cropx. 0 = no icon column.
constexpr float kColumnIconCropX[9][4] = {
    {224.0f, 32.0f, 160.0f, 256.0f},  // 1 weapons:   value weight health damage
    {224.0f, 32.0f, 128.0f, 256.0f},  // 2 apparel:   value weight armor health
    {224.0f, 32.0f, 96.0f, 0.0f},     // 3 apparatus: value weight quality
    {224.0f, 32.0f, 0.0f, 0.0f},      // 4 potions
    {224.0f, 32.0f, 0.0f, 0.0f},      // 5 ingredients
    {224.0f, 32.0f, 0.0f, 0.0f},      // 6 books
    {224.0f, 32.0f, 0.0f, 0.0f},      // 7 keys
    {224.0f, 32.0f, 0.0f, 0.0f},      // 8 stones
    {224.0f, 32.0f, 0.0f, 0.0f},      // 9 misc
};

// Assets.
constexpr const char* kAssetBackground = "menus/shared__shared_main_background_1.png";
constexpr const char* kAssetBackground2 = "menus/shared__shared_main_background_2.png";
constexpr const char* kAssetPageTab[5] = {
    "menus/shared__shared_01_tab.png",
    "menus/shared__shared_02_tab.png",
    "menus/shared__shared_03_tab.png",
    "menus/shared__shared_04_tab.png",
    "menus/shared__shared_05_tab.png",
};
constexpr const char* kAssetBorderTop = "inventory/inv_border_top.png";
constexpr const char* kAssetBorderMiddle = "inventory/inv_border_middle.png";
constexpr const char* kAssetBorderBottom = "inventory/inv_border_bottom.png";
constexpr const char* kAssetTabIcons = "menus/shared__all_tabs_icons.png";
constexpr const char* kAssetTabSelectionOff = "menus/shared__general_tab_selection_off.png";
constexpr const char* kAssetTabSelectionOn = "menus/shared__general_tab_selection_on.png";
constexpr const char* kAssetSmallIcons = "menus/shared__all_small_icons.png";
constexpr const char* kAssetScrollBar = "menus/shared__all_general_scrollbar.png";
constexpr const char* kAssetDecorLeft = "menus/shared__shared_decor_blue_left.png";
constexpr const char* kAssetDecorRight = "menus/shared__shared_decor_blue_right.png";
constexpr const char* kAssetEquipMarker = "inventory/inv_equiped_marker_1.png";
constexpr const char* kAssetFocusTop = "menus/focus__focus_top.png";
constexpr const char* kAssetFocusBottom = "menus/focus__focus_bottom.png";
constexpr const char* kAssetFocusLeft = "menus/focus__focus_left.png";
constexpr const char* kAssetFocusRight = "menus/focus__focus_right.png";
constexpr const char* kAssetFocusCenter = "menus/focus__focus_center_x4.png";

// Text colours (117/59/33 for list text, 51/79/142 for the category title).
const glm::vec4 kRowTextColor(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 1.0f);
const glm::vec4 kHeaderTextColor(51.0f / 255.0f, 79.0f / 255.0f, 142.0f / 255.0f, 1.0f);
const glm::vec4 kWhite(1.0f, 1.0f, 1.0f, 1.0f);

// <font> 3 is TahomaBoldSmall; item names and the value columns leave <font>
// unspecified, which resolves to KingthingsRegular.
constexpr FontType kNameFont = FontType::KingthingsRegular;
constexpr FontType kSmallFont = FontType::TahomaBoldSmall;

enum class Justify { Left, Center, Right };

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

const SectionDef* sectionFor(int headerIndex) {
    for (const SectionDef& section : kSections) {
        if (section.headerIndex == headerIndex) return &section;
    }
    return nullptr;
}

const char* sectionLabel(int headerIndex) {
    const SectionDef* section = sectionFor(headerIndex);
    return section != nullptr ? section->label : "";
}

float columnIconCrop(int headerIndex, int column) {
    if (headerIndex < 1 || headerIndex > 9 || column < 1 || column > 4) return 0.0f;
    return kColumnIconCropX[headerIndex - 1][column - 1];
}

// The sort header shows as many icon columns as the widest section on the page.
int columnCountForPage(int page) {
    switch (page) {
        case kPageAll:
            return 4;  // the All tab stacks every section, weapons included
        case kPageWeapons:
        case kPageApparel:
            return 4;
        case kPageAlchemy:
            return 3;
        case kPageMisc:
            return 2;
        default:
            return 2;
    }
}

// inv_pane.y = user7 * -81 - 8 and inv_item.y = listindex * 81, so a row's top
// in sheet space is windowY - 8 + (index - scroll) * 81.
float rowTopFor(int rowIndex, int scroll) {
    return kWindowY + kPaneOffsetY + static_cast<float>(rowIndex - scroll) * kRowHeight;
}

std::string formatWeight(float value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.1f", static_cast<double>(value));
    return std::string(buffer);
}

// Numeric value backing a list column, used for sorting. Kept in step with
// itemColumns() so sorting and display never disagree.
float columnValue(const inventory::Item& item, int headerIndex, int column) {
    switch (column) {
        case 1:
            return static_cast<float>(item.value);
        case 2:
            return item.weight;
        case 3:
            if (headerIndex == 1) return static_cast<float>(item.stats.maxHealth);
            if (headerIndex == 2) return static_cast<float>(item.stats.defense);
            if (headerIndex == 3) return static_cast<float>(item.stats.maxMana);
            return 0.0f;
        case 4:
            return static_cast<float>(item.stats.maxHealth);
        default:
            return 0.0f;
    }
}

// Draws a string honouring the XML's justify attribute and returns its width in
// screen pixels so callers can position dependent widgets.
float drawUiText(TextRenderer* renderer, const std::string& text, float x, float y,
                 Justify justify, const glm::vec4& color, float scale) {
    if (renderer == nullptr || text.empty()) return 0.0f;

    const float width = renderer->getTextWidth(text, scale);
    float drawX = x;
    if (justify == Justify::Center) {
        drawX = x - width * 0.5f;
    } else if (justify == Justify::Right) {
        drawX = x - width;
    }

    renderer->renderText(text, drawX, y, color, scale);
    return width;
}

// Text widgets are positioned as (parent.height - me.height) / 2, so the
// capital cell ends up centred inside the row.
float centeredTextY(float rowTopPx, float rowHeightPx, TextRenderer* renderer, float scale) {
    const float textHeight = (renderer != nullptr) ? renderer->getTextCapHeight(scale) : 0.0f;
    return rowTopPx + (rowHeightPx - textHeight) * 0.5f;
}

// RAII font switch, mirroring UIPauseMenu::FontScope.
class FontScope {
public:
    FontScope(TextRenderer* renderer, FontType font)
        : renderer_(renderer),
          previous_(renderer != nullptr ? renderer->getActiveFont() : FontType::KingthingsRegular) {
        if (renderer_ != nullptr && previous_ != font) {
            renderer_->setActiveFont(font);
        }
    }

    ~FontScope() {
        if (renderer_ != nullptr) {
            renderer_->setActiveFont(previous_);
        }
    }

    FontScope(const FontScope&) = delete;
    FontScope& operator=(const FontScope&) = delete;

private:
    TextRenderer* renderer_;
    FontType previous_;
};

}  // namespace

UIInventoryPanel::UIInventoryPanel(const std::string& title)
    : UIPanel(title) {
    // inventory_menu.xml brings its own background and borders, so none of
    // UIPanel's chrome may be drawn, dragged or hit-tested.
    setTitleBarHeight(0.0f);
    setCloseButtonVisible(false);
    setDraggable(false);
    setBorderWidth(0.0f);
    setContentMargin(0.0f);

    refreshLayout();
}

UIInventoryPanel::~UIInventoryPanel() {
    // Must run while the GL context is still current, like UIPauseMenu::cleanup().
    clearTextureCache();
}

void UIInventoryPanel::setInventory(inventory::InventoryGrid* inv) {
    inventory = inv;
    rowsDirty = true;
    scrollOffset = 0;
    selectedRow = -1;
    refreshLayout();
}

void UIInventoryPanel::setGold(uint32_t value) {
    gold = value;
}

void UIInventoryPanel::setPage(int value) {
    const int page = std::clamp(value, 1, 5);
    if (page == currentPage) return;

    currentPage = page;
    rowsDirty = true;
    scrollOffset = 0;
    selectedRow = -1;
    refreshLayout();
}

void UIInventoryPanel::onScreenResize(int width, int height) {
    UIPanel::onScreenResize(width, height);

    if (width <= 0 || height <= 0) {
        layoutScale = 1.0f;
    } else {
        // One scale for the whole menu. The sheet is 792 tall so height binds on
        // phones; the width term keeps 4:3 tablets from overflowing.
        layoutScale = std::min(static_cast<float>(width) / kCanvasWidth,
                               static_cast<float>(height) / kCanvasHeight);
        if (layoutScale <= 0.0f) layoutScale = 1.0f;
    }

    refreshLayout();
}

float UIInventoryPanel::originX() const {
    // The XML pins the sheet at x=7 on a 1280 canvas; centring it instead keeps
    // the menu readable on the tall displays this port targets.
    return (static_cast<float>(screenWidth) - kBackgroundWidth * layoutScale) * 0.5f;
}

float UIInventoryPanel::originY() const {
    // XML: y = screen.height - me.height + 3
    return static_cast<float>(screenHeight) - (kBackgroundHeight + 3.0f) * layoutScale;
}

void UIInventoryPanel::update(float deltaTime) {
    UIPanel::update(deltaTime);
    refreshLayout();
}

void UIInventoryPanel::refreshLayout() {
    if (inventory != nullptr && rowsDirty) {
        rebuildRows();
    }

    totalWeight = 0;
    if (inventory != nullptr) {
        totalWeight = static_cast<int>(inventory->getTotalWeight() + 0.5f);
    }

    armorRating = 0;
    if (equipment != nullptr) {
        armorRating = equipment->getTotalStats().defense;
    }

    scrollOffset = std::clamp(scrollOffset, 0, maxScrollOffset());

    // The XML's _headernum counts how many section headers sit at or above the
    // top visible row; it picks the title and the img3/img4 icon variants. That
    // formula is off by one on the All page, so the ordinal of the top visible
    // section within the page is used instead.
    headerOfTopSection = 1;
    for (int i = 0; i <= scrollOffset && i < static_cast<int>(rows.size()); ++i) {
        if (rows[i].isHeader) headerOfTopSection = rows[i].headerIndex;
    }
    headernum = 1;
    const int* pageSections = kPageSections[std::clamp(currentPage, 1, 5) - 1];
    for (int i = 0; i < 9; ++i) {
        if (pageSections[i] < 0) break;
        if (kSections[pageSections[i]].headerIndex == headerOfTopSection) {
            headernum = i + 1;
            break;
        }
    }
}

void UIInventoryPanel::rebuildRows() {
    rows.clear();
    rowsDirty = false;
    selectedRow = -1;

    if (inventory == nullptr) return;

    const std::vector<inventory::GridSlot>& slots = inventory->getAllSlots();
    const int* pageSections = kPageSections[std::clamp(currentPage, 1, 5) - 1];

    for (int s = 0; s < 9; ++s) {
        const int sectionIndex = pageSections[s];
        if (sectionIndex < 0) break;

        const SectionDef& section = kSections[sectionIndex];

        std::vector<int> itemSlots;
        for (size_t i = 0; i < slots.size(); ++i) {
            if (slots[i].isEmpty()) continue;
            const int sub = static_cast<int>(inventory::effectiveSubcategory(slots[i].item));
            if (sub != section.headerIndex) continue;
            itemSlots.push_back(static_cast<int>(i));
        }

        // The XML hides a header whose position matches the next header, which is
        // how empty sections disappear from the list.
        if (itemSlots.empty()) continue;

        if (sortColumn >= 1 && sortColumn <= section.columnCount) {
            const int column = sortColumn;
            const int headerIndex = section.headerIndex;
            const bool ascending = sortAscending;
            std::stable_sort(itemSlots.begin(), itemSlots.end(),
                             [&slots, column, headerIndex, ascending](int a, int b) {
                                 const float va = columnValue(slots[a].item, headerIndex, column);
                                 const float vb = columnValue(slots[b].item, headerIndex, column);
                                 return ascending ? (va < vb) : (vb < va);
                             });
        }

        ListRow header;
        header.isHeader = true;
        header.headerIndex = section.headerIndex;
        header.columnCount = section.columnCount;
        rows.push_back(header);

        for (int slotIndex : itemSlots) {
            ListRow item;
            item.isHeader = false;
            item.headerIndex = section.headerIndex;
            item.columnCount = section.columnCount;
            item.slotIndex = slotIndex;
            rows.push_back(item);
        }
    }
}

int UIInventoryPanel::maxScrollOffset() const {
    return std::max(0, static_cast<int>(rows.size()) - kVisibleRows);
}

int UIInventoryPanel::hitTestRow(float x, float y) const {
    if (rows.empty() || layoutScale <= 0.0f) return -1;

    const float uiX = (x - originX()) / layoutScale;
    const float uiY = (y - originY()) / layoutScale;

    if (uiX < kWindowX || uiX > kWindowX + kWindowWidth) return -1;
    if (uiY < kWindowY || uiY > kWindowY + kWindowHeight) return -1;

    for (int i = scrollOffset; i < static_cast<int>(rows.size()); ++i) {
        const float top = rowTopFor(i, scrollOffset);
        if (uiY < top || uiY >= top + kRowHeight) continue;
        return rows[i].isHeader ? -1 : i;
    }

    return -1;
}

int UIInventoryPanel::hitTestTab(float x, float y) const {
    if (layoutScale <= 0.0f) return -1;

    const float uiX = (x - originX()) / layoutScale;
    const float uiY = (y - originY()) / layoutScale;
    const float tabTop = kTabsY + kTabY;

    if (uiY < tabTop || uiY > tabTop + kTabHeight) return -1;

    for (int i = 0; i < 5; ++i) {
        if (uiX >= kTabX[i] && uiX <= kTabX[i] + kTabWidth[i]) return i + 1;
    }

    return -1;
}

int UIInventoryPanel::hitTestSortHeader(float x, float y) const {
    if (layoutScale <= 0.0f) return -1;

    const float uiX = (x - originX()) / layoutScale;
    const float uiY = (y - originY()) / layoutScale;

    if (uiY < kSortHeaderY || uiY > kSortHeaderY + kRowHeight) return -1;

    const int columnCount = columnCountForPage(currentPage);
    for (int c = 0; c < columnCount; ++c) {
        const float center = kHeaderX + kHeaderIconX[c] + kColumnIconSize * 0.5f;
        if (std::fabs(uiX - center) <= kColumnIconSize * 0.5f) return c + 1;
    }

    return -1;
}

bool UIInventoryPanel::hitTestScrollBar(float x, float y) const {
    if (layoutScale <= 0.0f) return false;

    const float uiX = (x - originX()) / layoutScale;
    const float uiY = (y - originY()) / layoutScale;

    // Generous horizontal band: the rail is only 20px wide, which is well under
    // a finger's width at any sane scale.
    if (uiX < kScrollBarX - 10.0f || uiX > kScrollBarX + kScrollBarWidth + 10.0f) return false;
    if (uiY < kWindowY - kScrollBarCapHeight) return false;
    if (uiY > kWindowY + kWindowHeight + kScrollBarCapHeight) return false;

    return true;
}

void UIInventoryPanel::toggleSort(int column) {
    if (column < 1) return;

    if (sortColumn == column) {
        sortAscending = !sortAscending;
    } else {
        sortColumn = column;
        sortAscending = true;
    }
    rowsDirty = true;
    refreshLayout();
}

void UIInventoryPanel::scrollBy(int rowDelta) {
    if (rowDelta == 0) return;

    scrollOffset = std::clamp(scrollOffset + rowDelta, 0, maxScrollOffset());
    selectedRow = -1;
}

void UIInventoryPanel::setScrollFromMarkerY(float screenY) {
    const int maximum = maxScrollOffset();
    if (maximum <= 0) {
        scrollOffset = 0;
        return;
    }
    if (layoutScale <= 0.0f) return;

    const float track = kWindowHeight - kScrollBarMarkerHeight;
    if (track <= 0.0f) return;

    const float uiY = (screenY - originY()) / layoutScale - markerGrabOffset;
    const float ratio = std::clamp((uiY - kWindowY) / track, 0.0f, 1.0f);
    scrollOffset = std::clamp(static_cast<int>(std::lround(ratio * maximum)), 0, maximum);
}

bool UIInventoryPanel::isEquipped(const inventory::Item& item) const {
    if (equipment == nullptr) return false;

    const inventory::Item* equipped = equipment->getEquipped(item.equipSlot);
    if (equipped == nullptr) return false;

    if (equipped->id != 0 && item.id != 0) {
        return equipped->id == item.id;
    }
    return equipped->name == item.name;
}

float UIInventoryPanel::extraDataX() const {
    // inv_extra_data.x = me.width - (moneyText.x - moneyText.width / 2) + 40,
    // i.e. 640 - (460 - width / 2) + 40, so the gold readout stays flush right.
    float moneyWidth = 0.0f;
    if (textRenderer != nullptr) {
        moneyWidth = textRenderer->getTextWidth(std::to_string(gold), 1.0f);
    }
    return kExtraDataWidth - (kExtraTextX[2] - moneyWidth * 0.5f) + 40.0f;
}

void UIInventoryPanel::itemColumns(const inventory::Item& item, const ListRow& row,
                                   std::string out[4]) const {
    for (int i = 0; i < 4; ++i) out[i].clear();

    const int count = std::min(row.columnCount, 4);
    if (count >= 1) out[0] = std::to_string(item.value);
    if (count >= 2) out[1] = formatWeight(item.weight);

    switch (row.headerIndex) {
        case 1:  // weapons: value, weight, health, damage
            if (count >= 3) out[2] = std::to_string(item.stats.maxHealth);
            if (count >= 4) out[3] = std::to_string(item.stats.damage);
            break;
        case 2:  // apparel: value, weight, armor, health
            if (count >= 3) out[2] = std::to_string(item.stats.defense);
            if (count >= 4) out[3] = std::to_string(item.stats.maxHealth);
            break;
        case 3:  // apparatus: value, weight, quality
            if (count >= 3) out[2] = std::to_string(item.stats.maxMana);
            break;
        default:
            break;
    }
}

std::string UIInventoryPanel::iconPathFor(uint32_t iconId) const {
    switch (iconId) {
        case inventory::ItemFactory::ICON_ID_IRON_SWORD:
            return "icons/icon_iron_sword.png";
        case inventory::ItemFactory::ICON_ID_IRON_CUIRASS:
            return "icons/icon_iron_cuirass.png";
        case inventory::ItemFactory::ICON_ID_HEALTH_POTION:
            return "icons/icon_health_potion.png";
        case inventory::ItemFactory::ICON_ID_MANA_POTION:
            return "icons/icon_mana_potion.png";
        case inventory::ItemFactory::ICON_ID_IRON_ORE:
            return "icons/icon_iron_ore.png";
        case inventory::ItemFactory::ICON_ID_LEATHER:
            return "icons/icon_leather.png";
        case inventory::ItemFactory::ICON_ID_SCROLL_SHIELD:
            return "icons/icon_scroll_shield.png";
        default:
            return std::string();
    }
}

GLuint UIInventoryPanel::texture(const std::string& name) {
    auto it = uiTextures.find(name);
    if (it != uiTextures.end()) return it->second;

    const GLuint id = TextureLoader::loadTextureFromAsset("textures/ui/" + name);
    if (id == 0) {
        LOGW("Missing UI texture: textures/ui/%s", name.c_str());
    }
    uiTextures[name] = id;
    return id;
}

GLuint UIInventoryPanel::getIconTexture(uint32_t iconId) {
    if (iconId == 0) return 0;

    auto it = iconTextures.find(iconId);
    if (it != iconTextures.end()) return it->second;

    GLuint id = 0;
    const std::string path = iconPathFor(iconId);
    if (!path.empty()) {
        id = TextureLoader::loadTextureFromAsset("textures/ui/" + path);
    }
    if (id == 0) {
        LOGW("Missing inventory icon texture for iconId %u", iconId);
    }
    iconTextures[iconId] = id;
    return id;
}

void UIInventoryPanel::clearTextureCache() {
    for (const auto& entry : uiTextures) {
        TextureLoader::deleteTexture(entry.second);
    }
    uiTextures.clear();

    for (const auto& entry : iconTextures) {
        TextureLoader::deleteTexture(entry.second);
    }
    iconTextures.clear();
}

void UIInventoryPanel::drawWidget(const char* assetName, float x, float y, float w, float h,
                                  float cropx, float cropy, float cropW, float cropH,
                                  float alpha) {
    const GLuint tex = texture(assetName);
    if (tex == 0) return;

    int texW = 0;
    int texH = 0;
    TextureLoader::getTextureSize(tex, texW, texH);

    float uMin = 0.0f;
    float vMin = 0.0f;
    float uMax = 1.0f;
    float vMax = 1.0f;
    float drawY = y;
    float drawH = h;

    if (texW > 0 && texH > 0) {
        float srcX = cropx;
        float srcY = cropy;
        const float srcW = (cropW > 0.0f) ? cropW : static_cast<float>(texW);
        float srcH = (cropH > 0.0f) ? cropH : static_cast<float>(texH);

        // inv_window is clipwindow="true", so a partially scrolled row is cut at
        // the window edge by trimming the source rect instead of the quad.
        if (clipActive) {
            const float visibleTop = std::max(y, clipTop);
            const float visibleBottom = std::min(y + h, clipBottom);
            if (visibleBottom <= visibleTop) return;

            srcY += (visibleTop - y);
            srcH -= (visibleTop - y) + (y + h - visibleBottom);
            if (srcH <= 0.0f) return;

            drawY = visibleTop;
            drawH = visibleBottom - visibleTop;
        }

        uMin = srcX / static_cast<float>(texW);
        uMax = (srcX + srcW) / static_cast<float>(texW);
        vMin = srcY / static_cast<float>(texH);
        vMax = (srcY + srcH) / static_cast<float>(texH);
    }

    UIDrawHelper::drawTexturedQuad(originX() + x * layoutScale, originY() + drawY * layoutScale,
                                   w * layoutScale, drawH * layoutScale, tex,
                                   glm::vec4(1.0f, 1.0f, 1.0f, alpha),
                                   screenWidth, screenHeight, uMin, vMin, uMax, vMax);
}

void UIInventoryPanel::drawAtlasIcon(float cropx, float x, float y, float size) {
    if (cropx <= 0.0f) return;
    drawWidget(kAssetSmallIcons, x, y, size, size, cropx, 0.0f, size, size);
}

void UIInventoryPanel::drawFocusBox(float x, float y, float w, float h) {
    // focus_box.xml draws an 8px frame just outside the widget it highlights;
    // only the four edge strips matter for a full-width row.
    drawWidget(kAssetFocusLeft, x - 8.0f, y - 1.0f, 8.0f, h + 2.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    drawWidget(kAssetFocusRight, x + w, y - 1.0f, 12.0f, h + 2.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    drawWidget(kAssetFocusTop, x - 1.0f, y - 8.0f, w + 2.0f, 9.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    drawWidget(kAssetFocusBottom, x - 1.0f, y + h, w + 2.0f, 14.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    drawWidget(kAssetFocusCenter, x - 1.0f, y - 1.0f, w + 2.0f, h + 2.0f, 0.0f, 0.0f, 0.0f,
               0.0f);
}

void UIInventoryPanel::drawBackground() {
    drawWidget(kAssetBackground, 0.0f, 0.0f, kBackgroundWidth, kBackgroundHeight, 0.0f, 0.0f,
               0.0f, 0.0f);
    // inv_background_2 extends the sheet to the right (x=700).
    drawWidget(kAssetBackground2, 700.0f, 0.0f, 54.0f, kBackgroundHeight, 0.0f, 0.0f, 0.0f,
               0.0f);

    // inv_p1..p5: the page artwork that the tabs sit on, active page only.
    const int pageIndex = std::clamp(currentPage, 1, 5) - 1;
    drawWidget(kAssetPageTab[pageIndex], 23.0f, 542.0f, 791.0f, 176.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    // inv_border_top at (110, 65), with the middle and bottom stacked inside it.
    drawWidget(kAssetBorderTop, 110.0f, 65.0f, 596.0f, 160.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    drawWidget(kAssetBorderMiddle, 110.0f, 65.0f + 157.0f, 576.0f, 249.0f, 0.0f, 0.0f, 0.0f,
               0.0f);
    // inv_border_bottom crops from x=4 and is offset -57 from the top's x.
    drawWidget(kAssetBorderBottom, 110.0f - 57.0f, 65.0f + 393.0f, 628.0f, 160.0f, 4.0f, 0.0f,
               0.0f, 0.0f);
}

void UIInventoryPanel::drawTabs() {
    const float tabTop = kTabsY + kTabY;

    for (int i = 0; i < 5; ++i) {
        const bool active = (i + 1) == currentPage;

        // Every tab keeps its icon; the active tab is the one whose page artwork
        // is showing, which is exactly when the XML's target flag is false.
        drawWidget(kAssetTabIcons, kTabX[i] + kTabIconX, tabTop + kTabIconY, kTabIconSize,
                   kTabIconSize, static_cast<float>(i) * kTabIconSize, 0.0f, kTabIconSize,
                   kTabIconSize);
        drawWidget(active ? kAssetTabSelectionOn : kAssetTabSelectionOff, kTabX[i],
                   tabTop + kTabPlateY, kTabPlateWidth, kTabPlateHeight, 0.0f, 0.0f, 0.0f, 0.0f);
    }
}

void UIInventoryPanel::drawExtraData() {
    const float blockX = extraDataX();
    const float s = layoutScale;
    const float ox = originX();

    for (int i = 0; i < 3; ++i) {
        drawAtlasIcon(kExtraIconCropX[i], blockX + kExtraIconX[i], kExtraDataY,
                      kExtraDataHeight);
    }

    FontScope font(textRenderer, kSmallFont);
    const float textY =
        centeredTextY(originY() + kExtraDataY * s, kExtraDataHeight * s, textRenderer, s);

    // The slash and max value hang off the running text width, as in the XML.
    const float encumbranceX = ox + (blockX + kExtraTextX[0]) * s;
    const float encumbranceWidth =
        drawUiText(textRenderer, std::to_string(totalWeight), encumbranceX, textY,
                   Justify::Left, kRowTextColor, s);

    const float slashX = encumbranceX + encumbranceWidth + s;
    const float slashWidth =
        drawUiText(textRenderer, "/", slashX, textY, Justify::Left, kRowTextColor, s);

    const float maxWeight = (inventory != nullptr) ? inventory->getMaxWeight() : 0.0f;
    drawUiText(textRenderer, std::to_string(static_cast<int>(maxWeight + 0.5f)),
               slashX + slashWidth + s, textY, Justify::Left, kRowTextColor, s);

    drawUiText(textRenderer, std::to_string(armorRating),
               ox + (blockX + kExtraTextX[1]) * s, textY, Justify::Left, kRowTextColor, s);

    drawUiText(textRenderer, std::to_string(gold), ox + (blockX + kExtraTextX[2]) * s, textY,
               Justify::Left, kRowTextColor, s);
}

void UIInventoryPanel::drawScrollBar() {
    // vertical_scroll.visible = (user2 + user8) > user8, i.e. user2 > 0.
    if (maxScrollOffset() <= 0) return;

    // Rail: the 20px band at cropx=32, stretched to the window height.
    drawWidget(kAssetScrollBar, kScrollBarX, kWindowY, kScrollBarWidth, kWindowHeight, 32.0f,
               0.0f, kScrollBarWidth, 0.0f);
    // Top cap sits above the rail (y = -me.height).
    drawWidget(kAssetScrollBar, kScrollBarX, kWindowY - kScrollBarCapHeight, kScrollBarWidth,
               kScrollBarCapHeight, 0.0f, 0.0f, kScrollBarWidth, kScrollBarCapHeight);
    // Bottom cap sits at the rail's end.
    drawWidget(kAssetScrollBar, kScrollBarX, kWindowY + kWindowHeight, kScrollBarWidth,
               kScrollBarCapHeight, 0.0f, kScrollBarBottomCropY, kScrollBarWidth,
               kScrollBarCapHeight);

    const int maximum = maxScrollOffset();
    const float track = kWindowHeight - kScrollBarMarkerHeight;
    const float ratio = static_cast<float>(scrollOffset) / maximum;

    drawWidget(kAssetScrollBar, kScrollBarX, kWindowY + track * ratio, kScrollBarWidth,
               kScrollBarMarkerHeight, 0.0f, kScrollBarCapHeight, kScrollBarWidth,
               kScrollBarMarkerHeight);
}

void UIInventoryPanel::drawTopHeader() {
    const int columnCount = columnCountForPage(currentPage);
    const float s = layoutScale;
    const float ox = originX();
    const float textY = originY() + (kSortHeaderY + kSortTextY) * s;

    float textWidthUi = 0.0f;
    {
        FontScope font(textRenderer, kSmallFont);
        const float widthPx = drawUiText(textRenderer, sectionLabel(headerOfTopSection),
                                         ox + (kHeaderX + kHeaderTextX) * s, textY,
                                         Justify::Center, kHeaderTextColor, s);
        textWidthUi = (s > 0.0f) ? (widthPx / s) : 0.0f;
    }

    const float centerX = kHeaderX + kHeaderTextX;
    const float decorY = kSortHeaderY + kSortTextY + kSortDecorDy;
    drawWidget(kAssetDecorLeft, centerX - textWidthUi * 0.5f - 55.0f, decorY, kDecorWidth,
               kDecorHeight, 0.0f, 0.0f, 0.0f, 0.0f);
    drawWidget(kAssetDecorRight, centerX + textWidthUi * 0.5f + 20.0f, decorY, kDecorWidth,
               kDecorHeight, 0.0f, 0.0f, 0.0f, 0.0f);

    // img1/img2 are unconditional; img3/img4 depend on which section is on top.
    drawAtlasIcon(columnIconCrop(headerOfTopSection, 1), kHeaderX + kHeaderIconX[0],
                  kSortHeaderY, kColumnIconSize);
    drawAtlasIcon(columnIconCrop(headerOfTopSection, 2), kHeaderX + kHeaderIconX[1],
                  kSortHeaderY, kColumnIconSize);

    if (columnCount >= 3 && headernum >= 2) {
        drawAtlasIcon(columnIconCrop(headerOfTopSection, 3), kHeaderX + kHeaderIconX[2],
                      kSortHeaderY, kColumnIconSize);
    }
    if (columnCount >= 4 && headernum <= 2) {
        drawAtlasIcon(columnIconCrop(headerOfTopSection, 4), kHeaderX + kHeaderIconX[3],
                      kSortHeaderY, kColumnIconSize);
    }
}

void UIInventoryPanel::drawHeaderRow(int rowIndex, float top) {
    if (rowIndex < 0 || rowIndex >= static_cast<int>(rows.size())) return;

    const ListRow& row = rows[rowIndex];
    const SectionDef* section = sectionFor(row.headerIndex);
    if (section == nullptr) return;

    const float s = layoutScale;
    const float ox = originX();

    float textWidthUi = 0.0f;
    {
        FontScope font(textRenderer, kSmallFont);
        const float widthPx =
            drawUiText(textRenderer, section->label, ox + (kHeaderX + kHeaderTextX) * s,
                       originY() + (top + kHeaderTextY) * s, Justify::Center, kHeaderTextColor,
                       s);
        textWidthUi = (s > 0.0f) ? (widthPx / s) : 0.0f;
    }

    const float centerX = kHeaderX + kHeaderTextX;
    const float decorY = top + kHeaderTextY + kHeaderDecorDy;
    drawWidget(kAssetDecorLeft, centerX - textWidthUi * 0.5f - 55.0f, decorY, kDecorWidth,
               kDecorHeight, 0.0f, 0.0f, 0.0f, 0.0f);
    drawWidget(kAssetDecorRight, centerX + textWidthUi * 0.5f + 20.0f, decorY, kDecorWidth,
               kDecorHeight, 0.0f, 0.0f, 0.0f, 0.0f);

    const int count = std::min(row.columnCount, 4);
    for (int c = 0; c < count; ++c) {
        drawAtlasIcon(columnIconCrop(row.headerIndex, c + 1), kHeaderX + kHeaderIconX[c],
                      top + kHeaderIconY, kColumnIconSize);
    }
}

void UIInventoryPanel::drawItemRow(int rowIndex, float top) {
    if (inventory == nullptr) return;
    if (rowIndex < 0 || rowIndex >= static_cast<int>(rows.size())) return;

    const ListRow& row = rows[rowIndex];
    if (row.slotIndex < 0) return;

    const inventory::GridSlot& slot =
        inventory->getSlot(static_cast<uint32_t>(row.slotIndex));
    if (slot.isEmpty()) return;

    const inventory::Item& item = slot.item;
    const float s = layoutScale;
    const float ox = originX();
    const float rowTopPx = originY() + top * s;

    if (selectedRow == rowIndex) {
        drawFocusBox(kWindowX, top, kWindowWidth, kRowHeight);
    }

    if (isEquipped(item)) {
        drawWidget(kAssetEquipMarker, kWindowX + kItemMarkerX, top + kItemMarkerY,
                   kItemMarkerWidth, kItemMarkerHeight, 0.0f, 0.0f, 0.0f, 0.0f,
                   160.0f / 255.0f);
    }

    const GLuint icon = getIconTexture(item.iconId);
    if (icon != 0) {
        UIDrawHelper::drawTexturedQuad(ox + (kWindowX + kItemIconX) * s,
                                       originY() + (top + kItemIconY) * s, kItemIconSize * s,
                                       kItemIconSize * s, icon, kWhite, screenWidth, screenHeight);
    }

    std::string columns[4];
    itemColumns(item, row, columns);

    {
        FontScope font(textRenderer, kNameFont);
        // getTextCapHeight depends on the active font, so the centred baseline
        // has to be resolved after the switch.
        const float textY = centeredTextY(rowTopPx, kRowHeight * s, textRenderer, s);

        if (slot.quantity > 1) {
            drawUiText(textRenderer, std::to_string(slot.quantity),
                       ox + (kWindowX + kItemCountX) * s, textY, Justify::Right, kRowTextColor,
                       s);
        }

        drawUiText(textRenderer, item.name, ox + (kWindowX + kItemNameX) * s, textY,
                   Justify::Left, kRowTextColor, s);

        const int count = std::min(row.columnCount, 4);
        for (int c = 0; c < count; ++c) {
            if (columns[c].empty()) continue;
            drawUiText(textRenderer, columns[c], ox + (kWindowX + kItemColumnX[c]) * s, textY,
                       Justify::Center, kRowTextColor, s);
        }
    }
}

void UIInventoryPanel::drawRows() {
    if (rows.empty()) return;

    clipActive = true;
    clipTop = kWindowY;
    clipBottom = kWindowY + kWindowHeight;

    const int first = std::clamp(scrollOffset, 0, static_cast<int>(rows.size()));
    const int last = std::min(static_cast<int>(rows.size()), first + kVisibleRows + 1);
    for (int i = first; i < last; ++i) {
        const float top = rowTopFor(i, scrollOffset);
        if (rows[i].isHeader) {
            drawHeaderRow(i, top);
        } else {
            drawItemRow(i, top);
        }
    }

    clipActive = false;
}

void UIInventoryPanel::render() {
    if (!isVisible() || layoutScale <= 0.0f) return;

    // UIPanel::render() is deliberately not called: it would draw a title bar and
    // close button that inventory_menu.xml does not have.
    GLboolean depthWasEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_TEST, &depthWasEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawBackground();
    drawTabs();
    drawExtraData();
    drawTopHeader();
    drawRows();
    drawScrollBar();

    if (depthWasEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

bool UIInventoryPanel::onEvent(const UIEvent& event) {
    if (!isVisible() || !isEnabled()) return false;

    // The original gates the whole world update on the menu-open flag, so the
    // menu swallows every event while it is up rather than letting the camera
    // see a drag that started on a tab.
    switch (event.type) {
        case UIEventType::TOUCH_DOWN:
            onTouchDown(event.x, event.y, event.pointerId);
            break;
        case UIEventType::TOUCH_UP:
            onTouchUp(event.x, event.y, event.pointerId);
            break;
        case UIEventType::TOUCH_MOVE:
            onTouchMove(event.x, event.y, event.dx, event.dy, event.pointerId);
            break;
        default:
            break;
    }

    return true;
}

bool UIInventoryPanel::onTouchDown(float x, float y, int pointerId) {
    (void)pointerId;

    const int tab = hitTestTab(x, y);
    if (tab > 0) {
        setPage(tab);
        return true;
    }

    const int sortColumnHit = hitTestSortHeader(x, y);
    if (sortColumnHit > 0) {
        toggleSort(sortColumnHit);
        return true;
    }

    if (maxScrollOffset() > 0 && hitTestScrollBar(x, y)) {
        const float s = layoutScale;
        const float track = kWindowHeight - kScrollBarMarkerHeight;
        const float markerTop =
            kWindowY + track * (static_cast<float>(scrollOffset) / maxScrollOffset());
        const float uiY = (y - originY()) / s;

        if (uiY < markerTop) {
            // Above the marker: the top cap steps by user3 (1), the rail by user4 (5).
            scrollBy(uiY < kWindowY ? 1 : 5);
        } else if (uiY > markerTop + kScrollBarMarkerHeight) {
            scrollBy(uiY > kWindowY + kWindowHeight ? -1 : -5);
        } else {
            // On the marker: drag it.
            markerGrabOffset = uiY - markerTop;
            draggingMarker = true;
        }
        return true;
    }

    const int row = hitTestRow(x, y);
    if (row >= 0) {
        selectedRow = (selectedRow == row) ? -1 : row;
        return true;
    }

    return true;
}

bool UIInventoryPanel::onTouchUp(float x, float y, int pointerId) {
    (void)x;
    (void)y;
    (void)pointerId;

    draggingMarker = false;
    return true;
}

bool UIInventoryPanel::onTouchMove(float, float y, float dx, float dy, int pointerId) {
    (void)dx;
    (void)dy;
    (void)pointerId;

    if (draggingMarker) {
        setScrollFromMarkerY(y);
    }
    return true;
}

}  // namespace ui
