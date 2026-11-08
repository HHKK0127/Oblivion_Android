#!/usr/bin/env bash
# Build and run the native C++ host test suites for CI.
# Fails (exit 1) on any compilation error or suite failure.
set -euo pipefail

cd "$(dirname "$0")/../.."   # repo root

# On Windows the linked binary needs the compiler's own runtime DLLs
# (libstdc++-6, libgcc_s_seh-1, libwinpthread-1) on PATH at run time.
# Git Bash puts its bundled /mingw64/bin first, whose older copies shadow the
# ones matching the compiler we just built with (exec then fails with 127), so
# move the compiler's directory to the front of PATH.
if GXX_PATH="$(command -v g++ 2>/dev/null)"; then
  export PATH="$(dirname "$GXX_PATH"):$PATH"
fi

BIN="${TMPDIR:-/tmp}/host_tests/host_tests"
mkdir -p "$(dirname "$BIN")"

# The pure-logic asset/bsa/plugin/nif/dds layers are Rust behind a C ABI (see
# CMakeLists.txt: OBLIVION_RUST_DIR). The host runner needs their headers to
# compile and their host-triple archives to link. Since the 2026-10-26 Rust
# migration these were missing, so every local/CI run died on assets_ffi.h and
# then on the FFI symbols. Override the location with OBLIVION_RUST_DIR.
RUST_DIR="${OBLIVION_RUST_DIR:-$(cd .. && pwd)/oblivion-re-tools}"
RUST_HOST_TRIPLE="${OBLIVION_RUST_HOST_TRIPLE:-}"
if [ -z "$RUST_HOST_TRIPLE" ] && command -v rustc >/dev/null 2>&1; then
  RUST_HOST_TRIPLE="$(rustc -vV | sed -n 's/^host: //p')"
fi
# A non-login shell (this script under Git Bash) often lacks the shim directory
# that carries rustc, so probe the usual installs before giving up.
if [ -z "$RUST_HOST_TRIPLE" ]; then
  for _rustc in "$HOME/.cargo/bin/rustc" "/c/ProgramData/scoop/shims/rustc" "/usr/local/cargo/bin/rustc"; do
    if [ -x "$_rustc" ]; then
      RUST_HOST_TRIPLE="$("$_rustc" -vV | sed -n 's/^host: //p')"
      [ -n "$RUST_HOST_TRIPLE" ] && break
    fi
  done
fi
# The Android build links exactly these three archives (CMakeLists.txt), so a
# candidate directory only counts when it carries all of them.
have_rust_archives() {
  for _a in libassets_ffi.a libbsa_ffi.a libplugin_ffi.a; do
    [ -f "$1/$_a" ] || return 1
  done
  return 0
}
RUST_LIB_DIR=""
_cand_triples=("$RUST_HOST_TRIPLE")
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    # The host linker here is MinGW g++, which cannot consume MSVC-built
    # archives, while scoop rustc reports an msvc host triple; the gnu-built
    # archives live under target/gnu/x86_64-pc-windows-gnu/release.
    _cand_triples+=("x86_64-pc-windows-gnu")
    ;;
esac
for _t in "${_cand_triples[@]}"; do
  for _cand in \
      "$RUST_DIR/target/gnu/${_t}/release" \
      "$RUST_DIR/target/${_t}/release" \
      "$RUST_DIR/target/release"; do
    if have_rust_archives "$_cand"; then
      RUST_LIB_DIR="$_cand"
      break
    fi
  done
  [ -n "$RUST_LIB_DIR" ] && break
done
# Still nothing: search, skipping the cross builds (android/cmake) and the
# intermediate deps/ copies, and accept only a directory with the full set
# (target/gnu/release, for instance, has no esm/nif but does carry the three).
if [ -z "$RUST_LIB_DIR" ] && [ -d "$RUST_DIR/target" ]; then
  _found_all="$(find "$RUST_DIR/target" -name libassets_ffi.a -path '*/release/*' 2>/dev/null \
      | grep -vE '/(android|cmake|deps)/' | grep -vE '/(aarch64|armv7)-' | grep -v 'linux-android' || true)"
  for _found in $_found_all; do
    if have_rust_archives "$(dirname "$_found")"; then
      RUST_LIB_DIR="$(dirname "$_found")"
      break
    fi
  done
