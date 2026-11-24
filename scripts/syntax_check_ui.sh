#!/usr/bin/env bash
# Syntax-check the UI translation units the host test runner does NOT compile.
#
# Why this exists: race_sex_panel_instantiator.cpp is device-only (it needs a GL
# context and a TextRenderer), so tools/host_tests/run_host_tests.sh never
# compiles it. On 2026-11-23 a `clearChildren()` typo in that file passed the
# host suite and only failed in the (slow) APK CI job. This script compiles the
# same device-only TUs locally with the APK's DEFINES/INCLUDES so the error is
# caught before pushing. Exit 1 on any error.
#
# renderer.cpp is included: -fsyntax-only does not link, so the GL entry points
# the host stub lacks are not needed, and it catches type errors like the
# Player/NPC& mismatch that only the APK job would otherwise see.
set -euo pipefail
cd "$(dirname "$0")/.."

RUST_DIR="${OBLIVION_RUST_DIR:-$(pwd)/rust}"
STUBS=tools/host_tests/stubs

INCLUDES=(
  -I"$STUBS"
  -Iapp/src/main/cpp
  -Iapp/src/main/cpp/include
  -Iapp/src/main/cpp/assets
  -Iapp/src/main/cpp/third_party/JoltPhysics
  -I"$RUST_DIR/crates/assets-ffi/include"
  -I"$RUST_DIR/crates/bsa-ffi/include"
  -I"$RUST_DIR/crates/plugin-ffi/include"
  -I"$RUST_DIR/crates/esm-ffi/include"
  -I"$RUST_DIR/crates/nif-ffi/include"
  -I"$RUST_DIR/crates/dds-ffi/include"
)
# Mirrors tools/host_tests/run_host_tests.sh DEFINES.
DEFINES=(
  -DAUDIO_SYSTEM_ENABLED
  -DJPH_PROFILE_ENABLED
  -include string -include cstring -include algorithm -include cstdint
  -include vector -include memory -include unordered_map -include functional
  -include array -include map -include set -include queue -include limits
  -include cmath -include cstdlib -include cstdio -include stdexcept
  -include utility -include new
)
# Device-only TUs (in the APK CMake build, not in the host runner).
SOURCES=(
  app/src/main/cpp/ui/race_sex_panel_instantiator.cpp
  app/src/main/cpp/ui/ui_character_creation.cpp
  app/src/main/cpp/ui/ui_slider.cpp
  app/src/main/cpp/ui/ui_toggle.cpp
  app/src/main/cpp/ui/game_console.cpp
  app/src/main/cpp/engine/renderer.cpp
)

for src in "${SOURCES[@]}"; do
  echo "::group::syntax $src"
  g++ -std=c++17 -fsyntax-only "${DEFINES[@]}" "${INCLUDES[@]}" "$src"
  echo "::endgroup::"
done
echo "syntax_check_ui: OK (${#SOURCES[@]} device-only TU(s))"
