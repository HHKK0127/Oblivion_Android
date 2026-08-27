#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include "../assets/esm_reader.h"

// ============================================================================
// Oblivion Dialogue Record - Extended INFO Record Processing
// Parses full INFO subrecords: EDID, NAME, RESP, BNAM, GNAM, ANAM, TIFC,
// TRDT, SCVR, TCLT, TCLF, DATA
// ============================================================================

namespace oblivion {
namespace dialogue {

// ============================================================================
// Dialogue types (from DIAL record)
// ============================================================================
enum class DialogueType : uint8_t {
    Topic        = 0,  // Regular conversation
    Conversation = 1,  // Auto-start conversation
    Combat       = 2,  // Combat dialogue
    Persuasion   = 3,  // Persuasion dialogue
    Detection    = 4,  // Detection dialogue
    Service      = 5,  // Service dialogue (merchants)
    Misc         = 6   // Miscellaneous
};

// ============================================================================
// INFO response type (from TRDT subrecord)
// ============================================================================
enum class ResponseType : uint8_t {
    Neutral  = 0,
    Positive = 1,
    Negative = 2,
    Command  = 3
};

// ============================================================================
// Condition function types (from CTDA subrecord)
// ============================================================================
enum class ConditionFunction : uint16_t {
    GetDistance           = 1,
    GetLocked             = 5,
    GetPos                = 6,
    GetAngle              = 8,
    GetStartingPos        = 10,
    GetStartingAngle      = 11,
    GetSecondsPassed      = 12,
    GetActorValue         = 14,
    GetCurrentTime        = 17,
    GetScale              = 18,
    IsMoving              = 29,
    IsTurning             = 30,
    GetDisabled           = 33,
    MenuMode              = 35,
    GetDisease            = 39,
    GetClothingValue      = 41,
    SameFaction           = 42,
    SameRace              = 43,
    SameSex               = 44,
    GetDetected           = 45,
    GetDead               = 46,
    GetItemCount          = 47,
    GetGold               = 48,
    GetSleeping           = 49,
    GetTalkedToPC         = 50,
    GetScriptVariable     = 53,
    GetQuestRunning       = 56,
    GetStage              = 58,
    GetStageDone          = 59,
    GetFactionRank        = 60,
    GetGlobalValue        = 61,
    IsSnowing             = 63,
    GetDisposition        = 65,
    GetRandomPercent      = 69,
    GetQuestVariable      = 72,
    GetLevel              = 73,
    GetArmorRating        = 74,
    GetDeadCount          = 75,
    GetIsAlerted          = 76,
    GetPlayerControlsDisabled = 80,
    GetHeadingAngle       = 81,
    IsWeaponOut           = 82,
    IsTalking             = 84,
    GetWalkSpeed          = 86,
    GetCurrentAIPackage   = 88,
    GetCurrentAIProcedure = 89,
    GetCrimeGold          = 92,
    GetCrimeGoldViolent   = 94,
    IsWeaponMagic         = 96,
    GetDestruction        = 100,
    GetIllusion            = 101,
    GetAlteration          = 102,
    GetConjuration         = 103,
    GetMysticism           = 104,
    GetRestoration         = 105,
    GetCombatStyle         = 106,
    GetInSameCell          = 108,
    GetPlayerInSameCell    = 110,
    GetPersuasionNumber    = 114,
    GetVampire             = 115,
    GetCannibal            = 117,
    GetClassDefaultMatch   = 118,
    GetVampireLord         = 120,
    GetWindSpeed           = 123,
    GetCurrentWeather      = 124,
    IsContinuingPackagePCNear = 126,
    GetIsClass             = 128,
    GetIsRace              = 129,
    GetIsSex               = 130,
    GetInFaction           = 131,
    GetIsID                = 132,
    GetFactionRankDifference = 133,
    GetLevelDifference     = 134,
    GetAttacked            = 136,
    GetKnockedState        = 138,
    GetWeaponAnimType      = 141,
    IsWeaponSkillType      = 142,
    IsInInterior           = 145,
    GetInvestmentGold      = 146,
    GetActorValuePercent   = 148,
    IsPCSleeping           = 150,
    IsPCAMurderer          = 152,
    GetRelationshipRank    = 154,
    GetBaseActorValue      = 157,
    IsOwner                = 158,
    IsCellOwner            = 159,
    IsHorseStolen          = 160,
    IsLeftUp               = 165,
    IsSneaking             = 167,
    GetStaminaPercentage   = 168,
    GetPCIsClass           = 170,
    GetPCIsRace            = 171,
    GetPCIsSex             = 172,
    GetPCInFaction         = 173,
    SameFactionAsPC        = 174,
    GetPCIsID              = 176,
    GetIsVoiceType         = 178,
    GetQuestCompleted      = 180,
    GetIsCreatureType      = 188,
    GetPCExpelled          = 193,
    GetPCFactionMurder     = 195,
    GetPCEnemyofFaction     = 197,
    GetPCFactionAttack     = 199,
    GetDestroyed           = 203
};

// ============================================================================
// Condition comparison operators
// ============================================================================
enum class ConditionOp : uint8_t {
    Equal         = 0,
    NotEqual      = 1,
    GreaterThan   = 2,
    GreaterEqual  = 3,
    LessThan      = 4,
    LessEqual     = 5
};

// ============================================================================
// Single condition entry (CTDA subrecord)
// ============================================================================
struct DialogueCondition {
    uint8_t runOnType = 0;        // 0=Subject, 1=Target, 2=Reference, 3=Combat Target
    float comparisonValue = 0.0f; // Value to compare against
    uint16_t functionIndex = 0;   // Condition function ID
    uint32_t param1 = 0;         // First parameter (FormID or value)
    uint32_t param2 = 0;         // Second parameter
    ConditionOp op = ConditionOp::Equal;
    uint32_t reference = 0;      // Run-on reference FormID

