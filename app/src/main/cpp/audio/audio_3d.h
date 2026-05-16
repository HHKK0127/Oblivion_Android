#pragma once

#include <glm/glm.hpp>
#include <android/log.h>

// Note: OpenAL not needed for Java MediaPlayer approach via JNI

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGW
#undef LOGE

#define LOG_TAG "Audio3D"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * @brief 3D空間オーディオ処理
 * リスナー位置・方向の管理と、距離減衰モデルの設定
 *
 * OpenAL の 3D空間音声機能を統一的に管理
 */
class Audio3D {
public:
    /**
     * @brief 距離減衰モデルの種類
     */
    enum class DistanceModel {
        INVERSE_DISTANCE,           // 距離に応じた自然な減衰
        INVERSE_DISTANCE_CLAMPED,   // クランプ付き（最小・最大距離設定）
        LINEAR_DISTANCE,            // 線形減衰
        EXPONENT_DISTANCE           // 指数関数的減衰
    };

    /**
     * @brief コンストラクタ
     */
    Audio3D();

    /**
     * @brief デストラクタ
     */
    ~Audio3D();

    /**
     * @brief リスナー位置を設定
     * @param pos ワールド座標
     */
    void setListenerPosition(const glm::vec3& pos);

    /**
     * @brief リスナー向きを設定
     * @param forward 前方向ベクトル（正規化済み）
     * @param up 上向きベクトル（正規化済み）
     */
    void setListenerOrientation(const glm::vec3& forward, const glm::vec3& up);

    /**
     * @brief リスナー速度を設定（ドップラー効果用）
     * @param vel 速度ベクトル
     */
    void setListenerVelocity(const glm::vec3& vel);

    /**
     * @brief 距離減衰モデルを設定
     * @param model 減衰モデル
     */
    void setDistanceModel(DistanceModel model);

    /**
     * @brief ドップラー係数を設定（0.0 = 無効、1.0 = 自然）
     * @param factor ドップラー係数
     */
    void setDopplerFactor(float factor);

    /**
     * @brief スピード・オブ・サウンド（エアチェック用）
     * @param speed 音速（m/s、デフォルト 343.0）
     */
    void setSpeedOfSound(float speed);

    /**
     * @brief ゲイン（マスターボリューム）を設定
     * @param gain 0.0 - 1.0
     */
    void setGain(float gain);

    /**
     * @brief 距離減衰を計算（逆二乗則）
     * @param sourcePos 音源位置
     * @param referenceDistance リファレンス距離（減衰開始距離、デフォルト1.0m）
     * @param maxDistance 最大距離（これ以上は無音、デフォルト無限）
     * @return 減衰係数（0.0 - 1.0）
     */
    float calculateAttenuation(const glm::vec3& sourcePos, float referenceDistance = 1.0f,
                              float maxDistance = 1000.0f) const;

    /**
     * @brief 現在のリスナー位置を取得
     */
    const glm::vec3& getListenerPosition() const { return listenerPosition; }

    /**
     * @brief 現在のリスナー前方向を取得
     */
    const glm::vec3& getListenerForward() const { return listenerForward; }

    /**
     * @brief 現在のリスナー上向きを取得
     */
    const glm::vec3& getListenerUp() const { return listenerUp; }

private:
    // Listener state
    glm::vec3 listenerPosition;      // ワールド座標
    glm::vec3 listenerVelocity;      // ドップラー用速度
    glm::vec3 listenerForward;       // 前方向（正規化）
    glm::vec3 listenerUp;            // 上向き（正規化）

    // Configuration
    float dopplerFactor;             // 0.0 - 2.0 (default 1.0)
    float speedOfSound;              // m/s (default 343.0)
    float masterGain;                // 0.0 - 1.0 (default 1.0)
};
