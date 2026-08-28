#include "touch_calibration.h"
#include <algorithm>
#include <cstring>

TouchCalibration::TouchCalibration() {
    activeProfile = DeviceCalibrationProfile();
}

void TouchCalibration::initialize() {
    // Set up default profile
    activeProfile = DeviceCalibrationProfile();
    TCAL_LOGI("TouchCalibration initialized with default profile (deadZone=%.2f, curve=%d)",
              activeProfile.joystickDeadZone, static_cast<int>(activeProfile.joystickCurve));
}

float TouchCalibration::clamp01(float v) const {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

float TouchCalibration::applySensitivityCurve(float rawValue, SensitivityCurve curve, float exponent) const {
    float v = clamp01(std::abs(rawValue));
    float sign = (rawValue >= 0.0f) ? 1.0f : -1.0f;

    float result = 0.0f;
    switch (curve) {
        case SensitivityCurve::LINEAR:
            result = v;
            break;

        case SensitivityCurve::EXPONENTIAL:
            // Slow at low input, fast at high input
            // y = x^exponent
            result = std::pow(v, exponent);
            break;

        case SensitivityCurve::LOGARITHMIC:
            // Fast at low input, slow at high input
            // y = 1 - (1 - x)^(1/exponent)
            if (v <= 0.0f) {
                result = 0.0f;
            } else if (v >= 1.0f) {
                result = 1.0f;
            } else {
                result = 1.0f - std::pow(1.0f - v, 1.0f / exponent);
            }
            break;
    }

    return result * sign;
}

void TouchCalibration::applyDeadZone(float inputX, float inputY, float& outX, float& outY) const {
    float magnitude = std::sqrt(inputX * inputX + inputY * inputY);
    float deadZone = activeProfile.joystickDeadZone;

    if (magnitude < deadZone) {
        outX = 0.0f;
        outY = 0.0f;
        return;
    }

    // Normalize and rescale to [0, 1] range after dead zone removal
    float normalizedMagnitude = (magnitude - deadZone) / (1.0f - deadZone);
    normalizedMagnitude = clamp01(normalizedMagnitude);

    // Apply direction with normalized magnitude
    float dirX = inputX / magnitude;
    float dirY = inputY / magnitude;

    outX = dirX * normalizedMagnitude;
    outY = dirY * normalizedMagnitude;
}

void TouchCalibration::processJoystickInput(float rawX, float rawY, float& outX, float& outY) const {
    // Step 1: Apply dead zone
    float dzX = 0.0f, dzY = 0.0f;
    applyDeadZone(rawX, rawY, dzX, dzY);

    // Step 2: Apply sensitivity curve to magnitude
    float magnitude = std::sqrt(dzX * dzX + dzY * dzY);
    if (magnitude < 1e-6f) {
        outX = 0.0f;
        outY = 0.0f;
        return;
    }

    float curvedMagnitude = applySensitivityCurve(magnitude, activeProfile.joystickCurve,
                                                   activeProfile.joystickExponent);

    // Step 3: Reconstruct with curved magnitude
    float dirX = dzX / magnitude;
    float dirY = dzY / magnitude;
    outX = dirX * curvedMagnitude;
    outY = dirY * curvedMagnitude;
}

void TouchCalibration::setActiveProfile(const DeviceCalibrationProfile& profile) {
    activeProfile = profile;
    TCAL_LOGI("Active profile set: %s (deadZone=%.2f)",
              profile.deviceName.c_str(), profile.joystickDeadZone);
}

void TouchCalibration::saveProfile(const DeviceCalibrationProfile& profile) {
    savedProfiles[profile.deviceName] = profile;
    TCAL_LOGI("Profile saved: %s", profile.deviceName.c_str());
}

bool TouchCalibration::loadProfile(const std::string& deviceName) {
    auto it = savedProfiles.find(deviceName);
    if (it != savedProfiles.end()) {
        activeProfile = it->second;
        TCAL_LOGI("Profile loaded: %s", deviceName.c_str());
        return true;
    }
    TCAL_LOGW("Profile not found: %s", deviceName.c_str());
    return false;
}

std::vector<std::string> TouchCalibration::getSavedProfileNames() const {
    std::vector<std::string> names;
    names.reserve(savedProfiles.size());
    for (const auto& pair : savedProfiles) {
        names.push_back(pair.first);
    }
    return names;
}

void TouchCalibration::setJoystickDeadZone(float deadZone) {
    activeProfile.joystickDeadZone = std::max(0.0f, std::min(0.3f, deadZone));
}

void TouchCalibration::setJoystickCurve(SensitivityCurve curve) {
    activeProfile.joystickCurve = curve;
}

void TouchCalibration::setJoystickExponent(float exponent) {
    activeProfile.joystickExponent = std::max(1.0f, std::min(5.0f, exponent));
}

void TouchCalibration::setTapThreshold(float seconds) {
    activeProfile.tapThresholdSeconds = std::max(0.05f, std::min(1.0f, seconds));
}

void TouchCalibration::setSwipeMinDistance(float pixels) {
    activeProfile.swipeMinDistance = std::max(10.0f, std::min(500.0f, pixels));
}

void TouchCalibration::setPinchSensitivity(float sensitivity) {
    activeProfile.pinchScaleSensitivity = std::max(0.1f, std::min(5.0f, sensitivity));
}

void TouchCalibration::setCameraSensitivityX(float sensitivity) {
    activeProfile.cameraSensitivityX = std::max(0.1f, std::min(5.0f, sensitivity));
}

void TouchCalibration::setCameraSensitivityY(float sensitivity) {
    activeProfile.cameraSensitivityY = std::max(0.1f, std::min(5.0f, sensitivity));
}
