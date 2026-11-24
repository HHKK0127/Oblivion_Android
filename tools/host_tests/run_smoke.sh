#!/usr/bin/env bash
# Build and run the deterministic newgame host smoke.
#
# Reuses run_host_tests.sh's build environment (Rust archives, INCLUDES,
# DEFINES) so the two host jobs cannot drift apart, but links a small curated
# source set around tools/host_tests/newgame_smoke.cpp instead of the full test
# binary. The smoke replays the chargen commit against the fixed fixture ESM
# (docs/evidence/fixtures/minimal.esm) and prints JSON on stdout.
#
# Usage: tools/host_tests/run_smoke.sh [fixture.esm]
# Exit codes: the smoke tool's own (see newgame_smoke.cpp), plus 1 on build
# failure.
set -euo pipefail

cd "$(dirname "$0")/../.."   # repo root

# Pull in RUST_LIB_DIR, INCLUDES, DEFINES, EXTRA_LIBS, RUST_LIBS without
# building or running the unit-test binary.
HOST_TESTS_SOURCE_ONLY=1 source tools/host_tests/run_host_tests.sh

BIN="${TMPDIR:-/tmp}/host_tests/newgame_smoke"
mkdir -p "$(dirname "$BIN")"

# Only what the commit path needs: Player + NpcManager::applyRaceToPlayer over
# the ESM reader, with the same host stubs the unit-test binary uses. Unused
# functions are dropped by -ffunction-sections/--gc-sections at link time.
SMOKE_SOURCES=(
  tools/host_tests/newgame_smoke.cpp
  tools/host_tests/host_stub_syms.cpp
  tools/host_tests/host_gl_stubs.cpp
  tools/host_tests/host_physics_stubs.cpp
  app/src/main/cpp/game/npc_manager.cpp
  app/src/main/cpp/game/npc.cpp
  app/src/main/cpp/game/player.cpp
  app/src/main/cpp/game/chargen_commit.cpp
  app/src/main/cpp/game/spell_manager.cpp
  app/src/main/cpp/assets/esm_reader.cpp
  app/src/main/cpp/assets/asset_manager.cpp
  app/src/main/cpp/assets/asset_path_resolver.cpp
  app/src/main/cpp/character/face_gen_morpher.cpp
  app/src/main/cpp/character/face_gen_cache.cpp
  # RaceSexPanel (page/item totals + the Random body), with the same UI/GL
  # stubs the unit-test binary links.
  app/src/main/cpp/ui/race_sex_panel.cpp
  app/src/main/cpp/ui/ui_panel.cpp
  app/src/main/cpp/ui/ui_component.cpp
  app/src/main/cpp/ui/ui_draw_helper.cpp
  app/src/main/cpp/ui/ui_slider.cpp
  app/src/main/cpp/ui/ui_toggle.cpp
  app/src/main/cpp/engine/texture_loader.cpp
)

# Progress/build messages go to stderr so stdout is exactly the smoke JSON
# (the CI job redirects stdout to out.json and compares it).
echo "::group::Build newgame smoke" >&2
g++ -std=c++17 -O0 -ffunction-sections -fdata-sections \
    "${DEFINES[@]}" "${INCLUDES[@]}" "${SMOKE_SOURCES[@]}" \
    "${RUST_LIBS[@]}" ${EXTRA_LIBS[@]+"${EXTRA_LIBS[@]}"} \
    -Wl,--gc-sections -o "$BIN" -lz
echo "::endgroup::" >&2

echo "::group::Run newgame smoke" >&2
"$BIN" "$@"
echo "::endgroup::" >&2
