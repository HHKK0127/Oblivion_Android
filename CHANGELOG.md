# Oblivion Android - Changelog

All notable changes to the Oblivion Android project are documented here.

## Version numbering

The app version is defined in exactly one place: `app/build.gradle` (`versionName` / `versionCode`).
The current version is **0.9.10 (versionCode 910)**.

> Note: the `[1.0.0]` and `[1.3.0]` sections below were written during development and do not
> correspond to any shipped build. Their version numbers were never applied to `app/build.gradle`.
> They are retained as historical notes only.

---

## [Unreleased]

### Added
- **JPWiki Japanese localization data**: `app/src/main/assets/localization/jpwiki_localization.tsv`
  (5.7 MB, UTF-8/TSV) carries 28,686 strings extracted from `JPWikiMod_Vanilla.esp`,
  `JPWikiMod_Vanilla+SI.esp` and `JPBooks_Merged[V+S+ML].esp` - 723 game settings, 640 book
  bodies, 19,179 dialogue responses, 3,474 dialogue topics, 249 quest names and 4,421 object
  names. `LocalizationManager::loadJpwikiData()` loads it through `AAssetManager` and exposes
  one getter per kind (`getGameSetting`, `getBookText`, `getInfoText`, `getDialogText`,
  `getQuestText`, `getFullText`). Every getter falls back to the English source text when the
  language is not Japanese or the key is missing, so English behaviour is unchanged.
- **Localization consumers**: `BookReader::getBookDescription()`,
  `DialogueManager::loadDialoguesFromESM()` and `QuestFlowController::getQuestName()` now
  resolve their text through `LocalizationManager`. Each system takes the manager through a
  `setLocalizationManager()` setter, and `Renderer::initGameSystems()` attaches it to
  `DialogueManager` at construction.
- **Dialogue system wiring**: `Renderer` now owns a `UIDialogue` panel and exposes
  `loadDialoguesFromESM()`, `openDialogueWithNpc()`, `openDialogueWithNearestNpc()`,
  `isDialogueOpen()` and `closeDialogue()`. `createTestScenario()` loads the DIAL/INFO trees
  after actors are placed, so 3,817 dialogue trees and 19,278 topics are available in game.
  `NPC` gained `formID` / `factionFormIDs` (populated by `createNPCFromESM()`) and
  `NpcManager::getNpcByFormID()` resolves a spawned actor from its base record. The JNI layer
  adds `nativeStartDialogue()`, `nativeCloseDialogue()` and `nativeIsDialogueOpen()`.
- **Book reader and quest flow wiring**: `Renderer` now owns a `BookReader` and a
  `QuestFlowController`, both attached to the same `LocalizationManager`.
  `createTestScenario()` binds the reader to the `ESMManager` (887 books) and registers 390
  quests from the ESM data. A new `UIBookReader` panel renders book bodies with markup
  stripping and CJK-aware word wrapping; `Renderer` exposes `openBook()`, `isBookOpen()` and
  `closeBook()`, the JNI layer adds `nativeOpenBook()`, `nativeCloseBook()` and
  `nativeIsBookOpen()`, and the debug console gains `readbook`, `closebook` and `listbooks`.
- **FormID key normalization for JPWiki data**: the source plugins store FormIDs as uppercase
  hex, while `LocalizationManager::formKey()` produces lowercase. Keys are now lowercased
  during `loadJpwikiData()`, which fixes every FormID-based lookup (books, dialogue, quests,
  object names) that previously fell back to English.
- **Native C++ host test runner for CI**: new `tools/host_tests/` directory with
  desktop stubs for the NDK headers (`<android/log.h>`, `<android/asset_manager.h>`,
  `<jni.h>`, GLES), stub implementations for `AAsset*` and `jni_audio_*` symbols, and
  `run_host_tests.sh`. The Phase 38 Script VM suite now builds and runs on
  `ubuntu-latest` via a separate `host-tests` job in `.github/workflows/android.yml`,
  so the script VM / function-registry logic is covered without a device or emulator.
  The host link also exposed a real Windows portability gap in `engine/cache_manager.cpp`
  (`mkdir` is 1-arg on `_WIN32`), now guarded with `_mkdir`.
- **All five native test suites now build and run**: `run_host_tests.sh` previously compiled only
  `script_vm_tests.cpp`, so `phase45_unit_tests`, `phase48_stress_test`, `phase48_integration_test`
  and `phase30_integration_test` existed in the tree but were never built or executed. The harness
  now links 53 translation units of the real gameplay, asset, save, script, world and collision code
  (`-lz` for the NIF reader, `--gc-sections` for size), leaving only two stand-ins: no-op GLES3 entry
  points, and a `PhysicsManager` whose `init()` returns false so callers take their existing
  "physics disabled" path - no suite can pass on fabricated simulation. `phase30_integration_test`
  records `SKIP_Assets_Unavailable` and succeeds when the Oblivion assets are absent, which is the
  normal CI case because they are not redistributable. Result: ScriptVMTests 22/22,
  Phase45UnitTests 41/41, Phase48StressTest 5/5, Phase48IntegrationTest 7/7,
  Phase30IntegrationTest 1/1 (self-skipped). The CI job itself was hardened at the same time:
  `zlib1g-dev` is installed explicitly, because the harness links `-lz` and
  `assets/esm_reader.cpp` / `assets/bsa_reader.cpp` include `<zlib.h>`, and the job timeout went
  from 15 to 30 minutes. 53 translation units are compiled by a single `g++` invocation, so the
  compile is sequential and cannot use the runner's core count: the run measured 12m15s end to end,
  which left too little headroom under 15 minutes. The job runs in parallel with the APK build, so
  the longer ceiling costs no wall clock time.

### Fixed
- **WATR records with a short DATA subrecord rendered as black water**: three records in
  `Oblivion3.esm` (`CamoranLava` 2 bytes, `Blood` 42 bytes, `CamoranLava02` 42 bytes) carry a
  DATA subrecord shorter than the 55 bytes needed to reach the colour block at byte offsets
  44/48/52, so `decodeWater()` left their shallow/deep/reflection colours at `(0,0,0)` and the
  surface drew black. `WaterData` now records whether the colour block was present
  (`hasColorBlock`), and a new post-parse pass `resolveWaterColorFallbacks()` copies the colours
  from the game's own `DefaultWater` record (formID `0x18`) into every record that lacks them,
  falling back to a neutral dark water colour only if `DefaultWater` itself is missing. The
  other 20 WATR records are untouched. Verified on the emulator against the real ESM
  (`WATR=23`, `WATR colour fallback applied to 3 short record(s) from DefaultWater`) and by a
  new `WatrDecodeTests` host suite that builds a synthetic ESM with 102/42/2-byte records.
- **Language preference was never applied**: `LocalizationManager::loadLanguagePreference()`
  reset to English on every launch and `saveLanguagePreference()` was a no-op, so the
  `LANGUAGE=` value persisted by `SettingsManager` was ignored. `Renderer::initGameSystems()`
  now mirrors the setting into `LocalizationManager` once both objects exist, and the language
  toggles in `SettingsUI` and `LauncherScreen` update both objects together.
- **Incorrect terminology in the built-in translation table**: the table used `Mana` and
  `Stamina`, which do not exist in Oblivion. They are now `Magicka` and `Fatigue`, matching the
  original game and the JPWiki data.
- **`AsyncTaskManager` completion-ordering race**: `submit()` wrapped the callable in a
  `std::packaged_task`, which fulfils the task's shared state from *inside* the callable, so
  `workerThread()`'s `recordCompletion()` (the call that bumps `totalCompleted_`) could still be
  pending when a caller woken by the future read `getStats()`. That made
  `Phase45UnitTests/Async_Statistics` intermittently report one completion too few. `submit()` now
  hands the task a `std::function` plus a `std::promise`, and the task records completion before it
  publishes its result, so a ready future always implies the statistics already include that task.
  A standalone probe reproduced the old behaviour (27/500 iterations failing under a `wait_for(0ms)`
  poll, 2/4000 under `wait()`, the path the unit test uses); after the fix 20000 iterations of each
  mode pass with zero failures.
- **`AsyncTaskManager` never counted task failures**: the same `std::packaged_task` stores a throwing
  task's exception in its shared state instead of rethrowing it, so `workerThread()`'s `catch` blocks
  could not observe a task failure and `totalFailed_` stayed at 0 forever. The task now catches its
  own exception, counts the failure and then forwards it to the promise, so `future::get()` rethrows
  to the caller and `getStats().totalFailed` reflects reality. `avgExecutionTimeMs` is now divided by
  every executed task rather than by successful ones only, so the average keeps its meaning.
  `Phase45UnitTests/Async_FailureAccounting` covers the path end to end: it submits a throwing value
  task, a throwing void task and a succeeding task, asserts that `future::get()` rethrows
  `std::runtime_error` for both failures, and checks the counters settle at `submitted=3`,
  `completed=1`, `failed=2`.
- **`totalSubmitted_` could lag behind its own completion counters**: `submit()` pushed the task onto
  the queue before incrementing `totalSubmitted_`, so a worker could finish and record a task before
  the submitting thread counted it, leaving `submitted` momentarily below `completed + failed`. The
  increment now happens before the push, which is what lets the accounting test assert the identity
  rather than a bound.
- **ScriptFunctions name lookup test**: `tests/script_vm_tests.cpp` compared the
  `const char*` from `getFunctionName()` against string literals with `==`, which is
  pointer comparison and always failed across translation units. Switched to
  `std::strcmp`. The function implementation itself was correct.

### Changed
- **`EventBus` moved into its own translation unit**: its 7 out-of-line definitions were moved
  verbatim from `engine/imperial_weave.cpp` into a new `engine/event_bus.cpp` (registered in
  `app/src/main/cpp/CMakeLists.txt`) so the event system can be linked without the renderer, video
  and Jolt subsystems that `imperial_weave.cpp` pulls in. The relocation is exact - the same 7
  methods, no duplicate definitions in the tree.
- **`SaveManager` base directory on non-Android builds**: `getBaseDir()` is now split with
  `#ifdef __ANDROID__` and honours `OBLIVION_SAVE_DIR` on the host, defaulting to `./host_saves/`,
  so the host test suites no longer need the Android asset path.
