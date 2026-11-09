/* c_abi_check.c - compiles dds_ffi.h as C11 and exercises the ABI edges the
 * C++ unit tests in Oblivion_Android cannot reach: NULL handles, the error
 * message slot, short buffers, the borrowed RGBA pointer lifetime, bad magic,
 * missing dimensions, truncated uncompressed payloads and each decode path
 * (DXT1 / DXT3 / DXT5 / RXGB / 32bpp BGRA / 24bpp BGR / masked 32bpp).
 *
 * It builds every fixture in memory so the assertions cover real parses, not
 * stubs.
 *
 * Build and run (mingw-w64, from the repository root, with
 * CARGO_TARGET_DIR=<repo>/target/gnu):
 *   gcc -std=c11 -O1 -Wall -Wextra -Wpedantic -I crates/dds-ffi/include \
 *       crates/dds-ffi/tests/c_abi_check.c \
 *       target/gnu/x86_64-pc-windows-gnu/release/libdds_ffi.a \
 *       -lws2_32 -lbcrypt -luserenv -lntdll -lole32 -loleaut32 -ladvapi32 \
 *       -luser32 -lshell32 -lkernel32 -o target/dds_c_abi_check.exe
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "dds_ffi.h"

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

static unsigned char fixture[4096];
static size_t fixture_pos;

static void put_u8(unsigned value) { fixture[fixture_pos++] = (unsigned char)(value & 0xFFu); }

static void put_u32(uint32_t value)
{
    put_u8(value & 0xFFu);
    put_u8((value >> 8) & 0xFFu);
    put_u8((value >> 16) & 0xFFu);
    put_u8((value >> 24) & 0xFFu);
}

/* Writes the 128-byte DDS prefix: magic, header and pixel-format sub-struct.
 * Everything the loader reads is set explicitly, so the fixture is stable
 * regardless of padding. */
static void put_dds_prefix(uint32_t flags, uint32_t height, uint32_t width,
                           uint32_t mipmap_count, uint32_t pf_flags,
                           uint32_t four_cc, uint32_t bit_count,
                           uint32_t red_mask, uint32_t green_mask,
                           uint32_t blue_mask, uint32_t alpha_mask)
{
    put_u32(0x20534444u); /* DDS_MAGIC */
    put_u32(124u);        /* header size */
    put_u32(flags);
    put_u32(height);
    put_u32(width);
    put_u32(0u); /* pitch or linear size */
    put_u32(0u); /* depth */
    put_u32(mipmap_count);
    for (int i = 0; i < 11; ++i) {
        put_u32(0u); /* reserved1 */
    }
    /* pixel format at offset 72 inside the header */
    put_u32(32u);       /* pixel format size */
    put_u32(pf_flags);
    put_u32(four_cc);
    put_u32(bit_count);
    put_u32(red_mask);
    put_u32(green_mask);
    put_u32(blue_mask);
    put_u32(alpha_mask);
    put_u32(0u); /* caps */
    put_u32(0u);
    put_u32(0u);
    put_u32(0u);
    put_u32(0u); /* reserved2 */
}

static void reset_fixture(void) { fixture_pos = 0; }

/* ------------------------------------------------------------------ *
 * Shared helpers.
 * ------------------------------------------------------------------ */

static void check_error_message_has_text(void *handle)
{
    char buffer[128];
    int needed = dds_last_error(handle, buffer, sizeof(buffer));
    CHECK(needed > 1);
    CHECK(buffer[0] != '\0');
}

static void check_version_banner(void)
{
    const char *banner = dds_version();
    CHECK(banner != NULL);
    CHECK(strncmp(banner, "dds-ffi ", 7) == 0);
}

/* A NULL handle must never crash; accessors report the empty state, calls
 * report the invalid-argument code, and dds_last_error reports length zero. */
