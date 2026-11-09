//! NIF header layouts.
//!
//! Retail Oblivion meshes ship four header families, and the walker has to
//! recognise all of them before it can find block 0:
//!
//! * `Gamebryo File Format` 20.0.0.x — endian byte, user version, BS version
//!   and three BzString script names;
//! * `Gamebryo File Format` 10.x — the same tail without the endian byte;
//! * `NetImmerse File Format` 10.0.1.x — no endian byte, no user version, no
//!   script names, block count straight after the version;
//! * anything below 5.0.0.1 — only a version and a block count, no type table
//!   at all, with every block body naming itself.
//!
//! The layout comments in `nif_types.h` were verified against 322 meshes
//! sampled from `Oblivion - Meshes.bsa`; this module keeps those rules.

use crate::reader::Reader;
use crate::NifError;

/// Version cut-offs the header logic depends on.
pub(crate) const VERSION_5_0_0_1: u32 = 0x0500_0001;
pub(crate) const VERSION_10_0_1_2: u32 = 0x0A00_0102;
pub(crate) const VERSION_20_0_0_0: u32 = 0x1400_0000;

/// Upper bounds that keep a corrupt header from turning into a huge
/// allocation. Retail files stay far below all of them.
const MAX_BLOCKS: u32 = 0x100_000;
const MAX_BLOCK_TYPES: u16 = 4096;
const MAX_GROUPS: u32 = 0x10000;
const MAX_MAGIC_BYTES: usize = 255;

/// The parsed header of one NIF file.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Header {
    /// Magic line without its terminator, e.g.
    /// `Gamebryo File Format, Version 20.0.0.5`.
    pub magic: String,
    /// Packed version, e.g. `0x1400_0005` for 20.0.0.5.
    pub version: u32,
    /// Endian byte, valid only when [`has_endian_byte`](Self::has_endian_byte).
    pub endian: u8,
    /// True for the 20.0.0.x family and later.
    pub has_endian_byte: bool,
    /// True for the compact `NetImmerse File Format` header.
    pub legacy_net_immerse: bool,
    /// False below 5.0.0.1, where the header holds no type table and every
    /// block body starts with its own inline type name.
    pub has_block_type_table: bool,
    /// User version (10 or 11 on the Gamebryo streams).
    pub user_version: u32,
    /// Number of blocks in the object array.
    pub num_objects: u32,
    /// Bethesda stream version: 11 for 20.0.0.4, 5 for 10.1.0.106,
    /// 6 or 9 for 10.2.0.0. Zero on the legacy NetImmerse streams, which carry
    /// no BS header at all.
    pub unknown_field: u32,
    /// Number of block groups; zero in every sampled retail mesh.
    pub num_groups: u32,
    /// Creator, process script and export script of the exporting tool.
    pub creator: String,
    pub process_script: String,
    pub export_script: String,
    /// One entry per distinct block type, in file order.
    pub block_type_names: Vec<String>,
    /// One entry per block, each a valid index into
    /// [`block_type_names`](Self::block_type_names).
    pub block_type_indices: Vec<u16>,
    /// File offset at which block 0 begins.
    pub block_data_offset: usize,
}

impl Default for Header {
    fn default() -> Self {
        Self {
            magic: String::new(),
            version: 0,
            endian: 0,
            has_endian_byte: false,
            legacy_net_immerse: false,
            // The type table is the default: only meshes below 5.0.0.1 lack
            // one, and read_header clears the flag for those.
            has_block_type_table: true,
            user_version: 0,
            num_objects: 0,
            unknown_field: 0,
            num_groups: 0,
            creator: String::new(),
            process_script: String::new(),
            export_script: String::new(),
            block_type_names: Vec::new(),
            block_type_indices: Vec::new(),
            block_data_offset: 0,
        }
    }
}

