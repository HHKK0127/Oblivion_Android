#include "esm_reader.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <zlib.h>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGE
#define LOG_TAG "ESMReader"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {

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
    // Null-terminated string inside data
    return std::string(reinterpret_cast<const char*>(sub->data.data()));
}

uint32_t ESMRecord::getUint(const char* tag) const {
    auto* sub = findSubRecord(tag);
    if (!sub || sub->data.size() < 4) return 0;
    return *reinterpret_cast<const uint32_t*>(sub->data.data());
}

int32_t ESMRecord::getInt(const char* tag) const {
    auto* sub = findSubRecord(tag);
    if (!sub || sub->data.size() < 4) return 0;
    return *reinterpret_cast<const int32_t*>(sub->data.data());
}

float ESMRecord::getFloat(const char* tag) const {
    auto* sub = findSubRecord(tag);
    if (!sub || sub->data.size() < 4) return 0.0f;
    return *reinterpret_cast<const float*>(sub->data.data());
}

uint32_t ESMRecord::getFormID(const char* tag) const {
    auto* sub = findSubRecord(tag);
    if (!sub || sub->data.size() < 4) return 0;
    // FormID in subrecord is 4 bytes (uint32)
    return *reinterpret_cast<const uint32_t*>(sub->data.data());
}

// ============================================================================
// ESMFile implementation
// ============================================================================

bool ESMFile::open(const std::string& filePath) {
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

    // Read top-level structure
    // First record must be TES4 header
    ESMRecord header;
    if (!readRecordHeader(file, header)) {
        LOGE("Failed to read TES4 header in %s", filePath.c_str());
        return false;
    }

    if (std::memcmp(header.recType, "TES4", 4) != 0) {
        LOGE("Invalid ESM file: expected TES4 header, got %.4s", header.recType);
        return false;
    }

    LOGD("TES4 header parsed successfully, formID=0x%08X, flags=0x%08X",
         header.formID, header.flags);

    // Read records at the top level
    while (file && file.peek() != EOF) {
        // Check if next is GRUP or a record
        char peekTag[4];
        file.read(peekTag, 4);
        if (file.gcount() < 4) break;

        file.seekg(-4, std::ios::cur);  // Put the 4 bytes back

        if (peekTag[0] == 'G' && peekTag[1] == 'R' && peekTag[2] == 'U' && peekTag[3] == 'P') {
            // Read GRUP
            GroupHeader gh;
            file.read(reinterpret_cast<char*>(&gh), sizeof(gh));

            if (std::memcmp(gh.recType, "GRUP", 4) != 0) {
                LOGE("Expected GRUP, got %.4s", gh.recType);
                break;
            }

            LOGD("GRUP: type=%u, label=0x%08X, size=%u",
                 gh.groupType, gh.groupLabel, gh.groupSize);

            // Parse contents of this group
            readGroup(file, static_cast<GroupType>(gh.groupType), gh.groupSize - sizeof(gh));

            // Align to next group
            // The group is already consumed if readGroup consumed exactly groupSize
        } else {
            // Standalone record (not inside a GRUP)
            ESMRecord rec;
            if (!readRecordHeader(file, rec)) break;

            decodeRecord(rec);
        }
    }

    file.close();
    LOGD("ESM file parsed: %zu cells, %zu NPCs, %zu weapons, %zu quests, %zu dialogs, "
         "%zu refs, %zu terrains",
         m_cells.size(), m_npcs.size(), m_weapons.size(), m_quests.size(), m_dialogs.size(),
         m_references.size(), m_terrains.size());

    return true;
}

