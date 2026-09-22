# Oblivion Android Implementation Plan

## Phase 36: Virtual Controller Integration

### Completed
- [x] Virtual Controller implementation (virtual_controller.h/cpp)
- [x] renderer.cpp/h に VirtualController 統合（タッチイベント転送、入力処理）
- [x] VirtualController の入力を PlayerController に接続

### Next Steps

#### 1. HUD追加
- [x] Health/Magicka/Stamina bars (HUDRenderer + UIHudStatusDisplay)
- [x] Compass (UIHudCompass)
- [x] Crosshair (Crosshair)
- [x] Enemy health bars (UITargetInfo)
- [x] Active effects (UIActiveEffects)

#### 2. BSAアセット統合
- [x] BSA file parser (BSArchive)
- [x] Texture extraction (AssetManager)
- [x] Mesh extraction (AssetManager)
- [x] Sound extraction (AssetManager)

#### 3. ゲームシステム完成
- [x] Combat system refinement (CombatManager)
- [x] Magic system (SpellManager)
- [x] Inventory system (InventoryManager)
- [x] Dialogue system (DialogueManager)

---

## Phase 37-45: Completed

All phases 37-45 have been completed:
- Phase 37: DDS Texture Expansion (DXT1/DXT3/DXT5)
- Phase 38: World Geometry Rendering
- Phase 39: Script VM Game Integration
- Phase 40: NPC AI Polish
- Phase 41: Character Creation Complete
- Phase 42: Shop/Trade System
- Phase 43: Menu/Flow Integration
- Phase 44: LOD/Streaming Optimization
- Phase 45: Localization/Persistence

---

## Phase 46-55: Completion Plan

**Project**: HHKK0127/Oblivion_Android
**Current Version**: 0.9.10 (Phase 45 Complete)
**Goal**: Complete Oblivion gameplay experience on Android

### Unimplemented Features Identified
- renderer.cpp: Weather, Time, Equip/Unequip, Magicka, Combat debug (11 TODOs)
- audio_decoder.cpp: MP3/OGG decode (header parsing only)
- Other: Icon mapping, Gender selection, Texture loading

### Phase Schedule

| Phase | Feature | Priority | Estimate | Dependencies | Status |
|-------|---------|----------|----------|--------------|--------|
| **Phase 46** | Physics System Integration | Critical | 5-7 days | None | Complete |
| **Phase 47** | Weather & Sky Rendering | High | 3-4 days | Phase 46 | Complete |
| **Phase 48** | Equipment & Stats System | High | 4-5 days | Phase 46 | Complete |
| **Phase 49** | Audio Decode Completion (MP3/OGG) | High | 3-4 days | None | Complete |
| **Phase 50** | Crime & Reputation System | Medium | 3-4 days | Phase 46, 48 | Complete |
| **Phase 51** | Lockpicking & Minigames | Medium | 2-3 days | Phase 48 | Complete |
| **Phase 52** | Alchemy & Enchanting | Medium | 3-4 days | Phase 48 | Complete |
| **Phase 53** | Dialogue & Persuasion | Medium | 2-3 days | Phase 50 | Complete |
| **Phase 54** | World Map & Fast Travel | Low | 3-4 days | Phase 47 | Complete |
| **Phase 55** | Final Polish & Optimization | Low | 3-5 days | All Phases | Complete |

**Total Estimate**: 31-44 days

### Phase 56-59: Future Expansion

| Phase | Feature | Priority | Estimate | Dependencies | Status |
|-------|---------|----------|----------|--------------|--------|
| **Phase 56** | Quest Map with Markers | High | 3-4 days | Phase 47 | Complete |
| **Phase 57** | ESM Render Verification | High | 2-3 days | Phase 38 | Complete |
| **Phase 58** | Controller Support | Medium | 3-4 days | Phase 36 | Complete |
| **Phase 59** | SpeedTree Vegetation | Low | 4-5 days | Phase 38 | Complete |

### Phase 60-61: Runtime Integration

| Phase | Feature | Priority | Estimate | Dependencies | Status |
|-------|---------|----------|----------|--------------|--------|
| **Phase 60** | ScriptVM Runtime Integration | High | 2-3 days | Phase 37 | Complete |
| **Phase 61** | SpeedTree Renderer Verification | Medium | 1-2 days | Phase 59 | Complete |

### Milestones

