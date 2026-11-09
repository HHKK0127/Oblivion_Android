//! Reader for Oblivion (TES4) `.bsa` archives.
//!
//! The archive holds a header, a table of folder records, one contiguous block
//! per folder with that folder's name and file records, a trailing block of
//! file names, and the file data itself, addressed by absolute offset.
//!
//! Behaviour follows the engine's quirks rather than an idealised format,
//! because archives in the wild depend on them:
//!
//! * a folder record's offset is biased by the header's file name length;
//! * a path without a separator is hashed whole, extension included;
//! * when a folder name is one byte long the record block is read one byte
//!   early, exactly as the original reader does.
//!
//! Lookups and extraction work on raw bytes: archive names carry no encoding.

pub mod format;
pub mod hash;
pub mod path;

use std::collections::HashMap;
use std::fmt;
use std::fs::File;
use std::io::{Read, Seek, SeekFrom};
use std::path::Path;

use flate2::{Decompress, FlushDecompress, Status};

pub use format::{
    FileRecord, FolderRecord, Header, BSA_MAGIC, FILE_RECORD_SIZE, FLAG_COMPRESSED,
    FLAG_HAS_FILE_NAMES, FLAG_HAS_FOLDER_NAMES, FOLDER_RECORD_SIZE, HEADER_SIZE, OB_HEADER_VERSION,
    SIZE_COMPRESS_TOGGLE, SIZE_MASK,
};
pub use hash::{build_hash, compute_file_hash, compute_folder_hash, hash32};
pub use path::{extension_part, file_name_part, join_archive_path, normalize_lookup_path};

/// Upper bound on the folder records accepted from one archive.
const MAX_FOLDERS: u32 = 1 << 20;

/// Upper bound on the byte length of either name block.
const MAX_NAME_BYTES: u32 = 16 << 20;

/// Upper bound on the number of entries built from one archive.
const MAX_ENTRIES: usize = 1 << 22;

/// Size of the buffer an inflate round uses, matching the original reader.
const INFLATE_CHUNK_SIZE: usize = 16 * 1024;

/// Upper bound on the bytes one entry may inflate to, guarding against bombs.
const MAX_INFLATE_SIZE: usize = 1 << 30;

/// Why an archive could not be opened or read.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum BsaError {
    /// The archive is not open, or its file could not be opened or read.
    Io,
    /// The archive is not open.
    NotOpen,
    /// The first four bytes are not `BSA\0`.
    BadMagic,
    /// A structure the header points at runs past the end of the file.
    Truncated,
    /// The header asks for more structure than this reader accepts.
    Corrupt,
    /// An entry's data could not be inflated.
    Decompress,
    /// The requested entry index does not exist.
    IndexOutOfRange,
}

impl fmt::Display for BsaError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        let message = match self {
            BsaError::Io => "archive file could not be read",
            BsaError::NotOpen => "archive is not open",
            BsaError::BadMagic => "not a BSA archive",
            BsaError::Truncated => "archive structure runs past the end of the file",
            BsaError::Corrupt => "archive header declares unsupported sizes",
            BsaError::Decompress => "entry data could not be decompressed",
            BsaError::IndexOutOfRange => "entry index out of range",
        };
        formatter.write_str(message)
    }
}

impl std::error::Error for BsaError {}

/// One file inside an archive.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Entry {
    /// Folder and file name joined with `\`, as stored in the archive.
    pub full_path: Vec<u8>,
    /// [`full_path`](Entry::full_path) with `/` separators and lowercase ASCII.
    pub lookup_path: Vec<u8>,
    /// The file name on its own.
    pub file_name: Vec<u8>,
    /// The text after the last dot of the file name, lowercased; empty when the
    /// name has no dot.
    pub extension: Vec<u8>,
    /// The folder name, empty for archives without folder names.
    pub folder_path: Vec<u8>,
    /// The hash stored in the file record.
    pub hash: u64,
    /// Absolute offset of the entry's data.
    pub offset: u32,
    /// Stored byte count, with the compression toggle bit removed.
    pub size: u32,
    /// Uncompressed byte count, known only for stored (uncompressed) entries.
    pub real_size: u32,
    /// Whether the stored data is zlib-compressed.
    pub compressed: bool,
}

/// An open archive.
///
/// The struct holds one file handle and one reusable extraction buffer, so a
/// single archive must not be used from two threads at once. That matches the
/// original reader, which shared one `std::ifstream` per archive.
pub struct Archive {
    file: Option<File>,
    path: String,
    header: Header,
    entries: Vec<Entry>,
    hash_index: HashMap<u64, usize>,
    name_block_offset: u64,
    file_len: u64,
    open: bool,
    parsed: bool,
    data: Vec<u8>,
    found: Vec<usize>,
}

impl Default for Archive {
    fn default() -> Archive {
        Archive::new()
    }
}

