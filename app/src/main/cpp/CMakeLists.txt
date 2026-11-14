cmake_minimum_required(VERSION 3.18.1)
project(oblivion_native)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# Jolt Physics integration
# ============================================================================
set(JOLT_PHYSICS_DIR "${CMAKE_SOURCE_DIR}/third_party/JoltPhysics")

# Add Jolt as subdirectory (recommended)
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(JPH_USE_VK OFF CACHE BOOL "" FORCE)  # Disable Vulkan (not needed for Android OpenGL ES)

add_subdirectory(${JOLT_PHYSICS_DIR}/Build ${CMAKE_BINARY_DIR}/JoltPhysics)

# Compile flags for Jolt
# JPH_DISABLE_CUSTOM_ALLOCATOR is not used (Jolt library provides its own allocator)
target_compile_definitions(Jolt PRIVATE
    JPH_ENABLE_ASSERTS
)

# ============================================================================
# Rust components (vendored Rust workspace: repo-local rust/, since 2026-11-09)
#
# The FormID load-order translation in assets/formid_index.cpp, the plugin list
# handling in assets/plugin_list.cpp, the Data-folder path expansion in
# assets/asset_path_resolver.cpp, the multi-shape merge in
# assets/mesh_builder.cpp, the BSA archive reader in assets/bsa_reader.cpp,
# the NIF mesh parser in assets/nif_parser.cpp
# and the DDS texture loader in assets/dds_loader.cpp
# are shims over six C ABIs (plugin_ffi, assets_ffi, bsa_ffi, nif_ffi, dds_ffi
# and esm_ffi).
# The implementations live in the Rust workspace vendored under rust/ at the
# repository root, built here through cargo-ndk. Point OBLIVION_RUST_DIR at a
# different checkout if needed, and set OBLIVION_RUST_OFFLINE to keep cargo from
# reaching the network.
# ============================================================================
set(OBLIVION_RUST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../rust"
    CACHE PATH "Path to the Rust workspace (vendored rust/ by default)")
set(OBLIVION_RUST_OFFLINE OFF CACHE BOOL "Run cargo with --offline")

if(NOT EXISTS "${OBLIVION_RUST_DIR}/Cargo.toml")
    message(FATAL_ERROR
        "No Cargo.toml in OBLIVION_RUST_DIR (${OBLIVION_RUST_DIR}). "
        "The workspace should live at the repo-root rust/ directory; "
        "pass -DOBLIVION_RUST_DIR=<path to a Rust workspace checkout> to override.")
endif()

if(ANDROID_ABI STREQUAL "arm64-v8a")
    set(OBLIVION_RUST_TRIPLE aarch64-linux-android)
elseif(ANDROID_ABI STREQUAL "armeabi-v7a")
    set(OBLIVION_RUST_TRIPLE armv7-linux-androideabi)
elseif(ANDROID_ABI STREQUAL "x86_64")
    set(OBLIVION_RUST_TRIPLE x86_64-linux-android)
elseif(ANDROID_ABI STREQUAL "x86")
    set(OBLIVION_RUST_TRIPLE i686-linux-android)
else()
    message(FATAL_ERROR "No Rust target known for ANDROID_ABI=${ANDROID_ABI}")
endif()

set(OBLIVION_RUST_API_LEVEL 25)
if(ANDROID_PLATFORM MATCHES "^android-([0-9]+)$")
    set(OBLIVION_RUST_API_LEVEL ${CMAKE_MATCH_1})
elseif(DEFINED CMAKE_SYSTEM_VERSION)
    set(OBLIVION_RUST_API_LEVEL ${CMAKE_SYSTEM_VERSION})
endif()

if(DEFINED CMAKE_ANDROID_NDK)
    set(OBLIVION_RUST_NDK "${CMAKE_ANDROID_NDK}")
else()
    set(OBLIVION_RUST_NDK "${ANDROID_NDK}")
endif()

# A shared target directory would be mixed with host builds, whose artifacts are
# not valid for Android. Keep the CMake-driven builds separate.
set(OBLIVION_RUST_TARGET_DIR "${OBLIVION_RUST_DIR}/target/cmake")
set(OBLIVION_RUST_ARCHIVE
    "${OBLIVION_RUST_TARGET_DIR}/${OBLIVION_RUST_TRIPLE}/release/libplugin_ffi.a")
