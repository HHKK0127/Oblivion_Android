//! Block body walker.
//!
//! Retail NIF files store no per-block size table, so the only way to reach a
//! block body is to walk every preceding block in order. [`Walker`] decodes
//! each body just far enough to measure it and records the byte range of every
//! block, which is what lets the later readers seek straight to a block.
//!
//! The walk is a direct transcription of `NIFParser::walkBlockBody` and its
//! `skip*` helpers in `app/src/main/cpp/assets/nif_parser.cpp`: every branch
//! consumes exactly the bytes nif.xml describes for that block type at that
//! version. Behavioural quirks of the original are kept on purpose, because
//! files in the wild depend on them — see the comments on each helper.

use crate::header::Header;
use crate::reader::Reader;
use crate::Node;

/// Trailer guard: no shipped mesh stores more values than this.
const MAX_TRAILER_VALUES: u32 = 64;

/// Upper bound on one spline control point array, so a corrupt count cannot
/// drive an enormous skip.
const MAX_SPLINE_CONTROL_POINTS: u32 = 1 << 20;

// Gamebryo version cut-offs used by the .kf sequence records. Oblivion's own
// animations are 20.0.0.4, but the shipped archives still carry a handful of
// legacy 10.1.0.106 / 10.2.0.0 creature idle files whose sequence and
// controlled block records differ in exactly these ranges.
const VERSION_10_1_0_104: u32 = 0x0A01_0068;
const VERSION_10_1_0_106: u32 = 0x0A01_006A;
const VERSION_10_1_0_110: u32 = 0x0A01_006E;
const VERSION_10_1_0_113: u32 = 0x0A01_0071;
const VERSION_10_2_0_0: u32 = 0x0A02_0000;
const VERSION_10_4_0_1: u32 = 0x0A04_0001;
const VERSION_20_0_0_5: u32 = 0x1400_0005;
const VERSION_20_1_0_0: u32 = 0x1401_0000;

/// Everything one walk produces. Held separately from [`Header`] so the walker
/// can borrow the header, the node table and this state at the same time.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub(crate) struct WalkState {
    /// Body start of every block, per-block prefix included.
    pub(crate) offsets: Vec<usize>,
    /// One past the last body byte of every block.
    pub(crate) ends: Vec<usize>,
    /// Leading `u32` present on 10.0.1.x / 10.1.0.x meshes, 4 or 0.
    pub(crate) block_prefix: u32,
    /// True once a walk has run to the end of the trailer.
    pub(crate) walked: bool,
    /// Human-readable reason the walk stopped.
    pub(crate) error: String,
    /// Value count word of the file trailer.
    pub(crate) trailer_value_count: u32,
    /// Tool data skipped before a repeated trailer.
    pub(crate) trailing_data_size: usize,
}

/// Result of one walk: the cursor position it stopped at, plus its outcome.
pub(crate) type WalkOutcome = (usize, Result<(), String>);

/// Leading word in front of a block body, in bytes.
///
/// Every 10.0.1.x and 10.1.0.x sample carries one except the NiCollisionObject
/// subclasses, which start straight at their Target reference: floorplane01
/// measures `bhkCollisionObject` as ten bytes with the reference at the body
/// start, and four extra bytes there derail the whole walk. The 10.2+ and 20.x
/// meshes drop the prefix everywhere.
///
/// Two shape types read their own leading word, so counting it as a prefix and
/// again as body content would double it: `bhkListShape` starts at its Num Sub
/// Shapes count (measured 8 in handscythe01 and 2 in oar01, matching each
/// file's sub-shape groups) and `bhkMeshShape` at the first of its eight
/// leading words.
pub(crate) fn block_body_prefix(version: u32, block_prefix: u32, type_name: &str) -> u32 {
    if type_name == "bhkCollisionObject"
        || type_name == "bhkBlendCollisionObject"
        || type_name == "bhkSPCollisionObject"
    {
        return 0;
    }
    if version <= 0x0A00_0102 && (type_name == "bhkListShape" || type_name == "bhkMeshShape") {
        return 0;
    }
    block_prefix
}

/// First field of one block body, past its per-block prefix.
///
/// Returns `None` until the file has been walked — a walk that stopped early
/// leaves partial ranges behind, and the original reader refuses to hand any of
/// them out (`if (!blocksWalked || index >= ...) return false`).
pub(crate) fn locate_block(walk: &WalkState, header: &Header, index: u32) -> Option<usize> {
    if !walk.walked {
        return None;
    }
    let start = *walk.offsets.get(index as usize)?;
    let end = *walk.ends.get(index as usize)?;
    let type_name = header
        .block_type_indices
        .get(index as usize)
        .and_then(|type_index| header.block_type_names.get(*type_index as usize))
        .cloned()
        .unwrap_or_default();
    let offset = start + block_body_prefix(header.version, walk.block_prefix, &type_name) as usize;
    (offset <= end).then_some(offset)
}

/// Measures every block body of one file.
pub(crate) struct Walker<'a> {
    r: Reader<'a>,
    header: &'a mut Header,
    nodes: &'a mut [Node],
    state: &'a mut WalkState,
}

impl<'a> Walker<'a> {
    pub(crate) fn new(
        data: &'a [u8],
        header: &'a mut Header,
        nodes: &'a mut [Node],
        state: &'a mut WalkState,
    ) -> Self {
        Self {
            r: Reader::new(data),
            header,
            nodes,
            state,
        }
    }

    /// Walk every block body in order and record its byte range, then consume
    /// the file trailer. Returns the cursor position the walk stopped at; a
    /// failed walk leaves [`WalkState::error`] with the reason.
    pub(crate) fn run(mut self) -> WalkOutcome {
        self.state.offsets.clear();
        self.state.ends.clear();
        self.state.block_prefix = 0;
        self.state.walked = false;
        self.state.error.clear();
        self.state.trailer_value_count = 0;
        self.state.trailing_data_size = 0;

        let block_count = self.header.block_type_indices.len();
        if block_count == 0 {
            self.state.error = "no blocks in block table".to_string();
            return (0, Err(self.state.error.clone()));
        }

        // Meshes from the 10.0.1.x and 10.1.0.x lines prefix each block body
        // with a uint32 that reads as zero in every sample of the shipped
        // corpus. The 10.2+ and 20.x meshes drop the prefix.
        if (0x0A00_0100..=0x0A01_006A).contains(&self.header.version) {
            self.state.block_prefix = 4;
        }

        self.r.clear_error();
        self.state.offsets.reserve(block_count);
        self.state.ends.reserve(block_count);
        self.r.set_pos(self.header.block_data_offset);

        // Pre-5.0.0.1 meshes carry no type table, so the walk builds it from
        // the inline type name every block opens with.
        if !self.header.has_block_type_table {
            self.header.block_type_names.clear();
            self.header.block_type_indices.clear();
        }

        for index in 0..block_count {
            let body_start = self.r.pos();
            let type_name = if self.header.has_block_type_table {
                self.block_type_name(index)
            } else {
                // Every block of a pre-5.0.0.1 mesh opens with its own type name.
                match self.r.string() {
                    Ok(name) => name,
                    Err(_) => {
                        self.state.error = format!("cannot read inline type name of block {index}");
                        break;
                    }
                }
            };
            if !self.header.has_block_type_table {
                self.header.block_type_names.push(type_name.clone());
                self.header
                    .block_type_indices
                    .push(self.header.block_type_names.len() as u16 - 1);
                if let Some(node) = self.nodes.get_mut(index) {
                    node.block_type_index = *self.header.block_type_indices.last().unwrap_or(&0);
                    node.block_type_name = type_name.clone();
                }
            }

            let prefix = self.block_body_prefix(&type_name);
            if !self.r.skip(prefix as usize) {
                break;
            }
            if !self.walk_block_body(&type_name) {
                let shown = if type_name.is_empty() {
                    "unknown"
                } else {
                    type_name.as_str()
                };
                self.state.error = format!("cannot measure block {index} ({shown})");
                break;
            }
            self.state.offsets.push(body_start);
            self.state.ends.push(self.r.pos());
        }

        if self.state.offsets.len() != block_count {
            if self.state.error.is_empty() {
                self.state.error = format!(
                    "walk stopped after {} of {block_count} blocks",
                    self.state.offsets.len()
                );
            }
            self.r.set_error();
            return (self.r.pos(), Err(self.state.error.clone()));
        }

        if !self.read_trailer() {
            self.state.error = format!("unrecognised file trailer at offset {}", self.r.pos());
            self.r.set_error();
            return (self.r.pos(), Err(self.state.error.clone()));
        }

        self.state.walked = true;
        (self.r.pos(), Ok(()))
    }

    /// Type name of one block, empty when the table does not cover it.
    fn block_type_name(&self, index: usize) -> String {
        self.header
            .block_type_indices
            .get(index)
            .and_then(|type_index| self.header.block_type_names.get(*type_index as usize))
            .cloned()
            .unwrap_or_default()
    }

