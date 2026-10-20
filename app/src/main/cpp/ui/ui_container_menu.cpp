#include "ui_container_menu.h"
#include "item_icons.h"
#include "text_renderer.h"
#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"
#include "../game/container.h"
#include "../game/player.h"

#include <array>

#include <GLES3/gl3.h>
#include <android/log.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

#undef LOG_TAG
#undef LOGD
#undef LOGW
#define LOG_TAG "UIContainerMenu"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

namespace {

constexpr float kCanvasWidth = 1280.0f;
constexpr float kCanvasHeight = 960.0f;
constexpr float kDimAlpha = 0.5f;

// ── p68 UI texture keys ───────────────────────────────────────────
const char* const kTexFadersBlack   = "faders__black";
const char* const kTexSelectionOn    = "shared__general_tab_selection_on";
const char* const kTexSelectionOff   = "shared__general_tab_selection_off";
const char* const kTexButtonLongOff  = "shared__shared_button_long_off";
const char* const kTexButtonLongOn   = "shared__shared_button_long_on";
const char* const kTexContButtonOff  = "container__cont_button_long_off";
const char* const kTexContButtonOn   = "container__cont_button_long_on";

// Generic background 9-slice keys
const char* const kTexBgCenter        = "genericbackground__center_background";
const char* const kTexBgTop          = "genericbackground__edge_top";
const char* const kTexBgBottom       = "genericbackground__edge_bottom";
const char* const kTexBgLeft         = "genericbackground__edge_left";
const char* const kTexBgRight        = "genericbackground__edge_right";
const char* const kTexBgTopLeft      = "genericbackground__top_left_corner";
const char* const kTexBgTopRight     = "genericbackground__top_right_corner";
const char* const kTexBgBottomLeft   = "genericbackground__bottom_left_corner";
const char* const kTexBgBottomRight  = "genericbackground__bottom_right_corner";

// Inventory header textures
const char* const kTexHeaderWeapons  = "inv_weapon_header";
const char* const kTexHeaderArmor    = "inv_armor_header";
const char* const kTexHeaderAlchemy  = "inv_alchemy_header";
const char* const kTexHeaderMisc     = "inv_misc_header";

// Tab icon textures
const char* const kTexTabAll         = "inv_icon_tab_all";
const char* const kTexTabWeapons     = "inv_icon_tab_weapons";
const char* const kTexTabApparel     = "inv_icon_tab_apparel";
const char* const kTexTabAlchemy     = "inv_icon_tab_alchemy";
const char* const kTexTabMisc        = "inv_icon_tab_misc";

// Focus frame 9-slice keys
const char* const kTexFocusCenter     = "focus__focus_center_x4";
const char* const kTexFocusTop        = "focus__focus_top";
const char* const kTexFocusBottom     = "focus__focus_bottom";
const char* const kTexFocusLeft       = "focus__focus_left";
const char* const kTexFocusRight      = "focus__focus_right";
const char* const kTexFocusTopLeft    = "focus__focus_top_left";
const char* const kTexFocusTopRight   = "focus__focus_top_right";
const char* const kTexFocusBottomLeft = "focus__focus_bottom_left";
const char* const kTexFocusBottomRight= "focus__focus_bottom_right";

// Weight marker
const char* const kTexWeightSmall     = "inv_icon_small_weight";

constexpr float kContainerPaneX = 10.0f;
constexpr float kContainerPaneY = 150.0f;
constexpr float kContainerPaneW = 620.0f;
constexpr float kContainerPaneH = 430.0f;

constexpr float kPlayerPaneX = 650.0f;
constexpr float kPlayerPaneW = 620.0f;
constexpr float kPlayerPaneY = 150.0f;
constexpr float kPlayerPaneH = 430.0f;

constexpr float kPaneHeaderH = 30.0f;

constexpr float kRowHeight = 81.0f;
constexpr int kVisibleRows = 5;

constexpr float kItemIconX = 5.0f;
constexpr float kItemIconY = 7.0f;
constexpr float kItemIconSize = 64.0f;
constexpr float kItemNameX = 80.0f;
constexpr float kItemNameWrapW = 330.0f;
constexpr float kItemValueX = 420.0f;
constexpr float kItemWeightX = 495.0f;
constexpr float kItemDamageX = 555.0f;
constexpr float kItemHealthX = 615.0f;

constexpr float kTabsY = 590.0f;
constexpr float kTabH = 36.0f;
constexpr float kTabW[5] = {120.0f, 120.0f, 120.0f, 120.0f, 120.0f};
constexpr float kTabX[5] = {20.0f, 148.0f, 276.0f, 404.0f, 532.0f};

constexpr float kButtonsY = 635.0f;
constexpr float kButtonH = 40.0f;
constexpr float kButtonTakeAllX = 100.0f;
constexpr float kButtonHaggleX = 420.0f;
constexpr float kButtonExitX = 740.0f;
constexpr float kButtonW = 180.0f;

constexpr float kBottomBarY = 595.0f;
constexpr float kBottomBarH = 100.0f;

constexpr float kWeightDisplayX = 920.0f;
constexpr float kWeightDisplayY = 560.0f;

constexpr float kSelectionFrameW = 620.0f;
constexpr float kSelectionFrameH = 86.0f;

const glm::vec4 kRowTextColor(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 1.0f);
const glm::vec4 kHeaderTextColor(51.0f / 255.0f, 79.0f / 255.0f, 142.0f / 255.0f, 1.0f);
const glm::vec4 kWhite(1.0f, 1.0f, 1.0f, 1.0f);
const glm::vec4 kSelectedBg(0.8f, 0.7f, 0.5f, 0.2f);
const glm::vec4 kTabActiveBg(0.3f, 0.4f, 0.7f, 0.3f);
const glm::vec4 kTabIdleBg(0.1f, 0.1f, 0.2f, 0.2f);
const glm::vec4 kButtonBg(0.2f, 0.2f, 0.35f, 0.5f);
const glm::vec4 kButtonHoverBg(0.3f, 0.4f, 0.7f, 0.6f);
const glm::vec4 kPaneBg(0.0f, 0.0f, 0.0f, 0.35f);
const glm::vec4 kPaneHeaderBg(0.1f, 0.1f, 0.2f, 0.5f);

constexpr FontType kNameFont = FontType::KingthingsRegular;
constexpr FontType kSmallFont = FontType::TahomaBoldSmall;

const char* const kTabLabels[5] = {
    "All", "Weapons", "Apparel", "Magic", "Misc"
};

constexpr int kCategoryAll         = 0;
constexpr int kCategoryWeapon      = 1;
constexpr int kCategoryApparel     = 2;
constexpr int kCategoryApparatus   = 3;
constexpr int kCategoryPotions     = 4;
constexpr int kCategoryIngredients = 5;
constexpr int kCategoryBooks       = 6;
constexpr int kCategoryKeys        = 7;
constexpr int kCategoryStones      = 8;
constexpr int kCategoryMisc        = 9;

const char* const kCategoryLabels[10] = {
    "All", "Weapons", "Apparel", "Apparatus", "Potions",
    "Ingredients", "Books", "Keys", "Stones", "Misc"
};

constexpr int kTabToCategory[5][9] = {
    {1, 2, 3, 4, 5, 6, 7, 8, 9},
    {1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 4, -1, -1, -1, -1, -1, -1, -1},
    {5, 6, 7, 8, 9, -1, -1, -1, -1},
};

class FontScope {
public:
    FontScope(TextRenderer* renderer, FontType font)
        : renderer_(renderer),
          previous_(renderer ? renderer->getActiveFont() : FontType::KingthingsRegular) {
        if (renderer_ && previous_ != font) {
            renderer_->setActiveFont(font);
        }
    }
    ~FontScope() {
        if (renderer_) renderer_->setActiveFont(previous_);
    }
private:
    TextRenderer* renderer_;
    FontType previous_;
};

float drawUiText(TextRenderer* renderer, const std::string& text, float x, float y,
                 const glm::vec4& color, float scale) {
    if (!renderer || text.empty()) return 0.0f;
    const float width = renderer->getTextWidth(text, scale);
    renderer->renderText(text, x, y, color, scale);
    return width;
}

float centeredTextY(float rowTopPx, float rowHeightPx, TextRenderer* renderer, float scale) {
    const float textHeight = renderer ? renderer->getTextCapHeight(scale) : 0.0f;
    return rowTopPx + (rowHeightPx - textHeight) * 0.5f;
}

float rowTopFor(int rowIndex, int scrollOffset) {
    return kContainerPaneY + static_cast<float>(rowIndex - scrollOffset) * kRowHeight;
}

} // namespace

