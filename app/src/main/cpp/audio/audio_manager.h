#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <android/log.h>

// Note: Audio system uses Java MediaPlayer via JNI bridge
// OpenAL headers not needed for pragmatic approach

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGW
#undef LOGE

#define LOG_TAG "AudioManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#include "audio_clip.h"
#include "audio_source.h"
#include "audio_3d.h"

/**
 * @brief オーディオマネージャー
 * OpenAL デバイス・コンテキスト管理、
 * BGM/SE ローディング、再生制御を一元管理
 *
 * Manager Pattern に準拠：
 * - initialize(): OpenAL 初期化、デバイス検出
 * - update(deltaTime): 再生終了チェック、3D更新
 * - cleanup(): リソース解放、デバイス破棄
 */
class AudioManager {
public:
    /**
     * @brief コンストラクタ
     */
    AudioManager();

    /**
     * @brief デストラクタ
     */
    ~AudioManager();

    // ========== Lifecycle ==========

    /**
     * @brief 初期化（OpenAL デバイス・コンテキスト）
     * @return 成功時 true
     */
    bool initialize();

    /**
     * @brief 毎フレーム更新（再生終了チェック、3D更新）
     * @param deltaTime フレーム時間（秒）
     */
    void update(float deltaTime);

    /**
     * @brief クリーンアップ（リソース全解放）
     */
    void cleanup();

    // ========== Clip Management ==========

    /**
     * @brief オーディオクリップをロード
     * @param filename WAV ファイルパス（例："assets/music/oblivion_theme.wav"）
     * @param type 0=BGM, 1=SE, 2=Voice
     * @param isLooping ループ再生フラグ
     * @return クリップID（失敗時 0）
     */
    uint32_t loadClip(const std::string& filename, uint8_t type = 1,
                     bool isLooping = false);

    /**
     * @brief クリップをリソースから取得
     * @param clipId クリップID
     * @return クリップポインタ（見つからない場合 nullptr）
     */
    AudioClip* getClip(uint32_t clipId);

    /**
     * @brief クリップをアンロード
     * @param clipId クリップID
     */
    void unloadClip(uint32_t clipId);

    // ========== BGM Management ==========

    /**
     * @brief BGM を再生
     * @param clipId ロード済みのクリップID
     * @param fadeIn フェードイン時間（秒、0 = 即座）
     * @return 成功時 true
     */
    bool playBGM(uint32_t clipId, float fadeIn = 0.0f);

    /**
     * @brief BGM を停止
     * @param fadeOut フェードアウト時間（秒、0 = 即座）
     */
    void stopBGM(float fadeOut = 0.0f);

    /**
     * @brief BGM 再生中かどうか
     */
    bool isBGMPlaying() const;

    /**
     * @brief BGM のボリュームを設定
     * @param volume 0.0 - 1.0
     */
    void setBGMVolume(float volume);

    /**
     * @brief BGM の現在のボリュームを取得
     */
    float getBGMVolume() const { return bgmVolume; }

    // ========== SE Management ==========

    /**
     * @brief SE (効果音) を再生
     * @param clipId ロード済みのクリップID
     * @param position 3D 再生位置（デフォルト: リスナー位置）
     * @param volume ボリューム（デフォルト: 1.0）
     * @return ソースID（失敗時 0）
     */
    uint32_t playSE(uint32_t clipId, const glm::vec3& position = glm::vec3(0.0f),
                   float volume = 1.0f);

    /**
     * @brief SE を停止
     * @param sourceId ソースID
     */
    void stopSE(uint32_t sourceId);

    /**
     * @brief すべての SE を停止
     */
    void stopAllSE();

    // ========== 3D Audio ==========

    /**
     * @brief リスナー（プレイヤー）位置を設定
     * @param pos ワールド座標
     */
    void setListenerPosition(const glm::vec3& pos);

    /**
     * @brief リスナー向きを設定
     * @param forward 前方向ベクトル
     * @param up 上向きベクトル
     */
    void setListenerOrientation(const glm::vec3& forward, const glm::vec3& up);

    // ========== Volume Control ==========

    /**
     * @brief マスターボリュームを設定（全体）
     * @param volume 0.0 - 1.0
     */
    void setMasterVolume(float volume);

    /**
     * @brief マスターボリュームを取得
     */
    float getMasterVolume() const { return masterVolume; }

    /**
     * @brief SE のマスターボリュームを設定
     * @param volume 0.0 - 1.0
     */
    void setSEVolume(float volume);

    /**
     * @brief SE のマスターボリュームを取得
     */
    float getSEVolume() const { return seVolume; }

    // ========== Status Query ==========

    /**
     * @brief 現在ロード済みのクリップ数
     */
    size_t getLoadedClipsCount() const { return clips.size(); }

    /**
     * @brief 現在再生中のソース数
     */
    size_t getActiveSourcesesCount() const { return sources.size(); }

private:
    static constexpr uint32_t MAX_SOURCES = 32;
    static constexpr float FADE_UPDATE_RATE = 0.016f;  // 60 FPS

    // OpenAL device & context
    ALCdevice* device;
    ALCcontext* context;

    // 3D Audio system
    std::unique_ptr<Audio3D> audio3D;

    // Resource management
    std::unordered_map<uint32_t, std::shared_ptr<AudioClip>> clips;  // clipId -> Clip
    std::unordered_map<uint32_t, std::shared_ptr<AudioSource>> sources;  // sourceId -> Source

    uint32_t nextClipId;
    uint32_t nextSourceId;

    // BGM state
    uint32_t currentBGMSourceId;
    uint32_t currentBGMClipId;
    float bgmVolume;
    float bgmFadeTarget;
    float bgmFadeRate;
    bool bgmFading;

    // Volume levels
    float masterVolume;
    float seVolume;

    // ========== Private Methods ==========

    /**
     * @brief WAV ファイルをロード
     * @param filename ファイルパス
     * @return OpenAL buffer handle (0 = 失敗)
     */
    ALuint loadWavFile(const std::string& filename, ALint& format,
                      ALsizei& frequency, ALsizei& size);

    /**
     * @brief 再生完了したソースをクリーンアップ
     */
    void cleanupFinishedSources();

    /**
     * @brief BGM フェード処理
     * @param deltaTime フレーム時間
     */
    void updateBGMFade(float deltaTime);

    /**
     * @brief 新しいソースを作成
     * @param clipId クリップID
     * @return 作成されたソースID
     */
    uint32_t createSource(uint32_t clipId);

    /**
     * @brief ソースを削除
     * @param sourceId ソースID
     */
    void destroySource(uint32_t sourceId);
};
