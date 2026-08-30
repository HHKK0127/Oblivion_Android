#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

class TextRenderer;

/**
 * @brief Performance Graph Display
 *
 * Shows real-time performance graphs:
 * - FPS graph (last 60 frames)
 * - Frame time graph
 * - Memory usage (if available)
 * - Draw call count
 */
class PerformanceGraph {
public:
    PerformanceGraph();
    ~PerformanceGraph();

    bool initialize(TextRenderer* textRenderer);
    void cleanup();

    void toggle();
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

    void update(float deltaTime);
    void render();

    // Data input
    void addFrameTime(float ms);
    void setDrawCalls(int calls) { drawCalls = calls; }
    void setMemoryUsage(float mb) { memoryMB = mb; }

private:
    TextRenderer* textRenderer;
    bool visible;
    bool initialized;

    int screenWidth;
    int screenHeight;

    // Performance data
    static const int HISTORY_SIZE = 120;
    std::vector<float> frameTimeHistory;
    std::vector<float> fpsHistory;
    int historyIndex;

    float currentFPS;
    float currentFrameTime;
    float avgFPS;
    float minFPS;
    float maxFPS;
    int drawCalls;
    float memoryMB;

    // Graph rendering
    void renderGraph(float x, float y, float width, float height,
                     const std::vector<float>& data, float maxVal,
                     const glm::vec3& color, const std::string& label);
    void renderStats(float x, float y, float scale);
    float calculateAverage(const std::vector<float>& data) const;
    float calculateMin(const std::vector<float>& data) const;
    float calculateMax(const std::vector<float>& data) const;
};
