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
| `Phase45UnitTests` | `tests/phase45_unit_tests.cpp` | 41 | Combat/spell/NPC/dialogue/quest/save units plus `MemoryPool`, `AsyncTaskManager`, `CacheManager` |
| `Phase48StressTest` | `tests/phase48_stress_test.cpp` | 5 | Concurrent task, object pool, cache, `EventBus` and `NpcManager` stress |
| `Phase48IntegrationTest` | `tests/phase48_integration_test.cpp` | 7 | `StateManager` / `InputRouter` / `GameLoopCoordinator` / world / cell transition / combat / quest / save integration |
| `WatrDecodeTests` | `tests/watr_decode_tests.cpp` | 11 | WATR record decoding from a synthetic ESM: the TES4 header, a WATR GRUP holding a full 102-byte `DefaultWater` DATA block plus two short records (42 / 2 bytes) that must inherit the default colours |
| `XtelDecodeTests` | `tests/xtel_decode_tests.cpp` | 15 | XTEL (door intercept target) decoding: the type-6 cell-children GRUP label becomes each REFR's `parentFormID`, a full 28-byte XTEL resolves to its target door, and a missing or truncated block is ignored |
| `WeatherTransitionTests` | `tests/weather_transition_tests.cpp` | 14 | `SkyWeatherSystem` driven through a scripted transition at a fixed 60 Hz step: the ramp is linear in time rather than the geometric decay the pre-P13 code produced |
| `InteriorCellTests` | `tests/interior_cell_tests.cpp` | 47 | `WorldManager::enterInteriorCell`, the `teleportinterior` command's entry point: an interior cell is registered, keyed by FormID instead of a grid coordinate, marked INTERIOR and made current, and re-entering the same FormID reuses it |
| `GameStateReportTests` | `tests/gamestate_report_tests.cpp` | 30 | `GameStateReport` line format behind the `gamestate` console command, including the interior case where `grid` must not be printed |
| `AssetPathTests` | `tests/asset_path_tests.cpp` | 24 | `asset_path_resolver.h`: the expansion rules that bridge an ESM model path (`Dungeons\Misc\TrigZone01.NIF`) and its BSA spelling (`Meshes\...`) |
| `MeshBuilderTests` | `tests/mesh_builder_tests.cpp` | 27 | `mesh_builder.h` NIF geometry merge: every `NiTriShape` / `NiTriStrips` contributes, indices are rebased onto the merged vertex array, missing attributes fall back to the `Vertex` default, and unusable geometry is skipped instead of producing out-of-range indices |
| `MenuXmlTests` | `tests/menu_xml_tests.cpp` | 223 | The two-stage menu pipeline (`buildMenuDef()` then `resolve()`), pinned to the real `book_menu.xml` expressions: the `_pagenum` accumulator, the `mult` operator, the x controller traits and the `&xbox;` entity style |
| `MenuFadeTests` | `tests/menu_fade_tests.cpp` | 40 | `menu_fade_system.h`, the port of the original fade node list (RE_EXE_REPORT.md 48.3 / 49.6): linear one-shot fades, the 0.001 s duration floor, replace-by-source, auto-removal at progress 1.0 and the per-menu steady-state alpha |
| `FatigueSystemTests` | `tests/fatigue_system_tests.cpp` | 59 | `game/fatigue_system.h`, the port of the Fatigue AV (0x0A) formulas from RE_EXE_REPORT.md 50.2 / 56.1 / 57.3: the GMST defaults and the exact curve / cost shapes, including the `Combat_CalcMeleeDamage` "exactly full fatigue contributes 1.0" quirk |
| `CombatFormulasTests` | `tests/combat_formulas_tests.cpp` | 72 | `game/combat_formulas.h`, the FUN_005FEBF0 fragments confirmed in RE_EXE_REPORT.md 56.1 (Block 50 / Luck 50 / x 1.0 -> block amount 0.625, weapon block 0.3125, hand-to-hand block 0.15625) |
| `Phase69PersistenceTests` | `tests/phase69_persistence_tests.cpp` | 30 | Container persistence: `SaveManager` CNTR section, the lazy-materialisation buffer (`applyPendingContainerState`) and the carry-forward case where a save happens before the player ever opened the container |
| `PluginListTests` | `tests/plugin_list_tests.cpp` | 23 | `plugins.txt` grammar and load order: `#` comments, the `*` active marker, BOM/CR/blank trimming, case-insensitive duplicate rejection, plain-list-means-all-active, serialise round trip and the Oblivion.esm / masters / plugins ordering |
| `FormIDIndexTests` | `tests/formid_index_tests.cpp` | 15 | FormID index translation from a plugin's MAST-relative space into load order: the base file's identity map, masters resolving to their real load position, a plugin's own index, reordered masters, unresolved masters, out-of-range indices, case-insensitive master names and preservation of the low 24 bits |
| `LodBlockTests` | `tests/lod_block_tests.cpp` | 10 (7 asset) | The shipped landscape LOD blocks through the same `lod_block_rules.h` helpers `original_lod_provider.cpp` uses: the `<formID>.<blockX>.<blockY>.32.<ext>` grammar, negative block flooring, the 8-float vertex packing and index rebasing, the rejection paths (empty geometry, > 65535 vertices, no usable triangles, stray indices dropped) and, over real blocks, vertex/index bounds, one UV per vertex, UV range, block-local placement and the 1,1,1 colour; skips the asset half when no blocks are found |
| `ObjectLodTests` | `tests/object_lod_tests.cpp` | 9 | `world/distant_lod/object_lod_rules.h`, the parser for the RE'd `.lod` object-record format (RE_EXE_REPORT.md 54.8 / v255): the `count + 8+28n` record walk, EOF trailing-byte tolerance, the formId/n sanity gates, `<worldspace>_<cellX>_<cellY>.lod` cell-name parsing with strict signed coordinates, and the stats counters |
| `ConditionTests` | `tests/condition_tests.cpp` | 19 | `game/condition_system.h`, the CTDA condition evaluator ported verbatim from the engine (RE_EXE_REPORT.md 58.1 / FUN_0056a510 / FUN_0056ac50 / FUN_0056a3b0): the 20-byte record layout, comparison ops 0-7 (6/7 always pass), the OR-group skip fall-through, the GetDisposition greater-ops skip, the global-value right side, the swapped-condition operand swap, secondary-list adoption and the 169-entry function table |
| `AiPackageTests` | `tests/ai_package_tests.cpp` | 11 | First-match package selection chain (RE canon 0x569020: flag predicates -> schedule day/time -> CTDA list): priority-ordered stack, the `[start, start+duration)` midnight-wrap window, day-of-week mask, zero-duration rejection, 24h acceptance, CTDA AND/skip/empty/no-runtime acceptance rules and downstream-stage blocking |
| `BarterPriceTests` | `tests/barter_price_tests.cpp` | 12 | Canonical barter pricing known-values for the RE'd trade model (RE_EXE_REPORT.md 57.5 / 59.5): FUN_00547B90 effective-skill clamps, buy/sell coefficient GMST formulas, trunc-toward-zero disposition term, ceilf rounding, FUN_00484850 stolen sentinel/discount rules, FUN_00548130 haggle acceptance, base-value floor/cap on `Merchant::buyPriceForValue` / `sellPriceForValue` |
| `EnchantingSystemTests` | `tests/enchanting_system_tests.cpp` | 11 | The MGEF effect tables against the measured Oblivion data (145 codes -> 88 families, the 21 fabricated codes that must fall back to `UNKNOWN`, malformed input) and the public enchanting arithmetic (defaults, `createEnchantment`, the magnitude/duration/cost formulas, the skill bonus, and `loadEnchantmentsFromESM` over a synthetic plugin); fixture tables live in `tests/enchanting_system_mgef_tables.inc`, generated from the measured CSV, not from the implementation |
| `OblivionIniTests` | `tests/oblivion_ini_tests.cpp` | 153 | `system/oblivion_ini.h`: the merged default table (entry count, the type/provenance breakdown, and the `[HAVOK]` vs `[Havok]` ordering the binary search depends on) plus the parser and resolver semantics (the first occurrence of a duplicate key wins, only whole-line comments are skipped, an empty value is legal) |
| `SettingsManagerTests` | `tests/settings_manager_tests.cpp` | 68 | `system/settings_manager.h` layering: `settings.txt` outranks Oblivion.ini, which outranks the shipped defaults, and `saveSettings()` rewrites only the keys an in-game menu actually changed |
| `MerchantTests` | `tests/merchant_tests.cpp` | 122 | Merchant generation and barter over a synthetic ESM: NPC / CNTO decoding with zero-count rows dropped, `CNAM` class FormID resolution including the cross-plugin link, the `itemIdFromFormID` / `formIDFromItemId` round trip, `resolveItemInfo` fallbacks, `npcIsMerchant` and `buildMerchantFromNpc` stock rules, the buy / sell arithmetic with the mercantile clamp, a purse split across several Gold slots and the FormID merge that keeps a FormID on one row, `tradeGold`, and the refusal reasons |
| `MerchantPersistenceTests` | `tests/merchant_persistence_tests.cpp` | 39 | The MERC section of the binary save: the v5 round trip of purse plus stock (all nine `MerchantItem` fields), plugin stock replacing rather than merging, one-shot consumption of the pending buffer, the load-time apply into a live registry, sold-out rows dropped, the carry-forward case and v4 compatibility (a MERC-less payload is ignored) |
| `Phase30IntegrationTest` | `tests/phase30_integration_test.cpp` | 1 (skip) | NIF/animation/collision asset pipeline; skips itself when real Oblivion assets are absent (see below) |

