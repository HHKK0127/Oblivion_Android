#pragma once

#include <string>
#include <glm/glm.hpp>
#include "text_renderer.h"

/**
 * @brief デバッグHUD（ヘッドアップディスプレイ）
 * FPS、メモリ、フレームタイム、システム情報を表示
 */
class DebugHUD {
public:
    DebugHUD();
    ~DebugHUD();

    /**
     * @brief デバッグHUDを初期化
     * @param textRenderer テキストレンダラーへのポインタ
     */
    bool initialize(TextRenderer* textRenderer);

    /**
     * @brief フレームを更新（統計情報を計算）
     * @param deltaTime フレームタイム（ミリ秒）
     */
    void update(float deltaTime);

    /**
     * @brief デバッグ情報を描画
     */
    void render();

    /**
     * @brief 表示/非表示を切り替え
     */
    void toggle();

    /**
     * @brief 表示状態を取得
     */
    bool isVisible() const { return visible; }

    /**
     * @brief 表示状態を設定
     */
    void setVisible(bool v) { visible = v; }

    /**
     * @brief クリーンアップ
     */
    void cleanup();

private:
    TextRenderer* textRenderer;
    bool visible;

    // 統計情報
    float fps;
    float frameTimeMs;
    float avgFrameTimeMs;
    float minFrameTimeMs;
    float maxFrameTimeMs;

    int frameCount;
    float timeSinceLastUpdate;
    static constexpr float UPDATE_INTERVAL = 0.5f;  // 0.5秒ごとに更新

    // メモリ情報
    struct MemoryInfo {
        long totalMemory;
        long usedMemory;
        long freeMemory;
    };

    MemoryInfo getMemoryInfo() const;
    std::string formatMemorySize(long bytes) const;
};