    bool evaluate(float actualValue) const;
};

// ============================================================================
// Response data (TRDT subrecord)
// ============================================================================
struct ResponseData {
    ResponseType type = ResponseType::Neutral;
    uint32_t emotionType = 0;     // 0=Neutral, 1=Anger, 2=Disgust, 3=Fear, 4=Sad, 5=Happy, 6=Surprise
    int32_t emotionValue = 0;     // Emotion magnitude (0-100)
    uint32_t speakerFormID = 0;   // Speaker reference
    uint32_t soundFormID = 0;     // Sound file FormID
    uint8_t useEmotionAnimation = 0;
    std::string responseText;     // NAM1 - NPC response text
    std::string promptText;       // NAM2 - Player prompt text
};

// ============================================================================
// Script variable condition (SCVR subrecord)
// ============================================================================
struct ScriptVariableCondition {
    uint32_t variableFormID = 0;  // Script FormID
    std::string variableName;     // Variable name
    uint8_t variableType = 0;     // 0=integer, 1=float
    float comparisonValue = 0.0f;
    ConditionOp op = ConditionOp::Equal;

    bool evaluate(float actualValue) const {
        switch (op) {
            case ConditionOp::Equal:        return actualValue == comparisonValue;
            case ConditionOp::NotEqual:     return actualValue != comparisonValue;
            case ConditionOp::GreaterThan:  return actualValue > comparisonValue;
            case ConditionOp::GreaterEqual: return actualValue >= comparisonValue;
            case ConditionOp::LessThan:     return actualValue < comparisonValue;
            case ConditionOp::LessEqual:    return actualValue <= comparisonValue;
            default: return false;
        }
    }
};

// ============================================================================
// Topic link (TCLT subrecord)
// ============================================================================
struct TopicLink {
    uint32_t topicFormID = 0;     // Linked topic FormID
    uint8_t linkType = 0;         // 0=normal, 1=choice
};

// ============================================================================
// Topic flag (TCLF subrecord)
// ============================================================================
struct TopicFlag {
    uint32_t flagFormID = 0;      // Flag FormID
    uint8_t flagValue = 0;        // Flag value
};

// ============================================================================
// Extended INFO Record - Full Oblivion dialogue info
// ============================================================================
struct DialogueInfoRecord {
    uint32_t formID = 0;
    std::string editorID;              // EDID
    uint32_t dialFormID = 0;           // NAME - parent DIAL FormID
    std::vector<ResponseData> responses; // RESP - response data
    std::string promptText;            // BNAM - player prompt (alternative)
    std::string responseText;          // GNAM - response text (alternative)
    uint32_t actorFormID = 0;          // ANAM - speaking actor FormID
    uint32_t infoCount = 0;            // TIFC - info count in topic
    std::vector<DialogueCondition> conditions; // CTDA conditions
    std::vector<ScriptVariableCondition> scriptConditions; // SCVR
    std::vector<TopicLink> topicLinks;  // TCLT
    std::vector<TopicFlag> topicFlags;  // TCLF
    uint32_t flags = 0;                // DATA flags
    uint8_t dialogType = 0;            // DATA dialog type
    uint32_t questFormID = 0;          // QSTI - linked quest
    int32_t questStage = -1;           // QSTN - required quest stage
    uint32_t factionFormID = 0;        // Faction condition
    int32_t factionRank = -1;          // Required faction rank

