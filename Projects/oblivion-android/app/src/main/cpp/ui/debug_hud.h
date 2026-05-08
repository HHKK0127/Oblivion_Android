#pragma once

#include <string>
#include <cstdint>
#include <chrono>

class TextRenderer;

class DebugHUD {
public:
    DebugHUD();
    ~DebugHUD();

    bool initialize(TextRenderer* textRenderer);
    void cleanup();

    void update(float deltaTime);
    void render();

    void setFPS(float fps);
    void setDrawCalls(int calls);
    void setTriangleCount(int count);
    void setMemoryUsage(size_t bytes);

private:
    TextRenderer* textRenderer;
    float currentFPS;
    int drawCalls;
    int triangleCount;
    size_t memoryUsageBytes;
    bool initialized;
};