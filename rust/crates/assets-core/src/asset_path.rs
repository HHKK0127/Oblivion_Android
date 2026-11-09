//! Game-path expansion for Oblivion's Data folder layout.
//!
//! ESM records name models and textures relative to their type folder
//! (`Dungeons\Misc\TrigZone01.NIF`), while both the BSA archives and the loose
//! Data folder store them under that folder
//! (`Meshes\Dungeons\Misc\TrigZone01.NIF`). Every asset lookup therefore has to
//! try the type-folder spelling as well, otherwise interior cells resolve zero
//! meshes and the world silently renders as clear colour.
//!
//! The caller converts `\` to `/` before calling in, and the input is handled as
//! bytes: ESM strings are not guaranteed to be valid UTF-8, and only `'/'`, `'.'`
//! and `A`-`Z` are ever inspected.

/// Type folders that sit directly under `Data\` and therefore appear as the
/// first path segment of every BSA entry.
const ROOT_FOLDERS: [&str; 9] = [
    "meshes", "textures", "sound", "menus", "music", "fonts", "shaders", "video", "trees",
];

/// Type folders that sit directly under `Data\`, in lookup-priority order.
pub fn asset_root_folders() -> &'static [&'static str] {
    &ROOT_FOLDERS
}

/// Expand a game path into the spellings that can resolve, most likely first.
///
/// `normalized` must already have backslashes converted to `'/'`. An already
/// rooted path yields exactly one candidate, because prefixing it again would
/// only produce `meshes/meshes/...` and waste a lookup on every request.
pub fn build_asset_path_candidates(normalized: &[u8]) -> Vec<Vec<u8>> {
    let mut candidates: Vec<Vec<u8>> = vec![normalized.to_vec()];

    // Some ESM records spell a model path with a leading separator
    // ("/TreeSugarMapleYoungSU.spt"). Left in place the first segment is empty,
    // which matches no type folder, and every prefixed spelling then carries a
    // double separator ("trees//TreeSugarMapleYoungSU.spt") that no BSA entry or
    // loose file has, so all 22 SpeedTree models in the corpus failed to resolve.
    let path: &[u8] = match normalized.iter().position(|&c| c != b'/') {
        None => b"",
        Some(first_non_slash) => &normalized[first_non_slash..],
    };
    if path.is_empty() {
        return candidates;
    }
    if path != normalized {
        candidates.push(path.to_vec());
    }

    // Folder roots and extensions are matched case-insensitively: ESM records
    // are inconsistent about it ("TrigZone01.NIF" vs "trigzone01.nif").
    let slash = path.iter().position(|&c| c == b'/').unwrap_or(path.len());
    let first_segment = path[..slash].to_ascii_lowercase();
    if ROOT_FOLDERS
        .iter()
        .any(|root| root.as_bytes() == first_segment.as_slice())
    {
        return candidates; // Already carries its type folder.
    }

    // Extension-specific root first, then the remaining roots as a safety net
    // for assets whose extension does not reveal where they live.
    let extension = match path.iter().rposition(|&c| c == b'.') {
        None => Vec::new(),
        Some(dot) => path[dot..].to_ascii_lowercase(),
    };
    let mut ordered: Vec<&str> = Vec::new();
    match extension.as_slice() {
        b".nif" | b".kf" | b".kfa" => ordered.push("meshes"),
        b".dds" | b".tga" | b".bmp" => ordered.push("textures"),
        b".wav" | b".mp3" | b".ogg" => ordered.push("sound"),
        b".xml" | b".txt" | b".html" | b".fnt" => ordered.push("menus"),
        b".spt" => ordered.push("trees"),
        _ => {}
    }
    for root in ROOT_FOLDERS {
        if !ordered.iter().any(|&ordered_root| ordered_root == root) {
            ordered.push(root);
        }
    }

    for root in ordered {
        let mut candidate = Vec::with_capacity(root.len() + 1 + path.len());
        candidate.extend_from_slice(root.as_bytes());
        candidate.push(b'/');
        candidate.extend_from_slice(path);
        candidates.push(candidate);
    }
    candidates
}

#[cfg(test)]
mod tests {
    use super::*;

    fn candidates(rest_of_path: &str) -> Vec<Vec<u8>> {
        let normalized = rest_of_path.replace('\\', "/");
        build_asset_path_candidates(normalized.as_bytes())
    }

    fn contains(candidates: &[Vec<u8>], expected: &str) -> bool {
        candidates
            .iter()
            .any(|c| c.as_slice() == expected.as_bytes())
    }

    fn assert_candidate(candidates: &[Vec<u8>], index: usize, expected: &[u8]) {
        assert_eq!(candidates[index], expected, "candidate {index}");
    }

    fn as_texts(candidates: &[Vec<u8>]) -> Vec<String> {
        candidates
            .iter()
            .map(|c| String::from_utf8_lossy(c).into_owned())
            .collect()
    }

    #[test]
    fn nif_keeps_the_original_spelling_as_first_candidate() {
        // The regression: an ESM model path must gain the meshes root, which is
        // the only spelling the real Oblivion - Meshes.bsa stores.
        let c = candidates("Dungeons\\Misc\\Triggers\\TrigZone01.NIF");
        assert_candidate(&c, 0, b"Dungeons/Misc/Triggers/TrigZone01.NIF");
    }

