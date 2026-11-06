// Script function tests for the Phase-68 FunctionID band (V75a canon).
// See script_fn_tests.h for the handler -> native address mapping.

#include "script_fn_tests.h"

#include "../script/script_functions.h"
#include "../game/npc.h"
#include "../game/npc_manager.h"
#include "../game/crime_reputation_system.h"
#include "../assets/esm_reader.h"
#include "../audio/audio_manager.h"

#include <glm/glm.hpp>

using namespace oblivion::script;
using namespace oblivion;
using ::game::CrimeReputationSystem;

namespace oblivion {
namespace script_fn {

namespace {

// PLAYER_FORM_ID is a file-local constant in script_functions.cpp; mirror it here.
constexpr uint32_t kPlayerFormId = 0x00000014u;

bool errorContains(const FunctionResult& r, const std::string& needle) {
    return r.errorMessage.find(needle) != std::string::npos;
}

} // namespace

ScriptFnTests::ScriptFnTests() = default;

void ScriptFnTests::record(const std::string& name, bool passed, const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int ScriptFnTests::getPassCount() const {
    int count = 0;
    for (const auto& r : results) {
        if (r.passed) ++count;
    }
    return count;
}

int ScriptFnTests::getFailCount() const {
    int count = 0;
    for (const auto& r : results) {
        if (!r.passed) ++count;
    }
    return count;
}

std::string ScriptFnTests::getSummary() const {
    return std::to_string(getPassCount()) + "/" + std::to_string(results.size()) + " passed";
}

// Evp with an unresolvable self reference must fail with the NPC-only message.
void ScriptFnTests::test_evp_unresolved_self_ref() {
    bool ok = false;
    std::string msg;
    {
        NpcManager npcManager;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        funcs.init(nullptr, nullptr, &npcManager, nullptr);

        FunctionResult r = funcs.execute(FunctionID::Evp, ctx, {});
        ok = !r.success && errorContains(r, "only available for NPC references");
        if (!ok) msg = "unresolved Evp: success=" + std::to_string(r.success) + " msg=" + r.errorMessage;
    }
    record("evp_unresolved_self_ref", ok, msg);
}

// Evp on a non-combat NPC resets the AI state to IDLE (native PackageSelect_Main dispatch).
void ScriptFnTests::test_evp_resets_non_combat() {
    bool ok = false;
    std::string msg;
    {
        NpcManager npcManager;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        funcs.init(nullptr, nullptr, &npcManager, nullptr);
        auto npc = npcManager.createNPC("Tier 5 Actor", glm::vec3(1.0f, 2.0f, 3.0f));
        bool built = npc != nullptr;
        if (built) {
            npc->aiState = AIState::WANDER;
            ctx.setSelfRef(npc->npcId);
            FunctionResult r = funcs.execute(FunctionID::Evp, ctx, {});
            ok = r.success && r.returnValue.toInt() == 1 && npc->aiState == AIState::IDLE;
            if (!ok) msg = "Evp WANDER: success=" + std::to_string(r.success) +
                           " rv=" + std::to_string(r.returnValue.toInt()) +
                           " aiState=" + std::to_string(static_cast<int>(npc->aiState));
        } else {
            msg = "createNPC failed";
        }
    }
    record("evp_resets_non_combat", ok, msg);
}

// Evp must leave a COMBAT NPC in COMBAT (combat package preserved).
void ScriptFnTests::test_evp_preserves_combat() {
    bool ok = false;
    std::string msg;
    {
        NpcManager npcManager;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        funcs.init(nullptr, nullptr, &npcManager, nullptr);
        auto npc = npcManager.createNPC("Tier 5 Actor", glm::vec3(1.0f, 2.0f, 3.0f));
        bool built = npc != nullptr;
        if (built) {
            npc->aiState = AIState::COMBAT;
            ctx.setSelfRef(npc->npcId);
            FunctionResult r = funcs.execute(FunctionID::Evp, ctx, {});
            ok = r.success && npc->aiState == AIState::COMBAT;
            if (!ok) msg = "Evp COMBAT: success=" + std::to_string(r.success) +
                           " aiState=" + std::to_string(static_cast<int>(npc->aiState));
        } else {
            msg = "createNPC failed";
        }
    }
    record("evp_preserves_combat", ok, msg);
}

// PlaySound argument / ESMManager / AudioManager guards (native 0x509490 path).
void ScriptFnTests::test_playsound_guards() {
    bool ok = false;
    std::string msg;
    {
        NpcManager npcManager;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        funcs.init(nullptr, nullptr, &npcManager, nullptr);

        // 1. Missing argument.
        FunctionResult r1 = funcs.execute(FunctionID::PlaySound, ctx, {});
        bool argGuard = !r1.success && errorContains(r1, "requires 1 argument");

        // 2. Null ESMManager.
        FunctionResult r2 = funcs.execute(FunctionID::PlaySound, ctx, {ScriptValue::makeInt(0x00080001)});
        bool esmGuard = !r2.success && errorContains(r2, "requires an initialized ESMManager");

        // 3. ESMManager present; AudioManager state decides the next guard.
        ESMManager esm;
        funcs.init(nullptr, nullptr, &npcManager, nullptr, nullptr, &esm);
        FunctionResult r3 = funcs.execute(FunctionID::PlaySound, ctx, {ScriptValue::makeInt(0x00080001)});
        bool audioGuard;
        if (g_audioManager == nullptr) {
            audioGuard = !r3.success && errorContains(r3, "requires an initialized AudioManager");
        } else {
            // FormID absent from the empty ESMManager must hit the resolve guard.
            audioGuard = !r3.success && errorContains(r3, "could not resolve sound");
        }

        ok = argGuard && esmGuard && audioGuard;
        if (!ok) msg = "PlaySound guards: arg=" + std::to_string(argGuard) +
                       " esm=" + std::to_string(esmGuard) +
                       " audio=" + std::to_string(audioGuard) +
                       " last msg=" + r3.errorMessage;
    }
    record("playsound_guards", ok, msg);
}

// ModCrimeGold / SetCrimeGold must reject empty argument lists.
void ScriptFnTests::test_crimegold_arg_guards() {
    bool ok = false;
    std::string msg;
    {
        NpcManager npcManager;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        funcs.init(nullptr, nullptr, &npcManager, nullptr);

        FunctionResult rMod = funcs.execute(FunctionID::ModCrimeGold, ctx, {});
        FunctionResult rSet = funcs.execute(FunctionID::SetCrimeGold, ctx, {});
        ok = !rMod.success && errorContains(rMod, "ModCrimeGold requires 1 argument") &&
             !rSet.success && errorContains(rSet, "SetCrimeGold requires 1 argument");
        if (!ok) msg = "mod=" + rMod.errorMessage + " set=" + rSet.errorMessage;
    }
    record("crimegold_arg_guards", ok, msg);
}

// SetCrimeGold sets the bounty and clamps negative values to zero.
void ScriptFnTests::test_setcrimegold_set_and_clamp() {
    bool ok = false;
    std::string msg;
    {
        CrimeReputationSystem::getInstance().clearBounty();
        NpcManager npcManager;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        funcs.init(nullptr, nullptr, &npcManager, nullptr);

        FunctionResult r1 = funcs.execute(FunctionID::SetCrimeGold, ctx, {ScriptValue::makeInt(120)});
        bool setOk = r1.success && r1.returnValue.toInt() == 120 &&
                     CrimeReputationSystem::getInstance().getBounty() == 120;

        FunctionResult r2 = funcs.execute(FunctionID::SetCrimeGold, ctx, {ScriptValue::makeInt(-10)});
        bool clampOk = r2.success && r2.returnValue.toInt() == 0 &&
                       CrimeReputationSystem::getInstance().getBounty() == 0;

        ok = setOk && clampOk;
        if (!ok) msg = "set=" + std::to_string(r1.returnValue.toInt()) +
                       " clamp=" + std::to_string(r2.returnValue.toInt());
        CrimeReputationSystem::getInstance().clearBounty();
    }
    record("setcrimegold_set_and_clamp", ok, msg);
}

// ModCrimeGold adds the delta to the current bounty.
void ScriptFnTests::test_modcrimegold_delta() {
    bool ok = false;
    std::string msg;
    {
        CrimeReputationSystem::getInstance().clearBounty();
        NpcManager npcManager;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        funcs.init(nullptr, nullptr, &npcManager, nullptr);

        funcs.execute(FunctionID::SetCrimeGold, ctx, {ScriptValue::makeInt(100)});
        FunctionResult rAdd = funcs.execute(FunctionID::ModCrimeGold, ctx, {ScriptValue::makeInt(50)});
        bool addOk = rAdd.success && rAdd.returnValue.toInt() == 150;

        FunctionResult rSub = funcs.execute(FunctionID::ModCrimeGold, ctx, {ScriptValue::makeInt(-30)});
        bool subOk = rSub.success && rSub.returnValue.toInt() == 120;

        ok = addOk && subOk;
        if (!ok) msg = "add=" + std::to_string(rAdd.returnValue.toInt()) +
                       " sub=" + std::to_string(rSub.returnValue.toInt());
        CrimeReputationSystem::getInstance().clearBounty();
    }
    record("modcrimegold_delta", ok, msg);
}

// KillActor reference guards: zero selfRef, player formID, unresolvable formID.
void ScriptFnTests::test_killactor_guards() {
    bool ok = false;
    std::string msg;
    {
        NpcManager npcManager;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        funcs.init(nullptr, nullptr, &npcManager, nullptr);

        FunctionResult r1 = funcs.execute(FunctionID::KillActor, ctx, {});
        bool zeroGuard = !r1.success && errorContains(r1, "KillActor requires a reference");

        ctx.setSelfRef(kPlayerFormId);
        FunctionResult r2 = funcs.execute(FunctionID::KillActor, ctx, {});
        bool playerGuard = !r2.success && errorContains(r2, "cannot kill the player");

        ctx.setSelfRef(0x00030001u);
        FunctionResult r3 = funcs.execute(FunctionID::KillActor, ctx, {});
        bool resolveGuard = !r3.success && errorContains(r3, "could not resolve actor reference");

        ok = zeroGuard && playerGuard && resolveGuard;
        if (!ok) msg = "zero=" + std::to_string(zeroGuard) +
                       " player=" + std::to_string(playerGuard) +
                       " resolve=" + std::to_string(resolveGuard);
    }
    record("killactor_guards", ok, msg);
}

// KillActor on a live NPC must apply lethal damage and set the dead state.
void ScriptFnTests::test_killactor_kills_npc() {
    bool ok = false;
    std::string msg;
    {
        NpcManager npcManager;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        funcs.init(nullptr, nullptr, &npcManager, nullptr);
        auto npc = npcManager.createNPC("Tier 5 Actor", glm::vec3(1.0f, 2.0f, 3.0f));
        bool built = npc != nullptr;
        if (built) {
            npc->status.initialize(100.0f, 50.0f, 1);
            ctx.setSelfRef(npc->npcId);
            FunctionResult r = funcs.execute(FunctionID::KillActor, ctx, {});
            ok = r.success && r.returnValue.toInt() == 1 && npc->isDeadState();
            if (!ok) msg = "kill: success=" + std::to_string(r.success) +
                           " dead=" + std::to_string(npc->isDeadState()) +
                           " msg=" + r.errorMessage;
        } else {
            msg = "createNPC failed";
        }
    }
    record("killactor_kills_npc", ok, msg);
}

// ResurrectActor reference guards: zero selfRef, player formID, unresolvable formID.
void ScriptFnTests::test_resurrectactor_guards() {
    bool ok = false;
    std::string msg;
    {
        NpcManager npcManager;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        funcs.init(nullptr, nullptr, &npcManager, nullptr);

        FunctionResult r1 = funcs.execute(FunctionID::ResurrectActor, ctx, {});
        bool zeroGuard = !r1.success && errorContains(r1, "ResurrectActor requires a reference");

        ctx.setSelfRef(kPlayerFormId);
        FunctionResult r2 = funcs.execute(FunctionID::ResurrectActor, ctx, {});
        bool playerGuard = !r2.success && errorContains(r2, "cannot resurrect the player");

        ctx.setSelfRef(0x00030001u);
        FunctionResult r3 = funcs.execute(FunctionID::ResurrectActor, ctx, {});
        bool resolveGuard = !r3.success && errorContains(r3, "could not resolve actor reference");

        ok = zeroGuard && playerGuard && resolveGuard;
        if (!ok) msg = "zero=" + std::to_string(zeroGuard) +
                       " player=" + std::to_string(playerGuard) +
                       " resolve=" + std::to_string(resolveGuard);
    }
    record("resurrectactor_guards", ok, msg);
}

// Kill -> Resurrect cycle: resurrect clears the dead state, restores health, resets AI.
void ScriptFnTests::test_kill_resurrect_cycle() {
    bool ok = false;
    std::string msg;
    {
        NpcManager npcManager;
        ScriptFunctions funcs;
        ExecutionContext ctx;
        funcs.init(nullptr, nullptr, &npcManager, nullptr);
        auto npc = npcManager.createNPC("Tier 5 Actor", glm::vec3(1.0f, 2.0f, 3.0f));
        bool built = npc != nullptr;
        if (built) {
            npc->status.initialize(100.0f, 50.0f, 1);
            ctx.setSelfRef(npc->npcId);

            FunctionResult kill = funcs.execute(FunctionID::KillActor, ctx, {});
            bool killed = kill.success && npc->isDeadState();

            FunctionResult res = funcs.execute(FunctionID::ResurrectActor, ctx, {});
            bool revived = res.success && res.returnValue.toInt() == 1 &&
                           !npc->isDeadState() &&
                           npc->status.currentHealth > 0.0f &&
                           npc->aiState == AIState::IDLE;

            ok = killed && revived;
            if (!ok) msg = "killed=" + std::to_string(killed) +
                           " revived=" + std::to_string(revived) +
                           " hp=" + std::to_string(npc->status.currentHealth) +
                           " msg=" + res.errorMessage;
        } else {
            msg = "createNPC failed";
        }
    }
    record("kill_resurrect_cycle", ok, msg);
}

bool ScriptFnTests::runAllTests() {
    results.clear();

    test_evp_unresolved_self_ref();
    test_evp_resets_non_combat();
    test_evp_preserves_combat();
    test_playsound_guards();
    test_crimegold_arg_guards();
    test_setcrimegold_set_and_clamp();
    test_modcrimegold_delta();
    test_killactor_guards();
    test_killactor_kills_npc();
    test_resurrectactor_guards();
    test_kill_resurrect_cycle();

    for (const auto& r : results) {
        if (!r.passed) return false;
    }
    return true;
}

} // namespace script_fn
} // namespace oblivion
