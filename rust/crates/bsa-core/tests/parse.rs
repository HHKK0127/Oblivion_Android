//! Round-trips synthetic archives through the parser.
//!
//! Every archive here is laid out by this file, so the tests double as a
//! readable description of the on-disk format the reader expects.

use std::fs;
use std::io::Write;
use std::path::{Path, PathBuf};

use bsa_core::{
    Archive, BsaError, BSA_MAGIC, FILE_RECORD_SIZE, FLAG_COMPRESSED, FLAG_HAS_FILE_NAMES,
    FLAG_HAS_FOLDER_NAMES, FOLDER_RECORD_SIZE, HEADER_SIZE, OB_HEADER_VERSION,
    SIZE_COMPRESS_TOGGLE,
};
use flate2::write::ZlibEncoder;
use flate2::Compression;

struct FileSpec {
    name: &'static [u8],
    data: &'static [u8],
    toggle: bool,
}

struct FolderSpec {
    name: &'static [u8],
    files: Vec<FileSpec>,
}

fn zlib(data: &[u8]) -> Vec<u8> {
    let mut encoder = ZlibEncoder::new(Vec::new(), Compression::default());
    encoder.write_all(data).expect("compress");
    encoder.finish().expect("finish")
}

fn push_u32(buffer: &mut Vec<u8>, value: u32) {
    buffer.extend_from_slice(&value.to_le_bytes());
}

fn push_u64(buffer: &mut Vec<u8>, value: u64) {
    buffer.extend_from_slice(&value.to_le_bytes());
}

/// Lays out an archive. `pad_before_names` reproduces the extra byte the
/// original reader adds to the name block offset when a folder has no name.
fn build(folders: &[FolderSpec], flags: u32, pad_before_names: usize) -> Vec<u8> {
    let has_folder_names = flags & FLAG_HAS_FOLDER_NAMES != 0;
    let has_file_names = flags & FLAG_HAS_FILE_NAMES != 0;
    let archive_compressed = flags & FLAG_COMPRESSED != 0;

    let file_count: u32 = folders.iter().map(|folder| folder.files.len() as u32).sum();
    let folder_name_length: u32 = if has_folder_names {
        folders
            .iter()
            .map(|folder| 1 + folder.name.len() as u32 + 1)
            .sum()
    } else {
        0
    };
    let file_name_length: u32 = if has_file_names {
        folders
            .iter()
            .flat_map(|folder| folder.files.iter())
            .map(|file| file.name.len() as u32 + 1)
            .sum()
    } else {
        0
    };

    let records_end = HEADER_SIZE as u32 + folders.len() as u32 * FOLDER_RECORD_SIZE as u32;
    let mut block_offsets = Vec::with_capacity(folders.len());
    let mut cursor = records_end;
    for folder in folders {
        block_offsets.push(cursor);
        cursor += if has_folder_names {
            1 + folder.name.len() as u32 + 1
        } else {
            0
        };
        cursor += folder.files.len() as u32 * FILE_RECORD_SIZE as u32;
    }
    let name_block_offset = cursor + pad_before_names as u32;
    let data_start = name_block_offset + file_name_length;

    let mut payloads = Vec::new();
    let mut sizes = Vec::new();
    for folder in folders {
        for file in &folder.files {
            let compressed = archive_compressed ^ file.toggle;
            let payload = if compressed {
                let mut body = (file.data.len() as u32).to_le_bytes().to_vec();
                body.extend_from_slice(&zlib(file.data));
                body
            } else {
                file.data.to_vec()
            };
            let mut size = payload.len() as u32;
            if file.toggle {
                size |= SIZE_COMPRESS_TOGGLE;
            }
            sizes.push(size);
            payloads.push(payload);
        }
    }

    let mut out = Vec::new();
    push_u32(&mut out, BSA_MAGIC);
    push_u32(&mut out, OB_HEADER_VERSION);
    push_u32(&mut out, HEADER_SIZE as u32);
    push_u32(&mut out, flags);
    push_u32(&mut out, folders.len() as u32);
    push_u32(&mut out, file_count);
    push_u32(&mut out, folder_name_length);
    push_u32(&mut out, file_name_length);
    push_u32(&mut out, 0);

    // Each folder record's offset is biased by the header's file name length.
    for (index, folder) in folders.iter().enumerate() {
        push_u64(&mut out, bsa_core::compute_folder_hash(folder.name));
        push_u32(&mut out, folder.files.len() as u32);
        push_u32(&mut out, block_offsets[index] + file_name_length);
    }

    let mut data_cursor = data_start;
    let mut sizes = sizes.into_iter();
    for folder in folders {
        if has_folder_names {
            out.push(folder.name.len() as u8 + 1);
            out.extend_from_slice(folder.name);
            out.push(0);
        }
        for file in &folder.files {
            let mut probe = Vec::from(&b"x\\"[..]);
            probe.extend_from_slice(file.name);
            let size = sizes.next().expect("size per record");
            push_u64(&mut out, bsa_core::compute_file_hash(&probe));
            push_u32(&mut out, size);
            push_u32(&mut out, data_cursor);
            data_cursor += size & bsa_core::SIZE_MASK;
        }
    }

    if has_file_names {
        out.resize(out.len() + pad_before_names, 0);
        for folder in folders {
            for file in &folder.files {
                out.extend_from_slice(file.name);
                out.push(0);
            }
        }
    }

    debug_assert_eq!(out.len() as u32, data_start);
    for payload in payloads {
        out.extend_from_slice(&payload);
    }
    out
}

