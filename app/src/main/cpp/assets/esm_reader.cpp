#include "esm_reader.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>
#include <limits>
#include <zlib.h>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGE
#undef LOGI
#define LOG_TAG "ESMReader"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {

// Safe unaligned read helpers (avoid ARM SIGBUS from reinterpret_cast)
static inline uint32_t readU32(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
static inline int32_t  readI32(const uint8_t* p) { int32_t  v; std::memcpy(&v, p, 4); return v; }
static inline float    readF32(const uint8_t* p) { float    v; std::memcpy(&v, p, 4); return v; }
static inline uint16_t readU16(const uint8_t* p) { uint16_t v; std::memcpy(&v, p, 2); return v; }

// Case-insensitive prefix test (BSA and ESM store paths with differing case).
static inline bool startsWithNoCase(const std::string& value, const char* prefix) {
    size_t i = 0;
    for (; prefix[i] != '\0'; ++i) {
        if (i >= value.size()) return false;
        char a = value[i];
        char b = prefix[i];
        if (a >= 'A' && a <= 'Z') a = static_cast<char>(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = static_cast<char>(b - 'A' + 'a');
        if (a != b) return false;
    }
    return true;
}

// Oblivion encodes EFID (magic effect reference) as the 4-character MGEF
// editorID code rather than a formID. Decode it to a printable ASCII string.
static inline std::string efidCode(const uint8_t* p) {
    std::string code(reinterpret_cast<const char*>(p), 4);
    for (char& c : code) {
        if (c == '\0' || static_cast<unsigned char>(c) < 0x20 ||
            static_cast<unsigned char>(c) > 0x7E) {
            return std::string();
        }
    }
    return code;
}

// Oblivion encodes "magnitude"-style fields as either a float or a raw integer.
// Integer magnitudes serialize as denormal floats (e.g. 1 -> 1.4e-45), so fall
// back to the integer interpretation whenever the float value is not sane.
static inline float readFloatOrInt(const uint8_t* p) {
    float f = readF32(p);
    if (f == 0.0f || !std::isfinite(f) || std::fabs(f) < 1e-6f || std::fabs(f) > 1e9f) {
        return static_cast<float>(readI32(p));
    }
    return f;
}

// ============================================================================
// ESMRecord helpers
// ============================================================================

const SubRecord* ESMRecord::findSubRecord(const char* tag) const {
    for (const auto& sub : subRecords) {
        if (std::memcmp(sub.tag, tag, 4) == 0) {
            return &sub;
        }
    }
    return nullptr;
}

std::string ESMRecord::getString(const char* tag) const {
    auto* sub = findSubRecord(tag);
    if (!sub || sub->data.empty()) return "";
    // Null-terminated string inside data; bound the read to the subrecord size
    // so malformed or unterminated data cannot read past the buffer.
    size_t length = 0;
    while (length < sub->data.size() && sub->data[length] != 0) length++;
    return std::string(reinterpret_cast<const char*>(sub->data.data()), length);
}

uint32_t ESMRecord::getUint(const char* tag) const {
    auto* sub = findSubRecord(tag);
    if (!sub || sub->data.size() < 4) return 0;
    return readU32(sub->data.data());
}

int32_t ESMRecord::getInt(const char* tag) const {
    auto* sub = findSubRecord(tag);
    if (!sub || sub->data.size() < 4) return 0;
    return readI32(sub->data.data());
}

float ESMRecord::getFloat(const char* tag) const {
    auto* sub = findSubRecord(tag);
    if (!sub || sub->data.size() < 4) return 0.0f;
    return readF32(sub->data.data());
}

uint32_t ESMRecord::getFormID(const char* tag) const {
    auto* sub = findSubRecord(tag);
    if (!sub || sub->data.size() < 4) return 0;
    return readU32(sub->data.data());
}

// ============================================================================
// ESMFile implementation
// ============================================================================

// Aggregate SCPT variable-type census, emitted once per plugin. The per-script
// lines rotate out of the logcat ring buffer long before the load finishes, so
// the totals are what an APK run can actually be checked against. Measured on
// Oblivion.esm: 2,393 scripts / 7,266 variables / int 5,100 / float 1,170 /
// ref 996.
static void logScriptTypeCensus(const std::vector<script::ScriptData>& scripts) {
    uint32_t intVars = 0;
    uint32_t floatVars = 0;
    uint32_t refVars = 0;
    uint32_t totalVars = 0;
    for (const auto& script : scripts) {
        for (const auto& var : script.variables) {
            switch (var.type) {
                case script::ScriptValue::Type::Float: ++floatVars; break;
                case script::ScriptValue::Type::Ref:   ++refVars;   break;
                default:                               ++intVars;   break;
            }
            ++totalVars;
        }
    }
    LOGI("SCPT variable types: scripts=%zu vars=%u int=%u float=%u ref=%u",
         scripts.size(), totalVars, intVars, floatVars, refVars);
}

// Water/XCLW census for exterior cells. Classification:
//   (i)   XCLW present with a non-negative level -> drawable water surface
//   (ii)  XCLW present with a negative sentinel -> explicit "no water"
//   (iii) XCLW subrecord absent entirely
// decodeCell tallies these; this just logs the totals the APK run reports.
static void logWaterCensus(uint32_t withWater, uint32_t noWaterSentinel,
                           uint32_t noXclw) {
    LOGI("WaterCensus exterior cells: water=%u no_water_sentinel=%u no_xclw=%u "
         "total=%u",
         withWater, noWaterSentinel, noXclw,
         withWater + noWaterSentinel + noXclw);
}

bool ESMFile::open(const std::string& filePath) {
    m_filePath = filePath;
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to open ESM file: %s", filePath.c_str());
        return false;
    }

    // Extract file name
    size_t sep = filePath.find_last_of("/\\");
    m_fileName = (sep != std::string::npos) ? filePath.substr(sep + 1) : filePath;

    size_t ext = m_fileName.rfind('.');
    m_isMaster = (ext != std::string::npos &&
                  (m_fileName.substr(ext) == ".esm" ||
                   m_fileName.substr(ext) == ".ESM"));

    LOGD("Opening ESM file: %s (master=%s)", m_fileName.c_str(), m_isMaster ? "yes" : "no");

    // Phase 1: Build index of all records (just headers + offsets, no data loading)
    // This is memory-efficient: only stores ~32 bytes per record instead of full data
    ESMRecord header;
    if (!readRecordHeader(file, header)) {
        LOGE("Failed to read TES4 header in %s", filePath.c_str());
        return false;
    }

    if (std::memcmp(header.recType, "TES4", 4) != 0) {
        LOGE("Invalid ESM file: expected TES4 header, got %.4s", header.recType);
        return false;
    }

    // Index the TES4 header
    RecordIndex headerIdx;
    std::memcpy(headerIdx.recType, "TES4", 4);
    headerIdx.formID = header.formID;
    headerIdx.flags = header.flags;
    headerIdx.dataSize = header.dataSize;
    headerIdx.fileOffset = 0;
    headerIdx.compressed = (header.flags & REC_FLAG_COMPRESSED) != 0;
    m_recordIndex.push_back(headerIdx);

    // Seek to end of TES4 record data
    // TES4 record: 20-byte header + dataSize bytes of sub-records
    file.seekg(20 + header.dataSize, std::ios::beg);
    LOGD("TES4 header parsed. RecSize=%u, seekTo=%llu, actualPos=%llu",
         header.dataSize, (unsigned long long)(20 + header.dataSize), (unsigned long long)file.tellg());

    // Build index of all records in the file
    // Use a stack to track GRUP nesting and boundaries
    int recordCount = 0;
    int compressedCount = 0;
    int grupCount = 0;
    
    // Stack of GRUP end offsets (absolute file positions)
    std::vector<uint64_t> groupEndStack;
    // Parallel stack of (groupLabel, groupType) so records can be attributed to
    // their owning cell. LAND/PGRD/REFR records carry no cell FormID.
    std::vector<std::pair<uint32_t, uint32_t>> groupInfoStack;
    
    while (file && file.peek() != EOF) {
        uint64_t currentPos = file.tellg();
        
        // Pop expired GRUP levels
        while (!groupEndStack.empty() && currentPos >= groupEndStack.back()) {
            LOGD("  GRUP ended at offset %llu (stack size %zu)", (unsigned long long)currentPos, groupEndStack.size());
            groupEndStack.pop_back();
            if (!groupInfoStack.empty()) groupInfoStack.pop_back();
        }
        
        // Read 4-byte tag WITHOUT seekback to avoid position drift
        char tag[4];
        file.read(tag, 4);
        if (file.gcount() < 4) break;
        currentPos = currentPos;  // tag starts at original currentPos

        if (recordCount < 5 || grupCount < 5) {
            LOGD("  tag at offset %llu: '%c%c%c%c' hex=0x%02X%02X%02X%02X",
                 (unsigned long long)currentPos,
                 tag[0], tag[1], tag[2], tag[3],
                 (uint8_t)tag[0], (uint8_t)tag[1], (uint8_t)tag[2], (uint8_t)tag[3]);
        }

        if (tag[0] == 'G' && tag[1] == 'R' && tag[2] == 'U' && tag[3] == 'P') {
            // GRUP: already read recType(4). Read remaining 12 bytes: groupSize+groupLabel+groupType
            uint32_t groupSize, groupLabel, groupType;
            file.read(reinterpret_cast<char*>(&groupSize), 4);
            file.read(reinterpret_cast<char*>(&groupLabel), 4);
            file.read(reinterpret_cast<char*>(&groupType), 4);
            // Skip stamp(2) + unknown(2) = 4 bytes
            file.seekg(4, std::ios::cur);
            // Now at currentPos + 20 ✓
            
            uint64_t grupStart = currentPos;
            uint64_t grupEnd = grupStart + groupSize;
            groupEndStack.push_back(grupEnd);
            groupInfoStack.push_back({groupLabel, groupType});
            grupCount++;
            if (grupCount <= 8) {
                LOGD("  GRUP[%d] start=%llu end=%llu size=%u type=%u label=0x%08X stack=%zu pos=%llu",
                     grupCount, (unsigned long long)grupStart, (unsigned long long)grupEnd,
                     groupSize, groupType, groupLabel, groupEndStack.size(), (unsigned long long)file.tellg());
            }
        } else {
            // Record: already read tag(4). Read remaining 16 bytes: rawSize+flags+formID+version+unknown
            // Oblivion ESM record header is 20 bytes total (tag+size+flags+formID+version+unknown)
            uint32_t rawSize, flags, formID;
            uint16_t version, unknown16;
            file.read(reinterpret_cast<char*>(&rawSize), 4);
            file.read(reinterpret_cast<char*>(&flags), 4);
            file.read(reinterpret_cast<char*>(&formID), 4);
            file.read(reinterpret_cast<char*>(&version), 2);
            file.read(reinterpret_cast<char*>(&unknown16), 2);
            if (file.gcount() < 2) break;

            uint32_t dataSize = rawSize;
            bool compressed = (flags & REC_FLAG_COMPRESSED) != 0;
            uint64_t dataOffset = file.tellg();
            uint32_t decompSize = 0;

            if (compressed && dataSize >= 4) {
                // Read decompressed size (first 4 bytes of record data)
                file.read(reinterpret_cast<char*>(&decompSize), 4);
                // Skip to end of this record's data using absolute seek
                uint64_t nextRecord = dataOffset + dataSize;
                file.seekg(nextRecord, std::ios::beg);
            } else {
                // Skip non-compressed record data using absolute seek
                uint64_t nextRecord = dataOffset + dataSize;
                file.seekg(nextRecord, std::ios::beg);
            }

            uint64_t afterData = file.tellg();
            
            RecordIndex idx;
            std::memcpy(idx.recType, tag, 4);
            idx.formID = formID;
            idx.flags = flags;
            idx.dataSize = dataSize;
            idx.fileOffset = dataOffset;
            idx.compressed = compressed;
            idx.decompSize = decompSize;
            // Attribute the record to its owning cell when it sits inside a
            // cell-children GRUP (type 6). The GRUP label is the cell FormID.
            for (size_t g = groupInfoStack.size(); g-- > 0;) {
                if (groupInfoStack[g].second == 6) {
                    idx.parentFormID = groupInfoStack[g].first;
                    break;
                }
            }
            // Track the owning worldspace from the nearest WorldChildren GRUP
            // (type 1); its label is the WRLD FormID. CELL records carry no WRLD
            // subrecord, so this is the only way to group cells per worldspace.
            for (size_t g = groupInfoStack.size(); g-- > 0;) {
                if (groupInfoStack[g].second == 1) {
                    idx.worldspaceFormID = groupInfoStack[g].first;
                    break;
                }
            }
            m_recordIndex.push_back(idx);

            if (formID != 0) {
                m_formIDIndex[formID] = m_recordIndex.size() - 1;
            }

            recordCount++;
            if (compressed) compressedCount++;
            
            if (recordCount <= 10) {
                LOGD("  REC[%d]: %.4s formID=0x%08X size=%u comp=%d dataOff=%llu afterData=%llu",
                     recordCount, tag, formID, dataSize, compressed ? 1 : 0,
                     (unsigned long long)dataOffset, (unsigned long long)afterData);
                // Hex dump first 16 bytes of record header for debugging
                char hexDump[49];
                for (int h = 0; h < 4; h++) sprintf(hexDump + h*3, "%02X ", (uint8_t)tag[h]);
                hexDump[12] = 0;
                LOGD("    header hex: %s rawSize=0x%08X flags=0x%08X formID=0x%08X",
                     hexDump, rawSize, flags, formID);
            }
        }
    }

    LOGD("ESM index built: %d records, %d compressed", recordCount, compressedCount);

    // Phase 2: Decode only essential records (NPC_, CELL, WEAP, etc.)
    // Load and decode on-demand, one at a time, to keep memory usage low
    int decodedCount = 0;
    for (size_t i = 0; i < m_recordIndex.size(); i++) {
        const auto& idx = m_recordIndex[i];
        // Skip records we don't need to decode
        if (std::memcmp(idx.recType, "TES4", 4) == 0) continue;
        if (std::memcmp(idx.recType, "GRUP", 4) == 0) continue;

        // Only decode record types we care about
        bool shouldDecode = false;
        if (std::memcmp(idx.recType, "CELL", 4) == 0 ||
            std::memcmp(idx.recType, "NPC_", 4) == 0 ||
            std::memcmp(idx.recType, "CREA", 4) == 0 ||
            std::memcmp(idx.recType, "WEAP", 4) == 0 ||
            std::memcmp(idx.recType, "QUST", 4) == 0 ||
            std::memcmp(idx.recType, "DIAL", 4) == 0 ||
            std::memcmp(idx.recType, "INFO", 4) == 0 ||
            std::memcmp(idx.recType, "REFR", 4) == 0 ||
            std::memcmp(idx.recType, "ACHR", 4) == 0 ||
            std::memcmp(idx.recType, "ACRE", 4) == 0 ||
            std::memcmp(idx.recType, "LAND", 4) == 0 ||
            std::memcmp(idx.recType, "WRLD", 4) == 0 ||
            std::memcmp(idx.recType, "SPEL", 4) == 0 ||
            std::memcmp(idx.recType, "ENCH", 4) == 0 ||
            std::memcmp(idx.recType, "MGEF", 4) == 0 ||
            std::memcmp(idx.recType, "SKIL", 4) == 0 ||
            std::memcmp(idx.recType, "BSGN", 4) == 0 ||
            std::memcmp(idx.recType, "CONT", 4) == 0 ||
            std::memcmp(idx.recType, "LIGH", 4) == 0 ||
            std::memcmp(idx.recType, "STAT", 4) == 0 ||
            std::memcmp(idx.recType, "SOUN", 4) == 0 ||
            std::memcmp(idx.recType, "TREE", 4) == 0 ||
            std::memcmp(idx.recType, "FLOR", 4) == 0 ||
            std::memcmp(idx.recType, "ACTI", 4) == 0 ||
            std::memcmp(idx.recType, "APPA", 4) == 0 ||
            std::memcmp(idx.recType, "EYES", 4) == 0 ||
            std::memcmp(idx.recType, "HAIR", 4) == 0 ||
            std::memcmp(idx.recType, "CLMT", 4) == 0 ||
            std::memcmp(idx.recType, "REGN", 4) == 0 ||
            std::memcmp(idx.recType, "LVLI", 4) == 0 ||
            std::memcmp(idx.recType, "LVLC", 4) == 0 ||
            std::memcmp(idx.recType, "LVSP", 4) == 0 ||
            std::memcmp(idx.recType, "LVLN", 4) == 0 ||
            std::memcmp(idx.recType, "NAVM", 4) == 0 ||
            std::memcmp(idx.recType, "ARMO", 4) == 0 ||
            std::memcmp(idx.recType, "BOOK", 4) == 0 ||
            std::memcmp(idx.recType, "FACT", 4) == 0 ||
            std::memcmp(idx.recType, "RACE", 4) == 0 ||
            std::memcmp(idx.recType, "CLAS", 4) == 0 ||
            std::memcmp(idx.recType, "CLOT", 4) == 0 ||
            std::memcmp(idx.recType, "INGR", 4) == 0 ||
            std::memcmp(idx.recType, "ALCH", 4) == 0 ||
            std::memcmp(idx.recType, "MISC", 4) == 0 ||
            std::memcmp(idx.recType, "ROAD", 4) == 0 ||
            std::memcmp(idx.recType, "SCPT", 4) == 0 ||
            std::memcmp(idx.recType, "GMST", 4) == 0 ||
            std::memcmp(idx.recType, "GLOB", 4) == 0 ||
            std::memcmp(idx.recType, "DOOR", 4) == 0 ||
            std::memcmp(idx.recType, "PACK", 4) == 0 ||
            std::memcmp(idx.recType, "PGRD", 4) == 0 ||
            std::memcmp(idx.recType, "IDLE", 4) == 0 ||
            std::memcmp(idx.recType, "KEYM", 4) == 0 ||
            std::memcmp(idx.recType, "AMMO", 4) == 0 ||
            std::memcmp(idx.recType, "SGST", 4) == 0 ||
            std::memcmp(idx.recType, "SLGM", 4) == 0 ||
            std::memcmp(idx.recType, "FURN", 4) == 0 ||
            std::memcmp(idx.recType, "LTEX", 4) == 0 ||
            std::memcmp(idx.recType, "GRAS", 4) == 0 ||
            std::memcmp(idx.recType, "WATR", 4) == 0 ||
            std::memcmp(idx.recType, "WTHR", 4) == 0 ||
            std::memcmp(idx.recType, "CSTY", 4) == 0 ||
            std::memcmp(idx.recType, "LSCR", 4) == 0 ||
            std::memcmp(idx.recType, "EFSH", 4) == 0 ||
            std::memcmp(idx.recType, "ANIO", 4) == 0 ||
            std::memcmp(idx.recType, "SBSP", 4) == 0) {
            shouldDecode = true;
        }

        if (!shouldDecode) continue;

        // Load this record's data from file
        ESMRecord rec;
        std::memcpy(rec.recType, idx.recType, 4);
        rec.dataSize = idx.dataSize;
        rec.flags = idx.flags;
        rec.formID = idx.formID;
        rec.parentFormID = idx.parentFormID;
        rec.worldspaceFormID = idx.worldspaceFormID;

        file.seekg(idx.fileOffset);
        if (idx.compressed) {
            // Read compressed data
            uint32_t compSize = idx.dataSize - 4;
            std::vector<uint8_t> compressedData(compSize);
            file.seekg(4, std::ios::cur);  // Skip decompSize (already read)
            file.read(reinterpret_cast<char*>(compressedData.data()), compSize);

            // Decompress
            std::vector<uint8_t> decompressed(idx.decompSize);
            z_stream strm = {};
            // Oblivion stores record payloads as zlib streams (78 9C header).
            inflateInit2(&strm, 15 + 32);
            strm.next_in = compressedData.data();
            strm.avail_in = compSize;
            strm.next_out = decompressed.data();
            strm.avail_out = idx.decompSize;
            int ret = inflate(&strm, Z_FINISH);
            inflateEnd(&strm);

            if (ret != Z_STREAM_END) {
                LOGE("Decompression failed for record %.4s formID=0x%08X (zlib error %d)",
                     idx.recType, idx.formID, ret);
                continue;
            }

            // Parse subrecords from decompressed data
            size_t offset = 0;
            while (offset < decompressed.size()) {
                SubRecord sub;
                if (offset + 6 > decompressed.size()) break;
                std::memcpy(sub.tag, decompressed.data() + offset, 4);
                uint16_t subSize;
                std::memcpy(&subSize, decompressed.data() + offset + 4, 2);
                uint32_t subDataSize = subSize & 0xFFFF;
                offset += 6;
                if (offset + subDataSize > decompressed.size()) break;
                sub.data.resize(subDataSize);
                std::memcpy(sub.data.data(), decompressed.data() + offset, subDataSize);
                offset += subDataSize;
                rec.subRecords.push_back(std::move(sub));
            }
        } else {
            // Read non-compressed subrecords directly
            uint32_t bytesRead = 0;
            while (bytesRead < idx.dataSize) {
                SubRecord sub;
                if (!readSubRecord(file, sub)) break;
                rec.subRecords.push_back(std::move(sub));
                bytesRead = static_cast<uint32_t>(file.tellg()) - idx.fileOffset;
            }
        }

        decodeRecord(rec);
        decodedCount++;

        // Clear subrecords after decoding to free memory
        rec.subRecords.clear();

        if (decodedCount % 5000 == 0) {
            LOGD("  Decoded %d records...", decodedCount);
        }
    }

    file.close();
    LOGD("ESM file parsed: %zu cells, %zu NPCs, %zu weapons, %zu quests, %zu dialogs, "
         "%zu refs, %zu terrains",
         m_cells.size(), m_npcs.size(), m_weapons.size(), m_quests.size(), m_dialogs.size(),
         m_references.size(), m_terrains.size());
    LOGD("ESM secondary types: GMST=%zu GLOB=%zu DOOR=%zu PACK=%zu PGRD=%zu IDLE=%zu "
         "KEYM=%zu AMMO=%zu SGST=%zu SLGM=%zu FURN=%zu LTEX=%zu GRAS=%zu WATR=%zu "
         "WTHR=%zu CSTY=%zu LSCR=%zu EFSH=%zu ANIO=%zu SBSP=%zu",
         m_gameSettings.size(), m_globalVariables.size(), m_doors.size(), m_packages.size(),
         m_pathGrids.size(), m_idleAnimations.size(), m_keys.size(), m_ammo.size(),
         m_sigilStones.size(), m_soulGems.size(), m_furniture.size(),
         m_landscapeTextures.size(), m_grass.size(), m_waters.size(), m_weathers.size(),
         m_combatStyles.size(), m_loadScreens.size(), m_effectShaders.size(),
         m_animationObjects.size(), m_subspaces.size());

    // Terrain sanity check: how many LAND records produced a usable 33x33 grid,
    // plus the height range of the first one.
    {
        size_t validTerrains = 0;
        float minHeight = 0.0f;
        float maxHeight = 0.0f;
        bool haveSample = false;
        for (const auto& terrain : m_terrains) {
            if (!terrain.hasHeights()) continue;
            validTerrains++;
            if (!haveSample) {
                haveSample = true;
                const std::vector<float> sample = terrain.expandHeights();
                minHeight = maxHeight = sample[0];
                for (float h : sample) {
                    if (h < minHeight) minHeight = h;
                    if (h > maxHeight) maxHeight = h;
                }
            }
        }
        LOGD("ESM terrain: %zu LAND records, %zu with 33x33 heights, sample range %.1f .. %.1f",
             m_terrains.size(), validTerrains, minHeight, maxHeight);
    }

    logScriptTypeCensus(m_scripts);

    logWaterCensus(m_exteriorCellsWithWater, m_exteriorCellsNoWaterSentinel,
                   m_exteriorCellsNoXclw);

    resolveMagicReferences();

    return true;
}

bool ESMFile::readRecordHeader(std::ifstream& file, ESMRecord& rec) {
    file.read(reinterpret_cast<char*>(&rec.recType), 4);
    if (file.gcount() < 4) return false;

    uint32_t rawSize;
    uint16_t version, unknown16;
    file.read(reinterpret_cast<char*>(&rawSize), 4);  // dataSize
    file.read(reinterpret_cast<char*>(&rec.flags), 4);
    file.read(reinterpret_cast<char*>(&rec.formID), 4);
    file.read(reinterpret_cast<char*>(&version), 2);
    file.read(reinterpret_cast<char*>(&unknown16), 2);

    if (std::memcmp(rec.recType, "GRUP", 4) == 0) {
        // GRUP has a different structure — this shouldn't be called for GRUPs
        LOGE("readRecordHeader called on GRUP (%.4s), unexpected", rec.recType);
        return false;
    }

    bool compressed = (rec.flags & REC_FLAG_COMPRESSED) != 0;
    rec.dataSize = rawSize;

    LOGD("  RECORD: %.4s size=%u flags=0x%08X formID=0x%08X %s",
         rec.recType, rec.dataSize, rec.flags, rec.formID,
         compressed ? "(compressed)" : "");

    uint32_t recordsDataStart = static_cast<uint32_t>(file.tellg());

    if (compressed) {
        // Read compressed block: 4 bytes decompressed size + zlib stream
        uint32_t decompSize;
        file.read(reinterpret_cast<char*>(&decompSize), 4);

        // Remaining bytes in the record = compressed zlib data
        uint32_t compSize = rec.dataSize - 4;
        std::vector<uint8_t> compressedData(compSize);
        file.read(reinterpret_cast<char*>(compressedData.data()), compSize);

        // Decompress
        std::vector<uint8_t> decompressed(decompSize);
        z_stream strm = {};
        // Oblivion stores record payloads as zlib streams (78 9C header).
        inflateInit2(&strm, 15 + 32);
        strm.next_in = compressedData.data();
        strm.avail_in = compSize;
        strm.next_out = decompressed.data();
        strm.avail_out = decompSize;
        int ret = inflate(&strm, Z_FINISH);
        inflateEnd(&strm);

        if (ret != Z_STREAM_END) {
            LOGE("Decompression failed for record %.4s (zlib error %d)", rec.recType, ret);
            return false;
        }

        // Parse subrecords from decompressed data
        std::vector<uint8_t> buf = std::move(decompressed);
        size_t offset = 0;
        while (offset < buf.size()) {
            SubRecord sub;
            if (offset + 8 > buf.size()) break;
            std::memcpy(sub.tag, buf.data() + offset, 4);
            uint16_t subSize;
            std::memcpy(&subSize, buf.data() + offset + 4, 2);
            uint32_t subDataSize = subSize & 0xFFFF;
            offset += 6;
            if (offset + subDataSize > buf.size()) {
                LOGE("Subrecord data exceeds buffer");
                break;
            }
            sub.data.resize(subDataSize);
            std::memcpy(sub.data.data(), buf.data() + offset, subDataSize);
            offset += subDataSize;
            rec.subRecords.push_back(std::move(sub));
        }
    } else {
        // Parse subrecords directly from file
        uint32_t bytesRead = 0;
        while (bytesRead < rec.dataSize) {
            SubRecord sub;
            if (!readSubRecord(file, sub)) {
                LOGE("Failed to read subrecord");
                break;
            }
            rec.subRecords.push_back(std::move(sub));
            bytesRead = static_cast<uint32_t>(file.tellg()) - recordsDataStart;
        }
    }

    return true;
}

bool ESMFile::readSubRecord(std::ifstream& file, SubRecord& sub) {
    file.read(sub.tag, 4);
    if (file.gcount() < 4) return false;

    uint16_t subSize;
    file.read(reinterpret_cast<char*>(&subSize), 2);
    if (file.gcount() < 2) return false;

    uint32_t dataSize = subSize & 0xFFFF;  // Some records have upper bytes used for flags
    sub.data.resize(dataSize);

    if (dataSize > 0) {
        file.read(reinterpret_cast<char*>(sub.data.data()), dataSize);
        if (static_cast<uint32_t>(file.gcount()) < dataSize) return false;
    }

    return true;
}

bool ESMFile::readGroup(std::ifstream& file, GroupType groupType, uint32_t groupSize) {
    uint32_t start = static_cast<uint32_t>(file.tellg());
    uint32_t bytesRead = 0;
    int recordCount = 0;

    while (bytesRead < groupSize && file && file.peek() != EOF) {
        char peekTag[4];
        file.read(peekTag, 4);
        file.seekg(-4, std::ios::cur);

        if (peekTag[0] == 'G' && peekTag[1] == 'R' && peekTag[2] == 'U' && peekTag[3] == 'P') {
            // Nested GRUP
            GroupHeader gh;
            file.read(reinterpret_cast<char*>(&gh), sizeof(gh));

            if (!readGroup(file, static_cast<GroupType>(gh.groupType),
                           gh.groupSize - sizeof(gh))) {
                return false;
            }
            bytesRead = static_cast<uint32_t>(file.tellg()) - start;
        } else {
            // Regular record
            ESMRecord rec;
            if (!readRecordHeader(file, rec)) {
                LOGE("Failed to read record inside GRUP at offset %u", bytesRead);
                break;
            }

            decodeRecord(rec);
            recordCount++;
            bytesRead = static_cast<uint32_t>(file.tellg()) - start;
        }
    }

    return true;
}

void ESMFile::decodeRecord(const ESMRecord& rec) {
    if (std::memcmp(rec.recType, "CELL", 4) == 0) {
        decodeCell(rec);
    } else if (std::memcmp(rec.recType, "NPC_", 4) == 0) {
        decodeNPC(rec);
    } else if (std::memcmp(rec.recType, "CREA", 4) == 0) {
        decodeCreature(rec);
    } else if (std::memcmp(rec.recType, "WEAP", 4) == 0) {
        decodeWeapon(rec);
    } else if (std::memcmp(rec.recType, "QUST", 4) == 0) {
        decodeQuest(rec);
    } else if (std::memcmp(rec.recType, "DIAL", 4) == 0) {
        decodeDialog(rec);
    } else if (std::memcmp(rec.recType, "INFO", 4) == 0) {
        decodeInfo(rec);
    } else if (std::memcmp(rec.recType, "REFR", 4) == 0) {
        decodeReference(rec);
    } else if (std::memcmp(rec.recType, "ACHR", 4) == 0) {
        decodeActorReference(rec, 1);
    } else if (std::memcmp(rec.recType, "ACRE", 4) == 0) {
        decodeActorReference(rec, 2);
    } else if (std::memcmp(rec.recType, "LAND", 4) == 0) {
        decodeTerrain(rec);
    } else if (std::memcmp(rec.recType, "WRLD", 4) == 0) {
        decodeWorld(rec);
        } else if (std::memcmp(rec.recType, "SPEL", 4) == 0) {
            decodeSpell(rec);
        } else if (std::memcmp(rec.recType, "ENCH", 4) == 0) {
            decodeEnchantment(rec);
        } else if (std::memcmp(rec.recType, "MGEF", 4) == 0) {
            decodeMagicEffect(rec);
        } else if (std::memcmp(rec.recType, "SKIL", 4) == 0) {
            decodeSkill(rec);
        } else if (std::memcmp(rec.recType, "BSGN", 4) == 0) {
            decodeBirthsign(rec);
        } else if (std::memcmp(rec.recType, "CONT", 4) == 0) {
            decodeContainer(rec);
        } else if (std::memcmp(rec.recType, "LIGH", 4) == 0) {
            decodeLight(rec);
        } else if (std::memcmp(rec.recType, "STAT", 4) == 0) {
            decodeStatic(rec);
        } else if (std::memcmp(rec.recType, "SOUN", 4) == 0) {
            decodeSound(rec);
        } else if (std::memcmp(rec.recType, "TREE", 4) == 0) {
            decodeTree(rec);
        } else if (std::memcmp(rec.recType, "FLOR", 4) == 0) {
            decodeFlora(rec);
        } else if (std::memcmp(rec.recType, "ACTI", 4) == 0) {
            decodeActivator(rec);
        } else if (std::memcmp(rec.recType, "APPA", 4) == 0) {
            decodeApparatus(rec);
        } else if (std::memcmp(rec.recType, "EYES", 4) == 0) {
            decodeEyes(rec);
        } else if (std::memcmp(rec.recType, "HAIR", 4) == 0) {
            decodeHair(rec);
        } else if (std::memcmp(rec.recType, "CLMT", 4) == 0) {
            decodeClimate(rec);
        } else if (std::memcmp(rec.recType, "REGN", 4) == 0) {
            decodeRegion(rec);
        } else if (std::memcmp(rec.recType, "LVLI", 4) == 0 ||
                   std::memcmp(rec.recType, "LVLC", 4) == 0 ||
                   std::memcmp(rec.recType, "LVSP", 4) == 0 ||
                   std::memcmp(rec.recType, "LVLN", 4) == 0) {
            decodeLeveledList(rec);
        } else if (std::memcmp(rec.recType, "NAVM", 4) == 0) {
            decodeNavMesh(rec);
        } else if (std::memcmp(rec.recType, "ARMO", 4) == 0) {
            decodeArmor(rec);
        } else if (std::memcmp(rec.recType, "BOOK", 4) == 0) {
            decodeBook(rec);
        } else if (std::memcmp(rec.recType, "FACT", 4) == 0) {
            decodeFaction(rec);
        } else if (std::memcmp(rec.recType, "RACE", 4) == 0) {
            decodeRace(rec);
        } else if (std::memcmp(rec.recType, "CLAS", 4) == 0) {
            decodeClass(rec);
        } else if (std::memcmp(rec.recType, "CLOT", 4) == 0) {
            decodeClothing(rec);
        } else if (std::memcmp(rec.recType, "INGR", 4) == 0) {
            decodeIngredient(rec);
        } else if (std::memcmp(rec.recType, "ALCH", 4) == 0) {
            decodeAlchemy(rec);
        } else if (std::memcmp(rec.recType, "MISC", 4) == 0) {
            decodeMiscItem(rec);
        } else if (std::memcmp(rec.recType, "ROAD", 4) == 0) {
            decodeRoad(rec);
        } else if (std::memcmp(rec.recType, "SCPT", 4) == 0) {
            decodeScript(rec);
        } else if (std::memcmp(rec.recType, "GMST", 4) == 0) {
            decodeGameSetting(rec);
        } else if (std::memcmp(rec.recType, "GLOB", 4) == 0) {
            decodeGlobalVariable(rec);
        } else if (std::memcmp(rec.recType, "DOOR", 4) == 0) {
            decodeDoor(rec);
        } else if (std::memcmp(rec.recType, "PACK", 4) == 0) {
            decodePackage(rec);
        } else if (std::memcmp(rec.recType, "PGRD", 4) == 0) {
            decodePathGrid(rec);
        } else if (std::memcmp(rec.recType, "IDLE", 4) == 0) {
            decodeIdleAnimation(rec);
        } else if (std::memcmp(rec.recType, "KEYM", 4) == 0) {
            decodeKey(rec);
        } else if (std::memcmp(rec.recType, "AMMO", 4) == 0) {
            decodeAmmo(rec);
        } else if (std::memcmp(rec.recType, "SGST", 4) == 0) {
            decodeSigilStone(rec);
        } else if (std::memcmp(rec.recType, "SLGM", 4) == 0) {
            decodeSoulGem(rec);
        } else if (std::memcmp(rec.recType, "FURN", 4) == 0) {
            decodeFurniture(rec);
        } else if (std::memcmp(rec.recType, "LTEX", 4) == 0) {
            decodeLandscapeTexture(rec);
        } else if (std::memcmp(rec.recType, "GRAS", 4) == 0) {
            decodeGrass(rec);
        } else if (std::memcmp(rec.recType, "WATR", 4) == 0) {
            decodeWater(rec);
        } else if (std::memcmp(rec.recType, "WTHR", 4) == 0) {
            decodeWeather(rec);
        } else if (std::memcmp(rec.recType, "CSTY", 4) == 0) {
            decodeCombatStyle(rec);
        } else if (std::memcmp(rec.recType, "LSCR", 4) == 0) {
            decodeLoadScreen(rec);
        } else if (std::memcmp(rec.recType, "EFSH", 4) == 0) {
            decodeEffectShader(rec);
        } else if (std::memcmp(rec.recType, "ANIO", 4) == 0) {
            decodeAnimationObject(rec);
        } else if (std::memcmp(rec.recType, "SBSP", 4) == 0) {
            decodeSubspace(rec);
        }
    // Other types are not decoded yet
}

void ESMFile::decodeCell(const ESMRecord& rec) {
    CellData cell;
    cell.formID = rec.formID;
    cell.editorID = rec.getString("EDID");

    // Determine if interior or exterior
    auto* dataSub = rec.findSubRecord("DATA");
    if (dataSub && dataSub->size() >= 1) {
        // Oblivion CELL DATA is 1 byte of flags; bit 0 set means interior.
        // There are no grid coordinates here, so this is only used as a
        // cross-check against the GRUP-derived classification below.
        const bool dataSaysInterior = (dataSub->data[0] & 0x01) != 0;
        if (dataSaysInterior && rec.worldspaceFormID != 0) {
            LOGD("decodeCell: 0x%08X flagged interior but inside worldspace 0x%08X",
                 rec.formID, rec.worldspaceFormID);
        }
    }

    cell.fullName = rec.getString("FULL");
    // Exterior cells live inside a WorldChildren GRUP whose label is the
    // worldspace FormID. Interior cells are never inside one, so a non-zero
    // worldspace FormID is what distinguishes the two.
    cell.worldspaceID = rec.worldspaceFormID;
    cell.isExterior = (rec.worldspaceFormID != 0);

    // XCLC subrecord for grid coordinates (TES4 specific)
    auto* xclc = rec.findSubRecord("XCLC");
    if (xclc && xclc->size() >= 8) {
        cell.gridX = readI32(xclc->data.data());
        cell.gridY = readI32(xclc->data.data() + 4);
    }

    // XCLW subrecord: water level height in world units (TES4 specific).
    // Classic Oblivion encodes "no water" as a large negative sentinel
    // (e.g. -2000 / -4000 for most of Tamriel) and real water surfaces as
    // positive heights (roughly 100..7000). Map the sentinel to NaN so
    // hasWaterLevel alone no longer implies a drawable surface; only a
    // positive height does.
    auto* xclw = rec.findSubRecord("XCLW");
    if (xclw && xclw->size() >= 4) {
        cell.waterLevel = readF32(xclw->data.data());
        cell.hasWaterLevel = true;
        if (cell.waterLevel < 0.0f) {
            cell.waterLevel = std::numeric_limits<float>::quiet_NaN();
            if (cell.isExterior) ++m_exteriorCellsNoWaterSentinel;
        } else if (cell.isExterior) {
            ++m_exteriorCellsWithWater;
        }
    } else if (cell.isExterior) {
        ++m_exteriorCellsNoXclw;
    }

    // XCWT subrecord: WATR FormID for this cell's water type. Without it the
    // cell falls back to the worldspace default water type at render time.
    auto* xcwt = rec.findSubRecord("XCWT");
    if (xcwt && xcwt->size() >= 4) {
        cell.waterTypeFormID = readU32(xcwt->data.data());
    }

    m_cells.push_back(std::move(cell));
}

void ESMFile::decodeNPC(const ESMRecord& rec) {
    NPCData npc;
    npc.formID = rec.formID;
    npc.editorID = rec.getString("EDID");
    npc.fullName = rec.getString("FULL");
    npc.race = rec.getString("RNAM");  // Race name from string table
    npc.raceID = rec.getFormID("RNAM");
    npc.className = rec.getString("CNAM");

    auto* acbs = rec.findSubRecord("ACBS");
    if (acbs && acbs->size() >= 16) {
        npc.level = acbs->data[4];  // Level is byte 4 (0-indexed)
    }

    npc.health = static_cast<uint32_t>(10 * npc.level + 50);
    npc.stamina = static_cast<uint32_t>(10 * npc.level + 50);
    npc.magicka = static_cast<uint32_t>(10 * npc.level + 50);

    // Parse AIDT (AI Data) subrecord
    auto* aidt = rec.findSubRecord("AIDT");
    if (aidt && aidt->size() >= 8) {
        npc.aggression = aidt->data[0];      // Aggression (0-100)
        npc.confidence = aidt->data[1];      // Confidence (0-100)
        npc.energy = aidt->data[2];          // Energy (0-100)
        npc.responsibility = aidt->data[3];  // Responsibility (0-100)
        npc.mood = aidt->data[4];            // Mood (0-8)
        npc.aiFlags = aidt->data[5];         // AI flags
    }

    // Parse AI packages (AI_A, AI_E, AI_F, AI_T, AI_W, AI_PK)
    // Oblivion stores up to 8 AI packages per NPC
    for (const auto& sub : rec.subRecords) {
        AIPackageData pkg;

        if (std::memcmp(sub.tag, "AI_A", 4) == 0) {
            // AI Activate/Find
            pkg.type = AIPackageType::FIND;
            if (sub.size() >= 16) {
                pkg.targetFormID = readU32(sub.data.data());
                pkg.idleTime = sub.data[4];
            }
        } else if (std::memcmp(sub.tag, "AI_E", 4) == 0) {
            // AI Escort
            pkg.type = AIPackageType::ESCORT;
            if (sub.size() >= 16) {
                pkg.targetFormID = readU32(sub.data.data());
                pkg.targetX = readF32(sub.data.data() + 4);
                pkg.targetY = readF32(sub.data.data() + 8);
                pkg.targetZ = readF32(sub.data.data() + 12);
            }
        } else if (std::memcmp(sub.tag, "AI_F", 4) == 0) {
            // AI Follow
            pkg.type = AIPackageType::FOLLOW;
            if (sub.size() >= 16) {
                pkg.targetFormID = readU32(sub.data.data());
                pkg.targetX = readF32(sub.data.data() + 4);
                pkg.targetY = readF32(sub.data.data() + 8);
                pkg.targetZ = readF32(sub.data.data() + 12);
            }
        } else if (std::memcmp(sub.tag, "AI_T", 4) == 0) {
            // AI Travel
            pkg.type = AIPackageType::TRAVEL;
            if (sub.size() >= 12) {
                pkg.targetX = readF32(sub.data.data());
                pkg.targetY = readF32(sub.data.data() + 4);
                pkg.targetZ = readF32(sub.data.data() + 8);
            }
        } else if (std::memcmp(sub.tag, "AI_W", 4) == 0) {
            // AI Wander
            pkg.type = AIPackageType::WANDER;
            if (sub.size() >= 8) {
                pkg.idleTime = sub.data[0];
                pkg.wanderDistance = sub.data[1];
            }
        } else if (std::memcmp(sub.tag, "AI_PK", 4) == 0) {
            // AI Package (newer format with more data)
            if (sub.size() >= 20) {
                pkg.type = static_cast<AIPackageType>(sub.data[0]);
                pkg.flags = sub.data[1];
                pkg.scheduleDay = sub.data[2];
                pkg.scheduleHour = sub.data[3];
                pkg.scheduleDuration = sub.data[4];
                pkg.targetFormID = readU32(sub.data.data() + 8);
                pkg.locationFormID = readU32(sub.data.data() + 12);
                pkg.targetX = readF32(sub.data.data() + 16);
            }
        } else {
            continue;  // Not an AI package subrecord
        }

        npc.aiPackages.push_back(std::move(pkg));
    }

    m_npcs.push_back(std::move(npc));
}

void ESMFile::decodeCreature(const ESMRecord& rec) {
    CreatureData creature;
    creature.formID = rec.formID;
    creature.editorID = rec.getString("EDID");
    creature.fullName = rec.getString("FULL");
    creature.modelPath = rec.getString("MODL");

    auto* acbs = rec.findSubRecord("ACBS");
    if (acbs && acbs->size() >= 16) {
        creature.level = acbs->data[4];  // Level is byte 4
    }

    // Calculate stats based on level
    creature.health = static_cast<uint32_t>(10 * creature.level + 50);
    creature.attackDamage = static_cast<uint32_t>(5 + creature.level * 2);

    // Get combat/magic/stealth from DATA subrecord if present
    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 12) {
        creature.combat = readU16(data->data.data());
        creature.magic = readU16(data->data.data() + 2);
        creature.stealth = readU16(data->data.data() + 4);
    }

    // Soul level from SOUL subrecord
    creature.soulLevel = rec.getFormID("SOUL");

    // Faction from SNAM
    creature.factionID = rec.getFormID("SNAM");

    // Template from TNAM
    creature.templateFormID = rec.getFormID("TNAM");

    m_creatures.push_back(std::move(creature));
}

