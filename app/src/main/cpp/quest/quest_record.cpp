#include "quest_record.h"
#include <cstring>
#include <algorithm>

// ============================================================================
// QuestCondition - condition function name lookup
// ============================================================================

const char* QuestCondition::getFunctionName(uint8_t index) {
    // Oblivion condition function table (subset)
    static const char* names[] = {
        "GetDistance",           // 0
        "GetLocked",             // 1
        "GetPos",                // 2
        "GetAngle",              // 3
        "GetStartingPos",        // 4
        "GetStartingAngle",      // 5
        "GetSecondsPassed",      // 6
        "GetActorValue",         // 7
        "GetCurrentTime",        // 8
        "GetScale",              // 9
        "IsMoving",              // 10
        "IsTurning",             // 11
        "GetLineOfSight",        // 12
        "GetInSameCell",         // 13
        "GetDisabled",           // 14
        "MenuMode",              // 15
        "GetDisease",            // 16
        "GetVampire",            // 17
        "GetClothingValue",      // 18
        "SameFaction",           // 19
        "SameRace",              // 20
        "SameSex",               // 21
        "GetDetected",           // 22
        "GetDead",               // 23
        "GetItemCount",          // 24
        "GetGold",               // 25
        "GetSleeping",           // 26
        "GetTalkedToPC",         // 27
        "GetScriptVariable",     // 28
        "GetQuestRunning",       // 29
        "GetStage",              // 30
        "GetStageDone",          // 31
        "GetFactionRankDifference", // 32
        "GetAlarmed",            // 33
        "IsRaining",             // 34
        "GetAttacked",           // 35
        "GetIsCreature",         // 36
        "GetLockLevel",          // 37
        "GetShouldAttack",       // 38
        "GetInCell",             // 39
        "GetIsClass",            // 40
        "GetIsRace",             // 41
        "GetIsSex",              // 42
        "GetInFaction",          // 43
        "GetIsID",               // 44
        "GetFactionRank",        // 45
        "GetGlobalValue",        // 46
        "IsSnowing",             // 47
        "GetRandomPercent",      // 48
        "GetQuestVariable",      // 49
        "GetLevel",              // 50
        "GetArmorRating",        // 51
        "GetDeadCount",          // 52
        "GetIsAlerted",          // 53
        "GetPlayerControlsDisabled", // 54
        "GetHeadingAngle",       // 55
        "IsWeaponOut",           // 56
        "IsTorchOut",            // 57
        "IsShieldOut",           // 58
        "IsFacingUp",            // 59
        "GetInZone",             // 60
        "HasMagicEffect",        // 61
        "GetDefaultOpen",        // 62
        "IsAnimPlaying",         // 63
        "GetInSameQuest",        // 64
        "GetPCIsClass",          // 65
        "GetPCIsRace",           // 66
        "GetPCIsSex",            // 67
        "GetPCInFaction",        // 68
        "GetPCIsID",             // 69
        "GetPCFactionRank",      // 70
        "IsPlayerActionActive",  // 71
        "GetIsUsedItem",         // 72
        "GetIsReferenceType",    // 73
        "IsPlayerLastHit",       // 74
        "IsPlayerActionLast",    // 75
        "GetIsCreatureType",     // 76
        "GetInCurrentCell",      // 77
        "GetQuestCompleted",     // 78
    };
    static constexpr size_t count = sizeof(names) / sizeof(names[0]);
    if (index < count) return names[index];
    return "Unknown";
}

// ============================================================================
// QuestRecord - helper methods
// ============================================================================

const QuestStageEntry* QuestRecord::findStage(int32_t stageIndex) const {
    for (const auto& stage : stages) {
        if (stage.stageIndex == stageIndex) return &stage;
    }
    return nullptr;
}

const QuestObjectiveEntry* QuestRecord::findObjective(uint32_t objectiveIndex) const {
    for (const auto& obj : objectives) {
        if (obj.objectiveIndex == objectiveIndex) return &obj;
    }
    return nullptr;
}

bool QuestRecord::hasCompletionStage() const {
    for (const auto& stage : stages) {
        if (stage.isCompletionStage()) return true;
    }
    return false;
}

int32_t QuestRecord::getCompletionStageIndex() const {
    for (const auto& stage : stages) {
        if (stage.isCompletionStage()) return stage.stageIndex;
    }
    return -1;
}

std::vector<const QuestStageEntry*> QuestRecord::getSortedStages() const {
    std::vector<const QuestStageEntry*> sorted;
    sorted.reserve(stages.size());
    for (const auto& s : stages) sorted.push_back(&s);
    std::sort(sorted.begin(), sorted.end(),
        [](const QuestStageEntry* a, const QuestStageEntry* b) {
            return a->stageIndex < b->stageIndex;
        });
    return sorted;
}

