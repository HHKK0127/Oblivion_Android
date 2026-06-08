#include "map_system.h"
#include <algorithm>
#include <fstream>
#include <iostream>

namespace map {

void MapSystem::setWorldBounds(float minX, float maxX, float minY, float maxY) {
    worldBounds = glm::vec4(minX, maxX, minY, maxY);
}

void MapSystem::discoverCell(int cellX, int cellY) {
    discoveryGrid[cellKey(cellX, cellY)] = true;
}

bool MapSystem::isCellDiscovered(int cellX, int cellY) const {
    auto it = discoveryGrid.find(cellKey(cellX, cellY));
    return (it != discoveryGrid.end() && it->second);
}

void MapSystem::clearDiscovery() {
    discoveryGrid.clear();
}

uint32_t MapSystem::addMarker(const MapMarker& marker) {
    MapMarker m = marker;
    m.id = nextMarkerId++;
    markers.push_back(m);
    return m.id;
}

void MapSystem::removeMarker(uint32_t id) {
    markers.erase(
        std::remove_if(markers.begin(), markers.end(),
            [id](const MapMarker& m) { return m.id == id; }),
        markers.end());
}

void MapSystem::clearMarkersByType(MarkerType type) {
    markers.erase(
        std::remove_if(markers.begin(), markers.end(),
            [type](const MapMarker& m) { return m.type == type; }),
        markers.end());
}

MapMarker* MapSystem::getMarker(uint32_t id) {
    for (auto& m : markers) {
        if (m.id == id) return &m;
    }
    return nullptr;
}

std::vector<MapMarker*> MapSystem::getMarkersByType(MarkerType type) {
    std::vector<MapMarker*> result;
    for (auto& m : markers) {
        if (m.type == type && m.visible) result.push_back(&m);
    }
    return result;
}

void MapSystem::setPlayerPosition(const glm::vec2& pos) {
    playerPos = pos;
}

void MapSystem::addFastTravelPoint(int cellX, int cellY, const std::string& name) {
    CellInfo info;
    info.x = cellX;
    info.y = cellY;
    info.name = name;
    info.discovered = true;
    info.fastTravelEnabled = true;
    cellInfoMap[cellKey(cellX, cellY)] = info;
    discoverCell(cellX, cellY);
}

bool MapSystem::canFastTravelTo(int cellX, int cellY) const {
    auto it = cellInfoMap.find(cellKey(cellX, cellY));
    return (it != cellInfoMap.end() && it->second.fastTravelEnabled);
}

std::vector<CellInfo> MapSystem::getFastTravelPoints() const {
    std::vector<CellInfo> result;
    for (const auto& kv : cellInfoMap) {
        if (kv.second.fastTravelEnabled) result.push_back(kv.second);
    }
    return result;
}

void MapSystem::setCellInfo(int cellX, int cellY, const CellInfo& info) {
    cellInfoMap[cellKey(cellX, cellY)] = info;
}

CellInfo* MapSystem::getCellInfo(int cellX, int cellY) {
    auto it = cellInfoMap.find(cellKey(cellX, cellY));
    if (it != cellInfoMap.end()) return &it->second;
    return nullptr;
}

glm::vec2 MapSystem::worldToCell(const glm::vec2& worldPos, float cellSize) {
    return glm::vec2(
        static_cast<float>(std::floor(worldPos.x / cellSize)),
        static_cast<float>(std::floor(worldPos.y / cellSize))
    );
}

glm::vec2 MapSystem::cellToWorld(int cellX, int cellY, float cellSize) {
    return glm::vec2(
        static_cast<float>(cellX) * cellSize,
        static_cast<float>(cellY) * cellSize
    );
}

void MapSystem::saveState(const std::string& path) const {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return;

    // Write bounds
    file.write(reinterpret_cast<const char*>(&worldBounds), sizeof(worldBounds));
    file.write(reinterpret_cast<const char*>(&playerPos), sizeof(playerPos));
    file.write(reinterpret_cast<const char*>(&cellSize), sizeof(cellSize));

    // Discovery grid
    uint64_t discoveryCount = discoveryGrid.size();
    file.write(reinterpret_cast<const char*>(&discoveryCount), sizeof(discoveryCount));
    for (const auto& kv : discoveryGrid) {
        file.write(reinterpret_cast<const char*>(&kv.first), sizeof(kv.first));
        bool val = kv.second;
        file.write(reinterpret_cast<const char*>(&val), sizeof(val));
    }

    // Cell info
    uint64_t cellCount = cellInfoMap.size();
    file.write(reinterpret_cast<const char*>(&cellCount), sizeof(cellCount));
    for (const auto& kv : cellInfoMap) {
        file.write(reinterpret_cast<const char*>(&kv.first), sizeof(kv.first));
        const CellInfo& c = kv.second;
        file.write(reinterpret_cast<const char*>(&c.x), sizeof(c.x));
        file.write(reinterpret_cast<const char*>(&c.y), sizeof(c.y));
        uint64_t nameLen = c.name.size();
        file.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        file.write(c.name.data(), static_cast<std::streamsize>(nameLen));
        file.write(reinterpret_cast<const char*>(&c.discovered), sizeof(c.discovered));
        file.write(reinterpret_cast<const char*>(&c.fastTravelEnabled), sizeof(c.fastTravelEnabled));
        file.write(reinterpret_cast<const char*>(&c.terrainColor), sizeof(c.terrainColor));
    }

    // Markers
    uint64_t markerCount = markers.size();
    file.write(reinterpret_cast<const char*>(&markerCount), sizeof(markerCount));
    for (const auto& m : markers) {
        file.write(reinterpret_cast<const char*>(&m.id), sizeof(m.id));
        uint32_t type = static_cast<uint32_t>(m.type);
        file.write(reinterpret_cast<const char*>(&type), sizeof(type));
        file.write(reinterpret_cast<const char*>(&m.worldPos), sizeof(m.worldPos));
        uint64_t labelLen = m.label.size();
        file.write(reinterpret_cast<const char*>(&labelLen), sizeof(labelLen));
        file.write(m.label.data(), static_cast<std::streamsize>(labelLen));
        file.write(reinterpret_cast<const char*>(&m.iconId), sizeof(m.iconId));
        file.write(reinterpret_cast<const char*>(&m.visible), sizeof(m.visible));
        file.write(reinterpret_cast<const char*>(&m.questId), sizeof(m.questId));
        file.write(reinterpret_cast<const char*>(&m.color), sizeof(m.color));
    }
}

void MapSystem::loadState(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return;

    discoveryGrid.clear();
    cellInfoMap.clear();
    markers.clear();
    nextMarkerId = 1;

    file.read(reinterpret_cast<char*>(&worldBounds), sizeof(worldBounds));
    file.read(reinterpret_cast<char*>(&playerPos), sizeof(playerPos));
    file.read(reinterpret_cast<char*>(&cellSize), sizeof(cellSize));

    uint64_t discoveryCount = 0;
    file.read(reinterpret_cast<char*>(&discoveryCount), sizeof(discoveryCount));
    for (uint64_t i = 0; i < discoveryCount; ++i) {
        uint64_t key = 0;
        bool val = false;
        file.read(reinterpret_cast<char*>(&key), sizeof(key));
        file.read(reinterpret_cast<char*>(&val), sizeof(val));
        discoveryGrid[key] = val;
    }

    uint64_t cellCount = 0;
    file.read(reinterpret_cast<char*>(&cellCount), sizeof(cellCount));
    for (uint64_t i = 0; i < cellCount; ++i) {
        uint64_t key = 0;
        file.read(reinterpret_cast<char*>(&key), sizeof(key));
        CellInfo c;
        file.read(reinterpret_cast<char*>(&c.x), sizeof(c.x));
        file.read(reinterpret_cast<char*>(&c.y), sizeof(c.y));
        uint64_t nameLen = 0;
        file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        c.name.resize(nameLen);
        file.read(c.name.data(), static_cast<std::streamsize>(nameLen));
        file.read(reinterpret_cast<char*>(&c.discovered), sizeof(c.discovered));
        file.read(reinterpret_cast<char*>(&c.fastTravelEnabled), sizeof(c.fastTravelEnabled));
        file.read(reinterpret_cast<char*>(&c.terrainColor), sizeof(c.terrainColor));
        cellInfoMap[key] = c;
    }

    uint64_t markerCount = 0;
    file.read(reinterpret_cast<char*>(&markerCount), sizeof(markerCount));
    for (uint64_t i = 0; i < markerCount; ++i) {
        MapMarker m;
        file.read(reinterpret_cast<char*>(&m.id), sizeof(m.id));
        uint32_t type = 0;
        file.read(reinterpret_cast<char*>(&type), sizeof(type));
        m.type = static_cast<MarkerType>(type);
        file.read(reinterpret_cast<char*>(&m.worldPos), sizeof(m.worldPos));
        uint64_t labelLen = 0;
        file.read(reinterpret_cast<char*>(&labelLen), sizeof(labelLen));
        m.label.resize(labelLen);
        file.read(m.label.data(), static_cast<std::streamsize>(labelLen));
        file.read(reinterpret_cast<char*>(&m.iconId), sizeof(m.iconId));
        file.read(reinterpret_cast<char*>(&m.visible), sizeof(m.visible));
        file.read(reinterpret_cast<char*>(&m.questId), sizeof(m.questId));
        file.read(reinterpret_cast<char*>(&m.color), sizeof(m.color));
        markers.push_back(m);
        if (m.id >= nextMarkerId) nextMarkerId = m.id + 1;
    }
}

} // namespace map
