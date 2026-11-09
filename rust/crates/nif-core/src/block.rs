//! Skin, Havok collision and animation readers.
//!
//! These are the block-anchored readers of `nif_parser.cpp` (the Phase 30
//! skinning, collision and animation steps): unlike the geometry decode they do
//! not run during `parse`, they are called on demand by whoever needs a skin, a
//! collision shape or a controller sequence, and every one of them first seeks
//! to a block of a known type through the ranges the walker recorded.
//!
//! The layout comments mirror the C++ ones field for field, including the
//! quirks a retail corpus depends on: the skin partition link that lives on
//! `NiSkinData` only below 10.1.0.0, the vertex-weight flag byte that is absent
//! below 4.2.1.0, and `bhkMoppBvTreeShape` reading its leading shape reference
//! even on versions where the walker treats it as a per-block prefix.
//!
//! Every reader starts from a clean sticky flag on a cursor it positions
//! itself. The C++ parser instead inherits whatever the previous operation left
//! behind; after a successful parse the two agree, because a successful walk
//! clears that flag — which is what the differential oracle runs on.

use std::mem::{offset_of, size_of};

use crate::header::Header;
use crate::reader::Reader;
use crate::walk::{locate_block, WalkState};
use crate::{Matrix3x3, Node, Transform, Vector3, Vector4};

// The C ABI points at these two directly, so their layouts are part of the
// contract rather than an implementation detail. The field offsets are asserted
// too: `nif-ffi` asserts the same two on its side, and only matching offsets
// make the pointer casts sound.
const _: () = assert!(size_of::<VertexWeight>() == 8);
const _: () = assert!(offset_of!(VertexWeight, weight) == 4);
const _: () = assert!(size_of::<PackedWeight>() == 24);
const _: () = assert!(offset_of!(PackedWeight, weights) == 8);

/// Upper bounds that keep a corrupt block header from turning into a huge
/// allocation. Retail files stay far below all of them.
const MAX_SKIN_BONES: u32 = 4096;
const MAX_SKIN_PARTITIONS: u32 = 4096;
const MAX_WEIGHTS_PER_VERTEX: u32 = 4;
const MAX_RIGID_BODY_CONSTRAINTS: u32 = 4096;
const MAX_CONVEX_VERTICES: u32 = 65536;
const MAX_STRIPS_DATA: u32 = 4096;
const MAX_SHAPE_FILTERS: u32 = 4096;
const MAX_LIST_SUB_SHAPES: u32 = 4096;
const MAX_MOPP_DATA_SIZE: u32 = 1 << 20;
// Animation limits — the same two values `nif_parser.cpp` keeps for the
// controller manager and sequence readers that moved here.
const MAX_CONTROLLER_SEQUENCES: u32 = 4096;
const MAX_CONTROLLED_BLOCKS: u32 = 4096;

/// `NiSkinInstance`: Data | Skin Partition (since 10.1.0.101) | Skeleton Root |
/// Num Bones | Bones. The partition index is the `0xFFFFFFFF` sentinel below
/// 10.1.0.101, where the skin instance has no partition of its own.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct SkinInstance {
    pub skin_data_index: u32,
    pub skin_partition_index: u32,
    pub skeleton_root_index: u32,
    pub bone_node_indices: Vec<u32>,
}

/// One entry of a bone's `BoneVertData` array: an index and its weight.
///
/// `repr(C)` because the C ABI hands a pointer straight at a `Vec` of these,
/// matching `nif_skin_vertex_weight`.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct VertexWeight {
    pub vertex_index: u16,
    pub weight: f32,
}

/// `BoneData`: a skin transform, a bounding sphere (skipped, as in the
/// original) and the per-vertex weights.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct BoneData {
    pub skin_transform: Transform,
    pub vertex_weights: Vec<VertexWeight>,
}

/// `NiSkinData`: Skin Transform | [Skin Partition, 4.0.0.2..10.1.0.0] | Num
/// Bones | Has Vertex Weights (since 4.2.1.0) | BoneData[Num Bones].
#[derive(Clone, Debug, Default, PartialEq)]
pub struct SkinData {
    pub root_rotation: Matrix3x3,
    pub root_translation: Vector3,
    pub root_scale: f32,
    pub num_bones: u32,
    pub bone_data: Vec<BoneData>,
}

/// GPU-ready weights for one vertex: four bone indices and four weights, both
/// truncated to four entries the way the original packs them.
///
/// `repr(C)` for the same reason as [`VertexWeight`]: the C ABI points at a
/// `Vec` of these and expects `nif_packed_weight`'s layout.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct PackedWeight {
    pub bone_indices: [u16; 4],
    pub weights: [f32; 4],
}

/// One entry of the skin partition array. The vertex map is dropped — the
/// packed weights already cover every vertex — exactly as the original does.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct Partition {
    pub num_vertices: u32,
    pub num_triangles: u32,
    pub bone_palette: Vec<u16>,
    pub packed_weights: Vec<PackedWeight>,
    pub indices: Vec<u16>,
}

/// `NiSkinPartition`: Num Partitions | Partitions[Num Partitions], plus the two
/// maxima the reader derives while walking them.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct SkinPartition {
    pub partitions: Vec<Partition>,
    pub max_bones_per_partition: u32,
    pub max_bones_per_vertex: u32,
}

/// The concrete collision shape of a block, matching `CollisionShapeType`.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
#[repr(u32)]
pub enum ShapeType {
    #[default]
    None = 0,
    Box = 1,
    Sphere = 2,
    Capsule = 3,
    ConvexHull = 4,
    TriMesh = 5,
    MoppBvTree = 6,
    List = 7,
}

/// A decoded Havok shape. Which fields are populated depends on `shape_type`;
/// `sub_shape_refs` / `strips_data_refs` / `child_shape_ref` are block table
/// indices for the caller to follow, which is what the original records too.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct CollisionShape {
    pub shape_type: ShapeType,
    pub center: Vector3,
    pub half_extents: Vector3,
    pub radius: f32,
    pub height: f32,
    pub vertices: Vec<Vector3>,
    pub triangles: Vec<[u16; 3]>,
    pub mopp_data: Vec<u8>,
    pub mopp_data_size: u32,
    pub child_shape_ref: u32,
    pub strips_data_refs: Vec<u32>,
    pub sub_shape_refs: Vec<u32>,
}