    /// Leading word in front of one block body, see [`block_body_prefix`].
    fn block_body_prefix(&self, type_name: &str) -> u32 {
        block_body_prefix(self.header.version, self.state.block_prefix, type_name)
    }

    /// Every shipped mesh ends with a trailer: a uint32 value count followed by
    /// that many uint32 values. 8,030 of the 8,032 corpus meshes store one zero
    /// and the two magiceffects meshes store two values. Consuming it leaves the
    /// cursor exactly on the end of the file, which is how callers validate the
    /// walk.
    ///
    /// Eight landscape LOD tiles carry tool data between the trailer and a
    /// repeated trailer at the very end. The block table never references that
    /// data, so it is measured and skipped rather than parsed.
    fn read_trailer(&mut self) -> bool {
        self.state.trailer_value_count = 0;
        self.state.trailing_data_size = 0;

        let value_count = self.r.u32();
        if self.r.has_error() || value_count > MAX_TRAILER_VALUES {
            return false;
        }
        self.state.trailer_value_count = value_count;
        if !self.r.skip(value_count as usize * 4) {
            return false;
        }
        if self.r.pos() == self.r.len() {
            return true;
        }

        // Extra data is only accepted when the file still ends with a trailer
        // that repeats the same value count, so a truncated mesh keeps failing
        // the walk.
        let remaining = self.r.len() - self.r.pos();
        if remaining < 8 {
            return false;
        }
        let tail = &self.r.data()[self.r.len() - 8..];
        let tail_count = u32::from_le_bytes([tail[0], tail[1], tail[2], tail[3]]);
        let tail_terminator = u32::from_le_bytes([tail[4], tail[5], tail[6], tail[7]]);
        if tail_count != value_count || tail_terminator != 0 {
            return false;
        }

        self.state.trailing_data_size = remaining;
        self.r.set_pos(self.r.len());
        true
    }

    // -----------------------------------------------------------------
    // Measurement primitives
    // -----------------------------------------------------------------

    fn skip(&mut self, count: usize) -> bool {
        self.r.skip(count)
    }

    fn u8(&mut self) -> u8 {
        self.r.u8()
    }

    fn u16(&mut self) -> u16 {
        self.r.u16()
    }

    fn u32(&mut self) -> u32 {
        self.r.u32()
    }

    fn failed(&self) -> bool {
        self.r.has_error()
    }

    fn version(&self) -> u32 {
        self.header.version
    }

    /// nif.xml `#BSVER#`: the header's unknown field carries the Bethesda
    /// stream version. The legacy NetImmerse files have no BS header, which is
    /// the zero nif.xml means when it says the `#BSVER#` conditions also apply
    /// to the NI streams.
    fn bs_version(&self) -> u32 {
        self.header.unknown_field
    }

    fn has_type_table(&self) -> bool {
        self.header.has_block_type_table
    }

    /// Array skips. The multiplication saturates instead of wrapping: a count
    /// large enough to overflow only occurs in corrupt files, and failing the
    /// walk is the outcome the original reader reaches as well.
    fn skip_ref_array(&mut self, count: u32) {
        self.r.skip_ref_array(count);
    }

    fn skip_ptr_array(&mut self, count: u32) {
        self.r.skip_ref_array(count);
    }

    fn skip_vector3_array(&mut self, count: u32) {
        self.r.skip_vector3_array(count);
    }

    fn skip_vector4_array(&mut self, count: u32) {
        self.skip((count as usize).saturating_mul(16));
    }

    fn skip_color4_array(&mut self, count: u32) {
        self.skip((count as usize).saturating_mul(16));
    }

    fn skip_float_array(&mut self, count: u32) {
        self.skip((count as usize).saturating_mul(4));
    }

    fn skip_u16_array(&mut self, count: u32) {
        self.skip((count as usize).saturating_mul(2));
    }

    fn skip_string_array(&mut self, count: u32) {
        self.r.skip_string_array(count);
    }

    /// nif.xml: bool is "32-bit up to and including 4.0.0.2, 8-bit from
    /// 4.1.0.1 on". The pre-5.0.0.1 meshes that this walker now reaches all sit
    /// below that cut, so their flags occupy a full uint32.
    fn read_bool_field(&mut self) -> u32 {
        if self.version() <= 0x0400_0002 {
            return self.u32();
        }
        self.u8() as u32
    }

    // -----------------------------------------------------------------
    // Shared record layouts
    // -----------------------------------------------------------------

    /// NiObjectNET: Name | Extra Data List | Controller
    fn skip_ni_object_net(&mut self) {
        let has_type_table = self.header.has_block_type_table;
        self.r.skip_ni_object_net(has_type_table);
    }

    /// NiAVObject: NiObjectNET | Flags | Translation | Rotation | Scale |
    /// Properties | Collision Object
    fn skip_ni_av_object(&mut self) {
        self.skip_ni_object_net();
        if self.failed() {
            return;
        }
        self.skip(2);
        self.skip(12);
        self.skip(36);
        self.skip(4);
        if !self.has_type_table() {
            self.skip(12); // Velocity (until 4.2.2.0)
        }
        let property_count = self.u32();
        if self.failed() {
            return;
        }
        self.skip_ref_array(property_count);
        if !self.has_type_table() {
            // Has Bounding Volume is version-sized per nif.xml, and the
            // collision object only exists from 10.0.1.0 on.
            if self.read_bool_field() != 0 {
                self.skip(16); // Bounding Volume
            }
            return;
        }
        self.skip(4);
    }

    /// NiGeometry: NiAVObject | Data | Skin Instance | Material Data
    fn skip_ni_geometry(&mut self) {
        self.skip_ni_av_object();
        if self.failed() {
            return;
        }
        self.skip(4);
        if self.version() >= 0x0303_000D {
            self.skip(4); // Skin Instance (since 3.3.0.13)
        }
        if self.version() >= 0x0A00_0100 {
            self.skip_material_data(); // Material Data (since 10.0.1.0)
        }
    }

    /// MaterialData on these meshes is Has Shader plus its optional pair.
    fn skip_material_data(&mut self) {
        let has_shader = self.u8();
        if self.failed() {
            return;
        }
        if has_shader != 0 {
            self.skip_string_array(1); // Shader Name
            self.skip(4); // Shader Extra Data
        }
    }

    /// NiTimeController: Next Controller | Flags | Frequency | Phase | Start
    /// Time | Stop Time | Target
    fn skip_ni_time_controller(&mut self) {
        self.skip(4 + 2 + 4 + 4 + 4 + 4 + 4);
    }

    /// NiInterpController adds Manager Controlled inside the
    /// 10.1.0.104..10.1.0.108 window (inclusive), so only the 10.1.0.106 meshes
    /// carry the extra byte.
    fn skip_ni_interp_controller(&mut self) {
        self.skip_ni_time_controller();
        if (0x0A01_0068..=0x0A01_006C).contains(&self.version()) {
            self.skip(1); // Manager Controlled
        }
    }

    /// NiPSysModifier: Name | Order | Target | Active
    fn skip_ni_psys_modifier(&mut self) {
        self.skip_string_array(1);
        self.skip(4 + 4 + 1);
    }

    /// NiPSysEmitter: Speed | Speed Variation | Declination | Declination
    /// Variation | Planar Angle | Planar Angle Variation | Initial Color |
    /// Initial Radius | Radius Variation | Life Span | Life Span Variation
    /// [| Emitter Object]
    ///
    /// Radius Variation only exists from 10.4.0.1 on. The Oblivion meshes carry
    /// neither it nor the Unknown QQSpeed pair of nif.xml, which is contradicted
    /// by the exact block boundaries of miscfirefly01.nif (10.2.0.0), so the
    /// payload stays 52 bytes without an emitter object and 56 bytes with one.
    fn skip_ni_psys_emitter_base(&mut self, has_emitter_object: bool) {
        self.skip(6 * 4);
        self.skip(16); // Initial Color (Color4)
        self.skip(4); // Initial Radius
        if self.version() >= 0x0A04_0001 {
            self.skip(4); // Radius Variation (since 10.4.0.1)
        }
        self.skip(4 + 4); // Life Span, Life Span Variation
        if has_emitter_object && self.version() >= 0x0A01_0000 {
            self.skip(4); // Emitter Object (since 10.1.0.0)
        }
    }

    /// NiDynamicEffect: Switch State | Num Affected Nodes | Affected Nodes
    ///
    /// The affected node list only exists from 10.1.0.0 on, and the switch state
    /// only from 10.1.0.106 on, so the 10.0.1.0 lights carry neither field.
    fn skip_ni_dynamic_effect(&mut self) {
        if self.version() < 0x0A01_0000 {
            return;
        }
        if self.version() >= 0x0A01_006A {
            self.skip(1); // Switch State (since 10.1.0.106)
        }
        let num_affected_nodes = self.u32();
        if self.failed() {
            return;
        }
        self.skip_ptr_array(num_affected_nodes);
    }

