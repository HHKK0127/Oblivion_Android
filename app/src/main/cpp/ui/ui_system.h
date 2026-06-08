#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <android/log.h>
#include "ui_component.h"

#define UI_SYSTEM_LOG_TAG "UISystem"
#define SYS_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, UI_SYSTEM_LOG_TAG, __VA_ARGS__)
#define SYS_LOGI(...) __android_log_print(ANDROID_LOG_INFO, UI_SYSTEM_LOG_TAG, __VA_ARGS__)
#define SYS_LOGW(...) __android_log_print(ANDROID_LOG_WARN, UI_SYSTEM_LOG_TAG, __VA_ARGS__)
#define SYS_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, UI_SYSTEM_LOG_TAG, __VA_ARGS__)

// Forward declarations
class TextRenderer;

/**
 * @brief UIシステム管理クラス
 *
 * Phase 9: 全UIコンポーネントの親となる管理クラス。
 * - UIコンポーネントの登録・削除
 * - タッチイベントの配信（z-order考慮）
 * - 毎フレームの更新・描画呼び出し
 * - スクリーン解像度変更の通知
 *
 * 既存のRendererから呼び出され、全てのUIを統合管理する。
 */
class UISystem {
public:
    UISystem();
    ~UISystem();

    /**
     * @brief UIシステムを初期化
     * @param textRenderer 既存のテキストレンダラー（ラベル描画用）
     * @return 初期化成功時true
     */
    bool initialize(TextRenderer* textRenderer);

    /**
     * @brief クリーンアップ
     */
    void cleanup();

    // === コンポーネント管理 ===

    /**
     * @brief コンポーネントを登録
     * @param component 登録するコンポーネント
     * @param layer 描画レイヤー（大きいほど前面）
     */
    void registerComponent(std::shared_ptr<UIComponent> component, int layer = 0);

    /**
     * @brief コンポーネントを登録解除
     */
    void unregisterComponent(std::shared_ptr<UIComponent> component);

    /**
     * @brief 名前でコンポーネントを検索
     */
    std::shared_ptr<UIComponent> findComponent(const std::string& name) const;

    /**
     * @brief 全コンポーネントをクリア
     */
    void clearComponents();

    // === レイヤー管理 ===

    /**
     * @brief コンポーネントのレイヤーを変更
     */
    void setLayer(std::shared_ptr<UIComponent> component, int newLayer);

    // === 更新・描画 ===

    /**
     * @brief 全UIコンポーネントを更新
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    void update(float deltaTime);

    /**
     * @brief 全UIコンポーネントを描画（レイヤー順）
     */
    void render();

    // === イベント配信 ===

    /**
     * @brief タッチダウンイベントを配信
     * @param x スクリーンX座標
     * @param y スクリーンY座標
     * @param pointerId ポインタID
     * @return イベントを消費したコンポーネントがある場合true
     */
    bool onTouchDown(float x, float y, int pointerId = 0);

    /**
     * @brief タッチアップイベントを配信
     */
    bool onTouchUp(float x, float y, int pointerId = 0);

    /**
     * @brief タッチ移動イベントを配信
     */
    bool onTouchMove(float x, float y, float dx, float dy, int pointerId = 0);

    // === スクリーン解像度 ===

    /**
     * @brief スクリーン解像度変更時に呼び出す
     */
    void setScreenSize(int width, int height);
    glm::vec2 getScreenSize() const { return glm::vec2(static_cast<float>(screenWidth), static_cast<float>(screenHeight)); }

    // === テキストレンダラー ===

    TextRenderer* getTextRenderer() const { return textRenderer; }

    // === フォーカス管理 ===

    /**
     * @brief フォーカスを持つコンポーネントを設定
     */
    void setFocusedComponent(std::shared_ptr<UIComponent> component);
    std::shared_ptr<UIComponent> getFocusedComponent() const { return focusedComponent.lock(); }

    // === 便利メソッド ===

    /**
     * @brief 全コンポーネントの可視性を一括設定
     */
    void setAllVisible(bool visible);

    /**
     * @brief 特定レイヤーのコンポーネントのみ可視にする
     */
    void showOnlyLayer(int layer);

    /**
     * @brief 登録済みコンポーネント数を取得
     */
    size_t getComponentCount() const;

private:
    // レイヤー別コンポーネント管理（key: layer, value: components）
    std::map<int, std::vector<std::shared_ptr<UIComponent>>> layers;

    // 名前からの高速検索用
    std::map<std::string, std::weak_ptr<UIComponent>> nameMap;

    // テキストレンダラー（既存資産を利用）
    TextRenderer* textRenderer;

    // フォーカス管理
    std::weak_ptr<UIComponent> focusedComponent;

    // スクリーンサイズ
    int screenWidth;
    int screenHeight;

    bool initialized;

    // イベント配信ヘルパー（前面レイヤー優先）
    bool dispatchEvent(const UIEvent& event);

    // レイヤーをソートして描画
    void renderLayers();

    // レイヤーからコンポーネントを削除
    void removeFromLayer(std::shared_ptr<UIComponent> component, int layer);
};
