/* C ABI over the `bsa-core` crate.
 *
 * This interface owns state, unlike the other FFI crates in this workspace: a
 * BSA archive holds an open file handle, the parsed index, and a reusable
 * extraction buffer. The C side works with an opaque handle created by
 * bsa_create() and released by bsa_destroy(), and every other call takes that
 * handle as its first argument.
 *
 * A handle must be used on one thread at a time.
 *
 * Paths to look up cross the boundary as raw bytes plus a length, because archive
 * names are not guaranteed to be valid UTF-8 and a caller may hand over a path
 * with embedded NULs. Only bsa_open() takes a NUL-terminated string, because it
 * names a file on the device.
 *
 * Extraction returns a pointer into the archive's own buffer rather than filling
 * a caller buffer, so copy what you need before the next extract call on the same
 * handle. Search results are the same: a pointer into the archive's scratch list,
 * valid until the next search on that handle.
 *
 * Threading: each thread has its own last-error state, so bsa_last_error_code and
 * bsa_last_error_message only describe calls made on the calling thread.
 *
 * Strings copied into caller buffers follow the snprintf convention: the return
 * value is the length in bytes including the terminating NUL, so a caller detects
 * truncation by comparing it against the buffer capacity. Nothing is written past
 * `capacity` bytes, and a NULL `out` or a zero `capacity` queries the required
 * length without writing anything.
 */

#ifndef BSA_FFI_H
#define BSA_FFI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The call succeeded. */
#define BSA_OK 0
/* The call found what it was looking for. */
#define BSA_FOUND 1
/* The call succeeded but found nothing. */
#define BSA_NOT_FOUND 0
/* A required pointer argument was NULL, or a pointer was misaligned. */
#define BSA_ERROR_INVALID_ARGUMENT (-1)
/* The archive file could not be opened or read. */
#define BSA_ERROR_IO (-2)
/* The first four bytes are not "BSA\0". */
#define BSA_ERROR_BAD_MAGIC (-3)
/* A structure the header points at runs past the end of the file. */
#define BSA_ERROR_TRUNCATED (-4)
/* An entry's data could not be inflated. */
#define BSA_ERROR_DECOMPRESS (-5)
/* The requested entry index does not exist. */
#define BSA_ERROR_INDEX_OUT_OF_RANGE (-6)
/* A Rust panic was caught at the boundary; the call had no effect. */
#define BSA_ERROR_PANIC (-7)
/* An output buffer cannot hold the result; the out-parameter holds the size. */
#define BSA_ERROR_BUFFER_TOO_SMALL (-8)
/* The archive is not open. */
#define BSA_ERROR_NOT_OPEN (-9)
/* The header asks for more structure than this reader accepts. */
#define BSA_ERROR_CORRUPT (-10)

/* Field selector for bsa_entry_text(). */
#define BSA_FIELD_FULL_PATH 0
/* Field selector for bsa_entry_text(). */
#define BSA_FIELD_LOOKUP_PATH 1
/* Field selector for bsa_entry_text(). */
#define BSA_FIELD_FILE_NAME 2
/* Field selector for bsa_entry_text(). */
#define BSA_FIELD_EXTENSION 3
/* Field selector for bsa_entry_text(). */
#define BSA_FIELD_FOLDER_PATH 4

/* The nine header fields, in the order the archive stores them. */
typedef struct bsa_header {
    uint32_t magic;
    uint32_t version;
    uint32_t folder_record_offset;
    uint32_t archive_flags;
    uint32_t folder_count;
    uint32_t file_count;
    uint32_t folder_name_length;
    uint32_t file_name_length;
    uint32_t file_flags;
} bsa_header;

/* The per-entry numbers, without the strings. Strings come from bsa_entry_text(). */
typedef struct bsa_entry_info {
    uint64_t hash;
    uint32_t offset;
    uint32_t size;
    uint32_t real_size;
    /* Non-zero when the entry is stored compressed. */
    uint32_t compressed;
} bsa_entry_info;

