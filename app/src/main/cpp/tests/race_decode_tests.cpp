// RACE ATTR decode tests - see race_decode_tests.h.
//
// Synthetic ESM image in memory:
//   TES4 header
//   top-level GRUP (type 0, label 'RACE')
//     RACE 0x00000907 Imperial  EDID, ATTR 16B (real), FULL, DESC, MNAM, SPLO x2, DATA 36B
//     RACE 0x00023FE9 Argonian  EDID, ATTR 16B (real), DATA 36B
//     RACE 0x00001002 Distinct  EDID, ATTR 16B (all 8 differ M/F), FULL, DESC, SPLO, DATA 36B
//     RACE 0x00001000 Short15   EDID, ATTR 15B (one short), FULL, DESC
//     RACE 0x00001001 Over17    EDID, ATTR 17B (one over), FULL
//     RACE 0x00001003 NoAttr    EDID, FULL, DESC (no ATTR)
//
// V-664-2: ATTR is 16 bytes = 8 male u8 then 8 female u8 in AttrIndex order
// Strength..Luck, measured against Oblivion.esm on 2026-11-27 (all 15 records
// are exactly 16 bytes). Imperial and Argonian carry the real bytes, so those
// assertions double as a corpus check against UESP's per-race M/F table:
//   Imperial male  40 40 30 30 40 40 50 50 / female 40 40 40 30 30 40 50 50
//   Argonian male  40 40 30 50 50 30 30 50 / female 40 50 40 40 40 30 30 50
// "Distinct" is synthetic on purpose: every one of the 8 values differs between
// the sexes, so any male/female swap or off-by-one in the split is caught.
//
// Short15 / Over17 / NoAttr pin the guard paths and, crucially, the fields that
// live AFTER ATTR (FULL/DESC/MNAM/SPLO/DATA): decodeRace reads every field by
// 4-byte tag, so changing how much of ATTR is consumed must not move them.

#include "race_decode_tests.h"

#include "../assets/esm_reader.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace {

using oblivion::RaceData;

constexpr uint32_t kImperialForm = 0x00000907;
constexpr uint32_t kArgonianForm = 0x00023FE9;
constexpr uint32_t kDistinctForm = 0x00001002;
constexpr uint32_t kShortForm = 0x00001000;
constexpr uint32_t kOverForm = 0x00001001;
constexpr uint32_t kNoAttrForm = 0x00001003;

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

std::vector<uint8_t> u32Payload(uint32_t x) {
    std::vector<uint8_t> p;
    putU32(p, x);
    return p;
}

// The ATTR payload as measured in Oblivion.esm: 8 male u8 then 8 female u8.
std::vector<uint8_t> attrPayload(const std::vector<uint8_t>& male,
                                 const std::vector<uint8_t>& female) {
    std::vector<uint8_t> v(male);
    v.insert(v.end(), female.begin(), female.end());
    return v;
}

// DATA (36 bytes): 7 x (AV, bonus), 2 reserved, 4 floats, u32 flags (bit0 playable).
std::vector<uint8_t> dataPayload() {
    std::vector<uint8_t> v(36, 0);
    v[0] = 0x0C;  // skill AV Armorer, bonus 0
    v[32] = 0x01;  // flags bit0 = playable
    return v;
}

const std::vector<uint8_t> kImperialM = {40, 40, 30, 30, 40, 40, 50, 50};
const std::vector<uint8_t> kImperialF = {40, 40, 40, 30, 30, 40, 50, 50};
const std::vector<uint8_t> kArgonianM = {40, 40, 30, 50, 50, 30, 30, 50};
const std::vector<uint8_t> kArgonianF = {40, 50, 40, 40, 40, 30, 30, 50};
const std::vector<uint8_t> kDistinctM = {1, 2, 3, 4, 5, 6, 7, 8};
const std::vector<uint8_t> kDistinctF = {11, 12, 13, 14, 15, 16, 17, 18};

