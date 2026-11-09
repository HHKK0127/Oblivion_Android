//! DDS texture container parser and decoder.
//!
//! This crate ports the pure-logic portion of the Oblivion Android C++
//! `DDSLoader`: the header parse, the FourCC to compression-format mapping and
//! the block decoders (BC1/DXT1, BC2/DXT3, BC3/DXT5 plus the RXGB normal-map
//! variant) and the masked-uncompressed unpack. Every decode writes RGBA8.
//!
//! The file image handed to [`parse`] is the whole DDS file — the 4-byte magic,
//! the 124-byte header and the compressed payload that follows. Parsing mirrors
//! the original `loadFile`/`readHeader` behaviour: the header size field is
//! validated leniently (a mismatch is not fatal) but the width/height flags are
//! required, and the payload is every byte after the 128-byte prefix.

use thiserror::Error;

/// `"DDS "` in little-endian.
pub const DDS_MAGIC: u32 = 0x2053_4444;

pub const DDPF_ALPHAPIXELS: u32 = 0x0000_0001;
pub const DDPF_FOURCC: u32 = 0x0000_0004;
pub const DDPF_RGB: u32 = 0x0000_0040;

pub const DDSD_HEIGHT: u32 = 0x0000_0002;
pub const DDSD_WIDTH: u32 = 0x0000_0004;

pub const FOURCC_DXT1: u32 = 0x3154_5844;
pub const FOURCC_DXT3: u32 = 0x3354_5844;
pub const FOURCC_DXT5: u32 = 0x3554_5844;
pub const FOURCC_RXGB: u32 = 0x4247_5852;

/// Offset of the pixel-format sub-structure inside the 124-byte header.
const PIXEL_FORMAT_OFFSET: usize = 72;
/// Size of the fixed DDS prefix: 4-byte magic + 124-byte header.
const DATA_OFFSET: usize = 4 + 124;

/// The compression scheme a DDS payload uses, mirroring the C++ enum values.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(i32)]
pub enum CompressionFormat {
    Uncompressed = 0,
    Dxt1 = 1,
    Dxt3 = 2,
    Dxt5 = 3,
    Rxgb = 4,
    Unknown = 5,
}

/// Why a DDS image could not be parsed or decoded.
#[derive(Debug, Error, Clone, Copy, PartialEq, Eq)]
pub enum DDSError {
    #[error("DDS file shorter than the 4-byte magic")]
    TruncatedMagic,
    #[error("invalid DDS magic (expected 0x20534444)")]
    BadMagic,
    #[error("DDS header runs past the end of the file")]
    TruncatedHeader,
    #[error("DDS header missing the width or height flag")]
    MissingDimensions,
    #[error("no compressed DDS data to decompress")]
    EmptyData,
    #[error("unsupported DDS compression format")]
    UnsupportedFormat,
    #[error("unsupported uncompressed DDS bit count (expected 24 or 32)")]
    BadBitCount,
    #[error("uncompressed DDS data truncated")]
    TruncatedData,
}

/// The pixel-format sub-structure of the DDS header.
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct PixelFormat {
    pub size: u32,
    pub flags: u32,
    pub four_cc: u32,
    pub bit_count: u32,
    pub red_mask: u32,
    pub green_mask: u32,
    pub blue_mask: u32,
    pub alpha_mask: u32,
}

/// The fields of the DDS header the loader reads.
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct Header {
    pub size: u32,
    pub flags: u32,
    pub height: u32,
    pub width: u32,
    pub mipmap_count: u32,
    pub pixel_format: PixelFormat,
}

/// A parsed DDS file: the header, the mapped compression format and the raw
/// compressed payload that follows the 128-byte prefix. The raw 124-byte header
/// bytes are kept so the C++ shim can rebuild its `DDSHeader` bit-for-bit.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct DdsFile {
    pub header: Header,
    pub compression_format: CompressionFormat,
    pub compressed_data: Vec<u8>,
    pub raw_header: Vec<u8>,
}

impl Default for CompressionFormat {
    fn default() -> Self {
        CompressionFormat::Unknown
    }
}