set(OBLIVION_RUST_ASSETS_ARCHIVE
    "${OBLIVION_RUST_TARGET_DIR}/${OBLIVION_RUST_TRIPLE}/release/libassets_ffi.a")
set(OBLIVION_RUST_BSA_ARCHIVE
    "${OBLIVION_RUST_TARGET_DIR}/${OBLIVION_RUST_TRIPLE}/release/libbsa_ffi.a")
set(OBLIVION_RUST_NIF_ARCHIVE
    "${OBLIVION_RUST_TARGET_DIR}/${OBLIVION_RUST_TRIPLE}/release/libnif_ffi.a")
set(OBLIVION_RUST_DDS_ARCHIVE
    "${OBLIVION_RUST_TARGET_DIR}/${OBLIVION_RUST_TRIPLE}/release/libdds_ffi.a")
set(OBLIVION_RUST_ESM_ARCHIVE
    "${OBLIVION_RUST_TARGET_DIR}/${OBLIVION_RUST_TRIPLE}/release/libesm_ffi.a")

# Prefer the rustup toolchain over whatever cargo is first in PATH: a standalone
# Rust install typically carries host targets only.
find_program(OBLIVION_CARGO NAMES cargo cargo.exe
    HINTS "$ENV{USERPROFILE}/.cargo/bin" "$ENV{CARGO_HOME}/bin" "$ENV{HOME}/.cargo/bin")
if(NOT OBLIVION_CARGO)
    message(FATAL_ERROR "cargo was not found. Pass -DOBLIVION_CARGO=<path to cargo>.")
endif()

# cargo-ndk is required: it supplies the per-ABI linker wrapper that rustc needs
# on Windows, where the NDK ships clang as a .cmd file. It refuses to be started
# directly, so it is reached through "cargo ndk" with its directory on PATH.
find_program(OBLIVION_CARGO_NDK NAMES cargo-ndk cargo-ndk.exe
    HINTS "$ENV{USERPROFILE}/.cargo/bin" "$ENV{CARGO_HOME}/bin" "$ENV{HOME}/.cargo/bin")
if(NOT OBLIVION_CARGO_NDK)
    message(FATAL_ERROR
        "cargo-ndk was not found. Install it with \"cargo install cargo-ndk\", or pass "
        "-DOBLIVION_CARGO_NDK=<path>.")
endif()

if(CMAKE_HOST_WIN32)
    set(_oblivion_path_separator ";")
else()
    set(_oblivion_path_separator ":")
endif()
get_filename_component(_oblivion_cargo_ndk_dir "${OBLIVION_CARGO_NDK}" DIRECTORY)
set(_oblivion_rust_path "${_oblivion_cargo_ndk_dir}${_oblivion_path_separator}$ENV{PATH}")
# An unescaped ";" inside a COMMAND argument is split back into several arguments
# by CMake, which would hand cmake -E env a truncated PATH.
string(REPLACE ";" "$<SEMICOLON>" _oblivion_rust_path "${_oblivion_rust_path}")

# cargo looks up "rustc" on PATH when RUSTC is unset, which on this machine finds a
# standalone host-only toolchain before the rustup proxy. Pin both to the proxy
# next to cargo so the selected toolchain is the one that owns the Android targets.
set(_oblivion_rust_env
    "ANDROID_NDK_HOME=${OBLIVION_RUST_NDK}"
    "ANDROID_NDK_ROOT=${OBLIVION_RUST_NDK}"
    "CARGO=${OBLIVION_CARGO}"
    "CARGO_TARGET_DIR=${OBLIVION_RUST_TARGET_DIR}"
    "PATH=${_oblivion_rust_path}")
