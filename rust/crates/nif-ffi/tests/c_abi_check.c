/* c_abi_check.c - compiles nif_ffi.h as C11 and exercises the ABI edges the
 * C++ unit tests in Oblivion_Android cannot reach: NULL arguments, the length
 * queries, short buffers, a misaligned out-parameter, the error message slot
 * and the borrowed geometry arrays.
 *
 * It builds one NIF file in memory — the same 20.0.0.5 layout the Rust tests
 * use, with a NiTriShape over a NiTriShapeData payload — so the assertions
 * cover a real parse rather than a stub.
 *
 * Build and run (mingw-w64, from the repository root, with
 * CARGO_TARGET_DIR=<repo>/target/gnu):
 *   gcc -std=c11 -O1 -Wall -Wextra -Wpedantic -I crates/nif-ffi/include \
 *       crates/nif-ffi/tests/c_abi_check.c \
 *       target/gnu/x86_64-pc-windows-gnu/release/libnif_ffi.a \
 *       -lws2_32 -lbcrypt -luserenv -lntdll -lole32 -loleaut32 -ladvapi32 \
 *       -luser32 -lshell32 -lkernel32 -o target/nif_c_abi_check.exe
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nif_ffi.h"

static int failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (!(cond)) {                                                           \
            ++failures;                                                          \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
        }                                                                        \
    } while (0)

/* ------------------------------------------------------------------ *
 * Fixture assembly: a little-endian writer over a fixed buffer.
 * ------------------------------------------------------------------ */

static unsigned char fixture[2048];
static size_t fixture_pos;
/* File offset of block 0, captured after the header so the header test can
 * compare against it instead of recomputing the byte count. */
static size_t fixture_block_start;

static void put_u8(unsigned value) { fixture[fixture_pos++] = (unsigned char)(value & 0xFFu); }

static void put_u16(unsigned value)
{
    put_u8(value & 0xFFu);
    put_u8((value >> 8) & 0xFFu);
}

static void put_u32(uint32_t value)
{
    put_u8(value & 0xFFu);
    put_u8((value >> 8) & 0xFFu);
    put_u8((value >> 16) & 0xFFu);
    put_u8((value >> 24) & 0xFFu);
}

static void put_f32(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    put_u32(bits);
}

/* SizedString: a u32 length counting the bytes that follow. */
static void put_sized(const char *text)
{
    size_t length = strlen(text);
    size_t i;

    put_u32((uint32_t)length);
    for (i = 0; i < length; ++i) {
        put_u8((unsigned char)text[i]);
    }
}

/* BzString: a u8 length counting the terminating NUL, then the body. */
static void put_bzstring(const char *text)
{
    size_t length = strlen(text);
    size_t i;

    put_u8((unsigned char)(length + 1));
    for (i = 0; i < length; ++i) {
        put_u8((unsigned char)text[i]);
    }
    put_u8(0);
}

/* The header of a Gamebryo 20.0.0.5 file with two blocks. */
static void put_header(void)
{
    static const char magic[] = "Gamebryo File Format, Version 20.0.0.5\n";
    size_t i;

    for (i = 0; i < sizeof(magic) - 1; ++i) {
        put_u8((unsigned char)magic[i]);
    }
    put_u32(0x14000005u); /* version */
    put_u8(1);            /* endian byte */
    put_u32(11);          /* user version */
    put_u32(2);           /* num objects */
    put_u32(11);          /* block stream (BS) version */
    put_bzstring("ccafasso");
    put_bzstring("");
    put_bzstring("");
    put_u16(2); /* num block types */
    put_sized("NiTriShape");
    put_sized("NiTriShapeData");
    put_u16(0); /* block 0 type index */
    put_u16(1); /* block 1 type index */
    put_u32(0); /* num groups */
}

/* NiTriShape scene block: name, extra data, controller, flags, transform,
 * properties, collision object, then the data / skin / shader fields. */
