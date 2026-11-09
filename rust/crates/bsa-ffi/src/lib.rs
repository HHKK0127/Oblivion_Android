//! C ABI over the `bsa-core` crate.
//!
//! Unlike the other FFI crates in this workspace, this one owns state: a BSA
//! archive holds an open file handle, the parsed index, and a reusable
//! extraction buffer. The C side therefore works with an opaque handle created
//! by [`bsa_create`] and released by [`bsa_destroy`], and every other call takes
//! that handle as its first argument.
//!
//! A handle must be used on one thread at a time. The C++ shim keeps one handle
//! per `BSArchive` object, which is what the original reader did with its
//! `std::ifstream`.
//!
//! Paths to look up cross the boundary as raw bytes plus a length, because
//! archive names are not guaranteed to be valid UTF-8 and a caller may hand over
//! a path with embedded NULs. Only [`bsa_open`] takes a NUL-terminated string,
//! because it names a file on the device.
//!
//! Extraction returns a pointer into the archive's own buffer rather than
//! filling a caller buffer, so the caller copies what it needs before the next
//! extract call. Search results are the same: a pointer into the archive's
//! scratch list, valid until the next search on that handle.
//!
//! Threading: each thread has its own last-error state, so [`bsa_last_error_code`]
//! and [`bsa_last_error_message`] only describe calls made on the calling thread.
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
use std::ffi::{c_char, c_int, CStr};
use std::panic::catch_unwind;

use bsa_core::{Archive, BsaError};

/// The call succeeded.
pub const BSA_OK: c_int = 0;
/// The call found what it was looking for.
pub const BSA_FOUND: c_int = 1;
/// The call succeeded but found nothing.
pub const BSA_NOT_FOUND: c_int = 0;
/// A required pointer argument was NULL, or a pointer was misaligned.
pub const BSA_ERROR_INVALID_ARGUMENT: c_int = -1;
/// The archive file could not be opened or read.
pub const BSA_ERROR_IO: c_int = -2;
/// The first four bytes are not `BSA\0`.
pub const BSA_ERROR_BAD_MAGIC: c_int = -3;
/// A structure the header points at runs past the end of the file.
pub const BSA_ERROR_TRUNCATED: c_int = -4;
/// An entry's data could not be inflated.
pub const BSA_ERROR_DECOMPRESS: c_int = -5;
/// The requested entry index does not exist.
pub const BSA_ERROR_INDEX_OUT_OF_RANGE: c_int = -6;
/// A Rust panic was caught at the boundary; the call had no effect.
pub const BSA_ERROR_PANIC: c_int = -7;
/// An output buffer cannot hold the result; the out-parameter holds the size.
pub const BSA_ERROR_BUFFER_TOO_SMALL: c_int = -8;
/// The archive is not open.
pub const BSA_ERROR_NOT_OPEN: c_int = -9;
/// The header asks for more structure than this reader accepts.
pub const BSA_ERROR_CORRUPT: c_int = -10;

/// A static, NUL-terminated version banner.
const VERSION: &str = concat!("bsa-ffi ", env!("CARGO_PKG_VERSION"), "\0");

/// Field selector for [`bsa_entry_text`]: the folder and file name joined with
/// `\`, as stored in the archive.
pub const BSA_FIELD_FULL_PATH: c_int = 0;
/// Field selector for [`bsa_entry_text`]: the lookup path, with `/` separators
/// and lowercase ASCII.
pub const BSA_FIELD_LOOKUP_PATH: c_int = 1;
/// Field selector for [`bsa_entry_text`]: the file name on its own.
pub const BSA_FIELD_FILE_NAME: c_int = 2;
/// Field selector for [`bsa_entry_text`]: the extension, without its dot.
pub const BSA_FIELD_EXTENSION: c_int = 3;
/// Field selector for [`bsa_entry_text`]: the folder name, empty when the
/// archive has no folder names.
pub const BSA_FIELD_FOLDER_PATH: c_int = 4;

