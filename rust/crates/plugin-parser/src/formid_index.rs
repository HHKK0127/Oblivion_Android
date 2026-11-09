//! FormID translation between a plugin's own index space and load-order space.
//!
//! [`build_form_id_index_map`] captures the translation for one plugin and
//! [`rebase_form_id`] applies it. Names are compared as bytes rather than as
//! UTF-8 text, because a plugin file name on disk is a byte string and the
//! engine's own comparison never decodes it.

/// Index recorded for a MAST entry that is not in the loaded file list.
/// [`rebase_form_id`] leaves FormIDs that point at such an entry untouched.
pub const UNRESOLVED_FORMID_INDEX: u32 = 0xFFFF_FFFF;

/// The base game file. It is loaded first and always holds load-order index 0,
/// so its own FormIDs are already in load-order space and its map is the
/// identity.
pub const BASE_PLUGIN_FILE_NAME: &str = "Oblivion.esm";

/// Compares two plugin file names the way the engine matches a MAST entry
/// against the loaded file list: same length, then byte for byte with ASCII
/// letters folded.
///
/// Folding only ASCII keeps the result independent of the process locale and of
/// the ABI, and leaves a multi-byte UTF-8 name compared exactly.
///
/// ```
/// assert!(plugin_parser::plugin_name_equals("Oblivion.esm", "OBLIVION.ESM"));
/// assert!(!plugin_parser::plugin_name_equals("Mod.esp", "Mod2.esp"));
/// ```
pub fn plugin_name_equals(lhs: impl AsRef<[u8]>, rhs: impl AsRef<[u8]>) -> bool {
    let (lhs, rhs) = (lhs.as_ref(), rhs.as_ref());
    lhs.len() == rhs.len()
        && lhs
            .iter()
            .zip(rhs)
            .all(|(a, b)| a == b || a.to_ascii_lowercase() == b.to_ascii_lowercase())
}

/// Builds the index map for a plugin whose MAST entries are `masters` and that
/// sits at `self_index` in the engine's file list.
///
/// The result is indexed by a FormID's own high byte, so it holds one entry per
/// master followed by `self_index`, and its length is always
/// `masters.len() + 1`. A master absent from `loaded_file_names` (matched with
/// [`plugin_name_equals`]) maps to [`UNRESOLVED_FORMID_INDEX`]; the first match
/// wins, so a duplicated name resolves the way a linear scan resolves it.
pub fn build_form_id_index_map<M, L>(
    masters: &[M],
    loaded_file_names: &[L],
    self_index: u32,
) -> Vec<u32>
where
    M: AsRef<[u8]>,
    L: AsRef<[u8]>,
{
    let mut index_map = Vec::with_capacity(masters.len() + 1);
    for master in masters {
        let position = loaded_file_names
            .iter()
            .position(|loaded| plugin_name_equals(master.as_ref(), loaded.as_ref()))
            .map_or(UNRESOLVED_FORMID_INDEX, |index| index as u32);
        index_map.push(position);
    }
    // A plugin's own records are addressed by the index just past its masters.
    index_map.push(self_index);
    index_map
}