bool ESMFile::readRecordHeader(std::ifstream& file, ESMRecord& rec) {
    file.read(reinterpret_cast<char*>(&rec.recType), 4);
    if (file.gcount() < 4) return false;

    uint32_t rawSize;
    file.read(reinterpret_cast<char*>(&rawSize), 4);  // dataSize
    file.read(reinterpret_cast<char*>(&rec.flags), 4);
    file.read(reinterpret_cast<char*>(&rec.formID), 4);

    if (std::memcmp(rec.recType, "GRUP", 4) == 0) {
        // GRUP has a different structure — this shouldn't be called for GRUPs
        LOGE("readRecordHeader called on GRUP (%.4s), unexpected", rec.recType);
        return false;
    }

    bool compressed = (rec.flags & REC_FLAG_COMPRESSED) != 0;
    rec.dataSize = rawSize & 0x00FFFFFF;  // Upper byte is unused

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
        inflateInit(&strm);
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
    } else if (std::memcmp(rec.recType, "WEAP", 4) == 0) {
        decodeWeapon(rec);
    } else if (std::memcmp(rec.recType, "QUST", 4) == 0) {
        decodeQuest(rec);
    } else if (std::memcmp(rec.recType, "DIAL", 4) == 0) {
        decodeDialog(rec);
    } else if (std::memcmp(rec.recType, "REFR", 4) == 0) {
        decodeReference(rec);
    } else if (std::memcmp(rec.recType, "LAND", 4) == 0) {
        decodeTerrain(rec);
    } else if (std::memcmp(rec.recType, "WRLD", 4) == 0) {
        decodeWorld(rec);
        } else if (std::memcmp(rec.recType, "SPEL", 4) == 0) {
            decodeSpell(rec);
        }
    // Other types (ARMO, BOOK, CLOT, etc.) are not decoded yet
}