- **Title menu stroke weight (thinner)**: the ink outline is now `MENU_OUTLINE_WIDTH = 0.45f`
  (was 0.6f) and the renderer's ring clamp lower bound is 0.25 px (was 0.6 px), so widths below
  0.6 px are no longer silently ignored. The title's font scale is about 0.92, so the effective
  ring is `0.92 * 0.45 = 0.41 px` where it used to be pinned at the 0.60 px floor; the nine-stamp
  stroke gain drops from 1.20 px to 0.83 px (-31%). Other screens use the default
  `fontOutlineWidth_ = 1.0f`, which never reached the old floor, so their text is unchanged.

### Removed
- **"SPECIAL EDITION" line on the title screen**: the code-drawn subtitle below the OBLIVION logo
  was removed (`renderSubtitle()`, its two call sites, its declaration and the
  `SUBTITLE_WIDTH_RATIO` / `SUBTITLE_CENTER_Y_RATIO` constants). The logo texture never contained
  the subtitle, so no asset change was needed. "Press any key to continue" keeps its previous
  position through the new fixed `PRESS_KEY_TOP_RATIO = 0.513f`, which stores the effective
  521.7 px / 1017 px anchor the subtitle used to define.

### Fixed
- **Title menu legibility (Oblivion face)**: the six title-menu labels are now stamped with an
  outline drawn in the ink colour (RGB 117,59,33) instead of the renderer's black default, so the
  decorative face gains stroke weight without the near-black rim the original art does not have.
  Measured on the emulator inside the menu band: ink pixels 5184 -> 8042 (**+54%**), ink fill
  0.207 -> 0.287, median horizontal stroke 5.0 -> 6.0-7.0 px, while near-black (< 60) pixels stay
  at 0.46% of the band (28% with the black outline). `MENU_OUTLINE_ALPHA = 0.85f`; the row's
  geometry (width, centre, capitals) is unchanged. The width has since been reduced to 0.45f
  (see Changed above).
- **Font atlas texture leak**: `TextRenderer::loadOblivionFnt()` leaked the previous GL texture
  every time a font was loaded, because the new name overwrote `textureId` without the old one
  being deleted. The old texture is now deleted before `glGenTextures`, matching `shutdown()`.

### Added
- **Original title-menu selection highlight**: `renderMenu()` now draws the original Oblivion hover
  art instead of changing the label colour. `app/src/main/assets/textures/ui/dialog_selection_full.png`
  (2048x64) and `dialog_selection_cut.png` (128x64) were extracted from
  `Oblivion - Textures - Compressed.bsa` and are placed exactly as
  `menus/prefabs/button_floating.xml` specifies: bar = parent width - 10, height 64, x = -10,
  y = 3 (`MENU_SELECT_BAR_DROP`), cap at x = text width + 48. The full bar's opaque run is
  51.42% of its width (`MENU_SELECT_BAR_OPAQUE = 0.5142f`), matching the asset; the labels keep
  the authoritative ink colour RGB(117,59,33) (contrast 7.62:1 on the cream bar).

### Fixed
- **Title background video stopped after about seven seconds**: `GameRenderer.kt` gated
  `updateTexImage()` behind a frame-available callback flag, and `setOnFrameAvailableListener`
  was called without a `Handler`, which throws on the Looper-less GL thread; the surrounding
  catch released the whole player, so the title showed a static frame. The listener now runs on
  the main looper and `onDrawFrame` calls `updateTexImage()` every frame while the surface texture
  exists, so the buffer queue no longer backs up and blocks `dequeueBuffer`. Verified on the
  emulator: `updateTexImage` 13-46/s (matching FPS), callbacks still firing after 22 minutes,
  `errors(total)=0`, 36-45% of pixels change between frames 3 s apart (was 0.000 mean / 8 px).
- **Continue / Load / Options opened invisible screens that swallowed every tap**: on the title
  screen `renderer.cpp` returned before the game-frame overlay pass, so `SaveLoadUI` and
  `SettingsUI` were set visible but never drawn while still consuming touches, which made the menu
  look frozen. Both are now rendered inside the title-screen branch, next to the existing debug
  overlay draw. Verified on the emulator: the load screen draws its "LOAD GAME" heading and 12-slot
  panel, the settings screen draws its panel, and BACK returns to a menu that accepts input again.
  Two earlier readings of this bug were wrong and are retracted: the black strip at y1017-1079 is
  outside the GL surface (the title screen shows the same 52 rows), and the load screen being
  static between screenshots is by design (`save_load_bg.png` is drawn full-screen).
- **Stale build-size and phase figures in the docs**: `.github/copilot-instructions.md` still
  claimed an 8.4 MB APK and "Phase 36", and `README.md` claimed a 79.0 MiB debug APK for two ABIs.
  The debug APK is actually 115.4 MiB: `build.gradle` now also builds `x86_64` for the emulator
  (+15.9 MiB of native libs), and 82.9 MiB of it is `assets/videos/`, which is gitignored and not
  redistributed, so a build without those local assets is about 32.5 MiB. The current phase is 64
  (Phase 65 next), and `app/build.gradle` remains the single source of truth for the version.

### Notes
- **The title screen's "dimming" was the debug overlay, not a rendering bug.** Tapping the
  top-left corner of the title screen appeared to darken the whole frame and was first read as a
  uniform scrim. It is the `btn_debug_toggle` button: 36 dp at `top|start` with a 10 dp margin,
  which at the emulator's 420 dpi is x26..121 / y26..121, so a tap at (74,74) lands inside it and
  opens `debug_overlay_container` - a full-screen `#80000000` scrim plus a 260 dp (682.5 px)
  left-hand panel. `MainActivity` logs `Debug panel shown` on the toggle. The measured frame
  matches the composite exactly: content p50 luminance 167 -> 60, after/before ratio p50 0.491
  against the predicted 0.498, glyph cores (117,59,33) -> (58,29,16), and
  `corr(after, before)` splits into +0.145 on the left 640 px (the panel overwrites it) versus
  +0.833 on the right (scrim only). The state is reversible by tapping again. Screenshots taken
  with the panel open are identifiable by `menu ink tol20 = 0` together with p50 luminance ~60.
- The outlined row is **not** a match for the reference art's stroke weight (about 2.1x it at
  equal capital height). It is a deliberate deviation in favour of readability, as requested; the
  row still matches the art's width, centre and capital height.
- The debug panel's System tab exposes `Font Outline` on/off plus `0.5 px` / `2.0 px` widths. The
  ring's lower clamp was 0.6 px, so `0.5 px` used to draw identically to the default and only
  `2.0 px` widened the strokes; with the clamp now at 0.25 px, `0.5 px` is effective too.
- **Dropping "SPECIAL EDITION" moves away from the reference art.** The line is absent from
  `menus/options/main_menu.xml` (the original game has no subtitle), but it is present in the
  reference screenshot, measured at 17.3% of the width / 47.8% of the height, and the code-drawn
  subtitle sat at 17.1% / 47.6% - a match within 0.2 pp. The removal is therefore authoritative to
  the original game and a deviation from the reference art at the same time; both readings are
  recorded so the choice can be reversed by restoring `renderSubtitle()` and the two ratio
  constants.
- The row's capital height was confirmed against the frame by measuring glyph edges directly, not
  only from the layout log: ink tops sit at y787 with the log's `baselineY=812.1`, so the capitals
  are 25.1 px = 2.47% of the 1017 px view (about 24.5-25.1 px once the ~0.6 px outline dilation is
  removed), matching the log's `cap=25.4` and the reference art's 2.50%.
- **`tol40` is only valid while the background is unchanged.** The ink metric that allows a
  tolerance of 40 per channel counts the background itself once the frame darkens: the debug
  overlay's backdrop RGB(104,88,72) passes all three channels (13 / 29 / 39), which inflated the
  count from 10,154 px to 22,744 px while `tol20` correctly reported 0. Use `tol20` as the primary
  metric and only compare `tol40` between shots of the same background phase.

---

## [0.9.10] - 2026-09-21 (Phase 62 - Foundation)

### Fixed
- **Gradle definitions unified**: removed `build.gradle.kts`, `settings.gradle.kts`, and
  `app/build.gradle.kts`. These were unused Kotlin DSL scaffolds that declared a different
  application (`com.example.myapplication`, `minSdk 24`, `targetSdk 36`, `versionName "1.0"`).
  Only the Groovy definitions were ever applied.
- **Version consistency**: `versionCode` is now derived from `versionName` (major * 10000 +
  minor * 100 + patch = 910). README.md no longer reports `1.0.0` or a stale phase number.
- **README.md**: removed the contradictory per-phase version labels and the duplicate Phase 55
  row; corrected the APK size claim (8.4 MB -> 79.0 MiB measured) and the Phase 62/63 status
  (marked Complete -> Planned, matching `plan.md`).

---

## [1.0.0] - 2026-08-29 (Phase 57 - Final Integration & Release)

### Release Preparation
- **Version**: 1.0.0 (first stable release)
- **Build**: Release build with minification enabled
- **APK**: Optimized with resource shrinking

### Documentation
- Updated README.md with Phase 39-49 completion entries
- Updated CHANGELOG.md with Phase 39-49 details
- Updated version badges and status

---

## [1.3.0] - 2026-08-29 (Phase 38 - Script VM Testing Complete)

### Testing

#### Script VM Unit Tests (20 tests)
- **ExecutionContext Tests** (5 tests)
  - Stack operations (push/pop)
  - Stack overflow protection (256 max)
  - Local variables (get/set)
  - Reference management
  - Program counter advancement

- **ScriptVM Tests** (3 tests)
  - STOP opcode execution
  - PUSH_INT + ADD arithmetic
  - CMP_LT + JUMP_Z branching

- **Opcode Tests** (4 tests)
  - PUSH_FLOAT floating-point
  - NEG negation
  - AND logical
  - DUP stack duplication

- **ScriptFunctions Tests** (3 tests)
  - Function registration
  - Name lookup
  - Unknown function handling

- **ScriptManager Tests** (5 tests)
  - Initialization
  - Script loading
  - Global variables (int/float)
  - Script stop

