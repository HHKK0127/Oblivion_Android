//! Oblivion ESM/ESP plugin container parser.
//!
//! Rust counterpart of `Oblivion_Android/app/src/main/cpp/assets/esm_reader.cpp`
//! phase 1 (record indexing) and of the subrecord extraction in phase 2. The
//! container is indexed from a caller-supplied buffer and record payloads are
//! decoded on demand, so a 277 MB `Oblivion.esm` can be walked without copying
//! payloads into memory.
//!
//! Layout of the on-disk format handled here:
//!
//! ```text
//! TES4 record header (20 bytes) + TES4 subrecords (HEDR, MAST+DATA, ...)
//! GRUP header (20 bytes) + nested GRUP/record elements ...
//! record header (20 bytes) + subrecords (tag 4 + size 2 + payload)
//! ```
//!
//! Compressed records are zlib streams prefixed by their decompressed size.

use std::collections::HashMap;
use std::io::Read;

use flate2::read::ZlibDecoder;
use serde::{Deserialize, Serialize};
use thiserror::Error;

/// Record flag: the payload is a zlib stream prefixed by its decompressed size.
pub const REC_FLAG_COMPRESSED: u32 = 0x0004_0000;
/// Record flag: the record is deleted and should be ignored by callers.
pub const REC_FLAG_DELETED: u32 = 0x0000_0020;

/// `tag(4) + dataSize(4) + flags(4) + formID(4) + version(2) + unknown(2)`.
pub const RECORD_HEADER_SIZE: usize = 20;
/// `tag(4) + groupSize(4) + label(4) + groupType(4) + stamp(2) + unknown(2)`.
pub const GROUP_HEADER_SIZE: usize = 20;
/// `tag(4) + size(2)`.
pub const SUBRECORD_HEADER_SIZE: usize = 6;

#[derive(Debug, Error)]
pub enum EsmError {
    #[error("truncated ESM: need {needed} bytes at offset {offset}, only {available} available")]
    Truncated {
        offset: usize,
        needed: usize,
        available: usize,
    },
    #[error("not an ESM/ESP plugin: expected a TES4 header, found {found:?}")]
    BadHeader { found: [u8; 4] },
    #[error("zlib inflate failed for record {rec_type:?} formID 0x{form_id:08X}: {source}")]
    Inflate {
        rec_type: [u8; 4],
        form_id: u32,
        source: std::io::Error,
    },
}

/// GRUP group types. The meaning of the group label depends on the type.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Serialize, Deserialize)]
#[repr(u32)]
pub enum GroupType {
    Top = 0,
    World = 1,
    Interior = 2,
    Exterior = 3,
    Cell = 4,
    Topic = 5,
    CellChild = 6,
    TopicChild = 7,
    Dial = 8,
    DialChild = 9,
    Quest = 10,
    QuestChild = 11,
    NavMesh = 12,
    Misc = 13,
}

impl GroupType {
    pub fn from_raw(raw: u32) -> Option<Self> {
        Some(match raw {
            0 => Self::Top,
            1 => Self::World,
            2 => Self::Interior,
            3 => Self::Exterior,
            4 => Self::Cell,
            5 => Self::Topic,
            6 => Self::CellChild,
            7 => Self::TopicChild,
            8 => Self::Dial,
            9 => Self::DialChild,
            10 => Self::Quest,
            11 => Self::QuestChild,
            12 => Self::NavMesh,
            13 => Self::Misc,
            _ => return None,
        })
    }

    pub fn as_raw(self) -> u32 {
        self as u32
    }

    /// The group label is the FormID of the cell that owns the group's records.
    /// LAND/PGRD/REFR/ACHR/ACRE carry no cell FormID of their own, so the label
    /// is the only way to associate them with their cell.
    pub fn carries_cell_form_id(self) -> bool {
        self == Self::CellChild
    }

    /// The group label is the FormID of the WRLD record that owns the group's
    /// records. Exterior CELL records carry no WRLD link, so this is the only
    /// way to group cells per worldspace.
    pub fn carries_worldspace_form_id(self) -> bool {
        self == Self::World
    }
}