    #[test]
    fn nif_gains_the_meshes_root() {
        let c = candidates("Dungeons\\Misc\\Triggers\\TrigZone01.NIF");
        assert!(contains(&c, "meshes/Dungeons/Misc/Triggers/TrigZone01.NIF"));
    }

    #[test]
    fn meshes_is_the_preferred_nif_root() {
        let c = candidates("Dungeons\\Misc\\Triggers\\TrigZone01.NIF");
        assert_candidate(&c, 1, b"meshes/Dungeons/Misc/Triggers/TrigZone01.NIF");
    }

    #[test]
    fn textures_resolve_the_same_way() {
        let c = candidates("Landscape\\Grass01.dds");
        assert!(contains(&c, "textures/Landscape/Grass01.dds"));
        assert_candidate(&c, 1, b"textures/Landscape/Grass01.dds");
    }

    #[test]
    fn sound_files_live_under_sound() {
        let c = candidates("fx\\env\\dungeon.wav");
        assert!(contains(&c, "sound/fx/env/dungeon.wav"));
    }

    #[test]
    fn speedtree_files_live_under_trees_and_not_under_meshes() {
        let c = candidates("TreeCottonwoodSU.spt");
        assert!(contains(&c, "trees/TreeCottonwoodSU.spt"));
        assert_candidate(&c, 1, b"trees/TreeCottonwoodSU.spt");
    }

    #[test]
    fn an_already_rooted_path_is_returned_untouched() {
        let c = candidates("Meshes\\Architecture\\IC\\ICWall01.NIF");
        assert_eq!(as_texts(&c), vec!["Meshes/Architecture/IC/ICWall01.NIF"]);
    }

    #[test]
    fn root_detection_is_case_insensitive() {
        let c = candidates("MESHES\\Architecture\\ICWall01.NIF");
        assert_eq!(c.len(), 1);
    }

    #[test]
    fn the_original_spelling_always_precedes_prefixed_variants() {
        let c = candidates("Clutter\\Wine\\WineRed01.NIF");
        assert_candidate(&c, 0, b"Clutter/Wine/WineRed01.NIF");
    }

    #[test]
    fn every_known_root_is_offered_for_a_mesh() {
        // So that a mod shipping a model outside Meshes\ still resolves.
        let c = candidates("Characters\\Imperial\\Male.nif");
        for root in asset_root_folders() {
            let expected = format!("{root}/Characters/Imperial/Male.nif");
            assert!(contains(&c, &expected), "missing {expected}");
        }
    }

    #[test]
    fn uppercase_extensions_still_prefer_meshes() {
        let c = candidates("Dungeons\\Misc\\Thing.NIF");
        assert_candidate(&c, 1, b"meshes/Dungeons/Misc/Thing.NIF");
    }

    #[test]
    fn extensionless_paths_still_offer_the_meshes_root() {
        let c = candidates("Some\\Odd\\Asset");
        assert!(contains(&c, "meshes/Some/Odd/Asset"));
    }

    #[test]
    fn a_bare_file_name_is_still_expandable() {
        let c = candidates("Marker_Prison.nif");
        assert!(contains(&c, "meshes/Marker_Prison.nif"));
    }

    #[test]
    fn candidates_are_never_empty_and_never_duplicated() {
        let c = candidates("a\\b\\c.nif");
        assert!(!c.is_empty());
        let mut sorted = as_texts(&c);
        sorted.sort();
        let before = sorted.len();
        sorted.dedup();
        assert_eq!(sorted.len(), before, "duplicate candidates: {sorted:?}");
    }

    #[test]
    fn a_leading_separator_is_stripped_from_the_prefixed_spelling() {
        // The real corpus spells all 22 SpeedTree models this way, and the double
        // separator it used to produce ("trees//Tree...") resolves nowhere.
        let c = candidates("\\TreeSugarMapleYoungSU.spt");
        assert!(contains(&c, "trees/TreeSugarMapleYoungSU.spt"));
        assert!(!as_texts(&c).iter().any(|s| s.contains("//")));
        assert_candidate(&c, 2, b"trees/TreeSugarMapleYoungSU.spt");
    }

    #[test]
    fn a_separator_only_path_keeps_its_original_spelling() {
        let c = candidates("\\");
        assert_eq!(as_texts(&c), vec!["/"]);
    }

    #[test]
    fn a_separator_only_path_yields_no_empty_candidate() {
        let c = candidates("\\");
        assert!(!c.iter().any(|candidate| candidate.is_empty()));
    }

    #[test]
    fn high_bytes_are_left_alone() {
        // std::tolower in the C locale only folds ASCII, so a byte above 0x7F must
        // not change case and must not change the shape of the result.
        let c = build_asset_path_candidates(b"\xC3\x84sset/Thing.nif");
        assert_candidate(&c, 0, b"\xC3\x84sset/Thing.nif");
        assert_candidate(&c, 1, b"meshes/\xC3\x84sset/Thing.nif");
    }
}
