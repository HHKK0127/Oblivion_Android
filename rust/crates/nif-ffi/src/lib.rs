//! C ABI over the `nif-core` crate — the geometry wave.
//!
//! A handle owns one parsed NIF file: the resident image, the header, the
//! object array, the walked block ranges and every decoded mesh. The C++ shim
//! keeps one handle per `NIFParser` object and hands every call that handle,
//! which is what the original class did with its `fileBuffer`.
//!
//! The geometry records **borrow** from the handle: the five arrays point into
//! the parsed file's own buffers and stay valid until the next *parse* on that
//! handle, so a caller copies what it needs before parsing again. The accessors
//! that share the handle ([`nif_geometry_count`], [`nif_geometry_block_index`],
//! [`nif_geometry_text`]) only write the message slot, so a caller may interleave
//! them with its copy. That matches how `extractAllGeometry()` is used on the
//! C++ side — it copies the shapes into a `std::vector<NIFGeometry>` and never
//! touches the parser's storage afterwards.
//!
//! This wave covers the production path: parse a file, report its block count,
//! read the decoded meshes and their strings, the skin, animation and collision
//! records, and — since the range wave — the object array and header the C++
//! parser's inline getters expose.
//!
//! Every exported function runs inside a panic guard: a Rust panic must not
//! unwind into the C++ frames above, which are not built to take it.

use core::mem::{align_of, offset_of, size_of};
use core::panic::AssertUnwindSafe;
use core::slice;
use std::ffi::{c_char, c_int, CStr};
use std::panic::catch_unwind;

use nif_core::{
    CollisionObject, CollisionShape, ControllerSequence, Geometry, Matrix3x3, NifError, NifParser,
    RigidBodyInfo, SkinData, SkinInstance, SkinPartition, Transform,
};

/// The call succeeded.
pub const NIF_OK: c_int = 0;
/// A required pointer argument was NULL, or a pointer was misaligned.
pub const NIF_ERROR_INVALID_ARGUMENT: c_int = -1;
/// The file could not be opened or read.
pub const NIF_ERROR_IO: c_int = -2;
/// The header runs past the end of the file.
pub const NIF_ERROR_TRUNCATED: c_int = -3;
/// The magic line names neither a Gamebryo nor a NetImmerse stream.
pub const NIF_ERROR_BAD_HEADER: c_int = -4;
/// A header field holds a value this reader rejects.
pub const NIF_ERROR_CORRUPT: c_int = -5;
/// The buffer holds no data.
pub const NIF_ERROR_EMPTY: c_int = -6;
/// A Rust panic was caught at the boundary; the call had no effect.
pub const NIF_ERROR_PANIC: c_int = -7;
/// The file holds no readable block of the requested type: it has none, the
/// file was never walked, or the block could not be decoded.
pub const NIF_ERROR_NO_BLOCK: c_int = -8;
/// The block was found but its layout ran past what the file holds. The out
/// record still carries everything that was read — the original parser leaves
/// the same partial result behind when it returns false.
pub const NIF_ERROR_INCOMPLETE: c_int = -9;

/// Field selector for [`nif_collision_text`]: the name of the node the
/// collision object targets.
pub const NIF_COLL_FIELD_TARGET_NAME: c_int = 0;

/// Field selector for [`nif_geometry_text`]: the shape's object name.
pub const NIF_GEOM_FIELD_NAME: c_int = 0;
/// Field selector for [`nif_geometry_text`]: the resolved diffuse texture path.
pub const NIF_GEOM_FIELD_DIFFUSE_TEXTURE: c_int = 1;
/// Field selector for [`nif_geometry_text`]: the normal map path. Reserved; no
/// reader resolves it yet, so the call returns an empty string.
pub const NIF_GEOM_FIELD_NORMAL_TEXTURE: c_int = 2;

/// Field selector for [`nif_node_text`]: the node's object name.
pub const NIF_NODE_FIELD_NAME: c_int = 0;
/// Field selector for [`nif_node_text`]: the block's resolved type name.
pub const NIF_NODE_FIELD_BLOCK_TYPE_NAME: c_int = 1;

/// Field selector for [`nif_header_text`]: the magic line, without its trailing
/// newline — the reader keeps the bytes before it.
pub const NIF_HEADER_FIELD_MAGIC: c_int = 0;
/// Field selector for [`nif_header_text`]: the tool that wrote the file.
pub const NIF_HEADER_FIELD_CREATOR: c_int = 1;
/// Field selector for [`nif_header_text`]: the process script name.
pub const NIF_HEADER_FIELD_PROCESS_SCRIPT: c_int = 2;
/// Field selector for [`nif_header_text`]: the export script name.
pub const NIF_HEADER_FIELD_EXPORT_SCRIPT: c_int = 3;

/// A static, NUL-terminated version banner.
const VERSION: &str = concat!("nif-ffi ", env!("CARGO_PKG_VERSION"), "\0");

/// One decoded shape. See `include/nif_ffi.h` for the borrow contract.
///
/// The layout mirrors `nif_geometry_info`; the asserts below pin it.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifGeometryInfo {
    pub vertex_count: u32,
    pub normal_count: u32,
    pub color_count: u32,
    pub texcoord_count: u32,
    pub triangle_count: u32,
    pub vertices: *const f32,
    pub normals: *const f32,
    pub colors: *const f32,
    pub tex_coords: *const f32,
    pub triangles: *const u16,
    pub transform: [f32; 13],
    pub material_property_index: u32,
    pub texturing_property_index: u32,
}

// Five counts, alignment padding before the pointers, five pointers, 13 floats
// and two indices — and the padding differs with the pointer width, exactly as
// the C header's asserts spell out.
const _: () = assert!(size_of::<c_int>() == 4);
const _: () = assert!(align_of::<NifGeometryInfo>() == align_of::<*const f32>());
const _: () = assert!(
    size_of::<NifGeometryInfo>()
        == match size_of::<*const f32>() {
            8 => 128,
            _ => 100,
        }
);
const _: () = assert!(
    offset_of!(NifGeometryInfo, transform)
        == match size_of::<*const f32>() {
            8 => 64,
            _ => 40,
        }
);
const _: () = assert!(size_of::<nif_core::Vector3>() == 12);
const _: () = assert!(size_of::<nif_core::Vector4>() == 16);

/// One object-array entry, carrying the fields the C++ `NIFNode` exposes.
///
/// `parent_index` crosses exactly as the parser leaves it: `-1` for every node,
/// because neither the shipped corpus nor the original parser ever wired the
/// scene graph (`child_indices` is empty for the same reason and so is not
/// part of the record). `has_geometry` is 0 or 1.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifNodeInfo {
    pub node_index: u32,
    pub parent_index: i32,
    pub block_type_index: u32,
    pub has_geometry: u32,
}

const _: () = assert!(size_of::<NifNodeInfo>() == 16);

/// The parsed header laid out for C: fixed-width scalars, the borrowed
/// block-type index table, then the data offset widened to 64 bits so the
/// record is identical on both pointer widths.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifHeaderInfo {
    pub version: u32,
    pub endian: u8,
    pub has_endian_byte: u8,
    pub legacy_net_immerse: u8,
    pub has_block_type_table: u8,
    pub user_version: u32,
    pub num_objects: u32,
    pub unknown_field: u32,
    pub num_groups: u32,
    pub block_type_count: u32,
    pub block_type_indices: *const u16,
    pub block_data_offset: u64,
}

