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

# Reuse the unit-test binary's full source set (every dependency already links
# there), swapping the test runner main for the smoke main. This avoids a
# hand-curated list drifting out of sync with the ESM/quest/formid TUs that
# esm_reader.cpp pulls in.
SMOKE_SOURCES=()
for _src in "${SOURCES[@]}"; do
  case "$_src" in
    tools/host_tests/host_runner_main.cpp) continue ;;
  esac
  SMOKE_SOURCES+=("$_src")
done
SMOKE_SOURCES+=(tools/host_tests/newgame_smoke.cpp)

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