void ESMFile::decodeWeapon(const ESMRecord& rec) {
    WeaponData wpn;
    wpn.formID = rec.formID;
    wpn.editorID = rec.getString("EDID");
    wpn.fullName = rec.getString("FULL");

    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 16) {
        // Skip 2 bytes (type), read damage at byte 2
        wpn.damage = readU16(data->data.data() + 2);
        // Read value at byte 12
        wpn.value = readU32(data->data.data() + 12);
    } else {
        auto* data2 = rec.findSubRecord("DNAM");
        if (data2 && data2->size() >= 8) {
            wpn.damage = readU16(data2->data.data());
            wpn.value = readU32(data2->data.data() + 4);
        }
    }

    wpn.weight = rec.getFloat("WNAM");
    if (wpn.weight <= 0.0f) wpn.weight = 5.0f;

    m_weapons.push_back(std::move(wpn));
}

void ESMFile::decodeQuest(const ESMRecord& rec) {
    QuestData qst;
    qst.formID = rec.formID;
    qst.editorID = rec.getString("EDID");
    qst.fullName = rec.getString("FULL");

    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 1) {
        qst.flags = data->data[0];
    }

    m_quests.push_back(std::move(qst));
}

void ESMFile::decodeDialog(const ESMRecord& rec) {
    DialogData dia;
    dia.formID = rec.formID;
    dia.editorID = rec.getString("EDID");
    dia.fullName = rec.getString("FULL");

    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 1) {
        dia.dialogType = data->data[0];
        if (data->size() >= 2) {
            dia.flags = data->data[1];
        }
    }

    m_dialogs.push_back(std::move(dia));
    m_lastDialFormID = rec.formID;  // Track for child INFO records
}

