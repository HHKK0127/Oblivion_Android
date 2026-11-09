/* c_abi_check.c - compiles assets_ffi.h as C11 and exercises the ABI edges that
 * the C++ unit tests in Oblivion_Android cannot reach: NULL arguments, the
 * length queries, misaligned pointers, and short buffers.
 *
 * Build and run (mingw-w64, from the crate root, with
 * CARGO_TARGET_DIR=<repo>/target/gnu):
 *   gcc -std=c11 -Wall -Wextra -I include tests/c_abi_check.c \
 *       ../target/gnu/release/libassets_ffi.a \
 *       -lws2_32 -lbcrypt -luserenv -lntdll -lole32 -loleaut32 -ladvapi32 \
 *       -luser32 -lshell32 -lkernel32 -o c_abi_check.exe
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "assets_ffi.h"

static int failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            ++failures;                                                          \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
        }                                                                        \
    } while (0)

/* Walk a packed candidate block, returning the nth NUL-terminated string. */
static const char *candidate(const char *block, size_t count, size_t index)
{
    size_t i;

    if (index >= count) {
        return NULL;
    }
    for (i = 0; i < index; ++i) {
        block += strlen(block) + 1;
    }
    return block;
}

static void check_basics(void)
{
    char name[32];

    CHECK(AFL_OK == 0);
    CHECK(AFL_VERTEX_FLOATS == 11);
    CHECK(afl_version() != NULL);
    CHECK(strncmp(afl_version(), "assets-ffi ", 11) == 0);

    /* The root folders keep their lookup-priority order; only the first five
     * matter to the caller, but the count is a hard part of the ABI. */
    CHECK(afl_asset_root_folder_count() == 9);
    CHECK(afl_asset_root_folder(0, name, sizeof(name)) == 7);
    CHECK(strcmp(name, "meshes") == 0);
    CHECK(afl_asset_root_folder(1, name, sizeof(name)) == 9);
    CHECK(strcmp(name, "textures") == 0);
    CHECK(afl_asset_root_folder(8, name, sizeof(name)) == 6);
    CHECK(strcmp(name, "trees") == 0);

    /* A NULL out or a zero capacity is a length query, not an error. */
    CHECK(afl_asset_root_folder(0, NULL, 0) == 7);
    CHECK(afl_asset_root_folder(0, name, 0) == 7);

    /* Truncation is reported by the return value, and the copy stays in bounds. */
    memset(name, 'Z', sizeof(name));
    CHECK(afl_asset_root_folder(0, name, 5) == 7);
    CHECK(strcmp(name, "mesh") == 0);

    CHECK(afl_asset_root_folder(9, name, sizeof(name)) == AFL_ERROR_INVALID_ARGUMENT);
    CHECK(afl_last_error_code() == AFL_ERROR_INVALID_ARGUMENT);
    CHECK(afl_last_error_message(name, sizeof(name)) > 0);
    CHECK(name[0] != '\0');

    /* The constant accessors must not clear the error a computing call left. */
    CHECK(afl_asset_root_folder_count() == 9);
    CHECK(afl_last_error_code() == AFL_ERROR_INVALID_ARGUMENT);
}

