// CLAS (class) DATA decode tests - see clas_decode_tests.h.
//
// Builds a synthetic ESM image in memory:
//   TES4 header
//   top-level GRUP (type 0, label 'CLAS')
//     CLAS 0x00023787 Warrior   DATA 52 bytes, copied from Oblivion.esm
//     CLAS 0x00023785 Priest    DATA 48 bytes, copied from Oblivion.esm
//     CLAS 0x00001000 Short     DATA truncated to 40 bytes
//     CLAS 0x00001001 NoData    no DATA subrecord at all
//     CLAS 0x00001002 BadSkill  DATA 52 bytes with an out-of-window skill id
//
// Warrior and Priest carry the real bytes of those two classes, so the
// assertions double as a corpus check: Warrior is Strength/Endurance with the
// seven combat skills and flags=0x01 (bit0 autocalcs), while Priest is
// Willpower/Personality with the seven magic skills and flags=0. The other
// three records are synthetic on purpose - they pin the guard paths, which no
// shipped record exercises.
//
// The original copies min(DATA size, 0x34) bytes into the zero-initialised
// form+0x38 (FUN_0046BDA0), so "Short" (40 bytes) must still decode the two
// attributes and all seven skills and leave only the flags byte at its default.

#include "clas_decode_tests.h"

#include "../assets/esm_reader.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace {

constexpr uint32_t kWarriorForm = 0x00023787;
constexpr uint32_t kPriestForm = 0x00023785;
constexpr uint32_t kShortForm = 0x00001000;
constexpr uint32_t kNoDataForm = 0x00001001;
constexpr uint32_t kBadSkillForm = 0x00001002;

void putU32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(static_cast<uint8_t>(x & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 8) & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
}

void putTag(std::vector<uint8_t>& v, const char* tag) {
    v.insert(v.end(), tag, tag + 4);
}

// A subrecord: tag(4) + size(2) + payload.
void putSub(std::vector<uint8_t>& v, const char* tag, const std::vector<uint8_t>& payload) {
    putTag(v, tag);
    v.push_back(static_cast<uint8_t>(payload.size() & 0xFF));
    v.push_back(static_cast<uint8_t>((payload.size() >> 8) & 0xFF));
    v.insert(v.end(), payload.begin(), payload.end());
}

// A record: type(4) + size(4) + flags(4) + formID(4) + version/unknown(4) + payload.
void putRecord(std::vector<uint8_t>& v, const char* type, uint32_t formID,
               const std::vector<uint8_t>& payload) {
    putTag(v, type);
    putU32(v, static_cast<uint32_t>(payload.size()));
    putU32(v, 0);  // flags: uncompressed
    putU32(v, formID);
    putU32(v, 0);  // version + unknown
    v.insert(v.end(), payload.begin(), payload.end());
}

std::vector<uint8_t> stringPayload(const char* text) {
    std::vector<uint8_t> p(text, text + std::strlen(text));
    p.push_back(0);
    return p;
}

// Warrior DATA as stored in Oblivion.esm (52 bytes): Strength/Endurance, the
// seven combat skills 0x0C..0x12, flags byte 0x01 (autocalcs), then the unread
// dword and the trainer byte.
std::vector<uint8_t> warriorData() {
    return {
        0x00, 0x00, 0x00, 0x00,   // +0x00 attribute 1 = Strength
        0x05, 0x00, 0x00, 0x00,   // +0x04 attribute 2 = Endurance
        0x00, 0x00, 0x00, 0x00,   // +0x08 unread
        0x0C, 0x00, 0x00, 0x00,   // +0x0C skills: Armorer
        0x0D, 0x00, 0x00, 0x00,   //        Athletics
        0x0E, 0x00, 0x00, 0x00,   //        Blade
        0x0F, 0x00, 0x00, 0x00,   //        Block
        0x10, 0x00, 0x00, 0x00,   //        Blunt
        0x11, 0x00, 0x00, 0x00,   //        HandToHand
        0x12, 0x00, 0x00, 0x00,   //        HeavyArmor
        0x01, 0x00, 0x00, 0x00,   // +0x28 flags = 0x01 (autocalcs), then unread
        0x00, 0x00, 0x00, 0x00,   // +0x2C unread
        0x00, 0x00, 0x00, 0x00    // +0x30 trainer index = 0 (Armorer)
    };
}