static void check_null_handle(void)
{
    uint8_t pixel[4] = { 0, 0, 0, 0 };
    const uint8_t *out_data = NULL;
    size_t out_len = 0;

    CHECK(dds_width(NULL) == 0);
    CHECK(dds_height(NULL) == 0);
    CHECK(dds_mipmap_count(NULL) == 0);
    CHECK(dds_compression_format(NULL) == DDS_FORMAT_UNKNOWN);
    CHECK(dds_parse_memory(NULL, pixel, sizeof(pixel)) == DDS_ERROR_INVALID_ARGUMENT);
    CHECK(dds_decompress(NULL) == DDS_ERROR_INVALID_ARGUMENT);
    CHECK(dds_decompressed_data(NULL, &out_data, &out_len) == DDS_ERROR_INVALID_ARGUMENT);
    CHECK(dds_last_error(NULL, NULL, 0) == 0);
    dds_destroy(NULL);
}

/* A NULL payload pointer with a non-zero length is rejected. */
static void check_null_payload(void)
{
    void *handle = dds_create();
    CHECK(handle != NULL);
    CHECK(dds_parse_memory(handle, NULL, 8) == DDS_ERROR_INVALID_ARGUMENT);
    check_error_message_has_text(handle);
    dds_destroy(handle);
}

static void check_bad_magic(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_u32(0u); /* not "DDS " */
    for (int i = 0; i < 31; ++i) {
        put_u32(0u);
    }
    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_ERROR_BAD_MAGIC);
    CHECK(dds_width(handle) == 0);
    CHECK(dds_height(handle) == 0);
    CHECK(dds_compression_format(handle) == DDS_FORMAT_UNKNOWN);
    check_error_message_has_text(handle);
    dds_destroy(handle);
}

static void check_missing_dimensions(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_dds_prefix(0u /* no DDSD_WIDTH/HEIGHT */, 4u, 4u, 1u, 0x4u, 0u, 0u, 0u, 0u, 0u, 0u);
    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_ERROR_MISSING_DIMENSIONS);
    check_error_message_has_text(handle);
    dds_destroy(handle);
}

/* DXT1 with a single block: c0 = white, c1 = black, every index 0, so the
 * decoded image is sixteen white texels. */
static void check_dxt1(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_dds_prefix(0x1007u, 4u, 4u, 1u, 0x4u, 0x31545844u /* "DXT1" */, 0u, 0u, 0u, 0u, 0u);
    put_u32(0x0000FFFFu); /* c0 = 0xFFFF (white), c1 = 0x0000 (black) */
    put_u32(0u);          /* sixteen indices of 0 */

    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_OK);
    CHECK(dds_width(handle) == 4);
    CHECK(dds_height(handle) == 4);
    CHECK(dds_mipmap_count(handle) == 1);
    CHECK(dds_compression_format(handle) == DDS_FORMAT_DXT1);

    CHECK(dds_decompress(handle) == DDS_OK);

    const uint8_t *rgba = NULL;
    size_t rgba_len = 0;
    CHECK(dds_decompressed_data(handle, &rgba, &rgba_len) == DDS_OK);
    CHECK(rgba != NULL);
    CHECK(rgba_len == 4u * 4u * 4u);
    for (size_t i = 0; i < rgba_len; i += 4) {
        CHECK(rgba[i + 0] == 255);
        CHECK(rgba[i + 1] == 255);
        CHECK(rgba[i + 2] == 255);
        CHECK(rgba[i + 3] == 255);
    }

    /* The out pointers are mandatory. */
    CHECK(dds_decompressed_data(handle, NULL, &rgba_len) == DDS_ERROR_INVALID_ARGUMENT);
    CHECK(dds_decompressed_data(handle, &rgba, NULL) == DDS_ERROR_INVALID_ARGUMENT);

    dds_destroy(handle);
}

/* DXT3 (BC2): explicit 4-bit alpha then a BC1 colour block, all indices 0, so
 * the decoded image is sixteen white texels with the block's first alpha. */