static void put_shape_block(void)
{
    int i;

    put_sized("Tri");
    put_u32(0);                 /* extra data count */
    put_u32(0xFFFFFFFFu);       /* controller */
    put_u16(0);                 /* flags */
    put_f32(1.0f);
    put_f32(2.0f);
    put_f32(3.0f);              /* translation */
    for (i = 0; i < 9; ++i) {
        put_f32((i % 4 == 0) ? 1.0f : 0.0f); /* identity rotation */
    }
    put_f32(2.0f);              /* scale */
    put_u32(0);                 /* property count */
    put_u32(0xFFFFFFFFu);       /* collision object */
    put_u32(1);                 /* data ref: block 1 */
    put_u32(0xFFFFFFFFu);       /* skin instance */
    put_u8(0);                  /* no shader */
}

/* NiTriShapeData payload: three vertices, one UV set, one triangle. */
static void put_shape_data_block(void)
{
    put_u32(0);    /* group id */
    put_u16(3);    /* num vertices */
    put_u8(0);     /* keep flags */
    put_u8(0);     /* compress flags */
    put_u8(1);     /* has vertices */
    put_f32(0.0f); put_f32(0.0f); put_f32(0.0f);
    put_f32(1.0f); put_f32(0.0f); put_f32(0.0f);
    put_f32(0.0f); put_f32(1.0f); put_f32(0.0f);
    put_u16(1);    /* data flags: one UV set */
    put_u8(0);     /* no normals */
    memset(fixture + fixture_pos, 0, 16); fixture_pos += 16; /* bounding sphere */
    put_u8(0);     /* no vertex colors */
    put_f32(0.0f); put_f32(0.0f);
    put_f32(1.0f); put_f32(0.0f);
    put_f32(0.0f); put_f32(1.0f);
    put_u16(0);    /* consistency flags */
    put_u32(0);    /* additional data */
    put_u16(1);    /* num triangles */
    put_u32(3);    /* num triangle points */
    put_u8(1);     /* has triangles */
    put_u16(0); put_u16(1); put_u16(2);
    put_u16(0);    /* match groups */
}

static void put_trailer(void)
{
    put_u32(1); /* value count */
    put_u32(0); /* one zero */
}

/* ------------------------------------------------------------------ *
 * Assertions
 * ------------------------------------------------------------------ */

static void check_basics(void)
{
    CHECK(NIF_OK == 0);
    CHECK(NIF_ERROR_INVALID_ARGUMENT == -1);
    CHECK(NIF_ERROR_PANIC == -7);
    CHECK(NIF_GEOM_FIELD_NAME == 0);
    CHECK(NIF_GEOM_FIELD_DIFFUSE_TEXTURE == 1);

    CHECK(nif_version() != NULL);
    CHECK(strncmp(nif_version(), "nif-ffi ", 8) == 0);

    CHECK(nif_last_error(NULL) != NULL);
    CHECK(nif_last_error(NULL)[0] != '\0');

    CHECK(nif_block_count(NULL) == 0);
    CHECK(nif_parse_memory(NULL, fixture, fixture_pos) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_geometry_count(NULL, NULL) == NIF_ERROR_INVALID_ARGUMENT);

    /* A NULL count out-parameter is rejected even on a live handle. */
    {
        void *handle = nif_create();
        CHECK(handle != NULL);
        CHECK(nif_geometry_count(handle, NULL) == NIF_ERROR_INVALID_ARGUMENT);
        nif_destroy(handle);
    }

    /* A freed handle is refused by the magic marker instead of being reached as
     * a parser — checked here while the memory is still recognisably ours — and
     * destroying NULL is a no-op. Neither makes a second destroy supported; see
     * nif_ffi.h. */
    {
        void *handle = nif_create();
        nif_destroy(handle);
        CHECK(nif_last_error(handle) != NULL);
        CHECK(nif_last_error(handle)[0] != '\0');
        nif_destroy(NULL);
        nif_destroy(handle);
    }
}