The runner prints each suite's `getSummary()` and exits with status 1 if any
suite fails.

### Rust FFI shims (both runners wired; MSVC since 2026-11-01)

`assets/plugin_list.cpp`, `assets/formid_index.cpp`, `assets/asset_path_resolver.cpp`,
`assets/mesh_builder.cpp`, `assets/bsa_reader.cpp`, `assets/nif_parser.cpp` and
`assets/dds_loader.cpp` no longer carry their parsing logic themselves: they are
thin shims over the Rust `plugin_ffi` / `assets_ffi` / `bsa_ffi` / `nif_ffi` /
`dds_ffi` crates (`#include "*_ffi.h"` plus the C ABI), which
`app/src/main/cpp/CMakeLists.txt` builds for Android with `cargo ndk` out of
`OBLIVION_RUST_DIR` (default `<repo>/../../../../../oblivion-re-tools`).

`run_host_tests.sh` (bash: Git Bash / Linux / CI) is wired: it defaults
`OBLIVION_RUST_DIR` to the sibling checkout, adds `-I<crate>/crates/*-ffi/include`
for the five crates and links the host-triple `lib*_ffi.a` archives it finds.
`run_host_tests_msvc.ps1` is wired the same way: it resolves `OBLIVION_RUST_DIR`
(or the sibling checkout), adds the five `-I` flags, runs `cargo build --release`
against the `msvc` staticlib crates when the `.lib` archives are missing, and
links the same set as the sh runner: `assets_ffi.lib` / `bsa_ffi.lib` /
`plugin_ffi.lib` / `esm_ffi.lib` required (the first three are the parser shims,
`esm_ffi` backs `esm_reader.cpp`), `nif_ffi.lib` / `dds_ffi.lib` when present,
plus `zlib.lib`. On a MinGW host the sh runner additionally links
`ws2_32`/`ntdll`/`userenv`/`bcrypt`/`advapi32`/`shell32`/`ole32` for Rust's std;
MSVC resolves those from its default libraries.

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

