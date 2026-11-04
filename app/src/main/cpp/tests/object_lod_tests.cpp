#include "object_lod_tests.h"

#include "../world/distant_lod/object_lod_rules.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

using object_lod::ObjectLodCell;
using object_lod::ObjectLodParseStats;

struct InstSpec {
    float px, py, pz;
    float rz;  // rot[2]; rot[0]/rot[1] are always zero in shipped data
    float scale;
};

double nowMs() {
    using Clock = std::chrono::steady_clock;
    static const Clock::time_point start = Clock::now();
    auto now = Clock::now();
    return std::chrono::duration<double, std::milli>(now - start).count();
}

void appendU32(std::vector<uint8_t>& out, uint32_t value) {
    uint8_t bytes[4];
    std::memcpy(bytes, &value, 4);
    out.insert(out.end(), bytes, bytes + 4);
}

void appendF32(std::vector<uint8_t>& out, float value) {
    uint8_t bytes[4];
    std::memcpy(bytes, &value, 4);
    out.insert(out.end(), bytes, bytes + 4);
}

// Builds one full variable-length record: formId + n + pos[3n] + rot[3n] +
// scale[n], matching the shipped layout verified in RE report v255 §1.
void appendRecord(std::vector<uint8_t>& out, uint32_t formId,
                  const std::vector<InstSpec>& instances) {
    appendU32(out, formId);
    appendU32(out, static_cast<uint32_t>(instances.size()));
    for (const InstSpec& inst : instances) {
        appendF32(out, inst.px);
        appendF32(out, inst.py);
        appendF32(out, inst.pz);
    }
    for (const InstSpec& inst : instances) {
        appendF32(out, 0.0f);  // rot[0]
        appendF32(out, 0.0f);  // rot[1]
        appendF32(out, inst.rz);
    }
    for (const InstSpec& inst : instances) {
        appendF32(out, inst.scale);
    }
}

std::vector<uint8_t> buildFile(uint32_t headerCount,
                               const std::vector<std::pair<uint32_t, std::vector<InstSpec>>>& records) {
    std::vector<uint8_t> data;
    appendU32(data, headerCount);
    for (const auto& rec : records) {
        appendRecord(data, rec.first, rec.second);
    }
    return data;
}

bool nearEq(float a, float b) {
    return std::fabs(a - b) <= 1e-5f * std::max(1.0f, std::fabs(b));
}

}  // namespace

bool ObjectLodTests::runAllTests() {
    results.clear();

    std::cout << "[ObjectLodTests] Running object LOD invariant tests..." << std::endl;

    testRejectsUndersizedBuffer();
    testRecordLayoutRoundTrip();
    testEofWalkAdoptsTrailingRecords();
    testPartialTailIsTrailingBytes();
    testInstanceCountBounds();
    testMalformedTailStopsWalk();
    testMultiRecordFile();
    testCellNameGrammar();
    testCellNameRejections();

    int pass = getPassCount();
    int fail = getFailCount();
    std::cout << "[ObjectLodTests] Completed: " << pass << " passed, " << fail
              << " failed" << std::endl;

    return fail == 0;
}

int ObjectLodTests::getPassCount() const {
    int count = 0;
    for (const auto& result : results) {
        if (result.passed) {
            ++count;
        }
    }
    return count;
}

int ObjectLodTests::getFailCount() const {
    return static_cast<int>(results.size()) - getPassCount();
}

std::string ObjectLodTests::getSummary() const {
    std::ostringstream out;
    out << "Object LOD Test Results\n";
    out << "Total: " << results.size() << " | Pass: " << getPassCount()
        << " | Fail: " << getFailCount();
    return out.str();
}

void ObjectLodTests::record(const std::string& name, bool passed, const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
    std::cout << "[ObjectLodTests] " << (passed ? "PASS" : "FAIL") << " " << name;
    if (!msg.empty()) {
        std::cout << " - " << msg;
    }
    std::cout << " (" << ms << " ms)" << std::endl;
}