void ESMFile::decodeInfo(const ESMRecord& rec) {
    InfoData info;
    info.formID = rec.formID;
    info.editorID = rec.getString("EDID");
    info.dialFormID = m_lastDialFormID;

    // NAM1 = response text (NPC says this)
    info.responseText = rec.getString("NAM1");

    // NAM2 = prompt text (player says this, optional)
    info.promptText = rec.getString("NAM2");

    // DATA = info response data
    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 4) {
        info.responseType = data->data[0];
        // flags at bytes 1-3
        std::memcpy(&info.flags, data->data.data() + 1, 3);
    }

    // TRDT = speaker/trigger data (12 bytes: emotion type, emotion value, response number, speaker)
    auto* trdt = rec.findSubRecord("TRDT");
    if (trdt && trdt->size() >= 12) {
        std::memcpy(&info.speakerFormID, trdt->data.data() + 8, 4);
    }

    // ANAM = faction FormID condition
    info.factionFormID = rec.getFormID("ANAM");

    // CNAM = faction rank condition
    auto* cnam = rec.findSubRecord("CNAM");
    if (cnam && cnam->size() >= 4) {
        std::memcpy(&info.factionRank, cnam->data.data(), 4);
    }

    // QSTI = linked quest FormID
    info.questFormID = rec.getFormID("QSTI");

    // QSTN = required quest stage
    auto* qstn = rec.findSubRecord("QSTN");
    if (qstn && qstn->size() >= 4) {
        std::memcpy(&info.questStage, qstn->data.data(), 4);
    }

    // Attach to parent DIAL
    for (auto& dia : m_dialogs) {
        if (dia.formID == m_lastDialFormID) {
            dia.infos.push_back(std::move(info));
            return;
        }
    }

    // Orphan INFO — store standalone (shouldn't happen in valid ESM)
    LOGD("Orphan INFO 0x%08X (no parent DIAL 0x%08X)", rec.formID, m_lastDialFormID);
}

// ============================================================================
// Reference (REFR) decoding — placed objects/NPCs in cells
// ============================================================================

void ESMFile::decodeReference(const ESMRecord& rec) {
    ReferenceData ref;
    ref.formID = rec.formID;
    ref.baseFormID = rec.getFormID("NAME");  // Base object formID

    // Position + rotation from DATA subrecord (24 bytes: 3 floats pos, 3 floats rot)
    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 24) {
        ref.position.x = readF32(data->data.data());
        ref.position.y = readF32(data->data.data() + 4);
        ref.position.z = readF32(data->data.data() + 8);
        ref.rotation.x = readF32(data->data.data() + 12);
        ref.rotation.y = readF32(data->data.data() + 16);
        ref.rotation.z = readF32(data->data.data() + 20);
    }

    // Scale
    auto* xsca = rec.findSubRecord("XSCL");
    if (xsca && xsca->size() >= 4) {
        ref.scale = readF32(xsca->data.data());
    }

    // Cell formID from XRGD (or infer from surrounding context)
    auto* xrgd = rec.findSubRecord("XRGD");
    // Also XRGB, XRDS etc. — not critical for basic placement

    // Owning cell formID from the enclosing cell-children GRUP label. Needed to
    // tell exterior references (absolute world position) from interior ones
    // (position relative to the cell origin).
    ref.cellFormID = rec.parentFormID;

    m_references.push_back(std::move(ref));
}

// ============================================================================
// Actor reference (ACHR/ACRE) decoding — placed actors in cells.
// Layout matches REFR: NAME = base actor formID, DATA = pos/rot (24 bytes).
// ============================================================================
void ESMFile::decodeActorReference(const ESMRecord& rec, uint8_t refType) {
    ReferenceData ref;
    ref.formID = rec.formID;
    ref.baseFormID = rec.getFormID("NAME");
    ref.refType = refType;

    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 24) {
        ref.position.x = readF32(data->data.data());
        ref.position.y = readF32(data->data.data() + 4);
        ref.position.z = readF32(data->data.data() + 8);
        ref.rotation.x = readF32(data->data.data() + 12);
        ref.rotation.y = readF32(data->data.data() + 16);
        ref.rotation.z = readF32(data->data.data() + 20);
    }

    auto* xsca = rec.findSubRecord("XSCL");
    if (xsca && xsca->size() >= 4) {
        ref.scale = readF32(xsca->data.data());
    }

    if (refType == 1) {
        m_npcReferenceCount++;
    } else {
        m_creatureReferenceCount++;
    }

    // Owning cell formID (see decodeReference): distinguishes exterior references
    // from interior ones.
    ref.cellFormID = rec.parentFormID;

    m_references.push_back(std::move(ref));
}

// ============================================================================
// Terrain (LAND) decoding — 33x33 heightmap per cell
//
// VHGT layout (1096 bytes total):
//   offset   0 : float   base height of the cell's south-west corner
//   offset   4 : int8    gradient data, 33x33 = 1089 signed bytes
//   offset 1093 : 3 bytes unknown
// Each gradient step equals 8 game units. The leftmost column (x = 0) carries a
// per-row offset that accumulates northward; the remaining 32 columns accumulate
// eastward within their row. Both accumulations start from the cell offset.
// ============================================================================
std::vector<float> TerrainData::expandHeights() const {
    std::vector<float> heights;
    if (!hasHeights()) return heights;

    heights.resize(GRID * GRID);
    float columnOffset = 0.0f;
    for (int y = 0; y < GRID; ++y) {
        columnOffset += static_cast<float>(heightDeltas[y * GRID]);
        float rowOffset = 0.0f;
        for (int x = 0; x < GRID; ++x) {
            if (x > 0) rowOffset += static_cast<float>(heightDeltas[y * GRID + x]);
            heights[y * GRID + x] = (heightOffset + columnOffset + rowOffset) * 8.0f;
        }
    }
    return heights;
}

float TerrainData::heightAt(int x, int y) const {
    if (x < 0 || y < 0 || x >= GRID || y >= GRID) return 0.0f;
    if (!hasHeights()) return 0.0f;

    float columnOffset = 0.0f;
    for (int row = 0; row <= y; ++row) {
        columnOffset += static_cast<float>(heightDeltas[row * GRID]);
    }
    float rowOffset = 0.0f;
    for (int col = 1; col <= x; ++col) {
        rowOffset += static_cast<float>(heightDeltas[y * GRID + col]);
    }
    return (heightOffset + columnOffset + rowOffset) * 8.0f;
}

void ESMFile::decodeTerrain(const ESMRecord& rec) {
    TerrainData terrain;
    // LAND records are keyed by their own FormID; the owning cell is only
    // reachable through the enclosing cell-children GRUP label.
    terrain.formID = rec.parentFormID != 0 ? rec.parentFormID : rec.formID;

    const auto* vhgt = rec.findSubRecord("VHGT");
    if (vhgt && vhgt->size() >= 4 + 1089) {
        const uint8_t* ptr = vhgt->data.data();
        terrain.heightOffset = readF32(ptr);
        const int8_t* deltas = reinterpret_cast<const int8_t*>(ptr + 4);
        terrain.heightDeltas.assign(deltas,
                                    deltas + TerrainData::GRID * TerrainData::GRID);
    }

    // LAND additive layers are stored as ATXT headers each followed by one or
    // more VTXT subrecords. The VTXT weights belong to the most recent ATXT, so
    // the pair is decoded together and finished at the next non-VTXT record.
    TerrainData::AddedLayer pending;
    auto finishLayer = [&terrain](TerrainData::AddedLayer& layer) {
        if (!layer.positions.empty()) {
            terrain.addedLayers.push_back(std::move(layer));
        }
    };

    for (const auto& sub : rec.subRecords) {
        if (std::memcmp(sub.tag, "ATXT", 4) == 0 && sub.size() >= 8) {
            // ATXT: LTEX FormID(u32) + Quadrant(u8, 0=SW 1=SE 2=NW 3=NE)
            //       + Unknown(u8) + Layer(u16, 0-7).
            finishLayer(pending);
            pending = TerrainData::AddedLayer{};
            pending.textureFormID = readU32(sub.data.data());
            pending.quadrant = sub.data[4];
            pending.layer = readU16(sub.data.data() + 6);
            continue;
        }

            if (std::memcmp(sub.tag, "VTXT", 4) == 0 && sub.size() >= 8) {
                // VTXT: packed quadrant position(u16, 0-288) + Unknown(2B) + Opacity(f32).
                // Positions run 0..288 inside the layer's quadrant (17x17 grid), so the
                // renderer can translate them to cell-local 33x33 coordinates at expand
                // time. Only a layer with a valid ATXT header can absorb weights.
                if (pending.textureFormID != 0) {
                for (size_t o = 0; o + 8 <= sub.size(); o += 8) {
                    const uint8_t* p = sub.data.data() + o;
                    const uint16_t pos = readU16(p);
                    const float opacity = readF32(p + 4);
                    if (opacity <= 0.0f) continue;  // keep it compact
                    pending.positions.push_back(pos);
                    pending.opacities.push_back(opacity);
                }
            }
            continue;
        }

        // Every other subrecord ends the current ATXT/VTXT group.
        finishLayer(pending);
        pending = TerrainData::AddedLayer{};

        if (std::memcmp(sub.tag, "BTXT", 4) == 0 && sub.size() >= 5) {
            const uint32_t textureFormID = readU32(sub.data.data());
            const uint8_t quadrant = sub.data[4];
            if (quadrant < 4 && textureFormID != 0) {
                terrain.baseTextures[quadrant] = textureFormID;
            }
        } else if (std::memcmp(sub.tag, "VTEX", 4) == 0) {
            for (size_t o = 0; o + 4 <= sub.size(); o += 4) {
                const uint32_t textureFormID = readU32(sub.data.data() + o);
                if (textureFormID != 0) terrain.textureFormIDs.push_back(textureFormID);
            }
        }
    }
    finishLayer(pending);

        m_terrains.push_back(std::move(terrain));
}

// ============================================================================
// World (WRLD) decoding — worldspace definitions
// ============================================================================

void ESMFile::decodeWorld(const ESMRecord& rec) {
    WorldData world;
    world.formID = rec.formID;
    world.editorID = rec.getString("EDID");
    world.fullName = rec.getString("FULL");

    // DATA subrecord: 2 floats (offsetX, offsetY) + 4 bytes (minX, minY, maxX, maxY)
    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 16) {
        world.worldOffset.x = readF32(data->data.data());
        world.worldOffset.y = readF32(data->data.data() + 4);
        world.minX = readI32(data->data.data() + 8);
        world.minY = readI32(data->data.data() + 12);
    }

    // NAM0 subrecord: max bounds (maxX, maxY)
    auto* nam0 = rec.findSubRecord("NAM0");
    if (nam0 && nam0->size() >= 8) {
        world.maxX = readI32(nam0->data.data());
        world.maxY = readI32(nam0->data.data() + 4);
    }

    // WNAM subrecord: parent worldspace FormID, NOT a WATR water reference.
    // ESM analysis confirmed WRLD 0x3C (Tamriel, the root worldspace) carries
    // no WNAM and no WHGT at all; child worldspaces (e.g. Bruma 0x1C318) use
    // WNAM=0x3C to point back at their parent. No water surface is derived
    // from it.
    auto* wnam = rec.findSubRecord("WNAM");
    if (wnam && wnam->size() >= 4) {
        world.parentWorldspaceFormID = readU32(wnam->data.data());
    }

    LOGD("  WRLD: 0x%08X '%s' '%s' offset=(%.0f,%.0f) bounds=[(%d,%d)-(%d,%d)] parentWorldspace=0x%08X",
         world.formID, world.editorID.c_str(), world.fullName.c_str(),
         world.worldOffset.x, world.worldOffset.y,
         world.minX, world.minY, world.maxX, world.maxY,
         world.parentWorldspaceFormID);

    m_worlds.push_back(std::move(world));
}

// ============================================================================
// Spell (SPEL) decoding — spell definitions with effect lists
// ============================================================================

void ESMFile::decodeSpell(const ESMRecord& rec) {
    SpellData spell;
    spell.formID = rec.formID;
    spell.editorID = rec.getString("EDID");
    spell.fullName = rec.getString("FULL");

    // SPIT subrecord: magic data (16 bytes)
    // struct { uint32_t type; uint32_t cost; uint32_t level; uint32_t unused; }
    auto* spit = rec.findSubRecord("SPIT");
    if (spit && spit->size() >= 8) {
        spell.spellType = static_cast<uint8_t>(spit->data[0]);       // 0=spell, 1=disease, ...
        spell.cost = readU32(spit->data.data() + 4);
        if (spit->size() >= 9) {
            spell.level = spit->data[8];  // 0=novice .. 4=master
        }
        // SPIT+12 is not initialized by the Construction Set; leave flags at 0.
    }

    // Parse effects: iterate subrecords looking for EFID + EFIT pairs.
    // Oblivion's EFID holds the 4-character MGEF editorID code, not a formID.
    for (size_t i = 0; i < rec.subRecords.size(); i++) {
        const auto& sub = rec.subRecords[i];
        if (std::memcmp(sub.tag, "EFID", 4) == 0 && sub.size() >= 4) {
            spell.effectCodes.push_back(efidCode(sub.data.data()));
            spell.effectFormIDs.push_back(0);  // resolved later by resolveMagicReferences()

            // EFIT is the next sub-record: code(4) mag(4) area(4) duration(4) unused(4) actorValue(4)
            if (i + 1 < rec.subRecords.size() &&
                std::memcmp(rec.subRecords[i + 1].tag, "EFIT", 4) == 0 &&
                rec.subRecords[i + 1].size() >= 24) {
                const auto& efit = rec.subRecords[i + 1];
                spell.effectMagnitudes.push_back(readFloatOrInt(efit.data.data() + 4));
                spell.effectAreas.push_back(readU32(efit.data.data() + 8));
                spell.effectDurations.push_back(readU32(efit.data.data() + 12));
                if (spell.actorValue == 0) {
                    spell.actorValue = readU32(efit.data.data() + 20);
                }
            }
        }
    }

    LOGD("  SPEL: 0x%08X '%s' cost=%u type=%d level=%d effects=%zu",
         spell.formID, spell.fullName.c_str(),
         spell.cost, spell.spellType, spell.level,
         spell.effectFormIDs.size());

    m_spells.push_back(std::move(spell));
}

void ESMFile::decodeEnchantment(const ESMRecord& rec) {
    EnchantmentData enchant;
    enchant.formID = rec.formID;
    enchant.editorID = rec.getString("EDID");
    enchant.fullName = rec.getString("FULL");

    // ENIT subrecord: enchantment data
    auto* enit = rec.findSubRecord("ENIT");
    if (enit && enit->size() >= 16) {
        enchant.enchantType = readU32(enit->data.data());
        enchant.chargeAmount = readU32(enit->data.data() + 4);
        enchant.enchantCost = readU32(enit->data.data() + 8);
        enchant.flags = readU32(enit->data.data() + 12);
    }

    // Parse effects: iterate subrecords looking for EFID + EFIT pairs.
    // Oblivion's EFID holds the 4-character MGEF editorID code, not a formID.
    for (size_t i = 0; i < rec.subRecords.size(); i++) {
        const auto& sub = rec.subRecords[i];
        if (std::memcmp(sub.tag, "EFID", 4) == 0 && sub.size() >= 4) {
            enchant.effectCodes.push_back(efidCode(sub.data.data()));
            enchant.effectFormIDs.push_back(0);  // resolved later

            // EFIT is the next sub-record: code(4) mag(4) area(4) duration(4) unused(4) actorValue(4)
            if (i + 1 < rec.subRecords.size() &&
                std::memcmp(rec.subRecords[i + 1].tag, "EFIT", 4) == 0 &&
                rec.subRecords[i + 1].size() >= 24) {
                const auto& efit = rec.subRecords[i + 1];
                enchant.effectMagnitudes.push_back(readFloatOrInt(efit.data.data() + 4));
                enchant.effectAreas.push_back(readU32(efit.data.data() + 8));
                enchant.effectDurations.push_back(readU32(efit.data.data() + 12));
            }
        }
    }

    LOGD("  ENCH: 0x%08X '%s' type=%u charge=%u cost=%u effects=%zu",
         enchant.formID, enchant.fullName.c_str(),
         enchant.enchantType, enchant.chargeAmount, enchant.enchantCost,
         enchant.effectFormIDs.size());

    m_enchantments.push_back(std::move(enchant));
}

// ============================================================================
// Leveled List (LVLI / LVLC / LVSP) decoding
// ============================================================================

void ESMFile::decodeMagicEffect(const ESMRecord& rec) {
    MagicEffectData effect;
    effect.formID = rec.formID;
    effect.editorID = rec.getString("EDID");
    effect.fullName = rec.getString("FULL");
    effect.description = rec.getString("DESC");

    // DATA subrecord: magic effect data (36 or 64 bytes)
    // struct { uint32_t unknown0; float baseCost; uint32_t actorValue/link;
    //          uint32_t school; uint32_t flags; ... }
    auto* medt = rec.findSubRecord("MEDT");
    if (!medt) medt = rec.findSubRecord("DATA");
    if (medt && medt->size() >= 20) {
        effect.baseCost = readFloatOrInt(medt->data.data() + 4);
        uint32_t av = readU32(medt->data.data() + 8);
        // Conjuration/restoration effects store a formID (summoned creature,
        // bound item) in this slot instead of an actor value.
        if (av <= 200) {
            effect.actorValue = av;
        } else {
            effect.linkedFormID = av;
        }
        effect.school = readU32(medt->data.data() + 12);
        effect.flags = readU32(medt->data.data() + 16);
    }

    m_magicEffects.push_back(std::move(effect));
}

void ESMFile::decodeSkill(const ESMRecord& rec) {
    SkillData skill;
    skill.formID = rec.formID;
    skill.editorID = rec.getString("EDID");
    skill.fullName = rec.getString("FULL");
    skill.description = rec.getString("DESC");

    // SKDT subrecord: skill data (16 bytes)
    // struct { uint32_t skillID; uint32_t specialization; float useMult; float offsetMult; }
    auto* skdt = rec.findSubRecord("SKDT");
    if (skdt && skdt->size() >= 16) {
        skill.skillID = readU32(skdt->data.data());
        skill.specialization = readU32(skdt->data.data() + 4);
        std::memcpy(&skill.useMult, skdt->data.data() + 8, 4);
        std::memcpy(&skill.offsetMult, skdt->data.data() + 12, 4);
    }

    // ANAM: governing attribute
    auto* anam = rec.findSubRecord("ANAM");
    if (anam && anam->size() >= 4) {
        uint32_t attr;
        std::memcpy(&attr, anam->data.data(), 4);
        skill.governingAttribute.push_back(attr);
    }

    m_skills.push_back(std::move(skill));
}

void ESMFile::decodeBirthsign(const ESMRecord& rec) {
    BirthsignData birthsign;
    birthsign.formID = rec.formID;
    birthsign.editorID = rec.getString("EDID");
    birthsign.fullName = rec.getString("FULL");
    birthsign.description = rec.getString("DESC");

    // ICON: texture path
    birthsign.texturePath = rec.getString("ICON");

    // SPLO: spell/power formIDs
    for (size_t i = 0; i < rec.subRecords.size(); i++) {
        const auto& sub = rec.subRecords[i];
        if (std::memcmp(sub.tag, "SPLO", 4) == 0 && sub.size() >= 4) {
            uint32_t spellFormID;
            std::memcpy(&spellFormID, sub.data.data(), 4);
            birthsign.spellFormIDs.push_back(spellFormID);
        }
    }

    m_birthsigns.push_back(std::move(birthsign));
}

