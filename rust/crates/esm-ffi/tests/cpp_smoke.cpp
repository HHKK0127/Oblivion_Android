// Smoke test for the esm-ffi C ABI, driven from C++.
//
// The checks mirror the Rust unit tests so that a C-side caller proves the same
// behaviour through the header: a synthetic plugin exercises every accessor,
// and the retail plugin is verified when it is installed. Build and run with:
//
//   g++ -std=c++17 -I crates/esm-ffi/include crates/esm-ffi/tests/cpp_smoke.cpp
//       -L target/x86_64-pc-windows-gnu/release -lesm_ffi -o cpp_smoke.exe

#include "esm_ffi.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr uint32_t WEAP_FORM_ID = 0x000966A9u;
constexpr uint32_t CELL_FORM_ID = 0x00005FC6u;
constexpr uint32_t LAND_FORM_ID = 0x000074E7u;
constexpr uint32_t WORLD_FORM_ID = 0x0000003Cu;

int g_failures = 0;

void check(bool condition, const std::string& what) {
    if (!condition) {
        std::printf("  FAIL: %s\n", what.c_str());
        ++g_failures;
    }
}

template <typename T>
void append_le(std::vector<uint8_t>& out, T value) {
    for (size_t i = 0; i < sizeof(T); ++i) {
        out.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
    }
}

void append_tag(std::vector<uint8_t>& out, const char* tag) {
    out.insert(out.end(), tag, tag + 4);
}

std::vector<uint8_t> sub(const char* tag, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> out;
    append_tag(out, tag);
    append_le<uint16_t>(out, static_cast<uint16_t>(data.size()));
    out.insert(out.end(), data.begin(), data.end());
    return out;
}

