/* C ABI over the `nif-core` crate.
 *
 * This interface owns state: a handle holds one parsed NIF file — the resident
 * file image, the header, the object array, the walked block ranges and every
 * decoded mesh. The C side works with an opaque handle created by nif_create()
 * and released by nif_destroy(), and every other call takes that handle as its
 * first argument.
 *
 * A handle must be used on one thread at a time.
 *
 * This is the geometry, skin, collision, animation and range wave of the
 * interface. It covers what the C++ parser's production path and the tests
 * need: parse a file, report its block count, hand back the decoded meshes,
 * the skin and collision blocks that `world_loader` and the tests read on
 * demand, the controller sequences the animation readers return, and the
 * object array and header that the parser's inline getters expose.
 *
 * Geometry, skin, collision and animation records borrow memory from the
 * handle: their arrays point into the parsed file's own buffers and stay valid
 * until the next parse call on the same handle — `nif_parse_file`,
 * `nif_parse_memory` and every `nif_parse_*` below. The accessors that share
 * the handle
 * (nif_geometry_count, nif_geometry_block_index, nif_geometry_text,
 * nif_collision_text) only write the message slot and leave those buffers
 * alone, so a caller may interleave them with its copy. Copy what you need
 * before parsing again. A count of zero makes its pointer meaningless — do not
 * dereference it, whatever its value.
 *
 * Strings copied into caller buffers follow the snprintf convention: the return
 * value is the length in bytes including the terminating NUL, so a caller
 * detects truncation by comparing it against the buffer capacity. Nothing is
 * written past `capacity` bytes, and a NULL `out` or a zero `capacity` queries
 * the required length without writing anything.
 */

#ifndef NIF_FFI_H
#define NIF_FFI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The call succeeded. */
#define NIF_OK 0
/* A required pointer argument was NULL, or a pointer was misaligned. */
#define NIF_ERROR_INVALID_ARGUMENT (-1)
/* The file could not be opened or read. */
#define NIF_ERROR_IO (-2)
/* The header runs past the end of the file. */
#define NIF_ERROR_TRUNCATED (-3)
/* The magic line names neither a Gamebryo nor a NetImmerse stream. */
#define NIF_ERROR_BAD_HEADER (-4)
/* A header field holds a value this reader rejects. */
#define NIF_ERROR_CORRUPT (-5)
/* The buffer holds no data. */
#define NIF_ERROR_EMPTY (-6)
/* A Rust panic was caught at the boundary; the call had no effect. */
#define NIF_ERROR_PANIC (-7)
/* The file holds no readable block of the requested type: it has none, the
 * file was never walked, or the block could not be decoded. */
#define NIF_ERROR_NO_BLOCK (-8)
/* The block was found but its layout ran past what the file holds. The out
 * record still carries everything that was read — the original parser leaves
 * the same partial result behind when it returns false. */
#define NIF_ERROR_INCOMPLETE (-9)

/* Field selector for nif_geometry_text(): the shape's object name. */
#define NIF_GEOM_FIELD_NAME 0
/* Field selector for nif_geometry_text(): the resolved diffuse texture path,
 * with '/' separators; empty when the shape carries no texturing property. */
#define NIF_GEOM_FIELD_DIFFUSE_TEXTURE 1
/* Field selector for nif_geometry_text(): the normal map path. Reserved: no
 * reader resolves it yet, so the call returns an empty string. */
#define NIF_GEOM_FIELD_NORMAL_TEXTURE 2

/* One decoded shape.
 *
 * The five arrays borrow from the handle (see the file comment). The transform
 * is copied and ordered as the C++ `NIFTransform` reads it: rotation first,
 * then translation, then scale — 13 floats.
 *
 * The property indices are block table indices, or 0xFFFFFFFF when the shape
 * carries no such property. The same sentinel marks "no property" on the C++
 * side, so the value crosses unchanged.
 *
 * The layout is asserted at compile time on both sides of the boundary; the
 * size differs with the pointer width (128 bytes on 64-bit, 100 on 32-bit).
 */
