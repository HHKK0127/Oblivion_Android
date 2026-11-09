//! The game's BSA hash functions.
//!
//! Oblivion indexes archive entries by a 64-bit hash: the low 32 bits encode
//! the name's first, second-to-last and last characters plus its length, and
//! the high 32 bits are a polynomial digest of the middle characters. Files add
//! a small tweak for the four extensions the engine special-cases.
//!
//! Byte slices are used throughout because archive names are raw bytes; no
//! encoding is implied. All comparisons are ASCII-only, matching the engine.
//!
//! # Lowercasing
//!
//! Every byte the hash touches folds to lowercase with the same rule: only
//! `A`..`Z` become `a`..`z` by adding `0x20`. Bytes such as `_` (0x5F) or `[`
//! keep their value, so folding with `| 0x20` instead would corrupt the hash
//! for any name containing them. The golden values in the tests below are the
//! hashes Bethesda stored in shipped archives.

/// Multiplier of the polynomial digest.
const HASH_MULTIPLIER: u32 = 0x0010_03F;

/// Distance between an ASCII uppercase letter and its lowercase form.
const ASCII_LOWER_BIT: u8 = 0x20;

/// Folds an ASCII uppercase letter to lowercase, leaving every other byte alone.
fn lowercase_letter(byte: u8) -> u8 {
    if byte.is_ascii_uppercase() {
        byte + ASCII_LOWER_BIT
    } else {
        byte
    }
}

/// Hash of a name's middle bytes, used as the high half of a BSA hash.
///
/// Only `A`..`Z` fold; `_`, `[`, `\`, `]` and `^` fold to themselves.
pub fn hash32(bytes: &[u8]) -> u32 {
    let mut hash: u32 = 0;
    for &byte in bytes {
        hash = hash
            .wrapping_mul(HASH_MULTIPLIER)
            .wrapping_add(u32::from(lowercase_letter(byte)));
    }
    hash
}

/// Builds the 64-bit hash of `name`, optionally applying an extension tweak.
///
/// `extra` is the extension including its leading dot (for example `.nif`).
/// An empty slice means "no extension" and disables the tweak, which is what
/// folder hashes use.
///
/// Both inputs are treated as C strings: everything from the first NUL byte is
/// ignored, so a name carrying an embedded NUL hashes the same way the engine
/// hashes it.
pub fn build_hash(name: &[u8], extra: &[u8]) -> u64 {
    let name = c_string(name);
    let extra = c_string(extra);
    let length = name.len();

    let mut low = [0u8; 4];
    low[0] = if length > 0 {
        lowercase_letter(name[length - 1])
    } else {
        0
    };
    low[1] = if length > 2 {
        lowercase_letter(name[length - 2])
    } else {
        0
    };
    low[2] = (length & 0xFF) as u8;
    low[3] = if length > 0 {
        lowercase_letter(name[0])
    } else {
        0
    };

    let mut high: u32 = if length > 3 {
        // Skip the first character and the last two: they are already encoded
        // in `low`. The count is a byte, so long names fold only their prefix.
        let count = (length - 3) & 0xFF;
        hash32(&name[1..1 + count])
    } else {
        0
    };
    if !extra.is_empty() {
        let count = extra.len() & 0xFF;
        high = high.wrapping_add(hash32(&extra[..count]));
    }

    let tweak = u32::from(extension_tweak(extra));
    low[3] = low[3].wrapping_add(((tweak & 0xFC) << 5) as u8);
    low[0] = low[0].wrapping_add(((tweak & 0xFE) << 6) as u8);
    low[1] = low[1].wrapping_add(((tweak << 7) & 0xFF) as u8);

    u64::from(low[0])
        | (u64::from(low[1]) << 8)
        | (u64::from(low[2]) << 16)
        | (u64::from(low[3]) << 24)
        | (u64::from(high) << 32)
}