/// Render a 4-byte tag as printable ASCII; non-printable bytes become `.`.
pub fn tag_name(tag: &[u8; 4]) -> String {
    let end = tag.iter().position(|&b| b == 0).unwrap_or(tag.len());
    tag[..end]
        .iter()
        .map(|&b| {
            if (0x20..0x7f).contains(&b) {
                b as char
            } else {
                '.'
            }
        })
        .collect()
}

/// A single subrecord within a record body.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct SubRecord {
    pub tag: [u8; 4],
    pub data: Vec<u8>,
}

impl SubRecord {
    /// Subrecord tag as printable ASCII.
    pub fn name(&self) -> String {
        tag_name(&self.tag)
    }

    pub fn is(&self, tag: &[u8; 4]) -> bool {
        &self.tag == tag
    }

    /// Read the payload as a NUL-terminated string. Oblivion stores text in
    /// Windows-1252, so the bytes are converted lossily rather than validated.
    pub fn as_zstring(&self) -> String {
        let end = self
            .data
            .iter()
            .position(|&b| b == 0)
            .unwrap_or(self.data.len());
        String::from_utf8_lossy(&self.data[..end]).into_owned()
    }

    pub fn as_u16(&self) -> Option<u16> {
        self.data
            .get(0..2)
            .and_then(|b| b.try_into().ok())
            .map(u16::from_le_bytes)
    }

    pub fn as_u32(&self) -> Option<u32> {
        self.data
            .get(0..4)
            .and_then(|b| b.try_into().ok())
            .map(u32::from_le_bytes)
    }

    pub fn as_i32(&self) -> Option<i32> {
        self.data
            .get(0..4)
            .and_then(|b| b.try_into().ok())
            .map(i32::from_le_bytes)
    }

    pub fn as_f32(&self) -> Option<f32> {
        self.data
            .get(0..4)
            .and_then(|b| b.try_into().ok())
            .map(f32::from_le_bytes)
    }

    /// Read the payload as an object reference (a FormID).
    pub fn as_form_id(&self) -> Option<u32> {
        self.as_u32()
    }

    /// Read a "magnitude"-style field. Oblivion serialises integer magnitudes as
    /// denormal floats (1 becomes 1.4e-45), so fall back to the integer
    /// interpretation whenever the float value is not sane.
    pub fn as_float_or_int(&self) -> Option<f32> {
        let f = self.as_f32()?;
        if f == 0.0 || !f.is_finite() || f.abs() < 1e-6 || f.abs() > 1e9 {
            return Some(self.as_i32()? as f32);
        }
        Some(f)
    }

    /// The payload as an MGEF editorID code. Oblivion stores EFID as a
    /// four-character effect code rather than a FormID; returns `None` when the
    /// payload is not a printable 4-byte code.
    pub fn as_efid_code(&self) -> Option<String> {
        if self.data.len() < 4 || !self.data[..4].iter().all(|&b| (0x20..0x7f).contains(&b)) {
            return None;
        }
        Some(String::from_utf8_lossy(&self.data[..4]).into_owned())
    }
}

/// A 20-byte ESM record header.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub struct RecordHeader {
    pub rec_type: [u8; 4],
    /// Size of the payload that follows the header.
    pub data_size: u32,
    pub flags: u32,
    pub form_id: u32,
    pub version: u16,
    pub unknown: u16,
}

impl RecordHeader {
    pub fn name(&self) -> String {
        tag_name(&self.rec_type)
    }

    pub fn is_compressed(&self) -> bool {
        self.flags & REC_FLAG_COMPRESSED != 0
    }

    pub fn is_deleted(&self) -> bool {
        self.flags & REC_FLAG_DELETED != 0
    }
}