typedef struct nif_geometry_info {
    uint32_t vertex_count;
    uint32_t normal_count;
    uint32_t color_count;
    uint32_t texcoord_count;
    uint32_t triangle_count;
    const float *vertices;     /* 3 * vertex_count */
    const float *normals;      /* 3 * normal_count */
    const float *colors;       /* 4 * color_count */
    const float *tex_coords;   /* 2 * texcoord_count */
    const uint16_t *triangles; /* 3 * triangle_count */
    float transform[13];       /* rotation 9 + translation 3 + scale 1 */
    uint32_t material_property_index;
    uint32_t texturing_property_index;
} nif_geometry_info;

/* Compile-time ABI checks against the Rust definitions. */
#if defined(__cplusplus)
#if __cplusplus >= 201103L
#define NIF_FFI_STATIC_ASSERT(cond, name) static_assert((cond), #name)
#else
#define NIF_FFI_STATIC_ASSERT(cond, name) typedef char name[(cond) ? 1 : -1]
#endif
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define NIF_FFI_STATIC_ASSERT(cond, name) _Static_assert((cond), #name)
#else
#define NIF_FFI_STATIC_ASSERT(cond, name) typedef char name[(cond) ? 1 : -1]
#endif

NIF_FFI_STATIC_ASSERT(sizeof(float) == 4, nif_ffi_float_size);
NIF_FFI_STATIC_ASSERT(sizeof(uint32_t) == 4, nif_ffi_uint32_size);
#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFull
/* Five counts, four bytes of alignment padding, five pointers, 13 floats, two
 * indices, then four bytes of tail padding. */
NIF_FFI_STATIC_ASSERT(sizeof(nif_geometry_info) == 128, nif_ffi_geometry_info_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_geometry_info, transform) == 64,
                      nif_ffi_geometry_info_transform_offset);
#else
NIF_FFI_STATIC_ASSERT(sizeof(nif_geometry_info) == 100, nif_ffi_geometry_info_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_geometry_info, transform) == 40,
                      nif_ffi_geometry_info_transform_offset);
#endif

/* ---------------------------------------------------------------------------
 * Skinning and collision
 * ---------------------------------------------------------------------------
 *
 * Filled by the nif_parse_* calls below. Where a record holds a variable-length
 * payload — bone lists, palettes, weights, vertices, the MOPP code — the count
 * travels with a pointer into the handle, under the same borrow contract the
 * geometry records use.
 */

/* Field selector for nif_collision_text(). */
#define NIF_COLL_FIELD_TARGET_NAME 0

/* NiSkinInstance: Data | Skin Partition | Skeleton Root | bone references.
 * skin_partition_index is 0xFFFFFFFF below 10.1.0.101, where the instance has
 * no partition of its own. */
typedef struct nif_skin_instance {
    uint32_t skin_data_index;
    uint32_t skin_partition_index;
    uint32_t skeleton_root_index;
    uint32_t bone_count;
    const uint32_t *bone_node_indices;
} nif_skin_instance;

/* One BoneVertData entry: a vertex index and its weight. */
typedef struct nif_skin_vertex_weight {
    uint16_t vertex_index;
    float weight;
} nif_skin_vertex_weight;

/* One BoneData entry: a skin transform (13 floats, as in nif_geometry_info)
 * and the per-vertex weights behind it. */
typedef struct nif_bone_data {
    float skin_transform[13];
    uint32_t vertex_weight_count;
    const nif_skin_vertex_weight *vertex_weights;
} nif_bone_data;

/* NiSkinData: the root transform, the bone count, and the bone list. */
typedef struct nif_skin_data {
    float root_rotation[9];
    float root_translation[3];
    float root_scale;
    uint32_t num_bones;
    const nif_bone_data *bone_data;
} nif_skin_data;

/* Packed GPU weights for one vertex: four bone indices and four weights. */
typedef struct nif_packed_weight {
    uint16_t bone_indices[4];
    float weights[4];
} nif_packed_weight;

