//! C ABI for the Oblivion ESM/ESP container parser.
//!
//! [`esm_parser`] indexes a plugin into records and decodes record bodies into
//! subrecords. This crate exposes that index to C and C++ callers so the
//! Android native layer can replace the container handling in
//! `assets/esm_reader.cpp` with these entry points. The matching header is
//! `include/esm_ffi.h`.
//!
//! # Memory model
//!
//! * `esm_plugin_open` reads the file and owns the bytes.
//! * `esm_plugin_parse_memory` borrows the caller's buffer: it must stay valid
//!   and unmodified until `esm_plugin_free`. Nothing is copied, which is what
//!   a 277 MB `Oblivion.esm` loaded into Android memory needs.
//! * `esm_plugin_decode_*` return an [`EsmPayload`] that owns the decoded
//!   subrecords until `esm_payload_free`. Subrecord views point into that
//!   payload, so freeing it invalidates every view taken from it.
//! * Text is copied into caller buffers, snprintf style. The only pointers the
//!   caller has to free are plugin, payload and type-count handles.
//!
//! # Threading
//!
//! Handles are immutable once created, so one plugin may be read from several
//! threads concurrently. It must not be freed while another thread is using it.
//!
//! # Error handling
//!
//! No entry point unwinds. Failures return a sentinel (NULL, 0 or a negative
//! count) and record a code plus a message for the calling thread, which
//! `esm_last_error_code` and `esm_last_error_message` read back.

use std::cell::RefCell;
use std::ffi::{c_char, c_int, CStr};
use std::fs;
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::path::Path;
use std::ptr;
use std::slice;

use esm_parser::{EsmError, Plugin, RecordIndex, SubRecord};

/// The call succeeded.
pub const ESM_OK: c_int = 0;
/// A required pointer argument was NULL.
pub const ESM_ERROR_INVALID_ARGUMENT: c_int = -1;
/// The plugin file could not be read.
pub const ESM_ERROR_IO: c_int = -2;
/// The plugin ends before a record, group or subrecord is complete.
pub const ESM_ERROR_TRUNCATED: c_int = -3;
/// The buffer does not start with a TES4 record.
pub const ESM_ERROR_BAD_HEADER: c_int = -4;
/// A compressed record could not be inflated.
pub const ESM_ERROR_INFLATE: c_int = -5;
/// No record matched the requested FormID or index.
pub const ESM_ERROR_NOT_FOUND: c_int = -6;
/// A Rust panic was caught at the boundary; the call had no effect.
pub const ESM_ERROR_PANIC: c_int = -7;

const VERSION: &str = concat!("esm-ffi ", env!("CARGO_PKG_VERSION"), "\0");

/// One indexed record, mirrored field for field by `EsmRecordInfo` in the
/// header. `compressed` is 0 or 1 so that the struct has no padding holes.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct EsmRecordInfo {
    pub form_id: u32,
    pub flags: u32,
    /// Payload size as stored in the record header; when `compressed` is 1 it
    /// includes the 4-byte decompressed-size prefix.
    pub data_size: u32,
    /// Decompressed payload size, or 0 when the record is not compressed.
    pub decomp_size: u32,
    /// FormID of the nearest enclosing cell-children group (type 6). This is
    /// how LAND, PGRD, REFR, ACHR and ACRE records are attributed to a cell.
    pub parent_form_id: u32,
    /// FormID of the nearest enclosing world-children group (type 1), i.e. the
    /// worldspace of an exterior CELL. 0 for interior content.
    pub worldspace_form_id: u32,
    /// Offset of the payload, i.e. just past the 20-byte record header.
    pub file_offset: u64,
    /// 1 when the payload is a zlib stream, otherwise 0.
    pub compressed: u32,
    /// Record type tag, seen by C as `char rec_type[4]`.
    pub rec_type: [u8; 4],
}

/// A borrowed view of one subrecord inside an [`EsmPayload`].
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct EsmSubRecordView {
    /// 4-character tag, seen by C as `char tag[4]`.
    pub tag: [u8; 4],
    pub size: u32,
    /// Points into the payload; valid until `esm_payload_free`. Never NULL,
    /// but a zero-sized subrecord has nothing to read at it.
    pub data: *const u8,
}

// The header mirrors these layouts, so a mismatch would silently corrupt every
// caller. Pinning the sizes here makes a 32-bit build (armeabi-v7a) reject the
// layout at compile time instead of failing on the device.
const _: () = assert!(core::mem::size_of::<EsmRecordInfo>() == 40);
const _: () = assert!(core::mem::align_of::<EsmRecordInfo>() == 8);
const _: () = assert!(
    core::mem::size_of::<EsmSubRecordView>()
        == if cfg!(target_pointer_width = "64") {
            16
        } else {
            12
        }
);

/// An indexed plugin. Created by `esm_plugin_open` or
/// `esm_plugin_parse_memory` and released with `esm_plugin_free`.
pub struct EsmPlugin {
    plugin: Plugin,
    /// Bytes read by `esm_plugin_open`; `None` when the caller supplied them.
    owned: Option<Vec<u8>>,
    /// Caller-supplied buffer used when `owned` is `None`.
    borrowed: Borrowed,
}

/// A buffer the caller keeps alive for the lifetime of the plugin handle.
#[derive(Clone, Copy)]
struct Borrowed {
    ptr: *const u8,
    len: usize,
}

impl Borrowed {
    const EMPTY: Self = Self {
        ptr: ptr::null(),
        len: 0,
    };
}

impl EsmPlugin {
    fn data(&self) -> &[u8] {
        match &self.owned {
            Some(bytes) => bytes,
            None if self.borrowed.ptr.is_null() => &[],
            None => {
                // SAFETY: `esm_plugin_parse_memory` documents that the buffer
                // stays valid and unmodified until `esm_plugin_free`, which is
                // the only place that drops this handle.
                unsafe { slice::from_raw_parts(self.borrowed.ptr, self.borrowed.len) }
            }
        }
    }
}

/// The decoded subrecords of one record or of the TES4 header.
pub struct EsmPayload {
    subrecords: Vec<SubRecord>,
}

/// Record counts per type, ordered by descending count.
pub struct EsmTypeCounts {
    counts: Vec<(String, usize)>,
}

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
    set_error(ESM_OK, String::new());
}

/// Map a parser failure onto the ABI error code and record its message.
fn report(err: &EsmError) -> c_int {
    let code = match err {
        EsmError::Truncated { .. } => ESM_ERROR_TRUNCATED,
        EsmError::BadHeader { .. } => ESM_ERROR_BAD_HEADER,
        EsmError::Inflate { .. } => ESM_ERROR_INFLATE,
    };
    set_error(code, err.to_string());
    code
}

/// Run `body`, turning a caught panic into an error code plus a null result.
fn guard_ptr<T>(body: impl FnOnce() -> *mut T) -> *mut T {
    match catch_unwind(AssertUnwindSafe(body)) {
        Ok(value) => value,
        Err(_) => {
            set_error(ESM_ERROR_PANIC, "panic caught at the esm-ffi boundary");
            ptr::null_mut()
        }
    }
}

