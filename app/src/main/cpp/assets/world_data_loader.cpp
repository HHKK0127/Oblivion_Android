#include "world_data_loader.h"
#include <GLES3/gl3.h>
#include <cstring>
#include <cmath>
#include <algorithm>

// ============================================================================
// WorldDataLoader Implementation
// ============================================================================

WorldDataLoader::WorldDataLoader() = default;

WorldDataLoader::~WorldDataLoader() {
    cleanup();
}

bool WorldDataLoader::initialize(const oblivion::ESMManager* esm) {
    if (initialized) {
        LOGW_WDLDR("Already initialized");
        return true;
    }
    esmManager = esm;
    initialized = true;
    LOGI_WDLDR("WorldDataLoader initialized");
    return true;
}

void WorldDataLoader::cleanup() {
    // Free GPU resources for all terrain chunks
    for (auto& wsPair : worldSpaces) {
        auto& ws = wsPair.second;
        for (auto& chunkPair : ws->chunks) {
            auto& chunk = chunkPair.second;
            if (chunk && chunk->terrain) {
                freeTerrainGPU(*chunk->terrain);
            }
        }
    }
    worldSpaces.clear();

    for (auto& icPair : interiorCells) {
        if (icPair.second && icPair.second->terrain) {
            freeTerrainGPU(*icPair.second->terrain);
        }
    }
    interiorCells.clear();

    initialized = false;
    LOGI_WDLDR("WorldDataLoader cleaned up");
}

// ============================================================================
// World Space Management
// ============================================================================

bool WorldDataLoader::loadWorldSpace(uint32_t worldFormID) {
    if (!initialized || !esmManager) return false;

    // Check if already loaded
    if (worldSpaces.find(worldFormID) != worldSpaces.end()) {
        return true;
    }

    const oblivion::WorldData* worldData = esmManager->findWorld(worldFormID);
    if (!worldData) {
        LOGE_WDLDR("World space not found: 0x%08X", worldFormID);
        return false;
    }

    auto ws = std::make_unique<WorldSpace>();
    ws->formID = worldData->formID;
    ws->editorID = worldData->editorID;
    ws->fullName = worldData->fullName;
    ws->minX = worldData->minX;
    ws->minY = worldData->minY;
    ws->maxX = worldData->maxX;
    ws->maxY = worldData->maxY;

    LOGI_WDLDR("Loaded world space: %s (0x%08X, bounds [%d,%d]-[%d,%d])",
               ws->editorID.c_str(), worldFormID,
               ws->minX, ws->minY, ws->maxX, ws->maxY);

    worldSpaces[worldFormID] = std::move(ws);
    return true;
}

const WorldSpace* WorldDataLoader::getWorldSpace(uint32_t worldFormID) const {
    auto it = worldSpaces.find(worldFormID);
    return (it != worldSpaces.end()) ? it->second.get() : nullptr;
}

std::vector<uint32_t> WorldDataLoader::getLoadedWorldSpaceIDs() const {
    std::vector<uint32_t> ids;
    ids.reserve(worldSpaces.size());
    for (const auto& pair : worldSpaces) {
        ids.push_back(pair.first);
    }
    return ids;
}

// ============================================================================
// CELL -> World Chunk Loading
// ============================================================================