// Eight scalars, four bytes of padding before the pointer on 64-bit, then the
// widened offset. The 32-bit record keeps the pointer tight against the count
// but still pads the u64 to its 8-byte alignment.
const _: () = assert!(
    size_of::<NifHeaderInfo>()
        == match size_of::<*const u16>() {
            8 => 48,
            _ => 40,
        }
);
const _: () = assert!(
    offset_of!(NifHeaderInfo, block_type_indices)
        == match size_of::<*const u16>() {
            8 => 32,
            _ => 28,
        }
);
const _: () = assert!(
    offset_of!(NifHeaderInfo, block_data_offset)
        == match size_of::<*const u16>() {
            8 => 40,
            _ => 32,
        }
);

// ---------------------------------------------------------------------------
// Skinning and collision records. Same rule as the geometry record: each size
// is a function of the pointer width, so the C header asserts the same numbers
// with `UINTPTR_MAX`.
// ---------------------------------------------------------------------------

/// `NiSkinInstance`: data, partition, skeleton root and the bone references.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifSkinInstance {
    pub skin_data_index: u32,
    pub skin_partition_index: u32,
    pub skeleton_root_index: u32,
    pub bone_count: u32,
    pub bone_node_indices: *const u32,
}

/// One `BoneVertData` entry: a vertex index and its weight.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifSkinVertexWeight {
    pub vertex_index: u16,
    pub weight: f32,
}

/// One `BoneData` entry: the skin transform and the weights behind it.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifBoneData {
    pub skin_transform: [f32; 13],
    pub vertex_weight_count: u32,
    pub vertex_weights: *const NifSkinVertexWeight,
}

/// `NiSkinData`: the root transform, the bone count and the bone list.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifSkinData {
    pub root_rotation: [f32; 9],
    pub root_translation: [f32; 3],
    pub root_scale: f32,
    pub num_bones: u32,
    pub bone_data: *const NifBoneData,
}

/// Packed GPU weights for one vertex.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifPackedWeight {
    pub bone_indices: [u16; 4],
    pub weights: [f32; 4],
}

/// One entry of the skin partition array.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifPartition {
    pub num_vertices: u32,
    pub num_triangles: u32,
    pub bone_count: u32,
    pub bone_palette: *const u16,
    pub packed_weight_count: u32,
    pub packed_weights: *const NifPackedWeight,
    pub index_count: u32,
    pub indices: *const u16,
}

/// `NiSkinPartition` plus the two maxima derived from its entries.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifSkinPartition {
    pub partition_count: u32,
    pub partitions: *const NifPartition,
    pub max_bones_per_partition: u32,
    pub max_bones_per_vertex: u32,
}

/// The numbers a caller draws a rigid body with.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifRigidBody {
    pub mass: f32,
    pub friction: f32,
    pub restitution: f32,
    pub linear_velocity: [f32; 3],
    pub angular_velocity: [f32; 3],
    pub transform: [f32; 13],
    pub collision_group: u32,
    pub collision_filter: u32,
    pub is_trigger: u32,
}

/// One decoded Havok shape; `shape_type` carries the `CollisionShapeType`
/// numbering (`None = 0` .. `List = 7`).
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifShape {
    pub shape_type: u32,
    pub center: [f32; 3],
    pub half_extents: [f32; 3],
    pub radius: f32,
    pub height: f32,
    pub vertex_count: u32,
    pub vertices: *const f32,
    pub triangle_count: u32,
    pub triangles: *const u16,
    pub mopp_data_size: u32,
    pub mopp_data: *const u8,
    pub child_shape_ref: u32,
    pub strips_ref_count: u32,
    pub strips_data_refs: *const u32,
    pub sub_shape_count: u32,
    pub sub_shape_refs: *const u32,
}

/// `bhkCollisionObject` with body and shape already resolved.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifCollisionObject {
    pub node_index: u32,
    pub rigid_body_index: u32,
    pub body: NifRigidBody,
    pub shape: NifShape,
}

// ---------------------------------------------------------------------------
// Animation
//
// The sequence record leads with its four pointers so both pointer widths
// pack without trailing holes: 72 bytes on 64-bit, 52 on 32-bit. The C header
// asserts the same numbers with `UINTPTR_MAX`.
// ---------------------------------------------------------------------------

/// One `NiTextKeyExtraData` entry: the key time and its value as a
/// NUL-terminated placeholder — the original's `readStringRef` produces
/// `string_<index>` instead of the real text, and the value crosses the
/// boundary exactly as that placeholder.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifTextKey {
    pub time: f32,
    pub value: *const c_char,
}

/// One entry of a sequence's controlled block array: the raw
/// `ControlledBlock` fields of version 20.0.0.4, plus the two the original
/// seeds while reading (`keyframe_data_index` from the controller reference,
/// `resolved_bone_index` at -1).
///
/// No pointers, so the layout is 44 bytes at both pointer widths.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifControlledBlock {
    pub interpolator_index: u32,
    pub controller_index: u32,
    pub priority: u8,
    pub string_palette_index: u32,
    pub node_name_offset: u32,
    pub property_type_offset: u32,
    pub controller_type_offset: u32,
    pub controller_id_offset: u32,
    pub interpolator_id_offset: u32,
    pub keyframe_data_index: u32,
    pub resolved_bone_index: i32,
}

/// One `NiControllerSequence`: the two strings, the two arrays, then the
/// scalar timing fields. All three pointers borrow from the handle (see the
/// file comment); a count of zero makes the matching pointer meaningless.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifControllerSequence {
    pub name: *const c_char,
    pub target_name: *const c_char,
    pub controlled_blocks: *const NifControlledBlock,
    pub text_keys: *const NifTextKey,
    pub controller_manager_index: u32,
    pub duration: f32,
    pub frequency: f32,
    pub phase: f32,
    pub start_time: f32,
    pub stop_time: f32,
    pub is_loop: u32,
    pub controlled_block_count: u32,
    pub text_key_count: u32,
}

/// The first `NiControllerManager` of the file. `controller_sequence_count`
/// is what the file declares; `sequence_count` is how many sequences were
/// actually read and sit behind `sequences` — the original logs a sequence
/// that fails to parse and keeps going, so the two can differ.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct NifControllerManager {
    pub controller_sequence_count: u32,
    pub object_palette_index: u32,
    pub last_time: f32,
    pub sequence_count: u32,
    pub sequences: *const NifControllerSequence,
}