fn read_u32_le(data: &[u8], offset: usize) -> u32 {
    let bytes = [data[offset], data[offset + 1], data[offset + 2], data[offset + 3]];
    u32::from_le_bytes(bytes)
}

/// Maps a FourCC value to its compression format, matching the original
/// `getCompressionFormat`. Anything that is not DXT1/DXT3/DXT5/RXGB is Unknown.
pub fn compression_format_from_four_cc(four_cc: u32) -> CompressionFormat {
    match four_cc {
        FOURCC_DXT1 => CompressionFormat::Dxt1,
        FOURCC_DXT3 => CompressionFormat::Dxt3,
        FOURCC_DXT5 => CompressionFormat::Dxt5,
        FOURCC_RXGB => CompressionFormat::Rxgb,
        _ => CompressionFormat::Unknown,
    }
}

/// Parses a whole DDS image (magic + header + payload) from memory.
///
/// Mirrors the original `loadFile`: the magic and the width/height flags are
/// required, a header-size mismatch is tolerated, and the payload is every byte
/// after the 128-byte prefix.
pub fn parse(data: &[u8]) -> Result<DdsFile, DDSError> {
    if data.len() < 4 {
        return Err(DDSError::TruncatedMagic);
    }
    if read_u32_le(data, 0) != DDS_MAGIC {
        return Err(DDSError::BadMagic);
    }
    if data.len() < DATA_OFFSET {
        return Err(DDSError::TruncatedHeader);
    }

    let base = 4;
    let header = Header {
        size: read_u32_le(data, base + 0),
        flags: read_u32_le(data, base + 4),
        height: read_u32_le(data, base + 8),
        width: read_u32_le(data, base + 12),
        mipmap_count: read_u32_le(data, base + 24),
        pixel_format: PixelFormat {
            size: read_u32_le(data, base + PIXEL_FORMAT_OFFSET + 0),
            flags: read_u32_le(data, base + PIXEL_FORMAT_OFFSET + 4),
            four_cc: read_u32_le(data, base + PIXEL_FORMAT_OFFSET + 8),
            bit_count: read_u32_le(data, base + PIXEL_FORMAT_OFFSET + 12),
            red_mask: read_u32_le(data, base + PIXEL_FORMAT_OFFSET + 16),
            green_mask: read_u32_le(data, base + PIXEL_FORMAT_OFFSET + 20),
            blue_mask: read_u32_le(data, base + PIXEL_FORMAT_OFFSET + 24),
            alpha_mask: read_u32_le(data, base + PIXEL_FORMAT_OFFSET + 28),
        },
    };

    if header.flags & DDSD_WIDTH == 0 || header.flags & DDSD_HEIGHT == 0 {
        return Err(DDSError::MissingDimensions);
    }

    Ok(DdsFile {
        header,
        compression_format: compression_format_from_four_cc(header.pixel_format.four_cc),
        compressed_data: data[DATA_OFFSET..].to_vec(),
        raw_header: data[4..DATA_OFFSET].to_vec(),
    })
}

/// Expands a packed RGB565 colour to 8-bit RGB, replicating the high bits into
/// the low ones so that 0xFFFF maps to pure white.
fn decode_rgb565(packed: u16) -> [u8; 3] {
    let r5 = ((packed >> 11) & 0x1F) as u8;
    let g6 = ((packed >> 5) & 0x3F) as u8;
    let b5 = (packed & 0x1F) as u8;
    [(r5 << 3) | (r5 >> 2), (g6 << 2) | (g6 >> 4), (b5 << 3) | (b5 >> 2)]
}