// ── Generic background 9-slice key order: center,top,bottom,left,right,
//    top-left,top-right,bottom-left,bottom-right ─────────────────────
const char* UIContainerMenu::kGenericBgKeys[9] = {
    kTexBgCenter, kTexBgTop, kTexBgBottom,
    kTexBgLeft, kTexBgRight,
    kTexBgTopLeft, kTexBgTopRight,
    kTexBgBottomLeft, kTexBgBottomRight,
};

UIContainerMenu::UIContainerMenu()
    : textRenderer_(nullptr), screenWidth_(1280), screenHeight_(960),
      visible_(false), fadeAlpha_(1.0f), container_(nullptr), player_(nullptr),
      selectedContainerRow_(-1), selectedPlayerRow_(-1), activeTab_(0),
      activePane_(Pane::CONTAINER), scrollOffsetContainer_(0), scrollOffsetPlayer_(0),
      scaleX_(1.0f), scaleY_(1.0f) {
}

UIContainerMenu::~UIContainerMenu() {
    cleanup();
}

bool UIContainerMenu::initialize(TextRenderer* renderer) {
    if (!renderer) {
        __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "TextRenderer is null");
        return false;
    }
    textRenderer_ = renderer;
    preloadUiTextures();
    LOGD("ContainerMenu initialized with p68 original assets");
    return true;
}

void UIContainerMenu::cleanup() {
    clearTextureCache();
    textRenderer_ = nullptr;
    visible_ = false;
    container_.reset();
    player_ = nullptr;
}

void UIContainerMenu::open(std::shared_ptr<Container> container, Player* player) {
    if (!container || !player) return;
    container_ = std::move(container);
    player_ = player;
    visible_ = true;
    fadeAlpha_ = 1.0f;
    activeTab_ = 0;
    activePane_ = Pane::CONTAINER;
    selectedContainerRow_ = -1;
    selectedPlayerRow_ = -1;
    scrollOffsetContainer_ = 0;
    scrollOffsetPlayer_ = 0;
    rebuildContainerList();
    rebuildPlayerList();
    rebuildFilteredLists();
    LOGD("ContainerMenu opened");
}