static void check_dxt3(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_dds_prefix(0x1007u, 4u, 4u, 1u, 0x4u, 0x33545844u /* "DXT3" */, 0u, 0u, 0u, 0u, 0u);
    /* alpha block: every 4-bit alpha is 0xF */
    put_u32(0xFFFFFFFFu);
    put_u32(0xFFFFFFFFu);
    /* colour block: c0 = white, c1 = black, indices 0 */
    put_u32(0x0000FFFFu);
    put_u32(0u);

    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_OK);
    CHECK(dds_compression_format(handle) == DDS_FORMAT_DXT3);
    CHECK(dds_decompress(handle) == DDS_OK);

    const uint8_t *rgba = NULL;
    size_t rgba_len = 0;
    CHECK(dds_decompressed_data(handle, &rgba, &rgba_len) == DDS_OK);
    CHECK(rgba_len == 4u * 4u * 4u);
    for (size_t i = 0; i < rgba_len; i += 4) {
        CHECK(rgba[i + 0] == 255);
        CHECK(rgba[i + 3] == 255); /* 0xF * 17 = 255 */
    }
    dds_destroy(handle);
}

/* DXT5 (BC3): 8 bytes of interpolated alpha then a BC1 colour block. Every
 * index is 0, so alpha = a0 and colour = palette[0]. */
static void check_dxt5(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_dds_prefix(0x1007u, 4u, 4u, 1u, 0x4u, 0x35545844u /* "DXT5" */, 0u, 0u, 0u, 0u, 0u);
    put_u8(0x80); /* a0 */
    put_u8(0x00); /* a1 */
    for (int i = 0; i < 6; ++i) {
        put_u8(0u); /* eight 3-bit alpha indices of 0 */
    }
    /* colour block: c0 = white, c1 = black, indices 0 */
    put_u32(0x0000FFFFu);
    put_u32(0u);

    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_OK);
    CHECK(dds_compression_format(handle) == DDS_FORMAT_DXT5);
    CHECK(dds_decompress(handle) == DDS_OK);

    const uint8_t *rgba = NULL;
    size_t rgba_len = 0;
    CHECK(dds_decompressed_data(handle, &rgba, &rgba_len) == DDS_OK);
    CHECK(rgba_len == 4u * 4u * 4u);
    for (size_t i = 0; i < rgba_len; i += 4) {
        CHECK(rgba[i + 3] == 0x80);
    }
    dds_destroy(handle);
}

/* RXGB: the BC3 variant that swaps red and alpha, so the decoded red channel
 * reads the block's alpha and the decoded alpha reads the block's red. */
static void check_rxgb(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_dds_prefix(0x1007u, 4u, 4u, 1u, 0x4u, 0x42475852u /* "RXGB" */, 0u, 0u, 0u, 0u, 0u);
    put_u8(0x80); /* a0 */
    put_u8(0x00); /* a1 */
    for (int i = 0; i < 6; ++i) {
        put_u8(0u); /* six alpha-index bytes */
    }
    /* colour block: c0 = white, c1 = black, indices 0 */
    put_u32(0x0000FFFFu);
    put_u32(0u);

    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_OK);
    CHECK(dds_compression_format(handle) == DDS_FORMAT_RXGB);
    CHECK(dds_decompress(handle) == DDS_OK);

    const uint8_t *rgba = NULL;
    size_t rgba_len = 0;
    CHECK(dds_decompressed_data(handle, &rgba, &rgba_len) == DDS_OK);
    for (size_t i = 0; i < rgba_len; i += 4) {
        CHECK(rgba[i + 0] == 0x80); /* red carries the BC3 alpha */
        CHECK(rgba[i + 3] == 255);  /* alpha carries the palette red (white) */
    }
    dds_destroy(handle);
}

/* A DXT format with no payload must fail the decode without disturbing the
 * parsed header. */