// Priest DATA as stored in Oblivion.esm (one of the 31 real 48-byte records,
// which stop after the unread dword): Willpower/Personality, skills 0x19/
// 0x14/0x18/0x15/0x10/0x20/0x13, flags 0.
std::vector<uint8_t> priestData48() {
    return {
        0x02, 0x00, 0x00, 0x00,   // +0x00 attribute 1 = Willpower
        0x06, 0x00, 0x00, 0x00,   // +0x04 attribute 2 = Personality
        0x01, 0x00, 0x00, 0x00,   // +0x08 unread
        0x19, 0x00, 0x00, 0x00,   // +0x0C skills: Restoration
        0x14, 0x00, 0x00, 0x00,   //        Alteration
        0x18, 0x00, 0x00, 0x00,   //        Mysticism
        0x15, 0x00, 0x00, 0x00,   //        Conjuration
        0x10, 0x00, 0x00, 0x00,   //        Blunt
        0x20, 0x00, 0x00, 0x00,   //        Speechcraft
        0x13, 0x00, 0x00, 0x00,   //        Alchemy
        0x00, 0x00, 0x00, 0x00,   // +0x28 flags = 0
        0x00, 0x00, 0x00, 0x00    // +0x2C unread
    };
}

std::vector<uint8_t> buildEsm() {
    std::vector<uint8_t> body;

    // Warrior: full 52-byte DATA.
    {
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", stringPayload("Warrior"));
        putSub(payload, "FULL", stringPayload("Warrior"));
        putSub(payload, "DESC", stringPayload("A master of arms."));
        putSub(payload, "DATA", warriorData());
        putRecord(body, "CLAS", kWarriorForm, payload);
    }
    // Priest: real 48-byte DATA (no trailing trainer byte).
    {
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", stringPayload("Priest"));
        putSub(payload, "FULL", stringPayload("Priest"));
        putSub(payload, "DATA", priestData48());
        putRecord(body, "CLAS", kPriestForm, payload);
    }
    // Short: Warrior's DATA cut to 40 bytes, i.e. the skills end exactly at
    // +0x27 and the flags byte at +0x28 is missing.
    {
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", stringPayload("Short"));
        std::vector<uint8_t> truncated = warriorData();
        truncated.resize(0x28);
        putSub(payload, "DATA", truncated);
        putRecord(body, "CLAS", kShortForm, payload);
    }
    // NoData: no DATA subrecord at all.
    {
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", stringPayload("NoData"));
        putRecord(body, "CLAS", kNoDataForm, payload);
    }
    // BadSkill: Warrior's DATA with the first skill id (0x30) outside the
    // 0x0C..0x20 window the original's setter accepts.
    {
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", stringPayload("BadSkill"));
        std::vector<uint8_t> bad = warriorData();
        bad[0x0C] = 0x30;
        putSub(payload, "DATA", bad);
        putRecord(body, "CLAS", kBadSkillForm, payload);
    }

    // Top-level class group: type 0, label 'CLAS'.
    std::vector<uint8_t> esm;
    putTag(esm, "GRUP");
    putU32(esm, static_cast<uint32_t>(body.size() + 20));
    putTag(esm, "CLAS");
    putU32(esm, 0);           // groupType 0 = top level
    esm.push_back(0); esm.push_back(0);  // stamp
    esm.push_back(0); esm.push_back(0);  // unknown
    esm.insert(esm.end(), body.begin(), body.end());

    // Prepend the TES4 header record (empty payload).
    std::vector<uint8_t> out;
    putRecord(out, "TES4", 0, {});
    out.insert(out.end(), esm.begin(), esm.end());
    return out;
}

std::string skillsToString(const oblivion::ClassData& c) {
    std::string out;
    char buf[8];
    for (int i = 0; i < 7; ++i) {
        std::snprintf(buf, sizeof(buf), "%s%u", i ? "," : "",
                      static_cast<unsigned>(c.majorSkills[i]));
        out += buf;
    }
    return out;
}

bool skillsAre(const oblivion::ClassData& c, std::initializer_list<uint8_t> want) {
    if (want.size() != 7) return false;
    size_t i = 0;
    for (uint8_t v : want) {
        if (c.majorSkills[i++] != v) return false;
    }
    return true;
}

std::string hexByte(uint8_t v) {
    char buf[8];
    std::snprintf(buf, sizeof(buf), "0x%02X", static_cast<unsigned>(v));
    return buf;
}

}  // namespace

