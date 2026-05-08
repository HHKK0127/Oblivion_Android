#include "audio_manager.h"
#include <android/asset_manager.h>
#include <cstring>
#include <algorithm>
#include <glm/common.hpp>
#include "../jni_audio_bridge.h"

AudioManager::AudioManager()
    : device(nullptr), context(nullptr),
      nextClipId(1), nextSourceId(1),
      currentBGMSourceId(0), currentBGMClipId(0),
      bgmVolume(1.0f), bgmFadeTarget(1.0f), bgmFadeRate(0.0f),
      bgmFading(false),
      masterVolume(1.0f), seVolume(1.0f) {
    LOGD("AudioManager constructed");
}

AudioManager::~AudioManager() {
    cleanup();
    LOGD("AudioManager destroyed");
}

bool AudioManager::initialize() {
    LOGI("AudioManager initializing...");

    // OpenAL デバイスを開く
    device = alcOpenDevice(nullptr);
    if (!device) {
        LOGE("Failed to open OpenAL device");
        return false;
    }
    LOGD("OpenAL device opened");

    // コンテキストを作成
    context = alcCreateContext(device, nullptr);
    if (!context) {
        LOGE("Failed to create OpenAL context");
        alcCloseDevice(device);
        device = nullptr;
        return false;
    }
    LOGD("OpenAL context created");

    // コンテキストを有効化
    if (!alcMakeContextCurrent(context)) {
        LOGE("Failed to make OpenAL context current");
        alcDestroyContext(context);
        alcCloseDevice(device);
        context = nullptr;
        device = nullptr;
        return false;
    }
    LOGD("OpenAL context made current");

    // デバイス情報をログ出力
    const char* deviceName = alcGetString(device, ALC_DEVICE_SPECIFIER);
    LOGI("OpenAL Device: %s", deviceName ? deviceName : "Unknown");

    const char* vendorStr = alGetString(AL_VENDOR);
    const char* versionStr = alGetString(AL_VERSION);
    LOGI("OpenAL Vendor: %s", vendorStr ? vendorStr : "Unknown");
    LOGI("OpenAL Version: %s", versionStr ? versionStr : "Unknown");

    // 3D オーディオシステムを初期化
    audio3D = std::make_unique<Audio3D>();
    LOGD("Audio3D system initialized");

    LOGI("AudioManager initialization complete");
    return true;
}

void AudioManager::update(float deltaTime) {
    // BGM フェード処理
    if (bgmFading) {
        updateBGMFade(deltaTime);
    }

    // 再生終了したソースをクリーンアップ
    cleanupFinishedSources();

    // リスナー位置・向きは Renderer 経由で setListenerPosition/Orientation で設定される
}

void AudioManager::cleanup() {
    LOGI("AudioManager cleanup starting...");

    // すべてのソースを停止・削除
    for (auto& pair : sources) {
        if (pair.second && pair.second->alSource != 0) {
            alSourceStop(pair.second->alSource);
            alDeleteSources(1, &pair.second->alSource);
        }
    }
    sources.clear();
    LOGD("All audio sources deleted");

    // すべてのクリップ（バッファ）を削除
    for (auto& pair : clips) {
        if (pair.second && pair.second->alBuffer != 0) {
            alDeleteBuffers(1, &pair.second->alBuffer);
        }
    }
    clips.clear();
    LOGD("All audio clips unloaded");

    // 3D オーディオシステムをクリア
    audio3D.reset();

    // OpenAL コンテキスト・デバイスをクリーンアップ
    if (context) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        context = nullptr;
    }
    if (device) {
        alcCloseDevice(device);
        device = nullptr;
    }

    LOGI("AudioManager cleanup complete");
}