/// Location of one record within the plugin buffer.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
pub struct RecordIndex {
    pub rec_type: [u8; 4],
    pub form_id: u32,
    pub flags: u32,
    /// Payload size as stored in the header; includes the zlib size prefix when
    /// the record is compressed.
    pub data_size: u32,
    /// Buffer offset of the payload, i.e. just past the 20-byte header.
    pub file_offset: u64,
    pub compressed: bool,
    /// Decompressed payload size; 0 when the record is not compressed.
    pub decomp_size: u32,
    /// FormID of the nearest enclosing cell-children group (type 6).
    pub parent_form_id: u32,
    /// FormID of the nearest enclosing world-children group (type 1).
    pub worldspace_form_id: u32,
}

impl RecordIndex {
    pub fn name(&self) -> String {
        tag_name(&self.rec_type)
    }

    pub fn is_deleted(&self) -> bool {
        self.flags & REC_FLAG_DELETED != 0
    }
}

/// Aggregate record count for one record type.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct TypeCount {
    pub rec_type: String,
    pub count: usize,
}

/// Machine-readable overview of a parsed plugin.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct PluginSummary {
    pub file_name: String,
    pub is_master: bool,
    pub header_size: u32,
    pub masters: Vec<String>,
    pub record_count: usize,
    pub group_count: usize,
    pub compressed_count: usize,
    pub type_counts: Vec<TypeCount>,
}

struct GroupFrame {
    end: usize,
    label: u32,
    group_type: u32,
}

/// A parsed plugin: the TES4 header, its subrecords and the record index.
///
/// The record index covers every element in file order. The TES4 header is
/// exposed separately through [`Plugin::header`] and
/// [`Plugin::header_subrecords`] and is not repeated in [`Plugin::records`].
#[derive(Debug)]
pub struct Plugin {
    pub file_name: String,
    pub is_master: bool,
    pub header: RecordHeader,
    pub header_subrecords: Vec<SubRecord>,
    pub masters: Vec<String>,
    pub records: Vec<RecordIndex>,
    pub group_count: usize,
    form_id_index: HashMap<u32, usize>,
}

impl Plugin {
    /// Index a plugin that is already held in memory.
    pub fn parse(file_name: &str, data: &[u8]) -> Result<Self, EsmError> {
        let header = read_record_header(data, 0)?;
        if &header.rec_type != b"TES4" {
            return Err(EsmError::BadHeader {
                found: header.rec_type,
            });
        }

        let header_subrecords = decode_payload(
            data,
            RECORD_HEADER_SIZE,
            header.data_size,
            header.is_compressed(),
            header.rec_type,
            header.form_id,
        )?;
        let masters: Vec<String> = header_subrecords
            .iter()
            .filter(|sub| sub.is(b"MAST"))
            .map(|sub| sub.as_zstring())
            .filter(|name| !name.is_empty())
            .collect();

        let mut plugin = Self {
            file_name: file_name.to_string(),
            is_master: has_master_extension(file_name),
            header,
            header_subrecords,
            masters,
            records: Vec::new(),
            group_count: 0,
            form_id_index: HashMap::new(),
        };
        plugin.index_records(data);
        Ok(plugin)
    }