/// The numbers a caller draws a rigid body with, matching `RigidBodyInfo`.
#[derive(Clone, Debug, PartialEq)]
pub struct RigidBodyInfo {
    pub mass: f32,
    pub friction: f32,
    pub restitution: f32,
    pub linear_velocity: Vector3,
    pub angular_velocity: Vector3,
    pub transform: Transform,
    pub collision_group: u32,
    pub collision_filter: u32,
    pub is_trigger: bool,
}

impl Default for RigidBodyInfo {
    fn default() -> Self {
        Self {
            // The C++ struct's own initialisers, which is what a caller sees
            // when the body block could not be read.
            mass: 1.0,
            friction: 0.5,
            restitution: 0.3,
            linear_velocity: Vector3::default(),
            angular_velocity: Vector3::default(),
            transform: Transform::default(),
            collision_group: 0,
            collision_filter: 0,
            is_trigger: false,
        }
    }
}

/// `bhkCollisionObject` with its body and shape already resolved, matching
/// `CollisionObject`.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct CollisionObject {
    pub node_index: u32,
    pub rigid_body_index: u32,
    pub body_info: RigidBodyInfo,
    pub shape: CollisionShape,
    pub target_name: String,
}

/// One rigid body read: the values as far as they were read, the shape
/// reference that came with them, and whether the read ran to the end.
///
/// The original reader writes through a caller's struct and returns false when
/// a check trips, so the caller keeps everything read up to that point — the
/// collision object logs and continues with it. `complete` is how that shows
/// through an `Option`: `None` means the block could not be reached at all (the
/// original returns false having touched nothing), `Some` always carries the
/// partial values.
#[derive(Clone, Debug, PartialEq)]
pub struct RigidBodyRead {
    pub info: RigidBodyInfo,
    pub shape_index: u32,
    pub complete: bool,
}

/// Reads the skin and collision blocks of one already-walked file.
pub(crate) struct BlockReader<'a> {
    r: Reader<'a>,
    header: &'a Header,
    walk: &'a WalkState,
    nodes: &'a [Node],
}

impl<'a> BlockReader<'a> {
    pub(crate) fn new(
        data: &'a [u8],
        header: &'a Header,
        walk: &'a WalkState,
        nodes: &'a [Node],
    ) -> Self {
        Self {
            r: Reader::new(data),
            header,
            walk,
            nodes,
        }
    }

    // -----------------------------------------------------------------
    // Block location
    // -----------------------------------------------------------------

    fn block_type_name(&self, index: u32) -> String {
        self.header
            .block_type_indices
            .get(index as usize)
            .and_then(|type_index| self.header.block_type_names.get(*type_index as usize))
            .cloned()
            .unwrap_or_default()
    }

    /// One past the last body byte of a block; zero when out of range.
    fn block_end(&self, index: u32) -> usize {
        self.walk.ends.get(index as usize).copied().unwrap_or(0)
    }

    fn locate(&self, index: u32) -> Option<usize> {
        locate_block(self.walk, self.header, index)
    }

    /// Indices of every block of one type, in file order. Empty until the file
    /// has been walked, which is what `findBlocksOfType` reports too.
    fn find_blocks_of_type(&self, type_name: &str) -> Vec<u32> {
        if !self.walk.walked {
            return Vec::new();
        }
        (0..self.header.block_type_indices.len() as u32)
            .filter(|index| self.block_type_name(*index) == type_name)
            .collect()
    }

    /// Point the cursor at the first field of the first block of `type_name`.
    fn seek_to_block_of_type(&mut self, type_name: &str) -> Option<u32> {
        let blocks = self.find_blocks_of_type(type_name);
        let index = *blocks.first()?;
        let offset = self.locate(index)?;
        self.r.set_pos(offset);
        Some(index)
    }

    /// Point the cursor at the lowest-indexed block among `type_names`.
    fn seek_to_any_block_of_type(&mut self, type_names: &[&str]) -> Option<u32> {
        let mut best: Option<u32> = None;
        for type_name in type_names {
            for index in self.find_blocks_of_type(type_name) {
                if best.is_none_or(|current| index < current) {
                    best = Some(index);
                }
            }
        }
        let index = best?;
        let offset = self.locate(index)?;
        self.r.set_pos(offset);
        Some(index)
    }

    // -----------------------------------------------------------------
    // Typed reads shared by these readers
    // -----------------------------------------------------------------

    fn vector3(&mut self) -> Vector3 {
        Vector3 {
            x: self.r.f32(),
            y: self.r.f32(),
            z: self.r.f32(),
        }
    }

    /// hkVector4 / hkQuaternion: three components padded to sixteen bytes; the
    /// padding word is dropped.
    fn hk_vector3(&mut self) -> Vector3 {
        let vector = self.vector3();
        self.r.f32();
        vector
    }

    fn matrix3x3(&mut self) -> Matrix3x3 {
        let mut matrix = Matrix3x3::default();
        for row in matrix.m.iter_mut() {
            for cell in row.iter_mut() {
                *cell = self.r.f32();
            }
        }
        matrix
    }

    /// The skin transform inside a `BoneData` entry: rotation, translation,
    /// then scale — the opposite order to a NiAVObject transform.
    ///
    /// The original reader walks this block rotation first
    /// (`skinTransform.rotation = readMatrix3x3()` before the translation),
    /// which is what the differential oracle compares against; reading it the
    /// NiAVObject way round swaps the matrix and the vector and still "works",
    /// which is exactly the class of bug only a byte comparison catches.
    fn skin_transform(&mut self) -> Transform {
        let rotation = self.matrix3x3();
        let translation = self.vector3();
        let scale = self.r.f32();
        Transform {
            rotation,
            translation,
            scale,
        }
    }

    // -----------------------------------------------------------------
    // Skinning
    // -----------------------------------------------------------------