void UIContainerMenu::close() {
    visible_ = false;
    container_.reset();
    player_ = nullptr;
    LOGD("ContainerMenu closed");
}

void UIContainerMenu::setScreenSize(int width, int height) {
    if (width <= 0 || height <= 0) {
        scaleX_ = 1.0f;
        scaleY_ = 1.0f;
    } else {
        scaleX_ = static_cast<float>(width) / kCanvasWidth;
        scaleY_ = static_cast<float>(height) / kCanvasHeight;
        if (scaleX_ <= 0.0f) scaleX_ = 1.0f;
        if (scaleY_ <= 0.0f) scaleY_ = 1.0f;
    }
    screenWidth_ = width;
    screenHeight_ = height;
}

void UIContainerMenu::setFadeAlpha(float alpha) {
    fadeAlpha_ = std::clamp(alpha, 0.0f, 1.0f);
}

void UIContainerMenu::update(float deltaTime) {
    (void)deltaTime;
}

// ── p68: Texture preload ───────────────────────────────────────────────

void UIContainerMenu::preloadUiTextures() {
    // Background (menus/)
    loadTexture(std::string("menus/") + kTexFadersBlack);

    // Generic background 9-slice (menus/genericbackground__*)
    for (const char* key : kGenericBgKeys) {
        loadTexture(std::string("menus/") + key);
    }

    // Selection tabs (menus/)
    loadTexture(std::string("menus/") + kTexSelectionOn);
    loadTexture(std::string("menus/") + kTexSelectionOff);

    // Buttons (menus/)
    loadTexture(std::string("menus/") + kTexButtonLongOff);
    loadTexture(std::string("menus/") + kTexButtonLongOn);
    loadTexture(std::string("menus/") + kTexContButtonOff);
    loadTexture(std::string("menus/") + kTexContButtonOn);

    // Inventory headers (inventory/)
    loadTexture(std::string("inventory/") + kTexHeaderWeapons);
    loadTexture(std::string("inventory/") + kTexHeaderArmor);
    loadTexture(std::string("inventory/") + kTexHeaderAlchemy);
    loadTexture(std::string("inventory/") + kTexHeaderMisc);

    // Tab icons (inventory/)
    loadTexture(std::string("inventory/") + kTexTabAll);
    loadTexture(std::string("inventory/") + kTexTabWeapons);
    loadTexture(std::string("inventory/") + kTexTabApparel);
    loadTexture(std::string("inventory/") + kTexTabAlchemy);
    loadTexture(std::string("inventory/") + kTexTabMisc);

    // Focus frame 9-slice (menus/focus__*)
    loadTexture(std::string("menus/") + kTexFocusCenter);
    loadTexture(std::string("menus/") + kTexFocusTop);
    loadTexture(std::string("menus/") + kTexFocusBottom);
    loadTexture(std::string("menus/") + kTexFocusLeft);
    loadTexture(std::string("menus/") + kTexFocusRight);
    loadTexture(std::string("menus/") + kTexFocusTopLeft);
    loadTexture(std::string("menus/") + kTexFocusTopRight);
    loadTexture(std::string("menus/") + kTexFocusBottomLeft);
    loadTexture(std::string("menus/") + kTexFocusBottomRight);

    // Weight icon (inventory/)
    loadTexture(std::string("inventory/") + kTexWeightSmall);
}

GLuint UIContainerMenu::uiTex(const std::string& key) const {
    // key format: "menus/{name}" or "inventory/{name}" — matches uiTextures_ keys
    auto it = uiTextures_.find(key);
    if (it != uiTextures_.end()) return it->second;
    return 0;
}

int UIContainerMenu::itemTypeToCategory(int typeInt) {
    switch (static_cast<ItemType>(typeInt)) {
        case ItemType::WEAPON:   return kCategoryWeapon;
        case ItemType::ARMOR:
        case ItemType::CLOTHING:
        case ItemType::ACCESSORY: return kCategoryApparel;
        case ItemType::INGREDIENT: return kCategoryIngredients;
        case ItemType::POTION:
        case ItemType::SCROLL:    return kCategoryPotions;
        case ItemType::BOOK:      return kCategoryBooks;
        default:                  return kCategoryMisc;
    }
}

const char* UIContainerMenu::categoryLabel(int category) {
    if (category >= 0 && category < 10) return kCategoryLabels[category];
    return "";
}

int UIContainerMenu::categoryForTab(int tab) {
    switch (tab) {
        case 0: return kCategoryAll;
        case 1: return kCategoryWeapon;
        case 2: return kCategoryApparel;
        case 3: return kCategoryPotions;
        case 4: return kCategoryMisc;
        default: return kCategoryAll;
    }
}