const _: () = assert!(size_of::<NifSkinVertexWeight>() == 8);
const _: () = assert!(offset_of!(NifSkinVertexWeight, weight) == 4);
const _: () = assert!(size_of::<NifPackedWeight>() == 24);
const _: () = assert!(offset_of!(NifPackedWeight, weights) == 8);
const _: () = assert!(size_of::<NifRigidBody>() == 100);
const _: () = assert!(align_of::<NifRigidBody>() == 4);
const _: () = assert!(
    size_of::<NifSkinInstance>()
        == match size_of::<*const u32>() {
            8 => 24,
            _ => 20,
        }
);
const _: () = assert!(
    size_of::<NifBoneData>()
        == match size_of::<*const u8>() {
            8 => 64,
            _ => 60,
        }
);
const _: () = assert!(
    size_of::<NifSkinData>()
        == match size_of::<*const u8>() {
            8 => 64,
            _ => 60,
        }
);
const _: () = assert!(
    size_of::<NifPartition>()
        == match size_of::<*const u8>() {
            8 => 56,
            _ => 32,
        }
);
const _: () = assert!(
    size_of::<NifSkinPartition>()
        == match size_of::<*const u8>() {
            8 => 24,
            _ => 16,
        }
);
const _: () = assert!(
    size_of::<NifShape>()
        == match size_of::<*const u8>() {
            8 => 112,
            _ => 80,
        }
);
const _: () = assert!(offset_of!(NifShape, vertices) == 40);
const _: () = assert!(
    size_of::<NifCollisionObject>()
        == match size_of::<*const u8>() {
            8 => 224,
            _ => 188,
        }
);
const _: () = assert!(offset_of!(NifCollisionObject, body) == 8);
const _: () = assert!(
    offset_of!(NifCollisionObject, shape)
        == match size_of::<*const u8>() {
            8 => 112,
            _ => 108,
        }
);
const _: () = assert!(
    size_of::<NifTextKey>()
        == match size_of::<*const u8>() {
            8 => 16,
            _ => 8,
        }
);
const _: () = assert!(
    offset_of!(NifTextKey, value)
        == match size_of::<*const u8>() {
            8 => 8,
            _ => 4,
        }
);
const _: () = assert!(size_of::<NifControlledBlock>() == 44);
const _: () = assert!(offset_of!(NifControlledBlock, keyframe_data_index) == 36);
const _: () = assert!(
    size_of::<NifControllerSequence>()
        == match size_of::<*const u8>() {
            8 => 72,
            _ => 52,
        }
);
const _: () = assert!(
    offset_of!(NifControllerSequence, controller_manager_index)
        == match size_of::<*const u8>() {
            8 => 32,
            _ => 16,
        }
);
const _: () = assert!(
    size_of::<NifControllerManager>()
        == match size_of::<*const u8>() {
            8 => 24,
            _ => 20,
        }
);
const _: () = assert!(offset_of!(NifControllerManager, sequences) == 16);

/// What a handle points at: a magic marker in front of the parser itself, plus
/// the last parsed skin/collision/animation blocks — their records borrow out
/// of these fields, so they must outlive the caller's copy.
///
/// The type is public only so the `extern "C"` signatures can name it; both
/// kinds of field stay private, so C callers see the `void *` from `nif_ffi.h`.
pub struct Handle {
    magic: u32,
    parser: NifParser,
    /// NUL-terminated message handed out by [`nif_last_error`].
    message: Vec<u8>,
    skin_instance: Option<SkinInstance>,
    skin_data: Option<SkinData>,
    /// POD mirror of `skin_data`, whose inner pointers borrow from it.
    skin_data_records: Vec<NifBoneData>,
    skin_partition: Option<SkinPartition>,
    /// POD mirror of `skin_partition`, whose inner pointers borrow from it.
    skin_partition_records: Vec<NifPartition>,
    shape: Option<CollisionShape>,
    collision: Option<CollisionObject>,
    /// POD mirror of the parsed controller manager, plus the string storage
    /// its sequences' names and text key values point into.
    controller_sequences: Vec<NifControllerSequence>,
    controller_blocks: Vec<NifControlledBlock>,
    controller_text_keys: Vec<NifTextKey>,
    /// NUL-terminated copies of every animation string the records expose.
    controller_strings: Vec<Vec<u8>>,
}

/// Marker written into a live handle, so a stale or wrong pointer fails the
/// magic check instead of reaching a `NifParser` that is not there.
const HANDLE_MAGIC: u32 = 0x4E49_4648; // "NIFH"

const MSG_OK: &[u8] = b"\0";
const MSG_NULL_HANDLE: &[u8] = b"the NIF handle is NULL\0";
const MSG_DEAD_HANDLE: &[u8] = b"the NIF handle is not a live handle\0";

fn set_message(handle: &mut Handle, text: &str) {
    handle.message.clear();
    handle.message.extend_from_slice(text.as_bytes());
    handle.message.push(0);
}

/// Record a failure and hand its code back to the caller.
fn fail(handle: &mut Handle, code: c_int, text: impl std::fmt::Display) -> c_int {
    set_message(handle, &format!("{text}"));
    code
}

/// Clear the message and report success.
fn succeed(handle: &mut Handle) -> c_int {
    set_message(handle, "");
    NIF_OK
}

/// Turn a parse error into its C code, leaving the message on the handle.
fn report(handle: &mut Handle, error: NifError) -> c_int {
    let code = match error {
        NifError::Io => NIF_ERROR_IO,
        NifError::Empty => NIF_ERROR_EMPTY,
        NifError::BadMagic => NIF_ERROR_BAD_HEADER,
        NifError::Truncated => NIF_ERROR_TRUNCATED,
        NifError::Corrupt => NIF_ERROR_CORRUPT,
    };
    fail(handle, code, error)
}

/// Run `body`, turning a caught panic into `fallback`.
fn guard_value<T: Copy>(fallback: T, body: impl FnOnce() -> T) -> T {
    match catch_unwind(AssertUnwindSafe(body)) {
        Ok(value) => value,
        Err(_) => fallback,
    }
}

/// Borrow the handle behind a caller pointer, checking the magic marker first.
///
/// # Safety
/// `handle` must be NULL or a pointer returned by [`nif_create`] that has not
/// been passed to [`nif_destroy`].
unsafe fn borrow_handle<'a>(handle: *mut Handle) -> Result<&'a mut Handle, ()> {
    if handle.is_null() {
        return Err(());
    }
    let borrowed = &mut *handle;
    if borrowed.magic != HANDLE_MAGIC {
        return Err(());
    }
    Ok(borrowed)
}

/// Run `body` against a borrowed handle, turning a caught panic into
/// `fallback`. A NULL or stale handle never reaches `body`; `nif_last_error`
/// explains it to the caller.
///
/// # Safety
/// `handle` must be NULL or a pointer returned by [`nif_create`] that has not
/// been passed to [`nif_destroy`].
unsafe fn guard<T: Copy>(
    fallback: T,
    handle: *mut Handle,
    body: impl FnOnce(&mut Handle) -> T,
) -> T {
    match catch_unwind(AssertUnwindSafe(|| match borrow_handle(handle) {
        Ok(borrowed) => body(borrowed),
        Err(()) => fallback,
    })) {
        Ok(value) => value,
        Err(_) => {
            // The handle is still live — a panic unwound out of `body`, so its
            // fields are back in scope and the message can be recorded there.
            if let Ok(borrowed) = borrow_handle(handle) {
                set_message(borrowed, "panic caught at the nif-ffi boundary");
            }
            fallback
        }
    }
}

/// Borrow `length` readable bytes from a caller pointer, allowing an empty run.
///
/// # Safety
/// When `length` is non-zero, `pointer` must address `length` readable bytes
/// that stay alive and unaliased for `'a`.
unsafe fn borrow_bytes<'a>(pointer: *const u8, length: usize) -> Result<&'a [u8], ()> {
    if length == 0 {
        return Ok(&[]);
    }
    if pointer.is_null() {
        return Err(());
    }
    Ok(slice::from_raw_parts(pointer, length))
}

