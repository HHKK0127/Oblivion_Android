#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <cstdint>
#include <android/log.h>

#define INPUT_VIZ_LOG_TAG "InputVisualizer"
#define IVIZ_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, INPUT_VIZ_LOG_TAG, __VA_ARGS__)
#define IVIZ_LOGI(...) __android_log_print(ANDROID_LOG_INFO, INPUT_VIZ_LOG_TAG, __VA_ARGS__)

// Visualizer display flags (can be combined)
enum class VisualizerFlag : uint32_t {
    NONE              = 0,
    TOUCH_POINTS      = (1 << 0),
    JOYSTICK_STATE    = (1 << 1),
    BUTTON_HIGHLIGHT  = (1 << 2),
    GESTURE_TRAIL     = (1 << 3),
    LATENCY_DISPLAY   = (1 << 4),
    ALL               = 0xFFFFFFFF
};

inline VisualizerFlag operator|(VisualizerFlag a, VisualizerFlag b) {
    return static_cast<VisualizerFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline VisualizerFlag operator&(VisualizerFlag a, VisualizerFlag b) {
    return static_cast<VisualizerFlag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool hasFlag(VisualizerFlag flags, VisualizerFlag flag) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// Touch point visualization data
struct TouchPointViz {
    glm::vec2 position;
    float radius;
    float age;           // Seconds since touch began
    bool active;
    int pointerId;

    TouchPointViz()
        : position(0.0f, 0.0f), radius(30.0f), age(0.0f), active(false), pointerId(-1) {}
};

// Joystick visualization state
struct JoystickViz {
    glm::vec2 center;       // Center position on screen
    float outerRadius;      // Outer ring radius
    glm::vec2 knobPos;      // Current knob position
    glm::vec2 direction;    // Normalized direction
    float magnitude;        // 0.0 - 1.0
    bool active;

    JoystickViz()
        : center(0.0f, 0.0f), outerRadius(100.0f),
          knobPos(0.0f, 0.0f), direction(0.0f, 0.0f),
          magnitude(0.0f), active(false) {}
};

// Button highlight data
struct ButtonHighlightViz {
    glm::vec2 position;
    glm::vec2 size;
    std::string label;
    bool pressed;
    float pressIntensity;   // 0.0 - 1.0 for animation

    ButtonHighlightViz()
        : position(0.0f, 0.0f), size(0.0f, 0.0f),
          pressed(false), pressIntensity(0.0f) {}
};

// Gesture trail point
struct TrailPoint {
    glm::vec2 position;
    float age;

    TrailPoint() : position(0.0f, 0.0f), age(0.0f) {}
    TrailPoint(const glm::vec2& pos, float t) : position(pos), age(t) {}
};

// Latency measurement data
struct LatencyData {
    float inputToRenderMs;   // Touch event to render frame
    float frameTimeMs;       // Last frame time
    float avgFrameTimeMs;    // Average frame time (rolling)
    int frameCount;

    LatencyData()
        : inputToRenderMs(0.0f), frameTimeMs(0.0f),
          avgFrameTimeMs(0.0f), frameCount(0) {}
};

// Render command for the visualization overlay
// The renderer reads these to draw debug visuals
struct VisualizerRenderCommand {
    enum Type { CIRCLE, RING, LINE, TEXT, RECT };
    Type type;
    glm::vec2 position;
    glm::vec2 position2;    // For lines
    glm::vec4 color;
    float radius;
    float thickness;
    std::string text;

    VisualizerRenderCommand()
        : type(CIRCLE), position(0.0f, 0.0f), position2(0.0f, 0.0f),
          color(1.0f, 1.0f, 1.0f, 1.0f), radius(0.0f), thickness(1.0f) {}
};

// Input visualizer for debug overlay
// Generates render commands for touch points, joystick state,
// button highlights, gesture trails, and latency display
class InputVisualizer {
public:
    InputVisualizer();
    ~InputVisualizer() = default;

    // Initialize the visualizer
    void initialize(int screenWidth, int screenHeight);

    // Enable/disable specific visualization features
    void setFlags(VisualizerFlag flags) { displayFlags = flags; }
    VisualizerFlag getFlags() const { return displayFlags; }
    void enableFlag(VisualizerFlag flag);
    void disableFlag(VisualizerFlag flag);

    // Update all visualization data (call each frame)
    void update(float deltaTime);

    // Touch point tracking
    void onTouchDown(float x, float y, int pointerId);
    void onTouchMove(float x, float y, int pointerId);
    void onTouchUp(float x, float y, int pointerId);

    // Joystick state update
    void setJoystickState(const glm::vec2& center, float outerRadius,
                          const glm::vec2& knobPos, const glm::vec2& input);

    // Button state update
    void setButtonState(const glm::vec2& position, const glm::vec2& size,
                        const std::string& label, bool pressed);

    // Latency measurement
    void recordFrameTime(float frameTimeMs);
    void recordInputLatency(float latencyMs);

    // Generate render commands for the current frame
    const std::vector<VisualizerRenderCommand>& getRenderCommands() const { return commands; }

    // Get latency data for display
    const LatencyData& getLatencyData() const { return latencyData; }

private:
    VisualizerFlag displayFlags;
    int screenWidth;
    int screenHeight;

    // Touch point tracking (max 10 pointers)
    static constexpr int MAX_POINTERS = 10;
    TouchPointViz touchPoints[MAX_POINTERS];

    // Joystick visualization
    JoystickViz joystickViz;

    // Button highlights
    static constexpr int MAX_BUTTONS = 16;
    ButtonHighlightViz buttonHighlights[MAX_BUTTONS];
    int buttonCount;

    // Gesture trail
    static constexpr int MAX_TRAIL_POINTS = 64;
    TrailPoint trailPoints[MAX_TRAIL_POINTS];
    int trailHead;
    int trailCount;

    // Latency tracking
    LatencyData latencyData;
    static constexpr int FRAME_HISTORY_SIZE = 60;
    float frameTimeHistory[FRAME_HISTORY_SIZE];
    int frameHistoryIndex;

    // Render commands (rebuilt each frame)
    std::vector<VisualizerRenderCommand> commands;

    // Trail max age (seconds)
    float trailMaxAge;

    // Build render commands
    void buildTouchPointCommands();
    void buildJoystickCommands();
    void buildButtonCommands();
    void buildTrailCommands();
    void buildLatencyCommands();
};