void ESMFile::decodeCell(const ESMRecord& rec) {
    CellData cell;
    cell.formID = rec.formID;
    cell.editorID = rec.getString("EDID");

    // Determine if interior or exterior
    auto* dataSub = rec.findSubRecord("DATA");
    if (dataSub && dataSub->size() >= 4) {
        uint8_t cellFlags = dataSub->data[0];
        bool isInterior = (cellFlags & 0x01) != 0;
        if (!isInterior && dataSub->size() >= 12) {
            // Exterior cell: 8 bytes after flags = gridX, gridY
            cell.gridX = *reinterpret_cast<const int32_t*>(dataSub->data.data() + 4);
            cell.gridY = *reinterpret_cast<const int32_t*>(dataSub->data.data() + 8);
        }
    }

    cell.fullName = rec.getString("FULL");
    cell.worldspaceID = rec.getFormID("WRLD");

    // XCLC subrecord for grid coordinates (TES4 specific)
    auto* xclc = rec.findSubRecord("XCLC");
    if (xclc && xclc->size() >= 8) {
        cell.gridX = *reinterpret_cast<const int32_t*>(xclc->data.data());
        cell.gridY = *reinterpret_cast<const int32_t*>(xclc->data.data() + 4);
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

    m_npcs.push_back(std::move(npc));
}

void ESMFile::decodeWeapon(const ESMRecord& rec) {
    WeaponData wpn;
    wpn.formID = rec.formID;
    wpn.editorID = rec.getString("EDID");
    wpn.fullName = rec.getString("FULL");

    auto* data = rec.findSubRecord("DATA");
    if (data && data->size() >= 16) {
        // Skip 2 bytes (type), read damage at byte 2
        wpn.damage = *reinterpret_cast<const uint16_t*>(data->data.data() + 2);
        // Read value at byte 12
        wpn.value = *reinterpret_cast<const uint32_t*>(data->data.data() + 12);
    } else {
        auto* data2 = rec.findSubRecord("DNAM");
        if (data2 && data2->size() >= 8) {
            wpn.damage = *reinterpret_cast<const uint16_t*>(data2->data.data());
            wpn.value = *reinterpret_cast<const uint32_t*>(data2->data.data() + 4);
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
    if (data && data->size() >= 4) {
        dia.flags = data->data[0];
    }

    m_dialogs.push_back(std::move(dia));
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
        ref.position.x = *reinterpret_cast<const float*>(data->data.data());
        ref.position.y = *reinterpret_cast<const float*>(data->data.data() + 4);
        ref.position.z = *reinterpret_cast<const float*>(data->data.data() + 8);
        ref.rotation.x = *reinterpret_cast<const float*>(data->data.data() + 12);
        ref.rotation.y = *reinterpret_cast<const float*>(data->data.data() + 16);
        ref.rotation.z = *reinterpret_cast<const float*>(data->data.data() + 20);
    }

    // Scale
    auto* xsca = rec.findSubRecord("XSCL");
    if (xsca && xsca->size() >= 4) {
        ref.scale = *reinterpret_cast<const float*>(xsca->data.data());
    }

    // Cell formID from XRGD (or infer from surrounding context)
    auto* xrgd = rec.findSubRecord("XRGD");
    // Also XRGB, XRDS etc. — not critical for basic placement

    m_references.push_back(std::move(ref));
}

// ============================================================================
// Terrain (LAND) decoding — 65x65 heightmap per cell
// ============================================================================

void ESMFile::decodeTerrain(const ESMRecord& rec) {
    TerrainData terrain;
    terrain.formID = rec.formID;

    // VHGT subrecord: 1 byte quad size + 65x65 height deltas
    auto* vhgt = rec.findSubRecord("VHGT");
    if (vhgt && vhgt->size() >= 3) {
        // First byte: quad size (unused here, typically ignored)
        // Followed by 65x65 = 4225 delta values, 2 bytes each (int16)
        size_t expected = 1 + 65 * 65 * 2;  // 1 bytes header + 4225 * 2 bytes
        if (vhgt->size() >= expected) {
            terrain.heights.resize(65 * 65);
            
            // Reconstruct absolute heights from deltas
            // The height field is an unsigned 16-bit integer with fixed-point
            // Units: 1 unit = 1/32 of a game unit, but we convert to game units
            const uint8_t* dataPtr = vhgt->data.data() + 1;  // Skip quad size
            float currentHeight = 0.0f;
            for (int i = 0; i < 65 * 65; i++) {
                int16_t delta;
                std::memcpy(&delta, dataPtr + i * 2, 2);
                currentHeight += static_cast<float>(delta) / 16.0f;  // Convert to game units
                terrain.heights[i] = currentHeight;
            }
        }
    }

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
        world.worldOffset.x = *reinterpret_cast<const float*>(data->data.data());
        world.worldOffset.y = *reinterpret_cast<const float*>(data->data.data() + 4);
        world.minX = *reinterpret_cast<const int32_t*>(data->data.data() + 8);
        world.minY = *reinterpret_cast<const int32_t*>(data->data.data() + 12);
    }

    // NAM0 subrecord: max bounds (maxX, maxY)
    auto* nam0 = rec.findSubRecord("NAM0");
    if (nam0 && nam0->size() >= 8) {
        world.maxX = *reinterpret_cast<const int32_t*>(nam0->data.data());
        world.maxY = *reinterpret_cast<const int32_t*>(nam0->data.data() + 4);
    }

    LOGD("  WRLD: 0x%08X '%s' '%s' offset=(%.0f,%.0f) bounds=[(%d,%d)-(%d,%d)]",
         world.formID, world.editorID.c_str(), world.fullName.c_str(),
         world.worldOffset.x, world.worldOffset.y,
         world.minX, world.minY, world.maxX, world.maxY);

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

    // SPIT subrecord: magic data (24 bytes)
    // struct { uint32_t type; uint32_t cost; uint8_t level; uint32_t flags; ... }
    auto* spit = rec.findSubRecord("SPIT");
    if (spit && spit->size() >= 8) {
        spell.spellType = static_cast<uint8_t>(spit->data[0]);       // 0=spell, 1=disease, ...
        spell.cost = *reinterpret_cast<const uint32_t*>(spit->data.data() + 4);
        if (spit->size() >= 9) {
            spell.level = spit->data[8];  // 0=novice .. 4=master
        }
        if (spit->size() >= 16) {
            spell.flags = *reinterpret_cast<const uint32_t*>(spit->data.data() + 12);
        }
    }

    // Parse packed effects: pairs of EFID + EFIT + (optional) SCIT
    // EFID = Magic Effect FormID (4 bytes)
    // EFIT = Effect data (24 bytes for Morrowind, 16 for Oblivion)
    //   struct { uint32_t magnitude; uint32_t area; uint32_t duration; }
    size_t idx = 0;
    while (true) {
        // Find next EFID subrecord
        auto* efid = rec.findSubRecord("EFID", idx);
        if (!efid) break;
        ++idx;

        uint32_t mgefFormID = *reinterpret_cast<const uint32_t*>(efid->data.data());
        spell.effectFormIDs.push_back(mgefFormID);

        // EFIT follows EFID
        auto* efit = rec.findSubRecord("EFIT", idx);
        if (efit && efit->size() >= 12) {
            float magnitude;
            uint32_t area, duration;
            std::memcpy(&magnitude, efit->data.data(), 4);
            std::memcpy(&area, efit->data.data() + 4, 4);
            std::memcpy(&duration, efit->data.data() + 8, 4);
            spell.effectMagnitudes.push_back(magnitude);
            spell.effectAreas.push_back(area);
            spell.effectDurations.push_back(duration);
        }
        ++idx;
    }

    LOGD("  SPEL: 0x%08X '%s' cost=%u type=%d level=%d effects=%zu",
         spell.formID, spell.fullName.c_str(),
         spell.cost, spell.spellType, spell.level,
         spell.effectFormIDs.size());

    m_spells.push_back(std::move(spell));
}

// ============================================================================
// Memory-based parsing (for ESM inside BSA archives)
// ============================================================================

bool ESMFile::readRecordHeaderMem(const uint8_t*& pos, const uint8_t* end, ESMRecord& rec) {
    if (pos + 16 > end) return false;
    
    std::memcpy(rec.recType, pos, 4); pos += 4;
    
    uint32_t rawSize;
    std::memcpy(&rawSize, pos, 4); pos += 4;
    std::memcpy(&rec.flags, pos, 4); pos += 4;
    std::memcpy(&rec.formID, pos, 4); pos += 4;
    
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
            std::memcpy(&gh, pos, sizeof(gh)); pos += sizeof(gh);
            
            uint32_t childSize = gh.groupSize - sizeof(gh);
            readGroupMem(pos, groupEnd, static_cast<GroupType>(gh.groupType), childSize);
        } else {
            // Regular record
            ESMRecord rec;
            if (!readRecordHeaderMem(pos, groupEnd, rec)) break;
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
            std::memcpy(&gh, pos, sizeof(gh)); pos += sizeof(gh);
            
            uint32_t childSize = gh.groupSize - sizeof(gh);
            readGroupMem(pos, end, static_cast<GroupType>(gh.groupType), childSize);
        } else {
            ESMRecord rec;
            if (!readRecordHeaderMem(pos, end, rec)) break;
            decodeRecord(rec);
        }
    }
    
    LOGD("ESM parsed from memory: %zu cells, %zu NPCs, %zu weapons, %zu quests, %zu dialogs",
         m_cells.size(), m_npcs.size(), m_weapons.size(), m_quests.size(), m_dialogs.size());
    
    return true;
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
    m_questIndex.clear();
    m_dialogIndex.clear();
}

