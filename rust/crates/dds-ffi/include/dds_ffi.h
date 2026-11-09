/* C ABI over the `dds-core` crate.
 *
 * This interface owns state: a handle holds one DDS image — the parsed header,
 * the mapped compression format, the compressed payload and, once
 * dds_decompress() runs, the decoded RGBA8 buffer. The C side works with an
 * opaque handle created by dds_create() and released by dds_destroy(), and
 * every other call takes that handle as its first argument.
 *
 * A handle must be used on one thread at a time.
 *
 * The decoded RGBA8 buffer borrows memory from the handle: the pointer
 * returned by dds_decompressed_data() stays valid until the next
 * dds_parse_memory() or dds_decompress() on the same handle. Copy what you
 * need before reusing the handle. The width/height/format accessors only write
 * the message slot and leave those buffers alone, so a caller may interleave
 * them with its copy.
 *
 * Strings copied into caller buffers follow the snprintf convention: the return
 * value is the length in bytes including the terminating NUL, so a caller
 * detects truncation by comparing it against the buffer capacity. Nothing is
 * written past `capacity` bytes, and a NULL `out` or a zero `capacity` queries
 * the required length without writing anything.
 */

#ifndef DDS_FFI_H
#define DDS_FFI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The call succeeded. */
#define DDS_OK 0
/* A required pointer argument was NULL, or a pointer was misaligned. */
#define DDS_ERROR_INVALID_ARGUMENT (-1)
/* The magic is not "DDS ". */
#define DDS_ERROR_BAD_MAGIC (-2)
/* The 4-byte magic or the 124-byte header runs past the end of the file. */
#define DDS_ERROR_TRUNCATED (-3)
/* The header is missing the width or height flag. */
#define DDS_ERROR_MISSING_DIMENSIONS (-4)
/* A DXT-compressed format had no payload to decode. */
#define DDS_ERROR_EMPTY (-5)
/* The compression format is neither DXT1/3/5/RXGB nor an RGB payload. */
#define DDS_ERROR_UNSUPPORTED (-6)
/* An uncompressed payload declared a bit count other than 24 or 32. */
#define DDS_ERROR_BIT_COUNT (-7)
/* An uncompressed payload holds fewer bytes than width x height x bpp. */
#define DDS_ERROR_TRUNCATED_DATA (-8)
/* A Rust panic was caught at the boundary; the call had no effect. */
#define DDS_ERROR_PANIC (-9)

/* Compression format values, matching the C++ DDSCompressionFormat enum. */
#define DDS_FORMAT_UNCOMPRESSED 0
#define DDS_FORMAT_DXT1 1
#define DDS_FORMAT_DXT3 2
#define DDS_FORMAT_DXT5 3
#define DDS_FORMAT_RXGB 4
#define DDS_FORMAT_UNKNOWN 5

/* Creates a new empty DDS handle. The caller releases it with dds_destroy(). */
void *dds_create(void);

/* Destroys a handle created by dds_create(). NULL is ignored. */
void dds_destroy(void *handle);

/* Parses a whole DDS image (magic + header + payload) from memory into the
 * handle, replacing any previous state. Returns DDS_OK or a negative code; see
 * dds_last_error() for the message. */
int dds_parse_memory(void *handle, const uint8_t *data, size_t length);

/* Decodes the parsed image to RGBA8, storing the pixels in the handle. Returns
 * DDS_OK or a negative code. */
int dds_decompress(void *handle);

/* Reports the parsed image width in pixels, or 0 when nothing is parsed. */
uint32_t dds_width(void *handle);

/* Reports the parsed image height in pixels, or 0 when nothing is parsed. */
uint32_t dds_height(void *handle);

/* Reports the mipmap count, clamped to at least 1 like the original loader. */
uint32_t dds_mipmap_count(void *handle);

/* Reports the compression format as a DDS_FORMAT_* value. */
int dds_compression_format(void *handle);

/* Borrows the decoded RGBA8 buffer: writes the byte pointer to out_data and
 * its length to out_len. Both must be non-NULL. The pointer stays valid until
 * the next dds_parse_memory() or dds_decompress() on this handle. Returns
 * DDS_OK, or a negative code when no decoded data is available. */
int dds_decompressed_data(void *handle, const uint8_t **out_data, size_t *out_len);

/* Borrows the raw 124-byte DDS header: writes the byte pointer to out_data and
 * its length to out_len. Both must be non-NULL. The pointer stays valid until
 * the next dds_parse_memory() or dds_destroy() on this handle. */
int dds_raw_header(void *handle, const uint8_t **out_data, size_t *out_len);

/* Borrows the compressed payload that follows the 128-byte prefix: writes the
 * byte pointer to out_data and its length to out_len. Both must be non-NULL.
 * The pointer stays valid until the next dds_parse_memory() or dds_destroy()
 * on this handle. */
int dds_compressed_data(void *handle, const uint8_t **out_data, size_t *out_len);

/* Copies the last error message into a caller buffer, snprintf style. Returns
 * the length in bytes including the terminating NUL. */
int dds_last_error(void *handle, char *out, size_t capacity);

/* Reports the crate version as a static NUL-terminated banner. */
const char *dds_version(void);

#ifdef __cplusplus
}
#endif

#endif /* DDS_FFI_H */
