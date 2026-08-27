#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <android/log.h>

#define LOG_TAG "Camera"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

Camera::Camera()
    : position(0.0f, 1.5f, 3.0f),
      forward(0.0f, 0.0f, -1.0f),
      right(1.0f, 0.0f, 0.0f),
      up(0.0f, 1.0f, 0.0f),
      fov(45.0f),
      pitch(0.0f),
      yaw(0.0f) {}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + forward, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    if (aspectRatio <= 0.0f || fov <= 0.0f) {
        LOGE("Invalid aspectRatio (%.3f) or fov (%.1f)", aspectRatio, fov);
        return glm::mat4();
    }
    return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 1000.0f);
}

void Camera::rotate(float p, float y) {
    pitch += p;
    yaw += y;

    // Clamp pitch to avoid gimbal lock
    pitch = std::clamp(pitch, -89.0f, 89.0f);

    // Recalculate forward/right/up vectors from pitch/yaw
    float radPitch = glm::radians(pitch);
    float radYaw = glm::radians(yaw);

    forward.x = std::cos(radPitch) * std::sin(radYaw);
    forward.y = std::sin(radPitch);
    forward.z = std::cos(radPitch) * std::cos(radYaw);
    forward = glm::normalize(forward);

    right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, forward));
}

void Camera::pan(const glm::vec3& direction) {
    position = position + direction;
    clampToBounds();
}

void Camera::setWorldBounds(float minX, float minY, float maxX, float maxY) {
    boundsMinX = minX;
    boundsMinY = minY;
    boundsMaxX = maxX;
    boundsMaxY = maxY;
    worldBoundsSet = true;
    clampToBounds();
}

void Camera::clearWorldBounds() {
    worldBoundsSet = false;
}

void Camera::clampToBounds() {
    if (!worldBoundsSet) return;

    // Clamp X and Z to world bounds (Y is height, not clamped)
    position.x = std::clamp(position.x, boundsMinX, boundsMaxX);
    position.z = std::clamp(position.z, boundsMinY, boundsMaxY);
}
