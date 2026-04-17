#include "cube.h"

Cube::Cube() : vao(0), vbo(0), ebo(0), vertexCount(36), rotationAngle(0.0f) {}

Cube::~Cube() {}

void Cube::render() {}

void Cube::update(float deltaTime) {
    rotationAngle += 45.0f * deltaTime;
    if (rotationAngle >= 360.0f) {
        rotationAngle -= 360.0f;
    }
}
