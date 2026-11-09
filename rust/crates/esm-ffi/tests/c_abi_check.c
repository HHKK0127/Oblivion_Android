/*
 * Compiles the public header as C11 and exercises the ABI from a C translation
 * unit, which is the language the engine consumes when the Rust staticlib is
 * linked into the Android NDK build.
 *
 * The C++ smoke test covers behaviour; this file covers the two things that
 * only a C compiler can check:
 *   - the header is valid C (the C++ test cannot see a C-only syntax error)
 *   - the struct sizes and status codes the header advertises agree with the
 *     values the Rust library reports at runtime
 *
 * Build (see AGENTS.md for the full cross-build recipe):
 *   gcc -std=c11 -Wall -Wextra -I crates/esm-ffi/include \
 *       crates/esm-ffi/tests/c_abi_check.c <path>/libesm_ffi.a \
 *       -lws2_32 -lbcrypt -luserenv -lntdll ... -o c_abi_check.exe
 */

#include "esm_ffi.h"
/* Included twice on purpose: the guard must absorb the second inclusion. */
#include "esm_ffi.h"

#include <stdio.h>
#include <string.h>

/* The header asserts these under its own typedef names; repeat them here so a
 * regression in the macro itself shows up as a failure of this file too. */
ESM_FFI_STATIC_ASSERT(sizeof(EsmRecordInfo) == 40, c_abi_check_record_info_size);
ESM_FFI_STATIC_ASSERT(sizeof(EsmSubRecordView) == (UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFull ? 16 : 12),
                      c_abi_check_subrecord_view_size);

static int g_failures = 0;

static void check(int condition, const char *what) {
    if (!condition) {
        printf("  FAIL: %s\n", what);
        ++g_failures;
    }
}

/* ---------------------------------------------------------------------------
 * Minimal plugin builder: TES4 header plus MAST/DATA pairs, nothing else.
 * ------------------------------------------------------------------------ */

static uint8_t g_bytes[512];
static size_t g_len = 0;

static void put_u16(uint16_t value) {
    g_bytes[g_len++] = (uint8_t)(value & 0xFF);
    g_bytes[g_len++] = (uint8_t)((value >> 8) & 0xFF);
}

static void put_u32(uint32_t value) {
    g_bytes[g_len++] = (uint8_t)(value & 0xFF);
    g_bytes[g_len++] = (uint8_t)((value >> 8) & 0xFF);
    g_bytes[g_len++] = (uint8_t)((value >> 16) & 0xFF);
    g_bytes[g_len++] = (uint8_t)((value >> 24) & 0xFF);
}

static void patch_u32(size_t offset, uint32_t value) {
    g_bytes[offset + 0] = (uint8_t)(value & 0xFF);
    g_bytes[offset + 1] = (uint8_t)((value >> 8) & 0xFF);
    g_bytes[offset + 2] = (uint8_t)((value >> 16) & 0xFF);
    g_bytes[offset + 3] = (uint8_t)((value >> 24) & 0xFF);
}

static void put_tag(const char *tag) {
    memcpy(g_bytes + g_len, tag, 4);
    g_len += 4;
}

static void emit_sub_zeros(const char *tag, size_t count) {
    put_tag(tag);
    put_u16((uint16_t)count);
    memset(g_bytes + g_len, 0, count);
    g_len += count;
}

/* Writes `text` plus its terminator, the zstring form the engine expects. */
static void emit_sub_text(const char *tag, const char *text) {
    const size_t size = strlen(text) + 1;
    put_tag(tag);
    put_u16((uint16_t)size);
    memcpy(g_bytes + g_len, text, size);
    g_len += size;
}

static size_t build_tes4_plugin(void) {
    g_len = 0;
    put_tag("TES4");
    const size_t size_offset = g_len;
    put_u32(0); /* data_size, backpatched below */
    put_u32(0); /* flags */
    put_u32(0); /* form id */
    put_u16(1); /* version */
    put_u16(0); /* unknown */
    const size_t payload_start = g_len;
    emit_sub_zeros("HEDR", 12);
    emit_sub_text("MAST", "Oblivion.esm");
    emit_sub_zeros("DATA", 8);
    emit_sub_text("MAST", "Knights.esp");
    emit_sub_zeros("DATA", 8);
    patch_u32(size_offset, (uint32_t)(g_len - payload_start));
    return g_len;
}

/* ---------------------------------------------------------------------------
 * Checks
 * ------------------------------------------------------------------------ */

