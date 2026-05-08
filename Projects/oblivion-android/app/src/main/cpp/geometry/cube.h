#pragma once

class Cube {
public:
    Cube();
    ~Cube();

    void render();
    void update(float deltaTime);

private:
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;
    unsigned int vertexCount;
    float rotationAngle;
};