std::vector<uint8_t> buildEsm() {
    std::vector<uint8_t> body;

    // Imperial: real ATTR, and every other field placed AFTER it.
    {
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", stringPayload("Imperial"));
        putSub(payload, "ATTR", attrPayload(kImperialM, kImperialF));
        putSub(payload, "FULL", stringPayload("Imperial"));
        putSub(payload, "DESC", stringPayload("desc-imperial"));
        putSub(payload, "MNAM", stringPayload("Characters\\Imperial\\HeadHuman.nif"));
        putSub(payload, "SPLO", u32Payload(0x00012345));
        putSub(payload, "SPLO", u32Payload(0x00067890));
        putSub(payload, "DATA", dataPayload());
        putRecord(body, "RACE", kImperialForm, payload);
    }
    // Argonian: real ATTR only (corpus check for the M/F split).
    {
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", stringPayload("Argonian"));
        putSub(payload, "ATTR", attrPayload(kArgonianM, kArgonianF));
        putSub(payload, "DATA", dataPayload());
        putRecord(body, "RACE", kArgonianForm, payload);
    }
    // Distinct: all 8 values differ between the sexes.
    {
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", stringPayload("Distinct"));
        putSub(payload, "ATTR", attrPayload(kDistinctM, kDistinctF));
        putSub(payload, "FULL", stringPayload("Distinct"));
        putSub(payload, "DESC", stringPayload("after-attr-distinct"));
        putSub(payload, "SPLO", u32Payload(0x0000BEEF));
        putSub(payload, "DATA", dataPayload());
        putRecord(body, "RACE", kDistinctForm, payload);
    }
    // Short15: ATTR one byte short of the measured 16.
    {
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", stringPayload("Short15"));
        std::vector<uint8_t> shortAttr = attrPayload(kDistinctM, kDistinctF);
        shortAttr.resize(15);
        putSub(payload, "ATTR", shortAttr);
        putSub(payload, "FULL", stringPayload("Short15"));
        putSub(payload, "DESC", stringPayload("after-attr-short"));
        putRecord(body, "RACE", kShortForm, payload);
    }
    // Over17: ATTR one byte longer than 16; the extra byte must be dropped.
    {
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", stringPayload("Over17"));
        std::vector<uint8_t> overAttr = attrPayload(kDistinctM, kDistinctF);
        overAttr.push_back(0xEE);
        putSub(payload, "ATTR", overAttr);
        putSub(payload, "FULL", stringPayload("Over17"));
        putRecord(body, "RACE", kOverForm, payload);
    }
    // NoAttr: no ATTR subrecord at all.
    {
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", stringPayload("NoAttr"));
        putSub(payload, "FULL", stringPayload("NoAttr"));
        putSub(payload, "DESC", stringPayload("no-attr-desc"));
        putRecord(body, "RACE", kNoAttrForm, payload);
    }

    // Top-level race group: type 0, label 'RACE'.
    std::vector<uint8_t> esm;
    putTag(esm, "GRUP");
    putU32(esm, static_cast<uint32_t>(body.size() + 20));
    putTag(esm, "RACE");
    putU32(esm, 0);  // groupType 0 = top level
    esm.push_back(0); esm.push_back(0);  // stamp
    esm.push_back(0); esm.push_back(0);  // unknown
    esm.insert(esm.end(), body.begin(), body.end());

    // Prepend the TES4 header record (empty payload).
    std::vector<uint8_t> out;
    putRecord(out, "TES4", 0, {});
    out.insert(out.end(), esm.begin(), esm.end());
    return out;
}

const oblivion::RaceData* findRace(const oblivion::ESMFile& file, uint32_t formID) {
    for (const auto& r : file.getRaces()) {
        if (r.formID == formID) return &r;
    }
    return nullptr;
}

uint8_t at(const oblivion::RaceData& r, int i, bool female) {
    return r.attr(static_cast<oblivion::RaceData::AttrIndex>(i), female);
}

