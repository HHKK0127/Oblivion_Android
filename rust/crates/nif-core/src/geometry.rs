//! Geometry decode: the `NiTriShape` / `NiTriStrips` scene blocks and their
//! `NiTriShapeData` / `NiTriStripsData` payloads.
//!
//! Everything here anchors on the byte ranges the walker recorded, because a
//! block body has no size of its own: `locate` positions the cursor on the
//! first field of a body and the reads below then follow the same field order
//! the walker skipped. The two must stay in step or the payload lands on the
//! wrong bytes, so each read mirrors its `skip*` counterpart in [`crate::walk`].
//!
//! The diffuse texture is resolved last, through two other block bodies, and
//! only after the shape body has been fully consumed — a shape whose
//! `NiTexturingProperty` names no base texture keeps an empty string, which is
//! what callers treat as "untextured".

use crate::header::Header;
use crate::reader::Reader;
use crate::walk::{locate_block, WalkState};
use crate::{Geometry, Matrix3x3, Node, Transform, Vector3, Vector4};

/// Decodes every mesh block of one file into its node.
pub(crate) struct Decoder<'a> {
    r: Reader<'a>,
    header: &'a Header,
    walk: &'a WalkState,
    nodes: &'a mut [Node],
}

impl<'a> Decoder<'a> {
    pub(crate) fn new(
        data: &'a [u8],
        header: &'a Header,
        walk: &'a WalkState,
        nodes: &'a mut [Node],
    ) -> Self {
        Self {
            r: Reader::new(data),
            header,
            walk,
            nodes,
        }
    }

    /// Decode every `NiTriShape` / `NiTriStrips` block. Returns how many of
    /// them produced a mesh — the count the original parser logs after a
    /// decode.
    pub(crate) fn run(&mut self) -> usize {
        let block_count = self.header.block_type_indices.len();
        let mut mesh_count = 0;
        for index in 0..block_count {
            let type_name = self.block_type_name(index as u32);
            if type_name != "NiTriShape" && type_name != "NiTriStrips" {
                continue;
            }
            if index >= self.nodes.len() {
                continue;
            }
            if self.parse_geometry_node(index) {
                mesh_count += 1;
            }
        }
        mesh_count
    }

    /// Resolved type name of one block, empty when the index is out of range.
    fn block_type_name(&self, index: u32) -> String {
        self.header
            .block_type_indices
            .get(index as usize)
            .and_then(|type_index| self.header.block_type_names.get(*type_index as usize))
            .cloned()
            .unwrap_or_default()
    }

    /// First field of a block body, past the per-block prefix. The walk gate
    /// matches `locate_block_body`: a stopped walk yields nothing.
    fn locate(&self, index: u32) -> Option<usize> {
        locate_block(self.walk, self.header, index)
    }

    /// Put the cursor and the sticky flag back the way they were before a
    /// block-anchored read wandered off to another body.
    fn restore(&mut self, position: usize, error: bool) {
        self.r.set_pos(position);
        if error {
            self.r.set_error();
        } else {
            self.r.clear_error();
        }
    }

    // -----------------------------------------------------------------
    // Typed readers
    // -----------------------------------------------------------------

    /// NiTriShape / NiTriStrips: the NiObjectNET name, the NiAVObject transform
    /// and properties, then the NiGeometry payload reference. The byte order
    /// matches `skipNiGeometry()` in the walker, which produced this block's
    /// range.
    ///
    /// Returns whether the block produced a usable mesh. `has_geometry` is set
    /// as soon as the transform has been read, exactly like the original: a
    /// block that fails after that point is left flagged with a partial
    /// payload.
    fn parse_geometry_node(&mut self, index: usize) -> bool {
        let body_offset = match self.locate(index as u32) {
            Some(offset) => offset,
            None => return false,
        };
        self.r.set_pos(body_offset);
        self.r.clear_error();

        // The name is written as soon as it is read: a later failure leaves it
        // in the node, just as `readString(node->name)` does in the original.
        let name = match self.r.string() {
            Ok(name) => name,
            Err(_) => return false,
        };

        let extra_data_count = self.r.u32();
        if self.r.has_error() {
            return false;
        }
        self.r.skip_ref_array(extra_data_count);
        self.r.skip(4); // Controller

        let mut geometry = Geometry::default();
        geometry.name = name.clone();
        self.nodes[index].name = name;

        let (success, has_geometry) = self.decode_shape(&mut geometry);
        if has_geometry {
            self.nodes[index].geometry = Some(geometry);
        }
        self.nodes[index].has_geometry = has_geometry;
        success
    }