/// Copy `text` into a caller buffer, snprintf style: the return value is the
/// length in bytes including the terminating NUL, so a caller can detect
/// truncation, and nothing is written beyond `capacity`.
fn write_c_string(text: &[u8], out: *mut c_char, capacity: usize) -> c_int {
    let needed = text.len() + 1;
    if !out.is_null() && capacity > 0 {
        let copied = text.len().min(capacity - 1);
        // SAFETY: the caller guarantees `capacity` writable bytes at `out`.
        unsafe {
            core::ptr::copy_nonoverlapping(text.as_ptr().cast::<c_char>(), out, copied);
            *out.add(copied) = 0;
        }
    }
    needed as c_int
}

/// Rotation 9 + translation 3 + scale 1 — the order `nif_geometry_info`,
/// `nif_bone_data` and `nif_rigid_body` all carry a transform in.
fn flatten_transform(transform: &Transform) -> [f32; 13] {
    let mut flat = [0.0f32; 13];
    for (slot, value) in flat.iter_mut().zip(transform.rotation.m.iter().flatten()) {
        *slot = *value;
    }
    flat[9] = transform.translation.x;
    flat[10] = transform.translation.y;
    flat[11] = transform.translation.z;
    flat[12] = transform.scale;
    flat
}

/// Rotation only, as `nif_skin_data` stores the root transform's parts.
fn flatten_rotation(matrix: &Matrix3x3) -> [f32; 9] {
    let mut flat = [0.0f32; 9];
    for (slot, value) in flat.iter_mut().zip(matrix.m.iter().flatten()) {
        *slot = *value;
    }
    flat
}

fn build_rigid_body(body: &RigidBodyInfo) -> NifRigidBody {
    NifRigidBody {
        mass: body.mass,
        friction: body.friction,
        restitution: body.restitution,
        linear_velocity: [
            body.linear_velocity.x,
            body.linear_velocity.y,
            body.linear_velocity.z,
        ],
        angular_velocity: [
            body.angular_velocity.x,
            body.angular_velocity.y,
            body.angular_velocity.z,
        ],
        transform: flatten_transform(&body.transform),
        collision_group: body.collision_group,
        collision_filter: body.collision_filter,
        is_trigger: u32::from(body.is_trigger),
    }
}

/// The shape record, with every payload pointer aiming into the shape the
/// handle owns. `vertices` starts a run of `Vector3`, which `repr(C)` makes
/// exactly three `f32` wide, so it can be handed out as `const float *`.
fn build_shape(shape: &CollisionShape) -> NifShape {
    NifShape {
        shape_type: shape.shape_type as u32,
        center: [shape.center.x, shape.center.y, shape.center.z],
        half_extents: [
            shape.half_extents.x,
            shape.half_extents.y,
            shape.half_extents.z,
        ],
        radius: shape.radius,
        height: shape.height,
        vertex_count: shape.vertices.len() as u32,
        vertices: shape.vertices.as_ptr().cast::<f32>(),
        triangle_count: shape.triangles.len() as u32,
        triangles: shape.triangles.as_ptr().cast::<u16>(),
        mopp_data_size: shape.mopp_data_size,
        mopp_data: shape.mopp_data.as_ptr(),
        child_shape_ref: shape.child_shape_ref,
        strips_ref_count: shape.strips_data_refs.len() as u32,
        strips_data_refs: shape.strips_data_refs.as_ptr(),
        sub_shape_count: shape.sub_shape_refs.len() as u32,
        sub_shape_refs: shape.sub_shape_refs.as_ptr(),
    }
}

/// Validate an out-pointer for a POD record: non-NULL and correctly aligned.
/// The failure is recorded on the handle before it is handed back.
fn out_record<T>(handle: &mut Handle, out: *mut T, what: &str) -> Result<*mut T, c_int> {
    if out.is_null() {
        return Err(fail(
            handle,
            NIF_ERROR_INVALID_ARGUMENT,
            format!("the {what} out-parameter is NULL"),
        ));
    }
    if (out as usize) % align_of::<T>() != 0 {
        return Err(fail(
            handle,
            NIF_ERROR_INVALID_ARGUMENT,
            format!("the {what} out-parameter is misaligned"),
        ));
    }
    Ok(out)
}

/// The file has no readable block of the requested type.
fn no_block(handle: &mut Handle, what: &str) -> c_int {
    fail(
        handle,
        NIF_ERROR_NO_BLOCK,
        format!("no readable {what} block in this file"),
    )
}

/// Flatten one geometry into its C record. The arrays keep pointing at the
/// parser's own buffers, which is what the borrow contract in the header
/// describes.
fn fill_info(geometry: &Geometry) -> NifGeometryInfo {
    NifGeometryInfo {
        vertex_count: geometry.vertices.len() as u32,
        normal_count: geometry.normals.len() as u32,
        color_count: geometry.colors.len() as u32,
        texcoord_count: geometry.tex_coords.len() as u32,
        triangle_count: geometry.triangles.len() as u32,
        vertices: geometry.vertices.as_ptr().cast(),
        normals: geometry.normals.as_ptr().cast(),
        colors: geometry.colors.as_ptr().cast(),
        tex_coords: geometry.tex_coords.as_ptr().cast(),
        triangles: geometry.triangles.as_ptr().cast(),
        transform: flatten_transform(&geometry.transform),
        material_property_index: geometry.material_property_index,
        texturing_property_index: geometry.texturing_property_index,
    }
}

/// A static, NUL-terminated version banner, for logs and crash reports.
#[no_mangle]
pub extern "C" fn nif_version() -> *const c_char {
    VERSION.as_ptr().cast::<c_char>()
}

/// The message left by the last call on this handle, always valid.
#[no_mangle]
pub unsafe extern "C" fn nif_last_error(handle: *mut Handle) -> *const c_char {
    match borrow_handle(handle) {
        Ok(borrowed) => borrowed.message.as_ptr().cast::<c_char>(),
        Err(()) if handle.is_null() => MSG_NULL_HANDLE.as_ptr().cast::<c_char>(),
        Err(()) => MSG_DEAD_HANDLE.as_ptr().cast::<c_char>(),
    }
}

/// Creates an empty handle and returns it, or NULL if none could be allocated.
#[no_mangle]
pub extern "C" fn nif_create() -> *mut Handle {
    guard_value(core::ptr::null_mut(), || {
        let handle = Box::new(Handle {
            magic: HANDLE_MAGIC,
            parser: NifParser::new(),
            message: MSG_OK.to_vec(),
            skin_instance: None,
            skin_data: None,
            skin_data_records: Vec::new(),
            skin_partition: None,
            skin_partition_records: Vec::new(),
            shape: None,
            collision: None,
            controller_sequences: Vec::new(),
            controller_blocks: Vec::new(),
            controller_text_keys: Vec::new(),
            controller_strings: Vec::new(),
        });
        Box::into_raw(handle)
    })
}