`LodBlockTests` needs the pre-baked landscape blocks
(`meshes\landscape\lod\<formID>.<blockX>.<blockY>.32.nif`), which are absent for
the same reason. It looks for them in `OBLIVION_LOD_NIF_DIR` when that is set (a
flat directory holding the extracted block NIFs), otherwise in
`<OBLIVION_ASSET_BASE>/meshes/landscape/lod`, and finally directly in
`OBLIVION_ASSET_BASE`; `OBLIVION_LOD_BLOCK_LIMIT` caps how many blocks are read
(default 100, spread evenly over the whole landscape). With no block found it
records one `SKIP_Assets_Unavailable` entry and passes, so the grammar and
packing cases - which need no assets - still run on CI:

```sh
OBLIVION_LOD_NIF_DIR=/path/to/lod-nifs bash tools/host_tests/run_host_tests.sh
```

Parsing a hundred blocks is the slowest suite in the run (tens of seconds
uninstrumented, minutes under ASan). `run_host_tests_msvc.ps1` stops the binary
after `-RunTimeoutSeconds` (default 300) and reports the partial output, so
raise it for a full asset-backed LOD run:

```powershell
$env:OBLIVION_LOD_NIF_DIR = "C:\path\to\lod-nifs"
powershell -File tools/host_tests/run_host_tests_msvc.ps1 -RunTimeoutSeconds 1800
```