std::shared_ptr<WorldChunk> WorldDataLoader::loadCell(uint32_t cellFormID) {
    if (!initialized || !esmManager) return nullptr;

    const oblivion::CellData* cellData = esmManager->findCell(cellFormID);
    if (!cellData) {
        LOGE_WDLDR("Cell not found: 0x%08X", cellFormID);
        return nullptr;
    }

    // Interior cell
    if (cellData->worldspaceID == 0) {
        return loadInteriorCell(cellFormID);
    }

    // Exterior cell - find or create world space
    uint32_t worldID = cellData->worldspaceID;
    if (worldSpaces.find(worldID) == worldSpaces.end()) {
        if (!loadWorldSpace(worldID)) {
            LOGE_WDLDR("Failed to load world space: 0x%08X", worldID);
            return nullptr;
        }
    }

    auto& ws = worldSpaces[worldID];
    int64_t key = cellKey(cellData->gridX, cellData->gridY);

    // Already loaded?
    auto it = ws->chunks.find(key);
    if (it != ws->chunks.end() && it->second->loaded) {
        return std::shared_ptr<WorldChunk>(it->second.get(),
                                            [](WorldChunk*) {});
    }

    // Create new chunk
    auto chunk = std::make_unique<WorldChunk>();
    chunk->cellFormID = cellFormID;
    chunk->gridX = cellData->gridX;
    chunk->gridY = cellData->gridY;
    chunk->fullName = cellData->fullName;
    chunk->isInterior = false;

    // Load objects
    loadCellObjects(*chunk, cellFormID);

    // Load terrain
    loadCellTerrain(*chunk, cellFormID);

    chunk->loaded = true;

    LOGI_WDLDR("Loaded cell chunk: [%d,%d] (0x%08X, %lu objects)",
               chunk->gridX, chunk->gridY, cellFormID,
               static_cast<unsigned long>(chunk->objects.size()));

    auto result = std::shared_ptr<WorldChunk>(chunk.get(), [](WorldChunk*) {});
    ws->chunks[key] = std::move(chunk);
    return result;
}

std::shared_ptr<WorldChunk> WorldDataLoader::loadCellByGrid(
    uint32_t worldFormID, int32_t gridX, int32_t gridY) {
    if (!initialized || !esmManager) return nullptr;

    // Find cell by grid coordinates
    const auto& allCells = esmManager->getAllCells();
    for (const auto& cell : allCells) {
        if (cell.worldspaceID == worldFormID &&
            cell.gridX == gridX && cell.gridY == gridY) {
            return loadCell(cell.formID);
        }
    }

    LOGW_WDLDR("Cell not found at grid [%d,%d] in world 0x%08X",
               gridX, gridY, worldFormID);
    return nullptr;
}

const WorldChunk* WorldDataLoader::getChunk(uint32_t worldFormID,
                                              int32_t gridX,
                                              int32_t gridY) const {
    auto wsIt = worldSpaces.find(worldFormID);
    if (wsIt == worldSpaces.end()) return nullptr;

    int64_t key = cellKey(gridX, gridY);
    auto it = wsIt->second->chunks.find(key);
    if (it == wsIt->second->chunks.end()) return nullptr;
    return it->second.get();
}

// ============================================================================
// REFR -> Positioned Objects
// ============================================================================

std::vector<WorldObject> WorldDataLoader::extractCellObjects(
    uint32_t cellFormID) const {
    std::vector<WorldObject> objects;
    if (!esmManager) return objects;

    const auto& allRefs = esmManager->getAllReferences();
    for (const auto& ref : allRefs) {
        if (ref.cellFormID != cellFormID) continue;

        WorldObject obj;
        obj.refFormID = ref.formID;
        obj.baseFormID = ref.baseFormID;
        obj.position = ref.position;
        obj.rotation = ref.rotation;
        obj.scale = ref.scale;
        obj.flags = ref.flags;
        obj.modelPath = resolveModelPath(ref.baseFormID);
        objects.push_back(obj);
    }

    return objects;
}

