#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include <android/log.h>

#define UI_LOG_TAG "UIComponent"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, UI_LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, UI_LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, UI_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, UI_LOG_TAG, __VA_ARGS__)

// Forward declarations
class TextRenderer;

/**
 * @brief UIイベントタイプ
 */
enum class UIEventType {
    TOUCH_DOWN,
    TOUCH_UP,
    TOUCH_MOVE,
    HOVER_ENTER,
    HOVER_LEAVE,
    FOCUS_GAIN,
    FOCUS_LOST
};

/**
 * @brief UIイベント構造体
 */
struct UIEvent {
    UIEventType type;
    float x;
    float y;
    float dx;
    float dy;
    int pointerId;
    bool consumed;

    UIEvent(UIEventType t, float px, float py, int pid = 0)
        : type(t), x(px), y(py), dx(0.0f), dy(0.0f), pointerId(pid), consumed(false) {}
};

/**
 * @brief UIアンカー（配置基準点）
 */
enum class UIAnchor {
    TOP_LEFT,      // (0,0) from top-left
    TOP_CENTER,
    TOP_RIGHT,
    CENTER_LEFT,
    CENTER,
    CENTER_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_CENTER,
    BOTTOM_RIGHT
};

/**
 * @brief テクスチャスケーリングモード
 */
enum class TextureScaleMode {
    STRETCH,              // デフォルト：quadに合わせて引き伸ばし
    PRESERVE_ASPECT_FIT,  // アスペクト比維持、全体表示（レターボックス/ピラーボックス）
    PRESERVE_ASPECT_CROP  // アスペクト比維持、quadを埋める（はみ出し許可）
};

/**
 * @brief UIコンポーネント基底クラス
 *
 * Phase 9: Graphical UI Frameworkの基盤。
 * 全てのUI要素（パネル、ボタン、テキスト、HUD等）は
 * このクラスを継承して実装する。
 */
class UIComponent : public std::enable_shared_from_this<UIComponent> {
public:
    UIComponent(const std::string& name = "UIComponent");
    virtual ~UIComponent();

    // === ライフサイクル ===

    /**
     * @brief コンポーネントを初期化
     * @return 初期化成功時true
     */
    virtual bool initialize();

    /**
     * @brief 毎フレームの更新
     * @param deltaTime 前フレームからの経過時間（秒）
     */
    virtual void update(float deltaTime);

    /**
     * @brief コンポーネントを描画
     */
    virtual void render();

    /**
     * @brief リソース解放
     */
    virtual void cleanup();

    // === イベント処理 ===

    /**
     * @brief UIイベントを処理
     * @param event UIイベント
     * @return イベントを消費した場合true
     */
    virtual bool onEvent(const UIEvent& event);

    /**
     * @brief タッチダウンイベント（簡易API）
     */
    virtual bool onTouchDown(float x, float y, int pointerId = 0);

    /**
     * @brief タッチアップイベント（簡易API）
     */
    virtual bool onTouchUp(float x, float y, int pointerId = 0);

    /**
     * @brief タッチ移動イベント（簡易API）
     */
    virtual bool onTouchMove(float x, float y, float dx, float dy, int pointerId = 0);

    // === 表示・非表示 ===

    void setVisible(bool visible) { this->visible = visible; }
    bool isVisible() const { return visible; }
    void show() { setVisible(true); }
    void hide() { setVisible(false); }
    void toggle() { setVisible(!visible); }

    // === 位置・サイズ ===

    void setPosition(float x, float y);
    void setSize(float width, float height);
    void setAnchor(UIAnchor anchor);

    glm::vec2 getPosition() const { return position; }
    glm::vec2 getSize() const { return size; }
    glm::vec2 getAbsolutePosition() const;  // アンカー考慮後の絶対座標

    /**
     * @brief 点がコンポーネント内にあるか
     */
    bool contains(float x, float y) const;

    /**
     * @brief スクリーン解像度変更時のコールバック
     */
    virtual void onScreenResize(int width, int height);

    /**
     * @brief スクリーン解像度を設定
     */
    void setScreenSize(int width, int height) { onScreenResize(width, height); }

    // === 親子関係 ===

    void setParent(std::shared_ptr<UIComponent> parent);
    std::shared_ptr<UIComponent> getParent() const { return parent.lock(); }

    void addChild(std::shared_ptr<UIComponent> child);
    void removeChild(std::shared_ptr<UIComponent> child);
    const std::vector<std::shared_ptr<UIComponent>>& getChildren() const { return children; }

    // === 識別 ===

    const std::string& getName() const { return name; }
    void setName(const std::string& n) { name = n; }

    uint32_t getId() const { return id; }

    // === 描画設定 ===

    void setBackgroundColor(const glm::vec4& color) { backgroundColor = color; }
    void setBorderColor(const glm::vec4& color) { borderColor = color; }
    void setBorderWidth(float width) { borderWidth = width; }
    void setTexture(GLuint texId) { textureId = texId; }
    GLuint getTexture() const { return textureId; }

    void setTextureScaleMode(TextureScaleMode mode) { textureScaleMode = mode; }
    TextureScaleMode getTextureScaleMode() const { return textureScaleMode; }

    bool isEnabled() const { return enabled; }
    bool isInitialized() const { return initialized; }

protected:
    // 共通描画ヘルパー
    void renderBackground() const;
    void renderBorder() const;
    void renderTexture() const;

    // 子要素の描画（可視性チェック付き）
    void renderChildren();
    void updateChildren(float deltaTime);
    bool dispatchEventToChildren(const UIEvent& event);

    // 変換
    glm::vec2 position;  // ローカル座標（ピクセル）
    glm::vec2 size;      // 幅・高さ（ピクセル）

    // スクリーンサイズ（キャッシュ）
    int screenWidth;
    int screenHeight;

    // 描画設定
    glm::vec4 backgroundColor;
    glm::vec4 borderColor;
    float borderWidth;
    GLuint textureId;
    TextureScaleMode textureScaleMode = TextureScaleMode::STRETCH;

private:
    std::string name;
    uint32_t id;

    UIAnchor anchor;

    // 状態
    bool visible;
    bool initialized;
    bool enabled;

    // 親子関係
    std::weak_ptr<UIComponent> parent;
    std::vector<std::shared_ptr<UIComponent>> children;

    // 一意ID生成用
    static uint32_t nextId;
};
