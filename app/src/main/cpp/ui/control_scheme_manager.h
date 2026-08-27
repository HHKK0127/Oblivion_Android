#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include <android/log.h>

#define CTRL_LOG_TAG "ControlSchemeManager"
#define CTRL_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, CTRL_LOG_TAG, __VA_ARGS__)
#define CTRL_LOGI(...) __android_log_print(ANDROID_LOG_INFO, CTRL_LOG_TAG, __VA_ARGS__)
#define CTRL_LOGW(...) __android_log_print(ANDROID_LOG_WARN, CTRL_LOG_TAG, __VA_ARGS__)

/**
 * @brief 操作スキームタイプ
 */
enum class ControlSchemeType {
    TOUCH,           // タッチ操作（デフォルト）
    VIRTUAL_JOYSTICK, // 仮想ジョイスティック
    GAMEPAD          // ゲームパッド対応
};

/**
 * @brief 入力アクション
 */
enum class InputAction {
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    ATTACK,
    BLOCK,
    JUMP,
    INTERACT,
    CAST_SPELL,
    OPEN_INVENTORY,
    OPEN_MAP,
    OPEN_QUEST_LOG,
    OPEN_PAUSE_MENU,
    QUICK_SLOT_1,
    QUICK_SLOT_2,
    QUICK_SLOT_3,
    QUICK_SLOT_4,
    QUICK_SLOT_5,
    CAMERA_LOOK,
    SPRINT,
    COUNT
};

/**
 * @brief ボタン配置情報
 */
struct ButtonBinding {
    InputAction action;
    glm::vec2 position;   // ピクセル座標
    glm::vec2 size;       // ピクセルサイズ
    std::string label;
    bool visible;
    float opacity;

    ButtonBinding()
        : action(InputAction::MOVE_FORWARD), position(0.0f), size(80.0f),
          visible(true), opacity(0.7f) {}
};

/**
 * @brief 操作スキーム切替コールバック
 */
using SchemeChangeCallback = std::function<void(ControlSchemeType)>;

/**
 * @brief 操作スキーム管理
 *
 * Phase 43: 複数の操作スキームを管理し、スキーム切替と
 * カスタマイズ可能なボタン配置を提供する。
 */
class ControlSchemeManager {
public:
    ControlSchemeManager();
    ~ControlSchemeManager() = default;

    /**
     * @brief 初期化
     * @param screenWidth スクリーン幅
     * @param screenHeight スクリーン高さ
     */
    void initialize(int screenWidth, int screenHeight);

    /**
     * @brief 操作スキームを切替
     */
    void setScheme(ControlSchemeType type);

    /**
     * @brief 現在のスキームを取得
     */
    ControlSchemeType getCurrentScheme() const { return currentScheme; }

    /**
     * @brief スキーム名を取得
     */
    std::string getSchemeName(ControlSchemeType type) const;

    /**
     * @brief スキーム変更コールバックを登録
     */
    void registerSchemeChangeCallback(SchemeChangeCallback callback);

    // === ボタン配置 ===

    /**
     * @brief 現在のスキームのボタン配置を取得
     */
    const std::vector<ButtonBinding>& getButtonBindings() const;

    /**
     * @brief ボタン配置をカスタマイズ
     */
    void setButtonPosition(InputAction action, const glm::vec2& position);

    /**
     * @brief ボタンサイズを変更
     */
    void setButtonSize(InputAction action, const glm::vec2& size);

    /**
     * @brief ボタンの可視性を設定
     */
    void setButtonVisible(InputAction action, bool visible);

    /**
     * @brief ボタンの透明度を設定
     */
    void setButtonOpacity(InputAction action, float opacity);

    /**
     * @brief ボタン配置をデフォルトにリセット
     */
    void resetBindings();

    // === 入力処理 ===

    /**
     * @brief タッチ位置からアクションを検出
     * @return 該当アクション（なければ COUNT）
     */
    InputAction hitTest(float x, float y) const;

    /**
     * @brief スクリーンサイズ変更
     */
    void setScreenSize(int width, int height);

    /**
     * @brief ゲームパッド入力を処理
     * @param buttonId ボタンID
     * @param pressed 押下状態
     */
    void onGamepadButton(int buttonId, bool pressed);

    /**
     * @brief ゲームパッド軸入力を処理
     * @param axisId 軸ID
     * @param value 軸値 (-1.0 ~ 1.0)
     */
    void onGamepadAxis(int axisId, float value);

private:
    ControlSchemeType currentScheme;
    int screenWidth;
    int screenHeight;
    float uiScale;

    // スキーム別ボタン配置
    std::map<ControlSchemeType, std::vector<ButtonBinding>> schemeBindings;

    // コールバック
    std::vector<SchemeChangeCallback> changeCallbacks;

    // デフォルト配置を生成
    void setupTouchScheme();
    void setupVirtualJoystickScheme();
    void setupGamepadScheme();

    // ボタン配置ヘルパー
    ButtonBinding createBinding(InputAction action, const glm::vec2& pos,
                                const glm::vec2& size, const std::string& label);
};
