#pragma once

#include <string>
#include <cstdint>

// Stub: OpenAL not needed for Java MediaPlayer approach via JNI
using ALuint = unsigned int;

/**
 * @brief 音声リソース（クリップ）
 * WAV/OGG等のオーディオデータを表現
 */
struct AudioClip {
    uint32_t clipId;                // ユニークID
    std::string filename;            // ファイルパス
    ALuint alBuffer;                 // OpenAL buffer handle
    float duration;                  // 再生時間（秒）
    bool isLooping;                  // ループ再生フラグ
    bool isStreamed;                 // ストリーミング再生（true）vs 完全ロード（false）
    float volume;                    // 基本ボリューム (0.0 - 1.0)
    uint8_t type;                    // 音声タイプ: 0=BGM, 1=SE, 2=Voice

    /**
     * @brief コンストラクタ
     */
    AudioClip()
        : clipId(0), alBuffer(0), duration(0.0f), isLooping(false),
          isStreamed(false), volume(1.0f), type(1) {
    }

    /**
     * @brief デストラクタ
     */
    ~AudioClip() {
        if (alBuffer != 0) {
            alDeleteBuffers(1, &alBuffer);
            alBuffer = 0;
        }
    }
};