void UIContainerMenu::rebuildContainerList() {
    containerItems_.clear();
    if (!container_) return;

    const auto& items = container_->getItems();
    for (size_t i = 0; i < items.size(); ++i) {
        const InventorySlot& slot = items[i];
        if (slot.isEmpty()) continue;
        ItemEntry entry;
        entry.itemId = slot.item.itemId;
        entry.name = slot.item.name;
        entry.weight = slot.item.weight;
        entry.value = slot.item.value;
        entry.quantity = slot.quantity;
        entry.iconId = slot.item.itemId;
        entry.category = itemTypeToCategory(static_cast<int>(slot.item.type));
        entry.sourceIndex = i;
        containerItems_.push_back(entry);
    }
}

void UIContainerMenu::rebuildPlayerList() {
    playerItems_.clear();
    if (!player_) return;

    for (size_t i = 0; i < player_->inventory.size(); ++i) {
        const InventorySlot& slot = player_->inventory[i];
        if (slot.isEmpty()) continue;
        ItemEntry entry;
        entry.itemId = slot.item.itemId;
        entry.name = slot.item.name;
        entry.weight = slot.item.weight;
        entry.value = slot.item.value;
        entry.quantity = slot.quantity;
        entry.iconId = slot.item.itemId;
        entry.category = itemTypeToCategory(static_cast<int>(slot.item.type));
        entry.sourceIndex = i;
        playerItems_.push_back(entry);
    }
}

void UIContainerMenu::rebuildFilteredLists() {
    for (int c = 0; c < kCategoryCount; ++c) {
        filteredContainerIndices_[c].clear();
        filteredPlayerIndices_[c].clear();
    }

    for (size_t i = 0; i < containerItems_.size(); ++i) {
        int cat = containerItems_[i].category;
        if (cat < 0 || cat >= kCategoryCount) cat = kCategoryMisc;
        filteredContainerIndices_[kCategoryAll].push_back(i);
        filteredContainerIndices_[cat].push_back(i);
    }

    for (size_t i = 0; i < playerItems_.size(); ++i) {
        int cat = playerItems_[i].category;
        if (cat < 0 || cat >= kCategoryCount) cat = kCategoryMisc;
        filteredPlayerIndices_[kCategoryAll].push_back(i);
        filteredPlayerIndices_[cat].push_back(i);
    }
}

void UIContainerMenu::takeItem(size_t containerIndex) {
    if (!container_ || !player_) return;
    if (containerIndex >= containerItems_.size()) return;

    const ItemEntry& entry = containerItems_[containerIndex];
    if (!container_->removeItem(entry.itemId, entry.quantity)) return;

    InventorySlot slot;
    slot.item = Item(entry.itemId, entry.name, static_cast<ItemType>(entry.category),
                     entry.weight, entry.value);
    slot.quantity = entry.quantity;
    player_->inventory.push_back(slot);

    rebuildContainerList();
    rebuildPlayerList();
    rebuildFilteredLists();
    selectedContainerRow_ = -1;
    scrollOffsetContainer_ = 0;

    LOGD("Took item '%s' x%u from container", entry.name.c_str(), entry.quantity);
}

void UIContainerMenu::depositItem(size_t playerIndex) {
    if (!container_ || !player_) return;
    if (playerIndex >= playerItems_.size()) return;

    const ItemEntry& entry = playerItems_[playerIndex];
    if (entry.sourceIndex >= player_->inventory.size()) return;

    InventorySlot slot = player_->inventory[entry.sourceIndex];
    if (slot.isEmpty()) return;
    if (!container_->canAddItem(slot)) return;

    container_->addItem(slot);
    player_->inventory.erase(player_->inventory.begin() +
                            static_cast<ptrdiff_t>(entry.sourceIndex));

    rebuildContainerList();
    rebuildPlayerList();
    rebuildFilteredLists();
    selectedPlayerRow_ = -1;
    scrollOffsetPlayer_ = 0;

    LOGD("Deposited item '%s' x%u into container", entry.name.c_str(), entry.quantity);
}

void UIContainerMenu::takeAll() {
    if (!container_ || !player_) return;
    while (!containerItems_.empty()) {
        takeItem(0);
    }
    LOGD("Took all items from container");
}

bool UIContainerMenu::handleBack() {
    if (!visible_) return false;
    close();
    return true;
}

bool UIContainerMenu::onKeyPress(int keyCode) {
    if (!visible_) return false;

    const auto& indices = filteredContainerIndices_[activeTab_];
    const auto& pIndices = filteredPlayerIndices_[activeTab_];

    switch (keyCode) {
        case 19:
            if (activePane_ == Pane::CONTAINER) {
                if (selectedContainerRow_ > 0) {
                    selectedContainerRow_--;
                    if (selectedContainerRow_ < scrollOffsetContainer_) {
                        scrollOffsetContainer_ = std::max(0, scrollOffsetContainer_ - 1);
                    }
                }
            } else {
                if (selectedPlayerRow_ > 0) {
                    selectedPlayerRow_--;
                    if (selectedPlayerRow_ < scrollOffsetPlayer_) {
                        scrollOffsetPlayer_ = std::max(0, scrollOffsetPlayer_ - 1);
                    }
                }
            }
            return true;
        case 20:
            if (activePane_ == Pane::CONTAINER) {
                if (selectedContainerRow_ < static_cast<int>(indices.size()) - 1) {
                    selectedContainerRow_++;
                    if (selectedContainerRow_ >= scrollOffsetContainer_ + kVisibleRows) {
                        scrollOffsetContainer_++;
                    }
                }
            } else {
                if (selectedPlayerRow_ < static_cast<int>(pIndices.size()) - 1) {
                    selectedPlayerRow_++;
                    if (selectedPlayerRow_ >= scrollOffsetPlayer_ + kVisibleRows) {
                        scrollOffsetPlayer_++;
                    }
                }
            }
            return true;
        case 21:
            activePane_ = Pane::CONTAINER;
            return true;
        case 22:
            activePane_ = Pane::PLAYER;
            return true;
        case 23:
        case 66:
            if (activePane_ == Pane::CONTAINER && selectedContainerRow_ >= 0 &&
                selectedContainerRow_ < static_cast<int>(indices.size())) {
                takeItem(indices[selectedContainerRow_]);
            } else if (activePane_ == Pane::PLAYER && selectedPlayerRow_ >= 0 &&
                       selectedPlayerRow_ < static_cast<int>(pIndices.size())) {
                depositItem(pIndices[selectedPlayerRow_]);
            }
            return true;
        case 4:
            return handleBack();
        default:
            return false;
    }
}

