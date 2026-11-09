/* plugin_ffi.h - C ABI over the Rust plugin-parser crate.
 *
 * Scope: FormID index map construction, load-order rebasing and the plugins.txt
 * grammar, matching the former C++ implementation in assets/formid_index.cpp and
 * assets/plugin_list.cpp byte for byte.
 *
 * Threading: each thread has its own last-error state, so plg_last_error_code()
 * and plg_last_error_message() only describe calls made on the calling thread.
 * The functions themselves are reentrant and allocate nothing.
 *
 * Strings copied into caller buffers follow the snprintf convention: the return
 * value is the length in bytes including the terminating NUL, so a caller
 * detects truncation by comparing it against the buffer capacity. Nothing is
 * written past `capacity` bytes, and a NULL `out` or a zero `capacity` queries
 * the required length without writing anything.
 *
 * Out parameters follow the esm-ffi convention: they are written on success
 * only, with one documented exception - the calls that copy a result into a
 * caller buffer record the required count in *out_len when they return
 * PLG_ERROR_BUFFER_TOO_SMALL, so a caller can grow the buffer and retry.
 *
 * Only the eight computing calls reset the error state. The constant accessors
 * leave it alone, so a caller can inspect the error a computing call left
 * behind.
 */
#ifndef PLUGIN_FFI_H
#define PLUGIN_FFI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Status codes. The numbering is shared with esm_ffi.h so that a caller can log
 * one code space; PLG_ERROR_BUFFER_TOO_SMALL is new here. */
#define PLG_OK 0
#define PLG_ERROR_INVALID_ARGUMENT (-1)
#define PLG_ERROR_PANIC (-7)
#define PLG_ERROR_BUFFER_TOO_SMALL (-8)

/* The index recorded for a master that is not in the loaded file list. */
uint32_t plg_unresolved_formid_index(void);

/* The base game file name, "Oblivion.esm" (snprintf style). */
int plg_base_plugin_file_name(char *out, size_t capacity);

/* The entry count plg_build_formid_index_map() needs for masters_len masters,
 * which is always masters_len + 1 (saturating at SIZE_MAX). */
size_t plg_formid_index_map_len(size_t masters_len);

/* Writes 1 to *out_equal when the two names match, 0 when they do not.
 * Comparison is case-insensitive over ASCII. */
int plg_plugin_name_equals(const char *lhs, const char *rhs, int *out_equal);

/* Builds a FormID index map of plg_formid_index_map_len(masters_len) entries:
 * one resolved position per master, then self_index as the last entry.
 * A master that is not loaded maps to plg_unresolved_formid_index().
 *
 * masters and loaded_file_names may be NULL when their length is 0, which is how
 * an empty MAST list is passed. A NULL element inside a non-empty array is
 * PLG_ERROR_INVALID_ARGUMENT.
 *
 * A NULL out_map or a capacity of 0 is a length query: nothing is written and
 * *out_len receives the required count. A short capacity returns
 * PLG_ERROR_BUFFER_TOO_SMALL and also records the required count in *out_len.
 * *out_len may be NULL when the count is not wanted. */
int plg_build_formid_index_map(const char *const *masters, size_t masters_len,
                              const char *const *loaded_file_names, size_t loaded_len,
                              uint32_t self_index, uint32_t *out_map, size_t capacity,
                              size_t *out_len);

/* Rebases one FormID into load-order space through index_map.
 * A FormID of 0, one whose index is past the end of the map, and one whose index
 * maps to plg_unresolved_formid_index() all come back unchanged, which is the
 * safe degradation: an unmapped FormID is never silently pointed at a different
 * plugin. index_map may be NULL with a length of 0. */
int plg_rebase_formid(uint32_t form_id, const uint32_t *index_map, size_t index_map_len,
                      uint32_t *out_form_id);

/* One entry of a plugins.txt buffer parsed by plg_parse_plugin_list().
 * The name stays inside the caller's buffer, which must therefore outlive every
 * use of the entry: contents + name_offset, name_length bytes, is the name. */
typedef struct plg_plugin_list_entry {
    size_t name_offset;
    size_t name_length;
    int active;
} plg_plugin_list_entry;