    /// Everything from the NiAVObject transform onwards.
    ///
    /// `NiTriShape` and `NiTriStrips` reach this point through
    /// `parseNiTriShape` / `parseNiTriStrips`, which do the same work: the
    /// payload type only decides which data block may follow, which
    /// [`read_geometry_data`] checks.
    ///
    /// Returns `(success, has_geometry)`. `has_geometry` flips to true the
    /// moment the transform has been read and is finally replaced by the
    /// payload result, mirroring `parseGeometryTransform` followed by
    /// `node->hasGeometry = readGeometryData(...)`.
    fn decode_shape(&mut self, geometry: &mut Geometry) -> (bool, bool) {
        // NiAVObject: Flags | Translation | Rotation | Scale. The same field
        // order as `skipNiAVObject()`, which measured this block.
        let mut has_geometry = true;
        self.r.skip(2); // Flags
        geometry.transform = self.read_transform();
        if !self.header.has_block_type_table {
            self.r.skip(12); // Velocity (until 4.2.2.0)
        }
        if self.r.has_error() {
            return (false, has_geometry);
        }

        // The material and texturing properties are refs into the block table,
        // so they are recorded by index for the caller to resolve.
        let property_count = self.r.u32();
        if self.r.has_error() {
            return (false, has_geometry);
        }
        for _ in 0..property_count {
            let property_ref = self.r.u32();
            if self.r.has_error() {
                return (false, has_geometry);
            }
            let property_type = self.block_type_name(property_ref);
            if property_type == "NiMaterialProperty" && geometry.material_property_index == u32::MAX
            {
                geometry.material_property_index = property_ref;
            } else if property_type == "NiTexturingProperty"
                && geometry.texturing_property_index == u32::MAX
            {
                geometry.texturing_property_index = property_ref;
            }
        }

        self.r.skip(4); // Collision Object

        let data_ref = self.r.u32();
        self.r.skip(4); // Skin Instance
        let has_shader = self.r.u8();
        if self.r.has_error() {
            return (false, has_geometry);
        }
        if has_shader != 0 {
            if self.r.string().is_err() {
                return (false, has_geometry);
            }
            self.r.skip(4); // Shader Extra Data
        }

        let data_type = self.block_type_name(data_ref);
        if data_type != "NiTriShapeData" && data_type != "NiTriStripsData" {
            // No mesh payload behind this ref: collision-only shapes, markers
            // and particles legitimately end here.
            return (false, has_geometry);
        }

        has_geometry = self.read_geometry_data(data_ref, &data_type, geometry);
        if !has_geometry {
            return (false, has_geometry);
        }

        // Resolve the diffuse texture once the shape body has been fully
        // consumed, because the resolver moves the cursor into two other block
        // bodies.
        if geometry.texturing_property_index != u32::MAX {
            let mut path = String::new();
            if self.resolve_texture_path(geometry.texturing_property_index, &mut path) {
                geometry.diffuse_texture = path;
            }
        }
        (true, has_geometry)
    }

