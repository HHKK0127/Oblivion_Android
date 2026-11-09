//! The `plugins.txt` grammar: reading the launcher's load-order file and putting
//! its entries into retail load order.
//!
//! Names are handled as bytes and never decoded as UTF-8, for the same reason as
//! in [`crate::formid_index`]: a plugin file name on disk is a byte string, and
//! the engine's own comparison never decodes it.
//!
//! File grammar (tolerant, because several tools write this file):
//!   - one plugin filename per line
//!   - a line whose first non-blank character is `#` is a comment
//!   - a leading `*` marks the plugin as active; the marker is stripped
//!   - leading/trailing blanks, CR and a UTF-8 BOM are ignored
//!   - blank lines are skipped
//!   - the first occurrence of a filename wins; later duplicates are dropped
//!
//! The `*` marker is only meaningful when the file uses it. A plain Oblivion
//! `plugins.txt` lists active plugins without any marker, so a file with no
//! marked line is read as "every listed plugin is active".

use std::cmp::Ordering;
use std::collections::HashSet;

use crate::formid_index::{plugin_name_equals, BASE_PLUGIN_FILE_NAME};

/// One entry of a `plugins.txt` file, located inside the buffer it was parsed
/// from: `contents[name_start..name_start + name_len]` is the file name with the
/// marker and the surrounding blanks removed.
///
/// The entry keeps an offset rather than a copy, so parsing allocates only the
/// entry vector and never a name buffer. That is what lets the C ABI hand the
/// caller offsets into the `contents` string it already holds.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PluginListEntry {
    /// Offset of the name inside the parsed buffer.
    pub name_start: usize,
    /// Length of the name in bytes.
    pub name_len: usize,
    /// False only when the file marks lines and this one is unmarked.
    pub active: bool,
}

/// An entry's name and activity, the shape the entry-oriented helpers want.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct NamedPluginEntry<'a> {
    /// The file name, without the marker and without surrounding blanks.
    pub name: &'a [u8],
    /// False only when the file marks lines and this one is unmarked.
    pub active: bool,
}

/// The name `entry` points at inside `contents`.
///
/// # Panics
/// If `entry` did not come from parsing `contents`.
pub fn entry_name<'a>(contents: &'a [u8], entry: &PluginListEntry) -> &'a [u8] {
    &contents[entry.name_start..entry.name_start + entry.name_len]
}

/// True when the name ends in `.esm`, compared without regard to ASCII case.
///
/// Only the final four bytes are examined, so `"esm"` is rejected for being
/// shorter than the extension and `"Mod.esm.bak"` is rejected for ending in
/// something else.
pub fn is_master_file_name(name: impl AsRef<[u8]>) -> bool {
    let name = name.as_ref();
    name.len() >= 4 && name[name.len() - 4..].eq_ignore_ascii_case(b".esm")
}

/// The retail load-order group of a name: `Oblivion.esm` is 0, any other master
/// is 1 and everything else is 2. A lower rank sorts first.
pub fn plugin_load_order_rank(name: impl AsRef<[u8]>) -> u8 {
    let name = name.as_ref();
    if plugin_name_equals(name, BASE_PLUGIN_FILE_NAME) {
        0
    } else if is_master_file_name(name) {
        1
    } else {
        2
    }
}

/// Parses the body of a `plugins.txt` file, returning the entries in file order.
pub fn parse_plugin_list(contents: &[u8]) -> Vec<PluginListEntry> {
    struct RawLine {
        name_start: usize,
        name_len: usize,
        marked: bool,
    }

    let mut lines: Vec<RawLine> = Vec::new();
    let mut seen: HashSet<Vec<u8>> = HashSet::new();
    let mut file_uses_markers = false;

    for (start, end) in split_line_ranges(contents) {
        let mut line = trim_range(contents, start, end);
        if line.0 >= line.1 || contents[line.0] == b'#' {
            continue;
        }

        let mut marked = false;
        if contents[line.0] == b'*' {
            marked = true;
            file_uses_markers = true;
            line = trim_range(contents, line.0 + 1, line.1);
        }
        if line.0 >= line.1 {
            continue;
        }

        // Filenames are case-insensitive on the platforms Oblivion ships for, so
        // a repeated plugin spelled differently is still a duplicate.
        let name = &contents[line.0..line.1];
        if !seen.insert(name.to_ascii_lowercase()) {
            continue;
        }

        lines.push(RawLine {
            name_start: line.0,
            name_len: line.1 - line.0,
            marked,
        });
    }

    lines
        .iter()
        .map(|line| PluginListEntry {
            name_start: line.name_start,
            name_len: line.name_len,
            // Without any marker in the file every listed plugin is active,
            // which is how the retail launcher writes it.
            active: if file_uses_markers { line.marked } else { true },
        })
        .collect()
}

