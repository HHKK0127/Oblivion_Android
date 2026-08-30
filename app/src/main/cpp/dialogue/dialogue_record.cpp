#include "dialogue_record.h"
#include <cstring>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "DialogueRecord"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {
namespace dialogue {

// ============================================================================
// DialogueCondition
// ============================================================================

bool DialogueCondition::evaluate(float actualValue) const {
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

// ============================================================================
// DialogueInfoRecord
// ============================================================================

std::string DialogueInfoRecord::getResponseText() const {
    // Prefer GNAM, then first RESP, then responseText
    if (!responseText.empty()) return responseText;
    if (!responses.empty() && !responses[0].responseText.empty()) {
        return responses[0].responseText;
    }
    return "";
}

std::string DialogueInfoRecord::getPromptText() const {
    // Prefer BNAM, then first RESP prompt, then promptText
    if (!promptText.empty()) return promptText;
    if (!responses.empty() && !responses[0].promptText.empty()) {
        return responses[0].promptText;
    }
    return "";
}

// ============================================================================
// DialogueDialRecord
// ============================================================================

std::vector<const DialogueInfoRecord*> DialogueDialRecord::getInfosByPriority() const {
    std::vector<const DialogueInfoRecord*> result;
    result.reserve(infos.size());
    for (const auto& info : infos) {
        result.push_back(&info);
    }
    std::sort(result.begin(), result.end(),
        [](const DialogueInfoRecord* a, const DialogueInfoRecord* b) {
            return a->priority > b->priority;
        });
    return result;
}

std::vector<const DialogueInfoRecord*> DialogueDialRecord::getInfosForQuest(uint32_t questFormID) const {
    std::vector<const DialogueInfoRecord*> result;
    for (const auto& info : infos) {
        if (info.questFormID == questFormID) {
            result.push_back(&info);
        }
    }
    return result;
}

// ============================================================================
// DialogueRecordParser
// ============================================================================

DialogueInfoRecord DialogueRecordParser::parseInfoRecord(const ESMRecord& record) {
    DialogueInfoRecord info;
    info.formID = record.formID;
    info.editorID = record.getString("EDID");

    // NAME - parent DIAL FormID
    info.dialFormID = record.getFormID("NAME");

    // BNAM - player prompt text
    info.promptText = record.getString("BNAM");

    // GNAM - response text
    info.responseText = record.getString("GNAM");

    // ANAM - speaking actor FormID
    info.actorFormID = record.getFormID("ANAM");

    // TIFC - info count
    {
        const SubRecord* sub = record.findSubRecord("TIFC");
        if (sub && sub->data.size() >= 4) {
            std::memcpy(&info.infoCount, sub->data.data(), 4);
        }
    }

    // DATA - flags and dialog type
    {
        const SubRecord* sub = record.findSubRecord("DATA");
        if (sub && sub->data.size() >= 1) {
            info.dialogType = sub->data[0];
            if (sub->data.size() >= 5) {
                std::memcpy(&info.flags, sub->data.data() + 1, 4);
            }
        }
    }

    // QSTI - quest FormID
    info.questFormID = record.getFormID("QSTI");

    // QSTN - quest stage
    {
        const SubRecord* sub = record.findSubRecord("QSTN");
        if (sub && sub->data.size() >= 4) {
            std::memcpy(&info.questStage, sub->data.data(), 4);
        }
    }

    // Parse CTDA conditions
    for (const auto& sub : record.subRecords) {
        if (std::memcmp(sub.tag, "CTDA", 4) == 0) {
            info.conditions.push_back(parseCTDA(sub));
        }
    }

    // Parse TRDT responses
    for (const auto& sub : record.subRecords) {
        if (std::memcmp(sub.tag, "TRDT", 4) == 0) {
            info.responses.push_back(parseTRDT(sub));
        }
    }

    // Parse SCVR script variable conditions
    for (const auto& sub : record.subRecords) {
        if (std::memcmp(sub.tag, "SCVR", 4) == 0) {
            info.scriptConditions.push_back(parseSCVR(sub));
        }
    }

    // Parse TCLT topic links
    for (const auto& sub : record.subRecords) {
        if (std::memcmp(sub.tag, "TCLT", 4) == 0) {
            info.topicLinks.push_back(parseTCLT(sub));
        }
    }

    // Parse TCLF topic flags
    for (const auto& sub : record.subRecords) {
        if (std::memcmp(sub.tag, "TCLF", 4) == 0) {
            info.topicFlags.push_back(parseTCLF(sub));
        }
    }

    // Decode filter conditions
    decodeFilterConditions(info);

    LOGD("Parsed INFO record: formID=0x%08X editorID=%s dialogType=%u responses=%zu conditions=%zu",
         info.formID, info.editorID.c_str(), info.dialogType,
         info.responses.size(), info.conditions.size());

    return info;
}

DialogueDialRecord DialogueRecordParser::parseDialRecord(
    const ESMRecord& record,
    const std::vector<ESMRecord>& infoRecords) {

    DialogueDialRecord dial;
    dial.formID = record.formID;
    dial.editorID = record.getString("EDID");
    dial.fullName = record.getString("FULL");

    // DATA - dialog type and flags
    {
        const SubRecord* sub = record.findSubRecord("DATA");
        if (sub && sub->data.size() >= 1) {
            dial.type = static_cast<DialogueType>(sub->data[0]);
            if (sub->data.size() >= 2) {
                dial.flags = sub->data[1];
            }
        }
    }

    // Collect child INFO records
    for (const auto& infoRec : infoRecords) {
        // Check if this INFO's NAME points to this DIAL
        uint32_t parentDialFormID = infoRec.getFormID("NAME");
        if (parentDialFormID == dial.formID) {
            dial.infos.push_back(parseInfoRecord(infoRec));
        }
    }

    LOGI("Parsed DIAL record: formID=0x%08X editorID=%s type=%d infos=%zu",
         dial.formID, dial.editorID.c_str(), static_cast<int>(dial.type),
         dial.infos.size());

    return dial;
}

std::vector<DialogueDialRecord> DialogueRecordParser::parseAllDialogues(
    const std::vector<ESMRecord>& dialRecords,
    const std::vector<ESMRecord>& infoRecords) {

    std::vector<DialogueDialRecord> result;
    result.reserve(dialRecords.size());

    for (const auto& dialRec : dialRecords) {
        result.push_back(parseDialRecord(dialRec, infoRecords));
    }

    LOGI("Parsed %zu DIAL records with %zu total INFO records",
         result.size(), infoRecords.size());

    return result;
}

void DialogueRecordParser::decodeConditions(DialogueInfoRecord& info, const ESMRecord& record) {
    // Re-parse conditions from raw record
    info.conditions.clear();
    for (const auto& sub : record.subRecords) {
        if (std::memcmp(sub.tag, "CTDA", 4) == 0) {
            info.conditions.push_back(parseCTDA(sub));
        }
    }
    decodeFilterConditions(info);
}

DialogueCondition DialogueRecordParser::parseCTDA(const SubRecord& sub) {
    DialogueCondition cond;
    if (sub.data.size() < 20) return cond;

    // CTDA format (Oblivion):
    // Byte 0: runOnType (0=Subject, 1=Target, 2=Reference, 3=Combat Target)
    // Bytes 1-3: padding
    // Bytes 4-7: comparison value (float)
    // Bytes 8-9: function index (uint16)
    // Bytes 10-11: padding
    // Bytes 12-15: param1 (uint32)
    // Bytes 16-19: param2 (uint32)
    // Byte 20: operator (0=Equal, 1=NotEqual, 2=GT, 3=GE, 4=LT, 5=LE)
    // Bytes 21-23: padding

    cond.runOnType = sub.data[0];
    std::memcpy(&cond.comparisonValue, sub.data.data() + 4, 4);
    std::memcpy(&cond.functionIndex, sub.data.data() + 8, 2);
    std::memcpy(&cond.param1, sub.data.data() + 12, 4);
    std::memcpy(&cond.param2, sub.data.data() + 16, 4);

    if (sub.data.size() >= 21) {
        cond.op = static_cast<ConditionOp>(sub.data[20]);
    }

    return cond;
}

ResponseData DialogueRecordParser::parseTRDT(const SubRecord& sub) {
    ResponseData resp;
    if (sub.data.size() < 8) return resp;

    // TRDT format:
    // Byte 0: response type
    // Bytes 1-3: padding
    // Bytes 4-7: emotion type (uint32)
    // Bytes 8-11: emotion value (int32)
    // Bytes 12-15: speaker FormID
    // Bytes 16-19: sound FormID
    // Byte 20: use emotion animation

    resp.type = static_cast<ResponseType>(sub.data[0]);
    if (sub.data.size() >= 8) {
        std::memcpy(&resp.emotionType, sub.data.data() + 4, 4);
    }
    if (sub.data.size() >= 12) {
        std::memcpy(&resp.emotionValue, sub.data.data() + 8, 4);
    }
    if (sub.data.size() >= 16) {
        std::memcpy(&resp.speakerFormID, sub.data.data() + 12, 4);
    }
    if (sub.data.size() >= 20) {
        std::memcpy(&resp.soundFormID, sub.data.data() + 16, 4);
    }
    if (sub.data.size() >= 21) {
        resp.useEmotionAnimation = sub.data[20];
    }

    return resp;
}

ScriptVariableCondition DialogueRecordParser::parseSCVR(const SubRecord& sub) {
    ScriptVariableCondition svc;
    if (sub.data.size() < 8) return svc;

    // SCVR format:
    // Bytes 0-3: variable FormID
    // Bytes 4-7: variable type + comparison
    std::memcpy(&svc.variableFormID, sub.data.data(), 4);
        if (sub.data.size() >= 5) {
        svc.variableType = sub.data[4];
        }
        // Fix: read comparisonValue from offset +5 (not +4 which overlaps variableType)
        if (sub.data.size() >= 9) {
            std::memcpy(&svc.comparisonValue, sub.data.data() + 5, 4);
        }

    return svc;
}

TopicLink DialogueRecordParser::parseTCLT(const SubRecord& sub) {
    TopicLink link;
    if (sub.data.size() >= 4) {
        std::memcpy(&link.topicFormID, sub.data.data(), 4);
    }
    if (sub.data.size() >= 5) {
        link.linkType = sub.data[4];
    }
    return link;
}

TopicFlag DialogueRecordParser::parseTCLF(const SubRecord& sub) {
    TopicFlag flag;
    if (sub.data.size() >= 4) {
        std::memcpy(&flag.flagFormID, sub.data.data(), 4);
    }
    if (sub.data.size() >= 5) {
        flag.flagValue = sub.data[4];
    }
    return flag;
}

void DialogueRecordParser::decodeFilterConditions(DialogueInfoRecord& info) {
    for (const auto& cond : info.conditions) {
        auto func = static_cast<ConditionFunction>(cond.functionIndex);

        switch (func) {
            case ConditionFunction::GetIsID:
                // param1 = NPC FormID
                info.filterNPCFormID = cond.param1;
                break;

            case ConditionFunction::GetIsSex:
            case ConditionFunction::GetPCIsSex:
                // 0=Male, 1=Female
                info.filterGender = static_cast<uint8_t>(cond.comparisonValue);
                break;

            case ConditionFunction::GetIsRace:
            case ConditionFunction::GetPCIsRace:
                info.filterRaceFormID = cond.param1;
                break;

            case ConditionFunction::GetIsClass:
            case ConditionFunction::GetPCIsClass:
                info.filterClassFormID = cond.param1;
                break;

            case ConditionFunction::GetInFaction:
            case ConditionFunction::GetPCInFaction:
                info.filterFactionFormID = cond.param1;
                break;

            case ConditionFunction::GetFactionRank:
                info.filterFactionRankMin = static_cast<int32_t>(cond.comparisonValue);
                break;

            case ConditionFunction::GetLevel:
                if (cond.op == ConditionOp::GreaterEqual || cond.op == ConditionOp::GreaterThan) {
                    info.filterLevelMin = cond.comparisonValue;
                } else if (cond.op == ConditionOp::LessEqual || cond.op == ConditionOp::LessThan) {
                    info.filterLevelMax = cond.comparisonValue;
                }
                break;

            case ConditionFunction::GetActorValue:
            case ConditionFunction::GetBaseActorValue:
                // param1 = skill/attribute ID
                info.filterSkillID = static_cast<uint16_t>(cond.param1);
                if (cond.op == ConditionOp::GreaterEqual || cond.op == ConditionOp::GreaterThan) {
                    info.filterSkillMin = cond.comparisonValue;
                } else if (cond.op == ConditionOp::LessEqual || cond.op == ConditionOp::LessThan) {
                    info.filterSkillMax = cond.comparisonValue;
                }
                break;

            case ConditionFunction::GetStage:
                info.questStage = static_cast<int32_t>(cond.comparisonValue);
                break;

            case ConditionFunction::GetQuestRunning:
                if (cond.comparisonValue > 0) {
                    info.questFormID = cond.param1;
                }
                break;

            default:
                break;
        }
    }

    // Set priority based on specificity
    info.priority = 0;
    if (info.filterNPCFormID != 0) info.priority += 100;
    if (info.filterGender != 0xFF) info.priority += 10;
    if (info.filterRaceFormID != 0) info.priority += 10;
    if (info.filterClassFormID != 0) info.priority += 10;
    if (info.filterFactionFormID != 0) info.priority += 20;
    if (info.filterSkillMin >= 0) info.priority += 5;
    if (info.questFormID != 0) info.priority += 50;
    if (info.questStage >= 0) info.priority += 30;
}

} // namespace dialogue
} // namespace oblivion