    /// `NiSkinInstance`: Data:Ref | Skin Partition:Ref (since 10.1.0.101) |
    /// Skeleton Root:Ptr | Num Bones | Bones:Ptr[Num Bones].
    pub(crate) fn parse_skin_instance(&mut self) -> Option<SkinInstance> {
        self.seek_to_block_of_type("NiSkinInstance")?;
        let mut skin = SkinInstance {
            skin_data_index: self.r.u32(),
            ..SkinInstance::default()
        };
        skin.skin_partition_index = if self.header.version >= 0x0A01_0065 {
            self.r.u32()
        } else {
            0xFFFF_FFFF
        };
        skin.skeleton_root_index = self.r.u32();
        let num_bones = self.r.u32();
        if self.r.has_error() {
            return None;
        }
        if num_bones > MAX_SKIN_BONES {
            self.r.set_error();
            return None;
        }
        for _ in 0..num_bones {
            skin.bone_node_indices.push(self.r.u32());
        }
        if self.r.has_error() {
            return None;
        }
        Some(skin)
    }

    /// `NiSkinData` at the first block of that type.
    pub(crate) fn parse_skin_data(&mut self) -> Option<SkinData> {
        let index = self.seek_to_block_of_type("NiSkinData")?;
        self.parse_skin_data_at(index)
    }

    /// `NiSkinData` at a known block: Skin Transform | [Skin Partition] | Num
    /// Bones | Has Vertex Weights | BoneData[Num Bones].
    pub(crate) fn parse_skin_data_at(&mut self, index: u32) -> Option<SkinData> {
        let offset = self.locate(index)?;
        self.r.set_pos(offset);
        let block_end = self.block_end(index);

        let root_rotation = self.matrix3x3();
        let root_translation = self.vector3();
        let root_scale = self.r.f32();

        // The partition link lives on this block only below 10.1.0.0, where the
        // skin instance has no partition of its own.
        if (0x0400_0002..0x0A01_0000).contains(&self.header.version) {
            self.r.skip(4); // Skin Partition
        }

        let num_bones = self.r.u32();
        if self.r.has_error() {
            return None;
        }
        if num_bones == 0 || num_bones > MAX_SKIN_BONES {
            self.r.set_error();
            return None;
        }

        // Without this flag the bone list holds transforms only and every Num
        // Vertices field is followed directly by the next bone. The byte itself
        // is absent below 4.2.1.0, where the flag defaults to true.
        let mut has_vertex_weights_byte = 1u8;
        if self.header.version >= 0x0402_0100 {
            has_vertex_weights_byte = self.r.u8();
        }
        let has_vertex_weights = has_vertex_weights_byte != 0;
        if self.r.has_error() {
            return None;
        }

        let mut data = SkinData {
            root_rotation,
            root_translation,
            root_scale,
            num_bones,
            bone_data: Vec::with_capacity(num_bones as usize),
        };

        for _ in 0..num_bones {
            let mut bone = BoneData {
                skin_transform: self.skin_transform(),
                vertex_weights: Vec::new(),
            };
            self.r.skip(16); // Bounding Sphere: Center + Radius

            let num_vertices = self.r.u16();
            if self.r.has_error() {
                return None;
            }
            if !has_vertex_weights {
                data.bone_data.push(bone);
                continue;
            }
            let needed = (num_vertices as usize).saturating_mul(6);
            if self.r.pos().saturating_add(needed) > block_end {
                self.r.set_error();
                return None;
            }
            bone.vertex_weights.reserve(num_vertices as usize);
            for _ in 0..num_vertices {
                let vertex_index = self.r.u16();
                let weight = self.r.f32();
                bone.vertex_weights.push(VertexWeight {
                    vertex_index,
                    weight,
                });
            }
            if self.r.has_error() {
                return None;
            }
            data.bone_data.push(bone);
        }

        Some(data)
    }

    /// `NiSkinPartition` at the first block of that type.
    pub(crate) fn parse_skin_partition(&mut self) -> Option<SkinPartition> {
        let index = self.seek_to_block_of_type("NiSkinPartition")?;
        self.parse_skin_partition_at(index)
    }

    /// `NiSkinPartition` at a known block.
    ///
    /// Every optional payload is gated by its own flag byte, and each one is
    /// measured against the block end before it is read, so a corrupt count
    /// stops the read instead of walking off the block.
    pub(crate) fn parse_skin_partition_at(&mut self, index: u32) -> Option<SkinPartition> {
        let offset = self.locate(index)?;
        self.r.set_pos(offset);
        let block_end = self.block_end(index);

        let num_partitions = self.r.u32();
        if self.r.has_error() {
            return None;
        }
        if num_partitions > MAX_SKIN_PARTITIONS {
            self.r.set_error();
            return None;
        }

        let mut partition = SkinPartition {
            partitions: Vec::with_capacity(num_partitions as usize),
            max_bones_per_partition: 0,
            max_bones_per_vertex: 0,
        };

        for _ in 0..num_partitions {
            let num_vertices = self.r.u16() as u32;
            let num_triangles = self.r.u16() as u32;
            let num_bones = self.r.u16() as u32;
            let num_strips = self.r.u16() as u32;
            let num_weights_per_vertex = self.r.u16() as u32;
            if self.r.has_error() {
                return None;
            }
            if num_bones > MAX_SKIN_BONES || num_weights_per_vertex > MAX_WEIGHTS_PER_VERTEX {
                self.r.set_error();
                return None;
            }

            partition.max_bones_per_partition = partition.max_bones_per_partition.max(num_bones);
            partition.max_bones_per_vertex =
                partition.max_bones_per_vertex.max(num_weights_per_vertex);

            let mut part = Partition {
                num_vertices,
                num_triangles,
                ..Partition::default()
            };

            part.bone_palette.reserve(num_bones as usize);
            for _ in 0..num_bones {
                part.bone_palette.push(self.r.u16());
            }

            // Vertex map: dropped, the packed weights cover every vertex.
            if self.r.u8() != 0 {
                let needed = (num_vertices as usize).saturating_mul(2);
                if self.r.pos().saturating_add(needed) > block_end {
                    self.r.set_error();
                    return None;
                }
                self.r.skip(needed);
            }

            if self.r.u8() != 0 {
                let needed = (num_vertices as usize)
                    .saturating_mul(num_weights_per_vertex as usize)
                    .saturating_mul(4);
                if self.r.pos().saturating_add(needed) > block_end {
                    self.r.set_error();
                    return None;
                }
                part.packed_weights
                    .resize(num_vertices as usize, PackedWeight::default());
                for weight in part.packed_weights.iter_mut() {
                    for slot in 0..num_weights_per_vertex as usize {
                        let value = self.r.f32();
                        if slot < MAX_WEIGHTS_PER_VERTEX as usize {
                            weight.weights[slot] = value;
                        }
                    }
                }
            }

            // Strip lengths are present for every partition, even at zero strips.
            let mut strip_lengths = Vec::with_capacity(num_strips as usize);
            for _ in 0..num_strips {
                strip_lengths.push(self.r.u16());
            }
            if self.r.has_error() {
                return None;
            }

            let has_faces = self.r.u8() != 0;
            if self.r.has_error() {
                return None;
            }

            let mut total_indices = 0u32;
            if has_faces {
                if num_strips != 0 {
                    for length in &strip_lengths {
                        total_indices = total_indices.saturating_add(*length as u32);
                    }
                } else {
                    total_indices = num_triangles.saturating_mul(3);
                }
            }

            let needed = (total_indices as usize).saturating_mul(2);
            if self.r.pos().saturating_add(needed) > block_end {
                self.r.set_error();
                return None;
            }
            part.indices.reserve(total_indices as usize);
            for _ in 0..total_indices {
                part.indices.push(self.r.u16());
            }

            if self.r.u8() != 0 {
                let needed =
                    (num_vertices as usize).saturating_mul(num_weights_per_vertex as usize);
                if self.r.pos().saturating_add(needed) > block_end {
                    self.r.set_error();
                    return None;
                }
                if part.packed_weights.len() < num_vertices as usize {
                    part.packed_weights
                        .resize(num_vertices as usize, PackedWeight::default());
                }
                for weight in part.packed_weights.iter_mut() {
                    for slot in 0..num_weights_per_vertex as usize {
                        let value = self.r.u8() as u16;
                        if slot < MAX_WEIGHTS_PER_VERTEX as usize {
                            weight.bone_indices[slot] = value;
                        }
                    }
                }
            }
            if self.r.has_error() {
                return None;
            }

            partition.partitions.push(part);
        }

        Some(partition)
    }

