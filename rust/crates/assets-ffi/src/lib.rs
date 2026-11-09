//! C ABI over the `assets-core` crate.
//!
//! Exposes the game-path expansion and the NIF geometry merge to C and C++.
//! There are no handles here: every call is a pure function over caller-owned
//! buffers, so nothing is allocated on the Rust side and nothing needs to be
//! released.
//!
//! Paths cross the boundary as raw bytes rather than as NUL-terminated C strings,
//! because ESM records are not guaranteed to be valid UTF-8 and because a
//! candidate list is a packed run of NUL-terminated strings rather than one
//! string.
//!
//! Threading: each thread has its own last-error state, so `afl_last_error_code`
//! and `afl_last_error_message` only describe calls made on the calling thread.
//!
//! Strings copied into caller buffers follow the snprintf convention: the return
//! value is the length in bytes including the terminating NUL, so a caller
//! detects truncation by comparing it against the buffer capacity. Nothing is
//! written past `capacity` bytes, and a NULL `out` or a zero `capacity` queries
//! the required length without writing anything.

use core::mem::{align_of, size_of};
use core::panic::AssertUnwindSafe;
use core::slice;
use std::cell::RefCell;
use std::ffi::{c_char, c_int};
use std::panic::catch_unwind;

use assets_core::{
    asset_root_folders, build_asset_path_candidates, merge_geometry, GeometryInput, VERTEX_FLOATS,
};

/// The call succeeded.
pub const AFL_OK: c_int = 0;
/// A required pointer argument was NULL, or a pointer was misaligned.
pub const AFL_ERROR_INVALID_ARGUMENT: c_int = -1;
/// A Rust panic was caught at the boundary; the call had no effect.
pub const AFL_ERROR_PANIC: c_int = -7;
/// An output buffer cannot hold the result; the out-parameter holds the size.
pub const AFL_ERROR_BUFFER_TOO_SMALL: c_int = -8;

/// A static, NUL-terminated version banner.
const VERSION: &str = concat!("assets-ffi ", env!("CARGO_PKG_VERSION"), "\0");

/// One decoded geometry block, as the C caller spells it.
///
/// The layout mirrors `afl_geometry_input` in `include/assets_ffi.h` field for
/// field, and is only ever read through a reference to that struct, so the
/// compiler-generated layout is the contract.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct AflGeometryInput {
    pub positions: *const f32,
    pub position_count: usize,
    pub normals: *const f32,
    pub normal_count: usize,
    pub tex_coords: *const f32,
    pub tex_coord_count: usize,
    pub colors: *const f32,
    pub color_count: usize,
    pub triangles: *const u16,
    pub triangle_count: usize,
}

/// What `afl_merge_geometry` produced, or would need.
///
/// The layout mirrors `afl_merge_counts` in `include/assets_ffi.h`.
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct AflMergeCounts {
    pub vertex_count: usize,
    pub index_count: usize,
    pub geometry_count: usize,
    pub skipped_geometry_count: usize,
}

// Both structs are arrays of pointers and counts on the C side too, so a mismatch
// would only come from a bad `size_t` mapping rather than from padding. The
// asserts are here so a wrong assumption fails the build instead of the caller.
const _: () = assert!(size_of::<c_int>() == 4);
const _: () = assert!(size_of::<u32>() == 4);
const _: () = assert!(size_of::<AflGeometryInput>() == 10 * size_of::<usize>());
const _: () = assert!(align_of::<AflGeometryInput>() == align_of::<usize>());
const _: () = assert!(size_of::<AflMergeCounts>() == 4 * size_of::<usize>());
const _: () = assert!(align_of::<AflMergeCounts>() == align_of::<usize>());

#[derive(Default)]
struct ErrorState {
    code: c_int,
    message: String,
}

thread_local! {
    static LAST_ERROR: RefCell<ErrorState> = RefCell::new(ErrorState::default());
}

fn set_error(code: c_int, message: impl Into<String>) {
    LAST_ERROR.with(|state| {
        let mut state = state.borrow_mut();
        state.code = code;
        state.message = message.into();
    });
}

fn clear_error() {
    set_error(AFL_OK, String::new());
}

