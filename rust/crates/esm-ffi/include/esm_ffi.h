/*
 * esm-ffi - C ABI over the esm-parser crate.
 *
 * Indexes an Oblivion plugin (ESM/ESP) and exposes record headers, per-type
 * counts and decoded subrecords to C and C++. The Rust side never keeps a
 * pointer into caller memory longer than documented here, and every handle
 * returned is owned by the caller and must be released with the matching free
 * function.
 *
 * Threading: one handle is not thread safe. Each thread has its own last-error
 * state, so `esm_last_error_code` and `esm_last_error_message` only describe
 * calls made on the calling thread.
 *
 * Strings copied into caller buffers follow the snprintf convention: the return
 * value is the length in bytes including the terminating NUL, so a caller
 * detects truncation by comparing it against the buffer capacity. Nothing is
 * written past `capacity` bytes, and a NULL `out` or a zero `capacity` queries
 * the required length without writing anything.
 *
 * Out parameters are only written when the call succeeds. A call that fails
 * leaves the struct or buffer the caller passed untouched, so a caller can
 * distinguish "not found" from "found and zeroed" without a sentinel.
 */

#ifndef ESM_FFI_H
#define ESM_FFI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------------------
 * Status codes
 * ------------------------------------------------------------------------ */

/** The call succeeded. */
#define ESM_OK 0
/** A required pointer argument was NULL. */
#define ESM_ERROR_INVALID_ARGUMENT (-1)
/** The plugin file could not be read. */
#define ESM_ERROR_IO (-2)
/** A record, group or subrecord is truncated. */
#define ESM_ERROR_TRUNCATED (-3)
/** The buffer does not start with a TES4 record. */
#define ESM_ERROR_BAD_HEADER (-4)
/** A compressed record could not be inflated. */
#define ESM_ERROR_INFLATE (-5)
/** No record matched the requested FormID or index. */
#define ESM_ERROR_NOT_FOUND (-6)
/** A Rust panic was caught at the boundary; the call had no effect. */
#define ESM_ERROR_PANIC (-7)

/* ---------------------------------------------------------------------------
 * Opaque handles
 * ------------------------------------------------------------------------ */

/** An indexed plugin. Created by `esm_plugin_open` or `esm_plugin_parse_memory`. */
typedef struct EsmPlugin EsmPlugin;
/** Decoded subrecords of one record or of the TES4 header. */
typedef struct EsmPayload EsmPayload;
/** Per-type record counts, ordered by descending count. */
typedef struct EsmTypeCounts EsmTypeCounts;

/* ---------------------------------------------------------------------------
 * Value types
 * ------------------------------------------------------------------------ */

/**
 * One indexed record. Mirrored field for field by `EsmRecordInfo` in the Rust
 * source; the layout is identical on 32-bit and 64-bit targets.
 */
typedef struct EsmRecordInfo {
    uint32_t form_id;
    uint32_t flags;
    /**
     * Payload size as stored in the record header. When `compressed` is 1 this
     * includes the 4-byte decompressed-size prefix.
     */
    uint32_t data_size;
    /** Decompressed payload size, or 0 when the record is not compressed. */
    uint32_t decomp_size;
    /**
     * FormID of the nearest enclosing cell-children group (type 6). This is how
     * LAND, PGRD, REFR, ACHR and ACRE records are attributed to a cell.
     */
    uint32_t parent_form_id;
    /**
     * FormID of the nearest enclosing world-children group (type 1), i.e. the
     * worldspace of an exterior CELL. 0 for interior content.
     */
    uint32_t worldspace_form_id;
    /** Offset of the payload, i.e. just past the 20-byte record header. */
    uint64_t file_offset;
    /** 1 when the payload is a zlib stream, otherwise 0. */
    uint32_t compressed;
    /** Record type tag, NUL padded only when shorter than 4 characters. */
    char rec_type[4];
} EsmRecordInfo;

/**
 * A borrowed view of one subrecord. `data` points into the owning `EsmPayload`
 * and stays valid until that payload is freed, which also invalidates the view.
 */
typedef struct EsmSubRecordView {
    /** 4-character tag, NUL padded only when shorter than 4 characters. */
    char tag[4];
    uint32_t size;
    /** Never NULL, but a zero-sized subrecord has nothing to read at it. */
    const uint8_t *data;
} EsmSubRecordView;