/// Frees the handle; the pointer must not be used afterwards.
///
/// The caller's pointer is not cleared, because C cannot be given one. A stale
/// pointer usually fails the magic check instead of reaching the parser, but
/// that check reads memory which has already been freed — so a second destroy is
/// a caller bug rather than a supported path.
///
/// # Safety
/// `handle` must be NULL or a pointer returned by [`nif_create`].
#[no_mangle]
pub unsafe extern "C" fn nif_destroy(handle: *mut Handle) {
    guard_value((), || {
        if handle.is_null() {
            return;
        }
        let owned = &mut *handle;
        if owned.magic != HANDLE_MAGIC {
            return;
        }
        owned.magic = 0;
        drop(Box::from_raw(handle));
    });
}

/// Reads `path` into the handle and parses it, replacing whatever the handle
/// held.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract, and `path` must be NULL or
/// a NUL-terminated string.
#[no_mangle]
pub unsafe extern "C" fn nif_parse_file(handle: *mut Handle, path: *const c_char) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        if path.is_null() {
            return fail(
                borrowed,
                NIF_ERROR_INVALID_ARGUMENT,
                "the path pointer is NULL",
            );
        }
        // SAFETY: the caller guarantees a NUL-terminated string.
        let text = match CStr::from_ptr(path).to_str() {
            Ok(text) => text,
            Err(_) => {
                return fail(
                    borrowed,
                    NIF_ERROR_INVALID_ARGUMENT,
                    "the path is not valid UTF-8",
                )
            }
        };
        match borrowed.parser.parse_file(text) {
            Ok(()) => succeed(borrowed),
            Err(error) => report(borrowed, error),
        }
    })
}

/// Parses `length` bytes, replacing whatever the handle held.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract, and when `length` is
/// non-zero `data` must address that many readable bytes.
#[no_mangle]
pub unsafe extern "C" fn nif_parse_memory(
    handle: *mut Handle,
    data: *const u8,
    length: usize,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let bytes = match borrow_bytes(data, length) {
            Ok(bytes) => bytes,
            Err(()) => {
                return fail(
                    borrowed,
                    NIF_ERROR_INVALID_ARGUMENT,
                    "the buffer is NULL but its length is not zero",
                )
            }
        };
        match borrowed.parser.parse_from_data(bytes) {
            Ok(()) => succeed(borrowed),
            Err(error) => report(borrowed, error),
        }
    })
}

/// The block count of the parsed object array.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract.
#[no_mangle]
pub unsafe extern "C" fn nif_block_count(handle: *mut Handle) -> u32 {
    guard(0, handle, |borrowed| borrowed.parser.block_count())
}

/// Writes the number of decoded meshes to `out_count`.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract, and `out_count` must be
/// NULL or point to one writable `uint32_t`.
#[no_mangle]
pub unsafe extern "C" fn nif_geometry_count(handle: *mut Handle, out_count: *mut u32) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        if out_count.is_null() {
            return fail(
                borrowed,
                NIF_ERROR_INVALID_ARGUMENT,
                "the count out-parameter is NULL",
            );
        }
        *out_count = borrowed.parser.geometry_nodes().count() as u32;
        succeed(borrowed)
    })
}

/// Fills up to `capacity` records and returns the total number of meshes.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract. When `out` is non-NULL and
/// `capacity` is non-zero, `out` must address `capacity` writable, correctly
/// aligned `nif_geometry_info` records.
#[no_mangle]
pub unsafe extern "C" fn nif_get_geometries(
    handle: *mut Handle,
    out: *mut NifGeometryInfo,
    capacity: usize,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let total = borrowed.parser.geometry_nodes().count();
        if out.is_null() || capacity == 0 {
            // Length query: nothing is written, the total comes back.
            set_message(borrowed, "");
            return total as c_int;
        }
        if (out as usize) % align_of::<NifGeometryInfo>() != 0 {
            return fail(
                borrowed,
                NIF_ERROR_INVALID_ARGUMENT,
                "the geometry out-parameter is misaligned",
            );
        }

        let fill = total.min(capacity);
        // Collect before writing: the records borrow from the parser, and the
        // copies below must not alias a mutable borrow of the same handle.
        let records: Vec<NifGeometryInfo> = borrowed
            .parser
            .geometry_nodes()
            .take(fill)
            .map(|(_, geometry)| fill_info(geometry))
            .collect();
        for (index, record) in records.iter().enumerate() {
            // SAFETY: the caller guarantees `capacity` writable records at
            // `out`, and `index < fill <= capacity`.
            *out.add(index) = *record;
        }
        set_message(borrowed, "");
        total as c_int
    })
}

/// Writes the block index of one mesh to `out_index`.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract, and `out_index` must be
/// NULL or point to one writable `uint32_t`.
#[no_mangle]
pub unsafe extern "C" fn nif_geometry_block_index(
    handle: *mut Handle,
    geometry_index: u32,
    out_index: *mut u32,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        if out_index.is_null() {
            return fail(
                borrowed,
                NIF_ERROR_INVALID_ARGUMENT,
                "the block index out-parameter is NULL",
            );
        }
        // Bind the answer to an owned value first: the lookup borrows the
        // parser, and the arms below need to write the message.
        let block_index = borrowed
            .parser
            .geometry_nodes()
            .nth(geometry_index as usize)
            .map(|(block_index, _)| block_index);
        match block_index {
            Some(block_index) => {
                *out_index = block_index;
                succeed(borrowed)
            }
            None => fail(
                borrowed,
                NIF_ERROR_INVALID_ARGUMENT,
                format!("geometry index {geometry_index} is out of range"),
            ),
        }
    })
}

/// Copies one mesh's string into `out`, snprintf style.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract, and `out` must be NULL or
/// point to `capacity` writable bytes.
#[no_mangle]
pub unsafe extern "C" fn nif_geometry_text(
    handle: *mut Handle,
    geometry_index: u32,
    field: c_int,
    out: *mut c_char,
    capacity: usize,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        if field != NIF_GEOM_FIELD_NAME
            && field != NIF_GEOM_FIELD_DIFFUSE_TEXTURE
            && field != NIF_GEOM_FIELD_NORMAL_TEXTURE
        {
            return fail(
                borrowed,
                NIF_ERROR_INVALID_ARGUMENT,
                format!("unknown geometry text field {field}"),
            );
        }
        // The copy leaves the parser borrow behind, so the message can be
        // written below without overlapping it.
        let text = borrowed
            .parser
            .geometry_nodes()
            .nth(geometry_index as usize)
            .map(|(_, geometry)| match field {
                NIF_GEOM_FIELD_DIFFUSE_TEXTURE => geometry.diffuse_texture.clone(),
                NIF_GEOM_FIELD_NORMAL_TEXTURE => geometry.normal_texture.clone(),
                _ => geometry.name.clone(),
            });
        match text {
            Some(text) => {
                let needed = write_c_string(text.as_bytes(), out, capacity);
                set_message(borrowed, "");
                needed
            }
            None => fail(
                borrowed,
                NIF_ERROR_INVALID_ARGUMENT,
                format!("geometry index {geometry_index} is out of range"),
            ),
        }
    })
}

// ---------------------------------------------------------------------------
// Skinning and collision
//
// Each parse stores its result on the handle first and then builds the record
// from the stored value: the payload pointers aim into vectors the handle owns,
// so they outlive the call and die with the next parse, exactly like the
// geometry records do.
// ---------------------------------------------------------------------------

