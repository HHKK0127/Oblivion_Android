//! Oblivion plugin load-order handling.
//!
//! A plugin file stores FormIDs in its own index space: the high byte of a
//! FormID indexes that file's MAST list, and the plugin's own records sit at
//! the index just past its masters. The engine instead numbers every loaded
//! file from 0 in load order, so a FormID read from disk has to be translated
//! before it can be looked up. This crate owns that translation and the
//! plugins.txt grammar that produces the file list it translates against.
//!
//! Nothing here touches the filesystem, the Android logging layer or the
//! renderer, so the whole crate is testable off-device.

mod formid_index;
mod plugin_list;

pub use formid_index::{
    build_form_id_index_map, plugin_name_equals, rebase_form_id, BASE_PLUGIN_FILE_NAME,
    UNRESOLVED_FORMID_INDEX,
};
pub use plugin_list::{
    active_plugin_indices, entry_name, is_master_file_name, parse_plugin_list,
    plugin_load_order_rank, serialize_plugin_list, sort_plugin_load_order, NamedPluginEntry,
    PluginListEntry,
};