/* One entry of the skin partition array. */
typedef struct nif_partition {
    uint32_t num_vertices;
    uint32_t num_triangles;
    uint32_t bone_count;
    const uint16_t *bone_palette;
    uint32_t packed_weight_count;
    const nif_packed_weight *packed_weights;
    uint32_t index_count;
    const uint16_t *indices;
} nif_partition;

/* NiSkinPartition plus the two maxima the reader derives from its entries. */
typedef struct nif_skin_partition {
    uint32_t partition_count;
    const nif_partition *partitions;
    uint32_t max_bones_per_partition;
    uint32_t max_bones_per_vertex;
} nif_skin_partition;

/* The numbers a caller draws a rigid body with. `transform` keeps the
 * rotation / translation / scale order of nif_geometry_info; only the
 * translation is filled in, exactly as the original reader does. */
typedef struct nif_rigid_body {
    float mass;
    float friction;
    float restitution;
    float linear_velocity[3];
    float angular_velocity[3];
    float transform[13];
    uint32_t collision_group;
    uint32_t collision_filter;
    uint32_t is_trigger;
} nif_rigid_body;

/* One decoded Havok shape. `type` carries the CollisionShapeType numbering
 * (None = 0 .. List = 7) and which of the payloads is populated follows it;
 * the reference arrays are block table indices for the caller to follow. */
typedef struct nif_shape {
    uint32_t type;
    float center[3];
    float half_extents[3];
    float radius;
    float height;
    uint32_t vertex_count;
    const float *vertices;            /* 3 * vertex_count */
    uint32_t triangle_count;
    const uint16_t *triangles;        /* 3 * triangle_count */
    uint32_t mopp_data_size;
    const uint8_t *mopp_data;
    uint32_t child_shape_ref;
    uint32_t strips_ref_count;
    const uint32_t *strips_data_refs;
    uint32_t sub_shape_count;
    const uint32_t *sub_shape_refs;
} nif_shape;

/* bhkCollisionObject with its body and shape already resolved. The target
 * name is a string, so it comes from nif_collision_text() rather than from
 * this record. */
typedef struct nif_collision_object {
    uint32_t node_index;
    uint32_t rigid_body_index;
    nif_rigid_body body;
    nif_shape shape;
} nif_collision_object;

/* Skin and collision layout checks against the Rust definitions. Every size
 * depends on the pointer width: 64-bit pointers need alignment padding that
 * 32-bit ones do not. */
NIF_FFI_STATIC_ASSERT(sizeof(nif_skin_vertex_weight) == 8, nif_ffi_skin_vertex_weight_size);
NIF_FFI_STATIC_ASSERT(sizeof(nif_packed_weight) == 24, nif_ffi_packed_weight_size);
NIF_FFI_STATIC_ASSERT(sizeof(nif_rigid_body) == 100, nif_ffi_rigid_body_size);
#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFull
NIF_FFI_STATIC_ASSERT(sizeof(nif_skin_instance) == 24, nif_ffi_skin_instance_size);
NIF_FFI_STATIC_ASSERT(sizeof(nif_bone_data) == 64, nif_ffi_bone_data_size);
NIF_FFI_STATIC_ASSERT(sizeof(nif_skin_data) == 64, nif_ffi_skin_data_size);
NIF_FFI_STATIC_ASSERT(sizeof(nif_partition) == 56, nif_ffi_partition_size);
NIF_FFI_STATIC_ASSERT(sizeof(nif_skin_partition) == 24, nif_ffi_skin_partition_size);
NIF_FFI_STATIC_ASSERT(sizeof(nif_shape) == 112, nif_ffi_shape_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_shape, vertices) == 40, nif_ffi_shape_vertices_offset);
NIF_FFI_STATIC_ASSERT(sizeof(nif_collision_object) == 224, nif_ffi_collision_object_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_collision_object, shape) == 112,
                      nif_ffi_collision_object_shape_offset);