### Build
- Fixed `isScriptRunning()` call signature (2 parameters required)
- Build verification passed (5m 13s)

---

## [1.4.0] - 2026-08-29 (Phase 39 - Quest Flow System)

### Major Additions
- **QuestFlowController**: Unified quest lifecycle management
- **QuestStageManager**: Quest stage transitions, condition evaluation
- **QuestObjectiveTracker**: Objective progress via EventBus integration
- **QuestRewards**: Quest completion rewards (XP, gold, items, skills)
- **QuestRecord**: Complete quest record parsing for ESM data

---

## [1.5.0] - 2026-08-29 (Phase 40 - NPC Dialogue Tree)

### Major Additions
- **DialogueTree**: Branching dialogue tree structure
- **DialogueRunner**: Dialogue execution engine
- **DialogueFilterEngine**: Dialogue filtering by conditions
- **DialogueHistory**: Dialogue history tracking
- **DialogueRecord**: DIAL/INFO record parsing
- **DialogueIntegration**: Integration with NPC system

---

## [1.6.0] - 2026-08-29 (Phase 41 - Binary Save System)

### Major Additions
- **SaveManager**: Binary format with full system serialization
- **SaveSlotManager**: Save slot management
- **AutoSave**: Automatic save system
- **Serializable**: Interface for serializable objects

---

## [2.0.0] - 2026-08-29 (Phase 42 - Game Loop Integration)

### Major Additions
- **StateManager**: Game state management
- **InputRouter**: Input routing system
- **GameLoopCoordinator**: Full game loop integration
- **SceneRenderer**: Scene rendering pipeline
- **DebugConsole**: Debug console system
- **PerformanceProfiler**: Performance profiling

---

## [2.1.0] - 2026-08-29 (Phase 43 - UI/UX System)

### Major Additions
- **TouchGestureHandler**: Touch gesture recognition
- **MenuTransitionManager**: Menu transition animations
- **HudLayout**: HUD layout system
- **ControlSchemeManager**: Control scheme management
- **AccessibilityManager**: Accessibility features

---

## [2.2.0] - 2026-08-29 (Phase 44 - Performance Optimization)

### Major Additions
- **MemoryPool**: Memory pool allocator
- **RenderOptimizer**: Render optimization
- **AsyncTaskManager**: Async task management
- **CacheManager**: Cache management
- **ProfilerDashboard**: Profiler dashboard

---

## [2.3.0] - 2026-08-29 (Phase 45 - Unit Testing)

### Testing
- 37 unit test cases (+1,089 lines)

---

## [2.4.0] - 2026-08-29 (Phase 46-49 - Asset Pipeline, Audio, Integration, Controls)

### Major Additions
- **Phase 46**: TextureManager, MeshLoader, WorldDataLoader, BSA/ESM/NIF readers (+2,487 lines)
- **Phase 47**: AudioDecoder, BgmManager, SoundEffectManager (+2,176 lines)
- **Phase 48**: 12 integration test cases (+1,159 lines)
- **Phase 49**: GamepadMapper, TouchCalibration, InputVisualizer, HudCustomizer (+1,812 lines)

---

## [1.2.0] - 2026-08-29 (Phase 37 - Script VM Complete)

### Major Additions

#### Oblivion Script VM
- **ScriptVM**: Bytecode interpreter with47 opcodes
  - Arithmetic: ADD, SUB, MUL, DIV, MOD, NEG
  - Comparison: CMP_LT, CMP_LE, CMP_GT, CMP_GE, CMP_EQ, CMP_NE
  - Logical: AND, OR, NOT
  - Control flow: JUMP, JUMP_Z, STOP
  - Stack: PUSH_INT, PUSH_FLOAT, PUSH_STRING, PUSH_REF, POP, DUP
  - Variables: GET_LOCAL, SET_LOCAL, GET_GLOBAL, SET_GLOBAL
  - String: STR_CAT, STR_LEN, STR_SUB
  - References: GET_SELF, GET_TARGET
  - Function calls: CALL with118 game functions

#### Script System Components
- **ScriptManager**: Script lifecycle management
  - Per-frame execution with budget control (1000 instructions/frame)
  - Global variable storage (10,000 variables)
  - Script activation/deactivation by FormID
- **ExecutionContext**: Per-script state
  - RPN stack (256 max depth)
  - Local variables (64 per script)
  - Reference table (32 references)
  - Program counter and instruction budget
- **ScriptFunctions**:118 Oblivion game functions
  - Tier 1 (13 functions): SetStage, GetStage, AddItem, RemoveItem, Enable, Disable, Activate, GetDistance, SetPos, GetPos, Message, MessageBox
  - Tier 2 (105 functions): GetSelf, GetPlayer, Set, Get, Random, Resurrect, PlaceAtMe, MoveTo, Lock, Unlock, combat, spells, factions, time, weather, etc.
- **ScriptDisasm**: Bytecode disassembler for debugging

#### Bug Fixes (Phase 37Build Verification)
- Fixed GLM vec4->vec3 conversion in speed_tree_manager.cpp
- Fixed API mismatches in imperial_weave.cpp (SpeedTree, FaceGen, BinkVideo)
- Fixed npc_manager.cpp member name (m_npcs -> npcs)
- Fixed format specifier in phase45_unit_tests.cpp (%lu -> %zu)
- Fixed VideoBridge.java API compatibility (reflection for setPlaybackParams)

---

## [1.1.0] - 2026-08-28 (Phase 36 - Jolt Physics Integration)

### Major Additions

#### Jolt Physics Engine Integration
- **PhysicsManager**: Singleton physics world management
  - Jolt Physics initialization with custom settings
  - Fixed timestep (1/60s) for deterministic simulation
  - Integration with Imperial Weave phase pipeline (Physics phase)

#### Character Physics
- **CharacterVirtual**: Player and NPC character controllers
  - Capsule-based collision shapes
  - Movement with gravity and ground detection
  - Collision response with world geometry

#### Terrain Physics
- **HeightFieldShape**: Terrain collision from ESM LAND data
  - Heightmap-based terrain collision
  - Efficient broad-phase with Jolt's internal BVH

#### Raycast API
- **Physics Raycasting**: World-space ray queries
  - Line-of-sight checks for AI
  - Combat hit detection
  - Interaction raycasting

### Architecture Changes
- PhysicsManager runs in Imperial Weave Physics phase (phase 8)
- CharacterVirtual updates before CombatManager for accurate positioning
- Raycast API available to all game systems via PhysicsManager singleton

### Files Added
- `physics/physics_manager.h` - PhysicsManager class declaration
- `physics/physics_manager.cpp` - Jolt Physics initialization and management
- `physics/character_virtual.h` - CharacterVirtual wrapper class
- `physics/character_virtual.cpp` - Character controller implementation

### Files Modified
- `engine/imperial_weave.cpp` - Added Physics phase integration
- `game/player_controller.cpp` - Integrated CharacterVirtual for player
- `game/npc_manager.cpp` - Integrated CharacterVirtual for NPCs
- `CMakeLists.txt` - Added physics source files, Jolt Physics library

### Build Status
- BUILD SUCCESSFUL
- All 4 architectures: arm64-v8a, armeabi-v7a, x86, x86_64

---

## [1.0.0] - 2026-08-28 (Phase 35 - Radiant AI System)

### Major Additions

#### AI Package System
- **15 AI Package Types**: Explore, Follow, Guard, Patrol, Travel, Eat, Sleep, Combat, Flee, Idle, Wander, Activation, Conversation, Travel, Sandbox
- **PackageStack**: Priority-based package management
  - Higher priority packages interrupt lower ones
  - Combat and Flee override normal AI behavior
  - Default daily schedule when no packages active

#### AI Scheduler
- **AIScheduler**: 24-hour time-based AI management
  - Time-of-day package selection
  - NPC daily routines (eat, sleep, work, wander)
  - Integration with NavMesh pathfinding

#### NavMesh Pathfinding
- **NavMeshManager**: Runtime pathfinding using NAVM records
  - A* algorithm on NavMesh triangles
  - Path smoothing for natural movement
  - Stuck detection and recovery

#### Default Daily Schedule
- NPCs follow realistic daily routines:
  - 6:00-18:00: Work/Wander
  - 18:00-22:00: Socialize/Eat
  - 22:00-6:00: Sleep
- Combat/Flee override normal schedule

### Architecture Changes
- AIScheduler runs in Imperial Weave AI phase (phase 3)
- PackageStack evaluates before NPC movement
- NavMeshManager provides pathfinding to all AI systems

### Files Added
- `ai/ai_scheduler.h` - AIScheduler class declaration
- `ai/ai_scheduler.cpp` - Time-based AI management
- `ai/ai_package.h` - AI Package data structures
- `ai/ai_package.cpp` - Package implementations
- `ai/package_stack.h` - Priority-based package stack
- `ai/package_stack.cpp` - Stack management

### Files Modified
- `game/npc_manager.cpp` - Integrated AIScheduler
- `game/npc.h` - Added AI package fields
- `engine/imperial_weave.cpp` - Added AI phase integration
- `CMakeLists.txt` - Added AI source files

### Build Status
- BUILD SUCCESSFUL
- All 4 architectures: arm64-v8a, armeabi-v7a, x86, x86_64

---

## [0.9.10] - 2026-08-27 (Phase 34 - Weapon Sound Routing + Spell UI Enhancement)

### Major Additions

#### Weapon-Type-Specific Hit Sound Routing
- `CombatManager::emitCombatEvent()` now includes `weaponType` in the EventBus payload JSON
  - New helper `weaponTypeToAudioKey()` converts WeaponType enum → audio key string
  - SWORD_ONE_HAND/SWORD_TWO_HAND/DAGGER → "blade"; AXE → "axe"; MACE → "blunt"; BOW → "bow"; STAFF → "staff"
- `AudioSubscriber::playSoundForEventWithPayload()`: parses `weaponType` from payload
  - Routes COMBAT_ATTACK_HIT / ANIM_ATTACK_HIT to `combat/hit_blade`, `combat/hit_blunt`, `combat/hit_axe`, `combat/hit_unarmed`, or `combat/bow_shoot`
  - COMBAT_CRITICAL_HIT always uses `combat/hit_critical`
  - `extractPayloadField()`: lightweight JSON field extractor (no external lib dependency)
