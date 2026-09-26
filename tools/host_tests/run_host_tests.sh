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

INCLUDES=(
  -Itools/host_tests/stubs          # android/log.h, asset_manager.h, jni.h
  -Iapp/src/main/cpp
  -Iapp/src/main/cpp/include
  -Iapp/src/main/cpp/assets
  -Iapp/src/main/cpp/third_party/JoltPhysics
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
  app/src/main/cpp/tests/weather_transition_tests.cpp
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
  app/src/main/cpp/assets/esm_reader.cpp
  app/src/main/cpp/engine/memory_pool.cpp
  app/src/main/cpp/engine/async_task_manager.cpp
  app/src/main/cpp/engine/cache_manager.cpp
  app/src/main/cpp/engine/event_bus.cpp
  app/src/main/cpp/engine/state_manager.cpp
  app/src/main/cpp/save_system/save_manager.cpp
  app/src/main/cpp/save_system/save_slot_manager.cpp
  app/src/main/cpp/script/script_context.cpp
  app/src/main/cpp/script/script_disasm.cpp
  app/src/main/cpp/script/script_vm.cpp
  app/src/main/cpp/script/script_functions.cpp
  app/src/main/cpp/script/script_manager.cpp
  app/src/main/cpp/game/quest_manager.cpp
  app/src/main/cpp/game/inventory_manager.cpp
  app/src/main/cpp/game/inventory.cpp
  app/src/main/cpp/audio/audio_manager.cpp
  app/src/main/cpp/audio/audio_3d.cpp
  app/src/main/cpp/animation/skeleton.cpp
  app/src/main/cpp/animation/animation_player.cpp
  app/src/main/cpp/assets/nif_parser.cpp
  app/src/main/cpp/assets/nif_block_type_map.cpp
  app/src/main/cpp/assets/mesh_loader.cpp
  app/src/main/cpp/character/face_gen_morpher.cpp
  app/src/main/cpp/character/face_gen_cache.cpp
  app/src/main/cpp/geometry/skin_partition_packer.cpp
  app/src/main/cpp/collision/collision_world.cpp
  app/src/main/cpp/collision/aabb_tree.cpp
  app/src/main/cpp/collision/character_controller.cpp
  app/src/main/cpp/world/world_manager.cpp
  app/src/main/cpp/world/cell.cpp
  app/src/main/cpp/world/door.cpp
  app/src/main/cpp/world/cell_transition_manager.cpp
)

echo "::group::Build host tests"
g++ -std=c++17 -O2 -ffunction-sections -fdata-sections \
    "${DEFINES[@]}" "${INCLUDES[@]}" "${SOURCES[@]}" \
    -Wl,--gc-sections -o "$BIN" -lz
echo "::endgroup::"

echo "::group::Run host tests"
"$BIN"
echo "::endgroup::"