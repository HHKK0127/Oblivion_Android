#pragma once

#include "ui_panel.h"
#include "text_renderer.h"
#include "../game/merchant.h"
#include <memory>
#include <functional>
#include <string>
#include <vector>

struct Player;
class Inventory;

/**
 * @brief Shop UI
 *
 * Phase 11: Oblivion-style merchant trading screen
 * Displays merchant item list, player inventory,
 * and trade buttons (buy/sell) on parchment panel
 *
 * Phase 68: both lists are derived from live data (the Merchant stock built
 * from NPC_ CNTO records and the player inventory), and Buy/Sell run through
 * game/barter so the purse, the stock and the merchant cash stay in step.
 *
 * Phase 69: the traded pack is the grid Inventory that owns the player items,
 * so the shelf list, the purse and the save file all read one store.
 */
class UIShop : public UIPanel {
public:
    using TransactionCallback = std::function<void(const std::string& itemId, int quantity, bool isBuy)>;

    explicit UIShop(const std::string& title = "Shop");
    ~UIShop() override = default;

    bool initialize(TextRenderer* textRenderer);

    // Open shop. The merchant is the shop; the player supplies the Mercantile
    // skill (prices) and the inventory supplies the purse and the goods. Both
    // are borrowed, not owned.
    void openShop(std::shared_ptr<Merchant> merchant, Player* player = nullptr,
                  Inventory* playerInventory = nullptr);
    void closeShop();

    void update(float deltaTime) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    void render() override;

    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }

    // Callback for transaction results
    void setOnTransaction(TransactionCallback cb) { onTransaction = cb; }

    // Attach the traded player. It only supplies the Mercantile skill (prices),
    // so a shop without one still trades, just at base prices.
    void setPlayer(Player* player) { player_ = player; }
    Player* getPlayer() const { return player_; }

    // Attach the traded pack. Without one the screen stays read only, because
    // there is no purse to move gold through.
    void setPlayerInventory(Inventory* playerInventory) { playerInventory_ = playerInventory; }
    Inventory* getPlayerInventory() const { return playerInventory_; }

    // Player gold. With a pack attached this mirrors the Gold stack of the
    // player inventory; without one it is the display value set here.
    void setPlayerGold(float gold) { playerGold_ = gold; }
    float getPlayerGold() const;

    // Merchant cash available for buy backs.
    float getMerchantGold() const;

    // Why the last trade attempt was refused, empty when it succeeded or when no
    // attempt has been made yet. game/barter is the only place that decides a
    // refusal, so the screen reports it instead of second guessing it.
    const std::string& getLastRefusal() const { return lastRefusal_; }

private:
    TextRenderer* textRenderer = nullptr;
    std::shared_ptr<Merchant> currentMerchant;
    Player* player_ = nullptr;
    Inventory* playerInventory_ = nullptr;
    TransactionCallback onTransaction;

    float playerGold_ = 0.0f;

    enum Tab {
        MERCHANT_INVENTORY = 0,
        PLAYER_INVENTORY = 1
    };

    // One display line of the active tab. formID is 0 when a stock row does not
    // round-trip back to a base item FormID.
    struct Row {
        uint32_t formID = 0;
        std::string itemId;
        std::string name;
        int quantity = 0;
        float unitPrice = 0.0f;
    };

    // Rect of the two scroll arrows, shared by hit testing and rendering so they
    // can never drift apart.
    struct ScrollRects {
        float upX = 0.0f;
        float upY = 0.0f;
        float downX = 0.0f;
        float downY = 0.0f;
    };

    Tab currentTab = MERCHANT_INVENTORY;
    int selectedItemIndex = -1;
    int scrollOffset = 0;
    int merchantQuantity = 1;     // Purchase/sale quantity
    std::string lastRefusal_;

    int screenWidth  = 1080;
    int screenHeight = 1920;

    static constexpr float ITEM_ROW_H   = 48.0f;
    static constexpr float ITEM_ROW_GAP = 2.0f;
    static constexpr int   MAX_VISIBLE  = 8;
    static constexpr float SCROLL_BTN_W = 34.0f;
    static constexpr float SCROLL_BTN_H = 18.0f;

    std::vector<Row> buildRows() const;
    float playerMercantile() const;
    int   clampedQuantity() const;
    bool  executeTrade(bool isBuy);

    void renderTabButtons();
    void renderItemList();
    void renderTransactionPanel();
    void renderScrollButtons();

    int  hitTestTabButton(float x, float y) const;
    int  hitTestItemRow(float x, float y) const;
    ScrollRects scrollButtons() const;
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