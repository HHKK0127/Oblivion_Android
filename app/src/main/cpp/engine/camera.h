#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    Camera();

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    void rotate(float pitch, float yaw);
    void pan(const glm::vec3& direction);

private:
    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 right;
    glm::vec3 up;
    float fov;
    float pitch;
    float yaw;
};
