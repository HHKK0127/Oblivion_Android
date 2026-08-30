#include "npc_debug_visualizer.h"
#include "text_renderer.h"
#include "../game/npc_manager.h"
#include "../game/npc.h"
#include <GLES3/gl3.h>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG_NPC_DBG "NpcDebugVisualizer"
#define LOGD_NPC_DBG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_NPC_DBG, __VA_ARGS__)

NpcDebugVisualizer::NpcDebugVisualizer()
    : textRenderer(nullptr), npcManager(nullptr), visible(false), initialized(false),
      playerPos(0.0f, 0.0f, 0.0f), maxDistance(50.0f),
      showHPBars(true), showAIState(true), showNames(true), showDistance(true), showIDs(true),
      screenWidth(1080), screenHeight(1920) {
}

NpcDebugVisualizer::~NpcDebugVisualizer() {
    cleanup();
}

bool NpcDebugVisualizer::initialize(TextRenderer* tr, NpcManager* nm) {
    if (initialized) return true;
    textRenderer = tr;
    npcManager = nm;
    initialized = true;
    LOGD_NPC_DBG("NpcDebugVisualizer initialized");
    return true;
}

void NpcDebugVisualizer::cleanup() {
    initialized = false;
}

void NpcDebugVisualizer::toggle() {
    visible = !visible;
}

void NpcDebugVisualizer::update(float deltaTime) {
    // Update screen size if needed
}

void NpcDebugVisualizer::render() {
    if (!visible || !textRenderer || !npcManager) return;

    // DPI-aware scaling
    float minDim = static_cast<float>(std::min(screenWidth, screenHeight));
    float scale = minDim / 1080.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    // Get all NPCs from manager
    auto allNpcs = npcManager->getAllNPCs();

    float fontSize = 0.5f * scale;
    float lineH = fontSize * 25.0f;

    // Header
    glm::vec3 headerColor(0.3f, 0.9f, 0.4f);
    textRenderer->renderText("NPC DEBUG", 10.0f * scale, lineH * 1.5f, headerColor, 0.7f * scale);

    int count = 0;
    float y = lineH * 3.0f;

    for (const auto& npc : allNpcs) {
        if (!npc) continue;

        // Calculate distance from player
        glm::vec3 diff = npc->position - playerPos;
        float distance = glm::length(diff);

        // Skip NPCs too far away
        if (distance > maxDistance) continue;

        // Build debug info
        NpcDebugInfo info;
        info.npcId = npc->npcId;
        info.name = npc->name;
        info.position = npc->position;
        info.currentHP = npc->status.currentHealth;
        info.maxHP = npc->status.maxHealth;
        info.distance = distance;
        info.aiState = getAIStateName(static_cast<int>(npc->aiState));

        // Render NPC info
        renderNpcInfo(info, 10.0f * scale, y, scale);
        y += lineH * 3.5f; // Space for HP bar + text

        count++;
        if (y > screenHeight - lineH * 2.0f) break; // Stop if off screen
    }

    // Summary
    glm::vec3 summaryColor(0.7f, 0.7f, 0.8f);
    char buf[128];
    snprintf(buf, sizeof(buf), "NPCs in range: %d / %zu", count, allNpcs.size());
    textRenderer->renderText(buf, 10.0f * scale, screenHeight - lineH * 2.0f, summaryColor, fontSize);
}

void NpcDebugVisualizer::renderHPBar(float x, float y, float width, float height, float currentHP, float maxHP) {
    if (maxHP <= 0.0f) return;

    // Background (dark gray)
    glm::vec4 bgColor(0.2f, 0.2f, 0.2f, 0.8f);
    // Draw background quad using colored quad helper
    // For now, we'll use text-based HP display

    float hpPercent = currentHP / maxHP;
    glm::vec3 hpColor;
    if (hpPercent > 0.6f) {
        hpColor = glm::vec3(0.2f, 0.8f, 0.2f); // Green
    } else if (hpPercent > 0.3f) {
        hpColor = glm::vec3(0.8f, 0.8f, 0.2f); // Yellow
    } else {
        hpColor = glm::vec3(0.8f, 0.2f, 0.2f); // Red
    }

    // Render HP text
    char hpText[32];
    snprintf(hpText, sizeof(hpText), "%.0f/%.0f", currentHP, maxHP);
    textRenderer->renderText(hpText, x, y, hpColor, 0.45f);
}

void NpcDebugVisualizer::renderNpcInfo(const NpcDebugInfo& info, float x, float y, float scale) {
    float fontSize = 0.45f * scale;
    float lineH = fontSize * 22.0f;

    // NPC ID and Name
    glm::vec3 nameColor(0.9f, 0.9f, 0.9f);
    char nameBuf[128];
    if (showIDs) {
        snprintf(nameBuf, sizeof(nameBuf), "[%u] %s", info.npcId, info.name.c_str());
    } else {
        snprintf(nameBuf, sizeof(nameBuf), "%s", info.name.c_str());
    }
    textRenderer->renderText(nameBuf, x, y, nameColor, fontSize);
    y += lineH;

    // HP Bar
    if (showHPBars) {
        renderHPBar(x, y, 100.0f * scale, 8.0f * scale, info.currentHP, info.maxHP);
        y += lineH;
    }

    // AI State
    if (showAIState) {
        glm::vec3 stateColor(0.7f, 0.8f, 0.9f);
        char stateBuf[64];
        snprintf(stateBuf, sizeof(stateBuf), "AI: %s", info.aiState.c_str());
        textRenderer->renderText(stateBuf, x, y, stateColor, fontSize * 0.9f);
        y += lineH * 0.9f;
    }

    // Distance
    if (showDistance) {
        glm::vec3 distColor(0.6f, 0.6f, 0.7f);
        char distBuf[32];
        snprintf(distBuf, sizeof(distBuf), "%.1fm", info.distance);
        textRenderer->renderText(distBuf, x, y, distColor, fontSize * 0.85f);
    }
}

std::string NpcDebugVisualizer::getAIStateName(int state) const {
    switch (state) {
        case 0: return "Idle";
        case 1: return "Walking";
        case 2: return "Running";
        case 3: return "Combat";
        case 4: return "Fleeing";
        case 5: return "Talking";
        case 6: return "Sleeping";
        case 7: return "Dead";
        default: return "Unknown(" + std::to_string(state) + ")";
    }
}