/* A static, NUL-terminated version banner, for logs and crash reports. */
const char *bsa_version(void);

/* The error code left by the last call on this thread. */
int bsa_last_error_code(void);
/* The message left by the last call on this thread (snprintf style). */
int bsa_last_error_message(char *out, size_t capacity);

/* Creates a closed archive and returns its handle, or NULL if none could be
 * allocated. */
void *bsa_create(void);
/* Closes the archive and frees the handle. Passing the same handle twice is a
 * caller bug the handle's magic marker turns into a no-op. */
void bsa_destroy(void *handle);

/* Opens `path` and parses its index. A failed open still closes whatever the
 * handle held before, and leaves the archive closed, so a caller can react to the
 * code and try another path. `path` must be a NUL-terminated UTF-8 string. */
int bsa_open(void *handle, const char *path);
/* Closes the archive, dropping the file handle and the parsed index. */
int bsa_close(void *handle);
/* 1 when the archive holds an open file, 0 when it does not, negative on error. */
int bsa_is_open(void *handle);
/* 1 when the index finished parsing, 0 when it did not, negative on error. */
int bsa_is_parsed(void *handle);

/* Copies the header into `out`. */
int bsa_get_header(void *handle, bsa_header *out);
/* The file count the header declares. */
uint32_t bsa_file_count(void *handle);
/* The number of entries the parse produced. */
size_t bsa_entry_count(void *handle);

/* Copies up to `capacity` entry records into `out` and returns how many the
 * archive holds in total. A NULL `out` or a zero `capacity` only reports the
 * count, so a caller can size its array first. Returns 0 on error. */
size_t bsa_get_files(void *handle, bsa_entry_info *out, size_t capacity);

/* Copies one of an entry's strings into `out` (snprintf style). `field` is one of
 * the BSA_FIELD_* constants. An index past the end or an unknown field is a
 * BSA_ERROR_INVALID_ARGUMENT. */
int bsa_entry_text(void *handle, size_t index, int field, char *out, size_t capacity);

/* Finds an entry by exact path, accepting either separator and any case.
 *
 * Writes the entry index to `out_index` and returns BSA_FOUND, or returns
 * BSA_NOT_FOUND without touching `out_index`. A hash collision is rejected by
 * comparing the full path, as the engine does, so a shadowed entry is not found.
 *
 * `path` points to `path_length` bytes and may carry embedded NULs. `out_index`
 * must be non-NULL. */
int bsa_find_file(void *handle, const char *path, size_t path_length, size_t *out_index);

/* Finds every entry whose lookup path starts with `prefix`.
 *
 * Writes the scratch list to `out_indices` and its length to `out_count`, then
 * returns how many entries matched. The list stays valid until the next search on
 * this handle, and extraction does not disturb it. Both out-parameters must be
 * non-NULL. */
int bsa_find_files_by_prefix(void *handle, const char *prefix, size_t prefix_length,
                             const size_t **out_indices, size_t *out_count);

/* Finds every entry whose extension equals `extension`, ignoring ASCII case. The
 * extension is written without its dot. Behaves like bsa_find_files_by_prefix()
 * otherwise. */
int bsa_find_files_by_extension(void *handle, const char *extension, size_t extension_length,
                                const size_t **out_indices, size_t *out_count);

/* Points `out_data` and `out_length` at an entry's stored bytes. Both
 * out-parameters must be non-NULL. */
int bsa_extract_raw(void *handle, size_t index, const uint8_t **out_data, size_t *out_length);

/* Points `out_data` and `out_length` at an entry's bytes, inflating them when the
 * entry is compressed. Both out-parameters must be non-NULL. */
int bsa_extract_decompressed(void *handle, size_t index, const uint8_t **out_data,
                             size_t *out_length);

#ifdef __cplusplus
}
#endif

#endif /* BSA_FFI_H */
