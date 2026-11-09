//! C ABI over the `dds-core` crate — the texture wave.
//!
//! A handle owns one DDS image: the parsed header, the mapped compression
//! format, the compressed payload and, once [`dds_decompress`] runs, the
//! decoded RGBA8 buffer. The C++ `DDSLoader` shim keeps one handle per loader
//! object and hands every call that handle, which is what the original class
//! did with its `header` and `DDSTexture` members.
//!
//! The decoded RGBA buffer **borrows** from the handle: the pointer returned by
//! [`dds_decompressed_data`] stays valid until the next `dds_parse_memory` or
//! `dds_decompress` on that handle. Copy what you need before reusing the
//! handle. The width/height/format accessors only write the message slot, so a
//! caller may interleave them with its copy. That matches how the loader is used
//! on the C++ side — `uploadToGPU()` reads the decoded pixels once, and
//! `getTexture()` only reports the dimensions.
//!
//! Every exported function runs inside a panic guard: a Rust panic must not
//! unwind into the C++ frames above, which are not built to take it.

use core::panic::AssertUnwindSafe;
use core::slice;
use std::ffi::{c_char, c_int};
use std::panic::catch_unwind;

use dds_core::{self, DDSError, DdsFile};

/// The call succeeded.
pub const DDS_OK: c_int = 0;
/// A required pointer argument was NULL, or a pointer was misaligned.
pub const DDS_ERROR_INVALID_ARGUMENT: c_int = -1;
/// The magic is not `"DDS "`.
pub const DDS_ERROR_BAD_MAGIC: c_int = -2;
/// The 4-byte magic or the 124-byte header runs past the end of the file.
pub const DDS_ERROR_TRUNCATED: c_int = -3;
/// The header is missing the width or height flag.
pub const DDS_ERROR_MISSING_DIMENSIONS: c_int = -4;
/// A DXT-compressed format had no payload to decode.
pub const DDS_ERROR_EMPTY: c_int = -5;
/// The compression format is neither DXT1/3/5/RXGB nor an RGB payload.
pub const DDS_ERROR_UNSUPPORTED: c_int = -6;
/// An uncompressed payload declared a bit count other than 24 or 32.
pub const DDS_ERROR_BIT_COUNT: c_int = -7;
/// An uncompressed payload holds fewer bytes than width x height x bpp.
pub const DDS_ERROR_TRUNCATED_DATA: c_int = -8;
/// A Rust panic was caught at the boundary; the call had no effect.
pub const DDS_ERROR_PANIC: c_int = -9;

/// What a handle points at: a magic marker in front of the owned DDS state.
///
/// The type is public only so the `extern "C"` signatures can name it; both
/// fields stay private, so C callers see the `void *` from `dds_ffi.h`.
pub struct Handle {
    magic: u32,
    file: Option<DdsFile>,
    /// Decoded RGBA8 pixels; borrows out to `dds_decompressed_data`.
    decompressed: Vec<u8>,
    /// NUL-terminated message handed out by [`dds_last_error`].
    message: Vec<u8>,
}

/// Marker written into a live handle, so a stale or wrong pointer fails the
/// magic check instead of reaching a `Handle` that is not there.
const HANDLE_MAGIC: u32 = 0x4444_5348; // "DDSH"

const MSG_OK: &[u8] = b"\0";

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
    DDS_OK
}

/// Turn a parse/decode error into its C code, leaving the message on the handle.
fn report(handle: &mut Handle, error: DDSError) -> c_int {
    let code = match error {
        DDSError::TruncatedMagic | DDSError::TruncatedHeader => DDS_ERROR_TRUNCATED,
        DDSError::BadMagic => DDS_ERROR_BAD_MAGIC,
        DDSError::MissingDimensions => DDS_ERROR_MISSING_DIMENSIONS,
        DDSError::EmptyData => DDS_ERROR_EMPTY,
        DDSError::UnsupportedFormat => DDS_ERROR_UNSUPPORTED,
        DDSError::BadBitCount => DDS_ERROR_BIT_COUNT,
        DDSError::TruncatedData => DDS_ERROR_TRUNCATED_DATA,
    };
    fail(handle, code, error)
}