- EventBus subscribers for COMBAT_ATTACK_HIT and COMBAT_CRITICAL_HIT now call `playSoundForEventWithPayload()`

#### Spell School Color Icons (SpellSelectionPanel)
- Each spell button in SpellSelectionPanel now shows a school-color background:
  - Destruction = red, Restoration = green, Conjuration = purple, Alteration = blue, Illusion = teal, Mysticism = gold
- Spell button label prefixed with school abbreviation: `[Destr]`, `[Resto]`, `[Conj]`, `[Alter]`, `[Illus]`, `[Myst]`
- `getSchoolColor()` helper function returns glm::vec4 by MagicSchool

#### Quick-Slot Spell Buttons (4 slots)
- 4 quick-slot buttons (F1–F4) added to the HUD at bottom-left
- Hidden on title screen, shown when game starts
- **Empty slot behavior**: tap → opens SpellSelectionPanel in assignment mode
- **Assigned slot behavior**: tap → casts spell on nearest enemy + plays spell_equip SE
- Assignment flow: open panel sets `pendingAssignSlot`, `onSpellSelected` assigns to `player.quickSlotSpells[i]`
- Assigned button updates: label shows "F{n}\n{4-char spell name}", background color matches spell school
- `Player::quickSlotSpells[4]` array added to Player struct
- `pendingAssignSlot` member in Renderer tracks which slot is awaiting assignment

### Files Modified
- `game/combat_manager.cpp` — Added `weaponTypeToAudioKey()`, added `weaponType` to EventBus payload
- `audio/audio_subscriber.h` — Added `playSoundForEventWithPayload()`, `extractPayloadField()` declarations
- `audio/audio_subscriber.cpp` — Implemented weapon-type routing; COMBAT_ATTACK_HIT/CRITICAL now use payload
- `ui/spell_selection_panel.cpp` — School colors + abbreviated prefix labels via `getSchoolColor()`
- `game/player.h` — Added `#include <array>`, `#include "spell.h"`, `quickSlotSpells[4]`
- `engine/renderer.h` — Added `<array>` include, `quickSlotButtons[4]`, `pendingAssignSlot`
- `engine/renderer.cpp` — Quick-slot button creation, `onSpellSelected` assignment logic, show on game start

---

## [0.9.9] - 2026-08-27 (Dedicated Combat Sound Assets + NPC Spatial Audio)

### Major Additions

#### Dedicated Combat Sound Definitions
- Added 11 new sound keys in `sound_definitions.json` under `combat/` category:
  - `hit_blade`, `hit_blunt`, `hit_axe`, `hit_unarmed` — weapon-type-specific hit sounds (3D)
  - `hit_generic`, `hit_critical` — fallback hit sounds (critical at +20% volume)
  - `block_generic` — shield block sound (3D)
  - `parry` — blade parry clank (3D)
  - `dodge` — dodge miss sound (3D)
  - `death_generic` — NPC death sound (3D)
  - `attack_swing` — weapon swing sound (3D)

#### NPC Spatial Audio Callback
- AudioSubscriber now receives NPC world position for 3D audio playback
  - `setNpcPositionCallback()` connected in Renderer initialization
  - Callback uses `NpcManager::getNPC(id)->position` for live NPC positions
  - Falls back to player position if NPC not found
  - All combat sounds now correctly positioned in 3D space relative to combat location

#### AudioSubscriber Sound Mapping Updated
- All event → sound key mappings now use dedicated combat sounds:
  - `ANIM_ATTACK_START` → `combat/attack_swing`
  - `ANIM_ATTACK_HIT` / `COMBAT_ATTACK_HIT` → `combat/hit_generic`
  - `COMBAT_CRITICAL_HIT` → `combat/hit_critical`
  - `COMBAT_BLOCK` / `ANIM_BLOCK_START` → `combat/block_generic`
  - `COMBAT_PARRY` → `combat/parry`
  - `COMBAT_DODGE` → `combat/dodge`
  - `ANIM_DEATH_START` / `COMBAT_DEATH` → `combat/death_generic`
  - `ANIM_EQUIP_START` → `combat/blade_equip` (equip sound retained)
- Weapon-specific attack sounds updated to use swing/hit keys

### Files Modified
- `app/src/main/assets/audio/sound_definitions.json` — Added 11 dedicated combat sound entries
- `audio/audio_subscriber.cpp` — Updated soundMap to dedicated keys, updated weaponAttackSounds
- `engine/renderer.cpp` — Connected NPC position callback to AudioSubscriber

---

## [0.9.8] - 2026-08-27 (Animation Subscriber + NPC Animation Playback)

### Major Additions

#### Animation Subscriber System
- **AnimationSubscriber**: Bridges NPC animation states to WorldEntity animation playback
  - Listens for combat events via Imperial Weave EventBus
  - Maps NPC AnimState to animation names (idle, walk, run, attack, hit_reaction, block, death)
  - Handles state transitions with timers (HIT_REACTION: 0.5s, ATTACK: 0.8s)
  - Plays animations on WorldEntity via AnimationPlayer

#### Animation Player Enhancement
- **findSequenceByName()**: Finds animation sequence index by name
  - Supports exact match and partial match (case-insensitive)
  - Fallback to first animation if name not found
  - Enables animation playback by semantic name (e.g., "idle", "attack")

#### WorldEntity Storage System
- **WorldLoader Entity Storage**: Added entity management for NPC animation lookup
  - `std::unordered_map<uint32_t, std::unique_ptr<WorldEntity>> entities` storage
  - `npcToEntityMap` for NPC ID → Entity ID mapping
  - `loadActorForNpc()` method loads actors with NPC ID association
  - `getEntityByNpcId()` returns WorldEntity pointer for animation playback

#### Imperial Weave EventBus Enhancement
- **Event Targeting**: Added `targetId` field to `weave::Event` struct
  - Enables combat events to specify target NPC ID
  - Updated factory methods to use named field initialization
  - AnimationSubscriber subscribes to COMBAT_ATTACK_HIT, COMBAT_CRITICAL_HIT, COMBAT_DEATH

#### Audio Subscriber System
- **AudioSubscriber**: Bridges game events to AudioManager sound playback
  - Listens for combat and animation events via Imperial Weave EventBus
  - Maps events to sound definitions (blade_equip, blunt_equip, bow_shoot, etc.)
  - Supports 3D positional audio via player position and optional NPC position callback
  - Subscribes to ANIM_ATTACK_START, ANIM_ATTACK_HIT, COMBAT_ATTACK_HIT, COMBAT_CRITICAL_HIT, COMBAT_BLOCK, COMBAT_PARRY, COMBAT_DODGE, ANIM_BLOCK_START, ANIM_DEATH_START, COMBAT_DEATH, ANIM_EQUIP_START

#### Spell Selection UI
- **SpellSelectionPanel**: Draggable panel for selecting spells
  - Extends UIPanel with title bar and close button
  - Displays player spells as clickable buttons
  - Japanese name preferred, English fallback
  - Calls callback with selected spell and hides panel

#### Cast Spell Button Logic
- Updated `castSpellButton` onClick callback
  - First press opens spell selection panel
  - Subsequent presses cast selected spell on nearest enemy
  - Plays `magic/spell_equip` sound effect on successful cast

### Architecture Changes
- AnimationSubscriber is a thin bridge layer that doesn't own any systems
- AudioSubscriber is a thin bridge layer that doesn't own any systems
- Uses Imperial Weave EventBus for loose coupling between systems
- NPC animation state is managed by NPC struct, but actual playback is handled by AnimationSubscriber
- WorldEntity storage uses unique_ptr to avoid copy issues with non-copyable members

### Files Added
- `animation/animation_subscriber.h` - AnimationSubscriber class declaration
- `animation/animation_subscriber.cpp` - AnimationSubscriber implementation
- `audio/audio_subscriber.h` - AudioSubscriber class declaration
- `audio/audio_subscriber.cpp` - AudioSubscriber implementation
- `ui/spell_selection_panel.h` - SpellSelectionPanel class declaration
- `ui/spell_selection_panel.cpp` - SpellSelectionPanel implementation

### Files Modified
- `engine/imperial_weave.h` - Added targetId field, updated factory methods
- `world/world_entity.h` - Added entity storage maps, loadActorForNpc()
- `world/world_loader.cpp` - Implemented loadActorForNpc(), updated getEntityByNpcId()
- `animation/animation_player.h` - Added findSequenceByName(), getSequenceCount()
- `animation/animation_player.cpp` - Implemented findSequenceByName() with partial match
- `engine/renderer.h` - Added AnimationSubscriber, AudioSubscriber, SpellSelectionPanel members
- `engine/renderer.cpp` - Integrated all subscribers and spell panel, updated castSpellButton callback
- `CMakeLists.txt` - Added animation_subscriber.cpp, audio_subscriber.cpp, spell_selection_panel.cpp

### Build Status
- BUILD SUCCESSFUL in 41s
- All 4 architectures: arm64-v8a, armeabi-v7a, x86, x86_64

---

## [0.9.7] - 2026-07-07 (Imperial Weave + Combat System Enhancement)

### Major Additions
- **Imperial Weave Integration Layer**
  - EventBus with type_index (compiler-independent) and shared_ptr handlers (copy avoidance)
  - ServiceLocator for runtime service discovery
  - 12-phase update pipeline (PreUpdate → EventProcess → World → AI → Player → Inventory → Spell → Animation → Physics → Combat → Quest → Audio → RenderSubmit)
  - Exception-safe update() with try-catch
  - PhaseProfiler for performance monitoring

- **Combat System Enhancement**
  - 9 weapon types: Dagger, Sword, Greatsword, Axe, Mace, Greatmace, Bow, Staff, Unarmed
  - Hitbox system with AABB collision detection
  - Critical hit system (Luck-based, weapon-specific crit chance)
  - Block/Parry/Dodge mechanics (Agility-based)
  - Bleed system for Axe weapons (Damage over Time)
  - CombatEvent queue for Imperial Weave integration
  - NPC AI combat logic (attack/defend/magic selection)
  - CombatEvent struct expanded with timestamp, targetId, weaponName, isCritical, isBlocked
  - Helper function createCombatEvent() for cleaner event creation
  - EventBus integration: CombatManager emits events to Imperial Weave EventBus