/// Run `body`, turning a caught panic into an error code plus `fallback`.
fn guard_value<T: Copy>(fallback: T, body: impl FnOnce() -> T) -> T {
    match catch_unwind(AssertUnwindSafe(body)) {
        Ok(value) => value,
        Err(_) => {
            set_error(ESM_ERROR_PANIC, "panic caught at the esm-ffi boundary");
            fallback
        }
    }
}

/// Copy `text` into a caller buffer, snprintf style: the return value is the
/// length in bytes including the terminating NUL, so a caller can detect
/// truncation, and nothing is written beyond `capacity`.
fn write_c_string(text: &str, out: *mut c_char, capacity: usize) -> c_int {
    let needed = text.len() + 1;
    if !out.is_null() && capacity > 0 {
        let copied = text.len().min(capacity - 1);
        // SAFETY: the caller guarantees `capacity` writable bytes at `out`.
        unsafe {
            ptr::copy_nonoverlapping(text.as_ptr().cast::<c_char>(), out, copied);
            *out.add(copied) = 0;
        }
    }
    needed as c_int
}

/// Borrow a plugin handle. `None` (with the error set) when it is NULL.
///
/// # Safety
/// `handle` must be NULL or a live handle from `esm_plugin_open` /
/// `esm_plugin_parse_memory` that has not been freed.
unsafe fn plugin_ref<'a>(handle: *const EsmPlugin) -> Option<&'a EsmPlugin> {
    if handle.is_null() {
        set_error(ESM_ERROR_INVALID_ARGUMENT, "plugin handle is NULL");
        return None;
    }
    // SAFETY: the caller guarantees a live handle.
    Some(unsafe { &*handle })
}

/// Borrow a payload handle. `None` (with the error set) when it is NULL.
///
/// # Safety
/// `payload` must be NULL or a live handle from one of the `esm_plugin_decode_*`
/// functions that has not been freed.
unsafe fn payload_ref<'a>(payload: *const EsmPayload) -> Option<&'a EsmPayload> {
    if payload.is_null() {
        set_error(ESM_ERROR_INVALID_ARGUMENT, "payload handle is NULL");
        return None;
    }
    // SAFETY: the caller guarantees a live handle.
    Some(unsafe { &*payload })
}

fn payload_from(subrecords: Vec<SubRecord>) -> *mut EsmPayload {
    Box::into_raw(Box::new(EsmPayload { subrecords }))
}

/// Read the 4-byte tag from a C string, rejecting shorter input.
///
/// # Safety
/// `tag` must be NULL or point to at least 4 readable bytes.
unsafe fn tag_from(tag: *const c_char) -> Option<[u8; 4]> {
    if tag.is_null() {
        set_error(ESM_ERROR_INVALID_ARGUMENT, "tag is NULL");
        return None;
    }
    // SAFETY: the caller guarantees 4 readable bytes.
    Some(
        unsafe { slice::from_raw_parts(tag.cast::<u8>(), 4) }
            .try_into()
            .unwrap(),
    )
}

fn record_info(index: &RecordIndex) -> EsmRecordInfo {
    EsmRecordInfo {
        form_id: index.form_id,
        flags: index.flags,
        data_size: index.data_size,
        decomp_size: index.decomp_size,
        parent_form_id: index.parent_form_id,
        worldspace_form_id: index.worldspace_form_id,
        file_offset: index.file_offset,
        compressed: u32::from(index.compressed),
        rec_type: index.rec_type,
    }
}

/// Build a payload by decoding one indexed record.
fn decode_index(plugin: &EsmPlugin, index: &RecordIndex) -> *mut EsmPayload {
    match plugin.plugin.decode_record(plugin.data(), index) {
        Ok(subrecords) => payload_from(subrecords),
        Err(err) => {
            report(&err);
            ptr::null_mut()
        }
    }
}

// ============================================================================
// Lifecycle
// ============================================================================

/// Index a plugin that the caller already holds in memory.
///
/// `name` is the file name reported by `esm_plugin_file_name`; it may be NULL.
///
/// # Safety
/// `data` must point to `len` readable bytes that stay valid and unmodified
/// until the returned handle is freed. Returns NULL on failure.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_parse_memory(
    name: *const c_char,
    data: *const u8,
    len: usize,
) -> *mut EsmPlugin {
    guard_ptr(|| {
        clear_error();
        if data.is_null() {
            set_error(ESM_ERROR_INVALID_ARGUMENT, "data is NULL");
            return ptr::null_mut();
        }
        let file_name = if name.is_null() {
            String::new()
        } else {
            // SAFETY: the caller passes a NUL-terminated string or NULL.
            unsafe { CStr::from_ptr(name) }
                .to_string_lossy()
                .into_owned()
        };
        // SAFETY: the caller guarantees `len` readable bytes.
        let bytes = unsafe { slice::from_raw_parts(data, len) };
        match Plugin::parse(&file_name, bytes) {
            Ok(plugin) => {
                let handle = EsmPlugin {
                    plugin,
                    owned: None,
                    borrowed: Borrowed { ptr: data, len },
                };
                Box::into_raw(Box::new(handle))
            }
            Err(err) => {
                report(&err);
                ptr::null_mut()
            }
        }
    })
}

/// Read and index a plugin from disk. The handle owns the file bytes.
///
/// # Safety
/// `path` must be a NUL-terminated UTF-8 path, or NULL. Returns NULL on
/// failure.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_open(path: *const c_char) -> *mut EsmPlugin {
    guard_ptr(|| {
        clear_error();
        if path.is_null() {
            set_error(ESM_ERROR_INVALID_ARGUMENT, "path is NULL");
            return ptr::null_mut();
        }
        // SAFETY: the caller passes a NUL-terminated string or NULL.
        let path_text = unsafe { CStr::from_ptr(path) }
            .to_string_lossy()
            .into_owned();
        let bytes = match fs::read(&path_text) {
            Ok(bytes) => bytes,
            Err(err) => {
                set_error(ESM_ERROR_IO, format!("cannot read {path_text}: {err}"));
                return ptr::null_mut();
            }
        };
        // esm_reader.cpp reports the base name of the opened path.
        let file_name = Path::new(&path_text).file_name().map_or_else(
            || path_text.clone(),
            |name| name.to_string_lossy().into_owned(),
        );
        match Plugin::parse(&file_name, &bytes) {
            Ok(plugin) => {
                let handle = EsmPlugin {
                    plugin,
                    owned: Some(bytes),
                    borrowed: Borrowed::EMPTY,
                };
                Box::into_raw(Box::new(handle))
            }
            Err(err) => {
                report(&err);
                ptr::null_mut()
            }
        }
    })
}

/// Release a plugin handle. Passing NULL is a no-op.
///
/// # Safety
/// `handle` must come from `esm_plugin_open` / `esm_plugin_parse_memory` and
/// must not be used afterwards.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_free(handle: *mut EsmPlugin) {
    guard_value((), || {
        if !handle.is_null() {
            // SAFETY: the caller guarantees the handle came from Box::into_raw
            // in this crate and is freed once.
            drop(unsafe { Box::from_raw(handle) });
        }
    });
}

// ============================================================================
// Plugin metadata
// ============================================================================