### Resolved: `AsyncTaskManager` completion ordering

`Phase45UnitTests`' `Async_Statistics` case used to be racy: it submitted two
no-op tasks, waited on their futures and then read
`AsyncTaskManager::getStats()`, expecting `totalCompleted >= 2`.
`AsyncTaskManager::submit()` wrapped the callable in a `std::packaged_task`,
which fulfils the shared state from *inside* the callable, so
`workerThread()`'s `recordCompletion()` — the call that bumps
`totalCompleted_` — could still be pending when the waiter woke up.

A standalone probe reproduced it before the fix: 27 of 500 iterations failed
when polling with `wait_for(0ms)`, and 2 of 4000 iterations failed with
`wait()`, the path the unit test uses. `submit()` now hands the task a
`std::function` plus a `std::promise`, and `task.func()` records completion
*before* it publishes the result, so a ready future always implies the
statistics already include that task. After the fix, 20000 iterations of each
mode pass with zero failures.

The same change fixed a second defect: `std::packaged_task` stores a throwing
task's exception in its shared state instead of rethrowing it, so
`workerThread()`'s `catch` blocks never observed task failures and
`totalFailed_` stayed at 0 forever. The task now counts its own failure and
then forwards the exception to the promise, which makes a failing task both
observable to its caller (`future::get()` rethrows) and visible in
`getStats().totalFailed`.

`Phase45UnitTests/Async_FailureAccounting` pins that behaviour: it submits a
throwing value task, a throwing void task and a succeeding task, asserts that
`future::get()` rethrows `std::runtime_error` for both failures, and checks the
counters settle at `submitted=3`, `completed=1`, `failed=2` with
`submitted == completed + failed`.

Writing that assertion exposed a third, related hazard: `submit()` pushed the
task onto the queue *before* incrementing `totalSubmitted_`, so a worker could
finish and record a task before the submitting thread counted it, leaving
`submitted` momentarily below `completed + failed`. The increment now happens
before the push, so the identity holds at every observation point rather than
only in the steady state. The suite count went from 40 to 41 with this case.

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
- `run_host_tests_msvc.ps1` — local-Windows counterpart of the above: builds the
  same sources with MSVC x64 and runs them, AddressSanitizer on by default. See
  "Usage (local, MSVC + AddressSanitizer)".
- `msvc_compat.h` — force-included (`/FI`) by the MSVC runner only. Supplies the
  `S_ISDIR` / `S_ISREG` macros that g++ gets from `<sys/stat.h>` and MSVC does
  not define.

## Usage (local)

```sh
# From the repo root (needs a C++17 compiler):
bash tools/host_tests/run_host_tests.sh
```

Configured include flags are the same ones the Android build uses
(`-DAUDIO_SYSTEM_ENABLED -DJPH_PROFILE_ENABLED`), plus a `-include` list that
force-includes the standard headers the sources use implicitly. MinGW's
libstdc++ hands several of those out transitively (a source can therefore
compile here while it is missing an `#include`), while Ubuntu g++ does not, so
the workaround lives centrally in `DEFINES` rather than in each source. The list
is deliberately generous (`fstream`, `sstream`, `thread`, `mutex`, `atomic`,
`future`, `chrono`, ...); extending it is the supported fix for a header that is
only missing in CI. Do not add compiler-specific branches or Windows-only paths
to the runner - the only Windows-shaped code is the `PATH` prepend below, which
just puts the directory of the `g++` in use first and is a no-op on Linux.

Both toolchains are verified green with the current `SOURCES`: MinGW g++ 16.2.0
on Windows and Ubuntu 24.04 g++ 13.3.0 (the CI image) under WSL.