std::string WorldDataLoader::resolveModelPath(uint32_t baseFormID) const {
    if (!esmManager) return "";

    // Try common record types that have model paths
    const oblivion::StaticData* stat = esmManager->findStatic(baseFormID);
    if (stat) return stat->modelPath;

    const oblivion::ContainerData* cont = esmManager->findContainer(baseFormID);
    if (cont) return cont->modelPath;

    const oblivion::LightData* light = esmManager->findLight(baseFormID);
    if (light) return light->modelPath;

    const oblivion::ActivatorData* acti = esmManager->findActivator(baseFormID);
    if (acti) return acti->modelPath;

    const oblivion::TreeData* tree = esmManager->findTree(baseFormID);
    if (tree) return tree->modelPath;

    const oblivion::FloraData* flora = esmManager->findFlora(baseFormID);
    if (flora) return flora->modelPath;

    const oblivion::ArmorData* armo = esmManager->findArmor(baseFormID);
    if (armo) return armo->modelPath;

    const oblivion::ClothingData* clot = esmManager->findClothing(baseFormID);
    if (clot) return clot->modelPath;

    const oblivion::MiscItemData* misc = esmManager->findMiscItem(baseFormID);
    if (misc) return misc->modelPath;

    const oblivion::BookData* book = esmManager->findBook(baseFormID);
    if (book) return book->modelPath;

    const oblivion::IngredientData* ingr = esmManager->findIngredient(baseFormID);
    if (ingr) return ingr->modelPath;

    const oblivion::AlchemyData* alch = esmManager->findAlchemy(baseFormID);
    if (alch) return alch->modelPath;

    const oblivion::ApparatusData* appa = esmManager->findApparatus(baseFormID);
    if (appa) return appa->modelPath;

    return "";
}

// ============================================================================
// LAND -> Terrain Mesh Generation
// ============================================================================

std::unique_ptr<TerrainChunk> WorldDataLoader::generateTerrain(
    uint32_t cellFormID) {
    if (!esmManager) return nullptr;

    const oblivion::TerrainData* terrainData = nullptr;
    const auto& allTerrains = esmManager->getAllTerrains();
    for (const auto& t : allTerrains) {
        if (t.formID == cellFormID) {
            terrainData = &t;
            break;
        }
    }

    if (!terrainData || !terrainData->hasHeights()) {
        return nullptr;
    }

    auto chunk = std::make_unique<TerrainChunk>();

    // Get cell grid position
    const oblivion::CellData* cellData = esmManager->findCell(cellFormID);
    if (cellData) {
        chunk->cellX = cellData->gridX;
        chunk->cellY = cellData->gridY;
    }

    // Copy heightmap data
    size_t heightCount = terrainData->heights.size();
    for (int y = 0; y < TERRAIN_GRID_SIZE; y++) {
        for (int x = 0; x < TERRAIN_GRID_SIZE; x++) {
            size_t idx = static_cast<size_t>(y * TERRAIN_GRID_SIZE + x);
            if (idx < heightCount) {
                chunk->heights[y][x] = terrainData->heights[idx];
            } else {
                chunk->heights[y][x] = 0.0f;
            }
        }
    }

    return chunk;
}