get_filename_component(_oblivion_cargo_dir "${OBLIVION_CARGO}" DIRECTORY)
foreach(_oblivion_proxy IN ITEMS rustc rustdoc)
    foreach(_oblivion_suffix IN ITEMS "" ".exe")
        set(_oblivion_proxy_path "${_oblivion_cargo_dir}/${_oblivion_proxy}${_oblivion_suffix}")
        if(EXISTS "${_oblivion_proxy_path}")
            string(TOUPPER "${_oblivion_proxy}" _oblivion_proxy_var)
            list(APPEND _oblivion_rust_env "${_oblivion_proxy_var}=${_oblivion_proxy_path}")
            break()
        endif()
    endforeach()
endforeach()

set(_oblivion_rust_cargo_args build --release -p plugin-ffi -p assets-ffi -p bsa-ffi -p nif-ffi -p dds-ffi -p esm-ffi)
if(OBLIVION_RUST_OFFLINE)
    list(APPEND _oblivion_rust_cargo_args --offline)
endif()

file(GLOB_RECURSE OBLIVION_RUST_SOURCES CONFIGURE_DEPENDS
    "${OBLIVION_RUST_DIR}/Cargo.toml"
    "${OBLIVION_RUST_DIR}/Cargo.lock"
    "${OBLIVION_RUST_DIR}/crates/plugin-parser/Cargo.toml"
    "${OBLIVION_RUST_DIR}/crates/plugin-parser/src/*.rs"
    "${OBLIVION_RUST_DIR}/crates/plugin-ffi/Cargo.toml"
    "${OBLIVION_RUST_DIR}/crates/plugin-ffi/src/*.rs"
    "${OBLIVION_RUST_DIR}/crates/assets-core/Cargo.toml"
    "${OBLIVION_RUST_DIR}/crates/assets-core/src/*.rs"
    "${OBLIVION_RUST_DIR}/crates/assets-ffi/Cargo.toml"
    "${OBLIVION_RUST_DIR}/crates/assets-ffi/src/*.rs"
    "${OBLIVION_RUST_DIR}/crates/bsa-core/Cargo.toml"
    "${OBLIVION_RUST_DIR}/crates/bsa-core/src/*.rs"
    "${OBLIVION_RUST_DIR}/crates/bsa-ffi/Cargo.toml"
    "${OBLIVION_RUST_DIR}/crates/bsa-ffi/src/*.rs"
    "${OBLIVION_RUST_DIR}/crates/nif-core/Cargo.toml"
    "${OBLIVION_RUST_DIR}/crates/nif-core/src/*.rs"
    "${OBLIVION_RUST_DIR}/crates/nif-ffi/Cargo.toml"
    "${OBLIVION_RUST_DIR}/crates/nif-ffi/src/*.rs"
    "${OBLIVION_RUST_DIR}/crates/dds-core/Cargo.toml"
    "${OBLIVION_RUST_DIR}/crates/dds-core/src/*.rs"
    "${OBLIVION_RUST_DIR}/crates/dds-ffi/Cargo.toml"
    "${OBLIVION_RUST_DIR}/crates/dds-ffi/src/*.rs"
    "${OBLIVION_RUST_DIR}/crates/esm-parser/Cargo.toml"
    "${OBLIVION_RUST_DIR}/crates/esm-parser/src/*.rs"
    "${OBLIVION_RUST_DIR}/crates/esm-ffi/Cargo.toml"
    "${OBLIVION_RUST_DIR}/crates/esm-ffi/src/*.rs")

# One cargo invocation produces all six static libraries, so the archives are
# outputs of a single command: building them separately would run the workspace
# build several times and let the invocations race on the same target directory.
add_custom_command(
    OUTPUT "${OBLIVION_RUST_ARCHIVE}" "${OBLIVION_RUST_ASSETS_ARCHIVE}"
           "${OBLIVION_RUST_BSA_ARCHIVE}" "${OBLIVION_RUST_NIF_ARCHIVE}"
           "${OBLIVION_RUST_DDS_ARCHIVE}" "${OBLIVION_RUST_ESM_ARCHIVE}"
    COMMAND ${CMAKE_COMMAND} -E env
            ${_oblivion_rust_env}
            "${OBLIVION_CARGO}" ndk -t ${ANDROID_ABI} --platform ${OBLIVION_RUST_API_LEVEL}
            ${_oblivion_rust_cargo_args}
    WORKING_DIRECTORY "${OBLIVION_RUST_DIR}"
    DEPENDS ${OBLIVION_RUST_SOURCES}
    COMMENT "Building plugin-ffi, assets-ffi, bsa-ffi, nif-ffi, dds-ffi and esm-ffi for ${ANDROID_ABI} (Rust)"
    VERBATIM)