std::vector<const QuestObjectiveEntry*> QuestRecord::getObjectivesForStage(int32_t stageIndex) const {
    // In Oblivion, objectives are generally active across the whole quest,
    // but we filter by conditions that reference the current stage
    std::vector<const QuestObjectiveEntry*> result;
    for (const auto& obj : objectives) {
        // If no conditions, objective is always active
        if (obj.conditions.empty()) {
            result.push_back(&obj);
            continue;
        }
        // Check if any condition references GetStage with matching value
        for (const auto& cond : obj.conditions) {
            // GetStage (30) or GetStageDone (31) conditions
            if (cond.functionIndex == 30 || cond.functionIndex == 31) {
                if (static_cast<int32_t>(cond.comparisonValue) == stageIndex) {
                    result.push_back(&obj);
                    break;
                }
            }
        }
    }
    return result;
}

// ============================================================================
// QuestRecordParser - parse QUST subrecords
// ============================================================================

bool QuestRecordParser::parse(const uint8_t* data, size_t dataSize, QuestRecord& outRecord) {
    if (!data || dataSize < 4) {
        LOGE("Invalid quest record data");
        return false;
    }

    // The stage body is strictly "script entries then target groups": no QSDT
    // ever follows that stage's first QSTA, and no stage carries QSTA without
    // QSDT. CTDA is owned by the nearest preceding owner subrecord, which is
    // QSTA (2,115), QSDT (950) or the quest-level run after DATA (719).
    QuestStageEntry* currentStage = nullptr;
    QuestStageBlock* currentBlock = nullptr;
    QuestTarget* currentTarget = nullptr;
    uint16_t blockIndex = 0;

    size_t offset = 0;
    while (offset + 6 <= dataSize) {
        char recType[5] = {};
        std::memcpy(recType, data + offset, 4);
        uint16_t recSize = 0;
        std::memcpy(&recSize, data + offset + 4, 2);

        offset += 6;
        if (offset + recSize > dataSize) break;

        const uint8_t* recData = data + offset;

        if (std::memcmp(recType, "EDID", 4) == 0) {
            parseEDID(recData, recSize, outRecord.editorID);
        } else if (std::memcmp(recType, "FULL", 4) == 0) {
            parseFULL(recData, recSize, outRecord.fullName);
        } else if (std::memcmp(recType, "DESC", 4) == 0) {
            parseDESC(recData, recSize, outRecord.description);
        } else if (std::memcmp(recType, "DATA", 4) == 0) {
            parseDATA(recData, recSize, outRecord.questFlags, outRecord.priority);
        } else if (std::memcmp(recType, "NNAM", 4) == 0) {
            if (recSize >= 4) {
                std::memcpy(&outRecord.nextQuestFormID, recData, 4);
            }
        } else if (std::memcmp(recType, "FNAM", 4) == 0) {
            if (recSize >= 4) {
                std::memcpy(&outRecord.extendedFlags, recData, 4);
            }
        } else if (std::memcmp(recType, "VNAM", 4) == 0) {
            if (recSize >= 1) {
                outRecord.questType = recData[0];
            }
        } else if (std::memcmp(recType, "ANAM", 4) == 0) {
            if (recSize > 0) {
                size_t len = recSize;
                while (len > 0 && recData[len - 1] == '\0') --len;
                outRecord.altName.assign(reinterpret_cast<const char*>(recData), len);
            }
        } else if (std::memcmp(recType, "INDX", 4) == 0) {
            // INDX is the stage index and is always 2 bytes. It is a plain
            // stage number, not a decile grid: 112 of the 136 distinct values
            // are not multiples of 10 and 0 is a legal stage.
            QuestStageEntry stage;
            if (parseStageEntry(recData, recSize, stage)) {
                outRecord.stages.push_back(std::move(stage));
                currentStage = &outRecord.stages.back();
                currentBlock = nullptr;
                currentTarget = nullptr;
                blockIndex = 0;
            }
        } else if (std::memcmp(recType, "QSDT", 4) == 0) {
            // QSDT opens a script block. Its byte is a run-once flag, and the
            // block index must be the QSDT ordinal rather than the ordinal of
            // the blocks that happen to carry SCDA, because 1,277 blocks
            // declare a header with neither bytecode nor source.
            if (currentStage && recSize >= 1) {
                QuestStageBlock block;
                block.scriptIndex = blockIndex++;
                block.qsdtFlags = recData[0];
                currentStage->blocks.push_back(std::move(block));
                currentBlock = &currentStage->blocks.back();
                currentTarget = nullptr;
            }
        } else if (std::memcmp(recType, "SCHR", 4) == 0) {
            // Script header, always 20 bytes and 1:1 with QSDT.
            if (currentBlock && recSize >= 20) {
                currentBlock->script.formID = outRecord.formID;
                currentBlock->script.editorID = outRecord.editorID;
            }
        } else if (std::memcmp(recType, "SCDA", 4) == 0) {
            // Compiled bytecode. Long scripts are split across several SCDA
            // subrecords, so every chunk is appended rather than replaced.
            if (currentBlock) {
                currentBlock->script.bytecode.insert(
                    currentBlock->script.bytecode.end(), recData, recData + recSize);
            }
        } else if (std::memcmp(recType, "SCTX", 4) == 0) {
            if (currentBlock) {
                currentBlock->script.source.append(
                    reinterpret_cast<const char*>(recData), recSize);
            }
        } else if (std::memcmp(recType, "SCRO", 4) == 0) {
            if (currentBlock && recSize >= 4) {
                uint32_t reference = 0;
                std::memcpy(&reference, recData, 4);
                currentBlock->script.references.push_back(reference);
            }
        } else if (std::memcmp(recType, "CNAM", 4) == 0) {
            // Journal text of the enclosing block, not of the stage: 192 of the
            // 194 multi-block stages carrying more than one CNAM have texts
            // that all differ, so a single per-stage logText cannot represent
            // real data.
            if (currentBlock && recSize > 0) {
                size_t len = recSize;
                while (len > 0 && recData[len - 1] == '\0') --len;
                currentBlock->logText.assign(reinterpret_cast<const char*>(recData), len);
                if (currentStage->logText.empty()) {
                    currentStage->logText = currentBlock->logText;
                }
            }
        } else if (std::memcmp(recType, "QSTA", 4) == 0) {
            // QSTA opens a target group and owns the CTDA that follow it.
            QuestTarget target;
            if (recSize >= 4) {
                std::memcpy(&target.targetFormID, recData, 4);
            }
            if (recSize >= 8) {
                std::memcpy(&target.objectiveIndex, recData + 4, 4);
            }
            outRecord.targets.push_back(std::move(target));
            currentTarget = &outRecord.targets.back();
            currentBlock = nullptr;
        } else if (std::memcmp(recType, "CTDA", 4) == 0) {
            QuestCondition cond;
            if (parseCondition(recData, recSize, cond)) {
                if (currentTarget) {
                    // QSTA-owned conditions gate the target, not the stage.
                } else if (currentBlock) {
                    currentBlock->conditions.push_back(cond);
                } else if (currentStage) {
                    currentStage->conditions.push_back(cond);
                } else {
                    // Quest-level conditions run after DATA and belong to no
                    // stage; they gate the quest itself.
                    outRecord.conditions.push_back(cond);
                }
            }
        } else if (std::memcmp(recType, "SCRN", 4) == 0) {
            if (recSize > 0) {
                size_t len = recSize;
                while (len > 0 && recData[len - 1] == '\0') --len;
                outRecord.iconPath.assign(reinterpret_cast<const char*>(recData), len);
            }
        }

        offset += recSize;
    }

    LOGD("Parsed quest record: FormID=0x%08X, EDID=%s, FULL=%s, Stages=%zu, Objectives=%zu",
         outRecord.formID, outRecord.editorID.c_str(), outRecord.fullName.c_str(),
         outRecord.stages.size(), outRecord.objectives.size());

    return true;
}