    fn index_records(&mut self, data: &[u8]) {
        let mut pos = RECORD_HEADER_SIZE + self.header.data_size as usize;
        let mut stack: Vec<GroupFrame> = Vec::new();

        while pos + 4 <= data.len() {
            // Drop groups whose extent has already been walked past.
            while stack.last().is_some_and(|frame| pos >= frame.end) {
                stack.pop();
            }

            let tag: [u8; 4] = data[pos..pos + 4].try_into().unwrap();
            if &tag == b"GRUP" {
                let Some(header) = data.get(pos..pos + GROUP_HEADER_SIZE) else {
                    break;
                };
                let group_size = u32::from_le_bytes(header[4..8].try_into().unwrap()) as usize;
                let label = u32::from_le_bytes(header[8..12].try_into().unwrap());
                let group_type = u32::from_le_bytes(header[12..16].try_into().unwrap());
                let end = pos.checked_add(group_size).filter(|&end| end <= data.len());
                let Some(end) = end.filter(|_| group_size >= GROUP_HEADER_SIZE) else {
                    break;
                };
                stack.push(GroupFrame {
                    end,
                    label,
                    group_type,
                });
                self.group_count += 1;
                pos += GROUP_HEADER_SIZE;
                continue;
            }

            let Some(header) = record_header_at(data, pos) else {
                break;
            };
            let file_offset = pos + RECORD_HEADER_SIZE;
            let end = file_offset
                .checked_add(header.data_size as usize)
                .filter(|&end| end <= data.len());
            let Some(end) = end else {
                break;
            };

            let compressed = header.is_compressed();
            let decomp_size = if compressed && header.data_size >= 4 {
                u32::from_le_bytes(data[file_offset..file_offset + 4].try_into().unwrap())
            } else {
                0
            };
            let parent_form_id = stack
                .iter()
                .rev()
                .find(|frame| frame.group_type == GroupType::CellChild.as_raw())
                .map(|frame| frame.label)
                .unwrap_or(0);
            let worldspace_form_id = stack
                .iter()
                .rev()
                .find(|frame| frame.group_type == GroupType::World.as_raw())
                .map(|frame| frame.label)
                .unwrap_or(0);

            let index = RecordIndex {
                rec_type: header.rec_type,
                form_id: header.form_id,
                flags: header.flags,
                data_size: header.data_size,
                file_offset: file_offset as u64,
                compressed,
                decomp_size,
                parent_form_id,
                worldspace_form_id,
            };
            if index.form_id != 0 {
                self.form_id_index.insert(index.form_id, self.records.len());
            }
            self.records.push(index);
            pos = end;
        }
    }

    pub fn record_by_form_id(&self, form_id: u32) -> Option<&RecordIndex> {
        self.form_id_index.get(&form_id).map(|&i| &self.records[i])
    }

    /// Index entries whose 4-byte type matches, in file order.
    pub fn records_of_type<'a>(
        &'a self,
        rec_type: &'a [u8; 4],
    ) -> impl Iterator<Item = &'a RecordIndex> {
        self.records
            .iter()
            .filter(move |rec| &rec.rec_type == rec_type)
    }

    /// Decode the subrecords of one indexed record.
    pub fn decode_record(
        &self,
        data: &[u8],
        index: &RecordIndex,
    ) -> Result<Vec<SubRecord>, EsmError> {
        decode_payload(
            data,
            index.file_offset as usize,
            index.data_size,
            index.compressed,
            index.rec_type,
            index.form_id,
        )
    }

    /// Record counts per type, ordered by descending count.
    pub fn type_counts(&self) -> Vec<TypeCount> {
        let mut counts: HashMap<[u8; 4], usize> = HashMap::new();
        for rec in &self.records {
            *counts.entry(rec.rec_type).or_default() += 1;
        }
        let mut counts: Vec<TypeCount> = counts
            .into_iter()
            .map(|(rec_type, count)| TypeCount {
                rec_type: tag_name(&rec_type),
                count,
            })
            .collect();
        counts.sort_by(|a, b| {
            b.count
                .cmp(&a.count)
                .then_with(|| a.rec_type.cmp(&b.rec_type))
        });
        counts
    }

    pub fn summary(&self) -> PluginSummary {
        PluginSummary {
            file_name: self.file_name.clone(),
            is_master: self.is_master,
            header_size: self.header.data_size,
            masters: self.masters.clone(),
            record_count: self.records.len(),
            group_count: self.group_count,
            compressed_count: self.records.iter().filter(|rec| rec.compressed).count(),
            type_counts: self.type_counts(),
        }
    }
}

/// Collect the 4-byte type IDs of every record in the plugin, in file order.
/// Returns an empty vector when `data` is not a valid plugin.
pub fn record_ids(data: &[u8]) -> Vec<[u8; 4]> {
    Plugin::parse("", data)
        .map(|plugin| plugin.records.iter().map(|rec| rec.rec_type).collect())
        .unwrap_or_default()
}

