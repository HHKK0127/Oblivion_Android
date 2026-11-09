/* c_abi_check.c - compiles bsa_ffi.h as C11 and exercises the ABI edges that the
 * C++ unit tests in Oblivion_Android cannot reach: NULL arguments, the handle
 * lifecycle, the length queries, and short buffers.
 *
 * The archive-backed half of the test runs only when BSA_TEST_ARCHIVE names a real
 * .bsa file, because a checkout has no archive to read.
 *
 * Build and run (mingw-w64, from the crate root, with
 * CARGO_TARGET_DIR=<repo>/target/gnu):
 *   gcc -std=c11 -Wall -Wextra -I include tests/c_abi_check.c \
 *       ../target/gnu/release/libbsa_ffi.a \
 *       -lws2_32 -lbcrypt -luserenv -lntdll -lole32 -loleaut32 -ladvapi32 \
 *       -luser32 -lshell32 -lkernel32 -o c_abi_check.exe
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsa_ffi.h"

static int failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            ++failures;                                                          \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
        }                                                                        \
    } while (0)

#define NOT_AN_ARCHIVE "c_abi_check_not_an_archive.tmp"
#define TRUNCATED_ARCHIVE "c_abi_check_truncated.tmp"

/* Write `length` bytes to `path`, so the failure paths have a file to read. */
static void write_file(const char *path, const void *bytes, size_t length)
{
    FILE *file = fopen(path, "wb");

    if (file == NULL) {
        return;
    }
    if (length > 0) {
        fwrite(bytes, 1, length, file);
    }
    fclose(file);
}

static void check_basics(void)
{
    char banner[64];
    char message[128];
    bsa_header header;
    size_t index = 0;
    size_t count = 0;
    const size_t *indices = NULL;
    const uint8_t *bytes = NULL;

    CHECK(BSA_OK == 0);
    CHECK(BSA_FOUND == 1);
    CHECK(BSA_NOT_FOUND == 0);
    CHECK(bsa_version() != NULL);
    CHECK(strncmp(bsa_version(), "bsa-ffi ", 8) == 0);

    /* Every call takes a handle, so a NULL one is reported rather than crashing. */
    CHECK(bsa_open(NULL, "x") == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_last_error_code() == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_last_error_message(message, sizeof(message)) > 0);
    CHECK(message[0] != '\0');
    CHECK(bsa_close(NULL) == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_is_open(NULL) == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_is_parsed(NULL) == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_get_header(NULL, &header) == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_file_count(NULL) == 0);
    CHECK(bsa_entry_count(NULL) == 0);
    CHECK(bsa_get_files(NULL, NULL, 0) == 0);
    CHECK(bsa_entry_text(NULL, 0, BSA_FIELD_FULL_PATH, message, sizeof(message)) ==
          BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_find_file(NULL, "a", 1, &index) == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_find_files_by_prefix(NULL, "a", 1, &indices, &count) == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_find_files_by_extension(NULL, "a", 1, &indices, &count) == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_extract_raw(NULL, 0, &bytes, &count) == BSA_ERROR_INVALID_ARGUMENT);

    /* Snprintf-style reporting: a NULL out and a zero capacity are length queries,
     * and a one-byte buffer holds only the terminator. */
    CHECK(bsa_last_error_message(NULL, 0) > 0);
    CHECK(bsa_last_error_message(message, 1) > 0);
    CHECK(message[0] == '\0');
    CHECK(bsa_last_error_message(banner, sizeof(banner)) > 0);
}

