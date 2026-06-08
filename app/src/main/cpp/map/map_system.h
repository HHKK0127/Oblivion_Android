#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace map {

enum class MarkerType : uint32_t {
    Player      = 0,
    QuestMain   = 1,
    QuestSide   = 2,
    Location    = 3,
    FastTravel  = 4,
    Custom      = 5
};

struct MapMarker {
    uint32_t    id = 0;
    MarkerType  type = MarkerType::Custom;
    glm::vec2   worldPos{0.0f, 0.0f};
    std::string label;
    uint32_t    iconId = 0;
    bool        visible = true;
    uint32_t    questId = 0; // valid if quest-related
    uint32_t    color = 0xFFFFFFFF; // ABGR
};

struct CellInfo {
    int         x = 0;
    int         y = 0;
    std::string name;
    bool        discovered = false;
    bool        fastTravelEnabled = false;
    uint32_t    terrainColor = 0xFF888888; // ABGR fallback
};

class MapSystem {
public:
    MapSystem() = default;
    ~MapSystem() = default;

    // World bounds (game units)
    void setWorldBounds(float minX, float maxX, float minY, float maxY);
    glm::vec4 getWorldBounds() const { return worldBounds; }

    // Discovery
    void discoverCell(int cellX, int cellY);
    bool isCellDiscovered(int cellX, int cellY) const;
    void clearDiscovery();

    // Markers
    uint32_t addMarker(const MapMarker& marker);
    void removeMarker(uint32_t id);
    void clearMarkersByType(MarkerType type);
    MapMarker* getMarker(uint32_t id);
    const std::vector<MapMarker>& getMarkers() const { return markers; }
    std::vector<MapMarker*> getMarkersByType(MarkerType type);

    // Player position
    void setPlayerPosition(const glm::vec2& pos);
    glm::vec2 getPlayerPosition() const { return playerPos; }

    // Fast travel
    void addFastTravelPoint(int cellX, int cellY, const std::string& name);
    bool canFastTravelTo(int cellX, int cellY) const;
    std::vector<CellInfo> getFastTravelPoints() const;

    // Cell info
    void setCellInfo(int cellX, int cellY, const CellInfo& info);
    CellInfo* getCellInfo(int cellX, int cellY);

    // Cell size accessor
    float getCellSize() const { return cellSize; }

    // Utility
    static glm::vec2 worldToCell(const glm::vec2& worldPos, float cellSize);
    static glm::vec2 cellToWorld(int cellX, int cellY, float cellSize);

    // Serialization (placeholder paths)
    void saveState(const std::string& path) const;
    void loadState(const std::string& path);

private:
    static uint64_t cellKey(int x, int y) {
        return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) |
               static_cast<uint64_t>(static_cast<uint32_t>(y));
    }

    glm::vec4 worldBounds{0.0f, 0.0f, 0.0f, 0.0f};
    glm::vec2 playerPos{0.0f, 0.0f};
    float     cellSize = 4096.0f; // Oblivion-like exterior cell size

    std::vector<MapMarker> markers;
    uint32_t nextMarkerId = 1;

    std::unordered_map<uint64_t, bool>       discoveryGrid;
    std::unordered_map<uint64_t, CellInfo>   cellInfoMap;
};

} // namespace map