/// Rewrites a FormID from the plugin's own index space into load-order space.
///
/// A null FormID, a FormID whose index is outside `index_map`, and a FormID
/// whose index maps to [`UNRESOLVED_FORMID_INDEX`] are each returned unchanged
/// rather than folded into an index the file cannot mean. Only the high index
/// byte moves; the low 24 bits are preserved exactly.
pub fn rebase_form_id(form_id: u32, index_map: &[u32]) -> u32 {
    if form_id == 0 {
        return 0;
    }
    let plugin_index = (form_id >> 24) & 0xFF;
    let Some(&load_order_index) = index_map.get(plugin_index as usize) else {
        return form_id;
    };
    if load_order_index == UNRESOLVED_FORMID_INDEX {
        return form_id;
    }
    ((load_order_index & 0xFF) << 24) | (form_id & 0x00FF_FFFF)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn plugin_names_compare_case_insensitively() {
        assert!(plugin_name_equals("Oblivion.esm", "OBLIVION.ESM"));
        assert!(plugin_name_equals("Knights.esp", "knights.ESP"));
    }

    #[test]
    fn plugin_names_of_different_length_or_spelling_differ() {
        assert!(!plugin_name_equals("Mod.esp", "Mod.esp.bak"));
        assert!(!plugin_name_equals("Mod.esp", "Mod2.esp"));
        assert!(!plugin_name_equals("", "Mod.esp"));
        assert!(plugin_name_equals("", ""));
    }

    #[test]
    fn the_base_file_maps_onto_itself() {
        let index_map = build_form_id_index_map::<&str, &str>(&[], &["Oblivion.esm"], 0);
        assert_eq!(index_map, vec![0]);
    }

    #[test]
    fn masters_resolve_to_their_load_order_position() {
        let index_map = build_form_id_index_map(
            &["Oblivion.esm", "Knights.esp"],
            &["Oblivion.esm", "Mod.esp", "Knights.esp"],
            3,
        );
        assert_eq!(index_map, vec![0, 2, 3]);
    }

    #[test]
    fn an_absent_master_is_marked_unresolved() {
        let index_map = build_form_id_index_map(&["Missing.esp"], &["Oblivion.esm"], 1);
        assert_eq!(index_map.len(), 2);
        assert_eq!(index_map[0], UNRESOLVED_FORMID_INDEX);
        assert_eq!(index_map[1], 1);
    }

    #[test]
    fn masters_match_loaded_files_case_insensitively() {
        let index_map =
            build_form_id_index_map(&["KNIGHTS.ESP"], &["Oblivion.esm", "Knights.esp"], 1);
        assert_eq!(index_map.len(), 2);
        assert_eq!(index_map[0], 1);
    }

    #[test]
    fn a_plugin_keeps_its_own_index_and_its_master_references() {
        let index_map = build_form_id_index_map(&["Oblivion.esm"], &["Oblivion.esm"], 1);
        assert_eq!(rebase_form_id(0x0100_ABCD, &index_map), 0x0100_ABCD);
        assert_eq!(rebase_form_id(0x0000_ABCD, &index_map), 0x0000_ABCD);
    }

    #[test]
    fn a_plugins_own_index_follows_its_load_position() {
        let index_map = build_form_id_index_map(&["Oblivion.esm"], &["Oblivion.esm", "A.esp"], 2);
        assert_eq!(rebase_form_id(0x0100_ABCD, &index_map), 0x0200_ABCD);
        assert_eq!(rebase_form_id(0x0000_ABCD, &index_map), 0x0000_ABCD);
    }

    #[test]
    fn a_reordered_master_is_followed_to_its_load_order_index() {
        let index_map = build_form_id_index_map(
            &["Oblivion.esm", "Knights.esp"],
            &["Oblivion.esm", "Mod.esp", "Knights.esp"],
            3,
        );
        assert_eq!(rebase_form_id(0x0100_BEEF, &index_map), 0x0200_BEEF);
        assert_eq!(rebase_form_id(0x0200_BEEF, &index_map), 0x0300_BEEF);
    }

    #[test]
    fn an_unresolved_master_leaves_its_formids_alone() {
        let index_map =
            build_form_id_index_map(&["Oblivion.esm", "Missing.esp"], &["Oblivion.esm"], 1);
        assert_eq!(rebase_form_id(0x0000_ABCD, &index_map), 0x0000_ABCD);
        assert_eq!(rebase_form_id(0x0100_ABCD, &index_map), 0x0100_ABCD);
        assert_eq!(rebase_form_id(0x0200_ABCD, &index_map), 0x0100_ABCD);
    }

    #[test]
    fn the_base_files_formids_are_not_touched() {
        let index_map = build_form_id_index_map::<&str, &str>(&[], &["Oblivion.esm"], 0);
        assert_eq!(rebase_form_id(0x0000_1234, &index_map), 0x0000_1234);
        assert_eq!(rebase_form_id(0x0000_FFFF, &index_map), 0x0000_FFFF);
    }

    #[test]
    fn an_index_outside_the_plugins_space_is_left_as_it_is() {
        let index_map = build_form_id_index_map::<&str, &str>(&[], &["Oblivion.esm"], 0);
        assert_eq!(rebase_form_id(0x0100_0001, &index_map), 0x0100_0001);
        assert_eq!(rebase_form_id(0xFF00_BEEF, &index_map), 0xFF00_BEEF);
    }

    #[test]
    fn a_null_formid_stays_null() {
        let index_map = build_form_id_index_map(&["Oblivion.esm"], &["Oblivion.esm"], 1);
        assert_eq!(rebase_form_id(0x0000_0000, &index_map), 0x0000_0000);
    }

    #[test]
    fn the_low_24_bits_survive_the_rebase() {
        let index_map = build_form_id_index_map(&["Oblivion.esm"], &["Oblivion.esm"], 3);
        assert_eq!(rebase_form_id(0x01FF_FFFF, &index_map), 0x03FF_FFFF);
        assert_eq!(rebase_form_id(0x0100_0001, &index_map), 0x0300_0001);
    }

    #[test]
    fn repeating_the_base_files_translation_is_harmless() {
        let index_map = build_form_id_index_map::<&str, &str>(&[], &["Oblivion.esm"], 0);
        let mut form_id = 0x0000_BEEF;
        form_id = rebase_form_id(form_id, &index_map);
        form_id = rebase_form_id(form_id, &index_map);
        assert_eq!(form_id, 0x0000_BEEF);
    }

    #[test]
    fn names_are_compared_as_bytes_not_as_utf8() {
        // A byte above 0x7F is never folded, so two names that differ only in
        // their high bytes stay distinct regardless of the caller's locale.
        let lower = [0x4Du8, 0xE9];
        let upper = [0x4Du8, 0xC9];
        assert!(plugin_name_equals(lower, lower));
        assert!(!plugin_name_equals(upper, lower));
    }
}