static void check_parse_and_geometry(void)
{
    void *handle = nif_create();
    nif_geometry_info info;
    uint32_t count = 99;
    uint32_t block = 99;
    char text[32];
    char tiny[3];
    FILE *file;

    CHECK(handle != NULL);
    if (handle == NULL) {
        return;
    }

    /* Nothing parsed yet: zero blocks, zero meshes, and the message says why
     * the empty parse failed. */
    CHECK(nif_block_count(handle) == 0);

    CHECK(nif_parse_memory(handle, NULL, 0) == NIF_ERROR_EMPTY);
    CHECK(nif_last_error(handle)[0] != '\0');
    CHECK(nif_block_count(handle) == 0);
    CHECK(nif_geometry_count(handle, &count) == NIF_OK && count == 0);
    CHECK(nif_get_geometries(handle, NULL, 0) == 0);

    /* Bytes that stop the magic read: no terminator before the end. */
    CHECK(nif_parse_memory(handle, (const uint8_t *)"not a nif", 9) == NIF_ERROR_TRUNCATED);
    CHECK(nif_last_error(handle)[0] != '\0');
    CHECK(nif_block_count(handle) == 0);

    /* A terminated magic line that names neither stream family. */
    CHECK(nif_parse_memory(handle, (const uint8_t *)"Not a NIF file\n", 15) ==
          NIF_ERROR_BAD_HEADER);
    CHECK(nif_last_error(handle)[0] != '\0');
    CHECK(nif_block_count(handle) == 0);

    /* A NULL buffer with a non-zero length is a caller bug, not an empty file. */
    CHECK(nif_parse_memory(handle, NULL, 16) == NIF_ERROR_INVALID_ARGUMENT);

    /* The real parse. */
    CHECK(nif_parse_memory(handle, fixture, fixture_pos) == NIF_OK);
    CHECK(nif_last_error(handle)[0] == '\0');
    CHECK(nif_block_count(handle) == 2);

    CHECK(nif_geometry_count(handle, &count) == NIF_OK);
    CHECK(count == 1);

    /* Length query, then the fill. */
    CHECK(nif_get_geometries(handle, NULL, 0) == 1);
    memset(&info, 0xAA, sizeof(info));
    CHECK(nif_get_geometries(handle, &info, 1) == 1);
    CHECK(info.vertex_count == 3);
    CHECK(info.normal_count == 0);
    CHECK(info.color_count == 0);
    CHECK(info.texcoord_count == 3);
    CHECK(info.triangle_count == 1);

    /* The arrays borrow from the handle: exact values, no copies to drift. */
    CHECK(info.vertices[0] == 0.0f && info.vertices[1] == 0.0f);
    CHECK(info.vertices[3] == 1.0f && info.vertices[4] == 0.0f);
    CHECK(info.vertices[6] == 0.0f && info.vertices[7] == 1.0f);
    CHECK(info.tex_coords[0] == 0.0f && info.tex_coords[1] == 0.0f);
    CHECK(info.tex_coords[2] == 1.0f && info.tex_coords[3] == 0.0f);
    CHECK(info.tex_coords[4] == 0.0f && info.tex_coords[5] == 1.0f);
    CHECK(info.triangles[0] == 0 && info.triangles[1] == 1 && info.triangles[2] == 2);
    CHECK(info.vertices != NULL && info.tex_coords != NULL && info.triangles != NULL);

    /* Transform: rotation first, then translation, then scale. */
    CHECK(info.transform[0] == 1.0f && info.transform[4] == 1.0f && info.transform[8] == 1.0f);
    CHECK(info.transform[1] == 0.0f && info.transform[3] == 0.0f);
    CHECK(info.transform[9] == 1.0f && info.transform[10] == 2.0f && info.transform[11] == 3.0f);
    CHECK(info.transform[12] == 2.0f);

    CHECK(info.material_property_index == 0xFFFFFFFFu);
    CHECK(info.texturing_property_index == 0xFFFFFFFFu);

    /* The mesh maps back onto its block. */
    CHECK(nif_geometry_block_index(handle, 0, &block) == NIF_OK && block == 0);
    CHECK(nif_geometry_block_index(handle, 1, &block) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_geometry_block_index(handle, 0, NULL) == NIF_ERROR_INVALID_ARGUMENT);

    /* Strings, snprintf style. */
    CHECK(nif_geometry_text(handle, 0, NIF_GEOM_FIELD_NAME, text, sizeof(text)) == 4);
    CHECK(strcmp(text, "Tri") == 0);
    CHECK(nif_geometry_text(handle, 0, NIF_GEOM_FIELD_NAME, NULL, 0) == 4);
    CHECK(nif_geometry_text(handle, 0, NIF_GEOM_FIELD_DIFFUSE_TEXTURE, text, sizeof(text)) == 1);
    CHECK(strcmp(text, "") == 0);
    CHECK(nif_geometry_text(handle, 0, NIF_GEOM_FIELD_NORMAL_TEXTURE, text, sizeof(text)) == 1);
    CHECK(nif_geometry_text(handle, 0, 99, text, sizeof(text)) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_geometry_text(handle, 7, NIF_GEOM_FIELD_NAME, text, sizeof(text)) ==
          NIF_ERROR_INVALID_ARGUMENT);

    /* A short buffer truncates but reports the full length. */
    memset(tiny, 'Z', sizeof(tiny));
    CHECK(nif_geometry_text(handle, 0, NIF_GEOM_FIELD_NAME, tiny, sizeof(tiny)) == 4);
    CHECK(tiny[2] == '\0');

    /* A misaligned record array is rejected before anything is written. */
    CHECK(nif_get_geometries(handle, (nif_geometry_info *)((char *)&info + 1), 1) ==
          NIF_ERROR_INVALID_ARGUMENT);

    /* The same file through the path entry point. */
    file = fopen("nif_c_abi_fixture.tmp", "wb");
    CHECK(file != NULL);
    if (file != NULL) {
        CHECK(fwrite(fixture, 1, fixture_pos, file) == fixture_pos);
        fclose(file);
        CHECK(nif_parse_file(handle, "nif_c_abi_fixture.tmp") == NIF_OK);
        CHECK(nif_block_count(handle) == 2);
        CHECK(nif_geometry_count(handle, &count) == NIF_OK && count == 1);
        remove("nif_c_abi_fixture.tmp");
    }

    /* A path that does not exist is an I/O failure with a readable message. */
    CHECK(nif_parse_file(handle, "no_such_directory\\no_such_file.nif") == NIF_ERROR_IO);
    CHECK(nif_last_error(handle)[0] != '\0');
    CHECK(nif_parse_file(handle, NULL) == NIF_ERROR_INVALID_ARGUMENT);

    nif_destroy(handle);
}

