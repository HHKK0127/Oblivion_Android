//! C ABI over the `plugin-parser` crate.
//!
//! Exposes the FormID index map, the load-order rebase and the `plugins.txt`
//! grammar to C and C++. There are no handles here: every call is a pure
//! function over caller-owned buffers and a caller-owned map, so nothing is
//! allocated on the Rust side and nothing needs to be released.
//!
//! Threading: each thread has its own last-error state, so `plg_last_error_code`
//! and `plg_last_error_message` only describe calls made on the calling thread.
//!
//! Strings copied into caller buffers follow the snprintf convention: the return
//! value is the length in bytes including the terminating NUL, so a caller
//! detects truncation by comparing it against the buffer capacity. Nothing is
//! written past `capacity` bytes, and a NULL `out` or a zero `capacity` queries
//! the required length without writing anything.
//!
//! Only the eight computing calls (`plg_plugin_name_equals`,
//! `plg_build_formid_index_map`, `plg_rebase_formid`, `plg_parse_plugin_list`,
//! `plg_serialize_plugin_list`, `plg_active_plugin_indices`,
//! `plg_sort_plugin_load_order` and `plg_is_master_file_name`) reset the error
//! state. The constant accessors leave it alone so that a caller can inspect the
//! error a computing call left behind.

use std::cell::RefCell;
use std::ffi::{c_char, c_int, CStr};
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::ptr;

use plugin_parser::{
    active_plugin_indices, build_form_id_index_map, is_master_file_name, parse_plugin_list,
    plugin_name_equals, rebase_form_id, serialize_plugin_list, sort_plugin_load_order,
    NamedPluginEntry, BASE_PLUGIN_FILE_NAME, UNRESOLVED_FORMID_INDEX,
};

/// The call succeeded.
pub const PLG_OK: c_int = 0;
/// A required pointer argument was NULL.
pub const PLG_ERROR_INVALID_ARGUMENT: c_int = -1;
/// A Rust panic was caught at the boundary; the call had no effect.
pub const PLG_ERROR_PANIC: c_int = -7;
/// `out_map` cannot hold the result; `*out_len` holds the required entry count.
pub const PLG_ERROR_BUFFER_TOO_SMALL: c_int = -8;

const VERSION: &str = concat!("plugin-ffi ", env!("CARGO_PKG_VERSION"), "\0");

/// A C `int` is 32 bits on every ABI the engine targets, and the returned map is
/// counted in `u32` entries. A mismatch would silently corrupt every caller.
const _: () = assert!(core::mem::size_of::<c_int>() == 4);
const _: () = assert!(core::mem::size_of::<u32>() == 4);

/// One entry of a `plugins.txt` buffer parsed by `plg_parse_plugin_list`, with
/// the name left where it was in the caller's buffer.
///
/// Keeping an offset rather than a copy is what lets the whole parse run without
/// allocating a name buffer on either side of the boundary.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PlgPluginListEntry {
    /// Offset of the name inside the `contents` buffer passed to the parse call.
    pub name_offset: usize,
    /// Length of the name in bytes.
    pub name_length: usize,
    /// 1 when the entry is active, 0 when it is not.
    pub active: c_int,
}

// The C++ side declares this struct field for field and hands out arrays of it,
// so the padding `active` leaves behind must be the same on both sides.
const _: () =
    assert!(core::mem::size_of::<PlgPluginListEntry>() == 3 * core::mem::size_of::<usize>());
const _: () =
    assert!(core::mem::align_of::<PlgPluginListEntry>() == core::mem::align_of::<usize>());

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
    set_error(PLG_OK, String::new());
}

/// Run `body`, turning a caught panic into the panic code plus `fallback`.
///
/// Every exported function routes through here: a Rust panic must not unwind
/// into the C++ frames above, which are not built to take it.
fn guard_value<T: Copy>(fallback: T, body: impl FnOnce() -> T) -> T {
    match catch_unwind(AssertUnwindSafe(body)) {
        Ok(value) => value,
        Err(_) => {
            set_error(PLG_ERROR_PANIC, "panic caught at the plugin-ffi boundary");
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
            ptr::copy_nonoverlapping(text.as_ptr().cast::<c_char>(), out, copied);
            *out.add(copied) = 0;
        }
    }
    needed as c_int
}

