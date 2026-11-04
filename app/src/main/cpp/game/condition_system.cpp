// Condition (CTDA) evaluator — literal port of the shipping engine chain
// FUN_0056a510 (list) -> FUN_0056ac50 (single) -> FUN_0056a3b0 (compare).
// See condition_system.h for the RE basis and record layout.
#include "condition_system.h"

namespace game {

namespace {

uint32_t read_u32_le(const uint8_t* data) {
    return static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) | (static_cast<uint32_t>(data[3]) << 24);
}

uint16_t read_u16_le(const uint8_t* data) {
    return static_cast<uint16_t>(static_cast<uint32_t>(data[0]) | (static_cast<uint32_t>(data[1]) << 8));
}

}  // namespace

bool ConditionItem::parse(const uint8_t* data, size_t size, ConditionItem& out) {
    if (data == nullptr || size < 20) {
        return false;
    }
    out.flag = data[0];
    out.comparison_value = read_u32_le(data + 4);
    out.function_index = read_u16_le(data + 8);
    out.param1 = read_u32_le(data + 12);
    out.param2 = read_u32_le(data + 16);
    return true;
}

// Literal port of FUN_0056a3b0 @ 0x56A3B0.
// The original switch has no default case: operations 6 and 7 fall through and
// always pass. Preserved verbatim.
uint32_t compare_conditions(uint32_t op, float left, float right) {
    switch (op) {
        case 0:  // Equal
            if (right != left) return 0;
            break;
        case 1:  // NotEqual
            if (right == left) return 0;
            break;
        case 2:  // Greater: fail when left <= right
            if (left <= right) return 0;
            break;
        case 3:  // GreaterEqual: fail when left < right
            if (left < right) return 0;
            break;
        case 4:  // Less: fail when left >= right
            if (left >= right) return 0;
            break;
        case 5:  // LessEqual: fail when left > right
            if (left > right) return 0;
            break;
        default:
            // ops 6..7: no default case in the original — always pass
            break;
    }
    return 1;
}

// Literal port of FUN_0056ac50 @ 0x56AC50 (single-condition evaluator).
//
// Differences from the engine, documented deliberately:
//  * A function index with no table row, or a row this runtime does not
//    implement, is reported as skip (unknown) instead of a hard failure, so
//    callers (AI selection, topic resolution) can choose conservative behavior.
//  * "funcTakesParam" is approximated by param_count >= 1 from the table
//    (RE open question R5); the engine consults the function's script signature.
EvalResult evaluate_single(const ConditionItem& item, uint32_t run_as,
                           uint32_t compare_target, ConditionRuntime& rt) {
    EvalResult out;  // {passed=false, skip=false}

    uint32_t operand = run_as;
    uint32_t target = compare_target;
    if (item.is_swapped()) {
        operand = compare_target;
        target = run_as;
    }

    const CondFuncEntry* entry = find_cond_function(item.function_index);
    if (entry == nullptr) {
        out.skip = true;
        return out;
    }

    uint32_t p1 = item.param1;
    if (p1 == 0 && entry->param_count >= 1) {
        p1 = target;
    }

    // All engine failure paths converge here: GetDisposition (0x4C) with
    // op Greater/GreaterEqual raises the skip marker; anything else is a hard
    // failure with no marker.
    const bool disposition_skip =
        item.function_index == 0x4C &&
        (item.comparison_op() == 2 || item.comparison_op() == 3);
    auto fail = [&]() {
        out.passed = false;
        out.skip = disposition_skip;
        return out;
    };

    if (entry->needs_ref != 0 && operand == 0) {
        return fail();
    }

    float result = 0.0f;
    if (!rt.query(item.function_index, operand, p1, item.param2, result)) {
        // Not implemented by this runtime: treat as unknown (skip).
        out.skip = true;
        return out;
    }

    float right;
    if (item.uses_global() && item.comparison_value != 0) {
        right = rt.global_value(item.comparison_value);
    } else {
        right = item.compare_float();
    }

    if (compare_conditions(item.comparison_op(), result, right) != 0) {
        out.passed = true;
        return out;
    }
    return fail();
}

