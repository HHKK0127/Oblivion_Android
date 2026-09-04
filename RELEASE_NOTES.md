# Oblivion Android - Release Notes

A concise summary of each release. Full details are in [CHANGELOG.md](CHANGELOG.md).

---

## v3.2.0 (Phase 56) - Gamebryo Complete

- ParticleSystem (7 presets), PostProcessPipeline (8 effects)
- WaterRenderer (Gerstner waves, 6 types)
- SkyWeatherSystem (8 weather types, day/night cycle)
- SceneGraph with hierarchy and AABB culling
- MaterialSystem (8 texture slots, 8 default materials)

## v3.1.0 (Phase 55) - Engine Polish

- FrameBudgetManager (16.6ms per-frame budget)
- MemoryDefrag for runtime memory management
- ShaderCache with LRU eviction
- OcclusionCuller and BatchRenderer
- FaceGen brush-up and Jolt Physics extension

## v3.0.0 (Phase 54) - Imperial Weave v4.0

- 15-phase pipeline with ImperialWeaveConfig
- ServiceLocator for runtime service discovery
- 12 event types, frame budget enforcement

## v2.5.0 (Phase 50-53) - Visual Systems

- **Phase 50**: Distant LOD with frustum culling and HorizonRing mountains
- **Phase 51**: SpeedTree vegetation (4-stage LOD, instanced rendering, Perlin wind)
- **Phase 52**: FaceGen system (race morphs, expressions, hair/beard)
- **Phase 53**: Bink video player via MediaCodec JNI bridge

## v2.4.0 (Phase 46-49) - Asset Pipeline, Audio, Controls

- TextureManager, MeshLoader, WorldDataLoader, BSA/ESM/NIF readers
- AudioDecoder, BgmManager, SoundEffectManager
- 12 integration test cases
- GamepadMapper, TouchCalibration, InputVisualizer, HudCustomizer

## v2.3.0 (Phase 45) - Unit Testing

- 37 unit test cases

## v2.2.0 (Phase 44) - Performance Optimization

- MemoryPool, RenderOptimizer, AsyncTaskManager, CacheManager
- ProfilerDashboard

## v2.1.0 (Phase 43) - UI/UX System

- TouchGestureHandler, MenuTransitionManager
- HudLayout, ControlSchemeManager, AccessibilityManager

## v2.0.0 (Phase 42) - Game Loop Integration

- StateManager, InputRouter, GameLoopCoordinator
- SceneRenderer, DebugConsole, PerformanceProfiler

## v1.6.0 (Phase 41) - Binary Save System

- SaveManager with binary format, SaveSlotManager, AutoSave
- Serializable interface

## v1.5.0 (Phase 40) - NPC Dialogue Tree

- DialogueTree, DialogueRunner, DialogueFilterEngine
- DialogueHistory, DialogueRecord, NPC integration

## v1.4.0 (Phase 39) - Quest Flow System

- QuestFlowController, QuestStageManager, QuestObjectiveTracker
- QuestRewards (XP, gold, items, skills), QuestRecord parsing

## v1.3.0 (Phase 38) - Script VM Testing

- 20 unit tests for ScriptVM: ExecutionContext, Opcodes, ScriptFunctions, ScriptManager

## v1.2.0 (Phase 37) - Script VM

- Oblivion bytecode interpreter with 47 opcodes
- 118 game functions (Tier 1: 13 core, Tier 2: 105 extended)
- ScriptManager, ExecutionContext, ScriptDisasm

## v1.1.0 (Phase 36) - Jolt Physics

- PhysicsManager singleton with Jolt Physics
- CharacterVirtual player/NPC controllers (capsule-based)
- HeightFieldShape terrain collision from LAND data
- Raycast API for AI, combat, and interaction
- Fixed timestep (1/60s) for deterministic simulation

## v1.0.0 (Phase 35) - Radiant AI System

- 15 AI package types (Explore, Follow, Guard, Patrol, Combat, Flee, etc.)
- Priority-based PackageStack with combat/flee override
- AIScheduler with 24-hour time-based NPC routines
- NavMesh pathfinding (A* with path smoothing and stuck recovery)

## v0.9.10 (Phase 34) - Weapon Sound Routing + Quick-Slots

- Weapon-type-specific hit sound routing (blade, blunt, axe, bow, staff, unarmed)
- SpellSelectionPanel school-color icons
- Quick-slot spell buttons (F1-F4)

## v0.9.9 (Phase 33) - Combat Sounds + NPC Spatial Audio

- 11 dedicated combat sound definitions (hit, block, parry, dodge, death)
- NPC spatial audio callback for 3D positioning

## v0.9.8 (Phase 32) - Animation & Audio Integration

- AnimationSubscriber (EventBus to AnimationPlayer bridge)
- AudioSubscriber (EventBus to AudioManager bridge)
- SpellSelectionPanel UI, findSequenceByName()
- Imperial Weave Event.targetId field

## v0.9.7 (Phase 10-24) - Imperial Weave + Combat Enhancement

- Imperial Weave EventBus with 12-phase pipeline
- 9 weapon types with hitbox system
- Critical hit, block/parry/dodge mechanics
- NPC animation state management

## v0.9.5 (Phase 10-24) - Complete UI & HUD

- HUD components: minimap, compass, floating text, quick-slot bar, etc.
- Core menus: pause, character sheet, shop, quest log, dialogue
- Inventory & items: consumables, equipment effects, world drops

## v0.9.0 (Phase 9) - Graphical UI & Sound Effects

- TextureLoader, UIPanel, UIButton with multi-state textures
- UIDrawHelper for OpenGL ES 3.0 rendering
- 93 sound definitions, 307 WAV files
- TitleScreen graphical overhaul

## v0.8.0 (Phase 8) - Audio & Post-Processing

- OpenAL 3D audio with spatial positioning
- RetroFilter effects (pixelation, scanlines, CRT distortion)
- SaveLoadUI system with multiple slots
- Settings UI with debug mode and language toggle

---

*For the full change history including file-level details, see [CHANGELOG.md](CHANGELOG.md).*
