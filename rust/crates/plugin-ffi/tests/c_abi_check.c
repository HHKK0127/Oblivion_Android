/* c_abi_check.c - compiles plugin_ffi.h as C11 and exercises the ABI edges that
 * the C++ unit tests in Oblivion_Android cannot reach: NULL arguments, the
 * length query, and a short buffer.
 *
 * Build and run (mingw-w64, from the crate root):
 *   gcc -std=c11 -Wall -Wextra -I include tests/c_abi_check.c \
 *       target/gnu/x86_64-pc-windows-gnu/release/libplugin_ffi.a \
 *       -lws2_32 -lbcrypt -luserenv -lntdll -lole32 -loleaut32 -ladvapi32 \
 *       -luser32 -lshell32 -lkernel32 -o c_abi_check.exe
 */
#include <stdio.h>
#include <string.h>

#include "plugin_ffi.h"

static int failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            ++failures;                                                          \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
        }                                                                        \
    } while (0)

static const char *g_masters[] = {"Oblivion.esm", "Knights.esp"};
static const char *g_loaded[] = {"Oblivion.esm", "Mod.esp", "Knights.esp"};

static void check_constants(void)
{
    char name[32];

    CHECK(plg_unresolved_formid_index() == 0xFFFFFFFFu);
    CHECK(plg_formid_index_map_len(0) == 1);
    CHECK(plg_formid_index_map_len(2) == 3);
    CHECK(plg_formid_index_map_len((size_t)-1) == (size_t)-1);

    CHECK(plg_base_plugin_file_name(name, sizeof(name)) == 13);
    CHECK(strcmp(name, "Oblivion.esm") == 0);

    /* A NULL out or a zero capacity is a length query, not an error. */
    CHECK(plg_base_plugin_file_name(NULL, 0) == 13);
    CHECK(plg_base_plugin_file_name(name, 0) == 13);
    CHECK(name[0] == 'O'); /* untouched */

    /* Truncation is reported by the return value, and the copy stays in bounds. */
    memset(name, 'Z', sizeof(name));
    CHECK(plg_base_plugin_file_name(name, 5) == 13);
    CHECK(strcmp(name, "Obli") == 0);

    CHECK(plg_version() != NULL);
    CHECK(strncmp(plg_version(), "plugin-ffi ", 11) == 0);
}

static void check_name_equals(void)
{
    int equal = -1;

    CHECK(plg_plugin_name_equals("Oblivion.esm", "OBLIVION.ESM", &equal) == PLG_OK);
    CHECK(equal == 1);
    CHECK(plg_plugin_name_equals("Knights.esp", "knights.ESP", &equal) == PLG_OK);
    CHECK(equal == 1);
    CHECK(plg_plugin_name_equals("Knights.esp", "Knights.ESM", &equal) == PLG_OK);
    CHECK(equal == 0);
    CHECK(plg_plugin_name_equals("", "", &equal) == PLG_OK);
    CHECK(equal == 1);

    equal = -1;
    CHECK(plg_plugin_name_equals(NULL, "a", &equal) == PLG_ERROR_INVALID_ARGUMENT);
    CHECK(equal == -1); /* out parameter untouched on failure */
    CHECK(plg_last_error_code() == PLG_ERROR_INVALID_ARGUMENT);
    CHECK(plg_last_error_message(NULL, 0) > 0);
    CHECK(plg_plugin_name_equals("a", "a", NULL) == PLG_ERROR_INVALID_ARGUMENT);

    /* A success clears the previous error. */
    CHECK(plg_plugin_name_equals("a", "a", &equal) == PLG_OK);
    CHECK(plg_last_error_code() == PLG_OK);
    CHECK(plg_last_error_message(NULL, 0) == 1); /* just the NUL */
}