/* Skin and collision: the record layouts, the out-parameter rules, and the
 * two failure codes the parse calls report. Positive coverage — real skin and
 * collision blocks round-tripping — comes from the differential oracle, which
 * runs these same calls over retail meshes. */
static void check_skin_and_collision_abi(void)
{
    void *handle = nif_create();
    nif_skin_instance instance;
    nif_skin_data data;
    nif_skin_partition partition;
    nif_rigid_body body;
    nif_shape shape;
    nif_collision_object object;
    const char *message;

    CHECK(handle != NULL);
    CHECK(NIF_ERROR_NO_BLOCK == -8);
    CHECK(NIF_ERROR_INCOMPLETE == -9);
    CHECK(NIF_COLL_FIELD_TARGET_NAME == 0);

    /* Sizes the Rust side asserts too. */
    CHECK(sizeof(nif_skin_vertex_weight) == 8);
    CHECK(sizeof(nif_packed_weight) == 24);
    CHECK(sizeof(nif_rigid_body) == 100);
    CHECK(offsetof(nif_shape, vertices) == 40);
    CHECK(offsetof(nif_collision_object, body) == 8);

    /* NULL and misaligned out-records are rejected before anything is read. */
    CHECK(nif_parse_skin_instance(handle, NULL) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_parse_skin_data(handle, NULL) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_parse_skin_partition(handle, NULL) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_parse_rigid_body(handle, NULL) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_parse_shape(handle, NULL) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_parse_collision(handle, NULL) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_parse_skin_instance(handle,
                                  (nif_skin_instance *)((char *)&instance + 1)) ==
          NIF_ERROR_INVALID_ARGUMENT);

    /* The fixture carries neither a skin nor a collision block. */
    CHECK(nif_parse_skin_instance(handle, &instance) == NIF_ERROR_NO_BLOCK);
    CHECK(nif_parse_skin_data(handle, &data) == NIF_ERROR_NO_BLOCK);
    CHECK(nif_parse_skin_partition(handle, &partition) == NIF_ERROR_NO_BLOCK);
    CHECK(nif_parse_rigid_body(handle, &body) == NIF_ERROR_NO_BLOCK);
    CHECK(nif_parse_shape(handle, &shape) == NIF_ERROR_NO_BLOCK);
    CHECK(nif_parse_collision(handle, &object) == NIF_ERROR_NO_BLOCK);
    message = nif_last_error(handle);
    CHECK(message != NULL && message[0] != '\0');

    /* A collision string before any collision parse is a no-block; an unknown
     * field is rejected on its own. */
    CHECK(nif_collision_text(handle, NIF_COLL_FIELD_TARGET_NAME, NULL, 0) ==
          NIF_ERROR_NO_BLOCK);
    CHECK(nif_collision_text(handle, 42, NULL, 0) == NIF_ERROR_INVALID_ARGUMENT);

    nif_destroy(handle);
}

