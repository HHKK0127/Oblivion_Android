// Host-runner stub implementations for Android/JNI extern symbols.
#include <jni.h>
#include <android/asset_manager.h>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// From audio/jni_audio_bridge.h (real one is under cpp/audio/). We inline
// definitions here so the host link does not need the real bridge.
extern "C" {

void jni_audio_set_asset_manager(AAssetManager*) {}
AAssetManager* jni_audio_get_asset_manager() { return nullptr; }
void jni_audio_set_java_vm(JavaVM*) {}
void jni_audio_set_main_activity(void*) {}
void jni_audio_call_play_bgm(const char*) {}
void jni_audio_call_stop_bgm() {}
void jni_audio_call_play_se(const char*) {}

AAsset* AAssetManager_open(AAssetManager*, const char*, int) { return nullptr; }
void AAsset_close(AAsset*) {}
off_t AAsset_getLength(AAsset*) { return 0; }
int AAsset_read(AAsset*, void*, size_t) { return 0; }

} // extern "C"