    // -----------------------------------------------------------------
    // Collision
    // -----------------------------------------------------------------

    /// `bhkCollisionObject` / `bhkBlendCollisionObject` with body and shape
    /// resolved. A body or shape that cannot be read is reported by leaving it
    /// at its default, which is what the original does too (it logs and
    /// continues), so a collision object whose body is broken still comes back.
    pub(crate) fn parse_collision_object(&mut self) -> Option<CollisionObject> {
        let index =
            self.seek_to_any_block_of_type(&["bhkCollisionObject", "bhkBlendCollisionObject"])?;
        let is_blend = self.block_type_name(index) == "bhkBlendCollisionObject";

        let mut object = CollisionObject {
            node_index: self.r.u32(),
            ..CollisionObject::default()
        };
        let _flags = self.r.u16();
        object.rigid_body_index = self.r.u32();
        if self.r.has_error() {
            return None;
        }
        if is_blend {
            self.r.skip(8); // Heir Gain + Vel Gain
            if self.r.has_error() {
                return None;
            }
        }

        if let Some(read) = self.parse_rigid_body_at(object.rigid_body_index) {
            // The original writes the body through its reference even when the
            // read stopped partway, so a partial body is still what a caller
            // sees; the shape is only followed on a complete read.
            object.body_info = read.info;
            if read.complete && read.shape_index != 0xFFFF_FFFF {
                if let Some(shape) = self.parse_shape_at(read.shape_index) {
                    object.shape = shape;
                }
            }
        }

        // The Target is a block index; surface the node name when it is in
        // range, as the original does.
        if let Some(node) = self.nodes.get(object.node_index as usize) {
            object.target_name = node.name.clone();
        }

        Some(object)
    }

    /// `bhkRigidBody` / `bhkRigidBodyT` at the first block of either type. The
    /// T variant derives from the plain one without adding a single field.
    pub(crate) fn parse_rigid_body(&mut self) -> Option<RigidBodyRead> {
        let index = self.seek_to_any_block_of_type(&["bhkRigidBody", "bhkRigidBodyT"])?;
        self.parse_rigid_body_at(index)
    }

    /// The rigid body at a known block: its values (possibly partial), its
    /// shape reference, and how far the read got.
    ///
    /// Layout (version 20.0.0.4): Shape | Havok Filter | World Object Info(20)
    /// | Entity Info(4) | Rigid Body Info(196) | Num Constraints | Constraints |
    /// Body Flags.
    ///
    /// The legacy files below 10.1.0.0 are shorter than that sum, so a reader
    /// that follows the layout runs past the block and trips the constraint
    /// check on the bytes that follow — which is where the original stops too,
    /// leaving everything it read in the caller's struct.
    pub(crate) fn parse_rigid_body_at(&mut self, index: u32) -> Option<RigidBodyRead> {
        let offset = self.locate(index)?;
        self.r.set_pos(offset);

        let shape_index = self.r.u32();

        // Havok Filter at the bhkWorldObject level carries the layer/group bits
        // the rest of the engine uses for collision filtering.
        let layer = self.r.u8();
        let filter_flags = self.r.u8();
        let group = self.r.u16();
        let mut info = RigidBodyInfo {
            collision_group: layer as u32,
            collision_filter: group as u32,
            is_trigger: (filter_flags & 0x80) != 0,
            ..RigidBodyInfo::default()
        };

        self.r.skip(20); // World Object Info
        self.r.skip(4); // Entity Info

        // bhkRigidBodyCInfo550_660 (196 bytes)
        self.r.skip(4); // Unused 01
        self.r.skip(4); // Havok Filter (duplicate)
        self.r.skip(4); // Unused 02
        self.r.u8(); // Collision Response
        self.r.skip(1); // Unused 03
        self.r.skip(2); // Process Contact Callback Delay
        self.r.skip(4); // Unused 04

        let translation = self.hk_vector3();
        let rotation = self.hk_vector3(); // hkQuaternion
        info.linear_velocity = self.hk_vector3();
        info.angular_velocity = self.hk_vector3();
        self.r.skip(48); // Inertia Tensor (hkMatrix3)
        self.r.skip(16); // Center (hkVector4)

        info.mass = self.r.f32();
        self.r.f32(); // Linear Damping
        self.r.f32(); // Angular Damping
        info.friction = self.r.f32();
        info.restitution = self.r.f32();
        self.r.f32(); // Max Linear Velocity
        self.r.f32(); // Max Angular Velocity
        self.r.f32(); // Penetration Depth
        self.r.u8(); // Motion System
        self.r.u8(); // Deactivator Type
        self.r.u8(); // Solver Deactivation
        self.r.u8(); // Quality Type
        self.r.skip(12); // Unused 05
        if self.r.has_error() {
            return Some(RigidBodyRead {
                info,
                shape_index,
                complete: false,
            });
        }

        info.transform.translation = translation;
        let _rotation = rotation; // Quaternion order needs the Havok convention

        let num_constraints = self.r.u32();
        if self.r.has_error() {
            return Some(RigidBodyRead {
                info,
                shape_index,
                complete: false,
            });
        }
        if num_constraints > MAX_RIGID_BODY_CONSTRAINTS {
            // The original reports this and returns false without touching the
            // sticky flag; nothing reads again on this path either way.
            return Some(RigidBodyRead {
                info,
                shape_index,
                complete: false,
            });
        }
        self.r.skip_ref_array(num_constraints);
        self.r.skip(4); // Body Flags
        if self.r.has_error() {
            return Some(RigidBodyRead {
                info,
                shape_index,
                complete: false,
            });
        }

        Some(RigidBodyRead {
            info,
            shape_index,
            complete: true,
        })
    }