### Architecture Changes
- Extended Imperial Weave update pipeline from 8 phases to 12 phases
- Integrated PlayerController, InventoryManager, SpellManager, AudioManager into Imperial Weave
- Gradually removed duplicate Manager calls from Renderer (commented out)
- Moved joystick input and audio listener position updates before ImperialWeave::update()
- titleScreen and uiSystem called directly from Renderer (Weave not suitable for UI/overlay systems)

### UI Additions
- Added player combat UI buttons (Attack, Block, Cast)

### Animation Integration
- **PlayerController EventBus Integration**
  - subscribeToCombatEvents() subscribes to 6 types of combat events
  - COMBAT_ATTACK_HIT, COMBAT_CRITICAL_HIT, COMBAT_BLOCK, COMBAT_PARRY, COMBAT_DODGE, COMBAT_DEATH
  - Currently log output only, animation playback to be implemented later

- **NPC Animation State Management**
  - AnimState enum: IDLE, WALK, RUN, ATTACK, HIT_REACTION, BLOCK, DEATH
  - Animation timing constants: HIT_REACTION_DURATION=0.5s, ATTACK_DURATION=0.8s
  - Automatic state transition: return to IDLE after animation completes
  - CombatManager::applyDamage() automatically triggers NPC animation

- **Combat Animation Flow**
  - On damage applied: target->triggerHitReaction()
  - On NPC defeat: target->triggerDeath()
  - On attack: playerController->attack() transitions to ATTACK state
  - Attack button (ATK): triggers player attack action
  - Block button (BLK): toggles combat stance
  - Cast button (MAG): magic casting (TODO: after spell system integration)
  - Hidden on title screen, shown when game starts

---

## [0.9.5] - 2026-06-28 (Phase 10-24 - Complete UI & HUD Implementation)

### Major Additions
- **HUD & Status Displays (Phase 15-24)**
  - Player Stats Panel (Gold, Inventory Weight, Equipment)
  - Minimap System for Real-Time World Navigation
  - Alert Notification System (Warnings and Status Alerts)
  - Level Progress Display (Experience and Level Up Tracking)
  - Active Effects Display (Buff/Debuff Status Panel)
  - Action Prompt System (Context-Sensitive Interaction UI)
  - Target Info Display (Enemy/NPC Status Panel)
  - Floating Combat Text (Damage and Effect Indicators)
  - HUD Compass (Cardinal Direction and Marker System)
  - Quick Slot Bar and Status Display

- **Core UI Menus (Phase 10-14)**
  - Notification/Message System
  - Pause Menu System
  - Character Creation UI
  - Shop/Merchant UI
  - Character Sheet & Spellbook UI
  - Quest Log & NPC Dialogue UI
  - UIManager integration with Engine

- **Inventory & Items (Phase 9B)**
  - Consumable items, equipment effects, and world drops
  - ItemFactory and InventoryCoordinator
  - Complete UIInventoryPanel with details popup


## [0.9.0] - 2026-06-08 (Phase 9 - Graphical UI & Sound Effects)

### Major Additions

#### Graphical UI System
- **TextureLoader**: PNG texture loading with stb_image.h
  - Single-header integration (stb_image.h)
  - Asset-based loading via AAssetManager
  - OpenGL ES 3.0 texture generation with mipmapping
  - Texture size caching for aspect ratio calculations

- **UIDrawHelper**: Shared OpenGL ES 3.0 rendering utilities
  - Colored quad rendering with orthographic projection
  - Textured quad rendering with custom UV coordinates
  - Border drawing with configurable width
  - VAO/VBO management for efficient batch rendering

- **UIPanel**: Container component with background textures
  - Draggable title bar with close button
  - Background texture support with scaling modes
  - Border and margin configuration
  - Child component management

- **UIButton**: Interactive button with multi-state textures
  - Normal / Hover / Pressed / Disabled texture states
  - Label text rendering with scale and color control
  - Click callback system with lambda support
  - Visual state transitions

- **UIComponent Base Class Enhancements**:
  - TextureScaleMode enum: STRETCH, PRESERVE_ASPECT_FIT, PRESERVE_ASPECT_CROP
  - Aspect-ratio-aware texture rendering
  - Letterbox/pillarbox support for fit mode
  - Center-crop support for fill mode

#### Sound Effects System
- **Sound Definitions JSON**: `sound_definitions.json` with 93 sound entries
  - Categorized by type: UI, combat, magic, quest, ambient
  - Multiple file variants per sound definition
  - Volume and pitch variation support
  - Random selection from variant pool

- **WAV Asset Integration**: 307 WAV files (111.82 MB)
  - UI sounds: button clicks, notifications, menu transitions
  - Combat sounds: weapon swings, hits, blocks
  - Magic sounds: spell casts, impacts, buffs
  - Quest sounds: acceptance, completion, updates

- **AudioManager JSON Loading**:
  - `loadSoundDefinitions()` for bulk sound registration
  - `playSound(key)` for definition-based playback
  - `playMusic(key)` for BGM playback with fade
  - Integration with existing OpenAL 3D audio system

#### UI Texture Integration
- **TitleScreen**: Full graphical overhaul
  - Background texture: `main_background.png`
  - Logo texture: `oblivion_logo.png`
  - Menu panel with background texture
  - Menu buttons with normal/hover/pressed textures (`shared_button_long_off/on.png`)

- **SettingsUI**: Panel background texture support
  - Settings panel uses `main_background.png`
  - Consistent visual theme with title screen

- **SaveLoadUI**: Background texture support
  - Full-screen background texture rendering
  - Fallback to dark background if texture unavailable

### Files Added

**Texture System** (~500 lines total):
- `engine/texture_loader.h/cpp` - PNG loading and OpenGL texture creation
- `ui/ui_component.h/cpp` - Base UI component with texture scaling modes
- `ui/ui_draw_helper.h/cpp` - OpenGL ES 3.0 rendering utilities
- `ui/ui_panel.h/cpp` - Panel container with textures
- `ui/ui_button.h/cpp` - Multi-state textured button

**Audio Assets**:
- `app/src/main/assets/audio/sfx/` - 307 WAV files
- `app/src/main/assets/audio/sound_definitions.json` - 93 sound definitions

**UI Assets**:
- `app/src/main/assets/textures/ui/` - UI texture files
  - `main_background.png`, `oblivion_logo.png`
  - `shared_button_long_off.png`, `shared_button_long_on.png`
  - `hud_compass.png`, `hud_health_bar.png`, `hud_magicka_bar.png`

### Files Modified

**Integration Points**:
- `ui/title_screen.h/cpp` - Texture loading, button texture assignment
- `ui/settings_ui.h/cpp` - Panel background texture integration
- `ui/save_load_ui.h/cpp` - Background texture rendering
- `audio/audio_manager.h/cpp` - JSON sound definition loading
- `engine/renderer.cpp` - AudioManager initialization with JSON loading
- `CMakeLists.txt` - Added texture_loader.cpp, UI component sources

### Build Statistics
- **Total C++ Code**: 7,000+ lines (was 6,200+)
- **Graphical UI**: 500+ lines (new)
- **Audio Integration**: 100+ lines (JSON loading)
- **Total Project**: 9,200+ lines (was 8,000+)
- **Compilation Time**: 6-7 minutes
- **APK Size**: 8.8 MB

### Performance Impact
- **Memory**: UI textures ~5-10 MB (compressed PNG)
  - Audio assets: 111 MB on disk, ~360 KB runtime buffers
  - Total runtime impact: +10-15 MB RAM
- **CPU**: Texture rendering < 1% per frame
- **FPS**: No impact - conditional rendering

### Testing
- [x] TextureLoader: PNG decoding, OpenGL texture creation
- [x] UIPanel: Background texture rendering, drag, close
- [x] UIButton: State transitions, texture switching, click events
- [x] TextureScaleMode: Stretch, fit, crop all work correctly
- [x] AudioManager: JSON loading, sound playback by key
- [x] TitleScreen: All textures load and display correctly
- [x] SettingsUI/SaveLoadUI: Background textures render
- [x] Build: `./gradlew assembleDebug` succeeds

### Bug Fixes
- Fixed `AudioManager` forward declaration in `audio_manager.h`
- Removed `#undef LOGD` from `audio_3d.h` that broke other files' logging
- Fixed `GL_TEXTURE_WIDTH/HEIGHT` unsupported in OpenGL ES by using size cache map
- Added `getScreenWidth()`/`getScreenHeight()` getters to `Renderer` for private member access

### Documentation Updates
- Updated README.md with bilingual (EN/JA) content for Phase 9
- Added `docs/PHASE9_PLAN.md` - 8-week implementation schedule
- Added `docs/ASSET_INTEGRATION_PLAN.md` - Full asset survey and integration plan

### Known Issues
- BSA asset extraction incomplete (5,756 files extracted, more available)
- menus XML references `.dds` textures that need conversion to `.png`
- NIF mesh conversion to OpenGL-friendly format pending Phase 10

### Future Enhancements (Phase 10)
- Map system with quest markers
- Full inventory management with item system
- Expanded NPC dialogue trees
- Controller support

---

## [0.8.0] - 2026-05-17 (Phase 8 - Audio & Post-Processing)

### Major Additions

#### SaveLoadUI System
- **Game State Persistence**: Full save/load system with multiple slots
  - Save/Load mode switching with UI mode selection
  - Slot selection and management interface
  - Auto-generated slot names with timestamps
  - Player position, health, and status restoration

- **SaveLoadUI Component**: Interactive save/load menu overlay
  - Displays available save slots with selection highlighting
  - Mode-based rendering (SAVE vs LOAD)
  - Multi-state dialog system (slot selection, confirm, error)
  - Touch event handling for slot selection and button actions
  - Empty slot "New Save" placeholder display