/// Number of indexed records, excluding the TES4 header. 0 for NULL handles.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_record_count(handle: *const EsmPlugin) -> usize {
    guard_value(0, || {
        clear_error();
        match unsafe { plugin_ref(handle) } {
            Some(plugin) => plugin.plugin.records.len(),
            None => 0,
        }
    })
}

/// Number of GRUP headers traversed. 0 for NULL handles.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_group_count(handle: *const EsmPlugin) -> usize {
    guard_value(0, || {
        clear_error();
        match unsafe { plugin_ref(handle) } {
            Some(plugin) => plugin.plugin.group_count,
            None => 0,
        }
    })
}

/// Number of records whose payload is zlib compressed. 0 for NULL handles.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_compressed_count(handle: *const EsmPlugin) -> usize {
    guard_value(0, || {
        clear_error();
        match unsafe { plugin_ref(handle) } {
            Some(plugin) => plugin
                .plugin
                .records
                .iter()
                .filter(|rec| rec.compressed)
                .count(),
            None => 0,
        }
    })
}

/// 1 when the plugin is a master (`.esm`), 0 otherwise or for NULL handles.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_is_master(handle: *const EsmPlugin) -> c_int {
    guard_value(0, || {
        clear_error();
        match unsafe { plugin_ref(handle) } {
            Some(plugin) => c_int::from(plugin.plugin.is_master),
            None => 0,
        }
    })
}

/// Size of the TES4 header payload in bytes. 0 for NULL handles.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_header_size(handle: *const EsmPlugin) -> u32 {
    guard_value(0, || {
        clear_error();
        match unsafe { plugin_ref(handle) } {
            Some(plugin) => plugin.plugin.header.data_size,
            None => 0,
        }
    })
}

/// Copy the plugin file name into `out` (snprintf style). -1 for NULL handles.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle, and `out` must point to
/// `capacity` writable bytes or be NULL.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_file_name(
    handle: *const EsmPlugin,
    out: *mut c_char,
    capacity: usize,
) -> c_int {
    guard_value(-1, || {
        clear_error();
        match unsafe { plugin_ref(handle) } {
            Some(plugin) => write_c_string(&plugin.plugin.file_name, out, capacity),
            None => -1,
        }
    })
}

/// Number of masters listed in the TES4 header. 0 for NULL handles.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_master_count(handle: *const EsmPlugin) -> usize {
    guard_value(0, || {
        clear_error();
        match unsafe { plugin_ref(handle) } {
            Some(plugin) => plugin.plugin.masters.len(),
            None => 0,
        }
    })
}

/// Copy master `index` into `out` (snprintf style). -1 on error.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle, and `out` must point to
/// `capacity` writable bytes or be NULL.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_master_at(
    handle: *const EsmPlugin,
    index: usize,
    out: *mut c_char,
    capacity: usize,
) -> c_int {
    guard_value(-1, || {
        clear_error();
        let Some(plugin) = (unsafe { plugin_ref(handle) }) else {
            return -1;
        };
        match plugin.plugin.masters.get(index) {
            Some(master) => write_c_string(master, out, capacity),
            None => {
                set_error(ESM_ERROR_NOT_FOUND, format!("no master at index {index}"));
                -1
            }
        }
    })
}

/// Copy the info of the record at `index` into `out`. Returns 0 on success.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle, and `out` must point to a
/// writable `EsmRecordInfo`.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_record_info_at(
    handle: *const EsmPlugin,
    index: usize,
    out: *mut EsmRecordInfo,
) -> c_int {
    guard_value(ESM_ERROR_INVALID_ARGUMENT, || {
        clear_error();
        let Some(plugin) = (unsafe { plugin_ref(handle) }) else {
            return ESM_ERROR_INVALID_ARGUMENT;
        };
        if out.is_null() {
            set_error(ESM_ERROR_INVALID_ARGUMENT, "out is NULL");
            return ESM_ERROR_INVALID_ARGUMENT;
        }
        match plugin.plugin.records.get(index) {
            Some(record) => {
                // SAFETY: the caller passes a writable EsmRecordInfo.
                unsafe { out.write(record_info(record)) };
                ESM_OK
            }
            None => {
                set_error(ESM_ERROR_NOT_FOUND, format!("no record at index {index}"));
                ESM_ERROR_NOT_FOUND
            }
        }
    })
}

/// Copy the info of the record with `form_id` into `out`. Returns 0 on success,
/// `ESM_ERROR_NOT_FOUND` when no record carries that FormID.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle, and `out` must point to a
/// writable `EsmRecordInfo`.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_find_record(
    handle: *const EsmPlugin,
    form_id: u32,
    out: *mut EsmRecordInfo,
) -> c_int {
    guard_value(ESM_ERROR_INVALID_ARGUMENT, || {
        clear_error();
        let Some(plugin) = (unsafe { plugin_ref(handle) }) else {
            return ESM_ERROR_INVALID_ARGUMENT;
        };
        if out.is_null() {
            set_error(ESM_ERROR_INVALID_ARGUMENT, "out is NULL");
            return ESM_ERROR_INVALID_ARGUMENT;
        }
        match plugin.plugin.record_by_form_id(form_id) {
            Some(record) => {
                // SAFETY: the caller passes a writable EsmRecordInfo.
                unsafe { out.write(record_info(record)) };
                ESM_OK
            }
            None => {
                set_error(
                    ESM_ERROR_NOT_FOUND,
                    format!("no record with formID 0x{form_id:08X}"),
                );
                ESM_ERROR_NOT_FOUND
            }
        }
    })
}

/// Compute the per-type record counts. Free with `esm_type_counts_free`.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_type_counts(handle: *const EsmPlugin) -> *mut EsmTypeCounts {
    guard_ptr(|| {
        clear_error();
        let Some(plugin) = (unsafe { plugin_ref(handle) }) else {
            return ptr::null_mut();
        };
        let counts = plugin
            .plugin
            .type_counts()
            .into_iter()
            .map(|entry| (entry.rec_type, entry.count))
            .collect();
        Box::into_raw(Box::new(EsmTypeCounts { counts }))
    })
}

/// Number of distinct record types. 0 for NULL handles.
///
/// # Safety
/// `counts` must be NULL or a live handle from `esm_plugin_type_counts`.
#[no_mangle]
pub unsafe extern "C" fn esm_type_counts_len(counts: *const EsmTypeCounts) -> usize {
    guard_value(0, || {
        clear_error();
        if counts.is_null() {
            set_error(ESM_ERROR_INVALID_ARGUMENT, "type counts handle is NULL");
            return 0;
        }
        // SAFETY: the caller guarantees a live handle.
        unsafe { &*counts }.counts.len()
    })
}