static void check_build_map(void)
{
    uint32_t map[4];
    size_t len = 0;

    /* Length query, then a short buffer, then the real call. */
    CHECK(plg_build_formid_index_map(g_masters, 2, g_loaded, 3, 3, NULL, 0, &len) == PLG_OK);
    CHECK(len == 3);
    len = 0;
    CHECK(plg_build_formid_index_map(g_masters, 2, g_loaded, 3, 3, map, 0, &len) == PLG_OK);
    CHECK(len == 3);
    len = 0;
    CHECK(plg_build_formid_index_map(g_masters, 2, g_loaded, 3, 3, map, 2, &len) ==
          PLG_ERROR_BUFFER_TOO_SMALL);
    CHECK(len == 3); /* the required count, so a caller can retry */

    len = 0;
    CHECK(plg_build_formid_index_map(g_masters, 2, g_loaded, 3, 3, map, 4, &len) == PLG_OK);
    CHECK(len == 3);
    CHECK(map[0] == 0);
    CHECK(map[1] == 2);
    CHECK(map[2] == 3);

    /* An empty MAST list still gets the self entry, and NULL arrays are fine. */
    len = 0;
    CHECK(plg_build_formid_index_map(NULL, 0, NULL, 0, 0, map, 4, &len) == PLG_OK);
    CHECK(len == 1);
    CHECK(map[0] == 0);

    /* A NULL element inside a non-empty array is rejected, out untouched. */
    {
        const char *bad[] = {"Oblivion.esm", NULL};
        size_t untouched = 123;
        map[0] = 0xDEADBEEF;
        CHECK(plg_build_formid_index_map(bad, 2, g_loaded, 3, 3, map, 4, &untouched) ==
              PLG_ERROR_INVALID_ARGUMENT);
        CHECK(untouched == 123);
        CHECK(map[0] == 0xDEADBEEF);
    }
    CHECK(plg_build_formid_index_map(NULL, 1, g_loaded, 3, 3, map, 4, NULL) ==
          PLG_ERROR_INVALID_ARGUMENT);

    /* out_len may be NULL when the count is not wanted. */
    map[0] = 0xDEADBEEF;
    map[3] = 0xDEADBEEF;
    CHECK(plg_build_formid_index_map(g_masters, 2, g_loaded, 3, 3, map, 4, NULL) == PLG_OK);
    CHECK(map[0] == 0);
    CHECK(map[1] == 2);
    CHECK(map[2] == 3);
    CHECK(map[3] == 0xDEADBEEF); /* capacity is a ceiling, not a target */
}

static void check_rebase(void)
{
    const uint32_t map[] = {0, 2, 3};
    uint32_t out = 0;

    CHECK(plg_rebase_formid(0x0100BEEF, map, 3, &out) == PLG_OK);
    CHECK(out == 0x0200BEEF);
    CHECK(plg_rebase_formid(0x0200BEEF, map, 3, &out) == PLG_OK);
    CHECK(out == 0x0300BEEF);
    CHECK(plg_rebase_formid(0, map, 3, &out) == PLG_OK);
    CHECK(out == 0);
    CHECK(plg_rebase_formid(0xFF00BEEF, map, 3, &out) == PLG_OK);
    CHECK(out == 0xFF00BEEF); /* past the end of the map */

    /* An unresolved master leaves its FormIDs alone. */
    {
        const uint32_t unresolved[] = {0xFFFFFFFFu, 1};
        CHECK(plg_rebase_formid(0x0100ABCD, unresolved, 2, &out) == PLG_OK);
        CHECK(out == 0x0100ABCD);
    }

    /* A NULL map with a length of 0 rebases nothing. */
    CHECK(plg_rebase_formid(0x0100BEEF, NULL, 0, &out) == PLG_OK);
    CHECK(out == 0x0100BEEF);
    CHECK(plg_rebase_formid(0x0100BEEF, NULL, 1, &out) == PLG_ERROR_INVALID_ARGUMENT);
    CHECK(plg_rebase_formid(0x0100BEEF, map, 3, NULL) == PLG_ERROR_INVALID_ARGUMENT);
}

