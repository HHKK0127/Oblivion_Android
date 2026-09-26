// Interior cell tests - see interior_cell_tests.h for the rationale.
//
// Exercises WorldManager::enterInteriorCell, the entry point behind the
// `teleportinterior` console command. Asserts that an interior cell is
// registered, keyed by FormID (not by a grid coordinate), marked INTERIOR, and
// made the current cell, and that re-entering the same FormID reuses it.

#include "interior_cell_tests.h"

#include "../world/world_manager.h"

#include <chrono>

namespace {

constexpr uint32_t kFormChorrol = 0x0001D1F0;
constexpr uint32_t kFormBruma = 0x0001D1F1;

}  // namespace

void InteriorCellTests::record(const std::string& name, bool passed,
                               const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int InteriorCellTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int InteriorCellTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string InteriorCellTests::getSummary() const {
    return "Interior Cell Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool InteriorCellTests::runAllTests() {
    results.clear();
    const auto t0 = std::chrono::high_resolution_clock::now();

    WorldManager wm;

    // 1. A fresh manager has no current cell.
    record("no current cell before entering", wm.getCurrentCell() == nullptr);
    record("not indoors before entering", !wm.isPlayerIndoors());

    // 2. Entering an interior cell registers it and makes it current.
    auto cell = wm.enterInteriorCell(kFormChorrol, "ChorrolFightersGuild",
                                     "Chorrol Fighters Guild");
    record("enterInteriorCell returns a cell", cell != nullptr);
    record("entered cell is current", wm.getCurrentCell() == cell);
    record("entered cell is INTERIOR",
           cell && cell->cellType == CellType::INTERIOR);
    record("entered cell keeps its FormID",
           cell && cell->tesFormID == kFormChorrol);
    record("entered cell has no worldspace",
           cell && cell->worldspaceFormID == 0);
    record("entered cell is LOADED",
           cell && cell->loadState == CellLoadState::LOADED);
    record("entered cell uses the full name",
           cell && cell->cellName == "Chorrol Fighters Guild");
    record("player is indoors after entering", wm.isPlayerIndoors());

    // 3. The cell is reachable by FormID lookup.
    record("cell is findable by FormID",
           wm.getCellByFormID(kFormChorrol) == cell);

    // 4. Re-entering the same FormID reuses the existing cell (no duplicate).
    auto again = wm.enterInteriorCell(kFormChorrol, "ChorrolFightersGuild",
                                      "Chorrol Fighters Guild");
    record("re-entering reuses the same cell", again == cell);
    record("re-entering does not grow the cell map",
           wm.getAllCellsMap().size() == 1);

    // 5. A second interior cell is distinct and becomes current.
    auto second = wm.enterInteriorCell(kFormBruma, "BrumaMagesGuild",
                                       "Bruma Mages Guild");
    record("second interior cell is distinct", second && second != cell);
    record("second interior cell is current", wm.getCurrentCell() == second);
    record("both interior cells are registered",
           wm.getAllCellsMap().size() == 2);
    record("first cell still findable after second",
           wm.getCellByFormID(kFormChorrol) == cell);

    // 6. FormID 0 is rejected (no valid cell).
    record("FormID 0 is rejected", wm.enterInteriorCell(0, "", "") == nullptr);

    // 7. An empty name falls back to the editor ID.
    auto unnamed = wm.enterInteriorCell(0x0001D1F2, "SomeEditorID", "");
    record("empty name falls back to editor ID",
           unnamed && unnamed->cellName == "SomeEditorID");

    // 8. Exterior cell streaming must not pull the player back outside. The
    //    player controller calls setPlayerPosition() every frame, and interior
    //    cells have no grid coordinate for getCellAt() to resolve.
    wm.enterInteriorCell(kFormChorrol, "ChorrolFightersGuild",
                         "Chorrol Fighters Guild");
    wm.setPlayerPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    record("setPlayerPosition keeps the interior cell current",
           wm.getCurrentCell() == wm.getCellByFormID(kFormChorrol));
    record("still indoors after a position update", wm.isPlayerIndoors());
    wm.setPlayerPosition(glm::vec3(4096.0f, 128.0f, 4096.0f));
    record("a distant position update also keeps the interior",
           wm.getCurrentCell() == wm.getCellByFormID(kFormChorrol));

    // 9. Leaving an interior resumes exterior streaming and clears indoors.
    record("leaveInteriorCell succeeds while indoors", wm.leaveInteriorCell());
    record("not indoors after leaving", !wm.isPlayerIndoors());
    record("leaveInteriorCell is a no-op when already outside",
           !wm.leaveInteriorCell());
    record("interior cells stay registered after leaving",
           wm.getCellByFormID(kFormChorrol) != nullptr);

    // 10. A position update after leaving behaves normally again.
    wm.setPlayerPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    record("position updates apply again after leaving",
           !wm.isPlayerIndoors());

    const auto t1 = std::chrono::high_resolution_clock::now();
    const float totalMs =
        std::chrono::duration<float, std::milli>(t1 - t0).count();
    (void)totalMs;

    return getFailCount() == 0;
}