/// Borrow `len` NUL-terminated names without decoding them, so a name that is
/// not valid UTF-8 still compares the way the engine compares it. Returns `None`
/// with the error set when an entry is NULL.
///
/// # Safety
/// `items` must be NULL only when `len` is 0. Otherwise it must point to `len`
/// readable pointers, each NUL-terminated and staying valid for the call.
unsafe fn collect_names<'a>(items: *const *const c_char, len: usize) -> Option<Vec<&'a [u8]>> {
    if len == 0 {
        return Some(Vec::new());
    }
    if items.is_null() {
        set_error(PLG_ERROR_INVALID_ARGUMENT, "the name array is NULL");
        return None;
    }
    let mut names = Vec::with_capacity(len);
    for index in 0..len {
        // SAFETY: the caller promises `len` readable pointers at `items`.
        let item = unsafe { *items.add(index) };
        if item.is_null() {
            set_error(PLG_ERROR_INVALID_ARGUMENT, format!("name {index} is NULL"));
            return None;
        }
        // SAFETY: the caller promises a NUL-terminated string at each pointer.
        names.push(unsafe { CStr::from_ptr(item) }.to_bytes());
    }
    Some(names)
}

/// Borrow `len` active flags, the parallel array the serialising and
/// active-selection calls pair with their names. Returns `None` with the error
/// set when the array is missing.
///
/// # Safety
/// `flags` must be NULL only when `len` is 0. Otherwise it must point to `len`
/// readable `int` values staying valid for the call.
unsafe fn collect_active_flags(flags: *const c_int, len: usize) -> Option<Vec<bool>> {
    if len == 0 {
        return Some(Vec::new());
    }
    if flags.is_null() {
        set_error(PLG_ERROR_INVALID_ARGUMENT, "the active flag array is NULL");
        return None;
    }
    let mut active = Vec::with_capacity(len);
    for index in 0..len {
        // SAFETY: the caller promises `len` readable int values at `flags`.
        active.push(unsafe { *flags.add(index) } != 0);
    }
    Some(active)
}