add_custom_target(oblivion_rust_ffi
    DEPENDS "${OBLIVION_RUST_ARCHIVE}" "${OBLIVION_RUST_ASSETS_ARCHIVE}"
            "${OBLIVION_RUST_BSA_ARCHIVE}" "${OBLIVION_RUST_NIF_ARCHIVE}"
            "${OBLIVION_RUST_DDS_ARCHIVE}" "${OBLIVION_RUST_ESM_ARCHIVE}")

add_library(plugin_ffi STATIC IMPORTED GLOBAL)
set_target_properties(plugin_ffi PROPERTIES
    IMPORTED_LOCATION "${OBLIVION_RUST_ARCHIVE}")
add_dependencies(plugin_ffi oblivion_rust_ffi)

add_library(assets_ffi STATIC IMPORTED GLOBAL)
set_target_properties(assets_ffi PROPERTIES
    IMPORTED_LOCATION "${OBLIVION_RUST_ASSETS_ARCHIVE}")
add_dependencies(assets_ffi oblivion_rust_ffi)

add_library(bsa_ffi STATIC IMPORTED GLOBAL)
set_target_properties(bsa_ffi PROPERTIES
    IMPORTED_LOCATION "${OBLIVION_RUST_BSA_ARCHIVE}")
add_dependencies(bsa_ffi oblivion_rust_ffi)

add_library(nif_ffi STATIC IMPORTED GLOBAL)
set_target_properties(nif_ffi PROPERTIES
    IMPORTED_LOCATION "${OBLIVION_RUST_NIF_ARCHIVE}")
add_dependencies(nif_ffi oblivion_rust_ffi)

add_library(dds_ffi STATIC IMPORTED GLOBAL)
set_target_properties(dds_ffi PROPERTIES
    IMPORTED_LOCATION "${OBLIVION_RUST_DDS_ARCHIVE}")
add_dependencies(dds_ffi oblivion_rust_ffi)

add_library(esm_ffi STATIC IMPORTED GLOBAL)
set_target_properties(esm_ffi PROPERTIES
    IMPORTED_LOCATION "${OBLIVION_RUST_ESM_ARCHIVE}")
add_dependencies(esm_ffi oblivion_rust_ffi)

message(STATUS "Rust: plugin-ffi, assets-ffi, bsa-ffi, nif-ffi, dds-ffi and esm-ffi from ${OBLIVION_RUST_DIR} (${ANDROID_ABI}, API ${OBLIVION_RUST_API_LEVEL})")

# ============================================================================