/// Write type `index` into `out_type` (at least 5 bytes, tag plus NUL) and its
/// count into `out_count` when that is not NULL. Returns 0 on success.
///
/// # Safety
/// `counts` must be NULL or a live type-count handle, `out_type` must be NULL
/// or point to at least 5 writable bytes, and `out_count` must be NULL or
/// writable.
#[no_mangle]
pub unsafe extern "C" fn esm_type_counts_at(
    counts: *const EsmTypeCounts,
    index: usize,
    out_type: *mut c_char,
    out_count: *mut usize,
) -> c_int {
    guard_value(ESM_ERROR_INVALID_ARGUMENT, || {
        clear_error();
        if counts.is_null() {
            set_error(ESM_ERROR_INVALID_ARGUMENT, "type counts handle is NULL");
            return ESM_ERROR_INVALID_ARGUMENT;
        }
        // SAFETY: the caller guarantees a live handle.
        let counts = unsafe { &*counts };
        let Some((name, count)) = counts.counts.get(index) else {
            set_error(
                ESM_ERROR_NOT_FOUND,
                format!("no record type at index {index}"),
            );
            return ESM_ERROR_NOT_FOUND;
        };
        if !out_type.is_null() {
            let bytes = name.as_bytes();
            // SAFETY: the caller guarantees 5 writable bytes.
            unsafe {
                for (offset, byte) in bytes.iter().take(4).enumerate() {
                    *out_type.add(offset) = *byte as c_char;
                }
                *out_type.add(bytes.len().min(4)) = 0;
            }
        }
        if !out_count.is_null() {
            // SAFETY: the caller guarantees a writable usize.
            unsafe { *out_count = *count };
        }
        ESM_OK
    })
}

/// Release a type-count handle. Passing NULL is a no-op.
///
/// # Safety
/// `counts` must come from `esm_plugin_type_counts` and must not be used
/// afterwards.
#[no_mangle]
pub unsafe extern "C" fn esm_type_counts_free(counts: *mut EsmTypeCounts) {
    guard_value((), || {
        if !counts.is_null() {
            // SAFETY: the caller guarantees a handle from Box::into_raw.
            drop(unsafe { Box::from_raw(counts) });
        }
    });
}

// ============================================================================
// Payload decoding
// ============================================================================

/// Decode the subrecords of the record with `form_id`.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle. Returns NULL when no record
/// carries that FormID or when the payload cannot be decoded.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_decode_record(
    handle: *const EsmPlugin,
    form_id: u32,
) -> *mut EsmPayload {
    guard_ptr(|| {
        clear_error();
        let Some(plugin) = (unsafe { plugin_ref(handle) }) else {
            return ptr::null_mut();
        };
        match plugin.plugin.record_by_form_id(form_id) {
            Some(index) => decode_index(plugin, index),
            None => {
                set_error(
                    ESM_ERROR_NOT_FOUND,
                    format!("no record with formID 0x{form_id:08X}"),
                );
                ptr::null_mut()
            }
        }
    })
}

/// Decode the subrecords of the record at `index` in file order.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle. Returns NULL when the index
/// is out of range or the payload cannot be decoded.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_decode_record_at(
    handle: *const EsmPlugin,
    index: usize,
) -> *mut EsmPayload {
    guard_ptr(|| {
        clear_error();
        let Some(plugin) = (unsafe { plugin_ref(handle) }) else {
            return ptr::null_mut();
        };
        match plugin.plugin.records.get(index) {
            Some(record) => decode_index(plugin, record),
            None => {
                set_error(ESM_ERROR_NOT_FOUND, format!("no record at index {index}"));
                ptr::null_mut()
            }
        }
    })
}

/// Decode the TES4 header subrecords, i.e. HEDR, CNAM, MAST and friends.
///
/// # Safety
/// `handle` must be NULL or a live plugin handle.
#[no_mangle]
pub unsafe extern "C" fn esm_plugin_decode_header(handle: *const EsmPlugin) -> *mut EsmPayload {
    guard_ptr(|| {
        clear_error();
        let Some(plugin) = (unsafe { plugin_ref(handle) }) else {
            return ptr::null_mut();
        };
        payload_from(plugin.plugin.header_subrecords.clone())
    })
}

/// Release a payload handle. Passing NULL is a no-op.
///
/// # Safety
/// `payload` must come from one of the `esm_plugin_decode_*` functions and must
/// not be used afterwards, including any subrecord view taken from it.
#[no_mangle]
pub unsafe extern "C" fn esm_payload_free(payload: *mut EsmPayload) {
    guard_value((), || {
        if !payload.is_null() {
            // SAFETY: the caller guarantees a handle from Box::into_raw.
            drop(unsafe { Box::from_raw(payload) });
        }
    });
}

/// Number of subrecords in the payload. 0 for NULL handles.
///
/// # Safety
/// `payload` must be NULL or a live payload handle.
#[no_mangle]
pub unsafe extern "C" fn esm_payload_subrecord_count(payload: *const EsmPayload) -> usize {
    guard_value(0, || {
        clear_error();
        match unsafe { payload_ref(payload) } {
            Some(payload) => payload.subrecords.len(),
            None => 0,
        }
    })
}

/// Fill `out` with a view of subrecord `index`. Returns 0 on success.
///
/// # Safety
/// `payload` must be NULL or a live payload handle, and `out` must point to a
/// writable `EsmSubRecordView`.
#[no_mangle]
pub unsafe extern "C" fn esm_payload_subrecord_at(
    payload: *const EsmPayload,
    index: usize,
    out: *mut EsmSubRecordView,
) -> c_int {
    guard_value(ESM_ERROR_INVALID_ARGUMENT, || {
        clear_error();
        let Some(payload) = (unsafe { payload_ref(payload) }) else {
            return ESM_ERROR_INVALID_ARGUMENT;
        };
        if out.is_null() {
            set_error(ESM_ERROR_INVALID_ARGUMENT, "out is NULL");
            return ESM_ERROR_INVALID_ARGUMENT;
        }
        match payload.subrecords.get(index) {
            Some(sub) => {
                let view = EsmSubRecordView {
                    tag: sub.tag,
                    size: sub.data.len() as u32,
                    data: sub.data.as_ptr(),
                };
                // SAFETY: the caller passes a writable EsmSubRecordView.
                unsafe { out.write(view) };
                ESM_OK
            }
            None => {
                set_error(
                    ESM_ERROR_NOT_FOUND,
                    format!("no subrecord at index {index}"),
                );
                ESM_ERROR_NOT_FOUND
            }
        }
    })
}

/// Fill `out` with a view of the first subrecord tagged `tag`.
///
/// # Safety
/// `payload` must be NULL or a live payload handle, `tag` must be NULL or point
/// to 4 readable bytes, and `out` must point to a writable `EsmSubRecordView`.
#[no_mangle]
pub unsafe extern "C" fn esm_payload_find_subrecord(
    payload: *const EsmPayload,
    tag: *const c_char,
    out: *mut EsmSubRecordView,
) -> c_int {
    guard_value(ESM_ERROR_INVALID_ARGUMENT, || {
        clear_error();
        let Some(payload) = (unsafe { payload_ref(payload) }) else {
            return ESM_ERROR_INVALID_ARGUMENT;
        };
        let Some(tag) = (unsafe { tag_from(tag) }) else {
            return ESM_ERROR_INVALID_ARGUMENT;
        };
        if out.is_null() {
            set_error(ESM_ERROR_INVALID_ARGUMENT, "out is NULL");
            return ESM_ERROR_INVALID_ARGUMENT;
        }
        match payload.subrecords.iter().find(|sub| sub.tag == tag) {
            Some(sub) => {
                let view = EsmSubRecordView {
                    tag: sub.tag,
                    size: sub.data.len() as u32,
                    data: sub.data.as_ptr(),
                };
                // SAFETY: the caller passes a writable EsmSubRecordView.
                unsafe { out.write(view) };
                ESM_OK
            }
            None => {
                set_error(
                    ESM_ERROR_NOT_FOUND,
                    format!("no subrecord tagged {:?}", String::from_utf8_lossy(&tag)),
                );
                ESM_ERROR_NOT_FOUND
            }
        }
    })
}

