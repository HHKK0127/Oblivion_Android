#pragma once

#include <glm/glm.hpp>
#include <android/log.h>

// Note: OpenAL not needed for Java MediaPlayer approach via JNI

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGW
#undef LOGE

#define LOG_TAG "Audio3D"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * @brief 3D spatial audio processing
 * Manages listener position/orientation and distance attenuation models
 *
 * Unified management of OpenAL 3D spatial audio features
 */
class Audio3D {
public:
    /**
     * @brief Distance attenuation model types
     */
    enum class DistanceModel {
        INVERSE_DISTANCE,           // Natural attenuation based on distance
        INVERSE_DISTANCE_CLAMPED,   // Clamped (min/max distance settings)
        LINEAR_DISTANCE,            // Linear attenuation
        EXPONENT_DISTANCE           // Exponential attenuation
    };

    /**
     * @brief Constructor
     */
    Audio3D();

    /**
     * @brief Destructor
     */
    ~Audio3D();

    /**
     * @brief Set listener position
     * @param pos World coordinates
     */
    void setListenerPosition(const glm::vec3& pos);

    /**
     * @brief Set listener orientation
     * @param forward Forward direction vector (normalized)
     * @param up Up direction vector (normalized)
     */
    void setListenerOrientation(const glm::vec3& forward, const glm::vec3& up);

    /**
     * @brief Set listener velocity (for Doppler effect)
     * @param vel Velocity vector
     */
    void setListenerVelocity(const glm::vec3& vel);

    /**
     * @brief Set distance attenuation model
     * @param model Attenuation model
     */
    void setDistanceModel(DistanceModel model);

    /**
     * @brief Set Doppler factor (0.0 = disabled, 1.0 = natural)
     * @param factor Doppler factor
     */
    void setDopplerFactor(float factor);

    /**
     * @brief Set speed of sound (for air check)
     * @param speed Speed of sound (m/s, default 343.0)
     */
    void setSpeedOfSound(float speed);

    /**
     * @brief Set gain (master volume)
     * @param gain 0.0 - 1.0
     */
    void setGain(float gain);

    /**
     * @brief Calculate distance attenuation (inverse square law)
     * @param sourcePos Source position
     * @param referenceDistance Reference distance (attenuation start distance, default 1.0m)
     * @param maxDistance Maximum distance (silence beyond this, default infinite)
     * @return Attenuation coefficient (0.0 - 1.0)
     */
    float calculateAttenuation(const glm::vec3& sourcePos, float referenceDistance = 1.0f,
                              float maxDistance = 1000.0f) const;

    /**
     * @brief Get current listener position
     */
    const glm::vec3& getListenerPosition() const { return listenerPosition; }

    /**
     * @brief Get current listener forward direction
     */
    const glm::vec3& getListenerForward() const { return listenerForward; }

    /**
     * @brief Get current listener up direction
     */
    const glm::vec3& getListenerUp() const { return listenerUp; }

private:
    // Listener state
    glm::vec3 listenerPosition;      // World coordinates
    glm::vec3 listenerVelocity;      // Velocity for Doppler effect
    glm::vec3 listenerForward;       // Forward direction (normalized)
    glm::vec3 listenerUp;            // Up direction (normalized)

    // Configuration
    float dopplerFactor;             // 0.0 - 2.0 (default 1.0)
    float speedOfSound;              // m/s (default 343.0)
    float masterGain;                // 0.0 - 1.0 (default 1.0)
};


