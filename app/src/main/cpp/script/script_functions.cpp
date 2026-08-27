#include "script_functions.h"
#include <cmath>
#include <cstdlib>
#include <android/log.h>

// Forward declarations for game systems
#include "../game/quest_manager.h"
#include "../world/world_manager.h"
#include "../game/npc_manager.h"
#include "../game/inventory_manager.h"

#define SF_LOG_TAG "ScriptFunctions"
#ifdef ENABLE_DEBUG_LOGS
#define SF_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, SF_LOG_TAG, __VA_ARGS__)
#else
#define SF_LOGD(...) do {} while(0)
#endif
#define SF_LOGI(...) __android_log_print(ANDROID_LOG_INFO, SF_LOG_TAG, __VA_ARGS__)
#define SF_LOGW(...) __android_log_print(ANDROID_LOG_WARN, SF_LOG_TAG, __VA_ARGS__)
#define SF_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, SF_LOG_TAG, __VA_ARGS__)

// ============================================================================
// Oblivion Script VM - Game Function API Implementation
// ============================================================================

namespace oblivion {
namespace script {

ScriptFunctions::ScriptFunctions() {
    registerTier1Functions();
    registerTier2Functions();
}

void ScriptFunctions::init(
    QuestManager* questMgr,
    WorldManager* worldMgr,
    NpcManager* npcMgr,
    InventoryManager* invMgr
) {
    questManager_ = questMgr;
    worldManager_ = worldMgr;
    npcManager_ = npcMgr;
    inventoryManager_ = invMgr;
}

FunctionResult ScriptFunctions::execute(
    FunctionID funcID,
    ExecutionContext& ctx,
    const std::vector<ScriptValue>& args
) {
    uint16_t id = static_cast<uint16_t>(funcID);
    auto it = handlers_.find(id);
    if (it != handlers_.end()) {
        return it->second(ctx, args);
    }

    FunctionResult result;
    result.success = false;
    result.errorMessage = "Unimplemented function: 0x" + std::to_string(id);
    SF_LOGW("%s", result.errorMessage.c_str());
    return result;
}

bool ScriptFunctions::hasFunction(FunctionID funcID) const {
    return handlers_.find(static_cast<uint16_t>(funcID)) != handlers_.end();
}

const char* ScriptFunctions::getFunctionName(FunctionID funcID) const {
    switch (funcID) {
        case FunctionID::SetStage: return "SetStage";
        case FunctionID::GetStage: return "GetStage";
        case FunctionID::AddItem: return "AddItem";
        case FunctionID::RemoveItem: return "RemoveItem";
        case FunctionID::GetItemCount: return "GetItemCount";
        case FunctionID::Enable: return "Enable";
        case FunctionID::Disable: return "Disable";
        case FunctionID::Activate: return "Activate";
        case FunctionID::GetDistance: return "GetDistance";
        case FunctionID::SetPos: return "SetPos";
        case FunctionID::GetPos: return "GetPos";
        case FunctionID::Message: return "Message";
        case FunctionID::MessageBox: return "MessageBox";
        case FunctionID::GetSelf: return "GetSelf";
        case FunctionID::GetPlayer: return "GetPlayer";
        case FunctionID::Set: return "Set";
        case FunctionID::Get: return "Get";
        case FunctionID::Random: return "Random";
        case FunctionID::Resurrect: return "Resurrect";
        case FunctionID::PlaceAtMe: return "PlaceAtMe";
        case FunctionID::MoveTo: return "MoveTo";
        case FunctionID::Lock: return "Lock";
        case FunctionID::Unlock: return "Unlock";
        case FunctionID::GetLockLevel: return "GetLockLevel";
        case FunctionID::IsLocked: return "IsLocked";
        case FunctionID::GetHealth: return "GetHealth";
        case FunctionID::SetHealth: return "SetHealth";
        case FunctionID::GetLevel: return "GetLevel";
        case FunctionID::SetLevel: return "SetLevel";
        case FunctionID::IsDead: return "IsDead";
        case FunctionID::IsInCombat: return "IsInCombat";
        case FunctionID::GetCombatTarget: return "GetCombatTarget";
        case FunctionID::StartCombat: return "StartCombat";
        case FunctionID::StopCombat: return "StopCombat";
        case FunctionID::GetAV: return "GetAV";
        case FunctionID::SetAV: return "SetAV";
        case FunctionID::ModAV: return "ModAV";
        case FunctionID::GetBaseAV: return "GetBaseAV";
        case FunctionID::AddSpell: return "AddSpell";
        case FunctionID::RemoveSpell: return "RemoveSpell";
        case FunctionID::HasSpell: return "HasSpell";
        case FunctionID::Cast: return "Cast";
        case FunctionID::IsGhost: return "IsGhost";
        case FunctionID::SetGhost: return "SetGhost";
        case FunctionID::IsEssential: return "IsEssential";
        case FunctionID::SetEssential: return "SetEssential";
        case FunctionID::IsPlayer: return "IsPlayer";
        case FunctionID::GetSex: return "GetSex";
        case FunctionID::GetRace: return "GetRace";
        case FunctionID::GetClass: return "GetClass";
        case FunctionID::GetFactionRank: return "GetFactionRank";
        case FunctionID::SetFactionRank: return "SetFactionRank";
        case FunctionID::IsInFaction: return "IsInFaction";
        case FunctionID::GetParentCell: return "GetParentCell";
        case FunctionID::IsInInterior: return "IsInInterior";
        case FunctionID::GetOpenState: return "GetOpenState";
        case FunctionID::IsOpen: return "IsOpen";
        case FunctionID::GetValue: return "GetValue";
        case FunctionID::GetWeight: return "GetWeight";
        case FunctionID::GetOwner: return "GetOwner";
        case FunctionID::SetOwner: return "SetOwner";
        case FunctionID::Say: return "Say";
        case FunctionID::SayTo: return "SayTo";
        case FunctionID::Show: return "Show";
        case FunctionID::Wait: return "Wait";
        case FunctionID::PlayGroup: return "PlayGroup";
        case FunctionID::LoopGroup: return "LoopGroup";
        case FunctionID::SetUnconscious: return "SetUnconscious";
        case FunctionID::IsUnconscious: return "IsUnconscious";
        case FunctionID::IsWeaponOut: return "IsWeaponOut";
        case FunctionID::IsSneaking: return "IsSneaking";
        case FunctionID::IsAlerted: return "IsAlerted";
        case FunctionID::IsAnimal: return "IsAnimal";
        case FunctionID::IsCreature: return "IsCreature";
        case FunctionID::IsHumanoid: return "IsHumanoid";
        case FunctionID::GetPCLevel: return "GetPCLevel";
        case FunctionID::GetPCCell: return "GetPCCell";
        case FunctionID::GetCurrentTime: return "GetCurrentTime";
        case FunctionID::GetCurrentDay: return "GetCurrentDay";
        case FunctionID::GetCurrentMonth: return "GetCurrentMonth";
        case FunctionID::GetCurrentYear: return "GetCurrentYear";
        case FunctionID::GetWeather: return "GetWeather";
        case FunctionID::IsRaining: return "IsRaining";
        case FunctionID::IsSnowing: return "IsSnowing";
        case FunctionID::GetFame: return "GetFame";
        case FunctionID::GetInfamy: return "GetInfamy";
        case FunctionID::GetPCFame: return "GetPCFame";
        case FunctionID::GetPCInfamy: return "GetPCInfamy";
        case FunctionID::GetRegion: return "GetRegion";
        case FunctionID::GetRelationship: return "GetRelationship";
        case FunctionID::SetRelationship: return "SetRelationship";
        case FunctionID::GetWeapon: return "GetWeapon";
        case FunctionID::GetArmor: return "GetArmor";
        case FunctionID::IsDetected: return "IsDetected";
        case FunctionID::GetDetectionLevel: return "GetDetectionLevel";
        case FunctionID::IsInjured: return "IsInjured";
        case FunctionID::IsActivated: return "IsActivated";
        case FunctionID::GetActivator: return "GetActivator";
        case FunctionID::IsCellOwner: return "IsCellOwner";
        case FunctionID::GetStartLocation: return "GetStartLocation";
        case FunctionID::GetEnteringLocation: return "GetEnteringLocation";
        case FunctionID::IsInMyCell: return "IsInMyCell";
        case FunctionID::GetInCell: return "GetInCell";
        case FunctionID::IsInExterior: return "IsInExterior";
        case FunctionID::GetDaysInMonth: return "GetDaysInMonth";
        case FunctionID::GetSpell: return "GetSpell";
        case FunctionID::GetSpellCount: return "GetSpellCount";
        case FunctionID::GetNthSpell: return "GetNthSpell";
        case FunctionID::IsImmobile: return "IsImmobile";
        case FunctionID::SetImmobile: return "SetImmobile";
        case FunctionID::IsInvulnerable: return "IsInvulnerable";
        case FunctionID::SetInvulnerable: return "SetInvulnerable";
        case FunctionID::IsPlayerTeammate: return "IsPlayerTeammate";
        case FunctionID::GetPlayerSkill: return "GetPlayerSkill";
        case FunctionID::GetPCCount: return "GetPCCount";
        case FunctionID::IsPCAmount: return "IsPCAmount";
        case FunctionID::GetPCLocation: return "GetPCLocation";
        case FunctionID::IsPCLocation: return "IsPCLocation";
        default: return "Unknown";
    }
}

// ============================================================================
// Registration
// ============================================================================

void ScriptFunctions::registerTier1Functions() {
    using namespace std::placeholders;

    handlers_[static_cast<uint16_t>(FunctionID::SetStage)] =
        std::bind(&ScriptFunctions::fnSetStage, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetStage)] =
        std::bind(&ScriptFunctions::fnGetStage, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::AddItem)] =
        std::bind(&ScriptFunctions::fnAddItem, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::RemoveItem)] =
        std::bind(&ScriptFunctions::fnRemoveItem, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetItemCount)] =
        std::bind(&ScriptFunctions::fnGetItemCount, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Enable)] =
        std::bind(&ScriptFunctions::fnEnable, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Disable)] =
        std::bind(&ScriptFunctions::fnDisable, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Activate)] =
        std::bind(&ScriptFunctions::fnActivate, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetDistance)] =
        std::bind(&ScriptFunctions::fnGetDistance, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::SetPos)] =
        std::bind(&ScriptFunctions::fnSetPos, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetPos)] =
        std::bind(&ScriptFunctions::fnGetPos, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Message)] =
        std::bind(&ScriptFunctions::fnMessage, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::MessageBox)] =
        std::bind(&ScriptFunctions::fnMessageBox, this, _1, _2);
}