fi
if [ ! -f "$RUST_DIR/crates/assets-ffi/include/assets_ffi.h" ] || [ -z "$RUST_LIB_DIR" ]; then
  echo "error: Rust FFI not found under '$RUST_DIR' (needs crates/*/include headers and host-triple archives)." >&2
  echo "       Set OBLIVION_RUST_DIR, or OBLIVION_RUST_HOST_TRIPLE when rustc is not on PATH." >&2
  exit 1
fi

INCLUDES=(
  -Itools/host_tests/stubs          # android/log.h, asset_manager.h, jni.h
  -Iapp/src/main/cpp
  -Iapp/src/main/cpp/include
  -Iapp/src/main/cpp/assets
  -Iapp/src/main/cpp/third_party/JoltPhysics
  -I"$RUST_DIR/crates/assets-ffi/include"   # assets_ffi.h
  -I"$RUST_DIR/crates/bsa-ffi/include"      # bsa_ffi.h
  -I"$RUST_DIR/crates/plugin-ffi/include"   # plugin_ffi.h
  -I"$RUST_DIR/crates/esm-ffi/include"      # esm_ffi.h
  -I"$RUST_DIR/crates/nif-ffi/include"      # nif_ffi.h
  -I"$RUST_DIR/crates/dds-ffi/include"        # dds_ffi.h
)

# Flags mirroring the Android target (CMakeLists.txt):
#   AUDIO_SYSTEM_ENABLED  - audio_manager.h guards <AL/al.h>/<AL/alc.h> behind it
#   JPH_PROFILE_ENABLED   - physics_manager.h overrides Jolt's virtual
#   -include              - sources rely on transitive includes; be explicit
DEFINES=(
  -DAUDIO_SYSTEM_ENABLED
  -DJPH_PROFILE_ENABLED
  -include string
  -include cstring
  -include algorithm
  -include cstdint
  -include vector
  -include memory
  -include unordered_map
  -include cfloat
  # Ubuntu g++ does not hand these out transitively the way MinGW's libstdc++
  # does, so the sources that rely on a transitive include would break in CI.
  # Keep the workaround here, centrally, instead of editing each source.
  -include fstream
  -include sstream
  -include iostream
  -include istream
  -include ostream
  -include thread
  -include mutex
  -include atomic
  -include condition_variable
  -include future
  -include chrono
  -include functional
  -include array
  -include map
  -include set
  -include queue
  -include limits
  -include cmath
  -include cstdlib
  -include cstdio
  -include exception
  -include stdexcept
  -include utility
  -include type_traits
  -include iomanip
  -include new
)

