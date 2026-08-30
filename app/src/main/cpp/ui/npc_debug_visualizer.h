#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

class TextRenderer;
class NpcManager;

/**
 * @brief NPC Debug Visualization System
 *
 * Displays debug information for NPCs in-game:
 * - HP bars above NPCs
 * - AI state labels
 * - NPC ID and name
 * - Distance from player
 * - Combat state indicators
 */
class NpcDebugVisualizer {
public:
    NpcDebugVisualizer();
    ~NpcDebugVisualizer();

    bool initialize(TextRenderer* textRenderer, NpcManager* npcManager);
    void cleanup();

    void toggle();
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

    void setPlayerPosition(const glm::vec3& pos) { playerPos = pos; }

    void update(float deltaTime);
    void render();

    // Configuration
    void setShowHPBars(bool show) { showHPBars = show; }
    void setShowAIState(bool show) { showAIState = show; }
    void setShowNames(bool show) { showNames = show; }
    void setShowDistance(bool show) { showDistance = show; }
    void setShowIDs(bool show) { showIDs = show; }
    void setMaxDistance(float dist) { maxDistance = dist; }

private:
    TextRenderer* textRenderer;
    NpcManager* npcManager;
    bool visible;
    bool initialized;

    glm::vec3 playerPos;
    float maxDistance;

    // Display options
    bool showHPBars;
    bool showAIState;
    bool showNames;
    bool showDistance;
    bool showIDs;

    int screenWidth;
    int screenHeight;

    struct NpcDebugInfo {
        uint32_t npcId;
        std::string name;
        glm::vec3 position;
        float currentHP;
        float maxHP;
        std::string aiState;
        float distance;
    };

    void renderHPBar(float x, float y, float width, float height, float currentHP, float maxHP);
    void renderNpcInfo(const NpcDebugInfo& info, float x, float y, float scale);
    std::string getAIStateName(int state) const;
};
