//! Gamebryo / NetImmerse `.nif` container parser.
//!
//! This crate ports the container half of `NIFParser` from
//! `Oblivion_Android/app/src/main/cpp/assets/nif_parser.cpp`: the four header
//! families retail Oblivion ships, the object array that maps a block index to
//! its type, and the walker that measures every block body. Geometry, skin,
//! animation and collision decoding arrive in later segments on top of the
//! block ranges recorded here.
//!
//! Why the walker exists: retail NIF files carry no per-block size table, so
//! the byte range of block *n* is only known after every preceding block has
//! been decoded far enough to measure it. The ranges are therefore the single
//! source of truth for every block-anchored reader that follows.
//!
//! Behaviour follows the original reader rather than an idealised format,
//! because files in the wild depend on its quirks — the per-block prefix on
//! 10.0.1.x / 10.1.0.x meshes, the exception for the NiCollisionObject
//! subclasses, and the repeated trailer that lets eight landscape LOD tiles
//! carry tool data. Each is commented where it is implemented.
//!
//! Parse failures are reported as [`NifError`]. A walk that cannot measure a
//! block is *not* a parse failure: the file is still usable, only the
//! block-anchored readers become unavailable, exactly as in the C++ original.

mod block;
mod geometry;
mod header;
mod reader;
mod walk;

pub use block::{
    AnimationClip, AnimationKeyframe, BoneData, CollisionObject, CollisionShape, ControllerManager,
    ControllerSequence, FloatKey, FloatKeyGroup, KeyframeController, KeyframeData, PackedWeight,
    Partition, QuaternionKey, RigidBodyInfo, RigidBodyRead, ShapeType, SkinData, SkinInstance,
    SkinPartition, TextKey, Vector3Key, Vector3KeyGroup, VertexWeight,
};

use std::fmt;
use std::mem::size_of;
use std::path::Path;

use header::Header;
use reader::Reader;
use walk::{WalkState, Walker};

pub use header::Header as NifHeader;

/// Why a NIF file could not be parsed.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum NifError {
    /// The file could not be opened or read.
    Io,
    /// The buffer holds no data.
    Empty,
    /// The magic line names neither a Gamebryo nor a NetImmerse stream.
    BadMagic,
    /// The header runs past the end of the buffer.
    Truncated,
    /// A header field holds a value this reader rejects.
    Corrupt,
}

impl fmt::Display for NifError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        let message = match self {
            NifError::Io => "NIF file could not be read",
            NifError::Empty => "NIF buffer is empty",
            NifError::BadMagic => "not a NIF file",
            NifError::Truncated => "NIF header runs past the end of the file",
            NifError::Corrupt => "NIF header declares unsupported values",
        };
        formatter.write_str(message)
    }
}

impl std::error::Error for NifError {}

/// One block of the object array.
///
/// `parent_index` stays `-1` until the geometry phase wires the scene graph,
/// which is what the original `parseObjectArray` leaves behind as well.
#[derive(Clone, Debug, PartialEq)]
pub struct Node {
    /// Inline object name of the block, read only for NiObjectNET-derived
    /// types; empty for every other block.
    pub name: String,
    /// Index of this node inside the object array.
    pub node_index: u32,
    /// Parent node index, `-1` while the node is a root.
    pub parent_index: i32,
    /// Child node indices, filled by the geometry phase.
    pub child_indices: Vec<i32>,
    /// Index into [`Header::block_type_names`].
    pub block_type_index: u16,
    /// Resolved block type name, e.g. `NiNode`.
    pub block_type_name: String,
    /// True once the geometry phase has decoded a payload into `geometry`.
    pub has_geometry: bool,
    /// Decoded geometry payload, if this block carries one.
    pub geometry: Option<Geometry>,
}

impl Node {
    fn new(node_index: u32) -> Self {
        Self {
            name: String::new(),
            node_index,
            parent_index: -1,
            child_indices: Vec::new(),
            block_type_index: 0,
            block_type_name: String::new(),
            has_geometry: false,
            geometry: None,
        }
    }
}

/// A 3-component vector, matching `NIFVector3`.
///
/// `repr(C)` because the C ABI hands out a `const float *` straight into a
/// `Vec<Vector3>`: three `f32` fields with no padding is a guarantee only the
/// C layout makes.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct Vector3 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}

/// A 4-component vector, matching `NIFVector4`. Defaults to the identity
/// `w = 1`.
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Vector4 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub w: f32,
}

impl Default for Vector4 {
    fn default() -> Self {
        Self {
            x: 0.0,
            y: 0.0,
            z: 0.0,
            w: 1.0,
        }
    }
}

/// Row-major 3x3 rotation, matching `NIFMatrix3x3`. Defaults to identity.
#[repr(C)]
#[derive(Clone, Debug, PartialEq)]
pub struct Matrix3x3 {
    pub m: [[f32; 3]; 3],
}

impl Default for Matrix3x3 {
    fn default() -> Self {
        Self {
            m: [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]],
        }
    }
}

/// Translation / rotation / scale of a scene block, matching `NIFTransform`.
#[repr(C)]
#[derive(Clone, Debug, PartialEq)]
pub struct Transform {
    pub rotation: Matrix3x3,
    pub translation: Vector3,
    pub scale: f32,
}

impl Default for Transform {
    fn default() -> Self {
        Self {
            rotation: Matrix3x3::default(),
            translation: Vector3::default(),
            scale: 1.0,
        }
    }
}

/// One decoded shape, matching `NIFGeometry`.
///
/// Texture coordinates are stored as `[u, v]` pairs instead of `glm::vec2`:
/// the C ABI flattens them to two floats per vertex, and the C++ shim converts
/// back on its side of the boundary.
#[derive(Clone, Debug, PartialEq)]
pub struct Geometry {
    pub name: String,
    pub transform: Transform,
    pub vertices: Vec<Vector3>,
    pub normals: Vec<Vector3>,
    pub colors: Vec<Vector4>,
    pub tex_coords: Vec<[f32; 2]>,
    pub triangles: Vec<[u16; 3]>,
    pub diffuse_texture: String,
    pub normal_texture: String,
    /// Block table index of the attached material property, or `u32::MAX`.
    pub material_property_index: u32,
    /// Block table index of the attached texturing property, or `u32::MAX`.
    pub texturing_property_index: u32,
}

impl Default for Geometry {
    fn default() -> Self {
        Self {
            name: String::new(),
            transform: Transform::default(),
            vertices: Vec::new(),
            normals: Vec::new(),
            colors: Vec::new(),
            tex_coords: Vec::new(),
            triangles: Vec::new(),
            diffuse_texture: String::new(),
            normal_texture: String::new(),
            // The C++ struct initialises both refs to 0xFFFFFFFFu: "no
            // property attached" is a sentinel, not block zero.
            material_property_index: u32::MAX,
            texturing_property_index: u32::MAX,
        }
    }
}

// The C ABI reads these straight out of the parser's own buffers, so the four
// types are plain C layouts by contract: a `const float *` into a
// `Vec<Vector3>` may only be handed across if three `f32` fields occupy twelve
// bytes with no padding.
const _: () = assert!(size_of::<Vector3>() == 12);
const _: () = assert!(size_of::<Vector4>() == 16);
const _: () = assert!(size_of::<Matrix3x3>() == 36);
const _: () = assert!(size_of::<Transform>() == 52);

/// Types that derive from NiObjectNET and therefore start their block body
/// with an inline object name. Only the scene-graph types are listed, which
/// covers block 0 of every retail Oblivion mesh.
fn is_named_block_type(type_name: &str) -> bool {
    matches!(
        type_name,
        "NiNode"
            | "NiTriShape"
            | "NiTriStrips"
            | "NiBSAnimationNode"
            | "BSFadeNode"
            | "NiBillboardNode"
            | "NiLODNode"
            | "NiSwitchNode"
            | "NiSortAdjustNode"
            | "NiBSBoneLODController"
    )
}

/// One parsed NIF file.
///
/// The whole file stays resident: block bodies have no size table, so parsing
/// continues after [`parse_from_data`](Self::parse_from_data) returns and needs
/// random access. One parser must not be used from two threads at once, which
/// matches the original class.
#[derive(Default)]
pub struct NifParser {
    header: Header,
    nodes: Vec<Node>,
    root_node_indices: Vec<u32>,
    data: Vec<u8>,
    cursor: usize,
    walk: WalkState,
}

impl NifParser {
    pub fn new() -> Self {
        Self::default()
    }

    /// Read a whole `.nif` file into memory and parse it.
    pub fn parse_file(&mut self, path: impl AsRef<Path>) -> Result<(), NifError> {
        let data = std::fs::read(path).map_err(|_| NifError::Io)?;
        self.parse_from_data(&data)
    }

    /// Parse an in-memory NIF image. The buffer is copied, so the caller may
    /// release it afterwards.
    pub fn parse_from_data(&mut self, data: &[u8]) -> Result<(), NifError> {
        self.nodes.clear();
        self.root_node_indices.clear();
        self.data = data.to_vec();
        self.cursor = 0;
        self.header = Header::default();
        self.walk = WalkState::default();

        if self.data.is_empty() {
            self.data.clear();
            return Err(NifError::Empty);
        }

        // The reader borrows the buffer, so the header is produced in its own
        // scope: a failing header has to leave the parser holding no file
        // image, exactly like the original `parseFromData`.
        let header_result = {
            let mut reader = Reader::new(&self.data);
            let result = header::read_header(&mut reader);
            let position = reader.pos();
            (result, position)
        };
        self.header = match header_result.0 {
            Ok(header) => header,
            Err(error) => {
                self.data.clear();
                return Err(error);
            }
        };
        self.cursor = header_result.1;

        if let Err(error) = self.parse_object_array() {
            self.data.clear();
            return Err(error);
        }

        self.build_node_hierarchy();

        // Recording the byte range of every block body is best effort: a walk
        // failure only disables the block-anchored readers below, it does not
        // make the file unusable.
        let walker = Walker::new(
            &self.data,
            &mut self.header,
            &mut self.nodes,
            &mut self.walk,
        );
        let (cursor, _walk_result) = walker.run();
        self.cursor = cursor;

        // Meshes are decoded once the walk has made their bodies addressable.
        // Meshes without geometry (collision only, markers, particles)
        // legitimately report nothing here. The mesh count the original parser
        // logs is discarded: this crate does no logging.
        if self.walk.walked {
            let mut decoder =
                geometry::Decoder::new(&self.data, &self.header, &self.walk, &mut self.nodes);
            let _mesh_count = decoder.run();
        }

        Ok(())
    }