/// Builds the four-entry colour palette of a BC1 colour block. The two-colour
/// interpolated mode (with a transparent index) is only reachable for DXT1;
/// DXT3/DXT5 always use the three-colour interpolation. Each entry is RGBA.
fn build_colour_palette(block: &[u8], allow_two_colour_mode: bool) -> [[u8; 4]; 4] {
    let c0 = u16::from_le_bytes([block[0], block[1]]);
    let c1 = u16::from_le_bytes([block[2], block[3]]);

    let mut palette = [[0u8; 4]; 4];
    let rgb0 = decode_rgb565(c0);
    let rgb1 = decode_rgb565(c1);
    palette[0][..3].copy_from_slice(&rgb0);
    palette[0][3] = 255;
    palette[1][..3].copy_from_slice(&rgb1);
    palette[1][3] = 255;

    if c0 > c1 || !allow_two_colour_mode {
        for channel in 0..3 {
            palette[2][channel] = ((2 * u16::from(rgb0[channel]) + u16::from(rgb1[channel])) / 3) as u8;
            palette[3][channel] = ((u16::from(rgb0[channel]) + 2 * u16::from(rgb1[channel])) / 3) as u8;
        }
        palette[2][3] = 255;
        palette[3][3] = 255;
    } else {
        for channel in 0..3 {
            palette[2][channel] = ((u16::from(rgb0[channel]) + u16::from(rgb1[channel])) / 2) as u8;
            palette[3][channel] = 0;
        }
        palette[2][3] = 255;
        palette[3][3] = 0;
    }
    palette
}

/// Builds the eight-entry interpolated alpha palette of a BC3 alpha block.
fn build_alpha_palette(block: &[u8]) -> [u8; 8] {
    let a0 = block[0];
    let a1 = block[1];
    let mut palette = [0u8; 8];
    palette[0] = a0;
    palette[1] = a1;

    if a0 > a1 {
        for step in 1u16..=6 {
            palette[1 + step as usize] = (((7 - step) * u16::from(a0) + step * u16::from(a1)) / 7) as u8;
        }
    } else {
        for step in 1u16..=4 {
            palette[1 + step as usize] = (((5 - step) * u16::from(a0) + step * u16::from(a1)) / 5) as u8;
        }
        palette[6] = 0;
        palette[7] = 255;
    }
    palette
}

/// Writes one texel, discarding writes that fall outside the texture so that
/// dimensions which are not a multiple of four stay in bounds.
fn write_texel(destination: &mut [u8], width: u32, height: u32, x: u32, y: u32, rgba: [u8; 4]) {
    if x >= width || y >= height {
        return;
    }
    let out_index = ((y as usize) * (width as usize) + (x as usize)) * 4;
    destination[out_index..out_index + 4].copy_from_slice(&rgba);
}

/// DXT1 (BC1): 8-byte blocks holding two RGB565 endpoints and sixteen 2-bit
/// indices. An endpoint pair with c0 <= c1 enables the 1-bit alpha mode.
fn decode_bc1(source: &[u8], destination: &mut [u8], width: u32, height: u32) {
    let blocks_x = (width + 3) / 4;
    let blocks_y = (height + 3) / 4;

    for block_y in 0..blocks_y {
        for block_x in 0..blocks_x {
            let offset = ((block_y as usize) * (blocks_x as usize) + (block_x as usize)) * 8;
            if offset + 8 > source.len() {
                return;
            }
            let block = &source[offset..];

            let palette = build_colour_palette(block, true);

            let mut indices: u32 = 0;
            for byte in 0..4 {
                indices |= (block[4 + byte] as u32) << (8 * byte);
            }

            for texel in 0..16 {
                let colour = palette[((indices >> (2 * texel)) & 0x03) as usize];
                write_texel(destination, width, height,
                            block_x * 4 + (texel as u32 & 3),
                            block_y * 4 + (texel as u32 >> 2), colour);
            }
        }
    }
}

