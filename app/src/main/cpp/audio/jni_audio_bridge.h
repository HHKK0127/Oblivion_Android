#pragma once

#include <jni.h>
#include <android/asset_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AssetManager をネイティブコードに設定
 * @param mgr Android AssetManager ポインタ
 */
void jni_audio_set_asset_manager(AAssetManager* mgr);

/**
 * @brief AssetManager を取得
 * @return AAssetManager ポインタ
 */
AAssetManager* jni_audio_get_asset_manager();

/**
 * @brief JavaVM をオーディオシステムに設定
 * @param vm JavaVM ポインタ
 */
void jni_audio_set_java_vm(JavaVM* vm);

/**
 * @brief MainActivity インスタンスをオーディオシステムに設定
 * JNI メソッド呼び出しの対象となるオブジェクト
 * @param activity MainActivity インスタンスの jobject
 */
void jni_audio_set_main_activity(jobject activity);

/**
 * @brief BGM 再生を Java 側から実行
 * MainActivity.playBGM(path) を JNI 経由で呼び出す
 * @param path オーディオファイルパス（assets/ 相対）
 */
void jni_audio_call_play_bgm(const char* path);

/**
 * @brief BGM 停止を Java 側から実行
 * MainActivity.stopBGM() を JNI 経由で呼び出す
 */
void jni_audio_call_stop_bgm();

/**
 * @brief SE 再生を Java 側から実行
 * MainActivity.playSE(path) を JNI 経由で呼び出す
 * @param path オーディオファイルパス（assets/ 相対）
 */
void jni_audio_call_play_se(const char* path);

#ifdef __cplusplus
}
#endif
