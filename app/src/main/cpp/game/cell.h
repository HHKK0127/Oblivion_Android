#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "../geometry/mesh.h"

// Forward declarations
class WorldObject;

// Cell types
enum class CellType {
    INTERIOR,     // Underground/indoor cell
    EXTERIOR      // Outdoor cell (grid-based)
};

// World object (NPC, item, furniture, etc.)
struct WorldObject {
    uint32_t refId;                    // Reference ID
    std::string modelPath;             // Path to NIF model
    glm::vec3 position;                // World position
    glm::vec3 rotation;                // Rotation (Euler angles)
    glm::vec3 scale;                   // Scale
    std::shared_ptr<Mesh> mesh;        // Loaded mesh
    bool visible;                      // Visibility flag

    WorldObject()
        : refId(0), position(0.0f, 0.0f, 0.0f), rotation(0.0f, 0.0f, 0.0f),
          scale(1.0f, 1.0f, 1.0f), mesh(nullptr), visible(true) {}
};

// Cell - represents a single interior or exterior cell
class Cell {
public:
    Cell(uint32_t cellId, const std::string& cellName, CellType type);
    ~Cell();

    // Getters
    uint32_t getCellId() const { return cellId; }
    const std::string& getCellName() const { return cellName; }
    CellType getCellType() const { return cellType; }
    size_t getObjectCount() const { return objects.size(); }
    const std::vector<std::shared_ptr<WorldObject>>& getObjects() const { return objects; }

    // Object management
    void addObject(std::shared_ptr<WorldObject> obj);
    void removeObject(uint32_t refId);
    std::shared_ptr<WorldObject> getObject(uint32_t refId);

    // Cell grid coordinates (for exterior cells)
    void setCellGridCoords(int32_t x, int32_t y) { gridX = x; gridY = y; }
    int32_t getGridX() const { return gridX; }
    int32_t getGridY() const { return gridY; }

    // Memory management
    void loadObjects();   // Load all objects (meshes, etc.)
    void unloadObjects(); // Unload all objects
    bool isLoaded() const { return loaded; }

    // Get cell bounds
    glm::vec3 getBoundsMin() const { return boundsMin; }
    glm::vec3 getBoundsMax() const { return boundsMax; }
    void setBounds(const glm::vec3& min, const glm::vec3& max) {
        boundsMin = min;
        boundsMax = max;
    }

private:
    uint32_t cellId;
    std::string cellName;
    CellType cellType;
    int32_t gridX, gridY;              // Grid coordinates (exterior cells only)
    std::vector<std::shared_ptr<WorldObject>> objects;
    glm::vec3 boundsMin, boundsMax;    // Cell boundary box
    bool loaded;

    void logCellInfo() const;
};