void ObjectLodTests::testRejectsUndersizedBuffer() {
    const double t0 = nowMs();
    const std::string name = "RejectsUndersizedBuffer";

    bool allRejected = true;
    for (size_t size = 0; size < 4; ++size) {
        std::vector<uint8_t> data(size, 0xAB);
        ObjectLodCell cell;
        if (object_lod::parseObjectLodFile(data, cell)) {
            allRejected = false;
        }
    }

    // Exactly a u32 header must parse (as an empty cell).
    std::vector<uint8_t> headerOnly = {0x00, 0x00, 0x00, 0x00};
    ObjectLodCell cell;
    ObjectLodParseStats stats;
    const bool emptyOk = object_lod::parseObjectLodFile(headerOnly, cell, &stats);

    const bool pass = allRejected && emptyOk && cell.entries.empty() &&
                      stats.headerPresent && stats.headerCountMatches &&
                      stats.adoptedRecords == 0 && stats.trailingBytes == 0;
    record(name, pass, allRejected ? "" : "buffer shorter than the header parsed",
           static_cast<float>(nowMs() - t0));
}

void ObjectLodTests::testRecordLayoutRoundTrip() {
    const double t0 = nowMs();
    const std::string name = "RecordLayoutRoundTrip";

    const std::vector<InstSpec> specs = {
        {12345.0f, -6789.0f, 1000.0f, 1.5707964f, 101.0f},
        {22222.5f, 33333.25f, 44444.75f, -0.5f, 50.2f},
    };
    const std::vector<uint8_t> data = buildFile(1, {{0x00000D2Fu, specs}});

    ObjectLodCell cell;
    ObjectLodParseStats stats;
    bool ok = object_lod::parseObjectLodFile(data, cell, &stats);
    bool pass = ok && cell.entries.size() == 1 && stats.adoptedRecords == 1 &&
                stats.trailingBytes == 0 && stats.headerCountMatches;

    if (pass) {
        const auto& entry = cell.entries[0];
        pass = entry.formId == 0x00000D2Fu && entry.instances.size() == 2;
        for (size_t i = 0; i < 2 && pass; ++i) {
            const auto& inst = entry.instances[i];
            pass = nearEq(inst.position.x, specs[i].px) &&
                   nearEq(inst.position.y, specs[i].py) &&
                   nearEq(inst.position.z, specs[i].pz) &&
                   nearEq(inst.zRotation, specs[i].rz) &&
                   nearEq(inst.scale, specs[i].scale);
        }
    }
    record(name, pass, pass ? "" : "field offsets or yaw source wrong",
           static_cast<float>(nowMs() - t0));
}

void ObjectLodTests::testEofWalkAdoptsTrailingRecords() {
    const double t0 = nowMs();
    const std::string name = "EofWalkAdoptsTrailingRecords";

    // ~11% of shipped SI files carry extra records past the header count
    // (v255 §1); §54.8 item 2 adopts them all.
    const std::vector<InstSpec> one = {{1.0f, 2.0f, 3.0f, 0.25f, 100.0f}};
    const std::vector<uint8_t> data =
        buildFile(1, {{0x00000010u, one}, {0x00000020u, one}});

    ObjectLodCell cell;
    ObjectLodParseStats stats;
    const bool ok = object_lod::parseObjectLodFile(data, cell, &stats);
    const bool pass = ok && cell.entries.size() == 2 && stats.adoptedRecords == 2 &&
                      !stats.headerCountMatches && stats.headerCount == 1 &&
                      stats.trailingBytes == 0 &&
                      cell.entries[0].formId == 0x00000010u &&
                      cell.entries[1].formId == 0x00000020u;
    record(name, pass, pass ? "" : "trailing records not adopted",
           static_cast<float>(nowMs() - t0));
}

void ObjectLodTests::testPartialTailIsTrailingBytes() {
    const double t0 = nowMs();
    const std::string name = "PartialTailIsTrailingBytes";

    const std::vector<InstSpec> one = {{5.0f, 6.0f, 7.0f, 0.125f, 99.0f}};
    std::vector<uint8_t> data = buildFile(3, {{0x00000010u, one}, {0x00000020u, one}});
    // A third record header (formId + n=1) whose 36-byte body never arrives.
    appendU32(data, 0x00000030u);
    appendU32(data, 1u);

    ObjectLodCell cell;
    ObjectLodParseStats stats;
    const bool ok = object_lod::parseObjectLodFile(data, cell, &stats);
    const bool pass = ok && cell.entries.size() == 2 && stats.adoptedRecords == 2 &&
                      stats.trailingBytes == 8 && !stats.headerCountMatches;
    record(name, pass, pass ? "" : "partial tail not accounted as trailing bytes",
           static_cast<float>(nowMs() - t0));
}