    /// The first shape-derived block in the file. `bhkShape` is abstract: the
    /// concrete reader is picked from the block's own type name.
    pub(crate) fn parse_shape(&mut self) -> Option<CollisionShape> {
        let index = self.seek_to_any_block_of_type(&[
            "bhkSphereShape",
            "bhkBoxShape",
            "bhkCapsuleShape",
            "bhkConvexVerticesShape",
            "bhkNiTriStripsShape",
            "bhkMoppBvTreeShape",
            "bhkListShape",
            "bhkMeshShape",
        ])?;
        self.parse_shape_at(index)
    }

    /// The shape carried by a known block.
    pub(crate) fn parse_shape_at(&mut self, index: u32) -> Option<CollisionShape> {
        let type_name = self.block_type_name(index);
        let offset = self.locate(index)?;
        self.r.set_pos(offset);

        let shape = match type_name.as_str() {
            "bhkSphereShape" => self.read_sphere_shape(),
            "bhkBoxShape" => self.read_box_shape(),
            "bhkCapsuleShape" => self.read_capsule_shape(),
            "bhkConvexVerticesShape" => self.read_convex_vertices_shape(),
            "bhkNiTriStripsShape" => self.read_ni_tri_strips_shape(),
            "bhkMoppBvTreeShape" => self.read_mopp_bv_tree_shape(),
            "bhkListShape" => self.read_list_shape(),
            "bhkMeshShape" => self.read_mesh_shape(),
            _ => {
                // Unsupported shape type: the original reports failure here.
                self.r.set_error();
                CollisionShape::default()
            }
        };
        // Every reader leaves the sticky flag raised when it gave up, so this
        // is where "could not read this shape" becomes `None` — the same point
        // the C++ reader returns false.
        if self.r.has_error() {
            return None;
        }
        Some(shape)
    }

    /// Material | Radius | Unused01[8] | Dimensions(hkVector4) | Unused Float.
    fn read_box_shape(&mut self) -> CollisionShape {
        self.r.skip(4); // Material
        let radius = self.r.f32();
        self.r.skip(8); // Unused 01
        let half_extents = self.hk_vector3();
        self.r.skip(4); // Unused Float
        if self.r.has_error() {
            return CollisionShape::default();
        }
        CollisionShape {
            shape_type: ShapeType::Box,
            half_extents,
            radius,
            ..CollisionShape::default()
        }
    }

    /// Material | Radius.
    fn read_sphere_shape(&mut self) -> CollisionShape {
        self.r.skip(4); // Material
        let radius = self.r.f32();
        if self.r.has_error() {
            return CollisionShape::default();
        }
        CollisionShape {
            shape_type: ShapeType::Sphere,
            radius,
            ..CollisionShape::default()
        }
    }

    /// Material | Radius | Unused01[8] | Point1(hkVector4) | Radius1 |
    /// Point2(hkVector4) | Radius2. The capsule runs between the two points.
    fn read_capsule_shape(&mut self) -> CollisionShape {
        self.r.skip(4); // Material
        let radius = self.r.f32();
        self.r.skip(8); // Unused 01
        let first_point = self.hk_vector3();
        self.r.skip(4); // Radius 1
        let second_point = self.hk_vector3();
        self.r.skip(4); // Radius 2
        if self.r.has_error() {
            return CollisionShape::default();
        }

        let dx = second_point.x - first_point.x;
        let dy = second_point.y - first_point.y;
        let dz = second_point.z - first_point.z;
        CollisionShape {
            shape_type: ShapeType::Capsule,
            center: Vector3 {
                x: (first_point.x + second_point.x) * 0.5,
                y: (first_point.y + second_point.y) * 0.5,
                z: (first_point.z + second_point.z) * 0.5,
            },
            radius,
            height: (dx * dx + dy * dy + dz * dz).sqrt(),
            ..CollisionShape::default()
        }
    }

    /// Material | Radius | two properties (24 bytes) | Num Vertices | Vertices
    /// (hkVector4 each) | Num Normals | Normals (skipped).
    fn read_convex_vertices_shape(&mut self) -> CollisionShape {
        self.r.skip(4); // Material
        let radius = self.r.f32();
        self.r.skip(24); // Vertices + Normals properties

        let num_vertices = self.r.u32();
        if self.r.has_error() {
            return CollisionShape::default();
        }
        if num_vertices > MAX_CONVEX_VERTICES {
            self.r.set_error();
            return CollisionShape::default();
        }
        let mut vertices = Vec::with_capacity(num_vertices as usize);
        for _ in 0..num_vertices {
            vertices.push(self.hk_vector3());
        }
        if self.r.has_error() {
            return CollisionShape::default();
        }

        let num_normals = self.r.u32();
        if self.r.has_error() {
            return CollisionShape::default();
        }
        if num_normals > MAX_CONVEX_VERTICES {
            self.r.set_error();
            return CollisionShape::default();
        }
        self.r.skip((num_normals as usize).saturating_mul(16));
        if self.r.has_error() {
            return CollisionShape::default();
        }

        CollisionShape {
            shape_type: ShapeType::ConvexHull,
            radius,
            vertices,
            ..CollisionShape::default()
        }
    }