    /// NiLight: Dimmer | Ambient Color | Diffuse Color | Specular Color
    fn skip_ni_light(&mut self) {
        self.skip_ni_dynamic_effect();
        if self.failed() {
            return;
        }
        self.skip(4 + 12 + 12 + 12);
    }

    /// TexDesc: Source | Clamp Mode | Filter Mode | UV Set | PS2 L | PS2 K |
    /// Has Texture Transform plus the transform block when present.
    ///
    /// PS2 L / PS2 K only exist up to 10.4.0.1.
    fn skip_tex_desc(&mut self) {
        self.skip(4 + 4 + 4 + 4);
        if self.version() <= 0x0A04_0001 {
            self.skip(2 + 2); // PS2 L, PS2 K
        }
        if self.version() >= 0x0A01_0000 {
            let has_transform = self.u8();
            if self.failed() {
                return;
            }
            if has_transform != 0 {
                self.skip(8 + 8 + 4 + 4 + 8); // Translation, Scale, Rotation, Method, Center
            }
        }
    }

    fn skip_shader_tex_descs(&mut self, count: u32) {
        for _ in 0..count {
            if self.failed() {
                break;
            }
            let has_map = self.u8();
            if self.failed() {
                break;
            }
            if has_map != 0 {
                self.skip_tex_desc();
                self.skip(4); // Map ID
            }
        }
    }

    fn key_value_size(value_type: &str) -> usize {
        match value_type {
            "float" => 4,
            "byte" => 1,
            "Color4" => 16,
            "Vector3" => 12,
            "Quaternion" => 16,
            // "string" and anything else are length prefixed.
            _ => 0,
        }
    }

    /// Key: Time | Value [| Forward | Backward] [| TBC]. `arg` carries the
    /// interpolation of the enclosing KeyGroup: 2 adds a tangent pair, 3 a TBC
    /// triple.
    fn skip_keys(&mut self, count: u32, value_type: &str, arg: i32) {
        let is_string = value_type == "string";
        let value_bytes = if is_string {
            0
        } else {
            Self::key_value_size(value_type)
        };
        let tangent_bytes = if arg == 2 { 2 * value_bytes } else { 0 };
        let tbc_bytes = if arg == 3 { 12 } else { 0 };
        for _ in 0..count {
            if self.failed() {
                break;
            }
            self.skip(4); // Time
            if is_string {
                self.skip_string_array(1); // Value
            } else {
                self.skip(value_bytes);
            }
            if tangent_bytes > 0 {
                self.skip(tangent_bytes);
            }
            if tbc_bytes > 0 {
                self.skip(tbc_bytes);
            }
        }
    }

    /// QuatKey: Time | Value, plus TBC for rotation type 3. Rotation type 4
    /// switches the whole array over to XYZ Euler KeyGroups and reads no
    /// quaternion at all.
    fn skip_quat_keys(&mut self, count: u32, rotation_type: i32) {
        if rotation_type == 4 {
            return;
        }
        for _ in 0..count {
            if self.failed() {
                break;
            }
            self.skip(4); // Time
            self.skip(16); // Value (Quaternion)
            if rotation_type == 3 {
                self.skip(12); // TBC
            }
        }
    }

    /// KeyGroup: Num Keys | Interpolation | Keys[Num Keys]. Num Keys == 0
    /// leaves the interpolation unread, which matches the zero length array
    /// that follows.
    fn skip_key_group(&mut self, value_type: &str) {
        let num_keys = self.u32();
        if self.failed() {
            return;
        }
        let mut interpolation = 0;
        if num_keys != 0 {
            interpolation = self.u32() as i32;
            if self.failed() {
                return;
            }
        }
        self.skip_keys(num_keys, value_type, interpolation);
    }

    /// NiKeyframeData: rotation keys (quaternion or XYZ KeyGroups) |
    /// Translations | Scales.
    fn skip_keyframe_data(&mut self) {
        let num_rotation_keys = self.u32();
        if self.failed() {
            return;
        }
        let mut rotation_type = 0;
        if num_rotation_keys != 0 {
            rotation_type = self.u32() as i32;
            if self.failed() {
                return;
            }
        }
        self.skip_quat_keys(num_rotation_keys, rotation_type);
        if rotation_type == 4 {
            if self.version() <= 0x0A01_0000 {
                self.skip(4); // Order
            }
            for _ in 0..3 {
                if self.failed() {
                    break;
                }
                self.skip_key_group("float"); // XYZ Rotations
            }
        }
        self.skip_key_group("Vector3"); // Translations
        self.skip_key_group("float"); // Scales
    }

    /// NiBSplineInterpolator family. Every member opens with the same four
    /// fields and only the tail differs, so the block type name picks the tail.
    /// Oblivion's .kf files use NiBSplineCompTransformInterpolator for the human
    /// and horse skeletons and NiTransformInterpolator for the creature
    /// skeletons.
    fn skip_bspline_interpolator(&mut self, type_name: &str) {
        self.skip(4 + 4 + 4 + 4); // Start Time, Stop Time, Spline Data, Basis Data
        if type_name == "NiBSplineCompTransformInterpolator"
            || type_name == "NiBSplineTransformInterpolator"
        {
            self.skip(32); // Transform
            self.skip(4 + 4 + 4); // Translation, Rotation, Scale Handle
            if type_name == "NiBSplineCompTransformInterpolator" {
                self.skip(4 * 6); // Offset / Half Range for translation, rotation, scale
            }
        } else if type_name == "NiBSplineCompPoint3Interpolator" {
            self.skip(12 + 4); // Value, Handle
            self.skip(4 + 4); // Position Offset, Position Half Range
        } else if type_name == "NiBSplinePoint3Interpolator" {
            self.skip(12 + 4); // Value, Handle
        } else if type_name == "NiBSplineCompFloatInterpolator" {
            self.skip(4 + 4); // Value, Handle
            self.skip(4 + 4); // Float Offset, Float Half Range
        } else if type_name == "NiBSplineFloatInterpolator" {
            self.skip(4 + 4); // Value, Handle
        }
    }

    /// NiBSplineData: Num Float Control Points | Float Control Points | Num
    /// Compact Control Points | Compact Control Points. The compact control
    /// points are 16 bit values that the interpolator's offset and half range
    /// expand back to the original floats.
    fn skip_bspline_data(&mut self) {
        let num_float_control_points = self.u32();
        if self.failed() {
            return;
        }
        if num_float_control_points > MAX_SPLINE_CONTROL_POINTS {
            self.r.set_error();
            return;
        }
        self.skip_float_array(num_float_control_points);
        let num_compact_control_points = self.u32();
        if self.failed() {
            return;
        }
        if num_compact_control_points > MAX_SPLINE_CONTROL_POINTS {
            self.r.set_error();
            return;
        }
        self.skip_u16_array(num_compact_control_points);
    }

    /// InterpBlendItem: Interpolator | Weight | Normalized Weight | Priority |
    /// Ease Spinner. The priority field narrows from an int to a byte at
    /// 10.1.0.110, so the item stride is 20 bytes below that cut-off and 17 from
    /// 10.1.0.110 onwards.
    fn skip_interp_blend_items(&mut self, count: u32) {
        let stride = if self.version() <= 0x0A01_006D {
            20
        } else {
            17
        };
        self.skip((count as usize).saturating_mul(stride));
    }

    /// NiBlendInterpolator. Three record layouts ship in the shipped archives:
    /// the 10.1.0.106/107 records keep 16 bit array and count fields,
    /// 10.1.0.108/109 add the single interpolator shortcut, 10.1.0.110/111
    /// narrow those fields to bytes, and 10.1.0.112 leads with a flags byte and
    /// drops the array grow by field.
    fn skip_blend_interpolator(&mut self, value_size: usize) {
        let version = self.version();
        if version >= 0x0A01_0070 {
            // 10.1.0.112 and later
            let flags = self.u8();
            let array_size = self.u8() as u32;
            if self.failed() {
                return;
            }
            self.skip(4); // Weight Threshold
            if (flags & 1) == 0 {
                self.skip(1 + 1 + 1 + 1 + 4 + 4 + 4 + 4);
                self.skip_interp_blend_items(array_size);
            }
            self.skip(value_size); // Value
            return;
        }
        let array_size = if version >= 0x0A01_006E {
            self.u8() as u32
        } else {
            self.u16() as u32
        };
        if version <= 0x0A01_006D {
            self.skip(2); // Array Grow By
        }
        if self.failed() {
            return;
        }
        self.skip_interp_blend_items(array_size);
        self.skip(1 + 4 + 1); // Manager Controlled, Weight Threshold, Only Use Highest Weight
        if version >= 0x0A01_006E {
            // 10.1.0.110, 10.1.0.111
            self.skip(1 + 1); // Interp Count, Single Index
            self.skip(4 + 4); // Single Interpolator, Single Time
            self.skip(1 + 1); // High Priority, Next High Priority
        } else {
            self.skip(2 + 2); // Interp Count, Single Index
            if version >= 0x0A01_006C {
                // 10.1.0.108 and later
                self.skip(4 + 4); // Single Interpolator, Single Time
            }
            self.skip(4 + 4); // High Priority, Next High Priority
        }
        self.skip(value_size); // Value
    }