/// Run `body`, turning a caught panic into the panic code plus `fallback`.
///
/// Every exported function routes through here: a Rust panic must not unwind
/// into the C++ frames above, which are not built to take it.
fn guard_value<T: Copy>(fallback: T, body: impl FnOnce() -> T) -> T {
    match catch_unwind(AssertUnwindSafe(body)) {
        Ok(value) => value,
        Err(_) => {
            set_error(AFL_ERROR_PANIC, "panic caught at the assets-ffi boundary");
            fallback
        }
    }
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

/// Borrow `count` contiguous `T` values from a caller pointer.
///
/// A zero count yields an empty slice whatever the pointer is, which keeps a
/// geometry with no colours from having to pass a dummy non-NULL pointer. Any
/// other count demands a non-NULL, correctly aligned pointer: reading through a
/// misaligned one is undefined behaviour, and rejecting it here turns a caller
/// bug into a status code instead.
///
/// # Safety
/// The caller must guarantee that `pointer` addresses `count` initialised `T`
/// values that stay alive and unaliased for `'a`.
unsafe fn borrow_slice<'a, T>(pointer: *const T, count: usize) -> Result<&'a [T], c_int> {
    if count == 0 {
        return Ok(&[]);
    }
    if pointer.is_null() || (pointer as usize) % align_of::<T>() != 0 {
        return Err(AFL_ERROR_INVALID_ARGUMENT);
    }
    // SAFETY: the caller's contract, plus the checks above.
    Ok(slice::from_raw_parts(pointer, count))
}

/// Borrow `count` writable `T` values from a caller pointer, for output.
///
/// # Safety
/// The caller must guarantee that `pointer` addresses `count` writable, correctly
/// aligned `T` values that stay alive and unaliased for `'a`.
unsafe fn borrow_slice_mut<'a, T>(pointer: *mut T, count: usize) -> Result<&'a mut [T], c_int> {
    if count == 0 {
        return Ok(&mut []);
    }
    if pointer.is_null() || (pointer as usize) % align_of::<T>() != 0 {
        return Err(AFL_ERROR_INVALID_ARGUMENT);
    }
    // SAFETY: the caller's contract, plus the checks above.
    Ok(slice::from_raw_parts_mut(pointer, count))
}

/// It is an error to name a buffer without saying how big it is.
fn buffer_too_small(what: &str) -> c_int {
    set_error(
        AFL_ERROR_BUFFER_TOO_SMALL,
        format!("the {what} buffer is too small for the result"),
    );
    AFL_ERROR_BUFFER_TOO_SMALL
}

fn invalid_argument(what: &str) -> c_int {
    set_error(AFL_ERROR_INVALID_ARGUMENT, what);
    AFL_ERROR_INVALID_ARGUMENT
}

/// Marshal the C geometry array into borrows of the caller's own memory.
///
/// # Safety
/// Every pointer in `geometries` must satisfy [`borrow_slice`] for `'a`.
unsafe fn collect_inputs<'a>(
    geometries: *const AflGeometryInput,
    geometry_count: usize,
) -> Result<Vec<GeometryInput<'a>>, c_int> {
    if geometry_count == 0 {
        return Ok(Vec::new());
    }
    if geometries.is_null() || (geometries as usize) % align_of::<AflGeometryInput>() != 0 {
        return Err(AFL_ERROR_INVALID_ARGUMENT);
    }
    // SAFETY: the caller's contract, plus the checks above.
    let raw = slice::from_raw_parts(geometries, geometry_count);

    let mut inputs = Vec::with_capacity(geometry_count);
    for entry in raw {
        inputs.push(GeometryInput {
            positions: borrow_slice(entry.positions.cast::<[f32; 3]>(), entry.position_count)?,
            normals: borrow_slice(entry.normals.cast::<[f32; 3]>(), entry.normal_count)?,
            tex_coords: borrow_slice(entry.tex_coords.cast::<[f32; 2]>(), entry.tex_coord_count)?,
            colors: borrow_slice(entry.colors.cast::<[f32; 4]>(), entry.color_count)?,
            triangles: borrow_slice(entry.triangles.cast::<[u16; 3]>(), entry.triangle_count)?,
        });
    }
    Ok(inputs)
}

/// The type folders that sit directly under `Data\`. Always 9.
#[no_mangle]
pub extern "C" fn afl_asset_root_folder_count() -> usize {
    asset_root_folders().len()
}

/// The root folder at `index`, in lookup-priority order (snprintf style).
///
/// # Safety
/// `out` must point to `capacity` writable bytes or be NULL.
#[no_mangle]
pub unsafe extern "C" fn afl_asset_root_folder(
    index: usize,
    out: *mut c_char,
    capacity: usize,
) -> c_int {
    guard_value(AFL_ERROR_PANIC, || {
        clear_error();
        let folders = asset_root_folders();
        match folders.get(index) {
            None => invalid_argument("the root folder index is out of range"),
            Some(folder) => write_c_string(folder.as_bytes(), out, capacity),
        }
    })
}

/// A static, NUL-terminated version banner, for logs and crash reports.
#[no_mangle]
pub extern "C" fn afl_version() -> *const c_char {
    VERSION.as_ptr().cast::<c_char>()
}

/// The error code left by the last computing call on this thread.
#[no_mangle]
pub extern "C" fn afl_last_error_code() -> c_int {
    LAST_ERROR.with(|state| state.borrow().code)
}

/// The message left by the last computing call on this thread (snprintf style).
///
/// # Safety
/// `out` must point to `capacity` writable bytes or be NULL.
#[no_mangle]
pub unsafe extern "C" fn afl_last_error_message(out: *mut c_char, capacity: usize) -> c_int {
    LAST_ERROR.with(|state| write_c_string(state.borrow().message.as_bytes(), out, capacity))
}