void ObjectLodTests::testInstanceCountBounds() {
    const double t0 = nowMs();
    const std::string name = "InstanceCountBounds";

    const InstSpec spec{1.0f, 2.0f, 3.0f, 0.5f, 100.0f};
    const std::vector<InstSpec> one(1, spec);
    const std::vector<InstSpec> many(object_lod::kMaxInstancesPerRecord, spec);
    const std::vector<InstSpec> tooMany(object_lod::kMaxInstancesPerRecord + 1, spec);

    // n=1 and n=512 (sanity ceiling, far above the shipped max of 11) parse.
    ObjectLodCell cell1;
    ObjectLodParseStats stats1;
    const bool n1Ok = object_lod::parseObjectLodFile(buildFile(1, {{0x10u, one}}),
                                                     cell1, &stats1) &&
                      stats1.adoptedRecords == 1;

    ObjectLodCell cell512;
    ObjectLodParseStats stats512;
    const bool n512Ok = object_lod::parseObjectLodFile(buildFile(1, {{0x10u, many}}),
                                                       cell512, &stats512) &&
                         stats512.adoptedRecords == 1 &&
                         cell512.entries.size() == 1 &&
                         cell512.entries[0].instances.size() ==
                             object_lod::kMaxInstancesPerRecord;

    // n=513 exceeds the ceiling: the walk must stop with zero adoptions.
    std::vector<uint8_t> big = buildFile(1, {{0x10u, tooMany}});
    ObjectLodCell cellBad;
    ObjectLodParseStats statsBad;
    const bool n513Stopped = !object_lod::parseObjectLodFile(big, cellBad, &statsBad) ||
                             statsBad.adoptedRecords == 0;
    // (parse only returns false for undersized buffers, so the real check is
    // that the oversized record was rejected by the ceiling.)
    const bool rejected = cellBad.entries.empty() && statsBad.adoptedRecords == 0;

    // n=0 is invalid too.
    ObjectLodCell cellZero;
    ObjectLodParseStats statsZero;
    const bool zeroRejected =
        object_lod::parseObjectLodFile(buildFile(1, {{0x10u, {}}}), cellZero, &statsZero) &&
        statsZero.adoptedRecords == 0 && cellZero.entries.empty();

    const bool pass = n1Ok && n512Ok && rejected && zeroRejected;
    std::string msg;
    if (!pass) {
        std::ostringstream out;
        out << "n1=" << n1Ok << " n512=" << n512Ok << " n513=" << rejected
            << " n0=" << zeroRejected;
        msg = out.str();
    }
    record(name, pass, msg, static_cast<float>(nowMs() - t0));
}

void ObjectLodTests::testMalformedTailStopsWalk() {
    const double t0 = nowMs();
    const std::string name = "MalformedTailStopsWalk";

    const std::vector<InstSpec> one = {{1.0f, 2.0f, 3.0f, 0.25f, 100.0f}};
    std::vector<uint8_t> data = buildFile(2, {{0x00000010u, one}});
    // A formId==0 record (never valid) followed by a perfectly good record:
    // the walk stops at the bad record, so the good one is NOT adopted.
    appendRecord(data, 0u, one);
    appendRecord(data, 0x00000030u, one);

    ObjectLodCell cell;
    ObjectLodParseStats stats;
    const bool ok = object_lod::parseObjectLodFile(data, cell, &stats);
    const bool pass = ok && stats.adoptedRecords == 1 && cell.entries.size() == 1 &&
                      stats.trailingBytes == 36 + 36;
    record(name, pass, pass ? "" : "walk did not stop at the malformed record",
           static_cast<float>(nowMs() - t0));
}

