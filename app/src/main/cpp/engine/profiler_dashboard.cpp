#include "profiler_dashboard.h"
#include "memory_pool.h"
#include "render_optimizer.h"
#include "async_task_manager.h"
#include "cache_manager.h"
#include "../ui/text_renderer.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

// ============================================================================
// ProfilerDashboard Implementation
// ============================================================================

ProfilerDashboard::ProfilerDashboard() = default;

ProfilerDashboard::~ProfilerDashboard() {
    cleanup();
}

bool ProfilerDashboard::initialize(TextRenderer* textRenderer) {
    if (initialized_) return true;

    textRenderer_ = textRenderer;

    // Initialize graph ranges
    fpsGraph_.minValue = 0.0f;
    fpsGraph_.maxValue = 65.0f;

    frameTimeGraph_.minValue = 0.0f;
    frameTimeGraph_.maxValue = 33.0f;  // 30fps = 33ms

    memoryGraph_.minValue = 0.0f;
    memoryGraph_.maxValue = 512.0f;  // 512MB

    drawCallGraph_.minValue = 0.0f;
    drawCallGraph_.maxValue = 500.0f;

    initialized_ = true;
    LOGI_PD("ProfilerDashboard initialized");
    return true;
}

void ProfilerDashboard::cleanup() {
    if (!initialized_) return;
    initialized_ = false;
    LOGI_PD("ProfilerDashboard cleaned up");
}

// ============================================================================
// Data Input
// ============================================================================

void ProfilerDashboard::recordFrame(float frameTimeMs) {
    frameTimeGraph_.push(frameTimeMs);

    // Calculate FPS from frame time
    if (frameTimeMs > 0.0f) {
        float fps = 1000.0f / frameTimeMs;
        fpsGraph_.push(fps);
    }

    frameCount_++;
    fpsAccumulator_ += frameTimeMs;
}

void ProfilerDashboard::recordSystemTiming(const SystemTiming& timing) {
    lastTiming_ = timing;
    updateAverages();
}

void ProfilerDashboard::recordDrawCalls(uint32_t count) {
    currentDrawCalls_ = count;
    drawCallGraph_.push(static_cast<float>(count));
    if (count > peakDrawCalls_) {
        peakDrawCalls_ = count;
    }
}

void ProfilerDashboard::recordMemoryUsage(size_t usedBytes, size_t totalBytes) {
    currentMemoryUsed_ = usedBytes;
    currentMemoryTotal_ = totalBytes;
    float usedMB = static_cast<float>(usedBytes) / (1024.0f * 1024.0f);
    memoryGraph_.push(usedMB);
    if (usedBytes > peakMemoryUsed_) {
        peakMemoryUsed_ = usedBytes;
    }
}

// ============================================================================
// Update
// ============================================================================

void ProfilerDashboard::update(float deltaTime) {
    if (!initialized_ || !visible_) return;

    timeSinceUpdate_ += deltaTime;

    if (timeSinceUpdate_ >= config_.updateIntervalSec) {
        // Collect stats from connected systems
        collectPoolStats();
        collectCacheStats();
        collectTaskStats();

        timeSinceUpdate_ = 0.0f;
    }
}

void ProfilerDashboard::updateAverages() {
    timingSampleCount_++;
    float n = static_cast<float>(timingSampleCount_);
    float inv = 1.0f / n;

    avgTiming_.worldUpdateMs += (lastTiming_.worldUpdateMs - avgTiming_.worldUpdateMs) * inv;
    avgTiming_.renderMs += (lastTiming_.renderMs - avgTiming_.renderMs) * inv;
    avgTiming_.physicsMs += (lastTiming_.physicsMs - avgTiming_.physicsMs) * inv;
    avgTiming_.aiUpdateMs += (lastTiming_.aiUpdateMs - avgTiming_.aiUpdateMs) * inv;
    avgTiming_.uiUpdateMs += (lastTiming_.uiUpdateMs - avgTiming_.uiUpdateMs) * inv;
    avgTiming_.audioMs += (lastTiming_.audioMs - avgTiming_.audioMs) * inv;
    avgTiming_.totalMs += (lastTiming_.totalMs - avgTiming_.totalMs) * inv;
}

void ProfilerDashboard::collectPoolStats() {
    // Stats collected via memoryPool_ pointer if available
    // Rendered in the dashboard render pass
}

void ProfilerDashboard::collectCacheStats() {
    // Stats collected via cacheMgr_ pointer if available
}

void ProfilerDashboard::collectTaskStats() {
    // Stats collected via asyncTaskMgr_ pointer if available
}