SOURCES=(
  tools/host_tests/host_runner_main.cpp
  tools/host_tests/host_stub_syms.cpp
  tools/host_tests/host_gl_stubs.cpp
  tools/host_tests/host_physics_stubs.cpp
  app/src/main/cpp/tests/script_vm_tests.cpp
  app/src/main/cpp/tests/phase45_unit_tests.cpp
  app/src/main/cpp/tests/phase48_stress_test.cpp
  app/src/main/cpp/tests/phase48_integration_test.cpp
  app/src/main/cpp/tests/phase30_integration_test.cpp
  app/src/main/cpp/tests/watr_decode_tests.cpp
  app/src/main/cpp/tests/xtel_decode_tests.cpp
  app/src/main/cpp/tests/weather_transition_tests.cpp
  app/src/main/cpp/tests/interior_cell_tests.cpp
  app/src/main/cpp/tests/gamestate_report_tests.cpp
  app/src/main/cpp/tests/asset_path_tests.cpp
  app/src/main/cpp/tests/mesh_builder_tests.cpp
  app/src/main/cpp/tests/menu_xml_tests.cpp
  app/src/main/cpp/tests/menu_fade_tests.cpp
  app/src/main/cpp/tests/fader_slot_array_tests.cpp
  app/src/main/cpp/tests/input_router_tests.cpp
  app/src/main/cpp/tests/fatigue_system_tests.cpp
  app/src/main/cpp/tests/combat_formulas_tests.cpp
  app/src/main/cpp/util/msvc_lcg.cpp
  app/src/main/cpp/tests/msvc_lcg_tests.cpp
  app/src/main/cpp/tests/phase69_persistence_tests.cpp
  app/src/main/cpp/tests/plugin_list_tests.cpp
  app/src/main/cpp/tests/formid_index_tests.cpp
  app/src/main/cpp/tests/lod_block_tests.cpp
  app/src/main/cpp/tests/object_lod_tests.cpp
  app/src/main/cpp/tests/enchanting_system_tests.cpp
  app/src/main/cpp/tests/oblivion_ini_tests.cpp
  app/src/main/cpp/tests/settings_manager_tests.cpp
  app/src/main/cpp/tests/merchant_tests.cpp
  app/src/main/cpp/tests/merchant_persistence_tests.cpp
  app/src/main/cpp/tests/condition_tests.cpp
  app/src/main/cpp/game/condition_system.cpp
  app/src/main/cpp/tests/ai_package_tests.cpp
  app/src/main/cpp/game/ai_package.cpp
  app/src/main/cpp/tests/barter_price_tests.cpp
  app/src/main/cpp/tests/blend_settings_tests.cpp
  app/src/main/cpp/system/blend_settings.cpp
  app/src/main/cpp/tests/gmst_string_tests.cpp
  app/src/main/cpp/tests/script_fn_tests.cpp
  app/src/main/cpp/tests/topic_resolve_tests.cpp
  app/src/main/cpp/dialogue/topic_resolver.cpp
  app/src/main/cpp/tests/player_character_state_tests.cpp
  app/src/main/cpp/tests/dds_upload_tests.cpp
  app/src/main/cpp/game/player_character_state.cpp
  app/src/main/cpp/ui/xml_parser.cpp
  app/src/main/cpp/ui/menu_xml_interpreter.cpp
  app/src/main/cpp/ui/menu_ui_builder.cpp
  app/src/main/cpp/ui/menu_fade_system.cpp
  app/src/main/cpp/ui/fader_slot_array.cpp
  app/src/main/cpp/quest/quest_flow_controller.cpp
  app/src/main/cpp/localization/localization_manager.cpp
  app/src/main/cpp/quest/quest_stage_manager.cpp
  app/src/main/cpp/quest/quest_objective_tracker.cpp
  app/src/main/cpp/quest/quest_rewards.cpp
  app/src/main/cpp/quest/quest_record.cpp
  app/src/main/cpp/game/npc.cpp
  app/src/main/cpp/game/npc_manager.cpp
  app/src/main/cpp/game/player.cpp
  app/src/main/cpp/game/player_controller.cpp
  app/src/main/cpp/game/spell_manager.cpp
  app/src/main/cpp/game/navmesh_manager.cpp
  app/src/main/cpp/game/crime_reputation_system.cpp
  app/src/main/cpp/game/enchanting_system.cpp
  app/src/main/cpp/assets/esm_reader.cpp
  app/src/main/cpp/assets/asset_manager.cpp
  app/src/main/cpp/assets/asset_path_resolver.cpp
  app/src/main/cpp/assets/plugin_list.cpp
  app/src/main/cpp/assets/formid_index.cpp
  app/src/main/cpp/assets/bsa_reader.cpp
  app/src/main/cpp/assets/dds_loader.cpp
  app/src/main/cpp/geometry/mesh.cpp
  app/src/main/cpp/geometry/material.cpp
  app/src/main/cpp/engine/shader.cpp
  app/src/main/cpp/engine/memory_pool.cpp
  app/src/main/cpp/engine/async_task_manager.cpp
  app/src/main/cpp/engine/cache_manager.cpp
  app/src/main/cpp/engine/event_bus.cpp
  app/src/main/cpp/engine/state_manager.cpp
  app/src/main/cpp/engine/input_router.cpp
  app/src/main/cpp/save_system/save_manager.cpp
  app/src/main/cpp/save_system/save_slot_manager.cpp
  app/src/main/cpp/script/script_context.cpp
  app/src/main/cpp/script/script_disasm.cpp
  app/src/main/cpp/script/script_vm.cpp
  app/src/main/cpp/script/script_functions.cpp
  app/src/main/cpp/script/script_manager.cpp
  app/src/main/cpp/script/native_scda_decoder.cpp
  app/src/main/cpp/script/native_scda_vm.cpp
  app/src/main/cpp/script/native_scda_bridge.cpp
  app/src/main/cpp/game/quest_manager.cpp
  app/src/main/cpp/game/inventory_manager.cpp
  app/src/main/cpp/game/inventory.cpp
  app/src/main/cpp/audio/audio_manager.cpp
  app/src/main/cpp/audio/audio_3d.cpp
  app/src/main/cpp/animation/skeleton.cpp
  app/src/main/cpp/animation/animation_player.cpp
  app/src/main/cpp/assets/nif_parser.cpp
  app/src/main/cpp/assets/nif_block_type_map.cpp
  app/src/main/cpp/assets/mesh_builder.cpp
  app/src/main/cpp/assets/mesh_loader.cpp
  app/src/main/cpp/character/face_gen_morpher.cpp
  app/src/main/cpp/character/face_gen_cache.cpp
  app/src/main/cpp/geometry/skin_partition_packer.cpp
  app/src/main/cpp/collision/collision_world.cpp
  app/src/main/cpp/collision/aabb_tree.cpp
  app/src/main/cpp/collision/character_controller.cpp
  app/src/main/cpp/world/world_manager.cpp
  app/src/main/cpp/world/game_state_report.cpp
  app/src/main/cpp/world/cell.cpp
  app/src/main/cpp/world/door.cpp
  app/src/main/cpp/world/cell_transition_manager.cpp
  app/src/main/cpp/game/interactable.cpp
  app/src/main/cpp/game/container.cpp
  app/src/main/cpp/game/merchant.cpp
  app/src/main/cpp/game/merchant_factory.cpp
  app/src/main/cpp/game/barter.cpp
  app/src/main/cpp/game/item_catalog.cpp
  app/src/main/cpp/system/settings_manager.cpp
  app/src/main/cpp/system/oblivion_ini.cpp
  app/src/main/cpp/system/oblivion_ini_defaults.cpp
)

