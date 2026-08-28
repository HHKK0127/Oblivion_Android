#pragma once

#include <string>
#include <map>
#include <vector>
#include <cmath>
#include <android/log.h>

#define TOUCH_CAL_LOG_TAG "TouchCalibration"
#define TCAL_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TOUCH_CAL_LOG_TAG, __VA_ARGS__)
#define TCAL_LOGI(...) __android_log_print(ANDROID_LOG_INFO, TOUCH_CAL_LOG_TAG, __VA_ARGS__)
#define TCAL_LOGW(...) __android_log_print(ANDROID_LOG_WARN, TOUCH_CAL_LOG_TAG, __VA_ARGS__)

// Sensitivity curve types for input mapping
enum class SensitivityCurve {
    LINEAR,        // Direct 1:1 mapping
    EXPONENTIAL,   // Slow start, fast end (precision at low input)
    LOGARITHMIC    // Fast start, slow end (responsiveness at low input)
};

// Per-device calibration profile
struct DeviceCalibrationProfile {
    std::string deviceName;

    // Virtual joystick dead zone (0.0 - 0.3)
    float joystickDeadZone;

    // Sensitivity curve for joystick input
    SensitivityCurve joystickCurve;
    float joystickExponent;  // For EXPONENTIAL curve (2.0 - 4.0)

    // Touch gesture tuning
    float tapThresholdSeconds;    // Max duration for tap (seconds)
    float swipeMinDistance;       // Minimum swipe distance (pixels)
    float pinchScaleSensitivity;  // Pinch zoom sensitivity multiplier

    // Camera look sensitivity
    float cameraSensitivityX;
    float cameraSensitivityY;

    DeviceCalibrationProfile()
        : deviceName("default"),
          joystickDeadZone(0.15f),
          joystickCurve(SensitivityCurve::LINEAR),
          joystickExponent(2.0f),
          tapThresholdSeconds(0.3f),
          swipeMinDistance(50.0f),
          pinchScaleSensitivity(1.0f),
          cameraSensitivityX(1.0f),
          cameraSensitivityY(1.0f) {}
};

// Touch calibration system
// Manages dead zones, sensitivity curves, gesture tuning, and per-device profiles
class TouchCalibration {
public:
    TouchCalibration();
    ~TouchCalibration() = default;

    // Initialize with default profile
    void initialize();

    // Apply a sensitivity curve to a raw input value (0.0 - 1.0)
    // Returns the curved output (0.0 - 1.0)
    float applySensitivityCurve(float rawValue, SensitivityCurve curve, float exponent = 2.0f) const;

    // Apply dead zone to a 2D joystick input
    // Returns the adjusted input with dead zone removed
    void applyDeadZone(float inputX, float inputY, float& outX, float& outY) const;

    // Apply full joystick processing (dead zone + curve)
    void processJoystickInput(float rawX, float rawY, float& outX, float& outY) const;

    // Profile management
    void setActiveProfile(const DeviceCalibrationProfile& profile);
    const DeviceCalibrationProfile& getActiveProfile() const { return activeProfile; }

    // Save/load profiles by device name
    void saveProfile(const DeviceCalibrationProfile& profile);
    bool loadProfile(const std::string& deviceName);
    std::vector<std::string> getSavedProfileNames() const;

    // Individual parameter setters (modify active profile)
    void setJoystickDeadZone(float deadZone);
    void setJoystickCurve(SensitivityCurve curve);
    void setJoystickExponent(float exponent);
    void setTapThreshold(float seconds);
    void setSwipeMinDistance(float pixels);
    void setPinchSensitivity(float sensitivity);
    void setCameraSensitivityX(float sensitivity);
    void setCameraSensitivityY(float sensitivity);

    // Getters
    float getJoystickDeadZone() const { return activeProfile.joystickDeadZone; }
    SensitivityCurve getJoystickCurve() const { return activeProfile.joystickCurve; }
    float getTapThreshold() const { return activeProfile.tapThresholdSeconds; }
    float getSwipeMinDistance() const { return activeProfile.swipeMinDistance; }
    float getPinchSensitivity() const { return activeProfile.pinchScaleSensitivity; }

private:
    DeviceCalibrationProfile activeProfile;
    std::map<std::string, DeviceCalibrationProfile> savedProfiles;

    // Clamp value to [0, 1]
    float clamp01(float v) const;
};