void ESMFile::decodeContainer(const ESMRecord& rec) {
    ContainerData container;
    container.formID = rec.formID;
    container.editorID = rec.getString("EDID");
    container.fullName = rec.getString("FULL");
    container.modelPath = rec.getString("MODL");

    // CNTO: container data (weight + flags)
    auto* cnto = rec.findSubRecord("CNTO");
    if (cnto && cnto->size() >= 8) {
        std::memcpy(&container.weight, cnto->data.data(), 4);
        container.flags = readU32(cnto->data.data() + 4);
    }

    // Parse container items: iterate subrecords looking for CNTO + COCT pairs
    for (size_t i = 0; i < rec.subRecords.size(); i++) {
        const auto& sub = rec.subRecords[i];
        if (std::memcmp(sub.tag, "CNTO", 4) == 0 && sub.size() >= 8) {
            ContainerData::ContainerItem item;
            std::memcpy(&item.itemFormID, sub.data.data(), 4);
            item.count = readU32(sub.data.data() + 4);
            container.items.push_back(item);
        }
    }

    m_containers.push_back(std::move(container));
}

void ESMFile::decodeLight(const ESMRecord& rec) {
    LightData light;
    light.formID = rec.formID;
    light.editorID = rec.getString("EDID");
    light.fullName = rec.getString("FULL");
    light.modelPath = rec.getString("MODL");
    light.iconPath = rec.getString("ICON");

    // DATA subrecord: contains light properties
    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 32) {
        std::memcpy(&light.duration, data->data.data(), 4);
        std::memcpy(&light.radius, data->data.data() + 4, 4);
        std::memcpy(&light.color, data->data.data() + 8, 4);
        std::memcpy(&light.flags, data->data.data() + 12, 4);
        std::memcpy(&light.falloff, data->data.data() + 16, 4);
        std::memcpy(&light.fov, data->data.data() + 20, 4);
        std::memcpy(&light.weight, data->data.data() + 24, 4);
        std::memcpy(&light.value, data->data.data() + 28, 4);
    }

    m_lights.push_back(std::move(light));
}

void ESMFile::decodeStatic(const ESMRecord& rec) {
    StaticData stat;
    stat.formID = rec.formID;
    stat.editorID = rec.getString("EDID");
    stat.modelPath = rec.getString("MODL");
    m_statics.push_back(std::move(stat));
}

void ESMFile::decodeSound(const ESMRecord& rec) {
    SoundData sound;
    sound.formID = rec.formID;
    sound.editorID = rec.getString("EDID");
    sound.soundPath = rec.getString("FNAM");

    // SOUN subrecord: sound properties
    auto* soun = rec.findSubRecord("SOUN");
    if (soun && soun->size() >= 4) {
        sound.minDistance = soun->data[0];
        sound.maxDistance = soun->data[1];
        sound.freqAdjust = soun->data[2];
        sound.flags = soun->data[3];
    }

    m_sounds.push_back(std::move(sound));
}

void ESMFile::decodeTree(const ESMRecord& rec) {
    TreeData tree;
    tree.formID = rec.formID;
    tree.editorID = rec.getString("EDID");
    tree.modelPath = rec.getString("MODL");
    tree.iconPath = rec.getString("ICON");

    // INGR subrecord: ingredient formID
    auto* ingr = rec.findSubRecord("INGR");
    if (ingr && ingr->size() >= 4) {
        std::memcpy(&tree.ingredientFormID, ingr->data.data(), 4);
    }

    // CNTO subrecord: harvest chance (float)
    auto* cnto = rec.findSubRecord("CNTO");
    if (cnto && cnto->size() >= 4) {
        std::memcpy(&tree.harvestChance, cnto->data.data(), 4);
    }

    m_trees.push_back(std::move(tree));
}

void ESMFile::decodeFlora(const ESMRecord& rec) {
    FloraData flora;
    flora.formID = rec.formID;
    flora.editorID = rec.getString("EDID");
    flora.fullName = rec.getString("FULL");
    flora.modelPath = rec.getString("MODL");

    // INGR subrecord: ingredient formID
    auto* ingr = rec.findSubRecord("INGR");
    if (ingr && ingr->size() >= 4) {
        std::memcpy(&flora.ingredientFormID, ingr->data.data(), 4);
    }

    // CNTO subrecord: harvest chance (float)
    auto* cnto = rec.findSubRecord("CNTO");
    if (cnto && cnto->size() >= 4) {
        std::memcpy(&flora.harvestChance, cnto->data.data(), 4);
    }

    m_floras.push_back(std::move(flora));
}

void ESMFile::decodeActivator(const ESMRecord& rec) {
    ActivatorData acti;
    acti.formID = rec.formID;
    acti.editorID = rec.getString("EDID");
    acti.fullName = rec.getString("FULL");
    acti.modelPath = rec.getString("MODL");

    // SCRI subrecord: script formID
    auto* scri = rec.findSubRecord("SCRI");
    if (scri && scri->size() >= 4) {
        std::memcpy(&acti.scriptFormID, scri->data.data(), 4);
    }

    m_activators.push_back(std::move(acti));
}

void ESMFile::decodeApparatus(const ESMRecord& rec) {
    ApparatusData appa;
    appa.formID = rec.formID;
    appa.editorID = rec.getString("EDID");
    appa.fullName = rec.getString("FULL");
    appa.modelPath = rec.getString("MODL");
    appa.iconPath = rec.getString("ICON");

    // APPA subrecord: quality (4 bytes)
    auto* appaSub = rec.findSubRecord("APPA");
    if (appaSub && appaSub->size() >= 4) {
        std::memcpy(&appa.quality, appaSub->data.data(), 4);
    }

    // DATA subrecord: weight + value
    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 8) {
        std::memcpy(&appa.weight, data->data.data(), 4);
        std::memcpy(&appa.value, data->data.data() + 4, 4);
    }

    m_apparatuses.push_back(std::move(appa));
}

void ESMFile::decodeEyes(const ESMRecord& rec) {
    EyesData eyes;
    eyes.formID = rec.formID;
    eyes.editorID = rec.getString("EDID");
    eyes.fullName = rec.getString("FULL");
    eyes.iconPath = rec.getString("ICON");

    // DATA subrecord: flags (1 byte)
    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 1) {
        eyes.flags = data->data[0];
    }

    m_eyes.push_back(std::move(eyes));
}

void ESMFile::decodeHair(const ESMRecord& rec) {
    HairData hair;
    hair.formID = rec.formID;
    hair.editorID = rec.getString("EDID");
    hair.fullName = rec.getString("FULL");
    hair.modelPath = rec.getString("MODL");
    hair.iconPath = rec.getString("ICON");

    // DATA subrecord: flags (1 byte)
    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 1) {
        hair.flags = data->data[0];
    }

    m_hairs.push_back(std::move(hair));
}

void ESMFile::decodeClimate(const ESMRecord& rec) {
    ClimateData clmt;
    clmt.formID = rec.formID;
    clmt.editorID = rec.getString("EDID");

    // WLST subrecord: weather types (64 bytes = 16 x uint32_t)
    auto* wlst = rec.findSubRecord("WLST");
    if (wlst && wlst->size() >= 64) {
        for (int i = 0; i < 16; i++) {
            std::memcpy(&clmt.weatherTypes[i], wlst->data.data() + i * 4, 4);
        }
    }

    // TNAM subrecord: timing (6 bytes)
    auto* tnam = rec.findSubRecord("TNAM");
    if (tnam && tnam->size() >= 6) {
        clmt.sunriseBegin = tnam->data[0];
        clmt.sunriseEnd = tnam->data[1];
        clmt.sunsetBegin = tnam->data[2];
        clmt.sunsetEnd = tnam->data[3];
        clmt.volatility = tnam->data[4];
        clmt.moons = tnam->data[5];
    }

    m_climates.push_back(std::move(clmt));
}

void ESMFile::decodeRegion(const ESMRecord& rec) {
    RegionData regn;
    regn.formID = rec.formID;
    regn.editorID = rec.getString("EDID");
    regn.fullName = rec.getString("FULL");

    // RCLR subrecord: map color (4 bytes)
    auto* rclr = rec.findSubRecord("RCLR");
    if (rclr && rclr->size() >= 4) {
        std::memcpy(&regn.mapColor, rclr->data.data(), 4);
    }

    // WNAM subrecord: world space formID
    auto* wnam = rec.findSubRecord("WNAM");
    if (wnam && wnam->size() >= 4) {
        std::memcpy(&regn.worldSpaceFormID, wnam->data.data(), 4);
    }

    // Weather entries (RDWT subrecords)
    for (size_t i = 0; i < rec.subRecords.size(); i++) {
        const auto& sub = rec.subRecords[i];
        if (std::memcmp(sub.tag, "RDWT", 4) == 0 && sub.size() >= 8) {
            RegionData::RegionWeather weather;
            std::memcpy(&weather.weatherFormID, sub.data.data(), 4);
            std::memcpy(&weather.chance, sub.data.data() + 4, 4);
            regn.weathers.push_back(weather);
        }
    }

    m_regions.push_back(std::move(regn));
}

void ESMFile::decodeLeveledList(const ESMRecord& rec) {
    LeveledListData list;
    list.formID = rec.formID;
    list.editorID = rec.getString("EDID");

    // LVLD: flags + chance none (4 bytes)
    // struct { uint8_t chanceNone; uint8_t flags; uint8_t padding[2]; }
    auto* lvld = rec.findSubRecord("LVLD");
    if (lvld && lvld->size() >= 2) {
        list.chanceNone = lvld->data[0];
        list.flags = lvld->data[1];
    }

    // LVLO: list entries (each 8 bytes)
    // struct { uint32_t formID; uint16_t level; uint16_t count; }
    for (size_t i = 0; i < rec.subRecords.size(); i++) {
        const auto& sub = rec.subRecords[i];
        if (std::memcmp(sub.tag, "LVLO", 4) == 0 && sub.size() >= 8) {
            LeveledListEntry entry;
            std::memcpy(&entry.referencedFormID, sub.data.data(), 4);
            entry.level = readU16(sub.data.data() + 4);
            if (sub.size() >= 10) {
                entry.count = readU16(sub.data.data() + 8);
            } else {
                entry.count = 1;
            }
            list.entries.push_back(entry);
        }
    }

    const char* recLabel = std::memcmp(rec.recType, "LVLC", 4) == 0 ? "LVLC" :
                           (std::memcmp(rec.recType, "LVSP", 4) == 0 ? "LVSP" : "LVLI");

    LOGD("  %s: 0x%08X '%s' chanceNone=%u flags=0x%02X entries=%zu",
         recLabel, list.formID, list.editorID.c_str(),
         list.chanceNone, list.flags, list.entries.size());

    m_leveledLists.push_back(std::move(list));
}

// ============================================================================
// NavMesh (NAVM) decoding — AI pathfinding data
// ============================================================================

void ESMFile::decodeNavMesh(const ESMRecord& rec) {
    NavMeshData navMesh;
    navMesh.formID = rec.formID;
    navMesh.editorID = rec.getString("EDID");
    navMesh.cellFormID = rec.getFormID("NVER");   // Cell reference

    // NNAM: location (12 bytes = 3 floats)
    auto* nnam = rec.findSubRecord("NNAM");
    if (nnam && nnam->size() >= 12) {
        std::memcpy(&navMesh.location, nnam->data.data(), 12);
    }

    // NVNM: actual navmesh data
    // Structure: header (12 bytes) + vertices + triangles + external connections
    auto* nvnm = rec.findSubRecord("NVNM");
    if (nvnm && nvnm->size() >= 16) {
        size_t offset = 0;

        // Header: version(4) + numVertices(4) + numTriangles(4)
        uint32_t version = 0;
        if (offset + 12 <= nvnm->size()) {
            std::memcpy(&version, nvnm->data.data() + offset, 4);
            offset += 4;
            std::memcpy(&navMesh.numVertices, nvnm->data.data() + offset, 4);
            offset += 4;
            std::memcpy(&navMesh.numTriangles, nvnm->data.data() + offset, 4);
            offset += 4;
        }

        // Read vertices: each vertex = 3 floats = 12 bytes
        if (navMesh.numVertices > 0 && navMesh.numVertices < 10000) {
            navMesh.vertices.reserve(navMesh.numVertices);
            for (int i = 0; i < navMesh.numVertices && offset + 12 <= nvnm->size(); i++) {
                glm::vec3 v;
                std::memcpy(&v, nvnm->data.data() + offset, 12);
                offset += 12;
                navMesh.vertices.push_back(v);
            }
        } else {
            navMesh.numVertices = 0;
        }

        // Read triangles: each = 3 vertex indices(2 bytes each) + 3 edge links(2 bytes each) = 12 bytes
        if (navMesh.numTriangles > 0 && navMesh.numTriangles < 20000) {
            navMesh.triangles.reserve(navMesh.numTriangles);
            for (int i = 0; i < navMesh.numTriangles && offset + 12 <= nvnm->size(); i++) {
                NavMeshTriangle tri;
                std::memcpy(tri.vertex, nvnm->data.data() + offset, 6);
                std::memcpy(tri.adjacentEdge, nvnm->data.data() + offset + 6, 6);
                offset += 12;
                navMesh.triangles.push_back(tri);
            }
        } else {
            navMesh.numTriangles = 0;
        }

        // Remaining NVNM data (preferred paths, door links, cover) is ignored for now
    }

    LOGD("  NAVM: 0x%08X '%s' cell=0x%08X verts=%d tris=%d",
         navMesh.formID, navMesh.editorID.c_str(),
         navMesh.cellFormID, navMesh.numVertices, navMesh.numTriangles);

    m_navMeshes.push_back(std::move(navMesh));
}

// ============================================================================
// Armor (ARMO) decoding
// ============================================================================

void ESMFile::decodeArmor(const ESMRecord& rec) {
    ArmorData armor;
    armor.formID = rec.formID;
    armor.editorID = rec.getString("EDID");
    armor.fullName = rec.getString("FULL");
    armor.modelPath = rec.getString("MODL");

    // BMDT: body part data (includes armor type)
    auto* bmdt = rec.findSubRecord("BMDT");
    if (bmdt && bmdt->size() >= 4) {
        uint32_t bmdtVal;
        std::memcpy(&bmdtVal, bmdt->data.data(), 4);
        armor.armorType = static_cast<uint8_t>((bmdtVal >> 4) & 0x03);  // bits 4-5: 0=light,1=heavy,2=cloth
    }

    // DATA: armor data (armor rating 4 bytes, value 4 bytes, weight 4 bytes)
    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 8) {
        armor.armorRating = readU32(data->data.data());
        if (data->size() >= 12) {
            armor.value = readU32(data->data.data() + 4);
            std::memcpy(&armor.weight, data->data.data() + 8, 4);
        }
    }

    // ENAM: enchantment FormID
    auto* enam = rec.findSubRecord("ENAM");
    if (enam && enam->size() >= 4) {
        armor.enchantmentID = readU32(enam->data.data());
    }

    LOGD("  ARMO: 0x%08X '%s' rating=%u weight=%.1f value=%u",
         armor.formID, armor.fullName.c_str(),
         armor.armorRating, armor.weight, armor.value);

    m_armors.push_back(std::move(armor));
}

// ============================================================================
// Book (BOOK) decoding — skill books and regular books
// ============================================================================

void ESMFile::decodeBook(const ESMRecord& rec) {
    BookData book;
    book.formID = rec.formID;
    book.editorID = rec.getString("EDID");
    book.fullName = rec.getString("FULL");
    book.modelPath = rec.getString("MODL");
    book.description = rec.getString("DESC");

    // DATA: book data (flags 4 bytes + teachesSkill 4 bytes)
    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 4) {
        uint32_t flags;
        std::memcpy(&flags, data->data.data(), 4);
        if (flags & 0x01 && data->size() >= 8) {
            // Bit 0 set = skill book; teachesSkill is the skill FormID
            uint32_t teachesSkill;
            std::memcpy(&teachesSkill, data->data.data() + 4, 4);
            book.teachesSkillID = teachesSkill;
            book.teachesSkillLevel = 1;  // Default +1 for skill books
        }
    }

    LOGD("  BOOK: 0x%08X '%s' teachesSkill=0x%08X",
         book.formID, book.fullName.c_str(), book.teachesSkillID);

    m_books.push_back(std::move(book));
}

// ============================================================================
// Faction (FACT) decoding
// ============================================================================

void ESMFile::decodeFaction(const ESMRecord& rec) {
    FactionData faction;
    faction.formID = rec.formID;
    faction.editorID = rec.getString("EDID");
    faction.fullName = rec.getString("FULL");

    // CRIM: crime gold multiplier
    auto* crim = rec.findSubRecord("CRIM");
    if (crim && crim->size() >= 4) {
        faction.crimeGoldMultiplier = readI32(crim->data.data());
    }

    // RNAM: rank name; MNAM: male rank data (position, perks)
    // These come in alternating pairs
    for (size_t i = 0; i < rec.subRecords.size(); i++) {
        const auto& sub = rec.subRecords[i];
        if (std::memcmp(sub.tag, "RNAM", 4) == 0) {
            FactionRank rank;
            rank.rankName = std::string(reinterpret_cast<const char*>(sub.data.data()), sub.data.size());
            // Next subrecord should be MNAM
            if (i + 1 < rec.subRecords.size() &&
                std::memcmp(rec.subRecords[i + 1].tag, "MNAM", 4) == 0) {
                const auto& mnam = rec.subRecords[i + 1];
                if (mnam.size() >= 4) {
                    rank.rankData = readU32(mnam.data.data());
                }
            }
            faction.ranks.push_back(rank);
        }
    }

    // XNAM: faction relations
    for (size_t i = 0; i < rec.subRecords.size(); i++) {
        const auto& sub = rec.subRecords[i];
        if (std::memcmp(sub.tag, "XNAM", 4) == 0 && sub.size() >= 12) {
            FactionRelation rel;
            std::memcpy(&rel.factionFormID, sub.data.data(), 4);
            std::memcpy(&rel.modifier, sub.data.data() + 4, 4);
            std::memcpy(&rel.groupFlags, sub.data.data() + 8, 4);
            faction.relations.push_back(rel);
        }
    }

    LOGD("  FACT: 0x%08X '%s' ranks=%zu relations=%zu",
         faction.formID, faction.fullName.c_str(),
         faction.ranks.size(), faction.relations.size());

    m_factions.push_back(std::move(faction));
}

// ============================================================================
// Race (RACE) decoding
// ============================================================================

void ESMFile::decodeRace(const ESMRecord& rec) {
    RaceData race;
    race.formID = rec.formID;
    race.editorID = rec.getString("EDID");
    race.fullName = rec.getString("FULL");
    race.description = rec.getString("DESC");

    // RNAM: male model path
    race.maleModelPath = rec.getString("RNAM");
    // FNAM: female model path (actually used in RACE)
    // But Oblivion uses MNAM/FNAM for model paths in RACE records
    auto* fnam = rec.findSubRecord("FNAM");
    if (fnam) {
        race.femaleModelPath = std::string(reinterpret_cast<const char*>(fnam->data.data()), fnam->data.size());
    }

    // Spells: SPLO subrecords
    for (const auto& sub : rec.subRecords) {
        if (std::memcmp(sub.tag, "SPLO", 4) == 0 && sub.size() >= 4) {
            uint32_t spellID;
            std::memcpy(&spellID, sub.data.data(), 4);
            race.spellFormIDs.push_back(spellID);
        }
    }

    // Attribute/Skill bonuses parsed but simplified for now
    // Full parsing requires understanding subrecord order in RACE
    // ATTR and SKIL subrecords are not always present in decompiled dumps

    LOGD("  RACE: 0x%08X '%s' spells=%zu",
         race.formID, race.fullName.c_str(),
         race.spellFormIDs.size());

    m_races.push_back(std::move(race));
}

// ============================================================================
// Class (CLAS) decoding
// ============================================================================