static void check_candidates(void)
{
    char block[512];
    char small[8];
    size_t count = 0;
    size_t bytes = 0;
    const char *path = "Dungeons\\Misc\\TrigZone01.NIF";

    CHECK(afl_build_asset_path_candidates(NULL, 0, NULL, 0, &count, &bytes) == AFL_OK);
    CHECK(count == 1);
    CHECK(bytes == 1); /* just the terminator */

    CHECK(afl_build_asset_path_candidates(path, strlen(path), NULL, 0, &count, &bytes) == AFL_OK);
    CHECK(count > 1);
    CHECK(bytes > strlen(path) + 1);

    CHECK(afl_build_asset_path_candidates(path, strlen(path), block, sizeof(block), &count,
                                          &bytes) == AFL_OK);
    CHECK(strcmp(candidate(block, count, 0), path) == 0); /* original spelling first */

    /* The already-rooted spelling is left alone rather than doubled up. */
    {
        const char *rooted = "meshes/Dungeons/Misc/TrigZone01.NIF";
        CHECK(afl_build_asset_path_candidates(rooted, strlen(rooted), block, sizeof(block), &count,
                                              &bytes) == AFL_OK);
        CHECK(count == 1);
        CHECK(strcmp(candidate(block, count, 0), rooted) == 0);
    }

    /* A separator-only path yields exactly one candidate and no empty ones. */
    {
        const char *sep = "/";
        CHECK(afl_build_asset_path_candidates(sep, 1, block, sizeof(block), &count, &bytes) == AFL_OK);
        CHECK(count == 1);
        CHECK(strcmp(candidate(block, count, 0), sep) == 0);
    }

    /* An embedded NUL is just another byte: a path is a byte run, not a C string,
     * so the terminator appears after the third byte rather than after the first. */
    {
        const char raw[] = {'a', '\0', 'b'};
        CHECK(afl_build_asset_path_candidates(raw, sizeof(raw), block, sizeof(block), &count,
                                              &bytes) == AFL_OK);
        CHECK(count == 10); /* the path itself plus one spelling per root folder */
        CHECK(memcmp(block, raw, sizeof(raw)) == 0);
        CHECK(block[sizeof(raw)] == '\0');
        CHECK(bytes > sizeof(raw) + 1);
    }

    /* A short buffer reports the size, writes nothing, and still fills counts. */
    memset(small, 'Z', sizeof(small));
    count = 0;
    bytes = 0;
    CHECK(afl_build_asset_path_candidates(path, strlen(path), small, sizeof(small), &count, &bytes) ==
          AFL_ERROR_BUFFER_TOO_SMALL);
    CHECK(count > 1);
    CHECK(bytes > sizeof(small));
    CHECK(small[0] == 'Z');
    CHECK(afl_last_error_code() == AFL_ERROR_BUFFER_TOO_SMALL);

    /* The out-parameters are not optional. */
    CHECK(afl_build_asset_path_candidates(path, strlen(path), block, sizeof(block), NULL, &bytes) ==
          AFL_ERROR_INVALID_ARGUMENT);
    CHECK(afl_build_asset_path_candidates(path, strlen(path), block, sizeof(block), &count, NULL) ==
          AFL_ERROR_INVALID_ARGUMENT);
    CHECK(afl_build_asset_path_candidates(NULL, 5, block, sizeof(block), &count, &bytes) ==
          AFL_ERROR_INVALID_ARGUMENT);
}

/* A quad: 4 vertices, 2 triangles, all attributes present. */
static const float g_quad_positions[] = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0};
static const float g_quad_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1};
static const float g_quad_tex_coords[] = {0, 0, 1, 0, 1, 1, 0, 1};
static const float g_quad_colors[] = {1, 1, 1, 1, 0.5f, 0.5f, 0.5f, 1,
                                      1, 1, 1, 1, 1, 1, 1, 1};
static const uint16_t g_quad_triangles[] = {0, 1, 2, 0, 2, 3};

