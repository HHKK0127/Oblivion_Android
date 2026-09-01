#pragma once

#include <glm/glm.hpp>
#include <algorithm>

/**
 * @brief Safe area manager for notch/punch-hole camera support
 * 
 * Handles safe area insets to ensure UI elements are not obscured
 * by device-specific hardware features like notches or punch-hole cameras.
 */
class SafeAreaManager {
public:
    struct SafeAreaInsets {
        float top = 0.0f;
        float bottom = 0.0f;
        float left = 0.0f;
        float right = 0.0f;
    };

    enum class Anchor {
        TOP_LEFT,
        TOP_CENTER,
        TOP_RIGHT,
        CENTER_LEFT,
        CENTER,
        CENTER_RIGHT,
        BOTTOM_LEFT,
        BOTTOM_CENTER,
        BOTTOM_RIGHT
    };

private:
    SafeAreaInsets m_insets;
    glm::vec2 m_screenSize = {0.0f, 0.0f};

public:
    /**
     * @brief Update safe area insets
     * @param screenSize Current screen dimensions
     * @param insets Safe area insets from the system
     */
    void UpdateSafeArea(const glm::vec2& screenSize, const SafeAreaInsets& insets) {
        m_screenSize = screenSize;
        m_insets = insets;
    }

    /**
     * @brief Get safe position for a given anchor and offset
     * @param anchor UI anchor point
     * @param baseOffset Base offset from anchor
     * @return Adjusted position respecting safe area
     */
    glm::vec2 GetSafePosition(Anchor anchor, const glm::vec2& baseOffset) {
        glm::vec2 safeOffset = baseOffset;

        // Adjust Y based on anchor vertical position
        switch (anchor) {
            case Anchor::TOP_LEFT:
            case Anchor::TOP_CENTER:
            case Anchor::TOP_RIGHT:
                safeOffset.y += m_insets.top;
                break;
            case Anchor::BOTTOM_LEFT:
            case Anchor::BOTTOM_CENTER:
            case Anchor::BOTTOM_RIGHT:
                safeOffset.y -= m_insets.bottom;
                break;
            default:
                break;
        }

        // Adjust X based on anchor horizontal position
        if (anchor == Anchor::TOP_LEFT || 
            anchor == Anchor::CENTER_LEFT || 
            anchor == Anchor::BOTTOM_LEFT) {
            safeOffset.x += m_insets.left;
        } else if (anchor == Anchor::TOP_RIGHT || 
                   anchor == Anchor::CENTER_RIGHT || 
                   anchor == Anchor::BOTTOM_RIGHT) {
            safeOffset.x -= m_insets.right;
        }

        return safeOffset;
    }

    /**
     * @brief Get safe content size (screen size minus insets)
     * @param baseSize Base content size
     * @return Size that fits within safe area
     */
    glm::vec2 GetSafeContentSize(const glm::vec2& baseSize) {
        return {
            baseSize.x - (m_insets.left + m_insets.right),
            baseSize.y - (m_insets.top + m_insets.bottom)
        };
    }

    /**
     * @brief Get current safe area insets
     */
    const SafeAreaInsets& GetInsets() const { return m_insets; }

    /**
     * @brief Get current screen size
     */
    const glm::vec2& GetScreenSize() const { return m_screenSize; }
};