| Milestone | Phase | Goal | Success Criteria |
|-----------|-------|------|------------------|
| **M7: Core Gameplay** | 46-48 | Physics/Weather/Equipment | Physics working, weather visible, equipment functional |
| **M8: Immersion** | 49-50 | Audio/Crime | MP3/OGG playback, crime system active |
| **M9: Minigames** | 51-52 | Lockpicking/Alchemy | Minigames playable |
| **M10: Complete** | 53-55 | Final features | Full Oblivion experience |

### Parallel Work Possible
- Phase 46 (Physics) + Phase 49 (Audio)
- Phase 47 (Weather) + Phase 48 (Equipment)
- Phase 51 (Lockpick) + Phase 52 (Alchemy)

### Risk Factors
1. DDS decode compatibility - all Oblivion texture formats must be supported
2. Script VM complexity - 1000+ script functions need integration
3. Memory management - large world streaming may cause memory exhaustion
4. Android device compatibility - OpenGL ES 3.0 implementation differences

### Quality Assurance
- Unit tests at each phase completion
- Device testing (Android 9+)
- Performance profiling

---

*Plan created: 2026-09-17*
*Next update: After Phase 46 completion*

---

## Phase 62-72: Roadmap to Product Completion (Full Game Scope)

**Scope decision**: Full game reproduction - all quests, all DLC, full Script VM compatibility, mod support.
**Basis**: All estimates below are derived from measured data (see "Measured Content Inventory"), not guesses.

### Measured Content Inventory

| Item | Measured value |
|------|----------------|
| Oblivion.esm | 277,504,985 B (264.65 MiB) / 1,167,017 records / 85,079 GRUPs / max depth 5 / 64 record types / 0 errors / 100% coverage |
| DLC ESP files | 9 files / 23,835 records (Knights.esp largest at 9,563) |
| DLCShiveringIsles.esp | 85-byte stub (header record only) |
| Shivering Isles content | Merged into Oblivion.esm (Steam GOTY build) - confirmed via SEDementia 118 / Bliss 177 / Crucible 194 inside the ESM |
| BSA archives | 17 files / 5.47 GB / nif 9,645 - dds 21,856 - kf 2,687 - mp3 50,943 - lip 50,916 - lod 10,810 - wav 2,093 - xml 759 - egm 155 - fnt 7 - tex 38 - scc 8 |
| Total content to support | REFR 1,044,109 / CELL 35,787 / QUST 416 / SCPT 3,046 / NPC_ 2,664 / INFO 20,333 / CREA 1,001 / WRLD 103 / LAND 31,927 / PACK 7,717 / DIAL 4,126 / SPEL 1,314 |

**Binary format (confirmed)**: Oblivion (TES4) uses a 20-byte record header, a 20-byte GRUP header, and 6-byte subrecord headers.
`esm_reader.cpp` already implements this correctly (L116 "TES4 record: 20-byte header", L183, L471 `size & 0xFFFF` with `offset += 6`).

### Implementation Coverage (measured)

| Area | Current | Required | Gap |
|------|---------|----------|-----|
| ESM binary format | Correct | - | None |
| Decoded record types | 65 | 64 | None (all 20 secondary types decoded and verified against real data) |
| Script functions | 118 | ~1,200 | ~1,080 functions |
| Mod / load order | None (single file parse only) | plugins.txt, override resolution, BSA priority | Design from scratch |
| Tests | 5 suites exist, `runAllTests()` never called | CI green | Not wired |
| Performance | 30-33 fps gameplay / 60 fps title (measured on the emulator, hardware GL, x86_64) | 30 fps stable | Met on the emulator; A4 needs a real-device run |
| Memory | 626 MB native heap / 670 MB PSS (measured) | A3/A4 budget 1 GB | Thin margin - see below |

Note: ACHR/ACRE are decoded and 105 exterior actors are placed from real data (449,824 interior references are deliberately skipped); the renderer still also spawns hardcoded demo NPCs (Izar/Hellas, renderer.cpp L2447-2448) as a fallback.

### Acceptance Criteria (product level)