# Add main library
add_library(native-lib SHARED
    # Engine Core
    engine/renderer.cpp
    engine/crash_handler.cpp
    engine/shader.cpp
    engine/camera.cpp
    engine/texture_loader.cpp
    engine/graphics/retro_filter.cpp
    engine/imperial_weave.cpp
    engine/event_bus.cpp

    # Phase 44: Performance Optimization
    engine/memory_pool.cpp
    engine/render_optimizer.cpp
    engine/async_task_manager.cpp
    engine/cache_manager.cpp
    engine/profiler_dashboard.cpp
    # Phase 42: Game Loop Integration
    engine/state_manager.cpp
    engine/input_router.cpp
    engine/game_loop_coordinator.cpp
    engine/scene_renderer.cpp
    engine/performance_profiler.cpp
    ui/spell_selection_panel.cpp
    audio/audio_subscriber.cpp
    animation/animation_subscriber.cpp

    # Geometry
    geometry/cube.cpp
    geometry/mesh.cpp
    geometry/skin_partition_packer.cpp
    geometry/material.cpp
    animation/skeleton.cpp
    animation/animation_player.cpp
    collision/aabb_tree.cpp
    collision/collision_world.cpp
    collision/character_controller.cpp

    # World System (Phase 3+)
    world/world_manager.cpp
    world/world_item_manager.cpp
    world/game_state_report.cpp
    world/cell.cpp
    world/cell_loader.cpp
    world/cell_transition_manager.cpp
    world/door.cpp
    world/grid.cpp
    world/terrain.cpp

    # Phase 31: World Entity & Loader
    world/world_loader.cpp

    # Phase 38: Full World Load (ESM -> 3D Space)
    # The Phase 38 cluster that used to sit here - landscape_renderer, texture_manager,
    # lod_system and world_builder - was removed on 2026-09-27. It was reachable only
    # from WorldBuilder itself, and WorldBuilder had no caller anywhere in the tree, so
    # none of it ever ran. ObjectPlacer is the one survivor: renderer.cpp uses it for
    # the static-object and interior-REFR passes.
    world/object_placer.cpp

    # Phase 50: Distant LOD System
    world/distant_lod/distant_lod_manager.cpp
    world/distant_lod/distant_lod_horizon.cpp

    # Phase 65: original landscape LOD blocks (meshes\landscape\lod)
    world/distant_lod/original_lod_provider.cpp

    # Original object LOD (distantlod\*.lod instance records)
    world/distant_lod/original_object_lod_provider.cpp

    # Game Systems
    game/npc.cpp
    game/npc_manager.cpp
    game/combat_manager.cpp
    game/quest_manager.cpp
    game/condition_system.cpp
    game/spell_manager.cpp
    game/navmesh_manager.cpp
    game/ai_package.cpp
    game/ai_scheduler.cpp
    game/loot_generator.cpp
    game/book_reader.cpp
    game/clothing_converter.cpp
    game/alchemy_system.cpp
    game/misc_item_converter.cpp
    game/faction_manager.cpp
    game/dialogue.cpp
    game/door.cpp
    game/interactable.cpp
    game/container.cpp
    game/interaction_manager.cpp
    game/merchant.cpp
    game/merchant_factory.cpp
    game/barter.cpp
    game/item_catalog.cpp

    # Player & Movement (Phase 3+)
    game/player.cpp
    game/player_controller.cpp
    input/touch_input_handler.cpp

    # Inventory System (Phase 3+)
    game/inventory.cpp
    game/inventory_manager.cpp
    game/consumable_system.cpp
    game/equipment_effect_system.cpp
    game/crime_reputation_system.cpp
    game/lockpick_system.cpp
    game/enchanting_system.cpp
    game/book_database.cpp

    # UI Systems
    ui/title_screen.cpp
    ui/launcher_screen.cpp
        ui/loading_screen.cpp
    ui/quest_ui.cpp
    ui/inventory_ui.cpp
    ui/text_renderer.cpp
    ui/debug_hud.cpp
    ui/game_console.cpp
    ui/debug_menu.cpp
    ui/texture_viewer.cpp
    ui/model_viewer.cpp
    ui/world_viewer.cpp

    # Phase 64: Debug Tools
    debug/picker.cpp
    debug/teleport.cpp
    debug/ai_view.cpp
    debug/anim_scrub.cpp
    debug/audio_browse.cpp
    debug/phys_tool.cpp
    debug/slot_view.cpp
    debug/nav_view.cpp
    debug/time_ctl.cpp
    debug/aggro_hud.cpp
    debug/player_stats.cpp
    ui/mesh_3d.cpp
    ui/viewer_3d.cpp
    ui/npc_debug_visualizer.cpp
    ui/world_debug_info.cpp
    ui/performance_graph.cpp
    ui/debug_draw.cpp
    ui/settings_ui.cpp
    ui/save_load_ui.cpp
    ui/options_ui.cpp
    ui/audio_options_ui.cpp
    ui/gameplay_options_ui.cpp
    ui/video_options_ui.cpp
    ui/video_display_ui.cpp
    ui/controls_options_ui.cpp
    ui/ui_component.cpp
    ui/ui_system.cpp
    ui/ui_menu_stack.cpp
    ui/ui_panel.cpp
    ui/ui_button.cpp
    ui/ui_draw_helper.cpp
    ui/ui_map_panel.cpp
    ui/ui_inventory_panel.cpp
    ui/ui_container_menu.cpp
    ui/item_icons.cpp
    ui/ui_sleep_wait_menu.cpp
    ui/ui_character_sheet.cpp
    ui/ui_spellbook.cpp
    ui/ui_quest_log.cpp
    ui/ui_dialogue.cpp
    ui/ui_book_reader.cpp
    ui/ui_shop.cpp
    ui/ui_character_creation.cpp
    ui/ui_pause_menu.cpp
    ui/ui_level_up_menu.cpp
    ui/ui_message_box.cpp
    ui/fast_travel_controller.cpp
    ui/text_wrap.cpp
    ui/ui_toast.cpp
    ui/ui_quick_slot_bar.cpp
    ui/ui_quick_keys_menu.cpp
    ui/ui_hud_status_display.cpp
    ui/ui_hud_compass.cpp
    ui/ui_floating_text.cpp
    ui/subtitle_system.cpp
    ui/ui_target_info.cpp
    ui/ui_action_prompt.cpp
    ui/ui_active_effects.cpp
    ui/ui_level_progress.cpp
    ui/ui_alert_notification.cpp
    ui/ui_minimap.cpp
    ui/ui_player_stats.cpp
    ui/ui_joystick.cpp
    ui/virtual_controller.cpp
    ui/crosshair.cpp
    ui/placeholder_assets.cpp
    ui/hud_renderer.cpp
    ui/xml_parser.cpp
    ui/menu_xml_interpreter.cpp
        ui/menu_ui_builder.cpp
        ui/menu_text_component.cpp
        ui/menu_ui_instantiator.cpp

    # Phase 43: UI/UX Final Adjustments
    ui/touch_gesture_handler.cpp
    ui/menu_transition_manager.cpp

    # RE 48: menu open/close fade (MenuManager fade node list)
    ui/menu_fade_system.cpp
    # RE 48.2: full-screen fader slot array (fader array @0x00B12DC0)
    ui/fader_slot_array.cpp
    ui/hud_layout.cpp
    ui/control_scheme_manager.cpp
    ui/accessibility_manager.cpp

    # Phase 49: Control System Optimization
    input/touch_calibration.cpp
    input/gamepad_mapper.cpp
    input/input_visualizer.cpp
    ui/hud_customizer.cpp

    # Map System (Phase 9.1)
    map/map_system.cpp

    # Inventory System (Phase 9.2)
    inventory/inventory_grid.cpp
    inventory/equipment_manager.cpp
    inventory/item_factory.cpp
    inventory/inventory_coordinator.cpp

    # System
    system/settings_manager.cpp
    system/debug_system.cpp
    system/display_modes.cpp

    # INI reader and authentic Oblivion.ini defaults (Phase 66)
    system/oblivion_ini.cpp
    system/oblivion_ini_defaults.cpp

    # Vanilla BSBlendSettings default table (62 entries, v64 §2)
    system/blend_settings.cpp

    # Localization
    localization/localization_manager.cpp

    # Profiling
    profiling/performance_monitor.cpp

    # Save System
    save_system/save_manager.cpp
    save_system/save_slot_manager.cpp

    # Audio System (Phase 8+)
    audio/audio_manager.cpp
    audio/audio_3d.cpp
    audio/jni_audio_bridge.cpp

    # Phase 47: Audio System Integration
    audio/audio_decoder.cpp
    audio/bgm_manager.cpp
    audio/sound_effect_manager.cpp

    # Asset System
    assets/nif_parser.cpp
    assets/nif_block_type_map.cpp
    assets/asset_path_resolver.cpp
    assets/mesh_builder.cpp
    assets/dds_loader.cpp
    assets/asset_manager.cpp
    assets/bsa_reader.cpp
    assets/esm_reader.cpp
    assets/plugin_list.cpp
    assets/formid_index.cpp

    # Phase 46: Asset Pipeline
    assets/mesh_loader.cpp
    assets/world_data_loader.cpp

    # Tests (Phase 30 Step 13)
    tests/phase30_integration_test.cpp

    # Tests (Phase 38: Script VM)
    tests/script_vm_tests.cpp

    # Tests (Phase 45: Unit Tests)
    tests/phase45_unit_tests.cpp

    # Tests (Phase 48: Game Loop Integration & Stress)
    tests/phase48_integration_test.cpp
    tests/phase48_stress_test.cpp

    # Tests (Phase 66 P8: WATR colour fallback)
    tests/watr_decode_tests.cpp
        tests/xtel_decode_tests.cpp
        tests/weather_transition_tests.cpp
        tests/interior_cell_tests.cpp

        # Tests (Phase 65: LIP decode)
        tests/lip_sync_decode_tests.cpp

    # Tests (Phase 45: Container Menu)
    tests/container_menu_tests.cpp

    # Tests (Phase 45: SleepWait Menu)
    tests/sleep_wait_menu_tests.cpp

    # Tests (Menu Stack)
    tests/menu_stack_tests.cpp

    # Tests (QuickKeys Menu)
    tests/quick_keys_menu_tests.cpp

    # Tests (Level Up Menu)
    tests/level_up_menu_tests.cpp

    # Tests (Phase 69: container persistence)
    tests/phase69_persistence_tests.cpp

    # Tests (Phase 70: plugins.txt grammar and load order)
    tests/plugin_list_tests.cpp

    # Tests (Phase 70: FormID index remapping)
    tests/formid_index_tests.cpp

    # Tests (Phase 65: original landscape LOD block invariants)
    tests/lod_block_tests.cpp

    # Tests (object LOD .lod parser invariants)
    tests/object_lod_tests.cpp

    # Tests (Phase 65: MGEF effect-code table invariants)
    tests/enchanting_system_tests.cpp

    # Physics System (Phase 36 - Jolt Physics)
    physics/physics_manager.cpp

    # Script VM (Phase 37 - Oblivion Script System)
    script/script_vm.cpp
    script/script_context.cpp
    script/script_manager.cpp
    script/script_functions.cpp
    script/script_disasm.cpp
    script/native_scda_decoder.cpp
    script/native_scda_vm.cpp
    script/native_scda_bridge.cpp

    # Quest System (Phase 39 - Quest Flow)
    quest/quest_record.cpp
    quest/quest_stage_manager.cpp
    quest/quest_objective_tracker.cpp
    quest/quest_rewards.cpp
    quest/quest_flow_controller.cpp

    # Dialogue System (Phase 40 - NPC Dialogue Tree)
    dialogue/dialogue_record.cpp
    dialogue/dialogue_tree.cpp
    dialogue/dialogue_filter_engine.cpp
    dialogue/dialogue_runner.cpp
    dialogue/dialogue_history.cpp
    dialogue/dialogue_integration.cpp
        dialogue/voice_sound_db.cpp
    dialogue/topic_resolver.cpp
    game/player_character_state.cpp

    # Phase 52: FaceGen System
    character/face_gen_morpher.cpp
    character/face_gen_cache.cpp

        # Phase 65: Lip sync decoder
        character/lip_sync_decoder.cpp

    # Phase 53: Bink Video replacement (MediaCodec-based video playback)
    video/bink_video_player.cpp
    video/video_decoder_jni.cpp
    video/video_clip_manager.cpp
    video/video_renderer.cpp

    # Utilities (j6: MSVC rand() LCG replacement)
    util/msvc_lcg.cpp

    # JNI Bridge (Java/C++ communication)
    jni_bridge.cpp
)