#else
NIF_FFI_STATIC_ASSERT(sizeof(nif_skin_instance) == 20, nif_ffi_skin_instance_size);
NIF_FFI_STATIC_ASSERT(sizeof(nif_bone_data) == 60, nif_ffi_bone_data_size);
NIF_FFI_STATIC_ASSERT(sizeof(nif_skin_data) == 60, nif_ffi_skin_data_size);
NIF_FFI_STATIC_ASSERT(sizeof(nif_partition) == 32, nif_ffi_partition_size);
NIF_FFI_STATIC_ASSERT(sizeof(nif_skin_partition) == 16, nif_ffi_skin_partition_size);
NIF_FFI_STATIC_ASSERT(sizeof(nif_shape) == 80, nif_ffi_shape_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_shape, vertices) == 40, nif_ffi_shape_vertices_offset);
NIF_FFI_STATIC_ASSERT(sizeof(nif_collision_object) == 188, nif_ffi_collision_object_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_collision_object, shape) == 108,
                      nif_ffi_collision_object_shape_offset);
#endif

/* A static, NUL-terminated version banner, for logs and crash reports. */
const char *nif_version(void);

/* Creates an empty handle and returns it, or NULL if none could be allocated. */
void *nif_create(void);
/* Frees the handle; the pointer must not be used afterwards. Passing NULL is a
 * no-op. A stale pointer usually fails the handle's magic check instead of
 * reaching the parser, but that check reads memory which has already been freed,
 * so a second destroy is a caller bug rather than a supported path. */
void nif_destroy(void *handle);

/* The message left by the last call on this handle, always valid, readable
 * until the next call writes it. The message is empty after a successful call.
 * A NULL or stale handle yields a static string instead. */
const char *nif_last_error(void *handle);

/* Reads `path` (a NUL-terminated string) into the handle and parses it,
 * replacing whatever the handle held. */
int nif_parse_file(void *handle, const char *path);
/* Parses `length` readable bytes, replacing whatever the handle held. */
int nif_parse_memory(void *handle, const uint8_t *data, size_t length);

/* The block count of the parsed object array. Returns 0 for a NULL handle or
 * an unparsed one — a valid NIF always declares at least one block, so 0 means
 * the call failed; nif_last_error() says why. */
uint32_t nif_block_count(void *handle);

/* Writes the number of decoded meshes to `out_count`. The count is what
 * `extractAllGeometry()` would report: shapes whose payload decoded, in block
 * order. */
int nif_geometry_count(void *handle, uint32_t *out_count);

/* Fills up to `capacity` records and returns the *total* number of meshes, so
 * a caller detects truncation by comparing the result against `capacity`. A
 * NULL `out` or a zero `capacity` is a length query: nothing is written and the
 * total is returned. Returns a negative NIF_ERROR_* code on failure. */
int nif_get_geometries(void *handle, nif_geometry_info *out, size_t capacity);

/* Writes the block index of mesh `geometry_index` to `out_index`, so a caller
 * holding its own node table can map the mesh back onto the right block. */
int nif_geometry_block_index(void *handle, uint32_t geometry_index, uint32_t *out_index);

/* Copies one mesh's string into `out` (snprintf style). `field` is one of the
 * NIF_GEOM_FIELD_* constants. An index past the end or an unknown field is a
 * NIF_ERROR_INVALID_ARGUMENT. */
int nif_geometry_text(void *handle, uint32_t geometry_index, int field, char *out,
                      size_t capacity);

/* Fills `out` with the first NiSkinInstance of the file, or NIF_ERROR_NO_BLOCK
 * when the file holds none or it could not be read. `out` must be NULL or a
 * correctly aligned record; its pointer borrows from the handle.
 *
 * On failure these three leave the caller's struct untouched, where the
 * original C++ readers kept whatever they had read before returning false.
 * nif_parse_rigid_body is the exception: it reports NIF_ERROR_INCOMPLETE after
 * filling `out`, which is the failure retail data actually reaches (legacy
 * meshes whose block is shorter than the layout) and the one the shim has to
 * reproduce bit for bit. */