/* Animation: the record layouts and the two failure codes. Positive coverage —
 * real controller managers round-tripping — comes from the differential oracle
 * and the phase30 summary diff, which run these calls over retail meshes. */
static void check_animation_abi(void)
{
    void *handle = nif_create();
    nif_controller_manager manager;
    nif_controller_sequence sequence;

    CHECK(handle != NULL);

    /* Sizes and offsets the Rust side asserts too. */
    CHECK(sizeof(nif_controlled_block) == 44);
    CHECK(offsetof(nif_controlled_block, keyframe_data_index) == 36);
#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFull
    CHECK(sizeof(nif_text_key) == 16);
    CHECK(offsetof(nif_text_key, value) == 8);
    CHECK(sizeof(nif_controller_sequence) == 72);
    CHECK(offsetof(nif_controller_sequence, controller_manager_index) == 32);
    CHECK(sizeof(nif_controller_manager) == 24);
    CHECK(offsetof(nif_controller_manager, sequences) == 16);
#else
    CHECK(sizeof(nif_text_key) == 8);
    CHECK(offsetof(nif_text_key, value) == 4);
    CHECK(sizeof(nif_controller_sequence) == 52);
    CHECK(offsetof(nif_controller_sequence, controller_manager_index) == 16);
    CHECK(sizeof(nif_controller_manager) == 20);
    CHECK(offsetof(nif_controller_manager, sequences) == 16);
#endif

    /* NULL and misaligned out-records are rejected before anything is read. */
    CHECK(nif_parse_controller_manager(handle, NULL) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_parse_controller_sequence(handle, NULL) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_parse_controller_manager(
              handle, (nif_controller_manager *)((char *)&manager + 1)) ==
          NIF_ERROR_INVALID_ARGUMENT);

    /* The fixture carries no animation blocks. */
    CHECK(nif_parse_controller_manager(handle, &manager) == NIF_ERROR_NO_BLOCK);
    CHECK(nif_parse_controller_sequence(handle, &sequence) == NIF_ERROR_NO_BLOCK);
    CHECK(nif_last_error(handle)[0] != '\0');

    nif_destroy(handle);
}