/// Read the 20-byte record header at `offset`.
pub fn read_record_header(data: &[u8], offset: usize) -> Result<RecordHeader, EsmError> {
    record_header_at(data, offset).ok_or_else(|| EsmError::Truncated {
        offset,
        needed: RECORD_HEADER_SIZE,
        available: data.len().saturating_sub(offset),
    })
}

/// Split a record body into subrecords, stopping at the first truncated entry.
pub fn parse_subrecords(buf: &[u8]) -> Vec<SubRecord> {
    let mut subrecords = Vec::new();
    let mut offset = 0usize;
    while offset + SUBRECORD_HEADER_SIZE <= buf.len() {
        let tag: [u8; 4] = buf[offset..offset + 4].try_into().unwrap();
        let size = u16::from_le_bytes(buf[offset + 4..offset + 6].try_into().unwrap()) as usize;
        offset += SUBRECORD_HEADER_SIZE;
        let Some(payload) = buf.get(offset..offset + size) else {
            break;
        };
        subrecords.push(SubRecord {
            tag,
            data: payload.to_vec(),
        });
        offset += size;
    }
    subrecords
}

/// Decompress (when needed) and split a record body into subrecords.
pub fn decode_payload(
    data: &[u8],
    file_offset: usize,
    data_size: u32,
    compressed: bool,
    rec_type: [u8; 4],
    form_id: u32,
) -> Result<Vec<SubRecord>, EsmError> {
    let truncated = EsmError::Truncated {
        offset: file_offset,
        needed: data_size as usize,
        available: data.len().saturating_sub(file_offset),
    };
    let payload = file_offset
        .checked_add(data_size as usize)
        .and_then(|end| data.get(file_offset..end))
        .ok_or(truncated)?;
    if !compressed {
        return Ok(parse_subrecords(payload));
    }

    // Compressed bodies start with the decompressed size, then a zlib stream.
    let Some(body) = payload.get(4..) else {
        return Err(EsmError::Truncated {
            offset: file_offset,
            needed: 4,
            available: payload.len(),
        });
    };
    let mut inflated =
        Vec::with_capacity(u32::from_le_bytes(payload[..4].try_into().unwrap()) as usize);
    let mut decoder = ZlibDecoder::new(body);
    decoder
        .read_to_end(&mut inflated)
        .map_err(|source| EsmError::Inflate {
            rec_type,
            form_id,
            source,
        })?;
    Ok(parse_subrecords(&inflated))
}

fn record_header_at(data: &[u8], offset: usize) -> Option<RecordHeader> {
    let header = data.get(offset..offset + RECORD_HEADER_SIZE)?;
    Some(RecordHeader {
        rec_type: header[0..4].try_into().ok()?,
        data_size: u32::from_le_bytes(header[4..8].try_into().ok()?),
        flags: u32::from_le_bytes(header[8..12].try_into().ok()?),
        form_id: u32::from_le_bytes(header[12..16].try_into().ok()?),
        version: u16::from_le_bytes(header[16..18].try_into().ok()?),
        unknown: u16::from_le_bytes(header[18..20].try_into().ok()?),
    })
}

