#pragma once

class Cube {
public:
    Cube();
    ~Cube();

    void render();
    void update(float deltaTime);

    // Position and rotation setters
    void setPosition(float x, float y, float z);
    void setRotation(float angle);

private:
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;
    unsigned int vertexCount;
    unsigned int shaderProgram;
    float rotationAngle;
    float posX, posY, posZ;  // Position in world space

    void initializeCube();
    void compileShaders();
};