/* Scene nodes and header: the record layouts, the field selectors, the root
 * list, and the index checks. */
static void check_nodes_and_header_abi(void)
{
    void *handle = nif_create();
    nif_node_info node;
    nif_header_info header;
    uint32_t root = 99;
    char text[64];
    char tiny[3];

    CHECK(handle != NULL);
    if (handle == NULL) {
        return;
    }

    /* Sizes and offsets the Rust side asserts too. */
    CHECK(sizeof(nif_node_info) == 16);
#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFull
    CHECK(sizeof(nif_header_info) == 48);
    CHECK(offsetof(nif_header_info, block_type_indices) == 32);
    CHECK(offsetof(nif_header_info, block_data_offset) == 40);
#else
    CHECK(sizeof(nif_header_info) == 40);
    CHECK(offsetof(nif_header_info, block_type_indices) == 28);
    CHECK(offsetof(nif_header_info, block_data_offset) == 32);
#endif

    /* NULL and misaligned out-records are rejected before anything is read. */
    CHECK(nif_get_node_info(NULL, 0, &node) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_node_text(NULL, 0, NIF_NODE_FIELD_NAME, text, sizeof(text)) ==
          NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_root_node_count(NULL) == 0);
    CHECK(nif_root_node_at(NULL, 0, &root) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_get_header_info(NULL, &header) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_header_text(NULL, NIF_HEADER_FIELD_MAGIC, text, sizeof(text)) ==
          NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_header_block_type_name(NULL, 0, text, sizeof(text)) ==
          NIF_ERROR_INVALID_ARGUMENT);

    CHECK(nif_parse_memory(handle, fixture, fixture_pos) == NIF_OK);

    /* NULL and misaligned out-records are rejected on a live handle too. */
    CHECK(nif_get_node_info(handle, 0, NULL) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_get_node_info(handle, 0, (nif_node_info *)((char *)&node + 1)) ==
          NIF_ERROR_INVALID_ARGUMENT);

    /* Block 0 is the NiTriShape "Tri"; block 1 the unnamed NiTriShapeData it
     * points at. Only the shape carries geometry. */
    CHECK(nif_get_node_info(handle, 0, &node) == NIF_OK);
    CHECK(node.node_index == 0);
    CHECK(node.parent_index == -1);
    CHECK(node.block_type_index == 0);
    CHECK(node.has_geometry == 1);
    CHECK(nif_get_node_info(handle, 1, &node) == NIF_OK);
    CHECK(node.node_index == 1);
    CHECK(node.parent_index == -1);
    CHECK(node.block_type_index == 1);
    CHECK(node.has_geometry == 0);
    CHECK(nif_get_node_info(handle, 2, &node) == NIF_ERROR_INVALID_ARGUMENT);

    /* Strings, snprintf style. */
    CHECK(nif_node_text(handle, 0, NIF_NODE_FIELD_NAME, text, sizeof(text)) == 4);
    CHECK(strcmp(text, "Tri") == 0);
    CHECK(nif_node_text(handle, 0, NIF_NODE_FIELD_BLOCK_TYPE_NAME, text, sizeof(text)) == 11);
    CHECK(strcmp(text, "NiTriShape") == 0);
    CHECK(nif_node_text(handle, 1, NIF_NODE_FIELD_NAME, text, sizeof(text)) == 1);
    CHECK(strcmp(text, "") == 0);
    CHECK(nif_node_text(handle, 1, NIF_NODE_FIELD_BLOCK_TYPE_NAME, text, sizeof(text)) == 15);
    CHECK(strcmp(text, "NiTriShapeData") == 0);
    CHECK(nif_node_text(handle, 0, 99, text, sizeof(text)) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_node_text(handle, 2, NIF_NODE_FIELD_NAME, text, sizeof(text)) ==
          NIF_ERROR_INVALID_ARGUMENT);

    /* A short buffer truncates but reports the full length. */
    memset(tiny, 'Z', sizeof(tiny));
    CHECK(nif_node_text(handle, 0, NIF_NODE_FIELD_BLOCK_TYPE_NAME, tiny, sizeof(tiny)) == 11);
    CHECK(tiny[2] == '\0');

    /* Neither the original reader nor this port wires the scene graph, so both
     * blocks are roots. */
    CHECK(nif_root_node_count(handle) == 2);
    CHECK(nif_root_node_at(handle, 0, &root) == NIF_OK && root == 0);
    CHECK(nif_root_node_at(handle, 1, &root) == NIF_OK && root == 1);
    CHECK(nif_root_node_at(handle, 2, &root) == NIF_ERROR_INVALID_ARGUMENT);
    CHECK(nif_root_node_at(handle, 0, NULL) == NIF_ERROR_INVALID_ARGUMENT);

    /* The parsed header. */
    CHECK(nif_get_header_info(handle, &header) == NIF_OK);
    CHECK(header.version == 0x14000005u);
    CHECK(header.endian == 1);
    CHECK(header.has_endian_byte == 1);
    CHECK(header.legacy_net_immerse == 0);
    CHECK(header.has_block_type_table == 1);
    CHECK(header.user_version == 11);
    CHECK(header.num_objects == 2);
    CHECK(header.unknown_field == 11);
    CHECK(header.num_groups == 0);
    CHECK(header.block_type_count == 2);
    CHECK(header.block_type_indices != NULL);
    CHECK(header.block_type_indices[0] == 0 && header.block_type_indices[1] == 1);
    CHECK(header.block_data_offset == (uint64_t)fixture_block_start);

    /* The stored magic line carries no trailing newline: the reader stops at
     * it and keeps the bytes before. */
    CHECK(nif_header_text(handle, NIF_HEADER_FIELD_MAGIC, text, sizeof(text)) == 39);
    CHECK(strcmp(text, "Gamebryo File Format, Version 20.0.0.5") == 0);
    CHECK(nif_header_text(handle, NIF_HEADER_FIELD_CREATOR, text, sizeof(text)) == 9);
    CHECK(strcmp(text, "ccafasso") == 0);
    CHECK(nif_header_text(handle, NIF_HEADER_FIELD_PROCESS_SCRIPT, text, sizeof(text)) == 1);
    CHECK(nif_header_text(handle, NIF_HEADER_FIELD_EXPORT_SCRIPT, text, sizeof(text)) == 1);
    CHECK(nif_header_text(handle, 99, text, sizeof(text)) == NIF_ERROR_INVALID_ARGUMENT);

    CHECK(nif_header_block_type_name(handle, 0, text, sizeof(text)) == 11);
    CHECK(strcmp(text, "NiTriShape") == 0);
    CHECK(nif_header_block_type_name(handle, 1, text, sizeof(text)) == 15);
    CHECK(strcmp(text, "NiTriShapeData") == 0);
    CHECK(nif_header_block_type_name(handle, 2, text, sizeof(text)) ==
          NIF_ERROR_INVALID_ARGUMENT);

    nif_destroy(handle);
}

int main(void)
{
    fixture_pos = 0;
    put_header();
    fixture_block_start = fixture_pos;
    put_shape_block();
    put_shape_data_block();
    put_trailer();

    check_basics();
    check_parse_and_geometry();
    check_skin_and_collision_abi();
    check_animation_abi();
    check_nodes_and_header_abi();

    if (failures == 0) {
        printf("nif-ffi C ABI check: all assertions passed\n");
        return 0;
    }
    fprintf(stderr, "nif-ffi C ABI check: %d assertion(s) failed\n", failures);
    return 1;
}