/// Marker written into a live handle, so a stale or wrong pointer fails the
/// magic check instead of reaching an `Archive` that is not there.
const HANDLE_MAGIC: u32 = 0x4253_4148;

/// The nine header fields, in the order the archive stores them.
///
/// The layout mirrors `bsa_header` in `include/bsa_ffi.h`.
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct BsaHeader {
    pub magic: u32,
    pub version: u32,
    pub folder_record_offset: u32,
    pub archive_flags: u32,
    pub folder_count: u32,
    pub file_count: u32,
    pub folder_name_length: u32,
    pub file_name_length: u32,
    pub file_flags: u32,
}

/// The per-entry numbers, without the strings.
///
/// The layout mirrors `bsa_entry_info` in `include/bsa_ffi.h`.
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct BsaEntryInfo {
    pub hash: u64,
    pub offset: u32,
    pub size: u32,
    pub real_size: u32,
    pub compressed: u32,
}

// The header is nine `uint32_t` fields and the entry info is one `uint64_t` plus
// four `uint32_t` fields on both sides, so a mismatch could only come from a bad
// `uint32_t` mapping. The asserts make that a build failure instead.
const _: () = assert!(size_of::<c_int>() == 4);
const _: () = assert!(size_of::<BsaHeader>() == 9 * size_of::<u32>());
const _: () = assert!(align_of::<BsaHeader>() == align_of::<u32>());
const _: () = assert!(size_of::<BsaEntryInfo>() == 24);
const _: () = assert!(align_of::<BsaEntryInfo>() == align_of::<u64>());

/// What a handle points at: a magic marker in front of the archive itself.
///
/// The type is public only so the `extern "C"` signatures can name it; both fields
/// stay private, so C callers see the `void *` from `bsa_ffi.h` and Rust callers
/// cannot construct or inspect one.
pub struct Handle {
    magic: u32,
    archive: Archive,
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
    set_error(BSA_OK, String::new());
}

/// Run `body`, turning a caught panic into the panic code plus `fallback`.
///
/// Every exported function routes through here: a Rust panic must not unwind
/// into the C++ frames above, which are not built to take it.
fn guard_value<T: Copy>(fallback: T, body: impl FnOnce() -> T) -> T {
    match catch_unwind(AssertUnwindSafe(body)) {
        Ok(value) => value,
        Err(_) => {
            set_error(BSA_ERROR_PANIC, "panic caught at the bsa-ffi boundary");
            fallback
        }
    }
}

/// Turn a core error into its C code, leaving the message on the error state.
fn report(error: BsaError) -> c_int {
    let code = match error {
        BsaError::Io => BSA_ERROR_IO,
        BsaError::NotOpen => BSA_ERROR_NOT_OPEN,
        BsaError::BadMagic => BSA_ERROR_BAD_MAGIC,
        BsaError::Truncated => BSA_ERROR_TRUNCATED,
        BsaError::Corrupt => BSA_ERROR_CORRUPT,
        BsaError::Decompress => BSA_ERROR_DECOMPRESS,
        BsaError::IndexOutOfRange => BSA_ERROR_INDEX_OUT_OF_RANGE,
    };
    set_error(code, format!("{error}"));
    code
}