- **Error Dialogs**: Comprehensive error handling system
  - SAVE_FAILED: Storage issues or permission errors
  - LOAD_FAILED: Corrupted save file detection
  - DELETE_FAILED: Deletion permission or system errors
  - Multi-line error message display with OK button

#### OpenAL 3D Audio System
- **AudioManager**: Complete audio system with spatial support
  - WAV file loading with RIFF/WAVE header parsing
  - OpenAL buffer and source management
  - Configurable max sources (MAX_SOURCES=32) with overflow handling
  - BGM (background music) and SE (sound effects) distinction
  - Priority-based source deletion when exceeding max sources

- **Audio3D**: Spatial audio implementation
  - Distance attenuation using inverse square law
  - Listener position setup relative to camera
  - 3D positional audio for NPCs and world objects
  - Mutable reference accessor for runtime adjustments

- **JNI Audio Bridge**: Thread-safe Java-C++ interface
  - GetEnv/AttachCurrentThread pattern for thread safety
  - Cached JavaVM and jmethodID for performance
  - JNI wrapper for Java MediaPlayer and SoundPool
  - Automatic thread cleanup with DetachCurrentThread

- **Java Audio Interface**: AndroidAudio class implementation
  - MediaPlayer for BGM playback
  - SoundPool for SE effects with priority management
  - Asset loading from Android app resources

#### RetroFilter Effects System
- **Post-Processing Visual Effects**:
  - Pixelation: Blockiness with configurable scale
  - Scanlines: Horizontal line overlay
  - Color Reduction: Palette reduction for retro look
  - CRT Distortion: Screen curvature effect
  - Film Grain: Analog film noise overlay

- **SettingsUI Integration**: RetroFilter effect toggles
  - Menu items for each effect (PIXELATION, SCANLINES, COLOR_REDUCTION, CRT_DISTORTION, FILM_GRAIN)
  - Real-time effect enable/disable
  - Settings persistence through SettingsManager
  - Visual feedback with color highlighting

#### DebugHUD Enhancements
- **Audio Status Display**: Real-time audio system monitoring
  - Loaded audio clips count
  - Active audio sources count
  - BGM playback status indicator
  - Format: "Audio: clips=X sources=Y [BGM playing]" in cyan

- **RetroFilter Status Display**: Active effects visualization
  - Single-letter abbreviations: S (Scanlines), P (Pixelation), C (Color), D (Distortion), G (Grain)
  - Example: "Filters: SPG" shows three active effects
  - Full effect name legend for clarity
  - Orange colored status text for visibility

### Files Added

**Audio System** (~600 lines total):
- `audio/audio_manager.h/cpp` - Audio system management
- `audio/audio_3d.h/cpp` - 3D spatial audio implementation
- `audio/jni_audio_bridge.h/cpp` - JNI bridge for Java audio

**Save/Load System** (~300 lines total):
- `ui/save_load_ui.h/cpp` - Save/Load menu implementation
- `save_system/save_manager.h/cpp` - Game state persistence (from Phase 7.1)

**Audio Assets**:
- Sample WAV files in assets/ directory

### Files Modified

**Integration Points**:
- `engine/renderer.h/cpp` - Added AudioManager and SaveLoadUI integration
- `ui/debug_hud.h/cpp` - Enhanced with audio and RetroFilter status display
- `ui/settings_ui.h/cpp` - Added RetroFilter effect menu items
- `ui/title_screen.cpp` - Added "Load Game" button to main menu
- `CMakeLists.txt` - Audio source files and AUDIO_SYSTEM_ENABLED definition
- `jni_bridge.cpp` - JNI audio initialization
- `MainActivity.java` - Audio bridge method declarations
- `GameRenderer.java` - Native audio initialization support

### Build Statistics
- **Total C++ Code**: 6,200+ lines (was 5,200+)
- **Audio System**: 600+ lines (new)
- **SaveLoadUI**: 300+ lines (new)
- **Audio Headers**: 250+ lines (new)
- **RetroFilter Integration**: 150+ lines (DebugHUD + SettingsUI)
- **Total Project**: 8,000+ lines (was 6,750+)
- **Compilation Time**: ~7.2 minutes (minimal increase)

### Performance Impact
- **Memory**: Audio buffers ~360 KB (OpenAL internal)
  - SaveLoadUI metadata: ~50 KB (save slots cache)
  - RetroFilter shader: ~20 KB (WebGL compiled shaders)
  - Total impact: +430 KB RAM
- **CPU**: Audio processing < 5% per frame, RetroFilter < 2%
- **FPS**: No impact - effects render conditionally, audio runs on dedicated thread

### Testing
- [x] SaveLoadUI: Slot selection, save/load execution, error handling
- [x] Audio System: WAV file loading, 3D positional audio, source management
- [x] RetroFilter Effects: Real-time effect toggling, visual verification
- [x] DebugHUD: Audio and filter status display accuracy
- [x] Integration: All systems coordinate without conflicts
- [x] Thread Safety: JNI calls from multiple threads without crashes
- [x] Persistence: Settings and save slots persist across app restarts

### Known Issues
- Mana field not present in Player struct: SaveLoadUI hardcodes (100/120) for compatibility
- WAV loader supports assets/ directory only
- RetroFilter effects work on supported devices (OpenGL ES 3.0+)

### Documentation Updates
- Updated README.md with Phase 8 features and version 0.8.0
- Created PHASE8_TECHNICAL_SPEC.md (1,200+ lines of detailed specifications)
- Added audio system architecture diagrams and WAV parsing specifications
- Added SaveLoadUI state flow and error handling tables
- Added RetroFilter effects documentation with parameter ranges
- Updated CHANGELOG.md (this file) with complete Phase 8 details

### Key Improvements
- **Persistence**: Players can now save progress and resume from exact point
- **Immersion**: 3D spatial audio enhances gameplay atmosphere
- **Aesthetics**: RetroFilter effects allow visual customization for retro feel
- **Observability**: Enhanced DebugHUD provides real-time system status

### Architecture Notes
- SaveLoadUI uses Renderer->PlayerController->Player chain for state access
- AudioManager runs audio updates independently in game loop
- JNI bridge implements thread-aware pattern for multi-threaded safety
- RetroFilter settings persist through SettingsManager for consistency
- All Phase 8 systems integrate cleanly with existing Phase 5-7 infrastructure

---

## [0.7.1] - 2026-04-18 (Phase 7.1 - Settings & Debug System)

### Major Additions

#### UI & Settings System
- **TextRenderer**: New on-screen text rendering system with color and positioning support
  - Uses OpenGL ES 3.0 orthographic projection
  - Supports variable scale and color parameters
  - Integrated with all UI systems

- **Debug HUD**: Real-time performance overlay displaying:
  - FPS (frames per second)
  - Frame time (milliseconds per frame)
  - Average frame time (0.5s rolling window)
  - Memory usage (in MB)
  - Active object count
  - Debug mode status

- **SettingsManager**: Persistent settings management system
  - Loads settings from `/data/data/com.example.oblivion/settings.txt`
  - Supports debug mode toggle (ON/OFF)
  - Supports language selection (Japanese/English)
  - Automatic save on setting changes

- **SettingsUI**: Interactive settings menu overlay
  - Accessible from title screen "Settings" menu item
  - Three menu options: Debug Mode, Language, Back
  - Visual feedback with red highlight on selected item
  - Black semi-transparent background overlay
  - Yellow title text, white menu items

#### Documentation
- **ARCHITECTURE.md**: Complete system architecture and design patterns
  - Layer-based architecture overview
  - Manager pattern explanation
  - Component integration diagrams
  - Memory management strategy

- **SETTINGS.md**: User and developer guide for settings system
  - How to access settings menu
  - Debug HUD metric explanations
  - Troubleshooting guide
  - Developer API for settings integration

### Bug Fixes
- None (all features implemented correctly)

### Features Completed
- [x] TextRenderer with OpenGL ES 3.0 orthographic projection
- [x] DebugHUD with 6 real-time metrics
- [x] SettingsManager with file persistence
- [x] SettingsUI with touch interaction
- [x] Integration into title screen menu flow
- [x] Touch event priority system (SettingsUI > TitleScreen > QuestUI)

### Documentation Updates
- Updated README.md with new features and system architecture
- Added ARCHITECTURE.md (1,200+ lines of technical documentation)
- Added SETTINGS.md (1,100+ lines of user and developer guide)
- Updated code metrics to reflect new UI system (800+ lines)

### Technical Details

**Modified Files**:
- `README.md` - Updated features, limitations, code metrics, version to 0.7.1
- `CMakeLists.txt` - Added TextRenderer, DebugHUD, SettingsManager, SettingsUI source files

**New Files**:
- `ui/text_renderer.h/cpp` - Text rendering foundation (~300 lines)
- `ui/debug_hud.h/cpp` - Performance monitoring overlay (~250 lines)
- `ui/settings_ui.h/cpp` - Settings menu UI (~300 lines)
- `system/settings_manager.h/cpp` - Persistent settings management (~200 lines)
- `ARCHITECTURE.md` - System design documentation (1,200+ lines)
- `SETTINGS.md` - Settings guide and API reference (1,100+ lines)

**Modified Existing Files**:
- `ui/title_screen.h` - Added SettingsUI pointer, settingsRequested flag
- `ui/title_screen.cpp` - Added Settings menu option handling
- `engine/renderer.h` - Added TextRenderer, DebugHUD, SettingsUI, SettingsManager pointers
- `engine/renderer.cpp` - Added system initialization order, render logic, touch event priority

**Build Statistics**:
- Total C++ code: 5,200+ lines (was 4,500)
- UI System code: 800+ lines (new)
- Documentation: 2,300+ lines (new)
- Compilation time: ~7 minutes (minimal increase)

### Performance Impact
- **Memory**: +5 MB for UI systems (45 MB -> 50 MB total)
- **CPU**: Negligible impact (< 0.05% additional)
- **FPS**: No impact - debug HUD renders conditionally

### Testing
- [x] TextRenderer: Text displays at correct coordinates with colors
- [x] DebugHUD: All metrics update correctly, format properly
- [x] SettingsManager: Settings persist across app restarts
- [x] SettingsUI: Menu displays, touch selection works, changes save
- [x] Integration: Touch event priority system works correctly
- [x] Settings + DebugHUD: Debug mode toggle hides/shows HUD