void ESMFile::decodeClass(const ESMRecord& rec) {
    ClassData clas;
    clas.formID = rec.formID;
    clas.editorID = rec.getString("EDID");
    clas.fullName = rec.getString("FULL");
    clas.description = rec.getString("DESC");

    // DATA: class data (16 bytes)
    // struct { uint32_t flags; uint8_t specialization; ... }
    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 12) {
        std::memcpy(&clas.flags, data->data.data(), 4);
        clas.specialization = data->data[4];  // 0=Combat, 1=Mysticism, 2=Stealth
    }

    // PRAT: primary attributes (2 uint32_t)
    auto* prat = rec.findSubRecord("PRAT");
    if (prat && prat->size() >= 8) {
        std::memcpy(&clas.primaryAttribute1, prat->data.data(), 4);
        std::memcpy(&clas.primaryAttribute2, prat->data.data() + 4, 4);
    }

    LOGD("  CLAS: 0x%08X '%s' spec=%d attr1=%u attr2=%u",
         clas.formID, clas.fullName.c_str(),
         clas.specialization, clas.primaryAttribute1, clas.primaryAttribute2);

    m_classes.push_back(std::move(clas));
}

    // ============================================================================
    // Clothing (CLOT) decoding
    // ============================================================================

    void ESMFile::decodeClothing(const ESMRecord& rec) {
        ClothingData cloth;
        cloth.formID = rec.formID;
        cloth.editorID = rec.getString("EDID");
        cloth.fullName = rec.getString("FULL");
        cloth.modelPath = rec.getString("MODL");

        // DATA: value (4 bytes) + weight (4 bytes)
        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 8) {
            std::memcpy(&cloth.value, data->data.data(), 4);
            std::memcpy(&cloth.weight, data->data.data() + 4, 4);
        }

        // ENAM: enchantment
        auto* enam = rec.findSubRecord("ENAM");
        if (enam && enam->size() >= 4) {
            std::memcpy(&cloth.enchantmentID, enam->data.data(), 4);
        }

        LOGD("  CLOT: 0x%08X '%s' value=%u weight=%.1f", cloth.formID, cloth.fullName.c_str(), cloth.value, cloth.weight);
        m_clothing.push_back(std::move(cloth));
    }

    // ============================================================================
    // Ingredient (INGR) decoding
    // ============================================================================

    void ESMFile::decodeIngredient(const ESMRecord& rec) {
        IngredientData ingr;
        ingr.formID = rec.formID;
        ingr.editorID = rec.getString("EDID");
        ingr.fullName = rec.getString("FULL");
        ingr.modelPath = rec.getString("MODL");

        // DATA: weight only (4 bytes float). Value lives in ENIT.
        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 4) {
            std::memcpy(&ingr.weight, data->data.data(), 4);
        }
        auto* enit = rec.findSubRecord("ENIT");
        if (enit && enit->size() >= 4) {
            ingr.value = readU32(enit->data.data());
        }

        // Effects: EFID (4-char MGEF editorID code) + EFIT pairs.
        // IRQD/IRQF are kept as a fallback for plugin variants.
        for (size_t i = 0; i < rec.subRecords.size(); i++) {
            const auto& sub = rec.subRecords[i];
            if (std::memcmp(sub.tag, "EFID", 4) == 0 && sub.size() >= 4) {
                ingr.effectCodes.push_back(efidCode(sub.data.data()));
                ingr.effectFormIDs.push_back(0);  // resolved later
                float mag = 0;
                uint32_t area = 0, dur = 0;
                if (i + 1 < rec.subRecords.size() &&
                    std::memcmp(rec.subRecords[i + 1].tag, "EFIT", 4) == 0 &&
                    rec.subRecords[i + 1].size() >= 24) {
                    mag = readFloatOrInt(rec.subRecords[i + 1].data.data() + 4);
                    area = readU32(rec.subRecords[i + 1].data.data() + 8);
                    dur = readU32(rec.subRecords[i + 1].data.data() + 12);
                }
                ingr.effectMagnitudes.push_back(mag);
                ingr.effectAreas.push_back(area);
                ingr.effectDurations.push_back(dur);
            } else if (std::memcmp(sub.tag, "IRQD", 4) == 0 && sub.size() >= 4) {
                ingr.effectFormIDs.push_back(readU32(sub.data.data()));
                ingr.effectCodes.push_back(std::string());
                float mag = 0;
                if (i + 1 < rec.subRecords.size() && std::memcmp(rec.subRecords[i + 1].tag, "IRQF", 4) == 0 && rec.subRecords[i + 1].size() >= 4) {
                    mag = readFloatOrInt(rec.subRecords[i + 1].data.data());
                }
                ingr.effectMagnitudes.push_back(mag);
                ingr.effectAreas.push_back(0);
                ingr.effectDurations.push_back(0);
            }
        }

        LOGD("  INGR: 0x%08X '%s' value=%u effects=%zu", ingr.formID, ingr.fullName.c_str(), ingr.value, ingr.effectFormIDs.size());
        m_ingredients.push_back(std::move(ingr));
    }

    // ============================================================================
    // Alchemy Potion (ALCH) decoding
    // ============================================================================

    void ESMFile::decodeAlchemy(const ESMRecord& rec) {
        AlchemyData alch;
        alch.formID = rec.formID;
        alch.editorID = rec.getString("EDID");
        alch.fullName = rec.getString("FULL");
        alch.modelPath = rec.getString("MODL");

        // DATA: weight only (4 bytes float). Value lives in ENIT.
        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 4) {
            std::memcpy(&alch.weight, data->data.data(), 4);
        }
        auto* enit = rec.findSubRecord("ENIT");
        if (enit && enit->size() >= 4) {
            alch.value = readU32(enit->data.data());
        }

        // Effects use EFID + EFIT pairs (same as SPEL). EFID holds the
        // 4-character MGEF editorID code rather than a formID.
        for (size_t i = 0; i < rec.subRecords.size(); i++) {
            const auto& sub = rec.subRecords[i];
            if (std::memcmp(sub.tag, "EFID", 4) == 0 && sub.size() >= 4) {
                alch.effectCodes.push_back(efidCode(sub.data.data()));
                alch.effectFormIDs.push_back(0);  // resolved later

                // EFIT is the next sub-record: code(4) mag(4) area(4) duration(4) unused(4) actorValue(4)
                if (i + 1 < rec.subRecords.size() && std::memcmp(rec.subRecords[i + 1].tag, "EFIT", 4) == 0 && rec.subRecords[i + 1].size() >= 24) {
                    const auto& efit = rec.subRecords[i + 1];
                    alch.effectMagnitudes.push_back(readFloatOrInt(efit.data.data() + 4));
                    alch.effectAreas.push_back(readU32(efit.data.data() + 8));
                    alch.effectDurations.push_back(readU32(efit.data.data() + 12));
                }
            }
        }

        LOGD("  ALCH: 0x%08X '%s' value=%u effects=%zu", alch.formID, alch.fullName.c_str(), alch.value, alch.effectFormIDs.size());
        m_alchemy.push_back(std::move(alch));
    }

    // ============================================================================
    // Misc Item (MISC) decoding
    // ============================================================================

    void ESMFile::decodeMiscItem(const ESMRecord& rec) {
        MiscItemData misc;
        misc.formID = rec.formID;
        misc.editorID = rec.getString("EDID");
        misc.fullName = rec.getString("FULL");
        misc.modelPath = rec.getString("MODL");

        // DATA: value + weight
        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 8) {
            std::memcpy(&misc.value, data->data.data(), 4);
            std::memcpy(&misc.weight, data->data.data() + 4, 4);
        }

        LOGD("  MISC: 0x%08X '%s' value=%u weight=%.1f", misc.formID, misc.fullName.c_str(), misc.value, misc.weight);
        m_miscItems.push_back(std::move(misc));
    }

    // ============================================================================
    // Road/PathGrid (ROAD) decoding
    // ============================================================================

    void ESMFile::decodeRoad(const ESMRecord& rec) {
        RoadData road;
        road.formID = rec.formID;
        // LAND/PGRD/ROAD records carry no cell FormID of their own; the owning
        // cell comes from the enclosing cell-children GRUP.
        road.cellFormID = rec.parentFormID != 0 ? rec.parentFormID : rec.getFormID("XLCN");

        // PGRP: 16 bytes per point (x, y, z floats followed by a uint32)
        auto* pgrp = rec.findSubRecord("PGRP");
        if (pgrp && pgrp->size() >= 16) {
            int numNodes = static_cast<int>(pgrp->size() / 16);
            road.nodes.reserve(numNodes);
            for (int i = 0; i < numNodes && (i + 1) * 16 <= pgrp->size(); i++) {
                glm::vec3 node;
                std::memcpy(&node, pgrp->data.data() + i * 16, 12);
                road.nodes.push_back(node);
            }
        }

        // PGRR: PathGrid Edges (4 bytes per edge: 2 uint16_t indices)
        auto* pgrr = rec.findSubRecord("PGRR");
        if (pgrr && pgrr->size() >= 4) {
            int numEdges = static_cast<int>(pgrr->size() / 4);
            road.edges.reserve(numEdges);
            for (int i = 0; i < numEdges; i++) {
                uint16_t a = readU16(pgrr->data.data() + i * 4);
                uint16_t b = readU16(pgrr->data.data() + i * 4 + 2);
                road.edges.push_back({a, b});
            }
        }

        LOGD("  ROAD: 0x%08X nodes=%d edges=%d", road.formID, (int)road.nodes.size(), (int)road.edges.size());
        m_roads.push_back(std::move(road));
    }

    void ESMFile::decodeScript(const ESMRecord& rec) {
        script::ScriptData script;
        script.formID = rec.formID;

        // EDID: editor ID (script name)
        script.editorID = rec.getString("EDID");

        // SCHR: script header (20 bytes). Field offsets measured against the
        // real Oblivion.esm over all 2,393 SCPT records:
        //   +0  uint32_t unused          (0 in 2393/2393)
        //   +4  uint32_t refCount
        //   +8  uint32_t compiledLength  (equals the SCDA length in 2393/2393)
        //   +12 uint32_t lastVarIndex
        //   +16 uint32_t scriptType      (0=Object, 1=Quest, 256=Magic)
        auto* schr = rec.findSubRecord("SCHR");
        if (schr && schr->size() >= 20) {
            const uint8_t* data = schr->data.data();
            uint32_t refCount, compiledLength, lastVarIndex, scriptTypeRaw;
            std::memcpy(&refCount, data + 4, 4);
            std::memcpy(&compiledLength, data + 8, 4);
            std::memcpy(&lastVarIndex, data + 12, 4);
            std::memcpy(&scriptTypeRaw, data + 16, 4);

            // The type values are not contiguous: magic effect scripts use 0x100.
            script::ScriptType scriptType = script::ScriptType::Object;
            if (scriptTypeRaw == 1) {
                scriptType = script::ScriptType::Quest;
            } else if (scriptTypeRaw == 256) {
                scriptType = script::ScriptType::Magic;
            }

            script.scriptType = scriptType;
            script.refCount = refCount;
            script.compiledLength = compiledLength;
            script.lastVarIndex = lastVarIndex;

            LOGD("  SCPT: 0x%08X '%s' type=%d bytecode=%u lastVar=%u refs=%u",
                 script.formID, script.editorID.c_str(),
                 static_cast<int>(scriptType), compiledLength,
                 lastVarIndex, refCount);
        }

        // SCDA: compiled bytecode
        auto* scda = rec.findSubRecord("SCDA");
        if (scda && !scda->data.empty()) {
            script.bytecode = scda->data;
        }

        // SCTX: script source text (optional, for debugging)
        auto* sctx = rec.findSubRecord("SCTX");
        if (sctx && !sctx->data.empty()) {
            script.source = std::string(
                reinterpret_cast<const char*>(sctx->data.data()),
                sctx->data.size());
        }

        // SLSD: variable data (24 bytes). Measured on Oblivion.esm: only two bytes
        // carry information - the u32 index at +0 and a coarse type marker at +16
        // (1 for every short/long variable, 0 for every float/ref one). Bytes 4..7
        // are non-zero in 277 of the 7,266 subrecords but hold stale ASCII and
        // offset fragments there rather than a type or a default value, so nothing
        // else in the record is read.
        // SCVR: variable name (null-terminated string), stored as an SLSD/SCVR pair
        //
        // The exact type comes from the SCTX declarations: the declared name matches
        // the SCVR name for every one of the 7,266 variables (short+long 5,089 /
        // float 1,170 / ref 996 / int 11), so the type is parsed rather than guessed.
        std::unordered_map<std::string, script::ScriptValue::Type> declaredTypes;
        {
            std::istringstream source(script.source);
            std::string line;
            while (std::getline(source, line)) {
                size_t comment = line.find(';');
                if (comment != std::string::npos) {
                    line.erase(comment);
                }
                std::istringstream tokens(line);
                std::string keyword;
                std::string name;
                if (!(tokens >> keyword >> name)) {
                    continue;
                }
                for (char& c : keyword) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                for (char& c : name) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                if (keyword == "short" || keyword == "long" || keyword == "int") {
                    declaredTypes[name] = script::ScriptValue::Type::Integer;
                } else if (keyword == "float") {
                    declaredTypes[name] = script::ScriptValue::Type::Float;
                } else if (keyword == "ref") {
                    declaredTypes[name] = script::ScriptValue::Type::Ref;
                }
            }
        }

        // SCRV: the indices of the reference-typed variables - 996 across the ESM,
        // every one of them a `ref` declaration. Collected before the pairs so that
        // a name the source does not declare can still be classified.
        std::vector<uint32_t> referenceVariables;
        for (const auto& sub : rec.subRecords) {
            if (std::memcmp(sub.tag, "SCRV", 4) == 0 && sub.size() >= 4) {
                referenceVariables.push_back(readU32(sub.data.data()));
            }
        }

        for (size_t i = 0; i < rec.subRecords.size(); ++i) {
            const auto& sub = rec.subRecords[i];
            if (std::memcmp(sub.tag, "SLSD", 4) == 0 && sub.size() >= 4) {
                script::ScriptVariable var;
                const uint8_t* data = sub.data.data();
                uint32_t index;
                std::memcpy(&index, data, 4);

                var.index = index;

                // Next subrecord should be SCVR with the variable name
                if (i + 1 < rec.subRecords.size()) {
                    const auto& next = rec.subRecords[i + 1];
                    if (std::memcmp(next.tag, "SCVR", 4) == 0 && !next.data.empty()) {
                        var.name = std::string(
                            reinterpret_cast<const char*>(next.data.data()),
                            next.data.size());
                        // Remove null terminator if present
                        if (!var.name.empty() && var.name.back() == '\0') {
                            var.name.pop_back();
                        }
                    }
                }

                std::string key = var.name;
                for (char& c : key) {
                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                }
                auto declared = declaredTypes.find(key);
                if (declared != declaredTypes.end()) {
                    var.type = declared->second;
                } else if (std::find(referenceVariables.begin(), referenceVariables.end(),
                                    index) != referenceVariables.end()) {
                    var.type = script::ScriptValue::Type::Ref;
                } else if (sub.size() > 16 && data[16] != 0) {
                    var.type = script::ScriptValue::Type::Integer;
                } else {
                    var.type = script::ScriptValue::Type::Float;
                }

                script.variables.push_back(std::move(var));
            }
        }

        // The variable count is not stored in SCHR: derive it from the decoded pairs
        script.varCount = static_cast<uint32_t>(script.variables.size());

        // Type census, so an APK run can confirm the SCTX-derived types against the
        // measured distribution (int 5,100 / float 1,170 / ref 996 = 7,266).
        if (!script.variables.empty()) {
            uint32_t intVars = 0, floatVars = 0, refVars = 0;
            for (const auto& var : script.variables) {
                switch (var.type) {
                    case script::ScriptValue::Type::Float: ++floatVars; break;
                    case script::ScriptValue::Type::Ref:   ++refVars;   break;
                    default:                               ++intVars;   break;
                }
            }
            LOGD("  SCPT vars: 0x%08X '%s' count=%u int=%u float=%u ref=%u",
                 script.formID, script.editorID.c_str(), script.varCount,
                 intVars, floatVars, refVars);
        }

        // SCRO: object references (4 bytes each - FormID)
        for (const auto& sub : rec.subRecords) {
            if (std::memcmp(sub.tag, "SCRO", 4) == 0 && sub.size() >= 4) {
                uint32_t refFormID;
                std::memcpy(&refFormID, sub.data.data(), 4);
                script.references.push_back(refFormID);
            }
        }

        m_scripts.push_back(std::move(script));
    }

    // ============================================================================
    // Phase 64 — decoders for the remaining Oblivion record types
    // ============================================================================

    void ESMFile::decodeGameSetting(const ESMRecord& rec) {
        GameSettingData gmst;
        gmst.formID = rec.formID;
        gmst.editorID = rec.getString("EDID");
        gmst.valueType = gmst.editorID.empty() ? 's' : gmst.editorID[0];

        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() > 0) {
            if (gmst.valueType == 's') {
                gmst.stringValue = rec.getString("DATA");
            } else if (data->size() >= 4) {
                gmst.numericValue = readF32(data->data.data());
            }
        }

        m_gameSettings.push_back(std::move(gmst));
    }

    void ESMFile::decodeGlobalVariable(const ESMRecord& rec) {
        GlobalVariableData glob;
        glob.formID = rec.formID;
        glob.editorID = rec.getString("EDID");

        auto* fnam = rec.findSubRecord("FNAM");
        if (fnam && fnam->size() >= 1) {
            glob.valueType = static_cast<char>(fnam->data[0]);
        }

        auto* fltv = rec.findSubRecord("FLTV");
        if (fltv && fltv->size() >= 4) {
            glob.value = readF32(fltv->data.data());
        }

        m_globalVariables.push_back(std::move(glob));
    }

    void ESMFile::decodeDoor(const ESMRecord& rec) {
        DoorData door;
        door.formID = rec.formID;
        door.editorID = rec.getString("EDID");
        door.fullName = rec.getString("FULL");
        door.modelPath = rec.getString("MODL");
        door.scriptFormID = rec.getFormID("SCRI");
        door.openSoundFormID = rec.getFormID("SNAM");
        door.closeSoundFormID = rec.getFormID("ANAM");

        auto* fnam = rec.findSubRecord("FNAM");
        if (fnam && fnam->size() >= 1) door.flags = fnam->data[0];

        m_doors.push_back(std::move(door));
    }

    void ESMFile::decodePackage(const ESMRecord& rec) {
        AIPackageData pack;
        pack.formID = rec.formID;
        pack.editorID = rec.getString("EDID");

        auto* pkdt = rec.findSubRecord("PKDT");
        if (pkdt && pkdt->size() >= 8) {
            pack.packageFlags = readI32(pkdt->data.data());
            pack.packageType = pkdt->data[4];
        }

        for (const auto& sub : rec.subRecords) {
            if (std::memcmp(sub.tag, "PLDT", 4) == 0 && sub.size() >= 12) {
                pack.locationType = readI32(sub.data.data());
                pack.locationFormID = readI32(sub.data.data() + 4);
                pack.locationRadius = readI32(sub.data.data() + 8);
            } else if (std::memcmp(sub.tag, "PTDT", 4) == 0 && sub.size() >= 12) {
                pack.targetKind = readI32(sub.data.data());
                pack.targetFormID = readI32(sub.data.data() + 4);
                pack.targetCount = readI32(sub.data.data() + 8);
            } else if (std::memcmp(sub.tag, "PSDT", 4) == 0 && sub.size() >= 8) {
                pack.targetKind = readI32(sub.data.data());
            } else if (std::memcmp(sub.tag, "CTDA", 4) == 0) {
                pack.conditionCount++;
            }
        }

        m_packages.push_back(std::move(pack));
    }

    void ESMFile::decodePathGrid(const ESMRecord& rec) {
        PathGridData grid;
        grid.formID = rec.formID;
        grid.cellFormID = rec.parentFormID;

        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 2) {
            grid.gridX = static_cast<int8_t>(data->data[0]);
            grid.gridY = static_cast<int8_t>(data->data[1]);
        }

        // PGRP: 16 bytes per point (x, y, z floats followed by a uint32)
        auto* pgrp = rec.findSubRecord("PGRP");
        if (pgrp) {
            const size_t count = pgrp->size() / 16;
            grid.points.reserve(count);
            for (size_t i = 0; i < count; i++) {
                glm::vec3 point;
                std::memcpy(&point, pgrp->data.data() + i * 16, 12);
                grid.points.push_back(point);
            }
        }

        // PGAG: one bit per point, packed into ceil(N/8) bytes
        auto* pgag = rec.findSubRecord("PGAG");
        if (pgag) {
            grid.pointFlags = pgag->data;
        }

        // PGRR: 4 bytes per edge (two uint16 point indices)
        auto* pgrr = rec.findSubRecord("PGRR");
        if (pgrr) {
            const size_t count = pgrr->size() / 4;
            grid.edges.reserve(count);
            for (size_t i = 0; i < count; i++) {
                uint16_t a = readU16(pgrr->data.data() + i * 4);
                uint16_t b = readU16(pgrr->data.data() + i * 4 + 2);
                grid.edges.push_back({a, b});
            }
        }

        auto* pgrl = rec.findSubRecord("PGRL");
        if (pgrl) {
            grid.roadData = pgrl->data;
        }

        m_pathGrids.push_back(std::move(grid));
    }

    void ESMFile::decodeIdleAnimation(const ESMRecord& rec) {
        IdleAnimationData idle;
        idle.formID = rec.formID;
        idle.editorID = rec.getString("EDID");
        idle.modelPath = rec.getString("MODL");

        auto* anam = rec.findSubRecord("ANAM");
        if (anam && anam->size() >= 1) idle.animationGroup = anam->data[0];

        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 8) {
            idle.parentFormID = readI32(data->data.data() + 4);
        }

        for (const auto& sub : rec.subRecords) {
            if (std::memcmp(sub.tag, "CTDA", 4) == 0) idle.conditionCount++;
        }

        m_idleAnimations.push_back(std::move(idle));
    }

    void ESMFile::decodeKey(const ESMRecord& rec) {
        KeyData key;
        key.formID = rec.formID;
        key.editorID = rec.getString("EDID");
        key.fullName = rec.getString("FULL");
        key.modelPath = rec.getString("MODL");
        key.iconPath = rec.getString("ICON");

        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 8) {
            key.value = readI32(data->data.data());
            key.weight = readF32(data->data.data() + 4);
        }

        m_keys.push_back(std::move(key));
    }

    void ESMFile::decodeAmmo(const ESMRecord& rec) {
        AmmoData ammo;
        ammo.formID = rec.formID;
        ammo.editorID = rec.getString("EDID");
        ammo.fullName = rec.getString("FULL");
        ammo.modelPath = rec.getString("MODL");
        ammo.iconPath = rec.getString("ICON");
        ammo.enchantmentFormID = rec.getFormID("ENAM");

        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 18) {
            ammo.speed = readF32(data->data.data());
            ammo.flags = readI32(data->data.data() + 4);
            ammo.value = readI32(data->data.data() + 8);
            ammo.weight = readF32(data->data.data() + 12);
            ammo.damage = readU16(data->data.data() + 16);
        }

        m_ammo.push_back(std::move(ammo));
    }

    void ESMFile::decodeSigilStone(const ESMRecord& rec) {
        SigilStoneData stone;
        stone.formID = rec.formID;
        stone.editorID = rec.getString("EDID");
        stone.fullName = rec.getString("FULL");
        stone.modelPath = rec.getString("MODL");
        stone.iconPath = rec.getString("ICON");
        stone.scriptFormID = rec.getFormID("SCRI");

        for (const auto& sub : rec.subRecords) {
            if (std::memcmp(sub.tag, "EFID", 4) == 0 && sub.size() >= 4) {
                stone.effectCodes.push_back(efidCode(sub.data.data()));
                stone.effectFormIDs.push_back(0);
            } else if (std::memcmp(sub.tag, "EFIT", 4) == 0 && sub.size() >= 24) {
                stone.effectMagnitudes.push_back(readFloatOrInt(sub.data.data() + 4));
                stone.effectAreas.push_back(readFloatOrInt(sub.data.data() + 8));
                stone.effectDurations.push_back(readFloatOrInt(sub.data.data() + 12));
                stone.effectActorValues.push_back(readI32(sub.data.data() + 20));
            }
        }

        // DATA is 9 bytes: value (int32), 1 unused byte, weight (float)
        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 9) {
            stone.value = readI32(data->data.data());
            stone.weight = readF32(data->data.data() + 5);
        }

        m_sigilStones.push_back(std::move(stone));
    }

    void ESMFile::decodeSoulGem(const ESMRecord& rec) {
        SoulGemData gem;
        gem.formID = rec.formID;
        gem.editorID = rec.getString("EDID");
        gem.fullName = rec.getString("FULL");
        gem.modelPath = rec.getString("MODL");
        gem.iconPath = rec.getString("ICON");
        gem.scriptFormID = rec.getFormID("SCRI");

        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 8) {
            gem.value = readI32(data->data.data());
            gem.weight = readF32(data->data.data() + 4);
        }

        auto* soul = rec.findSubRecord("SOUL");
        if (soul && soul->size() >= 1) gem.currentSoul = soul->data[0];

        auto* slcp = rec.findSubRecord("SLCP");
        if (slcp && slcp->size() >= 1) gem.soulCapacity = slcp->data[0];

        m_soulGems.push_back(std::move(gem));
    }

    void ESMFile::decodeFurniture(const ESMRecord& rec) {
        FurnitureData furn;
        furn.formID = rec.formID;
        furn.editorID = rec.getString("EDID");
        furn.fullName = rec.getString("FULL");
        furn.modelPath = rec.getString("MODL");

        auto* mnam = rec.findSubRecord("MNAM");
        if (mnam && mnam->size() >= 4) {
            furn.rawMnam = readI32(mnam->data.data());
            furn.furnitureType = mnam->data[0];
        }

        m_furniture.push_back(std::move(furn));
    }

    void ESMFile::decodeLandscapeTexture(const ESMRecord& rec) {
        LandscapeTextureData ltex;
        ltex.formID = rec.formID;
        ltex.editorID = rec.getString("EDID");

        // LTEX ICON stores a path relative to "textures\landscape\" and omits that
        // root (e.g. "TerrainHDGrass01SU.dds", "Dementia\DementiaMoss01.dds").
        // BSA lookups need the full stored path, so normalize it here.
        std::string icon = rec.getString("ICON");
        for (char& ch : icon) {
            if (ch == '/') ch = '\\';
        }
        if (!icon.empty() && !startsWithNoCase(icon, "textures\\")) {
            icon = "textures\\landscape\\" + icon;
        }
        ltex.iconPath = icon;

        auto* hnam = rec.findSubRecord("HNAM");
        if (hnam && hnam->size() >= 3) {
            ltex.hnam[0] = hnam->data[0];
            ltex.hnam[1] = hnam->data[1];
            ltex.hnam[2] = hnam->data[2];
        }

        auto* snam = rec.findSubRecord("SNAM");
        if (snam && snam->size() >= 1) ltex.materialType = snam->data[0];

        m_landscapeTextures.push_back(std::move(ltex));
    }

    void ESMFile::decodeGrass(const ESMRecord& rec) {
        GrassData grass;
        grass.formID = rec.formID;
        grass.editorID = rec.getString("EDID");
        grass.modelPath = rec.getString("MODL");

        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 32) {
            grass.density = data->data[0];
            grass.minSlope = data->data[1];
            grass.maxSlope = data->data[2];
            grass.positionRange = readF32(data->data.data() + 12);
            grass.heightRange = readF32(data->data.data() + 16);
            grass.colorRange = readF32(data->data.data() + 20);
            grass.wavePeriod = readF32(data->data.data() + 24);
            grass.flags = readI32(data->data.data() + 28);
        }

        m_grass.push_back(std::move(grass));
    }

    void ESMFile::decodeWater(const ESMRecord& rec) {
        WaterData water;
        water.formID = rec.formID;
        water.editorID = rec.getString("EDID");
        water.texturePath = rec.getString("TNAM");

        auto* anam = rec.findSubRecord("ANAM");
        if (anam && anam->size() >= 1) water.opacity = anam->data[0];

        auto* fnam = rec.findSubRecord("FNAM");
        if (fnam && fnam->size() >= 1) water.flags = fnam->data[0];

        auto* mnam = rec.findSubRecord("MNAM");
        if (mnam && mnam->size() >= 1) water.materialType = mnam->data[0];

        water.soundFormID = rec.getFormID("SNAM");

        auto* data = rec.findSubRecord("DATA");
        if (data) {
            water.shaderData = data->data;
            const size_t floatCount = data->size() / 4;
            water.shaderFloats.reserve(floatCount);
            for (size_t i = 0; i < floatCount; i++) {
                water.shaderFloats.push_back(readF32(data->data.data() + i * 4));
            }
        }

        auto* gnam = rec.findSubRecord("GNAM");
        if (gnam && gnam->size() >= 12) {
            water.damage[0] = readF32(gnam->data.data());
            water.damage[1] = readF32(gnam->data.data() + 4);
            water.damage[2] = readF32(gnam->data.data() + 8);
        }

        m_waters.push_back(std::move(water));
    }

    void ESMFile::decodeWeather(const ESMRecord& rec) {
        WeatherData weather;
        weather.formID = rec.formID;
        weather.editorID = rec.getString("EDID");
        weather.cloudTextureUpper = rec.getString("CNAM");
        weather.cloudTextureLower = rec.getString("DNAM");

        // NAM0: 4 time-of-day blocks of 40 bytes (10 uint32 each)
        auto* nam0 = rec.findSubRecord("NAM0");
        if (nam0 && nam0->size() >= 160) {
            for (int block = 0; block < 4; block++) {
                const uint8_t* base = nam0->data.data() + block * 40;
                WeatherSkyColors& colors = weather.sky[block];
                colors.upperSky = static_cast<uint32_t>(readI32(base));
                colors.fog = static_cast<uint32_t>(readI32(base + 4));
                colors.unknown = static_cast<uint32_t>(readI32(base + 8));
                colors.clouds = static_cast<uint32_t>(readI32(base + 12));
                for (int d = 0; d < 6; d++) {
                    colors.detail[d] = static_cast<uint32_t>(readI32(base + 16 + d * 4));
                }
            }
        }

        auto* fnam = rec.findSubRecord("FNAM");
        if (fnam && fnam->size() >= 16) {
            weather.fogDayNear = readF32(fnam->data.data());
            weather.fogDayFar = readF32(fnam->data.data() + 4);
            weather.fogNightNear = readF32(fnam->data.data() + 8);
            weather.fogNightFar = readF32(fnam->data.data() + 12);
        }

        auto* hnam = rec.findSubRecord("HNAM");
        if (hnam) {
            const size_t floatCount = hnam->size() / 4;
            weather.hnamFloats.reserve(floatCount);
            for (size_t i = 0; i < floatCount; i++) {
                weather.hnamFloats.push_back(readF32(hnam->data.data() + i * 4));
            }
        }

        auto* data = rec.findSubRecord("DATA");
        if (data) {
            weather.rawData = data->data;
            if (data->size() >= 1) weather.windSpeed = data->data[0];
            if (data->size() >= 13) weather.weatherClassification = data->data[12];
        }

        for (const auto& sub : rec.subRecords) {
            if (std::memcmp(sub.tag, "SNAM", 4) == 0 && sub.size() >= 8) {
                WeatherSound sound;
                sound.formID = static_cast<uint32_t>(readI32(sub.data.data()));
                sound.type = static_cast<uint32_t>(readI32(sub.data.data() + 4));
                weather.sounds.push_back(sound);
            }
        }

        m_weathers.push_back(std::move(weather));
    }

    void ESMFile::decodeCombatStyle(const ESMRecord& rec) {
        CombatStyleData style;
        style.formID = rec.formID;
        style.editorID = rec.getString("EDID");

        auto* cstd = rec.findSubRecord("CSTD");
        if (cstd) {
            style.data = cstd->data;
            if (cstd->size() >= 2) style.weaponFlags = readU16(cstd->data.data());
        }

        m_combatStyles.push_back(std::move(style));
    }

    void ESMFile::decodeLoadScreen(const ESMRecord& rec) {
        LoadScreenData screen;
        screen.formID = rec.formID;
        screen.editorID = rec.getString("EDID");
        screen.iconPath = rec.getString("ICON");
        screen.description = rec.getString("DESC");

        auto* lnam = rec.findSubRecord("LNAM");
        if (lnam) {
            screen.lnam = lnam->data;
            if (lnam->size() >= 4) screen.lnamFormID = static_cast<uint32_t>(readI32(lnam->data.data()));
        }

        m_loadScreens.push_back(std::move(screen));
    }

    void ESMFile::decodeEffectShader(const ESMRecord& rec) {
        EffectShaderData shader;
        shader.formID = rec.formID;
        shader.editorID = rec.getString("EDID");
        shader.fillTexture = rec.getString("ICON");
        shader.particleTexture = rec.getString("ICO2");

        auto* data = rec.findSubRecord("DATA");
        if (data) shader.data = data->data;

        m_effectShaders.push_back(std::move(shader));
    }

    void ESMFile::decodeAnimationObject(const ESMRecord& rec) {
        AnimationObjectData anio;
        anio.formID = rec.formID;
        anio.editorID = rec.getString("EDID");
        anio.modelPath = rec.getString("MODL");

        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 4) {
            anio.animationGroupFormID = static_cast<uint32_t>(readI32(data->data.data()));
        }

        m_animationObjects.push_back(std::move(anio));
    }

    void ESMFile::decodeSubspace(const ESMRecord& rec) {
        SubspaceData sbsp;
        sbsp.formID = rec.formID;
        sbsp.editorID = rec.getString("EDID");

        auto* dnam = rec.findSubRecord("DNAM");
        if (dnam && dnam->size() >= 12) {
            sbsp.x = readF32(dnam->data.data());
            sbsp.y = readF32(dnam->data.data() + 4);
            sbsp.z = readF32(dnam->data.data() + 8);
        }

        m_subspaces.push_back(std::move(sbsp));
    }

