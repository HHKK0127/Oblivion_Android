#pragma once

#include <jni.h>
#include <android/asset_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set AssetManager to native code
 * @param mgr Android AssetManager pointer
 */
void jni_audio_set_asset_manager(AAssetManager* mgr);

/**
 * @brief Get AssetManager
 * @return AAssetManager pointer
 */
AAssetManager* jni_audio_get_asset_manager();

/**
 * @brief Set JavaVM to audio system
 * @param vm JavaVM pointer
 */
void jni_audio_set_java_vm(JavaVM* vm);

/**
 * @brief Set MainActivity instance to audio system
 * Object for JNI method calls
 * @param activity jobject of MainActivity instance
 */
void jni_audio_set_main_activity(jobject activity);

/**
 * @brief Execute BGM playback from Java side
 * Calls MainActivity.playBGM(path) via JNI
 * @param path Audio file path (relative to assets/)
 */
void jni_audio_call_play_bgm(const char* path);

/**
 * @brief Execute BGM stop from Java side
 * Calls MainActivity.stopBGM() via JNI
 */
void jni_audio_call_stop_bgm();

/**
 * @brief Execute SE playback from Java side
 * Calls MainActivity.playSE(path) via JNI
 * @param path Audio file path (relative to assets/)
 */
void jni_audio_call_play_se(const char* path);

#ifdef __cplusplus
}
#endif