uint32_t AudioManager::loadClip(const std::string& filename, uint8_t type,
                               bool isLooping) {
    LOGD("Loading audio clip: %s", filename.c_str());

    if (!device || !context) {
        LOGE("OpenAL not initialized, cannot load clip");
        return 0;
    }

    // WAV ファイルをロード
    ALint format;
    ALsizei frequency, size;
    ALuint buffer = loadWavFile(filename, format, frequency, size);

    if (buffer == 0) {
        LOGE("Failed to load WAV file: %s", filename.c_str());
        return 0;
    }

    // AudioClip を作成
    auto clip = std::make_shared<AudioClip>();
    clip->clipId = nextClipId++;
    clip->filename = filename;
    clip->alBuffer = buffer;
    clip->isLooping = isLooping;
    clip->type = type;
    clip->volume = 1.0f;
    clip->isStreamed = false;

    // 再生時間を計算 (バイト数 / (サンプルレート * チャンネル数 * サンプルサイズ))
    // ここでは簡略化して未実装（実装時には正確に計算）
    clip->duration = 0.0f;  // TODO: 正確に計算

    clips[clip->clipId] = clip;

    LOGI("Audio clip loaded: id=%u, file=%s, type=%u, looping=%s",
         clip->clipId, filename.c_str(), type, isLooping ? "true" : "false");

    return clip->clipId;
}

AudioClip* AudioManager::getClip(uint32_t clipId) {
    auto it = clips.find(clipId);
    if (it != clips.end()) {
        return it->second.get();
    }
    return nullptr;
}

void AudioManager::unloadClip(uint32_t clipId) {
    auto it = clips.find(clipId);
    if (it != clips.end()) {
        if (it->second->alBuffer != 0) {
            alDeleteBuffers(1, &it->second->alBuffer);
            LOGD("Audio clip unloaded: id=%u", clipId);
        }
        clips.erase(it);
    }
}

bool AudioManager::playBGM(uint32_t clipId, float fadeIn) {
    LOGD("Playing BGM: clipId=%u, fadeIn=%.2f", clipId, fadeIn);

    // 前の BGM を停止
    if (currentBGMSourceId != 0) {
        stopBGM(0.0f);
    }

    // クリップを取得
    AudioClip* clip = getClip(clipId);
    if (!clip) {
        LOGE("BGM clip not found: id=%u", clipId);
        return false;
    }

#ifdef AUDIO_SYSTEM_ENABLED
    // OpenAL を使用（ただし WAV 読み込みが実装されていないため、ここでは Java 経由で再生）

    // ソースを作成
    uint32_t sourceId = createSource(clipId);
    if (sourceId == 0) {
        LOGE("Failed to create audio source for BGM");
        return false;
    }

    // BGM ソースの設定
    auto source = sources[sourceId];
    source->setVolume(fadeIn > 0.0f ? 0.0f : bgmVolume);
    source->setPosition(glm::vec3(0.0f));
    source->disable3D();

    // 再生開始
    alSourcePlay(source->alSource);

    currentBGMSourceId = sourceId;
    currentBGMClipId = clipId;
    bgmVolume = 1.0f;

    if (fadeIn > 0.0f) {
        bgmFadeTarget = 1.0f;
        bgmFadeRate = 1.0f / fadeIn;
        bgmFading = true;
    }
#endif

    // Java MediaPlayer 経由で MP3 を再生
    LOGI("Playing BGM via Java MediaPlayer: %s", clip->filename.c_str());
    playBGMViaJava(clip->filename);

    return true;
}

/**
 * @brief Java の MediaPlayer を使用して BGM を再生
 *
 * Java の MainActivity.playBGM(String) メソッドを JNI 経由で呼び出す。
 * ファイル名は assets/audio/music/ 相対パスで指定される。
 */
void AudioManager::playBGMViaJava(const std::string& filename) {
    LOGD("playBGMViaJava: %s", filename.c_str());

    // JNI audio bridge 経由で Java の playBGM メソッドを呼び出す
    // このメソッドは MediaPlayer をセットアップして再生を開始する
    jni_audio_play_bgm(filename);
}

