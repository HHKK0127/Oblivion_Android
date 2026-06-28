#include "ui_shop.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cstdio>
#include <algorithm>

UIShop::UIShop(const std::string& title)
    : UIPanel(title.empty() ? "Shop" : title) {
    // 商品棚風の背景色
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

void UIShop::openShop(std::shared_ptr<Merchant> merchant) {
    currentMerchant = merchant;
    currentTab = MERCHANT_INVENTORY;
    selectedItemIndex = -1;
    scrollOffset = 0;
    merchantQuantity = 1;
    setVisible(true);
}

void UIShop::closeShop() {
    currentMerchant = nullptr;
    setVisible(false);
}

void UIShop::update(float deltaTime) {
    UIPanel::update(deltaTime);
}

bool UIShop::onTouchDown(float x, float y, int pointerId) {
    if (!isVisible() || !isEnabled()) return false;

    if (isInsideCloseButton(x, y)) {
        closeShop();
        return true;
    }

    if (!currentMerchant) return false;

    // Tab 切り替え
    int tabIdx = hitTestTabButton(x, y);
    if (tabIdx >= 0) {
        currentTab = static_cast<Tab>(tabIdx);
        selectedItemIndex = -1;
        scrollOffset = 0;
        return true;
    }

    // スクロール
    if (hitTestScrollUp(x, y)) {
        scrollOffset = std::max(0, scrollOffset - 1);
        return true;
    }
    if (hitTestScrollDown(x, y)) {
        const auto& items = currentTab == MERCHANT_INVENTORY
                              ? currentMerchant->getInventory()
                              : std::vector<MerchantItem>();
        int maxScroll = std::max(0, (int)items.size() - MAX_VISIBLE);
        scrollOffset = std::min(scrollOffset + 1, maxScroll);
        return true;
    }

    // アイテム選択
    int row = hitTestItemRow(x, y);
    if (row >= 0) {
        selectedItemIndex = scrollOffset + row;
        return true;
    }

    // 購入/売却ボタン
    if (currentTab == MERCHANT_INVENTORY && hitTestBuyButton(x, y)) {
        if (selectedItemIndex >= 0) {
            const auto& items = currentMerchant->getInventory();
            if (selectedItemIndex < (int)items.size()) {
                const auto& item = items[selectedItemIndex];
                float price = currentMerchant->calculateBuyPrice(item.itemId);
                if (playerGold_ >= price && item.quantity > 0) {
                    if (onTransaction) {
                        onTransaction(item.itemId, merchantQuantity, true);
                    }
                    playerGold_ -= price * merchantQuantity;
                    merchantQuantity = 1;
                }
            }
        }
        return true;
    }

    if (currentTab == PLAYER_INVENTORY && hitTestSellButton(x, y)) {
        // TODO: Implement selling from player inventory
        return true;
    }

    // 数量増減ボタン
    if (hitTestIncreaseQty(x, y)) {
        merchantQuantity = std::min(merchantQuantity + 1, 99);
        return true;
    }
    if (hitTestDecreaseQty(x, y)) {
        merchantQuantity = std::max(merchantQuantity - 1, 1);
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
    renderScrollButtons();

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

    const auto& items = currentMerchant->getInventory();
    int itemCount = (int)items.size();
    int visEnd = std::min(scrollOffset + MAX_VISIBLE, itemCount);

    // Item list header
    PlaceholderAssets::drawPanel(cp.x, ty, getContentSize().x, 20.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText(currentTab == MERCHANT_INVENTORY ? "Merchant Items" : "Your Items",
        cp.x + 6.0f, ty + 2.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.65f);
    ty += 22.0f;

    // Render items
    for (int i = scrollOffset; i < visEnd; ++i) {
        const auto& item = items[i];
        bool sel = (i == selectedItemIndex);

        glm::vec3 bg = sel
            ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK)
            : glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.85f;
        glm::vec3 border = sel
            ? glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT)
            : glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT) * 0.7f;

        float ry = ty + (i - scrollOffset) * (ITEM_ROW_H + ITEM_ROW_GAP);
        PlaceholderAssets::drawPanel(cp.x, ry, getContentSize().x, ITEM_ROW_H, bg, border);

        // Item name and price
        float price = currentMerchant->calculateBuyPrice(item.itemId);
        char priceStr[64];
        snprintf(priceStr, sizeof(priceStr), "%s (%.0fgp) [%d]",
                 item.itemName.c_str(), price, item.quantity);

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
    float panelH = 80.0f;

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

    const auto& items = currentMerchant->getInventory();
    if (selectedItemIndex >= (int)items.size()) return;

    const auto& item = items[selectedItemIndex];
    float price = currentMerchant->calculateBuyPrice(item.itemId);

    // Display selected item info
    char infoStr[128];
    snprintf(infoStr, sizeof(infoStr), "%s: %.0fgp each | Qty: %d",
             item.itemName.c_str(), price, merchantQuantity);
    textRenderer->renderText(infoStr,
        cp.x + 6.0f, py + 6.0f,
        glm::vec3(0.1f, 0.08f, 0.06f), 0.65f);

    // Player gold
    char goldStr[64];
    snprintf(goldStr, sizeof(goldStr), "Your Gold: %.0f", playerGold_);
    textRenderer->renderText(goldStr,
        cp.x + 6.0f, py + 26.0f,
        glm::vec3(0.3f, 0.25f, 0.1f), 0.65f);

    // Quantity +/- buttons
    PlaceholderAssets::drawPanel(cp.x + 6.0f, py + 45.0f, 40.0f, 24.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("-",
        cp.x + 16.0f, py + 50.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.7f);

    char qtyStr[16];
    snprintf(qtyStr, sizeof(qtyStr), "%d", merchantQuantity);
    textRenderer->renderText(qtyStr,
        cp.x + 52.0f, py + 50.0f,
        glm::vec3(0.1f, 0.08f, 0.06f), 0.7f);

    PlaceholderAssets::drawPanel(cp.x + 86.0f, py + 45.0f, 40.0f, 24.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("+",
        cp.x + 100.0f, py + 50.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.7f);

    // Buy/Sell buttons
    glm::vec4 buyButtonColor = (playerGold_ >= price * merchantQuantity)
                                ? glm::vec4(PlaceholderAssets::Colors::GOLD_HIGHLIGHT.x, PlaceholderAssets::Colors::GOLD_HIGHLIGHT.y, PlaceholderAssets::Colors::GOLD_HIGHLIGHT.z, 1.0f)
                                : glm::vec4(0.5f, 0.5f, 0.5f, 0.7f);
    UIDrawHelper::drawColoredQuad(cp.x + panelW - 130.0f, py + 45.0f, 55.0f, 24.0f, buyButtonColor,
        screenWidth, screenHeight);
    textRenderer->renderText("Buy",
        cp.x + panelW - 115.0f, py + 50.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.65f);

    UIDrawHelper::drawColoredQuad(cp.x + panelW - 70.0f, py + 45.0f, 55.0f, 24.0f,
        glm::vec4(PlaceholderAssets::Colors::BROWN_ACCENT.x, PlaceholderAssets::Colors::BROWN_ACCENT.y, PlaceholderAssets::Colors::BROWN_ACCENT.z, 0.8f),
        screenWidth, screenHeight);
    textRenderer->renderText("Sell",
        cp.x + panelW - 55.0f, py + 50.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.65f);
}

void UIShop::renderScrollButtons() {
    // Scroll buttons (簡略版)
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

bool UIShop::hitTestScrollUp(float x, float y) const {
    // TODO: Implement scroll button hit test
    return false;
}

bool UIShop::hitTestScrollDown(float x, float y) const {
    // TODO: Implement scroll button hit test
    return false;
}

bool UIShop::hitTestBuyButton(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float py = getTransactionPanelY();
    float buttonX = cp.x + getContentSize().x - 130.0f;
    float buttonY = py + 45.0f;

    return x >= buttonX && x <= buttonX + 55.0f &&
           y >= buttonY && y <= buttonY + 24.0f;
}

bool UIShop::hitTestSellButton(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float py = getTransactionPanelY();
    float buttonX = cp.x + getContentSize().x - 70.0f;
    float buttonY = py + 45.0f;

    return x >= buttonX && x <= buttonX + 55.0f &&
           y >= buttonY && y <= buttonY + 24.0f;
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