void ClasDecodeTests::record(const std::string& name, bool passed,
                             const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int ClasDecodeTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int ClasDecodeTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string ClasDecodeTests::getSummary() const {
    return "CLAS Decode Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool ClasDecodeTests::runAllTests() {
    results.clear();
    const auto t0 = std::chrono::high_resolution_clock::now();

    std::vector<uint8_t> esm = buildEsm();
    oblivion::ESMFile file;
    const bool parsed = file.parseFromMemory("synthetic.esm", esm.data(), esm.size());

    record("ParseSyntheticEsm", parsed, parsed ? "parsed OK" : "parseFromMemory returned false");
    record("FiveClassesDecoded", file.getClasses().size() == 5,
           "decoded " + std::to_string(file.getClasses().size()));

    const oblivion::ClassData* warrior = nullptr;
    const oblivion::ClassData* priest = nullptr;
    const oblivion::ClassData* shortData = nullptr;
    const oblivion::ClassData* noData = nullptr;
    const oblivion::ClassData* badSkill = nullptr;
    for (const auto& c : file.getClasses()) {
        if (c.formID == kWarriorForm) warrior = &c;
        else if (c.formID == kPriestForm) priest = &c;
        else if (c.formID == kShortForm) shortData = &c;
        else if (c.formID == kNoDataForm) noData = &c;
        else if (c.formID == kBadSkillForm) badSkill = &c;
    }

    record("WarriorFound", warrior != nullptr, warrior ? "found" : "missing");
    record("PriestFound", priest != nullptr, priest ? "found" : "missing");
    record("ShortDataFound", shortData != nullptr, shortData ? "found" : "missing");
    record("NoDataFound", noData != nullptr, noData ? "found" : "missing");
    record("BadSkillFound", badSkill != nullptr, badSkill ? "found" : "missing");

    if (warrior) {
        record("WarriorEditorID", warrior->editorID == "Warrior", warrior->editorID);
        record("WarriorFullName", warrior->fullName == "Warrior", warrior->fullName);
        record("WarriorAttribute1",
               warrior->primaryAttribute1 == 0,
               "attr1=" + std::to_string(warrior->primaryAttribute1));
        record("WarriorAttribute2",
               warrior->primaryAttribute2 == 5,
               "attr2=" + std::to_string(warrior->primaryAttribute2));
        record("WarriorSkills", skillsAre(*warrior, {12, 13, 14, 15, 16, 17, 18}),
               "skills=" + skillsToString(*warrior));
        record("WarriorFlags", warrior->flags == 0x01, "flags=" + hexByte(warrior->flags));
    }

    if (priest) {
        record("PriestAttribute1", priest->primaryAttribute1 == 2,
               "attr1=" + std::to_string(priest->primaryAttribute1));
        record("PriestAttribute2", priest->primaryAttribute2 == 6,
               "attr2=" + std::to_string(priest->primaryAttribute2));
        record("PriestSkills48ByteVariant",
               skillsAre(*priest, {0x19, 0x14, 0x18, 0x15, 0x10, 0x20, 0x13}),
               "skills=" + skillsToString(*priest));
        record("PriestFlags", priest->flags == 0x00, "flags=" + hexByte(priest->flags));
    }

    if (shortData) {
        // 40 bytes cover the attributes and all seven skill dwords, so only the
        // flags byte at +0x28 is missing: the original's min-copy leaves it 0.
        record("ShortDataPartialFill",
               shortData->primaryAttribute1 == 0 && shortData->primaryAttribute2 == 5 &&
                   skillsAre(*shortData, {12, 13, 14, 15, 16, 17, 18}) &&
                   shortData->flags == 0,
               "attr1=" + std::to_string(shortData->primaryAttribute1) +
                   " attr2=" + std::to_string(shortData->primaryAttribute2) +
                   " skills=" + skillsToString(*shortData) +
                   " flags=" + hexByte(shortData->flags));
    }

    if (noData) {
        record("NoDataLeavesDefaults", noData->primaryAttribute1 == 0 &&
                                          noData->primaryAttribute2 == 0 &&
                                          noData->flags == 0 &&
                                          noData->majorSkills[0] == 0,
               "defaults preserved");
    }

    if (badSkill) {
        // 0x30 is outside 0x0C..0x20, so FUN_0051C0F0 would reject it; the slot
        // stays 0 and the six valid ids after it are unaffected.
        record("BadSkillRejected",
               skillsAre(*badSkill, {0, 13, 14, 15, 16, 17, 18}),
               "skills=" + skillsToString(*badSkill));
    }

    const auto t1 = std::chrono::high_resolution_clock::now();
    const float elapsedMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
    (void)elapsedMs;

    return getFailCount() == 0;
}
