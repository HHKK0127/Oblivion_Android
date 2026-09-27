// WATR (water) decode tests - see watr_decode_tests.h for the rationale.
//
// Builds a synthetic ESM image in memory:
//   TES4 header
//   WATR GRUP
//     DefaultWater  (formID 0x18)   DATA = 102 bytes, real colour block
//     Blood         (formID 0x90DDC) DATA = 42 bytes, no colour block
//     CamoranLava   (formID 0x3AFD3) DATA = 2 bytes,  no colour block
//
// Asserts that the short records inherit DefaultWater's colours (P8) and that
// the full record keeps its own colours unchanged.

#include "watr_decode_tests.h"

#include "../assets/esm_reader.h"

#include <chrono>
#include <cstring>
#include <cstdint>
#include <vector>

namespace {

constexpr uint32_t kFormDefaultWater = 0x00000018;
constexpr uint32_t kFormBlood = 0x00090DDC;
constexpr uint32_t kFormCamoranLava = 0x0003AFD3;

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

std::vector<uint8_t> edid(const char* name) {
    std::vector<uint8_t> p(name, name + std::strlen(name));
    p.push_back(0);
    return p;
}

// DATA payload of `size` bytes. The first 44 bytes are the float block; bytes
// 44/48/52 are the shallow/deep/reflection RGBA colours when size >= 55.
std::vector<uint8_t> dataPayload(size_t size, uint8_t shallow[3], uint8_t deep[3],
                                 uint8_t reflection[3]) {
    std::vector<uint8_t> d(size, 0);
    if (size >= 55) {
        d[44] = shallow[0]; d[45] = shallow[1]; d[46] = shallow[2];
        d[48] = deep[0];    d[49] = deep[1];    d[50] = deep[2];
        d[52] = reflection[0]; d[53] = reflection[1]; d[54] = reflection[2];
    }
    return d;
}

std::vector<uint8_t> buildEsm() {
    std::vector<uint8_t> body;

    // DefaultWater: full 102-byte DATA with a distinctive colour block.
    {
        uint8_t sh[3] = {2, 21, 30};
        uint8_t dp[3] = {32, 46, 53};
        uint8_t rf[3] = {31, 68, 75};
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", edid("DefaultWater"));
        putSub(payload, "ANAM", {100});
        putSub(payload, "DATA", dataPayload(102, sh, dp, rf));
        putRecord(body, "WATR", kFormDefaultWater, payload);
    }
    // Blood: 42-byte DATA, no colour block.
    {
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", edid("Blood"));
        putSub(payload, "ANAM", {100});
        putSub(payload, "DATA", dataPayload(42, nullptr, nullptr, nullptr));
        putRecord(body, "WATR", kFormBlood, payload);
    }
    // CamoranLava: 2-byte DATA, no colour block.
    {
        std::vector<uint8_t> payload;
        putSub(payload, "EDID", edid("CamoranLava"));
        putSub(payload, "ANAM", {95});
        putSub(payload, "DATA", {0xFF, 0xFF});
        putRecord(body, "WATR", kFormCamoranLava, payload);
    }

    // WATR GRUP: tag(4) + groupSize(4) + label(4) + groupType(4) + stamp(2) + unknown(2)
    std::vector<uint8_t> esm;
    putTag(esm, "GRUP");
    putU32(esm, static_cast<uint32_t>(body.size() + 20));
    putTag(esm, "WATR");
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

bool nearlyEqual(float a, float b) {
    return (a > b ? a - b : b - a) < 0.002f;
}

}  // namespace

void WatrDecodeTests::record(const std::string& name, bool passed,
                             const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int WatrDecodeTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int WatrDecodeTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string WatrDecodeTests::getSummary() const {
    return "WATR Decode Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool WatrDecodeTests::runAllTests() {
    results.clear();
    const auto t0 = std::chrono::high_resolution_clock::now();

    std::vector<uint8_t> esm = buildEsm();
    oblivion::ESMFile file;
    const bool parsed = file.parseFromMemory("synthetic.esm", esm.data(), esm.size());

    record("ParseSyntheticEsm", parsed, parsed ? "parsed OK" : "parseFromMemory returned false");

    const oblivion::WaterData* def = nullptr;
    const oblivion::WaterData* blood = nullptr;
    const oblivion::WaterData* lava = nullptr;
    for (const auto& w : file.getWaters()) {
        if (w.formID == kFormDefaultWater) def = &w;
        else if (w.formID == kFormBlood) blood = &w;
        else if (w.formID == kFormCamoranLava) lava = &w;
    }

    record("DefaultWaterFound", def != nullptr, def ? "found" : "missing");
    record("BloodFound", blood != nullptr, blood ? "found" : "missing");
    record("CamoranLavaFound", lava != nullptr, lava ? "found" : "missing");

    if (def) {
        // 32/255 = 0.1255, 46/255 = 0.1804, 53/255 = 0.2078
        const bool ok = nearlyEqual(def->deepColor[0], 32.0f / 255.0f) &&
                        nearlyEqual(def->deepColor[1], 46.0f / 255.0f) &&
                        nearlyEqual(def->deepColor[2], 53.0f / 255.0f);
        record("DefaultWaterKeepsOwnColour", ok,
               ok ? "deep=(32,46,53)/255" : "deep colour changed unexpectedly");
        record("DefaultWaterHasColorBlock", def->hasColorBlock,
               def->hasColorBlock ? "flagged" : "not flagged");
    }

    if (blood && def) {
        const bool ok = nearlyEqual(blood->deepColor[0], def->deepColor[0]) &&
                        nearlyEqual(blood->deepColor[1], def->deepColor[1]) &&
                        nearlyEqual(blood->deepColor[2], def->deepColor[2]);
        record("BloodInheritsDefaultWaterColour", ok,
               ok ? "deep colour copied from DefaultWater" : "deep colour not copied");
        record("BloodNotBlack", !(blood->deepColor[0] == 0.0f &&
                                  blood->deepColor[1] == 0.0f &&
                                  blood->deepColor[2] == 0.0f),
               "short record must not stay black");
        record("BloodHasNoColorBlock", !blood->hasColorBlock,
               blood->hasColorBlock ? "wrongly flagged" : "correctly flagged");
    }

    if (lava && def) {
        const bool ok = nearlyEqual(lava->deepColor[0], def->deepColor[0]) &&
                        nearlyEqual(lava->deepColor[1], def->deepColor[1]) &&
                        nearlyEqual(lava->deepColor[2], def->deepColor[2]);
        record("CamoranLavaInheritsDefaultWaterColour", ok,
               ok ? "deep colour copied from DefaultWater" : "deep colour not copied");
        record("CamoranLavaNotBlack", !(lava->deepColor[0] == 0.0f &&
                                        lava->deepColor[1] == 0.0f &&
                                        lava->deepColor[2] == 0.0f),
               "short record must not stay black");
    }

    const auto t1 = std::chrono::high_resolution_clock::now();
    const float totalMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
    if (!results.empty()) results.back().durationMs = totalMs;

    return getFailCount() == 0;
}