static void check_dxt_empty(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_dds_prefix(0x1007u, 4u, 4u, 1u, 0x4u, 0x31545844u, 0u, 0u, 0u, 0u, 0u);
    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_OK);
    CHECK(dds_decompress(handle) == DDS_ERROR_EMPTY);
    check_error_message_has_text(handle);
    CHECK(dds_width(handle) == 4);
    dds_destroy(handle);
}

/* 32bpp BGRA without masks: the bytes pass through, red and blue swapped. */
static void check_uncompressed_bgra(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_dds_prefix(0x1007u, 1u, 1u, 1u, 0x41u /* DDPF_RGB | DDPF_ALPHAPIXELS */, 0u, 32u,
                   0u, 0u, 0u, 0u);
    put_u8(0x11); /* B */
    put_u8(0x22); /* G */
    put_u8(0x33); /* R */
    put_u8(0x44); /* A */

    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_OK);
    CHECK(dds_compression_format(handle) == DDS_FORMAT_UNKNOWN);
    CHECK(dds_decompress(handle) == DDS_OK);

    const uint8_t *rgba = NULL;
    size_t rgba_len = 0;
    CHECK(dds_decompressed_data(handle, &rgba, &rgba_len) == DDS_OK);
    CHECK(rgba_len == 4u);
    CHECK(rgba[0] == 0x33);
    CHECK(rgba[1] == 0x22);
    CHECK(rgba[2] == 0x11);
    CHECK(rgba[3] == 0x44);
    dds_destroy(handle);
}

/* 24bpp BGR without masks: the bytes pass through with an opaque alpha. */
static void check_uncompressed_bgr(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_dds_prefix(0x1007u, 1u, 1u, 1u, 0x40u /* DDPF_RGB */, 0u, 24u, 0u, 0u, 0u, 0u);
    put_u8(0x11); /* B */
    put_u8(0x22); /* G */
    put_u8(0x33); /* R */

    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_OK);
    CHECK(dds_decompress(handle) == DDS_OK);

    const uint8_t *rgba = NULL;
    size_t rgba_len = 0;
    CHECK(dds_decompressed_data(handle, &rgba, &rgba_len) == DDS_OK);
    CHECK(rgba_len == 4u);
    CHECK(rgba[0] == 0x33);
    CHECK(rgba[1] == 0x22);
    CHECK(rgba[2] == 0x11);
    CHECK(rgba[3] == 255);
    dds_destroy(handle);
}

/* 32bpp with masks: a packed little-endian pixel is spread across the full
 * 8-bit range per channel, with alpha only when the flag and mask are set. */
static void check_uncompressed_masks(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_dds_prefix(0x1007u, 1u, 1u, 1u, 0x41u, 0u, 32u,
                   0x00FF0000u /* R */, 0x0000FF00u /* G */, 0x000000FFu /* B */,
                   0xFF000000u /* A */);
    put_u8(0x44); /* B */
    put_u8(0x33); /* G */
    put_u8(0x22); /* R */
    put_u8(0x11); /* A */

    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_OK);
    CHECK(dds_decompress(handle) == DDS_OK);

    const uint8_t *rgba = NULL;
    size_t rgba_len = 0;
    CHECK(dds_decompressed_data(handle, &rgba, &rgba_len) == DDS_OK);
    CHECK(rgba_len == 4u);
    CHECK(rgba[0] == 0x22);
    CHECK(rgba[1] == 0x33);
    CHECK(rgba[2] == 0x44);
    CHECK(rgba[3] == 0x11);
    dds_destroy(handle);
}

/* An unsupported bit count must fail the decode without disturbing the parsed
 * header. */
static void check_bad_bit_count(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_dds_prefix(0x1007u, 1u, 1u, 1u, 0x40u, 0u, 16u, 0u, 0u, 0u, 0u);
    put_u32(0u);
    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_OK);
    CHECK(dds_decompress(handle) == DDS_ERROR_BIT_COUNT);
    check_error_message_has_text(handle);
    dds_destroy(handle);
}

