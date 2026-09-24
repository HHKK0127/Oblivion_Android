#!/usr/bin/env bash
# Build and run the native C++ host test suites for CI.
# Fails (exit 1) on any compilation error or suite failure.
set -euo pipefail

cd "$(dirname "$0")/../.."   # repo root

BIN="${TMPDIR:-/tmp}/host_tests/host_tests"
mkdir -p "$(dirname "$BIN")"

INCLUDES=(
  -Itools/host_tests/stubs          # android/log.h, asset_manager.h, jni.h
  -Iapp/src/main/cpp
  -Iapp/src/main/cpp/include
  -Iapp/src/main/cpp/assets
  -Iapp/src/main/cpp/third_party/JoltPhysics
  -Itests
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
)

SOURCES=(
  tools/host_tests/host_runner_main.cpp
  tools/host_tests/host_stub_syms.cpp
  app/src/main/cpp/tests/script_vm_tests.cpp
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
)

echo "::group::Build host tests"
g++ -std=c++17 -O2 "${DEFINES[@]}" "${INCLUDES[@]}" "${SOURCES[@]}" -o "$BIN"
echo "::endgroup::"

echo "::group::Run host tests"
"$BIN"
echo "::endgroup::"