static void test_abi_values(void) {
    printf("abi values\n");
    check(esm_record_info_size() == sizeof(EsmRecordInfo), "record info size agrees with C");
    check(esm_record_info_size() == 40, "record info size is 40");
    check(esm_subrecord_view_size() == sizeof(EsmSubRecordView), "subrecord view size agrees with C");
    check(esm_subrecord_view_size() == (UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFull ? 16u : 12u),
          "subrecord view size matches pointer width");

    check(ESM_OK == 0, "ESM_OK is 0");
    check(ESM_ERROR_INVALID_ARGUMENT == -1, "invalid argument is -1");
    check(ESM_ERROR_IO == -2, "io is -2");
    check(ESM_ERROR_TRUNCATED == -3, "truncated is -3");
    check(ESM_ERROR_BAD_HEADER == -4, "bad header is -4");
    check(ESM_ERROR_INFLATE == -5, "inflate is -5");
    check(ESM_ERROR_NOT_FOUND == -6, "not found is -6");
    check(ESM_ERROR_PANIC == -7, "panic is -7");

    check(esm_version() != NULL, "version is not NULL");
    check(strncmp(esm_version(), "esm-ffi ", 8) == 0, "version starts with the crate name");
}

static void test_null_handles(void) {
    char buffer[64];

    printf("null handles\n");
    memset(buffer, 0x7F, sizeof(buffer));
    check(esm_plugin_record_count(NULL) == 0, "null record count");
    check(esm_plugin_group_count(NULL) == 0, "null group count");
    check(esm_plugin_compressed_count(NULL) == 0, "null compressed count");
    check(esm_plugin_is_master(NULL) == 0, "null master flag");
    check(esm_plugin_header_size(NULL) == 0, "null header size");
    check(esm_plugin_master_count(NULL) == 0, "null master count");
    check(esm_plugin_file_name(NULL, buffer, sizeof(buffer)) == -1, "null file name");
    check(esm_plugin_master_at(NULL, 0, buffer, sizeof(buffer)) == -1, "null master at");
    check(esm_plugin_type_counts(NULL) == NULL, "null type counts");
    check(esm_plugin_decode_record(NULL, 1) == NULL, "null decode record");
    check(esm_plugin_decode_record_at(NULL, 0) == NULL, "null decode record at");
    check(esm_plugin_decode_header(NULL) == NULL, "null decode header");
    check(esm_type_counts_len(NULL) == 0, "null type counts len");
    check(esm_payload_subrecord_count(NULL) == 0, "null subrecord count");
    check(esm_payload_u16(NULL, 0) == 0, "null u16");
    check(esm_payload_u32(NULL, 0) == 0, "null u32");
    check(esm_payload_i32(NULL, 0) == 0, "null i32");
    check(esm_payload_f32(NULL, 0) == 0.0f, "null f32");
    check(esm_payload_float_or_int(NULL, 0) == 0.0f, "null float or int");
    check(esm_payload_zstring(NULL, 0, buffer, sizeof(buffer)) == -1, "null zstring");
    check(esm_last_error_code() == ESM_ERROR_INVALID_ARGUMENT,
          "reading a NULL payload reports invalid argument");
    check(esm_last_error_message(buffer, sizeof(buffer)) == (int)strlen(buffer) + 1,
          "message length includes the terminator");

    {
        const int before = esm_last_error_code();
        esm_plugin_free(NULL);
        esm_payload_free(NULL);
        esm_type_counts_free(NULL);
        check(esm_last_error_code() == before, "free of NULL leaves the error state alone");
    }
}

static int bytes_are(const void *block, size_t size, uint8_t value) {
    const uint8_t *raw = (const uint8_t *)block;
    size_t i;
    for (i = 0; i < size; ++i) {
        if (raw[i] != value) {
            return 0;
        }
    }
    return 1;
}