void UIContainerMenu::onTouchEvent(float x, float y) {
    if (!visible_) return;

    const float sx = scaleX();
    const float sy = scaleY();
    const float ox = (screenWidth_ - kCanvasWidth * sx) * 0.5f;
    const float oy = (screenHeight_ - kCanvasHeight * sy) * 0.5f;
    const float uiX = (x - ox) / sx;
    const float uiY = (y - oy) / sy;

    int tabHit = hitTestTab(uiX, uiY);
    if (tabHit >= 0) {
        activeTab_ = tabHit;
        selectedContainerRow_ = -1;
        selectedPlayerRow_ = -1;
        scrollOffsetContainer_ = 0;
        scrollOffsetPlayer_ = 0;
        return;
    }

    if (hitTestButton(uiX, uiY, kButtonTakeAllX, kButtonsY, kButtonW, kButtonH) >= 0) {
        takeAll();
        return;
    }

    if (hitTestButton(uiX, uiY, kButtonExitX, kButtonsY, kButtonW, kButtonH) >= 0) {
        close();
        return;
    }

    int containerRow = hitTestRow(uiX, uiY, kContainerPaneX, kContainerPaneY + kPaneHeaderH,
                                  kContainerPaneW, kContainerPaneH - kPaneHeaderH,
                                  scrollOffsetContainer_, filteredContainerIndices_[activeTab_]);
    if (containerRow >= 0) {
        activePane_ = Pane::CONTAINER;
        if (containerRow == selectedContainerRow_ &&
            static_cast<size_t>(containerRow) < filteredContainerIndices_[activeTab_].size()) {
            takeItem(filteredContainerIndices_[activeTab_][containerRow]);
        }
        selectedContainerRow_ = containerRow;
        selectedPlayerRow_ = -1;
        return;
    }

    int playerRow = hitTestRow(uiX, uiY, kPlayerPaneX, kPlayerPaneY + kPaneHeaderH,
                               kPlayerPaneW, kPlayerPaneH - kPaneHeaderH,
                               scrollOffsetPlayer_, filteredPlayerIndices_[activeTab_]);
    if (playerRow >= 0) {
        activePane_ = Pane::PLAYER;
        if (playerRow == selectedPlayerRow_ &&
            static_cast<size_t>(playerRow) < filteredPlayerIndices_[activeTab_].size()) {
            depositItem(filteredPlayerIndices_[activeTab_][playerRow]);
        }
        selectedPlayerRow_ = playerRow;
        selectedContainerRow_ = -1;
        return;
    }
}

int UIContainerMenu::hitTestTab(float uiX, float uiY) const {
    if (uiY < kTabsY || uiY > kTabsY + kTabH) return -1;
    for (int i = 0; i < 5; ++i) {
        if (uiX >= kTabX[i] && uiX <= kTabX[i] + kTabW[i]) return i;
    }
    return -1;
}

int UIContainerMenu::hitTestButton(float uiX, float uiY, float btnX, float btnY,
                                   float btnW, float btnH) const {
    if (uiX >= btnX && uiX <= btnX + btnW && uiY >= btnY && uiY <= btnY + btnH) return 0;
    return -1;
}

int UIContainerMenu::hitTestRow(float uiX, float uiY, float paneX, float paneY, float paneW,
                                 float paneH, int scrollOffset, const std::vector<size_t>& indices) const {
    if (uiX < paneX || uiX > paneX + paneW) return -1;
    if (uiY < paneY || uiY > paneY + paneH) return -1;

    const int maxVisible = std::min(static_cast<int>(indices.size()) - scrollOffset, kVisibleRows);
    for (int i = 0; i < maxVisible; ++i) {
        const float top = paneY + static_cast<float>(i) * kRowHeight;
        if (uiY >= top && uiY < top + kRowHeight) {
            return scrollOffset + i;
        }
    }
    return -1;
}

