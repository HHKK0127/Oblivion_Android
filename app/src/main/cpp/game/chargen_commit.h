// game/chargen_commit.h — the chargen "Done -> Yes" commit, shared by the
// renderer's RaceSexPanel callback and the host newgame smoke.
//
// Keeping this in one function is deliberate: a smoke that re-implements the
// commit can stay green while the renderer's real path regresses. Both callers
// go through chargen::commit() so that cannot happen.
#pragma once

#include <cstdint>
#include <string>

class NpcManager;
struct Player;

namespace chargen {

// The chargen state the commit consumes (spec §10.2/§10.4).
struct CommitInput {
    // The original's 0x51C090 no-op guard: the race is applied only when the
    // selection differs from the player's current race.
    bool raceChanged = false;
    uint32_t raceFormID = 0;
    // The entered name. Empty means "leave the current name" (the renderer only
    // writes a non-empty field).
    std::string name;
    // V-631-1b: the Gender toggle's value. Player::gender is 0=Male / 1=Female
    // (player.h:69; face_race_presets.h Gender::MALE=0/FEMALE=1). The toggle's
    // OFF/ON meaning is the port's choice (the original's is not measured, #199):
    // OFF = Male (0), ON = Female (1), which matches the Player default.
    uint8_t gender = 0;
};

struct CommitResult {
    bool raceApplied = false;
    bool nameApplied = false;
    bool genderApplied = false;
};

// Applies the confirmed chargen state to `player`: the race (only when changed)
// then the name (when non-empty). `npcMgr` may be null — the race FormID is
// still recorded, but the racial bonuses/spells are skipped (the renderer's
// null-guard).
CommitResult commit(NpcManager* npcMgr, Player& player, const CommitInput& in);

}  // namespace chargen
