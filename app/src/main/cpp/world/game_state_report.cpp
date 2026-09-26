#include "game_state_report.h"

#include <cmath>
#include <cstdio>

namespace GameStateReport {

const char* phaseName(Phase phase) {
    switch (phase) {
        case Phase::Launcher: return "LAUNCHER";
        case Phase::Title: return "TITLE";
        case Phase::Playing: return "PLAYING";
    }
    return "UNKNOWN";
}

const char* spaceName(Space space) {
    switch (space) {
        case Space::Exterior: return "EXTERIOR";
        case Space::Interior: return "INTERIOR";
    }
    return "UNKNOWN";
}

namespace {

// The clock is game time in hours; clamp so a NaN or an out-of-range value
// (the weather system can push timeOfDay past 24 while advancing) still
// produces a readable HH:MM instead of "nan:nan".
void formatClock(float timeOfDay, char* out, std::size_t size) {
    if (!std::isfinite(timeOfDay)) {
        std::snprintf(out, size, "--:--");
        return;
    }
    float wrapped = std::fmod(timeOfDay, 24.0f);
    if (wrapped < 0.0f) wrapped += 24.0f;
    // Round to the nearest minute, then carry: truncating the fraction reports
    // 16:23 for a time of 16.4 because 16.4f is really 16.39999961.
    int totalMinutes = static_cast<int>(std::lround(wrapped * 60.0f)) % 1440;
    if (totalMinutes < 0) totalMinutes += 1440;
    std::snprintf(out, size, "%02d:%02d", totalMinutes / 60, totalMinutes % 60);
}

}  // namespace

std::string format(const Snapshot& snapshot) {
    std::string report = "GAMESTATE phase=";
    report += phaseName(snapshot.phase);

    if (!snapshot.worldAvailable) {
        report += " world=unavailable";
        return report;
    }

    report += " space=";
    report += spaceName(snapshot.space);

    if (snapshot.hasCell) {
        char formId[16];
        std::snprintf(formId, sizeof(formId), "0x%08X", snapshot.cellFormID);
        report += " cell=\"";
        report += snapshot.cellName;
        report += "\" formID=";
        report += formId;
        if (!snapshot.editorID.empty()) {
            report += " editorID=";
            report += snapshot.editorID;
        }
        if (snapshot.space == Space::Exterior) {
            report += " grid=(" + std::to_string(snapshot.gridX) + "," +
                      std::to_string(snapshot.gridY) + ")";
        }
    } else {
        report += " cell=<none>";
    }

    report += " active=" + std::to_string(snapshot.activeCells);
    report += " cached=" + std::to_string(snapshot.cachedCells);
    report += " terrainCells=" + std::to_string(snapshot.terrainCells);

    char clock[16];
    formatClock(snapshot.timeOfDay, clock, sizeof(clock));
    report += " time=";
    report += clock;
    report += " day=" + std::to_string(snapshot.day);

    if (snapshot.hasPlayerPosition) {
        char position[80];
        std::snprintf(position, sizeof(position), " pos=(%.0f,%.0f,%.0f)",
                      snapshot.playerX, snapshot.playerY, snapshot.playerZ);
        report += position;
    }

    return report;
}

}  // namespace GameStateReport
