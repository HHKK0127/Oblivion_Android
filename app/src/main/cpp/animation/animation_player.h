#pragma once

// Phase 30 Step 8: Animation Player
// Plays NiControllerSequence animations on a Skeleton

#include <vector>
#include <functional>
#include <string>
#include <set>
#include <glm/glm.hpp>
#include "../assets/nif_types.h"

class Skeleton;

namespace animation {

// Pre-resolved animation track for fast sampling
struct BoneTrack {
    int32_t boneIndex = -1;  // Skeleton bone index (resolved at load time)

    // Keyframe data pointers (not owned)
    const std::vector<NIFKeyframe>* keyframes = nullptr;

    // Interpolation type
    enum InterpType : uint8_t { LINEAR, BEZIER, TCB } interpType = LINEAR;
};

// Per-sequence playback state
struct SequenceState {
    uint32_t sequenceIndex = 0;
    float currentTime = 0.0f;
    float playbackSpeed = 1.0f;
    bool looping = false;
    bool active = false;
    float blendWeight = 1.0f;

    // TextKey event tracking (per-sequence, per design feedback)
    float lastEmittedTime = -1.0f;
    std::set<float> firedTextKeys;

    // Pre-resolved bone tracks
    std::vector<BoneTrack> tracks;
};

class AnimationPlayer {
public:
    using TextKeyCallback = std::function<void(const std::string&)>;

    AnimationPlayer();
    ~AnimationPlayer();

    // Initialize with skeleton and parsed animation data
    void initialize(Skeleton* skel,
                    const std::vector<NIFControllerSequence>* sequences);

    // Playback control
    void play(uint32_t sequenceIndex, bool loop = true, float speed = 1.0f);
    void stop(uint32_t sequenceIndex);
    void stopAll();
    void crossfade(uint32_t fromSeq, uint32_t toSeq, float duration);

    // Per-frame update
    void update(float deltaTime);

    // Get current bone matrices (output parameter to avoid heap allocation)
    void getBoneMatrices(std::vector<glm::mat4>& outMatrices) const;

    // Text key callback
    void setTextKeyCallback(TextKeyCallback cb) { textKeyCallback = cb; }

    // Query
    bool isPlaying(uint32_t sequenceIndex) const;
    float getCurrentTime(uint32_t sequenceIndex) const;

private:
    Skeleton* skeleton = nullptr;
    const std::vector<NIFControllerSequence>* sequences = nullptr;
    std::vector<SequenceState> activeSequences;
    float globalTime = 0.0f;

    TextKeyCallback textKeyCallback;

    // Build bone tracks for a sequence
    void buildTracks(SequenceState& state);

    // Sample a single track at given time
    void sampleTrack(const BoneTrack& track, float time,
                     glm::vec3& outPos, glm::quat& outRot, float& outScale) const;

    // Emit text keys in time range
    void emitTextKeys(SequenceState& state, const NIFControllerSequence& seq,
                      float oldTime, float newTime);

    // Binary search for keyframe index
    static int findKeyIndex(const std::vector<NIFKeyframe>& keys, float time);
};

} // namespace animation