bool ESMFile::readRecordHeaderMem(const uint8_t*& pos, const uint8_t* end, ESMRecord& rec) {
    if (pos + 20 > end) return false;
    
    std::memcpy(rec.recType, pos, 4); pos += 4;
    
    uint32_t rawSize;
    std::memcpy(&rawSize, pos, 4); pos += 4;
    std::memcpy(&rec.flags, pos, 4); pos += 4;
    std::memcpy(&rec.formID, pos, 4); pos += 4;
    // Records carry a 2 byte version and a 2 byte unknown field after the form ID,
    // so the header is 20 bytes. Skipping only 16 desynchronises every following
    // read and the whole file decodes to nothing.
    pos += 4;
    
    if (std::memcmp(rec.recType, "GRUP", 4) == 0) {
        LOGE("readRecordHeaderMem called on GRUP");
        return false;
    }
    
    bool compressed = (rec.flags & REC_FLAG_COMPRESSED) != 0;
    rec.dataSize = rawSize & 0x00FFFFFF;
    
    // If compressed, decompress into subRecords vector
    if (compressed) {
        if (pos + 4 > end) return false;
        uint32_t decompSize;
        std::memcpy(&decompSize, pos, 4); pos += 4;
        
        uint32_t compSize = rec.dataSize - 4;
        if (pos + compSize > end) return false;
        
        // Decompress
        z_stream strm;
        std::memset(&strm, 0, sizeof(strm));
        strm.next_in = const_cast<uint8_t*>(pos);
        strm.avail_in = compSize;
        
        std::vector<uint8_t> decompBuf(decompSize);
        strm.next_out = decompBuf.data();
        strm.avail_out = decompSize;
        
        int ret = inflateInit(&strm);
        if (ret != Z_OK) {
            LOGE("inflateInit failed: %d", ret);
            return false;
        }
        ret = inflate(&strm, Z_FINISH);
        if (ret != Z_STREAM_END) {
            LOGE("inflate failed: %d", ret);
            inflateEnd(&strm);
            return false;
        }
        inflateEnd(&strm);
        
        pos += compSize;
        
        // Parse decompressed data as subrecords
        const uint8_t* subPos = decompBuf.data();
        const uint8_t* subEnd = subPos + decompSize;
        while (subPos < subEnd) {
            SubRecord sub;
            if (!readSubRecordMem(subPos, subEnd, sub)) break;
            rec.subRecords.push_back(std::move(sub));
        }
    } else {
        // Uncompressed: parse subrecords directly
        const uint8_t* subPos = pos;
        const uint8_t* subEnd = pos + rec.dataSize;
        if (subEnd > end) subEnd = end;
        while (subPos < subEnd) {
            SubRecord sub;
            if (!readSubRecordMem(subPos, subEnd, sub)) break;
            rec.subRecords.push_back(std::move(sub));
        }
        pos = subEnd;
    }
    
    return true;
}

bool ESMFile::readSubRecordMem(const uint8_t*& pos, const uint8_t* end, SubRecord& sub) {
    if (pos + 6 > end) return false;
    
    std::memcpy(sub.tag, pos, 4); pos += 4;
    
    uint16_t size;
    std::memcpy(&size, pos, 2); pos += 2;
    uint16_t realSize = size & 0xFFFF;
    
    if (pos + realSize > end) return false;
    
    sub.data.assign(pos, pos + realSize);
    pos += realSize;
    
    return true;
}

// GRUP header on disk is tag(4) + groupSize(4) + label(4) + groupType(4) + stamp(2) + unknown(2).
// GroupHeader models only the first 16 bytes, so the trailing 4 must be skipped explicitly.
static constexpr uint32_t GRUP_HEADER_SIZE = 20;

bool ESMFile::readGroupMem(const uint8_t*& pos, const uint8_t* end, GroupType groupType, uint32_t groupSize) {
    if (pos + groupSize > end) {
        LOGE("readGroupMem: group size exceeds buffer");
        return false;
    }
    
    const uint8_t* groupEnd = pos + groupSize;
    
    LOGD("readGroupMem: type=%u, size=%u", static_cast<uint32_t>(groupType), groupSize);
    
    while (pos < groupEnd) {
        if (pos + 4 > groupEnd) break;
        
        char peekTag[4];
        std::memcpy(peekTag, pos, 4);
        
        if (peekTag[0] == 'G' && peekTag[1] == 'R' && peekTag[2] == 'U' && peekTag[3] == 'P') {
            // Nested GRUP
            GroupHeader gh;
            std::memcpy(&gh, pos, sizeof(gh)); pos += GRUP_HEADER_SIZE;
            if (gh.groupSize < GRUP_HEADER_SIZE) break;
            
            uint32_t childSize = gh.groupSize - GRUP_HEADER_SIZE;
            uint32_t savedCell = m_currentCellFormID;
            uint32_t savedWorld = m_currentWorldspaceFormID;
            if (gh.groupType == 6) m_currentCellFormID = gh.groupLabel;
            if (gh.groupType == 1) m_currentWorldspaceFormID = gh.groupLabel;
            readGroupMem(pos, groupEnd, static_cast<GroupType>(gh.groupType), childSize);
            m_currentCellFormID = savedCell;
            m_currentWorldspaceFormID = savedWorld;
        } else {
            // Regular record
            ESMRecord rec;
            if (!readRecordHeaderMem(pos, groupEnd, rec)) break;
            rec.parentFormID = m_currentCellFormID;
            rec.worldspaceFormID = m_currentWorldspaceFormID;
            decodeRecord(rec);
        }
    }
    
    return true;
}

bool ESMFile::parseFromMemory(const std::string& name, const uint8_t* data, size_t dataSize) {
    m_fileName = name;
    size_t ext = m_fileName.rfind('.');
    m_isMaster = (ext != std::string::npos &&
                  (m_fileName.substr(ext) == ".esm" ||
                   m_fileName.substr(ext) == ".ESM"));
    
    LOGD("Parsing ESM from memory: %s (master=%s, %zu bytes)",
         m_fileName.c_str(), m_isMaster ? "yes" : "no", dataSize);
    
    const uint8_t* pos = data;
    const uint8_t* end = data + dataSize;
    
    // Read TES4 header
    ESMRecord header;
    if (!readRecordHeaderMem(pos, end, header)) {
        LOGE("Failed to read TES4 header from memory");
        return false;
    }
    
    if (std::memcmp(header.recType, "TES4", 4) != 0) {
        LOGE("Invalid ESM from memory: expected TES4, got %.4s", header.recType);
        return false;
    }
    
    LOGD("TES4 header parsed, formID=0x%08X, flags=0x%08X", header.formID, header.flags);
    
    // Parse remaining records
    while (pos < end) {
        if (pos + 4 > end) break;
        
        char peekTag[4];
        std::memcpy(peekTag, pos, 4);
        
        if (peekTag[0] == 'G' && peekTag[1] == 'R' && peekTag[2] == 'U' && peekTag[3] == 'P') {
            GroupHeader gh;
            std::memcpy(&gh, pos, sizeof(gh)); pos += GRUP_HEADER_SIZE;
            if (gh.groupSize < GRUP_HEADER_SIZE) break;
            
            uint32_t childSize = gh.groupSize - GRUP_HEADER_SIZE;
            uint32_t savedCell = m_currentCellFormID;
            if (gh.groupType == 6) m_currentCellFormID = gh.groupLabel;
            readGroupMem(pos, end, static_cast<GroupType>(gh.groupType), childSize);
            m_currentCellFormID = savedCell;
        } else {
            ESMRecord rec;
            if (!readRecordHeaderMem(pos, end, rec)) break;
            rec.parentFormID = m_currentCellFormID;
            decodeRecord(rec);
        }
    }
    
    LOGD("ESM parsed from memory: %zu cells, %zu NPCs, %zu weapons, %zu quests, %zu dialogs",
         m_cells.size(), m_npcs.size(), m_weapons.size(), m_quests.size(), m_dialogs.size());
    LOGD("ESM secondary types: GMST=%zu GLOB=%zu DOOR=%zu PACK=%zu PGRD=%zu IDLE=%zu "
         "KEYM=%zu AMMO=%zu SGST=%zu SLGM=%zu FURN=%zu LTEX=%zu GRAS=%zu WATR=%zu "
         "WTHR=%zu CSTY=%zu LSCR=%zu EFSH=%zu ANIO=%zu SBSP=%zu",
         m_gameSettings.size(), m_globalVariables.size(), m_doors.size(), m_packages.size(),
         m_pathGrids.size(), m_idleAnimations.size(), m_keys.size(), m_ammo.size(),
         m_sigilStones.size(), m_soulGems.size(), m_furniture.size(),
         m_landscapeTextures.size(), m_grass.size(), m_waters.size(), m_weathers.size(),
         m_combatStyles.size(), m_loadScreens.size(), m_effectShaders.size(),
         m_animationObjects.size(), m_subspaces.size());

    logScriptTypeCensus(m_scripts);

    logWaterCensus(m_exteriorCellsWithWater, m_exteriorCellsNoWaterSentinel,
                   m_exteriorCellsNoXclw);

    resolveMagicReferences();

    return true;
}

ESMFile::VerificationResult ESMFile::verify() const {
    VerificationResult result;

    // Count records
    result.npcCount = static_cast<int>(m_npcs.size());
    result.cellCount = static_cast<int>(m_cells.size());
    result.weaponCount = static_cast<int>(m_weapons.size());
    result.questCount = static_cast<int>(m_quests.size());
    result.referenceCount = static_cast<int>(m_references.size());
    result.recordCount = result.npcCount + result.cellCount + result.weaponCount +
                         result.questCount + result.referenceCount +
                         static_cast<int>(m_creatures.size()) +
                         static_cast<int>(m_spells.size()) +
                         static_cast<int>(m_dialogs.size());

    // Validate NPCs
    for (const auto& npc : m_npcs) {
        if (npc.formID == 0) {
            result.errors.push_back("NPC with formID=0: " + npc.editorID);
            result.valid = false;
        }
        if (npc.editorID.empty()) {
            result.warnings.push_back("NPC with empty editorID, formID=0x" +
                                      std::to_string(npc.formID));
        }
    }

    // Validate cells
    for (const auto& cell : m_cells) {
        if (cell.formID == 0) {
            result.errors.push_back("Cell with formID=0");
            result.valid = false;
        }
    }

    // Validate weapons
    for (const auto& weap : m_weapons) {
        if (weap.formID == 0) {
            result.errors.push_back("Weapon with formID=0: " + weap.editorID);
            result.valid = false;
        }
    }

    // Validate references (REFR)
    for (const auto& ref : m_references) {
        if (ref.formID == 0) {
            result.warnings.push_back("Reference with formID=0");
        }
    }

    // Validate quests
    for (const auto& quest : m_quests) {
        if (quest.formID == 0) {
            result.errors.push_back("Quest with formID=0: " + quest.editorID);
            result.valid = false;
        }
    }

    LOGI("ESM verification: records=%d, npcs=%d, cells=%d, weapons=%d, quests=%d, refs=%d, errors=%zu, warnings=%zu",
         result.recordCount, result.npcCount, result.cellCount, result.weaponCount,
         result.questCount, result.referenceCount, result.errors.size(), result.warnings.size());

    return result;
}