# Include directories
target_include_directories(native-lib PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/world
    ${CMAKE_CURRENT_SOURCE_DIR}/player
    ${CMAKE_CURRENT_SOURCE_DIR}/input
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/geometry
    ${CMAKE_CURRENT_SOURCE_DIR}/math
    ${CMAKE_CURRENT_SOURCE_DIR}/localization
    ${CMAKE_CURRENT_SOURCE_DIR}/profiling
    ${CMAKE_CURRENT_SOURCE_DIR}/save_system
    ${CMAKE_CURRENT_SOURCE_DIR}/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/map
    ${CMAKE_CURRENT_SOURCE_DIR}/inventory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/collision
    ${CMAKE_CURRENT_SOURCE_DIR}/animation
    ${CMAKE_CURRENT_SOURCE_DIR}/debug
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/physics
    ${CMAKE_CURRENT_SOURCE_DIR}/script
    ${CMAKE_CURRENT_SOURCE_DIR}/quest
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogue
    ${CMAKE_CURRENT_SOURCE_DIR}/vegetation
    ${CMAKE_CURRENT_SOURCE_DIR}/character
    ${CMAKE_CURRENT_SOURCE_DIR}/video
    ${CMAKE_CURRENT_SOURCE_DIR}/system
    ${OBLIVION_RUST_DIR}/crates/plugin-ffi/include
    ${OBLIVION_RUST_DIR}/crates/assets-ffi/include
    ${OBLIVION_RUST_DIR}/crates/bsa-ffi/include
    ${OBLIVION_RUST_DIR}/crates/nif-ffi/include
    ${OBLIVION_RUST_DIR}/crates/dds-ffi/include
    ${OBLIVION_RUST_DIR}/crates/esm-ffi/include
)

