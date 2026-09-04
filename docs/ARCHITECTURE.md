# Oblivion Android - Architecture & System Design

## Overview

This document describes the architecture and design patterns used in the Oblivion Android native port. The codebase follows a **Manager Pattern** with clear separation of concerns across rendering, game logic, UI, and system layers.

## Layer Architecture

### 1. Android Framework Layer (Java/Kotlin)
**Responsibility**: Native platform integration, lifecycle management, input dispatch

```
MainActivity
    -> GameSurfaceView (GLSurfaceView)
    -> GameRenderer (Renderer callback)
    -> JNI Bridge (native-lib.cpp)
```

**Key Classes**:
- `MainActivity.java` - Entry point, lifecycle management, permissions
- `GameSurfaceView.java` - OpenGL surface, touch input handling
- `GameRenderer.java` - Implements GLSurfaceView.Renderer

### 2. JNI Bridge Layer (C++)
**Responsibility**: Java <-> C++ communication, lifecycle callbacks

**Key Function**:
```cpp
// native-lib.cpp
Java_com_example_oblivion_GameRenderer_nativeRender(JNIEnv*, jobject)
```

Calls into Renderer::render() for each frame.

### 3. Core Engine Layer (C++)

#### 3.1 Rendering Engine
**Files**: `engine/renderer.h/cpp`, `engine/shader.h/cpp`, `engine/camera.h/cpp`

```
Renderer
|-- OpenGL ES 3.0 Context
|-- ShaderProgram (Vertex + Fragment)
|-- Camera (View Matrix)
|-- Viewport Management
|-- Frame Rendering Pipeline
|-- SkinShader (skinned mesh rendering)
`-- RetroFilter (optional pixelation, scanlines, CRT)
```

**Key Methods**:
- `Renderer::init()` - Initialize all systems
- `Renderer::render()` - Main game loop
- `Renderer::cleanup()` - Resource cleanup

#### 3.2 Imperial Weave (Update Coordinator)
**Files**: `engine/imperial_weave.h/cpp`

The central update coordinator using a **15-phase pipeline** with loose-coupled EventBus messaging.

```
ImperialWeave::update(dt)
    Phase 1:  EventProcess
    Phase 2:  World
    Phase 3:  AI
    Phase 4:  Player
    Phase 5:  Inventory
    Phase 6:  Spell
    Phase 7:  Animation
    Phase 8:  Physics
    Phase 9:  Combat
    Phase 10: Quest
    Phase 11: Audio
    Phase 12: RenderSubmit
```

**EventBus** (`engine/event_bus.h`):
- Loose-coupled messaging between systems
- Systems emit events without knowing subscribers
- Type-safe via `type_index` (compiler-independent)
- Shared_ptr handlers (copy avoidance)

**Example attack flow**:
```
ATK button -> PlayerController.attack()
           -> CombatManager.playerAttack()
           -> EventBus emit "COMBAT_ATTACK_HIT"
                +-- AnimationSubscriber -> target plays hit-reaction anim
                +-- AudioSubscriber     -> combat hit SE (weapon-type routed)
                +-- UIFloatingText      -> "Hit!" appears on screen
