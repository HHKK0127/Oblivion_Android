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
                pkg.targetFormID = *reinterpret_cast<const uint32_t*>(sub.data.data());
                pkg.idleTime = sub.data[4];
            }
        } else if (std::memcmp(sub.tag, "AI_E", 4) == 0) {
            // AI Escort
            pkg.type = AIPackageType::ESCORT;
            if (sub.size() >= 16) {
                pkg.targetFormID = *reinterpret_cast<const uint32_t*>(sub.data.data());
                pkg.targetX = *reinterpret_cast<const float*>(sub.data.data() + 4);
                pkg.targetY = *reinterpret_cast<const float*>(sub.data.data() + 8);
                pkg.targetZ = *reinterpret_cast<const float*>(sub.data.data() + 12);
            }
        } else if (std::memcmp(sub.tag, "AI_F", 4) == 0) {
            // AI Follow
            pkg.type = AIPackageType::FOLLOW;
            if (sub.size() >= 16) {
                pkg.targetFormID = *reinterpret_cast<const uint32_t*>(sub.data.data());
                pkg.targetX = *reinterpret_cast<const float*>(sub.data.data() + 4);
                pkg.targetY = *reinterpret_cast<const float*>(sub.data.data() + 8);
                pkg.targetZ = *reinterpret_cast<const float*>(sub.data.data() + 12);
            }
        } else if (std::memcmp(sub.tag, "AI_T", 4) == 0) {
            // AI Travel
            pkg.type = AIPackageType::TRAVEL;
            if (sub.size() >= 12) {
                pkg.targetX = *reinterpret_cast<const float*>(sub.data.data());
                pkg.targetY = *reinterpret_cast<const float*>(sub.data.data() + 4);
                pkg.targetZ = *reinterpret_cast<const float*>(sub.data.data() + 8);
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
                pkg.targetFormID = *reinterpret_cast<const uint32_t*>(sub.data.data() + 8);
                pkg.locationFormID = *reinterpret_cast<const uint32_t*>(sub.data.data() + 12);
                pkg.targetX = *reinterpret_cast<const float*>(sub.data.data() + 16);
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
        creature.combat = *reinterpret_cast<const uint16_t*>(data->data.data());
        creature.magic = *reinterpret_cast<const uint16_t*>(data->data.data() + 2);
        creature.stealth = *reinterpret_cast<const uint16_t*>(data->data.data() + 4);
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

    // Parse effects: iterate subrecords looking for EFID + EFIT pairs
    for (size_t i = 0; i < rec.subRecords.size(); i++) {
        const auto& sub = rec.subRecords[i];
        if (std::memcmp(sub.tag, "EFID", 4) == 0 && sub.size() >= 4) {
            uint32_t mgefFormID;
            std::memcpy(&mgefFormID, sub.data.data(), 4);
            spell.effectFormIDs.push_back(mgefFormID);

            // EFIT should be the next sub-record
            if (i + 1 < rec.subRecords.size() &&
                std::memcmp(rec.subRecords[i + 1].tag, "EFIT", 4) == 0 &&
                rec.subRecords[i + 1].size() >= 12) {
                const auto& efit = rec.subRecords[i + 1];
                float magnitude;
                uint32_t area, duration;
                std::memcpy(&magnitude, efit.data.data(), 4);
                std::memcpy(&area, efit.data.data() + 4, 4);
                std::memcpy(&duration, efit.data.data() + 8, 4);
                spell.effectMagnitudes.push_back(magnitude);
                spell.effectAreas.push_back(area);
                spell.effectDurations.push_back(duration);
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
        enchant.enchantType = *reinterpret_cast<const uint32_t*>(enit->data.data());
        enchant.chargeAmount = *reinterpret_cast<const uint32_t*>(enit->data.data() + 4);
        enchant.enchantCost = *reinterpret_cast<const uint32_t*>(enit->data.data() + 8);
        enchant.flags = *reinterpret_cast<const uint32_t*>(enit->data.data() + 12);
    }

    // Parse effects: iterate subrecords looking for EFID + EFIT pairs
    for (size_t i = 0; i < rec.subRecords.size(); i++) {
        const auto& sub = rec.subRecords[i];
        if (std::memcmp(sub.tag, "EFID", 4) == 0 && sub.size() >= 4) {
            uint32_t mgefFormID;
            std::memcpy(&mgefFormID, sub.data.data(), 4);
            enchant.effectFormIDs.push_back(mgefFormID);

            // EFIT should be the next sub-record
            if (i + 1 < rec.subRecords.size() &&
                std::memcmp(rec.subRecords[i + 1].tag, "EFIT", 4) == 0 &&
                rec.subRecords[i + 1].size() >= 12) {
                const auto& efit = rec.subRecords[i + 1];
                float magnitude;
                uint32_t area, duration;
                std::memcpy(&magnitude, efit.data.data(), 4);
                std::memcpy(&area, efit.data.data() + 4, 4);
                std::memcpy(&duration, efit.data.data() + 8, 4);
                enchant.effectMagnitudes.push_back(magnitude);
                enchant.effectAreas.push_back(area);
                enchant.effectDurations.push_back(duration);
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

    // MEDT subrecord: magic effect data
    auto* medt = rec.findSubRecord("MEDT");
    if (medt && medt->size() >= 20) {
        effect.school = *reinterpret_cast<const uint32_t*>(medt->data.data());
        effect.baseCost = *reinterpret_cast<const uint32_t*>(medt->data.data() + 4);
        effect.flags = *reinterpret_cast<const uint32_t*>(medt->data.data() + 8);
        std::memcpy(&effect.baseMagnitude, medt->data.data() + 12, 4);
        std::memcpy(&effect.baseDuration, medt->data.data() + 16, 4);
        if (medt->size() >= 24) {
            std::memcpy(&effect.range, medt->data.data() + 20, 4);
        }
        if (medt->size() >= 28) {
            effect.actorValue = *reinterpret_cast<const uint32_t*>(medt->data.data() + 24);
        }
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
        skill.skillID = *reinterpret_cast<const uint32_t*>(skdt->data.data());
        skill.specialization = *reinterpret_cast<const uint32_t*>(skdt->data.data() + 4);
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
        container.flags = *reinterpret_cast<const uint32_t*>(cnto->data.data() + 4);
    }

    // Parse container items: iterate subrecords looking for CNTO + COCT pairs
    for (size_t i = 0; i < rec.subRecords.size(); i++) {
        const auto& sub = rec.subRecords[i];
        if (std::memcmp(sub.tag, "CNTO", 4) == 0 && sub.size() >= 8) {
            ContainerData::ContainerItem item;
            std::memcpy(&item.itemFormID, sub.data.data(), 4);
            item.count = *reinterpret_cast<const uint32_t*>(sub.data.data() + 4);
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
            entry.level = *reinterpret_cast<const uint16_t*>(sub.data.data() + 4);
            if (sub.size() >= 10) {
                entry.count = *reinterpret_cast<const uint16_t*>(sub.data.data() + 8);
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
        armor.armorRating = *reinterpret_cast<const uint32_t*>(data->data.data());
        if (data->size() >= 12) {
            armor.value = *reinterpret_cast<const uint32_t*>(data->data.data() + 4);
            std::memcpy(&armor.weight, data->data.data() + 8, 4);
        }
    }

    // ENAM: enchantment FormID
    auto* enam = rec.findSubRecord("ENAM");
    if (enam && enam->size() >= 4) {
        armor.enchantmentID = *reinterpret_cast<const uint32_t*>(enam->data.data());
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
        faction.crimeGoldMultiplier = *reinterpret_cast<const int32_t*>(crim->data.data());
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
                    rank.rankData = *reinterpret_cast<const uint32_t*>(mnam.data.data());
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

        // DATA: value + weight
        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 8) {
            std::memcpy(&ingr.value, data->data.data(), 4);
            std::memcpy(&ingr.weight, data->data.data() + 4, 4);
        }

        // Effects: IRQD (formID), IRQF (magnitude float), IRQA (area), IRQT (duration)
        // All 4 subrecords repeat per effect slot
        for (size_t i = 0; i < rec.subRecords.size(); i++) {
            const auto& sub = rec.subRecords[i];
            if (std::memcmp(sub.tag, "IRQD", 4) == 0 && sub.size() >= 4) {
                uint32_t efid;
                std::memcpy(&efid, sub.data.data(), 4);
                ingr.effectFormIDs.push_back(efid);

                // Try to parse companion subrecords
                float mag = 0;
                if (i + 1 < rec.subRecords.size() && std::memcmp(rec.subRecords[i + 1].tag, "IRQF", 4) == 0 && rec.subRecords[i + 1].size() >= 4) {
                    std::memcpy(&mag, rec.subRecords[i + 1].data.data(), 4);
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

        // DATA: value + weight
        auto* data = rec.findSubRecord("DATA");
        if (data && data->size() >= 8) {
            std::memcpy(&alch.value, data->data.data(), 4);
            std::memcpy(&alch.weight, data->data.data() + 4, 4);
        }

        // ENIT: header (12 bytes) - contains flags, etc.
        // Effects use EFID + EFIT pairs (same as SPEL)
        float currentMag = 0;
        uint32_t currentArea = 0, currentDur = 0;
        bool collecting = false;

        for (size_t i = 0; i < rec.subRecords.size(); i++) {
            const auto& sub = rec.subRecords[i];
            if (std::memcmp(sub.tag, "EFID", 4) == 0 && sub.size() >= 4) {
                uint32_t efid;
                std::memcpy(&efid, sub.data.data(), 4);
                alch.effectFormIDs.push_back(efid);

                // EFIT follows EFID
                if (i + 1 < rec.subRecords.size() && std::memcmp(rec.subRecords[i + 1].tag, "EFIT", 4) == 0 && rec.subRecords[i + 1].size() >= 12) {
                    float mag;
                    uint32_t area, dur;
                    std::memcpy(&mag, rec.subRecords[i + 1].data.data(), 4);
                    std::memcpy(&area, rec.subRecords[i + 1].data.data() + 4, 4);
                    std::memcpy(&dur, rec.subRecords[i + 1].data.data() + 8, 4);
                    alch.effectMagnitudes.push_back(mag);
                    alch.effectAreas.push_back(area);
                    alch.effectDurations.push_back(dur);
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
        road.cellFormID = rec.getFormID("XLCN");  // Linked cell

        // PGRD/PGRI: path grid data
        // PGRP: PGRP (PathGrid Points) - array of 3D vertices
        auto* pgrp = rec.findSubRecord("PGRP");
        if (pgrp && pgrp->size() >= 12) {
            int numNodes = static_cast<int>(pgrp->size() / 12);
            road.nodes.reserve(numNodes);
            for (int i = 0; i < numNodes && (i + 1) * 12 <= pgrp->size(); i++) {
                glm::vec3 node;
                std::memcpy(&node, pgrp->data.data() + i * 12, 12);
                road.nodes.push_back(node);
            }
        }

        // PGRR: PathGrid Edges (4 bytes per edge: 2 uint16_t indices)
        auto* pgrr = rec.findSubRecord("PGRR");
        if (pgrr && pgrr->size() >= 4) {
            int numEdges = static_cast<int>(pgrr->size() / 4);
            road.edges.reserve(numEdges);
            for (int i = 0; i < numEdges; i++) {
                uint16_t a = *reinterpret_cast<const uint16_t*>(pgrr->data.data() + i * 4);
                uint16_t b = *reinterpret_cast<const uint16_t*>(pgrr->data.data() + i * 4 + 2);
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

        // SCHR: script header (20 bytes)
        //   uint32_t refCount
        //   uint32_t compiledLength
        //   uint32_t lastVarIndex
        //   uint32_t scriptType (0=Object, 1=Quest, 2=Magic)
        //   uint32_t varCount
        auto* schr = rec.findSubRecord("SCHR");
        if (schr && schr->size() >= 20) {
            const uint8_t* data = schr->data.data();
            uint32_t refCount, compiledLength, lastVarIndex, scriptType, varCount;
            std::memcpy(&refCount, data, 4);
            std::memcpy(&compiledLength, data + 4, 4);
            std::memcpy(&lastVarIndex, data + 8, 4);
            std::memcpy(&scriptType, data + 12, 4);
            std::memcpy(&varCount, data + 16, 4);

            script.scriptType = static_cast<script::ScriptType>(scriptType);
            script.varCount = varCount;
            script.refCount = refCount;
            script.compiledLength = compiledLength;

            LOGD("  SCPT: 0x%08X '%s' type=%d bytecode=%u vars=%u refs=%u",
                 script.formID, script.editorID.c_str(), scriptType,
                 compiledLength, varCount, refCount);
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

        // SLSD: variable data (12 bytes per variable)
        //   uint32_t index
        //   uint8_t  type (0=int, 1=float, 2=string)
        //   uint8_t  flags
        //   uint8_t  padding[2]
        //   float    floatValue (default)
        // SCVR: variable name (null-terminated string)
        // Variables are stored as SLSD/SCVR pairs
        for (size_t i = 0; i < rec.subRecords.size(); ++i) {
            const auto& sub = rec.subRecords[i];
            if (std::memcmp(sub.tag, "SLSD", 4) == 0 && sub.size() >= 12) {
                script::ScriptVariable var;
                const uint8_t* data = sub.data.data();
                uint32_t index;
                uint8_t type;
                std::memcpy(&index, data, 4);
                type = data[4];

                var.index = index;

                // Default value
                if (type == 0) {
                    var.type = script::ScriptValue::Type::Integer;
                    int32_t defVal;
                    std::memcpy(&defVal, data + 8, 4);
                    var.defaultValue = script::ScriptValue::makeInt(defVal);
                } else if (type == 1) {
                    var.type = script::ScriptValue::Type::Float;
                    float defVal;
                    std::memcpy(&defVal, data + 8, 4);
                    var.defaultValue = script::ScriptValue::makeFloat(defVal);
                } else if (type == 2) {
                    var.type = script::ScriptValue::Type::String;
                    var.defaultValue = script::ScriptValue::makeString("");
                }

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

                script.variables.push_back(std::move(var));
            }
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
