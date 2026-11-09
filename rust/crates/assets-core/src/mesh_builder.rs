//! NIF geometry merge: many decoded shapes into one drawable buffer.
//!
//! A cell's static mesh arrives as one geometry block per NiTriShape, while the
//! renderer uploads a single interleaved vertex array and one `u32` index array
//! per draw call. This module flattens the blocks and rebases every index into
//! the merged vertex array, so an index can never address another shape's
//! vertices.
//!
//! The vertex layout is fixed at [`VERTEX_FLOATS`] floats in the order the
//! renderer's `Vertex` struct declares them: position, normal, texture
//! coordinate, colour.

/// Floats per merged vertex: position (3), normal (3), texcoord (2), colour (3).
pub const VERTEX_FLOATS: usize = 11;

/// Normal used when a geometry supplies fewer normals than vertices.
pub const DEFAULT_NORMAL: [f32; 3] = [0.0, 0.0, 1.0];
/// Texture coordinate used when a geometry supplies fewer texcoords than vertices.
pub const DEFAULT_TEX_COORD: [f32; 2] = [0.0, 0.0];
/// Colour used when a geometry supplies fewer colours than vertices.
pub const DEFAULT_COLOR: [f32; 3] = [1.0, 1.0, 1.0];

/// One decoded geometry block, borrowed as parallel attribute arrays.
///
/// `positions` fixes the vertex count. The other arrays may be shorter: an
/// attribute that is missing for a particular vertex falls back to its default
/// per vertex, never per geometry, so a shape with no normals cannot inherit the
/// previous shape's normals.
#[derive(Debug, Clone, Copy, Default)]
pub struct GeometryInput<'a> {
    pub positions: &'a [[f32; 3]],
    pub normals: &'a [[f32; 3]],
    pub tex_coords: &'a [[f32; 2]],
    /// `NIFVector4` carries a w component that the renderer's `vec3` colour
    /// drops, so only x, y and z are read.
    pub colors: &'a [[f32; 4]],
    pub triangles: &'a [[u16; 3]],
}

/// The flat buffers the renderer uploads, plus the merge statistics.
#[derive(Debug, Clone, Default, PartialEq)]
pub struct MergedMesh {
    /// `[f32; VERTEX_FLOATS]` per vertex, in upload order.
    pub vertices: Vec<[f32; VERTEX_FLOATS]>,
    /// Triangle indices into `vertices`, rebased across all merged geometries.
    pub indices: Vec<u32>,
    /// Geometries that contributed at least one triangle.
    pub geometry_count: usize,
    /// Geometries that contributed nothing, because they had no vertices or no
    /// triangle that stayed inside their own vertex array.
    pub skipped_geometry_count: usize,
}