bool WorldDataLoader::uploadTerrainGPU(TerrainChunk& chunk) {
    if (chunk.gpuUploaded) return true;

    // Generate mesh vertices from heightmap
    // Each cell is CELL_SIZE x CELL_SIZE world units
    struct TerrainVertex {
        float position[3];
        float normal[3];
        float texCoord[2];
    };

    float cellOriginX = chunk.cellX * CELL_SIZE;
    float cellOriginY = chunk.cellY * CELL_SIZE;
    float step = CELL_SIZE / (TERRAIN_GRID_SIZE - 1);

    std::vector<TerrainVertex> vertices(TERRAIN_GRID_SIZE * TERRAIN_GRID_SIZE);
    for (int y = 0; y < TERRAIN_GRID_SIZE; y++) {
        for (int x = 0; x < TERRAIN_GRID_SIZE; x++) {
            int idx = y * TERRAIN_GRID_SIZE + x;
            TerrainVertex& v = vertices[idx];

            v.position[0] = cellOriginX + x * step;
            v.position[1] = cellOriginY + y * step;
            v.position[2] = chunk.heights[y][x];

            // Calculate normal from neighboring heights
            float hL = (x > 0) ? chunk.heights[y][x - 1] : chunk.heights[y][x];
            float hR = (x < TERRAIN_GRID_SIZE - 1) ?
                       chunk.heights[y][x + 1] : chunk.heights[y][x];
            float hD = (y > 0) ? chunk.heights[y - 1][x] : chunk.heights[y][x];
            float hU = (y < TERRAIN_GRID_SIZE - 1) ?
                       chunk.heights[y + 1][x] : chunk.heights[y][x];

            float nx = hL - hR;
            float ny = hD - hU;
            float nz = 2.0f * step;
            float len = std::sqrt(nx * nx + ny * ny + nz * nz);
            v.normal[0] = nx / len;
            v.normal[1] = ny / len;
            v.normal[2] = nz / len;

            v.texCoord[0] = static_cast<float>(x) / (TERRAIN_GRID_SIZE - 1);
            v.texCoord[1] = static_cast<float>(y) / (TERRAIN_GRID_SIZE - 1);
        }
    }

    // Generate triangle indices
    std::vector<uint32_t> indices;
    indices.reserve((TERRAIN_GRID_SIZE - 1) * (TERRAIN_GRID_SIZE - 1) * 6);
    for (int y = 0; y < TERRAIN_GRID_SIZE - 1; y++) {
        for (int x = 0; x < TERRAIN_GRID_SIZE - 1; x++) {
            uint32_t topLeft = y * TERRAIN_GRID_SIZE + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (y + 1) * TERRAIN_GRID_SIZE + x;
            uint32_t bottomRight = bottomLeft + 1;

            // Two triangles per quad
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    chunk.indexCount = static_cast<uint32_t>(indices.size());

    // Create VAO
    glGenVertexArrays(1, &chunk.vao);
    glBindVertexArray(chunk.vao);

    // VBO
    glGenBuffers(1, &chunk.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(TerrainVertex)),
                 vertices.data(), GL_STATIC_DRAW);

    // Position (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                           reinterpret_cast<void*>(0));

    // Normal (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                           reinterpret_cast<void*>(12));

    // TexCoord (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TerrainVertex),
                           reinterpret_cast<void*>(24));

    // EBO
    glGenBuffers(1, &chunk.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
                 indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);

    chunk.gpuUploaded = true;
    return true;
}

void WorldDataLoader::freeTerrainGPU(TerrainChunk& chunk) {
    if (!chunk.gpuUploaded) return;

    if (chunk.vao) {
        glDeleteVertexArrays(1, &chunk.vao);
        chunk.vao = 0;
    }
    if (chunk.vbo) {
        glDeleteBuffers(1, &chunk.vbo);
        chunk.vbo = 0;
    }
    if (chunk.ebo) {
        glDeleteBuffers(1, &chunk.ebo);
        chunk.ebo = 0;
    }
    chunk.gpuUploaded = false;
}

// ============================================================================
// Streaming Loader
// ============================================================================

void WorldDataLoader::updateStreaming(const glm::vec3& playerWorldPos,
                                       uint32_t worldFormID) {
    if (!initialized || !esmManager) return;

    // Calculate player cell coordinates
    int32_t playerCellX = static_cast<int32_t>(
        std::floor(playerWorldPos.x / CELL_SIZE));
    int32_t playerCellY = static_cast<int32_t>(
        std::floor(playerWorldPos.y / CELL_SIZE));

    // Only update if player moved to a new cell
    if (playerCellX == lastPlayerCellX && playerCellY == lastPlayerCellY) {
        return;
    }
    lastPlayerCellX = playerCellX;
    lastPlayerCellY = playerCellY;

    // Ensure world space is loaded
    if (worldSpaces.find(worldFormID) == worldSpaces.end()) {
        if (!loadWorldSpace(worldFormID)) return;
    }

    auto& ws = worldSpaces[worldFormID];

    // Load nearby cells
    for (int dy = -loadRadius; dy <= loadRadius; dy++) {
        for (int dx = -loadRadius; dx <= loadRadius; dx++) {
            int32_t cx = playerCellX + dx;
            int32_t cy = playerCellY + dy;
            int64_t key = cellKey(cx, cy);

            auto it = ws->chunks.find(key);
            if (it != ws->chunks.end() && it->second->loaded) {
                continue; // Already loaded
            }

            // Try to load this cell
            auto chunk = loadCellByGrid(worldFormID, cx, cy);
            if (chunk && onLoadCallback) {
                onLoadCallback(*chunk);
            }
        }
    }

    // Unload distant cells
    unloadDistantChunks(*ws, playerCellX, playerCellY);
}