/// Borrow the subrecord at `index` for the accessor functions below.
///
/// # Safety
/// `payload` must be NULL or a live payload handle.
unsafe fn subrecord_at<'a>(payload: *const EsmPayload, index: usize) -> Option<&'a SubRecord> {
    // SAFETY: the caller guarantees a live handle.
    let payload = unsafe { payload_ref(payload) }?;
    match payload.subrecords.get(index) {
        Some(sub) => Some(sub),
        None => {
            set_error(
                ESM_ERROR_NOT_FOUND,
                format!("no subrecord at index {index}"),
            );
            None
        }
    }
}

/// Little-endian u16 of subrecord `index`, or 0 when the payload is shorter.
///
/// # Safety
/// `payload` must be NULL or a live payload handle.
#[no_mangle]
pub unsafe extern "C" fn esm_payload_u16(payload: *const EsmPayload, index: usize) -> u16 {
    guard_value(0, || {
        clear_error();
        match unsafe { subrecord_at(payload, index) } {
            Some(sub) => sub.as_u16().unwrap_or(0),
            None => 0,
        }
    })
}

/// Little-endian u32 of subrecord `index`, or 0 when the payload is shorter.
/// FormIDs are read with this too.
///
/// # Safety
/// `payload` must be NULL or a live payload handle.
#[no_mangle]
pub unsafe extern "C" fn esm_payload_u32(payload: *const EsmPayload, index: usize) -> u32 {
    guard_value(0, || {
        clear_error();
        match unsafe { subrecord_at(payload, index) } {
            Some(sub) => sub.as_u32().unwrap_or(0),
            None => 0,
        }
    })
}

/// Little-endian i32 of subrecord `index`, or 0 when the payload is shorter.
///
/// # Safety
/// `payload` must be NULL or a live payload handle.
#[no_mangle]
pub unsafe extern "C" fn esm_payload_i32(payload: *const EsmPayload, index: usize) -> i32 {
    guard_value(0, || {
        clear_error();
        match unsafe { subrecord_at(payload, index) } {
            Some(sub) => sub.as_i32().unwrap_or(0),
            None => 0,
        }
    })
}

/// Little-endian f32 of subrecord `index`, or 0.0 when the payload is shorter.
///
/// # Safety
/// `payload` must be NULL or a live payload handle.
#[no_mangle]
pub unsafe extern "C" fn esm_payload_f32(payload: *const EsmPayload, index: usize) -> f32 {
    guard_value(0.0, || {
        clear_error();
        match unsafe { subrecord_at(payload, index) } {
            Some(sub) => sub.as_f32().unwrap_or(0.0),
            None => 0.0,
        }
    })
}

/// Read subrecord `index` as a float, falling back to an integer interpretation
/// when the value is not a plausible float. Oblivion stores integer-only
/// magnitudes as denormal floats, so callers must use this for magnitudes.
/// Returns 0.0 when the payload is shorter than 4 bytes.
///
/// # Safety
/// `payload` must be NULL or a live payload handle.
#[no_mangle]
pub unsafe extern "C" fn esm_payload_float_or_int(payload: *const EsmPayload, index: usize) -> f32 {
    guard_value(0.0, || {
        clear_error();
        match unsafe { subrecord_at(payload, index) } {
            Some(sub) => sub.as_float_or_int().unwrap_or(0.0),
            None => 0.0,
        }
    })
}

/// Copy subrecord `index` as a NUL-terminated string (snprintf style).
///
/// Text is Windows-1252 in the retail plugins; only ASCII survives the UTF-8
/// conversion, so read the subrecord view directly for byte-exact text.
///
/// # Safety
/// `payload` must be NULL or a live payload handle, and `out` must point to
/// `capacity` writable bytes or be NULL.
#[no_mangle]
pub unsafe extern "C" fn esm_payload_zstring(
    payload: *const EsmPayload,
    index: usize,
    out: *mut c_char,
    capacity: usize,
) -> c_int {
    guard_value(-1, || {
        clear_error();
        match unsafe { subrecord_at(payload, index) } {
            Some(sub) => write_c_string(&sub.as_zstring(), out, capacity),
            None => -1,
        }
    })
}

// ============================================================================
// Diagnostics
// ============================================================================

/// Error code of the last call on this thread; 0 when it succeeded.
#[no_mangle]
pub extern "C" fn esm_last_error_code() -> c_int {
    LAST_ERROR.with(|state| state.borrow().code)
}

/// Copy the message of the last error on this thread (snprintf style).
#[no_mangle]
pub extern "C" fn esm_last_error_message(out: *mut c_char, capacity: usize) -> c_int {
    LAST_ERROR.with(|state| write_c_string(&state.borrow().message, out, capacity))
}

/// Size of `EsmRecordInfo`, for a compile-time ABI assertion on the C side.
#[no_mangle]
pub extern "C" fn esm_record_info_size() -> usize {
    std::mem::size_of::<EsmRecordInfo>()
}

/// Size of `EsmSubRecordView`, for a compile-time ABI assertion on the C side.
#[no_mangle]
pub extern "C" fn esm_subrecord_view_size() -> usize {
    std::mem::size_of::<EsmSubRecordView>()
}

/// Static version string of this library; never freed by the caller.
#[no_mangle]
pub extern "C" fn esm_version() -> *const c_char {
    VERSION.as_ptr().cast::<c_char>()
}

#[cfg(test)]
mod tests {
    use super::*;
    use esm_parser::GroupType;
    use std::io::Write;

    const WEAP_FORM_ID: u32 = 0x0009_66A9;
    const CELL_FORM_ID: u32 = 0x0000_5FC6;
    const LAND_FORM_ID: u32 = 0x0000_74E7;
    const WORLD_FORM_ID: u32 = 0x0000_003C;

    fn sub(tag: &[u8; 4], data: &[u8]) -> Vec<u8> {
        let mut out = Vec::new();
        out.extend_from_slice(tag);
        out.extend_from_slice(&(data.len() as u16).to_le_bytes());
        out.extend_from_slice(data);
        out
    }

    fn record(tag: &[u8; 4], form_id: u32, flags: u32, payload: &[u8]) -> Vec<u8> {
        let mut out = Vec::new();
        out.extend_from_slice(tag);
        out.extend_from_slice(&(payload.len() as u32).to_le_bytes());
        out.extend_from_slice(&flags.to_le_bytes());
        out.extend_from_slice(&form_id.to_le_bytes());
        out.extend_from_slice(&1u16.to_le_bytes());
        out.extend_from_slice(&0u16.to_le_bytes());
        out.extend_from_slice(payload);
        out
    }

