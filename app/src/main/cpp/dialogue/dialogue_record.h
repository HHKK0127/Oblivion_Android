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
// Condition function types (from CTDA subrecord).
// Values verified against the Oblivion.exe condition function table
// (0x00B0C8C0, stride 0x28, 169 cond-capable entries; RE_EXE_REPORT.md sec.58).
// ============================================================================
enum class ConditionFunction : uint16_t {
    GetDistance = 1,
    GetLocked = 5,
    GetPos = 6,
    GetAngle = 8,
    GetStartingPos = 10,
    GetStartingAngle = 11,
    GetSecondsPassed = 12,
    GetActorValue = 14,
    GetCurrentTime = 18,
    GetScale = 24,
    GetLineOfSight = 27,
    GetInSameCell = 32,
    GetDisabled = 35,
    MenuMode = 36,
    GetDisease = 39,
    GetVampire = 40,
    GetClothingValue = 41,
    SameFaction = 42,
    SameRace = 43,
    SameSex = 44,
    GetDetected = 45,
    GetDead = 46,
    GetItemCount = 47,
    GetGold = 48,
    GetSleeping = 49,
    GetTalkedToPC = 50,
    GetScriptVariable = 53,
    GetQuestRunning = 56,
    GetStage = 58,
    GetStageDone = 59,
    GetFactionRankDifference = 60,
    GetAlarmed = 61,
    IsRaining = 62,
    GetAttacked = 63,
    GetIsCreature = 64,
    GetLockLevel = 65,
    GetShouldAttack = 66,
    GetInCell = 67,
    GetIsClass = 68,
    GetIsRace = 69,
    GetIsSex = 70,
    GetInFaction = 71,
    GetIsID = 72,
    GetFactionRank = 73,
    GetGlobalValue = 74,
    IsSnowing = 75,
    GetDisposition = 76,
    GetRandomPercent = 77,
    GetQuestVariable = 79,
    GetLevel = 80,
    GetArmorRating = 81,
    GetDeadCount = 84,
    GetIsAlerted = 91,
    GetPlayerControlsDisabled = 98,
    GetHeadingAngle = 99,
    IsWeaponOut = 101,
    IsTorchOut = 102,
    IsShieldOut = 103,
    IsYielding = 104,
    IsFacingUp = 106,
    GetKnockedState = 107,
    GetWeaponAnimType = 108,
    GetWeaponSkillType = 109,
    GetCurrentAIPackage = 110,
    IsWaiting = 111,
    IsIdlePlaying = 112,
    GetCrimeGold = 116,
    GetCrime = 122,
    IsGuard = 125,
    CanPayCrimeGold = 127,
    GetFatiguePercentage = 128,
    GetPCIsClass = 129,
    GetPCIsRace = 130,
    GetPCIsSex = 131,
    GetPCInFaction = 132,
    SameFactionAsPC = 133,
    SameRaceAsPC = 134,
    SameSexAsPC = 135,
    GetIsReference = 136,
    IsTalking = 141,
    GetWalkSpeed = 142,
    GetCurrentAIProcedure = 143,
    GetTrespassWarningLevel = 144,
    IsTrespassing = 145,
    IsInMyOwnedCell = 146,
    GetWindSpeed = 147,
    GetCurrentWeatherPercent = 148,
    GetIsCurrentWeather = 149,
    IsContinuingPackagePCNear = 150,
    CanHaveFlames = 153,
    HasFlames = 154,
    GetOpenState = 157,
    GetSitting = 159,
    GetFurnitureMarkerID = 160,
    GetIsCurrentPackage = 161,
    IsCurrentFurnitureRef = 162,
    IsCurrentFurnitureObj = 163,
    GetDayOfWeek = 170,
    IsPlayerInJail = 171,
    GetTalkedToPCParam = 172,
    IsPCSleeping = 175,
    IsPCAMurderer = 176,
    GetDetectionLevel = 180,
    GetEquipped = 182,
    IsSwimming = 185,
    GetAmountSoldStolen = 190,
    GetPCExpelled = 193,
    GetPCFactionMurder = 195,
    GetPCFactionSteal = 197,
    GetPCFactionAttack = 199,
    GetPCFactionSubmitAuthority = 201,
    GetDestroyed = 203,
    HasMagicEffect = 214,
    GetDoorDefaultOpen = 215,
    IsSpellTarget = 223,
    GetIsPlayerBirthsign = 224,
    GetPersuasionNumber = 225,
    HasVampireFed = 227,
    GetIsClassDefault = 228,
    GetClassDefaultMatch = 229,
    GetInCellParam = 230,
    GetIsGhost = 237,
    GetUnconscious = 242,
    GetRestrained = 244,
    GetIsUsedItem = 246,
    GetIsUsedItemType = 247,
    GetPCFame = 249,
    GetPCInfamy = 251,
    GetIsPlayableRace = 254,
    GetOffersServicesNow = 255,
    GetUsedItemLevel = 258,
    GetUsedItemActivate = 259,
    GetBarterGold = 264,
    IsTimePassing = 265,
    IsPleasant = 266,
    IsCloudy = 267,
    GetArmorRatingUpperBody = 274,
    GetBaseActorValue = 277,
    IsOwner = 278,
    IsCellOwner = 280,
    IsHorseStolen = 282,
    IsLeftUp = 285,
    IsSneaking = 286,
    IsRunning = 287,
    GetFriendHit = 288,
    IsInCombat = 289,
    IsInInterior = 300,
    GetInvestmentGold = 305,
    IsActorUsingATorch = 306,
    IsXBox = 309,
    GetInWorldspace = 310,
    GetPCMiscStat = 312,
    IsActorEvil = 313,
    IsActorAVictim = 314,
    GetTotalPersuasionNumber = 315,
    GetIdleDoneOnce = 318,
    GetNoRumors = 320,
    WhichServiceMenu = 323,
    IsRidingHorse = 327,
    IsTurnArrest = 329,
    IsInDangerousWater = 332,
    GetIgnoreFriendlyHits = 338,
    IsPlayersLastRiddenHorse = 339,
    IsActor = 353,
    IsEssential = 354,
    IsPlayerMovingIntoNewSpace = 358,
    GetTimeDead = 361,
    GetPlayerHasLastRiddenHorse = 362,
    GetPlayerInSEWorld = 365,
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
    uint8_t runOnType = 0;        // 0=NPC subject, 1=player (CTDA flag swap bit)
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
    ResponseType type = ResponseType::Neutral;  // Set from DATA[0] by the integration path; not part of TRDT
    uint32_t emotionType = 0;     // TRDT[0] - 0=Neutral, 1=Anger, 2=Disgust, 3=Fear, 4=Sad, 5=Happy, 6=Surprise
    int32_t emotionValue = 0;     // TRDT[4] - emotion magnitude (0-100)
    uint8_t responseNumber = 0;   // TRDT[12] - 1-based ordinal of this response within the INFO
    uint32_t speakerFormID = 0;   // Not present in TRDT (measured over all 23,877 TRDT subrecords)
    uint32_t soundFormID = 0;     // Not present in TRDT (measured)
    uint8_t useEmotionAnimation = 0;  // Not present in TRDT (measured)
    std::string responseText;     // NAM1 - the line the NPC speaks
    std::string actingNotes;      // NAM2 - voice-actor direction, never player-visible text
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
    // NAM2 is a voice-acting direction for the performer, not player-facing
    // text. Keep it separate so it can never be shown as a prompt.
    std::string actingNotes;           // NAM2 - acting direction
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