std::string attrsOf(const oblivion::RaceData& r, bool female) {
    std::string out;
    char buf[8];
    for (int i = 0; i < oblivion::RaceData::kAttrCount; ++i) {
        std::snprintf(buf, sizeof(buf), "%s%u", i ? "," : "",
                      static_cast<unsigned>(at(r, i, female)));
        out += buf;
    }
    return out;
}

bool attrsAre(const oblivion::RaceData& r, bool female,
              std::initializer_list<uint8_t> want) {
    if (want.size() != static_cast<size_t>(oblivion::RaceData::kAttrCount)) return false;
    int i = 0;
    for (uint8_t v : want) {
        if (at(r, i++, female) != v) return false;
    }
    return true;
}

bool attrsAllZero(const oblivion::RaceData& r, bool female) {
    for (int i = 0; i < oblivion::RaceData::kAttrCount; ++i) {
        if (at(r, i, female) != 0) return false;
    }
    return true;
}

std::string spellList(const oblivion::RaceData& r) {
    std::string out;
    char buf[16];
    for (size_t i = 0; i < r.spellFormIDs.size(); ++i) {
        std::snprintf(buf, sizeof(buf), "%s%08X", i ? "," : "", r.spellFormIDs[i]);
        out += buf;
    }
    return out;
}

}  // namespace

void RaceDecodeTests::record(const std::string& name, bool passed,
                             const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int RaceDecodeTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) ++n;
    return n;
}

int RaceDecodeTests::getFailCount() const {
    return static_cast<int>(results.size()) - getPassCount();
}