    /// NiGeometryData plus the per-type tail. The field order mirrors
    /// `skipNiGeometryData()`, and the two must stay in step or the
    /// strip/triangle payload below the shared header lands on the wrong
    /// bytes.
    ///
    /// Returns false when the block holds no vertices.
    fn read_geometry_data(
        &mut self,
        block_index: u32,
        type_name: &str,
        geometry: &mut Geometry,
    ) -> bool {
        let body_offset = match self.locate(block_index) {
            Some(offset) => offset,
            None => return false,
        };
        self.r.set_pos(body_offset);
        self.r.clear_error();

        if self.header.version >= 0x0A01_0072 {
            self.r.skip(4); // Group ID (since 10.1.0.114)
        }
        let num_vertices = self.r.u16() as u32;
        if self.r.has_error() {
            return false;
        }
        if self.header.version >= 0x0A01_0000 {
            self.r.skip(2); // Keep Flags, Compress Flags
        }

        if self.r.u8() != 0 {
            // Has Vertices
            for _ in 0..num_vertices {
                if self.r.has_error() {
                    break;
                }
                geometry.vertices.push(self.read_vector3());
            }
            if self.r.has_error() {
                return false;
            }
        }

        let data_flags = self.r.u16();
        if self.r.has_error() {
            return false;
        }

        if self.r.u8() != 0 {
            // Has Normals
            for _ in 0..num_vertices {
                if self.r.has_error() {
                    break;
                }
                geometry.normals.push(self.read_vector3());
            }
            if self.r.has_error() {
                return false;
            }
            // Tangents are only present on the 10.1+ streams, where Data Flags
            // bit 12 gates them; the 20.x meshes this port ships carry none.
            if self.header.version >= 0x0A01_0000 && (data_flags & 0x1000) != 0 {
                self.r.skip_vector3_array(num_vertices * 2); // Tangents, Bitangents
            }
        }

        self.r.skip(16); // Bounding Sphere

        if self.r.u8() != 0 {
            // Has Vertex Colors
            for _ in 0..num_vertices {
                if self.r.has_error() {
                    break;
                }
                geometry.colors.push(self.read_vector4());
            }
            if self.r.has_error() {
                return false;
            }
        }

        // The low six bits of Data Flags hold the number of UV sets. Only the
        // first set is kept, because a mesh material samples a single diffuse
        // texture.
        let uv_sets = data_flags & 63;
        if uv_sets != 0 {
            for _ in 0..num_vertices {
                if self.r.has_error() {
                    break;
                }
                let u = self.r.f32();
                let v = self.r.f32();
                geometry.tex_coords.push([u, v]);
            }
            if self.r.has_error() {
                return false;
            }
            if uv_sets > 1 {
                self.r.skip(
                    (uv_sets as usize - 1)
                        .saturating_mul(num_vertices as usize)
                        .saturating_mul(8),
                );
            }
        }

        self.r.skip(2); // Consistency Flags
        if self.header.version >= 0x1400_0004 {
            self.r.skip(4); // Additional Data (since 20.0.0.4)
        }

        if type_name == "NiTriShapeData" {
            let num_triangles = self.r.u16() as u32;
            self.r.skip(4); // Num Triangle Points
            let mut has_triangles = true; // Unconditional until 10.0.1.2
            if self.header.version >= 0x0A01_0000 {
                has_triangles = self.r.u8() != 0; // Has Triangles (since 10.1.0.0)
            }
            if self.r.has_error() {
                return false;
            }
            if has_triangles {
                for _ in 0..num_triangles {
                    if self.r.has_error() {
                        break;
                    }
                    let triangle = [self.r.u16(), self.r.u16(), self.r.u16()];
                    geometry.triangles.push(triangle);
                }
                if self.r.has_error() {
                    return false;
                }
            }
            return !geometry.vertices.is_empty();
        }

        // NiTriStripsData
        self.r.skip(2); // Num Triangles
        let num_strips = self.r.u16() as u32;
        if self.r.has_error() {
            return false;
        }
        let mut strip_lengths = Vec::with_capacity(num_strips as usize);
        let mut total_points: usize = 0;
        for _ in 0..num_strips {
            if self.r.has_error() {
                break;
            }
            let length = self.r.u16();
            strip_lengths.push(length);
            total_points += length as usize;
        }
        if self.r.has_error() {
            return false;
        }
        let has_points = if self.header.version >= 0x0A01_0003 {
            self.r.u8()
        } else {
            1
        };
        if self.r.has_error() {
            return false;
        }
        if has_points != 0 {
            // A corrupt strip length can claim more points than the file holds,
            // so the payload is measured against the remaining bytes before it
            // is allocated.
            let remaining = if self.r.pos() <= self.r.len() {
                self.r.len() - self.r.pos()
            } else {
                0
            };
            if total_points > remaining / 2 {
                return false;
            }
            let mut points = Vec::with_capacity(total_points);
            for _ in 0..total_points {
                if self.r.has_error() {
                    break;
                }
                points.push(self.r.u16());
            }
            if self.r.has_error() {
                return false;
            }
            append_strip_triangles(&points, &strip_lengths, &mut geometry.triangles);
        }

        !geometry.vertices.is_empty()
    }