On Windows the runner also prepends the directory holding the `g++` it uses to
`PATH`, because Git Bash's bundled `/mingw64/bin` ships an older
`libstdc++-6.dll` / `libgcc_s_seh-1.dll` that shadows the ones matching the
compiler and makes the freshly built binary fail to start.

## Usage (local, MSVC + AddressSanitizer)

`run_host_tests_msvc.ps1` is the local-Windows counterpart of
`run_host_tests.sh`: same sources, same suites, same include/define set, built
with MSVC x64 instead of g++. It covers the two things the MinGW runner cannot:

- MSVC's AddressSanitizer (`/fsanitize=address`, enabled by default here)
  instruments the suites and reports heap/stack misuse that the g++ host build
  cannot detect.
- The resulting binary opens in the Visual Studio debugger:
  `devenv /debugexe tmp\host_tests\host_tests_msvc.exe`.

```powershell
# From the repo root (needs Visual Studio with the C++ x64 toolset):
powershell -File tools/host_tests/run_host_tests_msvc.ps1

# Without AddressSanitizer (roughly twice as fast, no memory checking):
powershell -File tools/host_tests/run_host_tests_msvc.ps1 -NoAsan

# Print the whole compiler output instead of only errors and the summary:
powershell -File tools/host_tests/run_host_tests_msvc.ps1 -ShowBuildOutput
```

### Known limitation: ASan plus an in-process AV hook stalls `Phase45UnitTests`

On a machine where an antivirus injects a monitoring module into every process
(`tmmon64.dll` on the development box), the instrumented run can wedge in
`Phase45UnitTests::~Phase45UnitTests()`: the destructor calls
`AsyncTaskManager::cleanup()`, whose `std::thread::join()` never returns. Two
minidumps pin the main thread in `msvcp140`'s `_Thrd_join` ->
`WaitForSingleObjectEx` while the worker stack holds only loader frames
(`ntdll`, `kernel32`, `ucrtbase`, `clang_rt.asan_dynamic-x86_64.dll`, the AV
module) and none from this binary, which is the shape of a `DLL_THREAD_ATTACH`
loader-lock deadlock between ASan's thread hook and the AV hook. This is an
environment limitation, not a suite defect: `AsyncTaskManager` is sound
(`stop_` is atomic, the wait predicate is evaluated under `queueMutex_`,
`cleanup()` is idempotent), and `-NoAsan` runs the identical 89 translation
units and every suite to `ALL GREEN` in a few minutes. If an ASan run stops
after `Phase45UnitTests`, add the repo and asset directories to the AV
exclusion list or use `-NoAsan`.

The runner finds Visual Studio through `vswhere.exe`, parses `INCLUDES`,
`DEFINES` and `SOURCES` out of `run_host_tests.sh` (so the two runners cannot
drift - edit the shell script to change the build set) and links zlib, which it
downloads from `https://zlib.net/fossils/` and builds with MSVC into
`tmp/host_tests/zlib-build/zlib.lib` on first use. `-RebuildZlib` forces a fresh
zlib build; `-ZlibLib <dir>\zlib.lib -ZlibInclude <dir>` links an external zlib
instead (for example one from vcpkg). The compiler log is written to
`tmp/host_tests/msvc_build.log`; objects go to `tmp/host_tests/obj/` and the
executable to `tmp/host_tests/host_tests_msvc.exe`.

`run_host_tests.sh` remains the CI path and the source of truth. The suite
sources stay toolchain-neutral: the only MSVC-specific piece is `msvc_compat.h`,
which is force-included rather than sprinkled as `#ifdef _MSC_VER` branches.

## Expanding coverage

When a suite needs no hardware/asset access, add its `runAllTests()` to
`host_runner_main.cpp` (it returns exit code 1 on any suite failure) and list
its dependency `.cpp` files in `run_host_tests.sh`. If a suite needs symbols
you have not stubbed yet, follow the pattern of `host_stub_syms.cpp`.

Note that MinGW/PE links resolve every symbol of every compiled translation
unit, so a suite only links when the whole transitive closure is satisfiable;
`--gc-sections` does not remove unreachable undefined references there.