void ScriptFunctions::registerTier2Functions() {
    using namespace std::placeholders;

    handlers_[static_cast<uint16_t>(FunctionID::GetSelf)] =
        std::bind(&ScriptFunctions::fnGetSelf, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetPlayer)] =
        std::bind(&ScriptFunctions::fnGetPlayer, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Set)] =
        std::bind(&ScriptFunctions::fnSet, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Get)] =
        std::bind(&ScriptFunctions::fnGet, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Random)] =
        std::bind(&ScriptFunctions::fnRandom, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Resurrect)] =
        std::bind(&ScriptFunctions::fnResurrect, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::PlaceAtMe)] =
        std::bind(&ScriptFunctions::fnPlaceAtMe, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::MoveTo)] =
        std::bind(&ScriptFunctions::fnMoveTo, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Lock)] =
        std::bind(&ScriptFunctions::fnLock, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Unlock)] =
        std::bind(&ScriptFunctions::fnUnlock, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetLockLevel)] =
        std::bind(&ScriptFunctions::fnGetLockLevel, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsLocked)] =
        std::bind(&ScriptFunctions::fnIsLocked, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetHealth)] =
        std::bind(&ScriptFunctions::fnGetHealth, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::SetHealth)] =
        std::bind(&ScriptFunctions::fnSetHealth, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetLevel)] =
        std::bind(&ScriptFunctions::fnGetLevel, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::SetLevel)] =
        std::bind(&ScriptFunctions::fnSetLevel, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsDead)] =
        std::bind(&ScriptFunctions::fnIsDead, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsInCombat)] =
        std::bind(&ScriptFunctions::fnIsInCombat, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetCombatTarget)] =
        std::bind(&ScriptFunctions::fnGetCombatTarget, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::StartCombat)] =
        std::bind(&ScriptFunctions::fnStartCombat, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::StopCombat)] =
        std::bind(&ScriptFunctions::fnStopCombat, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetAV)] =
        std::bind(&ScriptFunctions::fnGetAV, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::SetAV)] =
        std::bind(&ScriptFunctions::fnSetAV, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::ModAV)] =
        std::bind(&ScriptFunctions::fnModAV, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetBaseAV)] =
        std::bind(&ScriptFunctions::fnGetBaseAV, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::AddSpell)] =
        std::bind(&ScriptFunctions::fnAddSpell, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::RemoveSpell)] =
        std::bind(&ScriptFunctions::fnRemoveSpell, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::HasSpell)] =
        std::bind(&ScriptFunctions::fnHasSpell, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Cast)] =
        std::bind(&ScriptFunctions::fnCast, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsGhost)] =
        std::bind(&ScriptFunctions::fnIsGhost, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::SetGhost)] =
        std::bind(&ScriptFunctions::fnSetGhost, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsEssential)] =
        std::bind(&ScriptFunctions::fnIsEssential, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::SetEssential)] =
        std::bind(&ScriptFunctions::fnSetEssential, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsPlayer)] =
        std::bind(&ScriptFunctions::fnIsPlayer, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetSex)] =
        std::bind(&ScriptFunctions::fnGetSex, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetRace)] =
        std::bind(&ScriptFunctions::fnGetRace, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetClass)] =
        std::bind(&ScriptFunctions::fnGetClass, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetFactionRank)] =
        std::bind(&ScriptFunctions::fnGetFactionRank, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::SetFactionRank)] =
        std::bind(&ScriptFunctions::fnSetFactionRank, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsInFaction)] =
        std::bind(&ScriptFunctions::fnIsInFaction, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetParentCell)] =
        std::bind(&ScriptFunctions::fnGetParentCell, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsInInterior)] =
        std::bind(&ScriptFunctions::fnIsInInterior, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetOpenState)] =
        std::bind(&ScriptFunctions::fnGetOpenState, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsOpen)] =
        std::bind(&ScriptFunctions::fnIsOpen, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetValue)] =
        std::bind(&ScriptFunctions::fnGetValue, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetWeight)] =
        std::bind(&ScriptFunctions::fnGetWeight, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetOwner)] =
        std::bind(&ScriptFunctions::fnGetOwner, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::SetOwner)] =
        std::bind(&ScriptFunctions::fnSetOwner, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Say)] =
        std::bind(&ScriptFunctions::fnSay, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::SayTo)] =
        std::bind(&ScriptFunctions::fnSayTo, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Show)] =
        std::bind(&ScriptFunctions::fnShow, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::Wait)] =
        std::bind(&ScriptFunctions::fnWait, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::PlayGroup)] =
        std::bind(&ScriptFunctions::fnPlayGroup, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::LoopGroup)] =
        std::bind(&ScriptFunctions::fnLoopGroup, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::SetUnconscious)] =
        std::bind(&ScriptFunctions::fnSetUnconscious, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsUnconscious)] =
        std::bind(&ScriptFunctions::fnIsUnconscious, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsWeaponOut)] =
        std::bind(&ScriptFunctions::fnIsWeaponOut, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsSneaking)] =
        std::bind(&ScriptFunctions::fnIsSneaking, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsAlerted)] =
        std::bind(&ScriptFunctions::fnIsAlerted, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsAnimal)] =
        std::bind(&ScriptFunctions::fnIsAnimal, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsCreature)] =
        std::bind(&ScriptFunctions::fnIsCreature, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsHumanoid)] =
        std::bind(&ScriptFunctions::fnIsHumanoid, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetPCLevel)] =
        std::bind(&ScriptFunctions::fnGetPCLevel, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetPCCell)] =
        std::bind(&ScriptFunctions::fnGetPCCell, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetCurrentTime)] =
        std::bind(&ScriptFunctions::fnGetCurrentTime, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetCurrentDay)] =
        std::bind(&ScriptFunctions::fnGetCurrentDay, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetCurrentMonth)] =
        std::bind(&ScriptFunctions::fnGetCurrentMonth, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetCurrentYear)] =
        std::bind(&ScriptFunctions::fnGetCurrentYear, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetWeather)] =
        std::bind(&ScriptFunctions::fnGetWeather, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsRaining)] =
        std::bind(&ScriptFunctions::fnIsRaining, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsSnowing)] =
        std::bind(&ScriptFunctions::fnIsSnowing, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetFame)] =
        std::bind(&ScriptFunctions::fnGetFame, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetInfamy)] =
        std::bind(&ScriptFunctions::fnGetInfamy, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetPCFame)] =
        std::bind(&ScriptFunctions::fnGetPCFame, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetPCInfamy)] =
        std::bind(&ScriptFunctions::fnGetPCInfamy, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetRegion)] =
        std::bind(&ScriptFunctions::fnGetRegion, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetRelationship)] =
        std::bind(&ScriptFunctions::fnGetRelationship, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::SetRelationship)] =
        std::bind(&ScriptFunctions::fnSetRelationship, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetWeapon)] =
        std::bind(&ScriptFunctions::fnGetWeapon, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetArmor)] =
        std::bind(&ScriptFunctions::fnGetArmor, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsDetected)] =
        std::bind(&ScriptFunctions::fnIsDetected, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetDetectionLevel)] =
        std::bind(&ScriptFunctions::fnGetDetectionLevel, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsInjured)] =
        std::bind(&ScriptFunctions::fnIsInjured, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsActivated)] =
        std::bind(&ScriptFunctions::fnIsActivated, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetActivator)] =
        std::bind(&ScriptFunctions::fnGetActivator, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsCellOwner)] =
        std::bind(&ScriptFunctions::fnIsCellOwner, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetStartLocation)] =
        std::bind(&ScriptFunctions::fnGetStartLocation, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetEnteringLocation)] =
        std::bind(&ScriptFunctions::fnGetEnteringLocation, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsInMyCell)] =
        std::bind(&ScriptFunctions::fnIsInMyCell, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetInCell)] =
        std::bind(&ScriptFunctions::fnGetInCell, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsInExterior)] =
        std::bind(&ScriptFunctions::fnIsInExterior, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetDaysInMonth)] =
        std::bind(&ScriptFunctions::fnGetDaysInMonth, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetSpell)] =
        std::bind(&ScriptFunctions::fnGetSpell, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetSpellCount)] =
        std::bind(&ScriptFunctions::fnGetSpellCount, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetNthSpell)] =
        std::bind(&ScriptFunctions::fnGetNthSpell, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsImmobile)] =
        std::bind(&ScriptFunctions::fnIsImmobile, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::SetImmobile)] =
        std::bind(&ScriptFunctions::fnSetImmobile, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsInvulnerable)] =
        std::bind(&ScriptFunctions::fnIsInvulnerable, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::SetInvulnerable)] =
        std::bind(&ScriptFunctions::fnSetInvulnerable, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsPlayerTeammate)] =
        std::bind(&ScriptFunctions::fnIsPlayerTeammate, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetPlayerSkill)] =
        std::bind(&ScriptFunctions::fnGetPlayerSkill, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetPCCount)] =
        std::bind(&ScriptFunctions::fnGetPCCount, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsPCAmount)] =
        std::bind(&ScriptFunctions::fnIsPCAmount, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::GetPCLocation)] =
        std::bind(&ScriptFunctions::fnGetPCLocation, this, _1, _2);
    handlers_[static_cast<uint16_t>(FunctionID::IsPCLocation)] =
        std::bind(&ScriptFunctions::fnIsPCLocation, this, _1, _2);
}

