#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    Camera();

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    void rotate(float pitch, float yaw);
    void pan(const glm::vec3& direction);

    // World bounds
    void setWorldBounds(float minX, float minY, float maxX, float maxY);
    void clearWorldBounds();
    bool hasWorldBounds() const { return worldBoundsSet; }

private:
    glm::vec3 position;
    glm::vec3 forward;
    glm::vec3 right;
    glm::vec3 up;
    float fov;
    float pitch;
    float yaw;

    // World bounds
    bool worldBoundsSet = false;
    float boundsMinX = 0.0f;
    float boundsMinY = 0.0f;
    float boundsMaxX = 0.0f;
    float boundsMaxY = 0.0f;

    void clampToBounds();
};