/* Compile-time ABI checks against the Rust definitions. */
#if defined(__cplusplus)
#if __cplusplus >= 201103L
#define ESM_FFI_STATIC_ASSERT(cond, name) static_assert((cond), #name)
#else
#define ESM_FFI_STATIC_ASSERT(cond, name) typedef char name[(cond) ? 1 : -1]
#endif
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define ESM_FFI_STATIC_ASSERT(cond, name) _Static_assert((cond), #name)
#else
#define ESM_FFI_STATIC_ASSERT(cond, name) typedef char name[(cond) ? 1 : -1]
#endif

ESM_FFI_STATIC_ASSERT(sizeof(EsmRecordInfo) == 40, esm_ffi_record_info_size);
#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFull
ESM_FFI_STATIC_ASSERT(sizeof(EsmSubRecordView) == 16, esm_ffi_subrecord_view_size);
#else
ESM_FFI_STATIC_ASSERT(sizeof(EsmSubRecordView) == 12, esm_ffi_subrecord_view_size);
#endif

/* ---------------------------------------------------------------------------
 * Lifecycle
 * ------------------------------------------------------------------------ */

/**
 * Read and index a plugin from disk. The handle owns the file bytes.
 *
 * `path` must be a NUL-terminated UTF-8 path, or NULL. Returns NULL on failure
 * and sets the last error.
 */
EsmPlugin *esm_plugin_open(const char *path);

/**
 * Index a plugin the caller already holds in memory.
 *
 * `name` is the file name reported by `esm_plugin_file_name`; it may be NULL.
 * `data` must point to `len` readable bytes that stay valid and unmodified
 * until the returned handle is freed. Returns NULL on failure and sets the last
 * error.
 */
EsmPlugin *esm_plugin_parse_memory(const char *name, const uint8_t *data, size_t len);

/** Release a plugin handle. Passing NULL is a no-op. */
void esm_plugin_free(EsmPlugin *handle);

/** Release a payload handle. Passing NULL is a no-op. */
void esm_payload_free(EsmPayload *payload);

/** Release a type-count handle. Passing NULL is a no-op. */
void esm_type_counts_free(EsmTypeCounts *counts);

/* ---------------------------------------------------------------------------
 * Plugin metadata
 * ------------------------------------------------------------------------ */

/** Number of indexed records, excluding the TES4 header. 0 for NULL. */
size_t esm_plugin_record_count(const EsmPlugin *handle);

/** Number of GRUP headers traversed. 0 for NULL. */
size_t esm_plugin_group_count(const EsmPlugin *handle);

/** Number of records whose payload is zlib compressed. 0 for NULL. */
size_t esm_plugin_compressed_count(const EsmPlugin *handle);

/** 1 when the plugin is a master (`.esm`), 0 otherwise or for NULL. */
int esm_plugin_is_master(const EsmPlugin *handle);

/** Size of the TES4 header payload in bytes. 0 for NULL. */
uint32_t esm_plugin_header_size(const EsmPlugin *handle);

/**
 * Copy the plugin file name into `out`. Returns the snprintf-style length, or
 * -1 for a NULL handle.
 */
int esm_plugin_file_name(const EsmPlugin *handle, char *out, size_t capacity);

/** Number of masters listed in the TES4 header. 0 for NULL. */
size_t esm_plugin_master_count(const EsmPlugin *handle);

/**
 * Copy master `index` into `out`. Returns the snprintf-style length, or -1 when
 * the handle is NULL or the index is out of range.
 */
int esm_plugin_master_at(const EsmPlugin *handle, size_t index, char *out, size_t capacity);

/* ---------------------------------------------------------------------------
 * Record index
 * ------------------------------------------------------------------------ */

/**
 * Copy the info of the record at `index`, in file order, into `out`. Returns
 * ESM_OK on success, ESM_ERROR_NOT_FOUND when the index is out of range.
 */
int esm_plugin_record_info_at(const EsmPlugin *handle, size_t index, EsmRecordInfo *out);

/**
 * Copy the info of the record carrying `form_id` into `out`. Returns ESM_OK on
 * success, ESM_ERROR_NOT_FOUND when no record has that FormID.
 */
int esm_plugin_find_record(const EsmPlugin *handle, uint32_t form_id, EsmRecordInfo *out);

/** Compute the per-type record counts. Free with `esm_type_counts_free`. */
EsmTypeCounts *esm_plugin_type_counts(const EsmPlugin *handle);

/** Number of distinct record types. 0 for NULL. */
size_t esm_type_counts_len(const EsmTypeCounts *counts);

/**
 * Write type `index` into `out_type`, which must have room for at least 5 bytes
 * (4-character tag plus NUL), and its count into `out_count` when that is not
 * NULL. Returns ESM_OK on success, ESM_ERROR_NOT_FOUND when the index is out of
 * range.
 */
int esm_type_counts_at(const EsmTypeCounts *counts, size_t index, char *out_type, size_t *out_count);

/* ---------------------------------------------------------------------------
 * Payload decoding
 * ------------------------------------------------------------------------ */

/**
 * Decode the subrecords of the record carrying `form_id`. Returns NULL when no
 * record has that FormID or when the payload cannot be decoded, and sets the
 * last error.
 */
EsmPayload *esm_plugin_decode_record(const EsmPlugin *handle, uint32_t form_id);

/**
 * Decode the subrecords of the record at `index`. Returns NULL when the index
 * is out of range or the payload cannot be decoded, and sets the last error.
 */
EsmPayload *esm_plugin_decode_record_at(const EsmPlugin *handle, size_t index);

/** Decode the TES4 header subrecords, i.e. HEDR, CNAM, MAST and friends. */
EsmPayload *esm_plugin_decode_header(const EsmPlugin *handle);

/* ---------------------------------------------------------------------------
 * Subrecord access
 * ------------------------------------------------------------------------ */

/** Number of subrecords in the payload. 0 for NULL. */
size_t esm_payload_subrecord_count(const EsmPayload *payload);

/**
 * Fill `out` with a view of subrecord `index`. Returns ESM_OK on success,
 * ESM_ERROR_NOT_FOUND when the index is out of range.
 */
int esm_payload_subrecord_at(const EsmPayload *payload, size_t index, EsmSubRecordView *out);

/**
 * Fill `out` with a view of the first subrecord tagged `tag`. `tag` must point
 * to at least 4 readable bytes. Returns ESM_OK on success, ESM_ERROR_NOT_FOUND
 * when no subrecord carries that tag.
 */
int esm_payload_find_subrecord(const EsmPayload *payload, const char *tag, EsmSubRecordView *out);

/* The accessors below read subrecord `index` and return 0 (or 0.0) when the
 * subrecord is missing or shorter than the requested width. */

/** Little-endian uint16_t. */
uint16_t esm_payload_u16(const EsmPayload *payload, size_t index);
/** Little-endian uint32_t. FormIDs are read with this too. */
uint32_t esm_payload_u32(const EsmPayload *payload, size_t index);
/** Little-endian int32_t. */
int32_t esm_payload_i32(const EsmPayload *payload, size_t index);
/** Little-endian float. */
float esm_payload_f32(const EsmPayload *payload, size_t index);

/**
 * Read subrecord `index` as a float, falling back to an integer interpretation
 * when the value is not a plausible float. Oblivion stores integer-only
 * magnitudes as denormal floats, so callers must use this for magnitudes.
 */
float esm_payload_float_or_int(const EsmPayload *payload, size_t index);

/**
 * Copy subrecord `index` as a NUL-terminated string.
 *
 * Text is Windows-1252 in the retail plugins; only ASCII survives the UTF-8
 * conversion, so read the subrecord view directly for byte-exact text. Returns
 * the snprintf-style length, or -1 when the index is out of range.
 */
int esm_payload_zstring(const EsmPayload *payload, size_t index, char *out, size_t capacity);

/* ---------------------------------------------------------------------------
 * Diagnostics
 * ------------------------------------------------------------------------ */

/** Error code of the last call on this thread; ESM_OK when it succeeded. */
int esm_last_error_code(void);

/** Copy the message of the last error on this thread. */
int esm_last_error_message(char *out, size_t capacity);

/** Size of `EsmRecordInfo`, for a runtime ABI assertion. */
size_t esm_record_info_size(void);

/** Size of `EsmSubRecordView`, for a runtime ABI assertion. */
size_t esm_subrecord_view_size(void);

/** Static version string of this library; never freed by the caller. */
const char *esm_version(void);

/*
 * Example:
 *
 *   EsmPlugin *plugin = esm_plugin_open("Oblivion.esm");
 *   if (plugin == NULL) { ... esm_last_error_message(...); }
 *
 *   EsmRecordInfo info;
 *   if (esm_plugin_find_record(plugin, 0x000966A9u, &info) == ESM_OK) {
 *       EsmPayload *payload = esm_plugin_decode_record(plugin, info.form_id);
 *       EsmSubRecordView edid;
 *       if (esm_payload_find_subrecord(payload, "EDID", &edid) == ESM_OK) { ... }
 *       esm_payload_free(payload);
 *   }
 *   esm_plugin_free(plugin);
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ESM_FFI_H */