### Dependency policy for `SOURCES`

List a source only when a wired suite's assertions actually reach it, so every
entry is justifiable as "suite X calls Y". The graph is derivable per object
with `nm -P --defined-only` / `nm -P -u`. Most of the current entries are
reachable from the wired suites; the unreachable ones are kept on purpose: the
five `quest/*.cpp` files were requested by the script-VM expansion workstream
(whose test file calls `QuestFlowController` and the quest sub-systems) and
`script/script_disasm.cpp` is used by that same file
(`ScriptDisasm::disassemble`). Nothing in the current master test set
references either, so dropping them would only save a few seconds of compile
time - and would break the expansion branch's link.

Reachability is not just about what the assertions call: MinGW/PE links resolve
every symbol of every compiled translation unit, so a source pulled in as a
dependency drags in its own closure. `assets/asset_manager.cpp` is the clearest
example - `npc_manager.cpp` (`fileExists`) and `audio_manager.cpp`
(`loadFileData`) need it, and it in turn instantiates `NIFParser`, `DDSLoader`,
`BSArchive`, `Mesh`, `Material` and `ShaderProgram`, so `geometry/mesh.cpp`,
`geometry/material.cpp` and `engine/shader.cpp` are all in `SOURCES`. Do not
conclude a closure is satisfied from an `nm` pass alone; a filtered `nm` list
hides C++ mangled names easily. Verify by actually linking.

`game/container.cpp` and `game/interactable.cpp` are the same story on the
Phase 69 side: `save_manager.cpp` calls `Container::addItem` / `clearItems` /
`setLocked` and `WorldManager` stores `std::shared_ptr<Container>`, so the
moment the CNTR section existed the link broke without them
(`undefined reference to Container::addItem(InventorySlot const&)`).
`Phase69PersistenceTests` asserts through the real `Container`, so both are
listed for their assertions too - do not replace either with a stub.

Stubs are only acceptable for *leaf* dependencies, i.e. things with no host
implementation at all: the NDK/JNI APIs (`stubs/`, `host_stub_syms.cpp`), GLES
(needs a GL context) and Jolt (`host_physics_stubs.cpp`). Never stub a
mid-layer class a suite exercises (`CellManager`, `DoorManager`,
`CellTransitionManager`, `WorldManager`, `ESMManager`, `NpcManager`,
`SaveManager`, `weave::EventBus`, `AssetManager`, ...) - a suite that passes
only because its subject was stubbed verifies nothing but the stub's defaults.
All of those are real sources here. If a suite cannot link without stubbing what
it tests, leave it unwired and report the blocker rather than wiring a hollow
suite; and keep any skip explicit in code, because the runner runs under
`set -euo pipefail`.

The GLES stub is a *leaf* stand-in for a driver, not for a renderer: it declares
the entry points `geometry/mesh.cpp`, `geometry/material.cpp` and
`engine/shader.cpp` reference and defines them as no-ops in
`host_gl_stubs.cpp`. `glCreateShader` / `glCreateProgram` return `0` and
`glGetShaderiv` / `glGetProgramiv` write `0`, so `ShaderProgram::compile()` takes
its failure path and never dereferences a null program. Extend the header and
the stub file together, one declaration per definition - a declaration without a
definition is an undefined reference, and a definition without a declaration
does not compile.

Keep stubs and real sources mutually exclusive: a stub and the real `.cpp`
for the same symbol must never both be listed in `SOURCES`, because MinGW/PE
links then fail with a duplicate-symbol error. Every symbol the wired suites
need comes from exactly one of the two sides. `weave::EventBus`
(`engine/event_bus.cpp`), `NpcManager::getNPC(uint32_t)` (`game/npc_manager.cpp`)
and `Player::addExperience(float)` (`game/player.cpp`) are real implementation
sources - no stub defines them. The only host-only stand-ins are the `AAsset*` /
`jni_audio_*` symbols, the GLES3 entry points and `PhysicsManager`, whose real
counterparts (NDK, the JNI bridge, a GL driver, the Jolt library) cannot run on a
host. `PhysicsManager` is reached only from `game/npc_manager.cpp` and
`game/player_controller.cpp`, and only on the physics-disabled path.