static void test_header_plugin(void) {
    const size_t length = build_tes4_plugin();
    const uint32_t header_size = (uint32_t)(length - 20);
    char buffer[64];
    EsmRecordInfo info;
    EsmPayload *header;
    EsmTypeCounts *counts;
    EsmSubRecordView view;

    printf("tes4-only plugin\n");
    {
        EsmPlugin *plugin = esm_plugin_parse_memory("c_abi_check.esm", g_bytes, length);
        check(plugin != NULL, "parse succeeds");
        if (plugin == NULL) {
            return;
        }
        check(esm_last_error_code() == ESM_OK, "parse clears the error");
        check(esm_plugin_record_count(plugin) == 0, "no records past the header");
        check(esm_plugin_group_count(plugin) == 0, "no groups");
        check(esm_plugin_compressed_count(plugin) == 0, "nothing compressed");
        check(esm_plugin_is_master(plugin) == 1, "esm extension marks a master");
        check(esm_plugin_header_size(plugin) == header_size, "header size");
        check(esm_plugin_master_count(plugin) == 2, "two masters");

        check(esm_plugin_file_name(plugin, buffer, sizeof(buffer)) == (int)strlen("c_abi_check.esm") + 1,
              "file name length");
        check(strcmp(buffer, "c_abi_check.esm") == 0, "file name text");

        memset(buffer, 0x7F, sizeof(buffer));
        check(esm_plugin_master_at(plugin, 0, buffer, sizeof(buffer)) == (int)strlen("Oblivion.esm") + 1,
              "master length");
        check(strcmp(buffer, "Oblivion.esm") == 0, "master text");
        check(esm_plugin_master_at(plugin, 1, buffer, sizeof(buffer)) == (int)strlen("Knights.esp") + 1,
              "second master length");
        check(esm_plugin_master_at(plugin, 2, buffer, sizeof(buffer)) == -1, "master index out of range");
        /* A NULL buffer is a length query, not an error. */
        check(esm_plugin_master_at(plugin, 0, NULL, 0) == (int)strlen("Oblivion.esm") + 1,
              "master length query");

        memset(&info, 0xAA, sizeof(info));
        check(esm_plugin_record_info_at(plugin, 0, &info) == ESM_ERROR_NOT_FOUND,
              "record info index out of range");
        check(bytes_are(&info, sizeof(info), 0xAA),
              "record info is not partially written on failure");
        check(esm_plugin_find_record(plugin, 0x000966A9u, &info) == ESM_ERROR_NOT_FOUND,
              "unknown form id");
        check(esm_plugin_decode_record(plugin, 0x000966A9u) == NULL, "decode of an unknown form id");
        check(esm_plugin_decode_record_at(plugin, 0) == NULL, "decode of an out-of-range index");

        counts = esm_plugin_type_counts(plugin);
        check(counts != NULL, "type counts are produced");
        check(esm_type_counts_len(counts) == 0, "no record types");
        check(esm_type_counts_at(counts, 0, buffer, NULL) == ESM_ERROR_NOT_FOUND,
              "type counts index out of range");
        esm_type_counts_free(counts);

        header = esm_plugin_decode_header(plugin);
        check(header != NULL, "header decodes");
        if (header != NULL) {
            check(esm_payload_subrecord_count(header) == 5, "header has 5 subrecords");
            memset(&view, 0, sizeof(view));
            check(esm_payload_subrecord_at(header, 0, &view) == ESM_OK, "first subrecord");
            check(memcmp(view.tag, "HEDR", 4) == 0, "first tag is HEDR");
            check(view.size == 12, "HEDR is 12 bytes");
            check(view.data != NULL, "HEDR has data");
            check(esm_payload_subrecord_at(header, 5, &view) == ESM_ERROR_NOT_FOUND,
                  "subrecord index out of range");

            check(esm_payload_find_subrecord(header, "MAST", &view) == ESM_OK, "find MAST");
            check(view.size == (uint32_t)strlen("Oblivion.esm") + 1, "MAST size");
            check(memcmp(view.data, "Oblivion.esm", strlen("Oblivion.esm")) == 0, "MAST text");
            check(esm_payload_find_subrecord(header, "NOPE", &view) == ESM_ERROR_NOT_FOUND, "find NOPE");

            memset(buffer, 0x7F, sizeof(buffer));
            check(esm_payload_zstring(header, 1, buffer, sizeof(buffer)) ==
                      (int)strlen("Oblivion.esm") + 1,
                  "zstring length");
            check(strcmp(buffer, "Oblivion.esm") == 0, "zstring text");

            memset(buffer, 0x7F, sizeof(buffer));
            check(esm_payload_zstring(header, 1, buffer, 4) == (int)strlen("Oblivion.esm") + 1,
                  "truncated zstring still reports the full length");
            check(buffer[3] == '\0', "truncated zstring terminates the buffer");

            check(esm_payload_u16(header, 2) == 0, "DATA placeholder is zero");
            check(esm_payload_zstring(header, 5, buffer, sizeof(buffer)) == -1,
                  "zstring index out of range");
            esm_payload_free(header);
        }
        esm_plugin_free(plugin);
    }
}

static void test_invalid_input(void) {
    uint8_t bogus[64];

    printf("invalid input\n");
    memset(bogus, 0, sizeof(bogus));
    check(esm_plugin_parse_memory("Bogus.esm", bogus, sizeof(bogus)) == NULL, "bogus bytes");
    check(esm_last_error_code() == ESM_ERROR_BAD_HEADER, "bad header code");
    check(esm_plugin_parse_memory("Empty.esm", NULL, 0) == NULL, "null data");
    check(esm_last_error_code() == ESM_ERROR_INVALID_ARGUMENT, "null data code");
    check(esm_plugin_parse_memory("Short.esm", bogus, 3) == NULL, "truncated record header");
    check(esm_last_error_code() == ESM_ERROR_TRUNCATED, "truncated code");
    check(esm_plugin_open(NULL) == NULL, "null path");
    check(esm_last_error_code() == ESM_ERROR_INVALID_ARGUMENT, "null path code");
    check(esm_plugin_open("C:\\no\\such\\plugin.esm") == NULL, "missing file");
    check(esm_last_error_code() == ESM_ERROR_IO, "missing file code");
}

int main(void) {
    printf("c_abi_check: sizeof(EsmRecordInfo)=%u sizeof(EsmSubRecordView)=%u pointer=%u bits\n",
           (unsigned)sizeof(EsmRecordInfo), (unsigned)sizeof(EsmSubRecordView),
           (unsigned)(sizeof(void *) * 8));

    test_abi_values();
    test_null_handles();
    test_header_plugin();
    test_invalid_input();

    if (g_failures != 0) {
        printf("FAILED: %d check(s)\n", g_failures);
        return 1;
    }
    printf("ALL PASSED\n");
    return 0;
}
