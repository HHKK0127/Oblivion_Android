#pragma once

// Phase 68: the rules of the shipped object LOD cells.
//
// The retail game ships per-cell object placement for the distant LOD as
// tiny binary files:
//
//   DistantLOD\<worldspace>_<cellX>_<cellY>.lod   (logical, engine printf form)
//   assets/distantlod/<worldspace>_<x>_<y>.lod    (shipped, lowercase folder)
//
// Unlike the landscape blocks these files carry no geometry at all. Each
// one is a count header followed by variable length placement records:
//
//   u32 count                     total number of records in the file
//   record: u32 formId            TESObjectSTAT reference (plugin 0)
//           u32 n                 instance count for this static (1..11 in
//                                 the shipped data)
//           float3 pos[n]         world absolute coordinates (cell * 4096)
//           float3 rot[n]         euler; rot.x and rot.y are always 0 in the
//                                 shipped files, so yaw is rot.z
//           float scale[n]        raw scale, observed 50.2..136.0
//
// A record spans 8 + 28 * n bytes. The engine reads exactly `count`
// records; 89% of the shipped files end at that boundary but the rest
// carry 8..380 trailing bytes of extra records (RE report v255 §1,
// porting ruling §54.8 item 2). This parser therefore walks to EOF and
// adopts every record that passes the size and formId sanity checks,
// which is a superset of what the engine draws.
//
// The constants mirror what the game data actually ships; they are not
// tunables. Like lod_block_rules.h, this header is pure and included by
// both the production provider and tests/object_lod_tests.cpp so the
// host suite exercises the exact code the device runs.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

#include <glm/glm.hpp>