bool QuestRecordParser::parseEDID(const uint8_t* data, size_t size, std::string& outEditorID) {
    if (!data || size == 0) return false;
    size_t len = size;
    while (len > 0 && data[len - 1] == '\0') --len;
    outEditorID.assign(reinterpret_cast<const char*>(data), len);
    return true;
}

bool QuestRecordParser::parseFULL(const uint8_t* data, size_t size, std::string& outFullName) {
    if (!data || size == 0) return false;
    size_t len = size;
    while (len > 0 && data[len - 1] == '\0') --len;
    outFullName.assign(reinterpret_cast<const char*>(data), len);
    return true;
}

bool QuestRecordParser::parseDESC(const uint8_t* data, size_t size, std::string& outDescription) {
    if (!data || size == 0) return false;
    size_t len = size;
    while (len > 0 && data[len - 1] == '\0') --len;
    outDescription.assign(reinterpret_cast<const char*>(data), len);
    return true;
}

bool QuestRecordParser::parseDATA(const uint8_t* data, size_t size,
                                   uint8_t& outFlags, uint8_t& outPriority) {
    if (!data || size < MIN_QUEST_DATA_SIZE) return false;
    outFlags = data[0];
    outPriority = data[1];
    return true;
}

bool QuestRecordParser::parseStageEntry(const uint8_t* data, size_t size,
                                         QuestStageEntry& outStage) {
    // INDX is always 2 bytes on real data; the old size < 4 guard made this
    // return false for every stage in the file.
    if (!data || size < 2) return false;
    uint16_t stageIndex = 0;
    std::memcpy(&stageIndex, data, sizeof(stageIndex));
    outStage.stageIndex = static_cast<int32_t>(stageIndex);
    return true;
}

bool QuestRecordParser::parseObjectiveEntry(const uint8_t* data, size_t size,
                                             QuestObjectiveEntry& outObjective) {
    if (!data || size < 4) return false;
    std::memcpy(&outObjective.objectiveIndex, data, 4);
    return true;
}

bool QuestRecordParser::parseCondition(const uint8_t* data, size_t size,
                                        QuestCondition& outCondition) {
    if (!data || size < CONDITION_SIZE) return false;

    outCondition.functionIndex = data[0];
    outCondition.flags = data[1];
    outCondition.comparisonOp = data[2];
    outCondition.padding = data[3];
    std::memcpy(&outCondition.comparisonValue, data + 4, 4);
    std::memcpy(&outCondition.param1, data + 8, 4);
    std::memcpy(&outCondition.param2, data + 12, 4);

    return true;
}
