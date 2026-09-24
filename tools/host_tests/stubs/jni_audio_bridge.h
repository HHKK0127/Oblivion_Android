#ifndef JNI_AUDIO_BRIDGE_H
#define JNI_AUDIO_BRIDGE_H

#include <jni.h>
#include <android/asset_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

inline void jni_audio_set_asset_manager(AAssetManager* mgr) {}
inline AAssetManager* jni_audio_get_asset_manager() { return nullptr; }
inline void jni_audio_set_java_vm(JavaVM* vm) {}
inline void jni_audio_set_main_activity(void* activity) {}
inline void jni_audio_call_play_bgm(const char* path) {}
inline void jni_audio_call_stop_bgm() {}
inline void jni_audio_call_play_se(const char* path) {}

#ifdef __cplusplus
}
#endif

#endif
