// Phase 30 Step 8: Animation Player Implementation

#include "animation_player.h"
#include "skeleton.h"
#include <algorithm>
#include <cmath>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGE
#define LOG_TAG "AnimPlayer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace animation {

AnimationPlayer::AnimationPlayer() = default;
AnimationPlayer::~AnimationPlayer() = default;

void AnimationPlayer::initialize(Skeleton* skel,
                                  const std::vector<NIFControllerSequence>* seqs) {
    skeleton = skel;
    sequences = seqs;
    activeSequences.clear();
    globalTime = 0.0f;
    LOGD("AnimationPlayer initialized with %zu sequences",
         sequences ? sequences->size() : 0);
}

void AnimationPlayer::play(uint32_t sequenceIndex, bool loop, float speed) {
    if (!sequences || sequenceIndex >= sequences->size()) {
        LOGE("Invalid sequence index: %u", sequenceIndex);
        return;
    }

    // Check if already playing
    for (auto& state : activeSequences) {
        if (state.sequenceIndex == sequenceIndex && state.active) {
            state.currentTime = 0.0f;
            state.playbackSpeed = speed;
            state.looping = loop;
            state.firedTextKeys.clear();
            LOGD("Restarting sequence %u", sequenceIndex);
            return;
        }
    }

    // Add new sequence
    SequenceState state;
    state.sequenceIndex = sequenceIndex;
    state.currentTime = 0.0f;
    state.playbackSpeed = speed;
    state.looping = loop;
    state.active = true;
    state.blendWeight = 1.0f;

    buildTracks(state);
    activeSequences.push_back(state);

    LOGD("Playing sequence %u ('%s'), loop=%d, speed=%.2f",
         sequenceIndex, (*sequences)[sequenceIndex].name.c_str(),
         loop ? 1 : 0, speed);
}

void AnimationPlayer::stop(uint32_t sequenceIndex) {
    for (auto& state : activeSequences) {
        if (state.sequenceIndex == sequenceIndex) {
            state.active = false;
            LOGD("Stopped sequence %u", sequenceIndex);
            return;
        }
    }
}

void AnimationPlayer::stopAll() {
    for (auto& state : activeSequences) {
        state.active = false;
    }
    LOGD("Stopped all sequences");
}

void AnimationPlayer::crossfade(uint32_t fromSeq, uint32_t toSeq, float duration) {
    // Start new sequence with blend weight 0
    play(toSeq, true, 1.0f);

    // Find and set blend weights
    for (auto& state : activeSequences) {
        if (state.sequenceIndex == toSeq) {
            state.blendWeight = 0.0f;
        }
        if (state.sequenceIndex == fromSeq) {
            state.blendWeight = 1.0f;
        }
    }

    // Store crossfade state for update() to ramp weights
    crossfading = true;
    crossfadeFromSeq = fromSeq;
    crossfadeToSeq = toSeq;
    crossfadeDuration = (duration > 0.0f) ? duration : 0.3f;
    crossfadeElapsed = 0.0f;

    LOGD("Crossfade from %u to %u over %.2fs", fromSeq, toSeq, crossfadeDuration);
}

void AnimationPlayer::buildTracks(SequenceState& state) {
    if (!sequences || state.sequenceIndex >= sequences->size()) return;

    const auto& seq = (*sequences)[state.sequenceIndex];
    state.tracks.clear();

    // Build tracks from controlled blocks
    for (const auto& cb : seq.controlledBlocks) {
        BoneTrack track;
        track.boneIndex = cb.resolvedBoneIndex;

        // Use the clip's keyframes
        if (!cb.clip.keyframes.empty()) {
            track.keyframes = &cb.clip.keyframes;
        }

        if (track.boneIndex >= 0 && track.keyframes != nullptr) {
            state.tracks.push_back(track);
        }
    }

    LOGD("Built %zu tracks for sequence '%s'",
         state.tracks.size(), seq.name.c_str());
}

