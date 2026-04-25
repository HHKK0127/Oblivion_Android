#pragma once

#include <cstdint>
#include <glm/glm.hpp>

// Stub: OpenAL not needed for Java MediaPlayer approach via JNI
using ALuint = unsigned int;

/**
 * @brief オーディオ再生チャンネル（AudioSource）
 * OpenAL source を管理し、個々の音声再生を制御
 *
 * BGM、SE、音声などの個別の再生チャンネルを表現
 * 3D位置、ボリューム、ピッチ、再生状態を管理
 */
struct AudioSource {
    uint32_t sourceId;              // ユニークID（Renderer が割り当て）
    ALuint alSource;                // OpenAL source handle
    uint32_t clipId;                // 再生中のクリップ ID

    // 3D Audio
    glm::vec3 position;             // ワールド座標
    glm::vec3 velocity;             // ドップラー効果用

    // Volume & Pitch
    float volume;                   // 0.0 - 1.0
    float pitch;                    // 0.5 - 2.0

    // Playback state
    bool isPlaying;
    float playbackTime;             // 再生経過時間（秒）

    // Flags
    bool isLooping;                 // ループ再生フラグ
    bool is3D;                      // 3D音声フラグ

    /**
     * @brief デフォルトコンストラクタ
     */
    AudioSource()
        : sourceId(0), alSource(0), clipId(0),
          position(0.0f), velocity(0.0f),
          volume(1.0f), pitch(1.0f),
          isPlaying(false), playbackTime(0.0f),
          isLooping(false), is3D(false) {
    }

    /**
     * @brief ボリュームを設定
     * @param vol 0.0 - 1.0
     */
    void setVolume(float vol) {
        volume = glm::clamp(vol, 0.0f, 1.0f);
        if (alSource != 0) {
            alSourcef(alSource, AL_GAIN, volume);
        }
    }

    /**
     * @brief ピッチを設定
     * @param p 0.5 - 2.0
     */
    void setPitch(float p) {
        pitch = glm::clamp(p, 0.5f, 2.0f);
        if (alSource != 0) {
            alSourcef(alSource, AL_PITCH, pitch);
        }
    }

    /**
     * @brief 3D位置を設定
     * @param pos ワールド座標
     */
    void setPosition(const glm::vec3& pos) {
        position = pos;
        if (alSource != 0 && is3D) {
            alSource3f(alSource, AL_POSITION, pos.x, pos.y, pos.z);
        }
    }

    /**
     * @brief 速度を設定（ドップラー効果用）
     * @param vel 速度ベクトル
     */
    void setVelocity(const glm::vec3& vel) {
        velocity = vel;
        if (alSource != 0 && is3D) {
            alSource3f(alSource, AL_VELOCITY, vel.x, vel.y, vel.z);
        }
    }

    /**
     * @brief 3D音声の有効化
     */
    void enable3D() {
        is3D = true;
        if (alSource != 0) {
            // 相対位置有効化
            alSourcei(alSource, AL_SOURCE_RELATIVE, AL_FALSE);
        }
    }

    /**
     * @brief 3D音声の無効化（モノラル再生）
     */
    void disable3D() {
        is3D = false;
        if (alSource != 0) {
            alSourcei(alSource, AL_SOURCE_RELATIVE, AL_TRUE);
            position = glm::vec3(0.0f);
        }
    }
};