    /// Map every block index to its type and read block 0's inline object
    /// name, which is the only name reachable without walking the bodies.
    fn parse_object_array(&mut self) -> Result<(), NifError> {
        let num_objects = self.header.num_objects as usize;
        self.nodes = (0..num_objects)
            .map(|index| Node::new(index as u32))
            .collect();

        let mut reader = Reader::with_position(&self.data, self.header.block_data_offset);

        // Pre-5.0.0.1 meshes carry no type table, so block 0's own inline type
        // name is the only block type known before the walk. The remaining
        // entries are filled in by the walker, which reads the type name of
        // every block.
        if !self.header.has_block_type_table {
            let root_type_name = reader.string().map_err(|_| NifError::Corrupt)?;
            self.header.block_type_names.clear();
            self.header.block_type_names.push(root_type_name);
            self.header.block_type_indices = vec![0; num_objects];
        }

        for (index, node) in self.nodes.iter_mut().enumerate() {
            let type_index = self
                .header
                .block_type_indices
                .get(index)
                .copied()
                .unwrap_or(0);
            node.block_type_index = type_index;
            node.block_type_name = self
                .header
                .block_type_names
                .get(type_index as usize)
                .cloned()
                .unwrap_or_default();
        }

        // Block 0 is the scene root. Like every NiObjectNET-derived block it
        // starts with its inline object name, which is the only block name that
        // can be read without walking the variable-length block bodies.
        if self.nodes.is_empty() || !is_named_block_type(&self.nodes[0].block_type_name) {
            self.cursor = reader.pos();
            return Ok(());
        }

        reader.set_pos(self.header.block_data_offset);
        if !self.header.has_block_type_table {
            // The type name precedes the object name.
            reader.string().map_err(|_| NifError::Corrupt)?;
        }
        self.nodes[0].name = reader.string().map_err(|_| NifError::Corrupt)?;
        self.cursor = reader.pos();
        Ok(())
    }

    /// Collect the nodes without a parent. The geometry phase reparents them
    /// later, so at this point every node is still a root.
    fn build_node_hierarchy(&mut self) {
        self.root_node_indices = self
            .nodes
            .iter()
            .filter(|node| node.parent_index == -1)
            .map(|node| node.node_index)
            .collect();
    }

    // -----------------------------------------------------------------
    // File access
    // -----------------------------------------------------------------

    pub fn header(&self) -> &Header {
        &self.header
    }

    pub fn nodes(&self) -> &[Node] {
        &self.nodes
    }

    pub fn root_node_indices(&self) -> &[u32] {
        &self.root_node_indices
    }

    pub fn node_by_name(&self, name: &str) -> Option<&Node> {
        self.nodes.iter().find(|node| node.name == name)
    }

    /// Every decoded mesh with the block index it came from, in block order.
    ///
    /// The geometry stays where the parser put it: callers read through this
    /// view instead of taking a copy, which is what lets the C ABI hand out
    /// pointers into the parser's own buffers.
    pub fn geometry_nodes(&self) -> impl Iterator<Item = (u32, &Geometry)> {
        self.nodes
            .iter()
            .filter(|node| node.has_geometry)
            .filter_map(|node| {
                node.geometry
                    .as_ref()
                    .map(|geometry| (node.node_index, geometry))
            })
    }

    /// Every decoded mesh, in block order. Nodes whose payload could not be
    /// decoded are skipped, matching `extractAllGeometry()`.
    pub fn extract_all_geometry(&self) -> Vec<Geometry> {
        self.geometry_nodes()
            .map(|(_, geometry)| geometry.clone())
            .collect()
    }

    // -----------------------------------------------------------------
    // Skin and collision readers (block-anchored, called on demand)
    // -----------------------------------------------------------------

    /// `NiSkinInstance` of the first block of that type. `None` when the file
    /// holds none, or when the walk never ran, or when the block could not be
    /// read — the same three ways the original returns false.
    pub fn parse_skin_instance(&self) -> Option<SkinInstance> {
        block::BlockReader::new(&self.data, &self.header, &self.walk, &self.nodes)
            .parse_skin_instance()
    }

    /// `NiSkinData` of the first block of that type.
    pub fn parse_skin_data(&self) -> Option<SkinData> {
        block::BlockReader::new(&self.data, &self.header, &self.walk, &self.nodes).parse_skin_data()
    }

    /// `NiSkinPartition` of the first block of that type.
    pub fn parse_skin_partition(&self) -> Option<SkinPartition> {
        block::BlockReader::new(&self.data, &self.header, &self.walk, &self.nodes)
            .parse_skin_partition()
    }

    /// The first `bhkCollisionObject` / `bhkBlendCollisionObject` with its body
    /// and shape resolved. A body or shape that cannot be read stays at its
    /// default and the object still comes back, which is what the original does.
    pub fn parse_collision_object(&self) -> Option<CollisionObject> {
        block::BlockReader::new(&self.data, &self.header, &self.walk, &self.nodes)
            .parse_collision_object()
    }

    /// The first `bhkRigidBody` / `bhkRigidBodyT` in the file. `complete` is
    /// false when the layout ran past a legacy block: the values read so far
    /// are still returned, which is what the original leaves in its caller's
    /// struct.
    pub fn parse_rigid_body(&self) -> Option<RigidBodyRead> {
        block::BlockReader::new(&self.data, &self.header, &self.walk, &self.nodes)
            .parse_rigid_body()
    }

    /// The first shape-derived block in the file.
    pub fn parse_shape(&self) -> Option<CollisionShape> {
        block::BlockReader::new(&self.data, &self.header, &self.walk, &self.nodes).parse_shape()
    }

    /// The first `NiControllerManager` of the file with every sequence it
    /// references resolved. A sequence that cannot be read is skipped, which
    /// is what the original does — it logs and keeps the ones that worked.
    pub fn parse_controller_manager(&self) -> Option<ControllerManager> {
        block::BlockReader::new(&self.data, &self.header, &self.walk, &self.nodes)
            .parse_controller_manager()
    }

    /// The first `NiControllerSequence` of the file, without the manager
    /// around it.
    pub fn parse_controller_sequence(&self) -> Option<ControllerSequence> {
        block::BlockReader::new(&self.data, &self.header, &self.walk, &self.nodes)
            .parse_controller_sequence()
    }

    /// The first keyframe data block of the file, whichever of the two names
    /// it carries: `NiKeyframeData` on the .nif streams, `NiTransformData`
    /// on the .kf streams the retail animations ship.
    pub fn parse_keyframe_data(&self) -> Option<KeyframeData> {
        block::BlockReader::new(&self.data, &self.header, &self.walk, &self.nodes)
            .parse_keyframe_data()
    }

    /// Block count of the object array.
    pub fn block_count(&self) -> u32 {
        self.header.block_type_indices.len() as u32
    }

    /// Resolved type name of one block, empty when the index is out of range.
    pub fn block_type_name(&self, index: u32) -> String {
        self.header
            .block_type_indices
            .get(index as usize)
            .and_then(|type_index| self.header.block_type_names.get(*type_index as usize))
            .cloned()
            .unwrap_or_default()
    }

    /// True when any block of the file carries this type.
    pub fn has_block_type(&self, type_name: &str) -> bool {
        (0..self.block_count()).any(|index| self.block_type_name(index) == type_name)
    }

    /// File offset at which block 0 begins.
    pub fn block_data_offset(&self) -> usize {
        self.header.block_data_offset
    }

    /// Size of the resident file image.
    pub fn file_size(&self) -> usize {
        self.data.len()
    }

    /// Current read position, kept in step with the last phase that read.
    pub fn cursor(&self) -> usize {
        self.cursor
    }

    pub fn set_cursor(&mut self, offset: usize) {
        self.cursor = offset;
    }

    // -----------------------------------------------------------------
    // Block body walker
    // -----------------------------------------------------------------

    /// Re-run the walk on the resident file image. `parse_from_data` already
    /// does this once; calling it again is how the original API exposes the
    /// walker to callers that changed the cursor.
    pub fn walk_all_blocks(&mut self) -> Result<(), String> {
        let walker = Walker::new(
            &self.data,
            &mut self.header,
            &mut self.nodes,
            &mut self.walk,
        );
        let (cursor, result) = walker.run();
        self.cursor = cursor;
        result
    }

    pub fn are_blocks_walked(&self) -> bool {
        self.walk.walked
    }

    /// Per-block leading `u32` on 10.0.1.x / 10.1.0.x meshes, 4 or 0.
    pub fn block_prefix(&self) -> u32 {
        self.walk.block_prefix
    }

    /// The prefix this block type carries on this file's version.
    pub fn block_body_prefix(&self, type_name: &str) -> u32 {
        walk::block_body_prefix(self.header.version, self.walk.block_prefix, type_name)
    }

    /// Body start of a block, prefix included. Zero when out of range.
    pub fn block_body_offset(&self, index: u32) -> usize {
        self.walk.offsets.get(index as usize).copied().unwrap_or(0)
    }

    /// One past the last body byte of a block. Zero when out of range.
    pub fn block_body_end(&self, index: u32) -> usize {
        self.walk.ends.get(index as usize).copied().unwrap_or(0)
    }

    /// First field of a block body, past the per-block prefix.
    ///
    /// Returns `None` until the file has been walked: a walk that stopped early
    /// leaves partial ranges behind, and the original reader refuses to hand
    /// out any of them.
    pub fn locate_block_body(&self, index: u32) -> Option<usize> {
        // One locate rule for the whole crate: the geometry decoder, the skin
        // and collision readers and this accessor all go through here.
        walk::locate_block(&self.walk, &self.header, index)
    }

    /// Indices of every block of one type, in file order. Empty when the file
    /// has not been walked.
    pub fn find_blocks_of_type(&self, type_name: &str) -> Vec<u32> {
        if !self.walk.walked {
            return Vec::new();
        }
        (0..self.block_count())
            .filter(|index| self.block_type_name(*index) == type_name)
            .collect()
    }

    /// Human-readable reason the last walk stopped; empty when it succeeded.
    pub fn walk_error(&self) -> &str {
        &self.walk.error
    }

    /// Value count word of the file trailer.
    pub fn trailer_value_count(&self) -> u32 {
        self.walk.trailer_value_count
    }

