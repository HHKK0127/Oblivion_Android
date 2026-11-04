#include "ui_shop.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include "../game/barter.h"
#include "../game/inventory.h"
#include "../game/item_catalog.h"
#include "../game/player.h"
#include <GLES3/gl3.h>
#include <cstdio>
#include <algorithm>

UIShop::UIShop(const std::string& title)
    : UIPanel(title.empty() ? "Shop" : title) {
    // Shelf-style background color
    setBackgroundColor(glm::vec4(
        PlaceholderAssets::Colors::PARCHMENT_DARK.x * 0.8f,
        PlaceholderAssets::Colors::PARCHMENT_DARK.y * 0.75f,
        PlaceholderAssets::Colors::PARCHMENT_DARK.z * 0.7f, 0.96f));
    setBorderColor(glm::vec4(
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.x,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.y,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.z, 1.0f));
    setBorderWidth(3.0f);
    setTitleBarColor(glm::vec4(0.2f, 0.15f, 0.1f, 0.85f));
    setCloseButtonVisible(true);
    setDraggable(false);
}

bool UIShop::initialize(TextRenderer* tr) {
    if (!tr) return false;
    textRenderer = tr;
    return UIPanel::initialize();
}

void UIShop::openShop(std::shared_ptr<Merchant> merchant, Player* player,
                      Inventory* playerInventory) {
    currentMerchant = merchant;
    if (player != nullptr) player_ = player;
    if (playerInventory != nullptr) playerInventory_ = playerInventory;
    currentTab = MERCHANT_INVENTORY;
    selectedItemIndex = -1;
    scrollOffset = 0;
    merchantQuantity = 1;
    lastRefusal_.clear();
    setVisible(true);
}

void UIShop::closeShop() {
    currentMerchant = nullptr;
    selectedItemIndex = -1;
    scrollOffset = 0;
    merchantQuantity = 1;
    lastRefusal_.clear();
    setVisible(false);
}

void UIShop::update(float deltaTime) {
    UIPanel::update(deltaTime);
}

float UIShop::getPlayerGold() const {
    if (playerInventory_ != nullptr) {
        return static_cast<float>(game::playerGold(*playerInventory_));
    }
    return playerGold_;
}

float UIShop::getMerchantGold() const {
    if (!currentMerchant) return 0.0f;
    return currentMerchant->getGold();
}

float UIShop::playerMercantile() const {
    if (player_ == nullptr) return 0.0f;
    return static_cast<float>(player_->skills.Mercantile);
}

// Rows are derived on demand instead of cached: the stock, the purse and the
// inventory all change on trade, so a cache would need invalidating in every
// path that calls into game/barter.
std::vector<UIShop::Row> UIShop::buildRows() const {
    std::vector<Row> rows;
    if (!currentMerchant) return rows;

    if (currentTab == MERCHANT_INVENTORY) {
        for (const MerchantItem& item : currentMerchant->getInventory()) {
            if (item.quantity <= 0) continue;
            Row row;
            row.formID = game::formIDFromItemId(item.itemId);
            row.itemId = item.itemId;
            row.name = item.itemName;
            row.quantity = item.quantity;
            row.unitPrice = currentMerchant->calculateBuyPrice(item.itemId);
            rows.push_back(row);
        }
        return rows;
    }

    if (playerInventory_ == nullptr) return rows;
    for (const InventorySlot& slot : playerInventory_->getAllSlots()) {
        // The pack is a fixed 60 row grid, so empty rows are holes, not items.
        if (slot.isEmpty()) continue;
        if (slot.item.itemId == game::kGoldFormID) continue;  // Currency is never sold
        Row row;
        row.formID = slot.item.itemId;
        row.itemId = game::itemIdFromFormID(slot.item.itemId);
        row.name = slot.item.name.empty() ? row.itemId : slot.item.name;
        row.quantity = static_cast<int>(slot.quantity);
        // Canonical pricing uses the merchant's effective Barter skill and the
        // merchant's disposition toward the player, not the player's own
        // Mercantile skill (see FUN_00488e50 / FUN_005E11A0 / FUN_005E1200).
        row.unitPrice = Merchant::sellPriceForValue(
            static_cast<float>(slot.item.value), currentMerchant->effectiveBarterSkill(),
            currentMerchant->getDisposition());
        rows.push_back(row);
    }
    return rows;
}

