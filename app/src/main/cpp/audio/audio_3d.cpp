#include "audio_3d.h"
#include <glm/glm.hpp>

Audio3D::Audio3D()
    : listenerPosition(0.0f, 0.0f, 0.0f),
      listenerVelocity(0.0f, 0.0f, 0.0f),
      listenerForward(0.0f, 0.0f, -1.0f),
      listenerUp(0.0f, 1.0f, 0.0f),
      dopplerFactor(1.0f),
      speedOfSound(343.0f),
      masterGain(1.0f) {
    LOGD("Audio3D initialized");

    // OpenAL グローバル設定
    alDopplerFactor(dopplerFactor);
    alSpeedOfSound(speedOfSound);

    // デフォルト距離減衰モデル
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
    listenerForward = glm::normalize(forward);
    listenerUp = glm::normalize(up);

    // OpenAL 要求形式：[forward.x, forward.y, forward.z, up.x, up.y, up.z]
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
    dopplerFactor = glm::clamp(factor, 0.0f, 2.0f);
    alDopplerFactor(dopplerFactor);
    LOGD("Doppler factor set: %.2f", dopplerFactor);
}

void Audio3D::setSpeedOfSound(float speed) {
    speedOfSound = glm::max(speed, 0.1f);
    alSpeedOfSound(speedOfSound);
    LOGD("Speed of sound set: %.2f m/s", speedOfSound);
}

void Audio3D::setGain(float gain) {
    masterGain = glm::clamp(gain, 0.0f, 1.0f);
    alListenerf(AL_GAIN, masterGain);
    LOGD("Master gain set: %.2f", masterGain);
}

float Audio3D::calculateAttenuation(const glm::vec3& sourcePos, float referenceDistance,
                                   float maxDistance) const {
    // リスナーから音源への距離を計算
    float distance = glm::distance(listenerPosition, sourcePos);

    // 最大距離を超えた場合は無音
    if (distance >= maxDistance) {
        return 0.0f;
    }

    // リファレンス距離より近い場合は最大音量
    if (distance <= referenceDistance) {
        return 1.0f;
    }

    // 逆二乗則（自然な3D音響減衰）
    // attenuation = referenceDistance / distance
    float attenuation = referenceDistance / distance;

    // リスナー距離でクランプ（オプション）
    return glm::clamp(attenuation, 0.0f, 1.0f);
}