static void check_lifecycle(void)
{
    void *handle = bsa_create();
    char message[128];
    size_t index = 0;
    size_t count = 0;
    const size_t *indices = NULL;
    bsa_entry_info entries[4];

    CHECK(handle != NULL);
    if (handle == NULL) {
        return;
    }

    /* A fresh handle is closed but usable. */
    CHECK(bsa_is_open(handle) == 0);
    CHECK(bsa_is_parsed(handle) == 0);
    CHECK(bsa_file_count(handle) == 0);
    CHECK(bsa_entry_count(handle) == 0);
    CHECK(bsa_close(handle) == BSA_OK);

    /* A path that cannot be opened is an I/O error, and the archive stays closed. */
    CHECK(bsa_open(handle, "c_abi_check_definitely_missing.bsa") == BSA_ERROR_IO);
    CHECK(bsa_last_error_code() == BSA_ERROR_IO);
    CHECK(bsa_last_error_message(message, sizeof(message)) > 0);
    CHECK(bsa_is_open(handle) == 0);
    CHECK(bsa_is_parsed(handle) == 0);

    /* Nothing is indexed, so the entry surface is empty rather than broken. */
    CHECK(bsa_get_files(handle, NULL, 0) == 0);
    CHECK(bsa_get_files(handle, entries, 4) == 0);
    CHECK(bsa_entry_count(handle) == 0);

    /* A lookup against an empty index is a miss, not an error. */
    CHECK(bsa_find_file(handle, "a.nif", 5, &index) == BSA_NOT_FOUND);
    CHECK(bsa_find_files_by_prefix(handle, "a", 1, &indices, &count) == 0);
    CHECK(count == 0);
    CHECK(bsa_find_files_by_extension(handle, "nif", 3, &indices, &count) == 0);
    CHECK(count == 0);

    /* An entry that does not exist is out of range, not a crash. */
    CHECK(bsa_entry_text(handle, 0, BSA_FIELD_FULL_PATH, message, sizeof(message)) ==
          BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_extract_raw(handle, 0, NULL, &count) == BSA_ERROR_INVALID_ARGUMENT);

    bsa_destroy(handle);
}

static void check_bad_files(void)
{
    /* A whole header's worth of bytes so the magic check runs before the length
     * check, which is the order the reader uses. */
    unsigned char not_an_archive[64];
    static const char truncated[] = {'B', 'S', 'A', '\0'};
    void *handle;
    char message[128];
    bsa_header header;
    size_t count = 0;
    const uint8_t *bytes = NULL;

    memset(not_an_archive, 0, sizeof(not_an_archive));
    memcpy(not_an_archive, "NOPE", 4);
    write_file(NOT_AN_ARCHIVE, not_an_archive, sizeof(not_an_archive));
    write_file(TRUNCATED_ARCHIVE, truncated, sizeof(truncated));

    handle = bsa_create();
    CHECK(handle != NULL);
    if (handle == NULL) {
        remove(NOT_AN_ARCHIVE);
        remove(TRUNCATED_ARCHIVE);
        return;
    }

    CHECK(bsa_open(handle, NOT_AN_ARCHIVE) == BSA_ERROR_BAD_MAGIC);
    CHECK(bsa_last_error_code() == BSA_ERROR_BAD_MAGIC);
    CHECK(bsa_is_open(handle) == 0);
    CHECK(bsa_get_header(handle, &header) == BSA_OK);
    CHECK(header.magic == 0); /* cleared by the failed open */

    /* A correct magic with no header behind it is a truncated file. */
    CHECK(bsa_open(handle, TRUNCATED_ARCHIVE) == BSA_ERROR_TRUNCATED);
    CHECK(bsa_last_error_code() == BSA_ERROR_TRUNCATED);
    CHECK(bsa_is_open(handle) == 0);
    CHECK(bsa_last_error_message(message, sizeof(message)) > 0);

    /* A closed handle has nothing to extract, and says so instead of reading. */
    CHECK(bsa_extract_raw(handle, 0, &bytes, &count) == BSA_ERROR_NOT_OPEN);
    CHECK(bsa_extract_decompressed(handle, 0, &bytes, &count) == BSA_ERROR_NOT_OPEN);
    CHECK(bsa_last_error_code() == BSA_ERROR_NOT_OPEN);

    bsa_destroy(handle);
    remove(NOT_AN_ARCHIVE);
    remove(TRUNCATED_ARCHIVE);
}

