//! On-disk layout of an Oblivion `.bsa` archive.
//!
//! The layout follows the public UESP description ("Oblivion Mod:BSA File
//! Format"): a 36-byte header, a table of folder records, one contiguous block
//! per folder holding that folder's name and its file records, and finally the
//! concatenated file name block. File data lives after the metadata region and
//! is addressed by absolute offset.
//!
//! All integers are little-endian; every supported target (x86, x86-64, ARM,
//! AArch64) stores them that way, and decoding explicitly keeps the parser
//! independent of host endianness.

/// Magic in the first four bytes: `"BSA\0"`.
pub const BSA_MAGIC: u32 = 0x0041_5342;

/// Oblivion-era archive version. Morrowind and Skyrim use other values.
pub const OB_HEADER_VERSION: u32 = 0x67;

/// Folder names are stored in the folder blocks.
pub const FLAG_HAS_FOLDER_NAMES: u32 = 0x0000_0001;

/// File names are stored in the trailing name block.
pub const FLAG_HAS_FILE_NAMES: u32 = 0x0000_0002;

/// Entries are zlib-compressed unless their record toggles the state.
pub const FLAG_COMPRESSED: u32 = 0x0000_0004;

/// In a file record's size field, this bit inverts the archive default.
pub const SIZE_COMPRESS_TOGGLE: u32 = 0x4000_0000;

/// The part of a file record's size field that is the stored byte count.
pub const SIZE_MASK: u32 = 0x3FFF_FFFF;

/// Byte length of the archive header, nine `u32` fields.
pub const HEADER_SIZE: usize = 36;

/// Byte length of one folder record: `u64` hash, `u32` file count, `u32` offset.
pub const FOLDER_RECORD_SIZE: usize = 16;

/// Byte length of one file record: `u64` hash, `u32` size, `u32` offset.
pub const FILE_RECORD_SIZE: usize = 16;

/// The archive header, field for field.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Header {
    /// [`BSA_MAGIC`] in a readable Oblivion archive.
    pub magic: u32,
    /// [`OB_HEADER_VERSION`] in an Oblivion archive.
    pub version: u32,
    /// Absolute offset of the folder record table.
    pub folder_record_offset: u32,
    /// Bit mask of the `FLAG_*` constants.
    pub archive_flags: u32,
    /// Number of folder records.
    pub folder_count: u32,
    /// Number of file records across all folders.
    pub file_count: u32,
    /// Total byte length of the folder name strings, including terminators.
    pub folder_name_length: u32,
    /// Total byte length of the file name block, including terminators.
    pub file_name_length: u32,
    /// Per-archive file flags; not interpreted by this reader.
    pub file_flags: u32,
}

impl Header {
    /// Decode a header from its 36-byte on-disk form.
    pub fn parse(bytes: &[u8; HEADER_SIZE]) -> Header {
        Header {
            magic: read_u32(&bytes[0..4]),
            version: read_u32(&bytes[4..8]),
            folder_record_offset: read_u32(&bytes[8..12]),
            archive_flags: read_u32(&bytes[12..16]),
            folder_count: read_u32(&bytes[16..20]),
            file_count: read_u32(&bytes[20..24]),
            folder_name_length: read_u32(&bytes[24..28]),
            file_name_length: read_u32(&bytes[28..32]),
            file_flags: read_u32(&bytes[32..36]),
        }
    }

    /// Whether the archive compresses entries by default.
    pub fn is_compressed(&self) -> bool {
        self.archive_flags & FLAG_COMPRESSED != 0
    }
}

/// One entry of the folder record table.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct FolderRecord {
    /// Hash of the folder path.
    pub hash: u64,
    /// Number of file records that follow the folder name.
    pub file_count: u32,
    /// Offset of the folder block, biased by [`Header::file_name_length`].
    pub offset: u32,
}

impl FolderRecord {
    /// Decode one folder record from its 16-byte on-disk form.
    pub fn parse(bytes: &[u8]) -> FolderRecord {
        FolderRecord {
            hash: read_u64(&bytes[0..8]),
            file_count: read_u32(&bytes[8..12]),
            offset: read_u32(&bytes[12..16]),
        }
    }
}

/// One file record inside a folder block.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct FileRecord {
    /// Hash of the file name and extension.
    pub hash: u64,
    /// Stored byte count, plus [`SIZE_COMPRESS_TOGGLE`] when the compression
    /// state is inverted for this entry.
    pub size: u32,
    /// Absolute offset of the entry's data.
    pub offset: u32,
}

impl FileRecord {
    /// Decode one file record from its 16-byte on-disk form.
    pub fn parse(bytes: &[u8]) -> FileRecord {
        FileRecord {
            hash: read_u64(&bytes[0..8]),
            size: read_u32(&bytes[8..12]),
            offset: read_u32(&bytes[12..16]),
        }
    }
}

fn read_u32(bytes: &[u8]) -> u32 {
    let mut value = [0u8; 4];
    value.copy_from_slice(&bytes[..4]);
    u32::from_le_bytes(value)
}

fn read_u64(bytes: &[u8]) -> u64 {
    let mut value = [0u8; 8];
    value.copy_from_slice(&bytes[..8]);
    u64::from_le_bytes(value)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn header_round_trips_field_order() {
        let mut bytes = [0u8; HEADER_SIZE];
        for (index, value) in [BSA_MAGIC, 0x67, 36, 0x03, 2, 3, 17, 29, 0x07]
            .iter()
            .enumerate()
        {
            bytes[index * 4..index * 4 + 4].copy_from_slice(&value.to_le_bytes());
        }

        let header = Header::parse(&bytes);
        assert_eq!(header.magic, BSA_MAGIC);
        assert_eq!(header.version, OB_HEADER_VERSION);
        assert_eq!(header.folder_record_offset, 36);
        assert_eq!(header.archive_flags, 0x03);
        assert_eq!(header.folder_count, 2);
        assert_eq!(header.file_count, 3);
        assert_eq!(header.folder_name_length, 17);
        assert_eq!(header.file_name_length, 29);
        assert_eq!(header.file_flags, 0x07);
        assert!(!header.is_compressed());
    }

    #[test]
    fn folder_and_file_records_decode() {
        let mut bytes = Vec::new();
        bytes.extend_from_slice(&0x1122_3344_5566_7788u64.to_le_bytes());
        bytes.extend_from_slice(&9u32.to_le_bytes());
        bytes.extend_from_slice(&0xABCD_1234u32.to_le_bytes());

        let folder = FolderRecord::parse(&bytes);
        assert_eq!(folder.hash, 0x1122_3344_5566_7788);
        assert_eq!(folder.file_count, 9);
        assert_eq!(folder.offset, 0xABCD_1234);

        let file = FileRecord::parse(&bytes);
        assert_eq!(file.hash, 0x1122_3344_5566_7788);
        assert_eq!(file.size, 9);
        assert_eq!(file.offset, 0xABCD_1234);
    }
}
