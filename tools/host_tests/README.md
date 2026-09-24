# Host Test Runner (Native C++ tests on desktop CI)

The Android build compiles the native C++ engine with the NDK. Some test suites
are pure logic (no GLES surface, no assets) and can be compiled and executed on
a plain desktop host. This directory contains everything needed to run them in
CI without an Android device or emulator.

## What is covered

The runner builds one binary that executes every wired suite in order:

| Suite | Source | Tests | Notes |
|-------|--------|-------|-------|
| `ScriptVMTests` | `tests/script_vm_tests.cpp` | 22 | Script bytecode interpreter (`ExecutionContext`, `ScriptVM`, opcode execution) and the native game function registry (`ScriptFunctions`) |
| `Phase45UnitTests` | `tests/phase45_unit_tests.cpp` | 40 | Combat/spell/NPC/dialogue/quest/save units plus `MemoryPool`, `AsyncTaskManager`, `CacheManager` |
| `Phase48StressTest` | `tests/phase48_stress_test.cpp` | 5 | Concurrent task, object pool, cache, `EventBus` and `NpcManager` stress |
| `Phase48IntegrationTest` | `tests/phase48_integration_test.cpp` | 7 | `StateManager` / `InputRouter` / `GameLoopCoordinator` / world / cell transition / combat / quest / save integration |
| `Phase30IntegrationTest` | `tests/phase30_integration_test.cpp` | 1 (skip) | NIF/animation/collision asset pipeline; skips itself when real Oblivion assets are absent (see below) |

The runner prints each suite's `getSummary()` and exits with status 1 if any
suite fails.

### Asset-dependent suites

`Phase30IntegrationTest` needs real Oblivion NIF files, which are not
redistributable and therefore absent on CI. It reads the asset root from the
`OBLIVION_ASSET_BASE` environment variable (empty by default) and, when that
path is missing or does not contain the first expected NIF, records a single
`SKIP_Assets_Unavailable` entry, prints
`SKIPPED: assets not available`, and reports success. Point the variable at a
real asset tree to run the full suite:

```sh
OBLIVION_ASSET_BASE=/path/to/Oblivion/Data bash tools/host_tests/run_host_tests.sh
```

No test assertion is weakened by the gate: every test is left intact and runs
unchanged as soon as assets are present.

### Known intermittent failure (pre-existing, app side)

`Phase45UnitTests`' `Async_Statistics` case is racy: it submits two no-op tasks,
waits on their futures and then reads `AsyncTaskManager::getStats()`, expecting
`totalCompleted >= 2`. `AsyncTaskManager::workerThread()` fulfils a task's
promise inside `task.func()` *before* `recordCompletion()` bumps
`totalCompleted_`, so a woken waiter can still observe the counter lagging by
one (observed once in ~86 consecutive runs of the same binary). The harness does
not mask this and no assertion was relaxed; removing it means restructuring
`AsyncTaskManager::submit()` / `workerThread()` so completion bookkeeping
happens before the future becomes ready.

## Layout

- `stubs/` — minimal re-implementations of NDK/JNI/GLES headers needed to
  compile engine sources on a host: `<android/log.h>`,
  `<android/asset_manager.h>`, `<jni.h>`, `GLES2/GL2.h` and `GLES3/gl3.h`
  (including the GL enumerants and entry points the asset/animation sources
  reference).
- `host_stub_syms.cpp` — `extern "C"` stub definitions for the `AAsset*` API and
  `jni_audio_*` bridge functions.
- `host_gl_stubs.cpp` — no-op definitions for the GLES3 entry points declared by
  the GLES3 stub header. Host runs never create a GL context, so these are only
  there to satisfy the linker; no test depends on GL output.
- `host_physics_stubs.cpp` — host stand-in for the Jolt-backed
  `oblivion::PhysicsManager`. Jolt is far too large to build for a host smoke
  test, so this reports "physics unavailable" (`init()` returns `false`,
  `createCharacter()` returns `nullptr`), which is exactly the physics-disabled
  path callers already null-guard. It fabricates no simulation results.
- `run_host_tests.sh` — build + run on CI (ubuntu-latest). Fails the job on any
  suite failure or build error. Links against zlib (`-lz`) for the asset
  decompression helpers.

## Usage (local)

```sh
# From the repo root (needs a C++17 compiler):
bash tools/host_tests/run_host_tests.sh
```

Configured include flags are the same ones the Android build uses
(`-DAUDIO_SYSTEM_ENABLED -DJPH_PROFILE_ENABLED`), plus `-include` headers to
compensate for sources that rely on transitive includes.

On Windows the runner also prepends the directory holding the `g++` it uses to
`PATH`, because Git Bash's bundled `/mingw64/bin` ships an older
`libstdc++-6.dll` / `libgcc_s_seh-1.dll` that shadows the ones matching the
compiler and makes the freshly built binary fail to start.

## Expanding coverage

When a suite needs no hardware/asset access, add its `runAllTests()` to
`host_runner_main.cpp` (it returns exit code 1 on any suite failure) and list
its dependency `.cpp` files in `run_host_tests.sh`. If a suite needs symbols
you have not stubbed yet, follow the pattern of `host_stub_syms.cpp`.

Note that MinGW/PE links resolve every symbol of every compiled translation
unit, so a suite only links when the whole transitive closure is satisfiable;
`--gc-sections` does not remove unreachable undefined references there.