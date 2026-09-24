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
**Current Version**: 0.9.10 (`versionName`, deriving `versionCode` 910) - features through Phase 64 are complete, Phase 65 is in progress
**Goal**: Complete Oblivion gameplay experience on Android
**Last updated**: 2026-09-24 - the measured state is in "Verified build and status (2026-09-24)" near the end of this file

### Unimplemented Features Identified (all closed as of 2026-09-24)
- renderer.cpp: Weather, Time, Equip/Unequip, Magicka, Combat debug (11 TODOs) - **closed**: `renderer.cpp` now contains 0 TODO markers (Phase 47 weather, Phase 48 equipment)
- audio_decoder.cpp: MP3/OGG decode (header parsing only) - **closed**: `decodeMp3()` uses minimp3 and `decodeOgg()` uses stb_vorbis (`third_party/minimp3`, `third_party/stb`)
- Other: Icon mapping, Gender selection, Texture loading - **closed** in Phases 41 and 44

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
*Last updated: 2026-09-25*
*Next update: WS-B settles `MoveTo` arity from the measured `argLength` histogram, or WS-C lands water (WATR)*

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
| Total content to support | REFR 1,044,109 / CELL 35,787 / QUST 390 / SCPT 2,393 / NPC_ 2,664 / INFO 19,278 / CREA 1,001 / WRLD 103 / LAND 31,927 / PACK 7,717 / DIAL 4,126 / SPEL 1,314 |
| Script bytecode (SCDA) | 2,393 blocks / 807,893 B total (min 4 / median 188 / max 7,371) / exactly 1 block per SCPT record / 63,235 decoded instructions / 141 distinct opcodes / 0 compressed / 0 inflate failures |
| Script source (SCTX) | 2,393 records / 0 compressed |
| SCPT subrecords | EDID 2,393 / SCHR 2,393 / SCDA 2,393 / SCTX 2,393 / SCRO 10,848 / SLSD 7,266 / SCVR 7,266 / SCRV 996 - the subrecord walk completes for all 2,393 records with no truncation |
| SCPT script variables | SLSD 7,266 / SCVR 7,266 / SCRV 996, paired positionally (SLSD[i] <-> SCVR[i], counts identical) - **SLSD +0** is a 1-based u32 index (contiguous 1,230 / sparse 422 / no variables 741, smallest index per script 1..12), **+16** is a coarse type marker (1 = integer family, 0 = float/ref), +4..+7 carry stale ASCII fragments in 277 records and +8..+23 are otherwise unused; **SCVR** is the variable name; **SCRV** lists the index of every `ref` variable (all 996 of them). The exact type is not in the record at all - it is resolved from the SCTX declaration, which names the SCVR variable in **7,266 / 7,266** cases (Integer 5,100 = `short` 5,088 + `long` 1 + `int` 11 / Float 1,170 / Ref 996) |

**Census correction (2026-09-24)**: an earlier byte-substring scan over the raw ESM reported inflated counts (SCPT 3,046 / QUST 416 / INFO 20,333 / SCDA 9,646 / SCTX 9,992) because a four-character substring match also hits the same tag inside other records' payloads. A record-boundary walk over all 1,167,017 records gives the values above (SCPT 2,393 / QUST 390 / INFO 19,278 / SCDA 2,393 / SCTX 2,393), independently reproduced by a second reader written from scratch. The "SCPT/QUST record counts disagree between tools" risk row is closed by this correction; the reader is byte-complete, not truncated.

**Binary format (confirmed)**: Oblivion (TES4) uses a 20-byte record header, a 20-byte GRUP header, and 6-byte subrecord headers.
`esm_reader.cpp` already implements this correctly (L116 "TES4 record: 20-byte header", L183, L471 `size & 0xFFFF` with `offset += 6`).

### Implementation Coverage (measured)

| Area | Current | Required | Gap |
|------|---------|----------|-----|
| ESM binary format | Correct | - | None |
| Decoded record types | 65 | 64 | None (all 20 secondary types decoded and verified against real data) |
| Script functions | 118 | ~1,200 | ~1,080 functions |
| Mod / load order | None (single file parse only) | plugins.txt, override resolution, BSA priority | Design from scratch |
| Tests | 5 suites exist and all 5 are wired into CI through `tools/host_tests/run_host_tests.sh`; the runner links the real gameplay, asset, save, script, world and collision code and is green (ScriptVMTests 22/22 on `master`, 27/27 on the WS-B branch which adds 5 more; Phase45UnitTests 40/40, Phase48StressTest 5/5, Phase48IntegrationTest 7/7, Phase30IntegrationTest 1/1, self-skipped because Oblivion assets are not redistributable) | CI green | 5 of 5 wired |
| Performance | 42-49 fps gameplay / 60 fps title (measured on the emulator, hardware GL, x86_64); 60 fps title reconfirmed on the 115 MB APK (2026-09-24) | 30 fps stable | Met on the emulator; A4 needs a real-device run |
| Memory | 362 MB native heap / 408 MB PSS (measured, 93 MB APK) and 421-425 MB total PSS with SWAP 0 on both a 2 GB and a 4 GB guest (measured 2026-09-24, 115 MB APK) | A3/A4 budget 1 GB | Met with margin - see below |

