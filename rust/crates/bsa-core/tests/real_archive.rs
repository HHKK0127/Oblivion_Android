//! Checks the reader against a real archive from an Oblivion installation.
//!
//! The tests do nothing unless `BSA_TEST_ARCHIVE` names a `.bsa` file, so the
//! suite stays runnable on a machine without the game data:
//!
//! ```text
//! BSA_TEST_ARCHIVE="C:\...\Data\DLCThievesDen.bsa" cargo test -p bsa-core --test real_archive
//! ```
//!
//! `BSA_TEST_MAX_ENTRIES` caps the extraction pass for very large archives.

use bsa_core::{compute_file_hash, Archive};

fn archive_path() -> Option<String> {
    match std::env::var("BSA_TEST_ARCHIVE") {
        Ok(path) if !path.trim().is_empty() => Some(path),
        _ => {
            eprintln!("BSA_TEST_ARCHIVE is not set, skipping real-archive checks");
            None
        }
    }
}

fn max_entries(default: usize) -> usize {
    std::env::var("BSA_TEST_MAX_ENTRIES")
        .ok()
        .and_then(|value| value.parse().ok())
        .unwrap_or(default)
}

#[test]
fn stored_hashes_match_the_engine_hash() {
    let Some(path) = archive_path() else {
        return;
    };
    let mut bsa = Archive::new();
    bsa.open(&path).expect("open real archive");
    assert!(bsa.is_parsed(), "archive index should parse");

    let entries = bsa.entries();
    assert!(!entries.is_empty(), "archive should hold entries");
    assert_eq!(
        entries.len() as u32,
        bsa.header().file_count,
        "every declared record should produce an entry"
    );

    let mut lookup_paths = std::collections::BTreeSet::new();
    // A file hash covers only the file name, so the same name in two folders
    // collides. The index keeps the last record for a colliding hash, as the
    // engine does, which makes the earlier ones unreachable by name.
    let mut winner = std::collections::HashMap::new();
    for (index, entry) in entries.iter().enumerate() {
        winner.insert(entry.hash, index);
    }
    let mut found_by_name = 0usize;
    let mut shadowed = 0usize;
    for (index, entry) in entries.iter().enumerate() {
        assert!(!entry.full_path.is_empty(), "entry {index} has no path");
        // Archives are created with the usual split hash, so a probe path with a
        // separator must reproduce the hash stored in the file record.
        let mut probe = Vec::from(&b"x\\"[..]);
        probe.extend_from_slice(&entry.file_name);
        assert_eq!(
            compute_file_hash(&probe),
            entry.hash,
            "hash mismatch for {:?}",
            String::from_utf8_lossy(&entry.full_path)
        );

        lookup_paths.insert(entry.lookup_path.clone());
        if entry.full_path.contains(&b'\\') {
            found_by_name += 1;
            let expected = (winner[&entry.hash] == index).then_some(index);
            if expected.is_none() {
                shadowed += 1;
            }
            assert_eq!(
                bsa.find_file(&entry.full_path),
                expected,
                "find_file disagreed for {:?}",
                String::from_utf8_lossy(&entry.full_path)
            );
        }
        assert!(
            entry.full_path.len() >= entry.file_name.len(),
            "file name should be a part of the path"
        );
    }

    assert!(found_by_name > 0, "archive should name its folders");
    assert!(
        lookup_paths.len() <= entries.len(),
        "lookup paths are unique only up to hash collisions"
    );
    eprintln!(
        "{} entries, {} distinct lookup paths, {} folders, {} shadowed by a hash collision",
        entries.len(),
        lookup_paths.len(),
        bsa.header().folder_count,
        shadowed
    );
}

#[test]
fn every_entry_extracts() {
    let Some(path) = archive_path() else {
        return;
    };
    let mut bsa = Archive::new();
    bsa.open(&path).expect("open real archive");

    let total = bsa.entries().len();
    let limit = max_entries(4096).min(total);
    let mut compressed = 0usize;
    let mut bytes = 0usize;

    for index in 0..limit {
        let (stored_size, real_size, is_compressed, name) = {
            let entry = &bsa.entries()[index];
            (
                entry.size,
                entry.real_size,
                entry.compressed,
                String::from_utf8_lossy(&entry.full_path).into_owned(),
            )
        };

        let raw = bsa
            .extract_raw(index)
            .unwrap_or_else(|error| panic!("raw read of {name} failed: {error}"))
            .to_vec();
        assert_eq!(raw.len(), stored_size as usize, "stored size of {name}");

        let data = bsa
            .extract_decompressed(index)
            .unwrap_or_else(|error| panic!("extract of {name} failed: {error}"));
        if is_compressed {
            compressed += 1;
            assert!(
                !data.is_empty() || stored_size == 4,
                "{name} inflated empty"
            );
        } else {
            assert_eq!(data.len(), real_size as usize, "real size of {name}");
            assert_eq!(data, raw, "stored bytes of {name} should round-trip");
        }
        bytes += data.len();
    }

    eprintln!("extracted {limit} of {total} entries, {compressed} compressed, {bytes} bytes");
}