    /// Material | Radius | Unused01[20] | Grow By | Scale | Num Strips Data |
    /// Strips Data refs | Num Filters | Filters. The strip vertices live in
    /// separate `bhkNiTriStripsData` blocks, so only the references are kept.
    fn read_ni_tri_strips_shape(&mut self) -> CollisionShape {
        self.r.skip(4); // Material
        let radius = self.r.f32();
        self.r.skip(20); // Unused 01
        self.r.skip(4); // Grow By
        self.r.skip(16); // Scale

        let num_strips_data = self.r.u32();
        if self.r.has_error() {
            return CollisionShape::default();
        }
        if num_strips_data > MAX_STRIPS_DATA {
            self.r.set_error();
            return CollisionShape::default();
        }
        let mut strips_data_refs = Vec::with_capacity(num_strips_data as usize);
        for _ in 0..num_strips_data {
            strips_data_refs.push(self.r.u32());
        }

        let num_filters = self.r.u32();
        if self.r.has_error() {
            return CollisionShape::default();
        }
        if num_filters > MAX_SHAPE_FILTERS {
            self.r.set_error();
            return CollisionShape::default();
        }
        self.r.skip((num_filters as usize).saturating_mul(4));
        if self.r.has_error() {
            return CollisionShape::default();
        }

        CollisionShape {
            shape_type: ShapeType::TriMesh,
            radius,
            strips_data_refs,
            ..CollisionShape::default()
        }
    }

    /// Num Sub Shapes | Sub Shapes refs | Material | two properties | Num
    /// Filters | Filters. The material follows the sub-shape array here, which
    /// is what distinguishes the list shape from every other one.
    fn read_list_shape(&mut self) -> CollisionShape {
        let num_sub_shapes = self.r.u32();
        if self.r.has_error() {
            return CollisionShape::default();
        }
        if num_sub_shapes > MAX_LIST_SUB_SHAPES {
            self.r.set_error();
            return CollisionShape::default();
        }
        let mut sub_shape_refs = Vec::with_capacity(num_sub_shapes as usize);
        for _ in 0..num_sub_shapes {
            sub_shape_refs.push(self.r.u32());
        }

        self.r.skip(4); // Material
        self.r.skip(12); // Child Shape Property
        self.r.skip(12); // Child Filter Property

        let num_filters = self.r.u32();
        if self.r.has_error() {
            return CollisionShape::default();
        }
        if num_filters > MAX_SHAPE_FILTERS {
            self.r.set_error();
            return CollisionShape::default();
        }
        self.r.skip((num_filters as usize).saturating_mul(4));
        if self.r.has_error() {
            return CollisionShape::default();
        }

        CollisionShape {
            shape_type: ShapeType::List,
            sub_shape_refs,
            ..CollisionShape::default()
        }
    }

    /// Shape ref | Unused01[12] | Scale | MOPP code { size | offset[16] |
    /// data }.
    ///
    /// The shape reference is read unconditionally, even on versions where the
    /// walker treats it as a per-block prefix — the original reader does the
    /// same, and the oracle is what would notice if that ever disagreed with a
    /// shipped file.
    fn read_mopp_bv_tree_shape(&mut self) -> CollisionShape {
        let child_shape_ref = self.r.u32();
        self.r.skip(12); // Unused 01
        self.r.f32(); // Scale
        let mopp_data_size = self.r.u32();
        self.r.skip(16); // Offset (hkVector4)
        if self.r.has_error() {
            return CollisionShape::default();
        }
        if mopp_data_size > MAX_MOPP_DATA_SIZE {
            self.r.set_error();
            return CollisionShape::default();
        }

        let mut mopp_data = vec![0u8; mopp_data_size as usize];
        if mopp_data_size > 0 && !self.r.read_bytes(&mut mopp_data) {
            return CollisionShape::default();
        }

        CollisionShape {
            shape_type: ShapeType::MoppBvTree,
            child_shape_ref,
            mopp_data_size,
            mopp_data,
            ..CollisionShape::default()
        }
    }

    /// Material | Radius | unknowns | Num Sub Shapes | refs | Num Vertices |
    /// Vertices (hkVector4, w dropped) | Num Triangles | triangles with a
    /// material word each. 10.0.1.0 only in retail data.
    fn read_mesh_shape(&mut self) -> CollisionShape {
        self.r.u32(); // Material
        let radius = self.r.f32();
        self.r.skip(8); // Unknown 01
        self.r.skip(4); // Unknown 02
        self.r.skip(4); // Unknown 03

        let num_sub_shapes = self.r.u32();
        for _ in 0..num_sub_shapes {
            if self.r.has_error() {
                break;
            }
            self.r.u32(); // Sub shape ref
        }
        self.r.skip(4); // Unknown 04
        self.r.skip(4); // Unknown 05

        let num_vertices = self.r.u32();
        let mut vertices = Vec::with_capacity(num_vertices as usize);
        for _ in 0..num_vertices {
            if self.r.has_error() {
                break;
            }
            vertices.push(self.hk_vector3());
        }
        if self.r.has_error() {
            return CollisionShape::default();
        }

        let num_triangles = self.r.u32();
        let mut triangles = Vec::with_capacity(num_triangles as usize);
        for _ in 0..num_triangles {
            if self.r.has_error() {
                break;
            }
            let triangle = [self.r.u16(), self.r.u16(), self.r.u16()];
            self.r.u16(); // per-triangle material
            triangles.push(triangle);
        }
        if self.r.has_error() {
            return CollisionShape::default();
        }

        CollisionShape {
            shape_type: ShapeType::TriMesh,
            radius,
            vertices,
            triangles,
            ..CollisionShape::default()
        }
    }