impl Archive {
    /// Creates a closed archive with no state.
    pub fn new() -> Archive {
        Archive {
            file: None,
            path: String::new(),
            header: Header::default(),
            entries: Vec::new(),
            hash_index: HashMap::new(),
            name_block_offset: 0,
            file_len: 0,
            open: false,
            parsed: false,
            data: Vec::new(),
            found: Vec::new(),
        }
    }

    /// Opens `path`, reads the header and parses the archive index.
    ///
    /// A failure after the header was read leaves the archive open with
    /// whatever was parsed so far, so callers can inspect a partially readable
    /// archive the same way they could with the original reader.
    pub fn open(&mut self, path: impl AsRef<Path>) -> Result<(), BsaError> {
        self.close();

        self.path = path.as_ref().to_string_lossy().into_owned();
        let file = File::open(path.as_ref()).map_err(|_| BsaError::Io)?;
        self.file_len = file.metadata().map_err(|_| BsaError::Io)?.len();
        self.file = Some(file);

        let mut raw = [0u8; HEADER_SIZE];
        if let Err(error) = self.read_at(0, &mut raw) {
            self.close();
            return Err(error);
        }

        self.header = Header::parse(&raw);
        if self.header.magic != BSA_MAGIC {
            self.close();
            return Err(BsaError::BadMagic);
        }

        self.open = true;
        self.parse()
    }

    /// Closes the archive and drops everything parsed from it.
    pub fn close(&mut self) {
        self.file = None;
        self.path.clear();
        self.header = Header::default();
        self.entries.clear();
        self.hash_index.clear();
        self.found.clear();
        self.data.clear();
        self.name_block_offset = 0;
        self.file_len = 0;
        self.open = false;
        self.parsed = false;
    }

    /// Whether the archive file is open.
    pub fn is_open(&self) -> bool {
        self.open
    }

    /// Whether the index was parsed completely.
    pub fn is_parsed(&self) -> bool {
        self.parsed
    }

    /// The header as read from the archive.
    pub fn header(&self) -> &Header {
        &self.header
    }

    /// Every entry, in folder-then-record order.
    pub fn entries(&self) -> &[Entry] {
        &self.entries
    }

    /// The file count declared by the header.
    ///
    /// This is the header's number, not the number of entries that were parsed.
    pub fn file_count(&self) -> u32 {
        self.header.file_count
    }

    /// Whether entries are compressed unless a record toggles the state.
    pub fn is_compressed(&self) -> bool {
        self.header.is_compressed()
    }

    /// The path this archive was opened from, empty once closed.
    pub fn path(&self) -> &str {
        &self.path
    }

    /// Finds an entry by exact path, accepting either separator and any case.
    ///
    /// Returns `None` when the hash is not indexed or the indexed entry's path
    /// differs, which is how a hash collision is rejected.
    pub fn find_file(&self, path: &[u8]) -> Option<usize> {
        let normalized = normalize_lookup_path(path);
        let hash = compute_file_hash(&normalized);
        let index = *self.hash_index.get(&hash)?;
        let entry = &self.entries[index];
        if entry.lookup_path == normalized {
            Some(index)
        } else {
            None
        }
    }

    /// Finds every entry whose lookup path starts with `prefix`.
    pub fn find_files_by_prefix(&mut self, prefix: &[u8]) -> &[usize] {
        let normalized = normalize_lookup_path(prefix);
        let Archive { entries, found, .. } = self;
        found.clear();
        for (index, entry) in entries.iter().enumerate() {
            if entry.lookup_path.len() >= normalized.len()
                && entry.lookup_path[..normalized.len()] == normalized[..]
            {
                found.push(index);
            }
        }
        found
    }

    /// Finds every entry whose extension equals `extension`, ignoring ASCII case.
    ///
    /// The extension is written without its dot, for example `nif`.
    pub fn find_files_by_extension(&mut self, extension: &[u8]) -> &[usize] {
        let lowered: Vec<u8> = extension.iter().map(u8::to_ascii_lowercase).collect();
        let Archive { entries, found, .. } = self;
        found.clear();
        for (index, entry) in entries.iter().enumerate() {
            if entry.extension == lowered {
                found.push(index);
            }
        }
        found
    }

    /// Reads an entry's stored bytes, without inflating them.
    pub fn extract_raw(&mut self, index: usize) -> Result<&[u8], BsaError> {
        let (offset, size) = {
            let entry = self.entry(index)?;
            (entry.offset, entry.size)
        };
        self.read_into_data(offset as u64, size as usize)?;
        Ok(&self.data)
    }