    /// NodeSet: Num Nodes | Nodes
    fn skip_node_set(&mut self) {
        let num_nodes = self.u32();
        if self.failed() {
            return;
        }
        self.skip_ptr_array(num_nodes);
    }

    /// AVObject: Name | AV Object
    fn skip_av_object_array(&mut self, count: u32) {
        for _ in 0..count {
            if self.failed() {
                break;
            }
            self.skip_string_array(1);
            self.skip(4);
        }
    }

    /// MatchGroup: Num Vertices | Vertex Indices
    fn skip_match_groups(&mut self, count: u32) {
        for _ in 0..count {
            if self.failed() {
                break;
            }
            let num_vertices = self.u16() as u32;
            if self.failed() {
                break;
            }
            self.skip_u16_array(num_vertices);
        }
    }

    /// FurniturePosition: Offset | Orientation | Position Ref 1 | Position Ref 2
    fn skip_furniture_positions(&mut self, count: u32) {
        self.skip((count as usize).saturating_mul(16));
    }

    // -----------------------------------------------------------------
    // Block-specific payloads
    // -----------------------------------------------------------------

    /// NiGeometryData: the shared vertex payload followed by the per-type tail.
    fn skip_ni_geometry_data(&mut self, type_name: &str) {
        // Pre-5.0.0.1 meshes predate Group ID, Keep/Compress Flags and
        // Consistency Flags, order the flags as Bound -> Vertex Colors -> Data
        // Flags -> Has UV, and store every flag as a full uint32. Verified
        // byte-exact against marker_arrow.nif (4.0.0.2) and marker_radius.nif
        // (3.3.0.13).
        if !self.has_type_table() {
            let num_vertices = self.u16() as u32;
            if self.failed() {
                return;
            }
            if self.read_bool_field() != 0 {
                self.skip_vector3_array(num_vertices); // Vertices
            }
            if self.failed() {
                return;
            }
            if self.read_bool_field() != 0 {
                self.skip_vector3_array(num_vertices); // Normals
            }
            if self.failed() {
                return;
            }
            self.skip(16); // Bounding Sphere
            if self.read_bool_field() != 0 {
                self.skip_color4_array(num_vertices); // Vertex Colors
            }
            if self.failed() {
                return;
            }
            let legacy_data_flags = self.u16();
            // Has UV is an explicit bool only up to and including 4.0.0.2. From
            // 4.1.0.1 the set count lives in Data Flags instead.
            if self.version() <= 0x0400_0002 {
                if self.read_bool_field() != 0 {
                    self.skip((num_vertices as usize).saturating_mul(8));
                }
            } else {
                let uv_sets = legacy_data_flags & 63;
                if uv_sets != 0 {
                    self.skip((uv_sets as usize).saturating_mul(num_vertices as usize) * 8);
                }
            }
            if self.failed() {
                return;
            }
            if type_name == "NiTriShapeData" {
                let num_triangles = self.u16() as u32;
                self.skip(4); // Num Triangle Points
                self.skip((num_triangles as usize).saturating_mul(6));
                let num_match_groups = self.u16() as u32;
                if self.failed() {
                    return;
                }
                self.skip_match_groups(num_match_groups);
                return;
            }
            if type_name == "NiTriStripsData" {
                self.skip(2); // Num Triangles
                let num_strips = self.u16() as u32;
                if self.failed() {
                    return;
                }
                let mut total_points: usize = 0;
                for _ in 0..num_strips {
                    if self.failed() {
                        break;
                    }
                    total_points += self.u16() as usize;
                }
                if self.failed() {
                    return;
                }
                self.skip(total_points.saturating_mul(2));
            }
            return;
        }

        if self.version() >= 0x0A01_0072 {
            self.skip(4); // Group ID (since 10.1.0.114)
        }
        let num_vertices = self.u16() as u32;
        if self.failed() {
            return;
        }
        if self.version() >= 0x0A01_0000 {
            self.skip(1); // Keep Flags (since 10.1.0.0)
            self.skip(1); // Compress Flags (since 10.1.0.0)
        }
        let has_vertices = self.u8();
        if self.failed() {
            return;
        }
        if has_vertices != 0 {
            self.skip_vector3_array(num_vertices);
        }

        let data_flags = self.u16();
        let has_normals = self.u8();
        if self.failed() {
            return;
        }
        if has_normals != 0 {
            self.skip_vector3_array(num_vertices); // Normals
                                                   // BS Data Flags only exists on the 20.2+ streams, which these meshes
                                                   // predate, so the tangent bit comes from Data Flags alone.
            if self.version() >= 0x0A01_0000 && (data_flags & 0x1000) != 0 {
                self.skip_vector3_array(num_vertices); // Tangents
                self.skip_vector3_array(num_vertices); // Bitangents
            }
        }

        self.skip(16); // Bounding Sphere
        let has_vertex_colors = self.u8();
        if self.failed() {
            return;
        }
        if has_vertex_colors != 0 {
            self.skip_color4_array(num_vertices);
        }
        self.skip((data_flags as usize & 63).saturating_mul(num_vertices as usize) * 8); // UV Sets
        self.skip(2); // Consistency Flags
        if self.version() >= 0x1400_0004 {
            self.skip(4); // Additional Data (since 20.0.0.4)
        }

        if type_name == "NiTriShapeData" {
            let num_triangles = self.u16() as u32;
            self.skip(4); // Num Triangle Points
            let mut has_triangles = true; // Triangles are unconditional until 10.0.1.2
            if self.version() >= 0x0A01_0000 {
                has_triangles = self.u8() != 0; // Has Triangles (since 10.1.0.0)
                if self.failed() {
                    return;
                }
            }
            if has_triangles {
                self.skip((num_triangles as usize).saturating_mul(6));
            }
            let num_match_groups = self.u16() as u32;
            if self.failed() {
                return;
            }
            self.skip_match_groups(num_match_groups);
            return;
        }

        if type_name == "NiTriStripsData" {
            self.skip(2); // Num Triangles
            let num_strips = self.u16() as u32;
            if self.failed() {
                return;
            }
            let mut total_points: usize = 0;
            for _ in 0..num_strips {
                if self.failed() {
                    break;
                }
                total_points += self.u16() as usize;
            }
            if self.failed() {
                return;
            }
            let has_points = if self.version() >= 0x0A01_0003 {
                self.u8()
            } else {
                1
            };
            if self.failed() {
                return;
            }
            if has_points != 0 {
                self.skip(total_points.saturating_mul(2));
            }
            return;
        }

        // NiPSysData
        let mut has_radii = 0;
        if self.version() >= 0x0A01_0000 {
            has_radii = self.u8(); // Has Radii (since 10.1.0.0)
            if self.failed() {
                return;
            }
        } else {
            self.skip(4); // Particle Radius (until 10.0.1.0)
        }
        if has_radii != 0 {
            self.skip_float_array(num_vertices);
        }
        self.skip(2); // Num Active
        let has_sizes = self.u8();
        if self.failed() {
            return;
        }
        if has_sizes != 0 {
            self.skip_float_array(num_vertices);
        }
        let mut has_rotations = 0;
        if self.version() >= 0x0A00_0100 {
            has_rotations = self.u8(); // Has Rotations (since 10.0.1.0)
            if self.failed() {
                return;
            }
        }
        if has_rotations != 0 {
            self.skip((num_vertices as usize).saturating_mul(16)); // Quaternion
        }
        // Has Rotation Angles and Has Rotation Axes only exist since 20.0.0.4,
        // so Oblivion meshes never carry them.
        if self.version() >= 0x1400_0004 {
            let has_rotation_angles = self.u8();
            if self.failed() {
                return;
            }
            if has_rotation_angles != 0 {
                self.skip_float_array(num_vertices);
            }
            let has_rotation_axes = self.u8();
            if self.failed() {
                return;
            }
            if has_rotation_axes != 0 {
                self.skip_vector3_array(num_vertices);
            }
        }
        // NiParticleInfo is 40 bytes until 10.4.0.1 because of Rotation Axis,
        // and 28 bytes afterwards.
        let particle_info_stride = if self.version() <= 0x0A04_0001 {
            40
        } else {
            28
        };
        self.skip((num_vertices as usize).saturating_mul(particle_info_stride)); // Particle Info
        if self.version() >= 0x1400_0002 {
            let has_rotation_speeds = self.u8(); // Has Rotation Speeds (since 20.0.0.2)
            if self.failed() {
                return;
            }
            if has_rotation_speeds != 0 {
                self.skip_float_array(num_vertices);
            }
        }
        self.skip(2 + 2); // Num Added Particles, Base

        if type_name == "NiMeshPSysData" {
            self.skip(4); // Default Pool Size
            self.skip(1); // Fill Pools On Load
            let num_generations = self.u32();
            if self.failed() {
                return;
            }
            self.skip((num_generations as usize).saturating_mul(4)); // Generations
            self.skip(4); // Particle Meshes
        }
    }