    /// Tool data skipped before a repeated trailer.
    pub fn trailing_data_size(&self) -> usize {
        self.walk.trailing_data_size
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn sized(text: &str) -> Vec<u8> {
        let mut out = (text.len() as u32).to_le_bytes().to_vec();
        out.extend_from_slice(text.as_bytes());
        out
    }

    /// BzString: u8 length counting the terminating NUL, then the body.
    fn bz(text: &str) -> Vec<u8> {
        let mut out = vec![text.len() as u8 + 1];
        out.extend_from_slice(text.as_bytes());
        out.push(0);
        out
    }

    fn magic_line(text: &str) -> Vec<u8> {
        let mut out = text.as_bytes().to_vec();
        out.push(b'\n');
        out
    }

    /// Gamebryo header: version, optional endian byte, user version, block
    /// count, BS version, three script strings, type table, index array and the
    /// empty group tail.
    fn gamebryo_header(
        magic: &str,
        version: u32,
        user_version: u32,
        unknown: u32,
        type_names: &[&str],
        type_indices: &[u16],
    ) -> Vec<u8> {
        let mut out = magic_line(magic);
        out.extend_from_slice(&version.to_le_bytes());
        if version >= 0x1400_0000 {
            out.push(1); // endian byte
        }
        out.extend_from_slice(&user_version.to_le_bytes());
        out.extend_from_slice(&(type_indices.len() as u32).to_le_bytes());
        out.extend_from_slice(&unknown.to_le_bytes());
        out.extend_from_slice(&bz("ccafasso"));
        out.extend_from_slice(&bz(""));
        out.extend_from_slice(&bz(""));
        out.extend_from_slice(&(type_names.len() as u16).to_le_bytes());
        for name in type_names {
            out.extend_from_slice(&sized(name));
        }
        for index in type_indices {
            out.extend_from_slice(&index.to_le_bytes());
        }
        out.extend_from_slice(&0u32.to_le_bytes()); // numGroups
        out
    }

    /// Compact NetImmerse header: no endian byte, no user version, no script
    /// strings; the block count follows the version.
    fn netimmerse_header(
        magic: &str,
        version: u32,
        type_names: &[&str],
        type_indices: &[u16],
    ) -> Vec<u8> {
        let mut out = magic_line(magic);
        out.extend_from_slice(&version.to_le_bytes());
        out.extend_from_slice(&(type_indices.len() as u32).to_le_bytes());
        if version == 0x0A00_0102 {
            out.extend_from_slice(&5u32.to_le_bytes());
            out.extend_from_slice(&bz(""));
            out.extend_from_slice(&bz(""));
            out.extend_from_slice(&bz(""));
        }
        out.extend_from_slice(&(type_names.len() as u16).to_le_bytes());
        for name in type_names {
            out.extend_from_slice(&sized(name));
        }
        for index in type_indices {
            out.extend_from_slice(&index.to_le_bytes());
        }
        out.extend_from_slice(&0u32.to_le_bytes()); // numGroups
        out
    }

    /// Pre-5.0.0.1 header: version and block count, nothing else.
    fn pre5_header(magic: &str, version: u32, num_objects: u32) -> Vec<u8> {
        let mut out = magic_line(magic);
        out.extend_from_slice(&version.to_le_bytes());
        out.extend_from_slice(&num_objects.to_le_bytes());
        out
    }

    /// Assembles a file and remembers where every block body landed, which is
    /// what the walker's ranges are asserted against.
    struct Built {
        bytes: Vec<u8>,
        blocks: Vec<(usize, usize)>,
    }

    impl Built {
        fn with_header(header: Vec<u8>) -> Self {
            Self {
                bytes: header,
                blocks: Vec::new(),
            }
        }

        fn block(&mut self, body: Vec<u8>) {
            let start = self.bytes.len();
            self.bytes.extend_from_slice(&body);
            self.blocks.push((start, self.bytes.len()));
        }

        fn trailer(mut self, values: &[u32]) -> (Vec<u8>, Vec<(usize, usize)>) {
            self.bytes
                .extend_from_slice(&(values.len() as u32).to_le_bytes());
            for value in values {
                self.bytes.extend_from_slice(&value.to_le_bytes());
            }
            (self.bytes, self.blocks)
        }
    }

    /// NiNode body of a type-table file: name, extra data list, controller,
    /// flags, transform, properties, collision object and the two counts.
    fn ninode_table(name: &str) -> Vec<u8> {
        let mut body = sized(name);
        body.extend_from_slice(&0u32.to_le_bytes()); // extra data count
        body.extend_from_slice(&0xFFFF_FFFFu32.to_le_bytes()); // controller
        body.extend_from_slice(&0u16.to_le_bytes()); // flags
        body.extend_from_slice(&[0u8; 12]); // translation
        body.extend_from_slice(&[0u8; 36]); // rotation
        body.extend_from_slice(&1.0f32.to_le_bytes()); // scale
        body.extend_from_slice(&0u32.to_le_bytes()); // property count
        body.extend_from_slice(&0xFFFF_FFFFu32.to_le_bytes()); // collision object
        body.extend_from_slice(&0u32.to_le_bytes()); // child count
        body.extend_from_slice(&0u32.to_le_bytes()); // effect count
        body
    }

    /// NiNode body of a pre-5.0.0.1 file: inline type name, single extra data
    /// and controller refs, velocity, and a 32-bit bounding volume flag.
    fn ninode_legacy(name: &str) -> Vec<u8> {
        let mut body = sized("NiNode");
        body.extend_from_slice(&sized(name));
        body.extend_from_slice(&0xFFFF_FFFFu32.to_le_bytes()); // extra data
        body.extend_from_slice(&0xFFFF_FFFFu32.to_le_bytes()); // controller
        body.extend_from_slice(&0u16.to_le_bytes()); // flags
        body.extend_from_slice(&[0u8; 12]); // translation
        body.extend_from_slice(&[0u8; 36]); // rotation
        body.extend_from_slice(&1.0f32.to_le_bytes()); // scale
        body.extend_from_slice(&[0u8; 12]); // velocity
        body.extend_from_slice(&0u32.to_le_bytes()); // property count
        body.extend_from_slice(&0u32.to_le_bytes()); // has bounding volume
        body.extend_from_slice(&0u32.to_le_bytes()); // child count
        body.extend_from_slice(&0u32.to_le_bytes()); // effect count
        body
    }

    fn bhk_sphere() -> Vec<u8> {
        let mut body = 0u32.to_le_bytes().to_vec(); // material
        body.extend_from_slice(&1.0f32.to_le_bytes()); // radius
        body
    }

    /// A body as the 10.0.1.x / 10.1.0.x walker expects it: one leading zero
    /// word in front of the record.
    fn prefixed(mut body: Vec<u8>) -> Vec<u8> {
        let mut out = 0u32.to_le_bytes().to_vec();
        out.append(&mut body);
        out
    }

    /// bhkCollisionObject is the documented exception: it starts straight at
    /// its Target reference, prefix or no prefix.
    fn bhk_collision_object() -> Vec<u8> {
        let mut body = 0xFFFF_FFFFu32.to_le_bytes().to_vec(); // target
        body.extend_from_slice(&0u16.to_le_bytes()); // flags
        body.extend_from_slice(&0u32.to_le_bytes()); // body
        body
    }

    fn push_u16(out: &mut Vec<u8>, value: u16) {
        out.extend_from_slice(&value.to_le_bytes());
    }

    fn push_u32(out: &mut Vec<u8>, value: u32) {
        out.extend_from_slice(&value.to_le_bytes());
    }

    fn push_f32(out: &mut Vec<u8>, value: f32) {
        out.extend_from_slice(&value.to_le_bytes());
    }

    /// NiTriShape / NiTriStrips scene block of a type-table file: name, extra
    /// data, controller, flags, transform, properties, collision object, then
    /// the data / skin / shader fields of NiGeometry.
    ///
    /// The transform is fixed so the decode tests can assert it: translation
    /// (1, 2, 3), identity rotation, scale 2.
    fn shape_body(name: &str, data_ref: u32, property_refs: &[u32]) -> Vec<u8> {
        let mut body = sized(name);
        push_u32(&mut body, 0); // extra data count
        push_u32(&mut body, 0xFFFF_FFFF); // controller
        push_u16(&mut body, 0); // flags
        for value in [1.0, 2.0, 3.0] {
            push_f32(&mut body, value); // translation
        }
        for value in [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0] {
            push_f32(&mut body, value); // rotation
        }
        push_f32(&mut body, 2.0); // scale
        push_u32(&mut body, property_refs.len() as u32);
        for property_ref in property_refs {
            push_u32(&mut body, *property_ref);
        }
        push_u32(&mut body, 0xFFFF_FFFF); // collision object
        push_u32(&mut body, data_ref);
        push_u32(&mut body, 0xFFFF_FFFF); // skin instance
        body.push(0); // no shader
        body
    }

    /// Shared NiGeometryData header: group id, vertex counts and flags, the
    /// optional vertex arrays, the bounding sphere and the UV set count.
    fn shape_data_header(
        body: &mut Vec<u8>,
        vertices: &[[f32; 3]],
        normals: Option<&[[f32; 3]]>,
        colors: Option<&[[f32; 4]]>,
        uvs: Option<&[[f32; 2]]>,
    ) {
        push_u32(body, 0); // group id (since 10.1.0.114)
        push_u16(body, vertices.len() as u16);
        body.push(0); // keep flags
        body.push(0); // compress flags
        body.push(1); // has vertices
        for vertex in vertices {
            for value in vertex {
                push_f32(body, *value);
            }
        }
        push_u16(body, if uvs.is_some() { 1 } else { 0 }); // data flags: one UV set
        body.push(if normals.is_some() { 1 } else { 0 });
        if let Some(list) = normals {
            for normal in list {
                for value in normal {
                    push_f32(body, *value);
                }
            }
        }
        body.extend_from_slice(&[0u8; 16]); // bounding sphere
        body.push(if colors.is_some() { 1 } else { 0 });
        if let Some(list) = colors {
            for color in list {
                for value in color {
                    push_f32(body, *value);
                }
            }
        }
        if let Some(list) = uvs {
            for uv in list {
                for value in uv {
                    push_f32(body, *value);
                }
            }
        }
        push_u16(body, 0); // consistency flags
        push_u32(body, 0); // additional data (since 20.0.0.4)
    }

    /// NiTriShapeData payload: the shared header plus the triangle tail.
    fn shape_data_body(
        vertices: &[[f32; 3]],
        normals: Option<&[[f32; 3]]>,
        colors: Option<&[[f32; 4]]>,
        uvs: Option<&[[f32; 2]]>,
        triangles: &[[u16; 3]],
    ) -> Vec<u8> {
        let mut body = Vec::new();
        shape_data_header(&mut body, vertices, normals, colors, uvs);
        push_u16(&mut body, triangles.len() as u16);
        push_u32(&mut body, triangles.len() as u32 * 3); // num triangle points
        body.push(1); // has triangles (since 10.1.0.0)
        for triangle in triangles {
            for index in triangle {
                push_u16(&mut body, *index);
            }
        }
        push_u16(&mut body, 0); // match groups
        body
    }

    /// NiTriStripsData payload: the shared header plus the strip tail.
    fn strips_data_body(
        vertices: &[[f32; 3]],
        uvs: Option<&[[f32; 2]]>,
        strip_lengths: &[u16],
        points: &[u16],
    ) -> Vec<u8> {
        let mut body = Vec::new();
        shape_data_header(&mut body, vertices, None, None, uvs);
        push_u16(&mut body, 0); // num triangles
        push_u16(&mut body, strip_lengths.len() as u16);
        for length in strip_lengths {
            push_u16(&mut body, *length);
        }
        body.push(1); // has points (since 10.1.0.03)
        for point in points {
            push_u16(&mut body, *point);
        }
        body
    }

    /// NiTexturingProperty with a single base texture slot pointing at
    /// `source_ref`. Its TexDesc is what the walker measures and the resolver
    /// reads the first field of.
    fn texturing_property_body(source_ref: u32) -> Vec<u8> {
        let mut body = sized("");
        push_u32(&mut body, 0); // extra data count
        push_u32(&mut body, 0xFFFF_FFFF); // controller
        push_u32(&mut body, 0); // apply mode
        push_u32(&mut body, 1); // texture count
        body.push(1); // base texture present
        push_u32(&mut body, source_ref); // TexDesc: source
        push_u32(&mut body, 0); // clamp mode
        push_u32(&mut body, 0); // filter mode
        push_u32(&mut body, 0); // uv set
        body.push(0); // no texture transform
        for _ in 0..4 {
            body.push(0); // slots 1..5 absent
        }
        push_u32(&mut body, 0); // shader texture count
        body
    }

    /// NiSourceTexture holding an external path. The backslashes are what
    /// retail archives store and what the resolver normalises away.
    fn source_texture_body(path: &str) -> Vec<u8> {
        let mut body = sized("");
        push_u32(&mut body, 0); // extra data count
        push_u32(&mut body, 0xFFFF_FFFF); // controller
        body.push(1); // use external
        body.extend_from_slice(&sized(path));
        push_u32(&mut body, 0xFFFF_FFFF); // pixel data
        body.extend_from_slice(&[0u8; 12]); // format prefs
        body.push(0); // is static
        body.push(0); // direct render (since 10.1.0.103)
        body
    }

    /// NiTexturingProperty that declares no texture slots at all, so the
    /// resolver stops at the texture count.
    fn empty_texturing_property_body() -> Vec<u8> {
        let mut body = sized("");
        push_u32(&mut body, 0); // extra data count
        push_u32(&mut body, 0xFFFF_FFFF); // controller
        push_u32(&mut body, 0); // apply mode
        push_u32(&mut body, 0); // texture count
        for _ in 0..5 {
            body.push(0); // the walker reads all five slot flags regardless
        }
        push_u32(&mut body, 0); // shader texture count
        body
    }

    fn parse(bytes: &[u8]) -> NifParser {
        let mut parser = NifParser::new();
        parser.parse_from_data(bytes).expect("fixture parses");
        parser
    }

    #[test]
    fn gamebryo_20_header_and_block_walk() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiNode", "bhkSphereShape"],
            &[0, 1],
        );
        let mut built = Built::with_header(header);
        let header_len = built.bytes.len();
        built.block(ninode_table("Scene"));
        built.block(bhk_sphere());
        let (bytes, blocks) = built.trailer(&[0]);