/// Hash of a full path, computed from its file name and extension only.
///
/// The folder part takes no part in a file hash; folder paths are hashed
/// separately. A path with no separator is hashed whole, extension included,
/// because the engine only isolates an extension after a separator.
pub fn compute_file_hash(path: &[u8]) -> u64 {
    let separator = path.iter().rposition(|&byte| byte == b'\\' || byte == b'/');
    let file_name = match separator {
        Some(index) => &path[index + 1..],
        None => path,
    };

    // The engine stores each record as a name plus an extension, so the split is
    // always made on the file name part. The C++ reader compared the dot against
    // the separator index and skipped the split entirely when the path had no
    // separator, which produced hashes no shipped archive contains.
    let (stem, extension) = match file_name.iter().rposition(|&byte| byte == b'.') {
        Some(dot) => (&file_name[..dot], &file_name[dot..]),
        None => (file_name, &[][..]),
    };

    build_hash(stem, extension)
}

/// Hash of a folder path, with no extension tweak.
pub fn compute_folder_hash(folder_path: &[u8]) -> u64 {
    build_hash(folder_path, &[])
}

/// The extension tweak the engine applies to `.nif`, `.kf`, `.dds` and `.wav`.
///
/// Returns `0` for every other extension. `extension` must include the leading
/// dot.
fn extension_tweak(extension: &[u8]) -> u8 {
    if extension.first() != Some(&b'.') {
        return 0;
    }

    match extension.len() {
        4 => match (
            extension[1].to_ascii_lowercase(),
            extension[2].to_ascii_lowercase(),
            extension[3].to_ascii_lowercase(),
        ) {
            (b'n', b'i', b'f') => 1,
            (b'd', b'd', b's') => 3,
            (b'w', b'a', b'v') => 4,
            _ => 0,
        },
        3 => {
            let second = extension[1].to_ascii_lowercase();
            let third = extension[2].to_ascii_lowercase();
            if second == b'k' && third == b'f' {
                2
            } else {
                0
            }
        }
        _ => 0,
    }
}