struct TempArchive {
    path: PathBuf,
}

impl TempArchive {
    fn new(name: &str, bytes: &[u8]) -> TempArchive {
        let mut path = std::env::temp_dir();
        path.push(format!("bsa_core_test_{}_{}.bsa", name, std::process::id()));
        fs::write(&path, bytes).expect("write archive");
        TempArchive { path }
    }

    fn path(&self) -> &Path {
        &self.path
    }
}

impl Drop for TempArchive {
    fn drop(&mut self) {
        let _ = fs::remove_file(&self.path);
    }
}

fn with_names() -> Vec<FolderSpec> {
    vec![
        FolderSpec {
            name: b"meshes",
            files: vec![
                FileSpec {
                    name: b"A.NIF",
                    data: b"first",
                    toggle: false,
                },
                FileSpec {
                    name: b"b.nif",
                    data: b"second",
                    toggle: false,
                },
            ],
        },
        FolderSpec {
            name: b"Textures",
            files: vec![FileSpec {
                name: b"c.dds",
                data: b"compressed payload",
                toggle: true,
            }],
        },
    ]
}

#[test]
fn parses_headers_entries_and_lookups() {
    let flags = FLAG_HAS_FOLDER_NAMES | FLAG_HAS_FILE_NAMES;
    let files = with_names();
    let archive = TempArchive::new("named", &build(&files, flags, 0));

    let mut bsa = Archive::new();
    assert!(!bsa.is_open());
    bsa.open(archive.path()).expect("open");
    assert!(bsa.is_open());
    assert!(bsa.is_parsed());

    let header = bsa.header();
    assert_eq!(header.magic, BSA_MAGIC);
    assert_eq!(header.version, OB_HEADER_VERSION);
    assert_eq!(header.folder_count, 2);
    assert_eq!(header.file_count, 3);
    assert_eq!(header.folder_name_length, 8 + 10);
    assert_eq!(header.file_name_length, 6 + 6 + 6);
    assert!(!bsa.is_compressed());
    assert_eq!(bsa.file_count(), 3);

    let entries = bsa.entries();
    assert_eq!(entries.len(), 3);
    assert_eq!(entries[0].full_path, b"meshes\\A.NIF");
    assert_eq!(entries[0].lookup_path, b"meshes/a.nif");
    assert_eq!(entries[0].file_name, b"A.NIF");
    assert_eq!(entries[0].extension, b"nif");
    assert_eq!(entries[0].folder_path, b"meshes");
    assert_eq!(entries[0].size, 5);
    assert_eq!(entries[0].real_size, 5);
    assert!(!entries[0].compressed);
    assert_eq!(entries[2].full_path, b"Textures\\c.dds");
    assert_eq!(entries[2].lookup_path, b"textures/c.dds");
    assert_eq!(entries[2].extension, b"dds");
    assert!(entries[2].compressed);
    assert_eq!(entries[2].real_size, 0);

    assert_eq!(bsa.extract_raw(0).expect("raw"), b"first");
    assert_eq!(bsa.extract_raw(1).expect("raw"), b"second");
    assert_eq!(
        bsa.extract_decompressed(2).expect("inflate"),
        b"compressed payload"
    );
    assert!(matches!(bsa.extract_raw(3), Err(BsaError::IndexOutOfRange)));

    for (path, expected) in [
        (&b"meshes\\A.NIF"[..], 0usize),
        (b"meshes/a.nif", 0),
        (b"MESHES\\B.NIF", 1),
        (b"textures/c.dds", 2),
    ] {
        assert_eq!(bsa.find_file(path), Some(expected), "lookup {:?}", path);
    }
    assert_eq!(bsa.find_file(b"meshes/missing.nif"), None);

    let prefix = bsa.find_files_by_prefix(b"meshes\\").to_vec();
    assert_eq!(prefix, vec![0, 1]);
    let textures = bsa.find_files_by_prefix(b"Textures/").to_vec();
    assert_eq!(textures, vec![2]);
    assert!(bsa.find_files_by_prefix(b"nothing").is_empty());

    let nif = bsa.find_files_by_extension(b"nif").to_vec();
    assert_eq!(nif, vec![0, 1]);
    let dds = bsa.find_files_by_extension(b"DDS").to_vec();
    assert_eq!(dds, vec![2]);
    assert!(bsa.find_files_by_extension(b"wav").is_empty());

    assert!(!bsa.path().is_empty());
    bsa.close();
    assert!(!bsa.is_open());
    assert!(bsa.entries().is_empty());
    assert!(bsa.path().is_empty());
    assert!(matches!(bsa.extract_raw(0), Err(BsaError::NotOpen)));
}

