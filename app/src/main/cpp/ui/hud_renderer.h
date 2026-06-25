#pragma once

#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include <memory>

class TextRenderer;
class UISystem;

/**
 * @brief ゲーム内HUD レンダラー
 *
 * Phase 9: ゲーム画面上のプレイヤーステータス、ミニマップ、クイックスロットなどを描画する
 * オーバーレイHUDシステム。PlaceholderAssets を使用してプレースホルダー表示を行う。
 */
class HUDRenderer {
public:
    HUDRenderer();
    ~HUDRenderer();

    /**
     * @brief HUDレンダラーを初期化
     * @param textRenderer テキストレンダラーへのポインタ
     * @param uiSystem UI システムへのポインタ（オプション）
     * @param screenWidth スクリーン幅
     * @param screenHeight スクリーン高さ
     */
    bool initialize(TextRenderer* textRenderer, UISystem* uiSystem = nullptr,
                   int screenWidth = 1080, int screenHeight = 1920);

    /**
     * @brief HUDを更新
     * @param deltaTime フレーム経過時間（秒）
     */
    void update(float deltaTime);

    /**
     * @brief HUDを描画
     */
    void render();

    /**
     * @brief クリーンアップ
     */
    void cleanup();

    /**
     * @brief スクリーン解像度変更時に呼び出す
     */
    void onScreenResize(int width, int height);

    // === 表示/非表示 ===

    void setVisible(bool visible) { this->visible = visible; }
    bool isVisible() const { return visible; }

    // === プレイヤーステータス設定 ===

    /**
     * @brief プレイヤー HP を設定
     * @param current 現在 HP
     * @param max 最大 HP
     */
    void setPlayerHealth(float current, float max);

    /**
     * @brief プレイヤー MP を設定
     * @param current 現在 MP
     * @param max 最大 MP
     */
    void setPlayerMana(float current, float max);

    /**
     * @brief プレイヤースタミナを設定
     * @param current 現在スタミナ
     * @param max 最大スタミナ
     */
    void setPlayerStamina(float current, float max);

    /**
     * @brief プレイヤーレベルを設定
     */
    void setPlayerLevel(int level) { playerLevel = level; }

    /**
     * @brief プレイヤー位置を設定（ミニマップ用）
     */
    void setPlayerPosition(const glm::vec3& pos) { playerPosition = pos; }

    // === クイックスロット設定 ===

    /**
     * @brief クイックスロットアイテムを設定
     * @param slotIndex スロット番号（0-9）
     * @param itemName アイテム名
     */
    void setQuickSlotItem(int slotIndex, const std::string& itemName);

    /**
     * @brief クイックスロットクリア
     */
    void clearQuickSlots();

    // === レイアウト設定 ===

    /**
     * @brief ステータスバーの位置を設定（デフォルト：画面左下）
     */
    void setStatusBarPosition(float x, float y);

    /**
     * @brief ミニマップの位置を設定（デフォルト：画面右上）
     */
    void setMinimapPosition(float x, float y);

    /**
     * @brief クイックスロットの位置を設定（デフォルト：画面下中央）
     */
    void setQuickSlotPosition(float x, float y);

private:
    // 初期化状態
    bool initialized = false;
    bool visible = true;

    // テキストレンダラー
    TextRenderer* textRenderer = nullptr;
    UISystem* uiSystem = nullptr;

    // スクリーンサイズ
    int screenWidth = 1080;
    int screenHeight = 1920;

    // === プレイヤーステータス ===

    float healthCurrent = 100.0f;
    float healthMax = 100.0f;
    float manaCurrent = 50.0f;
    float manaMax = 50.0f;
    float staminaCurrent = 100.0f;
    float staminaMax = 100.0f;
    int playerLevel = 1;
    glm::vec3 playerPosition = glm::vec3(0.0f);

    // === クイックスロット ===

    static constexpr int NUM_QUICK_SLOTS = 10;
    std::string quickSlotItems[NUM_QUICK_SLOTS];

    // === レイアウト ===

    float statusBarX = 20.0f;
    float statusBarY = 0.0f;  // 計算時に更新
    float minimapX = 0.0f;    // 計算時に更新
    float minimapY = 20.0f;
    float quickSlotX = 0.0f;  // 計算時に更新
    float quickSlotY = 0.0f;  // 計算時に更新

    // === ヘルパー描画関数 ===

    void renderStatusBars();
    void renderMinimap();
    void renderQuickSlots();
    void renderPlayerLevel();
};