int UIShop::clampedQuantity() const {
    const std::vector<Row> rows = buildRows();
    if (selectedItemIndex < 0 || selectedItemIndex >= static_cast<int>(rows.size())) return 1;
    const int held = rows[selectedItemIndex].quantity;
    if (held <= 0) return 1;
    return std::min(std::max(merchantQuantity, 1), held);
}

bool UIShop::executeTrade(bool isBuy) {
    // Both directions move gold through the player, so read-only mode cannot
    // trade at all.
    if (!currentMerchant || playerInventory_ == nullptr) return false;

    const std::vector<Row> rows = buildRows();
    if (selectedItemIndex < 0 || selectedItemIndex >= static_cast<int>(rows.size())) return false;

    const Row& row = rows[selectedItemIndex];
    const int count = clampedQuantity();

    // No haggle slider exists in this UI yet, so the offset stays at zero,
    // matching the original's default [player+0x11C] value.
    const game::BarterResult result = isBuy
        ? game::buyFromMerchant(*playerInventory_, *currentMerchant, row.itemId, count)
        : game::sellToMerchant(*playerInventory_, *currentMerchant, row.formID, count);

    // Report the refusal instead of swallowing it: a tapped Buy/Sell that
    // silently does nothing looks like a broken screen.
    lastRefusal_ = result.reason;
    if (!result.ok) return false;

    if (onTransaction) onTransaction(row.itemId, count, isBuy);

    // The traded row may be gone, so drop the selection rather than pointing at
    // whatever row moved up into its place.
    selectedItemIndex = -1;
    merchantQuantity = 1;
    return true;
}

bool UIShop::onTouchDown(float x, float y, int pointerId) {
    (void)pointerId;
    if (!isVisible() || !isEnabled()) return false;

    if (isInsideCloseButton(x, y)) {
        closeShop();
        return true;
    }

    if (!currentMerchant) return false;

    // Tab switching
    int tabIdx = hitTestTabButton(x, y);
    if (tabIdx >= 0) {
        currentTab = static_cast<Tab>(tabIdx);
        selectedItemIndex = -1;
        scrollOffset = 0;
        merchantQuantity = 1;
        lastRefusal_.clear();
        return true;
    }

    const int rowCount = static_cast<int>(buildRows().size());
    const int maxScroll = std::max(0, rowCount - MAX_VISIBLE);

    if (hitTestScrollUp(x, y)) {
        scrollOffset = std::max(0, scrollOffset - 1);
        return true;
    }
    if (hitTestScrollDown(x, y)) {
        scrollOffset = std::min(scrollOffset + 1, maxScroll);
        return true;
    }
    // The list shrank (a trade consumed a row) and kept the offset out of range.
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;

    // Item selection
    int row = hitTestItemRow(x, y);
    if (row >= 0) {
        const int index = scrollOffset + row;
        if (index < rowCount) {
            if (index != selectedItemIndex) {
                merchantQuantity = 1;
                // The refusal belonged to the row that was selected before.
                lastRefusal_.clear();
            }
            selectedItemIndex = index;
        }
        return true;
    }

    // Quantity +/- buttons act on the selected row only
    if (selectedItemIndex >= 0) {
        if (hitTestDecreaseQty(x, y)) {
            merchantQuantity = std::max(1, merchantQuantity - 1);
            lastRefusal_.clear();
            return true;
        }
        if (hitTestIncreaseQty(x, y)) {
            const std::vector<Row> rows = buildRows();
            if (selectedItemIndex < static_cast<int>(rows.size())) {
                const int held = std::max(1, rows[selectedItemIndex].quantity);
                merchantQuantity = std::min(merchantQuantity + 1, held);
            }
            lastRefusal_.clear();
            return true;
        }
    }

    // Buy/Sell button
    if (currentTab == MERCHANT_INVENTORY && hitTestBuyButton(x, y)) {
        executeTrade(true);
        return true;
    }
    if (currentTab == PLAYER_INVENTORY && hitTestSellButton(x, y)) {
        executeTrade(false);
        return true;
    }

    return true;  // Consume all touches while open
}

