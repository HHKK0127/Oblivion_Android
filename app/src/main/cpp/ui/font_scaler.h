#pragma once

#include <algorithm>
#include <cmath>

/**
 * @brief Font size auto-scaling for responsive UI
 * 
 * Automatically adjusts font sizes based on screen height to ensure
 * readability across different device resolutions.
 */
class FontScaler {
private:
    float m_baseScreenHeight = 1080.0f;  // Reference resolution height
    float m_minScale = 0.7f;
    float m_maxScale = 1.5f;
    float m_currentScale = 1.0f;

public:
    /**
     * @brief Update scale factor based on current screen height
     * @param currentScreenHeight Current screen height in pixels
     */
    void UpdateScale(float currentScreenHeight) {
        m_currentScale = currentScreenHeight / m_baseScreenHeight;
        m_currentScale = std::clamp(m_currentScale, m_minScale, m_maxScale);
    }

    /**
     * @brief Get scaled font size
     * @param baseFontSize Base font size
     * @return Scaled font size
     */
    float GetScaledFontSize(float baseFontSize) const {
        return baseFontSize * m_currentScale;
    }

    /**
     * @brief Get scaled line height
     * @param baseLineHeight Base line height
     * @return Scaled line height
     */
    float GetScaledLineHeight(float baseLineHeight) const {
        return baseLineHeight * m_currentScale;
    }

    /**
     * @brief Get minimum readable font size
     * @return Minimum font size that maintains readability
     */
    float GetMinimumReadableSize() const {
        return std::max(12.0f, 16.0f * m_currentScale);
    }

    /**
     * @brief Get current scale factor
     */
    float GetCurrentScale() const { return m_currentScale; }

    /**
     * @brief Set base screen height for scaling reference
     * @param height Reference screen height
     */
    void SetBaseScreenHeight(float height) { m_baseScreenHeight = height; }

    /**
     * @brief Set scale limits
     * @param minScale Minimum scale factor
     * @param maxScale Maximum scale factor
     */
    void SetScaleLimits(float minScale, float maxScale) {
        m_minScale = minScale;
        m_maxScale = maxScale;
    }
};