    /// Reads an entry and inflates it when it is compressed.
    pub fn extract_decompressed(&mut self, index: usize) -> Result<&[u8], BsaError> {
        let (offset, size, compressed) = {
            let entry = self.entry(index)?;
            (entry.offset, entry.size, entry.compressed)
        };
        if !compressed {
            return self.extract_raw(index);
        }
        if size < 4 {
            return Err(BsaError::Decompress);
        }

        let mut prefix = [0u8; 4];
        self.read_at(offset as u64, &mut prefix)?;
        let expected_size = u32::from_le_bytes(prefix) as usize;

        let mut compressed_data = vec![0u8; (size - 4) as usize];
        self.read_at(offset as u64 + 4, &mut compressed_data)?;

        let mut output = std::mem::take(&mut self.data);
        let result = inflate(&compressed_data, expected_size, &mut output);
        self.data = output;
        result?;
        Ok(&self.data)
    }

    /// Reads a stored (uncompressed) entry, or any entry, into `buffer`.
    pub fn read_entry_raw(&mut self, index: usize) -> Result<Vec<u8>, BsaError> {
        Ok(self.extract_raw(index)?.to_vec())
    }

    fn entry(&self, index: usize) -> Result<&Entry, BsaError> {
        if !self.is_open() {
            return Err(BsaError::NotOpen);
        }
        self.entries.get(index).ok_or(BsaError::IndexOutOfRange)
    }

    fn read_into_data(&mut self, offset: u64, size: usize) -> Result<(), BsaError> {
        let mut buffer = std::mem::take(&mut self.data);
        buffer.clear();
        buffer.resize(size, 0);
        let result = self.read_at(offset, &mut buffer);
        self.data = buffer;
        result
    }

    fn read_at(&mut self, offset: u64, buffer: &mut [u8]) -> Result<(), BsaError> {
        if buffer.is_empty() {
            return Ok(());
        }
        let end = offset
            .checked_add(buffer.len() as u64)
            .ok_or(BsaError::Truncated)?;
        if end > self.file_len {
            return Err(BsaError::Truncated);
        }
        let file = self.file.as_mut().ok_or(BsaError::NotOpen)?;
        file.seek(SeekFrom::Start(offset))
            .map_err(|_| BsaError::Io)?;
        file.read_exact(buffer).map_err(|_| BsaError::Io)
    }

    fn parse(&mut self) -> Result<(), BsaError> {
        self.parse_folders()?;
        self.read_name_table()?;

        let mut index = HashMap::with_capacity(self.entries.len());
        for (position, entry) in self.entries.iter().enumerate() {
            index.insert(entry.hash, position);
        }
        self.hash_index = index;
        self.parsed = true;
        Ok(())
    }

    fn parse_folders(&mut self) -> Result<(), BsaError> {
        let header = self.header;
        if header.folder_count > MAX_FOLDERS {
            return Err(BsaError::Corrupt);
        }

        let table_len = u64::from(header.folder_count) * FOLDER_RECORD_SIZE as u64;
        let table_offset = u64::from(header.folder_record_offset);
        if table_offset + table_len > self.file_len {
            return Err(BsaError::Truncated);
        }

        let mut table = vec![0u8; table_len as usize];
        self.read_at(table_offset, &mut table)?;

        let mut folders = Vec::with_capacity(header.folder_count as usize);
        for chunk in table.chunks_exact(FOLDER_RECORD_SIZE) {
            folders.push(FolderRecord::parse(chunk));
        }

        self.entries.clear();
        self.entries
            .reserve((header.file_count as usize).min(MAX_ENTRIES));

        let mut block_end = table_offset + table_len;
        let mut records = Vec::new();

        for folder in &folders {
            let block_offset = if folder.offset >= header.file_name_length {
                u64::from(folder.offset - header.file_name_length)
            } else {
                u64::from(folder.offset)
            };

            let mut folder_path = Vec::new();
            let mut records_offset = block_offset;
            if header.archive_flags & FLAG_HAS_FOLDER_NAMES != 0 {
                let mut length_byte = [0u8; 1];
                self.read_at(block_offset, &mut length_byte)?;
                records_offset += 1;

                if length_byte[0] > 1 {
                    let mut name = vec![0u8; length_byte[0] as usize];
                    self.read_at(records_offset, &mut name)?;
                    records_offset += u64::from(length_byte[0]);

                    let length = name
                        .iter()
                        .position(|&byte| byte == 0)
                        .unwrap_or(name.len());
                    name.truncate(length);
                    folder_path = name;
                }
            }

            let record_bytes = u64::from(folder.file_count) * FILE_RECORD_SIZE as u64;
            if records_offset + record_bytes > self.file_len {
                return Err(BsaError::Truncated);
            }
            if self.entries.len() + folder.file_count as usize > MAX_ENTRIES {
                return Err(BsaError::Corrupt);
            }

            records.clear();
            records.resize(record_bytes as usize, 0);
            self.read_at(records_offset, &mut records)?;

            for chunk in records.chunks_exact(FILE_RECORD_SIZE) {
                let record = FileRecord::parse(chunk);

                let mut compressed = header.is_compressed();
                if record.size & SIZE_COMPRESS_TOGGLE != 0 {
                    compressed = !compressed;
                }
                let size = record.size & SIZE_MASK;

                self.entries.push(Entry {
                    full_path: Vec::new(),
                    lookup_path: Vec::new(),
                    file_name: Vec::new(),
                    extension: Vec::new(),
                    folder_path: folder_path.clone(),
                    hash: record.hash,
                    offset: record.offset,
                    size,
                    real_size: if compressed { 0 } else { size },
                    compressed,
                });
            }

            let name_bytes = if folder_path.is_empty() {
                0
            } else {
                folder_path.len() as u64 + 1
            };
            block_end = block_end.max(block_offset + 1 + name_bytes + record_bytes);
        }

        self.name_block_offset = block_end;
        Ok(())
    }