static void check_parse_plugin_list(void)
{
    /* CRLF, a BOM, a marker with padding, a comment and a plain name. The BOM is
     * spelled out so the offset arithmetic below can be checked by eye. */
    static const char text[] =
        "\xEF\xBB\xBF# comment\r\n  *  Oblivion.esm  \r\nMod.esp\n";
    const size_t text_len = sizeof(text) - 1;
    plg_plugin_list_entry entries[2];
    size_t len = 0;

    CHECK(plg_parse_plugin_list(text, text_len, NULL, 0, &len) == PLG_OK);
    CHECK(len == 2);
    len = 0;
    CHECK(plg_parse_plugin_list(text, text_len, entries, 0, &len) == PLG_OK);
    CHECK(len == 2);
    len = 0;
    CHECK(plg_parse_plugin_list(text, text_len, entries, 1, &len) ==
          PLG_ERROR_BUFFER_TOO_SMALL);
    CHECK(len == 2);
    CHECK(plg_last_error_code() == PLG_ERROR_BUFFER_TOO_SMALL);

    len = 0;
    CHECK(plg_parse_plugin_list(text, text_len, entries, 2, &len) == PLG_OK);
    CHECK(len == 2);
    CHECK(plg_last_error_code() == PLG_OK);
    /* The names stay in the caller's buffer: the entries only point at them. The
     * offsets count the 3-byte BOM, "# comment", the CRLF and the padding around
     * the marker, so the trailing blanks of a line are part of the name range. */
    CHECK(entries[0].name_offset == 19);
    CHECK(entries[0].name_length == 12);
    CHECK(entries[0].active == 1);
    CHECK(memcmp(text + entries[0].name_offset, "Oblivion.esm", 12) == 0);
    CHECK(entries[1].name_offset == 35);
    CHECK(entries[1].name_length == 7);
    CHECK(memcmp(text + entries[1].name_offset, "Mod.esp", 7) == 0);
    /* A file with a marker means unmarked entries are inactive. */
    CHECK(entries[1].active == 0);

    /* A file with no marker at all is read as "everything is active". */
    {
        static const char plain[] = "Oblivion.esm\nMod.esp\n";
        CHECK(plg_parse_plugin_list(plain, sizeof(plain) - 1, entries, 2, &len) == PLG_OK);
        CHECK(len == 2);
        CHECK(entries[0].active == 1);
        CHECK(entries[1].active == 1);
    }

    /* Empty input, and NULL contents with a length of 0, are both empty. */
    CHECK(plg_parse_plugin_list("", 0, entries, 2, &len) == PLG_OK);
    CHECK(len == 0);
    CHECK(plg_parse_plugin_list(NULL, 0, entries, 2, &len) == PLG_OK);
    CHECK(len == 0);

    /* A NULL buffer with a non-zero length is rejected, out parameters untouched. */
    {
        size_t untouched = 123;
        memset(entries, 0x5A, sizeof(entries));
        CHECK(plg_parse_plugin_list(NULL, 1, entries, 2, &untouched) ==
              PLG_ERROR_INVALID_ARGUMENT);
        CHECK(untouched == 123);
        CHECK(entries[0].name_offset == (size_t)0x5A5A5A5A5A5A5A5Au);
        CHECK(entries[0].active == 0x5A5A5A5A);
    }
    CHECK(plg_parse_plugin_list(text, text_len, entries, 2, NULL) ==
          PLG_ERROR_INVALID_ARGUMENT);
}

static void check_serialize_plugin_list(void)
{
    static const char *const names[] = {"Oblivion.esm", "Mod.esp"};
    static const int flags[] = {1, 0};
    static const char expected[] = "*Oblivion.esm\nMod.esp\n";
    char out[32];
    size_t len = 0;

    CHECK(plg_serialize_plugin_list(names, flags, 2, NULL, 0, &len) == PLG_OK);
    CHECK(len == sizeof(expected) - 1);
    len = 0;
    CHECK(plg_serialize_plugin_list(names, flags, 2, out, 0, &len) == PLG_OK);
    CHECK(len == sizeof(expected) - 1);
    CHECK(plg_serialize_plugin_list(names, flags, 2, out, sizeof(expected) - 2, &len) ==
          PLG_ERROR_BUFFER_TOO_SMALL);
    CHECK(len == sizeof(expected) - 1);

    /* Capturing just enough bytes must not NUL-terminate: the text is file data. */
    memset(out, 'Z', sizeof(out));
    len = 0;
    CHECK(plg_serialize_plugin_list(names, flags, 2, out, sizeof(expected) - 1, &len) == PLG_OK);
    CHECK(len == sizeof(expected) - 1);
    CHECK(memcmp(out, expected, sizeof(expected) - 1) == 0);
    CHECK(out[sizeof(expected) - 1] == 'Z'); /* still the sentinel, no NUL written */

    /* An empty selection is an empty file, not an error. */
    out[0] = 'Z';
    CHECK(plg_serialize_plugin_list(NULL, NULL, 0, out, sizeof(out), &len) == PLG_OK);
    CHECK(len == 0);
    CHECK(out[0] == 'Z');
    /* Empty names are skipped even when their flag says active. */
    {
        static const char *const blank[] = {"", "A.esp"};
        static const int blank_flags[] = {1, 1};
        CHECK(plg_serialize_plugin_list(blank, blank_flags, 2, out, sizeof(out), &len) == PLG_OK);
        CHECK(len == 7);
        CHECK(memcmp(out, "*A.esp\n", 7) == 0);
    }

    /* A missing flag array, a NULL name element and a NULL out_len are errors. */
    {
        static const char *const bad[] = {"Oblivion.esm", NULL};
        size_t untouched = 123;
        memset(out, 'Z', sizeof(out));
        CHECK(plg_serialize_plugin_list(names, NULL, 2, out, sizeof(out), &len) ==
              PLG_ERROR_INVALID_ARGUMENT);
        CHECK(plg_serialize_plugin_list(bad, flags, 2, out, sizeof(out), &untouched) ==
              PLG_ERROR_INVALID_ARGUMENT);
        CHECK(untouched == 123);
        CHECK(plg_serialize_plugin_list(names, flags, 2, out, sizeof(out), NULL) ==
              PLG_ERROR_INVALID_ARGUMENT);
    }
}