// ============================================================================
// Render
// ============================================================================

void ProfilerDashboard::render() {
    if (!initialized_ || !visible_ || !textRenderer_) return;

    int x = 8;
    int y = 8;
    int lineHeight = 14;
    int sectionGap = 8;

    // Title
    renderText(x, y, "=== Performance Dashboard ===", 1.0f);
    y += lineHeight + sectionGap;

    // FPS Graph
    if (config_.showFPSGraph) {
        renderGraph(fpsGraph_, x, y, config_.graphWidth, config_.graphHeight,
                    "FPS", "");
        y += config_.graphHeight + 4;

        std::ostringstream fpsText;
        fpsText << "FPS: " << std::fixed << std::setprecision(1)
                << fpsGraph_.currentValue
                << " (avg: " << fpsGraph_.getAverage()
                << " max: " << fpsGraph_.getMax() << ")";
        renderText(x, y, fpsText.str());
        y += lineHeight + sectionGap;
    }

    // Frame Time
    std::ostringstream ftText;
    ftText << "Frame: " << std::fixed << std::setprecision(2)
           << frameTimeGraph_.currentValue << "ms";
    renderText(x, y, ftText.str());
    y += lineHeight;

    // Memory Graph
    if (config_.showMemoryGraph) {
        renderGraph(memoryGraph_, x, y, config_.graphWidth, config_.graphHeight,
                    "Memory", "MB");
        y += config_.graphHeight + 4;

        std::ostringstream memText;
        float usedMB = static_cast<float>(currentMemoryUsed_) / (1024.0f * 1024.0f);
        float totalMB = static_cast<float>(currentMemoryTotal_) / (1024.0f * 1024.0f);
        float peakMB = static_cast<float>(peakMemoryUsed_) / (1024.0f * 1024.0f);
        memText << "Memory: " << std::fixed << std::setprecision(1)
                << usedMB << "/" << totalMB << "MB"
                << " (peak: " << peakMB << "MB)";
        renderText(x, y, memText.str());
        y += lineHeight + sectionGap;
    }

    // Draw Calls
    if (config_.showDrawCalls) {
        std::ostringstream dcText;
        dcText << "Draw Calls: " << currentDrawCalls_
               << " (peak: " << peakDrawCalls_ << ")";
        renderText(x, y, dcText.str());
        y += lineHeight + sectionGap;
    }

    // System Timings
    if (config_.showSystemTimings) {
        renderText(x, y, "--- System Timings ---");
        y += lineHeight;

        auto renderTimingLine = [&](const std::string& name, float ms) {
            std::ostringstream ss;
            ss << "  " << name << ": " << std::fixed << std::setprecision(2) << "ms";
            renderText(x, y, ss.str());
            y += lineHeight;
        };

        renderTimingLine("World", lastTiming_.worldUpdateMs);
        renderTimingLine("Render", lastTiming_.renderMs);
        renderTimingLine("Physics", lastTiming_.physicsMs);
        renderTimingLine("AI", lastTiming_.aiUpdateMs);
        renderTimingLine("UI", lastTiming_.uiUpdateMs);
        renderTimingLine("Audio", lastTiming_.audioMs);
        renderTimingLine("Total", lastTiming_.totalMs);
        y += sectionGap;
    }

    // Pool Stats
    if (config_.showPoolStats && memoryPool_) {
        renderText(x, y, "--- Memory Pools ---");
        y += lineHeight;

        auto stats = memoryPool_->getStats();

        std::ostringstream npcText;
        npcText << "  NPC: " << stats.npcActive << "/" << MemoryPoolManager::NPC_POOL_SIZE
                << " (peak: " << stats.npcPeak << ")";
        renderText(x, y, npcText.str());
        y += lineHeight;

        std::ostringstream fxText;
        fxText << "  Effect: " << stats.effectActive << "/" << MemoryPoolManager::EFFECT_POOL_SIZE
               << " (peak: " << stats.effectPeak << ")";
        renderText(x, y, fxText.str());
        y += lineHeight;

        std::ostringstream texText;
        float texMB = static_cast<float>(stats.textureMemoryBytes) / (1024.0f * 1024.0f);
        texText << "  Texture: " << stats.textureEntries << " entries, "
                << std::fixed << std::setprecision(1) << texMB << "MB"
                << " (hit: " << std::setprecision(0) << stats.textureHitRate << "%)";
        renderText(x, y, texText.str());
        y += lineHeight + sectionGap;
    }

    // Cache Stats
    if (config_.showCacheStats && cacheMgr_) {
        renderText(x, y, "--- Cache ---");
        y += lineHeight;

        auto cStats = cacheMgr_->getStats();

        std::ostringstream l1Text;
        float l1MB = static_cast<float>(cStats.l1MemoryBytes) / (1024.0f * 1024.0f);
        l1Text << "  L1: " << cStats.l1EntryCount << " entries, "
               << std::fixed << std::setprecision(1) << l1MB << "MB"
               << " (hit: " << std::setprecision(0) << cStats.l1HitRate << "%)";
        renderText(x, y, l1Text.str());
        y += lineHeight;

        std::ostringstream l2Text;
        float l2MB = static_cast<float>(cStats.l2DiskBytes) / (1024.0f * 1024.0f);
        l2Text << "  L2: " << cStats.l2EntryCount << " entries, "
               << std::fixed << std::setprecision(1) << l2MB << "MB"
               << " (hit: " << std::setprecision(0) << cStats.l2HitRate << "%)";
        renderText(x, y, l2Text.str());
        y += lineHeight;

        std::ostringstream totalText;
        totalText << "  Overall hit: " << std::fixed << std::setprecision(0)
                  << cStats.overallHitRate << "%";
        renderText(x, y, totalText.str());
        y += lineHeight + sectionGap;
    }

    // Task Stats
    if (config_.showTaskStats && asyncTaskMgr_) {
        renderText(x, y, "--- Async Tasks ---");
        y += lineHeight;

        auto tStats = asyncTaskMgr_->getStats();

        std::ostringstream taskText;
        taskText << "  Active: " << tStats.activeTasks
                 << " Pending: " << tStats.pendingTasks
                 << " Threads: " << tStats.threadCount;
        renderText(x, y, taskText.str());
        y += lineHeight;

        std::ostringstream taskStatsText;
        taskStatsText << "  Done: " << tStats.totalCompleted
                      << " Failed: " << tStats.totalFailed
                      << " Avg: " << std::fixed << std::setprecision(2)
                      << tStats.avgExecutionTimeMs << "ms";
        renderText(x, y, taskStatsText.str());
        y += lineHeight;
    }
}

