#include "bgm_manager.h"
#include "audio_manager.h"
#include <algorithm>
#include <cmath>

namespace audio {

BgmManager::BgmManager()
    : audioManager(nullptr),
      state(BgmState::STOPPED),
      volume(0.7f),
      currentFadeValue(0.0f),
      crossfadeDuration(0.0f),
      crossfadeTimer(0.0f),
      crossfadeOutStartVolume(0.0f),
      unduckedVolume(0.7f) {
}

BgmManager::~BgmManager() {
    cleanup();
}

void BgmManager::initialize(AudioManager* manager) {
    audioManager = manager;
    LOGI("BgmManager initialized");
}

void BgmManager::update(float deltaTime) {
    switch (state) {
        case BgmState::FADING_IN:
            updateFadeIn(deltaTime);
            break;
        case BgmState::FADING_OUT:
            updateFadeOut(deltaTime);
            break;
        case BgmState::CROSSFADING:
            updateCrossfade(deltaTime);
            break;
        default:
            break;
    }

    // Update ducking regardless of fade state
    if (duckState.active) {
        updateDucking(deltaTime);
    }
}

void BgmManager::cleanup() {
    stop(0.0f);
    tracks.clear();
    playlists.clear();
    LOGI("BgmManager cleaned up");
}

// ========== Track Registration ==========

void BgmManager::registerTrack(const BgmTrack& track) {
    tracks[track.key] = track;
    LOGD("BGM track registered: %s -> %s", track.key.c_str(), track.filePath.c_str());
}

void BgmManager::registerTrack(const std::string& key, const std::string& filePath,
                                float vol, bool loop) {
    BgmTrack track;
    track.key = key;
    track.filePath = filePath;
    track.baseVolume = vol;
    track.loop = loop;
    track.loopStart = -1.0f;
    track.loopEnd = -1.0f;
    registerTrack(track);
}

bool BgmManager::hasTrack(const std::string& key) const {
    return tracks.find(key) != tracks.end();
}

// ========== Playback Control ==========

bool BgmManager::play(const std::string& key, float fadeIn) {
    if (!audioManager) {
        LOGE("BgmManager not initialized");
        return false;
    }

    auto it = tracks.find(key);
    if (it == tracks.end()) {
        LOGE("BGM track not found: %s", key.c_str());
        return false;
    }

    const BgmTrack& track = it->second;

    // Stop current BGM if playing
    if (state != BgmState::STOPPED) {
        stop(0.0f);
    }

    // Load and play via AudioManager
    uint32_t clipId = audioManager->loadClip(track.filePath, 0, track.loop);
    if (clipId == 0) {
        LOGE("Failed to load BGM clip: %s", track.filePath.c_str());
        return false;
    }

    if (!audioManager->playBGM(clipId, fadeIn)) {
        LOGE("Failed to play BGM: %s", key.c_str());
        return false;
    }

    currentTrackKey = key;
    unduckedVolume = track.baseVolume;

    if (fadeIn > 0.0f) {
        state = BgmState::FADING_IN;
        currentFadeValue = 0.0f;
        crossfadeDuration = fadeIn;
        crossfadeTimer = 0.0f;
    } else {
        state = BgmState::PLAYING;
        currentFadeValue = 1.0f;
    }

    applyVolume();

    LOGI("BGM playing: %s (fade=%.1fs)", key.c_str(), fadeIn);
    return true;
}

void BgmManager::stop(float fadeOut) {
    if (state == BgmState::STOPPED) {
        return;
    }

    if (fadeOut > 0.0f && state != BgmState::FADING_OUT) {
        state = BgmState::FADING_OUT;
        crossfadeDuration = fadeOut;
        crossfadeTimer = 0.0f;
        crossfadeOutStartVolume = currentFadeValue;
        LOGD("BGM fading out: %.1fs", fadeOut);
        return;
    }

    // Immediate stop
    if (audioManager) {
        audioManager->stopBGM(0.0f);
    }

    state = BgmState::STOPPED;
    currentTrackKey.clear();
    currentFadeValue = 0.0f;

    LOGD("BGM stopped");
}

bool BgmManager::crossfade(const std::string& key, float duration) {
    if (!audioManager) {
        return false;
    }

    auto it = tracks.find(key);
    if (it == tracks.end()) {
        LOGE("Crossfade target not found: %s", key.c_str());
        return false;
    }

    if (state == BgmState::STOPPED) {
        // Nothing to crossfade from, just play
        return play(key, duration);
    }

    // Begin crossfade
    crossfadeTargetKey = key;
    crossfadeDuration = duration;
    crossfadeTimer = 0.0f;
    crossfadeOutStartVolume = currentFadeValue;
    state = BgmState::CROSSFADING;

    LOGI("BGM crossfading: %s -> %s (%.1fs)", currentTrackKey.c_str(), key.c_str(), duration);
    return true;
}

void BgmManager::pause() {
    if (state == BgmState::PLAYING || state == BgmState::FADING_IN) {
        // Store current volume for resume
        LOGD("BGM paused");
    }
}

void BgmManager::resume() {
    LOGD("BGM resumed");
}

// ========== Volume Control ==========

void BgmManager::setVolume(float vol) {
    volume = std::max(0.0f, std::min(1.0f, vol));
    unduckedVolume = volume;
    applyVolume();
}

void BgmManager::startDucking(float duckLevel, float speed) {
    duckState.active = true;
    duckState.duckVolume = duckLevel;
    duckState.restoreVolume = unduckedVolume;
    duckState.fadeSpeed = speed;
    LOGD("BGM ducking started: target=%.2f", duckLevel);
}

void BgmManager::stopDucking() {
    duckState.active = false;
    LOGD("BGM ducking stopped");
}

// ========== Playlist Management ==========

void BgmManager::createPlaylist(const BgmPlaylist& playlist) {
    playlists[playlist.areaName] = playlist;
    LOGD("BGM playlist created: %s (%lu tracks)",
         playlist.areaName.c_str(),
         static_cast<unsigned long>(playlist.trackKeys.size()));
}

bool BgmManager::playArea(const std::string& areaName, float fadeIn) {
    auto it = playlists.find(areaName);
    if (it == playlists.end()) {
        LOGW("No playlist for area: %s", areaName.c_str());
        return false;
    }

    BgmPlaylist& playlist = it->second;
    if (playlist.trackKeys.empty()) {
        LOGW("Empty playlist for area: %s", areaName.c_str());
        return false;
    }

    currentArea = areaName;
    playlist.currentTrackIndex = 0;

    return play(playlist.trackKeys[0], fadeIn);
}

void BgmManager::nextTrack(float crossfadeDuration) {
    if (currentArea.empty()) {
        LOGW("No current area for next track");
        return;
    }

    auto it = playlists.find(currentArea);
    if (it == playlists.end()) {
        return;
    }

    BgmPlaylist& playlist = it->second;
    if (playlist.trackKeys.empty()) {
        return;
    }

    // Advance index
    playlist.currentTrackIndex++;
    if (playlist.currentTrackIndex >= static_cast<int>(playlist.trackKeys.size())) {
        playlist.currentTrackIndex = 0;  // Wrap around
    }

    const std::string& nextKey = playlist.trackKeys[playlist.currentTrackIndex];

    if (crossfadeDuration > 0.0f) {
        crossfade(nextKey, crossfadeDuration);
    } else {
        play(nextKey, 0.0f);
    }
}

// ========== Internal Update Methods ==========

void BgmManager::updateFadeIn(float deltaTime) {
    crossfadeTimer += deltaTime;
    float t = crossfadeTimer / crossfadeDuration;

    if (t >= 1.0f) {
        t = 1.0f;
        state = BgmState::PLAYING;
    }

    currentFadeValue = t;
    applyVolume();
}

void BgmManager::updateFadeOut(float deltaTime) {
    crossfadeTimer += deltaTime;
    float t = crossfadeTimer / crossfadeDuration;

    if (t >= 1.0f) {
        // Fade complete, stop
        stop(0.0f);
        return;
    }

    currentFadeValue = crossfadeOutStartVolume * (1.0f - t);
    applyVolume();
}

void BgmManager::updateCrossfade(float deltaTime) {
    crossfadeTimer += deltaTime;
    float t = crossfadeTimer / crossfadeDuration;

    if (t >= 1.0f) {
        // Crossfade complete: stop old, play new at full volume
        if (audioManager) {
            audioManager->stopBGM(0.0f);
        }

        // Play the new track
        auto it = tracks.find(crossfadeTargetKey);
        if (it != tracks.end() && audioManager) {
            const BgmTrack& track = it->second;
            uint32_t clipId = audioManager->loadClip(track.filePath, 0, track.loop);
            if (clipId != 0) {
                audioManager->playBGM(clipId, 0.0f);
            }
            currentTrackKey = crossfadeTargetKey;
            unduckedVolume = track.baseVolume;
        }

        crossfadeTargetKey.clear();
        state = BgmState::PLAYING;
        currentFadeValue = 1.0f;
        applyVolume();
        return;
    }

    // During crossfade: fade out old track volume
    currentFadeValue = crossfadeOutStartVolume * (1.0f - t);
    applyVolume();
}

void BgmManager::updateDucking(float deltaTime) {
    float targetVolume = duckState.active ? duckState.duckVolume : duckState.restoreVolume;
    float diff = targetVolume - volume;
    float step = duckState.fadeSpeed * deltaTime;

    if (std::abs(diff) < step) {
        volume = targetVolume;
    } else {
        volume += (diff > 0.0f ? step : -step);
    }

    applyVolume();
}

void BgmManager::applyVolume() {
    if (audioManager) {
        float effectiveVolume = volume * currentFadeValue;
        audioManager->setBGMVolume(effectiveVolume);
    }
}

} // namespace audio
