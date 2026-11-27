// game/chargen_commit.cpp — see chargen_commit.h.
#include "chargen_commit.h"

#include "npc_manager.h"
#include "player.h"

namespace chargen {

CommitResult commit(NpcManager* npcMgr, Player& player, const CommitInput& in) {
    CommitResult r;
    // §10.2/§10.4: apply the race when it changed (the original's 0x51C090
    // no-op guard), or when the sex changed (V-664-2), then the name.
    //
    // V-664: the race application reverts to the chargen base and applies the
    // race's base attributes (ATTR), skill bonuses and racial spells through
    // NpcManager (Player is not an NPC, so the NPC path initializePlayerFromESM
    // cannot be used). Health is not wired yet (V-664-3).
    //
    // V-631-1b / V-664-2: write the Gender toggle FIRST. applyRaceToPlayer()
    // picks the RACE ATTR male/female row from player.gender, so writing it
    // afterwards would apply the old sex's row on the first commit (the default
    // is Male). Write only when it changed and log it (the device pass condition
    // is a log line showing gender 0 -> 1 -> 0).
    if (in.gender != player.gender) {
        LOGI("chargen commit: gender %u -> %u", static_cast<unsigned>(player.gender),
             static_cast<unsigned>(in.gender));
        player.gender = in.gender;
        r.genderApplied = true;
    }
    // The race block runs on a race change OR a sex change: applyRaceToPlayer()
    // selects the ATTR male/female row from player.gender, so flipping Gender
    // alone (race unchanged) must re-apply or the previous sex's row survives.
    // applyRaceToPlayer() reverts first, so re-running it is idempotent. Only the
    // raceFormID write stays on raceChanged, and a sex-only re-apply is skipped
    // while no race is chosen (raceFormID == 0, nothing to look up).
    if (in.raceChanged || r.genderApplied) {
        if (in.raceChanged) {
            player.raceFormID = in.raceFormID;
        }
        if (npcMgr && player.raceFormID) {
            npcMgr->applyRaceToPlayer(player, player.raceFormID);
            r.raceApplied = true;
        }
    }
    if (!in.name.empty()) {
        player.name = in.name;
        r.nameApplied = true;
    }
    // #161 device evidence: log the race id plus HP/attributes here to show what
    // the commit actually applied. Health is not wired yet (V-664-3), so hp is
    // unchanged; str/end now reflect the race ATTR (V-664-2).
    LOGI("chargen commit applied: race=0x%08X hp=%.0f/%.0f str=%d end=%d gender=%u",
         player.raceFormID, player.health, player.maxHealth,
         player.attributes.Strength, player.attributes.Endurance,
         static_cast<unsigned>(player.gender));
    return r;
}

}  // namespace chargen