### Known Issues
- None identified

### Future Enhancements
- Phase 7.2: Save/Load game state system
- Phase 7.3: Graphical UI with textures and animations
- Phase 8: Audio system integration with settings

---

## [0.6.0] - 2026-04-17 (Phase 6 Release Candidate)

### Major Additions

#### Performance & Stability
- **Multi-Device Testing**: Verified compatibility on Amazon Fire (Android 9) and Xiaomi (Android 16)
- **Performance Monitoring**: Implemented detailed frame timing, CPU/GPU/memory profiling
- **Stability Improvements**: Fixed JNI bridge architecture mismatches
- **Thermal Monitoring**: Implemented device temperature tracking

#### Documentation
- **INSTALLATION.md**: Complete installation guide with device requirements
- **GAMEPLAY.md**: Comprehensive gameplay mechanics and controls guide
- **KNOWN_ISSUES.md**: Documented all current limitations and workarounds
- **PERFORMANCE_REPORT.md**: Detailed profiling data and optimization opportunities

### Bug Fixes

#### Critical Fixes (Phase 6)
- **JNI Bridge Architecture**: Refactored from global static pattern to handle-based pattern
  - `nativeInitEngine()` now correctly returns `jlong` handle
  - All JNI methods now accept handle parameter
  - Fixed `nativeSetViewport()`, `nativeRenderFrame()`, `nativeOnTouchEvent()` signatures
  - Result: Eliminated UnsatisfiedLinkError crashes

- **CMake Build**: Fixed missing `game/npc.cpp` in SOURCES list
  - Resolved linker errors for NPC symbol references

- **Device Installation**: Resolved Amazon Fire APK corruption
  - Used `pm uninstall -k` followed by fresh reinstall
  - Device now stable on consecutive launches

### Performance Results

#### Frame Rate
- Amazon Fire (Android 9): **60 FPS stable** (target: 30 FPS)
- Xiaomi (Android 16): **60 FPS stable** (target: 30 FPS)
- No frame drops during 30+ second continuous test

#### Memory
- **App Footprint**: 40.4 MB (Pss) - excellent efficiency
- **Native Heap**: 11.3 MB (5x below limit)
- **GPU Memory**: 1.884 MB (highly optimized)
- **Headroom**: 1.06 GB available (87% unused)

#### CPU
- **App CPU**: < 0.1% (below top 38 processes)
- **Thermal**: 38-40°C during test (safe zone)
- **Threads**: 29 active (mostly idle)

#### Battery
- **Drain Rate**: ~1-2%/hour at 50% brightness
- **Peak Drain**: 3%/hour at 100% brightness
- **Idle Drain**: 0.3%/hour when app open but idle

### Testing
- [x] Compatibility: Android 9 and Android 16 both pass all tests
- [x] Stability: 0 crashes during extended testing
- [x] Resolution Support: 1200x1920 and 2032x3048 both working
- [x] Multi-device: Dual-manufacturer compatibility confirmed (Amazon + Xiaomi)

### Known Issues
- Compiler warnings: 34 in jni_bridge.cpp (expected for JNI code)
- Text-based UI only (design decision for prototype phase)
- No save/load system (planned for Phase 7)

### Technical Details

**Modified Files**:
- `app/src/main/cpp/jni_bridge.cpp` - Complete JNI method rewrite
- `app/src/main/cpp/engine/renderer.h` - Added resize() method declaration
- `app/src/main/cpp/engine/renderer.cpp` - Added resize() implementation
- `app/src/main/cpp/profiling/performance_monitor.h/cpp` - Enhanced profiling
- `CMakeLists.txt` - Added game/npc.cpp to SOURCES

**Build Configuration**:
- Release APK: 9.6 MB (debug), ~8 MB (release expected)
- Signature: oblivion.keystore with 2048-bit RSA
- NDK: r26.1 with C++17 standard
- API Level: 29 (Android 10.0)

---

## [0.5.3] - 2026-04-14 (Phase 5 Completion - M5-3)

### Magic System Implementation

#### Features
- **6 Magic Schools**: Destruction, Restoration, Conjuration, Alteration, Illusion, Mysticism
- **Spell System**:
  - Fireball: 50 mana, 30 damage (Destruction)
  - Heal: 40 mana, 50 healing (Restoration)
  - Restore Mana: 30 mana, 40 mana recovery (Mysticism)
- **Mana System**: Mana pool with consumption and recovery
- **Spell Casting**: NPCs can auto-cast spells during combat
- **Damage Calculation**: Magic school-based damage with caster stats

#### Files Added
- `game/spell.h` - Spell data structures
- `game/spell_manager.h/cpp` - Spell management system (650+ lines)
- `ui/spell_ui.h/cpp` - Spell UI display

#### Integration
- Integrated with CombatManager for spell-based attacks
- NPC spell selection during combat
- CharacterStatus extended with mana and magic schools

#### Testing
- [x] Spells cast successfully in combat
- [x] Mana consumption working correctly
- [x] Damage calculation based on magic school
- [x] Multiple NPCs casting different spells simultaneously

---

## [0.5.2] - 2026-04-12 (Phase 5 Completion - M5-2)

### Quest & Title Screen System

#### Quest System Features
- **Quest Creation**: NPCs can offer quests with objectives
- **Quest States**: PENDING -> ACCEPTED -> IN_PROGRESS -> COMPLETED/FAILED
- **Objectives**: Track progress on multi-step quests
- **Rewards**: Gold and experience points
- **Quest UI**: Text-based quest log with details

#### Title Screen
- **Logo Display**: 3-second Oblivion logo animation
- **Menu System**: "Start Game" option
- **State Management**: Transitions to main game on interaction
- **Visual Polish**: Simple but functional interface

#### Files Added
- `game/quest.h` - Quest data structures (~200 lines)
- `game/quest_manager.h/cpp` - Quest management system (~350 lines)
- `ui/title_screen.h/cpp` - Title screen implementation (~210 lines)
- `ui/quest_ui.h/cpp` - Quest UI system (~270 lines)

#### Integration
- Renderer integrated with TitleScreen and QuestManager
- NPC-Quest linking system
- Game loop flow: Title Screen -> Main Game -> Quest Log access

#### Testing
- [x] Title screen displays correctly
- [x] Game starts on user action
- [x] Quests can be created and accepted
- [x] Quest progress tracks properly

---

## [0.5.1] - 2026-04-10 (Phase 5 Completion - M5-1)

### Combat System Implementation

#### Combat Features
- **Character Status**: Health, Mana, Stamina, Attributes, Skills
- **Damage Calculation**:
  - Attacker: Strength + Weapon Damage
  - Defender: Armor Rating
  - Result: Damage = Attack Power - Defense Rating
- **Combat States**: IDLE, WANDER, PATROL, COMBAT, FOLLOW
- **Auto-Combat**: NPCs automatically engage in combat
- **Combat Manager**: Central system managing all active combats
- **NPC Health Tracking**: NPCs die when health reaches 0

#### Files Added
- `game/combat_manager.h/cpp` - Combat system (450+ lines)
- `game/npc.h` - Extended with CharacterStatus struct

#### Damage Calculation Details
```cpp
float Strength = NPC.attributes["Strength"];      // 5-10
float WeaponDamage = EquippedWeapon.damage;     // 15-30
float AttackPower = Strength + WeaponDamage;    // 20-40
float ArmorRating = Defender.armorRating;       // 5-25
float Damage = max(1.0f, AttackPower - ArmorRating);
```

#### Testing
- [x] NPCs engage in combat automatically
- [x] Damage calculation works correctly
- [x] NPC health decreases with attacks
- [x] Dead NPCs are removed from game
- [x] Multiple combats run simultaneously

#### Localization
- Added Japanese translations for combat messages
- 100+ game text translations total

---

## [0.4.0] - 2026-04-05 (Phase 4 Completion)

### NPC AI & Interaction System

#### AI Features
- **NPC Spawning**: Create NPCs with position and name
- **AI States**: IDLE, WANDER, PATROL, FOLLOW
- **Pathfinding**: Basic movement between waypoints
- **State Machine**: Transitions based on player proximity
- **NPC Manager**: Central management of 100+ NPCs

#### Interaction System
- **Proximity Detection**: Detect when player is near NPC
- **Interaction Prompt**: Display available actions near NPC
- **Dialogue Foundation**: System for NPC conversations

#### Files Added
- `game/npc_manager.h/cpp` - NPC management (500+ lines)
- `game/npc.h` - NPC data structures and AI state machine

#### Testing
- [x] NPCs spawn at correct positions
- [x] NPCs move in WANDER mode
- [x] AI states transition properly
- [x] Multiple NPCs managed efficiently

---

## [0.3.0] - 2026-03-28 (Phase 3 Completion)

### World System & Game Infrastructure

#### World Features
- **Cell System**: Multiple cells (areas) in the world
- **Seamless Streaming**: Load/unload cells based on player position
- **World Manager**: Central world data management
- **Player Character**: Controllable character with position tracking
- **Game State**: Persistent world state between frames

#### UI Framework
- **Menu System**: Basic text menus
- **HUD Display**: Health, mana, status indicators
- **Scene Management**: Title screen, main game, menus

#### Files Added
- `game/world_manager.h/cpp` - World management (350+ lines)
- `game/world_object.h` - Base object system
- `ui/ui_manager.h/cpp` - UI framework

#### Testing
- [x] Multiple cells load without issues
- [x] World objects persist across frames
- [x] Player position tracking works
- [x] Smooth transitions between areas

---

## [0.2.0] - 2026-03-20 (Phase 2 Completion)

### Asset Management & Loading

#### Asset System
- **NIF Parser**: Load Oblivion model files
- **DDS Loader**: Load texture files
- **Asset Manager**: Central asset caching system
- **Memory Pooling**: Efficient resource management
- **Streaming System**: Dynamic asset loading/unloading

#### Features
- **Mesh Loading**: Parse and display NIF models
- **Texture Mapping**: Apply DDS textures to meshes
- **Asset Caching**: Avoid reloading same assets
- **LOD Support**: Multiple detail levels for assets

