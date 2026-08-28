#include "input_visualizer.h"
#include <cmath>
#include <cstring>
#include <algorithm>

InputVisualizer::InputVisualizer()
    : displayFlags(VisualizerFlag::NONE),
      screenWidth(1080), screenHeight(1920),
      buttonCount(0), trailHead(0), trailCount(0),
      frameHistoryIndex(0), trailMaxAge(2.0f) {
    std::memset(touchPoints, 0, sizeof(touchPoints));
    std::memset(frameTimeHistory, 0, sizeof(frameTimeHistory));
}

void InputVisualizer::initialize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    IVIZ_LOGI("InputVisualizer initialized (%dx%d)", width, height);
}

void InputVisualizer::enableFlag(VisualizerFlag flag) {
    displayFlags = displayFlags | flag;
}

void InputVisualizer::disableFlag(VisualizerFlag flag) {
    displayFlags = static_cast<VisualizerFlag>(
        static_cast<uint32_t>(displayFlags) & ~static_cast<uint32_t>(flag));
}

void InputVisualizer::update(float deltaTime) {
    commands.clear();

    // Update touch point ages
    for (int i = 0; i < MAX_POINTERS; i++) {
        if (touchPoints[i].active) {
            touchPoints[i].age += deltaTime;
        }
    }

    // Update trail point ages and remove expired
    for (int i = 0; i < trailCount; i++) {
        int idx = (trailHead - trailCount + i + MAX_TRAIL_POINTS) % MAX_TRAIL_POINTS;
        trailPoints[idx].age += deltaTime;
    }
    // Remove expired trail points from front
    while (trailCount > 0) {
        int idx = trailHead - trailCount + MAX_TRAIL_POINTS;
        idx = idx % MAX_TRAIL_POINTS;
        if (trailPoints[idx].age > trailMaxAge) {
            trailCount--;
        } else {
            break;
        }
    }

    // Update button press animations
    for (int i = 0; i < buttonCount; i++) {
        if (buttonHighlights[i].pressed) {
            buttonHighlights[i].pressIntensity = std::min(1.0f,
                buttonHighlights[i].pressIntensity + deltaTime * 5.0f);
        } else {
            buttonHighlights[i].pressIntensity = std::max(0.0f,
                buttonHighlights[i].pressIntensity - deltaTime * 3.0f);
        }
    }

    // Build render commands based on active flags
    if (hasFlag(displayFlags, VisualizerFlag::TOUCH_POINTS)) {
        buildTouchPointCommands();
    }
    if (hasFlag(displayFlags, VisualizerFlag::JOYSTICK_STATE)) {
        buildJoystickCommands();
    }
    if (hasFlag(displayFlags, VisualizerFlag::BUTTON_HIGHLIGHT)) {
        buildButtonCommands();
    }
    if (hasFlag(displayFlags, VisualizerFlag::GESTURE_TRAIL)) {
        buildTrailCommands();
    }
    if (hasFlag(displayFlags, VisualizerFlag::LATENCY_DISPLAY)) {
        buildLatencyCommands();
    }
}

void InputVisualizer::onTouchDown(float x, float y, int pointerId) {
    if (pointerId < 0 || pointerId >= MAX_POINTERS) return;

    touchPoints[pointerId].position = glm::vec2(x, y);
    touchPoints[pointerId].active = true;
    touchPoints[pointerId].age = 0.0f;
    touchPoints[pointerId].pointerId = pointerId;

    // Add trail point
    trailPoints[trailHead] = TrailPoint(glm::vec2(x, y), 0.0f);
    trailHead = (trailHead + 1) % MAX_TRAIL_POINTS;
    if (trailCount < MAX_TRAIL_POINTS) trailCount++;
}

void InputVisualizer::onTouchMove(float x, float y, int pointerId) {
    if (pointerId < 0 || pointerId >= MAX_POINTERS) return;
    if (!touchPoints[pointerId].active) return;

    touchPoints[pointerId].position = glm::vec2(x, y);

    // Add trail point
    trailPoints[trailHead] = TrailPoint(glm::vec2(x, y), 0.0f);
    trailHead = (trailHead + 1) % MAX_TRAIL_POINTS;
    if (trailCount < MAX_TRAIL_POINTS) trailCount++;
}

void InputVisualizer::onTouchUp(float /*x*/, float /*y*/, int pointerId) {
    if (pointerId < 0 || pointerId >= MAX_POINTERS) return;
    touchPoints[pointerId].active = false;
}

