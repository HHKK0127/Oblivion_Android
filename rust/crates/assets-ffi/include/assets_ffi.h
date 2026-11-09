/* C ABI over the `assets-core` crate.
 *
 * Two groups of calls, both pure functions over caller-owned buffers:
 *
 *   afl_asset_root_folder*, afl_build_asset_path_candidates
 *       Expand an ESM-relative game path into the Data-folder spellings that can
 *       resolve. Paths travel as raw bytes, never as a NUL-terminated C string,
 *       because ESM records are not guaranteed to be valid UTF-8 and because a
 *       candidate list is a packed run of NUL-terminated strings rather than one
 *       string.
 *
 *   afl_merge_geometry
 *       Flatten decoded NIF geometry blocks into one interleaved vertex array
 *       plus one u32 index array, which is what the renderer uploads.
 *
 * Nothing is allocated on the Rust side and nothing needs to be released.
 *
 * Threading: each thread has its own last-error state, so afl_last_error_code and
 * afl_last_error_message only describe calls made on the calling thread.
 *
 * Strings copied into caller buffers follow the snprintf convention: the return
 * value is the length in bytes including the terminating NUL, so a caller detects
 * truncation by comparing it against the buffer capacity. Nothing is written past
 * `capacity` bytes, and a NULL `out` or a zero `capacity` queries the required
 * length without writing anything.
 *
 * Only the computing calls (afl_asset_root_folder, afl_build_asset_path_candidates,
 * afl_merge_geometry) reset the error state. The constant accessors leave it alone
 * so that a caller can inspect the error a computing call left behind.
 */

#ifndef ASSETS_FFI_H
#define ASSETS_FFI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The call succeeded. */
#define AFL_OK 0
/* A required pointer argument was NULL, or a pointer was misaligned. */
#define AFL_ERROR_INVALID_ARGUMENT (-1)
/* A Rust panic was caught at the boundary; the call had no effect. */
#define AFL_ERROR_PANIC (-7)
/* An output buffer cannot hold the result; the out-parameter holds the size. */
#define AFL_ERROR_BUFFER_TOO_SMALL (-8)

/* A static, NUL-terminated version banner, for logs and crash reports. */
const char *afl_version(void);

/* The error code left by the last computing call on this thread. */
int afl_last_error_code(void);
/* The message left by the last computing call on this thread (snprintf style). */
int afl_last_error_message(char *out, size_t capacity);

/* Type folders that sit directly under Data\ and therefore appear as the first
 * path segment of every BSA entry. Always 9. */
size_t afl_asset_root_folder_count(void);
/* The root folder at `index`, in lookup-priority order (snprintf style).
 * Returns AFL_ERROR_INVALID_ARGUMENT when `index` is out of range. */
int afl_asset_root_folder(size_t index, char *out, size_t capacity);

/* Expand `normalized` into the spellings that can resolve, most likely first.
 *
 * `normalized` must already have backslashes converted to '/'; `normalized_length`
 * is its byte length, so an embedded NUL is just another byte. The first candidate
 * is always the input as given, and an already rooted path yields exactly one.
 *
 * `out` receives the candidates packed as consecutive NUL-terminated strings.
 * `out_count` receives the number of candidates and `out_bytes` the total size of
 * the packed block in bytes including every terminator, so a caller can size a
 * buffer with a NULL `out` / zero `capacity` query and then ask again to fill it.
 * Both out-parameters must be non-NULL. When `capacity` is too small nothing is
 * written, both out-parameters are still filled in, and the call returns
 * AFL_ERROR_BUFFER_TOO_SMALL so the caller can grow and retry.
 */
int afl_build_asset_path_candidates(const char *normalized, size_t normalized_length,
                                    char *out, size_t capacity,
                                    size_t *out_count, size_t *out_bytes);

/* Floats per merged vertex: position (3), normal (3), texcoord (2), colour (3),
 * in the order the renderer's Vertex struct declares them. */
#define AFL_VERTEX_FLOATS 11

/* One decoded geometry block, borrowed as parallel attribute arrays.
 *
 * `positions` fixes the vertex count; the other arrays may be shorter, and an
 * attribute missing for a particular vertex falls back to its default per vertex
 * (normal (0,0,1), texcoord (0,0), colour (1,1,1)). Each non-empty array must hold
 * that many contiguous elements: `positions`, `normals` and `colors` are float
 * triples and quadruples, `tex_coords` float pairs, `triangles` uint16 triples.
 */
typedef struct afl_geometry_input {
    const float *positions;
    size_t position_count;
    const float *normals;
    size_t normal_count;
    const float *tex_coords;
    size_t tex_coord_count;
    const float *colors;
    size_t color_count;
    const uint16_t *triangles;
    size_t triangle_count;
} afl_geometry_input;

/* What afl_merge_geometry produced, or would need. */
typedef struct afl_merge_counts {
    size_t vertex_count;
    size_t index_count;
    /* Geometries that contributed at least one triangle. */
    size_t geometry_count;
    /* Geometries that contributed nothing. */
    size_t skipped_geometry_count;
} afl_merge_counts;

/* Merge `geometry_count` blocks into one vertex array and one index array.
 *
 * `out_vertices` holds `vertex_capacity` floats and `out_indices` holds
 * `index_capacity` uint32 values. Either buffer may be NULL with a zero capacity,
 * and passing NULL for both is a length query: nothing is written and `out_counts`
 * receives the required sizes. When a capacity is too small nothing is written,
 * `out_counts` is still filled in, and the call returns
 * AFL_ERROR_BUFFER_TOO_SMALL so the caller can grow and retry. Vertices are
 * AFL_VERTEX_FLOATS floats each, so `out_vertices` must hold
 * `vertex_count * AFL_VERTEX_FLOATS` floats.
 *
 * `out_counts` must be non-NULL. `geometries` may be NULL only when
 * `geometry_count` is 0.
 */
int afl_merge_geometry(const afl_geometry_input *geometries, size_t geometry_count,
                       float *out_vertices, size_t vertex_capacity,
                       uint32_t *out_indices, size_t index_capacity,
                       afl_merge_counts *out_counts);

#ifdef __cplusplus
}
#endif

#endif /* ASSETS_FFI_H */
