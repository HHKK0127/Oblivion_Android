#pragma once

#include <glm/glm.hpp>
#include "safe_area_manager.h"

/**
 * @brief Screen orientation manager
 * 
 * Handles screen orientation changes and provides
 * orientation-specific layout parameters.
 */
class OrientationManager {
public:
    enum class Orientation {
        PORTRAIT,
        LANDSCAPE,
        UNKNOWN
    };

    enum class ComponentType {
        JOYSTICK,
        ACTION_BUTTONS,
        DEBUG_HUD,
        QUICK_SLOTS,
        MENU,
        DIALOG
    };

    struct LayoutParams {
        float spacingMultiplier = 1.0f;
        float scaleMultiplier = 1.0f;
        SafeAreaManager::Anchor preferredAnchor = SafeAreaManager::Anchor::CENTER;
    };

private:
    Orientation m_currentOrientation = Orientation::LANDSCAPE;
    glm::vec2 m_screenSize = {0.0f, 0.0f};

public:
    /**
     * @brief Update orientation based on screen size
     * @param screenSize Current screen dimensions
     */
    void UpdateOrientation(const glm::vec2& screenSize) {
        m_screenSize = screenSize;
        m_currentOrientation = (screenSize.x > screenSize.y) ? 
            Orientation::LANDSCAPE : Orientation::PORTRAIT;
    }

    /**
     * @brief Get current orientation
     */
    Orientation GetCurrentOrientation() const { return m_currentOrientation; }

    /**
     * @brief Check if currently in landscape mode
     */
    bool IsLandscape() const { return m_currentOrientation == Orientation::LANDSCAPE; }

    /**
     * @brief Check if currently in portrait mode
     */
    bool IsPortrait() const { return m_currentOrientation == Orientation::PORTRAIT; }

    /**
     * @brief Get optimal anchor position for a component type
     * @param type Component type
     * @return Recommended anchor position
     */
    SafeAreaManager::Anchor GetOptimalAnchorForComponent(ComponentType type) const {
        switch (type) {
            case ComponentType::JOYSTICK:
                return SafeAreaManager::Anchor::BOTTOM_LEFT;
                
            case ComponentType::ACTION_BUTTONS:
                return SafeAreaManager::Anchor::BOTTOM_RIGHT;
                
            case ComponentType::DEBUG_HUD:
                return SafeAreaManager::Anchor::TOP_LEFT;
                
            case ComponentType::QUICK_SLOTS:
                return SafeAreaManager::Anchor::TOP_RIGHT;
                
            case ComponentType::MENU:
            case ComponentType::DIALOG:
                return SafeAreaManager::Anchor::CENTER;
                
            default:
                return SafeAreaManager::Anchor::CENTER;
        }
    }

    /**
     * @brief Get layout parameters for orientation
     * @param type Component type
     * @return Layout parameters adjusted for current orientation
     */
    LayoutParams GetLayoutParams(ComponentType type) const {
        LayoutParams params;
        params.preferredAnchor = GetOptimalAnchorForComponent(type);
        
        if (m_currentOrientation == Orientation::PORTRAIT) {
            // Portrait mode adjustments
            params.spacingMultiplier = 1.5f;  // Increase spacing
            params.scaleMultiplier = 1.2f;    // Increase element size
        } else {
            // Landscape mode adjustments
            params.spacingMultiplier = 1.0f;
            params.scaleMultiplier = 1.0f;
        }
        
        return params;
    }

    /**
     * @brief Get current screen size
     */
    const glm::vec2& GetScreenSize() const { return m_screenSize; }
};