/* Parses the body of a plugins.txt file into out_entries.
 *
 * contents is read for exactly contents_len bytes and is never treated as a
 * NUL-terminated string, so a file holding a name that is not valid UTF-8 parses
 * the way it always did. The grammar matches the former C++ parser: one plugin
 * filename per line, a line whose first non-blank character is '#' is a comment,
 * a leading '*' marks the entry active, leading and trailing blanks, CR and a
 * UTF-8 BOM are ignored, blank lines are skipped, and the first occurrence of a
 * filename wins. A file with no '*' anywhere is read as "everything listed is
 * active", which is how the retail launcher writes it.
 *
 * contents may be NULL when contents_len is 0.
 *
 * A NULL out_entries or a capacity of 0 is a length query: nothing is written and
 * *out_len receives the required entry count. A short capacity returns
 * PLG_ERROR_BUFFER_TOO_SMALL and also records the required count in *out_len.
 * *out_len must not be NULL, since the count is the only way to learn how many
 * entries the file holds. */
int plg_parse_plugin_list(const char *contents, size_t contents_len,
                          plg_plugin_list_entry *out_entries, size_t capacity,
                          size_t *out_len);

/* Serialises names and their parallel active flags into plugins.txt text: one
 * name per line, a leading '*' on every active entry, a trailing newline after
 * each. Entries with an empty name are skipped, and an empty selection yields a
 * length of 0.
 *
 * The result is raw file bytes and is NOT NUL-terminated; *out_len carries its
 * exact length, so a caller writing the file passes that many bytes. names and
 * active_flags may be NULL when names_len is 0; a NULL array with a non-zero
 * names_len is PLG_ERROR_INVALID_ARGUMENT, as is a NULL element inside names.
 *
 * A NULL out or a capacity of 0 is a length query, and a short capacity returns
 * PLG_ERROR_BUFFER_TOO_SMALL. *out_len must not be NULL. */
int plg_serialize_plugin_list(const char *const *names, const int *active_flags,
                              size_t names_len, char *out, size_t capacity,
                              size_t *out_len);

/* Writes the indices of the active, named entries to out_indices: the entries a
 * caller should load, in the order they were given. An entry counts as active
 * when its flag is non-zero and its name is not empty.
 *
 * names and active_flags may be NULL when names_len is 0; a NULL array with a
 * non-zero names_len is PLG_ERROR_INVALID_ARGUMENT, as is a NULL element inside
 * names. A NULL out_indices or a capacity of 0 is a length query, and a short
 * capacity returns PLG_ERROR_BUFFER_TOO_SMALL. *out_len must not be NULL. */
int plg_active_plugin_indices(const char *const *names, const int *active_flags,
                              size_t names_len, size_t *out_indices, size_t capacity,
                              size_t *out_len);

/* Writes the retail load order of names to out_order, as indices into names:
 * Oblivion.esm first, then the other masters, then the plugins, each group
 * ordered case-insensitively by name. A permutation is returned rather than a
 * sorted copy so a caller can apply it to entries carrying more than a name; the
 * sort is stable, so names that tie keep their incoming order.
 *
 * names may be NULL when names_len is 0; a NULL array with a non-zero names_len
 * is PLG_ERROR_INVALID_ARGUMENT, as is a NULL element inside names. A NULL
 * out_order or a capacity of 0 is a length query, and a short capacity returns
 * PLG_ERROR_BUFFER_TOO_SMALL. *out_len must not be NULL. */
int plg_sort_plugin_load_order(const char *const *names, size_t names_len,
                               size_t *out_order, size_t capacity, size_t *out_len);

/* Writes 1 to *out_is_master when name ends in ".esm", 0 when it does not.
 * Only the final four bytes are examined, so "esm" is too short to be a master
 * and "Mod.esm.bak" ends in something else. Case is ignored. */
int plg_is_master_file_name(const char *name, int *out_is_master);

/* The error code left by the last computing call on this thread. */
int plg_last_error_code(void);

/* The message left by the last computing call on this thread (snprintf style). */
int plg_last_error_message(char *out, size_t capacity);

/* A static, NUL-terminated version banner, for logs and crash reports. */
const char *plg_version(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PLUGIN_FFI_H */