// ============================================================================
// Tier 1 Function Implementations
// ============================================================================

FunctionResult ScriptFunctions::fnSetStage(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 2) {
        result.errorMessage = "SetStage requires 2 arguments (questFormID, stage)";
        return result;
    }
    uint32_t questFormID = static_cast<uint32_t>(args[0].toInt());
    int stage = args[1].toInt();

    SF_LOGD("SetStage(0x%08X, %d)", questFormID, stage);

    // TODO: Integrate with QuestManager when quest stage tracking is implemented
    // if (questManager_) {
    //     questManager_->setQuestStage(questFormID, stage);
    // }

    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnGetStage(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 1) {
        result.errorMessage = "GetStage requires 1 argument (questFormID)";
        return result;
    }
    uint32_t questFormID = static_cast<uint32_t>(args[0].toInt());

    SF_LOGD("GetStage(0x%08X)", questFormID);

    // TODO: Integrate with QuestManager
    // if (questManager_) {
    //     result.returnValue = ScriptValue::makeInt(questManager_->getQuestStage(questFormID));
    // }

    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnAddItem(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 2) {
        result.errorMessage = "AddItem requires 2 arguments (itemFormID, count)";
        return result;
    }
    uint32_t itemFormID = static_cast<uint32_t>(args[0].toInt());
    int count = args[1].toInt();

    SF_LOGD("AddItem(0x%08X, %d) on self=0x%08X", itemFormID, count, ctx.getSelfRef());

    // TODO: Integrate with InventoryManager
    // if (inventoryManager_) {
    //     inventoryManager_->addItem(ctx.getSelfRef(), itemFormID, count);
    // }

    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnRemoveItem(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 2) {
        result.errorMessage = "RemoveItem requires 2 arguments (itemFormID, count)";
        return result;
    }
    uint32_t itemFormID = static_cast<uint32_t>(args[0].toInt());
    int count = args[1].toInt();

    SF_LOGD("RemoveItem(0x%08X, %d) on self=0x%08X", itemFormID, count, ctx.getSelfRef());

    // TODO: Integrate with InventoryManager

    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnGetItemCount(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 1) {
        result.errorMessage = "GetItemCount requires 1 argument (itemFormID)";
        return result;
    }
    uint32_t itemFormID = static_cast<uint32_t>(args[0].toInt());

    SF_LOGD("GetItemCount(0x%08X) on self=0x%08X", itemFormID, ctx.getSelfRef());

    // TODO: Integrate with InventoryManager

    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnEnable(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    SF_LOGD("Enable() on self=0x%08X", ctx.getSelfRef());

    // TODO: Integrate with WorldManager to enable the object

    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnDisable(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    SF_LOGD("Disable() on self=0x%08X", ctx.getSelfRef());

    // TODO: Integrate with WorldManager to disable the object

    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnActivate(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    SF_LOGD("Activate() on self=0x%08X", ctx.getSelfRef());

    // TODO: Integrate with interaction system

    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnGetDistance(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 1) {
        result.errorMessage = "GetDistance requires 1 argument (refFormID)";
        return result;
    }
    uint32_t refFormID = static_cast<uint32_t>(args[0].toInt());

    SF_LOGD("GetDistance(0x%08X) from self=0x%08X", refFormID, ctx.getSelfRef());

    // TODO: Calculate actual distance between objects

    result.success = true;
    result.returnValue = ScriptValue::makeFloat(0.0f);
    return result;
}

FunctionResult ScriptFunctions::fnSetPos(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 2) {
        result.errorMessage = "SetPos requires 2 arguments (axis, value)";
        return result;
    }
    int axis = args[0].toInt();  // 0=X, 1=Y, 2=Z
    float value = args[1].toFloat();

    SF_LOGD("SetPos(%d, %.2f) on self=0x%08X", axis, value, ctx.getSelfRef());

    // TODO: Integrate with WorldManager to set object position

    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnGetPos(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 1) {
        result.errorMessage = "GetPos requires 1 argument (axis)";
        return result;
    }
    int axis = args[0].toInt();  // 0=X, 1=Y, 2=Z

    SF_LOGD("GetPos(%d) on self=0x%08X", axis, ctx.getSelfRef());

    // TODO: Get actual position from WorldManager

    result.success = true;
    result.returnValue = ScriptValue::makeFloat(0.0f);
    return result;
}

FunctionResult ScriptFunctions::fnMessage(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 1) {
        result.errorMessage = "Message requires 1 argument (text)";
        return result;
    }
    std::string text;
    if (args[0].type == ScriptValue::Type::String) {
        text = args[0].strVal;
    } else {
        text = std::to_string(args[0].toInt());
    }

    SF_LOGI("Message: %s", text.c_str());

    // TODO: Display message in UI (toast or HUD message)

    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnMessageBox(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 1) {
        result.errorMessage = "MessageBox requires at least 1 argument (text)";
        return result;
    }
    std::string text;
    if (args[0].type == ScriptValue::Type::String) {
        text = args[0].strVal;
    } else {
        text = std::to_string(args[0].toInt());
    }

    SF_LOGI("MessageBox: %s", text.c_str());

    // TODO: Show message box UI

    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

// ============================================================================
// Tier 2 Function Implementations (stubs - return sensible defaults)
// ============================================================================

FunctionResult ScriptFunctions::fnGetSelf(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(ctx.getSelfRef());
    return result;
}

FunctionResult ScriptFunctions::fnGetPlayer(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    // Player FormID is typically 0x00000014 in Oblivion
    result.returnValue = ScriptValue::makeRef(0x00000014);
    return result;
}

FunctionResult ScriptFunctions::fnSet(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 2) {
        result.errorMessage = "Set requires 2 arguments (variable, value)";
        return result;
    }
    // TODO: Set script variable by name/FormID
    result.success = true;
    result.returnValue = args[1];
    return result;
}