Note: ACHR/ACRE are decoded and 105 exterior actors are placed from real data (449,824 interior references are deliberately skipped); the renderer still also spawns hardcoded demo NPCs (Izar/Hellas, renderer.cpp L2447-2448) as a fallback.

### Acceptance Criteria (product level)

| # | Criterion |
|---|-----------|
| A1 | Real Oblivion.esm + all DLC + BSAs load and all 35,787 cells render without corruption |
| A2 | Stable 30 fps (title / interior / exterior / combat) |
| A3 | Crash-free rate >= 99.5% over 1,000 sessions |
| A4 | 2-hour continuous play with no memory growth (no leaks) |
| A5 | Script VM implements ~1,200 functions and all 390 QUST records progress |
| A6 | CI green (5 existing suites wired plus real-data regression tests) |
| A7 | Legal compliance: BYO-data model enforced, zero decompiled-derived code |
| A8 | First-run UX: data install to gameplay in under 5 minutes |

### Phase Schedule

| Phase | Feature | Estimate | Dependencies | Status |
|-------|---------|----------|--------------|--------|
| **Phase 62** | Foundation: unify `build.gradle` / `build.gradle.kts` (versionName, minSdk, targetSdk conflicts), version consistency across README/CHANGELOG/plan.md, CI wiring, connect the 5 test suites, legal audit of decompiled-derived sources (OpenTES4Oblivion, xOBSE, Common-Oblivion-Engine-Framework) | 2 weeks | None | Done: build config unified and pushed (`012a5bb4`, `55542f87`, `d4b27570`, `d94317bc`); `Android CI` is green - run 35884733212, 6m23s, 2026-09-24 (launcher icon check, version-consistency gate, JVM unit tests, 3-ABI APK build, artifact upload). Native C++ host tests run in CI through `tools/host_tests/run_host_tests.sh` and **all 5 suites are now in it** (`48a9c934`): the runner compiles 53 translation units covering the real gameplay, asset, save, script, world and collision code, and is green with only two stand-ins left (no-op GLES3 entry points, and a `PhysicsManager` that reports "physics unavailable" rather than simulating). **One part of this phase is explicitly deferred, not done**: the legal audit of decompiled-derived sources (OpenTES4Oblivion, xOBSE, Common-Oblivion-Engine-Framework). The user decision on 2026-09-24 was "handle the asset legal blocker later, keep using the original assets for now", so the BYO-data model stays in place (no Bethesda asset is redistributed, no video or BSA is tracked) while the audit itself remains outstanding and is a release blocker for A7 |
| **Phase 63** | Performance foundation: batch TextRenderer (glyph atlas + instancing), remove per-frame LOGI (text_renderer.cpp L386, L524), cache `glGetProgramiv` (L331), merge draw calls | 2-3 weeks | None | Done (glyph loop batched, build green) |
| **Phase 64** | Real data pipeline: decode the 19 missing record types (ACHR, ACRE, PGRD, GMST, LTEX, WATR, AMMO, GLOB, FURN, IDLE, LSCR, SGST, EFSH, SLGM, CSTY, ...), fix BSA v103 folder table interpretation, asset resolution for nif 9,645 / dds 21,856, memory budget design | 4-6 weeks | 62 | Done (all 20 types decoded, verified on emulator with real Oblivion.esm) |
| **Phase 65** | Full world rendering: CELL 35,787, LAND 31,927 (terrain mesh + LTEX blending), WRLD 103, LOD 10,810, weather/time of day, water, cell streaming, door transitions | 8-12 weeks | 64 | In progress: terrain mesh, per-quadrant BTXT/LTEX texturing and **VTXT multi-layer opacity blending** are landed and verified - `Terrain overlay: 9 cells with ATXT/VTXT layers, 36 overlay texture bindings`, all 20,191 LAND records sampled carry ATXT/VTXT layers (13-31 layers per cell, ~700-1,400 weights), terrain colour diversity ~100 -> 10,671 distinct colours. Native-heap reduction is done (see Current Status). Remaining: water, weather/time of day, door transitions, LOD, static-object NIF rendering |
| **Phase 66** | Characters and AI: place ACHR/ACRE 3,663, full decode of NPC_ 2,664 / CREA 1,001, skeleton and animation (kf 2,687), execute PACK 7,717, PGRD 8,228 pathfinding, face generation (egm 155), lip sync (lip 50,916) | 10-14 weeks | 64, 65 | Pending |
| **Phase 67** | Full Script VM compatibility: implement ~1,080 additional functions, QUST 390, INFO 19,278 dialogue, condition evaluation, reference resolution | 16-24 weeks | 66 | **Unblocked (2026-09-24). The real SCDA instruction format is now fully determined and byte-verified.** Confirmed format: `[u16 opcode][u16 argLength][argLength bytes]`, i.e. `advance = 4 + argLength`, with exactly one exception - **opcode `0x001C` is always 4 bytes and its argLength field is ignored** (it carries a u16 line/label marker, not a payload length). Every script begins with the 4-byte prologue `1d 00 00 00` (`opcode 0x001D`, argLength 0) and ends with the 4-byte STOP `11 00 00 00` (`opcode 0x0011`, argLength 0); 72 of the 2,393 blocks are the prologue alone (4 bytes). Measured completion on the real ESM, counting only scripts that walk to an exact end: base rule alone **1,226 / 2,393**; with `0x001C` treated as `delta -1` **1,950 / 2,393**; with `0x001C` fixed at 4 bytes **2,393 / 2,393 (100%)**. The earlier `delta -1` result was an approximation that happens to coincide with the true rule whenever `argLength == 1`, which is only 2,702 of the 5,647 `0x001C` occurrences (the measured argLength histogram runs 1..37). The fix is one condition in the instruction-length calculation of `script_vm.cpp` and `script_disasm.cpp`; `script_opcodes.h` must then be re-derived from the measured table (141 distinct opcodes, 63,235 instructions, band `0x00xx` 50,573 / `0x10xx` 11,413 / `0x11xx` 1,249 - the `0x10xx` and `0x11xx` bands are the function-call opcodes). Note that the existing table's `JUMP_Z = 0x0011` is wrong: real `0x0011` is the argLength-0 STOP terminator. This was a **live correctness bug, not a census problem**: `dialogue_runner.cpp` L380 calls `ScriptManager::startScript()` and `imperial_weave.cpp` L324 / `game_loop_coordinator.cpp` L228 run `vm_.execute()` every frame against real SCDA (`script_context.cpp` L19), so the VM has been executing misaligned bytecode. Remaining Phase 67 critical path: apply the length fix, re-derive the opcode table from measured values, then expand handlers ordered by measured call-site frequency. **The script header side is settled as well (2026-09-25)**: `decodeScript` now reads SCHR as `+4 refCount / +8 compiledLength / +12 lastVarIndex / +16 scriptType` and derives `varCount` from the SLSD records, with `scriptType` mapped 0 / 1 / 256 to Object / Quest / Magic because the on-disk values are not contiguous. The decoder fix is verified by running the production code against the real ESM: `compiledLength == SCDA size` for 2,393 / 2,393 records, and the scriptType histogram (Object 2,031 / Quest 265 / Magic 97) matches the independent census. **The script variable side is settled as well (2026-09-25)**: SLSD carries only a 1-based index at +0 and a coarse type marker at +16, SCVR carries the variable name in a strict positional pair with SLSD, and SCRV is the list of `ref` variable indices (996 records, all of them `ref`). The real type comes from the SCTX declaration, which names the variable in 7,266 / 7,266 cases (Integer 5,100 / Float 1,170 / Ref 996), so `ScriptVariable::type` is now populated instead of defaulting to `Integer 0`. **The on-device proof has since landed**: `decodeScript` emits one aggregate line per plugin and the APK run printed `SCPT variable types: scripts=2393 vars=7266 int=5100 float=1170 ref=996`, which is the measured distribution exactly, so the decoder is confirmed on the device and not only offline |
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
| Legal: decompiled-derived code in the repo | Blocking for release | Mandatory audit in Phase 62; BYO-data model; no Bethesda asset redistribution. **Status 2026-09-24: deferred by user decision** ("handle the asset legal blocker later, keep using the original assets for now") - the audit has not been performed, so A7 is not satisfied yet |
| ~~**Real SCDA does not match the VM's instruction format**~~ | ~~Blocking for A5~~ **CLOSED 2026-09-24** | **Resolved by measurement.** The real format is `[u16 opcode][u16 argLength][argLength bytes]` with the single exception that opcode `0x001C` is a fixed 4-byte instruction whose argLength field is ignored, anchored by the prologue `1d 00 00 00` at offset 0 and the STOP `11 00 00 00` at the tail of every block. Falsification criteria set for a candidate model were all met by this one: (1) all 2,393 blocks walk to an exact end - measured **2,393 / 2,393**, up from 1,226 for the base rule and 1,950 for the `0x001C delta -1` approximation; (2) the opcode alphabet is sparse - **141 distinct values** across 63,235 instructions; (3) the final instruction is the constant STOP opcode `0x0011`. The remaining work is mechanical: apply the length rule and re-derive the opcode table from measured data rather than from assumed FunctionID values |
| **Real SCDA does not match the VM's instruction format (residual)** | A5 correctness | Even with the length rule fixed, the VM's *opcode semantics* are still the project's own guesses (`Opcode::JUMP_Z = 0x0011` is in fact the STOP terminator; `Opcode::CALL = 0x1000` is a real opcode but the `0x10xx`/`0x11xx` bands are the call space, not a single value). Re-derive every opcode meaning from the measured 141-entry table plus the SCTX source text before expanding handlers, and do not carry forward any assumed FunctionID value |
| Script VM scale (~1,080 functions) | Schedule (largest item) | Prioritize by measured call-site frequency over the real scripts, not by estimated quest importance. **The SCDA half of this premise is now available**: the bytecode format is settled (see the closed row above), so frequency can be derived from decoded instructions as well as from SCTX source text, and the two can be cross-checked. Measure coverage per milestone |
| ~~SCPT/QUST record counts disagree between tools (2,393 vs 3,046 SCPT, 394 vs 416 QUST)~~ | ~~Census completeness, A5 scope~~ **CLOSED 2026-09-24** | **Resolved: the reader was never truncated - the higher numbers were an artifact of byte-substring counting.** A record-boundary walk over all 1,167,017 records gives SCPT 2,393 / QUST 390 / INFO 19,278 / SCDA 2,393 / SCTX 2,393, reproduced independently by a second from-scratch reader (identical totals, identical `{1: 2393}` fragment-per-record histogram, 0 compressed SCPT, 0 inflate failures, max record 35,159 B so the `size & 0xFFFF` mask never truncates). The inflated values came from matching four-character tags inside other records' payloads |
| Mobile performance (1,044,109 REFR, 31,927 LAND) | Playability | **Native heap is the binding constraint, not draw calls** (state before the fix): 626 MB native heap drove the 2 GB emulator into swap and cost 15-30x frame rate. The dominant cost is `m_terrains` (see the measured breakdown in Current Status), not raw subrecord payloads - `esm_reader.cpp:446` already calls `rec.subRecords.clear()` after `decodeRecord()`, so the raw bytes are freed per record. Shrink the retained typed model first (`m_terrains`, then `m_recordIndex`, then `m_references`), then the landscape texture cache, then batching/instancing/LOD/culling. **Resolved as measured (2026-09-24)**: peak native heap was 2,020,948 KB before the fixes and is 430,378 KB now, with 421-425 MB total PSS, SWAP 0 and 0 LMK kills on a 2 GB guest; the remaining frame-rate lever was GL server round-trips in the HUD, not memory. The next scale step (all 1,044,109 REFR / 31,927 LAND resident) still runs into the retained typed model, so keep shrinking it before adding content |
| SCPT script header (SCHR) fields read at the wrong offset | A5 correctness, script startup | **Fixed in `assets/esm_reader.cpp` 2026-09-25, verified against the real ESM.** The confirmed on-disk layout is `+0 unused (always 0) / +4 refCount / +8 compiledLength / +12 lastVarIndex / +16 scriptType`, and `decodeScript` now reads exactly that. `compiledLength` equals the SCDA block size for **2,393 / 2,393** records when read at +8 (the old +4 / +12 reading was 4 bytes off and matched only 4). Two of these fields are **compiler high-water marks, not record counts**: `+12` equals the largest SLSD index in **2,250 / 2,393** scripts and is never below it (`lt = 0`), while it equals the SLSD record count in only 1,855 (537 above, 1 below - `Dark05AssassinatedScript` at 26 vs 28); `+4` behaves the same way against the SCRO count (equal 1,839 / above 554 / **below 0**), so it must not be used to pre-size a reference array either. **There is no `varCount` field in SCHR** - the count has to be derived from the SLSD records, which is what the decoder now does. `scriptType` is a raw u32 that is *not* contiguous (`{0: 2,031, 1: 265, 256: 97}`), so `static_cast<ScriptType>` is unsafe; the reader maps 0 / 1 / 256 to Object / Quest / Magic (histogram matches the raw census) |
| SLSD variable type and default value read from bytes that are always zero | A5 correctness, script startup | **Fixed in `assets/esm_reader.cpp` 2026-09-25.** The old code took `type` from `data[4]` (0 in 7,002 of 7,266 records) and the default value from `data + 8` (**0 in all 7,266**), so every script variable in the game was decoded as `Integer 0` regardless of its real declaration. The type is now resolved from the SCTX declaration, which names the variable in **7,266 / 7,266** cases - `short` / `long` / `int` -> Integer, `float` -> Float, `ref` -> Ref (measured Integer 5,100 / Float 1,170 / Ref 996) - with `SCRV` membership and the SLSD `+16` marker retained only as a fallback for records whose source is unusable. Verified by running the production decoder against the real ESM: an independently recomputed expected type matches the decoded `ScriptVariable::type` for **7,266 / 7,266** variables with 0 mismatches and 0 unresolved, and the APK prints the same totals on the device (`SCPT variable types: scripts=2393 vars=7266 int=5100 float=1170 ref=996`). The per-script `SCPT vars:` lines are not a usable check on their own - only 1,472 of the 2,393 survive the logcat ring buffer - which is why the totals are logged as a single aggregate line after the load. Note that the type is *not* recoverable from SCVR names alone - a name matches the SCTX text for only 4,457 / 7,266 variables - and that `int` (11 declarations, all in `SE06SCRIPT`) is a real keyword that a `short` / `long` / `float` / `ref` parser silently drops. **The index is not an array position**: because 422 scripts have sparse indices (and one, `Dark05AssassinatedScript`, has duplicate indices), `ScriptData::variables` must be addressed by `var.index`, not by vector position, and a safe slot allocation upper bound is `lastVarIndex + 1` |
| No wired tests | Regression blindness | Phase 62 CI landed before any content work (`d94317bc`, green); the native suites followed in `48a9c934` (see the row below) |
| Build config duplication | Release correctness | Phase 62 single source of truth |
| BSA v103 folder table mismatch | Asset loading | Phase 64; extension counting already works via byte scan |
| Broken gitlink `tools/BSAFileExtractor` | Repo hygiene, CI | **Resolved 2026-09-24.** It was committed as mode 160000 (a gitlink) with no `.gitmodules` entry, so it showed as a permanently modified path and `git status` was never clean. `.gitignore:28` already listed `tools/BSAFileExtractor/`, so the intent was always to keep the tool untracked; the gitlink was removed from the index with `git rm --cached` and the checkout is left in place for local use. No source, script or workflow references the path |
| 4 of the 5 native test suites were unwired | Regression blindness | **Resolved 2026-09-24.** `tools/host_tests/run_host_tests.sh` now compiles all 5 suites and links the real gameplay, asset, save, script, world and collision code (53 translation units, `-lz`, `--gc-sections`). The two remaining stand-ins are deliberately honest: no-op GLES3 entry points, and a `PhysicsManager` whose `init()` returns false so callers take their existing "physics disabled" path - no suite can pass on fabricated simulation. `phase30_integration_test` self-skips when the Oblivion assets are absent, which is the normal CI case. See `48a9c934` |

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
- The whole body of local work is **committed and pushed** (`012a5bb4`, `55542f87`, `d4b27570`,
  `d94317bc`) and `master` is level with `origin/master`. `Android CI` has run: run 35884733212 on
  `d94317bc` is green in 6m23s. The `tools/BSAFileExtractor` gitlink that used to keep `git status`
  permanently dirty has been removed from the index, so the only uncommitted path left is
  `.github/memory/session-memory.json`.