    /// NiTexturingProperty -> base texture slot -> NiSourceTexture -> File Name.
    ///
    /// nif.xml TexDesc starts with the Source ref, so only that first field is
    /// read and the rest of the descriptor is skipped by the same widths the
    /// walker uses.
    fn resolve_texture_path(&mut self, texturing_block_index: u32, out_path: &mut String) -> bool {
        if texturing_block_index >= self.block_count() {
            return false;
        }
        if self.block_type_name(texturing_block_index) != "NiTexturingProperty" {
            return false;
        }

        let body_offset = match self.locate(texturing_block_index) {
            Some(offset) => offset,
            None => return false,
        };
        let saved_position = self.r.pos();
        let saved_error = self.r.has_error();
        self.r.set_pos(body_offset);
        self.r.clear_error();

        self.r.skip_ni_object_net(self.header.has_block_type_table);
        if self.header.version <= 0x0A00_0102 {
            self.r.skip(2); // Flags (until 10.0.1.2)
        }
        self.r.skip(4); // Apply Mode
        let texture_count = self.r.u32();
        if self.r.has_error() || texture_count == 0 {
            self.restore(saved_position, saved_error);
            return false;
        }

        // The base texture is slot 0. Its TexDesc leads with the Source ref.
        let has_base_texture = self.r.u8();
        if self.r.has_error() || has_base_texture == 0 {
            self.restore(saved_position, saved_error);
            return false;
        }
        let source_ref = self.r.u32();

        self.restore(saved_position, saved_error);
        if self.r.has_error() {
            return false;
        }
        self.read_source_texture_path(source_ref, out_path)
    }

    /// NiSourceTexture: the NiObjectNET header, then the external/internal
    /// flags that decide whether a File Name is stored at all. Everything past
    /// it (Pixel Data, Format Prefs, Is Static, Direct Render) is skipped by the
    /// walker's widths.
    fn read_source_texture_path(
        &mut self,
        source_texture_block_index: u32,
        out_path: &mut String,
    ) -> bool {
        if source_texture_block_index >= self.block_count() {
            return false;
        }
        if self.block_type_name(source_texture_block_index) != "NiSourceTexture" {
            return false;
        }

        let body_offset = match self.locate(source_texture_block_index) {
            Some(offset) => offset,
            None => return false,
        };
        let saved_position = self.r.pos();
        let saved_error = self.r.has_error();
        self.r.set_pos(body_offset);
        self.r.clear_error();

        self.r.skip_ni_object_net(self.header.has_block_type_table);
        let use_external = self.r.u8();

        let mut found = false;
        if self.header.version < 0x0A01_0000 {
            // Pre-10.1 files store File Name only for external textures;
            // internal ones carry the pixels inline and name nothing.
            if use_external != 0 {
                if let Ok(path) = self.r.string() {
                    *out_path = path;
                    found = true;
                }
            }
        } else if let Ok(path) = self.r.string() {
            // 10.1+ always stores File Name, external or not.
            *out_path = path;
            found = true;
        }
        if self.r.has_error() {
            found = false;
        }

        self.restore(saved_position, saved_error);

        if found {
            // Retail paths use backslashes; every consumer resolves forward
            // slashes.
            *out_path = out_path.replace('\\', "/");
        }
        found && !out_path.is_empty()
    }

    fn block_count(&self) -> u32 {
        self.header.block_type_indices.len() as u32
    }

    fn read_vector3(&mut self) -> Vector3 {
        Vector3 {
            x: self.r.f32(),
            y: self.r.f32(),
            z: self.r.f32(),
        }
    }

    fn read_vector4(&mut self) -> Vector4 {
        Vector4 {
            x: self.r.f32(),
            y: self.r.f32(),
            z: self.r.f32(),
            w: self.r.f32(),
        }
    }

    /// NiAVObject stores Translation, then Rotation, then Scale.
    fn read_transform(&mut self) -> Transform {
        let translation = self.read_vector3();
        let mut rotation = Matrix3x3::default();
        for row in rotation.m.iter_mut() {
            for cell in row.iter_mut() {
                *cell = self.r.f32();
            }
        }
        let scale = self.r.f32();
        Transform {
            rotation,
            translation,
            scale,
        }
    }
}

/// A strip of n points yields n-2 triangles. Every other triangle is wound the
/// other way, which is what keeps the facing consistent along the shared edge.
pub(crate) fn append_strip_triangles(
    points: &[u16],
    strip_lengths: &[u16],
    out: &mut Vec<[u16; 3]>,
) {
    let mut offset = 0usize;
    for strip_length in strip_lengths {
        let strip_length = *strip_length as usize;
        if offset + strip_length > points.len() {
            return;
        }
        for i in 0..strip_length.saturating_sub(2) {
            let triangle = if (i % 2) == 0 {
                [
                    points[offset + i],
                    points[offset + i + 1],
                    points[offset + i + 2],
                ]
            } else {
                [
                    points[offset + i + 1],
                    points[offset + i],
                    points[offset + i + 2],
                ]
            };
            out.push(triangle);
        }
        offset += strip_length;
    }
}