void AudioManager::stopBGM(float fadeOut) {
    if (currentBGMSourceId == 0) {
        LOGD("No BGM currently playing");
        return;
    }

    if (fadeOut > 0.0f) {
        // フェードアウト開始
        bgmFadeTarget = 0.0f;
        bgmFadeRate = -(1.0f / fadeOut);
        bgmFading = true;
        LOGD("BGM fading out: rate=%.3f", bgmFadeRate);
    } else {
        // 即座に停止
        auto source = sources[currentBGMSourceId];
        if (source) {
            alSourceStop(source->alSource);
        }
        destroySource(currentBGMSourceId);
        currentBGMSourceId = 0;
        currentBGMClipId = 0;

        // Java MediaPlayer 経由で BGM を停止
        jni_audio_stop_bgm();

        LOGD("BGM stopped");
    }
}

bool AudioManager::isBGMPlaying() const {
    if (currentBGMSourceId == 0) {
        return false;
    }

    auto it = sources.find(currentBGMSourceId);
    if (it == sources.end()) {
        return false;
    }

    return it->second->isPlaying;
}

void AudioManager::setBGMVolume(float volume) {
    bgmVolume = glm::clamp(volume, 0.0f, 1.0f);

    if (currentBGMSourceId != 0) {
        auto source = sources[currentBGMSourceId];
        if (source) {
            source->setVolume(bgmVolume * masterVolume);
        }
    }
}

uint32_t AudioManager::playSE(uint32_t clipId, const glm::vec3& position,
                             float volume) {
    if (sources.size() >= MAX_SOURCES) {
        LOGW("Maximum number of audio sources reached");
        return 0;
    }

    // クリップを取得
    AudioClip* clip = getClip(clipId);
    if (!clip) {
        LOGW("SE clip not found: id=%u", clipId);
        return 0;
    }

    // ソースを作成
    uint32_t sourceId = createSource(clipId);
    if (sourceId == 0) {
        LOGE("Failed to create audio source for SE");
        return 0;
    }

    // SE ソースの設定
    auto source = sources[sourceId];
    source->setVolume(volume * seVolume * masterVolume);
    source->setPosition(position);
    source->enable3D();

    // 再生開始
    alSourcePlay(source->alSource);

    LOGD("SE playing: sourceId=%u, clipId=%u, pos=(%.1f, %.1f, %.1f)",
         sourceId, clipId, position.x, position.y, position.z);

    return sourceId;
}

void AudioManager::stopSE(uint32_t sourceId) {
    auto it = sources.find(sourceId);
    if (it != sources.end()) {
        alSourceStop(it->second->alSource);
        destroySource(sourceId);
        LOGD("SE stopped: sourceId=%u", sourceId);
    }
}

void AudioManager::stopAllSE() {
    std::vector<uint32_t> sourceIds;
    for (auto& pair : sources) {
        if (pair.first != currentBGMSourceId) {
            sourceIds.push_back(pair.first);
        }
    }

    for (uint32_t sourceId : sourceIds) {
        stopSE(sourceId);
    }

    LOGD("All SE stopped");
}

void AudioManager::setListenerPosition(const glm::vec3& pos) {
    if (audio3D) {
        audio3D->setListenerPosition(pos);
    }
}

void AudioManager::setListenerOrientation(const glm::vec3& forward,
                                         const glm::vec3& up) {
    if (audio3D) {
        audio3D->setListenerOrientation(forward, up);
    }
}

void AudioManager::setMasterVolume(float volume) {
    masterVolume = glm::clamp(volume, 0.0f, 1.0f);

    // すべてのソースのボリュームを再計算
    for (auto& pair : sources) {
        if (pair.second) {
            if (pair.first == currentBGMSourceId) {
                pair.second->setVolume(bgmVolume * masterVolume);
            } else {
                // SE のボリューム（元のボリューム * マスター）
                // TODO: SE の元のボリュームを保持する必要あり
                pair.second->setVolume(seVolume * masterVolume);
            }
        }
    }

    LOGD("Master volume set: %.2f", masterVolume);
}