    fn compressed_record(tag: &[u8; 4], form_id: u32, payload: &[u8]) -> Vec<u8> {
        let mut encoder = flate2::write::ZlibEncoder::new(Vec::new(), flate2::Compression::fast());
        encoder.write_all(payload).unwrap();
        let stream = encoder.finish().unwrap();

        let mut body = (payload.len() as u32).to_le_bytes().to_vec();
        body.extend_from_slice(&stream);
        record(tag, form_id, esm_parser::REC_FLAG_COMPRESSED, &body)
    }

    fn grup(label: u32, group_type: GroupType, content: &[u8]) -> Vec<u8> {
        let mut out = Vec::new();
        out.extend_from_slice(b"GRUP");
        out.extend_from_slice(&((20 + content.len()) as u32).to_le_bytes());
        out.extend_from_slice(&label.to_le_bytes());
        out.extend_from_slice(&group_type.as_raw().to_le_bytes());
        out.extend_from_slice(&0u16.to_le_bytes());
        out.extend_from_slice(&0u16.to_le_bytes());
        out.extend_from_slice(content);
        out
    }

    fn tes4(masters: &[&str]) -> Vec<u8> {
        let mut payload = sub(b"HEDR", &[0u8; 12]);
        for master in masters {
            let mut name = master.as_bytes().to_vec();
            name.push(0);
            payload.extend_from_slice(&sub(b"MAST", &name));
            payload.extend_from_slice(&sub(b"DATA", &[0u8; 8]));
        }
        record(b"TES4", 0, 0, &payload)
    }

    fn weap_payload() -> Vec<u8> {
        let mut payload = sub(b"EDID", b"SE13TrophySword1\0");
        payload.extend_from_slice(&sub(b"FULL", b"Sword of Jyggalag\0"));
        payload.extend_from_slice(&sub(b"DATA", &42.5f32.to_le_bytes()));
        // A two-byte subrecord, to exercise the short-payload fallbacks.
        payload.extend_from_slice(&sub(b"ANAM", &[0x01, 0x02]));
        payload
    }

    fn land_payload() -> Vec<u8> {
        let mut payload = sub(b"VNML", &[0x40u8; 12]);
        payload.extend_from_slice(&sub(b"VHGT", &[0u8; 8]));
        payload
    }

    fn plugin_bytes() -> Vec<u8> {
        let mut out = tes4(&["Oblivion.esm", "Knights.esp"]);
        out.extend_from_slice(&record(b"WEAP", WEAP_FORM_ID, 0, &weap_payload()));
        let cell = record(b"CELL", CELL_FORM_ID, 0, &sub(b"EDID", b"TestCell\0"));
        out.extend_from_slice(&grup(WORLD_FORM_ID, GroupType::World, &cell));
        let land = compressed_record(b"LAND", LAND_FORM_ID, &land_payload());
        out.extend_from_slice(&grup(CELL_FORM_ID, GroupType::CellChild, &land));
        out
    }

    /// Leak a synthetic plugin into the ABI and return the handle.
    fn open_synthetic() -> (*mut EsmPlugin, Vec<u8>) {
        let bytes = plugin_bytes();
        let name = std::ffi::CString::new("Test.esm").unwrap();
        // SAFETY: `bytes` outlives every call in a test.
        let handle = unsafe { esm_plugin_parse_memory(name.as_ptr(), bytes.as_ptr(), bytes.len()) };
        assert!(!handle.is_null(), "parse failed");
        (handle, bytes)
    }

    /// Read a buffer the ABI just filled with a NUL-terminated string.
    fn cstr(buffer: &[c_char]) -> String {
        // SAFETY: the caller only passes buffers the ABI just terminated.
        unsafe { CStr::from_ptr(buffer.as_ptr()) }
            .to_string_lossy()
            .into_owned()
    }

    /// Copy the bytes a subrecord view points at.
    fn view_bytes(view: &EsmSubRecordView) -> Vec<u8> {
        // SAFETY: the view belongs to a payload that is still alive.
        unsafe { slice::from_raw_parts(view.data, view.size as usize) }.to_vec()
    }

    #[test]
    fn plugin_metadata_matches_the_index() {
        let (handle, _bytes) = open_synthetic();
        // SAFETY: `handle` is live for the whole test.
        unsafe {
            assert_eq!(esm_plugin_record_count(handle), 3);
            assert_eq!(esm_plugin_group_count(handle), 2);
            assert_eq!(esm_plugin_compressed_count(handle), 1);
            assert_eq!(esm_plugin_is_master(handle), 1);
            assert_eq!(esm_plugin_master_count(handle), 2);
            assert_eq!(esm_plugin_header_size(handle), 83);
            assert_eq!(esm_last_error_code(), ESM_OK);

            let mut buffer = [0 as c_char; 32];
            let needed = esm_plugin_file_name(handle, buffer.as_mut_ptr(), buffer.len());
            assert_eq!(needed, 9);
            assert_eq!(cstr(&buffer), "Test.esm");

            let mut master = [0 as c_char; 32];
            assert!(esm_plugin_master_at(handle, 1, master.as_mut_ptr(), master.len()) > 0);
            assert_eq!(cstr(&master), "Knights.esp");

            esm_plugin_free(handle);
        }
    }

    #[test]
    fn file_name_is_truncated_but_reports_the_full_length() {
        let (handle, _bytes) = open_synthetic();
        // SAFETY: `handle` is live for the whole test.
        unsafe {
            let mut buffer = [0 as c_char; 5];
            let needed = esm_plugin_file_name(handle, buffer.as_mut_ptr(), buffer.len());
            assert_eq!(needed, 9);
            assert_eq!(cstr(&buffer), "Test");
            esm_plugin_free(handle);
        }
    }

    #[test]
    fn record_info_carries_group_attribution() {
        let (handle, _bytes) = open_synthetic();
        // SAFETY: `handle` is live for the whole test.
        unsafe {
            let mut info = std::mem::zeroed::<EsmRecordInfo>();
            assert_eq!(
                esm_plugin_find_record(handle, WEAP_FORM_ID, &mut info),
                ESM_OK
            );
            assert_eq!(info.rec_type, *b"WEAP");
            assert_eq!(info.form_id, WEAP_FORM_ID);
            assert_eq!(info.compressed, 0);
            assert_eq!(info.parent_form_id, 0);
            assert_eq!(info.worldspace_form_id, 0);
            // Payload offset: the 20-byte TES4 header plus its 83-byte payload
            // put the WEAP record at 103, and its own 20-byte header makes its
            // payload start at 123.
            assert_eq!(info.file_offset, 123);

            assert_eq!(
                esm_plugin_find_record(handle, CELL_FORM_ID, &mut info),
                ESM_OK
            );
            assert_eq!(info.rec_type, *b"CELL");
            assert_eq!(info.worldspace_form_id, WORLD_FORM_ID);

            assert_eq!(
                esm_plugin_find_record(handle, LAND_FORM_ID, &mut info),
                ESM_OK
            );
            assert_eq!(info.rec_type, *b"LAND");
            assert_eq!(info.compressed, 1);
            assert_eq!(info.parent_form_id, CELL_FORM_ID);
            assert_eq!(info.decomp_size, 12 + 6 + 8 + 6);

            assert_eq!(
                esm_plugin_find_record(handle, 0xDEAD_BEEF, &mut info),
                ESM_ERROR_NOT_FOUND
            );
            assert_eq!(esm_last_error_code(), ESM_ERROR_NOT_FOUND);

            // The record index is enumerable in file order.
            let mut first = std::mem::zeroed::<EsmRecordInfo>();
            assert_eq!(esm_plugin_record_info_at(handle, 0, &mut first), ESM_OK);
            assert_eq!(first.rec_type, *b"WEAP");
            let mut third = std::mem::zeroed::<EsmRecordInfo>();
            assert_eq!(esm_plugin_record_info_at(handle, 2, &mut third), ESM_OK);
            assert_eq!(third.rec_type, *b"LAND");
            assert_eq!(
                esm_plugin_record_info_at(handle, 3, &mut third),
                ESM_ERROR_NOT_FOUND
            );

            esm_plugin_free(handle);
        }
    }

