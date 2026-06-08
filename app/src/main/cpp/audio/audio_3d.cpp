#include "audio_3d.h"
#include <AL/al.h>
#include <cmath>

#define LOG_TAG "Audio3D"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// GLM free function helpers for minimal GLM
namespace {
    glm::vec3 normalize_vec3(const glm::vec3& v) {
        float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        if (len == 0.0f) return glm::vec3(0.0f, 0.0f, 0.0f);
        return glm::vec3(v.x / len, v.y / len, v.z / len);
    }
    float distance_vec3(const glm::vec3& a, const glm::vec3& b) {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        float dz = a.z - b.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    float clamp_float(float v, float minV, float maxV) {
        return v < minV ? minV : (v > maxV ? maxV : v);
    }
    float max_float(float a, float b) {
        return a > b ? a : b;
    }
}

Audio3D::Audio3D()
    : listenerPosition(0.0f, 0.0f, 0.0f),
      listenerVelocity(0.0f, 0.0f, 0.0f),
      listenerForward(0.0f, 0.0f, -1.0f),
      listenerUp(0.0f, 1.0f, 0.0f),
      dopplerFactor(1.0f),
      speedOfSound(343.0f),
      masterGain(1.0f) {
    LOGD("Audio3D initialized");

    // OpenAL global settings
    alDopplerFactor(dopplerFactor);
    alSpeedOfSound(speedOfSound);

    // Default distance attenuation model
    setDistanceModel(DistanceModel::INVERSE_DISTANCE_CLAMPED);
}

Audio3D::~Audio3D() {
    LOGD("Audio3D destroyed");
}

void Audio3D::setListenerPosition(const glm::vec3& pos) {
    listenerPosition = pos;
    alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
    LOGD("Listener position set: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
}

void Audio3D::setListenerOrientation(const glm::vec3& forward, const glm::vec3& up) {
    listenerForward = normalize_vec3(forward);
    listenerUp = normalize_vec3(up);

    // OpenAL format: [forward.x, forward.y, forward.z, up.x, up.y, up.z]
    float orientation[6] = {
        listenerForward.x, listenerForward.y, listenerForward.z,
        listenerUp.x, listenerUp.y, listenerUp.z
    };

    alListenerfv(AL_ORIENTATION, orientation);
    LOGD("Listener orientation set: forward=(%.2f, %.2f, %.2f) up=(%.2f, %.2f, %.2f)",
         listenerForward.x, listenerForward.y, listenerForward.z,
         listenerUp.x, listenerUp.y, listenerUp.z);
}

void Audio3D::setListenerVelocity(const glm::vec3& vel) {
    listenerVelocity = vel;
    alListener3f(AL_VELOCITY, vel.x, vel.y, vel.z);
}

void Audio3D::setDistanceModel(DistanceModel model) {
    ALenum alModel;
    const char* modelName;

    switch (model) {
        case DistanceModel::INVERSE_DISTANCE:
            alModel = AL_INVERSE_DISTANCE;
            modelName = "INVERSE_DISTANCE";
            break;
        case DistanceModel::INVERSE_DISTANCE_CLAMPED:
            alModel = AL_INVERSE_DISTANCE_CLAMPED;
            modelName = "INVERSE_DISTANCE_CLAMPED";
            break;
        case DistanceModel::LINEAR_DISTANCE:
            alModel = AL_LINEAR_DISTANCE;
            modelName = "LINEAR_DISTANCE";
            break;
        case DistanceModel::EXPONENT_DISTANCE:
            alModel = AL_EXPONENT_DISTANCE;
            modelName = "EXPONENT_DISTANCE";
            break;
        default:
            alModel = AL_INVERSE_DISTANCE_CLAMPED;
            modelName = "INVERSE_DISTANCE_CLAMPED (default)";
    }

    alDistanceModel(alModel);
    LOGD("Distance model set: %s", modelName);
}

void Audio3D::setDopplerFactor(float factor) {
    dopplerFactor = clamp_float(factor, 0.0f, 2.0f);
    alDopplerFactor(dopplerFactor);
    LOGD("Doppler factor set: %.2f", dopplerFactor);
}

void Audio3D::setSpeedOfSound(float speed) {
    speedOfSound = max_float(speed, 0.1f);
    alSpeedOfSound(speedOfSound);
    LOGD("Speed of sound set: %.2f m/s", speedOfSound);
}

void Audio3D::setGain(float gain) {
    masterGain = clamp_float(gain, 0.0f, 1.0f);
    alListenerf(AL_GAIN, masterGain);
    LOGD("Master gain set: %.2f", masterGain);
}

float Audio3D::calculateAttenuation(const glm::vec3& sourcePos, float referenceDistance,
                                    float maxDistance) const {
    // Calculate distance from listener to source
    float distance = distance_vec3(listenerPosition, sourcePos);

    // Silent if beyond max distance
    if (distance >= maxDistance) {
        return 0.0f;
    }

    // Full volume within reference distance
    if (distance <= referenceDistance) {
        return 1.0f;
    }

    // Inverse square law (natural 3D audio attenuation)
    float attenuation = referenceDistance / distance;

    return clamp_float(attenuation, 0.0f, 1.0f);
}