void UIContainerMenu::render() {
    if (!visible_ || !textRenderer_) return;

    GLboolean depthWasEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_TEST, &depthWasEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawBackground();
    drawPane(true);
    drawPane(false);
    drawWeightDisplay();
    drawTabs();
    drawButtons();

    if (depthWasEnabled) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

float UIContainerMenu::originX() const {
    return (static_cast<float>(screenWidth_) - kCanvasWidth * scaleX_) * 0.5f;
}

float UIContainerMenu::originY() const {
    return (static_cast<float>(screenHeight_) - kCanvasHeight * scaleY_) * 0.5f;
}

void UIContainerMenu::drawBackground() {
    const float alpha = fadeAlpha_;

    // Try faders__black texture
    GLuint faderTex = uiTex(std::string("menus/") + kTexFadersBlack);
    if (faderTex != 0) {
        UIDrawHelper::drawTexturedQuad(0.0f, 0.0f,
                                       static_cast<float>(screenWidth_),
                                       static_cast<float>(screenHeight_),
                                       faderTex,
                                       glm::vec4(1.0f, 1.0f, 1.0f, kDimAlpha * alpha),
                                       screenWidth_, screenHeight_);
    } else {
        // Fallback: flat dim quad
        UIDrawHelper::drawColoredQuad(0.0f, 0.0f, static_cast<float>(screenWidth_),
                                       static_cast<float>(screenHeight_),
                                       glm::vec4(0.0f, 0.0f, 0.0f, kDimAlpha * alpha),
                                       screenWidth_, screenHeight_);
    }

    // Draw generic background 9-slice over the entire canvas area
    const float sx = scaleX();
    const float sy = scaleY();
    const float ox = originX();
    const float oy = originY();

    GLuint bgSlices[9];
    int i = 0;
    for (const char* key : kGenericBgKeys) {
        bgSlices[i++] = uiTex(std::string("menus/") + key);
    }

    // Generic background from ox,oy to ox+1280*sx, oy+960*sy
    UIDrawHelper::drawGenericBackground(bgSlices, ox, oy,
                                        1280.0f * sx, 960.0f * sy,
                                        screenWidth_, screenHeight_,
                                        alpha);
}

void UIContainerMenu::drawPane(bool containerSide) {
    const float paneX = containerSide ? kContainerPaneX : kPlayerPaneX;
    const float paneY = kContainerPaneY;
    const float paneW = kContainerPaneW;
    const float paneH = kContainerPaneH;
    const float sx = scaleX();
    const float sy = scaleY();
    const float ox = originX();
    const float oy = originY();

    const float absX = ox + paneX * sx;
    const float absY = oy + paneY * sy;
    const float absW = paneW * sx;
    const float absH = paneH * sy;

    // p68: Draw pane background with generic background 9-slice
    GLuint bgSlices[9];
    int i = 0;
    for (const char* key : kGenericBgKeys) {
        bgSlices[i++] = uiTex(std::string("menus/") + key);
    }
    UIDrawHelper::drawGenericBackground(bgSlices, absX, absY, absW, absH,
                                        screenWidth_, screenHeight_, fadeAlpha_);
    // p68: Draw header with inventory header texture
    {
        const float headerAbsY = oy + paneY * sy;
        const float headerH = kPaneHeaderH * sy;

        // Determine header texture based on active tab
        const char* headerTexKey = nullptr;
        switch (activeTab_) {
            case 1: headerTexKey = kTexHeaderWeapons; break;
            case 2: headerTexKey = kTexHeaderArmor; break;
            case 3: headerTexKey = kTexHeaderAlchemy; break;
            default: headerTexKey = kTexHeaderMisc; break;
        }
        GLuint headerTex = uiTex(std::string("inventory/") + headerTexKey);
        if (headerTex != 0) {
            UIDrawHelper::drawTexturedQuad(absX, headerAbsY, absW, headerH,
                                           headerTex,
                                           glm::vec4(1.0f, 1.0f, 1.0f, fadeAlpha_),
                                           screenWidth_, screenHeight_);
        } else {
            // Fallback: flat colored header
            UIDrawHelper::drawColoredQuad(absX, headerAbsY, absW, headerH,
                                          glm::vec4(kPaneHeaderBg.x, kPaneHeaderBg.y, kPaneHeaderBg.z, kPaneHeaderBg.w * fadeAlpha_),
                                          screenWidth_, screenHeight_);
        }

        const char* headerLabel = containerSide ? "Container" : "Inventory";
        FontScope font(textRenderer_, kSmallFont);
        const float labelX = absX + 10.0f * sx;
        const float labelY = centeredTextY(headerAbsY, headerH, textRenderer_, sy);
        glm::vec4 headerColor = kHeaderTextColor;
        headerColor.w *= fadeAlpha_;
        textRenderer_->renderText(headerLabel, labelX, labelY, headerColor, sy);
    }

    const auto& indices = containerSide ? filteredContainerIndices_[activeTab_]
                                        : filteredPlayerIndices_[activeTab_];
    const auto& items = containerSide ? containerItems_ : playerItems_;
    const int scrollOff = containerSide ? scrollOffsetContainer_ : scrollOffsetPlayer_;
    const int selRow = containerSide ? selectedContainerRow_ : selectedPlayerRow_;

    const float listY = paneY + kPaneHeaderH;
    const float itemAreaY = oy + listY * sy;

    const int first = std::clamp(scrollOff, 0, static_cast<int>(indices.size()));
    const int last = std::min(static_cast<int>(indices.size()), first + kVisibleRows);

    for (int i = first; i < last; ++i) {
        const float rowTop = itemAreaY + static_cast<float>(i - first) * kRowHeight * sy;
        const bool selected = (i == selRow);

        if (selected) {
            UIDrawHelper::drawColoredQuad(absX, rowTop, absW, kRowHeight * sy,
                                          glm::vec4(kSelectedBg.x, kSelectedBg.y, kSelectedBg.z, kSelectedBg.w * fadeAlpha_),
                                          screenWidth_, screenHeight_);
            drawSelectionFrame(absX, rowTop, absW, kRowHeight * sy);
        }

        if (static_cast<size_t>(indices[i]) < items.size()) {
            const ItemEntry& entry = items[indices[i]];

            const float iconDrawX = absX + kItemIconX * sx;
            const float iconDrawY = rowTop + kItemIconY * sy;
            const float iconDrawSize = kItemIconSize * sx;

            const GLuint iconTex = getIconTexture(entry);
            if (iconTex != 0) {
                UIDrawHelper::drawTexturedQuad(iconDrawX, iconDrawY, iconDrawSize, iconDrawSize,
                                               iconTex, glm::vec4(1.0f, 1.0f, 1.0f, fadeAlpha_),
                                               screenWidth_, screenHeight_);
            }

            glm::vec4 color = kRowTextColor;
            color.w *= fadeAlpha_;
            const float textY = centeredTextY(rowTop, kRowHeight * sy, textRenderer_, sy);

            {
                FontScope font(textRenderer_, kNameFont);
                const float nameX = absX + kItemNameX * sx;
                std::string label = entry.name;
                if (entry.quantity > 1) {
                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "%s (%u)", entry.name.c_str(), entry.quantity);
                    label = buf;
                }
                textRenderer_->renderText(label, nameX, textY, color, sy);
            }

            {
                FontScope font(textRenderer_, kSmallFont);

                char buf[32];
                std::snprintf(buf, sizeof(buf), "%u", entry.value);
                textRenderer_->renderText(buf, absX + kItemValueX * sx, textY, color, sy);

                std::snprintf(buf, sizeof(buf), "%.1f", entry.weight);
                textRenderer_->renderText(buf, absX + kItemWeightX * sx, textY, color, sy);
            }
        }
    }
}