fn has_master_extension(file_name: &str) -> bool {
    std::path::Path::new(file_name)
        .extension()
        .is_some_and(|ext| ext.eq_ignore_ascii_case("esm"))
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;

    fn sub(tag: &[u8; 4], data: &[u8]) -> Vec<u8> {
        let mut out = Vec::new();
        out.extend_from_slice(tag);
        out.extend_from_slice(&(data.len() as u16).to_le_bytes());
        out.extend_from_slice(data);
        out
    }

    fn record(tag: &[u8; 4], form_id: u32, flags: u32, payload: &[u8]) -> Vec<u8> {
        let mut out = Vec::new();
        out.extend_from_slice(tag);
        out.extend_from_slice(&(payload.len() as u32).to_le_bytes());
        out.extend_from_slice(&flags.to_le_bytes());
        out.extend_from_slice(&form_id.to_le_bytes());
        out.extend_from_slice(&1u16.to_le_bytes());
        out.extend_from_slice(&0u16.to_le_bytes());
        out.extend_from_slice(payload);
        out
    }

    fn grup(label: u32, group_type: GroupType, content: &[u8]) -> Vec<u8> {
        let mut out = Vec::new();
        out.extend_from_slice(b"GRUP");
        out.extend_from_slice(&((GROUP_HEADER_SIZE + content.len()) as u32).to_le_bytes());
        out.extend_from_slice(&label.to_le_bytes());
        out.extend_from_slice(&group_type.as_raw().to_le_bytes());
        out.extend_from_slice(&0u16.to_le_bytes());
        out.extend_from_slice(&0u16.to_le_bytes());
        out.extend_from_slice(content);
        out
    }

    fn deflate(payload: &[u8]) -> Vec<u8> {
        let mut encoder = flate2::write::ZlibEncoder::new(Vec::new(), flate2::Compression::fast());
        encoder.write_all(payload).unwrap();
        encoder.finish().unwrap()
    }

    fn tes4(masters: &[&str]) -> Vec<u8> {
        let mut payload = sub(b"HEDR", &[0u8; 12]);
        for master in masters {
            let mut name = master.as_bytes().to_vec();
            name.push(0);
            payload.extend_from_slice(&sub(b"MAST", &name));
            payload.extend_from_slice(&sub(b"DATA", &[0u8; 8]));
        }
        record(b"TES4", 0, 0, &payload)
    }

    #[test]
    fn empty_file_yields_no_records() {
        assert!(record_ids(&[]).is_empty());
    }

    #[test]
    fn parse_subrecords_reads_tag_and_payload() {
        let mut buf = sub(b"EDID", b"TestCell\0");
        buf.extend_from_slice(&sub(b"DATA", &1u32.to_le_bytes()));
        let subs = parse_subrecords(&buf);
        assert_eq!(subs.len(), 2);
        assert_eq!(subs[0].name(), "EDID");
        assert_eq!(subs[0].as_zstring(), "TestCell");
        assert_eq!(subs[1].as_u32(), Some(1));
    }

    #[test]
    fn parse_subrecords_stops_on_truncated_payload() {
        let mut buf = sub(b"FULL", b"ok\0");
        buf.extend_from_slice(b"XXXX");
        buf.extend_from_slice(&16u16.to_le_bytes());
        buf.extend_from_slice(b"short");
        let subs = parse_subrecords(&buf);
        assert_eq!(subs.len(), 1);
        assert_eq!(subs[0].as_zstring(), "ok");
    }

    #[test]
    fn plugin_parse_reads_header_and_masters() {
        let data = tes4(&["Oblivion.esm", "Knights.esp"]);
        let plugin = Plugin::parse("Test.esp", &data).unwrap();
        assert_eq!(plugin.header.rec_type, *b"TES4");
        assert!(!plugin.is_master);
        assert_eq!(plugin.masters, vec!["Oblivion.esm", "Knights.esp"]);
        assert!(plugin.records.is_empty());
        assert_eq!(plugin.header_subrecords.len(), 5);
    }

    #[test]
    fn master_detection_uses_extension_case_insensitively() {
        assert!(has_master_extension("Oblivion.ESM"));
        assert!(!has_master_extension("Knights.ESP"));
        assert!(!has_master_extension("no-extension"));
    }

    #[test]
    fn plugin_parse_attributes_cell_and_worldspace_groups() {
        let cell = record(b"CELL", 0x0001_2345, 0, &sub(b"EDID", b"TestCell\0"));
        let reference = record(b"REFR", 0x0002_0000, 0, &sub(b"NAME", &[0u8; 4]));
        let landscape = record(b"LAND", 0x0003_0000, 0, &sub(b"VHGT", &[0u8; 4]));
        let world_cell = record(b"CELL", 0x0004_0000, 0, &sub(b"DATA", &[0u8; 1]));

        let mut top = grup(0, GroupType::Top, &cell);
        top.extend_from_slice(&grup(
            0x0000_003C,
            GroupType::World,
            &grup(0, GroupType::Exterior, &world_cell),
        ));
        top.extend_from_slice(&grup(
            0x0001_2345,
            GroupType::CellChild,
            &[reference.clone(), landscape.clone()].concat(),
        ));

        let mut data = tes4(&["Oblivion.esm"]);
        data.extend_from_slice(&top);

        let plugin = Plugin::parse("Oblivion.esm", &data).unwrap();
        assert!(plugin.is_master);
        assert_eq!(plugin.group_count, 4);
        assert_eq!(plugin.records.len(), 4);

        let refr = plugin.record_by_form_id(0x0002_0000).unwrap();
        assert_eq!(refr.rec_type, *b"REFR");
        assert_eq!(refr.parent_form_id, 0x0001_2345);
        assert_eq!(refr.worldspace_form_id, 0);

        let land = plugin.records_of_type(b"LAND").next().unwrap();
        assert_eq!(land.parent_form_id, 0x0001_2345);

        // The cell in the Top group sits outside any cell-children group, while
        // the exterior cell inherits the worldspace from the type 1 group.
        let cells: Vec<_> = plugin.records_of_type(b"CELL").collect();
        assert_eq!(cells.len(), 2);
        assert_eq!(cells[0].parent_form_id, 0);
        assert_eq!(cells[0].worldspace_form_id, 0);
        assert_eq!(cells[1].parent_form_id, 0);
        assert_eq!(cells[1].worldspace_form_id, 0x0000_003C);

        assert_eq!(plugin.type_counts()[0].rec_type, "CELL");
        assert_eq!(plugin.summary().record_count, 4);
        assert_eq!(record_ids(&data).len(), 4);
    }

    #[test]
    fn compressed_record_round_trips() {
        let mut body = sub(b"EDID", b"Compressed\0");
        body.extend_from_slice(&sub(b"DATA", &[0xAB; 32]));
        let stream = deflate(&body);

        let mut payload = (body.len() as u32).to_le_bytes().to_vec();
        payload.extend_from_slice(&stream);

        let mut data = tes4(&["Oblivion.esm"]);
        data.extend_from_slice(&grup(
            0,
            GroupType::Top,
            &record(b"CELL", 0x0000_0100, REC_FLAG_COMPRESSED, &payload),
        ));

        let plugin = Plugin::parse("Oblivion.esm", &data).unwrap();
        let index = plugin.record_by_form_id(0x0000_0100).unwrap();
        assert!(index.compressed);
        assert_eq!(index.decomp_size, body.len() as u32);

        let subs = plugin.decode_record(&data, index).unwrap();
        assert_eq!(subs.len(), 2);
        assert_eq!(subs[0].as_zstring(), "Compressed");
        assert_eq!(subs[1].data, vec![0xAB; 32]);
        assert_eq!(plugin.summary().compressed_count, 1);

        // A non-compressed record decodes straight from the buffer.
        let mut plain = tes4(&["Oblivion.esm"]);
        plain.extend_from_slice(&grup(
            0,
            GroupType::Top,
            &record(b"WEAP", 0x0000_0200, 0, &sub(b"FULL", b"IronSword\0")),
        ));
        let plugin = Plugin::parse("Oblivion.esm", &plain).unwrap();
        let index = plugin.record_by_form_id(0x0000_0200).unwrap();
        assert!(!index.compressed);
        assert_eq!(
            plugin.decode_record(&plain, index).unwrap()[0].as_zstring(),
            "IronSword"
        );
    }

    #[test]
    fn record_body_beyond_buffer_is_not_indexed() {
        let mut data = tes4(&["Oblivion.esm"]);
        data.extend_from_slice(&record(b"CELL", 0x0000_0300, 0, &[0u8; 16]));
        let truncated = &data[..data.len() - 8];
        let plugin = Plugin::parse("Oblivion.esm", truncated).unwrap();
        assert_eq!(plugin.records.len(), 0);
    }

    #[test]
    fn rejects_non_plugin_data() {
        let data = record(b"NOPE", 0, 0, b"");
        assert!(matches!(
            Plugin::parse("bad.esm", &data),
            Err(EsmError::BadHeader { found }) if &found == b"NOPE"
        ));
        assert!(matches!(
            Plugin::parse("bad.esm", &[0u8; 4]),
            Err(EsmError::Truncated { .. })
        ));
    }

    #[test]
    fn subrecord_accessors_handle_short_payloads() {
        let short = SubRecord {
            tag: *b"XXXX",
            data: vec![1, 2],
        };
        assert_eq!(short.as_u32(), None);
        assert_eq!(short.as_f32(), None);
        assert_eq!(short.as_u16(), Some(0x0201));
        assert_eq!(short.as_zstring().len(), 2);
        assert_eq!(short.as_efid_code(), None);

        let efid = SubRecord {
            tag: *b"EFID",
            data: b"DISP".to_vec(),
        };
        assert_eq!(efid.as_efid_code().as_deref(), Some("DISP"));

        // Integer magnitudes are stored as denormal floats and must be read as
        // integers, while genuine floats pass through unchanged.
        let magnitude = SubRecord {
            tag: *b"DATA",
            data: 5i32.to_le_bytes().to_vec(),
        };
        assert_eq!(magnitude.as_float_or_int(), Some(5.0));
        let real = SubRecord {
            tag: *b"DATA",
            data: 1.5f32.to_le_bytes().to_vec(),
        };
        assert_eq!(real.as_float_or_int(), Some(1.5));
    }

    #[test]
    fn tag_name_and_group_types_are_stable() {
        assert_eq!(tag_name(b"DATA"), "DATA");
        assert_eq!(tag_name(b"AB\0\0"), "AB");
        assert_eq!(tag_name(&[0x01, 0x02, 0x03, 0x04]), "....");
        assert_eq!(GroupType::from_raw(6), Some(GroupType::CellChild));
        assert_eq!(GroupType::from_raw(99), None);
        assert!(GroupType::CellChild.carries_cell_form_id());
        assert!(GroupType::World.carries_worldspace_form_id());
    }

    /// Index the retail plugin when it is installed. Set `OBLIVION_ESM` to
    /// override the path; the test is skipped when the file is unavailable.
    #[test]
    fn real_oblivion_esm_indexes_records() {
        let path = std::env::var("OBLIVION_ESM").unwrap_or_else(|_| {
            r"C:\Program Files (x86)\Steam\steamapps\common\Oblivion\Data\Oblivion.esm".to_string()
        });
        let Ok(data) = std::fs::read(&path) else {
            eprintln!("skip: {path} unavailable");
            return;
        };

        let plugin = Plugin::parse("Oblivion.esm", &data).expect("index Oblivion.esm");
        assert_eq!(plugin.header.rec_type, *b"TES4");
        assert!(plugin.is_master);
        assert!(plugin.header_subrecords.iter().any(|sub| sub.is(b"HEDR")));
        assert!(
            plugin.records.len() > 300_000,
            "records={}",
            plugin.records.len()
        );
        assert!(plugin.group_count > 20_000, "groups={}", plugin.group_count);
        assert!(plugin.summary().compressed_count > 0);

        let cells = plugin.records_of_type(b"CELL").count();
        assert!(cells > 5_000, "cells={cells}");
        let npcs = plugin.records_of_type(b"NPC_").count();
        assert!(npcs > 300, "npcs={npcs}");

        // Exterior cells inherit their worldspace from the enclosing type 1
        // group, and cell children inherit their cell from the type 6 group.
        assert!(plugin.records.iter().any(|rec| rec.worldspace_form_id != 0));
        assert!(plugin.records.iter().any(|rec| rec.parent_form_id != 0));

        let with_data = plugin
            .records
            .iter()
            .find(|rec| rec.compressed && rec.decomp_size > 64)
            .expect("a compressed record");
        let subs = plugin.decode_record(&data, with_data).expect("inflate");
        assert!(!subs.is_empty());

        let counts = plugin.type_counts();
        for name in ["WEAP", "QUST", "MGEF", "SCPT"] {
            assert!(counts.iter().any(|t| t.rec_type == name), "missing {name}");
        }
    }
}