```

**Key principle**: Systems emit events. They do not call other systems directly.

#### 3.3 Subscriber Bridges
**Files**: `animation/animation_subscriber.h/cpp`, `audio/audio_subscriber.h/cpp`

Thin, EventBus-driven bridges that do not own any systems:

- **AnimationSubscriber**: Listens for combat events -> maps AnimState to animation names -> plays via AnimationPlayer
- **AudioSubscriber**: Listens for combat/animation events -> maps events to sound definitions -> plays via AudioManager with 3D positioning

#### 3.4 UI System
**Files**: `ui/*.h/cpp`

```
TextRenderer (Text Rendering Foundation)
    |-- TitleScreen (3-second logo + menu)
    |-- QuestUI (Quest log display)
    |-- DebugHUD (FPS, memory, frame time)
    |-- SettingsUI (Settings menu overlay)
    |-- UIPanel (draggable container with background texture)
    |-- UIButton (multi-state textured button)
    |-- SpellSelectionPanel (draggable spell picker)
    |-- FloatingText (damage/effect indicators)
`-- InventoryPanel (item management)
```

**State Flow**:
```
LOGO_DISPLAY (3 sec) -> MENU -> [Settings] -> SettingsUI -> [Back] -> MENU
                                                    -> [Start Game] -> GAME
```

#### 3.5 Game Systems Layer
**Files**: `game/*.h/cpp`

```
WorldManager       (Cell/Object streaming, NIF/DDS loading)
NpcManager         (100+ NPCs, AI state machine, AI packages)
CombatManager      (9 weapon types, hitbox, critical, block/parry/dodge)
QuestManager       (Multi-objective quests, rewards)
SpellManager       (6 schools, 10+ spells)
PlayerController   (Movement, combat, input handling)
InventoryManager   (Item management, equipment effects)
SpellManager       (Spell casting, mana management)
```

Each manager follows the standard pattern:
```cpp
bool initialize();            // One-time setup
void update(float deltaTime); // Per-frame logic
void cleanup();               // Resource deallocation
```

#### 3.6 AI System
**Files**: `ai/*.h/cpp`

- **AIScheduler**: 24-hour time-based NPC management
- **15 AI Package Types**: Explore, Follow, Guard, Patrol, Travel, Eat, Sleep, Combat, Flee, Idle, Wander, Activation, Conversation, Sandbox
- **PackageStack**: Priority-based package management (combat/flee override)
- **NavMeshManager**: A* pathfinding on NAVM data, path smoothing, stuck detection

#### 3.7 Physics System
**Files**: `physics/*.h/cpp`

- **PhysicsManager**: Jolt Physics singleton with custom settings
- **CharacterVirtual**: Capsule-based player/NPC character controllers
- **HeightFieldShape**: Terrain collision from ESM LAND data
- **Raycast API**: World-space ray queries (line-of-sight, combat, interaction)
- Fixed timestep (1/60s) for deterministic simulation
- Runs in Imperial Weave Physics phase (phase 8)

#### 3.8 Script VM
**Files**: `script/*.h/cpp`

- **ScriptVM**: Bytecode interpreter with 47 opcodes
- **ScriptManager**: Per-frame execution with budget control (1000 instructions/frame)
- **ScriptFunctions**: 118 Oblivion game functions
- **ExecutionContext**: Per-script state (RPN stack, local variables, references)

#### 3.9 World & Asset Layer
**Files**: `world/*.h/cpp`, `assets/*.h/cpp`

```
WorldManager       (Cell loading/unloading, seamless transitions)
WorldLoader        (Static/dynamic/actor loading, entity storage)
BSA Reader         (Archive extraction, ZLib decompression)
ESM Reader         (40 record types from Oblivion.esm)
NIF Parser         (Mesh, skeleton, skinning, collision)
DDS Loader         (DXT1/DXT3/DXT5 texture loading)
AssetManager       (LRU cache, reference counting)
```

#### 3.10 Animation & Audio Layer
**Files**: `animation/*.h/cpp`, `audio/*.h/cpp`

```
AnimationPlayer    (Sequence playback, slerp/lerp, text keys)
Skeleton           (Bone hierarchy, BFS traversal)
AudioManager       (OpenAL-Soft, BGM, SFX, definition loading)
Audio3D            (3D spatial audio, distance attenuation)
AudioSubscriber    (EventBus -> AudioManager bridge)
```

#### 3.11 System & Persistence Layer
**Files**: `save_system/*.h/cpp`, `system/*.h/cpp`, `profiling/*.h/cpp`

```
SaveManager        (Binary format, full system serialization)
SettingsManager    (Persistent debug mode and language preferences)
PerformanceMonitor (Frame timing, memory, CPU profiling)
LocalizationManager (Japanese/English, 100+ translations)
```

---

## Component Integration

### Renderer: Central Hub

The `Renderer` class orchestrates all systems:

```cpp
class Renderer {
    // Imperial Weave (Update Coordinator)
    std::unique_ptr<ImperialWeave> imperialWeave;

    // Subscribers (thin bridges)
    std::unique_ptr<AnimationSubscriber> animSubscriber;
    std::unique_ptr<AudioSubscriber> audioSubscriber;

    // UI Systems
    std::unique_ptr<TitleScreen> titleScreen;
    std::unique_ptr<SpellSelectionPanel> spellPanel;
    std::unique_ptr<DebugHUD> debugHUD;
    std::unique_ptr<SettingsUI> settingsUI;

    // Game Systems
    std::unique_ptr<WorldManager> worldManager;
    std::unique_ptr<NpcManager> npcManager;
    std::unique_ptr<CombatManager> combatManager;
    std::unique_ptr<QuestManager> questManager;
    std::unique_ptr<SpellManager> spellManager;
    std::unique_ptr<PlayerController> playerController;
    std::unique_ptr<PhysicsManager> physicsManager;

    // System Layer
    std::unique_ptr<SettingsManager> settingsManager;
    std::unique_ptr<SaveManager> saveManager;
    std::unique_ptr<PerformanceMonitor> performanceMonitor;
};
```

### Initialization Order

```
Renderer::init()
    |
    +-> SettingsManager::initialize()    [Load persistent settings]
    +-> LocalizationManager::initialize() [Load language strings]
    +-> PhysicsManager::initialize()     [Jolt Physics world]
    +-> WorldManager::initialize()       [Create world/cells]
    +-> NpcManager::initialize()         [Spawn NPCs]
    +-> PlayerController::initialize()   [Character controller]
    +-> CombatManager::initialize()      [Link with WorldManager]
    +-> QuestManager::initialize()       [Link with NpcManager]
    +-> SpellManager::initialize()       [Load spell database]
    +-> AnimationSubscriber::initialize() [Connect EventBus]
    +-> AudioSubscriber::initialize()    [Connect EventBus]
    +-> ImperialWeave::initialize()      [Wire all phases]
    +-> TextRenderer::initialize()       [Prepare font rendering]
    +-> DebugHUD::initialize()           [Link with PerfMonitor]
    +-> SettingsUI::initialize()         [Link with SettingsManager]
    +-> TitleScreen::initialize()        [Link with LocalizationManager]
```

### Main Game Loop

```
Renderer::render(float deltaTime)
    |
    +-> if (showTitleScreen) {
    |       titleScreen->update(deltaTime)
    |       titleScreen->render()
    |   } else {
    |       imperialWeave->update(deltaTime)
    |       [Imperial Weave orchestrates all systems via 15-phase pipeline]
    |       [EventBus distributes events to subscribers]
    |       debugHUD->update(deltaTime)
    |       debugHUD->render() [if debugModeEnabled]
    |   }
```

---

## Namespace Architecture

| Namespace | Classes |
|-----------|---------|
| Global | Renderer, WorldManager, NpcManager, CombatManager, QuestManager, CollisionWorld, PlayerController, InventoryManager, SpellManager, AudioManager, EquipmentEffectSystem |
| `animation::` | AnimationPlayer |
| `ai::` | AIScheduler |
| `oblivion::` | NavMeshManager, PhysicsManager, AlchemySystem, BookReader, ClothingConverter |

---

## Text Rendering System

**Purpose**: Render colored text at screen coordinates

**Implementation**:
- Uses OpenGL ES 3.0 orthographic projection
- Origin at top-left (0,0)
- Supports color and scale parameters

**Key Method**:
```cpp
void TextRenderer::renderText(
    const std::string& text,
    float x, float y,
    const glm::vec3& color = glm::vec3(1.0f),
    float scale = 1.0f
);
```

---

## Settings System Architecture

### SettingsManager: Persistent Storage

**File Location**: `/data/data/com.example.oblivion/settings.txt`

**File Format** (KEY=VALUE):
```
DEBUG_MODE=1
LANGUAGE=ja
```

---

## Performance Considerations

### Frame Rate Control
- **Target**: 60 FPS (exceeds original 30 FPS target)
- **Frame Budget**: 16.67 ms per frame
- **FrameBudgetManager** enforces per-phase budget allocation

### Memory Budget
- **Heap**: 40-50 MB runtime
- **Texture Cache**: managed by AssetManager with LRU eviction
- **MemoryDefrag**: runtime defragmentation

### CPU Budget
- **Game Logic**: 80% of frame budget
- **Rendering**: 15%
- **System**: 5%

---

## Thread Safety

### Current Architecture
- **Single-threaded rendering** via GLSurfaceView
- **JNI calls** only from render thread
- **Physics**: Jolt Physics runs in Imperial Weave Physics phase

---

## Error Handling

### Logging Strategy

Three levels of logging:
```cpp
LOGD(...)  // Debug: verbose system info (conditional compilation)
LOGI(...)  // Info: important events
LOGE(...)  // Error: failures requiring attention
```

---

## Build System

### CMakeLists.txt Structure

```cmake
cmake_minimum_required(VERSION 3.18.1)
project(oblivion_native)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)

# Source files organized by directory
set(SOURCES
    engine/renderer.cpp
    engine/imperial_weave.cpp
    engine/shader.cpp
    game/npc_manager.cpp
    game/combat_manager.cpp
    # ... more files ...
    animation/animation_subscriber.cpp
    audio/audio_subscriber.cpp
    ui/spell_selection_panel.cpp
)

# Link libraries
target_link_libraries(native-lib android EGL GLESv3 log)
```

---

## Testing Strategy

### Integration Testing
1. Initialize all systems in order
2. Simulate game loop (10+ frames)
3. Verify state consistency
4. Check memory cleanup

### Device Testing
- **Amazon Fire 7 (Android 9)**: 60 FPS, 42 MB
- **Xiaomi (Android 16)**: 60 FPS, 45 MB

---

**Last Updated**: 2026-09-04
**Version**: 1.6.0
**Phase**: 63 (Complete)