    /// SkinPartition: the vertex payload is sparse, so every optional block is
    /// gated by its own flag byte. Below 10.1.0.0 there are no flag bytes at
    /// all — the vertex map, the weight table and the face data are
    /// unconditional there (nif.xml SkinPartition).
    fn skip_skin_partition(&mut self) {
        let num_vertices = self.u16() as u32;
        let num_triangles = self.u16() as u32;
        let num_bones = self.u16() as u32;
        let num_strips = self.u16() as u32;
        let num_weights_per_vertex = self.u16() as u32;
        if self.failed() {
            return;
        }
        self.skip_u16_array(num_bones);

        if self.version() < 0x0A01_0000 {
            self.skip_u16_array(num_vertices); // Vertex Map
            self.skip(
                (num_vertices as usize)
                    .saturating_mul(num_weights_per_vertex as usize)
                    .saturating_mul(4),
            );
            let mut legacy_strip_lengths = Vec::with_capacity(num_strips as usize);
            for _ in 0..num_strips {
                if self.failed() {
                    break;
                }
                legacy_strip_lengths.push(self.u16() as u32);
            }
            if self.failed() {
                return;
            }
            if num_strips != 0 {
                for length in legacy_strip_lengths {
                    if self.failed() {
                        break;
                    }
                    self.skip((length as usize).saturating_mul(2));
                }
            } else {
                self.skip((num_triangles as usize).saturating_mul(6));
            }
            let legacy_has_bone_indices = self.u8();
            if self.failed() {
                return;
            }
            if legacy_has_bone_indices != 0 {
                self.skip((num_vertices as usize).saturating_mul(num_weights_per_vertex as usize));
            }
            return;
        }

        let has_vertex_map = self.u8();
        if self.failed() {
            return;
        }
        if has_vertex_map != 0 {
            self.skip_u16_array(num_vertices);
        }
        let has_vertex_weights = self.u8();
        if self.failed() {
            return;
        }
        if has_vertex_weights != 0 {
            self.skip(
                (num_vertices as usize)
                    .saturating_mul(num_weights_per_vertex as usize)
                    .saturating_mul(4),
            );
        }
        let mut strip_lengths = Vec::with_capacity(num_strips as usize);
        for _ in 0..num_strips {
            if self.failed() {
                break;
            }
            strip_lengths.push(self.u16() as u32);
        }
        if self.failed() {
            return;
        }
        let has_faces = self.u8();
        if self.failed() {
            return;
        }
        if has_faces != 0 {
            if num_strips != 0 {
                for length in strip_lengths {
                    if self.failed() {
                        break;
                    }
                    self.skip((length as usize).saturating_mul(2));
                }
            } else {
                self.skip((num_triangles as usize).saturating_mul(6));
            }
        }
        let has_bone_indices = self.u8();
        if self.failed() {
            return;
        }
        if has_bone_indices != 0 {
            self.skip((num_vertices as usize).saturating_mul(num_weights_per_vertex as usize));
        }
    }

    /// ControlledBlock. Records below 10.1.0.104 hold the target name and the
    /// controller reference only, 10.1.0.104/105 add the blend interpolator and
    /// index, and the controller ID strings run from 10.1.0.104 through
    /// 10.1.0.113. The name-in-palette offsets replace those strings between
    /// 10.2.0.0 and 20.1.0.0.
    fn skip_controlled_block(&mut self) {
        let version = self.version();
        if version < VERSION_10_1_0_104 {
            self.skip_string_array(1); // Target Name
            self.skip(4); // Controller
            return;
        }
        if version < VERSION_10_1_0_106 {
            self.skip(4); // Controller
        } else {
            self.skip(4 + 4); // Interpolator, Controller
        }
        if version <= VERSION_10_1_0_110 {
            self.skip(4); // Blend Interpolator
            self.skip(2); // Blend Index
        }
        if version >= VERSION_10_1_0_106 {
            self.skip(1); // Priority
        }
        if version <= VERSION_10_1_0_113 {
            self.skip_string_array(5); // Node Name .. Interpolator ID
        } else if version <= VERSION_20_1_0_0 {
            self.skip(4 + 20); // String Palette, five name offsets
        } else {
            self.skip_string_array(5); // Node Name .. Interpolator ID
        }
    }

    /// Morph: Frame Name (10.1.0.106 and later) | Num Keys, Interpolation and
    /// Keys (before 10.1.0.0) | Legacy Weight (10.1.0.104-20.1.0.2) | Vectors.
    ///
    /// The legacy weight follows the stream version, not the user version: the
    /// 10.1.0.106 menus report BS version 5 and keep the weight, while the
    /// 20.0.0.4 meshes report 11 and go straight from the frame name to the
    /// vectors.
    fn skip_morph(&mut self, num_vertices: u32) {
        let version = self.version();
        if version >= VERSION_10_1_0_106 {
            self.skip_string_array(1); // Frame Name
        } else if version < 0x0A01_0000 {
            let num_keys = self.u32();
            if self.failed() {
                return;
            }
            let mut interpolation = 0;
            if num_keys != 0 {
                interpolation = self.u32() as i32;
                if self.failed() {
                    return;
                }
            }
            self.skip_keys(num_keys, "float", interpolation);
        }
        if version >= VERSION_10_1_0_104 && self.bs_version() < 10 {
            self.skip(4); // Legacy Weight
        }
        self.skip_vector3_array(num_vertices);
    }

    // -----------------------------------------------------------------
    // The dispatcher
    // -----------------------------------------------------------------

