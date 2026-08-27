#pragma once

#include "script_opcodes.h"
#include "script_context.h"
#include <functional>
#include <unordered_map>
#include <string>
#include <vector>

// ============================================================================
// Oblivion Script VM - Game Function API
// Native functions callable from script bytecode via CALL opcode
// ============================================================================

// Forward declarations
class QuestManager;
class WorldManager;
class NpcManager;
class InventoryManager;

namespace oblivion {
namespace script {

// ============================================================================
// Function result
// ============================================================================
struct FunctionResult {
    bool success = false;
    ScriptValue returnValue;
    std::string errorMessage;
};

// ============================================================================
// Function handler signature
// ============================================================================
using FunctionHandler = std::function<FunctionResult(
    ExecutionContext& ctx,
    const std::vector<ScriptValue>& args
)>;

// ============================================================================
// ScriptFunctions - registry of native game functions
// ============================================================================
class ScriptFunctions {
public:
    ScriptFunctions();
    ~ScriptFunctions() = default;

    // Initialize with game system pointers
    void init(
        QuestManager* questMgr,
        WorldManager* worldMgr,
        NpcManager* npcMgr,
        InventoryManager* invMgr
    );

    // Execute a function by ID
    FunctionResult execute(
        FunctionID funcID,
        ExecutionContext& ctx,
        const std::vector<ScriptValue>& args
    );

    // Check if a function is registered
    bool hasFunction(FunctionID funcID) const;

    // Get function name (for debugging)
    const char* getFunctionName(FunctionID funcID) const;

private:
    // Game system pointers
    QuestManager* questManager_ = nullptr;
    WorldManager* worldManager_ = nullptr;
    NpcManager* npcManager_ = nullptr;
    InventoryManager* inventoryManager_ = nullptr;

    // Function registry
    std::unordered_map<uint16_t, FunctionHandler> handlers_;

    // Registration helpers
    void registerTier1Functions();
    void registerTier2Functions();

    // --- Tier 1 function implementations ---
    FunctionResult fnSetStage(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetStage(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnAddItem(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnRemoveItem(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetItemCount(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnEnable(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnDisable(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnActivate(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetDistance(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSetPos(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetPos(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnMessage(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnMessageBox(ExecutionContext& ctx, const std::vector<ScriptValue>& args);

    // --- Tier 2 function implementations ---
    FunctionResult fnGetSelf(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetPlayer(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSet(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGet(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnRandom(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnResurrect(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnPlaceAtMe(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnMoveTo(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnLock(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnUnlock(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetLockLevel(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsLocked(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetHealth(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSetHealth(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetLevel(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSetLevel(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsDead(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsInCombat(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetCombatTarget(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnStartCombat(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnStopCombat(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetAV(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSetAV(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnModAV(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetBaseAV(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnAddSpell(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnRemoveSpell(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnHasSpell(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnCast(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsGhost(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSetGhost(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsEssential(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSetEssential(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsPlayer(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetSex(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetRace(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetClass(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetFactionRank(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSetFactionRank(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsInFaction(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetParentCell(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsInInterior(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetOpenState(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsOpen(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetValue(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetWeight(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetOwner(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSetOwner(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSay(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSayTo(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnShow(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnWait(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnPlayGroup(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnLoopGroup(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSetUnconscious(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsUnconscious(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsWeaponOut(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsSneaking(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsAlerted(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsAnimal(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsCreature(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsHumanoid(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetPCLevel(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetPCCell(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetCurrentTime(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetCurrentDay(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetCurrentMonth(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetCurrentYear(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetWeather(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsRaining(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsSnowing(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetFame(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetInfamy(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetPCFame(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetPCInfamy(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetRegion(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetRelationship(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSetRelationship(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetWeapon(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetArmor(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsDetected(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetDetectionLevel(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsInjured(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsActivated(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetActivator(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsCellOwner(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetStartLocation(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetEnteringLocation(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsInMyCell(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetInCell(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsInExterior(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetDaysInMonth(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetSpell(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetSpellCount(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetNthSpell(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsImmobile(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSetImmobile(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsInvulnerable(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnSetInvulnerable(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsPlayerTeammate(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetPlayerSkill(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetPCCount(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsPCAmount(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnGetPCLocation(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
    FunctionResult fnIsPCLocation(ExecutionContext& ctx, const std::vector<ScriptValue>& args);
};

} // namespace script
} // namespace oblivion