fn invalid_argument(what: &str) -> c_int {
    set_error(BSA_ERROR_INVALID_ARGUMENT, what);
    BSA_ERROR_INVALID_ARGUMENT
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

/// Borrow the handle behind a caller pointer, checking the magic marker first.
///
/// # Safety
/// `handle` must be NULL or a pointer returned by [`bsa_create`] that has not
/// been passed to [`bsa_destroy`].
unsafe fn borrow_handle<'a>(handle: *mut Handle) -> Result<&'a mut Handle, c_int> {
    if handle.is_null() {
        return Err(invalid_argument("the archive handle is NULL"));
    }
    let borrowed = &mut *handle;
    if borrowed.magic != HANDLE_MAGIC {
        return Err(invalid_argument("the archive handle is not a live handle"));
    }
    Ok(borrowed)
}

/// Borrow `length` readable bytes from a caller pointer, allowing an empty run.
///
/// # Safety
/// When `length` is non-zero, `pointer` must address `length` readable bytes
/// that stay alive and unaliased for `'a`.
unsafe fn borrow_bytes<'a>(pointer: *const c_char, length: usize) -> Result<&'a [u8], c_int> {
    if length == 0 {
        return Ok(&[]);
    }
    if pointer.is_null() {
        return Err(invalid_argument(
            "a byte range is NULL but its length is not zero",
        ));
    }
    Ok(slice::from_raw_parts(pointer.cast::<u8>(), length))
}

/// A static, NUL-terminated version banner, for logs and crash reports.
#[no_mangle]
pub extern "C" fn bsa_version() -> *const c_char {
    VERSION.as_ptr().cast::<c_char>()
}

/// The error code left by the last call on this thread.
#[no_mangle]
pub extern "C" fn bsa_last_error_code() -> c_int {
    LAST_ERROR.with(|state| state.borrow().code)
}

/// The message left by the last call on this thread (snprintf style).
///
/// # Safety
/// `out` must point to `capacity` writable bytes or be NULL.
#[no_mangle]
pub unsafe extern "C" fn bsa_last_error_message(out: *mut c_char, capacity: usize) -> c_int {
    LAST_ERROR.with(|state| write_c_string(state.borrow().message.as_bytes(), out, capacity))
}

/// Creates a closed archive and returns its handle, or NULL if none could be
/// allocated.
#[no_mangle]
pub extern "C" fn bsa_create() -> *mut Handle {
    guard_value(core::ptr::null_mut(), || {
        clear_error();
        let handle = Box::new(Handle {
            magic: HANDLE_MAGIC,
            archive: Archive::new(),
        });
        Box::into_raw(handle)
    })
}

/// Closes the archive and frees the handle.
///
/// The caller's pointer is not cleared, because C cannot be given one. Passing
/// the same handle twice is a caller bug the magic check turns into a no-op.
///
/// # Safety
/// `handle` must be NULL or a pointer returned by [`bsa_create`].
#[no_mangle]
pub unsafe extern "C" fn bsa_destroy(handle: *mut Handle) {
    guard_value((), || {
        if handle.is_null() {
            return;
        }
        let owned = &mut *handle;
        if owned.magic != HANDLE_MAGIC {
            set_error(
                BSA_ERROR_INVALID_ARGUMENT,
                "the archive handle is not a live handle",
            );
            return;
        }
        owned.magic = 0;
        drop(Box::from_raw(handle));
    });
}

/// Opens `path` and parses its index.
///
/// A failed open still closes whatever the handle held before, and leaves the
/// archive closed, so a caller can react to the code and try another path.
///
/// # Safety
/// `path` must be a NUL-terminated string; `handle` must be a live handle.
#[no_mangle]
pub unsafe extern "C" fn bsa_open(handle: *mut Handle, path: *const c_char) -> c_int {
    guard_value(BSA_ERROR_PANIC, || {
        clear_error();
        let borrowed = match borrow_handle(handle) {
            Ok(borrowed) => borrowed,
            Err(code) => return code,
        };
        if path.is_null() {
            return invalid_argument("the archive path is NULL");
        }
        let text = match CStr::from_ptr(path).to_str() {
            Ok(text) => text,
            Err(_) => return invalid_argument("the archive path is not valid UTF-8"),
        };
        match borrowed.archive.open(text) {
            Ok(()) => BSA_OK,
            Err(error) => report(error),
        }
    })
}

/// Closes the archive, dropping the file handle and the parsed index.
///
/// # Safety
/// `handle` must be a live handle.
#[no_mangle]
pub unsafe extern "C" fn bsa_close(handle: *mut Handle) -> c_int {
    guard_value(BSA_ERROR_PANIC, || {
        clear_error();
        match borrow_handle(handle) {
            Ok(borrowed) => {
                borrowed.archive.close();
                BSA_OK
            }
            Err(code) => code,
        }
    })
}

/// Whether the archive holds an open file. 1 when it does, 0 when it does not.
///
/// # Safety
/// `handle` must be a live handle.
#[no_mangle]
pub unsafe extern "C" fn bsa_is_open(handle: *mut Handle) -> c_int {
    guard_value(BSA_ERROR_PANIC, || match borrow_handle(handle) {
        Ok(borrowed) => borrowed.archive.is_open() as c_int,
        Err(code) => code,
    })
}

/// Whether the archive index finished parsing. 1 when it did, 0 when it did not.
///
/// # Safety
/// `handle` must be a live handle.
#[no_mangle]
pub unsafe extern "C" fn bsa_is_parsed(handle: *mut Handle) -> c_int {
    guard_value(BSA_ERROR_PANIC, || match borrow_handle(handle) {
        Ok(borrowed) => borrowed.archive.is_parsed() as c_int,
        Err(code) => code,
    })
}

/// Copies the header into `out`.
///
/// # Safety
/// `out` must point to a writable `bsa_header`; `handle` must be a live handle.
#[no_mangle]
pub unsafe extern "C" fn bsa_get_header(handle: *mut Handle, out: *mut BsaHeader) -> c_int {
    guard_value(BSA_ERROR_PANIC, || {
        clear_error();
        let borrowed = match borrow_handle(handle) {
            Ok(borrowed) => borrowed,
            Err(code) => return code,
        };
        if out.is_null() {
            return invalid_argument("the header output is NULL");
        }
        let header = borrowed.archive.header();
        *out = BsaHeader {
            magic: header.magic,
            version: header.version,
            folder_record_offset: header.folder_record_offset,
            archive_flags: header.archive_flags,
            folder_count: header.folder_count,
            file_count: header.file_count,
            folder_name_length: header.folder_name_length,
            file_name_length: header.file_name_length,
            file_flags: header.file_flags,
        };
        BSA_OK
    })
}

/// The file count the header declares.
///
/// # Safety
/// `handle` must be a live handle.
#[no_mangle]
pub unsafe extern "C" fn bsa_file_count(handle: *mut Handle) -> u32 {
    guard_value(0, || match borrow_handle(handle) {
        Ok(borrowed) => borrowed.archive.file_count(),
        Err(_) => 0,
    })
}

/// The number of entries the parse produced.
///
/// # Safety
/// `handle` must be a live handle.
#[no_mangle]
pub unsafe extern "C" fn bsa_entry_count(handle: *mut Handle) -> usize {
    guard_value(0, || match borrow_handle(handle) {
        Ok(borrowed) => borrowed.archive.entries().len(),
        Err(_) => 0,
    })
}

/// Copies up to `capacity` entry records into `out` and returns how many the
/// archive holds in total.
///
/// A NULL `out` or a zero `capacity` only reports the count, so a caller can
/// size its array first. The strings are fetched separately, with
/// [`bsa_entry_text`], because they have no fixed width.
///
/// # Safety
/// `out` must point to `capacity` writable `bsa_entry_info` values, or be NULL.
#[no_mangle]
pub unsafe extern "C" fn bsa_get_files(
    handle: *mut Handle,
    out: *mut BsaEntryInfo,
    capacity: usize,
) -> usize {
    guard_value(0, || {
        clear_error();
        let borrowed = match borrow_handle(handle) {
            Ok(borrowed) => borrowed,
            Err(_) => return 0,
        };
        let entries = borrowed.archive.entries();
        let total = entries.len();
        if out.is_null() || capacity == 0 {
            return total;
        }
        if (out as usize) % align_of::<BsaEntryInfo>() != 0 {
            invalid_argument("the entry output is misaligned");
            return 0;
        }
        let writable = slice::from_raw_parts_mut(out, capacity.min(total));
        for (slot, entry) in writable.iter_mut().zip(entries) {
            *slot = BsaEntryInfo {
                hash: entry.hash,
                offset: entry.offset,
                size: entry.size,
                real_size: entry.real_size,
                compressed: u32::from(entry.compressed),
            };
        }
        total
    })
}

/// Copies one of an entry's strings into `out` (snprintf style).
///
/// `field` is one of the `BSA_FIELD_*` constants. An index past the end or an
/// unknown field is a `BSA_ERROR_INVALID_ARGUMENT`.
///
/// # Safety
/// `out` must point to `capacity` writable bytes or be NULL.
#[no_mangle]
pub unsafe extern "C" fn bsa_entry_text(
    handle: *mut Handle,
    index: usize,
    field: c_int,
    out: *mut c_char,
    capacity: usize,
) -> c_int {
    guard_value(BSA_ERROR_PANIC, || {
        clear_error();
        let borrowed = match borrow_handle(handle) {
            Ok(borrowed) => borrowed,
            Err(code) => return code,
        };
        let Some(entry) = borrowed.archive.entries().get(index) else {
            return invalid_argument("the entry index is out of range");
        };
        let text: &[u8] = match field {
            BSA_FIELD_FULL_PATH => &entry.full_path,
            BSA_FIELD_LOOKUP_PATH => &entry.lookup_path,
            BSA_FIELD_FILE_NAME => &entry.file_name,
            BSA_FIELD_EXTENSION => &entry.extension,
            BSA_FIELD_FOLDER_PATH => &entry.folder_path,
            _ => return invalid_argument("the entry field selector is unknown"),
        };
        write_c_string(text, out, capacity)
    })
}

/// Finds an entry by exact path, accepting either separator and any case.
///
/// Writes the entry index to `out_index` and returns `BSA_FOUND`, or returns
/// `BSA_NOT_FOUND` without touching `out_index`. A hash collision is rejected by
/// comparing the full path, as the engine does.
///
/// # Safety
/// `path` must point to `path_length` readable bytes or be NULL with a zero
/// length; `out_index` must be non-NULL and writable.
#[no_mangle]
pub unsafe extern "C" fn bsa_find_file(
    handle: *mut Handle,
    path: *const c_char,
    path_length: usize,
    out_index: *mut usize,
) -> c_int {
    guard_value(BSA_ERROR_PANIC, || {
        clear_error();
        let borrowed = match borrow_handle(handle) {
            Ok(borrowed) => borrowed,
            Err(code) => return code,
        };
        if out_index.is_null() {
            return invalid_argument("out_index must not be NULL");
        }
        let probe = match borrow_bytes(path, path_length) {
            Ok(probe) => probe,
            Err(code) => return code,
        };
        match borrowed.archive.find_file(probe) {
            Some(index) => {
                *out_index = index;
                BSA_FOUND
            }
            None => BSA_NOT_FOUND,
        }
    })
}

/// Finds every entry whose lookup path starts with `prefix`.
///
/// Writes the scratch list to `out_indices` and its length to `out_count`, then
/// returns how many entries matched. The list stays valid until the next search
/// on this handle, and extraction does not disturb it.
///
/// # Safety
/// `prefix` must point to `prefix_length` readable bytes or be NULL with a zero
/// length; both out-parameters must be non-NULL and writable.
#[no_mangle]
pub unsafe extern "C" fn bsa_find_files_by_prefix(
    handle: *mut Handle,
    prefix: *const c_char,
    prefix_length: usize,
    out_indices: *mut *const usize,
    out_count: *mut usize,
) -> c_int {
    guard_value(BSA_ERROR_PANIC, || {
        clear_error();
        match borrow_handle(handle) {
            Ok(borrowed) => search(
                borrowed,
                prefix,
                prefix_length,
                out_indices,
                out_count,
                true,
            ),
            Err(code) => code,
        }
    })
}

/// Finds every entry whose extension equals `extension`, ignoring ASCII case.
///
/// The extension is written without its dot. Behaves like
/// [`bsa_find_files_by_prefix`] otherwise.
///
/// # Safety
/// `extension` must point to `extension_length` readable bytes or be NULL with a
/// zero length; both out-parameters must be non-NULL and writable.
#[no_mangle]
pub unsafe extern "C" fn bsa_find_files_by_extension(
    handle: *mut Handle,
    extension: *const c_char,
    extension_length: usize,
    out_indices: *mut *const usize,
    out_count: *mut usize,
) -> c_int {
    guard_value(BSA_ERROR_PANIC, || {
        clear_error();
        match borrow_handle(handle) {
            Ok(borrowed) => search(
                borrowed,
                extension,
                extension_length,
                out_indices,
                out_count,
                false,
            ),
            Err(code) => code,
        }
    })
}

/// Shared body of the two search entry points.
///
/// # Safety
/// The pointer arguments must satisfy the contracts of the two callers.
unsafe fn search(
    handle: &mut Handle,
    needle: *const c_char,
    needle_length: usize,
    out_indices: *mut *const usize,
    out_count: *mut usize,
    by_prefix: bool,
) -> c_int {
    if out_indices.is_null() || out_count.is_null() {
        return invalid_argument("out_indices and out_count must not be NULL");
    }
    let probe = match borrow_bytes(needle, needle_length) {
        Ok(probe) => probe,
        Err(code) => return code,
    };
    let found = if by_prefix {
        handle.archive.find_files_by_prefix(probe)
    } else {
        handle.archive.find_files_by_extension(probe)
    };
    *out_indices = found.as_ptr();
    *out_count = found.len();
    found.len() as c_int
}

/// Points `out_data` and `out_length` at an entry's stored bytes.
///
/// # Safety
/// Both out-parameters must be non-NULL and writable; `handle` must be a live
/// handle.
#[no_mangle]
pub unsafe extern "C" fn bsa_extract_raw(
    handle: *mut Handle,
    index: usize,
    out_data: *mut *const u8,
    out_length: *mut usize,
) -> c_int {
    guard_value(BSA_ERROR_PANIC, || {
        clear_error();
        extract(handle, index, out_data, out_length, false)
    })
}

/// Points `out_data` and `out_length` at an entry's bytes, inflating them when
/// the entry is compressed.
///
/// # Safety
/// Both out-parameters must be non-NULL and writable; `handle` must be a live
/// handle.
#[no_mangle]
pub unsafe extern "C" fn bsa_extract_decompressed(
    handle: *mut Handle,
    index: usize,
    out_data: *mut *const u8,
    out_length: *mut usize,
) -> c_int {
    guard_value(BSA_ERROR_PANIC, || {
        clear_error();
        extract(handle, index, out_data, out_length, true)
    })
}

/// Shared body of the two extraction entry points.
///
/// # Safety
/// The pointer arguments must satisfy the contracts of the two callers.
unsafe fn extract(
    handle: *mut Handle,
    index: usize,
    out_data: *mut *const u8,
    out_length: *mut usize,
    decompress: bool,
) -> c_int {
    let borrowed = match borrow_handle(handle) {
        Ok(borrowed) => borrowed,
        Err(code) => return code,
    };
    if out_data.is_null() || out_length.is_null() {
        return invalid_argument("out_data and out_length must not be NULL");
    }
    let result = if decompress {
        borrowed.archive.extract_decompressed(index)
    } else {
        borrowed.archive.extract_raw(index)
    };
    match result {
        Ok(bytes) => {
            *out_data = bytes.as_ptr();
            *out_length = bytes.len();
            BSA_OK
        }
        Err(error) => report(error),
    }
}
