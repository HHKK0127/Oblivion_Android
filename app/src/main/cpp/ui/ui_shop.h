#pragma once

#include "ui_panel.h"
#include "text_renderer.h"
#include "../game/merchant.h"
#include <memory>
#include <functional>

/**
 * @brief ショップUI
 *
 * Phase 11: Oblivion風のマーチャント取引画面
 * マーチャント商品一覧、プレイヤーインベントリ、
 * 取引ボタン（購入/売却）を羊皮紙パネルで表示
 */
class UIShop : public UIPanel {
public:
    using TransactionCallback = std::function<void(const std::string& itemId, int quantity, bool isBuy)>;

    explicit UIShop(const std::string& title = "Shop");
    ~UIShop() override = default;

    bool initialize(TextRenderer* textRenderer);

    // Shop を開く
    void openShop(std::shared_ptr<Merchant> merchant);
    void closeShop();

    void update(float deltaTime) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    void render() override;

    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }

    // Callback for transaction results
    void setOnTransaction(TransactionCallback cb) { onTransaction = cb; }

    // Player inventory and gold
    void setPlayerGold(float gold) { playerGold_ = gold; }
    float getPlayerGold() const { return playerGold_; }

private:
    TextRenderer* textRenderer = nullptr;
    std::shared_ptr<Merchant> currentMerchant;
    TransactionCallback onTransaction;

    float playerGold_ = 0.0f;

    enum Tab {
        MERCHANT_INVENTORY = 0,
        PLAYER_INVENTORY = 1
    };

    Tab currentTab = MERCHANT_INVENTORY;
    int selectedItemIndex = -1;
    int scrollOffset = 0;
    int merchantQuantity = 1;     // 購入数量

    int screenWidth  = 1080;
    int screenHeight = 1920;

    static constexpr float ITEM_ROW_H   = 48.0f;
    static constexpr float ITEM_ROW_GAP = 2.0f;
    static constexpr int   MAX_VISIBLE  = 8;

    void renderTabButtons();
    void renderItemList();
    void renderTransactionPanel();
    void renderScrollButtons();

    int  hitTestTabButton(float x, float y) const;
    int  hitTestItemRow(float x, float y) const;
    bool hitTestScrollUp(float x, float y) const;
    bool hitTestScrollDown(float x, float y) const;
    bool hitTestBuyButton(float x, float y) const;
    bool hitTestSellButton(float x, float y) const;
    bool hitTestIncreaseQty(float x, float y) const;
    bool hitTestDecreaseQty(float x, float y) const;

    // Layout helpers
    float getTabY() const;
    float getItemListY() const;
    float getTransactionPanelY() const;
};