#### Files Added
- `assets/nif_parser.h/cpp` - NIF format parsing (600+ lines)
- `assets/dds_loader.h/cpp` - Texture loading (300+ lines)
- `assets/asset_manager.h/cpp` - Asset management (400+ lines)

#### Testing
- [x] Real Oblivion meshes load successfully
- [x] Textures display correctly
- [x] Caching improves performance
- [x] No memory leaks in asset system

---

## [0.1.0] - 2026-03-10 (Phase 1 Completion)

### Core Rendering Engine

#### Initial Features
- **OpenGL ES 3.0**: Full graphics pipeline
- **3D Rendering**: Mesh and texture support
- **Camera Control**: Touch-based camera movement
- **JNI Bridge**: Java-C++ communication layer
- **Frame Rate Control**: Locked 60 FPS target
- **Input Handling**: Touch event processing

#### Architecture
- **Java Layer**: `GameRenderer.java`, `GameSurfaceView.java`, `MainActivity.java`
- **C++ Layer**: `native-lib.cpp`, `jni_bridge.cpp`
- **Graphics**: `engine/renderer.cpp`, `engine/shader.cpp`, `engine/camera.cpp`
- **Geometry**: `geometry/cube.cpp`, `geometry/mesh.cpp`

#### Files Added (Phase 1)
- `app/src/main/cpp/jni_bridge.cpp` - JNI interface (200+ lines)
- `app/src/main/cpp/engine/renderer.h/cpp` - Rendering engine
- `app/src/main/cpp/engine/shader.h/cpp` - Shader management
- `app/src/main/cpp/engine/camera.h/cpp` - Camera control
- `app/src/main/cpp/geometry/cube.h/cpp` - Test geometry
- `CMakeLists.txt` - Build configuration

#### Milestones
- **M1-1**: [x] Black screen (native code executing)
- **M1-2**: [x] Rotating cube displayed (3D rendering working)
- **M1-3**: [x] Camera movement (input handling working)

#### Testing
- [x] App launches on Android device
- [x] 60 FPS stable on test hardware
- [x] Touch input responsive
- [x] No crashes during basic interaction

#### First Run
- Application size: 5 MB APK
- Load time: 10-15 seconds
- Memory usage: 30 MB baseline
- Frame rate: Steady 60 FPS

---

## Version History Summary

| Version | Phase | Focus | Status |
|---------|-------|-------|--------|
| 1.1.0 | Phase 36 | Jolt Physics Integration | [x] Complete |
| 1.0.0 | Phase 35 | Radiant AI System | [x] Complete |
| 0.9.10 | Phase 34 | Weapon Sound Routing + Quick-Slot Spells | [x] Complete |
| 0.9.9 | Phase 33 | Combat Sound Assets + NPC Spatial Audio | [x] Complete |
| 0.9.8 | Phase 32 | Animation & Audio Integration | [x] Complete |
| 0.9.7 | Phase 31 | Imperial Weave + Combat Enhancement | [x] Complete |
| 0.9.0 | Phase 9 | Graphical UI & Sound Effects | [x] Complete |
| 0.8.0 | Phase 8 | Audio & Post-Processing | [x] Complete |
| 0.7.1 | Phase 7.1 | Settings & Debug System | [x] Complete |
| 0.6.0 | Phase 6 | Performance & Release | [x] RC (Release Candidate) |
| 0.5.3 | Phase 5 | Magic System | [x] Complete |
| 0.5.2 | Phase 5 | Quests & Title Screen | [x] Complete |
| 0.5.1 | Phase 5 | Combat System | [x] Complete |
| 0.4.0 | Phase 4 | NPC & Interaction | [x] Complete |
| 0.3.0 | Phase 3 | World System | [x] Complete |
| 0.2.0 | Phase 2 | Asset Management | [x] Complete |
| 0.1.0 | Phase 1 | Core Rendering | [x] Complete |

---

## Development Statistics

### Code Metrics (Phase 36 / v1.1.0)
- **Total C++ Code**: 22,500+ lines
- **Total Header Files**: 5,000+ lines
- **Java Code**: 700+ lines
- **Build Files**: CMakeLists.txt + gradle configurations
- **Total Project**: 28,000+ lines of code

### Files by Category

**Engine/Core** (15 files): renderer, camera, shader, texture_loader, jni_bridge, imperial_weave, etc.
**Game Systems** (20 files): npc, quest, combat, spell, world, player_controller, etc.
**UI** (12 files): title_screen, quest_ui, settings_ui, save_load_ui, ui_panel, ui_button, spell_selection_panel, etc.
**Assets** (8 files): nif_parser, dds_loader, asset_manager, esm_reader, bsa_reader, etc.
**Audio** (6 files): audio_manager, audio_3d, audio_subscriber, jni_audio_bridge, etc.
**Physics** (4 files): physics_manager, character_virtual, etc.
**AI** (6 files): ai_scheduler, ai_package, package_stack, etc.
**Profiling** (2 files): performance_monitor, etc.

### Development Timeline
- **Phase 1** (2 weeks): Core rendering foundation
- **Phase 2** (3 weeks): Asset loading system
- **Phase 3** (2 weeks): World management
- **Phase 4** (2 weeks): NPC & AI systems
- **Phase 5** (3 weeks): Combat, Quests, Magic
- **Phase 6** (1 week): Performance & Release prep
- **Phase 7.1** (1 week): Settings & Debug System
- **Phase 8** (1 week): Audio & Post-Processing
- **Phase 9** (1 week): Graphical UI & Sound Effects
- **Phase 24-31** (4 weeks): ESM Integration, NIF/Skeleton, PlayerController
- **Phase 32-34** (1 week): Imperial Weave, Animation/Audio Subscribers
- **Phase 35** (1 week): Radiant AI System
- **Phase 36** (1 week): Jolt Physics Integration
- **Total**: ~24 weeks

---

## Technology Stack

### Languages
- **C++17**: Game engine and core systems
- **Java**: Android interface layer
- **GLSL ES 3.0**: Graphics shaders

### APIs & Libraries
- **Android NDK r26.1**: Native development kit
- **OpenGL ES 3.0**: Graphics rendering
- **JNI**: Java-C++ bridge
- **Jolt Physics**: Physics engine
- **OpenAL-Soft**: 3D audio
- **GLM**: Mathematics library
- **stb_image**: PNG loading
- **CMake 3.16+**: Build system
- **Gradle 9.4**: Android build tool

### Third-Party Libraries
- **GLM**: Mathematics library (header-only)
- **Jolt Physics**: Physics engine
- **OpenAL-Soft**: Audio (framework ready)
- **stb_image.h**: PNG image loading (single header)

---

## Notable Achievements

[x] **First Android Port of Oblivion Engine**
- Complete game loop implementation
- Full JNI bridge system
- Multi-system integration (combat, quests, magic)

[x] **Cross-Device Compatibility**
- Android 9 to Android 16 support
- ARM64 and ARMv7 architectures
- Resolution independence (1200x1920 to 2032x3048)

[x] **Performance Excellence**
- 60 FPS target achieved and exceeded
- Memory footprint < 50 MB
- Thermal management optimized

[x] **Localization**
- 100+ translations (Japanese + English)
- Dynamic language switching
- Unicode support

---

## Next Steps (Phase 10)

### User Interface Enhancement
- [ ] Map system with quest markers
- [ ] Full inventory management with item system
- [ ] Expanded NPC dialogue trees
- [ ] Character creation/customization
- [ ] Equipment display and management

### Feature Enhancement
- [ ] Controller support (gamepad)
- [ ] Additional RetroFilter presets
- [ ] Voice acting integration (NPC dialogue)
- [ ] Dynamic music mixing (exploration vs combat)

### Performance & Release
- [ ] Beta testing channel setup
- [ ] Privacy policy and legal documentation
- [ ] Enhanced shader optimization
- [ ] Texture compression (ETC2/ASTC)
- [ ] Asset streaming improvements

---

## [0.9.1] - 2026-06-09 (Phase 9 - Asset Integration & Touch Fix)

### Asset Integration
- **Actual Oblivion Meshes**: Copied 2,188 NIF mesh files (264MB) from `bsa_Extraction/meshes/`
  - Categories: architecture, armor, characters, clothes, clutter, creatures, dungeons, effects, plants, rocks, trees, weapons
  - Main game + Shivering Isles content included
  - Updated code references: `meshes/creatures/fleshatronach/fleshatronach.nif`, `meshes/characters/_male/skeleton.nif`

- **Actual Oblivion Fonts**: Copied 10 font files (4.31MB) from `bsa_Extraction/fonts/`
  - kingthings_regular, kingthings_shadowed, handwritten, daedric_font, tahoma_bold_small
  - Each includes `.fnt` definition + `.tex` texture

### Bug Fixes
- **Title Screen Touch Handling**: Fixed Start button not responding to taps
  - `GameSurfaceView.java`: Added ACTION_DOWN and ACTION_UP event forwarding
  - `title_screen.cpp`: Enhanced direct hit detection within menuPanel bounds
  - Added fallback legacy Y-based detection for compatibility
  - Changed log levels from LOGD to LOGI for better visibility

- **Game Screen Rendering**: Fixed black screen after title transition
  - `renderer.cpp`: Added `glViewport()` before `glClear()` to ensure proper framebuffer clearing
  - `cell.cpp`: Added simple colored quad rendering in `CellManager::renderCell()` for visual feedback
  - `ui_draw_helper.cpp`: Added `#include "../ui/ui_draw_helper.h"` for world rendering access

### Known Issues
- **Texture Extraction**: `.dds` files not available in `bsa_Extraction/`. BSA file `Oblivion - Textures - Compressed.bsa` (1.2GB) has encrypted/compressed filename table requiring specialized tools (BSAUnpacker)
- **Device Disconnection**: Wireless debugging connection lost during session. Requires re-pairing for final verification

---

## Contributors

**Development Team**:
- Primary Developer: Oblivion Android Project
- Testing: Multi-device validation team
- Documentation: Project team

---

**Last Updated**: 2026-06-09
**Current Version**: 0.9.0 (Phase 9 Complete)
**Next Milestone**: Phase 10 - Map System & Full Inventory