    // Filter conditions (decoded from CTDA)
    uint32_t filterNPCFormID = 0;      // NPC ID filter
    uint8_t filterGender = 0xFF;       // 0=Male, 1=Female, 0xFF=Any
    uint32_t filterRaceFormID = 0;     // Race filter
    uint32_t filterClassFormID = 0;    // Class filter
    float filterSkillMin = -1.0f;      // Minimum skill level
    float filterSkillMax = -1.0f;      // Maximum skill level
    uint16_t filterSkillID = 0xFFFF;   // Skill ID filter
    float filterLevelMin = -1.0f;      // Minimum level
    float filterLevelMax = -1.0f;      // Maximum level
    uint32_t filterFactionFormID = 0;  // Faction filter
    int32_t filterFactionRankMin = -1; // Minimum faction rank

    // Priority (higher = checked first)
    int32_t priority = 0;

    // Get the primary response text
    std::string getResponseText() const;

    // Get the primary prompt text
    std::string getPromptText() const;

    // Check if this INFO has quest linkage
    bool hasQuestLink() const { return questFormID != 0; }

    // Check if this INFO has faction requirement
    bool hasFactionRequirement() const { return factionFormID != 0; }
};

// ============================================================================
// Extended DIAL Record - Full dialogue topic
// ============================================================================
struct DialogueDialRecord {
    uint32_t formID = 0;
    std::string editorID;              // EDID
    std::string fullName;              // FULL
    DialogueType type = DialogueType::Topic;
    uint8_t flags = 0;
    std::vector<DialogueInfoRecord> infos; // Child INFO records

    // Get infos sorted by priority
    std::vector<const DialogueInfoRecord*> getInfosByPriority() const;

    // Get infos for a specific quest
    std::vector<const DialogueInfoRecord*> getInfosForQuest(uint32_t questFormID) const;
};

// ============================================================================
// Parser - Reads INFO subrecords from ESM
// ============================================================================
class DialogueRecordParser {
public:
    // Parse a single INFO record from raw ESM subrecords
    static DialogueInfoRecord parseInfoRecord(const ESMRecord& record);

    // Parse a DIAL record and its child INFO records
    static DialogueDialRecord parseDialRecord(const ESMRecord& record,
                                               const std::vector<ESMRecord>& infoRecords);

    // Parse all DIAL/INFO records from ESM
    static std::vector<DialogueDialRecord> parseAllDialogues(
        const std::vector<ESMRecord>& dialRecords,
        const std::vector<ESMRecord>& infoRecords);

    // Decode filter conditions from CTDA subrecords
    static void decodeConditions(DialogueInfoRecord& info, const ESMRecord& record);

private:
    // Parse CTDA condition subrecord
    static DialogueCondition parseCTDA(const SubRecord& sub);

    // Parse TRDT response subrecord
    static ResponseData parseTRDT(const SubRecord& sub);

    // Parse SCVR script variable condition
    static ScriptVariableCondition parseSCVR(const SubRecord& sub);

    // Parse TCLT topic link
    static TopicLink parseTCLT(const SubRecord& sub);

    // Parse TCLF topic flag
    static TopicFlag parseTCLF(const SubRecord& sub);

    // Decode filter conditions from parsed conditions
    static void decodeFilterConditions(DialogueInfoRecord& info);
};

} // namespace dialogue
} // namespace oblivion