void ESMFile::resolveMagicReferences(
    const std::unordered_map<std::string, uint32_t>* externalFormIDs,
    const std::unordered_map<uint32_t, uint32_t>* externalSchools) {
    std::unordered_map<std::string, uint32_t> formIDByCode;
    std::unordered_map<uint32_t, uint32_t> schoolByFormID;
    formIDByCode.reserve(m_magicEffects.size() * 2);
    schoolByFormID.reserve(m_magicEffects.size() * 2);
    for (const auto& mgef : m_magicEffects) {
        if (!mgef.editorID.empty()) {
            formIDByCode.emplace(mgef.editorID, mgef.formID);
        }
        schoolByFormID.emplace(mgef.formID, mgef.school);
    }

    size_t resolved = 0;
    size_t unresolved = 0;
    auto lookupFormID = [&](const std::string& code) -> uint32_t {
        auto it = formIDByCode.find(code);
        if (it != formIDByCode.end()) return it->second;
        if (externalFormIDs) {
            auto ext = externalFormIDs->find(code);
            if (ext != externalFormIDs->end()) return ext->second;
        }
        return 0;
    };
    auto lookupSchool = [&](uint32_t formID) -> int32_t {
        auto it = schoolByFormID.find(formID);
        if (it != schoolByFormID.end()) return static_cast<int32_t>(it->second);
        if (externalSchools) {
            auto ext = externalSchools->find(formID);
            if (ext != externalSchools->end()) return static_cast<int32_t>(ext->second);
        }
        return -1;
    };

    auto resolveEffects = [&](auto& records) {
        for (auto& rec : records) {
            for (size_t i = 0; i < rec.effectCodes.size() && i < rec.effectFormIDs.size(); ++i) {
                if (rec.effectCodes[i].empty()) continue;
                uint32_t formID = lookupFormID(rec.effectCodes[i]);
                if (formID == 0) {
                    ++unresolved;
                    continue;
                }
                rec.effectFormIDs[i] = formID;
                ++resolved;
            }
        }
    };
    resolveEffects(m_spells);
    resolveEffects(m_enchantments);
    resolveEffects(m_ingredients);
    resolveEffects(m_alchemy);
    resolveEffects(m_sigilStones);

    // A spell's school is not stored in SPEL; it comes from its magic effects.
    size_t schooled = 0;
    for (auto& spell : m_spells) {
        for (uint32_t effectFormID : spell.effectFormIDs) {
            int32_t school = (effectFormID == 0) ? -1 : lookupSchool(effectFormID);
            if (school >= 0 && school <= 5) {
                spell.school = static_cast<uint32_t>(school);
                ++schooled;
                break;
            }
        }
    }

    LOGI("Magic references resolved for %s: %zu effect links (%zu unresolved), "
         "%zu/%zu spells have a school",
         m_fileName.c_str(), resolved, unresolved, schooled, m_spells.size());
}

// ============================================================================
// ESMManager implementation
// ============================================================================
bool ESMManager::loadPlugin(const std::string& esmPath) {
    auto file = std::make_unique<ESMFile>();
    if (!file->open(esmPath)) {
        LOGE("Failed to load plugin: %s", esmPath.c_str());
        return false;
    }
    m_files.push_back(std::move(file));
    rebuildIndices();
    LOGD("Plugin loaded. Total: %zu", m_files.size());
    return true;
}

bool ESMManager::loadPluginFromMemory(const std::string& name, const uint8_t* data, size_t dataSize) {
    LOGD("Loading plugin from memory: %s (%zu bytes)", name.c_str(), dataSize);
    auto file = std::make_unique<ESMFile>();
    if (!file->parseFromMemory(name, data, dataSize)) {
        LOGE("Failed to load plugin from memory: %s", name.c_str());
        return false;
    }
    m_files.push_back(std::move(file));
    rebuildIndices();
    LOGD("Plugin loaded from memory. Total: %zu", m_files.size());
    return true;
}

void ESMManager::cleanup() {
    m_files.clear();
    m_npcIndex.clear();
    m_cellIndex.clear();
    m_weaponIndex.clear();
    m_armorIndex.clear();
    m_spellIndex.clear();
    m_questIndex.clear();
    m_dialogIndex.clear();
    m_leveledListIndex.clear();
    m_navMeshIndex.clear();
    m_worldIndex.clear();
    m_raceIndex.clear();
    m_classIndex.clear();
    m_bookIndex.clear();
    m_clothingIndex.clear();
    m_ingredientIndex.clear();
    m_alchemyIndex.clear();
    m_miscItemIndex.clear();
    m_factionIndex.clear();
    m_scriptIndex.clear();
    m_gameSettingIndex.clear();
    m_globalVariableIndex.clear();
    m_doorIndex.clear();
    m_packageIndex.clear();
    m_pathGridIndex.clear();
    m_idleAnimationIndex.clear();
    m_keyIndex.clear();
    m_ammoIndex.clear();
    m_sigilStoneIndex.clear();
    m_soulGemIndex.clear();
    m_furnitureIndex.clear();
    m_landscapeTextureIndex.clear();
    m_grassIndex.clear();
    m_waterIndex.clear();
    m_weatherIndex.clear();
    m_combatStyleIndex.clear();
    m_loadScreenIndex.clear();
    m_effectShaderIndex.clear();
    m_animationObjectIndex.clear();
    m_subspaceIndex.clear();
}