| # | Criterion |
|---|-----------|
| A1 | Real Oblivion.esm + all DLC + BSAs load and all 35,787 cells render without corruption |
| A2 | Stable 30 fps (title / interior / exterior / combat) |
| A3 | Crash-free rate >= 99.5% over 1,000 sessions |
| A4 | 2-hour continuous play with no memory growth (no leaks) |
| A5 | Script VM implements ~1,200 functions and all 416 QUST records progress |
| A6 | CI green (5 existing suites wired plus real-data regression tests) |
| A7 | Legal compliance: BYO-data model enforced, zero decompiled-derived code |
| A8 | First-run UX: data install to gameplay in under 5 minutes |

### Phase Schedule

| Phase | Feature | Estimate | Dependencies | Status |
|-------|---------|----------|--------------|--------|
| **Phase 62** | Foundation: unify `build.gradle` / `build.gradle.kts` (versionName, minSdk, targetSdk conflicts), version consistency across README/CHANGELOG/plan.md, CI wiring, connect the 5 test suites, legal audit of decompiled-derived sources (OpenTES4Oblivion, xOBSE, Common-Oblivion-Engine-Framework) | 2 weeks | None | Pending |
| **Phase 63** | Performance foundation: batch TextRenderer (glyph atlas + instancing), remove per-frame LOGI (text_renderer.cpp L386, L524), cache `glGetProgramiv` (L331), merge draw calls | 2-3 weeks | None | Done (glyph loop batched, build green) |
| **Phase 64** | Real data pipeline: decode the 19 missing record types (ACHR, ACRE, PGRD, GMST, LTEX, WATR, AMMO, GLOB, FURN, IDLE, LSCR, SGST, EFSH, SLGM, CSTY, ...), fix BSA v103 folder table interpretation, asset resolution for nif 9,645 / dds 21,856, memory budget design | 4-6 weeks | 62 | Done (all 20 types decoded, verified on emulator with real Oblivion.esm) |
| **Phase 65** | Full world rendering: CELL 35,787, LAND 31,927 (terrain mesh + LTEX blending), WRLD 103, LOD 10,810, weather/time of day, water, cell streaming, door transitions | 8-12 weeks | 64 | In progress (terrain mesh + per-quadrant LTEX texturing landed and verified - `Terrain textured: 9 of 9 drawn cells, 36 texture bindings, 9 LTEX loaded`; measured 30-33 fps in gameplay. Remaining: VTXT multi-layer opacity blending, native-heap reduction, water, weather, doors, LOD) |
| **Phase 66** | Characters and AI: place ACHR/ACRE 3,663, full decode of NPC_ 2,664 / CREA 1,001, skeleton and animation (kf 2,687), execute PACK 7,717, PGRD 8,228 pathfinding, face generation (egm 155), lip sync (lip 50,916) | 10-14 weeks | 64, 65 | Pending |
| **Phase 67** | Full Script VM compatibility: implement ~1,080 additional functions, QUST 416, INFO 20,333 dialogue, condition evaluation, reference resolution | 16-24 weeks | 66 | Pending |
| **Phase 68** | Game systems and GUI: combat, magic/enchanting/alchemy (SPEL 1,314 - ENCH 1,694 - ALCH 253), skills/leveling, crime/theft, inventory, magic, map, journal, trade, persuasion screens | 10-14 weeks | 67 | Pending |
| **Phase 69** | Persistence: save/load, REF change state, containers, quest state (PC save compatibility explicitly out of scope) | 3-4 weeks | 68 | Pending |
| **Phase 70** | Mods and load order: plugins.txt, record override resolution, BSA priority, FormID index remapping | 4-6 weeks | 64 | Pending |
| **Phase 71** | UX and QA: first-run onboarding (BYO data in 5 minutes), device matrix, crash-free 99.5%, 2-hour stability, English/Japanese localization | 4-6 weeks | All | Pending |
| **Phase 72** | Release: signing, distribution, license notices, final legal review, documentation | 2-3 weeks | 71 | Pending |

**Total estimate: 65-94 weeks (about 1.3-1.8 years, one developer with AI assistance).**
Critical path: 64 -> 65 -> 66 -> 67 -> 68. Phase 67 (Script VM) is the largest single work item.
With parallelization (rendering and Script VM split between developers), 45-60 weeks is achievable.

### Milestones