void WorldDataLoader::unloadDistantChunks(WorldSpace& ws,
                                            int32_t playerX, int32_t playerY) {
    std::vector<int64_t> toRemove;

    for (auto& pair : ws.chunks) {
        auto& chunk = pair.second;
        if (!chunk || !chunk->loaded) continue;

        int32_t dx = std::abs(chunk->gridX - playerX);
        int32_t dy = std::abs(chunk->gridY - playerY);

        if (dx > unloadRadius || dy > unloadRadius) {
            // Free terrain GPU resources
            if (chunk->terrain) {
                freeTerrainGPU(*chunk->terrain);
            }

            if (onUnloadCallback) {
                onUnloadCallback(chunk->cellFormID);
            }

            toRemove.push_back(pair.first);
        }
    }

    for (int64_t key : toRemove) {
        ws.chunks.erase(key);
    }

    if (!toRemove.empty()) {
        LOGD_WDLDR("Unloaded %lu distant chunks",
                    static_cast<unsigned long>(toRemove.size()));
    }
}

size_t WorldDataLoader::getLoadedChunkCount() const {
    size_t count = 0;
    for (const auto& wsPair : worldSpaces) {
        for (const auto& chunkPair : wsPair.second->chunks) {
            if (chunkPair.second && chunkPair.second->loaded) {
                count++;
            }
        }
    }
    count += interiorCells.size();
    return count;
}

// ============================================================================
// Interior Cells
// ============================================================================

std::shared_ptr<WorldChunk> WorldDataLoader::loadInteriorCell(
    uint32_t cellFormID) {
    // Check cache
    auto it = interiorCells.find(cellFormID);
    if (it != interiorCells.end() && it->second->loaded) {
        return it->second;
    }

    const oblivion::CellData* cellData = esmManager->findCell(cellFormID);
    if (!cellData) {
        LOGE_WDLDR("Interior cell not found: 0x%08X", cellFormID);
        return nullptr;
    }

    auto chunk = std::make_shared<WorldChunk>();
    chunk->cellFormID = cellFormID;
    chunk->gridX = cellData->gridX;
    chunk->gridY = cellData->gridY;
    chunk->fullName = cellData->fullName;
    chunk->isInterior = true;

    // Load objects
    loadCellObjects(*chunk, cellFormID);

    chunk->loaded = true;

    LOGI_WDLDR("Loaded interior cell: %s (0x%08X, %lu objects)",
               chunk->fullName.c_str(), cellFormID,
               static_cast<unsigned long>(chunk->objects.size()));

    interiorCells[cellFormID] = chunk;
    return chunk;
}

bool WorldDataLoader::isInteriorCell(uint32_t cellFormID) const {
    const oblivion::CellData* cellData = esmManager->findCell(cellFormID);
    return cellData && cellData->worldspaceID == 0;
}

// ============================================================================
// Internal Helpers
// ============================================================================

bool WorldDataLoader::loadCellObjects(WorldChunk& chunk,
                                        uint32_t cellFormID) const {
    chunk.objects = extractCellObjects(cellFormID);
    return true;
}

bool WorldDataLoader::loadCellTerrain(WorldChunk& chunk,
                                        uint32_t cellFormID) {
    auto terrain = generateTerrain(cellFormID);
    if (terrain) {
        chunk.terrain = std::move(terrain);
        return true;
    }
    return false;
}
