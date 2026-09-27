// XTEL (door intercept target) decode tests - see xtel_decode_tests.h.
//
// Builds a synthetic ESM image in memory:
//   TES4 header
//   cell-children GRUP (type 6, label = cell 0x0001D1F0)
//     ref 0x00001000  NAME + DATA + XTEL (28 bytes, target door 0x00001BAD)
//     ref 0x00001001  NAME + DATA (no XTEL)
//     ref 0x00001002  NAME + DATA + XTEL truncated to 20 bytes
//
// The type-6 GRUP label becomes each REFR's parentFormID, exactly as in the
// real master file, so the decoded references report it as cellFormID.

#include "xtel_decode_tests.h"

#include "../assets/esm_reader.h"

#include <chrono>
#include <cstring>
#include <cstdint>
#include <vector>

namespace {

constexpr uint32_t kCellForm = 0x0001D1F0;
constexpr uint32_t kRefWithXtel = 0x00001000;
constexpr uint32_t kRefNoXtel = 0x00001001;
constexpr uint32_t kRefTruncatedXtel = 0x00001002;
constexpr uint32_t kTargetDoor = 0x00001BAD;

void putU32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(static_cast<uint8_t>(x & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 8) & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
}

void putF32(std::vector<uint8_t>& v, float x) {
    uint32_t bits;
    std::memcpy(&bits, &x, 4);
    putU32(v, bits);
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

// NAME subrecord: 4-byte base object FormID.
std::vector<uint8_t> namePayload(uint32_t baseFormID) {
    std::vector<uint8_t> p;
    putU32(p, baseFormID);
    return p;
}

// DATA subrecord: 24 bytes of pos/rot (3 floats each).
std::vector<uint8_t> dataPayload(float px, float py, float pz) {
    std::vector<uint8_t> p;
    putF32(p, px); putF32(p, py); putF32(p, pz);
    putF32(p, 0.0f); putF32(p, 0.0f); putF32(p, 0.0f);
    return p;
}

// XTEL subrecord: target door FormID + 24 bytes pos/rot.
std::vector<uint8_t> xtelPayload(uint32_t targetDoor, float tx, float ty, float tz) {
    std::vector<uint8_t> p;
    putU32(p, targetDoor);
    putF32(p, tx); putF32(p, ty); putF32(p, tz);
    putF32(p, 0.0f); putF32(p, 0.0f); putF32(p, 0.0f);
    return p;
}

std::vector<uint8_t> buildEsm() {
    std::vector<uint8_t> body;

    // ref 0x1000: full 28-byte XTEL targeting 0x1BAD.
    {
        std::vector<uint8_t> payload;
        putSub(payload, "NAME", namePayload(0x00000223));
        putSub(payload, "DATA", dataPayload(100.0f, 200.0f, 300.0f));
        putSub(payload, "XTEL", xtelPayload(kTargetDoor, 10.0f, 20.0f, 30.0f));
        putRecord(body, "REFR", kRefWithXtel, payload);
    }
    // ref 0x1001: no XTEL.
    {
        std::vector<uint8_t> payload;
        putSub(payload, "NAME", namePayload(0x00000224));
        putSub(payload, "DATA", dataPayload(400.0f, 500.0f, 600.0f));
        putRecord(body, "REFR", kRefNoXtel, payload);
    }
    // ref 0x1002: XTEL truncated to 20 bytes (size 28 check must reject).
    {
        std::vector<uint8_t> payload;
        putSub(payload, "NAME", namePayload(0x00000225));
        putSub(payload, "DATA", dataPayload(700.0f, 800.0f, 900.0f));
        std::vector<uint8_t> truncated = xtelPayload(kTargetDoor, 0.0f, 0.0f, 0.0f);
        truncated.resize(20);
        putSub(payload, "XTEL", truncated);
        putRecord(body, "REFR", kRefTruncatedXtel, payload);
    }

    // cell-children GRUP (type 6), label = the owning cell's FormID.
    std::vector<uint8_t> esm;
    putTag(esm, "GRUP");
    putU32(esm, static_cast<uint32_t>(body.size() + 20));
    putU32(esm, kCellForm);   // label = cell FormID
    putU32(esm, 6);           // groupType 6 = cell children
    esm.push_back(0); esm.push_back(0);  // stamp
    esm.push_back(0); esm.push_back(0);  // unknown
    esm.insert(esm.end(), body.begin(), body.end());

    // Prepend the TES4 header record (empty payload).
    std::vector<uint8_t> out;
    putRecord(out, "TES4", 0, {});
    out.insert(out.end(), esm.begin(), esm.end());
    return out;
}

bool nearlyEqual(float a, float b) {
    return (a > b ? a - b : b - a) < 0.002f;
}

}  // namespace

void XtelDecodeTests::record(const std::string& name, bool passed,
                             const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int XtelDecodeTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int XtelDecodeTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string XtelDecodeTests::getSummary() const {
    return "XTEL Decode Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool XtelDecodeTests::runAllTests() {
    results.clear();
    const auto t0 = std::chrono::high_resolution_clock::now();

    std::vector<uint8_t> esm = buildEsm();
    oblivion::ESMFile file;
    const bool parsed = file.parseFromMemory("synthetic.esm", esm.data(), esm.size());

    record("ParseSyntheticEsm", parsed, parsed ? "parsed OK" : "parseFromMemory returned false");
    record("ThreeReferencesDecoded", file.getReferences().size() == 3,
           "decoded " + std::to_string(file.getReferences().size()));

    const oblivion::ReferenceData* refXtel = nullptr;
    const oblivion::ReferenceData* refNoXtel = nullptr;
    const oblivion::ReferenceData* refTruncated = nullptr;
    for (const auto& r : file.getReferences()) {
        if (r.formID == kRefWithXtel) refXtel = &r;
        else if (r.formID == kRefNoXtel) refNoXtel = &r;
        else if (r.formID == kRefTruncatedXtel) refTruncated = &r;
    }

    record("XtelReferenceFound", refXtel != nullptr, refXtel ? "found" : "missing");
    record("NoXtelReferenceFound", refNoXtel != nullptr, refNoXtel ? "found" : "missing");
    record("TruncatedXtelReferenceFound", refTruncated != nullptr,
           refTruncated ? "found" : "missing");

    if (refXtel) {
        record("XtelFlagSet", refXtel->hasXtel, refXtel->hasXtel ? "flagged" : "not flagged");
        record("XtelTargetFormID", refXtel->doorTargetFormID == kTargetDoor,
               "target=0x" + [&]() { char b[16]; std::snprintf(b, sizeof(b), "%08X", refXtel->doorTargetFormID); return std::string(b); }());
        const bool posOk = nearlyEqual(refXtel->doorTargetPos.x, 10.0f) &&
                           nearlyEqual(refXtel->doorTargetPos.y, 20.0f) &&
                           nearlyEqual(refXtel->doorTargetPos.z, 30.0f);
        record("XtelTargetPosDecoded", posOk,
               posOk ? "pos=(10,20,30)" : "pos mismatch");
        const bool rotOk = nearlyEqual(refXtel->doorTargetRot.x, 0.0f) &&
                           nearlyEqual(refXtel->doorTargetRot.y, 0.0f) &&
                           nearlyEqual(refXtel->doorTargetRot.z, 0.0f);
        record("XtelTargetRotDecoded", rotOk, rotOk ? "rot=(0,0,0)" : "rot mismatch");
        record("XtelParentCellFormID", refXtel->cellFormID == kCellForm,
               "cell=0x" + [&]() { char b[16]; std::snprintf(b, sizeof(b), "%08X", refXtel->cellFormID); return std::string(b); }());
    }

    if (refNoXtel) {
        record("NoXtelFlagClear", !refNoXtel->hasXtel,
               refNoXtel->hasXtel ? "wrongly flagged" : "correctly clear");
        record("NoXtelTargetZero", refNoXtel->doorTargetFormID == 0,
               "target must stay 0 without XTEL");
    }

    if (refTruncated) {
        // A truncate-to-20 XTEL must be rejected, not decoded and not fatal.
        record("TruncatedXtelRejected", !refTruncated->hasXtel,
               refTruncated->hasXtel ? "wrongly accepted" : "correctly rejected");
        record("TruncatedXtelTargetZero", refTruncated->doorTargetFormID == 0,
               "target must stay 0 for a rejected XTEL");
        record("TruncatedXtelKeepsPosition",
               nearlyEqual(refTruncated->position.x, 700.0f) &&
               nearlyEqual(refTruncated->position.y, 800.0f) &&
               nearlyEqual(refTruncated->position.z, 900.0f),
               "reference DATA must remain intact");
    }

    const auto t1 = std::chrono::high_resolution_clock::now();
    const float elapsedMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
    (void)elapsedMs;

    return getFailCount() == 0;
}