/// Truncates at the first NUL byte, mirroring the engine's C-string handling.
fn c_string(bytes: &[u8]) -> &[u8] {
    match bytes.iter().position(|&byte| byte == 0) {
        Some(index) => &bytes[..index],
        None => bytes,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn hash32_folds_only_ascii_letters() {
        assert_eq!(hash32(b"abc"), hash32(b"ABC"));
        assert_eq!(hash32(b"mesh"), hash32(b"MESH"));
        assert_ne!(hash32(b"mesh"), hash32(b"meshes"));
        assert_eq!(hash32(b""), 0);
        // `_` and `[` have no bit to add: folding must leave them alone.
        assert_eq!(hash32(b"_"), 0x5F);
        assert_eq!(hash32(b"["), 0x5B);
    }

    #[test]
    fn build_hash_lowercases_letters_only() {
        // Boundary bytes keep `_` (0x5F) intact instead of folding it to 0x7F.
        assert_eq!((build_hash(b"ab_", &[]) as u8), b'_');
        assert_eq!((build_hash(b"a_b", &[]) >> 8) as u8, b'_');
        assert_eq!((build_hash(b"ab_d", &[]) >> 24) as u8, b'a');
        assert_eq!((build_hash(b"ab_d", &[]) as u8), b'd');
        assert_eq!((build_hash(b"ABCD", &[]) >> 24) as u8, b'a');
    }

    #[test]
    fn file_hash_matches_hashes_stored_in_shipped_archives() {
        // Golden values read straight out of Oblivion's own BSA file records.
        assert_eq!(
            compute_file_hash(b"fonts\\tahoma_bold_small_0_lod_a.tex"),
            0x0044_5B21_FC74_195F_61
        );
        assert_eq!(
            compute_file_hash(b"menus\\loading_bar_ingame_menu.xml"),
            0x001A_3A4F_CE6C_176E_75
        );
        assert_eq!(
            compute_file_hash(
                b"sound\\voice\\dlcthievesden.esp\\imperial\\m\
                  \\dlc06thievesden_dlc06piratetopic02_00006fdc_1.lip"
            ),
            0x0002_3BB6_1364_2D5F_31
        );
        // The folder never takes part in a file hash.
        assert_eq!(
            compute_file_hash(b"x\\y\\daedric_helm.nif"),
            compute_file_hash(b"daedric_helm.nif")
        );
    }

    #[test]
    fn hash32_matches_hand_computed_sequence() {
        // h = 0; h = h * 0x1003F + byte, with 'A' folded to 'a'.
        let mut expected: u32 = 0;
        expected = expected.wrapping_mul(0x0010_03F).wrapping_add(0x61);
        expected = expected.wrapping_mul(0x0010_03F).wrapping_add(0x62);
        assert_eq!(hash32(b"Ab"), expected);
    }

    #[test]
    fn build_hash_encodes_length_in_third_byte() {
        let hash = build_hash(b"abcd", &[]);
        assert_eq!((hash >> 16) as u8, 4);
    }

    #[test]
    fn build_hash_lowercases_boundary_characters() {
        assert_eq!(build_hash(b"ABCD", &[]), build_hash(b"abcd", &[]));
        assert_eq!(build_hash(b"aBcd", &[]), build_hash(b"abcd", &[]));
    }

    #[test]
    fn build_hash_positions_boundary_characters() {
        let hash = build_hash(b"abcd", &[]);
        assert_eq!(hash as u8, b'd');
        assert_eq!((hash >> 8) as u8, b'c');
        assert_eq!((hash >> 24) as u8, b'a');
    }

    #[test]
    fn extension_tweak_recognizes_engine_extensions() {
        assert_eq!(extension_tweak(b".nif"), 1);
        assert_eq!(extension_tweak(b".NIF"), 1);
        assert_eq!(extension_tweak(b".kf"), 2);
        assert_eq!(extension_tweak(b".dds"), 3);
        assert_eq!(extension_tweak(b".wav"), 4);
        assert_eq!(extension_tweak(b".mp3"), 0);
        assert_eq!(extension_tweak(b"nif"), 0);
        assert_eq!(extension_tweak(b".nifx"), 0);
        assert_eq!(extension_tweak(b""), 0);
    }

    #[test]
    fn extension_tweak_changes_the_hash() {
        assert_ne!(build_hash(b"foo", b".nif"), build_hash(b"foo", &[]));
        assert_ne!(build_hash(b"foo", b".nif"), build_hash(b"foo", b".dds"));
    }

    #[test]
    fn compute_file_hash_ignores_the_folder() {
        let a = compute_file_hash(b"meshes\\armor\\helm.nif");
        let b = compute_file_hash(b"meshes\\clutter\\helm.nif");
        assert_eq!(a, b);
    }

    #[test]
    fn compute_file_hash_accepts_both_separators() {
        assert_eq!(
            compute_file_hash(b"meshes\\armor\\helm.nif"),
            compute_file_hash(b"meshes/armor/helm.nif")
        );
    }

    #[test]
    fn compute_file_hash_splits_the_extension_without_a_separator() {
        // A bare file name still splits: the engine stores name and extension
        // separately, so a path like `helm.nif` must hash as `helm` + `.nif`.
        let with_dot = compute_file_hash(b"helm.nif");
        assert_eq!(with_dot, build_hash(b"helm", b".nif"));
        assert_eq!(with_dot, compute_file_hash(b"meshes\\armor\\helm.nif"));
    }

    #[test]
    fn compute_file_hash_ignores_dots_in_folder() {
        let hash = compute_file_hash(b"meshes.d\\helm");
        assert_eq!(hash, build_hash(b"helm", &[]));
    }

    #[test]
    fn compute_file_hash_of_empty_name() {
        assert_eq!(compute_file_hash(b"meshes\\"), build_hash(b"", &[]));
    }

    #[test]
    fn build_hash_stops_at_nul() {
        assert_eq!(build_hash(b"abc\0def", &[]), build_hash(b"abc", &[]));
    }
}