/// Fills `out` with the first `NiSkinInstance` of the file.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out` must be NULL or one
/// writable, correctly aligned record.
#[no_mangle]
pub unsafe extern "C" fn nif_parse_skin_instance(
    handle: *mut Handle,
    out: *mut NifSkinInstance,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let out = match out_record(borrowed, out, "skin instance") {
            Ok(out) => out,
            Err(code) => return code,
        };
        let parsed = match borrowed.parser.parse_skin_instance() {
            Some(parsed) => parsed,
            None => return no_block(borrowed, "NiSkinInstance"),
        };

        let record = NifSkinInstance {
            skin_data_index: parsed.skin_data_index,
            skin_partition_index: parsed.skin_partition_index,
            skeleton_root_index: parsed.skeleton_root_index,
            bone_count: parsed.bone_node_indices.len() as u32,
            bone_node_indices: parsed.bone_node_indices.as_ptr(),
        };
        // Moving the struct leaves its vector buffers where they are, so the
        // pointer above stays valid for as long as the handle holds it.
        borrowed.skin_instance = Some(parsed);
        *out = record;
        succeed(borrowed)
    })
}

/// Fills `out` with the first `NiSkinData` of the file.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out` must be NULL or one
/// writable, correctly aligned record.
#[no_mangle]
pub unsafe extern "C" fn nif_parse_skin_data(handle: *mut Handle, out: *mut NifSkinData) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let out = match out_record(borrowed, out, "skin data") {
            Ok(out) => out,
            Err(code) => return code,
        };
        let parsed = match borrowed.parser.parse_skin_data() {
            Some(parsed) => parsed,
            None => return no_block(borrowed, "NiSkinData"),
        };

        let records: Vec<NifBoneData> = parsed
            .bone_data
            .iter()
            .map(|bone| NifBoneData {
                skin_transform: flatten_transform(&bone.skin_transform),
                vertex_weight_count: bone.vertex_weights.len() as u32,
                vertex_weights: bone.vertex_weights.as_ptr().cast::<NifSkinVertexWeight>(),
            })
            .collect();
        let record = NifSkinData {
            root_rotation: flatten_rotation(&parsed.root_rotation),
            root_translation: [
                parsed.root_translation.x,
                parsed.root_translation.y,
                parsed.root_translation.z,
            ],
            root_scale: parsed.root_scale,
            num_bones: parsed.num_bones,
            bone_data: records.as_ptr(),
        };

        // The record points into `records`, and every `vertex_weights` pointer
        // into `parsed`; storing both keeps all of them alive together.
        borrowed.skin_data = Some(parsed);
        borrowed.skin_data_records = records;
        *out = record;
        succeed(borrowed)
    })
}

/// Fills `out` with the first `NiSkinPartition` of the file.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out` must be NULL or one
/// writable, correctly aligned record.
#[no_mangle]
pub unsafe extern "C" fn nif_parse_skin_partition(
    handle: *mut Handle,
    out: *mut NifSkinPartition,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let out = match out_record(borrowed, out, "skin partition") {
            Ok(out) => out,
            Err(code) => return code,
        };
        let parsed = match borrowed.parser.parse_skin_partition() {
            Some(parsed) => parsed,
            None => return no_block(borrowed, "NiSkinPartition"),
        };

        let records: Vec<NifPartition> = parsed
            .partitions
            .iter()
            .map(|part| NifPartition {
                num_vertices: part.num_vertices,
                num_triangles: part.num_triangles,
                bone_count: part.bone_palette.len() as u32,
                bone_palette: part.bone_palette.as_ptr(),
                packed_weight_count: part.packed_weights.len() as u32,
                packed_weights: part.packed_weights.as_ptr().cast::<NifPackedWeight>(),
                index_count: part.indices.len() as u32,
                indices: part.indices.as_ptr(),
            })
            .collect();
        let record = NifSkinPartition {
            partition_count: records.len() as u32,
            partitions: records.as_ptr(),
            max_bones_per_partition: parsed.max_bones_per_partition,
            max_bones_per_vertex: parsed.max_bones_per_vertex,
        };

        borrowed.skin_partition = Some(parsed);
        borrowed.skin_partition_records = records;
        *out = record;
        succeed(borrowed)
    })
}

/// Fills `out` with the first `bhkRigidBody` / `bhkRigidBodyT` of the file.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out` must be NULL or one
/// writable, correctly aligned record.
#[no_mangle]
pub unsafe extern "C" fn nif_parse_rigid_body(
    handle: *mut Handle,
    out: *mut NifRigidBody,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let out = match out_record(borrowed, out, "rigid body") {
            Ok(out) => out,
            Err(code) => return code,
        };
        let read = match borrowed.parser.parse_rigid_body() {
            Some(read) => read,
            None => return no_block(borrowed, "bhkRigidBody"),
        };
        // This record carries no pointer, so nothing has to outlive the call.
        *out = build_rigid_body(&read.info);
        if !read.complete {
            // The original reports the same failure only after writing
            // everything it managed to read into the caller's struct, so the
            // record is filled before the failure is reported.
            return fail(
                borrowed,
                NIF_ERROR_INCOMPLETE,
                "the rigid body block stopped partway; out carries what was read",
            );
        }
        succeed(borrowed)
    })
}

/// Fills `out` with the first shape-derived block of the file.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out` must be NULL or one
/// writable, correctly aligned record.
#[no_mangle]
pub unsafe extern "C" fn nif_parse_shape(handle: *mut Handle, out: *mut NifShape) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let out = match out_record(borrowed, out, "shape") {
            Ok(out) => out,
            Err(code) => return code,
        };
        let parsed = match borrowed.parser.parse_shape() {
            Some(parsed) => parsed,
            None => return no_block(borrowed, "bhkShape"),
        };
        let record = build_shape(&parsed);
        borrowed.shape = Some(parsed);
        *out = record;
        succeed(borrowed)
    })
}

/// Fills `out` with the first `bhkCollisionObject` /
/// `bhkBlendCollisionObject`, body and shape already resolved.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out` must be NULL or one
/// writable, correctly aligned record.
#[no_mangle]
pub unsafe extern "C" fn nif_parse_collision(
    handle: *mut Handle,
    out: *mut NifCollisionObject,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let out = match out_record(borrowed, out, "collision object") {
            Ok(out) => out,
            Err(code) => return code,
        };
        let parsed = match borrowed.parser.parse_collision_object() {
            Some(parsed) => parsed,
            None => return no_block(borrowed, "bhkCollisionObject"),
        };

        let record = NifCollisionObject {
            node_index: parsed.node_index,
            rigid_body_index: parsed.rigid_body_index,
            body: build_rigid_body(&parsed.body_info),
            shape: build_shape(&parsed.shape),
        };
        borrowed.collision = Some(parsed);
        *out = record;
        succeed(borrowed)
    })
}

/// Copies one collision string into `out`, snprintf style.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract, and `out` must be NULL or
/// point to `capacity` writable bytes.
#[no_mangle]
pub unsafe extern "C" fn nif_collision_text(
    handle: *mut Handle,
    field: c_int,
    out: *mut c_char,
    capacity: usize,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        if field != NIF_COLL_FIELD_TARGET_NAME {
            return fail(
                borrowed,
                NIF_ERROR_INVALID_ARGUMENT,
                format!("unknown collision text field {field}"),
            );
        }
        let text = match borrowed.collision.as_ref() {
            Some(collision) => collision.target_name.clone(),
            None => return no_block(borrowed, "bhkCollisionObject"),
        };
        let needed = write_c_string(text.as_bytes(), out, capacity);
        set_message(borrowed, "");
        needed
    })
}