/// Merge decoded geometry blocks into one vertex array and one index array.
pub fn merge_geometry(geometries: &[GeometryInput<'_>]) -> MergedMesh {
    let mut merged = MergedMesh::default();

    for geometry in geometries {
        let vertex_count = geometry.positions.len();
        if vertex_count == 0 {
            merged.skipped_geometry_count += 1;
            continue;
        }

        // Collect this geometry's triangles first: a shape can decode its vertex
        // array and still have no usable triangle, and those vertices would be
        // unreachable once merged.
        let mut local_indices: Vec<u16> = Vec::with_capacity(geometry.triangles.len() * 3);
        for triangle in geometry.triangles {
            if usize::from(triangle[0]) >= vertex_count
                || usize::from(triangle[1]) >= vertex_count
                || usize::from(triangle[2]) >= vertex_count
            {
                continue;
            }
            local_indices.push(triangle[0]);
            local_indices.push(triangle[1]);
            local_indices.push(triangle[2]);
        }
        if local_indices.is_empty() {
            merged.skipped_geometry_count += 1;
            continue;
        }

        let base_index = merged.vertices.len() as u32;

        merged.vertices.reserve(vertex_count);
        for (i, position) in geometry.positions.iter().enumerate() {
            let mut vertex = [0.0_f32; VERTEX_FLOATS];
            vertex[0..3].copy_from_slice(position);
            vertex[3..6].copy_from_slice(geometry.normals.get(i).unwrap_or(&DEFAULT_NORMAL));
            vertex[6..8].copy_from_slice(geometry.tex_coords.get(i).unwrap_or(&DEFAULT_TEX_COORD));
            let color = geometry.colors.get(i);
            vertex[8] = color.map_or(DEFAULT_COLOR[0], |c| c[0]);
            vertex[9] = color.map_or(DEFAULT_COLOR[1], |c| c[1]);
            vertex[10] = color.map_or(DEFAULT_COLOR[2], |c| c[2]);
            merged.vertices.push(vertex);
        }

        merged.indices.reserve(local_indices.len());
        for index in local_indices {
            merged.indices.push(base_index + u32::from(index));
        }

        merged.geometry_count += 1;
    }

    merged
}

#[cfg(test)]
mod tests {
    use super::*;

    fn positions(values: &[[f32; 3]]) -> Vec<[f32; 3]> {
        values.to_vec()
    }

    /// A quad: 4 vertices, 2 triangles, indices 0..3.
    fn quad(x_offset: f32) -> (Vec<[f32; 3]>, Vec<[u16; 3]>) {
        let positions = positions(&[
            [x_offset, 0.0, 0.0],
            [x_offset + 1.0, 0.0, 0.0],
            [x_offset + 1.0, 1.0, 0.0],
            [x_offset, 1.0, 0.0],
        ]);
        let triangles = vec![[0, 1, 2], [0, 2, 3]];
        (positions, triangles)
    }

    fn position_of(vertex: &[f32; VERTEX_FLOATS]) -> [f32; 3] {
        [vertex[0], vertex[1], vertex[2]]
    }

    fn normal_of(vertex: &[f32; VERTEX_FLOATS]) -> [f32; 3] {
        [vertex[3], vertex[4], vertex[5]]
    }

    fn tex_coord_of(vertex: &[f32; VERTEX_FLOATS]) -> [f32; 2] {
        [vertex[6], vertex[7]]
    }

    fn color_of(vertex: &[f32; VERTEX_FLOATS]) -> [f32; 3] {
        [vertex[8], vertex[9], vertex[10]]
    }

    #[test]
    fn a_scene_of_several_shapes_merges_instead_of_collapsing_to_the_first() {
        // The regression this module exists for.
        let (wall_positions, wall_triangles) = quad(0.0);
        let (floor_positions, floor_triangles) = quad(10.0);
        let doorway_positions = positions(&[[20.0, 0.0, 0.0], [21.0, 0.0, 0.0], [20.0, 2.0, 0.0]]);
        let doorway_triangles = vec![[0, 1, 2]];

        let merged = merge_geometry(&[
            GeometryInput {
                positions: &wall_positions,
                triangles: &wall_triangles,
                ..Default::default()
            },
            GeometryInput {
                positions: &floor_positions,
                triangles: &floor_triangles,
                ..Default::default()
            },
            GeometryInput {
                positions: &doorway_positions,
                triangles: &doorway_triangles,
                ..Default::default()
            },
        ]);

        assert_eq!(merged.vertices.len(), 11);
        assert_eq!(merged.indices.len(), 15);
        assert_eq!(merged.geometry_count, 3);
        assert_eq!(merged.skipped_geometry_count, 0);

        // The first block keeps its original numbering, so a scene with one shape
        // is byte-identical to the old single-geometry path.
        assert_eq!(&merged.indices[0..3], &[0, 1, 2]);
        // The second and third blocks are rebased by 4 and 8.
        assert_eq!(&merged.indices[6..9], &[4, 5, 6]);
        assert_eq!(&merged.indices[12..15], &[8, 9, 10]);
        // The dropped-block bug in one assertion: the doorway's own position is
        // present at its rebased offset, which the old code never reached.
        assert_eq!(position_of(&merged.vertices[8]), [20.0, 0.0, 0.0]);
    }

    #[test]
    fn every_merged_index_is_inside_the_merged_vertex_array() {
        // An out-of-range index is an out-of-bounds GPU read, not a visual glitch.
        let (a_positions, a_triangles) = quad(0.0);
        let (b_positions, b_triangles) = quad(10.0);
        let c_positions = positions(&[[20.0, 0.0, 0.0], [21.0, 0.0, 0.0]]);
        let c_triangles = vec![[0, 1, 1]];

        let merged = merge_geometry(&[
            GeometryInput {
                positions: &a_positions,
                triangles: &a_triangles,
                ..Default::default()
            },
            GeometryInput {
                positions: &b_positions,
                triangles: &b_triangles,
                ..Default::default()
            },
            GeometryInput {
                positions: &c_positions,
                triangles: &c_triangles,
                ..Default::default()
            },
        ]);

        let vertex_count = merged.vertices.len() as u32;
        assert!(merged.indices.iter().all(|&index| index < vertex_count));
        assert_eq!(merged.indices.len() % 3, 0);
    }

    #[test]
    fn a_position_only_geometry_keeps_the_vertex_defaults() {
        // So a shape with no normals does not inherit the previous shape's data.
        let bare_positions = positions(&[[1.0, 2.0, 3.0], [4.0, 5.0, 6.0], [7.0, 8.0, 9.0]]);
        let bare_triangles = vec![[0, 1, 2]];

        let merged = merge_geometry(&[GeometryInput {
            positions: &bare_positions,
            triangles: &bare_triangles,
            ..Default::default()
        }]);

        assert_eq!(normal_of(&merged.vertices[0]), DEFAULT_NORMAL);
        assert_eq!(tex_coord_of(&merged.vertices[0]), DEFAULT_TEX_COORD);
        assert_eq!(color_of(&merged.vertices[0]), DEFAULT_COLOR);
        assert_eq!(position_of(&merged.vertices[2]), [7.0, 8.0, 9.0]);
    }

    #[test]
    fn supplied_attributes_are_copied_through() {
        let textured_positions = positions(&[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]]);
        let textured_normals = positions(&[[0.0, 0.0, -1.0], [0.0, 1.0, 0.0], [1.0, 0.0, 0.0]]);
        let textured_tex_coords: Vec<[f32; 2]> = vec![[0.25, 0.75], [0.5, 0.5]];
        let textured_colors: Vec<[f32; 4]> = vec![[0.1, 0.2, 0.3, 1.0]];
        let textured_triangles = vec![[0, 1, 2]];

        let merged = merge_geometry(&[GeometryInput {
            positions: &textured_positions,
            normals: &textured_normals,
            tex_coords: &textured_tex_coords,
            colors: &textured_colors,
            triangles: &textured_triangles,
        }]);

        assert_eq!(normal_of(&merged.vertices[2]), [1.0, 0.0, 0.0]);
        assert_eq!(tex_coord_of(&merged.vertices[0]), [0.25, 0.75]);
        assert_eq!(color_of(&merged.vertices[0]), [0.1, 0.2, 0.3]);
        // A short attribute array falls back per vertex, not per geometry.
        assert_eq!(tex_coord_of(&merged.vertices[2]), DEFAULT_TEX_COORD);
        assert_eq!(color_of(&merged.vertices[2]), DEFAULT_COLOR);
    }

    #[test]
    fn unusable_geometry_is_skipped_and_leaves_no_orphan_vertices() {
        // A vertex no index reaches is memory the GPU still has to hold.
        let empty_positions: Vec<[f32; 3]> = Vec::new();
        let empty_triangles: Vec<[u16; 3]> = Vec::new();
        let vertices_only_positions = positions(&[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]);
        let no_triangles: Vec<[u16; 3]> = Vec::new();
        let out_of_range_positions = positions(&[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]);
        let out_of_range_triangles = vec![[0, 1, 2]];
        let (good_positions, good_triangles) = quad(0.0);

        let merged = merge_geometry(&[
            GeometryInput {
                positions: &empty_positions,
                triangles: &empty_triangles,
                ..Default::default()
            },
            GeometryInput {
                positions: &vertices_only_positions,
                triangles: &no_triangles,
                ..Default::default()
            },
            GeometryInput {
                positions: &out_of_range_positions,
                triangles: &out_of_range_triangles,
                ..Default::default()
            },
            GeometryInput {
                positions: &good_positions,
                triangles: &good_triangles,
                ..Default::default()
            },
        ]);

        assert_eq!(merged.skipped_geometry_count, 3);
        assert_eq!(merged.geometry_count, 1);
        assert_eq!(merged.vertices.len(), 4);
        // A triangle past its own vertex array is dropped.
        assert_eq!(merged.indices.len(), 6);
    }

    #[test]
    fn a_usable_triangle_survives_next_to_an_unusable_one() {
        let mixed_positions = positions(&[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]]);
        let mixed_triangles = vec![[0, 1, 2], [0, 1, 9]];

        let merged = merge_geometry(&[GeometryInput {
            positions: &mixed_positions,
            triangles: &mixed_triangles,
            ..Default::default()
        }]);

        assert_eq!(merged.indices.len(), 3);
        assert_eq!(merged.geometry_count, 1);
        assert_eq!(&merged.indices[0..3], &[0, 1, 2]);
    }

    #[test]
    fn an_empty_scene_is_reported_as_empty_rather_than_as_one_blank_shape() {
        let merged = merge_geometry(&[]);

        assert!(merged.vertices.is_empty());
        assert!(merged.indices.is_empty());
        assert_eq!(merged.geometry_count, 0);
        assert_eq!(merged.skipped_geometry_count, 0);
    }

    #[test]
    fn a_later_geometry_reads_its_own_attributes() {
        // The per-vertex fallback must not leak the previous shape's attributes
        // into a shape that supplies its own.
        let (first_positions, first_triangles) = quad(0.0);
        let first_normals = positions(&[[0.0, 0.0, -1.0]; 4]);
        let second_positions = positions(&[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0], [0.0, 1.0, 0.0]]);
        let second_triangles = vec![[0, 1, 2]];

        let merged = merge_geometry(&[
            GeometryInput {
                positions: &first_positions,
                normals: &first_normals,
                triangles: &first_triangles,
                ..Default::default()
            },
            GeometryInput {
                positions: &second_positions,
                triangles: &second_triangles,
                ..Default::default()
            },
        ]);

        assert_eq!(merged.vertices.len(), 7);
        assert_eq!(normal_of(&merged.vertices[0]), [0.0, 0.0, -1.0]);
        assert_eq!(normal_of(&merged.vertices[4]), DEFAULT_NORMAL);
    }

    #[test]
    fn a_scene_of_quads_never_duplicates_a_vertex_slot() {
        // Every geometry pushes all of its vertices, referenced or not, so the
        // merged vertex count is the plain sum of the blocks that merge.
        let (a_positions, a_triangles) = quad(0.0);
        let (b_positions, b_triangles) = quad(10.0);
        let (c_positions, c_triangles) = quad(20.0);

        let merged = merge_geometry(&[
            GeometryInput {
                positions: &a_positions,
                triangles: &a_triangles,
                ..Default::default()
            },
            GeometryInput {
                positions: &b_positions,
                triangles: &b_triangles,
                ..Default::default()
            },
            GeometryInput {
                positions: &c_positions,
                triangles: &c_triangles,
                ..Default::default()
            },
        ]);

        assert_eq!(merged.vertices.len(), 12);
        assert_eq!(merged.indices.len(), 18);
        assert_eq!(position_of(&merged.vertices[4]), [10.0, 0.0, 0.0]);
        assert_eq!(position_of(&merged.vertices[8]), [20.0, 0.0, 0.0]);
    }

    #[test]
    fn a_degenerate_triangle_is_kept_as_written() {
        // The C++ original only range-checks indices; it does not drop a triangle
        // whose vertices coincide, so neither does this port.
        let positions = positions(&[[0.0, 0.0, 0.0], [1.0, 0.0, 0.0]]);
        let triangles = vec![[0, 0, 1]];

        let merged = merge_geometry(&[GeometryInput {
            positions: &positions,
            triangles: &triangles,
            ..Default::default()
        }]);

        assert_eq!(&merged.indices[0..3], &[0, 0, 1]);
    }
}