    // -----------------------------------------------------------------
    // Animation: NiControllerManager and its controller sequences
    //
    // These three readers are the Phase 30 animation path: the manager seeks
    // to the first `NiControllerManager`, resolves every sequence reference it
    // declares, and each sequence in turn resolves its `NiTextKeyExtraData`
    // block. The layout comments mirror `nif_parser.cpp` field for field —
    // including that the original reads a sequence's text keys through
    // `readStringRef` placeholders rather than the real strings, which the
    // differential oracle compares byte for byte.
    // -----------------------------------------------------------------

    /// The first `NiControllerManager` of the file, with every sequence it
    /// references resolved — a sequence that cannot be read is skipped and
    /// counted, which is what the original does (it logs and continues).
    pub(crate) fn parse_controller_manager(&mut self) -> Option<ControllerManager> {
        self.seek_to_block_of_type("NiControllerManager")?;

        // NiTimeController: Next Controller | Flags | Frequency | Phase |
        // Start Time | Stop Time | Target. The original keeps these in
        // locals for its log line only; the struct fields they feed are
        // assigned afterwards.
        self.r.u32(); // Next Controller
        self.r.u16(); // Flags
        self.r.f32(); // Frequency
        self.r.f32(); // Phase
        self.r.f32(); // Start Time
        self.r.f32(); // Stop Time
        self.r.u32(); // Target
        self.r.u8(); // Cumulative

        let controller_sequence_count = self.r.u32();
        if self.r.has_error() {
            return None;
        }
        if controller_sequence_count > MAX_CONTROLLER_SEQUENCES {
            // The original logs and returns false without raising the sticky
            // flag, so the limit must not raise it either: the caller may go
            // on to read other blocks with this reader.
            return None;
        }

        let mut sequence_refs = Vec::with_capacity(controller_sequence_count as usize);
        for _ in 0..controller_sequence_count {
            sequence_refs.push(self.r.u32());
        }
        let object_palette_index = self.r.u32();
        if self.r.has_error() {
            return None;
        }

        // lastTime has no counterpart in this block; the original keeps it at
        // zero so consumers never read a stale clock.
        let mut sequences = Vec::with_capacity(sequence_refs.len());
        for reference in sequence_refs {
            if let Some(sequence) = self.parse_controller_sequence_at(reference) {
                sequences.push(sequence);
            }
        }

        Some(ControllerManager {
            // The block carries no name field; the original never writes the
            // struct's `name` either, so it stays at its default.
            name: String::new(),
            controller_sequence_count,
            sequences,
            object_palette_index,
            last_time: 0.0,
        })
    }

    /// The first `NiControllerSequence` of the file.
    pub(crate) fn parse_controller_sequence(&mut self) -> Option<ControllerSequence> {
        let index = self.seek_to_block_of_type("NiControllerSequence")?;
        self.parse_controller_sequence_at(index)
    }

    /// The sequence at a known block: its name, the controlled block array,
    /// the timing fields, and its text keys.
    ///
    /// Reading the text keys moves the cursor into another block, so the
    /// sequence position is saved and restored around that call — and only
    /// the position: the original saves no error flag, so a text-key read
    /// that ran past the end of the file leaves the sticky failure in place
    /// for the caller, exactly as here.
    pub(crate) fn parse_controller_sequence_at(
        &mut self,
        index: u32,
    ) -> Option<ControllerSequence> {
        let offset = self.locate(index)?;
        self.r.set_pos(offset);

        // NiControllerSequence layout (version 20.0.0.4):
        //   Name:SizedString | Num Controlled Blocks | Array Grow By |
        //   ControlledBlocks[N] | Weight | Text Keys | Cycle Type | Frequency |
        //   Start Time | Stop Time | Manager | Accum Root Name | String Palette.
        // Play Backwards only exists in 10.1.0.106 and Phase only up to
        // 10.4.0.1, so neither is present here — as in the original, which
        // reads the 20.0.0.4 layout unconditionally.
        let name = self.r.string().ok()?;
        let num_controlled_blocks = self.r.u32();
        self.r.skip(4); // Array Grow By
        if self.r.has_error() {
            return None;
        }
        if num_controlled_blocks > MAX_CONTROLLED_BLOCKS {
            // As above: the original returns false without raising the flag.
            return None;
        }

        let mut controlled_blocks = Vec::with_capacity(num_controlled_blocks as usize);
        for _ in 0..num_controlled_blocks {
            let mut block = KeyframeController {
                interpolator_index: self.r.u32(),
                controller_index: self.r.u32(),
                priority: self.r.u8(),
                string_palette_index: self.r.u32(),
                node_name_offset: self.r.u32(),
                property_type_offset: self.r.u32(),
                controller_type_offset: self.r.u32(),
                controller_id_offset: self.r.u32(),
                interpolator_id_offset: self.r.u32(),
                ..KeyframeController::default()
            };
            // AnimationPlayer consumes keyframeDataIndex as an index into the
            // keyframe data array, which resolveControllerReferences() fills
            // in — the original assigns both here and never touches `clip`.
            block.keyframe_data_index = block.controller_index;
            block.resolved_bone_index = -1;
            controlled_blocks.push(block);
        }

        self.r.skip(4); // Weight
        let text_key_block = self.r.u32();
        let cycle_type = self.r.u32();
        let frequency = self.r.f32();
        let start_time = self.r.f32();
        let stop_time = self.r.f32();
        let controller_manager_index = self.r.u32();
        let target_name = self.r.string().ok()?;
        let _string_palette = self.r.u32();
        if self.r.has_error() {
            return None;
        }

        // Text Keys is a reference to a NiTextKeyExtraData block, not inline
        // data. Reading it moves the cursor, so the sequence position is
        // restored afterwards.
        let mut text_keys = Vec::new();
        if text_key_block != 0xFFFF_FFFF && text_key_block != 0 {
            if let Some(text_key_offset) = self.locate(text_key_block) {
                let saved_position = self.r.pos();
                self.r.set_pos(text_key_offset);
                self.parse_text_keys(&mut text_keys);
                self.r.set_pos(saved_position);
            }
        }

        Some(ControllerSequence {
            name,
            controller_manager_index,
            target_name,
            duration: stop_time - start_time,
            frequency,
            phase: 0.0, // Phase is absent in 20.0.0.4
            start_time,
            stop_time,
            is_loop: cycle_type == 0,
            controlled_blocks,
            text_keys,
        })
    }