/// Serialises entries back to `plugins.txt` form, one name per line, prefixing
/// active entries with `*`.
///
/// Entries with an empty name are skipped, the result always ends with a
/// trailing newline, and an empty selection yields an empty string.
pub fn serialize_plugin_list(entries: &[NamedPluginEntry<'_>]) -> Vec<u8> {
    let mut out = Vec::new();
    for entry in entries {
        if entry.name.is_empty() {
            continue;
        }
        if entry.active {
            out.push(b'*');
        }
        out.extend_from_slice(entry.name);
        out.push(b'\n');
    }
    out
}

/// Indices of the active entries that have a name, in the order they were given.
pub fn active_plugin_indices(entries: &[NamedPluginEntry<'_>]) -> Vec<usize> {
    entries
        .iter()
        .enumerate()
        .filter(|(_, entry)| entry.active && !entry.name.is_empty())
        .map(|(index, _)| index)
        .collect()
}

/// The retail load order of `names` as a permutation: indices into `names` with
/// `Oblivion.esm` first, then the other masters, then the plugins, each group
/// compared case-insensitively by name.
///
/// The permutation is returned rather than a sorted copy so a caller can apply
/// it to entries that carry more than a name. The sort is stable, so entries
/// that tie keep the order they were given in.
pub fn sort_plugin_load_order(names: &[&[u8]]) -> Vec<usize> {
    let mut order: Vec<usize> = (0..names.len()).collect();
    order.sort_by(|&lhs, &rhs| {
        plugin_load_order_rank(names[lhs])
            .cmp(&plugin_load_order_rank(names[rhs]))
            .then_with(|| compare_folded(names[lhs], names[rhs]))
    });
    order
}

/// Compares two names the way `std::string` compares them once folded to lower
/// case: byte by byte, with a prefix sorting before the longer name.
///
/// Folding inline rather than lowering both names into temporary strings keeps a
/// sort allocation-free, and folding only ASCII keeps the result independent of
/// the process locale.
fn compare_folded(lhs: &[u8], rhs: &[u8]) -> Ordering {
    for (a, b) in lhs.iter().zip(rhs) {
        let (a, b) = (a.to_ascii_lowercase(), b.to_ascii_lowercase());
        if a != b {
            return a.cmp(&b);
        }
    }
    lhs.len().cmp(&rhs.len())
}

/// The `(start, end)` of every line, splitting on `\n` only.
///
/// A trailing `\n` does not produce an empty last line, which is how the engine
/// split the file before this code moved here. The line terminator of a CRLF
/// pair stays inside the range and [`trim_range`] drops it.
fn split_line_ranges(contents: &[u8]) -> Vec<(usize, usize)> {
    let mut ranges = Vec::new();
    let mut start = 0;
    for (index, &byte) in contents.iter().enumerate() {
        if byte == b'\n' {
            ranges.push((start, index));
            start = index + 1;
        }
    }
    if start < contents.len() {
        ranges.push((start, contents.len()));
    }
    ranges
}

fn is_blank(byte: u8) -> bool {
    matches!(byte, b' ' | b'\t' | b'\r' | b'\n')
}

/// Removes leading and trailing blanks from `contents[start..end]`, and also
/// drops a UTF-8 BOM a Windows editor may have left at the start of the file.
///
/// The BOM is only dropped at the start of the range, so a BOM-only first line
/// leaves nothing behind while a BOM later in the file is just another name
/// byte, exactly as before.
fn trim_range(contents: &[u8], mut start: usize, mut end: usize) -> (usize, usize) {
    if end - start >= 3
        && contents[start] == 0xEF
        && contents[start + 1] == 0xBB
        && contents[start + 2] == 0xBF
    {
        start += 3;
    }
    while start < end && is_blank(contents[start]) {
        start += 1;
    }
    while end > start && is_blank(contents[end - 1]) {
        end -= 1;
    }
    (start, end)
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Asserts the names of `entries`, read out of `contents`, in order.
    fn assert_names(contents: &[u8], entries: &[PluginListEntry], expected: &[&str]) {
        let actual: Vec<Vec<u8>> = entries
            .iter()
            .map(|entry| entry_name(contents, entry).to_vec())
            .collect();
        let expected: Vec<Vec<u8>> = expected
            .iter()
            .map(|name| name.as_bytes().to_vec())
            .collect();
        assert_eq!(actual, expected);
    }

    fn named<'a>(names: &[&'a [u8]], active: &[bool]) -> Vec<NamedPluginEntry<'a>> {
        names
            .iter()
            .enumerate()
            .map(|(index, &name)| NamedPluginEntry {
                name,
                active: active.get(index).copied().unwrap_or(true),
            })
            .collect()
    }

    /// Serialises names and flags, and asserts the exact bytes of the result.
    fn assert_serialized(names: &[&[u8]], active: &[bool], expected: &[u8]) {
        assert_eq!(serialize_plugin_list(&named(names, active)), expected);
    }

    /// Sorts names and returns them in the resulting order.
    fn sorted<'a>(names: &[&'a [u8]]) -> Vec<&'a [u8]> {
        sort_plugin_load_order(names)
            .into_iter()
            .map(|index| names[index])
            .collect()
    }

    fn flat(names: &[&[u8]]) -> Vec<Vec<u8>> {
        names.iter().map(|name| name.to_vec()).collect()
    }

    fn parse(text: &str) -> (Vec<u8>, Vec<PluginListEntry>) {
        let contents = text.as_bytes().to_vec();
        let entries = parse_plugin_list(&contents);
        (contents, entries)
    }

    fn active_names(contents: &[u8], entries: &[PluginListEntry]) -> Vec<Vec<u8>> {
        let flags: Vec<bool> = entries.iter().map(|entry| entry.active).collect();
        let names: Vec<&[u8]> = entries
            .iter()
            .map(|entry| entry_name(contents, entry))
            .collect();
        active_plugin_indices(&named(&names, &flags))
            .into_iter()
            .map(|index| names[index].to_vec())
            .collect()
    }

    // ---------------------------------------------------------------- parsing

    #[test]
    fn plain_list_keeps_every_plugin_active() {
        let (contents, entries) = parse("Oblivion.esm\nUnofficial Oblivion Patch.esp\n");
        assert_eq!(entries.len(), 2);
        assert!(entries[0].active && entries[1].active);
        assert_names(
            &contents,
            &entries,
            &["Oblivion.esm", "Unofficial Oblivion Patch.esp"],
        );
    }

    #[test]
    fn marked_list_disables_unmarked_lines() {
        let (_, entries) = parse("*Oblivion.esm\nSomeMod.esp\n*OtherMod.esp\n");
        assert_eq!(entries.len(), 3);
        assert!(entries[0].active);
        assert!(!entries[1].active);
        assert!(entries[2].active);
    }

    #[test]
    fn comments_and_blank_lines_are_skipped() {
        let (contents, entries) = parse("# written by a mod manager\n\n   \n\t\n*Oblivion.esm\n");
        assert_names(&contents, &entries, &["Oblivion.esm"]);
    }

    #[test]
    fn carriage_returns_are_trimmed() {
        let (contents, entries) = parse("*Oblivion.esm\r\n*Mod.esp\r\n");
        assert_names(&contents, &entries, &["Oblivion.esm", "Mod.esp"]);
    }

    #[test]
    fn utf8_bom_is_trimmed() {
        let (contents, entries) = parse("\u{feff}Oblivion.esm\n");
        assert_names(&contents, &entries, &["Oblivion.esm"]);
    }

    #[test]
    fn marker_and_surrounding_blanks_are_trimmed() {
        let (contents, entries) = parse("  *  Mod.esp  \n");
        assert_names(&contents, &entries, &["Mod.esp"]);
        assert!(entries[0].active);
    }

    #[test]
    fn duplicate_names_are_dropped_case_insensitively() {
        let (contents, entries) = parse("Mod.esp\nMOD.ESP\nMod.Esp\n");
        assert_names(&contents, &entries, &["Mod.esp"]);
    }

    #[test]
    fn a_bare_marker_is_not_a_plugin() {
        let (_, entries) = parse("*\n   \n");
        assert!(entries.is_empty());
    }

    #[test]
    fn empty_input_yields_nothing() {
        let (_, entries) = parse("");
        assert!(entries.is_empty());
    }

    #[test]
    fn comment_only_file_yields_nothing() {
        let (_, entries) = parse("# only a comment\n");
        assert!(entries.is_empty());
    }

    #[test]
    fn a_line_without_a_trailing_newline_is_parsed() {
        let (contents, entries) = parse("Oblivion.esm");
        assert_names(&contents, &entries, &["Oblivion.esm"]);
    }

    #[test]
    fn entries_locate_their_names_inside_the_buffer() {
        // BOM (3) + "# comment" (9) + CRLF (2) + "  *  " (5) puts the name at 19.
        let (contents, entries) = parse("\u{feff}# comment\r\n  *  Mod.esp  \r\n");
        assert_eq!(entries.len(), 1);
        assert_eq!(entries[0].name_start, 19);
        assert_eq!(entry_name(&contents, &entries[0]), b"Mod.esp".as_slice());
    }

    #[test]
    fn names_that_are_not_utf8_survive_a_round_trip() {
        let original = b"*M\xf8d.esp\n".to_vec();
        let entries = parse_plugin_list(&original);
        assert_eq!(entry_name(&original, &entries[0]), b"M\xf8d.esp".as_slice());
        let entry = NamedPluginEntry {
            name: entry_name(&original, &entries[0]),
            active: entries[0].active,
        };
        assert_eq!(serialize_plugin_list(&[entry]), original);
    }

    #[test]
    fn active_names_keep_their_file_order() {
        let (contents, entries) = parse("*Oblivion.esm\n*B.esp\nA.esp\n");
        assert_eq!(
            active_names(&contents, &entries),
            flat(&[b"Oblivion.esm", b"B.esp"])
        );
    }

    #[test]
    fn active_names_skip_empty_and_inactive_entries() {
        let entries = [
            NamedPluginEntry {
                name: b"".as_slice(),
                active: true,
            },
            NamedPluginEntry {
                name: b"A.esp".as_slice(),
                active: false,
            },
            NamedPluginEntry {
                name: b"B.esp".as_slice(),
                active: true,
            },
        ];
        assert_eq!(active_plugin_indices(&entries), [2]);
    }

    // ------------------------------------------------------------ serializing

    #[test]
    fn active_entries_are_written_with_a_marker() {
        assert_serialized(
            &[b"Oblivion.esm", b"Mod.esp"],
            &[true, false],
            b"*Oblivion.esm\nMod.esp\n",
        );
    }

    #[test]
    fn an_empty_selection_serialises_to_an_empty_file() {
        assert_serialized(&[], &[], b"");
    }

    #[test]
    fn every_line_of_a_full_selection_is_marked() {
        assert_serialized(
            &[b"A.esp", b"B.esp", b"C.esp"],
            &[true, true, true],
            b"*A.esp\n*B.esp\n*C.esp\n",
        );
    }

    #[test]
    fn round_trip_preserves_a_marked_list() {
        let (contents, original) = parse("*Oblivion.esm\nMod.esp\n");
        let text = serialize_plugin_list(&entries_of(&contents, &original));
        assert_eq!(text, b"*Oblivion.esm\nMod.esp\n");

        let (round_tripped_contents, round_tripped) = parse(std::str::from_utf8(&text).unwrap());
        assert_names(
            &round_tripped_contents,
            &round_tripped,
            &["Oblivion.esm", "Mod.esp"],
        );
        assert!(round_tripped[0].active);
        assert!(!round_tripped[1].active);
    }

    #[test]
    fn round_trip_preserves_a_plain_list() {
        // A file without markers means "everything listed is active", so
        // serialising it writes the markers explicitly; parsing the result
        // yields the same active set, now stated rather than implied.
        let (contents, original) = parse("Oblivion.esm\nMod.esp\n");
        let text = serialize_plugin_list(&entries_of(&contents, &original));
        assert_eq!(text, b"*Oblivion.esm\n*Mod.esp\n");

        let (_, round_tripped) = parse(std::str::from_utf8(&text).unwrap());
        assert_eq!(round_tripped.len(), 2);
        assert!(round_tripped[0].active && round_tripped[1].active);
    }

    /// Views parsed entries as the `(name, active)` pairs the helpers take.
    fn entries_of<'a>(
        contents: &'a [u8],
        entries: &[PluginListEntry],
    ) -> Vec<NamedPluginEntry<'a>> {
        entries
            .iter()
            .map(|entry| NamedPluginEntry {
                name: entry_name(contents, entry),
                active: entry.active,
            })
            .collect()
    }

    // ------------------------------------------------------------- load order

    #[test]
    fn oblivion_esm_sorts_first() {
        let order = sorted(&[
            b"Mod.esp",
            b"Oblivion.esm",
            b"AnotherMod.esp",
            b"DLCShiveringIsles.esp",
        ]);
        assert_eq!(order[0], b"Oblivion.esm".as_slice());
    }

    #[test]
    fn masters_sort_before_plugins() {
        let order = sorted(&[
            b"Mod.esp",
            b"Oblivion.esm",
            b"DLCBattlehornCastle.esp",
            b"Knights.esp",
            b"Unofficial Oblivion Patch.esp",
        ]);
        assert_eq!(
            flat(&order),
            flat(&[
                b"Oblivion.esm",
                b"DLCBattlehornCastle.esp",
                b"Knights.esp",
                b"Mod.esp",
                b"Unofficial Oblivion Patch.esp",
            ])
        );
    }

    #[test]
    fn plugins_sort_alphabetically_ignoring_case() {
        let order = sorted(&[b"zeta.esp", b"Alpha.esp", b"beta.esp"]);
        assert_eq!(
            flat(&order),
            flat(&[b"Alpha.esp", b"beta.esp", b"zeta.esp"])
        );
    }

    #[test]
    fn masters_sort_alphabetically_among_themselves() {
        let order = sorted(&[b"Knights.esp", b"Oblivion.esm", b"DLCShiveringIsles.esp"]);
        assert_eq!(
            flat(&order),
            flat(&[b"Oblivion.esm", b"DLCShiveringIsles.esp", b"Knights.esp"])
        );
    }

    #[test]
    fn sort_keeps_the_incoming_order_for_equal_names() {
        // The sort is stable, so two names that only differ in case keep the
        // order they arrived in rather than swapping unpredictably.
        assert_eq!(sort_plugin_load_order(&[b"Mod.esp", b"MOD.ESP"]), [0, 1]);
    }

    #[test]
    fn rank_groups_names_and_folding_ignores_case() {
        assert_eq!(plugin_load_order_rank(b"OBLIVION.ESM"), 0);
        assert_eq!(plugin_load_order_rank(b"Knights.esm"), 1);
        assert_eq!(plugin_load_order_rank(b"Knights.esp"), 2);
    }

    #[test]
    fn folding_orders_a_prefix_before_a_longer_name() {
        assert_eq!(compare_folded(b"Mod.esp", b"Mod.esp.bak"), Ordering::Less);
        assert_eq!(compare_folded(b"mod.esp", b"MOD.ESP"), Ordering::Equal);
    }

    #[test]
    fn is_master_file_name_accepts_esm_in_any_case() {
        assert!(is_master_file_name("Oblivion.ESM"));
        assert!(is_master_file_name("Knights.esm"));
    }

    #[test]
    fn is_master_file_name_rejects_plugins() {
        assert!(!is_master_file_name("Mod.esp"));
        assert!(!is_master_file_name("Mod.esm.bak"));
        assert!(!is_master_file_name("esm"));
    }
}