#[test]
fn synthesizes_names_when_the_archive_has_no_name_table() {
    let flags = FLAG_HAS_FOLDER_NAMES;
    let files = vec![FolderSpec {
        name: b"meshes",
        files: vec![FileSpec {
            name: b"a.nif",
            data: b"payload",
            toggle: false,
        }],
    }];
    let archive = TempArchive::new("nonames", &build(&files, flags, 0));

    let mut bsa = Archive::new();
    bsa.open(archive.path()).expect("open");

    let entries = bsa.entries();
    assert_eq!(entries.len(), 1);
    let synthesized = format!("{}.dat", entries[0].hash);
    assert_eq!(entries[0].file_name, synthesized.as_bytes());
    assert_eq!(
        entries[0].full_path,
        format!("meshes\\{synthesized}").as_bytes()
    );
    assert_eq!(entries[0].extension, b"dat");
    assert_eq!(entries[0].folder_path, b"meshes");
}

#[test]
fn folders_without_names_leave_a_gap_the_reader_skips() {
    let flags = FLAG_HAS_FILE_NAMES | FLAG_COMPRESSED;
    let files = vec![FolderSpec {
        name: b"ignored",
        files: vec![
            FileSpec {
                name: b"a.nif",
                data: b"alpha",
                toggle: false,
            },
            FileSpec {
                name: b"b.nif",
                data: b"beta beta beta",
                toggle: true,
            },
        ],
    }];
    let archive = TempArchive::new("nofolders", &build(&files, flags, 1));

    let mut bsa = Archive::new();
    bsa.open(archive.path()).expect("open");
    assert!(bsa.is_compressed());

    let entries = bsa.entries();
    assert_eq!(entries.len(), 2);
    assert!(entries[0].folder_path.is_empty());
    assert_eq!(entries[0].full_path, b"a.nif");
    assert_eq!(entries[0].lookup_path, b"a.nif");
    assert!(entries[0].compressed);
    assert!(!entries[1].compressed);
    assert_eq!(entries[1].real_size, 14);

    assert_eq!(bsa.extract_decompressed(0).expect("inflate"), b"alpha");
    assert_eq!(
        bsa.extract_decompressed(1).expect("stored"),
        b"beta beta beta"
    );
    // A path without a separator still splits name and extension, so the entry
    // stays findable by its bare file name.
    assert_eq!(bsa.find_file(b"A.NIF"), Some(0));
    assert_eq!(bsa.find_file(b"b.nif"), Some(1));
    assert_eq!(bsa.find_files_by_extension(b"nif").to_vec(), vec![0, 1]);
}

#[test]
fn rejects_files_that_are_not_archives() {
    let archive = TempArchive::new(
        "bogus",
        b"this is not a BSA archive at all, it is only some text of the right length",
    );
    let mut bsa = Archive::new();
    assert!(matches!(bsa.open(archive.path()), Err(BsaError::BadMagic)));
    assert!(!bsa.is_open());
    assert!(bsa.path().is_empty());
}

#[test]
fn rejects_files_shorter_than_a_header() {
    let archive = TempArchive::new("tiny", b"BSA");
    let mut bsa = Archive::new();
    assert!(bsa.open(archive.path()).is_err());
    assert!(!bsa.is_open());
    assert!(bsa.path().is_empty());
}

#[test]
fn reports_truncated_tables() {
    let flags = FLAG_HAS_FOLDER_NAMES | FLAG_HAS_FILE_NAMES;
    let mut bytes = build(&with_names(), flags, 0);
    bytes.truncate(HEADER_SIZE + 8);
    let archive = TempArchive::new("truncated", &bytes);

    let mut bsa = Archive::new();
    assert!(matches!(bsa.open(archive.path()), Err(BsaError::Truncated)));
    assert!(bsa.is_open());
    assert!(!bsa.is_parsed());
}
