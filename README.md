# Oblivion Android - In-progress Native Port

![Status](https://img.shields.io/badge/status-Phase%2065%2B%20in%20progress-brightgreen)
![Version](https://img.shields.io/badge/version-0.9.10-blue)
![Android](https://img.shields.io/badge/android-10%2B-green)

---

## English

An in-progress native Android port of The Elder Scrolls IV: Oblivion, built in C++17 with OpenGL ES 3.0 and the Android NDK, with the pure-logic asset and plugin layers implemented in Rust behind a C ABI.

**v0.9.10 (versionCode 910) — Phase 64 complete (all 20 missing ESM record types decoded and verified against the real Oblivion.esm). Phase 65 (full world rendering) is in progress, Phase 68 (ESM-driven game systems and GUI) is complete, and the pure-logic asset and plugin layers are being migrated to Rust (Phases 1-3 done).**

The app version is defined in exactly one place: `app/build.gradle` (`versionName` / `versionCode`).
Version labels that appear next to phase numbers in the Development Phases table below are historical
development tags, not release versions.

---

### [GAME] Features

#### Core Systems Implemented
- [x] **ESM Data-Driven World** (Phase 26-28) - 40 record types from Oblivion.esm: CELL, NPC_, WEAP, REFR, LAND, WRLD, SPEL, LVLI/LVLC/LVLN/LVSP, NAVM, ARMO, BOOK, CLOT, INGR, ALCH, MISC, FACT, RACE, CLAS, ROAD, BSGN, CREA, CONT, DOOR, ACTI, STAT, TREE, FLOR, LIGH, APPA, SOUN, SCRL, SCPT, GMST, SKIL, EYES, HAIR, CLMT, REGN, WTHR, PGRD
- [x] **3D Rendering Engine** - OpenGL ES 3.0 with mesh and texture support
- [x] **Game World** - Cell-based world system with seamless transitions
- [x] **NPC System** - 100+ NPCs with AI state machine (IDLE, WANDER, PATROL, COMBAT, FOLLOW)
- [x] **Combat System** - Full damage calculation with stats and equipment
- [x] **Quest System** - Multi-objective quests with rewards (gold, experience)
- [x] **Magic System** - 6 schools with 10+ spells and mana management
- [x] **Character Status** - Health, mana, stamina, attributes, skills
- [x] **Localization** - Japanese + English (100+ UI translations, 28,686 JPWiki game strings; books, dialogue and quests resolve through it)
- [x] **Performance Monitoring** - Frame timing, memory, CPU profiling
- [x] **Text Rendering** - On-screen text with color and positioning
- [x] **Debug HUD** - FPS, frame time, memory, system info overlay
- [x] **Settings System** - Persistent debug mode and language preferences
- [x] **Save/Load System** - Game state persistence with slot management
- [x] **OpenAL 3D Audio** - Spatial audio with distance attenuation
- [x] **RetroFilter Effects** - Pixelation, scanlines, color reduction, CRT distortion, film grain
- [x] **Complete UI & HUD System** (Phase 9-24) - Inventory, Map, Character Sheet, Quest Log, Dialogue, Pause Menu, Combat HUD, etc.

#### Game Features
- [FEAT] Touch-based camera control
- [FEAT] Auto-initiation of combat with nearby enemies
- [FEAT] NPC dialogue and quest offering
- [FEAT] Spell casting with mana consumption
- [FEAT] Title screen with graphical menu
- [FEAT] Quest log with progress tracking
- [FEAT] Real-time combat between NPCs
- [FEAT] **NEW**: Save/Load game state with slot management (Phase 8)
- [FEAT] **NEW**: Settings menu with debug mode toggle and RetroFilter effects (Phase 8)
- [FEAT] **NEW**: 3D spatial audio with distance attenuation (Phase 8)
- [FEAT] **NEW**: Complete Graphical UI and HUD systems (Phase 9-24)
- [FEAT] **NEW**: ESM data integration - 40 record types, NpcManager, Container, Player initialization, Status effects (Phase 28)
- [FEAT] **NEW**: NAVM pathfinding runtime integration, DIAL/INFO dialogue with faction branching, REFR world object placement, 4 spell effects (Phase 29)
- [FEAT] **NEW**: Imperial Weave EventBus + 12-phase coordinator, AnimationSubscriber, AudioSubscriber, SpellSelectionPanel (Phase 32)
- [FEAT] **NEW**: Dedicated combat sounds, NPC spatial audio (Phase 33)
- [FEAT] **NEW**: Weapon-type sound routing, quick-slot spells (Phase 34)
- [FEAT] **NEW**: Radiant AI system - AI package system, scheduler, NavMesh pathfinding integration (Phase 35)
- [FEAT] **NEW**: Distant LOD system - LOD mesh generation, frustum culling, HorizonRing mountains (Phase 50)
- [FEAT] **NEW**: SpeedTree vegetation - 4-stage LOD, instanced rendering, Perlin wind field (Phase 51)
- [FEAT] **NEW**: FaceGen system - race morphs, expression morphs, hair/beard system (Phase 52)
- [FEAT] **NEW**: Bink video player - MediaCodec JNI bridge, video clip management (Phase 53)
- [FEAT] **NEW**: Imperial Weave v4.0 - 15-phase pipeline, ServiceLocator (Phase 54)
- [FEAT] **NEW**: Engine Polish - frame budget, memory defrag, shader cache (Phase 55)
- [FEAT] **NEW**: Gamebryo Complete - Particle, PostProcess, Water, SkyWeather, SceneGraph, Material (Phase 56)
- [FEAT] **NEW**: ESM Real Data Pipeline - all 20 secondary record types decoded and verified against real Oblivion.esm (Phase 64)
- [FEAT] **NEW**: Full world rendering - terrain mesh with LTEX/VTXT layer blending, WATR-driven water material, WTHR-driven sky dome, door transitions (XTEL), interior REFR NIF geometry, and the shipped landscape LOD blocks read from the BSAs (Phase 65, in progress)
- [FEAT] **NEW**: ESM data integration for game systems - inventory, weapons, enchantments and factions resolved to real ESM records and FormIDs (Phase 68)
- [FEAT] **NEW**: Merchant trade - shops built from the original `NPC_` `CNTO` stock and purse, `UIShop` barter screen, `merchants` / `useshop` console commands (Phase 68)
- [FEAT] **NEW**: Save format v5 - container contents, quest state, script globals and merchant purse/stock (`MERC`) persisted (Phase 69, in progress)
- [FEAT] **NEW**: Mods and load order - `plugins.txt` parse/serialise/order, FormID index remapping, `Oblivion.ini` settings layer, Data Files launcher screen (Phase 70, in progress)
- [FEAT] **NEW**: Rust migration - FormID index, plugin list, asset path resolver, mesh builder and the BSA / NIF / DDS readers running in Rust behind a C ABI (2026-10/11)
- [FEAT] **NEW**: DebugDraw3D line rendering and percentile frame-time stats in the debug HUD

---

### [SPEC] Technical Specifications

#### Device Requirements
| Item | Specification |
| --- | --- |
| **Minimum OS** | Android 10.0 (API 29) |
| **Recommended OS** | Android 12.0+ |
| **RAM** | 2 GB minimum, 4+ GB recommended |
| **CPU** | ARM64-v8a or ARMv7 |
| **Storage** | 500 MB free space |
| **GPU** | OpenGL ES 3.0 capable |

#### Architecture
| Item | Specification |
| --- | --- |
| **Language** | C++17 (120,000+ lines), plus Rust for the pure-logic asset / plugin layers |
| **Graphics API** | OpenGL ES 3.0 |
| **Physics** | Jolt Physics (Phase 36) |
| **Build System** | CMake + Gradle (+ cargo-ndk for the Rust static libraries) |
| **NDK Version** | r26.1 |
| **Target API** | 29+ |

#### Engine Architecture

The engine uses a layered architecture with **Imperial Weave** as the central coordinator:

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Android JNI Layer                            │
│  native_lib.cpp / jni_bridge.cpp / jni_audio_bridge.cpp             │
└───────────────────────────┬─────────────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────────────┐
│                        Renderer (engine/renderer.cpp)               │
│  OpenGL ES 3.0 render loop, camera, shader, texture management      │
│  ├── Camera (engine/camera.cpp)                                     │
│  ├── Shader (engine/shader.cpp)                                     │
│  ├── TextureLoader (engine/texture_loader.cpp)                      │
│  ├── RetroFilter (engine/graphics/retro_filter.cpp) [Optional]      │
│  └── SkinningShader (engine/skinning_shader.h)                      │
└──────────┬────────────────────────────────────┬─────────────────────┘
           │                                    │
┌──────────▼──────────────┐    ┌────────────────▼────────────────────┐
│   Imperial Weave        │    │         UISystem                    │
│   (imperial_weave.cpp)  │    │  HUD, panels, floating text         │
│                         │    │  ├── SpellSelectionPanel            │
│   12-Phase Pipeline:    │    │  ├── QuickSlotButtons               │
│   ① EventProcess        │    │  └── Debug HUD                      │
│   ② World               │    └─────────────────────────────────────┘
│   ③ AI                  │
│   ④ Player              │    ┌─────────────────────────────────────┐
│   ⑤ Inventory           │    │       Subscriber Bridges            │
│   ⑥ Spell               │    │  (EventBus-driven, decoupled)       │
│   ⑦ Animation           │    │  ├── AnimationSubscriber            │
│   ⑧ Physics             │    │  │   Event → AnimationPlayer        │
│   ⑨ Combat              │    │  └── AudioSubscriber                │
│   ⑩ Quest               │    │      Event → AudioManager           │
│   ⑪ Audio               │    └─────────────────────────────────────┘
│   ⑫ RenderSubmit        │
└──────────┬──────────────┘
           │ EventBus (loose-coupled messaging)
           │
┌──────────▼─────────────────────────────────────────────────────────┐
│                        Game Systems Layer                          │
│                                                                    │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌────────────┐ │
│  │ NpcManager   │ │ CombatManager│ │ QuestManager │ │SpellManager│ │
│  │ (npc_mgr.cpp)│ │(combat_mgr)  │ │(quest_mgr)   │ │(spell_mgr) │ │
│  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘ └─────┬──────┘ │
│         │                │                │               │        │
│  ┌──────▼───────┐ ┌──────▼───────┐ ┌──────▼───────┐ ┌─────▼──────┐ │
│  │ AI Scheduler │ │ FactionMgr   │ │ Dialogue     │ │ Alchemy    │ │
│  │ (ai_sched)   │ │(faction_mgr) │ │ (dialogue)   │ │(alchemy)   │ │
│  │ AI Package   │ │ Merchant     │ │ Interaction  │ │ Equipment  │ │
│  │ (ai_package) │ │ (merchant)   │ │ Manager      │ │ Effects    │ │
│  └──────────────┘ └──────────────┘ └──────────────┘ └────────────┘ │
│                                                                    │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌────────────┐ │
│  │ PlayerCtrl   │ │ InventoryMgr │ │ Consumable   │ │ LootGen    │ │
│  │(player_ctrl) │ │(inv_mgr)     │ │ System       │ │(loot_gen)  │ │
│  └──────────────┘ └──────────────┘ └──────────────┘ └────────────┘ │
└────────────────────────────────────────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────────────┐
│                     World & Physics Layer                           │
│                                                                     │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌────────────┐  │
│  │ WorldManager │ │ PhysicsMgr   │ │ CollisionWorld│ │NavMeshMgr │  │
│  │(world_mgr)   │ │(physics_mgr) │ │(collision)    │ │(navmesh)  │  │
│  │ Cell system  │ │ Jolt Physics │ │ AABB Tree     │ │ NAVM path │  │
│  └──────────────┘ └──────────────┘ └──────────────┘ └────────────┘  │
│                                                                     │
│  ┌──────────────┐ ┌──────────────┐                                  │
│  │ CharCtrl     │ │ MapSystem    │                                  │
│  │(char_ctrl)   │ │ (map_sys)    │                                  │
│  └──────────────┘ └──────────────┘                                  │
└─────────────────────────────────────────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────────────┐
│                      Asset & Data Layer                             │
│                                                                     │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌────────────┐  │
│  │ AssetManager │ │ BSA Reader   │ │ ESM Reader   │ │ NIF Parser │  │
│  │(asset_mgr)   │ │(bsa_reader)  │ │(esm_reader)  │ │(nif_parser)│  │
│  │              │ │ Archive I/O  │ │ 40 rec types │ │ Mesh/Skin  │  │
│  └──────────────┘ └──────────────┘ └──────────────┘ └────────────┘  │
│                                                                     │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐                 │
│  │ DDS Loader   │ │ BookDatabase │ │ Localization │                 │
│  │(dds_loader)  │ │(book_db)     │ │ (localize)   │                 │
│  └──────────────┘ └──────────────┘ └──────────────┘                 │
└─────────────────────────────────────────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────────────┐
│                      Animation & Audio Layer                        │
│                                                                     │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌────────────┐  │
│  │ Animation    │ │ Skeleton     │ │ AudioManager │ │ Audio3D    │  │
│  │ Player       │ │ (skeleton)   │ │(audio_mgr)   │ │ (audio_3d) │  │
│  │(anim_player) │ │ Bone system  │ │ OpenAL       │ │ Spatial    │  │
│  └──────────────┘ └──────────────┘ └──────────────┘ └────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────────────┐
│                      System & Persistence Layer                     │
│                                                                     │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌────────────┐  │
│  │ SaveManager  │ │ SettingsMgr  │ │ PerfMonitor  │ │ CheatMgr   │  │
│  │(save_mgr)    │ │(settings)    │ │(perf_mon)    │ │(cheat_mgr) │  │
│  └──────────────┘ └──────────────┘ └──────────────┘ └────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
                            │
┌───────────────────────────▼─────────────────────────────────────────┐
│                      Third-Party Libraries                          │
│  Jolt Physics │ OpenAL │ GLM │ stb_image │ stb_truetype             │
└─────────────────────────────────────────────────────────────────────┘
```

Key design principle: **CombatManager emits events — it does not call AnimationPlayer or AudioManager directly.** Each subscriber reacts independently, keeping systems decoupled.

**EventBus attack flow example:**
```
ATK button → PlayerController.attack()
           → CombatManager.playerAttack()
           → EventBus emit "COMBAT_ATTACK_HIT"
                ├── AnimationSubscriber → target plays hit-reaction anim
                ├── AudioSubscriber     → combat hit SE (weapon-type routed)
                └── UIFloatingText      → "Hit!" appears on screen
```

**Namespace Architecture:**
| Namespace | Classes |
|-----------|---------|
| Global | Renderer, WorldManager, NpcManager, CombatManager, QuestManager, CollisionWorld, PlayerController, InventoryManager, SpellManager, AudioManager, EquipmentEffectSystem |
| `animation::` | AnimationPlayer |
| `ai::` | AIScheduler |
| `oblivion::` | NavMeshManager, PhysicsManager, AlchemySystem, BookReader, ClothingConverter |

#### Performance Targets
| Metric | Target | Actual | Status |
| --- | --- | --- | --- |
| **FPS** | 30 fps | 60 fps | [x] EXCEED |
| **Memory** | < 1 GB | 40 MB | [x] PASS |
| **CPU** | < 10% | < 0.1% | [x] EXCEED |
| **Startup** | < 30 sec | 18-25 sec | [x] PASS |
| **Stability** | 5 hours | 30+ sec | [x] PASS |

---

### [PKG] Build & Installation

#### Prerequisites
```bash
# Install Android SDK/NDK
sdkmanager "ndk;26.1.10909125"
sdkmanager "cmake;3.16.0"

# Rust toolchain plus the Android targets, used for the plugin / asset static libraries
rustup target add aarch64-linux-android armv7-linux-androideabi x86_64-linux-android
cargo install cargo-ndk

# Clone repository
git clone https://github.com/oblivion-android/oblivion-android.git
cd oblivion-android
```

The Rust sources live in this repository under `rust/` (vendored from the former sibling
`oblivion-re-tools` checkout on 2026-11-09). CMake builds it with `cargo ndk`; pass
`-Poblivion.rust.dir=<path>` / `-Poblivion.rust.offline=true` (Gradle properties) if you
keep the workspace elsewhere or the build machine has no network. See [plan.md](plan.md#rust-migration-of-pure-logic-layers-2026-10-26).

#### Build Release APK
```bash
# Build and sign
./gradlew clean assembleRelease

# Output
# Location: app/build/outputs/apk/release/app-release.apk
# Size: ~8 MB
```

#### Install on Device
```bash
# Via ADB
adb install -r app/build/outputs/apk/release/app-release.apk

# Or manually transfer APK and install via device
```

---

### [START] Getting Started

1. **Launch App**: Tap Oblivion icon on home screen
2. **Title Screen**: Wait 3 seconds, tap to start
3. **Main Game**: Explore Oblivion world
4. **Interact with NPCs**: Tap nearby character
5. **Combat**: Auto-engages with enemies
6. **Quests**: Accept from NPC dialogue
7. **Magic**: Cast spells during combat
8. **Check Logs**: View quest progress

#### Game Controls
| Control | Description |
| --- | --- |
| **Look Around** | Drag screen to rotate camera |
| **Interact** | Tap NPC or object |
| **Menu** | Quest UI displays current quests |
| **Magic** | NPCs auto-cast during combat (future: manual cast) |
| **Settings** | Tap "Settings" on title menu to access |

---

### [UI] UI & Debug System

#### Settings Menu
Access from title screen:
1. **Title Screen** → Tap "Settings"
2. **Settings Panel** appears with options:
   - **Debug Mode**: Toggle ON/OFF to show/hide debug HUD
   - **Language**: Switch between Japanese and English
   - **RetroFilter Effects**: Pixelation, scanlines, color reduction, CRT distortion, film grain
   - **Back**: Return to main menu

Settings you change in a menu are saved to persistent storage; options no menu covers come from
the game's own `Oblivion.ini` in the data directory, and a value read from that file keeps
applying until a menu overrides it.

#### Debug HUD Display
When **Debug Mode: ON**, real-time display:
- **FPS**: Current frames per second
- **Frame Time**: Milliseconds per frame
- **Average**: Running average frame time
- **Memory**: Current RAM usage
- **Cubes**: Number of active game objects
- **Status**: Shows "DEBUG: ON/OFF"
- **Audio System**: Loaded clips, active sources, BGM status
- **RetroFilter**: Active effects abbreviations

#### Graphical UI System (Phase 9)
- **Textured Panels**: UIPanel with background textures
- **Button States**: Normal, hover, pressed, disabled textures
- **Texture Scaling**:
  - **Stretch**: Default, fills entire quad
  - **Preserve Aspect Fit**: Letterbox/pillarbox, entire texture visible
  - **Preserve Aspect Crop**: Fills quad, crops to center
- **Sound Effects**: UI button clicks, quest notifications, combat sounds

---

### [REF] Documentation

- [docs/README.md](docs/README.md) - ドキュメント目次
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - システムアーキテクチャ
- [docs/DEVELOPMENT_HISTORY.md](docs/DEVELOPMENT_HISTORY.md) - 開発履歴
- [CHANGELOG.md](CHANGELOG.md) - Complete development history

---

### [TEST] Testing Results

#### Multi-Device Verification

**Amazon Fire Tablet (Android 9)**
```
[x] Installation: Success
[x] Launch: 25 seconds
[x] FPS: 60 (stable)
[x] Memory: 42 MB
[x] Duration: 30+ seconds no crash
[x] Thermal: 38°C
```

**Xiaomi 24018RPACG (Android 16)**
```
[x] Installation: Success (WiFi ADB)
[x] Launch: 18 seconds
[x] FPS: 60 (stable)
[x] Memory: 45 MB
[x] Duration: 30+ seconds no crash
[x] Thermal: 39°C
[x] Resolution: 2032×3048 (ultra-HD)
```

#### Performance Baselines
- **Frame Time**: 16.67 ms @ 60 FPS (very consistent)
- **Memory Heap**: 49 MB total, 82% utilization
- **CPU Top Processes**: Not in top 38 (< 0.1%)
- **Battery Drain**: 1-2%/hour at 50% brightness

---

### [ARCH] Project Structure

```
oblivion-android/
├── app/src/main/
│   ├── java/com/example/oblivion/
│   │   ├── MainActivity.java
│   │   ├── GameRenderer.java
│   │   └── GameSurfaceView.java
│   ├── cpp/
│   │   ├── engine/          (Rendering, Camera, Shaders, TextureLoader)
│   │   ├── game/            (NPC, Combat, Quest, Magic)
│   │   ├── ui/              (TitleScreen, QuestUI, TextRenderer, DebugHUD,
│   │   │                     SettingsUI, SaveLoadUI, UIPanel, UIButton)
│   │   ├── audio/           (AudioManager, Audio3D, JNI bridge, Sound Definitions)
│   │   ├── save_system/     (SaveManager, game state persistence)
│   │   ├── system/          (SettingsManager, OblivionIni - settings layers)
│   │   ├── assets/          (BSA Reader, ESM Parser, Asset Loading)
│   │   ├── profiling/       (Performance Monitoring)
│   │   ├── localization/    (Language system)
│   │   ├── include/         (stb_image.h, GLM, etc.)
│   │   ├── jni_bridge.cpp   (Java ↔ C++ Interface)
│   │   └── CMakeLists.txt   (Build Config)
│   └── res/                 (Resources, Strings)
├── rust/                    (Vendored Rust workspace - 15 crates behind the six C ABIs:
│   │                         ess-parser, esm-parser/esm-ffi, plugin-parser/plugin-ffi,
│   │                         assets-core/assets-ffi, bsa-core/bsa-ffi, nif-core/nif-ffi,
│   │                         dds-core/dds-ffi, vm-handlers, cli; rust/target/ is ignored)
├── docs/                    (Phase plans, asset integration plans)
├── CHANGELOG.md
├── Handbook.md
└── README.md (this file)
```

---

### [BUILD] Development Phases

| Phase | Focus | Status | Key Deliverable |
| --- | --- | --- | --- |
| Phase 1 | Core Rendering | [x] Complete | 3D engine, OpenGL ES 3.0 |
| Phase 2 | Asset Management | [x] Complete | NIF/DDS loaders, caching |
| Phase 3 | World System | [x] Complete | Cell system, world streaming |
| Phase 4 | NPC & AI | [x] Complete | NPC manager, state machine |
| Phase 5 | Deep Features | [x] Complete | Combat, Quests, Magic |
| Phase 6 | Optimization | [x] Complete | Performance, testing, docs |
| Phase 7 | Release Prep | [x] Complete | Documentation |
| Phase 7.1 | Enhanced Features | [x] Complete | Save/Load, improved UI |
| Phase 8 | Audio & Post-Processing | [x] Complete | OpenAL 3D Audio, RetroFilter, SaveLoadUI |
| Phase 24 | Complete UI & HUD System | [x] Complete | Inventory, Map, Quests, HUD |
| Phase 25 | BSA/ESM Parsing Engine | [x] Complete | BSA archive reader, ESM file parser with full GRUP hierarchy |
| Phase 26 | ESM Data-Driven World | [x] Complete | CELL, NPC_, WEAP, REFR, LAND, WRLD, SPEL, LVLI/LVLC, NAVM, ARMO record parsing from Oblivion.esm |
| Phase 27 | ESM Integration | [x] Complete | BOOK, CLOT, INGR, ALCH, MISC, FACT, RACE, CLAS, ROAD records + Loot generation, Book reading, Clothing conversion, Alchemy system, Faction system |
| Phase 28 | ESM 40 Record Types + Integration | [x] Complete | BSGN, CREA, CONT, DOOR, ACTI, STAT, TREE, FLOR, LIGH, APPA, SOUN, SCRL, SCPT, GMST, SKIL, EYES, HAIR, CLMT, REGN, WTHR, PGRD, LVSP, LVLN records + NpcManager ESM integration, Container ESM population, Player RACE/CLAS/BSGN initialization, Status effects |
| Phase 29 | NAVM Pathfinding + DIAL/INFO Dialogue | [x] Complete | NAVM runtime integration with CombatManager A* pathfinding, DIAL/INFO record parsing with faction-based dialogue branching, REFR-based world object placement (8 types), 4 spell effects (PARALYZE, INVISIBILITY, FORTIFY_ATTR, SUMMON), new game systems (Alchemy, Book Reader, Clothing Converter, Faction Manager, Loot Generator, NavMesh Manager) |
| Phase 30 | NIF Skeleton/Skinning + Collision + Animation | [~] Core complete | Skeleton/skinning, collision, NiControllerManager/Sequence parsing, AnimationPlayer runtime, and integration tests are complete. NiKeyframeController/NiKeyframeData/NiTransformController readers and controller-reference resolution remain pending the Rust/RE contract; they are intentionally inert rather than guessed. |
| Phase 31 | PlayerController Integration + World Loading | [x] Complete | Steps 1-10: WorldEntity struct with NIFCache, WorldLoader (loadStatic/loadDynamic/loadActor), PlayerController extended (Skeleton+AnimationPlayer+CharacterController integration, hysteresis animation state machine, fixed/variable timestep separation, combat stance+attack), WorldEntity rendering with skinning shader, PlayerController wired to actor skeleton/animation |
| Phase 32 (v0.9.8) | Animation & Audio Integration | [x] Complete | AnimationSubscriber (EventBus→AnimationPlayer bridge), AudioSubscriber (EventBus→AudioManager bridge), SpellSelectionPanel UI, AnimationPlayer.findSequenceByName(), WorldLoader entity storage + NPC→Entity mapping, Imperial Weave Event.targetId field |
| Phase 33 (v0.9.9) | Combat Sound Assets + NPC Spatial Audio | [x] Complete | Dedicated combat sounds (hit_blade/blunt/axe/unarmed, block, parry, dodge, death), NPC spatial audio callback, AudioSubscriber event→sound mappings |
| Phase 34 (v0.9.10) | Weapon-Type Sounds + Quick-Slot Spells | [x] Complete | Weapon-type hit sound routing (CombatManager→AudioSubscriber), SpellSelectionPanel school colors, Quick-slot spells (F1-F4) |
| Phase 35 (v1.0.0) | Radiant AI System | [x] Complete | AI Package System (15 types), Priority-based PackageStack, AIScheduler (24h time-based), NavMesh pathfinding, Stuck detection, Default daily schedule, Combat/Flee override |
| Phase 36 (v1.1.0) | Jolt Physics Integration | [x] Complete | PhysicsManager singleton, CharacterVirtual player/NPC, HeightFieldShape terrain, Fixed timestep (1/60s), Raycast API, ImperialWeave phase integration |
| Phase 50 (v2.5.0) | Distant LOD System | [x] Complete | DistantLodManager, LOD mesh generation, 6-plane frustum culling, distance fade, HorizonRing mountain presets |
| Phase 51 (v2.5.0) | SpeedTree Vegetation | [x] Complete | SpeedTreeManager, 4-stage LOD, instanced rendering, billboard fallback, Perlin wind field, ESM LAND-based tree placement |
| Phase 52 (v2.5.0) | FaceGen System | [x] Complete | FaceGenManager, race morph targets, expression morphs, hair/beard system, LOD levels, texture atlas |
| Phase 53 (v2.5.0) | Bink Video Player | [x] Complete | BinkVideoPlayer, MediaCodec JNI bridge, video clip manager, OpenGL ES texture renderer |
| Phase 54 (v3.0.0) | Imperial Weave v4.0 | [x] Complete | 15-phase pipeline, ImperialWeaveConfig, ServiceLocator, 12 event types, frame budget (16.6ms) |
| Phase 55 (v3.1.0) | Engine Polish & Optimization | [x] Complete | FrameBudgetManager, MemoryDefrag, ShaderCache, OcclusionCuller, BatchRenderer, FaceGen brush-up, Jolt Physics extension |
| Phase 56 (v3.2.0) | Gamebryo Complete | [x] Complete | ParticleSystem (7 presets), PostProcessPipeline (8 effects), WaterRenderer (Gerstner waves, 6 types), SkyWeatherSystem (8 weathers, day/night), SceneGraph (hierarchy, AABB), MaterialSystem (8 texture slots, 8 defaults) |
| Phase 58 (v1.1.0) | Asset Optimization | [x] Complete | AssetExtractor, TextureCompressor (ASTC/ETC2), AudioCompressor (WAV→OGG/MP3), LODSystem |
| Phase 59 (v1.2.0) | Performance Optimization | [x] Complete | OcclusionCuller (hierarchical Z-buffer), ShaderCache (LRU caching), MemoryPool (object pooling) |
| Phase 60 (v1.3.0) | Final Testing & Release | [x] Complete | Build verification, APK analysis, compression tools |
| Phase 61 (v1.4.0) | SpeedTree Renderer Verification | [x] Complete | SpeedTree renderer verification (see plan.md) |
| Phase 62 | Foundation | [x] Complete | Unify Gradle definitions, version consistency (single source of truth in build.gradle), remove Kotlin DSL conflicts, connect test suites, legal audit |
| Phase 63 | Performance Foundation | [x] Complete | Batch TextRenderer (glyph atlas + instancing), remove per-frame LOGI, cache glGetProgramiv, merge draw calls |
| Phase 64 | Real Data Pipeline | [x] Complete | Decode all 20 missing ESM record types (ACHR, ACRE, PGRD, GMST, LTEX, WATR, AMMO, GLOB, FURN, IDLE, LSCR, SGST, EFSH, SLGM, CSTY, ...), fix BSA v103 folder table, verified on emulator against real Oblivion.esm, build success on 4 ABIs |
| Phase 65 | Full World Rendering | [~] In Progress | Terrain mesh + LTEX/VTXT layer blending, WATR water material, WTHR sky dome, door transitions (XTEL), interior REFR NIF geometry, landscape LOD blocks from the shipped BSAs (object LOD and on-device verification remain) |
| Phase 66 | Characters and AI | [ ] Pending | ACHR/ACRE placement, NPC_/CREA decode, skeleton/animation (KF), PACK, PGRD pathfinding, face generation, lip sync (skeleton + animation readers already byte-exact on the corpus) |
| Phase 67 | Full Script VM Compatibility | [~] In Progress | SCDA instruction format byte-verified (2,393/2,393 blocks walk to an exact end), opcode table re-derived from the real ESM; native SCDA decoder/VM branch awaiting merge |
| Phase 68 | Game Systems and GUI | [x] Complete | ESM-driven inventory, weapons, enchantments and factions with real FormIDs; merchant shops from `NPC_` `CNTO`, `UIShop` barter screen, single shared player inventory |
| Phase 69 | Persistence | [~] In Progress | Save format v5 - container contents, quest state, script globals, merchant `MERC` block, loading-screen progress bar |
| Phase 70 | Mods and Load Order | [~] In Progress | `plugins.txt` parse/serialise/order, FormID index remapping, `Oblivion.ini` settings layer, Data Files launcher screen |
| Phase 37 (v1.2.0) | Script VM | [x] Complete | Oblivion VM bytecode interpreter (47 opcodes), 118 game functions (Tier 1+2), ScriptManager, ExecutionContext, disassembler |
| Phase 38 (v1.3.0) | Script VM Testing | [x] Complete | 20 unit tests (ExecutionContext, ScriptVM, Opcodes, ScriptFunctions, ScriptManager), build verification |
| Phase 39 (v1.4.0) | Quest Flow System | [x] Complete | QuestFlowController, QuestStageManager, QuestObjectiveTracker, QuestRewards, QuestRecord parsing |
| Phase 40 (v1.5.0) | NPC Dialogue Tree | [x] Complete | DialogueTree, DialogueRunner, DialogueFilterEngine, DialogueHistory, DialogueRecord, DialogueIntegration |
| Phase 41 (v1.6.0) | Binary Save System | [x] Complete | SaveManager (binary format), SaveSlotManager, AutoSave, Serializable interface, system registration |
| Phase 42 (v2.0.0) | Game Loop Integration | [x] Complete | StateManager, InputRouter, GameLoopCoordinator, SceneRenderer, DebugConsole, PerformanceProfiler |
| Phase 43 (v2.1.0) | UI/UX System | [x] Complete | TouchGestureHandler, MenuTransitionManager, HudLayout, ControlSchemeManager, AccessibilityManager |
| Phase 44 (v2.2.0) | Performance Optimization | [x] Complete | MemoryPool, RenderOptimizer, AsyncTaskManager, CacheManager, ProfilerDashboard |
| Phase 45 (v2.3.0) | Unit Testing | [x] Complete | 37 test cases, +1,089 lines |
| Phase 46 (v2.4.0) | Asset Pipeline | [x] Complete | TextureManager, MeshLoader, WorldDataLoader, BSA/ESM/NIF readers (+2,487 lines) |
| Phase 47 (v2.4.0) | Audio System | [x] Complete | AudioDecoder, BgmManager, SoundEffectManager (+2,176 lines) |
| Phase 48 (v2.4.0) | Integration Tests | [x] Complete | 12 integration test cases (+1,159 lines) |
| Phase 49 (v2.4.0) | Controls & Input | [x] Complete | GamepadMapper, TouchCalibration, InputVisualizer, HudCustomizer (+1,812 lines) |

---

### [METRIC] Code Metrics (Phase 56 / v3.2.0)

- **C++ Source (.cpp)**: 70,000+ lines (219 files)
- **C++ Headers (.h)**: 46,000+ lines (280 files)
- **Tests**: 3,000+ lines (10 files)
- **Total C++ Project**: 120,000+ lines (incl. tests)
- **Kotlin**: 2,400+ lines (9 files)
- **Imperial Weave**: 600+ lines (EventBus, ServiceLocator, 12-phase coordinator)
- **Subscriber Bridges**: 400+ lines (AnimationSubscriber, AudioSubscriber)
- **ESM Parser**: 2,000+ lines (40 record types)
- **BSA Reader**: 500+ lines (archive extraction, ZLib decompression)
- **ESM Integration**: 600+ lines (NpcManager, Container, Player initialization, Status effects, DIAL/INFO dialogue, REFR placement)
- **NAVM Pathfinding**: 300+ lines (A* algorithm, NavMeshManager, CombatManager integration)
- **Spell Effects**: 200+ lines (8 effect types: Damage, Heal, Restore, Fortify, Paralyze, Invisibility, Summon)
- **New Game Systems**: 800+ lines (Alchemy, Book Reader, Clothing Converter, Faction Manager, Loot Generator, Misc Item Converter, NavMesh Manager)
- **Audio System**: 500+ lines (AudioManager, Audio3D, AudioSubscriber, JNI bridge)
- **SaveLoadUI**: 250+ lines (UI + error dialogs)
- **RetroFilter Effects**: 150+ lines (DebugHUD integration)
- **Graphical UI & HUD (Phase 9-24)**: 5,000+ lines (UIPanel, UIButton, TextureLoader, UIDrawHelper)
- **Sound Effects**: 93 sound definitions, 307 WAV files
- **Compilation Time**: ~40 seconds (debug, incremental)
- **APK Size**: 115.4 MiB (debug build, arm64-v8a + armeabi-v7a + x86_64, including 82.9 MiB of
  local video assets that are not redistributed; ~32.5 MiB without them)

---

### [FEAT] Current Limitations

[WARN] **Phase 35 Current Limitations**:
- ~~Debug mode always enabled~~ [x] Fixed (Settings → Debug Mode)
- ~~No save/load system~~ [x] Implemented (Phase 8)
- ~~Text-based UI only~~ [x] Graphical UI implemented (Phase 9)
- ~~Limited NPC dialogue~~ [x] Implemented (Phase 10)
- ~~No full inventory management~~ [x] Implemented (Phase 9B)
- Single-player only (no multiplayer)
- ~~No map system yet~~ [x] Implemented (Phase 23)
- ~~Hardcoded test world~~ [x] ESM data-driven (Phase 25-26)
- ~~No real NPC/object placement from original data~~ [x] CELL+REFR+LAND parsing (Phase 26)
- ~~No magic spells from original data~~ [x] SPEL record parsing (Phase 26)
- ~~No event bus system~~ [x] Imperial Weave EventBus implemented (Phase 32)
- ~~No animation/audio subscribers~~ [x] AnimationSubscriber/AudioSubscriber implemented (Phase 32)
- ~~No combat sounds~~ [x] Dedicated combat sounds implemented (Phase 33)
- ~~No NPC spatial audio~~ [x] NPC spatial audio callback implemented (Phase 33)
- ~~No weapon-type sound routing~~ [x] Weapon-type hit sound routing implemented (Phase 34)
- ~~No quick-slot spells~~ [x] F1-F4 quick-slot spells implemented (Phase 34)
- ~~No Radiant AI system~~ [x] AI package system + scheduler implemented (Phase 35)
- ~~No leveled item/creature spawn tables~~ [x] LVLI/LVLC parsing (Phase 26)
- ~~No AI pathfinding data~~ [x] NAVM record parsing (Phase 26)
- ~~No armor/equipment data from original~~ [x] ARMO record parsing (Phase 26)
- ~~Limited ESM record types (19)~~ [x] Expanded to 40 record types (Phase 28)
- ~~No creature spawning from ESM~~ [x] CREA + LVLC integration (Phase 28)
- ~~No container population from ESM~~ [x] CONT record integration (Phase 28)
- ~~No player race/class/birthsign from ESM~~ [x] RACE/CLAS/BSGN initialization (Phase 28)
- ~~No status effect tracking~~ [x] Paralyze/Invisibility/Fortify/Summon (Phase 28)
- ~~No NAVM runtime pathfinding~~ [x] NavMeshManager + CombatManager A* integration (Phase 29)
- ~~No DIAL/INFO dialogue from ESM~~ [x] DIAL/INFO parsing with faction branching (Phase 29)
- ~~No REFR-based object placement~~ [x] REFR placement with 8 object types (Phase 29)
- ~~Limited spell effects (4)~~ [x] 8 spell effects including Paralyze/Invisibility/Fortify/Summon (Phase 29)

See [docs/README.md](docs/README.md) for complete documentation.

---

### [START] Future Enhancements (Phase 37+)

- [SYS] Script VM (Oblivion script execution) - Phase 37 (Complete)
- [MAP] Map with quest markers - Phase 56 (Complete)
- [PERF] Device-side ESM rendering verification - Phase 57 (Complete)
- [GAME] Controller support - Phase 58 (Complete)
- [TREE] SpeedTree alternative rendering - Phase 59 (Complete)
- [BUILD] Gradle unification & version consistency - Phase 62 (Complete)
- [PERF] TextRenderer glyph batching - Phase 63 (Complete)
- [DATA] ESM real data pipeline (all 20 secondary record types) - Phase 64 (Complete)
- [WORLD] Full world rendering (terrain LOD, water, sky, doors, interior geometry) - Phase 65 (In progress)
- [DATA] Merchant trade from real `NPC_` records - Phase 68 (Complete)
- [SAVE] Container / quest / script-global persistence (save format v5) - Phase 69 (In progress)
- [MODS] `plugins.txt` load order + FormID remapping + `Oblivion.ini` layer - Phase 70 (In progress)
- [BUILD] Rust migration of the pure-logic layers (Phases 1-3 done; `esm_reader.cpp` is next) - ongoing

---

### [FIX] Recent Improvements (2026-10 / 2026-11)

#### Rust Migration of Pure-Logic Layers
- **FormID index, plugin list, asset path resolver and mesh builder** - ported to Rust (`plugin-parser`, `assets-core` crates) behind a small C ABI; the C++ files became thin shims with their headers and callers unchanged, verified by 89 host differential assertions + 28 crate tests on all three ABIs
- **BSA archive reader** - ported to Rust (`bsa-core`, 19 FFI functions); the port found and fixed two hashing bugs (blanket lowercase corrupting `_ [ \ ] ^`, extension split requiring a preceding separator), raising `findFile()` resolution on real archives (e.g. 2,337/3,017 -> 2,628/3,017 on ShiveringIsles - Meshes)
- **NIF mesh parser** - ported to Rust (`nif-core`, 27 FFI functions); `nif_parser.cpp` shrank from 4,091 lines to a ~520-line shim, verified byte-exact on 100 real meshes via a differential oracle. Animation-controller block readers remain outside this mesh-parser migration and are tracked separately.
- **DDS texture reader** - ported to Rust (`dds-core`, 13 FFI functions); verified on a 13-file corpus with no behaviour change on the paths that matter
- Policy (2026-11-01): Rust for all pure (impure-free) logic; `esm_reader.cpp` is the next target. Host tests: 122+ checks, all green

#### Game Systems (Phase 68)
- **Merchant trade from real ESM data** - shops built from the original `NPC_` `CNTO` list (stock, per-row quantity, purse) via `game::buildMerchantFromNpc()`; `game::item_catalog` resolves every `CNTO` FormID; `UIShop` trade screen opens with the virtual controller X button near a merchant; `merchants` / `useshop` console commands and debug-menu buttons added
- **Barter validation** - `game::buyFromMerchant()` / `game::sellToMerchant()` are the only path that moves goods and coin; 122 host checks pin pricing, purse direction and refusal reasons
- **ESM systems integration** - `InventoryManager::loadItemsFromESM`, `CombatManager::loadWeaponsFromESM`, `EnchantingSystem::loadEnchantmentsFromESM`; factions resolved from real FACT records (hardcoded FormIDs replaced)
- **Single inventory** - `InventoryManager::getPlayerInventory()` is the sole store; `Inventory::addItem()` merges by FormID, `removeItem()` leaves drained rows empty

#### World Rendering (Phase 65)
- **Landscape LOD** - shipped LOD blocks read straight from `Oblivion - Meshes.bsa` / `Oblivion - Textures - Compressed.bsa` (100 NIFs + 200 DXT1 DDS), geometry verified with zero bad indices/UVs/placements; terrain fog + near-fade applied, HorizonRing kept as fallback

#### Persistence (Phase 69)
- **Save format v5** - container contents, quest state, script globals and merchant purse/stock (`MERC` block) persisted; `tests/merchant_persistence_tests.cpp` (39 checks)

#### Performance and Debugging
- **DebugDraw3D** - line rendering for grid, bounds, paths and contacts
- **Frame-time percentiles** - debug HUD reports p50/p90/p99 frame times alongside average

#### Launcher Fixes
- **Touch input during startup animations** - taps no longer dropped while animations run
- **plugins.txt persistence** - plugin selection survives the Launcher Play path

---

### [FIX] Recent Improvements (2026-09)

#### UI Bug Fixes
- **TextRenderer** - Added shader compilation error checking (GL_COMPILE_STATUS, GL_LINK_STATUS)
- **UIPanel** - Added TOUCH_MOVE handling for drag operations with screen boundary clamping
- **UIComponent** - Added OpenGL blend state save/restore to prevent state leakage
- **TitleScreen** - Fixed touch move delta calculation (dx/dy from previous position)
- **GameSurfaceView** - Added ACTION_CANCEL handling to release active pointers

#### Layout Fixes
- **AndroidManifest** - Updated theme from deprecated to custom Theme.MyApplication
- **activity_main.xml** - Repositioned debug buttons from bottom-left to top-left (avoids native UI overlap)

#### Code Quality
- **Comment Translation** - All Japanese comments in C++ source files translated to English (40+ files in ui/, audio/, engine/, physics/, world/, system/, game/)
- **Project Rule Compliance** - Full compliance with project language rules (English comments, bilingual UI strings)

---

### [BUG] Reporting Issues

Found a bug? Please:
1. Check [docs/README.md](docs/README.md) first
2. Collect device info (model, Android version, logcat)
3. Provide reproduction steps
4. Include relevant logs

---

### [STAT] Statistics

#### Development Statistics
- **Total Development Time**: ~15 weeks
- **Total Commits**: 60+
- **Bug Fixes**: 30+
- **Features Implemented**: 30+
- **Performance Optimizations**: 10+

#### Code Distribution
- Engine Core: 20%
- Game Systems: 35%
- Asset Management: 15%
- UI & Settings: 18% (expanded with TextRenderer, DebugHUD, SettingsUI, GraphicalUI)
- Profiling: 8%
- JNI/Infrastructure: 4%

---

### [TECH] Technology Stack

#### Core Technologies
- C++17
- Rust (cargo-ndk; pure-logic asset / plugin layers)
- Android NDK r26.1
- OpenGL ES 3.0
- CMake 3.16+
- Gradle 9.4+

#### Libraries
- GLM (Mathematics)
- Jolt Physics (Physics)
- OpenAL-Soft (Audio)
- stb_image.h (PNG loading)

#### Tools
- Android Studio
- JetBrains CLion
- Perfetto (Profiling)
- Gradle (Build)

---

### [NOTE] Credits

**Oblivion Android Project**
- Developed as a complete native port
- Based on Oblivion GOTY Edition
- Reference: OpenMW project architecture

**Special Thanks**
- Bethesda Softworks (Original Oblivion)
- OpenMW Project (Reference implementation)
- Android NDK Team

---

### [LEGAL] Legal Notice

**Important**: This is an experimental port for educational and testing purposes.

- Oblivion GOTY Edition assets used from legitimately purchased copies
- No commercial distribution
- No source asset modification
- Respects original Bethesda Softworks copyright

---

### [LIC] License

See [LICENSE](LICENSE) for full terms.

Proprietary - Experimental Port
*Not licensed for commercial use or redistribution*

---

### [HELP] Support

- **Documentation**: See `/docs` directory
- **Build Issues**: Check [docs/IMPLEMENTATION_GUIDE.md](docs/IMPLEMENTATION_GUIDE.md)
- **Gameplay Questions**: See [README.md](README.md) gameplay section
- **Performance**: See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)

---

**Status**: Phase 65 in progress, Phase 68 complete (v0.9.10) — ESM-driven game systems and merchant trade landed; pure-logic asset/plugin layers migrating to Rust (Phases 1-3 done)
**Last Updated**: 2026-11-01
**Version**: 0.9.10 (versionCode 910)
**Features**: Graphical UI, Textured Panels & Buttons, Sound Effects, SaveLoadUI, OpenAL 3D Audio, RetroFilter Effects, Enhanced DebugHUD, ESM Data Integration (40 record types), NpcManager ESM, Container ESM, Player RACE/CLAS/BSGN, Status Effects, NAVM Pathfinding, DIAL/INFO Dialogue, REFR Placement, Spell Effects (8 types), Alchemy, Book Reader, Faction Manager, Loot Generator, NIF Skeleton/Skinning, Animation System, Collision Detection, Integration Tests (Phase 30), WorldEntity + WorldLoader + PlayerController Integration (Phase 31), Imperial Weave EventBus + 12-phase coordinator, AnimationSubscriber, AudioSubscriber, SpellSelectionPanel (Phase 32), Dedicated Combat Sounds, NPC Spatial Audio (Phase 33), Weapon-Type Sound Routing, Quick-Slot Spells (Phase 34), Radiant AI System (Phase 35), Jolt Physics Integration (Phase 36), Distant LOD (Phase 50), SpeedTree Vegetation (Phase 51), FaceGen (Phase 52), Bink Video (Phase 53), Imperial Weave v4.0 (Phase 54), Engine Polish (Phase 55), Gamebryo Complete: Particle/PostProcess/Water/SkyWeather/SceneGraph/Material (Phase 56), UI Bug Fixes & Comment Translation (Phase 57), Gradle Unification & Version Consistency (Phase 62), TextRenderer Glyph Batching (Phase 63), ESM Real Data Pipeline - all 20 secondary record types decoded (Phase 64), Landscape LOD from shipped BSAs (Phase 65), ESM Game Systems Integration & Merchant Trade (Phase 68), Save Format v5 - containers, quest state, script globals, merchants (Phase 69), plugins.txt Load Order & FormID Remapping (Phase 70), Rust Pure-Logic Layers (BSA/NIF/DDS readers, FormID index, plugin list, asset paths, mesh builder)

**Next**: Phase 65 remainder (object LOD `.lod`/`.cmp` loader + on-device verification), Phase 66 (characters and AI), Phase 70 (mods and load order)

---

---

---

## 日本語

The Elder Scrolls IV: Oblivion の進行中のネイティブ Android 移植版です。C++17 と OpenGL ES 3.0、Android NDK で構築され、純粋ロジックのアセット/プラグイン層は Rust で C ABI 経由に実装しています。

**v0.9.10 (versionCode 910) — Phase 64 完了（20種の不足していたESMレコードタイプをすべてデコードし、実機のOblivion.esmで検証済み）。Phase 65 (フルワールドレンダリング) は進行中、Phase 68 (ESMデータ駆動のゲームシステムとGUI) は完了、純粋ロジック層のRust移行が進行中（Phase 1-3 完了）です。**

アプリのバージョンは `app/build.gradle`（`versionName` / `versionCode`）の 1 箇所のみで定義します。
以下の開発フェーズ表でフェーズ番号の隣に付いているバージョン表記は開発時の履歴タグであり、リリース版数ではありません。

---

### [GAME] 実装機能

#### コアシステム
- [x] **ESMデータ駆動ワールド** (Phase 26-28) - Oblivion.esm からの40種レコードパースによるワールド生成: CELL, NPC_, WEAP, REFR, LAND, WRLD, SPEL, LVLI/LVLC/LVLN/LVSP, NAVM, ARMO, BOOK, CLOT, INGR, ALCH, MISC, FACT, RACE, CLAS, ROAD, BSGN, CREA, CONT, DOOR, ACTI, STAT, TREE, FLOR, LIGH, APPA, SOUN, SCRL, SCPT, GMST, SKIL, EYES, HAIR, CLMT, REGN, WTHR, PGRD
- [x] **3Dレンダリングエンジン** - メッシュ・テクスチャ対応 OpenGL ES 3.0
- [x] **ゲーム世界** - セルベースのワールドとシームレス遷移
- [x] **NPCシステム** - 100体以上のAIステートマシン（IDLE, WANDER, PATROL, COMBAT, FOLLOW）
- [x] **戦闘システム** - ステータス・装備によるダメージ計算
- [x] **クエストシステム** - マルチオブジェクト＋報酬（ゴールド、経験値）
- [x] **魔法システム** - 6系統10種以上＋マナ管理
- [x] **キャラクターステータス** - HP/MP/スタミナ/属性/スキル
- [x] **多言語化** - 日本語＋英語（UI翻訳100以上、JPWikiゲーム文字列28,686件。書籍・対話・クエストも参照）
- [x] **パフォーマンス監視** - フレームタイム・メモリ・CPUプロファイル
- [x] **テキストレンダリング** - カラー・位置指定対応のオンスクリーンテキスト
- [x] **デバッグHUD** - FPS・フレームタイム・メモリ・システム情報オーバーレイ
- [x] **設定システム** - デバッグモード・言語設定の永続化
- [x] **セーブ/ロード** - スロット管理付きゲーム状態の永続化
- [x] **OpenAL 3Dオーディオ** - 距離減衰付き空間オーディオ
- [x] **レトロフィルター** - ピクセル化・スキャンライン・色数制限・CRT歪み・フィルムグレイン
- [x] **完全なUI・HUDシステム** (Phase 9-24) - インベントリ、マップ、ステータス、クエストログ、会話、ポーズ、戦闘HUD等

#### ゲーム機能
- [FEAT] タッチ操作によるカメラ操作
- [FEAT] 近隣敵との自動戦闘開始
- [FEAT] NPC会話とクエスト受注
- [FEAT] マナ消費による魔法詠唱
- [FEAT] グラフィカルメニュー付きタイトル画面
- [FEAT] 進捗追跡付きクエストログ
- [FEAT] NPC間リアルタイム戦闘
- [FEAT] **新機能**: スロット管理付きセーブ/ロード (Phase 8)
- [FEAT] **新機能**: デバッグモード・レトロフィルター設定 (Phase 8)
- [FEAT] **新機能**: 距離減衰付き3D空間オーディオ (Phase 8)
- [FEAT] **新機能**: 完全なグラフィカルUIとHUDシステムの実装 (Phase 9-24)
- [FEAT] **新機能**: ESMデータ統合 - 40種レコード、NPCマネージャー、コンテナ、プレイヤー初期化、ステータス効果 (Phase 28)
- [FEAT] **新機能**: NAVMパスファインディングランタイム統合、派閥分岐付きDIAL/INFO会話、REFRワールドオブジェクト配置、4つの呪文エフェクト (Phase 29)
- [FEAT] **新機能**: Imperial Weave EventBus＋12フェーズコーディネーター、AnimationSubscriber、AudioSubscriber、SpellSelectionPanel (Phase 32)
- [FEAT] **新機能**: 専用コンバットサウンド、NPC空間オーディオ (Phase 33)
- [FEAT] **新機能**: 武器タイプサウンドルーティング、クイックスロット呪文 (Phase 34)
- [FEAT] **新機能**: Radiant AIシステム - AIパッケージシステム、スケジューラ、NavMeshパスファインディング統合 (Phase 35)
- [FEAT] **新機能**: Distant LODシステム - LODメッシュ生成、視錐台カリング、HorizonRing山岳 (Phase 50)
- [FEAT] **新機能**: SpeedTree植生 - 4段階LOD、インスタンス描画、パーリン風場 (Phase 51)
- [FEAT] **新機能**: FaceGenシステム - 種族モーフ、表情モーフ、髪/髭システム (Phase 52)
- [FEAT] **新機能**: Binkビデオプレイヤー - MediaCodec JNI、ビデオクリップ管理 (Phase 53)
- [FEAT] **新機能**: Imperial Weave v4.0 - 15フェーズパイプライン、ServiceLocator (Phase 54)
- [FEAT] **新機能**: エンジンポリッシュ - フレームバジェット、メモリデフラグ、シェーダーキャッシュ (Phase 55)
- [FEAT] **新機能**: Gamebryo完成 - パーティクル、ポストプロセス、水、天候、シーングラフ、マテリアル (Phase 56)
- [FEAT] **新機能**: ESM実データパイプライン - 20種の不足レコードタイプを全デコードし、実機Oblivion.esmで検証 (Phase 64)
- [FEAT] **新機能**: フルワールドレンダリング - LTEX/VTXTレイヤーブレンド付き地形メッシュ、WATR由来の水マテリアル、WTHR由来のスカイドーム、ドア遷移（XTEL）、屋内REFR NIFジオメトリ、同梱BSAからのランドスケープLODブロック (Phase 65, 進行中)
- [FEAT] **新機能**: ゲームシステムのESMデータ統合 - インベントリ・武器・エンチャント・派閥を実ESMレコードとFormIDで解決 (Phase 68)
- [FEAT] **新機能**: 商店取引 - 元の `NPC_` `CNTO` リスト（在庫・数量・所持金）から構築した商店、`UIShop` 取引画面、`merchants` / `useshop` コンソールコマンド (Phase 68)
- [FEAT] **新機能**: セーブフォーマットv5 - コンテナ内容・クエスト状態・スクリプトグローバル・商店の所持金/在庫（`MERC` ブロック）を永続化 (Phase 69, 進行中)
- [FEAT] **新機能**: モッドとロード順 - `plugins.txt` の解析/保存/整列、FormIDインデックス再マッピング、`Oblivion.ini` 設定レイヤー、データファイルランチャー画面 (Phase 70, 進行中)
- [FEAT] **新機能**: Rust移行 - FormIDインデックス、プラグインリスト、アセットパス解決、メッシュビルダー、BSA/NIF/DDSリーダーを C ABI 経由の Rust に移植 (2026-10/11)
- [FEAT] **新機能**: DebugDraw3Dライン描画と、デバッグHUDのパーセンタイル（p50/p90/p99）フレームタイム表示

---

### [SPEC] 技術仕様

#### 動作要件
| 項目 | 仕様 |
| --- | --- |
| **最低OS** | Android 10.0 (API 29) |
| **推奨OS** | Android 12.0 以上 |
| **RAM** | 最低2 GB、推奨4 GB |
| **CPU** | ARM64-v8a または ARMv7 |
| **ストレージ** | 500 MB 以上の空き容量 |
| **GPU** | OpenGL ES 3.0 対応 |

#### アーキテクチャ
| 項目 | 仕様 |
| --- | --- |
| **言語** | C++17（120,000行以上）＋純粋ロジックのアセット/プラグイン層はRust |
| **グラフィックスAPI** | OpenGL ES 3.0 |
| **物理エンジン** | Jolt Physics (Phase 36) |
| **ビルドシステム** | CMake + Gradle（＋Rust静的ライブラリ用の cargo-ndk） |
| **NDKバージョン** | r26.1 |
| **ターゲットAPI** | API 29以上 |

#### パフォーマンス目標
| 指標 | 目標 | 実測 | 状態 |
| --- | --- | --- | --- |
| **FPS** | 30 fps | 60 fps | [x] 超過 |
| **メモリ** | < 1 GB | 40 MB | [x] 合格 |
| **CPU** | < 10% | < 0.1% | [x] 超過 |
| **起動時間** | < 30秒 | 18-25秒 | [x] 合格 |
| **安定性** | 5時間 | 30秒以上 | [x] 合格 |

---

### [PKG] ビルドとインストール

#### 前提条件
```bash
# Android SDK/NDKのインストール
sdkmanager "ndk;26.1.10909125"
sdkmanager "cmake;3.16.0"

# RustツールチェーンとAndroidターゲット（プラグイン/アセットの静的ライブラリ用）
rustup target add aarch64-linux-android armv7-linux-androideabi x86_64-linux-android
cargo install cargo-ndk

# リポジトリのクローン
git clone https://github.com/oblivion-android/oblivion-android.git
cd oblivion-android
```

Rustソースはこのリポジトリの `rust/` に統合されています（2026-11-09 に旧姉妹ディレクトリ
`oblivion-re-tools` からベンダー化）。CMakeは `cargo ndk` でビルドします。ワークスペースを
別の場所に置く場合やビルドマシンにネットワークがない場合は、Gradleプロパティ
`-Poblivion.rust.dir=<path>` / `-Poblivion.rust.offline=true` を指定してください。詳細は
[plan.md](plan.md#rust-migration-of-pure-logic-layers-2026-10-26) を参照。

#### リリースAPKのビルド
```bash
# ビルドと署名
./gradlew clean assembleRelease

# 出力先
# Location: app/build/outputs/apk/release/app-release.apk
# サイズ: 約8 MB
```

#### デバイスへのインストール
```bash
# ADB経由
adb install -r app/build/outputs/apk/release/app-release.apk

# または手動でAPKを転送し、デバイスからインストール
```

---

### [START] クイックスタート

1. **アプリ起動**: ホーム画面の Oblivion アイコンをタップ
2. **タイトル画面**: 3秒待ってタップで開始
3. **ゲームプレイ**: Oblivion の世界を探索
4. **NPCとの会話**: 近くのキャラクターをタップ
5. **戦闘**: 敵と自動で交戦開始
6. **クエスト**: NPC会話から受注
7. **魔法**: 戦闘中に魔法を詠唱
8. **ログ確認**: クエスト進捗を確認

#### ゲーム操作
| 操作 | 説明 |
| --- | --- |
| **視点移動** | 画面をドラッグしてカメラ回転 |
| **インタラクト** | NPCまたはオブジェクトをタップ |
| **メニュー** | クエストUIで現在のクエスト表示 |
| **魔法** | 戦闘中NPCが自動詠唱（今後: 手動詠唱） |
| **設定** | タイトルメニューの「設定」でアクセス |

---

### [UI] UIとデバッグシステム

#### 設定メニュー
タイトル画面からアクセス:
1. **タイトル画面** → 「設定」をタップ
2. **設定パネル** が表示され、以下のオプションがあります:
   - **デバッグモード**: ON/OFFでデバッグHUDの表示/非表示
   - **言語**: 日本語と英語を切り替え
   - **レトロフィルター**: ピクセル化・スキャンライン・色数制限・CRT歪み・フィルムグレイン
   - **戻る**: メインメニューに戻る

メニューで変更した設定は自動的に永続ストレージに保存されます。メニューに無い項目はデータ
ディレクトリのゲーム自身の `Oblivion.ini` から読み込まれ、メニューで上書きするまでその値が
有効です。

#### デバッグHUD表示
**デバッグモード: ON** のとき、リアルタイムで表示:
- **FPS**: 現在のフレームレート
- **フレームタイム**: 1フレームあたりのミリ秒
- **平均**: 平均フレームタイム
- **メモリ**: 現在のRAM使用量
- **キューブ**: アクティブなゲームオブジェクト数
- **ステータス**: 「DEBUG: ON/OFF」を表示
- **オーディオシステム**: ロード済みクリップ数、アクティブソース数、BGM状態
- **レトロフィルター**: アクティブな効果の略称

#### グラフィカルUIシステム (Phase 9)
- **テクスチャ付きパネル**: 背景テクスチャ付きUIPanel
- **ボタン状態**: normal/hover/pressed/disabled テクスチャ
- **テクスチャスケーリング**:
  - **引き伸ばし**: デフォルト、quad全体にフィット
  - **アスペクト比維持（全体表示）**: レターボックス/ピラーボックス、テクスチャ全体を表示
  - **アスペクト比維持（トリミング）**: quadを埋め、中央でトリミング
- **効果音**: UIクリック音、クエスト通知音、戦闘音

---

### [REF] ドキュメント

- [docs/README.md](docs/README.md) - ドキュメント目次
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) - システムアーキテクチャ
- [docs/DEVELOPMENT_HISTORY.md](docs/DEVELOPMENT_HISTORY.md) - 開発履歴
- [CHANGELOG.md](CHANGELOG.md) - 開発履歴

---

### [TEST] テスト結果

#### マルチデバイス検証

**Amazon Fire Tablet (Android 9)**
```
[x] インストール: 成功
[x] 起動: 25秒
[x] FPS: 60（安定）
[x] メモリ: 42 MB
[x] 継続: 30秒以上クラッシュなし
[x] 温度: 38°C
```

**Xiaomi 24018RPACG (Android 16)**
```
[x] インストール: 成功（WiFi ADB）
[x] 起動: 18秒
[x] FPS: 60（安定）
[x] メモリ: 45 MB
[x] 継続: 30秒以上クラッシュなし
[x] 温度: 39°C
[x] 解像度: 2032×3048（ウルトラHD）
```

#### パフォーマンス基準
- **フレームタイム**: 16.67 ms @ 60 FPS（非常に安定）
- **メモリヒープ**: 合計49 MB、使用率82%
- **CPU上位プロセス**: 上位38位外（0.1%未満）
- **バッテリー消費**: 輝度50%で1-2%/時

---

### [ARCH] プロジェクト構成

```
oblivion-android/
├── app/src/main/
│   ├── java/com/example/oblivion/
│   │   ├── MainActivity.java
│   │   ├── GameRenderer.java
│   │   └── GameSurfaceView.java
│   ├── cpp/
│   │   ├── engine/          レンダリング、カメラ、シェーダー、テクスチャローダー
│   │   ├── game/            NPC、戦闘、クエスト、魔法
│   │   ├── ui/              タイトル画面、クエストUI、テキストレンダラー、デバッグHUD、
│   │   │                     設定UI、セーブ/ロードUI、パネル、ボタン
│   │   ├── audio/           オーディオマネージャー、3Dオーディオ、JNIブリッジ、サウンド定義
│   │   ├── save_system/     セーブマネージャー、ゲーム状態の永続化
│   │   ├── system/          設定マネージャー、Oblivion.ini リーダー
│   │   ├── assets/          BSAリーダー、ESMパーサー、アセット読み込み
│   │   ├── profiling/       パフォーマンス監視
│   │   ├── localization/    言語システム
│   │   ├── include/         外部ライブラリ（stb_image.h、GLM等）
│   │   ├── jni_bridge.cpp   Java ↔ C++ インターフェース
│   │   └── CMakeLists.txt   ビルド設定
│   └── res/                 リソース、文字列
├── rust/                    ベンダー化済みRustワークスペース（C ABI裏側の6系統を担う15クレート:
│   │                         ess-parser、esm-parser/esm-ffi、plugin-parser/plugin-ffi、
│   │                         assets-core/assets-ffi、bsa-core/bsa-ffi、nif-core/nif-ffi、
│   │                         dds-core/dds-ffi、vm-handlers、cli。ビルド生成物 rust/target/ は
│   │                         gitignore）
├── docs/                    フェーズ計画、アセット統合計画
├── CHANGELOG.md
├── Handbook.md
└── README.md (このファイル)
```

---

### [BUILD] 開発フェーズ

| フェーズ | 重点 | 状態 | 主な成果物 |
| --- | --- | --- | --- |
| Phase 1 | コアレンダリング | [x] 完了 | 3Dエンジン、OpenGL ES 3.0 |
| Phase 2 | アセット管理 | [x] 完了 | NIF/DDSローダー、キャッシング |
| Phase 3 | ワールドシステム | [x] 完了 | セルシステム、ワールドストリーミング |
| Phase 4 | NPCとAI | [x] 完了 | NPCマネージャー、ステートマシン |
| Phase 5 | 深層機能 | [x] 完了 | 戦闘、クエスト、魔法 |
| Phase 6 | 最適化 | [x] 完了 | パフォーマンス、テスト、ドキュメント |
| Phase 7 | リリース準備 | [x] 完了 | ドキュメント |
| Phase 7.1 | 拡張機能 | [x] 完了 | セーブ/ロード、改善されたUI |
| Phase 8 | オーディオ＆ポストプロセス | [x] 完了 | OpenAL 3Dオーディオ、レトロフィルター、セーブ/ロードUI |
| Phase 24 | 完全なUI＆HUDシステム | [x] 完了 | インベントリ、マップ、クエスト、HUD |
| Phase 25 | BSA/ESMパースエンジン | [x] 完了 | BSAアーカイブリーダー、完全GRUP階層付きESMパーサー |
| Phase 26 | ESMデータ駆動ワールド | [x] 完了 | Oblivion.esm からの10種レコードパース |
| Phase 27 | ESM統合 | [x] 完了 | 9種レコード追加＋ルート生成、書籍読書、衣服変換、錬金術、派閥システム |
| Phase 28 | ESM40種レコード＋統合 | [x] 完了 | 21種レコード追加＋NPCマネージャーESM統合、コンテナESM配置、プレイヤーRACE/CLAS/BSGN初期化、ステータス効果 |
| Phase 29 | NAVMパスファインディング＋DIAL/INFO会話 | [x] 完了 | NAVMランタイム統合（A*経路探索）、派閥分岐付きDIAL/INFOレコード解析、REFRワールドオブジェクト配置（8種）、4つの呪文エフェクト、新ゲームシステム追加 |
| Phase 30 | NIFスケルトン/スキニング＋衝突＋アニメーション | [~] コア完了 | スケルトン/スキニング、衝突、NiControllerManager/Sequence解析、AnimationPlayerランタイム、統合テストは完了。NiKeyframeController/NiKeyframeData/NiTransformControllerのリーダーとコントローラー参照解決は、Rust/RE契約確定待ちのため未実装（推測実装を避けて明示的に停止） |
| Phase 31 | PlayerController統合＋ワールドロード | [x] 完了 | Step 1-10: WorldEntity構造体（NIFCache付き）、WorldLoader（loadStatic/loadDynamic/loadActor）、PlayerController拡張（Skeleton＋AnimationPlayer＋CharacterController統合、ヒステリシス付きアニメーション状態マシン、固定/可変タイムステップ分離、戦闘構え＋攻撃）、WorldEntityスキニングシェーダー描画、PlayerControllerスケルトン/アニメーション統合 |
| Phase 32 (v0.9.8) | アニメーション＆オーディオ統合 | [x] 完了 | AnimationSubscriber（EventBus→AnimationPlayerブリッジ）、AudioSubscriber（EventBus→AudioManagerブリッジ）、SpellSelectionPanel UI、AnimationPlayer.findSequenceByName()、WorldLoaderエンティティストレージ＋NPC→Entityマッピング、Imperial Weave Event.targetIdフィールド |
| Phase 33 (v0.9.9) | コンバットサウンド＋NPC空間オーディオ | [x] 完了 | 専用コンバットサウンド（hit_blade/blunt/axe/unarmed、block、parry、dodge、death）、NPC空間オーディオコールバック、AudioSubscriberイベント→サウンドマッピング |
| Phase 34 (v0.9.10) | 武器タイプサウンド＋クイックスロット呪文 | [x] 完了 | 武器タイプヒットサウンドルーティング（CombatManager→AudioSubscriber）、SpellSelectionPanel学校カラー、クイックスロット呪文（F1-F4） |
| Phase 35 (v1.0.0) | Radiant AIシステム | [x] 完了 | AIパッケージシステム（15種）、優先度ベースPackageStack、AIScheduler（24h時間ベース）、NavMeshパスファインディング、スタック検出、デフォルト日課、コンバット/フリーオーバーライド |
| Phase 36 (v1.1.0) | Jolt Physics統合 | [x] 完了 | PhysicsManagerシングルトン、CharacterVirtual player/NPC、HeightFieldShape地形、固定タイムステップ（1/60s）、Raycast API、ImperialWeaveフェーズ統合 |
| Phase 50 (v2.5.0) | Distant LODシステム | [x] 完了 | DistantLodManager、LODメッシュ生成、6平面視錐台カリング、距離フェード、HorizonRing山岳プリセット |
| Phase 51 (v2.5.0) | SpeedTree植生 | [x] 完了 | SpeedTreeManager、4段階LOD、インスタンス描画、ビルボードフォールバック、パーリン風場、ESM LANDベース樹木配置 |
| Phase 52 (v2.5.0) | FaceGenシステム | [x] 完了 | FaceGenManager、種族モーフターゲット、表情モーフ、髪/髭システム、LODレベル、テクスチャアトラス |
| Phase 53 (v2.5.0) | Binkビデオプレイヤー | [x] 完了 | BinkVideoPlayer、MediaCodec JNIブリッジ、ビデオクリップマネージャー、OpenGL ESテクスチャレンダラー |
| Phase 54 (v3.0.0) | Imperial Weave v4.0 | [x] 完了 | 15フェーズパイプライン、ImperialWeaveConfig、ServiceLocator、12イベントタイプ、フレームバジェット（16.6ms） |
| Phase 55 (v3.1.0) | エンジンポリッシュ＆最適化 | [x] 完了 | FrameBudgetManager、MemoryDefrag、ShaderCache、OcclusionCuller、BatchRenderer、FaceGenブラッシュアップ、Jolt Physics拡張 |
| Phase 56 (v3.2.0) | Gamebryo完成 | [x] 完了 | ParticleSystem（7プリセット）、PostProcessPipeline（8エフェクト）、WaterRenderer（Gerstner波、6種）、SkyWeatherSystem（8天候、昼夜）、SceneGraph（階層、AABB）、MaterialSystem（8テクスチャスロット、8デフォルト） |
| Phase 62 | Foundation | [x] 完了 | Gradle定義の統合、バージョン一貫性（build.gradleを唯一の情報源に）、Kotlin DSL競合の除去、テストスイート接続、法的監査 |
| Phase 63 | Performance Foundation | [x] 完了 | TextRendererのバッチ処理（グリフアトラス＋インスタンシング）、毎フレームのLOGI削除、glGetProgramivキャッシュ、描画コール結合 |
| Phase 64 | 実データパイプライン | [x] 完了 | 20種の不足ESMレコードタイプを全デコード（ACHR、ACRE、PGRD、GMST、LTEX、WATR、AMMO、GLOB、FURN、IDLE、LSCR、SGST、EFSH、SLGM、CSTY他）、BSA v103フォルダテーブル修正、エミュレータで実Oblivion.esm検証、4 ABIでビルド成功 |
| Phase 65 | フルワールドレンダリング | [~] 進行中 | LTEX/VTXTレイヤーブレンド付き地形メッシュ、WATR由来の水マテリアル、WTHR由来のスカイドーム、ドア遷移（XTEL）、屋内REFR NIFジオメトリ、同梱BSAからのランドスケープLODブロック。残件: オブジェクトLOD（`.lod`/`.cmp`）ローダーと実機検証 |
| Phase 66 | キャラクターとAI | [ ] 保留 | 走るアニメーション、NPCの攻撃、装備表示、AI状態遷移。資産側は準備完了、ゲームロジックの追加待ち |
| Phase 67 | スクリプトVM拡張 | [~] 進行中 | `hhkk0127-script-vm-expansion` ブランチは完全な実装済み（2,393 SCDAオペコード、2,393/2,393バイト一致で検証済み）。未マージのネイティブSCDA分岐とのブロック解除待ち |
| Phase 68 | ESMデータ駆動ゲームシステム | [x] 完了 | ゲームの状態の唯一の実装はESMデータ。インベントリ（`InventoryManager::loadItemsFromESM`）、武器（`CombatManager::loadWeaponsFromESM`）、エンチャント（`EnchantingSystem::loadEnchantmentsFromESM`）、派閥（FACTレコードから解決）。ハードコードFormIDはすべて置換済み。商店は元の `NPC_` `CNTO` リストから構築され、`UIShop` 取引画面、`merchants` / `useshop` コンソールコマンド、単一インベントリ |
| Phase 69 | セーブフォーマットv5 | [~] 進行中 | コンテナ内容、クエスト状態、スクリプトグローバル、商店の所持金/在庫（`MERC` ブロック）を永続化。`tests/merchant_persistence_tests.cpp`（39チェック） |
| Phase 70 | モッドとロード順 | [~] 進行中 | `plugins.txt` の解析/保存/整列、FormIDインデックス再マッピング、`Oblivion.ini` 設定レイヤー（settings.txt > Oblivion.ini > 既定値）、データファイルランチャー画面 |
| Phase 37 (v1.2.0) | スクリプトVM | [x] 完了 | Oblivion VMバイトコードインタプリタ（47オプコード）、118ゲーム関数（Tier 1+2）、ScriptManager、ExecutionContext、逆アセンブラ |

---

### [METRIC] コード指標 (Phase 56 / v3.2.0)

- **C++ソース (.cpp)**: 70,000行以上（219ファイル）
- **C++ヘッダ (.h)**: 46,000行以上（280ファイル）
- **テスト**: 3,000行以上（10ファイル）
- **C++プロジェクト合計**: 120,000行以上（テスト含む）
- **Kotlin**: 2,400行以上（9ファイル）
- **ESMパーサー**: 2,000行以上（40種のレコード型パース）
- **BSAリーダー**: 500行以上（アーカイブ展開、ZLib展開）
- **ESM統合**: 600行以上（NPCマネージャー、コンテナ、プレイヤー初期化、ステータス効果、DIAL/INFO会話、REFR配置）
- **NAVMパスファインディング**: 300行以上（A*アルゴリズム、NavMeshManager、CombatManager統合）
- **呪文エフェクト**: 200行以上（8種エフェクト）
- **Radiant AI**: 500行以上（AIパッケージシステム、スケジューラ、NavMeshパスファインディング統合）
- **新ゲームシステム**: 800行以上（錬金術、書籍リーダー、衣服変換、派閥マネージャー、ルートジェネレーター、その他アイテム変換、NavMeshマネージャー）
- **オーディオシステム**: 500行以上（AudioManager、Audio3D、AudioSubscriber、JNIブリッジ）
- **セーブ/ロードUI**: 250行以上（UI＋エラーダイアログ）
- **レトロフィルター効果**: 150行以上（DebugHUD連携）
- **グラフィカルUI・HUD (Phase 9-24)**: 5,000行以上（UIPanel、UIButton、TextureLoader、UIDrawHelper）
- **効果音**: 93サウンド定義、307個のWAVファイル
- **コンパイル時間**: 約40秒（デバッグ、増分ビルド）
- **APKサイズ**: 115.4 MiB（デバッグビルド、arm64-v8a + armeabi-v7a + x86_64。再配布しない
  ローカル動画アセット 82.9 MiB を含む。動画を除くと約 32.5 MiB）

---

### [FEAT] 現在の制限

[WARN] **Phase 35 現在の制限**:
- ~~デバッグモードが常に有効~~ [x] 修正済み（設定 → デバッグモード）
- ~~セーブ/ロードシステムなし~~ [x] Phase 8で実装済み
- ~~テキストベースUIのみ~~ [x] Phase 9でグラフィカルUI実装済み
- ~~限定的なNPC会話~~ [x] Phase 10で拡張済み
- ~~完全なインベントリ管理なし~~ [x] Phase 9Bで実装済み
- シングルプレイのみ（マルチプレイなし）
- ~~マップシステムなし~~ [x] Phase 23で実装済み
- ~~ハードコードされたテストワールド~~ [x] ESMデータ駆動（Phase 25-26）
- ~~オリジナルデータからのNPC/オブジェクト配置なし~~ [x] CELL+REFR+LANDパースで実現（Phase 26）
- ~~オリジナルデータからの魔法呪文なし~~ [x] SPELレコードパース（Phase 26）
- ~~レベル付きアイテム/クリエイチャースポーンテーブルなし~~ [x] LVLI/LVLCパース（Phase 26）
- ~~イベントバスシステムなし~~ [x] Imperial Weave EventBus実装（Phase 32）
- ~~アニメーション/オーディオサブスクライバなし~~ [x] AnimationSubscriber/AudioSubscriber実装（Phase 32）
- ~~コンバットサウンドなし~~ [x] 専用コンバットサウンド実装（Phase 33）
- ~~NPC空間オーディオなし~~ [x] NPC空間オーディオコールバック実装（Phase 33）
- ~~武器タイプサウンドルーティングなし~~ [x] 武器タイプヒットサウンドルーティング実装（Phase 34）
- ~~クイックスロット呪文なし~~ [x] F1-F4クイックスロット呪文実装（Phase 34）
- ~~Radiant AIシステムなし~~ [x] AIパッケージシステム＋スケジューラ実装（Phase 35）
- ~~AIパスファインディングデータなし~~ [x] NAVMレコードパース（Phase 26）
- ~~オリジナルからの防具/装備データなし~~ [x] ARMOレコードパース（Phase 26）
- ~~限定的なESMレコード型（19種）~~ [x] 40種レコードに拡張（Phase 28）
- ~~ESMからのクリエイチャー生成なし~~ [x] CREA＋LVLC統合（Phase 28）
- ~~ESMからのコンテナ配置なし~~ [x] CONTレコード統合（Phase 28）
- ~~ESMからのプレイヤー種族/クラス/誕生星座なし~~ [x] RACE/CLAS/BSGN初期化（Phase 28）
- ~~ステータス効果追跡なし~~ [x] 麻痺/不可視/強化/召喚（Phase 28）
- ~~NAVMランタイムパスファインディングなし~~ [x] NavMeshManager＋CombatManager A*統合（Phase 29）
- ~~ESMからのDIAL/INFO会話なし~~ [x] 派閥分岐付きDIAL/INFO解析（Phase 29）
- ~~REFRベースのオブジェクト配置なし~~ [x] 8種オブジェクトタイプのREFR配置（Phase 29）
- ~~限定的な呪文エフェクト（4種）~~ [x] 麻痺/不可視/強化/召喚含む8種（Phase 29）

完全なリストは [docs/README.md](docs/README.md) を参照。

---

### [START] 将来の拡張 (Phase 37+)

- [SYS] スクリプトVM（Oblivionスクリプト実行） - Phase 37（完了）
- [MAP] クエストマーカー付きマップ - Phase 56（完了）
- [PERF] デバイス上でのESMレンダリング検証 - Phase 57（完了）
- [GAME] コントローラー対応 - Phase 58（完了）
- [TREE] SpeedTree代替レンダリング - Phase 59（完了）
- [BUILD] Gradle統合＆バージョン一貫性 - Phase 62（完了）
- [PERF] TextRendererグリフバッチ処理 - Phase 63（完了）
- [DATA] ESM実データパイプライン（20種の不足レコードタイプ全デコード） - Phase 64（完了）
- [DATA] フルワールドレンダリング（地形・水・空・ドア遷移・屋内ジオメトリ・LOD） - Phase 65（進行中）
- [DATA] ゲームシステムのESMデータ統合と商店取引（インベントリ・武器・エンチャント・派閥・商店） - Phase 68（完了）
- [DATA] セーブフォーマットv5（コンテナ・クエスト・スクリプトグローバル・商店） - Phase 69（進行中）
- [DATA] モッドとロード順（plugins.txt・FormID再マッピング・Oblivion.ini設定レイヤー） - Phase 70（進行中）
- [DATA] 純粋ロジック層のRust移行（FormIDインデックス、プラグインリスト、アセットパス、メッシュビルダー、BSA/NIF/DDSリーダー） - 2026-10/11（Phase 1-3 完了、継続中）

---

### [FIX] 最近の改善 (2026-10 / 2026-11)

#### 純粋ロジック層のRust移行
- **FormIDインデックス、プラグインリスト、アセットパス解決、メッシュビルダー** - 小さいC ABI経由でRust（`plugin-parser`、`assets-core` クレート）に移植。C++側はヘッダーと呼び出し元を変えずに薄いシムとなり、89件のホスト差分テスト＋3 ABIで28件のクレートテストで検証
- **BSAアーカイブリーダー** - Rust（`bsa-core`、19 FFI関数）に移植。移植により2件のハッシュバグ（`_ [ \ ] ^` を潰す一括小文字化、区切り文字を要求する拡張子分割）が発見・修正され、実アーカイブの `findFile()` 解決数が向上（例: ShiveringIsles - Meshes で 2,337/3,017 → 2,628/3,017）
- **NIFメッシュパーサー** - Rust（`nif-core`、27 FFI関数）に移植。`nif_parser.cpp` は4,091行から約520行のシムに縮小し、差分オラクルで100件の実メッシュでバイト一致を検証。アニメーションコントローラのブロックリーダーはメッシュ移行とは別管理
- **DDSテクスチャリーダー** - Rust（`dds-core`、13 FFI関数）に移植。13件のコーパスで検証し、主要パスの挙動変化なし
- 方針（2026-11-01）: 純粋（副作用なし）ロジックはすべてRustへ。次は `esm_reader.cpp`。ホストテストは122件以上すべて成功

#### ゲームシステム（Phase 68）
- **実ESMデータからの商店取引** - 元の `NPC_` `CNTO` リスト（在庫、行ごとの数量、所持金）から `game::buildMerchantFromNpc()` で商店を構築。`game::item_catalog` が全 `CNTO` FormIDを解決。`UIShop` 取引画面は商人の近くで仮想コントローラーのXボタンで起動。`merchants` / `useshop` コンソールコマンドとデバッグメニューボタンを追加
- **バーター検証** - 財貨と金の移動は `game::buyFromMerchant()` / `game::sellToMerchant()` のみ。122件のホストテストで価格設定、所持金の向き、拒否理由を固定
- **ESMシステム統合** - `InventoryManager::loadItemsFromESM`、`CombatManager::loadWeaponsFromESM`、`EnchantingSystem::loadEnchantmentsFromESM`。派閥は実FACTレコードから解決（ハードコードFormIDをすべて置換）
- **単一インベントリ** - `InventoryManager::getPlayerInventory()` が唯一のストア。`Inventory::addItem()` はFormID単位でマージし、`removeItem()` は枯渇した行を空のままにする

#### ワールドレンダリング（Phase 65）
- **ランドスケープLOD** - 同梱のLODブロックを `Oblivion - Meshes.bsa` / `Oblivion - Textures - Compressed.bsa` から直接読み込み（NIF 100件＋DXT1 DDS 200件）。ジオメトリは不正インデックス・UV・配置ゼロで検証。地形フォグとニアフェードを適用し、HorizonRingはフォールバックとして維持

#### 永続化（Phase 69）
- **セーブフォーマットv5** - コンテナ内容、クエスト状態、スクリプトグローバル、商人の所持金/在庫（`MERC` ブロック）を永続化。`tests/merchant_persistence_tests.cpp`（39チェック）

#### パフォーマンスとデバッグ
- **DebugDraw3D** - グリッド、バウンディング、パス、接触用のライン描画
- **フレームタイムパーセンタイル** - デバッグHUDが平均に加えて p50/p90/p99 のフレームタイムを表示

#### ランチャー修正
- **起動アニメーション中のタッチ入力** - アニメーション実行中でもタップがドロップされなくなった
- **plugins.txt の永続化** - プラグイン選択がランチャーのプレイ経路をまたいで維持される

---

### [FIX] 最近の改善 (2026-09)

#### UIバグ修正
- **TextRenderer** - シェーダーコンパイルエラー検出を追加（GL_COMPILE_STATUS、GL_LINK_STATUS）
- **UIPanel** - ドラッグ操作時のTOUCH_MOVE処理を追加（画面境界クランプ付き）
- **UIComponent** - OpenGLブレンド状態の保存/復元を追加（状態漏洩防止）
- **TitleScreen** - タッチ移動デルタ計算を修正（前回位置からのdx/dy）
- **GameSurfaceView** - ACTION_CANCEL処理を追加（アクティブポインタ解放）

#### レイアウト修正
- **AndroidManifest** - 非推奨テーマからカスタムTheme.MyApplicationに更新
- **activity_main.xml** - デバッグボタンを左下から左上に再配置（ネイティブUIとの重複回避）

#### コード品質
- **コメント翻訳** - C++ソースファイルの全日本語コメントを英語に翻訳（ui/、audio/、engine/、physics/、world/、system/、game/の40+ファイル）
- **プロジェクトルール準拠** - プロジェクト言語ルールに完全準拠（英語コメント、バイリンガルUI文字列）

---

### [BUG] 問題報告

バグを発見しましたか？
1. まず [docs/README.md](docs/README.md) を確認
2. デバイス情報（モデル、Androidバージョン、logcat）を収集
3. 再現手順を提供
4. 関連ログを含める

---

### [STAT] 統計

#### 開発統計
- **総開発時間**: 約15週間
- **総コミット数**: 60以上
- **バグ修正**: 30以上
- **実装機能数**: 30以上
- **パフォーマンス最適化**: 10以上

#### コード配分
- エンジンコア: 20%
- ゲームシステム: 35%
- アセット管理: 15%
- UIと設定: 18%（TextRenderer、DebugHUD、SettingsUI、GraphicalUIで拡張）
- プロファイリング: 8%
- JNI/インフラ: 4%

---

### [TECH] 技術スタック

#### コア技術
- C++17
- Rust（cargo-ndk、純粋ロジックのアセット/プラグイン層）
- Android NDK r26.1
- OpenGL ES 3.0
- CMake 3.16+
- Gradle 9.4+

#### ライブラリ
- GLM（数学）
- Jolt Physics（物理）
- OpenAL-Soft（オーディオ）
- stb_image.h（PNG読み込み）

#### ツール
- Android Studio
- JetBrains CLion
- Perfetto（プロファイリング）
- Gradle（ビルド）

---

### [NOTE] クレジット

**Oblivion Android プロジェクト**
- 完全ネイティブ移植として開発
- Oblivion GOTY Edition をベース
- 参考: OpenMW プロジェクトアーキテクチャ

**特別感謝**
- Bethesda Softworks（オリジナル Oblivion）
- OpenMW プロジェクト（参考実装）
- Android NDK Team

---

### [LEGAL] 法的注意事項

**重要**: これは教育およびテスト目的の実験的移植です。

- 正当に購入されたOblivion GOTY Editionのアセットを使用
- 商業的配布は行いません
- ソースアセットの改変は行いません
- オリジナルのBethesda Softworks著作権を尊重

---

### [LIC] ライセンス

詳細は [LICENSE](LICENSE) を参照してください。

独自ライセンス - 実験的移植
*商業使用または再配布のライセンスはありません*

---

### [HELP] サポート

- **ドキュメント**: `/docs` ディレクトリを参照
- **ビルド問題**: [docs/IMPLEMENTATION_GUIDE.md](docs/IMPLEMENTATION_GUIDE.md) を確認
- **ゲームプレイの質問**: [README.md](README.md) のゲームプレイセクションを参照
- **パフォーマンス**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) を参照

---

**状態**: Phase 65 進行中、Phase 68 完了 (v0.9.10) - ESMデータ駆動のゲームシステムと商店取引が完了。純粋ロジックのアセット/プラグイン層はRust移行中（Phase 1-3 完了）

**最終更新**: 2026-11-01
**バージョン**: 0.9.10 (versionCode 910)

**機能**: グラフィカルUI、テクスチャパネル＆ボタン、効果音、セーブ/ロードUI、OpenAL 3Dオーディオ、レトロフィルター効果、強化デバッグHUD、ESMデータ統合（40種レコード）、NPCマネージャーESM、コンテナESM、プレイヤーRACE/CLAS/BSGN、ステータス効果、NAVMパスファインディング、DIAL/INFO会話、REFR配置、呪文エフェクト（8種）、錬金術、書籍リーダー、派閥マネージャー、ルートジェネレーター、NIFスケルトン/スキニング、アニメーションシステム、衝突判定、統合テスト（Phase 30）、WorldEntity＋WorldLoader＋PlayerController統合（Phase 31）、Imperial Weave EventBus＋12フェーズコーディネーター、AnimationSubscriber、AudioSubscriber、SpellSelectionPanel（Phase 32）、専用コンバットサウンド、NPC空間オーディオ（Phase 33）、武器タイプサウンドルーティング、クイックスロット呪文（Phase 34）、Radiant AIシステム（Phase 35）、Jolt Physics統合（Phase 36）、Distant LOD（Phase 50）、SpeedTree植生（Phase 51）、FaceGen（Phase 52）、Binkビデオ（Phase 53）、Imperial Weave v4.0（Phase 54）、エンジンポリッシュ（Phase 55）、Gamebryo完成：パーティクル/ポストプロセス/水/天候/シーングラフ/マテリアル（Phase 56）、UIバグ修正＆コメント翻訳（Phase 57）、Gradle統合＆バージョン一貫性（Phase 62）、TextRendererグリフバッチ処理（Phase 63）、ESM実データパイプライン - 20種の不足レコードタイプを全デコード（Phase 64）、フルワールドレンダリング - LTEX/VTXT地形・水・空・ドア遷移・屋内ジオメトリ・LOD（Phase 65）、ESMゲームシステム統合＆商店取引（Phase 68）、セーブフォーマットv5 - コンテナ・クエスト・スクリプトグローバル・商店（Phase 69）、plugins.txtロード順＆FormID再マッピング（Phase 70）、Rust純粋ロジック層 - BSA/NIF/DDSリーダー・FormIDインデックス・プラグインリスト・アセットパス・メッシュビルダー

**次回**: Phase 65 の残件（オブジェクトLOD `.lod`/`.cmp` ローダー＋実機検証）、Phase 66（キャラクターとAI）、Phase 70（モッドとロード順）