/// Borrow the handle behind a caller pointer, checking the magic marker first.
///
/// # Safety
/// `handle` must be NULL or a pointer returned by [`dds_create`] that has not
/// been passed to [`dds_destroy`].
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
/// `fallback`. A NULL or stale handle never reaches `body`; `dds_last_error`
/// explains it to the caller.
///
/// # Safety
/// `handle` must be NULL or a pointer returned by [`dds_create`] that has not
/// been passed to [`dds_destroy`].
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
            if let Ok(borrowed) = borrow_handle(handle) {
                set_message(borrowed, "panic caught at the dds-ffi boundary");
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

/// Creates a new empty DDS handle. The caller releases it with [`dds_destroy`].
#[no_mangle]
pub extern "C" fn dds_create() -> *mut Handle {
    let handle = Box::new(Handle {
        magic: HANDLE_MAGIC,
        file: None,
        decompressed: Vec::new(),
        message: MSG_OK.to_vec(),
    });
    Box::into_raw(handle)
}

/// Destroys a handle created by [`dds_create`]. NULL is ignored.
///
/// # Safety
/// `handle` must be NULL or a pointer returned by [`dds_create`] that has not
/// been passed to `dds_destroy` before.
#[no_mangle]
pub unsafe extern "C" fn dds_destroy(handle: *mut Handle) {
    if handle.is_null() {
        return;
    }
    if borrow_handle(handle).is_ok() {
        drop(Box::from_raw(handle));
    }
}

/// Parses a whole DDS image (magic + header + payload) from memory into the
/// handle, replacing any previous state.
///
/// # Safety
/// `handle` must be a live handle from [`dds_create`]. When `length` is
/// non-zero, `data` must address `length` readable bytes.
#[no_mangle]
pub unsafe extern "C" fn dds_parse_memory(
    handle: *mut Handle,
    data: *const u8,
    length: usize,
) -> c_int {
    guard(DDS_ERROR_INVALID_ARGUMENT, handle, |h| {
        let bytes = match borrow_bytes(data, length) {
            Ok(bytes) => bytes,
            Err(()) => return fail(h, DDS_ERROR_INVALID_ARGUMENT, "DDS data pointer is NULL"),
        };
        h.decompressed.clear();
        match dds_core::parse(bytes) {
            Ok(file) => {
                h.file = Some(file);
                succeed(h)
            }
            Err(error) => {
                h.file = None;
                report(h, error)
            }
        }
    })
}

/// Decodes the parsed image to RGBA8, storing the pixels in the handle.
///
/// # Safety
/// `handle` must be a live handle from [`dds_create`].
#[no_mangle]
pub unsafe extern "C" fn dds_decompress(handle: *mut Handle) -> c_int {
    guard(DDS_ERROR_INVALID_ARGUMENT, handle, |h| {
        let file = match &h.file {
            Some(file) => file,
            None => return fail(h, DDS_ERROR_INVALID_ARGUMENT, "no DDS image parsed"),
        };
        match dds_core::decompress(file) {
            Ok(rgba) => {
                h.decompressed = rgba;
                succeed(h)
            }
            Err(error) => report(h, error),
        }
    })
}

/// Reports the parsed image width in pixels, or 0 when nothing is parsed.
///
/// # Safety
/// `handle` must be a live handle from [`dds_create`].
#[no_mangle]
pub unsafe extern "C" fn dds_width(handle: *mut Handle) -> u32 {
    guard(0, handle, |h| h.file.as_ref().map_or(0, |f| f.header.width))
}

/// Reports the parsed image height in pixels, or 0 when nothing is parsed.
///
/// # Safety
/// `handle` must be a live handle from [`dds_create`].
#[no_mangle]
pub unsafe extern "C" fn dds_height(handle: *mut Handle) -> u32 {
    guard(0, handle, |h| h.file.as_ref().map_or(0, |f| f.header.height))
}

/// Reports the mipmap count, clamped to at least 1 like the original loader.
///
/// # Safety
/// `handle` must be a live handle from [`dds_create`].
#[no_mangle]
pub unsafe extern "C" fn dds_mipmap_count(handle: *mut Handle) -> u32 {
    guard(0, handle, |h| {
        h.file.as_ref().map_or(0, |f| {
            if f.header.mipmap_count > 0 { f.header.mipmap_count } else { 1 }
        })
    })
}

/// Reports the compression format as a `DDSCompressionFormat` value.
///
/// # Safety
/// `handle` must be a live handle from [`dds_create`].
#[no_mangle]
pub unsafe extern "C" fn dds_compression_format(handle: *mut Handle) -> c_int {
    guard(5, handle, |h| {
        h.file.as_ref().map_or(5, |f| f.compression_format as c_int)
    })
}

/// Borrows the decoded RGBA8 buffer: writes the byte pointer to `out_data` and
/// its length to `out_len`. Both must be non-NULL. The pointer stays valid
/// until the next `dds_parse_memory` or `dds_decompress` on this handle.
///
/// # Safety
/// `handle` must be a live handle from [`dds_create`]. `out_data` and `out_len`
/// must be writable.
#[no_mangle]
pub unsafe extern "C" fn dds_decompressed_data(
    handle: *mut Handle,
    out_data: *mut *const u8,
    out_len: *mut usize,
) -> c_int {
    guard(DDS_ERROR_INVALID_ARGUMENT, handle, |h| {
        if out_data.is_null() || out_len.is_null() {
            return fail(h, DDS_ERROR_INVALID_ARGUMENT, "out pointer is NULL");
        }
        if h.decompressed.is_empty() {
            *out_data = core::ptr::null();
            *out_len = 0;
            return fail(h, DDS_ERROR_INVALID_ARGUMENT, "no decompressed DDS data");
        }
        *out_data = h.decompressed.as_ptr();
        *out_len = h.decompressed.len();
        succeed(h)
    })
}

/// Borrows the raw 124-byte DDS header: writes the byte pointer to `out_data`
/// and its length to `out_len`. Both must be non-NULL. The pointer stays valid
/// until the next `dds_parse_memory` or `dds_destroy` on this handle.
///
/// # Safety
/// `handle` must be a live handle from [`dds_create`]. `out_data` and `out_len`
/// must be writable.
#[no_mangle]
pub unsafe extern "C" fn dds_raw_header(
    handle: *mut Handle,
    out_data: *mut *const u8,
    out_len: *mut usize,
) -> c_int {
    guard(DDS_ERROR_INVALID_ARGUMENT, handle, |h| {
        if out_data.is_null() || out_len.is_null() {
            return fail(h, DDS_ERROR_INVALID_ARGUMENT, "out pointer is NULL");
        }
        match &h.file {
            Some(file) => {
                *out_data = file.raw_header.as_ptr();
                *out_len = file.raw_header.len();
                succeed(h)
            }
            None => {
                *out_data = core::ptr::null();
                *out_len = 0;
                fail(h, DDS_ERROR_INVALID_ARGUMENT, "no DDS image parsed")
            }
        }
    })
}

/// Borrows the compressed payload that follows the 128-byte prefix: writes the
/// byte pointer to `out_data` and its length to `out_len`. Both must be
/// non-NULL. The pointer stays valid until the next `dds_parse_memory` or
/// `dds_destroy` on this handle.
///
/// # Safety
/// `handle` must be a live handle from [`dds_create`]. `out_data` and `out_len`
/// must be writable.
#[no_mangle]
pub unsafe extern "C" fn dds_compressed_data(
    handle: *mut Handle,
    out_data: *mut *const u8,
    out_len: *mut usize,
) -> c_int {
    guard(DDS_ERROR_INVALID_ARGUMENT, handle, |h| {
        if out_data.is_null() || out_len.is_null() {
            return fail(h, DDS_ERROR_INVALID_ARGUMENT, "out pointer is NULL");
        }
        match &h.file {
            Some(file) => {
                *out_data = file.compressed_data.as_ptr();
                *out_len = file.compressed_data.len();
                succeed(h)
            }
            None => {
                *out_data = core::ptr::null();
                *out_len = 0;
                fail(h, DDS_ERROR_INVALID_ARGUMENT, "no DDS image parsed")
            }
        }
    })
}

/// Copies the last error message into a caller buffer, snprintf style.
///
/// # Safety
/// `handle` must be a live handle from [`dds_create`]. When `capacity` is
/// non-zero, `out` must be writable for `capacity` bytes.
#[no_mangle]
pub unsafe extern "C" fn dds_last_error(
    handle: *mut Handle,
    out: *mut c_char,
    capacity: usize,
) -> c_int {
    guard(0, handle, |h| write_c_string(&h.message, out, capacity))
}

/// Reports the crate version as a static NUL-terminated banner.
#[no_mangle]
pub extern "C" fn dds_version() -> *const c_char {
    concat!("dds-ffi ", env!("CARGO_PKG_VERSION"), "\0").as_ptr().cast::<c_char>()
}