int nif_parse_skin_instance(void *handle, nif_skin_instance *out);

/* Fills `out` with the first NiSkinData of the file, under the same rules. */
int nif_parse_skin_data(void *handle, nif_skin_data *out);

/* Fills `out` with the first NiSkinPartition of the file, under the same
 * rules. */
int nif_parse_skin_partition(void *handle, nif_skin_partition *out);

/* Fills `out` with the first bhkRigidBody / bhkRigidBodyT of the file. This
 * record carries no pointer, so nothing borrows from the handle here. Returns
 * NIF_ERROR_INCOMPLETE — and still fills `out` — when the block's layout runs
 * past what the file holds, which is how legacy (pre-10.1.0.0) meshes end: the
 * original returns false from the same spot having written whatever it read. */
int nif_parse_rigid_body(void *handle, nif_rigid_body *out);

/* Fills `out` with the first shape-derived block of the file. */
int nif_parse_shape(void *handle, nif_shape *out);

/* Fills `out` with the first bhkCollisionObject / bhkBlendCollisionObject,
 * body and shape already resolved. A body or shape that could not be read
 * stays at its defaults and the call still succeeds, which is what the C++
 * reader does; only a missing collision block reports NIF_ERROR_NO_BLOCK. */
int nif_parse_collision(void *handle, nif_collision_object *out);

/* Copies one collision string into `out` (snprintf style). `field` is one of
 * the NIF_COLL_FIELD_* constants; NIF_ERROR_NO_BLOCK when no collision object
 * has been parsed on this handle yet. */
int nif_collision_text(void *handle, int field, char *out, size_t capacity);

/* ---------------------------------------------------------------------------
 * Animation
 * ---------------------------------------------------------------------------
 *
 * The sequence record's two strings and its two arrays borrow from the handle
 * under the same contract as the records above; every other record here is
 * self-contained. `controller_sequence_count` is what the file declares,
 * `sequence_count` and `sequences` describe what was actually read — the
 * original logs a sequence that fails to parse and keeps the rest, so the two
 * can differ. A count of zero makes its pointer meaningless.
 *
 * The strings cross as NUL-terminated copies: `value` of a text key is the
 * original's `string_<index>` placeholder, because that is all the original
 * reader produces (`readStringRef` never looks a real string up).
 */

/* One NiTextKeyExtraData entry. */
typedef struct nif_text_key {
    float time;
    const char *value;
} nif_text_key;

/* One entry of a sequence's ControlledBlock array (version 20.0.0.4), plus
 * the two fields the original seeds while reading: keyframe_data_index from
 * the controller reference, resolved_bone_index at -1. No pointers, so the
 * layout is 44 bytes at both pointer widths. */
typedef struct nif_controlled_block {
    uint32_t interpolator_index;
    uint32_t controller_index;
    uint8_t priority;
    uint32_t string_palette_index;
    uint32_t node_name_offset;
    uint32_t property_type_offset;
    uint32_t controller_type_offset;
    uint32_t controller_id_offset;
    uint32_t interpolator_id_offset;
    uint32_t keyframe_data_index;
    int32_t resolved_bone_index;
} nif_controlled_block;

/* One NiControllerSequence. The four pointers lead so both pointer widths
 * pack without trailing holes: 72 bytes on 64-bit, 52 on 32-bit. `is_loop` is
 * 0/1 rather than a bool to keep the ABI free of bool representation rules. */
typedef struct nif_controller_sequence {
    const char *name;
    const char *target_name;
    const nif_controlled_block *controlled_blocks;
    const nif_text_key *text_keys;
    uint32_t controller_manager_index;
    float duration;
    float frequency;
    float phase;
    float start_time;
    float stop_time;
    uint32_t is_loop;
    uint32_t controlled_block_count;
    uint32_t text_key_count;
} nif_controller_sequence;

/* The first NiControllerManager of the file (see the section comment on the
 * two counts). 24 bytes on 64-bit, 20 on 32-bit. */