void UIShop::render() {
    if (!isVisible() || !currentMerchant) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    UIPanel::render();

    renderTabButtons();
    renderItemList();
    renderTransactionPanel();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

// ─── rendering sections ────────────────────────────────────────────────────────

void UIShop::renderTabButtons() {
    if (!textRenderer) return;
    glm::vec2 cp = getContentPosition();
    float tabW = getContentSize().x / 2.0f;
    float tabH = 28.0f;
    float ty = getTabY();

    // "Merchant" tab
    glm::vec3 merchantBg = (currentTab == MERCHANT_INVENTORY)
                            ? glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT)
                            : glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT);
    PlaceholderAssets::drawPanel(cp.x, ty, tabW - 1.0f, tabH, merchantBg,
        glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT));
    textRenderer->renderText("Merchant",
        cp.x + 6.0f, ty + 6.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.7f);

    // "Inventory" tab
    glm::vec3 inventoryBg = (currentTab == PLAYER_INVENTORY)
                            ? glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT)
                            : glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT);
    PlaceholderAssets::drawPanel(cp.x + tabW + 1.0f, ty, tabW - 1.0f, tabH, inventoryBg,
        glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT));
    textRenderer->renderText("Inventory",
        cp.x + tabW + 6.0f, ty + 6.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.7f);
}

void UIShop::renderItemList() {
    if (!textRenderer || !currentMerchant) return;
    glm::vec2 cp = getContentPosition();
    float ty = getItemListY();

    const std::vector<Row> rows = buildRows();
    const int rowCount = static_cast<int>(rows.size());
    const int maxScroll = std::max(0, rowCount - MAX_VISIBLE);
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;
    const int visEnd = std::min(scrollOffset + MAX_VISIBLE, rowCount);

    // Item list header
    PlaceholderAssets::drawPanel(cp.x, ty, getContentSize().x, 20.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText(currentTab == MERCHANT_INVENTORY ? "Merchant Items" : "Your Items",
        cp.x + 6.0f, ty + 2.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.65f);
    renderScrollButtons();
    ty += 22.0f;

    if (rowCount == 0) {
        textRenderer->renderText(currentTab == MERCHANT_INVENTORY ? "This merchant has nothing to sell"
                                                                  : "You are carrying nothing to sell",
            cp.x + 10.0f, ty + 14.0f,
            glm::vec3(0.3f, 0.3f, 0.3f), 0.65f);
        return;
    }

    // Render items
    for (int i = scrollOffset; i < visEnd; ++i) {
        const Row& row = rows[i];
        bool sel = (i == selectedItemIndex);

        glm::vec3 bg = sel
            ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK)
            : glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.85f;
        glm::vec3 border = sel
            ? glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT)
            : glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT) * 0.7f;

        float ry = ty + (i - scrollOffset) * (ITEM_ROW_H + ITEM_ROW_GAP);
        PlaceholderAssets::drawPanel(cp.x, ry, getContentSize().x, ITEM_ROW_H, bg, border);

        // The unit price shown in the list always matches what a trade charges,
        // and a row the trade path would refuse says so instead of printing a
        // price that cannot be paid.
        char priceStr[128];
        if (game::tradeGold(row.unitPrice, 1) > 0) {
            snprintf(priceStr, sizeof(priceStr), "%s  (%.0fgp)  x%d",
                     row.name.c_str(), row.unitPrice, row.quantity);
        } else {
            snprintf(priceStr, sizeof(priceStr), "%s  (Not for sale)  x%d",
                     row.name.c_str(), row.quantity);
        }

        textRenderer->renderText(priceStr,
            cp.x + 10.0f, ry + 14.0f,
            sel ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT)
                : glm::vec3(0.1f, 0.1f, 0.1f),
            0.7f);
    }
}