static void check_active_plugin_indices(void)
{
    static const char *const names[] = {"Oblivion.esm", "Mod.esp", "Knights.esp"};
    static const int flags[] = {1, 0, 1};
    size_t out[3];
    size_t len = 0;

    CHECK(plg_active_plugin_indices(names, flags, 3, NULL, 0, &len) == PLG_OK);
    CHECK(len == 2);
    out[0] = 99;
    CHECK(plg_active_plugin_indices(names, flags, 3, out, 1, &len) == PLG_ERROR_BUFFER_TOO_SMALL);
    CHECK(len == 2);
    CHECK(out[0] == 99); /* nothing written on a short buffer */
    CHECK(plg_active_plugin_indices(names, flags, 3, out, 3, &len) == PLG_OK);
    CHECK(len == 2);
    CHECK(out[0] == 0);
    CHECK(out[1] == 2);

    /* A flag array that marks everything active selects every named entry. */
    {
        static const char *const two[] = {"A.esp", "B.esp"};
        static const int all_active[] = {1, 1};
        CHECK(plg_active_plugin_indices(two, all_active, 2, out, 3, &len) == PLG_OK);
        CHECK(len == 2);
        CHECK(out[0] == 0 && out[1] == 1);
    }

    CHECK(plg_active_plugin_indices(NULL, NULL, 0, out, 3, &len) == PLG_OK);
    CHECK(len == 0);
    CHECK(plg_active_plugin_indices(names, NULL, 3, out, 3, &len) ==
          PLG_ERROR_INVALID_ARGUMENT);
    CHECK(plg_active_plugin_indices(NULL, flags, 3, out, 3, &len) ==
          PLG_ERROR_INVALID_ARGUMENT);
    CHECK(plg_active_plugin_indices(names, flags, 3, out, 3, NULL) ==
          PLG_ERROR_INVALID_ARGUMENT);
}

static void check_sort_plugin_load_order(void)
{
    static const char *const names[] = {
        "Mod.esp", "Oblivion.esm", "Knights.esp", "DLCBattlehornCastle.esm"};
    size_t out[4];
    size_t len = 0;

    CHECK(plg_sort_plugin_load_order(names, 4, NULL, 0, &len) == PLG_OK);
    CHECK(len == 4);
    out[0] = 99;
    CHECK(plg_sort_plugin_load_order(names, 4, out, 3, &len) == PLG_ERROR_BUFFER_TOO_SMALL);
    CHECK(len == 4);
    CHECK(out[0] == 99);

    CHECK(plg_sort_plugin_load_order(names, 4, out, 4, &len) == PLG_OK);
    CHECK(len == 4);
    CHECK(out[0] == 1); /* Oblivion.esm */
    CHECK(out[1] == 3); /* DLCBattlehornCastle.esm, the other master */
    CHECK(out[2] == 2); /* Knights.esp before Mod.esp */
    CHECK(out[3] == 0);

    CHECK(plg_sort_plugin_load_order(NULL, 0, out, 4, &len) == PLG_OK);
    CHECK(len == 0);
    CHECK(plg_sort_plugin_load_order(NULL, 1, out, 4, &len) == PLG_ERROR_INVALID_ARGUMENT);
    CHECK(plg_sort_plugin_load_order(names, 4, out, 4, NULL) == PLG_ERROR_INVALID_ARGUMENT);
}

static void check_is_master_file_name(void)
{
    int is_master = -1;

    CHECK(plg_is_master_file_name("Oblivion.esm", &is_master) == PLG_OK);
    CHECK(is_master == 1);
    CHECK(plg_is_master_file_name("Knights.ESM", &is_master) == PLG_OK);
    CHECK(is_master == 1);
    CHECK(plg_is_master_file_name("Mod.esp", &is_master) == PLG_OK);
    CHECK(is_master == 0);
    CHECK(plg_is_master_file_name("Mod.esm.bak", &is_master) == PLG_OK);
    CHECK(is_master == 0);
    CHECK(plg_is_master_file_name("esm", &is_master) == PLG_OK);
    CHECK(is_master == 0);
    CHECK(plg_is_master_file_name("", &is_master) == PLG_OK);
    CHECK(is_master == 0);

    is_master = -1;
    CHECK(plg_is_master_file_name(NULL, &is_master) == PLG_ERROR_INVALID_ARGUMENT);
    CHECK(is_master == -1); /* out parameter untouched on failure */
    CHECK(plg_is_master_file_name("Oblivion.esm", NULL) == PLG_ERROR_INVALID_ARGUMENT);
}

int main(void)
{
    check_constants();
    check_name_equals();
    check_build_map();
    check_rebase();
    check_parse_plugin_list();
    check_serialize_plugin_list();
    check_active_plugin_indices();
    check_sort_plugin_load_order();
    check_is_master_file_name();

    if (failures != 0) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }
    printf("plugin_ffi C ABI check: all checks passed\n");
    return 0;
}