/// Read one header from `reader`, leaving the cursor on block 0.
pub(crate) fn read_header(reader: &mut Reader<'_>) -> Result<Header, NifError> {
    let mut header = Header::default();

    // The magic line is variable length and ends at a newline or a NUL.
    let mut magic = Vec::with_capacity(48);
    while magic.len() < MAX_MAGIC_BYTES {
        let byte = reader.u8();
        if reader.has_error() {
            return Err(NifError::Truncated);
        }
        if byte == b'\n' || byte == 0 {
            break;
        }
        magic.push(byte);
    }
    header.magic = String::from_utf8_lossy(&magic).into_owned();

    // Retail Oblivion ships two families: the Gamebryo header used by almost
    // every mesh, and a compact NetImmerse header on legacy meshes.
    header.legacy_net_immerse = header.magic.contains("NetImmerse File Format");
    if !header.legacy_net_immerse && !header.magic.contains("Gamebryo File Format") {
        return Err(NifError::BadMagic);
    }

    header.version = reader.u32();

    // Meshes older than 5.0.0.1 have no block type table and no header tail at
    // all: nif.xml opens "User Version" at 10.0.1.8 and both "Num Block Types"
    // and "Block Type Index" at 5.0.0.1, so the block count is the last header
    // field and block 0 starts right after it.
    if header.version < VERSION_5_0_0_1 {
        header.has_block_type_table = false;
        header.num_objects = reader.u32();
        if reader.has_error() || header.num_objects == 0 || header.num_objects > MAX_BLOCKS {
            return Err(NifError::Corrupt);
        }
        header.block_data_offset = reader.pos();
        return Ok(header);
    }

    if header.legacy_net_immerse {
        // NetImmerse 10.0.1.0 meshes have no endian byte, no user version, no
        // unknown field and no creator strings: the block count follows the
        // version.
        header.num_objects = reader.u32();

        // 10.0.1.2 is the single NetImmerse version that carries the Bethesda
        // stream header (nif.xml: BSStreamHeader cond is VER == 10.0.1.2): one
        // unknown uint followed by three byte strings. Verified against all 23
        // retail 10.0.1.2 meshes in the shipped BSA archives.
        if header.version == VERSION_10_0_1_2 {
            header.unknown_field = reader.u32();
            read_header_strings(reader, &mut header)?;
        }
    } else {
        // The 20.0.0.x family and later insert an endian byte after the version.
        if header.version >= VERSION_20_0_0_0 {
            header.endian = reader.u8();
            header.has_endian_byte = true;
        }

        header.user_version = reader.u32();
        header.num_objects = reader.u32();
        header.unknown_field = reader.u32();
        read_header_strings(reader, &mut header)?;
    }

    let num_block_types = reader.u16();
    if num_block_types == 0 || num_block_types > MAX_BLOCK_TYPES {
        return Err(NifError::Corrupt);
    }

    header.block_type_names.reserve(num_block_types as usize);
    for _ in 0..num_block_types {
        let name = reader.string().map_err(|_| NifError::Corrupt)?;
        header.block_type_names.push(name);
    }

    if header.num_objects == 0 || header.num_objects > MAX_BLOCKS {
        return Err(NifError::Corrupt);
    }

    header
        .block_type_indices
        .reserve(header.num_objects as usize);
    for _ in 0..header.num_objects {
        let type_index = reader.u16();
        if type_index >= num_block_types {
            return Err(NifError::Corrupt);
        }
        header.block_type_indices.push(type_index);
    }

    // Block groups. numGroups is 0 in every sampled retail mesh, and the group
    // array is not a string table: it sits between the block type index array
    // and block 0, so block 0 begins immediately after numGroups.
    header.num_groups = reader.u32();
    if header.num_groups > MAX_GROUPS {
        return Err(NifError::Corrupt);
    }
    for _ in 0..header.num_groups {
        reader.u32(); // Group description index
    }

    header.block_data_offset = reader.pos();
    if reader.has_error() {
        return Err(NifError::Corrupt);
    }
    Ok(header)
}

/// Creator / process script / export script, one BzString each.
fn read_header_strings(reader: &mut Reader<'_>, header: &mut Header) -> Result<(), NifError> {
    header.creator = reader.byte_string().map_err(|_| NifError::Corrupt)?;
    header.process_script = reader.byte_string().map_err(|_| NifError::Corrupt)?;
    header.export_script = reader.byte_string().map_err(|_| NifError::Corrupt)?;
    Ok(())
}