    #[test]
    fn subrecord_views_and_accessors_read_the_payload() {
        let (handle, _bytes) = open_synthetic();
        // SAFETY: `handle` is live for the whole test.
        unsafe {
            let payload = esm_plugin_decode_record(handle, WEAP_FORM_ID);
            assert!(!payload.is_null());
            assert_eq!(esm_payload_subrecord_count(payload), 4);

            let mut view = std::mem::zeroed::<EsmSubRecordView>();
            assert_eq!(esm_payload_subrecord_at(payload, 0, &mut view), ESM_OK);
            assert_eq!(view.tag, *b"EDID");
            assert_eq!(view.size, 17);
            assert_eq!(view_bytes(&view), b"SE13TrophySword1\0");

            let mut buffer = [0 as c_char; 32];
            assert_eq!(
                esm_payload_zstring(payload, 1, buffer.as_mut_ptr(), buffer.len()),
                18
            );
            assert_eq!(cstr(&buffer), "Sword of Jyggalag");

            let mut data = std::mem::zeroed::<EsmSubRecordView>();
            assert_eq!(
                esm_payload_find_subrecord(payload, c"DATA".as_ptr(), &mut data),
                ESM_OK
            );
            assert_eq!(data.size, 4);
            assert_eq!(esm_payload_f32(payload, 2), 42.5);
            assert_eq!(esm_payload_u32(payload, 2), 42.5f32.to_bits());

            // ANAM is two bytes, so only the 16-bit accessor yields a value.
            let mut anam = std::mem::zeroed::<EsmSubRecordView>();
            assert_eq!(
                esm_payload_find_subrecord(payload, c"ANAM".as_ptr(), &mut anam),
                ESM_OK
            );
            assert_eq!(anam.size, 2);
            assert_eq!(esm_payload_u16(payload, 3), 0x0201);
            assert_eq!(esm_payload_u32(payload, 3), 0);
            assert_eq!(esm_payload_i32(payload, 3), 0);
            assert_eq!(esm_payload_f32(payload, 3), 0.0);
            assert_eq!(esm_payload_float_or_int(payload, 3), 0.0);
            assert_eq!(
                esm_payload_find_subrecord(payload, c"XXXX".as_ptr(), &mut data),
                ESM_ERROR_NOT_FOUND
            );
            assert_eq!(
                esm_payload_subrecord_at(payload, 9, &mut view),
                ESM_ERROR_NOT_FOUND
            );
            assert_eq!(esm_last_error_code(), ESM_ERROR_NOT_FOUND);

            esm_payload_free(payload);
            esm_plugin_free(handle);
        }
    }

    #[test]
    fn compressed_records_inflate_across_the_abi() {
        let (handle, _bytes) = open_synthetic();
        // SAFETY: `handle` is live for the whole test.
        unsafe {
            let payload = esm_plugin_decode_record(handle, LAND_FORM_ID);
            assert!(!payload.is_null(), "inflate failed: {}", last_error());
            assert_eq!(esm_payload_subrecord_count(payload), 2);

            let mut view = std::mem::zeroed::<EsmSubRecordView>();
            assert_eq!(esm_payload_subrecord_at(payload, 0, &mut view), ESM_OK);
            assert_eq!(view.tag, *b"VNML");
            assert_eq!(view.size, 12);
            esm_payload_free(payload);

            // The header decodes through the same payload API.
            let header = esm_plugin_decode_header(handle);
            assert!(!header.is_null());
            assert_eq!(esm_payload_subrecord_count(header), 5);
            let mut hedr = std::mem::zeroed::<EsmSubRecordView>();
            assert_eq!(
                esm_payload_find_subrecord(header, c"HEDR".as_ptr(), &mut hedr),
                ESM_OK
            );
            assert_eq!(hedr.tag, *b"HEDR");
            esm_payload_free(header);

            esm_plugin_free(handle);
        }
    }

    #[test]
    fn type_counts_come_back_in_descending_order() {
        let (handle, _bytes) = open_synthetic();
        // SAFETY: `handle` is live for the whole test.
        unsafe {
            let counts = esm_plugin_type_counts(handle);
            assert!(!counts.is_null());
            assert_eq!(esm_type_counts_len(counts), 3);

            // Equal counts fall back to the tag name, ascending.
            let mut tag = [0 as c_char; 5];
            let mut count = 0usize;
            assert_eq!(
                esm_type_counts_at(counts, 0, tag.as_mut_ptr(), &mut count),
                ESM_OK
            );
            assert_eq!(
                tag,
                [
                    b'C' as c_char,
                    b'E' as c_char,
                    b'L' as c_char,
                    b'L' as c_char,
                    0
                ]
            );
            assert_eq!(count, 1);

            let mut all = Vec::new();
            for index in 0..esm_type_counts_len(counts) {
                assert_eq!(
                    esm_type_counts_at(counts, index, tag.as_mut_ptr(), &mut count),
                    ESM_OK
                );
                let name = cstr(&tag);
                all.push((name, count));
            }
            assert_eq!(
                all,
                vec![
                    ("CELL".to_string(), 1usize),
                    ("LAND".to_string(), 1usize),
                    ("WEAP".to_string(), 1usize),
                ]
            );

            assert_eq!(
                esm_type_counts_at(counts, 3, tag.as_mut_ptr(), &mut count),
                ESM_ERROR_NOT_FOUND
            );
            esm_type_counts_free(counts);
            esm_plugin_free(handle);
        }
    }