// ============================================================================
// Rendering Helpers
// ============================================================================

void ProfilerDashboard::renderGraph(const GraphData& graph, int x, int y,
                                     int width, int height,
                                     const std::string& label,
                                     const std::string& unit) {
    // Graph border
    renderText(x, y - 2, label);

    // Draw graph using text characters (simple ASCII graph)
    // Each column represents one sample
    int samples = std::min(width, GRAPH_HISTORY_SIZE);
    float range = graph.maxValue - graph.minValue;
    if (range <= 0.0f) range = 1.0f;

    for (int col = 0; col < samples; ++col) {
        int sampleIdx = (graph.writeIndex - samples + col + GRAPH_HISTORY_SIZE) % GRAPH_HISTORY_SIZE;
        float value = graph.values[sampleIdx];
        float normalized = (value - graph.minValue) / range;
        normalized = std::max(0.0f, std::min(1.0f, normalized));
        int barHeight = static_cast<int>(normalized * height);

        // Render bar as vertical line of characters
        for (int row = 0; row < barHeight && row < height; ++row) {
            // Use block characters for the graph
            int charX = x + col;
            int charY = y + height - row - 1;
            renderText(charX, charY, "|");
        }
    }

    // Scale labels
    std::ostringstream maxLabel;
    maxLabel << std::fixed << std::setprecision(0) << graph.maxValue;
    renderText(x + width + 2, y, maxLabel.str() + unit);

    std::ostringstream minLabel;
    minLabel << std::fixed << std::setprecision(0) << graph.minValue;
    renderText(x + width + 2, y + height - 1, minLabel.str());
}

void ProfilerDashboard::renderText(int x, int y, const std::string& text, float scale) {
    if (textRenderer_) {
        textRenderer_->renderText(text, static_cast<float>(x), static_cast<float>(y),
                                  glm::vec3(1.0f, 1.0f, 1.0f), scale);
    }
}

void ProfilerDashboard::renderBar(int x, int y, int width, int height,
                                   float fillRatio, uint32_t color) {
    // Simple bar rendering using text characters
    int fillWidth = static_cast<int>(fillRatio * width);
    fillWidth = std::max(0, std::min(fillWidth, width));

    std::string bar(fillWidth, '#');
    std::string empty(width - fillWidth, '-');
    renderText(x, y, "[" + bar + empty + "]");
}
