// Game state report tests - see gamestate_report_tests.h for the rationale.

#include "gamestate_report_tests.h"

#include "../world/game_state_report.h"

#include <chrono>
#include <string>

using GameStateReport::Phase;
using GameStateReport::Snapshot;
using GameStateReport::Space;

namespace {

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

// A fully populated exterior state: this is what a healthy in-game frame should
// look like on the device.
Snapshot makeExterior() {
    Snapshot s;
    s.phase = Phase::Playing;
    s.worldAvailable = true;
    s.space = Space::Exterior;
    s.hasCell = true;
    s.cellName = "Imperial City";
    s.editorID = "ICMarketDistrict";
    s.cellFormID = 0x0001D1F0;
    s.gridX = -2;
    s.gridY = 3;
    s.activeCells = 9;
    s.cachedCells = 9;
    s.terrainCells = 9;
    s.timeOfDay = 16.4f;
    s.day = 0;
    s.hasPlayerPosition = true;
    s.playerX = 1234.6f;
    s.playerY = -567.2f;
    s.playerZ = 88.0f;
    return s;
}

}  // namespace

void GameStateReportTests::record(const std::string& name, bool passed,
                                  const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int GameStateReportTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int GameStateReportTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string GameStateReportTests::getSummary() const {
    return "Game State Report Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool GameStateReportTests::runAllTests() {
    results.clear();
    const auto t0 = std::chrono::high_resolution_clock::now();

    // 1. The report is always a single line - the device parser greps one line.
    {
        const std::string line = GameStateReport::format(makeExterior());
        record("report is non-empty", !line.empty());
        record("report has no newline", line.find('\n') == std::string::npos);
        record("report is prefixed with GAMESTATE",
               line.rfind("GAMESTATE ", 0) == 0);
    }

    // 2. Exterior: every field a verification script depends on is present.
    {
        const std::string line = GameStateReport::format(makeExterior());
        record("exterior phase is PLAYING", contains(line, " phase=PLAYING"));
        record("exterior space is EXTERIOR", contains(line, " space=EXTERIOR"));
        record("exterior prints the cell name",
               contains(line, "cell=\"Imperial City\""));
        record("exterior prints the FormID as 8 hex digits",
               contains(line, "formID=0x0001D1F0"));
        record("exterior prints the editorID",
               contains(line, "editorID=ICMarketDistrict"));
        record("exterior prints the grid", contains(line, " grid=(-2,3)"));
        record("exterior prints the cell counts",
               contains(line, " active=9 cached=9 terrainCells=9"));
        record("exterior prints the clock", contains(line, " time=16:24"));
        record("exterior prints the day", contains(line, " day=0"));
        record("exterior prints the player position",
               contains(line, " pos=(1235,-567,88)"));
    }

    // 3. Interior: the P15 case. No grid - an interior cell is keyed by FormID,
    //    and printing grid=(0,0) would read as a real worldspace coordinate.
    {
        Snapshot s = makeExterior();
        s.space = Space::Interior;
        s.cellName = "Chorrol Fighters Guild";
        s.editorID = "ChorrolFightersGuild";
        s.cellFormID = 0x0002C4B7;
        s.gridX = 0;
        s.gridY = 0;
        s.terrainCells = 0;
        const std::string line = GameStateReport::format(s);
        record("interior space is INTERIOR", contains(line, " space=INTERIOR"));
        record("interior prints the cell name",
               contains(line, "cell=\"Chorrol Fighters Guild\""));
        record("interior omits the grid", !contains(line, "grid="));
        record("interior still prints the FormID",
               contains(line, "formID=0x0002C4B7"));
    }

    // 4. Title screen: the trap that caused the P15 misdiagnosis. The phase must
    //    be distinguishable from PLAYING before any world data is trusted.
    {
        Snapshot s = makeExterior();
        s.phase = Phase::Title;
        const std::string line = GameStateReport::format(s);
        record("title phase is TITLE", contains(line, " phase=TITLE"));
        record("title phase is not PLAYING", !contains(line, "phase=PLAYING"));
    }

    // 5. Launcher and the pre-world window must not produce a malformed line.
    {
        Snapshot s;
        s.phase = Phase::Launcher;
        const std::string line = GameStateReport::format(s);
        record("launcher phase is LAUNCHER", contains(line, " phase=LAUNCHER"));
        record("missing world is reported", contains(line, " world=unavailable"));
        record("launcher does not print a cell",
               !contains(line, "cell="));
    }
    {
        Snapshot s;
        s.phase = Phase::Playing;
        s.worldAvailable = true;
        const std::string line = GameStateReport::format(s);
        record("no current cell is reported as <none>",
               contains(line, " cell=<none>"));
        record("no player position omits pos", !contains(line, "pos="));
        record("no cell still prints counts",
               contains(line, " active=0 cached=0 terrainCells=0"));
    }

    // 6. The clock is clamped: weather can advance time past 24h or into NaN,
    //    and neither may leak into the log line.
    {
        Snapshot s = makeExterior();
        s.timeOfDay = 25.5f;
        record("time past midnight wraps", contains(GameStateReport::format(s), " time=01:30"));
        s.timeOfDay = -1.25f;
        record("negative time wraps", contains(GameStateReport::format(s), " time=22:45"));
        s.timeOfDay = 0.0f;
        record("midnight formats", contains(GameStateReport::format(s), " time=00:00"));
    }

    // 7. An empty editorID is dropped rather than printed as editorID=.
    {
        Snapshot s = makeExterior();
        s.editorID.clear();
        const std::string line = GameStateReport::format(s);
        record("empty editorID is omitted", !contains(line, "editorID="));
        record("empty editorID keeps the name",
               contains(line, "cell=\"Imperial City\""));
    }

    const auto t1 = std::chrono::high_resolution_clock::now();
    const float totalMs =
        std::chrono::duration<float, std::milli>(t1 - t0).count();
    for (auto& r : results) r.durationMs = totalMs / static_cast<float>(results.size());

    return getFailCount() == 0;
}
