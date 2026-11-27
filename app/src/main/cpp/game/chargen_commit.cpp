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
    // V-631-1b: the Gender toggle. Write only when it changed and log the change
    // (the device pass condition is a log line showing gender 0 -> 1 -> 0).
    if (in.gender != player.gender) {
        LOGI("chargen commit: gender %u -> %u", static_cast<unsigned>(player.gender),
             static_cast<unsigned>(in.gender));
        player.gender = in.gender;
        r.genderApplied = true;
    }
    // #161 device evidence: applyRaceToPlayer() only reports the skill path, so
    // log the race id plus HP/attributes here to show what the commit actually
    // applied. Attribute bonuses and health are not wired yet (V-664-2/V-664-3),
    // so str/end stay at the chargen base and hp is unchanged.
    LOGI("chargen commit applied: race=0x%08X hp=%.0f/%.0f str=%d end=%d gender=%u",
         player.raceFormID, player.health, player.maxHealth,
         player.attributes.Strength, player.attributes.Endurance,
         static_cast<unsigned>(player.gender));
    return r;
}

}  // namespace chargen