    #[test]
    fn invalid_input_is_reported_without_crashing() {
        // SAFETY: a bogus buffer is exactly what this test wants to reject.
        unsafe {
            let mut bytes = vec![0u8; 64];
            let name = std::ffi::CString::new("Bogus.esm").unwrap();
            let handle = esm_plugin_parse_memory(name.as_ptr(), bytes.as_ptr(), bytes.len());
            assert!(handle.is_null());
            assert_eq!(esm_last_error_code(), ESM_ERROR_BAD_HEADER);
            assert!(last_error().contains("TES4"));

            bytes.clear();
            let truncated = esm_plugin_parse_memory(name.as_ptr(), bytes.as_ptr(), 0);
            assert!(truncated.is_null());

            assert!(esm_plugin_parse_memory(name.as_ptr(), ptr::null(), 0).is_null());
            assert_eq!(esm_last_error_code(), ESM_ERROR_INVALID_ARGUMENT);
            assert!(esm_plugin_open(ptr::null()).is_null());

            // A missing file reports the I/O failure instead of a bad header.
            let missing = std::ffi::CString::new(r"C:\no\such\plugin.esm").unwrap();
            assert!(esm_plugin_open(missing.as_ptr()).is_null());
            assert_eq!(esm_last_error_code(), ESM_ERROR_IO);

            // NULL handles are tolerated by every getter.
            assert_eq!(esm_plugin_record_count(ptr::null()), 0);
            assert_eq!(esm_plugin_group_count(ptr::null()), 0);
            assert_eq!(esm_plugin_compressed_count(ptr::null()), 0);
            assert_eq!(esm_plugin_is_master(ptr::null()), 0);
            assert_eq!(esm_plugin_header_size(ptr::null()), 0);
            assert_eq!(esm_plugin_master_count(ptr::null()), 0);
            assert_eq!(esm_plugin_file_name(ptr::null(), ptr::null_mut(), 0), -1);
            assert_eq!(esm_plugin_master_at(ptr::null(), 0, ptr::null_mut(), 0), -1);
            assert!(esm_plugin_type_counts(ptr::null()).is_null());
            assert!(esm_plugin_decode_record(ptr::null(), 1).is_null());
            assert!(esm_plugin_decode_record_at(ptr::null(), 0).is_null());
            assert!(esm_plugin_decode_header(ptr::null()).is_null());
            assert_eq!(esm_payload_subrecord_count(ptr::null()), 0);
            assert_eq!(esm_payload_u32(ptr::null(), 0), 0);
            assert_eq!(esm_payload_zstring(ptr::null(), 0, ptr::null_mut(), 0), -1);

            // Freeing NULL is a no-op.
            esm_plugin_free(ptr::null_mut());
            esm_payload_free(ptr::null_mut());
            esm_type_counts_free(ptr::null_mut());
        }
    }

    #[test]
    fn open_reads_the_file_and_reports_its_base_name() {
        let dir = std::env::temp_dir().join("esm_ffi_open_test");
        std::fs::create_dir_all(&dir).unwrap();
        let path = dir.join("Synth.esm");
        std::fs::write(&path, plugin_bytes()).unwrap();

        let path_text = std::ffi::CString::new(path.to_str().unwrap()).unwrap();
        // SAFETY: the path is a valid NUL-terminated string.
        let handle = unsafe { esm_plugin_open(path_text.as_ptr()) };
        assert!(!handle.is_null(), "open failed: {}", last_error());
        // SAFETY: `handle` is live until the free call below.
        unsafe {
            let mut buffer = [0 as c_char; 32];
            assert!(esm_plugin_file_name(handle, buffer.as_mut_ptr(), buffer.len()) > 0);
            assert_eq!(cstr(&buffer), "Synth.esm");
            assert_eq!(esm_plugin_record_count(handle), 3);
            let payload = esm_plugin_decode_record(handle, WEAP_FORM_ID);
            assert!(!payload.is_null());
            esm_payload_free(payload);
            esm_plugin_free(handle);
        }
        std::fs::remove_file(&path).ok();
    }

    #[test]
    fn abi_sizes_match_the_header_assertions() {
        assert_eq!(esm_record_info_size(), 40);
        assert_eq!(esm_subrecord_view_size(), 16);
        assert_eq!(std::mem::align_of::<EsmRecordInfo>(), 8);
        assert!(!esm_version().is_null());
        let version = unsafe { CStr::from_ptr(esm_version()) }.to_str().unwrap();
        assert_eq!(version, format!("esm-ffi {}", env!("CARGO_PKG_VERSION")));
    }

    fn last_error() -> String {
        let mut buffer = [0 as c_char; 256];
        let needed = esm_last_error_message(buffer.as_mut_ptr(), buffer.len());
        assert!(needed > 0);
        unsafe { CStr::from_ptr(buffer.as_ptr()) }
            .to_string_lossy()
            .into_owned()
    }

    /// Index the retail plugin when it is installed. Set `OBLIVION_ESM` to
    /// override the path; the test is skipped when the file is unavailable.
    #[test]
    fn real_oblivion_esm_round_trips_through_the_abi() {
        let path = std::env::var("OBLIVION_ESM").unwrap_or_else(|_| {
            r"C:\Program Files (x86)\Steam\steamapps\common\Oblivion\Data\Oblivion.esm".to_string()
        });
        let bytes = match std::fs::read(&path) {
            Ok(bytes) => bytes,
            Err(_) => {
                eprintln!("skip: {path} unavailable");
                return;
            }
        };
        let name = std::ffi::CString::new("Oblivion.esm").unwrap();
        // SAFETY: `bytes` outlives the handle.
        let handle = unsafe { esm_plugin_parse_memory(name.as_ptr(), bytes.as_ptr(), bytes.len()) };
        assert!(!handle.is_null(), "parse failed: {}", last_error());

        // SAFETY: `handle` is live for the whole test.
        unsafe {
            assert_eq!(esm_plugin_record_count(handle), 1_167_016);
            assert_eq!(esm_plugin_group_count(handle), 85_079);
            assert_eq!(esm_plugin_compressed_count(handle), 41_789);
            assert_eq!(esm_plugin_is_master(handle), 1);
            assert_eq!(esm_plugin_header_size(handle), 744);
            assert_eq!(esm_plugin_master_count(handle), 0);

            let mut info = std::mem::zeroed::<EsmRecordInfo>();
            assert_eq!(
                esm_plugin_find_record(handle, 0x0009_66A9, &mut info),
                ESM_OK
            );
            assert_eq!(info.rec_type, *b"WEAP");
            assert_eq!(info.compressed, 0);

            let payload = esm_plugin_decode_record(handle, 0x0009_66A9);
            assert!(!payload.is_null());
            let mut view = std::mem::zeroed::<EsmSubRecordView>();
            assert_eq!(
                esm_payload_find_subrecord(payload, c"EDID".as_ptr(), &mut view),
                ESM_OK
            );
            let text = slice::from_raw_parts(view.data, view.size as usize);
            let end = text.iter().position(|&b| b == 0).unwrap_or(text.len());
            assert_eq!(&text[..end], b"SE13TrophySword1");
            esm_payload_free(payload);

            // A compressed LAND belongs to cell 0x5FC6 of worldspace 0x3C.
            assert_eq!(
                esm_plugin_find_record(handle, 0x0000_74E7, &mut info),
                ESM_OK
            );
            assert_eq!(info.rec_type, *b"LAND");
            assert_eq!(info.compressed, 1);
            assert_eq!(info.parent_form_id, 0x0000_5FC6);
            assert_eq!(info.worldspace_form_id, 0x0000_003C);

            let payload = esm_plugin_decode_record_at(handle, 0);
            assert!(!payload.is_null(), "index 0 failed: {}", last_error());
            esm_payload_free(payload);

            let counts = esm_plugin_type_counts(handle);
            assert!(!counts.is_null());
            assert!(esm_type_counts_len(counts) > 1);
            esm_type_counts_free(counts);

            esm_plugin_free(handle);
        }
    }
}