void AudioManager::setSEVolume(float volume) {
    seVolume = glm::clamp(volume, 0.0f, 1.0f);
    LOGD("SE volume set: %.2f", seVolume);
}

ALuint AudioManager::loadWavFile(const std::string& filename, ALint& format,
                                ALsizei& frequency, ALsizei& size) {
    // TODO: Android assets から WAV ファイルを読み込む
    // ここでは簡略実装（実装時には以下の処理が必要）
    // 1. AAssetManager から AAsset を開く
    // 2. WAV ヘッダーをパース (RIFF, fmt, data チャンク)
    // 3. alBufferData で OpenAL バッファへアップロード

    LOGE("loadWavFile not yet implemented: %s", filename.c_str());

    // 仮実装：失敗を返す
    format = AL_FORMAT_MONO16;
    frequency = 44100;
    size = 0;
    return 0;
}

void AudioManager::cleanupFinishedSources() {
    std::vector<uint32_t> finishedSourceIds;

    for (auto& pair : sources) {
        uint32_t sourceId = pair.first;
        auto source = pair.second;

        if (!source || source->alSource == 0) {
            continue;
        }

        // 再生状態を確認
        ALint state;
        alGetSourcei(source->alSource, AL_SOURCE_STATE, &state);

        if (state != AL_PLAYING && sourceId != currentBGMSourceId) {
            finishedSourceIds.push_back(sourceId);
        }
    }

    // 終了したソースを削除
    for (uint32_t sourceId : finishedSourceIds) {
        destroySource(sourceId);
        LOGD("Finished source cleaned up: sourceId=%u", sourceId);
    }
}

void AudioManager::updateBGMFade(float deltaTime) {
    if (currentBGMSourceId == 0) {
        bgmFading = false;
        return;
    }

    auto source = sources[currentBGMSourceId];
    if (!source) {
        bgmFading = false;
        return;
    }

    // ボリュームを更新
    float newVolume = bgmVolume + (bgmFadeRate * deltaTime);
    newVolume = glm::clamp(newVolume, 0.0f, bgmFadeTarget);

    source->setVolume(newVolume * masterVolume);

    // フェード完了をチェック
    if (newVolume == bgmFadeTarget) {
        bgmFading = false;

        if (bgmFadeTarget == 0.0f) {
            // フェードアウト完了 → 停止
            alSourceStop(source->alSource);
            destroySource(currentBGMSourceId);
            currentBGMSourceId = 0;
            LOGD("BGM fade out complete, stopped");
        } else {
            LOGD("BGM fade in complete");
        }
    }

    bgmVolume = newVolume;
}

uint32_t AudioManager::createSource(uint32_t clipId) {
    AudioClip* clip = getClip(clipId);
    if (!clip) {
        return 0;
    }

    ALuint alSource;
    alGenSources(1, &alSource);

    if (alGetError() != AL_NO_ERROR) {
        LOGE("Failed to generate OpenAL source");
        return 0;
    }

    // ソースに バッファを割り当て
    alSourcei(alSource, AL_BUFFER, clip->alBuffer);
    alSourcei(alSource, AL_LOOPING, clip->isLooping ? AL_TRUE : AL_FALSE);

    auto source = std::make_shared<AudioSource>();
    source->sourceId = nextSourceId++;
    source->alSource = alSource;
    source->clipId = clipId;
    source->isLooping = clip->isLooping;

    sources[source->sourceId] = source;

    LOGD("Audio source created: sourceId=%u, clipId=%u", source->sourceId,
         clipId);

    return source->sourceId;
}

void AudioManager::destroySource(uint32_t sourceId) {
    auto it = sources.find(sourceId);
    if (it == sources.end()) {
        return;
    }

    auto source = it->second;
    if (source && source->alSource != 0) {
        alSourceStop(source->alSource);
        alDeleteSources(1, &source->alSource);
    }

    sources.erase(it);
    LOGD("Audio source destroyed: sourceId=%u", sourceId);
}