- **`.github/workflows/android.yml` has been pre-validated against the real project so the first CI run is not a debugging session.** Three mismatches were found and fixed: the workflow set up JDK 17 while `gradle/gradle-daemon-jvm.properties` pins `toolchainVersion=21`, so the daemon JVM criteria could not be satisfied; `ndkVersion` was not pinned anywhere, leaving AGP free to resolve a different NDK than the one the workflow installs (it is now `26.1.10909125` in `app/build.gradle`, matching AGP 8.5's default and the local SDK, verified with a successful `assembleDebug`); and `cmake;3.22.1` — which AGP 8.5 requires and will not substitute — is now installed explicitly alongside the NDK, after `sdkmanager --licenses`. The workflow's version-consistency gate was also run locally and passes: `versionName=0.9.10` derives `versionCode=910`. The remaining unverified parts are the GitHub-hosted runner itself and the native build time for 3 ABIs, which the 60-minute timeout covers. The first run has since happened (`d94317bc`, run 35884733212, 6m23s, green), which confirms the pre-validation: the JDK 21, NDK and CMake pins were all correct and the first green run needed no workflow fix
- **WS-B can start as a separate worktree session.** A worktree
  session branches off committed `master`, and `master` now carries the verified project
  (`app/build.gradle` with `ndkVersion '26.1.10909125'`, no Kotlin-DSL build files,
  `.github/workflows/android.yml`), so a child session builds exactly what was verified on the
  emulator. The push that was waiting on the user has happened, so every push to `master` now gets a
  green-or-red `Android CI` verdict. A WS-B worktree already exists
  (`hhkk0127-script-vm-expansion`, branch 0 commits ahead / 4 behind `master`, checked out at
  `012a5bb4`), so it must be brought up to `d94317bc` before the first commit to inherit CI and the
  host test runner. Two older worktrees are also stale (`hhkk0127-oblivion-android-completion-plan`
  31 behind, `hhkk0127-virtual-controller` 40 behind and prunable).

| WS | Scope | Phase | Depends on | Needs emulator | Owner |
|----|-------|-------|------------|----------------|-------|
| **WS-A** | Foundation: push the committed work and get `Android CI` green on GitHub, wire all 5 native C++ test suites (`phase30_integration_test`, `phase45_unit_tests`, `phase48_integration_test`, `phase48_stress_test`, `script_vm_tests`) into CI via a host-side runner, enforce version consistency | 62 | None | No | **Done** (`55542f87`, `d4b27570`, `d94317bc`): CI green, version gate + JVM unit tests + 3-ABI APK + host runner live. **Finishing increment landed 2026-09-24 as `48a9c934`**: the other 4 suites (`phase45_unit_tests`, `phase48_stress_test`, `phase48_integration_test`, `phase30_integration_test`) are now wired into `tools/host_tests/run_host_tests.sh` and `host_runner_main.cpp`, and the harness links the quest sub-systems (`quest_flow_controller.cpp`, `quest_stage_manager.cpp`, `quest_objective_tracker.cpp`, `quest_rewards.cpp`, `quest_record.cpp`) that WS-B's script functions call, plus NPC, Player, spell, navmesh, script, inventory, audio, animation, NIF, facegen, collision, world and save code - **53 translation units in total**, with `-lz` for the NIF reader and `--gc-sections`. All 5 suites are green: ScriptVMTests 22/22, Phase45UnitTests 40/40, Phase48StressTest 5/5, Phase48IntegrationTest 7/7, Phase30IntegrationTest 1/1 (self-skipped). Only two stand-ins remain, both deliberately honest: no-op GLES3 entry points, and a `PhysicsManager` whose `init()` returns false so callers take their existing "physics disabled" path. `weave::EventBus` needed to be linkable without the renderer/video/Jolt subsystems that `engine/imperial_weave.cpp` pulls in, so its 7 out-of-line definitions moved verbatim into a new `engine/event_bus.cpp` registered in `app/src/main/cpp/CMakeLists.txt`; the relocation is exact (the same 7 methods, no duplicate definitions anywhere in the tree) but it is a **production change**, so it is only proven by rebuilding the APK once WS-C's in-flight increments land. `save_manager.cpp` resolves its save base directory through `OBLIVION_SAVE_DIR` on non-Android builds, and `phase30_integration_test.cpp` records `SKIP_Assets_Unavailable` and returns success when `OBLIVION_ASSET_BASE` is unset or the first NIF is missing, because Oblivion assets are not redistributable |
| **WS-B** | Script VM coverage: grow the 118 implemented functions toward ~1,200, ordered by the main-quest critical path, each function covered by `script_vm_tests` | 67 (start) | None | No | **In progress** (session `0c8d192d`, branch `hhkk0127-script-vm-expansion`): batches committed as `4d52272a` (Quest functions) and `d3b07051` (inventory + actor functions) on top of a preserved 11-file / +340 -23 working tree, `origin/master` merged as `cb2c96e1`; handler count 118 -> 125; provisional runner 26 passed / 0 failed, still stub-backed so the permanent harness must re-confirm. Player `FormID 0x14` is resolved through `QuestFlowController::getPlayer()`, which keeps `engine/renderer.cpp` untouched beyond the already-approved injection. `c193afa6` then connected `GetDistance` / `SetPos` / `GetPos` / `MoveTo(ref)` to a shared Player/NPC resolver with axis validation (0..2); the count stays at 26 passed because the new position assertions were added inside the existing `testScriptFunctions()` group rather than as a new test function. **The SCDA census is now complete and it settled the format** (see Phase 67 and the closed risk row): the real instruction encoding is `[u16 opcode][u16 argLength][argLength bytes]` with opcode `0x001C` as a fixed 4-byte exception, which walks all **2,393 / 2,393** blocks to an exact end. The finding has been handed to WS-B with the full measured 141-opcode table, the prologue/STOP anchors, and the SCHR `compiledLength` +8 offset. WS-B then committed `45cabd5a` (`Parse SCDA marker instructions correctly`), which applies the `0x001C` rule in `script_vm.cpp` / `script_disasm.cpp` and fixes an out-of-range read in `script_disasm.cpp` that was segfaulting the host test; the census re-ran clean at 2,393 / 2,393. **A correction was sent back to WS-B afterwards**: the `varCount = +12` reading in that commit is wrong - `+12` is the compiler's last-variable-index high-water mark (equal to the largest SLSD index in 2,250 / 2,393 scripts and never below it), SCHR carries no count field at all, so `varCount` must come from the SLSD record count and variable lookup must go through `var.index` rather than vector position (422 scripts have sparse indices). Next for WS-B: re-derive `script_opcodes.h` from measured values, then expand handlers. WS-B then committed `b896eafd` (`Initialize script locals with declared types`), which is the first consumer of the decoder fix: `ExecutionContext` normalizes each sparse index slot to a zero value of the declared `ScriptVariable::type` instead of leaving it `Integer 0`, with no change to `ScriptData`; the provisional runner is at 27 passed / 0 failed |
| **WS-C** | World rendering: LTEX terrain texture blending, static-object NIF rendering, water, weather, door transitions, LOD | 65 | 64 | **Yes** | **In progress** (session `30cbb1e4`): terrain mesh + BTXT/LTEX + VTXT multi-layer blending verified on the emulator; next increment is water (WATR), then weather, doors and LOD |
| **WS-D** | Characters: load NPC_/CREA meshes from `nif`, skeleton and animation from `kf`, PACK execution, PGRD pathfinding | 66 | 64, 65 | **Yes** | **Not started**: held back until WS-C's static-object NIF loader is usable and until the emulator is free |

**Cross-workstream file contention**

`engine/renderer.cpp` / `renderer.h` are edited by both WS-B and WS-C, so they are the one
contended file in the split. WS-B's pending diff adds 21 lines plus one member: `Renderer` owns a
`std::unique_ptr<QuestFlowController>` and passes it to `ScriptManager::init(...)` immediately
after the `ScriptManager` is created, which is the only existing initialization point that can
connect `SetStage` / `GetStage` to the runtime `QuestFlowController` (on `master` that class is
currently never instantiated). WS-C's diff adds the terrain VTXT statistics log, the texture LRU
and the `cleanup()` join. Both are wanted: `master`'s rendering changes win and WS-B's injection is
re-applied by hand on top. The merge into `master` is performed by session `964d16fb` once WS-C's
current increment has landed, so neither workstream resolves the other's rendering code.

**Why this split**
- WS-A and WS-B are pure code/config work with no device dependency, so they can run fully in
  parallel with WS-C, which owns the emulator. WS-A has landed, so the split is now proven rather
  than proposed.
- WS-A went first: until CI ran, every other workstream's output was unverified by a
  regression gate. It was also the cheapest workstream and it unblocked regression safety for all
  the rest.
- WS-B is the single largest work item in the project (about 1,080 missing functions) and the
  critical path runs through Phase 67, so it should start as early as possible rather than
  waiting for Phase 65/66.
- WS-D is held back because it shares the emulator with WS-C and because NPC meshes need the
  same NIF loader that WS-C builds for static objects.

**Sequencing**
1. WS-A is done: the work is pushed and `Android CI` is green (`d94317bc`, run 35884733212), and its
   finishing increment landed as `48a9c934` - all 5 native suites are now wired into
   `tools/host_tests/run_host_tests.sh` and green. The `host-tests` job was then hardened in the same
   step: `zlib1g-dev` is installed explicitly (the harness links `-lz` because `assets/esm_reader.cpp`
   and `assets/bsa_reader.cpp` include `<zlib.h>`), and its timeout was raised from 15 to 30 minutes.
   The timeout is not padding: 53 translation units are compiled by a single `g++` invocation, so the
   compile is sequential and cannot use the runner's core count, and the run measured 12m15s end to end
   locally. The job runs in parallel with the 60-minute APK build, so the longer ceiling costs no wall
   clock time.
2. WS-C has closed its first increment: terrain streaming, BTXT/LTEX texturing and VTXT multi-layer
   opacity blending are done and verified (9 active cells, 36 overlay texture bindings).
3. WS-B has started: its batches are committed (`4d52272a`, `d3b07051`, `c193afa6`, `45cabd5a`,
   `1d3562df`, `b896eafd`, with `origin/master` merged as `cb2c96e1`) and its `script_vm_tests`
   vehicle runs in CI, so each further batch lands with a regression gate. Until the wiring landed
   its verification was stub-backed, which is weaker than implementation-backed verification: the
   provisional runner is only a gate, never the verdict, because return types are absent from C++
   mangled names and a stub whose return type differs from the real one links silently instead of
   failing. `48a9c934` removes that gap - the permanent runner now links the real quest, NPC, Player,
   spell, navmesh, script, inventory, audio, world, collision and save code.
4. WS-C's next increments are water, weather/time of day, door transitions and LOD; WS-D starts once
   WS-C's NIF loader is usable for static objects.
5. WS-B's branch did not need a push to pick the wiring up: worktrees share one object database and
   one set of refs, so `git merge master` inside its worktree was enough. It merged as `acf3619b`
   (conflict-free, as predicted - the 3 master commits it was missing touch none of the files WS-B
   changed) and its official post-merge run is green: exit 0, ScriptVMTests 27/27, Phase45UnitTests
   40/40, Phase48StressTest, Phase48IntegrationTest and Phase30IntegrationTest all passed, with the
   script function handler count unchanged at 125. Stub-backed verification is now a thing of the
   past for this branch: the verdict comes from the permanent runner linking the real code.

**WS-C first increment: landscape texturing (LTEX/BTXT/VTXT)**

The measured gap and the exact code path to close it:

- `Renderer::renderTerrainMeshes()` builds each cell mesh with only `position` and `normal`
  attributes and draws it with `placeholderVertexSrc` / `placeholderFragmentSrc` under a single
  solid `uColor` of `(0.32, 0.42, 0.22, 1.0)`. There is no texture coordinate attribute and no
  sampler, which is why every cell is flat green regardless of biome.
- `LAND` decoding already gives the raw material: `TerrainData::baseTextures[4]` (from `BTXT`,
  indexed 0=SW, 1=SE, 2=NW, 3=NE) and `TerrainData::textureFormIDs` (from `VTEX`).
- `VTXT` (per-vertex opacity for each `VTEX` layer) is **parsed now** — it was the missing
  piece for smooth multi-layer blending, and step 3 below records the verification.
- `LTEX` records are already decoded (`ESMFile::getLandscapeTexture(formID)` returns the record,
  whose `iconPath` is the `.dds` path).
- `AssetManager::loadDDSTexture(path)` loads a `.dds` out of the BSA archives and returns a
  `Material` with a GL texture id; results are cached per path.

Steps, each independently verifiable on the emulator:
1. Parse `VTXT` in the `LAND` decoder into per-layer 33x33 opacity arrays, kept **only for active
   cells** (14,686 stored LAND records x 4 layers x 1089 x 2 bytes would be ~128 MB, which the
   2 GB emulator cannot afford). **Done**: the 20,191 LAND records sampled all carry ATXT/VTXT
   layers, 13-31 layers per cell and roughly 700-1,400 weights per cell.
2. Replace the terrain shader with one that has a `texcoord` attribute and samples the cell's
   base textures, selected by quadrant. Expected log: `Terrain textured: 9 cells, 36 textures`.
   **Done**: `Terrain textured: 9 of 9 drawn cells, 36 texture bindings, 9 LTEX loaded`.
3. Add per-layer blending from the parsed `VTXT` opacities (up to 4 layers) with `uTex0..uTex3`
   and a `uBlend0..uBlend3` weight set. **Done**: 4 BTXT base slots plus 4 VTXT overlay slots are
   bound per cell (`Terrain overlay: 9 cells with ATXT/VTXT layers, 36 overlay texture bindings`),
   and terrain colour diversity rose from about 100 to 10,671 distinct colours, with grass and rock
   patches visible in the same cell.
4. Cap GPU texture memory with an LRU on the landscape-texture cache, and re-measure
   `World Status: ... Memory: X MB` against the current 9.04 MB baseline. **Done** for the terrain
   mesh cache (`engine/cache_manager.cpp`, observed `cache=17` at the budget cap; 24 cell moves
   changed total PSS by +0.2 %). The landscape-texture LRU is still open.

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
- Not at product level. Next: Phase 65 (full world rendering - terrain mesh, BTXT/LTEX texturing and VTXT blending are done; water, weather, doors, LOD and static-object NIF rendering remain)

### Verified build and status (2026-09-24)

Everything in this section was measured on the current APK, on the emulator, unless stated otherwise.

- **Current build: 115,289,641 B (2026-09-23 21:09:27), containing arm64-v8a, armeabi-v7a and x86_64.** It grew from 93,522,973 B because the 8 opening-sequence videos are now bundled in `app/src/main/assets/videos/` (`oblivion_intro` 47.93 MB, `credits_menu` 21.74 MB, `map_loop` 10.01 MB, `oblivion_iv_logo` 4.30 MB, `game_studios_logo` 2.06 MB, `bethesda_logo` 0.40 MB, `oblivion_legal` 0.28 MB, `2k_games_logo` 0.12 MB). That directory is gitignored (`.gitignore:33`) and no video is tracked, so a fresh clone still builds without them.
- **The opening sequence plays end to end and is skippable.** `IntroVideoActivity` plays `bethesda_logo -> 2k_games_logo -> game_studios_logo -> oblivion_legal` and then launches the game: the unattended run reached `Launching MainActivity` at 00:02:01 with 0 `Video error` and 0 safety-timeout events, and 4 taps advanced through the 4 clips in 18.4 s (`User tapped to skip video, advancing`). The skip predicate used to be `mediaPlayer.isPlaying`, which cannot be satisfied when playback never starts, leaving the user on a frozen frame; it is now `videoStarted && !videoCompleted`, plus a safety-net timer of `duration + 5 s` (30 s fixed when the duration is unknown) that force-launches the game. Both paths verified on the emulator.
- **The title screen has a two-stage video background.** `oblivion_iv_logo.mp4` plays first (14 s, `Title IV logo completed, switching to background: map_loop.mp4`) and `map_loop.mp4` then loops behind the menu; `credits_menu.mp4` is a 1 % easter egg. Measured: 60 fps title screen, `Title video: updateTexImage=60/s callbacks=30/s errors(total)=0`.
- **Memory on the 115 MB APK: 421,185 KB -> 424,964 KB total PSS with SWAP 0, 0 LMK kills and 0 FATAL/ANR/SIGABRT** over a full launch on a 4 GB guest, and **~425 MB startup PSS with 0 LMK kills on a 2 GB guest**, which puts a 2 GB device inside the practical range. This closes the 2 GB problem recorded above: the LMK kills were triggered by the zram swap watermark (`min watermark is breached and swap is low`), not by a raw RAM shortage, and every victim was an `oom_score_adj 999` cached background process. Read the numbers with the phase split in mind: 42 MB during the intro video, ~425 MB once the title screen is reached (+383 MB for the ESM/BSA parse and the title resources), then flat at +0.06 % over 55 s (no leak).
- **The title screen renders the same as before the APK swap.** Menu ink (tol20 over y765-841 / x420-1521) = 7,548 / 7,556 px, inside the 7,500-8,100 normal band; content p50 luminance 175.6 / 174.1; correlation with the pre-swap screenshot +0.917 overall (+0.947 left third, +0.911 right third, +0.839 menu band).
- **Screenshot contamination has a detector.** The debug toggle is a 36 dp button at `top|start` with `alpha 0.7` (x26..121 / y26..121 at 420 dpi) and it opens a full-screen 50 % black scrim plus a 260 dp panel, so `tap 74 74` lands inside it and must never be used as a neutral tap. A contaminated frame reads menu ink tol20 = 0, content p50 about 60 and max luminance 255; a clean frame reads 7,500-8,100, 160-175 and about 243. Combine this with an MD5 comparison of two consecutive `screencap`s (identical MD5 means the same frame was returned) to catch both kinds of contamination.
- **The debug console no longer aborts on Teleport/Move.** 24 inline command lambdas indexed `args[0]` (the command name) as their first parameter, shifting every argument by one, so `Teleport` and `Move` passed the command name into `std::stoi` and raised SIGABRT. Fixed with an N+1 shift plus a range guard, and `executeCommand()` now wraps dispatch in `try/catch (const std::exception&)`. Verified: 0 FATAL / 0 SIGABRT and every debug-menu button acts.
- **The terrain mesh cache is bounded.** `engine/cache_manager.cpp` (LRU against a budget cap; observed `cache=17`) keeps the heightmap and mesh working set flat: 24 cell moves changed total PSS by +0.2 %. The `Terrain overlay:` statistics log is kept (every 120 frames).
- **The launcher screen responds and animates.** The Play button used to stop responding because the launcher rebuilt its buttons on re-entry, after the touch targets had been created; the buttons are now re-initialised correctly, hover scaling works, and the launcher fades and slides in. Verified by pixel comparison and by the touch-path logs.
- **The SCPT variable-type decoder is proven on the device.** A clean launch of the current APK (120,955,832 B, 2026-09-24 13:15:37) prints `SCPT variable types: scripts=2393 vars=7266 int=5100 float=1170 ref=996`, which is the offline measurement exactly (`int` 5,100 = `short` 5,088 + `long` 1 + `int` 11), with 0 FATAL / SIGABRT / SIGSEGV and the title screen reached while the process stays alive. This is why the decoder logs one aggregate line per plugin instead of relying on the per-script lines: only **1,472 of the 2,393** `SCPT vars:` lines survive the logcat ring buffer (1,502 even after `adb logcat -G 32M`), because the terrain, overlay and FPS output pushes them out before the ESM load finishes. A record-by-record check of that size cannot be done from per-record logs on this device; the totals have to come from the production code itself.
