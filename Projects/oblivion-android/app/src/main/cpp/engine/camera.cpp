#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

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
    return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 100.0f);
}

void Camera::rotate(float p, float y) {
    pitch += p;
    yaw += y;
}

void Camera::pan(const glm::vec3& direction) {
    position += direction;
}