// Literal port of FUN_0056a510 @ 0x56A510 (condition-list evaluator).
//
// The engine walks a singly linked list of 0x18-byte items; the vector mirrors
// node order. `secondary` (INFO conditions following DIAL conditions) is
// adopted once, only after at least one item has been processed, when the
// primary list reaches its end — matching the original's "next" step.
//
// OR-group semantics are NOT textbook OR: a group passes only when its closing
// (non-OR-flagged) member passed while the AND accumulator was still 1, and
// evaluation of remaining group members is short-circuited once a member
// passes. Reproduced verbatim; see RE_EXE_REPORT.md section 58.1.
EvalResult evaluate_list(const std::vector<ConditionItem>* list, uint32_t run_as,
                         uint32_t compare_target, ConditionRuntime& rt,
                         const std::vector<ConditionItem>* secondary) {
    EvalResult out;

    if (compare_target != 0) {
        uint32_t ctx = rt.context_override(compare_target);
        if (ctx != 0) {
            compare_target = ctx;
        }
    }

    int and_acc = 1;
    bool in_group = false;
    int group_skip = 0;
    bool group_no_pass = false;
    bool secondary_followed = false;
    bool fail_no_skip = false;
    int last = 0;
    bool processed_any = false;

    const std::vector<ConditionItem>* cur = list;
    size_t i = 0;

    for (;;) {
        // Loop guard: while head non-null; a zero-size list is the sentinel.
        if (cur == nullptr || i >= cur->size()) {
            if (cur == list && secondary != nullptr && !secondary_followed &&
                processed_any) {
                secondary_followed = true;
                cur = secondary;
                i = 0;
                continue;
            }
            break;  // exhausted
        }

        const ConditionItem& item = cur->at(i);
        EvalResult r = evaluate_single(item, run_as, compare_target, rt);
        last = r.passed ? 1 : 0;
        const int skip = r.skip ? 1 : 0;
        processed_any = true;

        if (in_group) {
            if (last == 0) {
                if (group_skip == 0 && skip == 0) {
                    // group_skip stays 0
                } else {
                    group_skip = 1;
                    if (skip != 0) {
                        goto group_post;
                    }
                }
                group_no_pass = true;
            }
            goto group_post;
        }

        if (!item.is_or()) {
            // AND item
            and_acc = (and_acc != 0 && last != 0) ? 1 : 0;
            if (skip == 0) {
                if (last == 0) {
                    fail_no_skip = true;
                }
            } else {
                out.skip = true;
            }
            goto early_check;
        }

        // Start OR group
        group_skip = skip;
        in_group = true;
        group_no_pass = false;
        if (skip == 0 && last == 0) {
            group_no_pass = true;
        }
        goto after_item;

    group_post:
        if (item.is_or()) {
            goto after_item;  // group continues
        }
        // Group closes on this item.
        in_group = false;
        if (and_acc == 0 || last == 0) {
            and_acc = 0;
            if (group_skip != 0 && !group_no_pass) {
                out.skip = true;
                goto next;
            }
            fail_no_skip = true;
            goto early_check;
        }
        and_acc = 1;
        goto after_item;

    early_check:
        if (and_acc == 0 && !in_group && !out.skip) {
            out.passed = false;
            return out;  // hard AND failure without any skip: stop immediately
        }
        // fall through
    next:
        ++i;
        continue;
    after_item:
        ++i;
        continue;
    }

    // List exhausted.
    if (in_group) {
        if (and_acc != 0 && last != 0) {
            out.skip = false;
            out.passed = true;
            return out;
        }
        and_acc = 0;
        if (group_skip != 0 && !group_no_pass) {
            // FUN_0056a510 sets *outSkip = 1 then falls through LAB_0056a732:
            // a pending fail_no_skip (bVar5) still clears it at LAB_0056a739.
            out.skip = !fail_no_skip;
            out.passed = false;
            return out;
        }
        out.skip = false;
        out.passed = false;
        return out;
    }

    if (and_acc != 0) {
        // local_6 != 0 -> goto LAB_0056a739: *outSkip = 0; return and_acc
        out.skip = false;
        out.passed = true;
        return out;
    }

    if (fail_no_skip) {
        // LAB_0056a732 with bVar5: fall through LAB_0056a739, skip cleared.
        out.skip = false;
        out.passed = false;
        return out;
    }

    // LAB_0056a732 with !bVar5: return and_acc (0) WITHOUT touching outSkip.
    // A skip raised earlier (AND item or OR-group close) survives to here.
    out.passed = false;
    return out;
}