FunctionResult ScriptFunctions::fnGet(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 1) {
        result.errorMessage = "Get requires 1 argument (variable)";
        return result;
    }
    // TODO: Get script variable by name/FormID
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnRandom(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 1) {
        result.errorMessage = "Random requires 1 argument (max)";
        return result;
    }
    float maxVal = args[0].toFloat();
    float randomVal = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * maxVal;
    result.success = true;
    result.returnValue = ScriptValue::makeFloat(randomVal);
    return result;
}

FunctionResult ScriptFunctions::fnResurrect(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    SF_LOGD("Resurrect() on self=0x%08X", ctx.getSelfRef());
    // TODO: Integrate with NPC/creature system
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnPlaceAtMe(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 1) {
        result.errorMessage = "PlaceAtMe requires at least 1 argument (formID)";
        return result;
    }
    uint32_t formID = static_cast<uint32_t>(args[0].toInt());
    SF_LOGD("PlaceAtMe(0x%08X) at self=0x%08X", formID, ctx.getSelfRef());
    // TODO: Spawn object at current object's location
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);  // Return spawned object ref
    return result;
}

FunctionResult ScriptFunctions::fnMoveTo(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    if (args.size() < 1) {
        result.errorMessage = "MoveTo requires 1 argument (refFormID)";
        return result;
    }
    uint32_t refFormID = static_cast<uint32_t>(args[0].toInt());
    SF_LOGD("MoveTo(0x%08X) for self=0x%08X", refFormID, ctx.getSelfRef());
    // TODO: Teleport object to target location
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnLock(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    int level = args.empty() ? 1 : args[0].toInt();
    SF_LOGD("Lock(%d) on self=0x%08X", level, ctx.getSelfRef());
    // TODO: Lock the object
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnUnlock(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    SF_LOGD("Unlock() on self=0x%08X", ctx.getSelfRef());
    // TODO: Unlock the object
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnGetLockLevel(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsLocked(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetHealth(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeFloat(100.0f);
    return result;
}

FunctionResult ScriptFunctions::fnSetHealth(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnGetLevel(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnSetLevel(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnIsDead(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsInCombat(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetCombatTarget(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnStartCombat(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnStopCombat(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnGetAV(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeFloat(0.0f);
    return result;
}

FunctionResult ScriptFunctions::fnSetAV(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnModAV(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnGetBaseAV(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeFloat(0.0f);
    return result;
}

FunctionResult ScriptFunctions::fnAddSpell(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnRemoveSpell(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnHasSpell(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnCast(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnIsGhost(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnSetGhost(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnIsEssential(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnSetEssential(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnIsPlayer(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(ctx.getSelfRef() == 0x00000014 ? 1 : 0);
    return result;
}

FunctionResult ScriptFunctions::fnGetSex(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);  // 0=male, 1=female
    return result;
}

FunctionResult ScriptFunctions::fnGetRace(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetClass(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetFactionRank(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(-1);
    return result;
}

FunctionResult ScriptFunctions::fnSetFactionRank(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnIsInFaction(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetParentCell(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsInInterior(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetOpenState(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);  // 0=closed
    return result;
}

FunctionResult ScriptFunctions::fnIsOpen(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetValue(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetWeight(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeFloat(0.0f);
    return result;
}

FunctionResult ScriptFunctions::fnGetOwner(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnSetOwner(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnSay(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnSayTo(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnShow(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnWait(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnPlayGroup(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnLoopGroup(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnSetUnconscious(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnIsUnconscious(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsWeaponOut(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsSneaking(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsAlerted(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsAnimal(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsCreature(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsHumanoid(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnGetPCLevel(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnGetPCCell(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetCurrentTime(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeFloat(12.0f);  // Noon
    return result;
}

FunctionResult ScriptFunctions::fnGetCurrentDay(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnGetCurrentMonth(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);  // Morning Star
    return result;
}

FunctionResult ScriptFunctions::fnGetCurrentYear(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(3);  // 3E 433
    return result;
}

FunctionResult ScriptFunctions::fnGetWeather(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsRaining(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsSnowing(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetFame(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetInfamy(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetPCFame(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetPCInfamy(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetRegion(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetRelationship(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnSetRelationship(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnGetWeapon(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetArmor(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsDetected(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetDetectionLevel(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsInjured(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsActivated(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetActivator(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsCellOwner(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetStartLocation(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetEnteringLocation(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsInMyCell(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetInCell(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsInExterior(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnGetDaysInMonth(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(30);
    return result;
}

FunctionResult ScriptFunctions::fnGetSpell(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetSpellCount(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetNthSpell(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsImmobile(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnSetImmobile(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnIsInvulnerable(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnSetInvulnerable(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnIsPlayerTeammate(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetPlayerSkill(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeFloat(0.0f);
    return result;
}

FunctionResult ScriptFunctions::fnGetPCCount(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(1);
    return result;
}

FunctionResult ScriptFunctions::fnIsPCAmount(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

FunctionResult ScriptFunctions::fnGetPCLocation(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeRef(0);
    return result;
}

FunctionResult ScriptFunctions::fnIsPCLocation(ExecutionContext& ctx, const std::vector<ScriptValue>& args) {
    FunctionResult result;
    result.success = true;
    result.returnValue = ScriptValue::makeInt(0);
    return result;
}

} // namespace script
} // namespace oblivion