    /// `NiTextKeyExtraData` at the current cursor: the name read as a bare
    /// u32 (the original's `readStringRef`, not the SizedString the walker
    /// skips), then `Num Keys` and the keys themselves — a time and another
    /// `readStringRef` placeholder per key.
    ///
    /// The count is bounded by the bytes that remain — the original resizes a
    /// vector straight from the file's count, which a corrupt block turns into
    /// a multi-gigabyte allocation, so a count past the end of the buffer fails
    /// the read instead. On any file the walker measured, the keys fit in the
    /// block and the bound never fires. The original always reports success,
    /// so this returns nothing and leaves the keys it read.
    fn parse_text_keys(&mut self, text_keys: &mut Vec<TextKey>) {
        self.r.u32(); // Name, as a string reference
        let num_keys = self.r.u32();
        if self.r.has_error() {
            return;
        }
        let remaining = self.r.len().saturating_sub(self.r.pos()) / 8;
        if num_keys as usize > remaining {
            self.r.set_error();
            return;
        }
        text_keys.clear();
        text_keys.reserve(num_keys as usize);
        for _ in 0..num_keys {
            let time = self.r.f32();
            let value = self.r.string_ref().unwrap_or_default();
            text_keys.push(TextKey { time, value });
        }
    }
}

/// One text key of a `NiTextKeyExtraData` block: a timestamp and the value.
///
/// The original does not read the key's real string — it consumes a u32 and
/// writes the placeholder `string_<index>` instead (its `readStringRef` is
/// explicitly a stand-in for a string-table lookup that was never built).
/// The differential oracle compares those placeholders, so they are
/// reproduced exactly.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct TextKey {
    pub time: f32,
    pub value: String,
}

/// One entry of a sequence's controlled block array: which controller it
/// drives, where in the sequence's `NiStringPalette` its names live, and the
/// two fields the original seeds while reading.
///
/// The struct mirrors `NIFKeyframeController` because that is what the
/// original stores these in; `target_node_index` and `clip` are left at their
/// defaults here exactly as the original leaves them — `resolveControllerReferences()`
/// is what fills them, and that is not part of this wave.
#[derive(Clone, Debug, PartialEq)]
pub struct KeyframeController {
    pub target_node_index: u32,
    pub keyframe_data_index: u32,
    pub clip: AnimationClip,
    pub resolved_bone_index: i32,
    pub interpolator_index: u32,
    pub controller_index: u32,
    pub string_palette_index: u32,
    pub priority: u8,
    pub node_name_offset: u32,
    pub property_type_offset: u32,
    pub controller_type_offset: u32,
    pub controller_id_offset: u32,
    pub interpolator_id_offset: u32,
}

impl Default for KeyframeController {
    fn default() -> Self {
        Self {
            target_node_index: 0,
            keyframe_data_index: 0,
            clip: AnimationClip::default(),
            // NIFKeyframeController seeds this at -1; AnimationPlayer treats
            // it as "no bone resolved yet".
            resolved_bone_index: -1,
            interpolator_index: 0,
            controller_index: 0,
            string_palette_index: 0,
            priority: 0,
            node_name_offset: 0,
            property_type_offset: 0,
            controller_type_offset: 0,
            controller_id_offset: 0,
            interpolator_id_offset: 0,
        }
    }
}

/// One parsed `NiControllerSequence`.
#[derive(Clone, Debug, PartialEq)]
pub struct ControllerSequence {
    pub name: String,
    pub controller_manager_index: u32,
    pub target_name: String,
    pub duration: f32,
    pub frequency: f32,
    pub phase: f32,
    pub start_time: f32,
    pub stop_time: f32,
    /// Cycle type 0 means the sequence loops; the original stores it as
    /// `loop`, which is a keyword in Rust.
    pub is_loop: bool,
    pub controlled_blocks: Vec<KeyframeController>,
    pub text_keys: Vec<TextKey>,
}

impl Default for ControllerSequence {
    fn default() -> Self {
        Self {
            name: String::new(),
            controller_manager_index: 0,
            target_name: String::new(),
            duration: 0.0,
            // NIFControllerSequence starts its frequency at 1, not 0.
            frequency: 1.0,
            phase: 0.0,
            start_time: 0.0,
            stop_time: 0.0,
            is_loop: false,
            controlled_blocks: Vec::new(),
            text_keys: Vec::new(),
        }
    }
}

/// One parsed `NiControllerManager`: the count the file declares, the
/// sequences that could be read, and the object palette they hang off.
///
/// `name` has no counterpart in the block — the original never writes it.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct ControllerManager {
    pub name: String,
    pub controller_sequence_count: u32,
    pub sequences: Vec<ControllerSequence>,
    pub object_palette_index: u32,
    pub last_time: f32,
}

/// An animation clip: name, span and the keyframes inside it. The readers in
/// this wave never fill one — `resolveControllerReferences()` does — so it
/// travels through the ABI as its defaults.
#[derive(Clone, Debug, PartialEq)]
pub struct AnimationClip {
    pub name: String,
    pub duration: f32,
    pub frequency: f32,
    pub keyframes: Vec<AnimationKeyframe>,
}

impl Default for AnimationClip {
    fn default() -> Self {
        Self {
            name: String::new(),
            duration: 0.0,
            // NIFAnimationClip starts its frequency at 1, not 0.
            frequency: 1.0,
            keyframes: Vec::new(),
        }
    }
}

/// One keyframe of an [`AnimationClip`], matching `NIFKeyframe`.
#[derive(Clone, Debug, PartialEq)]
pub struct AnimationKeyframe {
    pub time: f32,
    pub rotation: Vector4,
    pub translation: Vector3,
    pub scale: f32,
    pub forward_tangent: Vector4,
    pub backward_tangent: Vector4,
}

impl Default for AnimationKeyframe {
    fn default() -> Self {
        Self {
            time: 0.0,
            rotation: Vector4::default(),
            translation: Vector3::default(),
            // NIFKeyframe starts its scale at 1, not 0.
            scale: 1.0,
            forward_tangent: Vector4::default(),
            backward_tangent: Vector4::default(),
        }
    }
}