        let parser = parse(&bytes);
        let header = parser.header();
        assert_eq!(header.version, 0x1400_0005);
        assert!(header.has_endian_byte);
        assert_eq!(header.endian, 1);
        assert!(!header.legacy_net_immerse);
        assert!(header.has_block_type_table);
        assert_eq!(header.user_version, 11);
        assert_eq!(header.unknown_field, 11);
        assert_eq!(header.creator, "ccafasso");
        assert_eq!(header.num_objects, 2);
        assert_eq!(header.num_groups, 0);
        assert_eq!(header.block_type_names, vec!["NiNode", "bhkSphereShape"]);
        assert_eq!(header.block_data_offset, header_len);

        assert_eq!(parser.block_count(), 2);
        assert_eq!(parser.nodes().len(), 2);
        assert_eq!(parser.nodes()[0].name, "Scene");
        assert_eq!(parser.nodes()[0].block_type_name, "NiNode");
        assert_eq!(parser.nodes()[1].block_type_name, "bhkSphereShape");
        assert_eq!(parser.root_node_indices(), &[0, 1]);
        assert_eq!(parser.node_by_name("Scene").unwrap().node_index, 0);

        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        assert_eq!(parser.walk_error(), "");
        assert_eq!(parser.block_prefix(), 0);
        assert_eq!(parser.block_body_offset(0), blocks[0].0);
        assert_eq!(parser.block_body_end(0), blocks[0].1);
        assert_eq!(parser.block_body_offset(1), blocks[1].0);
        assert_eq!(parser.block_body_end(1), blocks[1].1);
        assert_eq!(parser.locate_block_body(0), Some(blocks[0].0));
        assert_eq!(parser.locate_block_body(1), Some(blocks[1].0));
        assert_eq!(parser.find_blocks_of_type("bhkSphereShape"), vec![1]);
        assert!(parser.has_block_type("NiNode"));
        assert!(!parser.has_block_type("NiCamera"));
        assert_eq!(parser.trailer_value_count(), 1);
        assert_eq!(parser.trailing_data_size(), 0);
        assert_eq!(parser.cursor(), bytes.len());
        assert_eq!(parser.file_size(), bytes.len());
        // Neither block is a mesh, so nothing is decoded.
        assert!(parser.extract_all_geometry().is_empty());
    }

    #[test]
    fn gamebryo_10_2_header_has_no_endian_byte_and_no_prefix() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 10.2.0.0",
            0x0A02_0000,
            0,
            6,
            &["NiNode", "bhkSphereShape"],
            &[0, 1],
        );
        let mut built = Built::with_header(header);
        built.block(ninode_table("Root"));
        built.block(bhk_sphere());
        let (bytes, blocks) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(!parser.header().has_endian_byte);
        assert_eq!(parser.header().endian, 0);
        assert_eq!(parser.header().unknown_field, 6);
        assert_eq!(parser.block_prefix(), 0);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        assert_eq!(parser.block_body_offset(0), blocks[0].0);
        assert_eq!(parser.block_body_end(1), blocks[1].1);
        assert_eq!(parser.nodes()[0].name, "Root");
    }

    #[test]
    fn gamebryo_10_1_prefixes_every_body_and_is_skipped_by_locate() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 10.1.0.106",
            0x0A01_006A,
            0,
            5,
            &["NiNode", "bhkSphereShape"],
            &[0, 1],
        );
        let mut built = Built::with_header(header);
        built.block(prefixed(ninode_table("Scene")));
        built.block(prefixed(bhk_sphere()));
        let (bytes, blocks) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(!parser.header().has_endian_byte);
        assert_eq!(parser.block_prefix(), 4);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        assert_eq!(parser.block_body_offset(0), blocks[0].0);
        assert_eq!(parser.block_body_end(1), blocks[1].1);
        // locate() points at the first field, past the leading word.
        assert_eq!(parser.locate_block_body(0), Some(blocks[0].0 + 4));
        assert_eq!(parser.locate_block_body(1), Some(blocks[1].0 + 4));
        assert_eq!(parser.block_body_prefix("NiNode"), 4);
        // The object-name read in parseObjectArray does not skip the per-block
        // prefix, so a prefixed stream reads the zero word as an empty name —
        // the original parser reports the same empty root name.
        assert_eq!(parser.nodes()[0].name, "");
    }

    #[test]
    fn netimmerse_header_reads_the_block_count_straight_after_the_version() {
        let header = netimmerse_header(
            "NetImmerse File Format, Version 10.0.1.0",
            0x0A00_0100,
            &["NiNode", "bhkSphereShape"],
            &[0, 1],
        );
        let mut built = Built::with_header(header);
        built.block(prefixed(ninode_table("Legacy")));
        built.block(prefixed(bhk_sphere()));
        let (bytes, blocks) = built.trailer(&[0]);

        let parser = parse(&bytes);
        let header = parser.header();
        assert!(header.legacy_net_immerse);
        assert!(!header.has_endian_byte);
        assert_eq!(header.user_version, 0);
        assert_eq!(header.unknown_field, 0);
        assert_eq!(header.creator, "");
        assert_eq!(header.num_objects, 2);
        // 10.0.1.0 sits inside the prefixed window.
        assert_eq!(parser.block_prefix(), 4);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        assert_eq!(parser.block_body_offset(0), blocks[0].0);
        assert_eq!(parser.block_body_end(1), blocks[1].1);
        // The name read does not skip the per-block prefix, so the root name
        // comes back empty on this family, exactly as the C++ parser reports it.
        assert_eq!(parser.nodes()[0].name, "");
    }

    #[test]
    fn netimmerse_10_0_1_2_carries_the_bethesda_stream_header() {
        let header = netimmerse_header(
            "NetImmerse File Format, Version 10.0.1.2",
            0x0A00_0102,
            &["NiNode"],
            &[0],
        );
        let mut built = Built::with_header(header);
        built.block(prefixed(ninode_table("Streamed")));
        let (bytes, blocks) = built.trailer(&[0]);

        let parser = parse(&bytes);
        let header = parser.header();
        assert!(header.legacy_net_immerse);
        // The single NetImmerse version that carries the BS stream header: one
        // unknown uint followed by three byte strings.
        assert_eq!(header.unknown_field, 5);
        assert_eq!(header.num_objects, 1);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        assert_eq!(parser.block_body_offset(0), blocks[0].0);
        assert_eq!(parser.block_body_end(0), blocks[0].1);
    }

    #[test]
    fn pre_five_header_builds_its_type_table_from_inline_names() {
        let header = pre5_header("Gamebryo File Format, Version 4.0.0.2", 0x0400_0002, 2);
        let mut built = Built::with_header(header);
        built.block(ninode_legacy("Marker"));
        // Every block of a pre-5.0.0.1 mesh opens with its own type name.
        let mut sphere = sized("bhkSphereShape");
        sphere.extend_from_slice(&bhk_sphere());
        built.block(sphere);
        let (bytes, blocks) = built.trailer(&[0]);

        let parser = parse(&bytes);
        let header = parser.header();
        assert!(!header.has_block_type_table);
        assert_eq!(header.num_objects, 2);
        assert_eq!(header.num_groups, 0);
        // The header itself carries no type table; the walk below rebuilds it.

        assert_eq!(parser.block_type_name(0), "NiNode");
        assert_eq!(parser.nodes()[0].name, "Marker");

        // The walk rebuilds the table from each block's inline type name.
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        assert_eq!(parser.block_type_name(0), "NiNode");
        assert_eq!(parser.block_type_name(1), "bhkSphereShape");
        assert_eq!(parser.block_prefix(), 0);
        assert_eq!(parser.block_body_offset(0), blocks[0].0);
        assert_eq!(parser.block_body_end(1), blocks[1].1);
        assert_eq!(parser.find_blocks_of_type("bhkSphereShape"), vec![1]);
    }

    #[test]
    fn collision_objects_opt_out_of_the_ten_bit_prefix() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 10.1.0.106",
            0x0A01_006A,
            0,
            5,
            &["bhkCollisionObject"],
            &[0],
        );
        let mut built = Built::with_header(header);
        built.block(bhk_collision_object()); // no leading word on purpose
        let (bytes, blocks) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert_eq!(parser.block_prefix(), 4);
        assert_eq!(parser.block_body_prefix("bhkCollisionObject"), 0);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        assert_eq!(parser.block_body_offset(0), blocks[0].0);
        assert_eq!(parser.block_body_end(0), blocks[0].1);
        assert_eq!(parser.locate_block_body(0), Some(blocks[0].0));
        // Block 0 is not a NiObjectNET type, so it carries no object name.
        assert_eq!(parser.nodes()[0].name, "");
    }

    #[test]
    fn trailer_with_repeated_terminator_accepts_trailing_tool_data() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiNode"],
            &[0],
        );
        let mut built = Built::with_header(header);
        built.block(ninode_table("Scene"));
        let (mut bytes, _) = built.trailer(&[0]);

        bytes.extend_from_slice(&[0xAA; 12]); // tool data between the trailers
        bytes.extend_from_slice(&1u32.to_le_bytes()); // repeated value count
        bytes.extend_from_slice(&0u32.to_le_bytes());

        let parser = parse(&bytes);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        assert_eq!(parser.trailer_value_count(), 1);
        // Everything between the first trailer and the end of file counts as
        // trailing data: the tool data plus the repeated trailer itself.
        assert_eq!(parser.trailing_data_size(), 20);
        assert_eq!(parser.cursor(), bytes.len());
    }

    #[test]
    fn trailer_without_repeated_terminator_fails_the_walk() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiNode"],
            &[0],
        );
        let mut built = Built::with_header(header);
        built.block(ninode_table("Scene"));
        let (mut bytes, _) = built.trailer(&[0]);
        bytes.extend_from_slice(&[0xAA; 12]);

        // The parse itself still succeeds: a failed walk only disables the
        // block-anchored readers.
        let parser = parse(&bytes);
        assert!(!parser.are_blocks_walked());
        assert!(parser.walk_error().starts_with("unrecognised file trailer"));
        assert_eq!(parser.trailing_data_size(), 0);
        assert_eq!(parser.find_blocks_of_type("NiNode"), Vec::<u32>::new());
    }

    #[test]
    fn unknown_block_type_stops_the_walk_but_not_the_parse() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiNode", "MysteryBlock"],
            &[0, 1],
        );
        let mut built = Built::with_header(header);
        built.block(ninode_table("Scene"));
        built.block(vec![1, 2, 3, 4]);
        let (bytes, _) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(!parser.are_blocks_walked());
        assert_eq!(parser.walk_error(), "cannot measure block 1 (MysteryBlock)");
        assert_eq!(parser.nodes()[0].name, "Scene");
        // Block 0 was measured before the walk stopped, but a failed walk
        // hands out no block-anchored reads at all.
        assert!(parser.locate_block_body(0).is_none());
        assert_eq!(parser.find_blocks_of_type("NiNode"), Vec::<u32>::new());
    }

    #[test]
    fn empty_buffer_is_rejected() {
        let mut parser = NifParser::new();
        assert_eq!(parser.parse_from_data(&[]), Err(NifError::Empty));
    }

    #[test]
    fn non_nif_magic_is_rejected() {
        let mut bytes = magic_line("Not a NIF file");
        bytes.extend_from_slice(&0x1400_0005u32.to_le_bytes());
        let mut parser = NifParser::new();
        assert_eq!(parser.parse_from_data(&bytes), Err(NifError::BadMagic));
    }

    #[test]
    fn truncated_header_is_rejected() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiNode", "bhkSphereShape"],
            &[0, 1],
        );
        let bytes = &header[..header.len() - 3];
        let mut parser = NifParser::new();
        assert!(matches!(
            parser.parse_from_data(bytes),
            Err(NifError::Corrupt) | Err(NifError::Truncated)
        ));
    }

    #[test]
    fn out_of_range_block_type_index_is_rejected() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiNode"],
            &[0, 5],
        );
        let mut parser = NifParser::new();
        assert_eq!(parser.parse_from_data(&header), Err(NifError::Corrupt));
    }

    #[test]
    fn zero_block_count_is_rejected() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiNode"],
            &[],
        );
        let mut parser = NifParser::new();
        assert_eq!(parser.parse_from_data(&header), Err(NifError::Corrupt));
    }

    #[test]
    fn walking_a_second_file_clears_the_first_files_ranges() {
        let good = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiNode"],
            &[0],
        );
        let mut built = Built::with_header(good);
        built.block(ninode_table("Scene"));
        let (good_bytes, _) = built.trailer(&[0]);
        let parser = parse(&good_bytes);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        assert_ne!(parser.block_body_offset(0), 0);

        // A file whose walk fails before the first block must not keep the
        // previous file's ranges visible.
        let bad = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["MysteryBlock"],
            &[0],
        );
        let mut built = Built::with_header(bad);
        built.block(vec![1, 2, 3, 4]);
        let (bad_bytes, _) = built.trailer(&[0]);

        let parser = parse(&bad_bytes);
        assert!(!parser.are_blocks_walked());
        assert_eq!(parser.walk_error(), "cannot measure block 0 (MysteryBlock)");
        assert_eq!(parser.block_body_offset(0), 0);
        assert!(parser.locate_block_body(0).is_none());
    }

    // -----------------------------------------------------------------
    // N2: geometry decode
    // -----------------------------------------------------------------

    #[test]
    fn shape_data_decodes_vertices_colors_uvs_and_triangles() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiTriShape", "NiTriShapeData"],
            &[0, 1],
        );
        let mut built = Built::with_header(header);
        built.block(shape_body("Tri", 1, &[]));
        built.block(shape_data_body(
            &[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
            Some(&[[0.0, 0.0, 1.0], [0.0, 0.0, 1.0], [0.0, 0.0, 1.0]]),
            Some(&[
                [1.0, 0.0, 0.0, 1.0],
                [0.0, 1.0, 0.0, 1.0],
                [0.0, 0.0, 1.0, 0.5],
            ]),
            Some(&[[0.0, 0.0], [1.0, 0.0], [0.0, 1.0]]),
            &[[0, 1, 2]],
        ));
        let (bytes, _) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());

        let shape = &parser.nodes()[0];
        assert_eq!(shape.name, "Tri");
        assert!(shape.has_geometry);
        let geometry = shape.geometry.as_ref().expect("shape carries a mesh");

        assert_eq!(
            geometry.vertices,
            vec![
                Vector3 {
                    x: 0.0,
                    y: 0.0,
                    z: 0.0
                },
                Vector3 {
                    x: 1.0,
                    y: 0.0,
                    z: 0.0
                },
                Vector3 {
                    x: 0.0,
                    y: 1.0,
                    z: 0.0
                },
            ]
        );
        assert_eq!(
            geometry.normals,
            vec![
                Vector3 {
                    x: 0.0,
                    y: 0.0,
                    z: 1.0
                },
                Vector3 {
                    x: 0.0,
                    y: 0.0,
                    z: 1.0
                },
                Vector3 {
                    x: 0.0,
                    y: 0.0,
                    z: 1.0
                },
            ]
        );
        assert_eq!(
            geometry.colors,
            vec![
                Vector4 {
                    x: 1.0,
                    y: 0.0,
                    z: 0.0,
                    w: 1.0
                },
                Vector4 {
                    x: 0.0,
                    y: 1.0,
                    z: 0.0,
                    w: 1.0
                },
                Vector4 {
                    x: 0.0,
                    y: 0.0,
                    z: 1.0,
                    w: 0.5
                },
            ]
        );
        assert_eq!(
            geometry.tex_coords,
            vec![[0.0, 0.0], [1.0, 0.0], [0.0, 1.0]]
        );
        assert_eq!(geometry.triangles, vec![[0u16, 1, 2]]);

        // NiAVObject transform, read past the two flag bytes.
        assert_eq!(
            geometry.transform.translation,
            Vector3 {
                x: 1.0,
                y: 2.0,
                z: 3.0
            }
        );
        assert_eq!(geometry.transform.scale, 2.0);
        assert_eq!(geometry.transform.rotation, Matrix3x3::default());

        // No properties were attached.
        assert_eq!(geometry.material_property_index, u32::MAX);
        assert_eq!(geometry.texturing_property_index, u32::MAX);
        assert_eq!(geometry.diffuse_texture, "");

        // The data block itself is not a mesh node.
        assert!(!parser.nodes()[1].has_geometry);

        let all = parser.extract_all_geometry();
        assert_eq!(all.len(), 1);
        assert_eq!(all[0], *geometry);
    }

    #[test]
    fn tri_strips_expand_into_alternately_wound_triangles() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiTriStrips", "NiTriStripsData"],
            &[0, 1],
        );
        let mut built = Built::with_header(header);
        built.block(shape_body("Strip", 1, &[]));
        built.block(strips_data_body(
            &[
                [0.0, 0.0, 0.0],
                [1.0, 0.0, 0.0],
                [1.0, 1.0, 0.0],
                [0.0, 1.0, 0.0],
            ],
            Some(&[[0.0, 0.0], [1.0, 0.0], [1.0, 1.0], [0.0, 1.0]]),
            &[4],
            &[0, 1, 2, 3],
        ));
        let (bytes, _) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());

        let shape = &parser.nodes()[0];
        assert!(shape.has_geometry);
        let geometry = shape.geometry.as_ref().expect("shape carries a mesh");
        assert_eq!(geometry.vertices.len(), 4);
        assert_eq!(geometry.tex_coords.len(), 4);
        assert_eq!(geometry.triangles, vec![[0u16, 1, 2], [2, 1, 3]]);
        assert_eq!(parser.extract_all_geometry().len(), 1);
    }

    #[test]
    fn strip_expansion_winds_every_other_triangle_backwards() {
        let mut triangles = Vec::new();
        super::geometry::append_strip_triangles(&[10, 11, 12, 13, 14], &[5], &mut triangles);
        // Even steps keep the point order, odd steps swap the shared edge.
        assert_eq!(triangles, vec![[10, 11, 12], [12, 11, 13], [12, 13, 14]]);

        // Two strips run back to back, each starting a fresh winding.
        triangles.clear();
        super::geometry::append_strip_triangles(&[0, 1, 2, 5, 6, 7], &[3, 3], &mut triangles);
        assert_eq!(triangles, vec![[0, 1, 2], [5, 6, 7]]);

        // A strip longer than the payload it claims is dropped whole.
        triangles.clear();
        super::geometry::append_strip_triangles(&[1, 2], &[3], &mut triangles);
        assert!(triangles.is_empty());
    }

    #[test]
    fn diffuse_texture_resolves_through_the_texturing_property() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &[
                "NiTriShape",
                "NiTriShapeData",
                "NiTexturingProperty",
                "NiSourceTexture",
            ],
            &[0, 1, 2, 3],
        );
        let mut built = Built::with_header(header);
        built.block(shape_body("Textured", 1, &[2]));
        built.block(shape_data_body(
            &[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
            None,
            None,
            Some(&[[0.0, 0.0], [1.0, 0.0], [0.0, 1.0]]),
            &[[0, 1, 2]],
        ));
        built.block(texturing_property_body(3));
        built.block(source_texture_body("textures\\maps\\foo.dds"));
        let (bytes, _) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());

        let geometry = parser.extract_all_geometry().remove(0);
        assert_eq!(geometry.texturing_property_index, 2);
        assert_eq!(geometry.material_property_index, u32::MAX);
        // Retail paths use backslashes; the resolver normalises them.
        assert_eq!(geometry.diffuse_texture, "textures/maps/foo.dds");
        assert_eq!(geometry.triangles, vec![[0u16, 1, 2]]);
    }

    #[test]
    fn texture_resolution_is_skipped_when_the_property_has_no_base_slot() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiTriShape", "NiTriShapeData", "NiTexturingProperty"],
            &[0, 1, 2],
        );
        let mut built = Built::with_header(header);
        built.block(shape_body("Untextured", 1, &[2]));
        built.block(shape_data_body(
            &[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]],
            None,
            None,
            None,
            &[[0, 1, 2]],
        ));
        built.block(empty_texturing_property_body());
        let (bytes, _) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());

        // The property is attached, but its texture count is zero, so the
        // resolver fails before reaching a NiSourceTexture and the shape keeps
        // an empty path — what callers treat as "untextured".
        let geometry = parser.extract_all_geometry().remove(0);
        assert_eq!(geometry.texturing_property_index, 2);
        assert_eq!(geometry.material_property_index, u32::MAX);
        assert_eq!(geometry.diffuse_texture, "");
        // No UV set in this payload, so the vertex arrays stop at the bounds.
        assert!(geometry.tex_coords.is_empty());
        assert_eq!(geometry.triangles, vec![[0u16, 1, 2]]);
    }

    // -----------------------------------------------------------------
    // N5: skin and collision readers
    // -----------------------------------------------------------------

    /// Regression for the legacy path the differential oracle caught: on
    /// NetImmerse 10.0.x the rigid body block is shorter than the layout the
    /// reader follows, so the read runs past it and stops at the constraint
    /// count. The original still leaves everything it read in the caller's
    /// struct — mass, friction, filter, velocities — and reports failure; this
    /// pins the same partial delivery, and that no shape is followed afterwards.
    #[test]
    fn legacy_collision_body_is_delivered_when_the_read_stops_partway() {
        let root = std::env::var("OBLIVION_MESHES").unwrap_or_else(|_| {
            String::from(r"C:\Users\hiroki.kogarumai\Oblivion_Android\meshes_extracted")
        });
        let path =
            format!(r"{root}\meshes\architecture\castle\kvatch\kvatch  castle int hallway01.nif");
        let mut parser = NifParser::new();
        if parser.parse_file(&path).is_err() {
            eprintln!("skipping: retail mesh not available at {path}");
            return;
        }

        assert_eq!(parser.header().version, 0x0A00_0102);

        let object = parser
            .parse_collision_object()
            .expect("the mesh carries a collision object");
        assert_eq!(object.node_index, 0);
        assert_eq!(object.rigid_body_index, 5);

        // Values read before the constraint check tripped.
        let body = &object.body_info;
        assert_eq!(body.mass.to_bits(), 0x3E99_999A);
        assert_eq!(body.friction.to_bits(), 0x1E5A_76C0);
        assert_eq!(body.restitution.to_bits(), 0x0000_0000);
        assert_eq!(body.collision_group, 1);
        assert_eq!(body.collision_filter, 0);
        assert!(!body.is_trigger);
        // The read never completed, so the shape is never followed and stays at
        // its default — the same leaves-untouched behaviour as the original.
        assert_eq!(object.shape.shape_type, ShapeType::None);

        let read = parser
            .parse_rigid_body()
            .expect("the mesh carries a rigid body block");
        assert!(!read.complete, "the layout stops before the constraints");
        assert_eq!(read.info.mass.to_bits(), 0x3E99_999A);
    }

    /// Retail coverage for the skin and collision readers: shipped bytes and
    /// structural invariants. Byte-level parity against the original is the
    /// differential oracle's job (`tmp/nif_oracle`), not this test's — here the
    /// question is only whether the readers hold their limits, their bookkeeping
    /// and their value ranges on real files.
    ///
    /// Skipped when the extraction is not on this machine: point
    /// `OBLIVION_MESHES` at `meshes_extracted`.
    #[test]
    fn retail_skin_and_collision_readers_stay_in_range() {
        let root = std::env::var("OBLIVION_MESHES").unwrap_or_else(|_| {
            String::from(r"C:\Users\hiroki.kogarumai\Oblivion_Android\meshes_extracted")
        });
        let path = format!(r"{root}\meshes\clothes\lowerclass\02\f\shirt_gnd.nif");
        let mut parser = NifParser::new();
        if parser.parse_file(&path).is_err() {
            eprintln!("skipping: retail mesh not available at {path}");
            return;
        }

        let mut exercised = 0usize;

        if parser.has_block_type("NiSkinInstance") {
            let instance = parser
                .parse_skin_instance()
                .expect("NiSkinInstance reads on a shipped mesh");
            assert!(instance.skin_data_index < parser.block_count());
            assert!(instance.bone_node_indices.len() <= 4096);

            let data = parser
                .parse_skin_data()
                .expect("NiSkinData reads on a shipped mesh");
            assert_eq!(data.num_bones as usize, data.bone_data.len());
            assert!(!data.bone_data.is_empty(), "a skinned mesh has bones");
            assert!(data.root_scale.is_finite());
            for bone in &data.bone_data {
                assert!(bone.skin_transform.scale.is_finite());
                for weight in &bone.vertex_weights {
                    assert!(weight.weight.is_finite(), "weight is not a number");
                    assert!(
                        (-0.0001..=1.0001).contains(&weight.weight),
                        "vertex weight {} is outside the normalised range",
                        weight.weight
                    );
                }
            }

            if parser.has_block_type("NiSkinPartition") {
                let partition = parser
                    .parse_skin_partition()
                    .expect("NiSkinPartition reads on a shipped mesh");
                assert!(!partition.partitions.is_empty());
                assert!(partition.max_bones_per_vertex <= 4);
                let mut palette_max = 0u32;
                for part in &partition.partitions {
                    assert!(part.bone_palette.len() <= 4096);
                    assert!(
                        part.packed_weights.is_empty()
                            || part.packed_weights.len() as u32 == part.num_vertices
                    );
                    for packed in &part.packed_weights {
                        for weight in &packed.weights {
                            assert!(weight.is_finite());
                            assert!((-0.0001..=1.0001).contains(weight));
                        }
                    }
                    palette_max = palette_max.max(part.bone_palette.len() as u32);
                }
                assert_eq!(
                    partition.max_bones_per_partition, palette_max,
                    "the reported maximum follows the partitions it summarises"
                );
            }
            exercised += 1;
        }

        if parser.has_block_type("bhkCollisionObject")
            || parser.has_block_type("bhkBlendCollisionObject")
        {
            let object = parser
                .parse_collision_object()
                .expect("bhkCollisionObject reads on a shipped mesh");
            assert!(object.rigid_body_index < parser.block_count());
            let body = &object.body_info;
            assert!(
                body.mass.is_finite() && body.mass > 0.0,
                "mass {}",
                body.mass
            );
            assert!(
                (0.0..=1.0001).contains(&body.friction),
                "friction {}",
                body.friction
            );
            assert!(body.restitution.is_finite() && body.restitution >= 0.0);
            assert!(body.linear_velocity.x.is_finite());
            // The reader writes the translation and nothing else of the
            // transform, so the scale stays at its default.
            assert_eq!(body.transform.scale, 1.0);
            if object.shape.shape_type != ShapeType::None {
                for vertex in &object.shape.vertices {
                    assert!(vertex.x.is_finite() && vertex.y.is_finite() && vertex.z.is_finite());
                }
                assert!(object.shape.radius.is_finite());
            }
            exercised += 1;
        }

        assert!(
            exercised > 0,
            "{} carries neither a skin nor a collision object; the corpus moved?",
            path
        );
        eprintln!("retail readers exercised {exercised} branch(es) on {path}");
    }
    // -----------------------------------------------------------------
    // N5b: animation readers (NiControllerManager / NiControllerSequence /
    // NiTextKeyExtraData)
    // -----------------------------------------------------------------

    /// `NiControllerManager` body at the 20.0.0.4 layout: the NiTimeController
    /// header (26 bytes), the cumulative flag, then the sequence count, its
    /// references and the object palette.
    fn controller_manager_body(sequence_refs: &[u32], palette: u32) -> Vec<u8> {
        let mut body = Vec::new();
        push_u32(&mut body, 0xFFFF_FFFF); // next controller
        push_u16(&mut body, 0x0020); // flags
        push_f32(&mut body, 1.0); // frequency
        push_f32(&mut body, 0.0); // phase
        push_f32(&mut body, 0.0); // start time
        push_f32(&mut body, 1.0); // stop time
        push_u32(&mut body, 0xFFFF_FFFF); // target
        body.push(1); // cumulative
        push_u32(&mut body, sequence_refs.len() as u32);
        for reference in sequence_refs {
            push_u32(&mut body, *reference);
        }
        push_u32(&mut body, palette);
        body
    }

    /// One `ControlledBlock` at the 20.0.0.4 layout: interpolator, controller,
    /// priority, string palette and the five name offsets — 33 bytes.
    fn controlled_block_body(
        interpolator: u32,
        controller: u32,
        priority: u8,
        palette: u32,
        offsets: [u32; 5],
    ) -> Vec<u8> {
        let mut body = Vec::new();
        push_u32(&mut body, interpolator);
        push_u32(&mut body, controller);
        body.push(priority);
        push_u32(&mut body, palette);
        for offset in offsets {
            push_u32(&mut body, offset);
        }
        body
    }

    /// `NiControllerSequence` body at the 20.0.0.4 layout with fixed identity
    /// fields (name "walk", manager ref 1, accum root "root", timing
    /// 0.25..2.75 at frequency 1.5 → duration 2.5) and the sequence's shape as
    /// the knobs: its controlled blocks, its text key block and its cycle type.
    fn controller_sequence_body_with_blocks(
        controlled_blocks: Vec<Vec<u8>>,
        text_keys_ref: u32,
        cycle_type: u32,
    ) -> Vec<u8> {
        let mut body = sized("walk");
        push_u32(&mut body, controlled_blocks.len() as u32);
        push_u32(&mut body, 0); // array grow by
        for block in controlled_blocks {
            body.extend_from_slice(&block);
        }
        push_u32(&mut body, 0); // weight
        push_u32(&mut body, text_keys_ref);
        push_u32(&mut body, cycle_type);
        push_f32(&mut body, 1.5); // frequency
        push_f32(&mut body, 0.25); // start time
        push_f32(&mut body, 2.75); // stop time
        push_u32(&mut body, 1); // manager (block 1 in these fixtures)
        body.extend_from_slice(&sized("root")); // accum root name
        push_u32(&mut body, 0xFFFF_FFFF); // string palette
        body
    }

    /// The one-block sequence body the round-trip test uses.
    fn controller_sequence_body(text_keys_ref: u32, cycle_type: u32) -> Vec<u8> {
        controller_sequence_body_with_blocks(
            vec![controlled_block_body(3, 7, 2, 0xFFFF_FFFF, [1, 2, 3, 4, 5])],
            text_keys_ref,
            cycle_type,
        )
    }

    /// `NiTextKeyExtraData` body: name, count, then time + value per key.
    ///
    /// The name is empty on purpose: the original reader consumes it as a
    /// bare u32 rather than the SizedString the walker measures, so only an
    /// empty name keeps the count aligned — which is what retail ships (the
    /// retail test below re-checks it).
    fn text_key_body(keys: &[(f32, &str)]) -> Vec<u8> {
        let mut body = sized("");
        push_u32(&mut body, keys.len() as u32);
        for (time, value) in keys {
            push_f32(&mut body, *time);
            body.extend_from_slice(&sized(value));
        }
        body
    }

    /// NiKeyframeData body of the quaternion shape: the rotation cluster,
    /// then the Translations and Scales channels at interpolation 1 when they
    /// carry keys (the bare count when they do not, which leaves their
    /// interpolation unread).
    fn keyframe_data_body(
        rotation_type: u32,
        quats: &[(f32, [f32; 4])],
        translations: &[(f32, [f32; 3])],
        scales: &[(f32, f32)],
    ) -> Vec<u8> {
        let mut body = Vec::new();
        push_u32(&mut body, quats.len() as u32);
        if !quats.is_empty() {
            push_u32(&mut body, rotation_type);
        }
        for (time, value) in quats {
            push_f32(&mut body, *time);
            for component in value {
                push_f32(&mut body, *component);
            }
        }
        push_u32(&mut body, translations.len() as u32);
        if !translations.is_empty() {
            push_u32(&mut body, 1); // interpolation
            for (time, value) in translations {
                push_f32(&mut body, *time);
                for component in value {
                    push_f32(&mut body, *component);
                }
            }
        }
        push_u32(&mut body, scales.len() as u32);
        if !scales.is_empty() {
            push_u32(&mut body, 1); // interpolation
            for (time, value) in scales {
                push_f32(&mut body, *time);
                push_f32(&mut body, *value);
            }
        }
        body
    }

    #[test]
    fn keyframe_data_reads_quaternion_rotation_and_channels() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.4",
            0x1400_0004,
            11,
            11,
            &["NiNode", "NiKeyframeData"],
            &[0, 1],
        );
        let mut built = Built::with_header(header);
        built.block(ninode_table("Root"));
        built.block(keyframe_data_body(
            1,
            &[(0.5, [0.0, 0.0, 0.0, 1.0]), (1.0, [0.0, 0.0, 0.5, 0.5])],
            &[(0.25, [1.0, 2.0, 3.0])],
            &[],
        ));
        let (bytes, ranges) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        // The reader must consume exactly the body the walker measured —
        // that measurement is what places every later block in the file.
        assert_eq!(
            (parser.block_body_offset(1), parser.block_body_end(1)),
            ranges[1]
        );

        let data = parser
            .parse_keyframe_data()
            .expect("the keyframe data block is in the file");
        assert_eq!(data.rotation_type, 1);
        assert_eq!(data.rotation_keys.len(), 2);
        assert_eq!(data.rotation_keys[0].time, 0.5);
        assert_eq!(
            data.rotation_keys[0].value,
            Vector4 {
                x: 0.0,
                y: 0.0,
                z: 0.0,
                w: 1.0
            }
        );
        // Rotation type 1 carries no TBC bytes: the triple stays at defaults.
        assert_eq!(data.rotation_keys[1].tbc, [0.0; 3]);
        assert_eq!(data.translations.interpolation, 1);
        assert_eq!(data.translations.keys.len(), 1);
        assert_eq!(data.translations.keys[0].time, 0.25);
        assert_eq!(
            data.translations.keys[0].value,
            Vector3 {
                x: 1.0,
                y: 2.0,
                z: 3.0
            }
        );
        // The empty Scales group leaves its interpolation unread.
        assert_eq!(data.scales.interpolation, 0);
        assert!(data.scales.keys.is_empty());
    }

    #[test]
    fn keyframe_data_rotation_type_4_switches_to_euler_channels() {
        // The retail .kf streams carry whole-skeleton rotations as three
        // float channels, tangent- or TBC-interpolated — the shape the
        // goblin sample's NiTransformData blocks store. 20.0.0.4 files hold
        // no Euler order word.
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.4",
            0x1400_0004,
            11,
            11,
            &["NiNode", "NiTransformData"],
            &[0, 1],
        );
        let mut body = Vec::new();
        push_u32(&mut body, 1); // Num Rotation Keys, counted but not stored
        push_u32(&mut body, 4); // Rotation Type
        // Channel 0: one tangent-interpolated key.
        push_u32(&mut body, 1);
        push_u32(&mut body, 2);
        push_f32(&mut body, 0.25); // time
        push_f32(&mut body, 0.5); // value
        push_f32(&mut body, 0.125); // forward tangent
        push_f32(&mut body, 0.75); // backward tangent
        // Channel 1: one TBC-interpolated key.
        push_u32(&mut body, 1);
        push_u32(&mut body, 3);
        push_f32(&mut body, 0.5);
        push_f32(&mut body, 0.75);
        for value in [1.0, 2.0, 3.0] {
            push_f32(&mut body, value); // TBC triple
        }
        // Channel 2, Translations and Scales: empty groups.
        push_u32(&mut body, 0);
        push_u32(&mut body, 0);
        push_u32(&mut body, 0);
        let mut built = Built::with_header(header);
        built.block(ninode_table("Root"));
        built.block(body);
        let (bytes, ranges) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        assert_eq!(
            (parser.block_body_offset(1), parser.block_body_end(1)),
            ranges[1]
        );

        let data = parser
            .parse_keyframe_data()
            .expect("rotation type 4 body");
        assert!(data.rotation_keys.is_empty()); // type 4 reads no quaternions
        assert_eq!(data.rotation_order, 0); // absent at 20.0.0.4
        assert_eq!(data.xyz_rotations[0].interpolation, 2);
        assert_eq!(data.xyz_rotations[0].keys[0].time, 0.25);
        assert_eq!(data.xyz_rotations[0].keys[0].value, 0.5);
        assert_eq!(data.xyz_rotations[0].keys[0].forward_tangent, 0.125);
        assert_eq!(data.xyz_rotations[0].keys[0].backward_tangent, 0.75);
        assert_eq!(data.xyz_rotations[1].interpolation, 3);
        assert_eq!(data.xyz_rotations[1].keys[0].tbc, [1.0, 2.0, 3.0]);
        assert!(data.xyz_rotations[2].keys.is_empty());
        assert!(data.translations.keys.is_empty());
        assert!(data.scales.keys.is_empty());
    }

    #[test]
    fn keyframe_data_rotation_type_3_appends_tbc_to_quaternion_keys() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.4",
            0x1400_0004,
            11,
            11,
            &["NiNode", "NiKeyframeData"],
            &[0, 1],
        );
        let mut body = Vec::new();
        push_u32(&mut body, 1);
        push_u32(&mut body, 3); // rotation type
        push_f32(&mut body, 0.5); // time
        for value in [0.0, 0.0, 0.0, 1.0] {
            push_f32(&mut body, value); // quaternion
        }
        for value in [0.25, 0.5, 0.75] {
            push_f32(&mut body, value); // TBC triple
        }
        push_u32(&mut body, 0); // Translations
        push_u32(&mut body, 0); // Scales
        let mut built = Built::with_header(header);
        built.block(ninode_table("Root"));
        built.block(body);
        let (bytes, _) = built.trailer(&[0]);

        let parser = parse(&bytes);
        let data = parser
            .parse_keyframe_data()
            .expect("rotation type 3 body");
        assert_eq!(data.rotation_keys[0].tbc, [0.25, 0.5, 0.75]);
    }

    #[test]
    fn parse_keyframe_data_picks_the_earlier_of_the_two_names() {
        // Both keyframe data names in one file: file order wins, exactly as
        // `seek_to_block_of_type` picks the first block of a single type.
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.4",
            0x1400_0004,
            11,
            11,
            &["NiNode", "NiTransformData", "NiKeyframeData"],
            &[0, 1, 2],
        );
        let mut built = Built::with_header(header);
        built.block(ninode_table("Root"));
        built.block(keyframe_data_body(1, &[(7.5, [0.0, 0.0, 0.0, 1.0])], &[], &[]));
        built.block(keyframe_data_body(1, &[(9.5, [0.0, 0.0, 0.0, 1.0])], &[], &[]));
        let (bytes, _) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        let data = parser.parse_keyframe_data().expect("a data block exists");
        assert_eq!(data.rotation_keys[0].time, 7.5);
    }

    #[test]
    fn controller_manager_resolves_sequences_and_text_keys() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &[
                "NiNode",
                "NiControllerManager",
                "NiControllerSequence",
                "NiTextKeyExtraData",
            ],
            &[0, 1, 2, 3],
        );
        let mut built = Built::with_header(header);
        built.block(ninode_table("Root"));
        built.block(controller_manager_body(&[2], 0xFFFF_FFFF));
        built.block(controller_sequence_body(3, 0)); // cycle type 0 → loop
        built.block(text_key_body(&[(0.5, ""), (1.25, "start")]));
        let (bytes, _) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());

        let manager = parser
            .parse_controller_manager()
            .expect("the manager block is in the file");
        // The block carries no name and the original never writes one.
        assert_eq!(manager.name, "");
        assert_eq!(manager.controller_sequence_count, 1);
        assert_eq!(manager.object_palette_index, 0xFFFF_FFFF);
        assert_eq!(manager.last_time.to_bits(), 0.0f32.to_bits());
        assert_eq!(manager.sequences.len(), 1);

        let sequence = &manager.sequences[0];
        assert_eq!(sequence.name, "walk");
        assert_eq!(sequence.controller_manager_index, 1);
        assert_eq!(sequence.target_name, "root");
        assert_eq!(sequence.frequency.to_bits(), 1.5f32.to_bits());
        assert_eq!(sequence.start_time.to_bits(), 0.25f32.to_bits());
        assert_eq!(sequence.stop_time.to_bits(), 2.75f32.to_bits());
        assert_eq!(sequence.duration.to_bits(), 2.5f32.to_bits());
        assert_eq!(sequence.phase.to_bits(), 0.0f32.to_bits());
        assert!(sequence.is_loop);

        assert_eq!(sequence.controlled_blocks.len(), 1);
        let controlled = &sequence.controlled_blocks[0];
        assert_eq!(controlled.interpolator_index, 3);
        assert_eq!(controlled.controller_index, 7);
        assert_eq!(controlled.priority, 2);
        assert_eq!(controlled.string_palette_index, 0xFFFF_FFFF);
        assert_eq!(controlled.node_name_offset, 1);
        assert_eq!(controlled.property_type_offset, 2);
        assert_eq!(controlled.controller_type_offset, 3);
        assert_eq!(controlled.controller_id_offset, 4);
        assert_eq!(controlled.interpolator_id_offset, 5);
        // Seeded exactly as the original seeds them while reading.
        assert_eq!(controlled.keyframe_data_index, 7);
        assert_eq!(controlled.resolved_bone_index, -1);
        // resolveControllerReferences() territory: untouched by this wave.
        assert_eq!(controlled.target_node_index, 0);
        assert_eq!(controlled.clip.name, "");
        assert_eq!(controlled.clip.frequency.to_bits(), 1.0f32.to_bits());

        // Text keys: the original reads each value as a string *reference*
        // (readStringRef), so the length prefix becomes the placeholder — the
        // string itself is never consumed. The differential oracle compares
        // these placeholders byte for byte.
        assert_eq!(sequence.text_keys.len(), 2);
        assert_eq!(sequence.text_keys[0].time.to_bits(), 0.5f32.to_bits());
        assert_eq!(sequence.text_keys[0].value, "string_0");
        assert_eq!(sequence.text_keys[1].time.to_bits(), 1.25f32.to_bits());
        assert_eq!(sequence.text_keys[1].value, "string_5");

        // The standalone entry point finds the same block.
        let standalone = parser
            .parse_controller_sequence()
            .expect("the sequence block is in the file");
        assert_eq!(standalone, *sequence);
    }

    #[test]
    fn controller_manager_is_absent_without_the_block() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiNode"],
            &[0],
        );
        let mut built = Built::with_header(header);
        built.block(ninode_table("Root"));
        let (bytes, _) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(parser.parse_controller_manager().is_none());
        assert!(parser.parse_controller_sequence().is_none());
    }

    #[test]
    fn controller_manager_over_the_sequence_limit_is_rejected() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiNode", "NiControllerManager"],
            &[0, 1],
        );
        let mut built = Built::with_header(header);
        built.block(ninode_table("Root"));
        // One reference over MAX_CONTROLLER_SEQUENCES (4096): the walker still
        // measures the block, but the reader rejects it — as the original does.
        built.block(controller_manager_body(&[0u32; 4097], 0));
        let (bytes, _) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        assert!(parser.parse_controller_manager().is_none());
    }

    #[test]
    fn sequence_over_the_controlled_block_limit_is_dropped_not_fatal() {
        let header = gamebryo_header(
            "Gamebryo File Format, Version 20.0.0.5",
            0x1400_0005,
            11,
            11,
            &["NiNode", "NiControllerManager", "NiControllerSequence"],
            &[0, 1, 2],
        );
        let mut built = Built::with_header(header);
        built.block(ninode_table("Root"));
        built.block(controller_manager_body(&[2], 0xFFFF_FFFF));
        // One block over MAX_CONTROLLED_BLOCKS (4096): the original logs the
        // sequence off and keeps the manager — so the manager still parses,
        // with the over-limit sequence skipped.
        let oversized = vec![controlled_block_body(3, 7, 2, 0, [0; 5]); 4097];
        built.block(controller_sequence_body_with_blocks(
            oversized,
            0xFFFF_FFFF,
            0,
        ));
        let (bytes, _) = built.trailer(&[0]);

        let parser = parse(&bytes);
        assert!(parser.are_blocks_walked(), "{}", parser.walk_error());
        let manager = parser
            .parse_controller_manager()
            .expect("the manager itself is well-formed");
        assert_eq!(manager.controller_sequence_count, 1);
        assert!(manager.sequences.is_empty());
    }

    /// Phase 30's animation consumers (`phase30_integration_test`,
    /// `world_loader`) run these readers over real Oblivion assets, so the
    /// round trip has to hold on retail data too — and the retail files also
    /// pin the alignment precondition of the original's u32 name read: a
    /// non-empty text key name would shift `numKeys` into the string bytes.
    #[test]
    fn retail_animation_readers_stay_in_range() {
        let root = std::env::var("OBLIVION_MESHES").unwrap_or_else(|_| {
            String::from(r"C:\Users\hiroki.kogarumai\Oblivion_Android\meshes_extracted")
        });
        if !std::path::Path::new(&root).exists() {
            eprintln!("skipping: retail extraction not available at {root}");
            return;
        }
        let candidates = [
            r"meshes\architecture\castle\kvatch\stonewallgatedoor01.nif",
            r"meshes\magiceffects\alteration.nif",
            r"meshes\magiceffects\binditem.nif",
        ];
        let mut exercised = 0;
        for relative in candidates {
            let path = format!(r"{root}\{relative}");
            let mut parser = NifParser::new();
            if parser.parse_file(&path).is_err() {
                eprintln!("skipping: retail mesh not available at {path}");
                continue;
            }

            // The original reads the text key block name as a bare u32, so it
            // only lands on Num Keys while the name is empty.
            let raw = std::fs::read(&path).expect("mesh re-readable");
            for index in parser.find_blocks_of_type("NiTextKeyExtraData") {
                let offset = parser
                    .locate_block_body(index)
                    .expect("walked text key block locates");
                let name_length = u32::from_le_bytes(raw[offset..offset + 4].try_into().unwrap());
                assert_eq!(name_length, 0, "{relative} text key block {index} named");
            }

            let manager = parser
                .parse_controller_manager()
                .expect("animated retail file carries a NiControllerManager");
            assert!(!manager.sequences.is_empty());
            assert!(manager.sequences.len() as u32 <= manager.controller_sequence_count);
            assert!(manager
                .sequences
                .iter()
                .any(|sequence| !sequence.text_keys.is_empty()));
            for sequence in &manager.sequences {
                // Retail sequences are never inverted (stop < start), but a
                // zero-span one exists: binditem.nif ships two sequences with
                // start = stop = 0, so "> 0" is not a retail invariant.
                assert!(!sequence.name.is_empty());
                assert!(sequence.duration.is_finite());
                assert!(sequence.duration >= 0.0);
                for key in &sequence.text_keys {
                    assert!(key.value.starts_with("string_"));
                    assert!(key.time.is_finite());
                }
            }

            // phase30 only checks the strict invariants on its first candidate
            // (the animated door) and only for its first five sequences.
            if relative.ends_with("stonewallgatedoor01.nif") {
                for sequence in manager.sequences.iter().take(5) {
                    assert!(!sequence.name.is_empty());
                    assert!(sequence.duration > 0.0);
                }
            }

            // The standalone entry point agrees with the manager's view.
            let standalone = parser
                .parse_controller_sequence()
                .expect("the sequence block is in the file");
            assert!(!standalone.name.is_empty());

            exercised += 1;
        }

        assert!(
            exercised > 0,
            "no animated retail meshes available; the extraction moved?"
        );
        eprintln!("retail animation readers exercised {exercised} file(s)");
    }
}
