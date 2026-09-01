#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <android/log.h>
#include "ui_component.h"

#define UI_SYSTEM_LOG_TAG "UISystem"
#define SYS_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, UI_SYSTEM_LOG_TAG, __VA_ARGS__)
#define SYS_LOGI(...) __android_log_print(ANDROID_LOG_INFO, UI_SYSTEM_LOG_TAG, __VA_ARGS__)
#define SYS_LOGW(...) __android_log_print(ANDROID_LOG_WARN, UI_SYSTEM_LOG_TAG, __VA_ARGS__)
#define SYS_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, UI_SYSTEM_LOG_TAG, __VA_ARGS__)

// Forward declarations
class TextRenderer;

/**
 * @brief UI system management class
 *
 * Phase 9: Management class that is the parent of all UI components.
 * - Register/delete UI components
 * - Touch event dispatch (considering z-order)
 * - Per-frame update/render calls
 * - Screen resolution change notification
 *
 * Called from existing Renderer to manage all UI integrally.
 */
class UISystem {
public:
    UISystem();
    ~UISystem();

    /**
     * @brief Initialize UI system
     * @param textRenderer Existing text renderer (for label drawing)
     * @return True on successful initialization
     */
    bool initialize(TextRenderer* textRenderer);

    /**
     * @brief Cleanup
     */
    void cleanup();

    // === Component Management ===

    /**
     * @brief Register component
     * @param component Component to register
     * @param layer Drawing layer (larger = more front)
     */
    void registerComponent(std::shared_ptr<UIComponent> component, int layer = 0);

    /**
     * @brief Unregister component
     */
    void unregisterComponent(std::shared_ptr<UIComponent> component);

    /**
     * @brief Search component by name
     */
    std::shared_ptr<UIComponent> findComponent(const std::string& name) const;

    /**
     * @brief Clear all components
     */
    void clearComponents();

    // === Layer Management ===

    /**
     * @brief Change component layer
     */
    void setLayer(std::shared_ptr<UIComponent> component, int newLayer);

    // === Update/Render ===

    /**
     * @brief Update all UI components
     * @param deltaTime Elapsed time from previous frame (seconds)
     */
    void update(float deltaTime);

    /**
     * @brief Render all UI components (by layer order)
     */
    void render();

    // === Event Dispatch ===

    /**
     * @brief Dispatch touch down event
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param pointerId Pointer ID
     * @return True if any component consumed the event
     */
    bool onTouchDown(float x, float y, int pointerId = 0);

    /**
     * @brief Dispatch touch up event
     */
    bool onTouchUp(float x, float y, int pointerId = 0);

    /**
     * @brief Dispatch touch move event
     */
    bool onTouchMove(float x, float y, float dx, float dy, int pointerId = 0);

    // === Screen Resolution ===

    /**
     * @brief Call when screen resolution changes
     */
    void setScreenSize(int width, int height);
    glm::vec2 getScreenSize() const { return glm::vec2(static_cast<float>(screenWidth), static_cast<float>(screenHeight)); }

    // === Text renderer ===

    TextRenderer* getTextRenderer() const { return textRenderer; }

    // === Focus management ===

    /**
     * @brief Set component that has focus
     */
    void setFocusedComponent(std::shared_ptr<UIComponent> component);
    std::shared_ptr<UIComponent> getFocusedComponent() const { return focusedComponent.lock(); }

    // === Convenience Methods ===

    /**
     * @brief Set visibility of all components at once
     */
    void setAllVisible(bool visible);

    /**
     * @brief Make only components in specific layer visible
     */
    void showOnlyLayer(int layer);

    /**
     * @brief Get number of registered components
     */
    size_t getComponentCount() const;

private:
    // Component management by layer (key: layer, value: components)
    std::map<int, std::vector<std::shared_ptr<UIComponent>>> layers;

    // For fast lookup by name
    std::map<std::string, std::weak_ptr<UIComponent>> nameMap;

    // Text renderer (uses existing assets)
    TextRenderer* textRenderer;

    // Focus management
    std::weak_ptr<UIComponent> focusedComponent;

    // Screen size
    int screenWidth;
    int screenHeight;

    bool initialized;

    // Event delivery helper (front layer priority)
    bool dispatchEvent(const UIEvent& event);

    // Sort layers and draw
    void renderLayers();

    // Remove component from layer
    void removeFromLayer(std::shared_ptr<UIComponent> component, int layer);
};