    fn read_name_table(&mut self) -> Result<(), BsaError> {
        let header = self.header;

        if header.archive_flags & FLAG_HAS_FILE_NAMES == 0 || header.file_name_length == 0 {
            for entry in &mut self.entries {
                let name = format!("{}.dat", entry.hash);
                entry.full_path = join_archive_path(&entry.folder_path, name.as_bytes());
                entry.lookup_path = normalize_lookup_path(&entry.full_path);
                entry.file_name = file_name_part(&entry.full_path).to_vec();
                entry.extension = extension_part(&entry.file_name);
            }
            return Ok(());
        }

        if header.file_name_length > MAX_NAME_BYTES {
            return Err(BsaError::Corrupt);
        }

        let mut block = vec![0u8; header.file_name_length as usize];
        self.read_at(self.name_block_offset, &mut block)?;

        let count = (header.file_count as usize).min(self.entries.len());
        let mut cursor = 0usize;
        for entry in self.entries.iter_mut().take(count) {
            if cursor >= block.len() {
                return Err(BsaError::Truncated);
            }
            let terminator = block[cursor..]
                .iter()
                .position(|&byte| byte == 0)
                .ok_or(BsaError::Truncated)?;
            let name = block[cursor..cursor + terminator].to_vec();
            cursor += terminator + 1;

            entry.full_path = if entry.folder_path.is_empty() {
                name.clone()
            } else {
                join_archive_path(&entry.folder_path, &name)
            };
            entry.lookup_path = normalize_lookup_path(&entry.full_path);
            entry.file_name = name;
            entry.extension = extension_part(&entry.file_name);
        }

        Ok(())
    }
}

/// Inflates `input`, accepting both zlib and raw deflate streams.
///
/// The entry's own uncompressed size header is a hint only; the stream decides
/// how much comes out. Output is capped at [`MAX_INFLATE_SIZE`].
fn inflate(input: &[u8], expected_size: usize, output: &mut Vec<u8>) -> Result<(), BsaError> {
    for &zlib_header in &[true, false] {
        let mut decompressor = Decompress::new(zlib_header);
        let mut chunk = vec![0u8; INFLATE_CHUNK_SIZE];
        let mut consumed = 0u64;
        let mut produced_total = 0u64;

        output.clear();
        output.reserve(expected_size.min(1 << 20));

        loop {
            let status = match decompressor.decompress(
                &input[consumed as usize..],
                &mut chunk,
                FlushDecompress::None,
            ) {
                Ok(status) => status,
                Err(_) => break,
            };

            let produced = (decompressor.total_out() - produced_total) as usize;
            consumed = decompressor.total_in();
            produced_total = decompressor.total_out();

            if produced > 0 {
                if output.len() + produced > MAX_INFLATE_SIZE {
                    return Err(BsaError::Decompress);
                }
                output.extend_from_slice(&chunk[..produced]);
            } else if consumed as usize >= input.len() {
                break;
            }

            if status == Status::StreamEnd {
                return Ok(());
            }
        }
    }

    Err(BsaError::Decompress)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn closed_archive_reports_no_state() {
        let mut archive = Archive::new();
        assert!(!archive.is_open());
        assert!(!archive.is_parsed());
        assert_eq!(archive.file_count(), 0);
        assert_eq!(archive.entries().len(), 0);
        assert!(matches!(archive.extract_raw(0), Err(BsaError::NotOpen)));
    }

    #[test]
    fn open_of_a_missing_file_fails() {
        let mut archive = Archive::new();
        let error = archive
            .open(Path::new("this-file-does-not-exist.bsa"))
            .unwrap_err();
        assert_eq!(error, BsaError::Io);
        assert!(!archive.is_open());
    }

    #[test]
    fn inflate_rejects_garbage() {
        let mut output = Vec::new();
        assert!(inflate(b"not compressed at all", 0, &mut output).is_err());
    }
}