// ---------------------------------------------------------------------------
// Animation
//
// The sequence records own nothing: their two strings and their two arrays
// borrow out of handle storage, under the same contract as the geometry and
// skin records. The strings cross as NUL-terminated copies because the C++ side
// reads them as `const char *`.
// ---------------------------------------------------------------------------

/// Copies `text` into `storage` as a NUL-terminated byte string and returns a
/// pointer to the copy. The pointer stays valid while `storage` is not cleared:
/// pushing another entry may move the outer vector's headers, never the inner
/// buffers already handed out.
fn push_string(storage: &mut Vec<Vec<u8>>, text: &str) -> *const c_char {
    let mut bytes = text.as_bytes().to_vec();
    bytes.push(0);
    storage.push(bytes);
    storage.last().expect("just pushed").as_ptr().cast()
}

/// Fills the handle's animation storage from parsed sequences: every string as
/// a NUL-terminated copy, the controlled blocks and text keys as two flat POD
/// arrays, and one sequence record per input pointing into all of it.
///
/// Everything is built in local vectors first and only then moved onto the
/// handle — moving a vector does not move its buffer, so the pointers taken
/// while building stay valid. `target_node_index` and `clip` deliberately do
/// not cross: this wave never fills them, and `resolveControllerReferences()`
/// (still on the C++ side) owns those fields.
fn store_controller_data(borrowed: &mut Handle, sequences: &[ControllerSequence]) {
    let mut strings: Vec<Vec<u8>> = Vec::new();
    let mut blocks: Vec<NifControlledBlock> = Vec::new();
    let mut text_keys: Vec<NifTextKey> = Vec::new();

    /// Per-sequence location of its slices, resolved after the arrays stop growing.
    struct Placement {
        name: *const c_char,
        target_name: *const c_char,
        block_start: usize,
        block_len: usize,
        text_key_start: usize,
        text_key_len: usize,
    }

    let mut placements = Vec::with_capacity(sequences.len());
    for sequence in sequences {
        let name = push_string(&mut strings, &sequence.name);
        let target_name = push_string(&mut strings, &sequence.target_name);

        let block_start = blocks.len();
        for block in &sequence.controlled_blocks {
            blocks.push(NifControlledBlock {
                interpolator_index: block.interpolator_index,
                controller_index: block.controller_index,
                priority: block.priority,
                string_palette_index: block.string_palette_index,
                node_name_offset: block.node_name_offset,
                property_type_offset: block.property_type_offset,
                controller_type_offset: block.controller_type_offset,
                controller_id_offset: block.controller_id_offset,
                interpolator_id_offset: block.interpolator_id_offset,
                keyframe_data_index: block.keyframe_data_index,
                resolved_bone_index: block.resolved_bone_index,
            });
        }

        let text_key_start = text_keys.len();
        for key in &sequence.text_keys {
            let value = push_string(&mut strings, &key.value);
            text_keys.push(NifTextKey {
                time: key.time,
                value,
            });
        }

        placements.push(Placement {
            name,
            target_name,
            block_start,
            block_len: blocks.len() - block_start,
            text_key_start,
            text_key_len: text_keys.len() - text_key_start,
        });
    }

    // The arrays have stopped growing, so the slice pointers are stable now.
    let records: Vec<NifControllerSequence> = sequences
        .iter()
        .zip(&placements)
        .map(|(sequence, placement)| NifControllerSequence {
            name: placement.name,
            target_name: placement.target_name,
            controlled_blocks: if placement.block_len > 0 {
                // SAFETY: the offset plus the length stays within `blocks`,
                // which stopped growing before the pointer was formed.
                unsafe { blocks.as_ptr().add(placement.block_start) }
            } else {
                core::ptr::null()
            },
            text_keys: if placement.text_key_len > 0 {
                // SAFETY: same argument against `text_keys`.
                unsafe { text_keys.as_ptr().add(placement.text_key_start) }
            } else {
                core::ptr::null()
            },
            controller_manager_index: sequence.controller_manager_index,
            duration: sequence.duration,
            frequency: sequence.frequency,
            phase: sequence.phase,
            start_time: sequence.start_time,
            stop_time: sequence.stop_time,
            is_loop: sequence.is_loop as u32,
            controlled_block_count: placement.block_len as u32,
            text_key_count: placement.text_key_len as u32,
        })
        .collect();

    // Moving the vectors into the handle leaves every buffer where it is, so
    // the pointers inside `records` (and the strings') survive the move.
    borrowed.controller_strings = strings;
    borrowed.controller_blocks = blocks;
    borrowed.controller_text_keys = text_keys;
    borrowed.controller_sequences = records;
}

/// Fills `out` with the first `NiControllerManager` of the file.
///
/// `controller_sequence_count` is what the file declares; `sequence_count` and
/// `sequences` describe what was actually read — the original logs a sequence
/// that fails to parse and keeps the rest, so the two can differ. On failure
/// the out record is left untouched, where the original kept whatever it had
/// read before returning false; no caller reads the record on `false`.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out` must be NULL or one
/// writable, correctly aligned record.
#[no_mangle]
pub unsafe extern "C" fn nif_parse_controller_manager(
    handle: *mut Handle,
    out: *mut NifControllerManager,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let out = match out_record(borrowed, out, "controller manager") {
            Ok(out) => out,
            Err(code) => return code,
        };
        let parsed = match borrowed.parser.parse_controller_manager() {
            Some(parsed) => parsed,
            None => return no_block(borrowed, "NiControllerManager"),
        };

        store_controller_data(borrowed, &parsed.sequences);
        let record = NifControllerManager {
            controller_sequence_count: parsed.controller_sequence_count,
            object_palette_index: parsed.object_palette_index,
            last_time: parsed.last_time,
            sequence_count: borrowed.controller_sequences.len() as u32,
            sequences: borrowed.controller_sequences.as_ptr(),
        };
        *out = record;
        succeed(borrowed)
    })
}

/// Fills `out` with the first `NiControllerSequence` of the file, without the
/// manager around it — the standalone entry the original exposes next to
/// `parseNiControllerManager`. Storage rules are the manager's.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out` must be NULL or one
/// writable, correctly aligned record.
#[no_mangle]
pub unsafe extern "C" fn nif_parse_controller_sequence(
    handle: *mut Handle,
    out: *mut NifControllerSequence,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let out = match out_record(borrowed, out, "controller sequence") {
            Ok(out) => out,
            Err(code) => return code,
        };
        let parsed = match borrowed.parser.parse_controller_sequence() {
            Some(parsed) => parsed,
            None => return no_block(borrowed, "NiControllerSequence"),
        };

        store_controller_data(borrowed, std::slice::from_ref(&parsed));
        let record = match borrowed.controller_sequences.first() {
            Some(record) => *record,
            None => {
                return fail(
                    borrowed,
                    NIF_ERROR_NO_BLOCK,
                    "the stored controller sequence record is missing",
                )
            }
        };
        *out = record;
        succeed(borrowed)
    })
}