typedef struct nif_controller_manager {
    uint32_t controller_sequence_count;
    uint32_t object_palette_index;
    float last_time;
    uint32_t sequence_count;
    const nif_controller_sequence *sequences;
} nif_controller_manager;

/* Animation layout checks against the Rust definitions. */
#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFull
NIF_FFI_STATIC_ASSERT(sizeof(nif_text_key) == 16, nif_ffi_text_key_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_text_key, value) == 8, nif_ffi_text_key_value_offset);
NIF_FFI_STATIC_ASSERT(sizeof(nif_controlled_block) == 44, nif_ffi_controlled_block_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_controlled_block, keyframe_data_index) == 36,
                      nif_ffi_controlled_block_keyframe_offset);
NIF_FFI_STATIC_ASSERT(sizeof(nif_controller_sequence) == 72, nif_ffi_sequence_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_controller_sequence, controller_manager_index) == 32,
                      nif_ffi_sequence_manager_offset);
NIF_FFI_STATIC_ASSERT(sizeof(nif_controller_manager) == 24, nif_ffi_manager_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_controller_manager, sequences) == 16,
                      nif_ffi_manager_sequences_offset);
#else
NIF_FFI_STATIC_ASSERT(sizeof(nif_text_key) == 8, nif_ffi_text_key_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_text_key, value) == 4, nif_ffi_text_key_value_offset);
NIF_FFI_STATIC_ASSERT(sizeof(nif_controlled_block) == 44, nif_ffi_controlled_block_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_controlled_block, keyframe_data_index) == 36,
                      nif_ffi_controlled_block_keyframe_offset);
NIF_FFI_STATIC_ASSERT(sizeof(nif_controller_sequence) == 52, nif_ffi_sequence_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_controller_sequence, controller_manager_index) == 16,
                      nif_ffi_sequence_manager_offset);
NIF_FFI_STATIC_ASSERT(sizeof(nif_controller_manager) == 20, nif_ffi_manager_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_controller_manager, sequences) == 16,
                      nif_ffi_manager_sequences_offset);
#endif

/* Fills `out` with the first NiControllerManager of the file, or
 * NIF_ERROR_NO_BLOCK when the file holds none or it could not be read. On
 * failure the out record is left untouched, where the original kept whatever
 * it had read before returning false — no caller reads the record on `false`.
 * `out` must be NULL or a correctly aligned record; its pointers borrow from
 * the handle. */
int nif_parse_controller_manager(void *handle, nif_controller_manager *out);

/* Fills `out` with the first NiControllerSequence of the file, without the
 * manager around it, under the same rules. */
int nif_parse_controller_sequence(void *handle, nif_controller_sequence *out);

/* ---------------------------------------------------------------------------
 * Scene nodes and header
 * ---------------------------------------------------------------------------
 *
 * The object array and the parsed header, for the C++ parser's inline getters
 * (getNodes(), getRootNodeIndices(), getHeader()). The records are copies, not
 * borrows: the C++ shim materialises its NIFNode / NIFHeader members from them
 * during parse, exactly as the original walk did. The parent index crosses as
 * the parser leaves it — -1 for every node, because neither the original
 * reader nor this port ever wired the scene graph; child lists are empty for
 * the same reason and so are not part of the record.
 */

/* Field selector for nif_node_text(): the node's object name. */
#define NIF_NODE_FIELD_NAME 0
/* Field selector for nif_node_text(): the block's resolved type name. */
#define NIF_NODE_FIELD_BLOCK_TYPE_NAME 1

/* Field selector for nif_header_text(): the magic line, without its trailing
 * newline — the reader keeps the bytes before it. */
#define NIF_HEADER_FIELD_MAGIC 0
/* Field selector for nif_header_text(): the tool that wrote the file. */
#define NIF_HEADER_FIELD_CREATOR 1
/* Field selector for nif_header_text(): the process script name. */
#define NIF_HEADER_FIELD_PROCESS_SCRIPT 2
/* Field selector for nif_header_text(): the export script name. */
#define NIF_HEADER_FIELD_EXPORT_SCRIPT 3