void UIShop::renderTransactionPanel() {
    if (!textRenderer || !currentMerchant) return;
    glm::vec2 cp = getContentPosition();
    float py = getTransactionPanelY();
    float panelW = getContentSize().x;
    float panelH = 90.0f;
    const bool buying = (currentTab == MERCHANT_INVENTORY);

    // Transaction panel background
    PlaceholderAssets::drawPanel(cp.x, py, panelW, panelH,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.8f,
        glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT));

    if (selectedItemIndex < 0) {
        textRenderer->renderText("Select an item to trade",
            cp.x + 6.0f, py + 6.0f,
            glm::vec3(0.3f, 0.3f, 0.3f), 0.65f);
        return;
    }

    const std::vector<Row> rows = buildRows();
    if (selectedItemIndex >= static_cast<int>(rows.size())) return;

    const Row& row = rows[selectedItemIndex];
    const int quantity = clampedQuantity();
    // Priced with the same function the trade itself uses, so the total on
    // screen is exactly the gold that will change hands.
    const bool priced = game::tradeGold(row.unitPrice, 1) > 0;
    const int32_t total = priced ? game::tradeGold(row.unitPrice, quantity) : 0;

    // Display selected item info
    char infoStr[160];
    snprintf(infoStr, sizeof(infoStr), "%s: %.0fgp each | Qty: %d | Total: %dgp",
             row.name.c_str(), row.unitPrice, quantity, static_cast<int>(total));
    textRenderer->renderText(infoStr,
        cp.x + 6.0f, py + 6.0f,
        glm::vec3(0.1f, 0.08f, 0.06f), 0.65f);

    // Player gold
    char goldStr[128];
    snprintf(goldStr, sizeof(goldStr), "Your Gold: %.0f", getPlayerGold());
    textRenderer->renderText(goldStr,
        cp.x + 6.0f, py + 26.0f,
        glm::vec3(0.3f, 0.25f, 0.1f), 0.65f);

    char merchantGoldStr[128];
    snprintf(merchantGoldStr, sizeof(merchantGoldStr), "Merchant Gold: %.0f", getMerchantGold());
    textRenderer->renderText(merchantGoldStr,
        cp.x + 150.0f, py + 26.0f,
        glm::vec3(0.3f, 0.25f, 0.1f), 0.65f);

    // Quantity +/- buttons
    PlaceholderAssets::drawPanel(cp.x + 6.0f, py + 45.0f, 40.0f, 24.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("-",
        cp.x + 16.0f, py + 50.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.7f);

    char qtyStr[16];
    snprintf(qtyStr, sizeof(qtyStr), "%d", quantity);
    textRenderer->renderText(qtyStr,
        cp.x + 52.0f, py + 50.0f,
        glm::vec3(0.1f, 0.08f, 0.06f), 0.7f);

    PlaceholderAssets::drawPanel(cp.x + 86.0f, py + 45.0f, 40.0f, 24.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("+",
        cp.x + 100.0f, py + 50.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.7f);

    // The action button of the active tab; the other direction belongs to the
    // other tab, so it is not offered here. A row with no price has no button,
    // because tapping one could never do anything.
    const bool affordable = priced && (buying ? (getPlayerGold() >= static_cast<float>(total))
                                              : (getMerchantGold() >= static_cast<float>(total)));
    glm::vec4 actionColor = affordable
        ? glm::vec4(PlaceholderAssets::Colors::GOLD_HIGHLIGHT.x,
                    PlaceholderAssets::Colors::GOLD_HIGHLIGHT.y,
                    PlaceholderAssets::Colors::GOLD_HIGHLIGHT.z, 1.0f)
        : glm::vec4(0.5f, 0.5f, 0.5f, 0.7f);
    const float buttonX = cp.x + panelW - 70.0f;
    UIDrawHelper::drawColoredQuad(buttonX, py + 45.0f, 55.0f, 24.0f, actionColor,
        screenWidth, screenHeight);

    char actionStr[32];
    snprintf(actionStr, sizeof(actionStr), priced ? (buying ? "Buy" : "Sell") : "No price");
    textRenderer->renderText(actionStr,
        buttonX + 8.0f, py + 50.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.65f);

    // The refusal game/barter reported for the last attempt. Without it a
    // rejected trade would look like a dead button.
    if (!lastRefusal_.empty()) {
        textRenderer->renderText(lastRefusal_,
            cp.x + 6.0f, py + 72.0f,
            glm::vec3(0.55f, 0.15f, 0.12f), 0.6f);
    }
}

void UIShop::renderScrollButtons() {
    if (!textRenderer) return;
    const ScrollRects r = scrollButtons();
    const int rowCount = static_cast<int>(buildRows().size());

    const bool canUp = scrollOffset > 0;
    const bool canDown = scrollOffset + MAX_VISIBLE < rowCount;

    glm::vec3 border = glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    glm::vec3 upColor = canUp ? glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT)
                              : glm::vec3(0.35f, 0.32f, 0.28f);
    glm::vec3 downColor = canDown ? glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT)
                                  : glm::vec3(0.35f, 0.32f, 0.28f);

    PlaceholderAssets::drawPanel(r.upX, r.upY, SCROLL_BTN_W, SCROLL_BTN_H, upColor, border);
    textRenderer->renderText("^", r.upX + 12.0f, r.upY + 1.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.6f);

    PlaceholderAssets::drawPanel(r.downX, r.downY, SCROLL_BTN_W, SCROLL_BTN_H, downColor, border);
    textRenderer->renderText("v", r.downX + 12.0f, r.downY + 1.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.6f);
}