void UIContainerMenu::drawSelectionFrame(float x, float y, float w, float h) {
    // p68: Use focus 9-slice textures
    GLuint slices[9];
    slices[0] = uiTex(std::string("menus/") + kTexFocusCenter);
    slices[1] = uiTex(std::string("menus/") + kTexFocusTop);
    slices[2] = uiTex(std::string("menus/") + kTexFocusBottom);
    slices[3] = uiTex(std::string("menus/") + kTexFocusLeft);
    slices[4] = uiTex(std::string("menus/") + kTexFocusRight);
    slices[5] = uiTex(std::string("menus/") + kTexFocusTopLeft);
    slices[6] = uiTex(std::string("menus/") + kTexFocusTopRight);
    slices[7] = uiTex(std::string("menus/") + kTexFocusBottomLeft);
    slices[8] = uiTex(std::string("menus/") + kTexFocusBottomRight);

    bool hasAll = true;
    for (int i = 0; i < 9; ++i) { if (slices[i] == 0) hasAll = false; }

    if (hasAll) {
        UIDrawHelper::drawGenericBackground(slices, x, y, w, h,
                                            screenWidth_, screenHeight_, fadeAlpha_);
    } else {
        // Fallback: simple border
        const float borderW = 2.0f;
        const glm::vec4 frameColor(0.4f, 0.5f, 0.8f, 0.6f * fadeAlpha_);
        UIDrawHelper::drawBorder(x, y, w, h, borderW, frameColor, screenWidth_, screenHeight_);
    }
}

void UIContainerMenu::drawTabs() {
    const float sx = scaleX();
    const float sy = scaleY();
    const float ox = originX();
    const float oy = originY();
    const float absTabY = oy + kTabsY * sy;

    // Tab icon textures per index
    const char* tabTexKeys[5] = {
        kTexTabAll, kTexTabWeapons, kTexTabApparel, kTexTabAlchemy, kTexTabMisc
    };

    for (int i = 0; i < 5; ++i) {
        const float absX = ox + kTabX[i] * sx;
        const bool active = (i == activeTab_);

        // Draw tab background with selection texture if active
        GLuint selTex;
        if (active) {
            selTex = uiTex(std::string("menus/") + kTexSelectionOn);
        } else {
            selTex = uiTex(std::string("menus/") + kTexSelectionOff);
        }
        if (selTex != 0) {
            UIDrawHelper::drawTexturedQuad(absX, absTabY, kTabW[i] * sx, kTabH * sy,
                                           selTex,
                                           glm::vec4(1.0f, 1.0f, 1.0f, fadeAlpha_),
                                           screenWidth_, screenHeight_);
        } else {
            // Fallback: flat colored quad
            const glm::vec4 bgColor = active ? kTabActiveBg : kTabIdleBg;
            UIDrawHelper::drawColoredQuad(absX, absTabY, kTabW[i] * sx, kTabH * sy,
                                          glm::vec4(bgColor.x, bgColor.y, bgColor.z, bgColor.w * fadeAlpha_),
                                          screenWidth_, screenHeight_);
        }

        // Draw tab icon (small icon above text)
        GLuint tabIcon = uiTex(std::string("inventory/") + tabTexKeys[i]);
        const float iconSize = 20.0f * sx;
        if (tabIcon != 0) {
            const float iconX = absX + (kTabW[i] * sx - iconSize) * 0.5f;
            const float iconY = absTabY + 4.0f * sy;
            UIDrawHelper::drawTexturedQuad(iconX, iconY, iconSize, iconSize,
                                           tabIcon,
                                           glm::vec4(1.0f, 1.0f, 1.0f, fadeAlpha_),
                                           screenWidth_, screenHeight_);
        }

        // Draw tab label
        FontScope font(textRenderer_, kSmallFont);
        glm::vec4 textColor = active ? kHeaderTextColor : kRowTextColor;
        textColor.w *= fadeAlpha_;
        const float textW = textRenderer_->getTextWidth(kTabLabels[i], sy);
        const float textX = absX + (kTabW[i] * sx - textW) * 0.5f;
        const float textY = absTabY + (tabIcon != 0 ? 26.0f * sy : centeredTextY(absTabY, kTabH * sy, textRenderer_, sy));
        textRenderer_->renderText(kTabLabels[i], textX, textY, textColor, sy);
    }
}