std::vector<uint8_t> record(const char* tag, uint32_t form_id, uint32_t flags,
                            const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> out;
    append_tag(out, tag);
    append_le<uint32_t>(out, static_cast<uint32_t>(payload.size()));
    append_le<uint32_t>(out, flags);
    append_le<uint32_t>(out, form_id);
    append_le<uint16_t>(out, 1);
    append_le<uint16_t>(out, 0);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

std::vector<uint8_t> grup(uint32_t label, uint32_t group_type,
                          const std::vector<uint8_t>& content) {
    std::vector<uint8_t> out;
    append_tag(out, "GRUP");
    append_le<uint32_t>(out, static_cast<uint32_t>(20 + content.size()));
    append_le<uint32_t>(out, label);
    append_le<uint32_t>(out, group_type);
    append_le<uint16_t>(out, 0);
    append_le<uint16_t>(out, 0);
    out.insert(out.end(), content.begin(), content.end());
    return out;
}

std::vector<uint8_t> tes4(const std::vector<std::string>& masters) {
    std::vector<uint8_t> payload = sub("HEDR", std::vector<uint8_t>(12, 0));
    for (const std::string& master : masters) {
        std::vector<uint8_t> name(master.begin(), master.end());
        name.push_back(0);
        const std::vector<uint8_t> mast = sub("MAST", name);
        payload.insert(payload.end(), mast.begin(), mast.end());
        const std::vector<uint8_t> data = sub("DATA", std::vector<uint8_t>(8, 0));
        payload.insert(payload.end(), data.begin(), data.end());
    }
    return record("TES4", 0, 0, payload);
}

std::vector<uint8_t> weap_payload() {
    std::vector<uint8_t> payload =
        sub("EDID", std::vector<uint8_t>{'S', 'E', '1', '3', 'T', 'r', 'o', 'p', 'h', 'y', 'S',
                                         'w', 'o', 'r', 'd', '1', 0});
    const char* full = "Sword of Jyggalag";
    const std::vector<uint8_t> full_sub =
        sub("FULL", std::vector<uint8_t>(full, full + std::strlen(full) + 1));
    payload.insert(payload.end(), full_sub.begin(), full_sub.end());

    std::vector<uint8_t> data_sub;
    append_le<uint32_t>(data_sub, 0x422A0000u);  // 42.5f
    const std::vector<uint8_t> data = sub("DATA", data_sub);
    payload.insert(payload.end(), data.begin(), data.end());

    const std::vector<uint8_t> anam = sub("ANAM", std::vector<uint8_t>{0x01, 0x02});
    payload.insert(payload.end(), anam.begin(), anam.end());
    return payload;
}

std::vector<uint8_t> land_payload() {
    std::vector<uint8_t> payload = sub("VNML", std::vector<uint8_t>(12, 0x40));
    const std::vector<uint8_t> vhgt = sub("VHGT", std::vector<uint8_t>(8, 0));
    payload.insert(payload.end(), vhgt.begin(), vhgt.end());
    return payload;
}

uint32_t adler32(const std::vector<uint8_t>& data) {
    uint32_t a = 1;
    uint32_t b = 0;
    for (uint8_t byte : data) {
        a = (a + byte) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

// A zlib stream holding the payload in a single stored block, so that the
// inflate path can be exercised without linking zlib.
std::vector<uint8_t> zlib_stored(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> out{0x78, 0x01, 0x01};
    append_le<uint16_t>(out, static_cast<uint16_t>(payload.size()));
    append_le<uint16_t>(out, static_cast<uint16_t>(~payload.size() & 0xFFFF));
    out.insert(out.end(), payload.begin(), payload.end());
    const uint32_t sum = adler32(payload);
    out.push_back(static_cast<uint8_t>(sum >> 24));
    out.push_back(static_cast<uint8_t>(sum >> 16));
    out.push_back(static_cast<uint8_t>(sum >> 8));
    out.push_back(static_cast<uint8_t>(sum));
    return out;
}

constexpr uint32_t REC_FLAG_COMPRESSED = 0x00040000u;

std::vector<uint8_t> compressed_record(const char* tag, uint32_t form_id,
                                       const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> body;
    append_le<uint32_t>(body, static_cast<uint32_t>(payload.size()));
    const std::vector<uint8_t> stream = zlib_stored(payload);
    body.insert(body.end(), stream.begin(), stream.end());
    return record(tag, form_id, REC_FLAG_COMPRESSED, body);
}

// The plugin layout the Rust tests build: TES4, a WEAP record, a CELL inside a
// world group, and a compressed LAND inside a cell-children group.
std::vector<uint8_t> plugin_bytes() {
    std::vector<uint8_t> out = tes4({"Oblivion.esm", "Knights.esp"});
    const std::vector<uint8_t> weap = record("WEAP", WEAP_FORM_ID, 0, weap_payload());
    out.insert(out.end(), weap.begin(), weap.end());

    const std::vector<uint8_t> cell_payload = sub("EDID", std::vector<uint8_t>{'T', 'e', 's', 't',
                                                                              'C', 'e', 'l', 'l',
                                                                              0});
    const std::vector<uint8_t> cell = record("CELL", CELL_FORM_ID, 0, cell_payload);
    const std::vector<uint8_t> world = grup(WORLD_FORM_ID, 1, cell);
    out.insert(out.end(), world.begin(), world.end());

    const std::vector<uint8_t> land = compressed_record("LAND", LAND_FORM_ID, land_payload());
    const std::vector<uint8_t> cell_child = grup(CELL_FORM_ID, 6, land);
    out.insert(out.end(), cell_child.begin(), cell_child.end());
    return out;
}

bool type_is(const EsmRecordInfo& info, const char* tag) {
    return std::memcmp(info.rec_type, tag, 4) == 0;
}

std::string error_text() {
    char buffer[512] = {0};
    esm_last_error_message(buffer, sizeof(buffer));
    return std::string(buffer);
}

void test_abi_sizes() {
    std::printf("abi sizes\n");
    check(esm_record_info_size() == sizeof(EsmRecordInfo), "record info size");
    check(esm_subrecord_view_size() == sizeof(EsmSubRecordView), "subrecord view size");
    const char* version = esm_version();
    check(version != nullptr && std::strncmp(version, "esm-ffi ", 8) == 0, "version prefix");
    std::printf("  version=%s record_info=%zu view=%zu\n", version, sizeof(EsmRecordInfo),
                sizeof(EsmSubRecordView));
}

void test_metadata(const std::vector<uint8_t>& bytes) {
    std::printf("metadata\n");
    EsmPlugin* plugin = esm_plugin_parse_memory("Test.esm", bytes.data(), bytes.size());
    check(plugin != nullptr, "parse_memory: " + error_text());
    if (plugin == nullptr) {
        return;
    }
    check(esm_plugin_record_count(plugin) == 3, "record count");
    check(esm_plugin_group_count(plugin) == 2, "group count");
    check(esm_plugin_compressed_count(plugin) == 1, "compressed count");
    check(esm_plugin_is_master(plugin) == 1, "master flag");
    check(esm_plugin_master_count(plugin) == 2, "master count");
    check(esm_plugin_header_size(plugin) == 83, "header size");
    check(esm_last_error_code() == ESM_OK, "no error after parsing");

    char name[32] = {0};
    check(esm_plugin_file_name(plugin, name, sizeof(name)) == 9, "file name length");
    check(std::strcmp(name, "Test.esm") == 0, "file name");

    char master[32] = {0};
    check(esm_plugin_master_at(plugin, 1, master, sizeof(master)) > 0, "master 1");
    check(std::strcmp(master, "Knights.esp") == 0, "master 1 text");

    char small[5] = {0};
    check(esm_plugin_file_name(plugin, small, sizeof(small)) == 9, "truncated length");
    check(std::strcmp(small, "Test") == 0, "truncated text");
    check(esm_plugin_master_at(plugin, 9, master, sizeof(master)) == -1, "master out of range");

    esm_plugin_free(plugin);
}

void test_record_info(const std::vector<uint8_t>& bytes) {
    std::printf("record info\n");
    EsmPlugin* plugin = esm_plugin_parse_memory("Test.esm", bytes.data(), bytes.size());
    if (plugin == nullptr) {
        check(false, "parse_memory: " + error_text());
        return;
    }
    EsmRecordInfo info{};
    check(esm_plugin_find_record(plugin, WEAP_FORM_ID, &info) == ESM_OK, "find WEAP");
    check(type_is(info, "WEAP"), "WEAP tag");
    check(info.form_id == WEAP_FORM_ID, "WEAP form id");
    check(info.compressed == 0, "WEAP not compressed");
    check(info.file_offset == 123, "WEAP payload offset");
    check(info.parent_form_id == 0 && info.worldspace_form_id == 0, "WEAP not grouped");

    check(esm_plugin_find_record(plugin, CELL_FORM_ID, &info) == ESM_OK, "find CELL");
    check(type_is(info, "CELL"), "CELL tag");
    check(info.worldspace_form_id == WORLD_FORM_ID, "CELL worldspace");

    check(esm_plugin_find_record(plugin, LAND_FORM_ID, &info) == ESM_OK, "find LAND");
    check(type_is(info, "LAND"), "LAND tag");
    check(info.compressed == 1, "LAND compressed");
    check(info.parent_form_id == CELL_FORM_ID, "LAND parent cell");
    check(info.decomp_size == 32, "LAND decompressed size");

    check(esm_plugin_find_record(plugin, 0xDEADBEEFu, &info) == ESM_ERROR_NOT_FOUND,
          "unknown form id");
    check(esm_last_error_code() == ESM_ERROR_NOT_FOUND, "not found error code");
    check(error_text().find("no record") != std::string::npos, "not found message");

    EsmRecordInfo first{};
    check(esm_plugin_record_info_at(plugin, 0, &first) == ESM_OK, "record 0");
    check(type_is(first, "WEAP"), "record 0 tag");
    EsmRecordInfo third{};
    check(esm_plugin_record_info_at(plugin, 2, &third) == ESM_OK, "record 2");
    check(type_is(third, "LAND"), "record 2 tag");
    check(esm_plugin_record_info_at(plugin, 3, &third) == ESM_ERROR_NOT_FOUND, "record 3");

    esm_plugin_free(plugin);
}

void test_subrecords(const std::vector<uint8_t>& bytes) {
    std::printf("subrecords\n");
    EsmPlugin* plugin = esm_plugin_parse_memory("Test.esm", bytes.data(), bytes.size());
    if (plugin == nullptr) {
        check(false, "parse_memory: " + error_text());
        return;
    }
    EsmPayload* payload = esm_plugin_decode_record(plugin, WEAP_FORM_ID);
    check(payload != nullptr, "decode WEAP: " + error_text());
    if (payload == nullptr) {
        esm_plugin_free(plugin);
        return;
    }
    check(esm_payload_subrecord_count(payload) == 4, "subrecord count");

    EsmSubRecordView view{};
    check(esm_payload_subrecord_at(payload, 0, &view) == ESM_OK, "subrecord 0");
    check(std::memcmp(view.tag, "EDID", 4) == 0, "subrecord 0 tag");
    check(view.size == 17, "subrecord 0 size");
    check(std::memcmp(view.data, "SE13TrophySword1", 17) == 0, "subrecord 0 data");

    char text[32] = {0};
    check(esm_payload_zstring(payload, 1, text, sizeof(text)) == 18, "zstring length");
    check(std::strcmp(text, "Sword of Jyggalag") == 0, "zstring text");

    EsmSubRecordView data{};
    check(esm_payload_find_subrecord(payload, "DATA", &data) == ESM_OK, "find DATA");
    check(data.size == 4, "DATA size");
    check(esm_payload_f32(payload, 2) == 42.5f, "DATA float");
    check(esm_payload_u32(payload, 2) == 0x422A0000u, "DATA raw bits");

    EsmSubRecordView anam{};
    check(esm_payload_find_subrecord(payload, "ANAM", &anam) == ESM_OK, "find ANAM");
    check(anam.size == 2, "ANAM size");
    check(esm_payload_u16(payload, 3) == 0x0201, "ANAM u16");
    check(esm_payload_u32(payload, 3) == 0, "ANAM u32 fallback");
    check(esm_payload_i32(payload, 3) == 0, "ANAM i32 fallback");
    check(esm_payload_f32(payload, 3) == 0.0f, "ANAM f32 fallback");
    check(esm_payload_float_or_int(payload, 3) == 0.0f, "ANAM float_or_int fallback");

    check(esm_payload_find_subrecord(payload, "XXXX", &data) == ESM_ERROR_NOT_FOUND, "find XXXX");
    check(esm_payload_subrecord_at(payload, 9, &view) == ESM_ERROR_NOT_FOUND, "subrecord 9");
    check(esm_last_error_code() == ESM_ERROR_NOT_FOUND, "not found error code");

    esm_payload_free(payload);
    esm_plugin_free(plugin);
}

void test_compressed_and_header(const std::vector<uint8_t>& bytes) {
    std::printf("compressed records and header\n");
    EsmPlugin* plugin = esm_plugin_parse_memory("Test.esm", bytes.data(), bytes.size());
    if (plugin == nullptr) {
        check(false, "parse_memory: " + error_text());
        return;
    }
    EsmPayload* payload = esm_plugin_decode_record(plugin, LAND_FORM_ID);
    check(payload != nullptr, "inflate LAND: " + error_text());
    if (payload != nullptr) {
        check(esm_payload_subrecord_count(payload) == 2, "LAND subrecord count");
        EsmSubRecordView view{};
        check(esm_payload_subrecord_at(payload, 0, &view) == ESM_OK, "LAND subrecord 0");
        check(std::memcmp(view.tag, "VNML", 4) == 0, "LAND subrecord 0 tag");
        check(view.size == 12, "LAND subrecord 0 size");
        check(view.data[0] == 0x40, "LAND subrecord 0 data");
        esm_payload_free(payload);
    }

    EsmPayload* header = esm_plugin_decode_header(plugin);
    check(header != nullptr, "decode header: " + error_text());
    if (header != nullptr) {
        check(esm_payload_subrecord_count(header) == 5, "header subrecord count");
        EsmSubRecordView hedr{};
        check(esm_payload_find_subrecord(header, "HEDR", &hedr) == ESM_OK, "find HEDR");
        check(std::memcmp(hedr.tag, "HEDR", 4) == 0, "HEDR tag");
        esm_payload_free(header);
    }

    esm_plugin_free(plugin);
}

void test_type_counts(const std::vector<uint8_t>& bytes) {
    std::printf("type counts\n");
    EsmPlugin* plugin = esm_plugin_parse_memory("Test.esm", bytes.data(), bytes.size());
    if (plugin == nullptr) {
        check(false, "parse_memory: " + error_text());
        return;
    }
    EsmTypeCounts* counts = esm_plugin_type_counts(plugin);
    check(counts != nullptr, "type counts");
    if (counts != nullptr) {
        check(esm_type_counts_len(counts) == 3, "distinct types");
        const char* expected[3] = {"CELL", "LAND", "WEAP"};
        for (size_t index = 0; index < 3; ++index) {
            char tag[5] = {0};
            size_t count = 0;
            check(esm_type_counts_at(counts, index, tag, &count) == ESM_OK, "type index");
            check(std::strcmp(tag, expected[index]) == 0, std::string("type tag ") + expected[index]);
            check(count == 1, std::string("type count ") + expected[index]);
        }
        char tag[5] = {0};
        size_t count = 0;
        check(esm_type_counts_at(counts, 3, tag, &count) == ESM_ERROR_NOT_FOUND, "type index 3");
        esm_type_counts_free(counts);
    }
    esm_plugin_free(plugin);
}

void test_invalid_input() {
    std::printf("invalid input\n");
    std::vector<uint8_t> bogus(64, 0);
    check(esm_plugin_parse_memory("Bogus.esm", bogus.data(), bogus.size()) == nullptr, "bogus");
    check(esm_last_error_code() == ESM_ERROR_BAD_HEADER, "bad header code");
    check(error_text().find("TES4") != std::string::npos, "bad header message");

    check(esm_plugin_parse_memory("Empty.esm", nullptr, 0) == nullptr, "null data");
    check(esm_last_error_code() == ESM_ERROR_INVALID_ARGUMENT, "null data code");
    check(esm_plugin_open(nullptr) == nullptr, "null path");
    check(esm_plugin_open("C:\\no\\such\\plugin.esm") == nullptr, "missing file");
    check(esm_last_error_code() == ESM_ERROR_IO, "missing file code");

    check(esm_plugin_record_count(nullptr) == 0, "null record count");
    check(esm_plugin_group_count(nullptr) == 0, "null group count");
    check(esm_plugin_compressed_count(nullptr) == 0, "null compressed count");
    check(esm_plugin_is_master(nullptr) == 0, "null master flag");
    check(esm_plugin_header_size(nullptr) == 0, "null header size");
    check(esm_plugin_master_count(nullptr) == 0, "null master count");
    check(esm_plugin_file_name(nullptr, nullptr, 0) == -1, "null file name");
    check(esm_plugin_master_at(nullptr, 0, nullptr, 0) == -1, "null master");
    check(esm_plugin_type_counts(nullptr) == nullptr, "null type counts");
    check(esm_plugin_decode_record(nullptr, 1) == nullptr, "null decode record");
    check(esm_plugin_decode_record_at(nullptr, 0) == nullptr, "null decode record at");
    check(esm_plugin_decode_header(nullptr) == nullptr, "null decode header");
    check(esm_payload_subrecord_count(nullptr) == 0, "null subrecord count");
    check(esm_payload_u32(nullptr, 0) == 0, "null u32");
    check(esm_payload_zstring(nullptr, 0, nullptr, 0) == -1, "null zstring");
    esm_plugin_free(nullptr);
    esm_payload_free(nullptr);
    esm_type_counts_free(nullptr);
}

void test_retail_plugin(const char* path) {
    std::printf("retail plugin\n");
    EsmPlugin* plugin = esm_plugin_open(path);
    if (plugin == nullptr) {
        std::printf("  SKIP: %s unavailable (%s)\n", path, error_text().c_str());
        return;
    }
    check(esm_plugin_record_count(plugin) == 1167016u, "record count");
    check(esm_plugin_group_count(plugin) == 85079u, "group count");
    check(esm_plugin_compressed_count(plugin) == 41789u, "compressed count");
    check(esm_plugin_is_master(plugin) == 1, "master flag");
    check(esm_plugin_header_size(plugin) == 744, "header size");
    check(esm_plugin_master_count(plugin) == 0, "master count");

    char name[256] = {0};
    check(esm_plugin_file_name(plugin, name, sizeof(name)) > 0, "file name");
    check(std::strcmp(name, "Oblivion.esm") == 0, "file name text");

    EsmRecordInfo info{};
    check(esm_plugin_find_record(plugin, WEAP_FORM_ID, &info) == ESM_OK, "find WEAP");
    check(type_is(info, "WEAP"), "WEAP tag");
    EsmPayload* payload = esm_plugin_decode_record(plugin, WEAP_FORM_ID);
    check(payload != nullptr, "decode WEAP: " + error_text());
    if (payload != nullptr) {
        EsmSubRecordView edid{};
        check(esm_payload_find_subrecord(payload, "EDID", &edid) == ESM_OK, "find EDID");
        check(edid.size == 17 && std::memcmp(edid.data, "SE13TrophySword1", 17) == 0, "EDID text");
        esm_payload_free(payload);
    }

    check(esm_plugin_find_record(plugin, LAND_FORM_ID, &info) == ESM_OK, "find LAND");
    check(type_is(info, "LAND"), "LAND tag");
    check(info.compressed == 1, "LAND compressed");
    check(info.parent_form_id == CELL_FORM_ID, "LAND parent cell");
    check(info.worldspace_form_id == WORLD_FORM_ID, "LAND worldspace");

    payload = esm_plugin_decode_record_at(plugin, 0);
    check(payload != nullptr, "decode record 0: " + error_text());
    esm_payload_free(payload);

    EsmTypeCounts* counts = esm_plugin_type_counts(plugin);
    check(counts != nullptr, "type counts");
    check(esm_type_counts_len(counts) > 1, "distinct types");
    esm_type_counts_free(counts);

    esm_plugin_free(plugin);
}

}  // namespace

int main(int argc, char** argv) {
    std::printf("esm-ffi C++ smoke test\n");
    const std::vector<uint8_t> bytes = plugin_bytes();
    test_abi_sizes();
    test_metadata(bytes);
    test_record_info(bytes);
    test_subrecords(bytes);
    test_compressed_and_header(bytes);
    test_type_counts(bytes);
    test_invalid_input();

    std::string path;
    if (argc > 1) {
        path = argv[1];
    } else if (const char* env = std::getenv("OBLIVION_ESM")) {
        path = env;
    } else {
        path = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Oblivion\\Data\\Oblivion.esm";
    }
    test_retail_plugin(path.c_str());

    if (g_failures == 0) {
        std::printf("ALL PASSED\n");
        return 0;
    }
    std::printf("%d CHECK(S) FAILED\n", g_failures);
    return 1;
}