int AnimationPlayer::findKeyIndex(const std::vector<NIFKeyframe>& keys, float time) {
    if (keys.empty()) return -1;
    if (keys.size() == 1) return 0;

    // Binary search for the key at or before 'time'
    int lo = 0, hi = static_cast<int>(keys.size()) - 1;

    if (time <= keys[0].time) return 0;
    if (time >= keys[hi].time) return hi - 1;

    while (lo < hi - 1) {
        int mid = (lo + hi) / 2;
        if (keys[mid].time <= time) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return lo;
}

void AnimationPlayer::sampleTrack(const BoneTrack& track, float time,
                                    glm::vec3& outPos, glm::quat& outRot,
                                    float& outScale) const {
    if (!track.keyframes || track.keyframes->empty()) {
        outPos = glm::vec3(0, 0, 0);
        outRot = glm::quat();
        outScale = 1.0f;
        return;
    }

    const auto& keys = *track.keyframes;

    if (keys.size() == 1) {
        outPos = glm::vec3(keys[0].translation.x, keys[0].translation.y,
                           keys[0].translation.z);
        outRot = glm::quat(keys[0].rotation.x, keys[0].rotation.y,
                           keys[0].rotation.z, keys[0].rotation.w);
        outScale = keys[0].scale;
        return;
    }

    int idx = findKeyIndex(keys, time);
    int nextIdx = std::min(idx + 1, static_cast<int>(keys.size()) - 1);

    if (idx == nextIdx) {
        outPos = glm::vec3(keys[idx].translation.x, keys[idx].translation.y,
                           keys[idx].translation.z);
        outRot = glm::quat(keys[idx].rotation.x, keys[idx].rotation.y,
                           keys[idx].rotation.z, keys[idx].rotation.w);
        outScale = keys[idx].scale;
        return;
    }

    // Compute interpolation factor
    float t0 = keys[idx].time;
    float t1 = keys[nextIdx].time;
    float t = (t1 > t0) ? (time - t0) / (t1 - t0) : 0.0f;
    t = std::max(0.0f, std::min(1.0f, t));

    // Linear interpolation for position
    const auto& p0 = keys[idx].translation;
    const auto& p1 = keys[nextIdx].translation;
    outPos = glm::vec3(p0.x + (p1.x - p0.x) * t,
                       p0.y + (p1.y - p0.y) * t,
                       p0.z + (p1.z - p0.z) * t);

    // Slerp for rotation
    glm::quat q0(keys[idx].rotation.x, keys[idx].rotation.y,
                  keys[idx].rotation.z, keys[idx].rotation.w);
    glm::quat q1(keys[nextIdx].rotation.x, keys[nextIdx].rotation.y,
                  keys[nextIdx].rotation.z, keys[nextIdx].rotation.w);
    outRot = glm::slerp(q0, q1, t);

    // Linear interpolation for scale
    outScale = keys[idx].scale + (keys[nextIdx].scale - keys[idx].scale) * t;
}

void AnimationPlayer::emitTextKeys(SequenceState& state,
                                     const NIFControllerSequence& seq,
                                     float oldTime, float newTime) {
    if (!textKeyCallback) return;

    for (const auto& tk : seq.textKeys) {
        float keyTime = tk.time;
        bool inRange;

        if (state.looping) {
            float duration = seq.stopTime - seq.startTime;
            if (duration <= 0.0f) continue;
            // Handle wrap-around
            inRange = (oldTime <= keyTime && keyTime <= newTime) ||
                      (newTime < oldTime && (keyTime >= oldTime || keyTime <= newTime));
        } else {
            inRange = (oldTime <= keyTime && keyTime <= newTime);
        }

        if (inRange && state.firedTextKeys.find(keyTime) == state.firedTextKeys.end()) {
            textKeyCallback(tk.value);
            emitTextKeyToEventBus(tk.value);  // Also emit to Imperial Weave EventBus
            state.firedTextKeys.insert(keyTime);
        }
    }

    // Prevent unbounded growth
    if (state.firedTextKeys.size() > 100) {
        state.firedTextKeys.clear();
    }
}

// ============================================================================
// Imperial Weave EventBus integration
// ============================================================================
void AnimationPlayer::emitTextKeyToEventBus(const std::string& key) {
    if (!eventBus) return;

    // Map text keys to combat events
    weave::Event event;
    event.time = globalTime;

    if (key == "attack_start") {
        event.type = "ANIM_ATTACK_START";
    } else if (key == "attack_hit" || key == "hit") {
        event.type = "ANIM_ATTACK_HIT";
    } else if (key == "attack_end") {
        event.type = "ANIM_ATTACK_END";
    } else if (key == "block_start") {
        event.type = "ANIM_BLOCK_START";
    } else if (key == "block_end") {
        event.type = "ANIM_BLOCK_END";
    } else if (key == "death_start") {
        event.type = "ANIM_DEATH_START";
    } else if (key == "equip_start") {
        event.type = "ANIM_EQUIP_START";
    } else if (key == "equip_end") {
        event.type = "ANIM_EQUIP_END";
    } else {
        // Generic animation event
        event.type = "ANIM_EVENT";
    }

    event.payload = key;
    eventBus->emit(event);
}

void AnimationPlayer::update(float deltaTime) {
    if (!skeleton || !sequences) return;

    globalTime += deltaTime;

    // Advance crossfade if active — ramp blend weights linearly over duration
    if (crossfading && crossfadeDuration > 0.0f) {
        crossfadeElapsed += deltaTime;
        float t = (crossfadeElapsed >= crossfadeDuration) ? 1.0f
                                                           : (crossfadeElapsed / crossfadeDuration);
        for (auto& state : activeSequences) {
            if (state.sequenceIndex == crossfadeToSeq) {
                state.blendWeight = t;
            } else if (state.sequenceIndex == crossfadeFromSeq) {
                state.blendWeight = 1.0f - t;
            }
        }
        if (t >= 1.0f) {
            crossfading = false;
            // Stop the from sequence once transition completes
            stop(crossfadeFromSeq);
        }
    }

    // Update each active sequence
    for (auto& state : activeSequences) {
        if (!state.active) continue;

        const auto& seq = (*sequences)[state.sequenceIndex];
        float oldTime = state.currentTime;
        float newTime = oldTime + deltaTime * state.playbackSpeed;

        // Handle looping/clamping
        float duration = seq.stopTime - seq.startTime;
        if (duration <= 0.0f) continue;

        if (state.looping) {
            while (newTime >= seq.stopTime) {
                newTime -= duration;
                state.firedTextKeys.clear();
            }
            while (newTime < seq.startTime) {
                newTime += duration;
            }
        } else {
            if (newTime >= seq.stopTime) {
                newTime = seq.stopTime;
                state.active = false;
            }
        }

        state.currentTime = newTime;

        // Emit text key events
        emitTextKeys(state, seq, oldTime, newTime);

        // Sample and apply bone transforms
        for (const auto& track : state.tracks) {
            if (track.boneIndex < 0) continue;

            glm::vec3 pos;
            glm::quat rot;
            float scale;
            sampleTrack(track, newTime, pos, rot, scale);

            // Build local transform matrix
            glm::mat4 localTransform = glm::trsMatrix(pos, rot, scale);

            // Apply to skeleton bone
            skeleton->setBoneLocalTransform(track.boneIndex, localTransform);
        }
    }

    // Update skeleton world transforms
    skeleton->update();
}

void AnimationPlayer::getBoneMatrices(std::vector<glm::mat4>& outMatrices) const {
    if (skeleton) {
        const auto& matrices = skeleton->getSkinningMatrices();
        outMatrices = matrices;
    }
}

bool AnimationPlayer::isPlaying(uint32_t sequenceIndex) const {
    for (const auto& state : activeSequences) {
        if (state.sequenceIndex == sequenceIndex && state.active) {
            return true;
        }
    }
    return false;
}

float AnimationPlayer::getCurrentTime(uint32_t sequenceIndex) const {
    for (const auto& state : activeSequences) {
        if (state.sequenceIndex == sequenceIndex) {
            return state.currentTime;
        }
    }
    return 0.0f;
}

int32_t AnimationPlayer::findSequenceByName(const std::string& name) const {
    if (!sequences) return -1;

    for (size_t i = 0; i < sequences->size(); i++) {
        if ((*sequences)[i].name == name) {
            return static_cast<int32_t>(i);
        }
    }

    // Try partial match (e.g., "idle" matches "Idle" or "idle_loop")
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    for (size_t i = 0; i < sequences->size(); i++) {
        std::string seqName = (*sequences)[i].name;
        std::transform(seqName.begin(), seqName.end(), seqName.begin(), ::tolower);

        if (seqName.find(lowerName) != std::string::npos) {
            return static_cast<int32_t>(i);
        }
    }

    return -1;
}

uint32_t AnimationPlayer::getSequenceCount() const {
    if (!sequences) return 0;
    return static_cast<uint32_t>(sequences->size());
}

} // namespace animation