void ObjectLodTests::testMultiRecordFile() {
    const double t0 = nowMs();
    const std::string name = "MultiRecordFile";

    // n=1, n=2 and n=11 — the shapes observed across the 921 shipped files.
    const std::vector<InstSpec> two = {
        {1.0f, 2.0f, 3.0f, 0.1f, 100.0f},
        {4.0f, 5.0f, 6.0f, 0.2f, 101.0f},
    };
    std::vector<InstSpec> eleven;
    for (int i = 0; i < 11; ++i) {
        eleven.push_back({static_cast<float>(i), 0.0f, 2.0f * i, 0.5f * i,
                          50.0f + i});
    }

    const std::vector<InstSpec> one = {{1.0f, 2.0f, 3.0f, 0.1f, 100.0f}};
    std::vector<uint8_t> data = buildFile(
        3, {{0x00000010u, one}, {0x00000020u, two}, {0x00000030u, eleven}});
    // Exact expected size: 4 + (8+28*1) + (8+28*2) + (8+28*11).
    const bool sizeOk = data.size() == 4u + 36u + 64u + 316u;

    ObjectLodCell cell;
    ObjectLodParseStats stats;
    const bool ok = object_lod::parseObjectLodFile(data, cell, &stats);
    bool pass = ok && sizeOk && stats.adoptedRecords == 3 && stats.headerCountMatches &&
                stats.trailingBytes == 0 && cell.entries.size() == 3;

    if (pass) {
        pass = cell.entries[0].formId == 0x00000010u && cell.entries[0].instances.size() == 1 &&
               cell.entries[1].formId == 0x00000020u && cell.entries[1].instances.size() == 2 &&
               cell.entries[2].formId == 0x00000030u && cell.entries[2].instances.size() == 11;
    }
    if (pass) {
        // Spot-check the last instance of the widest record for stride errors.
        const auto& last = cell.entries[2].instances[10];
        pass = nearEq(last.position.x, 10.0f) && nearEq(last.position.z, 20.0f) &&
               nearEq(last.zRotation, 5.0f) && nearEq(last.scale, 60.0f);
    }
    record(name, pass, pass ? "" : "multi-record layout mismatch",
           static_cast<float>(nowMs() - t0));
}

void ObjectLodTests::testCellNameGrammar() {
    const double t0 = nowMs();
    const std::string name = "CellNameGrammar";

    struct Case {
        const char* path;
        const char* worldspace;
        int32_t x;
        int32_t y;
    };
    const Case cases[] = {
        {"distantlod/Tamriel_0_0.lod", "Tamriel", 0, 0},
        {"distantlod\\seworld_-1_15.lod", "seworld", -1, 15},
        {"Tamriel_63_-8.lod", "Tamriel", 63, -8},
        {"DistantLOD/Tamriel_1_2.LOD", "Tamriel", 1, 2},
        {"distantlod/se_world_3_4.lod", "se_world", 3, 4},
    };

    bool pass = true;
    std::string failedCase;
    for (const Case& c : cases) {
        std::string worldspace;
        int32_t x = 12345, y = 12345;
        const bool ok = object_lod::parseCellName(c.path, worldspace, x, y);
        if (!ok || worldspace != c.worldspace || x != c.x || y != c.y) {
            pass = false;
            failedCase = c.path;
            break;
        }
    }
    record(name, pass, failedCase, static_cast<float>(nowMs() - t0));
}

void ObjectLodTests::testCellNameRejections() {
    const double t0 = nowMs();
    const std::string name = "CellNameRejections";

    const char* bad[] = {
        "Tamriel.lod",          // no coordinates
        "_1_2.lod",             // empty worldspace
        "Tamriel__2.lod",       // empty x field
        "Tamriel_a_b.lod",      // non-numeric
        "Tamriel_1_2x.lod",     // trailing junk in y
        "Tamriel_1_2",          // no extension
        ".lod",                 // dot at start
        "",                     // empty path
        "Tamriel_1_.lod",       // empty y field
        "Tamriel_99999999999_0.lod",  // out of int32 range
    };

    bool pass = true;
    std::string accepted;
    for (const char* path : bad) {
        std::string worldspace;
        int32_t x = 0, y = 0;
        if (object_lod::parseCellName(path, worldspace, x, y)) {
            pass = false;
            accepted = path;
            break;
        }
    }
    record(name, pass, accepted, static_cast<float>(nowMs() - t0));
}