std::string RaceDecodeTests::getSummary() const {
    return "RACE Decode Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool RaceDecodeTests::runAllTests() {
    results.clear();
    const auto t0 = std::chrono::high_resolution_clock::now();

    std::vector<uint8_t> esm = buildEsm();
    oblivion::ESMFile file;
    const bool parsed = file.parseFromMemory("synthetic.esm", esm.data(), esm.size());
    record("ParseSyntheticEsm", parsed, parsed ? "parsed OK" : "parseFromMemory returned false");
    record("SixRacesDecoded", file.getRaces().size() == 6,
           "decoded " + std::to_string(file.getRaces().size()));

    const oblivion::RaceData* imperial = findRace(file, kImperialForm);
    const oblivion::RaceData* argonian = findRace(file, kArgonianForm);
    const oblivion::RaceData* distinct = findRace(file, kDistinctForm);
    const oblivion::RaceData* shortAttr = findRace(file, kShortForm);
    const oblivion::RaceData* overAttr = findRace(file, kOverForm);
    const oblivion::RaceData* noAttr = findRace(file, kNoAttrForm);

    // --- 16B normal: real Imperial (corpus) + all fields that follow ATTR ---
    if (imperial) {
        record("ImperialMaleAttrs", attrsAre(*imperial, false, {40, 40, 30, 30, 40, 40, 50, 50}),
               attrsOf(*imperial, false));
        record("ImperialFemaleAttrs", attrsAre(*imperial, true, {40, 40, 40, 30, 30, 40, 50, 50}),
               attrsOf(*imperial, true));
        record("ImperialFullNameAfterAttr", imperial->fullName == "Imperial", imperial->fullName);
        record("ImperialDescAfterAttr", imperial->description == "desc-imperial",
               imperial->description);
        record("ImperialMnamAfterAttr",
               imperial->maleModelPath == "Characters\\Imperial\\HeadHuman.nif",
               imperial->maleModelPath);
        record("ImperialSploAfterAttr",
               imperial->spellFormIDs.size() == 2 &&
                   imperial->spellFormIDs[0] == 0x00012345 &&
                   imperial->spellFormIDs[1] == 0x00067890,
               spellList(*imperial));
        record("ImperialFlagsAfterAttr", imperial->flags == 1u,
               "flags=" + std::to_string(imperial->flags));
    } else {
        record("ImperialFound", false, "missing");
    }

    // --- 16B normal: real Argonian (corpus) ---
    if (argonian) {
        record("ArgonianMaleAttrs", attrsAre(*argonian, false, {40, 40, 30, 50, 50, 30, 30, 50}),
               attrsOf(*argonian, false));
        record("ArgonianFemaleAttrs", attrsAre(*argonian, true, {40, 50, 40, 40, 40, 30, 30, 50}),
               attrsOf(*argonian, true));
    } else {
        record("ArgonianFound", false, "missing");
    }

    // --- 16B normal: Distinct, all 8 differ per sex (swap detection) ---
    if (distinct) {
        record("DistinctMaleAttrs", attrsAre(*distinct, false, {1, 2, 3, 4, 5, 6, 7, 8}),
               attrsOf(*distinct, false));
        record("DistinctFemaleAttrs", attrsAre(*distinct, true, {11, 12, 13, 14, 15, 16, 17, 18}),
               attrsOf(*distinct, true));
        record("DistinctFullNameAfterAttr", distinct->fullName == "Distinct", distinct->fullName);
        record("DistinctDescAfterAttr", distinct->description == "after-attr-distinct",
               distinct->description);
        record("DistinctSploAfterAttr",
               distinct->spellFormIDs.size() == 1 && distinct->spellFormIDs[0] == 0x0000BEEF,
               spellList(*distinct));
        record("DistinctFlagsAfterAttr", distinct->flags == 1u,
               "flags=" + std::to_string(distinct->flags));
    } else {
        record("DistinctFound", false, "missing");
    }

    // --- 15B short: whole ATTR ignored (both sexes 0), later fields intact ---
    if (shortAttr) {
        record("Short15MaleStaysDefault", attrsAllZero(*shortAttr, false),
               attrsOf(*shortAttr, false));
        record("Short15FemaleStaysDefault", attrsAllZero(*shortAttr, true),
               attrsOf(*shortAttr, true));
        record("Short15FullNameAfterAttr", shortAttr->fullName == "Short15",
               shortAttr->fullName);
        record("Short15DescAfterAttr", shortAttr->description == "after-attr-short",
               shortAttr->description);
    } else {
        record("Short15Found", false, "missing");
    }

    // --- 17B over: first 16 bytes read, extra dropped, later fields intact ---
    if (overAttr) {
        record("Over17MaleAttrs", attrsAre(*overAttr, false, {1, 2, 3, 4, 5, 6, 7, 8}),
               attrsOf(*overAttr, false));
        record("Over17FemaleAttrs", attrsAre(*overAttr, true, {11, 12, 13, 14, 15, 16, 17, 18}),
               attrsOf(*overAttr, true));
        record("Over17FullNameAfterAttr", overAttr->fullName == "Over17", overAttr->fullName);
    } else {
        record("Over17Found", false, "missing");
    }

    // --- no ATTR: all zero, later fields intact ---
    if (noAttr) {
        record("NoAttrMaleStaysDefault", attrsAllZero(*noAttr, false), attrsOf(*noAttr, false));
        record("NoAttrFemaleStaysDefault", attrsAllZero(*noAttr, true), attrsOf(*noAttr, true));
        record("NoAttrFullNameAfterAttr", noAttr->fullName == "NoAttr", noAttr->fullName);
        record("NoAttrDescAfterAttr", noAttr->description == "no-attr-desc", noAttr->description);
    } else {
        record("NoAttrFound", false, "missing");
    }

    // --- accessor guard: a cast-in out-of-range index reads 0 ---
    record("AttrOutOfRangeIsZero",
           imperial && imperial->attr(static_cast<oblivion::RaceData::AttrIndex>(99), false) == 0,
           imperial ? "out-of-range reads 0" : "Imperial missing");

    const auto t1 = std::chrono::high_resolution_clock::now();
    const float totalMs =
        std::chrono::duration<float, std::milli>(t1 - t0).count();
    if (!results.empty()) results.back().durationMs = totalMs;

    return getFailCount() == 0;
}
