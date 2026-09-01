#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

/**
 * @brief Touch target size enhancer for accessibility
 * 
 * Ensures all touch targets meet minimum size requirements
 * for comfortable interaction on mobile devices.
 * 
 * Reference: iOS Human Interface Guidelines (44pt minimum)
 *            Material Design (48dp minimum)
 */
class TouchTargetEnhancer {
private:
    float m_minimumTouchSize = 44.0f;  // Minimum touch target size in pixels
    float m_currentScaleFactor = 1.0f;
    
    // Reference diagonal for 1920x1080 screen
    static constexpr float REFERENCE_DIAGONAL = 2202.91f;  // sqrt(1920^2 + 1080^2)

public:
    /**
     * @brief Update scale factor based on screen diagonal
     * @param screenDiagonal Current screen diagonal in pixels
     */
    void UpdateScaleFactor(float screenDiagonal) {
        m_currentScaleFactor = screenDiagonal / REFERENCE_DIAGONAL;
    }

    /**
     * @brief Ensure size meets minimum touch target requirements
     * @param originalSize Original element size
     * @return Size adjusted to meet minimum requirements
     */
    glm::vec2 EnsureMinimumTouchSize(const glm::vec2& originalSize) const {
        glm::vec2 enhancedSize = originalSize;
        
        // Calculate minimum size for current resolution
        float currentMinSize = m_minimumTouchSize * m_currentScaleFactor;
        
        // Ensure width and height meet minimum
        enhancedSize.x = std::max(enhancedSize.x, currentMinSize);
        enhancedSize.y = std::max(enhancedSize.y, currentMinSize);
        
        return enhancedSize;
    }

    /**
     * @brief Expand touch area for small elements
     * @param position Element position
     * @param size Element visual size
     * @return Adjusted position for expanded touch area
     */
    glm::vec2 ExpandTouchArea(const glm::vec2& position, const glm::vec2& size) const {
        float currentMinSize = m_minimumTouchSize * m_currentScaleFactor;
        float expansion = (currentMinSize - std::min(size.x, size.y)) * 0.5f;
        
        if (expansion > 0) {
            return {position.x - expansion, position.y - expansion};
        }
        return position;
    }

    /**
     * @brief Get expanded touch size
     * @param size Original visual size
     * @return Size for touch hit testing
     */
    glm::vec2 GetTouchSize(const glm::vec2& size) const {
        return EnsureMinimumTouchSize(size);
    }

    /**
     * @brief Get current scale factor
     */
    float GetCurrentScaleFactor() const { return m_currentScaleFactor; }

    /**
     * @brief Set minimum touch size
     * @param size Minimum size in pixels
     */
    void SetMinimumTouchSize(float size) { m_minimumTouchSize = size; }

    /**
     * @brief Get minimum touch size
     */
    float GetMinimumTouchSize() const { return m_minimumTouchSize; }
};
