#pragma once

#include <glm/glm.hpp>
#include <cmath>
#include "safe_area_manager.h"
#include "font_scaler.h"
#include "touch_target_enhancer.h"
#include "orientation_manager.h"

/**
 * @brief Integrated responsive UI manager
 * 
 * Combines safe area management, font scaling, touch target enhancement,
 * and orientation management into a unified system.
 */
class ResponsiveUIManager {
public:
    struct UIParameters {
        glm::vec2 position;
        float fontSizeScale;
        float touchTargetScale;
        OrientationManager::Orientation orientation;
        float spacingMultiplier;
        float scaleMultiplier;
    };

private:
    SafeAreaManager m_safeAreaManager;
    FontScaler m_fontScaler;
    TouchTargetEnhancer m_touchTargetEnhancer;
    OrientationManager m_orientationManager;
    
    bool m_initialized = false;

public:
    /**
     * @brief Initialize the responsive UI manager
     */
    void Initialize() {
        // Initialize touch target enhancer with reference diagonal
        float referenceDiagonal = std::sqrt(1920.0f * 1920.0f + 1080.0f * 1080.0f);
        m_touchTargetEnhancer.UpdateScaleFactor(referenceDiagonal);
        m_initialized = true;
    }

    /**
     * @brief Update all managers for screen change
     * @param screenSize New screen dimensions
     * @param safeInsets Safe area insets
     */
    void UpdateForScreenChange(const glm::vec2& screenSize, 
                               const SafeAreaManager::SafeAreaInsets& safeInsets) {
        // Update safe area
        m_safeAreaManager.UpdateSafeArea(screenSize, safeInsets);
        
        // Update font scaling
        m_fontScaler.UpdateScale(screenSize.y);
        
        // Update touch target scaling
        float screenDiagonal = std::sqrt(screenSize.x * screenSize.x + screenSize.y * screenSize.y);
        m_touchTargetEnhancer.UpdateScaleFactor(screenDiagonal);
        
        // Update orientation
        m_orientationManager.UpdateOrientation(screenSize);
    }

    /**
     * @brief Get UI parameters for a component
     * @param baseAnchor Base anchor position
     * @param baseOffset Base offset from anchor
     * @param type Component type for orientation-specific adjustments
     * @return Complete UI parameters
     */
    UIParameters GetUIParameters(SafeAreaManager::Anchor baseAnchor, 
                                 const glm::vec2& baseOffset,
                                 OrientationManager::ComponentType type) {
        UIParameters params;
        
        // Get safe position
        params.position = m_safeAreaManager.GetSafePosition(baseAnchor, baseOffset);
        
        // Get font scale
        params.fontSizeScale = m_fontScaler.GetCurrentScale();
        
        // Get touch target scale
        params.touchTargetScale = m_touchTargetEnhancer.GetCurrentScaleFactor();
        
        // Get orientation info
        params.orientation = m_orientationManager.GetCurrentOrientation();
        
        // Get layout params for orientation
        auto layoutParams = m_orientationManager.GetLayoutParams(type);
        params.spacingMultiplier = layoutParams.spacingMultiplier;
        params.scaleMultiplier = layoutParams.scaleMultiplier;
        
        return params;
    }

    /**
     * @brief Get scaled font size
     * @param baseFontSize Base font size
     * @return Scaled font size
     */
    float GetScaledFontSize(float baseFontSize) const {
        return m_fontScaler.GetScaledFontSize(baseFontSize);
    }

    /**
     * @brief Get minimum readable font size
     */
    float GetMinimumReadableSize() const {
        return m_fontScaler.GetMinimumReadableSize();
    }

    /**
     * @brief Ensure touch size meets minimum requirements
     * @param size Original size
     * @return Size meeting minimum requirements
     */
    glm::vec2 EnsureMinimumTouchSize(const glm::vec2& size) const {
        return m_touchTargetEnhancer.EnsureMinimumTouchSize(size);
    }

    /**
     * @brief Get safe area insets
     */
    const SafeAreaManager::SafeAreaInsets& GetSafeAreaInsets() const {
        return m_safeAreaManager.GetInsets();
    }

    /**
     * @brief Get current orientation
     */
    OrientationManager::Orientation GetCurrentOrientation() const {
        return m_orientationManager.GetCurrentOrientation();
    }

    /**
     * @brief Check if in landscape mode
     */
    bool IsLandscape() const {
        return m_orientationManager.IsLandscape();
    }

    /**
     * @brief Check if in portrait mode
     */
    bool IsPortrait() const {
        return m_orientationManager.IsPortrait();
    }

    /**
     * @brief Get safe area manager
     */
    SafeAreaManager& GetSafeAreaManager() { return m_safeAreaManager; }

    /**
     * @brief Get font scaler
     */
    FontScaler& GetFontScaler() { return m_fontScaler; }

    /**
     * @brief Get touch target enhancer
     */
    TouchTargetEnhancer& GetTouchTargetEnhancer() { return m_touchTargetEnhancer; }

    /**
     * @brief Get orientation manager
     */
    OrientationManager& GetOrientationManager() { return m_orientationManager; }

    /**
     * @brief Check if initialized
     */
    bool IsInitialized() const { return m_initialized; }
};
