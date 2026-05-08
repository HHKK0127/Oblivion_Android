#include "cube.h"
#include <GLES3/gl3.h>
#include <android/log.h>
#include <cmath>
#include <cstring>

#undef LOG_TAG
#undef LOGD
#undef LOGE
#define LOG_TAG "Cube"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Simple vertex shader
const char* vertexShaderSource = R"(#version 300 es
precision mediump float;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 vertexColor;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    vertexColor = color;
}
)";

// Simple fragment shader
const char* fragmentShaderSource = R"(#version 300 es
precision mediump float;

in vec3 vertexColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vertexColor, 1.0);
}
)";

Cube::Cube() : vao(0), vbo(0), ebo(0), vertexCount(36), rotationAngle(0.0f), shaderProgram(0),
               posX(0.0f), posY(0.0f), posZ(3.0f) {
    initializeCube();
}

Cube::~Cube() {}

void Cube::initializeCube() {
    LOGD("=== Cube::initializeCube() called ===");

    // Compile shaders
    LOGD("Calling compileShaders()...");
    compileShaders();
    LOGD("compileShaders() completed. shaderProgram=%u", shaderProgram);

    // Cube vertices: 24 vertices for a cube with distinct colors per face
    float vertices[] = {
        // Front face (Red)
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,

        // Back face (Green)
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,

        // Top face (Blue)
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,

        // Bottom face (Yellow)
        -0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 1.0f, 0.0f,

        // Left face (Cyan)
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f,

        // Right face (Magenta)
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 1.0f,
    };

    // Indices for cube
    unsigned short indices[] = {
        // Front
        0, 1, 2, 2, 3, 0,
        // Back
        4, 6, 5, 6, 7, 4,
        // Top
        8, 10, 9, 10, 11, 8,
        // Bottom
        12, 13, 14, 14, 15, 12,
        // Left
        16, 18, 17, 18, 19, 16,
        // Right
        20, 21, 22, 22, 23, 20
    };

    // Generate VAO and VBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    // VBO: Vertex data
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // EBO: Index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Vertex attributes
    // Position (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color (3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    LOGD("=== Cube initialization COMPLETE ===");
    LOGD("VAO=%u, VBO=%u, EBO=%u, ShaderProgram=%u", vao, vbo, ebo, shaderProgram);
    LOGD("Vertex Count: %u", vertexCount);
}

void Cube::render() {
    LOGD("=== Cube::render() called ===");
    LOGD("VAO=%u, ShaderProgram=%u", vao, shaderProgram);

    if (vao == 0) {
        LOGE("ERROR: VAO not initialized (vao=0)");
        return;
    }

    if (shaderProgram == 0) {
        LOGE("ERROR: ShaderProgram not initialized (shaderProgram=0)");
        return;
    }

    LOGD("Using shader program %u", shaderProgram);
    glUseProgram(shaderProgram);

    // Model matrix: rotation + scale + translation
    // Rotate 45 degrees around Y and X to show corner view of cube
    float angle_y = 3.14159f / 4.0f;  // 45 degrees in radians
    float angle_x = 3.14159f / 6.0f;  // 30 degrees in radians
    float cos_y = cosf(angle_y);
    float sin_y = sinf(angle_y);
    float cos_x = cosf(angle_x);
    float sin_x = sinf(angle_x);

    float scale = 0.3f;

    // Rotation Y matrix times Rotation X matrix times Scale, in column-major
    // This is simplified for isometric-like view
    float model[16] = {
        scale * cos_y,           scale * sin_y * sin_x,  -scale * sin_y * cos_x,  0,
        0,                       scale * cos_x,           scale * sin_x,           0,
        scale * sin_y,          -scale * cos_y * sin_x,   scale * cos_y * cos_x,  0,
        posX,                    posY,                     posZ,                    1
    };

    // View matrix: identity (camera at origin looking down +Z)
    float view[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };

    // Projection matrix: simple orthographic-like for now (will switch to perspective)
    // Using a simple scale that works
    float projection[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,-0.01f,-1,
        0,0,-0.01f,0
    };

    // Pass matrices to shader
    LOGD("Getting uniform locations...");
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

    LOGD("Uniform locations: model=%d, view=%d, projection=%d", modelLoc, viewLoc, projLoc);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
    LOGD("Model matrix set");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
    LOGD("View matrix set");

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
    LOGD("Projection matrix set");

    // Render cube
    LOGD("Binding VAO %u", vao);
    glBindVertexArray(vao);

    LOGD("Drawing 36 indices with GL_TRIANGLES");
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);

    glBindVertexArray(0);
    glUseProgram(0);

    LOGD("=== Cube rendered successfully ===");
}

void Cube::update(float deltaTime) {
    rotationAngle += 45.0f * deltaTime;
    if (rotationAngle >= 360.0f) {
        rotationAngle -= 360.0f;
    }
}

void Cube::setPosition(float x, float y, float z) {
    posX = x;
    posY = y;
    posZ = z;
    LOGD("Cube position set to: (%.2f, %.2f, %.2f)", posX, posY, posZ);
}

void Cube::setRotation(float angle) {
    rotationAngle = angle;
    if (rotationAngle >= 360.0f) {
        rotationAngle -= 360.0f;
    }
    LOGD("Cube rotation set to: %.2f degrees", angle);
}

void Cube::compileShaders() {
    LOGD("=== Starting shader compilation ===");

    // Compile vertex shader
    LOGD("Creating vertex shader...");
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    LOGD("Vertex shader ID: %u", vertexShader);

    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    LOGD("Vertex shader source set");

    glCompileShader(vertexShader);
    LOGD("Vertex shader compilation invoked");

    // Check vertex shader compilation
    int success;
    char infoLog[512];
    memset(infoLog, 0, sizeof(infoLog));

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    LOGD("Vertex shader compile status: %d", success);

    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        LOGE("Vertex shader compilation FAILED: %s", infoLog);
        glDeleteShader(vertexShader);
        return;
    }
    LOGD("Vertex shader compiled successfully");

    // Compile fragment shader
    LOGD("Creating fragment shader...");
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    LOGD("Fragment shader ID: %u", fragmentShader);

    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    LOGD("Fragment shader source set");

    glCompileShader(fragmentShader);
    LOGD("Fragment shader compilation invoked");

    // Check fragment shader compilation
    memset(infoLog, 0, sizeof(infoLog));
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    LOGD("Fragment shader compile status: %d", success);

    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        LOGE("Fragment shader compilation FAILED: %s", infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }
    LOGD("Fragment shader compiled successfully");

    // Link program
    LOGD("Creating shader program...");
    shaderProgram = glCreateProgram();
    LOGD("Shader program ID: %u", shaderProgram);

    glAttachShader(shaderProgram, vertexShader);
    LOGD("Vertex shader attached");

    glAttachShader(shaderProgram, fragmentShader);
    LOGD("Fragment shader attached");

    glLinkProgram(shaderProgram);
    LOGD("Shader program linking invoked");

    // Check program linking
    memset(infoLog, 0, sizeof(infoLog));
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    LOGD("Program linking status: %d", success);

    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        LOGE("Shader program linking FAILED: %s", infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
        return;
    }
    LOGD("Shader program linked successfully");

    // Clean up
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    LOGD("=== Shader compilation COMPLETE: ID=%u ===", shaderProgram);
}