/// DXT3 (BC2): 8 bytes of explicit 4-bit alpha followed by a BC1 colour block
/// that always uses the three-colour interpolation.
fn decode_bc2(source: &[u8], destination: &mut [u8], width: u32, height: u32) {
    let blocks_x = (width + 3) / 4;
    let blocks_y = (height + 3) / 4;

    for block_y in 0..blocks_y {
        for block_x in 0..blocks_x {
            let offset = ((block_y as usize) * (blocks_x as usize) + (block_x as usize)) * 16;
            if offset + 16 > source.len() {
                return;
            }
            let block = &source[offset..];

            let palette = build_colour_palette(&block[8..], false);

            let mut indices: u32 = 0;
            for byte in 0..4 {
                indices |= (block[12 + byte] as u32) << (8 * byte);
            }

            for texel in 0..16 {
                let colour = palette[((indices >> (2 * texel)) & 0x03) as usize];
                let mut rgba = [0u8; 4];
                rgba[0] = colour[0];
                rgba[1] = colour[1];
                rgba[2] = colour[2];
                rgba[3] = (((block[texel >> 1] >> (4 * (texel & 1))) & 0x0F) as u8).wrapping_mul(17);
                write_texel(destination, width, height,
                            block_x * 4 + (texel as u32 & 3),
                            block_y * 4 + (texel as u32 >> 2), rgba);
            }
        }
    }
}

/// DXT5 (BC3) and RXGB: 8 bytes of interpolated alpha followed by a BC1 colour
/// block. RXGB is a BC3 variant used for normal maps where the X component is
/// stored in the alpha channel, so the red and alpha channels are swapped.
fn decode_bc3(source: &[u8], destination: &mut [u8], width: u32, height: u32, swap_red_alpha: bool) {
    let blocks_x = (width + 3) / 4;
    let blocks_y = (height + 3) / 4;

    for block_y in 0..blocks_y {
        for block_x in 0..blocks_x {
            let offset = ((block_y as usize) * (blocks_x as usize) + (block_x as usize)) * 16;
            if offset + 16 > source.len() {
                return;
            }
            let block = &source[offset..];

            let alpha_palette = build_alpha_palette(block);

            let mut alpha_indices: u64 = 0;
            for byte in 0..6 {
                alpha_indices |= (block[2 + byte] as u64) << (8 * byte);
            }

            let palette = build_colour_palette(&block[8..], false);

            let mut colour_indices: u32 = 0;
            for byte in 0..4 {
                colour_indices |= (block[12 + byte] as u32) << (8 * byte);
            }

            for texel in 0..16 {
                let colour = palette[((colour_indices >> (2 * texel)) & 0x03) as usize];
                let mut rgba = [0u8; 4];
                rgba[0] = colour[0];
                rgba[1] = colour[1];
                rgba[2] = colour[2];
                rgba[3] = alpha_palette[((alpha_indices >> (3 * texel)) & 0x07) as usize];
                if swap_red_alpha {
                    let red = rgba[0];
                    rgba[0] = rgba[3];
                    rgba[3] = red;
                }
                write_texel(destination, width, height,
                            block_x * 4 + (texel as u32 & 3),
                            block_y * 4 + (texel as u32 >> 2), rgba);
            }
        }
    }
}

/// Spreads a masked channel across the full 8-bit range.
fn extract(value: u32, mask: u32) -> u8 {
    if mask == 0 {
        return 255;
    }
    let mut shift: u32 = 0;
    while shift < 32 && ((mask >> shift) & 1) == 0 {
        shift += 1;
    }
    let field = (value & mask) >> shift;
    let max_value = mask >> shift;
    if max_value == 0 {
        return 255;
    }
    ((field * 255 + max_value / 2) / max_value) as u8
}