static void check_real_archive(const char *path)
{
    void *handle = bsa_create();
    bsa_header header;
    bsa_entry_info entries[8];
    bsa_entry_info probe;
    char text[256];
    char lookup[256];
    char spelling[256];
    char extension[32];
    size_t total = 0;
    size_t index = 0;
    size_t count = 0;
    size_t length = 0;
    size_t i;
    const size_t *indices = NULL;
    const uint8_t *bytes = NULL;

    CHECK(handle != NULL);
    if (handle == NULL) {
        return;
    }
    CHECK(bsa_open(handle, path) == BSA_OK);
    if (bsa_last_error_code() != BSA_OK) {
        bsa_destroy(handle);
        return;
    }
    CHECK(bsa_is_open(handle) == 1);
    CHECK(bsa_is_parsed(handle) == 1);

    CHECK(bsa_get_header(handle, &header) == BSA_OK);
    CHECK(header.magic == 0x00415342u);
    CHECK(header.folder_count > 0);
    CHECK(header.file_count > 0);
    CHECK(bsa_file_count(handle) == header.file_count);

    /* A NULL out or a zero capacity only reports the count. */
    total = bsa_get_files(handle, NULL, 0);
    CHECK(total > 0);
    CHECK(bsa_entry_count(handle) == total);
    CHECK(total == header.file_count);

    memset(entries, 0, sizeof(entries));
    CHECK(bsa_get_files(handle, entries, 8) == total);
    CHECK(entries[0].offset > 0);
    CHECK(entries[0].size > 0);

    /* Every entry's strings are readable, and the lookup path is lowercase with
     * '/' separators, which is what a caller can compare against. */
    for (i = 0; i < 8 && i < total; ++i) {
        int needed;
        const char *p;

        needed = bsa_entry_text(handle, i, BSA_FIELD_LOOKUP_PATH, NULL, 0);
        CHECK(needed > 1);
        CHECK((size_t)needed <= sizeof(lookup));
        CHECK(bsa_entry_text(handle, i, BSA_FIELD_LOOKUP_PATH, lookup, sizeof(lookup)) == needed);
        CHECK(strlen(lookup) + 1 == (size_t)needed);
        for (p = lookup; *p != '\0'; ++p) {
            CHECK(*p != '\\');
            CHECK(!(*p >= 'A' && *p <= 'Z'));
        }

        CHECK(bsa_entry_text(handle, i, BSA_FIELD_FULL_PATH, text, sizeof(text)) > 1);
        CHECK(bsa_entry_text(handle, i, BSA_FIELD_FILE_NAME, text, sizeof(text)) > 1);
        CHECK(bsa_entry_text(handle, i, BSA_FIELD_EXTENSION, text, sizeof(text)) >= 0);
        CHECK(bsa_entry_text(handle, i, BSA_FIELD_FOLDER_PATH, text, sizeof(text)) >= 0);

        /* Truncation is reported by the return value and stays in bounds. */
        memset(text, 'Z', sizeof(text));
        CHECK(bsa_entry_text(handle, i, BSA_FIELD_LOOKUP_PATH, text, 4) == needed);
        CHECK(strlen(text) == 3);
    }

    /* An unknown field selector is a caller bug, not a silent miss. */
    CHECK(bsa_entry_text(handle, 0, 99, text, sizeof(text)) == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_entry_text(handle, total, BSA_FIELD_FULL_PATH, text, sizeof(text)) ==
          BSA_ERROR_INVALID_ARGUMENT);

    /* The round trip a caller actually makes: take an entry's own lookup path and
     * find it again. */
    CHECK(bsa_entry_text(handle, 0, BSA_FIELD_LOOKUP_PATH, lookup, sizeof(lookup)) > 1);
    CHECK(bsa_find_file(handle, lookup, strlen(lookup), &index) == BSA_FOUND);
    CHECK(index == 0);
    CHECK(bsa_find_file(handle, "no/such/file.xyz", 17, &index) == BSA_NOT_FOUND);
    CHECK(bsa_find_file(handle, NULL, 0, &index) == BSA_NOT_FOUND);

    /* The same path with engine separators still resolves. */
    CHECK(bsa_entry_text(handle, 0, BSA_FIELD_FULL_PATH, spelling, sizeof(spelling)) > 1);
    CHECK(bsa_find_file(handle, spelling, strlen(spelling), &index) == BSA_FOUND);
    CHECK(index == 0);

    /* Extension search is exact and ignores case; the extension carries no dot. */
    CHECK(bsa_entry_text(handle, 0, BSA_FIELD_EXTENSION, extension, sizeof(extension)) >= 0);
    if (extension[0] != '\0') {
        CHECK(bsa_find_files_by_extension(handle, extension, strlen(extension), &indices, &count) >
              0);
        CHECK(count > 0);
        CHECK(indices != NULL);
        CHECK(indices[0] < total);
    }
    CHECK(bsa_find_files_by_extension(handle, "no-such-extension", 17, &indices, &count) == 0);
    CHECK(count == 0);

    /* A prefix search over a folder that the archive actually contains. */
    CHECK(bsa_entry_text(handle, 0, BSA_FIELD_FOLDER_PATH, text, sizeof(text)) >= 0);
    if (text[0] != '\0') {
        CHECK(bsa_find_files_by_prefix(handle, text, strlen(text), &indices, &count) > 0);
        CHECK(count > 0);
        CHECK(indices[0] < total);
    }

    /* Extraction reports the bytes for both a stored and an inflated entry, and
     * the raw size is what the entry record says. */
    probe = entries[0];
    CHECK(bsa_extract_raw(handle, 0, &bytes, &length) == BSA_OK);
    CHECK(bytes != NULL || length == 0);
    CHECK(length == probe.size);

    if (probe.compressed) {
        CHECK(bsa_extract_decompressed(handle, 0, &bytes, &length) == BSA_OK);
        CHECK(bytes != NULL || length == 0);
        CHECK(probe.real_size == 0 || length == probe.real_size);
    }

    /* Extraction must not disturb the entries or the header. */
    CHECK(bsa_get_files(handle, entries, 8) == total);
    CHECK(entries[0].size == probe.size);
    CHECK(bsa_get_header(handle, &header) == BSA_OK);
    CHECK(header.file_count == total);

    /* Out-parameters are not optional, and an index past the end is reported. */
    CHECK(bsa_extract_raw(handle, total, &bytes, &length) == BSA_ERROR_INDEX_OUT_OF_RANGE);
    CHECK(bsa_extract_raw(handle, 0, NULL, &length) == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_extract_raw(handle, 0, &bytes, NULL) == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_find_file(handle, "a", 1, NULL) == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_find_files_by_prefix(handle, "a", 1, NULL, &count) == BSA_ERROR_INVALID_ARGUMENT);
    CHECK(bsa_find_files_by_prefix(handle, "a", 1, &indices, NULL) == BSA_ERROR_INVALID_ARGUMENT);

    /* Closing drops the index, so a later lookup is a miss on an empty archive. */
    CHECK(bsa_close(handle) == BSA_OK);
    CHECK(bsa_is_open(handle) == 0);
    CHECK(bsa_is_parsed(handle) == 0);
    CHECK(bsa_entry_count(handle) == 0);
    CHECK(bsa_get_header(handle, &header) == BSA_OK);
    CHECK(header.magic == 0);

    bsa_destroy(handle);
}

int main(void)
{
    const char *archive = getenv("BSA_TEST_ARCHIVE");

    check_basics();
    check_lifecycle();
    check_bad_files();

    if (archive != NULL && archive[0] != '\0') {
        check_real_archive(archive);
    } else {
        printf("BSA_TEST_ARCHIVE is not set; skipping the archive-backed checks\n");
    }

    if (failures == 0) {
        printf("bsa-ffi C ABI check: all assertions passed\n");
        return 0;
    }
    fprintf(stderr, "bsa-ffi C ABI check: %d assertion(s) failed\n", failures);
    return 1;
}