| Milestone | Phase | Goal | Success Criteria |
|-----------|-------|------|------------------|
| **M11: Real Data Boot** | 62-64 | Load real game data | One Tamriel cell renders from Oblivion.esm |
| **M12: Performance** | 63 | Playable framerate | Stable 30 fps on title and exterior - **measured: 60 fps title screen, 30-33 fps exterior** (fresh emulator, 9 textured cells, controller drawn) |
| **M13: Full World** | 65 | Whole world traversable | All 35,787 cells reachable |
| **M14: Living World** | 66 | NPCs from data | NPCs spawn from ACHR and act via AI packages |
| **M15: Full Scripts** | 67 | Quest engine | Main quest completes end to end via Script VM |
| **M16: All DLC** | 66-68 | DLC parity | Knights and all 9 DLCs playable |
| **M17: Persistence** | 69 | Save system | Save/load fully functional |
| **M18: Mod Support** | 70 | Load order | .esp files apply in load order |
| **M19: Release** | 71-72 | Product ready | A1-A8 all satisfied |

### Risk Factors

| Risk | Impact | Mitigation |
|------|--------|------------|
| Legal: decompiled-derived code in the repo | Blocking for release | Mandatory audit in Phase 62; BYO-data model; no Bethesda asset redistribution |
| Script VM scale (~1,080 functions) | Schedule (largest item) | Prioritize by quest critical path; measure coverage per milestone |
| Mobile performance (1,044,109 REFR, 31,927 LAND) | Playability | **Native heap is the binding constraint, not draw calls**: 626 MB native heap drives the 2 GB emulator into swap and costs 15-30x frame rate. The dominant cost is `m_terrains` (see the measured breakdown in Current Status), not raw subrecord payloads - `esm_reader.cpp:446` already calls `rec.subRecords.clear()` after `decodeRecord()`, so the raw bytes are freed per record. Shrink the retained typed model first (`m_terrains`, then `m_recordIndex`, then `m_references`), then the landscape texture cache, then batching/instancing/LOD/culling |
| No wired tests | Regression blindness | Phase 62 CI before any content work |
| Build config duplication | Release correctness | Phase 62 single source of truth |
| BSA v103 folder table mismatch | Asset loading | Phase 64; extension counting already works via byte scan |

### Quality Assurance
- Unit tests wired into CI at Phase 62, extended per phase
- Device testing (Android 9+), device matrix defined in Phase 71
- Performance profiling against A2 thresholds each phase
- Real-data regression run (ESM + DLC + BSA) from Phase 64 onward

### Execution Plan: Parallel Workstreams

The phase schedule above is the *what*; this section is the *how* — how the work is split into
workstreams that can run at the same time without fighting each other.

**Environment constraints that shape the split**
- The Android emulator (`emulator-5554`) is a single shared resource. Only one workstream may
  build-and-install at a time. Workstreams that do not need a device must not touch it.
- Local `assembleDebug` takes ~1 minute and needs ~4 GB of RAM alongside the emulator; two
  concurrent Gradle builds on this machine degrade both.
- There is a large body of **uncommitted local work** (96 changed paths: the Gradle Kotlin-DSL to
  Groovy migration, removal of Bethesda loading-screen and intro-video assets, `.github/workflows/android.yml`,
  and 40 modified native sources). Until that is committed, CI has never run and none of the
  verified-on-emulator fixes are persisted. This is the highest-risk item in the whole plan.
