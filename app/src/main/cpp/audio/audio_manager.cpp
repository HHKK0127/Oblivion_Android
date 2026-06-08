#include "audio_manager.h"
#include <android/asset_manager.h>
#include <cstring>
#include <algorithm>
#include "../jni_audio_bridge.h"

extern "C" AAssetManager* jni_audio_get_asset_manager();

AudioManager* g_audioManager = nullptr;

#define LOG_TAG "AudioManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// GLM clamp helper for minimal GLM
namespace {
    float clampf(float v, float minV, float maxV) {
        return v < minV ? minV : (v > maxV ? maxV : v);
    }
}

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
    g_audioManager = this;

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
    g_audioManager = nullptr;
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
    source->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
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
    jni_audio_call_play_bgm(filename.c_str());
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
        jni_audio_call_stop_bgm();

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
    bgmVolume = clampf(volume, 0.0f, 1.0f);

    if (currentBGMSourceId != 0) {
        auto source = sources[currentBGMSourceId];
        if (source) {
            source->setVolume(bgmVolume * masterVolume);
        }
    }
}

uint32_t AudioManager::playSE(uint32_t clipId, const glm::vec3& position,
                             float volume) {
    // MAX_SOURCES 超過時：最も古い SE を削除して新規作成を優先
    if (sources.size() >= MAX_SOURCES) {
        uint32_t oldestSourceId = 0;
        uint32_t oldestValue = UINT32_MAX;

        // BGM 以外で最も古いソース ID を探す（ソース ID は単調増加）
        for (auto& pair : sources) {
            if (pair.first != currentBGMSourceId && pair.first < oldestValue) {
                oldestSourceId = pair.first;
                oldestValue = pair.first;
            }
        }

        if (oldestSourceId != 0) {
            // 最も古い SE を削除
            destroySource(oldestSourceId);
            LOGW("Oldest SE removed to make room: sourceId=%u", oldestSourceId);
        } else {
            // BGM のみで MAX_SOURCES に達している場合
            LOGW("Cannot create new SE: MAX_SOURCES reached (BGM playing)");
            return 0;
        }
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

    // Java SoundPool 経由でも再生（フォールバック用）
    jni_audio_call_play_se(clip->filename.c_str());

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
    masterVolume = clampf(volume, 0.0f, 1.0f);

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
    seVolume = clampf(volume, 0.0f, 1.0f);
    LOGD("SE volume set: %.2f", seVolume);
}

ALuint AudioManager::loadWavFile(const std::string& filename, ALint& format,
                                ALsizei& frequency, ALsizei& size) {
    LOGD("Loading WAV file: %s", filename.c_str());

    AAssetManager* mgr = jni_audio_get_asset_manager();
    if (!mgr) {
        LOGE("AAssetManager not initialized");
        return 0;
    }

    AAsset* asset = AAssetManager_open(mgr, filename.c_str(), AASSET_MODE_STREAMING);
    if (!asset) {
        LOGE("Failed to open asset: %s", filename.c_str());
        return 0;
    }

    off_t assetSize = AAsset_getLength(asset);
    if (assetSize < 44) {
        LOGE("Asset too small: %ld bytes", assetSize);
        AAsset_close(asset);
        return 0;
    }

    std::vector<uint8_t> wavData(assetSize);
    if (AAsset_read(asset, wavData.data(), assetSize) != assetSize) {
        LOGE("Failed to read asset: %s", filename.c_str());
        AAsset_close(asset);
        return 0;
    }
    AAsset_close(asset);

    if (*(uint32_t*)wavData.data() != 0x46464952) {
        LOGE("Invalid WAV (no RIFF): %s", filename.c_str());
        return 0;
    }

    uint16_t numChannels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint32_t audioDataOffset = 0;
    uint32_t audioDataSize = 0;

    uint32_t pos = 12;
    while (pos + 8 <= wavData.size()) {
        uint32_t chunkId = *(uint32_t*)&wavData[pos];
        uint32_t chunkSize = *(uint32_t*)&wavData[pos + 4];

        if (pos + 8 + chunkSize > wavData.size()) break;

        if (chunkId == 0x20746d66) {
            if (chunkSize < 16) {
                LOGE("Invalid fmt chunk size: %u", chunkSize);
                return 0;
            }
            numChannels = *(uint16_t*)&wavData[pos + 8];
            sampleRate = *(uint32_t*)&wavData[pos + 12];
            bitsPerSample = *(uint16_t*)&wavData[pos + 22];
            LOGD("WAV: ch=%u sr=%u bits=%u", numChannels, sampleRate, bitsPerSample);
        } else if (chunkId == 0x61746164) {
            audioDataOffset = pos + 8;
            audioDataSize = chunkSize;
            break;
        }

        pos += 8 + chunkSize;
    }

    if (audioDataSize == 0 || numChannels == 0 || sampleRate == 0) {
        LOGE("Invalid WAV: missing audio data");
        return 0;
    }

    if (bitsPerSample == 16) {
        format = (numChannels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    } else if (bitsPerSample == 8) {
        format = (numChannels == 1) ? AL_FORMAT_MONO8 : AL_FORMAT_STEREO8;
    } else {
        LOGE("Unsupported bits per sample: %u", bitsPerSample);
        return 0;
    }

    frequency = sampleRate;
    size = audioDataSize;

    ALuint buffer = 0;
    alGenBuffers(1, &buffer);
    if (alGetError() != AL_NO_ERROR) {
        LOGE("alGenBuffers failed");
        return 0;
    }

    alBufferData(buffer, format, wavData.data() + audioDataOffset, audioDataSize, sampleRate);
    if (alGetError() != AL_NO_ERROR) {
        LOGE("alBufferData failed");
        alDeleteBuffers(1, &buffer);
        return 0;
    }

    LOGD("Loaded WAV: id=%u size=%u", buffer, audioDataSize);
    return buffer;
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
    newVolume = clampf(newVolume, 0.0f, bgmFadeTarget);

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

// ========== Sound Definition JSON ==========

bool AudioManager::loadSoundDefinitions(const std::string& jsonPath) {
    LOGI("Loading sound definitions: %s", jsonPath.c_str());

    AAssetManager* mgr = jni_audio_get_asset_manager();
    if (!mgr) {
        LOGE("AAssetManager not initialized");
        return false;
    }

    AAsset* asset = AAssetManager_open(mgr, jsonPath.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("Failed to open sound definitions: %s", jsonPath.c_str());
        return false;
    }

    off_t len = AAsset_getLength(asset);
    std::string jsonContent(len, '\0');
    AAsset_read(asset, &jsonContent[0], len);
    AAsset_close(asset);

    bool success = parseSoundDefinitionsJson(jsonContent);
    if (success) {
        LOGI("Sound definitions loaded: %zu sounds", soundDefs.size());
    }
    return success;
}

bool AudioManager::parseSoundDefinitionsJson(const std::string& json) {
    soundDefs.clear();

    // Simple JSON parser for sound_definitions.json structure
    // Finds pattern: "key": { ... }

    size_t pos = 0;
    while (pos < json.size()) {
        // Find next category
        size_t catPos = json.find("\"sounds\"", pos);
        if (catPos == std::string::npos) break;

        // Find opening brace after "sounds"
        size_t braceOpen = json.find('{', catPos);
        if (braceOpen == std::string::npos) break;

        size_t braceClose = braceOpen + 1;
        int depth = 1;
        while (braceClose < json.size() && depth > 0) {
            if (json[braceClose] == '{') depth++;
            else if (json[braceClose] == '}') depth--;
            braceClose++;
        }

        std::string soundsBlock = json.substr(braceOpen, braceClose - braceOpen);

        // Parse individual sound entries within this block
        size_t sPos = 0;
        while (sPos < soundsBlock.size()) {
            // Find key string
            size_t keyStart = soundsBlock.find('"', sPos);
            if (keyStart == std::string::npos) break;
            size_t keyEnd = soundsBlock.find('"', keyStart + 1);
            if (keyEnd == std::string::npos) break;

            std::string key = soundsBlock.substr(keyStart + 1, keyEnd - keyStart - 1);

            // Find value object
            size_t valOpen = soundsBlock.find('{', keyEnd);
            if (valOpen == std::string::npos) break;

            size_t valClose = valOpen + 1;
            int valDepth = 1;
            while (valClose < soundsBlock.size() && valDepth > 0) {
                if (soundsBlock[valClose] == '{') valDepth++;
                else if (soundsBlock[valClose] == '}') valDepth--;
                valClose++;
            }

            std::string valBlock = soundsBlock.substr(valOpen, valClose - valOpen);

            // Extract fields
            auto getString = [&](const std::string& field) -> std::string {
                size_t fPos = valBlock.find("\"" + field + "\"");
                if (fPos == std::string::npos) return "";
                size_t colon = valBlock.find(':', fPos);
                if (colon == std::string::npos) return "";
                size_t quote1 = valBlock.find('"', colon);
                if (quote1 == std::string::npos) return "";
                size_t quote2 = valBlock.find('"', quote1 + 1);
                if (quote2 == std::string::npos) return "";
                return valBlock.substr(quote1 + 1, quote2 - quote1 - 1);
            };

            auto getFloat = [&](const std::string& field, float defaultVal) -> float {
                size_t fPos = valBlock.find("\"" + field + "\"");
                if (fPos == std::string::npos) return defaultVal;
                size_t colon = valBlock.find(':', fPos);
                if (colon == std::string::npos) return defaultVal;
                size_t numStart = colon + 1;
                while (numStart < valBlock.size() && (valBlock[numStart] == ' ' || valBlock[numStart] == '\t' || valBlock[numStart] == '\n' || valBlock[numStart] == '\r'))
                    numStart++;
                size_t numEnd = numStart;
                while (numEnd < valBlock.size() && (valBlock[numEnd] == '.' || (valBlock[numEnd] >= '0' && valBlock[numEnd] <= '9')))
                    numEnd++;
                try {
                    return std::stof(valBlock.substr(numStart, numEnd - numStart));
                } catch (...) {
                    return defaultVal;
                }
            };

            auto getBool = [&](const std::string& field, bool defaultVal) -> bool {
                size_t fPos = valBlock.find("\"" + field + "\"");
                if (fPos == std::string::npos) return defaultVal;
                size_t colon = valBlock.find(':', fPos);
                if (colon == std::string::npos) return defaultVal;
                size_t valPos = colon + 1;
                while (valPos < valBlock.size() && (valBlock[valPos] == ' ' || valBlock[valPos] == '\t' || valBlock[valPos] == '\n' || valBlock[valPos] == '\r'))
                    valPos++;
                if (valPos + 4 <= valBlock.size() && valBlock.substr(valPos, 4) == "true")
                    return true;
                if (valPos + 5 <= valBlock.size() && valBlock.substr(valPos, 5) == "false")
                    return false;
                return defaultVal;
            };

            SoundDef def;
            def.file = getString("file");
            if (def.file.empty()) {
                sPos = valClose;
                continue;
            }

            std::string typeStr = getString("type");
            def.type = (typeStr == "bgm" || typeStr == "BGM") ? 0 : 1;
            def.volume = getFloat("volume", 1.0f);
            def.loop = getBool("loop", false);
            def.is3D = getBool("3d", false);
            def.clipId = 0;

            // Pre-load clip
            uint32_t clipId = loadClip(def.file, def.type, def.loop);
            if (clipId != 0) {
                def.clipId = clipId;
                soundDefs[key] = def;
                LOGD("Sound registered: key=%s file=%s clipId=%u", key.c_str(), def.file.c_str(), clipId);
            } else {
                LOGW("Failed to load sound: key=%s file=%s", key.c_str(), def.file.c_str());
            }

            sPos = valClose;
        }

        pos = braceClose;
    }

    return !soundDefs.empty();
}

uint32_t AudioManager::playSound(const std::string& key, const glm::vec3& position) {
    auto it = soundDefs.find(key);
    if (it == soundDefs.end()) {
        LOGW("Sound key not found: %s", key.c_str());
        return 0;
    }

    const SoundDef& def = it->second;
    if (def.clipId == 0) {
        LOGW("Sound clip not loaded: %s", key.c_str());
        return 0;
    }

    return playSE(def.clipId, position, def.volume);
}

bool AudioManager::playMusic(const std::string& key, float fadeIn) {
    auto it = soundDefs.find(key);
    if (it == soundDefs.end()) {
        LOGW("Music key not found: %s", key.c_str());
        return false;
    }

    const SoundDef& def = it->second;
    if (def.clipId == 0) {
        LOGW("Music clip not loaded: %s", key.c_str());
        return false;
    }

    return playBGM(def.clipId, fadeIn);
}