/// Unpacks an uncompressed, mask-described pixel format to RGBA.
fn decompress_uncompressed(file: &DdsFile, destination: &mut [u8]) -> Result<(), DDSError> {
    let format = file.header.pixel_format;
    let bytes_per_pixel = (format.bit_count / 8) as usize;
    if bytes_per_pixel != 3 && bytes_per_pixel != 4 {
        return Err(DDSError::BadBitCount);
    }

    let width = file.header.width as usize;
    let height = file.header.height as usize;
    let required = width * height * bytes_per_pixel;
    if file.compressed_data.len() < required {
        return Err(DDSError::TruncatedData);
    }

    let has_alpha = (format.flags & DDPF_ALPHAPIXELS) != 0 && format.alpha_mask != 0;
    let has_masks = (format.red_mask | format.green_mask | format.blue_mask) != 0;

    let source = &file.compressed_data;
    let texel_count = width * height;

    for texel in 0..texel_count {
        let input = &source[texel * bytes_per_pixel..texel * bytes_per_pixel + bytes_per_pixel];
        let out_index = texel * 4;

        if has_masks {
            let mut packed: u32 = 0;
            for byte in 0..bytes_per_pixel {
                packed |= (input[byte] as u32) << (8 * byte);
            }
            destination[out_index] = extract(packed, format.red_mask);
            destination[out_index + 1] = extract(packed, format.green_mask);
            destination[out_index + 2] = extract(packed, format.blue_mask);
            destination[out_index + 3] = if has_alpha { extract(packed, format.alpha_mask) } else { 255 };
        } else if bytes_per_pixel == 4 {
            // Undocumented masks: assume the common little-endian BGRA layout.
            destination[out_index] = input[2];
            destination[out_index + 1] = input[1];
            destination[out_index + 2] = input[0];
            destination[out_index + 3] = input[3];
        } else {
            destination[out_index] = input[2];
            destination[out_index + 1] = input[1];
            destination[out_index + 2] = input[0];
            destination[out_index + 3] = 255;
        }
    }

    Ok(())
}