// Condition-capable function table (shipping table rows with a non-NULL
// condfunc pointer, base 0x00B0C8C0 stride 0x28). Generated verbatim from
// tmp/re/s58_condfunctions_compact.txt; needs_ref = low byte of the flags field.
const CondFuncEntry kCondFunctions[] = {
    { /*idx*/   1, /*name*/ "GetDistance                ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/   5, /*name*/ "GetLocked                  ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/   6, /*name*/ "GetPos                     ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/   8, /*name*/ "GetAngle                   ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  10, /*name*/ "GetStartingPos             ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  11, /*name*/ "GetStartingAngle           ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  12, /*name*/ "GetSecondsPassed           ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/  14, /*name*/ "GetActorValue              ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  18, /*name*/ "GetCurrentTime             ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/  24, /*name*/ "GetScale                   ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  27, /*name*/ "GetLineOfSight             ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  32, /*name*/ "GetInSameCell              ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  35, /*name*/ "GetDisabled                ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  36, /*name*/ "MenuMode                   ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/  39, /*name*/ "GetDisease                 ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  40, /*name*/ "GetVampire                 ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  41, /*name*/ "GetClothingValue           ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  42, /*name*/ "SameFaction                ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  43, /*name*/ "SameRace                   ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  44, /*name*/ "SameSex                    ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  45, /*name*/ "GetDetected                ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  46, /*name*/ "GetDead                    ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  47, /*name*/ "GetItemCount               ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  48, /*name*/ "GetGold                    ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  49, /*name*/ "GetSleeping                ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  50, /*name*/ "GetTalkedToPC              ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  53, /*name*/ "GetScriptVariable          ", /*param_count*/ 2, /*needs_ref*/ 0 },
    { /*idx*/  56, /*name*/ "GetQuestRunning            ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/  58, /*name*/ "GetStage                   ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/  59, /*name*/ "GetStageDone               ", /*param_count*/ 2, /*needs_ref*/ 0 },
    { /*idx*/  60, /*name*/ "GetFactionRankDifference   ", /*param_count*/ 2, /*needs_ref*/ 1 },
    { /*idx*/  61, /*name*/ "GetAlarmed                 ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  62, /*name*/ "IsRaining                  ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/  63, /*name*/ "GetAttacked                ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  64, /*name*/ "GetIsCreature              ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  65, /*name*/ "GetLockLevel               ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  66, /*name*/ "GetShouldAttack            ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  67, /*name*/ "GetInCell                  ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  68, /*name*/ "GetIsClass                 ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  69, /*name*/ "GetIsRace                  ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  70, /*name*/ "GetIsSex                   ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  71, /*name*/ "GetInFaction               ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  72, /*name*/ "GetIsID                    ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  73, /*name*/ "GetFactionRank             ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  74, /*name*/ "GetGlobalValue             ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/  75, /*name*/ "IsSnowing                  ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/  76, /*name*/ "GetDisposition             ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/  77, /*name*/ "GetRandomPercent           ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/  79, /*name*/ "GetQuestVariable           ", /*param_count*/ 2, /*needs_ref*/ 0 },
    { /*idx*/  80, /*name*/ "GetLevel                   ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  81, /*name*/ "GetArmorRating             ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  84, /*name*/ "GetDeadCount               ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/  91, /*name*/ "GetIsAlerted               ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/  98, /*name*/ "GetPlayerControlsDisabled  ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/  99, /*name*/ "GetHeadingAngle            ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/ 101, /*name*/ "IsWeaponOut                ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 102, /*name*/ "IsTorchOut                 ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 103, /*name*/ "IsShieldOut                ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 104, /*name*/ "IsYielding                 ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 106, /*name*/ "IsFacingUp                 ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 107, /*name*/ "GetKnockedState            ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 108, /*name*/ "GetWeaponAnimType          ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 109, /*name*/ "GetWeaponSkillType         ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 110, /*name*/ "GetCurrentAIPackage        ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 111, /*name*/ "IsWaiting                  ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 112, /*name*/ "IsIdlePlaying              ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 116, /*name*/ "GetCrimeGold               ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 122, /*name*/ "GetCrime                   ", /*param_count*/ 2, /*needs_ref*/ 1 },
    { /*idx*/ 125, /*name*/ "IsGuard                    ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 127, /*name*/ "CanPayCrimeGold            ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 128, /*name*/ "GetFatiguePercentage       ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 129, /*name*/ "GetPCIsClass               ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 130, /*name*/ "GetPCIsRace                ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 131, /*name*/ "GetPCIsSex                 ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 132, /*name*/ "GetPCInFaction             ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 133, /*name*/ "SameFactionAsPC            ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 134, /*name*/ "SameRaceAsPC               ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 135, /*name*/ "SameSexAsPC                ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 136, /*name*/ "GetIsReference             ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/ 141, /*name*/ "IsTalking                  ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 142, /*name*/ "GetWalkSpeed               ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 143, /*name*/ "GetCurrentAIProcedure      ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 144, /*name*/ "GetTrespassWarningLevel    ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 145, /*name*/ "IsTrespassing              ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 146, /*name*/ "IsInMyOwnedCell            ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 147, /*name*/ "GetWindSpeed               ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 148, /*name*/ "GetCurrentWeatherPercent   ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 149, /*name*/ "GetIsCurrentWeather        ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 150, /*name*/ "IsContinuingPackagePCNear  ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 153, /*name*/ "CanHaveFlames              ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 154, /*name*/ "HasFlames                  ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 157, /*name*/ "GetOpenState               ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 159, /*name*/ "GetSitting                 ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 160, /*name*/ "GetFurnitureMarkerID       ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 161, /*name*/ "GetIsCurrentPackage        ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/ 162, /*name*/ "IsCurrentFurnitureRef      ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/ 163, /*name*/ "IsCurrentFurnitureObj      ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/ 170, /*name*/ "GetDayOfWeek               ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 171, /*name*/ "IsPlayerInJail             ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 172, /*name*/ "GetTalkedToPCParam         ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 175, /*name*/ "IsPCSleeping               ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 176, /*name*/ "IsPCAMurderer              ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 180, /*name*/ "GetDetectionLevel          ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/ 182, /*name*/ "GetEquipped                ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/ 185, /*name*/ "IsSwimming                 ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 190, /*name*/ "GetAmountSoldStolen        ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 193, /*name*/ "GetPCExpelled              ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 195, /*name*/ "GetPCFactionMurder         ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 197, /*name*/ "GetPCFactionSteal          ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 199, /*name*/ "GetPCFactionAttack         ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 201, /*name*/ "GetPCFactionSubmitAuthority", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 203, /*name*/ "GetDestroyed               ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 214, /*name*/ "HasMagicEffect             ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/ 215, /*name*/ "GetDoorDefaultOpen         ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 223, /*name*/ "IsSpellTarget              ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/ 224, /*name*/ "GetIsPlayerBirthsign       ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 225, /*name*/ "GetPersuasionNumber        ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 227, /*name*/ "HasVampireFed              ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 228, /*name*/ "GetIsClassDefault          ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 229, /*name*/ "GetClassDefaultMatch       ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 230, /*name*/ "GetInCellParam             ", /*param_count*/ 2, /*needs_ref*/ 0 },
    { /*idx*/ 237, /*name*/ "GetIsGhost                 ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 242, /*name*/ "GetUnconscious             ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 244, /*name*/ "GetRestrained              ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 246, /*name*/ "GetIsUsedItem              ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 247, /*name*/ "GetIsUsedItemType          ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 249, /*name*/ "GetPCFame                  ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 251, /*name*/ "GetPCInfamy                ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 254, /*name*/ "GetIsPlayableRace          ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 255, /*name*/ "GetOffersServicesNow       ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 258, /*name*/ "GetUsedItemLevel           ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 259, /*name*/ "GetUsedItemActivate        ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 264, /*name*/ "GetBarterGold              ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 265, /*name*/ "IsTimePassing              ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 266, /*name*/ "IsPleasant                 ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 267, /*name*/ "IsCloudy                   ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 274, /*name*/ "GetArmorRatingUpperBody    ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 277, /*name*/ "GetBaseActorValue          ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/ 278, /*name*/ "IsOwner                    ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/ 280, /*name*/ "IsCellOwner                ", /*param_count*/ 2, /*needs_ref*/ 1 },
    { /*idx*/ 282, /*name*/ "IsHorseStolen              ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 285, /*name*/ "IsLeftUp                   ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 286, /*name*/ "IsSneaking                 ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 287, /*name*/ "IsRunning                  ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 288, /*name*/ "GetFriendHit               ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/ 289, /*name*/ "IsInCombat                 ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 300, /*name*/ "IsInInterior               ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 305, /*name*/ "GetInvestmentGold          ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 306, /*name*/ "IsActorUsingATorch         ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 309, /*name*/ "IsXBox                     ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 310, /*name*/ "GetInWorldspace            ", /*param_count*/ 1, /*needs_ref*/ 1 },
    { /*idx*/ 312, /*name*/ "GetPCMiscStat              ", /*param_count*/ 1, /*needs_ref*/ 0 },
    { /*idx*/ 313, /*name*/ "IsActorEvil                ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 314, /*name*/ "IsActorAVictim             ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 315, /*name*/ "GetTotalPersuasionNumber   ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 318, /*name*/ "GetIdleDoneOnce            ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 320, /*name*/ "GetNoRumors                ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 323, /*name*/ "WhichServiceMenu           ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 327, /*name*/ "IsRidingHorse              ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 329, /*name*/ "IsTurnArrest               ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 332, /*name*/ "IsInDangerousWater         ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 338, /*name*/ "GetIgnoreFriendlyHits      ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 339, /*name*/ "IsPlayersLastRiddenHorse   ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 353, /*name*/ "IsActor                    ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 354, /*name*/ "IsEssential                ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 358, /*name*/ "IsPlayerMovingIntoNewSpace ", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 361, /*name*/ "GetTimeDead                ", /*param_count*/ 0, /*needs_ref*/ 1 },
    { /*idx*/ 362, /*name*/ "GetPlayerHasLastRiddenHorse", /*param_count*/ 0, /*needs_ref*/ 0 },
    { /*idx*/ 365, /*name*/ "GetPlayerInSEWorld         ", /*param_count*/ 0, /*needs_ref*/ 0 },
};

const size_t kCondFunctionCount =
    sizeof(kCondFunctions) / sizeof(kCondFunctions[0]);

const CondFuncEntry* find_cond_function(uint16_t index) {
    for (size_t i = 0; i < kCondFunctionCount; ++i) {
        if (kCondFunctions[i].index == index) {
            return &kCondFunctions[i];
        }
    }
    return nullptr;
}

}  // namespace game