- **`.github/workflows/android.yml` has been pre-validated against the real project so the first CI run is not a debugging session.** Three mismatches were found and fixed: the workflow set up JDK 17 while `gradle/gradle-daemon-jvm.properties` pins `toolchainVersion=21`, so the daemon JVM criteria could not be satisfied; `ndkVersion` was not pinned anywhere, leaving AGP free to resolve a different NDK than the one the workflow installs (it is now `26.1.10909125` in `app/build.gradle`, matching AGP 8.5's default and the local SDK, verified with a successful `assembleDebug`); and `cmake;3.22.1` — which AGP 8.5 requires and will not substitute — is now installed explicitly alongside the NDK, after `sdkmanager --licenses`. The workflow's version-consistency gate was also run locally and passes: `versionName=0.9.10` derives `versionCode=910`. The remaining unverified parts are the GitHub-hosted runner itself and the native build time for 3 ABIs, which the 60-minute timeout covers
- **Consequence for orchestration: WS-A and WS-B cannot start as separate worktree sessions yet.**
  A worktree session branches off committed `master`, and committed `master` is materially older
  than the working tree: `HEAD` still carries `app/build.gradle.kts` with `versionName "1.0"` /
  `versionCode 1`, has no `app/build.gradle`, and has no `.github/workflows/android.yml`. A child
  session started now would build a different project than the one verified on the emulator, so
  the pending work must be committed first. Committing is a user decision, not an autonomous one.

| WS | Scope | Phase | Depends on | Needs emulator | Owner |
|----|-------|-------|------------|----------------|-------|
| **WS-A** | Foundation: commit the pending local work, get `Android CI` green on GitHub, wire the 5 native C++ test suites (`phase30_integration_test`, `phase45_unit_tests`, `phase48_integration_test`, `phase48_stress_test`, `script_vm_tests`) into CI via a host-side runner, enforce version consistency | 62 | None | No | Child session |
| **WS-B** | Script VM coverage: grow the 118 implemented functions toward ~1,200, ordered by the main-quest critical path, each function covered by `script_vm_tests` | 67 (start) | None | No | Child session |
| **WS-C** | World rendering: LTEX terrain texture blending, static-object NIF rendering, water, weather, door transitions, LOD | 65 | 64 | **Yes** | This session |
| **WS-D** | Characters: load NPC_/CREA meshes from `nif`, skeleton and animation from `kf`, PACK execution, PGRD pathfinding | 66 | 64, 65 | **Yes** | Later, after WS-C |

**Why this split**
- WS-A and WS-B are pure code/config work with no device dependency, so they can run fully in
  parallel with WS-C, which owns the emulator.
- WS-A is deliberately first: without it, every other workstream's output stays uncommitted and
  unverified by CI. It is also the cheapest workstream and unblocks regression safety for all
  the rest.
- WS-B is the single largest work item in the project (about 1,080 missing functions) and the
  critical path runs through Phase 67, so it should start as early as possible rather than
  waiting for Phase 65/66.
- WS-D is held back because it shares the emulator with WS-C and because NPC meshes need the
  same NIF loader that WS-C builds for static objects.

**Sequencing**
1. WS-A and WS-B start once the pending local work is committed (see the orchestration constraint above).
2. WS-C continues in this session: terrain streaming is done and verified (9 active cells);
   next is LTEX blending so terrain stops being a flat green colour.
3. When WS-A lands, WS-B's and WS-C's work gains CI coverage.
4. When WS-C's NIF loader is usable, WS-D starts.

**WS-C first increment: landscape texturing (LTEX/BTXT/VTXT)**

The measured gap and the exact code path to close it:

- `Renderer::renderTerrainMeshes()` builds each cell mesh with only `position` and `normal`
  attributes and draws it with `placeholderVertexSrc` / `placeholderFragmentSrc` under a single
  solid `uColor` of `(0.32, 0.42, 0.22, 1.0)`. There is no texture coordinate attribute and no
  sampler, which is why every cell is flat green regardless of biome.
- `LAND` decoding already gives the raw material: `TerrainData::baseTextures[4]` (from `BTXT`,
  indexed 0=SW, 1=SE, 2=NW, 3=NE) and `TerrainData::textureFormIDs` (from `VTEX`).
- `VTXT` (per-vertex opacity for each `VTEX` layer) is **not parsed yet** — this is the missing
  piece for smooth multi-layer blending.
- `LTEX` records are already decoded (`ESMFile::getLandscapeTexture(formID)` returns the record,
  whose `iconPath` is the `.dds` path).
- `AssetManager::loadDDSTexture(path)` loads a `.dds` out of the BSA archives and returns a
  `Material` with a GL texture id; results are cached per path.

Steps, each independently verifiable on the emulator:
1. Parse `VTXT` in the `LAND` decoder into per-layer 33x33 opacity arrays, kept **only for active
   cells** (14,686 stored LAND records x 4 layers x 1089 x 2 bytes would be ~128 MB, which the
   2 GB emulator cannot afford).
2. Replace the terrain shader with one that has a `texcoord` attribute and samples the cell's
   base textures, selected by quadrant. Expected log: `Terrain textured: 9 cells, 36 textures`.
3. Add per-layer blending from the parsed `VTXT` opacities (up to 4 layers) with `uTex0..uTex3`
   and a `uBlend0..uBlend3` weight set.
4. Cap GPU texture memory with an LRU on the landscape-texture cache, and re-measure
   `World Status: ... Memory: X MB` against the current 9.04 MB baseline.

### Current Status (measured on emulator)
- App launches, reaches the title screen, taps through to gameplay, and runs stably with no crash (verified via logcat: no FATAL / SIGSEGV / ANR)
- Real game data is deployed to `/data/data/com.example.oblivion/files/data` (25 files, 3,109.5 MB; Voices excluded)
- Real `Oblivion.esm` parses fully: 35,494 cells / 1,029,280 references / 31,823 terrain records / 2,482 NPCs / 390 quests, 1,137 spells loaded, 3 spells assigned to the player
- Exterior world builds from ESM data: 14,686 exterior cells registered for the main worldspace Tamriel (0x0000003C), 1,855 interior cells skipped, 18,952 other-worldspace cells skipped, 1 duplicate grid square resolved in favour of the terrain-bearing cell
- References are placed in exterior cells only, with the ESM Z-up coordinate system swapped to the engine's Y-up: 105 actors placed (ACHR/ACRE resolved through NPC_/CREA), 449,824 interior references skipped
- **Exterior terrain renders**: `Terrain mesh built for cell 12950 grid=(0,0) heights 2720.0..4464.0` ... `cell 4086 grid=(1,1) heights 928.0..3376.0`, then `Terrain rendered: 9 exterior cells with heightmap, 9 drawn, cache=9`. Confirmed visually (screenshot pixel analysis: 1,490,216 green terrain samples of 2,073,600 = 71.9% of the frame, with distinct shades per cell)
- **Cell streaming is single-owner**: `WorldManager` alone loads/unloads cells around the player (3x3 grid, radius 6144, capped at `MAX_ACTIVE_CELLS = 9`); `CellTransitionManager` no longer loads cells and instead mirrors the active set via `syncFromActiveCells()`. This removed a double-streaming bug where `CellTransitionManager` pre-loaded a 5x5 grid (25 cells) that silently consumed the 9-cell active budget, pinning the active set at 4 cells. Measured after the fix: `World Status: 14686 cells, 9 active, Memory: 9.04 MB`
- **The player stands on the terrain**: `Player spawned in cell 12950 (0x0000808B) grid=(0,0) ground=3080.0` and the placeholder camera reports `playerPos=(2048.0, 3144.0, 2048.0)` (ground + eye offset), not the origin
- UI is live after New Game: joystick, ATK/BLK/MAG buttons and 4 quick slots registered and made visible
- All 64 ESM record types are decoded. The 20 secondary types verified against real data:
  GMST=382 GLOB=94 DOOR=501 PACK=7209 PGRD=8228 IDLE=650 KEYM=464 AMMO=128 SGST=150 SLGM=29
  FURN=186 LTEX=229 GRAS=108 WATR=23 WTHR=37 CSTY=126 LSCR=337 EFSH=102 ANIO=34 SBSP=33
- BSA v103 reader parses 14 of 22 expected archives. The 1.2 GB `Oblivion - Textures - Compressed.bsa` and `DLCShiveringIsles - Textures.bsa` **are** deployed, so terrain texturing is not blocked on data. The 8 absent archives are `Oblivion - Textures.bsa`, `Oblivion - Voices1.bsa`, `Oblivion - Voices2.bsa`, `DLCShiveringIsles - Textures - Compressed.bsa`, `DLCShiveringIsles - Voices.bsa`, `DLCShiveringIsles - Misc.bsa`, `DLCMehrunesRazor.bsa`, `DLCSpellTomes.bsa` (voice audio and compressed-texture variants only)
- **Input is fully verified end to end on the emulator.** All four layers of the touch path log on a single `input tap 644 639` (New Game): `MainActivity: GLSurfaceView touch DOWN` -> `Renderer: === Touch event detected ===` -> `TitleScreen: Menu touch DOWN` -> `MainActivity: Forwarded touch to native`. The virtual controller is verified the same way: `ATK button callback fired!`, `BLK button callback fired!`, and a joystick drag reports `'Joystick' at (188, 829) size (282x282)` with `hit=1` / `event consumed by 'Joystick'`. The earlier input-dispatch ANR was caused by the app running under ARM-to-x86 binary translation and is fixed by adding `x86_64` to `abiFilters`; `primaryCpuAbi` is now `x86_64`
- **The minimap no longer paints outside its panel.** `ui_map_panel.cpp::renderMapContent()` clips each discovered cell to the panel content rect. Screenshot pixel analysis of the dominant terrain green `(74, 140, 74)`: before the fix 472,772 px in a 734x734 bbox spanning `x 1186-1919, y 0-733`; after the fix 23,041 px in a 180x180 bbox spanning `x 1710-1889, y 30-209`, which is exactly the minimap content rect
- **Frame rate now exceeds the A2 target: 42-49 fps in gameplay (average 42.7) and 60 fps at the title screen.** `PerformanceMonitor` reports `Average Frame Time: 22.06 ms`, `Min Frame Time: 15.63 ms`, `Max Frame Time: 40.96 ms`. This supersedes both the earlier 30-33 fps reading and the 19.3 fps regression described below
- **The gameplay frame-rate ceiling was `glGetUniformLocation` abuse, not the emulator's GL transport.** `ui/ui_draw_helper.cpp` resolved its uniform locations on *every* draw call — 2 per `drawColoredQuad`, 3 per `drawTexturedQuad`, 5 per `drawGradientQuad`, 7 per `drawRadialGlow` and 16 per `draw3DTextureQuad`. Each `glGetUniformLocation` is a synchronous round-trip to the GL server, so the in-game HUD (joystick, ATK/BLK/MAG, 4 quick slots, HUD text, minimap) issued hundreds of them per frame. The evidence chain: `PerformanceMonitor` showed a **`Min Frame Time: 37.19 ms`** floor (27 fps even on the best frame), pointing the camera at empty sky did not change the frame rate (so it was not fragment load), the title screen ran at 60 fps (it draws no HUD), `simpleperf` attributed **83.22%** of samples to `goldfish_pipe.ko: goldfish_pipe_read_write` with under 0.1% in application code, and `top -H` showed the GL thread at 96.5% with every other thread at 0.0%. Memoizing the locations per `(program, name)` collapsed the floor to **15.63 ms** and took gameplay from **19.3 fps to 42.7 fps**. `engine/shader.cpp` and `ui/text_renderer.cpp` already cached their locations, which is why the 3D mesh paths were never the problem. **Lesson: on this emulator, any GL call that requires a server round-trip inside a per-frame loop is a frame-rate cliff; cache every location and never query state per draw**
- **The 1-2 fps reading was a degraded-emulator symptom, and the cause is now understood: the emulator runs out of RAM and swaps.** On the fresh emulator `TOTAL SWAP PSS` is **461 KB**; on the degraded one it was **223 MB** of a 2 GB AVD. The app's own footprint is what pushes it there: **Native Heap 641,065 KB (626 MB) / TOTAL PSS 685,640 KB (670 MB)**, against an A3/A4 budget of 1 GB. The same effect shows in startup time: the ESM parse takes **13 s on the fresh emulator** versus **222 s when the emulator was swapping** (and 64 s at a reduced 1280x720 override). `simpleperf` on the swapping emulator attributed 54.30% of samples to `goldfish_pipe.ko: goldfish_pipe_read_write` and ~40% more to kernel softirq/scheduler paths with only ~0.2% in application code, i.e. it was measuring the swap path, not the renderer. **Corrected conclusion (measured later): peak native heap controls *startup time* and avoids swap thrash, but it is *not* the frame-rate lever.** After the heap fell to 421 MB with `SwapPss` down to 34 MB, gameplay still ran at only 19.3 fps, and stopping every piece of emulator bloatware to free 157 MB changed nothing (18.4 fps). The frame-rate lever turned out to be GL server round-trips in the HUD, described above. Current footprint after both fixes: **Native Heap Pss 370,275 KB (362 MB), TOTAL PSS 418,089 KB (408 MB), TOTAL SWAP PSS 455 KB** — effectively no swapping, against an A3/A4 budget of 1 GB. A4 must still be measured on a real device. The measured breakdown of the retained typed model (`assets/esm_reader.h`), which is what actually holds the memory:

| Container | Count | Bytes each | Total | Note |
|-----------|-------|-----------|-------|------|
| `m_terrains` (`TerrainData`) | 31,823 | ~1.2 KB | **~37 MB** | now `heightDeltas` 1089 x int8 = 1089 B, plus `heightOffset` and the LTEX ids; `normals`/`colors` are gone and heights stay in the file's int8 gradient form |
| `m_recordIndex` (`RecordIndex`) | 1,167,017 | 40 B | ~47 MB | recType/formID/flags/dataSize/fileOffset/decompSize/parentFormID/worldspaceFormID |
| `m_references` (`ReferenceData`) | 1,029,280 | 44 B | ~45 MB | formID/baseFormID/cellFormID/position/rotation/scale/flags/refType |
| remainder | - | - | ~180 MB | `m_cells` 35,494 with two `std::string` each, decoded NPC_/QUST/DIAL maps, GL textures and the 9-cell terrain mesh cache |

`m_terrains` alone was about 56% of the native heap and was the cheapest thing to shrink. **Done:** `TerrainData` now stores the raw 1089-byte int8 `VHGT` gradient plus `heightOffset` and expands on demand via `expandHeights()`/`heightAt()`; `normals` were dropped because `calculateNormals()` derives them, and `colors` (VCLR) were unused. Per-terrain cost fell from ~11.0 KB to ~1.2 KB. Measured: **native heap allocation 704,684 KB -> 430,916 KB, Native Heap SwapPss 629,158 KB -> 33,981 KB (-95%), TOTAL SWAP PSS 648,001 KB -> 49,735 KB (-92%)**. This also fixed a real rendering bug: the old decoder demanded the Morrowind 65x65 int16 `VHGT` (8451 bytes) while TES4's is 1096 bytes, so `hasHeights()` was *always* false and every terrain silently fell back to a flat plane. The expansion formula `(heightOffset + columnOffset + rowOffset) * 8.0f` was validated against an independent Python parse of `Oblivion.esm`: 14,052 of 14,613 east-edge heights and 14,066 of 14,608 north-edge heights agree (96%), with the remainder explained by cells from different worldspaces sharing grid coordinates
- **Remaining known memory item: `renderer.cpp` gives every terrain-bearing cell its own expanded heightmap.** `cell->heightData = terrain.expandHeights()` runs inside the loop over all 31,823 terrain records, so all **14,686** matched exterior cells hold 1089 floats (4356 B) each, i.e. **~64 MB**, although only the 9 active cells are ever rendered. It is retained for now because `Cell::getTerrainHeightAt()` reads `heightData` from other systems and returning 0 for an unloaded cell would change object placement and player grounding; the fix needs a compact or lazily expanded representation plus a check of those callers. Note this is *more* correct than the previous behaviour, where the flat-terrain fallback meant only the 9 loaded cells had heights at all
- The emulator is **not** using software GL. `dumpsys SurfaceFlinger` reports `Android Emulator OpenGL ES Translator (NVIDIA RTX 500 Ada Generation Laptop GPU/PCIe/SSE2), OpenGL ES 3.1 (4.5.0 NVIDIA 616.56)` even though the AVD has `hw.gpu.enabled=no`. WHPX acceleration is active (`emulator -accel-check` -> `WHPX(10.0.26200) is installed and usable`)
- The emulator's guest/host GL transport is pinned to `pipe` (`ro.boot.hardware.gltransport=pipe`, `ro.boot.qemu.gltransport.name=pipe`, `ro.boot.qemu.gltransport.drawFlushInterval=800`). The guest kernel does expose `/dev/goldfish_address_space`, but the transport cannot be moved to the zero-copy `address_space` path from this host: `-prop ro.boot.qemu.gltransport.name=address_space` is ignored (the `ro.boot.*` namespace is populated from the kernel command line) and `-feature GLDirectMem` does not change it either. This is a curiosity rather than a blocker, because the `pipe` transport already reaches 42-49 fps once the HUD stops issuing synchronous uniform queries, and it hits 60 fps on the title screen
- Parsing runs inside `GameRenderer.onSurfaceCreated()` -> `nativeInitEngine()`, so it blocks the GL thread and the app shows nothing until it finishes. Even the good case (13 s) is a black screen on launch, so moving the parse off the GL thread stays a Phase 62/65 work item: it is the first thing a user experiences
- **Emulator operating rule learned the hard way: restart the AVD (`adb emu kill` then `emulator -avd Pixel_API34 -no-snapshot-load`) before any performance measurement, and check `TOTAL SWAP PSS` first.** A long-running emulator silently degrades by 15-30x and will produce completely misleading profiles. `adb root` must also be re-issued after every emulator restart or `am start` fails with `SecurityException: ... not exported from uid`, because `MainActivity` is not exported
- Not at product level. Next: Phase 65 (full world rendering - terrain mesh + LTEX blending, cell streaming, doors, water, weather)
