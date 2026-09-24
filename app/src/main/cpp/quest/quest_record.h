#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <functional>
#include <android/log.h>
#include "../script/script_context.h"

#define LOG_TAG "QuestRecord"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// Oblivion QUST Record - Full Subrecord Definitions
// Phase 39: Complete quest record parsing for ESM data
// ============================================================================

// Quest flags (from QUST DATA subrecord)
enum class QuestFlag : uint8_t {
    NONE           = 0x00,
    START_ENABLED  = 0x01,
    REPEATING      = 0x02,
    UNKNOWN_04     = 0x04,
    UNKNOWN_08     = 0x08,
    UNKNOWN_10     = 0x10,
    UNKNOWN_20     = 0x20,
    UNKNOWN_40     = 0x40,
    UNKNOWN_80     = 0x80
};

// Quest stage flags. The on-disk QSDT byte is a per-block run-once flag, not a
// stage flag, so it lives on QuestStageBlock::qsdtFlags. A stage carries no
// flag byte of its own in the file; the only values that occur are 0x00 and
// 0x01, and 0x01 marks the block that terminates the quest.
enum class StageFlag : uint8_t {
    NONE     = 0x00,
    RUN_ONCE = 0x01
};

// ============================================================================
// Quest Condition (CTDA subrecord)
// ============================================================================
struct QuestCondition {
    uint8_t functionIndex = 0;
    uint8_t flags = 0;
    uint8_t comparisonOp = 0;
    uint8_t padding = 0;
    float comparisonValue = 0.0f;
    uint32_t param1 = 0;
    uint32_t param2 = 0;

    static const char* getFunctionName(uint8_t index);
};

// ============================================================================
// Quest Stage Entry (per-stage data)
// ============================================================================
struct QuestStageBlock {
    uint16_t scriptIndex = 0;  // QSDT/SCHR block ordinal within the stage
    uint8_t qsdtFlags = 0;     // Bit 0: run once
    std::vector<QuestCondition> conditions;
    std::string logText;
    oblivion::script::ScriptData script;

    bool hasBytecode() const { return !script.bytecode.empty(); }
    bool runsOnce() const { return (qsdtFlags & 0x01) != 0; }
};

struct QuestStageEntry {
    int32_t stageIndex = 0;
    StageFlag flags = StageFlag::NONE;
    std::string logText;
    std::vector<QuestCondition> conditions;
    uint32_t nextQuestFormID = 0;
    std::vector<QuestStageBlock> blocks;

    // A stage completes its quest when one of its blocks is a run-once block.
    // The measurement over all 390 QUST records shows QSDT bit 0 predicts
    // StopQuest far more strongly than position does (67% of flagged blocks
    // below the highest stage call it against 1% of unflagged ones), so the
    // flag is the completion signal and there is no separate stage flag byte.
    bool isCompletionStage() const {
        for (const auto& block : blocks) {
            if (block.runsOnce()) return true;
        }
        return false;
    }
};

// ============================================================================
// Quest Objective Entry
// ============================================================================
enum class ObjectiveType : uint8_t {
    NONE      = 0,
    KILL      = 1,
    REACH     = 2,
    FIND      = 3,
    TALK      = 4,
    ESCORT    = 5,
    DISCOVER  = 6,
    CUSTOM    = 7
};

struct QuestObjectiveEntry {
    uint32_t objectiveIndex = 0;
    std::string description;
    uint32_t targetFormID = 0;
    ObjectiveType type = ObjectiveType::NONE;
    bool isOptional = false;
    int32_t displayPriority = 0;
    std::vector<QuestCondition> conditions;
};

// ============================================================================
// Quest Target (for objectives with targets)
// ============================================================================
struct QuestTarget {
    uint32_t targetFormID = 0;
    uint32_t objectiveIndex = 0;
    uint8_t flags = 0;
    std::string targetName;
};

// ============================================================================
// Full Quest Record (parsed from ESM QUST record)
// ============================================================================
struct QuestRecord {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string description;
    std::string iconPath;
    uint8_t questFlags = 0;
    uint8_t priority = 0;

    std::vector<QuestStageEntry> stages;
    std::vector<QuestObjectiveEntry> objectives;
    std::vector<QuestTarget> targets;
    // Quest-level conditions: the 719 CTDA that run after DATA and belong to no
    // stage. They gate the quest itself.
    std::vector<QuestCondition> conditions;

    uint32_t nextQuestFormID = 0;
    uint32_t extendedFlags = 0;
    uint8_t questType = 0;
    std::string altName;

    const QuestStageEntry* findStage(int32_t stageIndex) const;
    const QuestObjectiveEntry* findObjective(uint32_t objectiveIndex) const;
    bool hasCompletionStage() const;
    int32_t getCompletionStageIndex() const;
    bool isMainQuest() const { return questType == 1; }
    bool isGuildQuest() const { return questType == 2; }
    bool isSideQuest() const { return questType == 3; }

    std::vector<const QuestStageEntry*> getSortedStages() const;
    std::vector<const QuestObjectiveEntry*> getObjectivesForStage(int32_t stageIndex) const;
};

// ============================================================================
// Quest Record Parser
// ============================================================================
class QuestRecordParser {
public:
    static bool parse(const uint8_t* data, size_t dataSize, QuestRecord& outRecord);

    static bool parseEDID(const uint8_t* data, size_t size, std::string& outEditorID);
    static bool parseFULL(const uint8_t* data, size_t size, std::string& outFullName);
    static bool parseDESC(const uint8_t* data, size_t size, std::string& outDescription);
    static bool parseDATA(const uint8_t* data, size_t size, uint8_t& outFlags, uint8_t& outPriority);
    static bool parseStageEntry(const uint8_t* data, size_t size, QuestStageEntry& outStage);
    static bool parseObjectiveEntry(const uint8_t* data, size_t size, QuestObjectiveEntry& outObjective);
    static bool parseCondition(const uint8_t* data, size_t size, QuestCondition& outCondition);

private:
    static constexpr size_t MIN_QUEST_DATA_SIZE = 2;
    static constexpr size_t CONDITION_SIZE = 24;
};