void ESMManager::rebuildIndices() {
    m_npcIndex.clear();
    m_creatureIndex.clear();
    m_cellIndex.clear();
    m_weaponIndex.clear();
    m_armorIndex.clear();
    m_spellIndex.clear();
    m_enchantmentIndex.clear();
    m_magicEffectIndex.clear();
    m_magicEffectByEditorID.clear();
    m_skillIndex.clear();
    m_birthsignIndex.clear();
    m_containerIndex.clear();
    m_lightIndex.clear();
    m_staticIndex.clear();
    m_soundIndex.clear();
    m_treeIndex.clear();
    m_floraIndex.clear();
    m_activatorIndex.clear();
    m_apparatusIndex.clear();
    m_eyesIndex.clear();
    m_hairIndex.clear();
    m_climateIndex.clear();
    m_regionIndex.clear();
    m_questIndex.clear();
    m_dialogIndex.clear();
    m_leveledListIndex.clear();
    m_navMeshIndex.clear();
    m_worldIndex.clear();
    m_raceIndex.clear();
    m_classIndex.clear();
    m_bookIndex.clear();
    m_clothingIndex.clear();
    m_ingredientIndex.clear();
    m_alchemyIndex.clear();
    m_miscItemIndex.clear();
    m_factionIndex.clear();
    m_gameSettingIndex.clear();
    m_globalVariableIndex.clear();
    m_doorIndex.clear();
    m_packageIndex.clear();
    m_pathGridIndex.clear();
    m_idleAnimationIndex.clear();
    m_keyIndex.clear();
    m_ammoIndex.clear();
    m_sigilStoneIndex.clear();
    m_soulGemIndex.clear();
    m_furnitureIndex.clear();
    m_landscapeTextureIndex.clear();
    m_grassIndex.clear();
    m_waterIndex.clear();
    m_weatherIndex.clear();
    m_combatStyleIndex.clear();
    m_loadScreenIndex.clear();
    m_effectShaderIndex.clear();
    m_animationObjectIndex.clear();
    m_subspaceIndex.clear();

    for (size_t fi = 0; fi < m_files.size(); ++fi) {
        const auto& file = m_files[fi];
        for (size_t i = 0; i < file->getNPCs().size(); ++i) {
            m_npcIndex[file->getNPCs()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getCreatures().size(); ++i) {
            m_creatureIndex[file->getCreatures()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getCells().size(); ++i) {
            m_cellIndex[file->getCells()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getWeapons().size(); ++i) {
            m_weaponIndex[file->getWeapons()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getArmors().size(); ++i) {
            m_armorIndex[file->getArmors()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getSpells().size(); ++i) {
            m_spellIndex[file->getSpells()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getEnchantments().size(); ++i) {
            m_enchantmentIndex[file->getEnchantments()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getMagicEffects().size(); ++i) {
            m_magicEffectIndex[file->getMagicEffects()[i].formID] = fi;
            if (!file->getMagicEffects()[i].editorID.empty()) {
                m_magicEffectByEditorID[file->getMagicEffects()[i].editorID] =
                    &file->getMagicEffects()[i];
            }
        }
        for (size_t i = 0; i < file->getSkills().size(); ++i) {
            m_skillIndex[file->getSkills()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getBirthsigns().size(); ++i) {
            m_birthsignIndex[file->getBirthsigns()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getContainers().size(); ++i) {
            m_containerIndex[file->getContainers()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getLights().size(); ++i) {
            m_lightIndex[file->getLights()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getStatics().size(); ++i) {
            m_staticIndex[file->getStatics()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getSounds().size(); ++i) {
            m_soundIndex[file->getSounds()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getTrees().size(); ++i) {
            m_treeIndex[file->getTrees()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getFloras().size(); ++i) {
            m_floraIndex[file->getFloras()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getActivators().size(); ++i) {
            m_activatorIndex[file->getActivators()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getApparatuses().size(); ++i) {
            m_apparatusIndex[file->getApparatuses()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getEyes().size(); ++i) {
            m_eyesIndex[file->getEyes()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getHairs().size(); ++i) {
            m_hairIndex[file->getHairs()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getClimates().size(); ++i) {
            m_climateIndex[file->getClimates()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getRegions().size(); ++i) {
            m_regionIndex[file->getRegions()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getQuests().size(); ++i) {
            m_questIndex[file->getQuests()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getDialogs().size(); ++i) {
            m_dialogIndex[file->getDialogs()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getLeveledLists().size(); ++i) {
            m_leveledListIndex[file->getLeveledLists()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getNavMeshes().size(); ++i) {
            m_navMeshIndex[file->getNavMeshes()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getWorlds().size(); ++i) {
            m_worldIndex[file->getWorlds()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getRaces().size(); ++i) {
            m_raceIndex[file->getRaces()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getClasses().size(); ++i) {
            m_classIndex[file->getClasses()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getBooks().size(); ++i) {
            m_bookIndex[file->getBooks()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getClothing().size(); ++i) {
            m_clothingIndex[file->getClothing()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getIngredients().size(); ++i) {
            m_ingredientIndex[file->getIngredients()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getAlchemy().size(); ++i) {
            m_alchemyIndex[file->getAlchemy()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getMiscItems().size(); ++i) {
            m_miscItemIndex[file->getMiscItems()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getFactions().size(); ++i) {
            m_factionIndex[file->getFactions()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getScripts().size(); ++i) {
            m_scriptIndex[file->getScripts()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getGameSettings().size(); ++i) {
            m_gameSettingIndex[file->getGameSettings()[i].editorID] = fi;
        }
        for (size_t i = 0; i < file->getGlobalVariables().size(); ++i) {
            m_globalVariableIndex[file->getGlobalVariables()[i].editorID] = fi;
        }
        for (size_t i = 0; i < file->getDoors().size(); ++i) {
            m_doorIndex[file->getDoors()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getPackages().size(); ++i) {
            m_packageIndex[file->getPackages()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getPathGrids().size(); ++i) {
            const auto& grid = file->getPathGrids()[i];
            if (grid.cellFormID != 0) m_pathGridIndex[grid.cellFormID] = fi;
        }
        for (size_t i = 0; i < file->getIdleAnimations().size(); ++i) {
            m_idleAnimationIndex[file->getIdleAnimations()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getKeys().size(); ++i) {
            m_keyIndex[file->getKeys()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getAmmo().size(); ++i) {
            m_ammoIndex[file->getAmmo()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getSigilStones().size(); ++i) {
            m_sigilStoneIndex[file->getSigilStones()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getSoulGems().size(); ++i) {
            m_soulGemIndex[file->getSoulGems()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getFurniture().size(); ++i) {
            m_furnitureIndex[file->getFurniture()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getLandscapeTextures().size(); ++i) {
            m_landscapeTextureIndex[file->getLandscapeTextures()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getGrass().size(); ++i) {
            m_grassIndex[file->getGrass()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getWaters().size(); ++i) {
            m_waterIndex[file->getWaters()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getWeathers().size(); ++i) {
            m_weatherIndex[file->getWeathers()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getCombatStyles().size(); ++i) {
            m_combatStyleIndex[file->getCombatStyles()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getLoadScreens().size(); ++i) {
            m_loadScreenIndex[file->getLoadScreens()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getEffectShaders().size(); ++i) {
            m_effectShaderIndex[file->getEffectShaders()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getAnimationObjects().size(); ++i) {
            m_animationObjectIndex[file->getAnimationObjects()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getSubspaces().size(); ++i) {
            m_subspaceIndex[file->getSubspaces()[i].formID] = fi;
        }
    }

    // Second pass: link EFID codes that point at MGEFs in other plugins
    // (e.g. DLC spells referencing Oblivion.esm magic effects).
    if (!m_magicEffectByEditorID.empty()) {
        std::unordered_map<std::string, uint32_t> formIDByCode;
        std::unordered_map<uint32_t, uint32_t> schoolByFormID;
        formIDByCode.reserve(m_magicEffectByEditorID.size());
        schoolByFormID.reserve(m_magicEffectByEditorID.size());
        for (const auto& entry : m_magicEffectByEditorID) {
            formIDByCode.emplace(entry.first, entry.second->formID);
            schoolByFormID.emplace(entry.second->formID, entry.second->school);
        }
        for (auto& file : m_files) {
            file->resolveMagicReferences(&formIDByCode, &schoolByFormID);
        }
    }
}

        size_t ESMManager::findRecordsByType(const std::string& type) const {
            if (type == "CELL") return getAllCells().size();
            if (type == "NPC_") return getAllNPCs().size();
            if (type == "CREA") return getAllCreatures().size();
            if (type == "WEAP") return getAllWeapons().size();
            if (type == "QUST") return getAllQuests().size();
            if (type == "DIAL") return getAllDialogs().size();
            if (type == "REFR") return getAllReferences().size();
            if (type == "LAND") return getAllTerrains().size();
            if (type == "WRLD") return getAllWorlds().size();
            if (type == "SPEL") return getAllSpells().size();
            if (type == "ENCH") return getAllEnchantments().size();
            if (type == "MGEF") return getAllMagicEffects().size();
            if (type == "SKIL") return getAllSkills().size();
            if (type == "BSGN") return getAllBirthsigns().size();
            if (type == "CONT") return getAllContainers().size();
            if (type == "LIGH") return getAllLights().size();
            if (type == "STAT") return getAllStatics().size();
            if (type == "SOUN") return getAllSounds().size();
            if (type == "TREE") return getAllTrees().size();
            if (type == "FLOR") return getAllFloras().size();
            if (type == "ACTI") return getAllActivators().size();
            if (type == "APPA") return getAllApparatuses().size();
            if (type == "EYES") return getAllEyes().size();
            if (type == "HAIR") return getAllHairs().size();
            if (type == "CLMT") return getAllClimates().size();
            if (type == "REGN") return getAllRegions().size();
            if (type == "LVLI") return getAllLeveledLists().size();
            if (type == "LVLC") return getAllLeveledLists().size();
            if (type == "LVLN") return getAllLeveledLists().size();
            if (type == "NAVM") return getAllNavMeshes().size();
            if (type == "ARMO") return getAllArmors().size();
            if (type == "BOOK") return getAllBooks().size();
            if (type == "FACT") return getAllFactions().size();
            if (type == "RACE") return getAllRaces().size();
            if (type == "CLAS") return getAllClasses().size();
            if (type == "CLOT") return getAllClothing().size();
            if (type == "INGR") return getAllIngredients().size();
            if (type == "ALCH") return getAllAlchemy().size();
            if (type == "MISC") return getAllMiscItems().size();
            if (type == "ROAD") return getAllRoads().size();
                return 0;
            }

        const NPCData* ESMManager::findNPC(uint32_t formID) const {
    auto it = m_npcIndex.find(formID);
    if (it == m_npcIndex.end()) return nullptr;
    for (const auto& npc : m_files[it->second]->getNPCs()) {
        if (npc.formID == formID) return &npc;
    }
    return nullptr;
}

        const CreatureData* ESMManager::findCreature(uint32_t formID) const {
            auto it = m_creatureIndex.find(formID);
            if (it == m_creatureIndex.end()) return nullptr;
            for (const auto& creature : m_files[it->second]->getCreatures()) {
                if (creature.formID == formID) return &creature;
            }
            return nullptr;
        }

const CellData* ESMManager::findCell(uint32_t formID) const {
    auto it = m_cellIndex.find(formID);
    if (it == m_cellIndex.end()) return nullptr;
    for (const auto& cell : m_files[it->second]->getCells()) {
        if (cell.formID == formID) return &cell;
    }
    return nullptr;
}

const WeaponData* ESMManager::findWeapon(uint32_t formID) const {
    auto it = m_weaponIndex.find(formID);
    if (it == m_weaponIndex.end()) return nullptr;
    for (const auto& wpn : m_files[it->second]->getWeapons()) {
        if (wpn.formID == formID) return &wpn;
    }
    return nullptr;
}

const ArmorData* ESMManager::findArmor(uint32_t formID) const {
    auto it = m_armorIndex.find(formID);
    if (it == m_armorIndex.end()) return nullptr;
    for (const auto& arm : m_files[it->second]->getArmors()) {
        if (arm.formID == formID) return &arm;
    }
    return nullptr;
}

const SpellData* ESMManager::findSpell(uint32_t formID) const {
    auto it = m_spellIndex.find(formID);
    if (it == m_spellIndex.end()) return nullptr;
    for (const auto& spl : m_files[it->second]->getSpells()) {
        if (spl.formID == formID) return &spl;
    }
    return nullptr;
}

const EnchantmentData* ESMManager::findEnchantment(uint32_t formID) const {
    auto it = m_enchantmentIndex.find(formID);
    if (it == m_enchantmentIndex.end()) return nullptr;
    for (const auto& ench : m_files[it->second]->getEnchantments()) {
        if (ench.formID == formID) return &ench;
    }
    return nullptr;
}

const MagicEffectData* ESMManager::findMagicEffect(uint32_t formID) const {
    auto it = m_magicEffectIndex.find(formID);
    if (it == m_magicEffectIndex.end()) return nullptr;
    for (const auto& mgef : m_files[it->second]->getMagicEffects()) {
        if (mgef.formID == formID) return &mgef;
    }
    return nullptr;
}

const MagicEffectData* ESMManager::findMagicEffectByEditorID(const std::string& editorID) const {
    auto it = m_magicEffectByEditorID.find(editorID);
    return (it == m_magicEffectByEditorID.end()) ? nullptr : it->second;
}

const SkillData* ESMManager::findSkill(uint32_t formID) const {
    auto it = m_skillIndex.find(formID);
    if (it == m_skillIndex.end()) return nullptr;
    for (const auto& skill : m_files[it->second]->getSkills()) {
        if (skill.formID == formID) return &skill;
    }
    return nullptr;
}

const BirthsignData* ESMManager::findBirthsign(uint32_t formID) const {
    auto it = m_birthsignIndex.find(formID);
    if (it == m_birthsignIndex.end()) return nullptr;
    for (const auto& bs : m_files[it->second]->getBirthsigns()) {
        if (bs.formID == formID) return &bs;
    }
    return nullptr;
}

const ContainerData* ESMManager::findContainer(uint32_t formID) const {
    auto it = m_containerIndex.find(formID);
    if (it == m_containerIndex.end()) return nullptr;
    for (const auto& cont : m_files[it->second]->getContainers()) {
        if (cont.formID == formID) return &cont;
    }
    return nullptr;
}

const LightData* ESMManager::findLight(uint32_t formID) const {
    auto it = m_lightIndex.find(formID);
    if (it == m_lightIndex.end()) return nullptr;
    for (const auto& light : m_files[it->second]->getLights()) {
        if (light.formID == formID) return &light;
    }
    return nullptr;
}

const StaticData* ESMManager::findStatic(uint32_t formID) const {
    auto it = m_staticIndex.find(formID);
    if (it == m_staticIndex.end()) return nullptr;
    for (const auto& stat : m_files[it->second]->getStatics()) {
        if (stat.formID == formID) return &stat;
    }
    return nullptr;
}

const SoundData* ESMManager::findSound(uint32_t formID) const {
    auto it = m_soundIndex.find(formID);
    if (it == m_soundIndex.end()) return nullptr;
    for (const auto& sound : m_files[it->second]->getSounds()) {
        if (sound.formID == formID) return &sound;
    }
    return nullptr;
}

const TreeData* ESMManager::findTree(uint32_t formID) const {
    auto it = m_treeIndex.find(formID);
    if (it == m_treeIndex.end()) return nullptr;
    for (const auto& tree : m_files[it->second]->getTrees()) {
        if (tree.formID == formID) return &tree;
    }
    return nullptr;
}

const FloraData* ESMManager::findFlora(uint32_t formID) const {
    auto it = m_floraIndex.find(formID);
    if (it == m_floraIndex.end()) return nullptr;
    for (const auto& flora : m_files[it->second]->getFloras()) {
        if (flora.formID == formID) return &flora;
    }
    return nullptr;
}

const ActivatorData* ESMManager::findActivator(uint32_t formID) const {
    auto it = m_activatorIndex.find(formID);
    if (it == m_activatorIndex.end()) return nullptr;
    for (const auto& acti : m_files[it->second]->getActivators()) {
        if (acti.formID == formID) return &acti;
    }
    return nullptr;
}

const ApparatusData* ESMManager::findApparatus(uint32_t formID) const {
    auto it = m_apparatusIndex.find(formID);
    if (it == m_apparatusIndex.end()) return nullptr;
    for (const auto& appa : m_files[it->second]->getApparatuses()) {
        if (appa.formID == formID) return &appa;
    }
    return nullptr;
}

const EyesData* ESMManager::findEyes(uint32_t formID) const {
    auto it = m_eyesIndex.find(formID);
    if (it == m_eyesIndex.end()) return nullptr;
    for (const auto& eyes : m_files[it->second]->getEyes()) {
        if (eyes.formID == formID) return &eyes;
    }
    return nullptr;
}

const HairData* ESMManager::findHair(uint32_t formID) const {
    auto it = m_hairIndex.find(formID);
    if (it == m_hairIndex.end()) return nullptr;
    for (const auto& hair : m_files[it->second]->getHairs()) {
        if (hair.formID == formID) return &hair;
    }
    return nullptr;
}

const ClimateData* ESMManager::findClimate(uint32_t formID) const {
    auto it = m_climateIndex.find(formID);
    if (it == m_climateIndex.end()) return nullptr;
    for (const auto& clmt : m_files[it->second]->getClimates()) {
        if (clmt.formID == formID) return &clmt;
    }
    return nullptr;
}

const RegionData* ESMManager::findRegion(uint32_t formID) const {
    auto it = m_regionIndex.find(formID);
    if (it == m_regionIndex.end()) return nullptr;
    for (const auto& regn : m_files[it->second]->getRegions()) {
        if (regn.formID == formID) return &regn;
    }
    return nullptr;
}

const QuestData* ESMManager::findQuest(uint32_t formID) const {
    auto it = m_questIndex.find(formID);
    if (it == m_questIndex.end()) return nullptr;
    for (const auto& qst : m_files[it->second]->getQuests()) {
        if (qst.formID == formID) return &qst;
    }
    return nullptr;
}

const DialogData* ESMManager::findDialog(uint32_t formID) const {
    auto it = m_dialogIndex.find(formID);
    if (it == m_dialogIndex.end()) return nullptr;
    for (const auto& dia : m_files[it->second]->getDialogs()) {
        if (dia.formID == formID) return &dia;
    }
    return nullptr;
}

const LeveledListData* ESMManager::findLeveledList(uint32_t formID) const {
    auto it = m_leveledListIndex.find(formID);
    if (it == m_leveledListIndex.end()) return nullptr;
    for (const auto& ll : m_files[it->second]->getLeveledLists()) {
        if (ll.formID == formID) return &ll;
    }
    return nullptr;
}

const NavMeshData* ESMManager::findNavMesh(uint32_t formID) const {
    auto it = m_navMeshIndex.find(formID);
    if (it == m_navMeshIndex.end()) return nullptr;
    for (const auto& nm : m_files[it->second]->getNavMeshes()) {
        if (nm.formID == formID) return &nm;
    }
    return nullptr;
}

const WorldData* ESMManager::findWorld(uint32_t formID) const {
    auto it = m_worldIndex.find(formID);
    if (it == m_worldIndex.end()) return nullptr;
    for (const auto& w : m_files[it->second]->getWorlds()) {
        if (w.formID == formID) return &w;
    }
    return nullptr;
}

const RaceData* ESMManager::findRace(uint32_t formID) const {
    auto it = m_raceIndex.find(formID);
    if (it == m_raceIndex.end()) return nullptr;
    for (const auto& r : m_files[it->second]->getRaces()) {
        if (r.formID == formID) return &r;
    }
    return nullptr;
}

const ClassData* ESMManager::findClass(uint32_t formID) const {
    auto it = m_classIndex.find(formID);
    if (it == m_classIndex.end()) return nullptr;
    for (const auto& c : m_files[it->second]->getClasses()) {
        if (c.formID == formID) return &c;
    }
    return nullptr;
}

const BookData* ESMManager::findBook(uint32_t formID) const {
    auto it = m_bookIndex.find(formID);
    if (it == m_bookIndex.end()) return nullptr;
    for (const auto& b : m_files[it->second]->getBooks()) {
        if (b.formID == formID) return &b;
    }
    return nullptr;
}

const ClothingData* ESMManager::findClothing(uint32_t formID) const {
    auto it = m_clothingIndex.find(formID);
    if (it == m_clothingIndex.end()) return nullptr;
    for (const auto& c : m_files[it->second]->getClothing()) {
        if (c.formID == formID) return &c;
    }
    return nullptr;
}

const IngredientData* ESMManager::findIngredient(uint32_t formID) const {
    auto it = m_ingredientIndex.find(formID);
    if (it == m_ingredientIndex.end()) return nullptr;
    for (const auto& ing : m_files[it->second]->getIngredients()) {
        if (ing.formID == formID) return &ing;
    }
    return nullptr;
}

const AlchemyData* ESMManager::findAlchemy(uint32_t formID) const {
    auto it = m_alchemyIndex.find(formID);
    if (it == m_alchemyIndex.end()) return nullptr;
    for (const auto& alc : m_files[it->second]->getAlchemy()) {
        if (alc.formID == formID) return &alc;
    }
    return nullptr;
}

const MiscItemData* ESMManager::findMiscItem(uint32_t formID) const {
    auto it = m_miscItemIndex.find(formID);
    if (it == m_miscItemIndex.end()) return nullptr;
    for (const auto& misc : m_files[it->second]->getMiscItems()) {
        if (misc.formID == formID) return &misc;
    }
    return nullptr;
}

const FactionData* ESMManager::findFaction(uint32_t formID) const {
    auto it = m_factionIndex.find(formID);
    if (it == m_factionIndex.end()) return nullptr;
    for (const auto& faction : m_files[it->second]->getFactions()) {
        if (faction.formID == formID) return &faction;
    }
    return nullptr;
}

const script::ScriptData* ESMManager::findScript(uint32_t formID) const {
    auto it = m_scriptIndex.find(formID);
    if (it == m_scriptIndex.end()) return nullptr;
    for (const auto& script : m_files[it->second]->getScripts()) {
        if (script.formID == formID) return &script;
    }
    return nullptr;
}

// ============================================================================
// Phase 64 record lookups
// ============================================================================

const GameSettingData* ESMManager::findGameSetting(const std::string& editorID) const {
    auto it = m_gameSettingIndex.find(editorID);
    if (it == m_gameSettingIndex.end()) return nullptr;
    for (const auto& gmst : m_files[it->second]->getGameSettings()) {
        if (gmst.editorID == editorID) return &gmst;
    }
    return nullptr;
}

const GlobalVariableData* ESMManager::findGlobalVariable(const std::string& editorID) const {
    auto it = m_globalVariableIndex.find(editorID);
    if (it == m_globalVariableIndex.end()) return nullptr;
    for (const auto& glob : m_files[it->second]->getGlobalVariables()) {
        if (glob.editorID == editorID) return &glob;
    }
    return nullptr;
}

const DoorData* ESMManager::findDoor(uint32_t formID) const {
    auto it = m_doorIndex.find(formID);
    if (it == m_doorIndex.end()) return nullptr;
    for (const auto& door : m_files[it->second]->getDoors()) {
        if (door.formID == formID) return &door;
    }
    return nullptr;
}

const AIPackageData* ESMManager::findPackage(uint32_t formID) const {
    auto it = m_packageIndex.find(formID);
    if (it == m_packageIndex.end()) return nullptr;
    for (const auto& pack : m_files[it->second]->getPackages()) {
        if (pack.formID == formID) return &pack;
    }
    return nullptr;
}

const PathGridData* ESMManager::findPathGridForCell(uint32_t cellFormID) const {
    auto it = m_pathGridIndex.find(cellFormID);
    if (it == m_pathGridIndex.end()) return nullptr;
    for (const auto& grid : m_files[it->second]->getPathGrids()) {
        if (grid.cellFormID == cellFormID) return &grid;
    }
    return nullptr;
}

const IdleAnimationData* ESMManager::findIdleAnimation(uint32_t formID) const {
    auto it = m_idleAnimationIndex.find(formID);
    if (it == m_idleAnimationIndex.end()) return nullptr;
    for (const auto& idle : m_files[it->second]->getIdleAnimations()) {
        if (idle.formID == formID) return &idle;
    }
    return nullptr;
}

const KeyData* ESMManager::findKey(uint32_t formID) const {
    auto it = m_keyIndex.find(formID);
    if (it == m_keyIndex.end()) return nullptr;
    for (const auto& key : m_files[it->second]->getKeys()) {
        if (key.formID == formID) return &key;
    }
    return nullptr;
}

const AmmoData* ESMManager::findAmmo(uint32_t formID) const {
    auto it = m_ammoIndex.find(formID);
    if (it == m_ammoIndex.end()) return nullptr;
    for (const auto& ammo : m_files[it->second]->getAmmo()) {
        if (ammo.formID == formID) return &ammo;
    }
    return nullptr;
}

const SigilStoneData* ESMManager::findSigilStone(uint32_t formID) const {
    auto it = m_sigilStoneIndex.find(formID);
    if (it == m_sigilStoneIndex.end()) return nullptr;
    for (const auto& stone : m_files[it->second]->getSigilStones()) {
        if (stone.formID == formID) return &stone;
    }
    return nullptr;
}

const SoulGemData* ESMManager::findSoulGem(uint32_t formID) const {
    auto it = m_soulGemIndex.find(formID);
    if (it == m_soulGemIndex.end()) return nullptr;
    for (const auto& gem : m_files[it->second]->getSoulGems()) {
        if (gem.formID == formID) return &gem;
    }
    return nullptr;
}

const FurnitureData* ESMManager::findFurniture(uint32_t formID) const {
    auto it = m_furnitureIndex.find(formID);
    if (it == m_furnitureIndex.end()) return nullptr;
    for (const auto& furn : m_files[it->second]->getFurniture()) {
        if (furn.formID == formID) return &furn;
    }
    return nullptr;
}

const LandscapeTextureData* ESMManager::findLandscapeTexture(uint32_t formID) const {
    auto it = m_landscapeTextureIndex.find(formID);
    if (it == m_landscapeTextureIndex.end()) return nullptr;
    for (const auto& ltex : m_files[it->second]->getLandscapeTextures()) {
        if (ltex.formID == formID) return &ltex;
    }
    return nullptr;
}

const GrassData* ESMManager::findGrass(uint32_t formID) const {
    auto it = m_grassIndex.find(formID);
    if (it == m_grassIndex.end()) return nullptr;
    for (const auto& grass : m_files[it->second]->getGrass()) {
        if (grass.formID == formID) return &grass;
    }
    return nullptr;
}

const WaterData* ESMManager::findWater(uint32_t formID) const {
    auto it = m_waterIndex.find(formID);
    if (it == m_waterIndex.end()) return nullptr;
    for (const auto& water : m_files[it->second]->getWaters()) {
        if (water.formID == formID) return &water;
    }
    return nullptr;
}

const WeatherData* ESMManager::findWeather(uint32_t formID) const {
    auto it = m_weatherIndex.find(formID);
    if (it == m_weatherIndex.end()) return nullptr;
    for (const auto& weather : m_files[it->second]->getWeathers()) {
        if (weather.formID == formID) return &weather;
    }
    return nullptr;
}

const CombatStyleData* ESMManager::findCombatStyle(uint32_t formID) const {
    auto it = m_combatStyleIndex.find(formID);
    if (it == m_combatStyleIndex.end()) return nullptr;
    for (const auto& style : m_files[it->second]->getCombatStyles()) {
        if (style.formID == formID) return &style;
    }
    return nullptr;
}

const LoadScreenData* ESMManager::findLoadScreen(uint32_t formID) const {
    auto it = m_loadScreenIndex.find(formID);
    if (it == m_loadScreenIndex.end()) return nullptr;
    for (const auto& screen : m_files[it->second]->getLoadScreens()) {
        if (screen.formID == formID) return &screen;
    }
    return nullptr;
}

const EffectShaderData* ESMManager::findEffectShader(uint32_t formID) const {
    auto it = m_effectShaderIndex.find(formID);
    if (it == m_effectShaderIndex.end()) return nullptr;
    for (const auto& shader : m_files[it->second]->getEffectShaders()) {
        if (shader.formID == formID) return &shader;
    }
    return nullptr;
}

const AnimationObjectData* ESMManager::findAnimationObject(uint32_t formID) const {
    auto it = m_animationObjectIndex.find(formID);
    if (it == m_animationObjectIndex.end()) return nullptr;
    for (const auto& anio : m_files[it->second]->getAnimationObjects()) {
        if (anio.formID == formID) return &anio;
    }
    return nullptr;
}

const SubspaceData* ESMManager::findSubspace(uint32_t formID) const {
    auto it = m_subspaceIndex.find(formID);
    if (it == m_subspaceIndex.end()) return nullptr;
    for (const auto& sbsp : m_files[it->second]->getSubspaces()) {
        if (sbsp.formID == formID) return &sbsp;
    }
    return nullptr;
}

ESMManager::DecodeStatistics ESMManager::getDecodeStatistics() const {
    DecodeStatistics stats;
    for (const auto& file : m_files) {
        stats.gameSettings += file->getGameSettings().size();
        stats.globalVariables += file->getGlobalVariables().size();
        stats.doors += file->getDoors().size();
        stats.packages += file->getPackages().size();
        stats.pathGrids += file->getPathGrids().size();
        stats.idleAnimations += file->getIdleAnimations().size();
        stats.keys += file->getKeys().size();
        stats.ammo += file->getAmmo().size();
        stats.sigilStones += file->getSigilStones().size();
        stats.soulGems += file->getSoulGems().size();
        stats.furniture += file->getFurniture().size();
        stats.landscapeTextures += file->getLandscapeTextures().size();
        stats.grass += file->getGrass().size();
        stats.waters += file->getWaters().size();
        stats.weathers += file->getWeathers().size();
        stats.combatStyles += file->getCombatStyles().size();
        stats.loadScreens += file->getLoadScreens().size();
        stats.effectShaders += file->getEffectShaders().size();
        stats.animationObjects += file->getAnimationObjects().size();
        stats.subspaces += file->getSubspaces().size();
    }
    return stats;
}

std::vector<std::pair<uint32_t, uint16_t>> ESMManager::resolveLeveledList(uint32_t listFormID, uint32_t playerLevel) const {
    std::vector<std::pair<uint32_t, uint16_t>> result;
    const LeveledListData* list = findLeveledList(listFormID);
    if (!list) return result;

    // ChanceNone: random chance that nothing is spawned
    if (list->chanceNone > 0) {
        uint8_t roll = static_cast<uint8_t>(rand() % 100);
        if (roll < list->chanceNone) {
            return result;  // Nothing selected
        }
    }

    // Flag bit 0: use all entries with level <= playerLevel
    // Flag bit 1: calculate for each item in count
    bool calcAllLevels = (list->flags & 0x01) != 0;

    if (calcAllLevels) {
        // Select all entries whose level requirement is met
        for (const auto& entry : list->entries) {
            if (entry.level <= playerLevel) {
                result.push_back({entry.referencedFormID, entry.count});
            }
        }
    } else {
        // Select the highest-level entry that the player qualifies for
        const LeveledListEntry* best = nullptr;
        for (const auto& entry : list->entries) {
            if (entry.level <= playerLevel) {
                if (!best || entry.level > best->level) {
                    best = &entry;
                }
            }
        }
        if (best) {
            result.push_back({best->referencedFormID, best->count});
        }
    }

    return result;
}

const std::vector<NPCData>& ESMManager::getAllNPCs() const {
    // Return data from last-loaded file (highest priority)
    // For simplicity, use the first file
    if (m_files.empty()) {
        static std::vector<NPCData> empty;
        return empty;
    }
    return m_files.back()->getNPCs();
}

const std::vector<CreatureData>& ESMManager::getAllCreatures() const {
    if (m_files.empty()) {
        static std::vector<CreatureData> empty;
        return empty;
    }
    return m_files.back()->getCreatures();
}

const std::vector<CellData>& ESMManager::getAllCells() const {
    if (m_files.empty()) {
        static std::vector<CellData> empty;
        return empty;
    }
    return m_files.back()->getCells();
}

const std::vector<WeaponData>& ESMManager::getAllWeapons() const {
    if (m_files.empty()) {
        static std::vector<WeaponData> empty;
        return empty;
    }
    return m_files.back()->getWeapons();
}

const std::vector<QuestData>& ESMManager::getAllQuests() const {
    if (m_files.empty()) {
        static std::vector<QuestData> empty;
        return empty;
    }
    return m_files.back()->getQuests();
}

const std::vector<DialogData>& ESMManager::getAllDialogs() const {
    if (m_files.empty()) {
        static std::vector<DialogData> empty;
        return empty;
    }
    return m_files.back()->getDialogs();
}

const std::vector<ReferenceData>& ESMManager::getAllReferences() const {
    if (m_files.empty()) {
        static std::vector<ReferenceData> empty;
        return empty;
    }
    return m_files.back()->getReferences();
}

size_t ESMManager::getNpcReferenceCount() const {
    if (m_files.empty()) return 0;
    return m_files.back()->getNpcReferenceCount();
}

size_t ESMManager::getCreatureReferenceCount() const {
    if (m_files.empty()) return 0;
    return m_files.back()->getCreatureReferenceCount();
}

const std::vector<TerrainData>& ESMManager::getAllTerrains() const {
    if (m_files.empty()) {
        static std::vector<TerrainData> empty;
        return empty;
    }
    return m_files.back()->getTerrains();
}

const std::vector<WorldData>& ESMManager::getAllWorlds() const {
    if (m_files.empty()) {
        static std::vector<WorldData> empty;
        return empty;
    }
    return m_files.back()->getWorlds();
}

const std::vector<SpellData>& ESMManager::getAllSpells() const {
    if (m_files.empty()) {
        static std::vector<SpellData> empty;
        return empty;
    }
    return m_files.back()->getSpells();
}

const std::vector<EnchantmentData>& ESMManager::getAllEnchantments() const {
    if (m_files.empty()) {
        static std::vector<EnchantmentData> empty;
        return empty;
    }
    return m_files.back()->getEnchantments();
}

const std::vector<MagicEffectData>& ESMManager::getAllMagicEffects() const {
    if (m_files.empty()) {
        static std::vector<MagicEffectData> empty;
        return empty;
    }
    return m_files.back()->getMagicEffects();
}

const std::vector<SkillData>& ESMManager::getAllSkills() const {
    if (m_files.empty()) {
        static std::vector<SkillData> empty;
        return empty;
    }
    return m_files.back()->getSkills();
}

const std::vector<BirthsignData>& ESMManager::getAllBirthsigns() const {
    if (m_files.empty()) {
        static std::vector<BirthsignData> empty;
        return empty;
    }
    return m_files.back()->getBirthsigns();
}

const std::vector<ContainerData>& ESMManager::getAllContainers() const {
    if (m_files.empty()) {
        static std::vector<ContainerData> empty;
        return empty;
    }
    return m_files.back()->getContainers();
}

const std::vector<LightData>& ESMManager::getAllLights() const {
    if (m_files.empty()) {
        static std::vector<LightData> empty;
        return empty;
    }
    return m_files.back()->getLights();
}

const std::vector<StaticData>& ESMManager::getAllStatics() const {
    if (m_files.empty()) {
        static std::vector<StaticData> empty;
        return empty;
    }
    return m_files.back()->getStatics();
}

const std::vector<SoundData>& ESMManager::getAllSounds() const {
    if (m_files.empty()) {
        static std::vector<SoundData> empty;
        return empty;
    }
    return m_files.back()->getSounds();
}

const std::vector<TreeData>& ESMManager::getAllTrees() const {
    if (m_files.empty()) {
        static std::vector<TreeData> empty;
        return empty;
    }
    return m_files.back()->getTrees();
}

const std::vector<FloraData>& ESMManager::getAllFloras() const {
    if (m_files.empty()) {
        static std::vector<FloraData> empty;
        return empty;
    }
    return m_files.back()->getFloras();
}

const std::vector<ActivatorData>& ESMManager::getAllActivators() const {
    if (m_files.empty()) {
        static std::vector<ActivatorData> empty;
        return empty;
    }
    return m_files.back()->getActivators();
}

const std::vector<ApparatusData>& ESMManager::getAllApparatuses() const {
    if (m_files.empty()) {
        static std::vector<ApparatusData> empty;
        return empty;
    }
    return m_files.back()->getApparatuses();
}

const std::vector<EyesData>& ESMManager::getAllEyes() const {
    if (m_files.empty()) {
        static std::vector<EyesData> empty;
        return empty;
    }
    return m_files.back()->getEyes();
}

const std::vector<HairData>& ESMManager::getAllHairs() const {
    if (m_files.empty()) {
        static std::vector<HairData> empty;
        return empty;
    }
    return m_files.back()->getHairs();
}

const std::vector<ClimateData>& ESMManager::getAllClimates() const {
    if (m_files.empty()) {
        static std::vector<ClimateData> empty;
        return empty;
    }
    return m_files.back()->getClimates();
}

const std::vector<RegionData>& ESMManager::getAllRegions() const {
    if (m_files.empty()) {
        static std::vector<RegionData> empty;
        return empty;
    }
    return m_files.back()->getRegions();
}

const std::vector<LeveledListData>& ESMManager::getAllLeveledLists() const {
    if (m_files.empty()) {
        static std::vector<LeveledListData> empty;
        return empty;
    }
    return m_files.back()->getLeveledLists();
}

const std::vector<NavMeshData>& ESMManager::getAllNavMeshes() const {
    if (m_files.empty()) {
        static std::vector<NavMeshData> empty;
        return empty;
    }
    return m_files.back()->getNavMeshes();
}

const std::vector<ArmorData>& ESMManager::getAllArmors() const {
    if (m_files.empty()) {
        static std::vector<ArmorData> empty;
        return empty;
    }
    return m_files.back()->getArmors();
}

const std::vector<BookData>& ESMManager::getAllBooks() const {
    if (m_files.empty()) {
        static std::vector<BookData> empty;
        return empty;
    }
    return m_files.back()->getBooks();
}

const std::vector<FactionData>& ESMManager::getAllFactions() const {
    if (m_files.empty()) {
        static std::vector<FactionData> empty;
        return empty;
    }
    return m_files.back()->getFactions();
}

const std::vector<RaceData>& ESMManager::getAllRaces() const {
    if (m_files.empty()) {
        static std::vector<RaceData> empty;
        return empty;
    }
    return m_files.back()->getRaces();
}

const std::vector<ClassData>& ESMManager::getAllClasses() const {
    if (m_files.empty()) {
        static std::vector<ClassData> empty;
        return empty;
    }
    return m_files.back()->getClasses();
}

const std::vector<ClothingData>& ESMManager::getAllClothing() const {
    if (m_files.empty()) {
        static std::vector<ClothingData> empty;
        return empty;
    }
    return m_files.back()->getClothing();
}

const std::vector<IngredientData>& ESMManager::getAllIngredients() const {
    if (m_files.empty()) {
        static std::vector<IngredientData> empty;
        return empty;
    }
    return m_files.back()->getIngredients();
}

const std::vector<AlchemyData>& ESMManager::getAllAlchemy() const {
    if (m_files.empty()) {
        static std::vector<AlchemyData> empty;
        return empty;
    }
    return m_files.back()->getAlchemy();
}

const std::vector<MiscItemData>& ESMManager::getAllMiscItems() const {
    if (m_files.empty()) {
        static std::vector<MiscItemData> empty;
        return empty;
    }
    return m_files.back()->getMiscItems();
}

const std::vector<RoadData>& ESMManager::getAllRoads() const {
    if (m_files.empty()) {
        static std::vector<RoadData> empty;
        return empty;
    }
    return m_files.back()->getRoads();
}

const std::vector<script::ScriptData>& ESMManager::getAllScripts() const {
    if (m_files.empty()) {
        static std::vector<script::ScriptData> empty;
        return empty;
    }
    return m_files.back()->getScripts();
}

} // namespace oblivion