void InputVisualizer::setJoystickState(const glm::vec2& center, float outerRadius,
                                        const glm::vec2& knobPos, const glm::vec2& input) {
    joystickViz.center = center;
    joystickViz.outerRadius = outerRadius;
    joystickViz.knobPos = knobPos;
    joystickViz.direction = input;
    joystickViz.magnitude = std::sqrt(input.x * input.x + input.y * input.y);
    joystickViz.active = joystickViz.magnitude > 0.01f;
}

void InputVisualizer::setButtonState(const glm::vec2& position, const glm::vec2& size,
                                      const std::string& label, bool pressed) {
    // Find existing or add new
    for (int i = 0; i < buttonCount; i++) {
        if (buttonHighlights[i].label == label) {
            buttonHighlights[i].position = position;
            buttonHighlights[i].size = size;
            buttonHighlights[i].pressed = pressed;
            return;
        }
    }

    // Add new button
    if (buttonCount < MAX_BUTTONS) {
        buttonHighlights[buttonCount].position = position;
        buttonHighlights[buttonCount].size = size;
        buttonHighlights[buttonCount].label = label;
        buttonHighlights[buttonCount].pressed = pressed;
        buttonHighlights[buttonCount].pressIntensity = 0.0f;
        buttonCount++;
    }
}

void InputVisualizer::recordFrameTime(float frameTimeMs) {
    frameTimeHistory[frameHistoryIndex] = frameTimeMs;
    frameHistoryIndex = (frameHistoryIndex + 1) % FRAME_HISTORY_SIZE;

    latencyData.frameTimeMs = frameTimeMs;
    latencyData.frameCount++;

    // Calculate rolling average
    float sum = 0.0f;
    int count = std::min(latencyData.frameCount, FRAME_HISTORY_SIZE);
    for (int i = 0; i < count; i++) {
        sum += frameTimeHistory[i];
    }
    latencyData.avgFrameTimeMs = sum / static_cast<float>(count);
}

void InputVisualizer::recordInputLatency(float latencyMs) {
    latencyData.inputToRenderMs = latencyMs;
}

void InputVisualizer::buildTouchPointCommands() {
    for (int i = 0; i < MAX_POINTERS; i++) {
        if (!touchPoints[i].active) continue;

        VisualizerRenderCommand cmd;
        cmd.type = VisualizerRenderCommand::CIRCLE;
        cmd.position = touchPoints[i].position;
        cmd.radius = touchPoints[i].radius;
        // Fade color based on age
        float alpha = std::max(0.3f, 1.0f - touchPoints[i].age * 0.5f);
        cmd.color = glm::vec4(0.0f, 1.0f, 0.5f, alpha);
        commands.push_back(cmd);

        // Pointer ID text
        VisualizerRenderCommand textCmd;
        textCmd.type = VisualizerRenderCommand::TEXT;
        textCmd.position = glm::vec2(touchPoints[i].position.x + 20.0f,
                                      touchPoints[i].position.y - 20.0f);
        textCmd.text = "P" + std::to_string(touchPoints[i].pointerId);
        textCmd.color = glm::vec4(1.0f, 1.0f, 1.0f, alpha);
        commands.push_back(textCmd);
    }
}

void InputVisualizer::buildJoystickCommands() {
    // Outer ring
    VisualizerRenderCommand outerRing;
    outerRing.type = VisualizerRenderCommand::RING;
    outerRing.position = joystickViz.center;
    outerRing.radius = joystickViz.outerRadius;
    outerRing.thickness = 3.0f;
    outerRing.color = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);
    commands.push_back(outerRing);

    // Inner knob
    VisualizerRenderCommand knob;
    knob.type = VisualizerRenderCommand::CIRCLE;
    knob.position = joystickViz.knobPos;
    knob.radius = 20.0f;
    knob.color = joystickViz.active
        ? glm::vec4(0.0f, 0.8f, 1.0f, 0.8f)
        : glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
    commands.push_back(knob);

    // Direction line
    if (joystickViz.active) {
        VisualizerRenderCommand dirLine;
        dirLine.type = VisualizerRenderCommand::LINE;
        dirLine.position = joystickViz.center;
        dirLine.position2 = glm::vec2(
            joystickViz.center.x + joystickViz.direction.x * joystickViz.outerRadius,
            joystickViz.center.y + joystickViz.direction.y * joystickViz.outerRadius);
        dirLine.color = glm::vec4(1.0f, 0.8f, 0.0f, 0.6f);
        dirLine.thickness = 2.0f;
        commands.push_back(dirLine);

        // Magnitude text
        VisualizerRenderCommand magText;
        magText.type = VisualizerRenderCommand::TEXT;
        magText.position = glm::vec2(joystickViz.center.x,
                                      joystickViz.center.y + joystickViz.outerRadius + 20.0f);
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", joystickViz.magnitude);
        magText.text = std::string("Mag: ") + buf;
        magText.color = glm::vec4(1.0f, 1.0f, 0.0f, 0.9f);
        commands.push_back(magText);
    }
}