# Find OpenAL-Soft (optional for now)
find_package(OpenAL CONFIG QUIET)
if(NOT OpenAL_FOUND)
    # Try to find OpenAL in standard paths
    find_package(OpenAL QUIET)
endif()

# Audio system: Using Android MediaPlayer via JNI bridge (no OpenAL dependency)
# Message output for build info
if(OpenAL_FOUND)
    message(STATUS "OpenAL found but not used - JNI bridge approach")
else()
    message(STATUS "Using Android MediaPlayer via JNI for audio playback")
endif()

# Link Android native libraries
target_link_libraries(native-lib
    Jolt
    plugin_ffi  # Rust: FormID index translation and plugin list (rust/ workspace)
    assets_ffi  # Rust: Data-folder path expansion and NIF geometry merge
    bsa_ffi     # Rust: BSA archive parsing, hashing lookup and ZLib inflation
    nif_ffi     # Rust: NIF mesh parsing backing the NIFParser shim (rust/ workspace)
    dds_ffi     # Rust: DDS texture loading backing the DDSLoader shim (rust/ workspace)
    esm_ffi    # Rust: ESM/ESP plugin parsing backing the ESMFile shim (rust/ workspace)
    android
    EGL
    GLESv3
    log
)

# Link OpenAL if found
if(OpenAL_FOUND)
    target_link_libraries(native-lib OpenAL::OpenAL)