void ESMManager::rebuildIndices() {
    m_npcIndex.clear();
    m_cellIndex.clear();
    m_weaponIndex.clear();
    m_questIndex.clear();
    m_dialogIndex.clear();

    for (size_t fi = 0; fi < m_files.size(); ++fi) {
        const auto& file = m_files[fi];
        for (size_t i = 0; i < file->getNPCs().size(); ++i) {
            m_npcIndex[file->getNPCs()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getCells().size(); ++i) {
            m_cellIndex[file->getCells()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getWeapons().size(); ++i) {
            m_weaponIndex[file->getWeapons()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getQuests().size(); ++i) {
            m_questIndex[file->getQuests()[i].formID] = fi;
        }
        for (size_t i = 0; i < file->getDialogs().size(); ++i) {
            m_dialogIndex[file->getDialogs()[i].formID] = fi;
        }
    }
}

        size_t ESMManager::findRecordsByType(const std::string& type) const {
            if (type == "CELL") return getAllCells().size();
            if (type == "NPC_") return getAllNPCs().size();
            if (type == "WEAP") return getAllWeapons().size();
            if (type == "QUST") return getAllQuests().size();
            if (type == "DIAL") return getAllDialogs().size();
            if (type == "REFR") return getAllReferences().size();
            if (type == "LAND") return getAllTerrains().size();
            if (type == "WRLD") return getAllWorlds().size();
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

const std::vector<NPCData>& ESMManager::getAllNPCs() const {
    // Return data from last-loaded file (highest priority)
    // For simplicity, use the first file
    if (m_files.empty()) {
        static std::vector<NPCData> empty;
        return empty;
    }
    return m_files.back()->getNPCs();
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

} // namespace oblivion