void InputVisualizer::buildButtonCommands() {
    for (int i = 0; i < buttonCount; i++) {
        if (buttonHighlights[i].pressIntensity < 0.01f) continue;

        VisualizerRenderCommand cmd;
        cmd.type = VisualizerRenderCommand::RECT;
        cmd.position = buttonHighlights[i].position;
        cmd.position2 = glm::vec2(
            buttonHighlights[i].position.x + buttonHighlights[i].size.x,
            buttonHighlights[i].position.y + buttonHighlights[i].size.y);
        float intensity = buttonHighlights[i].pressIntensity;
        cmd.color = glm::vec4(1.0f, 0.3f, 0.0f, intensity * 0.6f);
        commands.push_back(cmd);

        // Label
        VisualizerRenderCommand label;
        label.type = VisualizerRenderCommand::TEXT;
        label.position = glm::vec2(buttonHighlights[i].position.x,
                                    buttonHighlights[i].position.y - 10.0f);
        label.text = buttonHighlights[i].label;
        label.color = glm::vec4(1.0f, 1.0f, 1.0f, intensity);
        commands.push_back(label);
    }
}

void InputVisualizer::buildTrailCommands() {
    if (trailCount < 2) return;

    for (int i = 1; i < trailCount; i++) {
        int idx0 = (trailHead - trailCount + i - 1 + MAX_TRAIL_POINTS) % MAX_TRAIL_POINTS;
        int idx1 = (trailHead - trailCount + i + MAX_TRAIL_POINTS) % MAX_TRAIL_POINTS;

        float alpha0 = std::max(0.0f, 1.0f - trailPoints[idx0].age / trailMaxAge);
        float alpha1 = std::max(0.0f, 1.0f - trailPoints[idx1].age / trailMaxAge);

        VisualizerRenderCommand cmd;
        cmd.type = VisualizerRenderCommand::LINE;
        cmd.position = trailPoints[idx0].position;
        cmd.position2 = trailPoints[idx1].position;
        cmd.color = glm::vec4(1.0f, 0.5f, 0.0f, (alpha0 + alpha1) * 0.5f);
        cmd.thickness = 2.0f;
        commands.push_back(cmd);
    }
}

void InputVisualizer::buildLatencyCommands() {
    // Display latency info in top-right corner
    float startX = static_cast<float>(screenWidth) - 250.0f;
    float startY = 40.0f;
    float lineHeight = 22.0f;

    // Frame time
    {
        VisualizerRenderCommand cmd;
        cmd.type = VisualizerRenderCommand::TEXT;
        cmd.position = glm::vec2(startX, startY);
        char buf[64];
        snprintf(buf, sizeof(buf), "Frame: %.1f ms (%.0f FPS)",
                 latencyData.frameTimeMs,
                 (latencyData.frameTimeMs > 0.0f) ? (1000.0f / latencyData.frameTimeMs) : 0.0f);
        cmd.text = buf;
        cmd.color = glm::vec4(0.0f, 1.0f, 0.0f, 0.9f);
        commands.push_back(cmd);
    }

    // Average frame time
    {
        VisualizerRenderCommand cmd;
        cmd.type = VisualizerRenderCommand::TEXT;
        cmd.position = glm::vec2(startX, startY + lineHeight);
        char buf[64];
        snprintf(buf, sizeof(buf), "Avg: %.1f ms", latencyData.avgFrameTimeMs);
        cmd.text = buf;
        cmd.color = glm::vec4(0.0f, 1.0f, 0.5f, 0.9f);
        commands.push_back(cmd);
    }

    // Input latency
    {
        VisualizerRenderCommand cmd;
        cmd.type = VisualizerRenderCommand::TEXT;
        cmd.position = glm::vec2(startX, startY + lineHeight * 2.0f);
        char buf[64];
        snprintf(buf, sizeof(buf), "Input: %.1f ms", latencyData.inputToRenderMs);
        cmd.text = buf;
        cmd.color = glm::vec4(1.0f, 1.0f, 0.0f, 0.9f);
        commands.push_back(cmd);
    }
}