// ─── hit testing ──────────────────────────────────────────────────────────────

int UIShop::hitTestTabButton(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float tabW = getContentSize().x / 2.0f;
    float tabH = 28.0f;
    float ty = getTabY();

    if (x >= cp.x && x <= cp.x + tabW && y >= ty && y <= ty + tabH) {
        return 0;  // MERCHANT_INVENTORY
    }
    if (x >= cp.x + tabW && x <= cp.x + getContentSize().x && y >= ty && y <= ty + tabH) {
        return 1;  // PLAYER_INVENTORY
    }
    return -1;
}

int UIShop::hitTestItemRow(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float itemListY = getItemListY() + 22.0f;  // After header

    for (int i = 0; i < MAX_VISIBLE; ++i) {
        float ry = itemListY + i * (ITEM_ROW_H + ITEM_ROW_GAP);
        if (x >= cp.x && x <= cp.x + getContentSize().x &&
            y >= ry && y <= ry + ITEM_ROW_H) {
            return i;
        }
    }
    return -1;
}

UIShop::ScrollRects UIShop::scrollButtons() const {
    ScrollRects r;
    glm::vec2 cp = getContentPosition();
    const float headerRight = cp.x + getContentSize().x;
    r.upX = headerRight - (SCROLL_BTN_W * 2.0f + 8.0f);
    r.downX = headerRight - (SCROLL_BTN_W + 4.0f);
    r.upY = getItemListY() + 1.0f;
    r.downY = r.upY;
    return r;
}

bool UIShop::hitTestScrollUp(float x, float y) const {
    const ScrollRects r = scrollButtons();
    return x >= r.upX && x <= r.upX + SCROLL_BTN_W &&
           y >= r.upY && y <= r.upY + SCROLL_BTN_H;
}

bool UIShop::hitTestScrollDown(float x, float y) const {
    const ScrollRects r = scrollButtons();
    return x >= r.downX && x <= r.downX + SCROLL_BTN_W &&
           y >= r.downY && y <= r.downY + SCROLL_BTN_H;
}

bool UIShop::hitTestBuyButton(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float py = getTransactionPanelY();
    float buttonX = cp.x + getContentSize().x - 70.0f;
    float buttonY = py + 45.0f;

    return x >= buttonX && x <= buttonX + 55.0f &&
           y >= buttonY && y <= buttonY + 24.0f;
}

bool UIShop::hitTestSellButton(float x, float y) const {
    return hitTestBuyButton(x, y);
}

bool UIShop::hitTestIncreaseQty(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float py = getTransactionPanelY();
    float buttonX = cp.x + 86.0f;
    float buttonY = py + 45.0f;

    return x >= buttonX && x <= buttonX + 40.0f &&
           y >= buttonY && y <= buttonY + 24.0f;
}

bool UIShop::hitTestDecreaseQty(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float py = getTransactionPanelY();
    float buttonX = cp.x + 6.0f;
    float buttonY = py + 45.0f;

    return x >= buttonX && x <= buttonX + 40.0f &&
           y >= buttonY && y <= buttonY + 24.0f;
}

// ─── layout helpers ───────────────────────────────────────────────────────────

float UIShop::getTabY() const {
    return getContentPosition().y + 4.0f;
}

float UIShop::getItemListY() const {
    return getContentPosition().y + 36.0f;
}

float UIShop::getTransactionPanelY() const {
    return getContentPosition().y + getContentSize().y - 90.0f;
}