#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include <android/log.h>

#define UI_LOG_TAG "UIComponent"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, UI_LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, UI_LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, UI_LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, UI_LOG_TAG, __VA_ARGS__)

// Forward declarations
class TextRenderer;

/**
 * @brief UI event type
 */
enum class UIEventType {
    TOUCH_DOWN,
    TOUCH_UP,
    TOUCH_MOVE,
    HOVER_ENTER,
    HOVER_LEAVE,
    FOCUS_GAIN,
    FOCUS_LOST
};

/**
 * @brief UI event structure
 */
struct UIEvent {
    UIEventType type;
    float x;
    float y;
    float dx;
    float dy;
    int pointerId;
    bool consumed;

    UIEvent(UIEventType t, float px, float py, int pid = 0)
        : type(t), x(px), y(py), dx(0.0f), dy(0.0f), pointerId(pid), consumed(false) {}
};

/**
 * @brief UI anchor (placement reference point)
 */
enum class UIAnchor {
    TOP_LEFT,      // (0,0) from top-left
    TOP_CENTER,
    TOP_RIGHT,
    CENTER_LEFT,
    CENTER,
    CENTER_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_CENTER,
    BOTTOM_RIGHT
};

/**
 * @brief Texture scaling mode
 */
enum class TextureScaleMode {
    STRETCH,              // Default: stretch to fit quad
    PRESERVE_ASPECT_FIT,  // Preserve aspect ratio, fit all (letterbox/pillarbox)
    PRESERVE_ASPECT_CROP  // Preserve aspect ratio, fill quad (allow overflow)
};

/**
 * @brief UI component base class
 *
 * Phase 9: Foundation of the graphical UI framework.
 * All UI elements (panels, buttons, text, HUD, etc.)
 * inherit from this class.
 */
class UIComponent : public std::enable_shared_from_this<UIComponent> {
public:
    UIComponent(const std::string& name = "UIComponent");
    virtual ~UIComponent();

    // === Lifecycle ===

    /**
     * @brief Initialize component
     * @return True on successful initialization
     */
    virtual bool initialize();

    /**
     * @brief Update every frame
     * @param deltaTime Elapsed time from previous frame (seconds)
     */
    virtual void update(float deltaTime);

    /**
     * @brief Render component
     */
    virtual void render();

    /**
     * @brief Release resources
     */
    virtual void cleanup();

    // === Event Processing ===

    /**
     * @brief Handle UI event
     * @param event UI event
     * @return True if event was consumed
     */
    virtual bool onEvent(const UIEvent& event);

    /**
     * @brief Touch down event (simple API)
     */
    virtual bool onTouchDown(float x, float y, int pointerId = 0);

    /**
     * @brief Touch up event (simple API)
     */
    virtual bool onTouchUp(float x, float y, int pointerId = 0);

    /**
     * @brief Touch move event (simple API)
     */
    virtual bool onTouchMove(float x, float y, float dx, float dy, int pointerId = 0);

    // === Show/Hide ===

    void setVisible(bool visible) { this->visible = visible; }
    bool isVisible() const { return visible; }
    void show() { setVisible(true); }
    void hide() { setVisible(false); }
    void toggle() { setVisible(!visible); }

    // === Position/Size ===

    void setPosition(float x, float y);
    void setSize(float width, float height);
    void setAnchor(UIAnchor anchor);

    glm::vec2 getPosition() const { return position; }
    glm::vec2 getSize() const { return size; }
    glm::vec2 getAbsolutePosition() const;  // Absolute position after anchor adjustment

    /**
     * @brief Is point inside component
     */
    bool contains(float x, float y) const;

    /**
     * @brief Screen resolution change callback
     */
    virtual void onScreenResize(int width, int height);

    /**
     * @brief Set screen resolution
     */
    void setScreenSize(int width, int height) { onScreenResize(width, height); }

    // === Parent-child relationship ===

    void setParent(std::shared_ptr<UIComponent> parent);
    std::shared_ptr<UIComponent> getParent() const { return parent.lock(); }

    void addChild(std::shared_ptr<UIComponent> child);
    void removeChild(std::shared_ptr<UIComponent> child);
    const std::vector<std::shared_ptr<UIComponent>>& getChildren() const { return children; }

    // === Identification ===

    const std::string& getName() const { return name; }
    void setName(const std::string& n) { name = n; }

    uint32_t getId() const { return id; }

    // === Drawing settings ===

    void setBackgroundColor(const glm::vec4& color) { backgroundColor = color; }
    void setBorderColor(const glm::vec4& color) { borderColor = color; }
    void setBorderWidth(float width) { borderWidth = width; }
    void setTexture(GLuint texId) { textureId = texId; }
    GLuint getTexture() const { return textureId; }

    void setTextureScaleMode(TextureScaleMode mode) { textureScaleMode = mode; }
    TextureScaleMode getTextureScaleMode() const { return textureScaleMode; }

    bool isEnabled() const { return enabled; }
    bool isInitialized() const { return initialized; }

protected:
    // Common drawing helper
    void renderBackground() const;
    void renderBorder() const;
    void renderTexture() const;

    // Draw child elements (with visibility check)
    void renderChildren();
    void updateChildren(float deltaTime);
    bool dispatchEventToChildren(const UIEvent& event);

    // Transform
    glm::vec2 position;  // Local coordinates (pixels)
    glm::vec2 size;      // Width and height (pixels)

    // Screen size (cached)
    int screenWidth;
    int screenHeight;

    // Drawing settings
    glm::vec4 backgroundColor;
    glm::vec4 borderColor;
    float borderWidth;
    GLuint textureId;
    TextureScaleMode textureScaleMode = TextureScaleMode::STRETCH;

private:
    std::string name;
    uint32_t id;

    UIAnchor anchor;

    // State
    bool visible;
    bool initialized;
    bool enabled;

    // Parent-child relationship
    std::weak_ptr<UIComponent> parent;
    std::vector<std::shared_ptr<UIComponent>> children;

    // For unique ID generation
    static uint32_t nextId;
};
