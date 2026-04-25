#pragma once

#include <jni.h>
#include <string>

/**
 * JNI Audio Bridge
 * Provides C++ interface to Java audio methods (MediaPlayer, SoundPool)
 *
 * This module caches necessary JNI objects and provides simplified
 * functions for audio_manager.cpp to invoke Java audio methods.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize JNI audio bridge
 * Call this once from JNI_OnLoad to cache JavaVM and method IDs
 */
void jni_audio_bridge_init(JavaVM* vm);

/**
 * Play BGM via Java MediaPlayer
 * Filename should be relative to assets/audio/music/ (e.g., "explore.mp3")
 */
void jni_audio_play_bgm(const std::string& filename);

/**
 * Play SE via Java SoundPool
 * Filename should be relative to assets/audio/sounds/ (e.g., "battle.mp3")
 */
void jni_audio_play_se(const std::string& filename);

/**
 * Stop BGM playback
 */
void jni_audio_stop_bgm();

/**
 * Cleanup JNI audio bridge
 * Call this from app shutdown
 */
void jni_audio_bridge_cleanup();

#ifdef __cplusplus
}
#endif