/* One object-array entry. `has_geometry` is 0 or 1. */
typedef struct nif_node_info {
    uint32_t node_index;
    int32_t parent_index;
    uint32_t block_type_index;
    uint32_t has_geometry;
} nif_node_info;

/* The parsed header: fixed-width scalars, the borrowed block-type index table,
 * then the data offset widened to 64 bits so the record is identical on both
 * pointer widths. `block_type_indices` borrows from the handle under the
 * geometry records' storage rules; a count of zero makes it meaningless. */
typedef struct nif_header_info {
    uint32_t version;
    uint8_t endian;
    uint8_t has_endian_byte;
    uint8_t legacy_net_immerse;
    uint8_t has_block_type_table;
    uint32_t user_version;
    uint32_t num_objects;
    uint32_t unknown_field;
    uint32_t num_groups;
    uint32_t block_type_count;
    const uint16_t *block_type_indices;
    uint64_t block_data_offset;
} nif_header_info;

/* Range-wave layout checks against the Rust definitions. */
NIF_FFI_STATIC_ASSERT(sizeof(nif_node_info) == 16, nif_ffi_node_info_size);
#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFull
/* Eight scalars, four bytes of padding before the pointer, then the widened
 * offset. */
NIF_FFI_STATIC_ASSERT(sizeof(nif_header_info) == 48, nif_ffi_header_info_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_header_info, block_type_indices) == 32,
                      nif_ffi_header_info_indices_offset);
NIF_FFI_STATIC_ASSERT(offsetof(nif_header_info, block_data_offset) == 40,
                      nif_ffi_header_info_offset_offset);
#else
/* The pointer packs tight against the count, but the u64 still pads to its
 * 8-byte alignment. */
NIF_FFI_STATIC_ASSERT(sizeof(nif_header_info) == 40, nif_ffi_header_info_size);
NIF_FFI_STATIC_ASSERT(offsetof(nif_header_info, block_type_indices) == 28,
                      nif_ffi_header_info_indices_offset);
NIF_FFI_STATIC_ASSERT(offsetof(nif_header_info, block_data_offset) == 32,
                      nif_ffi_header_info_offset_offset);
#endif

/* Fills `out` with one object-array entry. An index past the end is a
 * NIF_ERROR_INVALID_ARGUMENT. */
int nif_get_node_info(void *handle, uint32_t node_index, nif_node_info *out);

/* Copies one node's string into `out` (snprintf style). `field` is one of the
 * NIF_NODE_FIELD_* constants. An index past the end or an unknown field is a
 * NIF_ERROR_INVALID_ARGUMENT. */
int nif_node_text(void *handle, uint32_t node_index, int field, char *out,
                  size_t capacity);

/* The number of root nodes — every node whose parent was never wired, which
 * is all of them. Returns 0 for a NULL handle. */
uint32_t nif_root_node_count(void *handle);

/* Writes the node index of one root to `out_index`, mirroring
 * getRootNodeIndices()[i]. A NULL out-index or an index past the end is a
 * NIF_ERROR_INVALID_ARGUMENT. */
int nif_root_node_at(void *handle, uint32_t root_index, uint32_t *out_index);

/* Fills `out` with the parsed header. */
int nif_get_header_info(void *handle, nif_header_info *out);

/* Copies one header string into `out` (snprintf style). `field` is one of the
 * NIF_HEADER_FIELD_* constants; an unknown field is a
 * NIF_ERROR_INVALID_ARGUMENT. */
int nif_header_text(void *handle, int field, char *out, size_t capacity);

/* Copies one block-type name into `out` (snprintf style). `type_index` indexes
 * the header's block-type name table, not the object array; past the end is a
 * NIF_ERROR_INVALID_ARGUMENT. */
int nif_header_block_type_name(void *handle, uint32_t type_index, char *out,
                               size_t capacity);

#ifdef __cplusplus
}
#endif

#endif /* NIF_FFI_H */