echo "::group::Build host tests"
# Rust archives must follow the objects that reference them (link order).
# Rust's std drags in winsock/ntdll/userenv symbols on a MinGW host only, so
# keep those out of the Linux CI link line.
# Host tests are a correctness gate, not a performance artifact.  This is one
# deliberately large translation unit; -O2 makes local and CI compilation
# needlessly long without improving the assertions being executed.
RUST_LIBS=(
  "$RUST_LIB_DIR/libassets_ffi.a"
  "$RUST_LIB_DIR/libbsa_ffi.a"
  "$RUST_LIB_DIR/libplugin_ffi.a"
  "$RUST_LIB_DIR/libesm_ffi.a"    # esm_reader.cpp is an ESMFile shim over esm_ffi.h
)
# nif/dds are linked the same way for symmetry; their shims are in SOURCES, so a
# build directory missing libnif_ffi.a / libdds_ffi.a will fail at link.
for _opt in "$RUST_LIB_DIR/libnif_ffi.a" "$RUST_LIB_DIR/libdds_ffi.a"; do
  if [ -f "$_opt" ]; then
    RUST_LIBS+=("$_opt")
  fi
done
EXTRA_LIBS=()
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    EXTRA_LIBS=(-lws2_32 -lntdll -luserenv -lbcrypt -ladvapi32 -lshell32 -lole32)
    ;;
esac
g++ -std=c++17 -O0 -ffunction-sections -fdata-sections \
    "${DEFINES[@]}" "${INCLUDES[@]}" "${SOURCES[@]}" \
    "${RUST_LIBS[@]}" ${EXTRA_LIBS[@]+"${EXTRA_LIBS[@]}"} \
    -Wl,--gc-sections -o "$BIN" -lz
echo "::endgroup::"

echo "::group::Run host tests"
"$BIN"
echo "::endgroup::"