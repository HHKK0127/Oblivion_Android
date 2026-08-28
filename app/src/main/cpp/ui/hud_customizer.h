#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <android/log.h>

#define HUD_CUSTOM_LOG_TAG "HUDCustomizer"
#define HCUST_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, HUD_CUSTOM_LOG_TAG, __VA_ARGS__)
#define HCUST_LOGI(...) __android_log_print(ANDROID_LOG_INFO, HUD_CUSTOM_LOG_TAG, __VA_ARGS__)
#define HCUST_LOGW(...) __android_log_print(ANDROID_LOG_WARN, HUD_CUSTOM_LOG_TAG, __VA_ARGS__)

// Layout preset types
enum class LayoutPreset {
    DEFAULT,       // Standard layout
    COMPACT,       // Smaller elements, more screen space
    ACCESSIBILITY  // Larger elements, higher contrast
};

// Customizable HUD element
struct CustomHUDElement {
    std::string elementId;     // Unique identifier (e.g., "health_bar", "quick_slots")
    std::string displayName;   // Human-readable name
    glm::vec2 position;        // Current position (pixels)
    glm::vec2 baseSize;        // Original size (pixels)
    glm::vec2 currentSize;     // Adjusted size (pixels)
    float opacity;             // 0.0 - 1.0
    float scale;               // Size multiplier (0.5 - 2.0)
    bool visible;
    bool locked;               // If true, cannot be dragged

    CustomHUDElement()
        : elementId(""), displayName(""),
          position(0.0f, 0.0f), baseSize(100.0f, 50.0f),
          currentSize(100.0f, 50.0f),
          opacity(1.0f), scale(1.0f), visible(true), locked(false) {}
};

// Saved layout data for serialization
struct SavedLayout {
    std::string layoutName;
    std::map<std::string, CustomHUDElement> elements;
    int screenWidth;
    int screenHeight;

    SavedLayout() : layoutName(""), screenWidth(0), screenHeight(0) {}
};

// Callback when layout changes
using LayoutChangeCallback = std::function<void(const std::string& elementId)>;

// HUD customizer system
// Provides drag-and-drop repositioning, button size adjustment,
// opacity control, layout presets, and save/load functionality
class HUDCustomizer {
public:
    HUDCustomizer();
    ~HUDCustomizer() = default;

    // Initialize with screen dimensions
    void initialize(int screenWidth, int screenHeight);

    // Register a customizable HUD element
    void registerElement(const CustomHUDElement& element);

    // Get all registered elements
    const std::map<std::string, CustomHUDElement>& getElements() const { return elements; }

    // Get a specific element (returns nullptr if not found)
    const CustomHUDElement* getElement(const std::string& elementId) const;
    CustomHUDElement* getElementMutable(const std::string& elementId);

    // === Drag-and-drop repositioning ===

    // Start dragging an element (returns elementId, empty if none hit)
    std::string beginDrag(float touchX, float touchY);

    // Update drag position
    void updateDrag(float touchX, float touchY);

    // End dragging
    void endDrag();

    // Check if currently dragging
    bool isDragging() const { return dragging; }
    const std::string& getDraggedElementId() const { return draggedElementId; }

    // === Element adjustment ===

    // Set element position
    void setElementPosition(const std::string& elementId, const glm::vec2& position);

    // Set element scale (affects size)
    void setElementScale(const std::string& elementId, float scale);

    // Set element opacity
    void setElementOpacity(const std::string& elementId, float opacity);

    // Set element visibility
    void setElementVisible(const std::string& elementId, bool visible);

    // Lock/unlock element (prevents dragging)
    void setElementLocked(const std::string& elementId, bool locked);

    // === Layout presets ===

    // Apply a layout preset
    void applyPreset(LayoutPreset preset);

    // Get current preset name
    std::string getPresetName(LayoutPreset preset) const;

    // === Save/Load ===

    // Save current layout with a name
    bool saveLayout(const std::string& layoutName);

    // Load a saved layout
    bool loadLayout(const std::string& layoutName);

    // Get list of saved layout names
    std::vector<std::string> getSavedLayoutNames() const;

    // Delete a saved layout
    bool deleteLayout(const std::string& layoutName);

    // Reset all elements to default positions
    void resetToDefaults();

    // === Callbacks ===

    void registerChangeCallback(LayoutChangeCallback callback) {
        changeCallbacks.push_back(callback);
    }

    // Screen size change
    void onScreenResize(int newWidth, int newHeight);

private:
    int screenWidth;
    int screenHeight;
    bool dragging;
    std::string draggedElementId;
    glm::vec2 dragOffset;       // Offset from element origin to touch point

    // All registered elements
    std::map<std::string, CustomHUDElement> elements;

    // Saved layouts
    std::map<std::string, SavedLayout> savedLayouts;

    // Callbacks
    std::vector<LayoutChangeCallback> changeCallbacks;

    // Notify callbacks
    void notifyChange(const std::string& elementId);

    // Clamp position to screen bounds
    glm::vec2 clampPosition(const glm::vec2& pos, const glm::vec2& size) const;

    // Set up default preset positions
    void setupDefaultPreset();

    // Set up compact preset positions
    void setupCompactPreset();

    // Set up accessibility preset positions
    void setupAccessibilityPreset();
};
