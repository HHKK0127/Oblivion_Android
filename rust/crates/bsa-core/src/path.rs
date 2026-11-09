//! Path helpers shared by archive parsing and lookup.
//!
//! Archive paths are raw bytes with backslash separators. Lookup accepts either
//! separator and ignores ASCII case, so both sides are folded into one
//! comparable form before they are matched.

/// Folds a path into its lookup form: `/` separators and lowercase ASCII.
///
/// Only bytes `A`..`Z` are lowered; everything else, including bytes above
/// `0x7F`, is copied unchanged so archive names survive the round trip.
pub fn normalize_lookup_path(path: &[u8]) -> Vec<u8> {
    let mut normalized = Vec::with_capacity(path.len());
    for &byte in path {
        normalized.push(match byte {
            b'\\' => b'/',
            b'A'..=b'Z' => byte + 0x20,
            _ => byte,
        });
    }
    normalized
}

/// The part of `path` after its last separator, or all of `path`.
pub fn file_name_part(path: &[u8]) -> &[u8] {
    match path.iter().rposition(|&byte| byte == b'\\' || byte == b'/') {
        Some(index) => &path[index + 1..],
        None => path,
    }
}

/// The extension of `path` without its leading dot, lowercased.
///
/// Returns an empty slice when the path has no dot. When the path has several
/// dots the last one wins.
pub fn extension_part(path: &[u8]) -> Vec<u8> {
    match path.iter().rposition(|&byte| byte == b'.') {
        Some(index) => path[index + 1..]
            .iter()
            .map(u8::to_ascii_lowercase)
            .collect(),
        None => Vec::new(),
    }
}

/// Concatenates `folder` and `name` with the archive's `\` separator.
///
/// An empty folder yields the name on its own, matching how archives without
/// folder names store their entries.
pub fn join_archive_path(folder: &[u8], name: &[u8]) -> Vec<u8> {
    let mut path = Vec::with_capacity(folder.len() + 1 + name.len());
    path.extend_from_slice(folder);
    if !folder.is_empty() {
        path.push(b'\\');
    }
    path.extend_from_slice(name);
    path
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn normalize_replaces_separators() {
        assert_eq!(
            normalize_lookup_path(b"Meshes\\Architecture\\Wall.nif"),
            b"meshes/architecture/wall.nif".to_vec()
        );
    }

    #[test]
    fn normalize_leaves_slashes_alone() {
        assert_eq!(
            normalize_lookup_path(b"meshes/armor/helm.nif"),
            b"meshes/armor/helm.nif".to_vec()
        );
    }

    #[test]
    fn normalize_preserves_non_ascii_bytes() {
        assert_eq!(
            normalize_lookup_path(&[0xC3, 0xA9, b'\\', b'A']),
            vec![0xC3, 0xA9, b'/', b'a']
        );
    }

    #[test]
    fn normalize_is_idempotent() {
        let once = normalize_lookup_path(b"A\\B/\xC7");
        assert_eq!(normalize_lookup_path(&once), once);
    }

    #[test]
    fn file_name_uses_last_separator() {
        assert_eq!(file_name_part(b"meshes\\armor\\helm.nif"), b"helm.nif");
        assert_eq!(file_name_part(b"meshes/armor/helm.nif"), b"helm.nif");
        assert_eq!(file_name_part(b"helm.nif"), b"helm.nif");
    }

    #[test]
    fn file_name_of_trailing_separator_is_empty() {
        assert_eq!(file_name_part(b"meshes\\"), b"");
    }

    #[test]
    fn extension_drops_the_dot_and_lowercases() {
        assert_eq!(extension_part(b"helm.NIF"), b"nif".to_vec());
        assert_eq!(extension_part(b"meshes\\helm.dds"), b"dds".to_vec());
        assert_eq!(extension_part(b"archive.7z.backup"), b"backup".to_vec());
    }

    #[test]
    fn extension_of_a_name_without_dot_is_empty() {
        assert_eq!(extension_part(b"readme"), Vec::<u8>::new());
        assert_eq!(extension_part(b"meshes\\readme"), Vec::<u8>::new());
    }

    #[test]
    fn extension_of_a_trailing_dot_is_empty() {
        assert_eq!(extension_part(b"helm."), Vec::<u8>::new());
    }

    #[test]
    fn join_adds_a_separator_only_when_needed() {
        assert_eq!(
            join_archive_path(b"meshes\\armor", b"helm.nif"),
            b"meshes\\armor\\helm.nif".to_vec()
        );
        assert_eq!(join_archive_path(b"", b"helm.nif"), b"helm.nif".to_vec());
    }
}