static void check_merge(void)
{
    float vertices[11 * 8];
    uint32_t indices[12];
    afl_merge_counts counts;
    afl_geometry_input input;

    memset(&input, 0, sizeof(input));
    input.positions = g_quad_positions;
    input.position_count = 4;
    input.normals = g_quad_normals;
    input.normal_count = 4;
    input.tex_coords = g_quad_tex_coords;
    input.tex_coord_count = 4;
    input.colors = g_quad_colors;
    input.color_count = 4;
    input.triangles = g_quad_triangles;
    input.triangle_count = 2;

    /* NULL geometry with a non-zero count is a caller bug. */
    memset(&counts, 0, sizeof(counts));
    CHECK(afl_merge_geometry(NULL, 1, NULL, 0, NULL, 0, &counts) == AFL_ERROR_INVALID_ARGUMENT);

    /* Offering no writable space at all is a pure length query. */
    memset(&counts, 0, sizeof(counts));
    CHECK(afl_merge_geometry(&input, 1, NULL, 0, NULL, 0, &counts) == AFL_OK);
    CHECK(counts.vertex_count == 4);
    CHECK(counts.index_count == 6);
    CHECK(counts.geometry_count == 1);
    CHECK(counts.skipped_geometry_count == 0);

    /* And the real transfer fills both arrays. */
    memset(vertices, 0, sizeof(vertices));
    memset(indices, 0, sizeof(indices));
    CHECK(afl_merge_geometry(&input, 1, vertices, 11 * 8, indices, 12, &counts) == AFL_OK);
    CHECK(counts.vertex_count == 4);
    CHECK(counts.index_count == 6);
    CHECK(vertices[0] == 0.0f);
    CHECK(vertices[3] == 0.0f);
    CHECK(vertices[5] == 1.0f);
    CHECK(vertices[6] == 0.0f);
    CHECK(vertices[8] == 1.0f);
    CHECK(vertices[11 + 8] == 0.5f); /* second vertex: position x is 1, colour r is 0.5 */
    CHECK(indices[0] == 0);
    CHECK(indices[1] == 1);
    CHECK(indices[2] == 2);
    CHECK(indices[5] == 3);

    /* A short index buffer must leave the vertex buffer untouched. */
    memset(vertices, 0, sizeof(vertices));
    memset(indices, 0, sizeof(indices));
    CHECK(afl_merge_geometry(&input, 1, vertices, 11 * 8, indices, 5, &counts) ==
          AFL_ERROR_BUFFER_TOO_SMALL);
    CHECK(counts.vertex_count == 4);
    CHECK(counts.index_count == 6);
    CHECK(vertices[0] == 0.0f);
    CHECK(indices[0] == 0);

    /* vertex_capacity counts floats, so sizing it in vertices must be refused
     * rather than overrun: 4 vertices need 44 floats, not 4. */
    memset(vertices, 0, sizeof(vertices));
    memset(indices, 0, sizeof(indices));
    CHECK(afl_merge_geometry(&input, 1, vertices, counts.vertex_count, indices, 8, &counts) ==
          AFL_ERROR_BUFFER_TOO_SMALL);
    CHECK(vertices[0] == 0.0f);
    CHECK(indices[0] == 0);

    /* Missing attributes fall back per vertex, not per geometry. */
    input.normals = NULL;
    input.normal_count = 0;
    input.tex_coords = NULL;
    input.tex_coord_count = 0;
    input.colors = NULL;
    input.color_count = 0;
    CHECK(afl_merge_geometry(&input, 1, NULL, 0, NULL, 0, &counts) == AFL_OK);
    CHECK(counts.vertex_count == 4);
    CHECK(afl_merge_geometry(&input, 1, vertices, 11 * 8, indices, 12, &counts) == AFL_OK);
    CHECK(vertices[3] == 0.0f);
    CHECK(vertices[4] == 0.0f);
    CHECK(vertices[5] == 1.0f); /* default normal */
    CHECK(vertices[6] == 0.0f);
    CHECK(vertices[7] == 0.0f); /* default texcoord */
    CHECK(vertices[8] == 1.0f);
    CHECK(vertices[9] == 1.0f);
    CHECK(vertices[10] == 1.0f); /* default colour */

    /* An out-of-range index drops the triangle; a geometry with no vertices at
     * all is skipped outright. */
    {
        const uint16_t out_of_range[] = {0, 1, 9};
        const uint16_t usable[] = {0, 1, 2};
        afl_geometry_input pair[2];

        memset(pair, 0, sizeof(pair));
        pair[0].positions = g_quad_positions; /* no triangles: skipped */
        pair[0].position_count = 4;
        pair[0].triangles = out_of_range;
        pair[0].triangle_count = 1;
        pair[1].positions = g_quad_positions;
        pair[1].position_count = 4;
        pair[1].triangles = usable;
        pair[1].triangle_count = 1;

        CHECK(afl_merge_geometry(pair, 2, NULL, 0, NULL, 0, &counts) == AFL_OK);
        CHECK(counts.vertex_count == 4);
        CHECK(counts.index_count == 3);
        CHECK(counts.geometry_count == 1);
        CHECK(counts.skipped_geometry_count == 1);

        CHECK(afl_merge_geometry(pair, 2, vertices, 11 * 8, indices, 12, &counts) == AFL_OK);
        CHECK(indices[0] == 0);
        CHECK(indices[1] == 1);
        CHECK(indices[2] == 2);
    }

    /* Zero geometries is an empty result, not an error. */
    CHECK(afl_merge_geometry(NULL, 0, NULL, 0, NULL, 0, &counts) == AFL_OK);
    CHECK(counts.vertex_count == 0);
    CHECK(counts.index_count == 0);

    /* out_counts carries the answer, so it cannot be omitted. */
    CHECK(afl_merge_geometry(&input, 1, NULL, 0, NULL, 0, NULL) == AFL_ERROR_INVALID_ARGUMENT);
    CHECK(afl_last_error_code() == AFL_ERROR_INVALID_ARGUMENT);
}

int main(void)
{
    check_basics();
    check_candidates();
    check_merge();

    if (failures == 0) {
        printf("assets-ffi C ABI check: all assertions passed\n");
        return 0;
    }
    fprintf(stderr, "assets-ffi C ABI check: %d assertion(s) failed\n", failures);
    return 1;
}