/* An uncompressed payload shorter than width x height x bpp must fail. */
static void check_truncated_payload(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_dds_prefix(0x1007u, 2u, 2u, 1u, 0x40u, 0u, 32u, 0u, 0u, 0u, 0u);
    put_u32(0u); /* only one texel of the four */
    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_OK);
    CHECK(dds_decompress(handle) == DDS_ERROR_TRUNCATED_DATA);
    check_error_message_has_text(handle);
    dds_destroy(handle);
}

/* Re-parsing replaces the previous image: the width, height, format and
 * decoded buffer all track the newest parse. */
static void check_reparse(void)
{
    void *handle = dds_create();

    reset_fixture();
    put_dds_prefix(0x1007u, 4u, 4u, 1u, 0x4u, 0x31545844u, 0u, 0u, 0u, 0u, 0u);
    put_u32(0x0000FFFFu);
    put_u32(0u);
    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_OK);
    CHECK(dds_decompress(handle) == DDS_OK);
    CHECK(dds_width(handle) == 4);

    reset_fixture();
    put_dds_prefix(0x1007u, 1u, 1u, 1u, 0x40u, 0u, 24u, 0u, 0u, 0u, 0u);
    put_u8(0x11);
    put_u8(0x22);
    put_u8(0x33);
    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_OK);
    CHECK(dds_width(handle) == 1);
    CHECK(dds_height(handle) == 1);
    CHECK(dds_decompress(handle) == DDS_OK);

    const uint8_t *rgba = NULL;
    size_t rgba_len = 0;
    CHECK(dds_decompressed_data(handle, &rgba, &rgba_len) == DDS_OK);
    CHECK(rgba_len == 4u);
    dds_destroy(handle);
}

/* Before any decode, the borrowed buffer reports NULL and length zero. */
static void check_no_decode_yet(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_dds_prefix(0x1007u, 4u, 4u, 1u, 0x4u, 0x31545844u, 0u, 0u, 0u, 0u, 0u);
    put_u32(0x0000FFFFu);
    put_u32(0u);
    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_OK);

    const uint8_t *rgba = (const uint8_t *)0x1;
    size_t rgba_len = 42;
    CHECK(dds_decompressed_data(handle, &rgba, &rgba_len) == DDS_ERROR_INVALID_ARGUMENT);
    CHECK(rgba == NULL);
    CHECK(rgba_len == 0);
    dds_destroy(handle);
}

/* A short buffer is NUL-terminated and truncated, and the return value still
 * reports the full needed length. A NULL buffer only queries the length. */
static void check_error_message_truncation(void)
{
    void *handle = dds_create();
    reset_fixture();
    put_u32(0u);
    for (int i = 0; i < 31; ++i) {
        put_u32(0u);
    }
    CHECK(dds_parse_memory(handle, fixture, fixture_pos) == DDS_ERROR_BAD_MAGIC);

    int needed = dds_last_error(handle, NULL, 0);
    CHECK(needed > 1);

    char short_buffer[8];
    memset(short_buffer, 'x', sizeof(short_buffer));
    int copied = dds_last_error(handle, short_buffer, sizeof(short_buffer));
    CHECK(copied == needed);
    CHECK(short_buffer[sizeof(short_buffer) - 1] == '\0');
    dds_destroy(handle);
}

int main(void)
{
    check_version_banner();
    check_null_handle();
    check_null_payload();
    check_bad_magic();
    check_missing_dimensions();
    check_dxt1();
    check_dxt3();
    check_dxt5();
    check_rxgb();
    check_dxt_empty();
    check_uncompressed_bgra();
    check_uncompressed_bgr();
    check_uncompressed_masks();
    check_bad_bit_count();
    check_truncated_payload();
    check_reparse();
    check_no_decode_yet();
    check_error_message_truncation();

    if (failures == 0) {
        printf("dds-ffi C ABI check: all assertions passed\n");
        return 0;
    }
    fprintf(stderr, "dds-ffi C ABI check: %d assertion(s) failed\n", failures);
    return 1;
}