/// Expand `normalized` into the spellings that can resolve, most likely first.
///
/// # Safety
/// `normalized` must point to `normalized_length` readable bytes or be NULL with
/// a zero length; `out` must point to `capacity` writable bytes or be NULL;
/// `out_count` and `out_bytes` must be non-NULL and writable.
#[no_mangle]
pub unsafe extern "C" fn afl_build_asset_path_candidates(
    normalized: *const c_char,
    normalized_length: usize,
    out: *mut c_char,
    capacity: usize,
    out_count: *mut usize,
    out_bytes: *mut usize,
) -> c_int {
    guard_value(AFL_ERROR_PANIC, || {
        clear_error();
        if out_count.is_null() || out_bytes.is_null() {
            return invalid_argument("out_count and out_bytes must not be NULL");
        }
        let input = if normalized_length == 0 {
            &[][..]
        } else if normalized.is_null() {
            return invalid_argument("normalized is NULL but normalized_length is not zero");
        } else {
            // SAFETY: the caller's contract, plus the NULL check above.
            slice::from_raw_parts(normalized.cast::<u8>(), normalized_length)
        };

        let candidates = build_asset_path_candidates(input);
        // Every candidate is NUL-terminated in the packed block, so the caller
        // can walk it without a second array.
        let total_bytes: usize = candidates.iter().map(|candidate| candidate.len() + 1).sum();

        *out_count = candidates.len();
        *out_bytes = total_bytes;

        if out.is_null() || capacity == 0 {
            return AFL_OK; // A length query.
        }
        if capacity < total_bytes {
            return buffer_too_small("candidate");
        }

        let mut offset = 0;
        for candidate in &candidates {
            // SAFETY: `capacity >= total_bytes` and `offset < total_bytes` here,
            // so every write lands inside the caller's buffer.
            core::ptr::copy_nonoverlapping(
                candidate.as_ptr().cast::<c_char>(),
                out.add(offset),
                candidate.len(),
            );
            offset += candidate.len();
            *out.add(offset) = 0;
            offset += 1;
        }
        AFL_OK
    })
}

/// Merge `geometry_count` blocks into one vertex array and one index array.
///
/// # Safety
/// `geometries` must point to `geometry_count` readable `afl_geometry_input`
/// values (or be NULL when the count is zero), every pointer inside them must
/// satisfy the same contract, `out_vertices` must point to `vertex_capacity`
/// writable floats or be NULL, `out_indices` must point to `index_capacity`
/// writable uint32 values or be NULL, and `out_counts` must be non-NULL and
/// writable.
#[no_mangle]
pub unsafe extern "C" fn afl_merge_geometry(
    geometries: *const AflGeometryInput,
    geometry_count: usize,
    out_vertices: *mut f32,
    vertex_capacity: usize,
    out_indices: *mut u32,
    index_capacity: usize,
    out_counts: *mut AflMergeCounts,
) -> c_int {
    guard_value(AFL_ERROR_PANIC, || {
        clear_error();
        if out_counts.is_null() {
            return invalid_argument("out_counts must not be NULL");
        }

        let inputs = match collect_inputs(geometries, geometry_count) {
            Ok(inputs) => inputs,
            Err(code) => {
                set_error(code, "a geometry pointer is NULL or misaligned");
                return code;
            }
        };
        let merged = merge_geometry(&inputs);
        let counts = AflMergeCounts {
            vertex_count: merged.vertices.len(),
            index_count: merged.indices.len(),
            geometry_count: merged.geometry_count,
            skipped_geometry_count: merged.skipped_geometry_count,
        };

        // A caller that offers no writable space wants the sizes and nothing
        // else. Offering any space at all commits to a full transfer, which is
        // what keeps an empty result from being mistaken for a length query.
        let explicit = (!out_vertices.is_null() && vertex_capacity > 0)
            || (!out_indices.is_null() && index_capacity > 0);
        if !explicit {
            *out_counts = counts;
            return AFL_OK;
        }

        // Check both capacities before either write, so a short buffer leaves
        // nothing half-written behind. `vertex_capacity` counts floats, and each
        // vertex is VERTEX_FLOATS of them, so compare against the float total --
        // comparing against the vertex count would let a caller who sized the
        // buffer in vertices pass the check and then be overrun.
        if vertex_capacity < merged.vertices.len() * VERTEX_FLOATS {
            *out_counts = counts;
            return buffer_too_small("vertex");
        }
        if index_capacity < merged.indices.len() {
            *out_counts = counts;
            return buffer_too_small("index");
        }

        let vertices = match borrow_slice_mut(
            out_vertices.cast::<[f32; VERTEX_FLOATS]>(),
            merged.vertices.len(),
        ) {
            Ok(vertices) => vertices,
            Err(code) => {
                set_error(code, "out_vertices is NULL or misaligned");
                return code;
            }
        };
        vertices.copy_from_slice(&merged.vertices);

        let indices = match borrow_slice_mut(out_indices, merged.indices.len()) {
            Ok(indices) => indices,
            Err(code) => {
                set_error(code, "out_indices is NULL or misaligned");
                return code;
            }
        };
        indices.copy_from_slice(&merged.indices);

        *out_counts = counts;
        AFL_OK
    })
}