namespace object_lod {

// Little-endian, as every other Oblivion file format.
constexpr size_t kRecordHeaderBytes = 8;    // formId + n
constexpr size_t kBytesPerInstance = 28;    // pos(12) + rot(12) + scale(4)

// The shipped files use n in 1..11. Anything above this bound means the
// stream is misaligned and the tail is dropped instead of trusted.
constexpr uint32_t kMaxInstancesPerRecord = 512;

// One replicated placement of one static.
struct ObjectLodInstance {
    glm::vec3 position{0.0f};
    float zRotation = 0.0f;
    float scale = 1.0f;
};

// All placements of one TESObjectSTAT inside one cell.
struct ObjectLodEntry {
    uint32_t formId = 0;
    std::vector<ObjectLodInstance> instances;
};

// The contents of one "<worldspace>_<x>_<y>.lod" file. The cell
// coordinates are parsed from the file name by the provider; the parser
// itself is position independent.
struct ObjectLodCell {
    int32_t cellX = 0;
    int32_t cellY = 0;
    std::vector<ObjectLodEntry> entries;
};

// What one parse pass observed, for counters and tests.
struct ObjectLodParseStats {
    uint32_t headerCount = 0;      // the u32 read from the file header
    size_t adoptedRecords = 0;     // records taken into the cell
    size_t trailingBytes = 0;      // bytes left after the last good record
    bool headerPresent = false;
    bool headerCountMatches = false;  // adoptedRecords == headerCount
};

inline uint32_t readU32Le(const uint8_t* p) {
    uint32_t v = 0;
    std::memcpy(&v, p, sizeof(v));
    return v;  // x86/ARM are little-endian; the format is too
}

inline float readF32Le(const uint8_t* p) {
    float v = 0.0f;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

// Parse the whole payload of one .lod file.
//
// Policy (RE report §54.8 item 2): walk past the header to EOF and adopt
// every record whose size fits the buffer and whose formId is non-zero.
// A record that fails either check ends the walk; the remaining bytes are
// reported as trailingBytes instead of being guessed at. Returns false
// only when the buffer cannot even hold the count header, so a caller can
// distinguish "not a .lod" from "a .lod with nothing to place".
inline bool parseObjectLodFile(const std::vector<uint8_t>& data,
                               ObjectLodCell& out,
                               ObjectLodParseStats* stats = nullptr) {
    ObjectLodParseStats local;
    if (data.size() < sizeof(uint32_t)) {
        if (stats) {
            *stats = local;
        }
        return false;
    }

    local.headerPresent = true;
    local.headerCount = readU32Le(data.data());

    const size_t size = data.size();
    size_t p = sizeof(uint32_t);
    while (p + kRecordHeaderBytes <= size) {
        const uint32_t formId = readU32Le(data.data() + p);
        const uint32_t n = readU32Le(data.data() + p + 4);

        if (formId == 0u || n == 0u || n > kMaxInstancesPerRecord) {
            break;
        }
        const size_t recordBytes = kRecordHeaderBytes + kBytesPerInstance * n;
        if (p + recordBytes > size) {
            break;
        }

        ObjectLodEntry entry;
        entry.formId = formId;
        entry.instances.reserve(n);
        const size_t posBase = p + kRecordHeaderBytes;
        const size_t rotBase = posBase + 12u * n;
        const size_t scaleBase = rotBase + 12u * n;
        for (uint32_t i = 0; i < n; ++i) {
            ObjectLodInstance inst;
            inst.position = glm::vec3(
                readF32Le(data.data() + posBase + 12u * i),
                readF32Le(data.data() + posBase + 12u * i + 4),
                readF32Le(data.data() + posBase + 12u * i + 8));
            // rot.x / rot.y are always zero in the shipped data; the draw
            // path only needs the yaw.
            inst.zRotation = readF32Le(data.data() + rotBase + 12u * i + 8);
            inst.scale = readF32Le(data.data() + scaleBase + 4u * i);
            entry.instances.push_back(inst);
        }
        out.entries.push_back(std::move(entry));
        ++local.adoptedRecords;
        p += recordBytes;
    }

    local.trailingBytes = size - p;
    local.headerCountMatches = (local.adoptedRecords == local.headerCount);
    if (stats) {
        *stats = local;
    }
    return true;
}

// Parse the "<worldspace>_<x>_<y>.lod" file name grammar used by the
// shipped archives (lowercase folder, signed decimal cell coordinates).
// Accepts a full path or a bare name. The worldspace itself (e.g.
// "Tamriel", "seworld") contains no separators, so the last two
// underscore-delimited fields of the stem are the coordinates.
inline bool parseCellName(const std::string& path,
                          std::string& worldspace,
                          int32_t& cellX,
                          int32_t& cellY) {
    const size_t slash = path.find_last_of("/\\");
    const std::string name =
        (slash == std::string::npos) ? path : path.substr(slash + 1);

    // Strip the ".lod" extension (case-insensitive on the last dot).
    const size_t dot = name.rfind('.');
    if (dot == std::string::npos || dot == 0) {
        return false;
    }
    const std::string stem = name.substr(0, dot);

    // Last two "_"-delimited fields are the coordinates.
    const size_t ySep = stem.rfind('_');
    if (ySep == std::string::npos || ySep + 1 >= stem.size()) {
        return false;
    }
    const size_t xSep = stem.rfind('_', ySep == 0 ? 0 : ySep - 1);
    if (xSep == std::string::npos || xSep == 0 || xSep + 1 >= ySep) {
        return false;
    }

    const std::string xText = stem.substr(xSep + 1, ySep - xSep - 1);
    const std::string yText = stem.substr(ySep + 1);
    worldspace = stem.substr(0, xSep);
    if (worldspace.empty()) {
        return false;
    }

    try {
        size_t usedX = 0;
        size_t usedY = 0;
        const long x = std::stol(xText, &usedX);
        const long y = std::stol(yText, &usedY);
        if (usedX != xText.size() || usedY != yText.size()) {
            return false;
        }
        cellX = static_cast<int32_t>(x);
        cellY = static_cast<int32_t>(y);
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

}  // namespace object_lod