// ---------------------------------------------------------------------------
// Scene nodes and header — the range wave
//
// The block-range queries the C++ parser's inline getters expose through
// getNodes(), getRootNodeIndices() and getHeader(). Every value crosses
// exactly as the original left it: parent indices stay -1 and child lists stay
// empty, because neither the shipped parser nor the corpus ever wired the
// scene graph (see `NifNodeInfo`). The caller copies; these calls only write
// the message slot, so a shim may interleave them with its copying loop the
// way the original's per-object walk worked.
// ---------------------------------------------------------------------------

/// Fills `out` with one object-array entry: index, parent, block type and the
/// geometry flag the C++ walk derived.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out` must be NULL or one
/// writable, correctly aligned `NifNodeInfo`.
#[no_mangle]
pub unsafe extern "C" fn nif_get_node_info(
    handle: *mut Handle,
    node_index: u32,
    out: *mut NifNodeInfo,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let out = match out_record(borrowed, out, "node") {
            Ok(out) => out,
            Err(code) => return code,
        };
        let node = match borrowed.parser.nodes().get(node_index as usize) {
            Some(node) => node,
            None => {
                return fail(
                    borrowed,
                    NIF_ERROR_INVALID_ARGUMENT,
                    format!("node index {node_index} is out of range"),
                )
            }
        };
        let record = NifNodeInfo {
            node_index: node.node_index,
            parent_index: node.parent_index,
            block_type_index: u32::from(node.block_type_index),
            has_geometry: u32::from(node.has_geometry),
        };
        *out = record;
        succeed(borrowed)
    })
}

/// Copies one node's string into `out`, snprintf style. `field` selects the
/// object name or the block's resolved type name; the return is the length the
/// text needs with its terminator.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out` must be NULL or at
/// least `capacity` writable bytes when `capacity` is non-zero.
#[no_mangle]
pub unsafe extern "C" fn nif_node_text(
    handle: *mut Handle,
    node_index: u32,
    field: c_int,
    out: *mut c_char,
    capacity: usize,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        if field != NIF_NODE_FIELD_NAME && field != NIF_NODE_FIELD_BLOCK_TYPE_NAME {
            return fail(
                borrowed,
                NIF_ERROR_INVALID_ARGUMENT,
                format!("unknown node text field {field}"),
            );
        }
        let text = borrowed
            .parser
            .nodes()
            .get(node_index as usize)
            .map(|node| match field {
                NIF_NODE_FIELD_BLOCK_TYPE_NAME => node.block_type_name.clone(),
                _ => node.name.clone(),
            });
        match text {
            Some(text) => {
                let needed = write_c_string(text.as_bytes(), out, capacity);
                set_message(borrowed, "");
                needed
            }
            None => fail(
                borrowed,
                NIF_ERROR_INVALID_ARGUMENT,
                format!("node index {node_index} is out of range"),
            ),
        }
    })
}

/// The number of root nodes — every node whose parent was never wired, which
/// is all of them in a file the original parser handled.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract.
#[no_mangle]
pub unsafe extern "C" fn nif_root_node_count(handle: *mut Handle) -> u32 {
    guard(0, handle, |borrowed| {
        borrowed.parser.root_node_indices().len() as u32
    })
}

/// Writes the node index of one root to `out_index`, mirroring
/// `getRootNodeIndices()[i]`.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out_index` must be NULL
/// or one writable `u32`.
#[no_mangle]
pub unsafe extern "C" fn nif_root_node_at(
    handle: *mut Handle,
    root_index: u32,
    out_index: *mut u32,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        if out_index.is_null() {
            return fail(
                borrowed,
                NIF_ERROR_INVALID_ARGUMENT,
                "the root index out-parameter is NULL",
            );
        }
        let index = borrowed
            .parser
            .root_node_indices()
            .get(root_index as usize)
            .copied();
        match index {
            Some(index) => {
                *out_index = index;
                succeed(borrowed)
            }
            None => fail(
                borrowed,
                NIF_ERROR_INVALID_ARGUMENT,
                format!("root index {root_index} is out of range"),
            ),
        }
    })
}

/// Fills `out` with the parsed header. The block-type index table **borrows**
/// from the handle under [`nif_geometry_info`]'s storage rules: valid until the
/// next parse on this handle.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out` must be NULL or one
/// writable, correctly aligned `NifHeaderInfo`.
#[no_mangle]
pub unsafe extern "C" fn nif_get_header_info(handle: *mut Handle, out: *mut NifHeaderInfo) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let out = match out_record(borrowed, out, "header") {
            Ok(out) => out,
            Err(code) => return code,
        };
        let header = borrowed.parser.header();
        let record = NifHeaderInfo {
            version: header.version,
            endian: header.endian,
            has_endian_byte: u8::from(header.has_endian_byte),
            legacy_net_immerse: u8::from(header.legacy_net_immerse),
            has_block_type_table: u8::from(header.has_block_type_table),
            user_version: header.user_version,
            num_objects: header.num_objects,
            unknown_field: header.unknown_field,
            num_groups: header.num_groups,
            block_type_count: header.block_type_indices.len() as u32,
            block_type_indices: header.block_type_indices.as_ptr(),
            block_data_offset: header.block_data_offset as u64,
        };
        *out = record;
        succeed(borrowed)
    })
}

/// Copies one header string into `out`, snprintf style: the magic line, the
/// creator, or either script name.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out` must be NULL or at
/// least `capacity` writable bytes when `capacity` is non-zero.
#[no_mangle]
pub unsafe extern "C" fn nif_header_text(
    handle: *mut Handle,
    field: c_int,
    out: *mut c_char,
    capacity: usize,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let text = match field {
            NIF_HEADER_FIELD_MAGIC => borrowed.parser.header().magic.clone(),
            NIF_HEADER_FIELD_CREATOR => borrowed.parser.header().creator.clone(),
            NIF_HEADER_FIELD_PROCESS_SCRIPT => borrowed.parser.header().process_script.clone(),
            NIF_HEADER_FIELD_EXPORT_SCRIPT => borrowed.parser.header().export_script.clone(),
            _ => {
                return fail(
                    borrowed,
                    NIF_ERROR_INVALID_ARGUMENT,
                    format!("unknown header text field {field}"),
                )
            }
        };
        let needed = write_c_string(text.as_bytes(), out, capacity);
        set_message(borrowed, "");
        needed
    })
}

/// Copies one block-type name into `out`, snprintf style. `type_index` indexes
/// the header's block-type name table, not the object array.
///
/// # Safety
/// `handle` must satisfy [`nif_create`]'s contract; `out` must be NULL or at
/// least `capacity` writable bytes when `capacity` is non-zero.
#[no_mangle]
pub unsafe extern "C" fn nif_header_block_type_name(
    handle: *mut Handle,
    type_index: u32,
    out: *mut c_char,
    capacity: usize,
) -> c_int {
    guard(NIF_ERROR_INVALID_ARGUMENT, handle, |borrowed| {
        let text = borrowed
            .parser
            .header()
            .block_type_names
            .get(type_index as usize)
            .cloned();
        match text {
            Some(text) => {
                let needed = write_c_string(text.as_bytes(), out, capacity);
                set_message(borrowed, "");
                needed
            }
            None => fail(
                borrowed,
                NIF_ERROR_INVALID_ARGUMENT,
                format!("block type index {type_index} is out of range"),
            ),
        }
    })
}
