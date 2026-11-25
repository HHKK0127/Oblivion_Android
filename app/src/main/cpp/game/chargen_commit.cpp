// game/chargen_commit.cpp — see chargen_commit.h.
#include "chargen_commit.h"

#include "npc_manager.h"
#include "player.h"

namespace chargen {

CommitResult commit(NpcManager* npcMgr, Player& player, const CommitInput& in) {
    CommitResult r;
    // §10.2/§10.4: apply the race only when it changed (the original's 0x51C090
    // no-op guard), then the name.
    //
    // V-664: the race application reverts to the chargen base and applies the
    // race's skill bonuses and racial spells through NpcManager (Player is not
    // an NPC, so the NPC path initializePlayerFromESM cannot be used). Attribute
    // bonuses (ATTR) and health are not wired yet (V-664-2/V-664-3).
    if (in.raceChanged) {
        player.raceFormID = in.raceFormID;
        if (npcMgr) {
            npcMgr->applyRaceToPlayer(player, player.raceFormID);
            r.raceApplied = true;
        }
    }
    if (!in.name.empty()) {
        player.name = in.name;
        r.nameApplied = true;
    }
    // #161 device evidence: applyRaceToPlayer() only reports the skill path, so
    // log the race id plus HP/attributes here to show what the commit actually
    // applied. Attribute bonuses and health are not wired yet (V-664-2/V-664-3),
    // so str/end stay at the chargen base and hp is unchanged.
    LOGI("chargen commit applied: race=0x%08X hp=%.0f/%.0f str=%d end=%d",
         player.raceFormID, player.health, player.maxHealth,
         player.attributes.Strength, player.attributes.Endurance);
    return r;
}

}  // namespace chargen