/// Zips the borrowed names with their flags into the pairs the entry-oriented
/// helpers take.
fn pair_names_with_flags<'a>(names: &'a [&'a [u8]], active: &[bool]) -> Vec<NamedPluginEntry<'a>> {
    debug_assert_eq!(names.len(), active.len());
    names
        .iter()
        .zip(active)
        .map(|(&name, &active)| NamedPluginEntry { name, active })
        .collect()
}

/// The shared half of the two-pass protocol for a copied result buffer: records
/// the required count in `*out_len`, copies the values when the buffer is large
/// enough, and asks for a bigger buffer otherwise.
///
/// `*out_len` is the only way a caller learns the count, so it must be writable;
/// that is what separates these calls from `plg_build_formid_index_map`, whose
/// count is derivable from its arguments.
///
/// `unit` names what is being counted in the error message, because the same
/// protocol carries `size_t` indices, byte offsets and raw text bytes.
///
/// # Safety
/// `out` must point to `capacity` writable `T` values or be NULL, and `out_len`
/// must be writable.
unsafe fn write_result<T: Copy>(
    values: &[T],
    out: *mut T,
    capacity: usize,
    out_len: *mut usize,
    unit: &str,
) -> c_int {
    let required = values.len();
    if out_len.is_null() {
        set_error(PLG_ERROR_INVALID_ARGUMENT, "out_len is NULL");
        return PLG_ERROR_INVALID_ARGUMENT;
    }
    // SAFETY: the caller promises `out_len` is writable.
    unsafe { *out_len = required };
    if out.is_null() || capacity == 0 {
        return PLG_OK;
    }
    if capacity < required {
        set_error(
            PLG_ERROR_BUFFER_TOO_SMALL,
            format!("the output holds {capacity} {unit} but {required} are required"),
        );
        return PLG_ERROR_BUFFER_TOO_SMALL;
    }
    // SAFETY: `out` is non-NULL and `capacity >= required`, which is the length
    // of `values`, so that many writable `T` slots exist.
    unsafe { ptr::copy_nonoverlapping(values.as_ptr(), out, required) };
    PLG_OK
}

/// The index recorded for a master that is not in the loaded file list.
#[no_mangle]
pub extern "C" fn plg_unresolved_formid_index() -> u32 {
    UNRESOLVED_FORMID_INDEX
}

/// The base game file name (snprintf style). Leaves the error state alone.
///
/// # Safety
/// `out` must point to `capacity` writable bytes or be NULL.
#[no_mangle]
pub unsafe extern "C" fn plg_base_plugin_file_name(out: *mut c_char, capacity: usize) -> c_int {
    write_c_string(BASE_PLUGIN_FILE_NAME.as_bytes(), out, capacity)
}

/// The entry count `plg_build_formid_index_map` needs for `masters_len` masters:
/// always `masters_len + 1`, saturating rather than wrapping.
#[no_mangle]
pub extern "C" fn plg_formid_index_map_len(masters_len: usize) -> usize {
    masters_len.saturating_add(1)
}

/// Writes 1 to `*out_equal` when the two names match, 0 when they do not.
///
/// # Safety
/// `lhs` and `rhs` must be NUL-terminated strings, and `out_equal` must be
/// writable.
#[no_mangle]
pub unsafe extern "C" fn plg_plugin_name_equals(
    lhs: *const c_char,
    rhs: *const c_char,
    out_equal: *mut c_int,
) -> c_int {
    guard_value(PLG_ERROR_PANIC, || {
        clear_error();
        if lhs.is_null() || rhs.is_null() {
            set_error(
                PLG_ERROR_INVALID_ARGUMENT,
                "lhs and rhs must both be non-NULL",
            );
            return PLG_ERROR_INVALID_ARGUMENT;
        }
        if out_equal.is_null() {
            set_error(PLG_ERROR_INVALID_ARGUMENT, "out_equal is NULL");
            return PLG_ERROR_INVALID_ARGUMENT;
        }
        // SAFETY: both pointers are NUL-terminated strings per the caller.
        let lhs = unsafe { CStr::from_ptr(lhs) }.to_bytes();
        // SAFETY: as above.
        let rhs = unsafe { CStr::from_ptr(rhs) }.to_bytes();
        // SAFETY: `out_equal` is non-NULL and writable per the caller.
        unsafe { *out_equal = c_int::from(plugin_name_equals(lhs, rhs)) };
        PLG_OK
    })
}

/// Builds a FormID index map of `masters_len + 1` entries: one per MAST entry
/// resolved against the loaded file list, then `self_index`.
///
/// `out_map` holds `capacity` entries. A NULL `out_map` or a `capacity` of 0 is
/// a length query: nothing is written and `*out_len` receives the required
/// count. A short `capacity` returns `PLG_ERROR_BUFFER_TOO_SMALL` and still
/// records the required count in `*out_len`, so a caller can grow and retry.
/// `*out_len` is left untouched only when the call fails with
/// `PLG_ERROR_INVALID_ARGUMENT`; it may be NULL when the count is not wanted,
/// since the count is always `plg_formid_index_map_len(masters_len)`.
///
/// # Safety
/// `masters` and `loaded_file_names` must each point to that many
/// NUL-terminated strings, or be NULL with a count of 0. `out_map` must point to
/// `capacity` writable `uint32_t` values or be NULL, and `out_len` must be
/// writable or NULL.
#[no_mangle]
#[allow(clippy::too_many_arguments)]
pub unsafe extern "C" fn plg_build_formid_index_map(
    masters: *const *const c_char,
    masters_len: usize,
    loaded_file_names: *const *const c_char,
    loaded_len: usize,
    self_index: u32,
    out_map: *mut u32,
    capacity: usize,
    out_len: *mut usize,
) -> c_int {
    guard_value(PLG_ERROR_PANIC, || {
        clear_error();
        let required = masters_len.saturating_add(1);
        let Some(masters) = (unsafe { collect_names(masters, masters_len) }) else {
            return PLG_ERROR_INVALID_ARGUMENT;
        };
        let Some(loaded) = (unsafe { collect_names(loaded_file_names, loaded_len) }) else {
            return PLG_ERROR_INVALID_ARGUMENT;
        };
        if out_map.is_null() || capacity == 0 {
            if !out_len.is_null() {
                // SAFETY: the caller promises `out_len` is writable.
                unsafe { *out_len = required };
            }
            return PLG_OK;
        }
        if capacity < required {
            if !out_len.is_null() {
                // SAFETY: the caller promises `out_len` is writable.
                unsafe { *out_len = required };
            }
            set_error(
                PLG_ERROR_BUFFER_TOO_SMALL,
                format!("out_map holds {capacity} entries but {required} are required"),
            );
            return PLG_ERROR_BUFFER_TOO_SMALL;
        }
        let index_map = build_form_id_index_map(&masters, &loaded, self_index);
        debug_assert_eq!(index_map.len(), required);
        // SAFETY: `out_map` is non-NULL and `capacity >= required`, which is the
        // length of `index_map`, so that many writable u32 slots exist.
        unsafe { ptr::copy_nonoverlapping(index_map.as_ptr(), out_map, index_map.len()) };
        if !out_len.is_null() {
            // SAFETY: the caller promises `out_len` is writable.
            unsafe { *out_len = index_map.len() };
        }
        PLG_OK
    })
}

/// Rebases one FormID into load-order space through `index_map` and writes the
/// result to `*out_form_id`.
///
/// A FormID of 0, one whose index is past the end of the map, and one whose
/// index maps to `plg_unresolved_formid_index()` all come back unchanged. A NULL
/// `index_map` with `index_map_len` of 0 is valid and rebases nothing.
///
/// # Safety
/// `index_map` must point to `index_map_len` readable `uint32_t` values, or be
/// NULL with a length of 0. `out_form_id` must be writable.
#[no_mangle]
pub unsafe extern "C" fn plg_rebase_formid(
    form_id: u32,
    index_map: *const u32,
    index_map_len: usize,
    out_form_id: *mut u32,
) -> c_int {
    guard_value(PLG_ERROR_PANIC, || {
        clear_error();
        if out_form_id.is_null() {
            set_error(PLG_ERROR_INVALID_ARGUMENT, "out_form_id is NULL");
            return PLG_ERROR_INVALID_ARGUMENT;
        }
        if index_map.is_null() && index_map_len > 0 {
            set_error(
                PLG_ERROR_INVALID_ARGUMENT,
                "index_map is NULL but index_map_len is not 0",
            );
            return PLG_ERROR_INVALID_ARGUMENT;
        }
        let index_map: &[u32] = if index_map_len == 0 {
            &[]
        } else {
            // SAFETY: the caller promises `index_map_len` readable u32 values.
            unsafe { std::slice::from_raw_parts(index_map, index_map_len) }
        };
        // SAFETY: `out_form_id` is non-NULL and writable per the caller.
        unsafe { *out_form_id = rebase_form_id(form_id, index_map) };
        PLG_OK
    })
}

/// Parses the body of a `plugins.txt` file into `PlgPluginListEntry` values,
/// each naming a byte range inside `contents`.
///
/// `contents` is read for exactly `contents_len` bytes and is never treated as a
/// NUL-terminated string, so a file holding a name that is not valid UTF-8
/// parses the way it always did. No name is copied: the entries point back into
/// the caller's buffer, which must therefore stay alive as long as they are used.
///
/// `out_entries` holds `capacity` entries and `*out_len` receives the entry
/// count. A NULL `out_entries` or a `capacity` of 0 is a length query: nothing is
/// written and the count is still recorded, so a caller can size its buffer with
/// one extra call. A short `capacity` returns `PLG_ERROR_BUFFER_TOO_SMALL` with
/// the required count in `*out_len`.
///
/// # Safety
/// `contents` must point to `contents_len` readable bytes, or be NULL with a
/// length of 0. `out_entries` must point to `capacity` writable entries or be
/// NULL, and `out_len` must be writable.
#[no_mangle]
pub unsafe extern "C" fn plg_parse_plugin_list(
    contents: *const c_char,
    contents_len: usize,
    out_entries: *mut PlgPluginListEntry,
    capacity: usize,
    out_len: *mut usize,
) -> c_int {
    guard_value(PLG_ERROR_PANIC, || {
        clear_error();
        if contents.is_null() && contents_len > 0 {
            set_error(
                PLG_ERROR_INVALID_ARGUMENT,
                "contents is NULL but contents_len is not 0",
            );
            return PLG_ERROR_INVALID_ARGUMENT;
        }
        let contents: &[u8] = if contents_len == 0 {
            &[]
        } else {
            // SAFETY: the caller promises `contents_len` readable bytes.
            unsafe { std::slice::from_raw_parts(contents.cast::<u8>(), contents_len) }
        };
        let entries = parse_plugin_list(contents);
        let mapped: Vec<PlgPluginListEntry> = entries
            .iter()
            .map(|entry| PlgPluginListEntry {
                name_offset: entry.name_start,
                name_length: entry.name_len,
                active: c_int::from(entry.active),
            })
            .collect();
        // SAFETY: the caller's `out_entries`, `capacity` and `out_len` follow the
        // documented contract.
        unsafe {
            write_result(
                &mapped,
                out_entries,
                capacity,
                out_len,
                "plugin list entries",
            )
        }
    })
}

/// Serialises `names_len` names with their parallel active flags back into
/// `plugins.txt` text: one name per line, a leading `*` on every active entry
/// and a trailing newline after each.
///
/// The text is raw file bytes, so it is **not** NUL-terminated; `*out_len` carries
/// its exact length, and an empty selection yields a length of 0. Entries with an
/// empty name are skipped. A NULL `out` or a `capacity` of 0 is a length query.
///
/// # Safety
/// `names` must point to `names_len` NUL-terminated strings, or be NULL with a
/// length of 0, and `active_flags` must point to `names_len` readable `int`
/// values or be NULL with a length of 0. `out` must point to `capacity` writable
/// bytes or be NULL, and `out_len` must be writable.
#[no_mangle]
#[allow(clippy::too_many_arguments)]
pub unsafe extern "C" fn plg_serialize_plugin_list(
    names: *const *const c_char,
    active_flags: *const c_int,
    names_len: usize,
    out: *mut c_char,
    capacity: usize,
    out_len: *mut usize,
) -> c_int {
    guard_value(PLG_ERROR_PANIC, || {
        clear_error();
        // SAFETY: both arrays follow the documented contract.
        let Some(names) = (unsafe { collect_names(names, names_len) }) else {
            return PLG_ERROR_INVALID_ARGUMENT;
        };
        // SAFETY: as above.
        let Some(active) = (unsafe { collect_active_flags(active_flags, names_len) }) else {
            return PLG_ERROR_INVALID_ARGUMENT;
        };
        let text = serialize_plugin_list(&pair_names_with_flags(&names, &active));
        // SAFETY: the caller's `out`, `capacity` and `out_len` follow the
        // documented contract.
        unsafe { write_result(&text, out.cast::<u8>(), capacity, out_len, "bytes") }
    })
}

/// Writes the indices of the active, named entries to `out_indices`: the entries
/// a caller should load, in the order they were given.
///
/// An entry counts as active when its flag is non-zero and its name is not empty.
/// `*out_len` receives the index count, a NULL `out_indices` or a `capacity` of 0
/// is a length query, and a short `capacity` returns
/// `PLG_ERROR_BUFFER_TOO_SMALL` with the required count in `*out_len`.
///
/// # Safety
/// `names` must point to `names_len` NUL-terminated strings, or be NULL with a
/// length of 0, and `active_flags` must point to `names_len` readable `int`
/// values or be NULL with a length of 0. `out_indices` must point to `capacity`
/// writable `size_t` values or be NULL, and `out_len` must be writable.
#[no_mangle]
#[allow(clippy::too_many_arguments)]
pub unsafe extern "C" fn plg_active_plugin_indices(
    names: *const *const c_char,
    active_flags: *const c_int,
    names_len: usize,
    out_indices: *mut usize,
    capacity: usize,
    out_len: *mut usize,
) -> c_int {
    guard_value(PLG_ERROR_PANIC, || {
        clear_error();
        // SAFETY: both arrays follow the documented contract.
        let Some(names) = (unsafe { collect_names(names, names_len) }) else {
            return PLG_ERROR_INVALID_ARGUMENT;
        };
        // SAFETY: as above.
        let Some(active) = (unsafe { collect_active_flags(active_flags, names_len) }) else {
            return PLG_ERROR_INVALID_ARGUMENT;
        };
        let indices = active_plugin_indices(&pair_names_with_flags(&names, &active));
        // SAFETY: the caller's `out_indices`, `capacity` and `out_len` follow the
        // documented contract.
        unsafe { write_result(&indices, out_indices, capacity, out_len, "indices") }
    })
}

/// Writes the retail load order of `names` to `out_order` as indices into
/// `names`: `Oblivion.esm` first, then the other masters, then the plugins, each
/// group ordered case-insensitively by name.
///
/// A permutation is returned rather than a sorted copy so a caller can apply it
/// to its own entries, which usually carry more than a name. The sort is stable,
/// so names that tie keep their incoming order. `*out_len` receives the index
/// count, a NULL `out_order` or a `capacity` of 0 is a length query, and a short
/// `capacity` returns `PLG_ERROR_BUFFER_TOO_SMALL` with the required count in
/// `*out_len`.
///
/// # Safety
/// `names` must point to `names_len` NUL-terminated strings, or be NULL with a
/// length of 0. `out_order` must point to `capacity` writable `size_t` values or
/// be NULL, and `out_len` must be writable.
#[no_mangle]
pub unsafe extern "C" fn plg_sort_plugin_load_order(
    names: *const *const c_char,
    names_len: usize,
    out_order: *mut usize,
    capacity: usize,
    out_len: *mut usize,
) -> c_int {
    guard_value(PLG_ERROR_PANIC, || {
        clear_error();
        // SAFETY: the array follows the documented contract.
        let Some(names) = (unsafe { collect_names(names, names_len) }) else {
            return PLG_ERROR_INVALID_ARGUMENT;
        };
        let order = sort_plugin_load_order(&names);
        // SAFETY: the caller's `out_order`, `capacity` and `out_len` follow the
        // documented contract.
        unsafe { write_result(&order, out_order, capacity, out_len, "indices") }
    })
}

/// Writes 1 to `*out_is_master` when `name` ends in `.esm`, 0 when it does not.
///
/// Only the final four bytes are examined, so `"esm"` is too short to be a master
/// and `"Mod.esm.bak"` ends in something else.
///
/// # Safety
/// `name` must be a NUL-terminated string and `out_is_master` must be writable.
#[no_mangle]
pub unsafe extern "C" fn plg_is_master_file_name(
    name: *const c_char,
    out_is_master: *mut c_int,
) -> c_int {
    guard_value(PLG_ERROR_PANIC, || {
        clear_error();
        if name.is_null() {
            set_error(PLG_ERROR_INVALID_ARGUMENT, "name is NULL");
            return PLG_ERROR_INVALID_ARGUMENT;
        }
        if out_is_master.is_null() {
            set_error(PLG_ERROR_INVALID_ARGUMENT, "out_is_master is NULL");
            return PLG_ERROR_INVALID_ARGUMENT;
        }
        // SAFETY: `name` is a NUL-terminated string per the caller.
        let name = unsafe { CStr::from_ptr(name) }.to_bytes();
        // SAFETY: `out_is_master` is non-NULL and writable per the caller.
        unsafe { *out_is_master = c_int::from(is_master_file_name(name)) };
        PLG_OK
    })
}

/// The error code left by the last computing call on this thread.
#[no_mangle]
pub extern "C" fn plg_last_error_code() -> c_int {
    LAST_ERROR.with(|state| state.borrow().code)
}
/// The message left by the last computing call on this thread (snprintf style).
///
/// # Safety
/// `out` must point to `capacity` writable bytes or be NULL.
#[no_mangle]
pub unsafe extern "C" fn plg_last_error_message(out: *mut c_char, capacity: usize) -> c_int {
    LAST_ERROR.with(|state| write_c_string(state.borrow().message.as_bytes(), out, capacity))
}

/// A static, NUL-terminated version banner, for logs and crash reports.
#[no_mangle]
pub extern "C" fn plg_version() -> *const c_char {
    VERSION.as_ptr().cast::<c_char>()
}