void UIContainerMenu::drawButtons() {
    const float sx = scaleX();
    const float sy = scaleY();
    const float ox = originX();
    const float oy = originY();
    const float absBtnY = oy + kButtonsY * sy;

    struct ButtonDef {
        const char* label;
        float x;
    };
    const ButtonDef buttons[3] = {
        {"Take All", kButtonTakeAllX},
        {"Haggle", kButtonHaggleX},
        {"Exit", kButtonExitX},
    };

    FontScope font(textRenderer_, kSmallFont);

    for (int i = 0; i < 3; ++i) {
        const float absX = ox + buttons[i].x * sx;

        // p68: textured button background (cont_button_long)
        GLuint btnTex = uiTex(std::string("menus/") + kTexContButtonOff);
        if (btnTex != 0) {
            UIDrawHelper::drawTexturedQuad(absX, absBtnY, kButtonW * sx, kButtonH * sy,
                                           btnTex,
                                           glm::vec4(1.0f, 1.0f, 1.0f, fadeAlpha_),
                                           screenWidth_, screenHeight_);
        } else {
            UIDrawHelper::drawColoredQuad(absX, absBtnY, kButtonW * sx, kButtonH * sy,
                                          glm::vec4(kButtonBg.x, kButtonBg.y, kButtonBg.z, kButtonBg.w * fadeAlpha_),
                                          screenWidth_, screenHeight_);
        }

        glm::vec4 color = kRowTextColor;
        color.w *= fadeAlpha_;
        const float textW = textRenderer_->getTextWidth(buttons[i].label, sy);
        const float textX = absX + (kButtonW * sx - textW) * 0.5f;
        const float textY = centeredTextY(absBtnY, kButtonH * sy, textRenderer_, sy);
        textRenderer_->renderText(buttons[i].label, textX, textY, color, sy);
    }
}

void UIContainerMenu::drawWeightDisplay() {
    if (!container_) return;

    const float sx = scaleX();
    const float sy = scaleY();
    const float ox = originX();
    const float oy = originY();

    const float absX = ox + kWeightDisplayX * sx;
    const float absY = oy + kWeightDisplayY * sy;

    float totalWeight = container_->getTotalWeight();
    float capacity = container_->getCapacity();

    // p68: Draw weight icon
    GLuint weightIcon = uiTex(std::string("inventory/") + kTexWeightSmall);
    if (weightIcon != 0) {
        const float iconSize = 16.0f * sx;
        UIDrawHelper::drawTexturedQuad(absX, absY, iconSize, iconSize,
                                       weightIcon,
                                       glm::vec4(1.0f, 1.0f, 1.0f, fadeAlpha_),
                                       screenWidth_, screenHeight_);
    }

    char buf[64];
    std::snprintf(buf, sizeof(buf), "Container: %.1f / %.1f kg", totalWeight, capacity);

    FontScope font(textRenderer_, kSmallFont);
    glm::vec4 color = kHeaderTextColor;
    color.w *= fadeAlpha_;
    const float textX = absX + (weightIcon != 0 ? 20.0f * sx : 0.0f);
    textRenderer_->renderText(buf, textX, absY, color, sy);
}

GLuint UIContainerMenu::loadTexture(const std::string& path) {
    auto it = uiTextures_.find(path);
    if (it != uiTextures_.end()) return it->second;

    // Append .png if missing
    const std::string fullPath = "textures/ui/" + path +
        (path.size() < 4 || path.substr(path.size() - 4) != ".png" ? ".png" : "");
    const GLuint id = TextureLoader::loadTextureFromAsset(fullPath);
    if (id == 0) {
        LOGW("Missing UI texture: textures/ui/%s.png", path.c_str());
    }
    uiTextures_[path] = id;
    return id;
}

GLuint UIContainerMenu::getIconTexture(const ItemEntry& entry) {
    return ItemIconResolver::instance().getIcon(entry.itemId, entry.name, entry.category);
}

void UIContainerMenu::clearTextureCache() {
    for (const auto& entry : uiTextures_) {
        TextureLoader::deleteTexture(entry.second);
    }
    uiTextures_.clear();
    // Clear ItemIconResolver cache as well
    ItemIconResolver::instance().clearCache();
}