    /// Measures one block body. Returns false for a type this walker does not
    /// know, or once a read has failed.
    fn walk_block_body(&mut self, type_name: &str) -> bool {
        match type_name {
            // --- NiExtraData roots (Name only, no extra data list or controller)
            "NiStringExtraData" => {
                self.skip_string_array(1); // Name
                self.skip_string_array(1); // String Data
            }
            "NiBinaryExtraData" => {
                self.skip_string_array(1); // Name
                self.skip_string_array(1); // Binary Data (ByteArray)
            }
            "NiBooleanExtraData" => {
                self.skip_string_array(1); // Name
                self.skip(1); // Boolean Data
            }
            "NiIntegerExtraData" | "BSXFlags" => {
                self.skip_string_array(1); // Name
                self.skip(4); // Integer Data
            }
            "BSBound" => {
                self.skip_string_array(1); // Name
                self.skip(12 + 12); // Center, Dimensions
            }
            "BSFurnitureMarker" => {
                self.skip_string_array(1); // Name
                let num_positions = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_furniture_positions(num_positions);
            }
            "NiTextKeyExtraData" => {
                self.skip_string_array(1); // Name
                let num_text_keys = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_keys(num_text_keys, "string", 1);
            }

            // --- NiObjectNET roots
            "NiMaterialProperty" => {
                self.skip_ni_object_net();
                if self.version() <= 0x0A00_0102 {
                    self.skip(2); // Flags (until 10.0.1.2)
                }
                self.skip(4 * 12 + 4 + 4); // Four colors, Glossiness, Alpha
            }
            "NiAlphaProperty" => {
                self.skip_ni_object_net();
                self.skip(2 + 1); // Flags, Threshold
            }
            "NiVertexColorProperty" => {
                self.skip_ni_object_net();
                self.skip(2 + 4 + 4); // Flags, Vertex Mode, Lighting Mode
            }
            "NiZBufferProperty" => {
                self.skip_ni_object_net();
                self.skip(2); // Flags
                if self.version() >= 0x0401_000C {
                    self.skip(4); // Function (since 4.1.0.12)
                }
            }
            "NiSpecularProperty" | "NiWireframeProperty" | "NiDitherProperty" => {
                self.skip_ni_object_net();
                self.skip(2); // Flags
            }
            "NiFogProperty" => {
                self.skip_ni_object_net();
                self.skip(2 + 4 + 12); // Flags, Fog Depth, Fog Color
            }
            "NiStencilProperty" => {
                self.skip_ni_object_net();
                if self.version() <= 0x0A00_0102 {
                    self.skip(2); // Flags (until 10.0.1.2)
                }
                self.skip(1 + 4 + 4 + 4 + 4 + 4 + 4 + 4);
            }
            "NiSourceTexture" => {
                self.skip_ni_object_net();
                let use_external = self.u8();
                if self.failed() {
                    return false;
                }
                if self.version() < 0x0A01_0000 {
                    let mut use_internal = 0;
                    if use_external == 0 {
                        use_internal = self.u8(); // Use Internal (until 10.0.1.3)
                        if self.failed() {
                            return false;
                        }
                    } else {
                        self.skip_string_array(1); // File Name
                    }
                    if use_external == 0 && use_internal != 0 {
                        self.skip(4); // Pixel Data
                    }
                } else {
                    self.skip_string_array(1); // File Name
                    self.skip(4); // Pixel Data
                }
                self.skip(12); // Format Prefs
                self.skip(1); // Is Static
                if self.version() >= 0x0A01_0067 {
                    self.skip(1); // Direct Render (since 10.1.0.103)
                }
            }
            "NiStringPalette" => {
                self.skip_string_array(1); // Palette
                self.skip(4); // Length
            }
            "NiTexturingProperty" => {
                self.skip_ni_object_net();
                if self.version() <= 0x0A00_0102 {
                    self.skip(2); // Flags (until 10.0.1.2)
                }
                self.skip(4); // Apply Mode
                let texture_count = self.u32();
                if self.failed() {
                    return false;
                }
                for _ in 0..5 {
                    if self.failed() {
                        break;
                    }
                    let has_texture = self.u8();
                    if self.failed() {
                        return false;
                    }
                    if has_texture != 0 {
                        self.skip_tex_desc();
                    }
                }
                if texture_count > 5 {
                    let has_bump_map = self.u8();
                    if self.failed() {
                        return false;
                    }
                    if has_bump_map != 0 {
                        self.skip_tex_desc();
                        self.skip(4 + 4 + 16); // Luma Scale, Luma Offset, Matrix22
                    }
                }
                for slot in 6..=9 {
                    if self.failed() {
                        break;
                    }
                    if texture_count <= slot {
                        break;
                    }
                    let has_decal = self.u8();
                    if self.failed() {
                        return false;
                    }
                    if has_decal != 0 {
                        self.skip_tex_desc();
                    }
                }
                let num_shader_textures = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_shader_tex_descs(num_shader_textures);
            }

            // --- NiAVObject roots
            "NiNode" | "NiBillboardNode" => {
                self.skip_ni_av_object();
                let child_count = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_ref_array(child_count);
                let effect_count = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_ref_array(effect_count);
                if type_name == "NiBillboardNode" {
                    self.skip(2); // Billboard Mode
                }
            }
            "NiAmbientLight" | "NiDirectionalLight" => {
                self.skip_ni_av_object();
                self.skip_ni_light();
            }
            "NiPointLight" => {
                self.skip_ni_av_object();
                self.skip_ni_light();
                self.skip(4 * 3); // Constant, Linear, Quadratic
            }
            "NiSpotLight" => {
                self.skip_ni_av_object();
                self.skip_ni_light();
                self.skip(4 * 3 + 4 + 4); // Point light pair, Angle, Exponent
            }
            "NiCamera" => {
                self.skip_ni_av_object();
                self.skip(2); // Camera Flags
                self.skip(6 * 4); // Frustum
                self.skip(1); // Use Orthographic Projection
                self.skip(4 * 4); // Viewport
                self.skip(4 + 4 + 4 + 4); // LOD Adjust, Scene, Screen counts
            }
            "NiTriShape" | "NiTriStrips" => {
                self.skip_ni_geometry();
            }
            "NiParticleSystem" | "NiMeshParticleSystem" => {
                self.skip_ni_geometry();
                self.skip(1); // World Space
                let num_modifiers = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_ref_array(num_modifiers);
            }

            // --- Geometry data
            "NiTriShapeData" | "NiTriStripsData" | "NiPSysData" | "NiMeshPSysData" => {
                self.skip_ni_geometry_data(type_name);
            }

            // --- NiTimeController roots
            "NiPSysUpdateCtlr" | "NiPSysResetOnLoopCtlr" => {
                self.skip_ni_time_controller();
            }
            "NiKeyframeController" => {
                // The interpolator link starts at 10.1.0.104 while the keyframe
                // data link stops at 10.1.0.103, so a file carries one, the
                // other, or (in the 10.1.0.104..10.1.0.108 window) both.
                self.skip_ni_interp_controller();
                if self.version() >= 0x0A01_0068 {
                    self.skip(4); // Interpolator
                }
                if self.version() <= 0x0A01_0067 {
                    self.skip(4); // Data
                }
            }
            "BSKeyframeController" => {
                // NiKeyframeController plus a second keyframe data link.
                self.skip_ni_interp_controller();
                if self.version() >= 0x0A01_0068 {
                    self.skip(4); // Interpolator
                }
                if self.version() <= 0x0A01_0067 {
                    self.skip(4); // Data
                }
                self.skip(4); // Data 2
            }
            "NiAlphaController" | "NiVisController" | "NiTransformController" => {
                self.skip_ni_interp_controller();
                self.skip(4); // Interpolator
            }
            "NiPSysEmitterSpeedCtlr"
            | "NiPSysEmitterDeclinationCtlr"
            | "NiPSysEmitterInitialRadiusCtlr"
            | "NiPSysModifierActiveCtlr"
            | "NiPSysGravityStrengthCtlr"
            | "NiPSysEmitterLifeSpanCtlr" => {
                self.skip_ni_interp_controller();
                self.skip(4); // Interpolator
                self.skip_string_array(1); // Modifier Name
            }
            "NiMaterialColorController" => {
                self.skip_ni_interp_controller();
                self.skip(4 + 2); // Interpolator, Target Color (MaterialColor)
            }
            "NiTextureTransformController" => {
                self.skip_ni_interp_controller();
                self.skip(4 + 1 + 4 + 4); // Interpolator, Shader Map, Slot, Operation
            }
            "NiFlipController" => {
                self.skip_ni_interp_controller();
                self.skip(4 + 4); // Interpolator, Texture Slot
                let num_sources = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_ref_array(num_sources);
            }
            "NiGeomMorpherController" => {
                self.skip_ni_interp_controller();
                self.skip(2); // Morpher Flags
                self.skip(4); // Data
                self.skip(1); // Always Update
                let num_interpolators = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_ref_array(num_interpolators);
                // The int list that 10.2.0.0 appends after the interpolator refs
                // is guarded by a BS version above 9. nif.xml bounds it with
                // until="20.0.0.5", which is inclusive, so the 20.0.0.5 meshes
                // carry it too: bow.nif has Num Unknown Ints = 2 right here.
                if self.version() >= VERSION_10_2_0_0
                    && self.version() <= VERSION_20_0_0_5
                    && self.bs_version() > 9
                {
                    let num_unknown_ints = self.u32();
                    if self.failed() {
                        return false;
                    }
                    self.skip((num_unknown_ints as usize).saturating_mul(4));
                }
            }
            "NiMultiTargetTransformController" => {
                self.skip_ni_interp_controller();
                let num_extra_targets = self.u16() as u32;
                if self.failed() {
                    return false;
                }
                self.skip_ptr_array(num_extra_targets);
            }
            "NiBSBoneLODController" => {
                self.skip_ni_time_controller();
                self.skip(4); // LOD
                let num_lods = self.u32();
                self.skip(4); // Num Node Groups
                if self.failed() {
                    return false;
                }
                // Node Groups is sized by Num LODs rather than Num Node Groups.
                for _ in 0..num_lods {
                    if self.failed() {
                        break;
                    }
                    self.skip_node_set();
                }
            }
            "NiPSysEmitterCtlr" => {
                self.skip_ni_interp_controller();
                self.skip(4); // Interpolator
                self.skip_string_array(1); // Modifier Name
                self.skip(4); // Visibility Interpolator
            }
            "NiControllerManager" => {
                self.skip_ni_time_controller();
                self.skip(1); // Cumulative
                let num_sequences = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_ref_array(num_sequences);
                self.skip(4); // Object Palette
            }
            "bhkBlendController" => {
                self.skip_ni_time_controller();
                self.skip(4); // Keys
            }

            // --- Interpolators
            "NiTransformInterpolator" => {
                self.skip(32); // Translation, Rotation, Scale
                if self.version() <= 0x0A01_006D {
                    self.skip(3); // TRS Valid: bool[3] (until 10.1.0.109)
                }
                self.skip(4); // Data
            }
            "NiFloatInterpolator" => {
                self.skip(4 + 4); // Value, Data
            }
            "NiBoolInterpolator" | "NiBoolTimelineInterpolator" => {
                self.skip(1 + 4); // Value, Data
            }
            "NiPoint3Interpolator" => {
                self.skip(12 + 4); // Value, Data
            }
            "NiBlendFloatInterpolator" => {
                self.skip_blend_interpolator(4);
            }
            "NiBlendBoolInterpolator" => {
                self.skip_blend_interpolator(1);
            }
            "NiBlendTransformInterpolator" => {
                // The stored NiQuatTransform disappears at 10.1.0.110.
                let value_size = if self.version() <= 0x0A01_006D { 35 } else { 0 };
                self.skip_blend_interpolator(value_size);
            }
            "NiBlendPoint3Interpolator" => {
                self.skip_blend_interpolator(12);
            }
            "NiBlendColorInterpolator" | "NiBlendQuaternionInterpolator" => {
                self.skip_blend_interpolator(16);
            }
            "NiPathInterpolator" => {
                self.skip(2 + 4 + 4 + 4 + 2 + 4 + 4); // Flags, Bank, Angle, Smoothing, Axis, Path, Percent
            }
            "NiBSplineInterpolator"
            | "NiBSplineTransformInterpolator"
            | "NiBSplineCompTransformInterpolator"
            | "NiBSplinePoint3Interpolator"
            | "NiBSplineCompPoint3Interpolator"
            | "NiBSplineFloatInterpolator"
            | "NiBSplineCompFloatInterpolator" => {
                self.skip_bspline_interpolator(type_name);
            }

            // --- Key data
            "NiFloatData" => {
                self.skip_key_group("float");
            }
            "NiBoolData" => {
                self.skip_key_group("byte");
            }
            "NiColorData" => {
                self.skip_key_group("Color4");
            }
            "NiPosData" => {
                self.skip_key_group("Vector3");
            }
            "NiTransformData" | "NiKeyframeData" => {
                self.skip_keyframe_data();
            }

            // --- Spline data
            "NiBSplineData" => {
                self.skip_bspline_data();
            }
            "NiBSplineBasisData" => {
                self.skip(4); // Num Control Points
            }

            // --- Animation payload
            "NiMorphData" => {
                let num_morphs = self.u32();
                let num_vertices = self.u32();
                self.skip(1); // Relative Targets
                if self.failed() {
                    return false;
                }
                for _ in 0..num_morphs {
                    if self.failed() {
                        break;
                    }
                    self.skip_morph(num_vertices);
                }
            }
            "NiControllerSequence" => {
                self.skip_string_array(1); // Name
                if self.version() < VERSION_10_1_0_106 {
                    // Legacy NiSequence: the accum root name and the text keys
                    // replace the weight, timing and manager block that
                    // 10.1.0.106 introduced, and the controlled block array has
                    // no grow by field.
                    self.skip_string_array(1); // Accum Root Name
                    self.skip(4); // Text Keys
                    let num_legacy_blocks = self.u32();
                    if self.failed() {
                        return false;
                    }
                    for _ in 0..num_legacy_blocks {
                        if self.failed() {
                            break;
                        }
                        self.skip_controlled_block();
                    }
                } else {
                    let num_controlled_blocks = self.u32();
                    self.skip(4); // Array Grow By
                    if self.failed() {
                        return false;
                    }
                    for _ in 0..num_controlled_blocks {
                        if self.failed() {
                            break;
                        }
                        self.skip_controlled_block();
                    }
                    self.skip(4 + 4 + 4 + 4); // Weight, Text Keys, Cycle Type, Frequency
                    if (VERSION_10_1_0_106..=VERSION_10_4_0_1).contains(&self.version()) {
                        self.skip(4); // Phase
                    }
                    self.skip(4 + 4); // Start Time, Stop Time
                    if self.version() == VERSION_10_1_0_106 {
                        self.skip(1); // Play Backwards
                    }
                    self.skip(4); // Manager
                    self.skip_string_array(1); // Accum Root Name
                    if (VERSION_10_1_0_113..=VERSION_20_1_0_0).contains(&self.version()) {
                        self.skip(4); // String Palette
                    }
                }
            }
            "NiDefaultAVObjectPalette" => {
                self.skip(4); // Scene
                let num_objects = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_av_object_array(num_objects);
            }
            "NiSkinInstance" => {
                // The partition link sits between Data and Skeleton Root, and
                // only exists since 10.1.0.101. Below that the two references
                // are adjacent, so reading a partition link there shifts every
                // following field by four bytes.
                self.skip(4); // Data
                if self.version() >= 0x0A01_0065 {
                    self.skip(4); // Skin Partition
                }
                self.skip(4); // Skeleton Root
                let num_bones = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_ptr_array(num_bones);
            }
            "NiSkinData" => {
                self.skip(52); // Skin Transform
                let num_bones = self.u32();
                // The partition link lives on this block only below 10.1.0.0,
                // where the skin instance has no partition of its own.
                if (0x0400_0002..0x0A01_0000).contains(&self.version()) {
                    self.skip(4); // Skin Partition
                }
                // Has Vertex Weights defaults to true below 4.2.1.0, where the
                // byte is absent from the stream.
                let mut has_vertex_weights = 1;
                if self.version() >= 0x0402_0100 {
                    has_vertex_weights = self.u8();
                }
                if self.failed() {
                    return false;
                }
                for _ in 0..num_bones {
                    if self.failed() {
                        break;
                    }
                    self.skip(52 + 16); // Bone skin transform, Bounding Sphere
                    let num_vertices = self.u16() as u32;
                    if self.failed() {
                        return false;
                    }
                    if has_vertex_weights != 0 {
                        self.skip((num_vertices as usize).saturating_mul(6)); // BoneVertData
                    }
                }
            }
            "NiSkinPartition" => {
                let num_partitions = self.u32();
                if self.failed() {
                    return false;
                }
                for _ in 0..num_partitions {
                    if self.failed() {
                        break;
                    }
                    self.skip_skin_partition();
                }
            }

            // --- Particle system modifiers
            "NiPSysAgeDeathModifier" => {
                self.skip_ni_psys_modifier();
                self.skip(1 + 4); // Spawn on Death, Spawn Modifier
            }
            "NiPSysBoundUpdateModifier" => {
                self.skip_ni_psys_modifier();
                self.skip(2); // Update Skip
            }
            "NiPSysGrowFadeModifier" => {
                self.skip_ni_psys_modifier();
                self.skip(4 + 2 + 4 + 2); // Grow Time, Grow Generation, Fade pair
            }
            "NiPSysPositionModifier" => {
                self.skip_ni_psys_modifier();
            }
            "NiPSysSpawnModifier" => {
                self.skip_ni_psys_modifier();
                self.skip(2 + 4 + 2 + 2 + 4 + 4 + 4 + 4);
            }
            "NiPSysColorModifier" => {
                self.skip_ni_psys_modifier();
                self.skip(4); // Data
            }
            "NiPSysColliderManager" => {
                self.skip_ni_psys_modifier();
                self.skip(4); // Collider
            }
            "NiPSysGravityModifier" => {
                self.skip_ni_psys_modifier();
                self.skip(4 + 12 + 4 + 4 + 4 + 4 + 4); // Object, Axis, Decay, Strength, Force Type, Turbulence pair
            }
            "NiPSysBombModifier" => {
                self.skip_ni_psys_modifier();
                self.skip(4 + 12 + 4 + 4 + 4 + 4); // Object, Axis, Decay, Delta V, Decay Type, Symmetry Type
            }
            "NiPSysDragModifier" => {
                self.skip_ni_psys_modifier();
                self.skip(4 + 12 + 4 + 4 + 4); // Object, Axis, Percentage, Range pair
            }
            "NiPSysMeshUpdateModifier" => {
                self.skip_ni_psys_modifier();
                let num_meshes = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_ptr_array(num_meshes);
            }
            "BSParentVelocityModifier" => {
                self.skip_ni_psys_modifier();
                self.skip(4); // Damping
            }
            "NiPSysSphereEmitter" => {
                self.skip_ni_psys_modifier();
                self.skip_ni_psys_emitter_base(true);
                self.skip(4); // Radius
            }
            "NiPSysCylinderEmitter" => {
                self.skip_ni_psys_modifier();
                self.skip_ni_psys_emitter_base(true);
                self.skip(4 + 4); // Radius, Height
            }
            "BSPSysArrayEmitter" => {
                self.skip_ni_psys_modifier();
                self.skip_ni_psys_emitter_base(true);
            }
            "NiPSysBoxEmitter" => {
                self.skip_ni_psys_modifier();
                self.skip_ni_psys_emitter_base(true);
                self.skip(4 + 4 + 4); // Width, Height, Depth
            }
            "NiPSysMeshEmitter" => {
                self.skip_ni_psys_modifier();
                self.skip_ni_psys_emitter_base(false); // No emitter object on this one
                let num_emitter_meshes = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_ptr_array(num_emitter_meshes);
                self.skip(4 + 4 + 12); // Velocity Type, Emission Type, Axis
            }
            "NiPSysRotationModifier" => {
                self.skip_ni_psys_modifier();
                self.skip(4 + 4 + 4 + 4 + 1 + 1 + 12);
            }
            "NiPSysPlanarCollider" => {
                // Derives from NiPSysCollider, not NiPSysModifier.
                self.skip(4 + 1 + 1 + 4 + 4 + 4 + 4); // Bounce .. Collider Object
                self.skip(4 + 4 + 12 + 12); // Width, Height, X Axis, Y Axis
            }

            // --- Havok collision
            "bhkCollisionObject" | "bhkSPCollisionObject" => {
                self.skip(4 + 2 + 4); // Target, Flags, Body
            }
            "bhkBlendCollisionObject" => {
                self.skip(4 + 2 + 4 + 4 + 4); // plus Heir Gain, Vel Gain
                if self.bs_version() < 9 {
                    self.skip(4 + 4); // Unknown Float 1, Unknown Float 2
                }
            }
            "bhkRigidBody" | "bhkRigidBodyT" => {
                // nif.xml names this word "Unknown Int" (declared until
                // 10.0.1.2, but the corpus needs it at 10.1.0.x as well, which
                // is why blockBodyPrefix() keeps its 4 there). The block's real
                // Shape reference is that leading word.
                self.skip(4); // Unknown Int
                self.skip(4); // Havok Filter
                self.skip(4 + 1 + 3 + 12); // World Object Info
                self.skip(1 + 1 + 2); // Entity Info
                                      // The leading six fields and the three Max fields of the info
                                      // struct only exist from 10.1.0.0 on.
                self.skip(if self.version() >= 0x0A01_0000 {
                    196
                } else {
                    168
                }); // bhkRigidBodyCInfo550_660
                let num_constraints = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_ref_array(num_constraints);
                self.skip(4); // Body Flags
            }
            "bhkTransformShape" | "bhkConvexTransformShape" => {
                // The wrapped shape index sits in the leading word that
                // blockBodyPrefix() skips, so these labels are one field late;
                // the byte count is unaffected.
                self.skip(4 + 4 + 4 + 8); // Shape, Material, Radius, Unused 01
                self.skip(64); // Transform
            }
            "bhkConvexSweepShape" => {
                self.skip(4 + 4 + 4 + 12); // Shape, Material, Radius, Unknown
            }
            "bhkMultiSphereShape" => {
                self.skip(4 + 12); // Material, Shape Property
                let num_spheres = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip((num_spheres as usize).saturating_mul(16)); // NiBound
            }
            "bhkSimpleShapePhantom" => {
                self.skip(4 + 4 + 20); // Shape, Havok Filter, World Object Info
                self.skip(8); // Unused 01
                self.skip(64); // Transform
            }
            "bhkSphereShape" => {
                self.skip(4 + 4); // Material, Radius
            }
            "bhkBoxShape" => {
                self.skip(4 + 4 + 8 + 12 + 4);
            }
            "bhkCapsuleShape" => {
                self.skip(4 + 4 + 8 + 12 + 4 + 12 + 4);
            }
            "bhkConvexVerticesShape" => {
                self.skip(4 + 4 + 12 + 12); // Material, Radius, two properties
                let num_vertices = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_vector4_array(num_vertices);
                let num_normals = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_vector4_array(num_normals);
            }
            "bhkNiTriStripsShape" => {
                self.skip(4 + 4 + 20 + 4); // Material, Radius, Unused 01, Grow By
                if self.version() >= 0x0A01_0000 {
                    self.skip(16); // Scale (since 10.1.0.0)
                }
                let num_strips_data = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_ref_array(num_strips_data);
                let num_filters = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip((num_filters as usize).saturating_mul(4)); // HavokFilter
            }
            "bhkListShape" => {
                let num_sub_shapes = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_ref_array(num_sub_shapes);
                // A HavokMaterial leads with an extra word up to and including
                // 10.0.1.2: oar01 measures the sub-shape list as 56 bytes with
                // eight bytes of material, the 20.0.0.4 meshes only four.
                self.skip((if self.version() <= 0x0A00_0102 { 8 } else { 4 }) + 12 + 12);
                let num_filters = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip((num_filters as usize).saturating_mul(4));
            }
            "bhkMeshShape" => {
                // 10.0.1.0 only. ungrdltraphingedoor measures 204 bytes: eight
                // bytes of unknown words, the radius, two more, a scale, the
                // shape property array, three trailing words and a single strip
                // data reference.
                self.skip(8 + 4 + 8 + 16);
                let num_shape_properties = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip((num_shape_properties as usize).saturating_mul(12));
                self.skip(12); // Unknown 03
                let num_strips_data = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip_ref_array(num_strips_data);
            }
            "bhkMoppBvTreeShape" => {
                // Below 10.1.0.0 the shape ref lives in the per-block prefix and
                // the MOPP code offset does not exist yet.
                if self.version() >= 0x0A01_0000 {
                    self.skip(4); // Shape
                }
                self.skip(12 + 4); // Unused, Scale
                let code_size = self.u32();
                if self.failed() {
                    return false;
                }
                if self.version() >= 0x0A01_0000 {
                    self.skip(16); // MOPP code offset
                }
                self.skip(code_size as usize); // MOPP code data
            }
            "bhkLimitedHingeConstraint" => {
                self.skip(16); // bhkConstraintCInfo
                self.skip(124); // 7 x Vector4, Min/Max Angle, Friction
            }
            "bhkHingeConstraint" => {
                self.skip(16); // bhkConstraintCInfo
                self.skip(80); // 5 x Vector4
            }
            "bhkPrismaticConstraint" => {
                self.skip(16); // bhkConstraintCInfo
                self.skip(140); // 8 x Vector4, Min/Max, Friction
            }
            "bhkStiffSpringConstraint" => {
                self.skip(16); // bhkConstraintCInfo
                self.skip(36); // Pivot pair, Length
            }
            "bhkRagdollConstraint" => {
                self.skip(16); // bhkConstraintCInfo
                self.skip(120); // 6 x Vector4, 5 angles, Friction
            }
            "bhkMalleableConstraint" => {
                self.skip(16); // bhkConstraintCInfo
                let constraint_type = self.u32();
                if self.failed() {
                    return false;
                }
                self.skip(16); // nested bhkConstraintCInfo
                let tail = match constraint_type {
                    0 => 32,  // Ball and socket
                    1 => 80,  // Hinge
                    2 => 124, // Limited hinge
                    6 => 140, // Prismatic
                    7 => 120, // Ragdoll
                    8 => 36,  // Stiff spring
                    _ => 0,
                };
                self.skip(tail);
                self.skip(4 + 4); // Tau, Damping
            }

            // --- Havok packed triangle data
            "hkPackedNiTriStripsData" => {
                // Shipped on the Shivering Isles (20.0.0.5) meshes.
                // ruinsonroomfloora01 measures the block as 96 bytes with two
                // triangles and four vertices: the triangle count is a uint, not
                // the ushort of the pre-BSVER-34 reads, and the payload is
                // always materialised (there is no "Has Data" flag).
                let num_triangles = self.u32();
                if self.failed() {
                    return false;
                }
                // TriangleData = Triangle (3 x ushort) + Welding Info (ushort)
                // plus the triangle normal, which nif.xml drops after 20.0.0.5.
                let triangle_size: usize =
                    6 + 2 + if self.version() <= 0x1400_0005 { 12 } else { 0 };
                self.skip((num_triangles as usize).saturating_mul(triangle_size));
                let num_vertices = self.u32();
                if self.failed() {
                    return false;
                }
                if self.version() >= 0x1402_0007 {
                    // From 20.2.0.7 on the vertices may be half-precision and
                    // the sub part data moved into this block.
                    let compressed = self.u8();
                    if self.failed() {
                        return false;
                    }
                    self.skip((num_vertices as usize).saturating_mul(if compressed != 0 {
                        6
                    } else {
                        12
                    }));
                    let num_sub_shapes = self.u16() as u32;
                    if self.failed() {
                        return false;
                    }
                    self.skip((num_sub_shapes as usize).saturating_mul(12)); // hkSubPartData
                } else {
                    self.skip_vector3_array(num_vertices);
                }
            }
            "bhkPackedNiTriStripsShape" => {
                // Shipped on the Shivering Isles (20.0.0.5) meshes.
                // ruinsonroomfloora01 measures the block as 70 bytes with one
                // sub shape: hkSubPartData is a HavokFilter (4 bytes), the
                // sub-shape vertex count and the Havok material, and the sub
                // shape list only exists up to and including 20.0.0.5.
                if self.version() <= 0x1400_0005 {
                    let num_sub_shapes = self.u16() as u32;
                    if self.failed() {
                        return false;
                    }
                    self.skip((num_sub_shapes as usize).saturating_mul(12));
                }
                self.skip(4); // User Data
                self.skip(4); // Unused 01
                self.skip(4); // Radius
                self.skip(4); // Unused 02
                self.skip(16); // Scale
                self.skip(4); // Radius Copy
                self.skip(16); // Scale Copy
                self.skip(4); // Data reference
            }

            // Unknown type: the walk cannot measure this block.
            _ => return false,
        }

        !self.failed()
    }
}