/// Decodes a parsed DDS file to RGBA8, mirroring the original
/// `decompressTexture` plus its per-format helpers.
pub fn decompress(file: &DdsFile) -> Result<Vec<u8>, DDSError> {
    let width = file.header.width as usize;
    let height = file.header.height as usize;
    let mut destination = vec![0u8; width * height * 4];

    match file.compression_format {
        CompressionFormat::Dxt1 => {
            if file.compressed_data.is_empty() {
                return Err(DDSError::EmptyData);
            }
            decode_bc1(&file.compressed_data, &mut destination, file.header.width, file.header.height);
        }
        CompressionFormat::Dxt3 => {
            if file.compressed_data.is_empty() {
                return Err(DDSError::EmptyData);
            }
            decode_bc2(&file.compressed_data, &mut destination, file.header.width, file.header.height);
        }
        CompressionFormat::Dxt5 => {
            if file.compressed_data.is_empty() {
                return Err(DDSError::EmptyData);
            }
            decode_bc3(&file.compressed_data, &mut destination, file.header.width, file.header.height, false);
        }
        CompressionFormat::Rxgb => {
            if file.compressed_data.is_empty() {
                return Err(DDSError::EmptyData);
            }
            decode_bc3(&file.compressed_data, &mut destination, file.header.width, file.header.height, true);
        }
        CompressionFormat::Uncompressed | CompressionFormat::Unknown => {
            // A missing FourCC with the RGB flag set means an uncompressed
            // pixel format described by the channel masks.
            if file.header.pixel_format.flags & DDPF_FOURCC == 0
                && file.header.pixel_format.flags & DDPF_RGB != 0
            {
                decompress_uncompressed(file, &mut destination)?;
            } else {
                return Err(DDSError::UnsupportedFormat);
            }
        }
    }

    Ok(destination)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn build_header_bytes(width: u32, height: u32, flags: u32, pf_flags: u32, four_cc: u32,
                          bit_count: u32, red: u32, green: u32, blue: u32, alpha: u32)
                          -> Vec<u8> {
        let mut data = Vec::new();
        data.extend_from_slice(&DDS_MAGIC.to_le_bytes());
        let mut header = Vec::new();
        header.extend_from_slice(&124u32.to_le_bytes()); // size
        header.extend_from_slice(&flags.to_le_bytes());
        header.extend_from_slice(&height.to_le_bytes());
        header.extend_from_slice(&width.to_le_bytes());
        header.extend_from_slice(&0u32.to_le_bytes()); // pitchOrLinearSize
        header.extend_from_slice(&0u32.to_le_bytes()); // depth
        header.extend_from_slice(&1u32.to_le_bytes()); // mipmapCount
        for _ in 0..11 {
            header.extend_from_slice(&0u32.to_le_bytes()); // reserved1
        }
        header.extend_from_slice(&32u32.to_le_bytes()); // pixelFormat.size
        header.extend_from_slice(&pf_flags.to_le_bytes());
        header.extend_from_slice(&four_cc.to_le_bytes());
        header.extend_from_slice(&bit_count.to_le_bytes());
        header.extend_from_slice(&red.to_le_bytes());
        header.extend_from_slice(&green.to_le_bytes());
        header.extend_from_slice(&blue.to_le_bytes());
        header.extend_from_slice(&alpha.to_le_bytes());
        header.extend_from_slice(&0u32.to_le_bytes()); // caps
        header.extend_from_slice(&0u32.to_le_bytes()); // caps2
        header.extend_from_slice(&0u32.to_le_bytes()); // caps3
        header.extend_from_slice(&0u32.to_le_bytes()); // caps4
        header.extend_from_slice(&0u32.to_le_bytes()); // reserved2
        assert_eq!(header.len(), 124);
        data.extend_from_slice(&header);
        data
    }

    #[test]
    fn rejects_bad_magic() {
        let data = vec![0u8; 128];
        assert_eq!(parse(&data), Err(DDSError::BadMagic));
    }

    #[test]
    fn rejects_missing_dimensions() {
        let data = build_header_bytes(4, 4, 0, DDPF_FOURCC, FOURCC_DXT1, 0, 0, 0, 0, 0);
        assert_eq!(parse(&data), Err(DDSError::MissingDimensions));
    }

    #[test]
    fn maps_four_cc() {
        assert_eq!(compression_format_from_four_cc(FOURCC_DXT1), CompressionFormat::Dxt1);
        assert_eq!(compression_format_from_four_cc(FOURCC_RXGB), CompressionFormat::Rxgb);
        assert_eq!(compression_format_from_four_cc(0), CompressionFormat::Unknown);
    }

    #[test]
    fn dxt1_empty_data_is_error() {
        let data = build_header_bytes(4, 4, DDSD_WIDTH | DDSD_HEIGHT, DDPF_FOURCC, FOURCC_DXT1, 0, 0, 0, 0, 0);
        let file = parse(&data).unwrap();
        assert_eq!(decompress(&file), Err(DDSError::EmptyData));
    }

    #[test]
    fn uncompressed_32bpp_bgra_no_masks() {
        let mut data = build_header_bytes(1, 1, DDSD_WIDTH | DDSD_HEIGHT, DDPF_RGB, 0, 32, 0, 0, 0, 0);
        data.extend_from_slice(&[0x11, 0x22, 0x33, 0x44]); // B, G, R, A
        let file = parse(&data).unwrap();
        let rgba = decompress(&file).unwrap();
        assert_eq!(rgba, vec![0x33, 0x22, 0x11, 0x44]);
    }

    #[test]
    fn uncompressed_24bpp_bgr_no_masks() {
        let mut data = build_header_bytes(1, 1, DDSD_WIDTH | DDSD_HEIGHT, DDPF_RGB, 0, 24, 0, 0, 0, 0);
        data.extend_from_slice(&[0x11, 0x22, 0x33]); // B, G, R
        let file = parse(&data).unwrap();
        let rgba = decompress(&file).unwrap();
        assert_eq!(rgba, vec![0x33, 0x22, 0x11, 0xFF]);
    }

    #[test]
    fn uncompressed_32bpp_with_masks() {
        let mut data = build_header_bytes(1, 1, DDSD_WIDTH | DDSD_HEIGHT,
                                          DDPF_RGB | DDPF_ALPHAPIXELS, 0, 32,
                                          0x00FF_0000, 0x0000_FF00, 0x0000_00FF, 0xFF00_0000);
        data.extend_from_slice(&[0x11, 0x22, 0x33, 0x44]); // B=0x11 G=0x22 R=0x33 A=0x44 (LE)
        let file = parse(&data).unwrap();
        let rgba = decompress(&file).unwrap();
        // red mask 0x00FF0000 -> 0x33, green 0x0000FF00 -> 0x22, blue 0x000000FF -> 0x11
        assert_eq!(rgba, vec![0x33, 0x22, 0x11, 0x44]);
    }
}
