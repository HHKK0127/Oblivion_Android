#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

/**
 * @brief Phase 66 P16: the one-line "where is the game right now?" report.
 *
 * Device verification repeatedly went wrong because nothing cheaply answered
 * whether the world was even being drawn (the title screen early-returns before
 * any world rendering, so every world measurement taken there is meaningless).
 * This is deliberately a pure formatter: Renderer gathers a Snapshot from the
 * live systems and GameStateReport turns it into a single line, so the format
 * itself is covered by a host test with no game data or device needed.
 */
namespace GameStateReport {

enum class Phase : uint8_t {
    Launcher = 0,   // launcher screen (nothing of the game exists yet)
    Title = 1,      // title screen - the world is NOT rendered
    Playing = 2     // in game
};

enum class Space : uint8_t {
    Exterior = 0,
    Interior = 1
};

struct Snapshot {
    Phase phase = Phase::Launcher;
    // False until WorldManager exists, so a report is still printable during
    // startup instead of crashing or printing half a line.
    bool worldAvailable = false;

    Space space = Space::Exterior;
    bool hasCell = false;
    std::string cellName;
    std::string editorID;
    uint32_t cellFormID = 0;
    // Grid coordinates only exist for exterior cells; interior cells are keyed
    // by TES FormID, so `grid` is omitted when space == Interior.
    int32_t gridX = 0;
    int32_t gridY = 0;

    std::size_t activeCells = 0;
    std::size_t cachedCells = 0;
    std::size_t terrainCells = 0;

    float timeOfDay = 0.0f;
    uint32_t day = 0;

    bool hasPlayerPosition = false;
    float playerX = 0.0f;
    float playerY = 0.0f;
    float playerZ = 0.0f;
};

const char* phaseName(Phase phase);
const char* spaceName(Space space);

// Build the report line. Always non-empty and never throws.
std::string format(const Snapshot& snapshot);

}  // namespace GameStateReport
