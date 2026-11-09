//! Oblivion asset handling that needs neither the device nor the renderer.
//!
//! Two jobs live here, both of which used to sit in the C++ asset layer:
//!
//! * [`build_asset_path_candidates`] expands an ESM-relative game path into the
//!   Data-folder spellings that an archive or a loose file can actually match.
//! * [`merge_geometry`] flattens per-NIF geometry blocks into one interleaved
//!   vertex buffer plus a `u32` index buffer, which is exactly what the
//!   renderer uploads.
//!
//! Both are pure functions over caller-owned memory. Paths are handled as raw
//! bytes rather than `str` because game paths come straight out of ESM records,
//! which are not guaranteed to be valid UTF-8, and because the C++ originals
//! compared bytes with `std::tolower` in the C locale (ASCII only). Path bytes
//! are therefore never inspected beyond `'/'`, `'.'` and `A`-`Z`.

mod asset_path;
mod mesh_builder;

pub use asset_path::{asset_root_folders, build_asset_path_candidates};
pub use mesh_builder::{
    merge_geometry, GeometryInput, MergedMesh, DEFAULT_COLOR, DEFAULT_NORMAL, DEFAULT_TEX_COORD,
    VERTEX_FLOATS,
};