endif()

# Compiler flags
if (ANDROID_ABI STREQUAL arm64-v8a)
    target_compile_options(native-lib PRIVATE -O3 -march=armv8-a -Wno-macro-redefined)
elseif (ANDROID_ABI STREQUAL armeabi-v7a)
    target_compile_options(native-lib PRIVATE -O3 -march=armv7-a -mfloat-abi=softfp -Wno-macro-redefined)
elseif (ANDROID_ABI STREQUAL x86)
    target_compile_options(native-lib PRIVATE -O3 -march=i686 -Wno-macro-redefined)
elseif (ANDROID_ABI STREQUAL x86_64)
    target_compile_options(native-lib PRIVATE -O3 -march=x86-64 -Wno-macro-redefined)
endif ()

# Common flags
target_compile_options(native-lib PRIVATE
    -Wall
    -Wextra
    -Werror=format
)

# Enable audio system (OpenAL stub headers used)
target_compile_definitions(native-lib PRIVATE AUDIO_SYSTEM_ENABLED)

# Enable debug logging in NpcManager (LOGD gated behind this macro)
target_compile_definitions(native-lib PRIVATE ENABLE_DEBUG_LOGS)

# Product build (release buildType passes -DOBLIVION_PRODUCT_BUILD=ON): hides the debug HUD
option(OBLIVION_PRODUCT_BUILD "Build as a product release (no debug HUD)" OFF)
if (OBLIVION_PRODUCT_BUILD)
    target_compile_definitions(native-lib PRIVATE OBLIVION_PRODUCT_BUILD=1)
endif ()

# Warning suppression for specific headers
set_source_files_properties(
    game/npc_manager.cpp
    game/quest_manager.cpp
    PROPERTIES
    COMPILE_FLAGS "-Wno-unused-variable"
)

