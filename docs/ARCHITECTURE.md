# Oblivion Android - Architecture & System Design

## Overview

This document describes the architecture and design patterns used in the Oblivion Android native port. The codebase follows a **Manager Pattern** with clear separation of concerns across rendering, game logic, UI, and system layers.

The native side is mostly C++17, but the pure-logic asset and plugin functions run in Rust behind a small C ABI - see [Native Code Split: C++ and Rust](#native-code-split-c-and-rust) for where the line sits and why.

## Layer Architecture

### 1. Android Framework Layer (Java/Kotlin)
**Responsibility**: Native platform integration, lifecycle management, input dispatch

```
MainActivity
    ↓
GameSurfaceView (GLSurfaceView)
    ↓
GameRenderer (Renderer callback)
    ↓
JNI Bridge (jni_bridge.cpp)
```

**Key Classes**:
- `MainActivity.java` - Entry point, lifecycle management, permissions
- `GameSurfaceView.java` - OpenGL surface, touch input handling
- `GameRenderer.java` - Implements GLSurfaceView.Renderer

### 2. JNI Bridge Layer (C++)
**Responsibility**: Java ↔ C++ communication, lifecycle callbacks

**Key Function**:
```cpp
// jni_bridge.cpp
Java_com_example_oblivion_GameRenderer_nativeRender(JNIEnv*, jobject)
```

Calls into Renderer::render() for each frame.

### 3. Core Engine Layer (C++)

#### 3.1 Rendering Engine
**Files**: `engine/renderer.h/cpp`, `engine/shader.h/cpp`, `engine/camera.h/cpp`

```
Renderer
├── OpenGL ES 3.0 Context
├── ShaderProgram (Vertex + Fragment)
├── Camera (View Matrix)
├── Viewport Management
└── Frame Rendering Pipeline
```

**Key Methods**:
- `Renderer::init()` - Initialize all systems
- `Renderer::render()` - Main game loop
- `Renderer::cleanup()` - Resource cleanup

#### 3.2 UI System
**Files**: `ui/text_renderer.h/cpp`, `ui/title_screen.h/cpp`, `ui/quest_ui.h/cpp`, `ui/debug_hud.h/cpp`, `ui/settings_ui.h/cpp`

```
TextRenderer (Text Rendering Foundation)
    ↓
├── TitleScreen (3-second logo + menu)
├── QuestUI (Quest log display)
├── DebugHUD (FPS, memory, frame time)
└── SettingsUI (Settings menu overlay)
```

**Architecture Pattern**: State Machine (TitleScreenState enum)

**State Flow**:
```
LOGO_DISPLAY (3 sec)
    ↓
MENU (show options)
    ↓
[Settings Selected]
    ↓
SettingsUI Toggle
    ↓
[Back]
    ↓
MENU
```

#### 3.3 System Layer
**Files**: `system/settings_manager.h/cpp`

```
SettingsManager
├── debugModeEnabled (bool)
├── currentLanguage (string: "ja"/"en")
├── Persistent File: /data/data/com.example.oblivion/settings.txt
└── Methods: save(), load(), reset()
```

**Persistence Format**:
```
DEBUG_MODE=1
LANGUAGE=ja
```

#### 3.4 Game Systems Layer
**Files**: `game/*.h/cpp`

```
WorldManager (Cell/Object Management)
NpcManager (100+ NPCs, AI state machine)
CombatManager (Damage calculation, AI combat)
QuestManager (Quest state, progression)
SpellManager (6 schools, 10+ spells)
LocalizationManager (Japanese/English)
SaveManager (Game state serialization)
```

Each manager follows the standard pattern:
```cpp
bool initialize();  // One-time setup
void update(float deltaTime);  // Per-frame logic
void cleanup();  // Resource deallocation
```

## Native Code Split: C++ and Rust

Most of the native code is C++17, but the parts that are pure functions of their inputs are
implemented in Rust and reached through small C ABIs - two stateless ones and three stateful. Those
parts are the ESM FormID translation, the plugin list, the Data-folder path expansion, the NIF
geometry merge, the BSA archive reader, the NIF mesh parser and the DDS texture reader.

The dividing line is impurity, not size:

| Stays C++ | Moved to Rust |
| --- | --- |
| OpenGL ES objects, shaders, the render loop | Record parsing over an in-memory buffer |
| Jolt Physics bodies and character controllers | String, path and extension rules |
| `AAssetManager`, `MediaCodec`, JNI entry points | FormID index building and rebasing |
| `std::ifstream` I/O, zlib inflate, threads | Vertex / index merge and rebase arithmetic |
| | BSA folder / file tables, both hashes, deflate inflate |
| | NIF header / node tree / geometry / skin / controller parsing |
| | DDS header parse and DXT / uncompressed decode |

A function moves only when it allocates nothing the caller must own, keeps no state between calls and
touches no platform handle. `assets/esm_reader.cpp` shows the other side of the rule: an `esm-parser`
crate exists in the Rust workspace and is used for host-side verification, but the device still runs
the C++ reader because the parse is interleaved with `std::ifstream`, zlib and the record types the
rest of the engine consumes. The BSA reader is the exception that proves it: it owns a file handle
and a materialised index, so it moves only by turning the state into an opaque handle that Rust owns
and C++ borrows.

```
assets/formid_index.cpp ──┐
assets/plugin_list.cpp  ──┴──► plugin_ffi.h ──► libplugin_ffi.a (plugin-parser)
assets/asset_path_resolver.cpp ──┐
assets/mesh_builder.cpp        ──┴──► assets_ffi.h ──► libassets_ffi.a (assets-core)
assets/bsa_reader.cpp ────────────────► bsa_ffi.h ───► libbsa_ffi.a (bsa-core)
assets/nif_parser.cpp ─────────────────► nif_ffi.h ───► libnif_ffi.a (nif-core)
assets/dds_loader.cpp ─────────────────► dds_ffi.h ───► libdds_ffi.a (dds-core)
```

Each C++ file keeps its original header and is now a shim: callers are unchanged, and the only
contract between the two languages is the header in
`oblivion-re-tools/crates/*-ffi/include/`.

### ABI conventions

- Every function returns an error code (`0` ok, `-1` invalid argument, `-7` caught panic, `-8` buffer
  too small) and reports diagnostics through `*_last_error_code` / `*_last_error_message`.
- Nothing allocates for the caller. Output is described by a pointer plus a **capacity**, and a
  capacity that is too small returns `-8` **without writing**, so the two-pass pattern is "ask for the
  lengths, allocate, call again".
- A capacity is always in the unit the caller was told to allocate. `afl_merge_geometry` packs 11
  floats per vertex, so its `vertex_capacity` is a float count; passing a vertex count is rejected
  rather than trusted.
- Panics never cross the boundary - each entry point wraps the Rust call so an unwind becomes `-7`.
- `bsa_ffi`, `nif_ffi` and `dds_ffi` are the stateful ABIs. Each `*_create()` returns an opaque handle
  that Rust owns and C++ borrows; the shim never frees it on the C++ side except through
  `*_destroy()`, which its destructor calls. Any search entry point
  (`bsa_find_files_by_prefix` / `_by_extension`)
  lends a scratch index array that stays valid only until the next search, so the shim converts the
  indices to `BSAFileEntry` pointers immediately instead of holding them.

### Build wiring

`app/src/main/cpp/CMakeLists.txt` runs `cargo ndk` once per ABI in a single custom command that
produces all three archives, then imports them as `IMPORTED STATIC` targets and links them into
`native-lib`. `OBLIVION_RUST_DIR` (default: a sibling `oblivion-re-tools` checkout) and
`OBLIVION_RUST_OFFLINE` are cache variables, and `app/build.gradle` forwards the matching
`-Poblivion.rust.dir` / `-Poblivion.rust.offline` Gradle properties. `rustc` and `rustdoc` are pinned
to the proxies next to `cargo`, because a standalone host-only toolchain earlier on `PATH` otherwise
wins and the build fails on a missing Android target. The mapping is verified end to end by linking
`libnative-lib.so` for all three ABIs (`arm64-v8a`, `armeabi-v7a`, `x86_64`) and confirming that all
19 `bsa_*`, all 27 `nif_*` and all 13 `dds_*` symbols are defined in each shared object.

### Testing the boundary

Behaviour is pinned by a host oracle: the original C++ implementation and the Rust one run over the
same inputs and every output field is compared, using the unmodified C++ test suites as the
assertions (89 checks across the FormID index, plugin list, asset path and mesh builder suites). The
C ABI itself has a separate C11 harness for what the oracle cannot see - NULL handling,
`BUFFER_TOO_SMALL` leaving the buffer untouched, and error-code numbering.

The BSA reader cannot use those suites, because its assertions are about real archives rather than
constructed inputs, so it gets a dump differ instead: one driver linked once to the original
`bsa_reader.cpp` and once to the shim prints a record per public-API call with a leading token, and
the two dumps are compared line by line. Over four shipped archives (10,981 lines) the entry tables,
headers, extracted bytes and both search orders match exactly; the only differences are `findFile`
results, which is where the port fixed two hashing bugs in the original
(`plan.md#rust-migration-of-pure-logic-layers-2026-10-26`).

The NIF parser is checked the same way: a driver dumps the header, the node tree and every
geometry's counts, raw-byte hashes and transforms from both implementations, and all 100 meshes in
`tmp/re/lodnif/` match with zero differing lines. The dump prints floats as `%a` and hashes byte
arrays with FNV-1a so the comparison is bit exact; the shim's one intentional gap is each `NiNode`'s
own transform (the FFI does not carry it), which no caller reads.

The DDS loader follows the dump-differ pattern too: a driver linked once to the original
`dds_loader.cpp` and once to the shim dumps every output field over a 13-file corpus
(`tmp/dds_oracle/corpus/`), and 11 of 13 files match exactly. The two differences are benign
dead-path cases - `missing_dims` (the original keeps a raw header that fails the flag check, the
Rust parse errors before saving it) and `empty_payload` (both fail the decompress with no output
data) - and `asset_manager.cpp` rejects both failures before returning anything, so neither state
escapes. The port also fixed two shim bugs found on the way: `dds_last_error` was passing a NULL
buffer to `%s` (crash) and a `textureType` assignment that the original never made was removed for
fidelity. `dds-core` carries 7 crate tests, and the `dds_*` symbols are confirmed in the linked
`libnative-lib.so` for all three ABIs - 13 defined symbols, matching the 13 prototypes in
`dds_ffi.h`.

## Component Integration

### Renderer: Central Hub

The `Renderer` class orchestrates all systems:

```cpp
class Renderer {
    // UI Systems
    std::unique_ptr<TitleScreen> titleScreen;
    std::unique_ptr<QuestUI> questUI;
    std::unique_ptr<TextRenderer> textRenderer;
    std::unique_ptr<DebugHUD> debugHUD;
    std::unique_ptr<SettingsUI> settingsUI;
    
    // Game Systems
    std::unique_ptr<WorldManager> worldManager;
    std::unique_ptr<NpcManager> npcManager;
    std::unique_ptr<CombatManager> combatManager;
    std::unique_ptr<QuestManager> questManager;
    std::unique_ptr<SpellManager> spellManager;
    
    // System Layer
    std::unique_ptr<SettingsManager> settingsManager;
    std::unique_ptr<LocalizationManager> localizationManager;
    std::unique_ptr<SaveManager> saveManager;
    std::unique_ptr<PerformanceMonitor> performanceMonitor;
};
```

### Initialization Order

```
Renderer::init()
    │
    ├─→ SettingsManager::initialize()    [Load persistent settings]
    ├─→ LocalizationManager::initialize() [Load language strings]
    ├─→ WorldManager::initialize()        [Create world/cells]
    ├─→ NpcManager::initialize()          [Spawn NPCs]
    ├─→ CombatManager::initialize()       [Link with WorldManager]
    ├─→ QuestManager::initialize()        [Link with NpcManager]
    ├─→ SpellManager::initialize()        [Load spell database]
    ├─→ TextRenderer::initialize()        [Prepare font rendering]
    ├─→ DebugHUD::initialize()            [Link with PerformanceMonitor]
    ├─→ SettingsUI::initialize()          [Link with SettingsManager]
    ├─→ TitleScreen::initialize()         [Link with LocalizationManager]
    └─→ QuestUI::initialize()             [Link with QuestManager]
```

### Main Game Loop

```cpp
Renderer::render(float deltaTime)
    │
    ├─→ if (showTitleScreen) {
    │       titleScreen->update(deltaTime)
    │       titleScreen->render()
    │       [Check if Settings requested → toggle SettingsUI]
    │   } else {
    │       [Normal Game Rendering]
    │       worldManager->update(deltaTime)
    │       npcManager->update(deltaTime)
    │       combatManager->update(deltaTime)
    │       questManager->update(deltaTime)
    │       [Render game world]
    │       debugHUD->update(deltaTime)
    │       debugHUD->render() [if debugModeEnabled]
    │       questUI->render()
    │   }
    │
    └─→ return
```

### Touch Event Priority

```cpp
Renderer::onTouchEvent(x, y)
    │
    ├─→ SettingsUI (highest priority)
    │   [If visible, handle and return]
    │
    ├─→ TitleScreen (if active)
    │   [If showing, handle menu/logo and return]
    │
    └─→ QuestUI (lowest priority)
        [If visible, handle and return]
```

## Text Rendering System

### TextRenderer: Foundation

**Purpose**: Render colored text at screen coordinates

**Implementation**:
- Uses OpenGL ES 3.0 orthographic projection
- Origin at top-left (0,0)
- Supports color and scale parameters
- Handles projection matrices internally

**Key Method**:
```cpp
void TextRenderer::renderText(
    const std::string& text,
    float x, float y,
    const glm::vec3& color = glm::vec3(1.0f),
    float scale = 1.0f
);
```

**Vertex Shader** (Orthographic):
```glsl
#version 300 es
layout(location = 0) in vec3 aPosition;
uniform mat4 uProjection;
uniform mat4 uView;
uniform mat4 uModel;
void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
```

### UI Text Rendering Pipeline

```
TextRenderer::renderText()
    │
    ├─→ 1. Bind VAO/VBO (character geometry)
    ├─→ 2. Set projection matrix (screen coordinates)
    ├─→ 3. Set text color (RGB)
    ├─→ 4. Set position (x, y screen pixels)
    ├─→ 5. Bind texture (character atlas)
    ├─→ 6. glDrawArrays() (render character)
    └─→ 7. Unbind shader program
```

## Settings System Architecture

### SettingsManager: Persistent Storage

**Responsibility**: 
- Maintain application settings in memory
- Load settings from disk on startup
- Save settings on change
- Provide getter/setter interface

**File Location**:
```
/data/data/com.example.oblivion/settings.txt
```

**File Format** (KEY=VALUE):
```
DEBUG_MODE=1
LANGUAGE=ja
```

**Settings Structure**:
```cpp
struct Settings {
    bool debugModeEnabled = false;      // Default: OFF
    std::string currentLanguage = "ja"; // Default: Japanese
};
```

### SettingsUI: User Interface

**Responsibility**:
- Display settings menu as overlay
- Handle touch events to change settings
- Update SettingsManager when changes occur
- Show current values

**Menu Items**:
1. **Debug Mode** - Toggle ON/OFF
   - ON: Shows FPS, frame time, memory info
   - OFF: Hides debug information

2. **Language** - Switch language
   - Japanese (日本語)
   - English

3. **Back** - Return to main menu

**Visual Design**:
- Black semi-transparent background
- Yellow title: "SETTINGS"
- White text for items
- Red highlight for selected item
- Touch-to-select interaction

## Debug HUD System

### DebugHUD: Performance Monitoring

**Responsibility**:
- Measure frame time in milliseconds
- Track FPS (frames per second)
- Read system memory from /proc/meminfo
- Display real-time statistics

**Displayed Metrics**:
```
FPS: 60.0              [Current frames per second]
Frame: 16.67 ms        [Time for current frame]
Avg: 16.50 ms          [Average frame time (0.5s window)]
Mem: 45 MB             [Current memory usage]
Cubes: 5               [Number of active game objects]
DEBUG: ON              [Debug mode status]
```

**Update Frequency**:
- Frame time: Every frame
- Average: Every 0.5 seconds
- Memory: Every 0.5 seconds

**Memory Reading**:
```cpp
// Read from /proc/meminfo
// File format:
// MemTotal:        4046676 kB
// MemFree:         2145632 kB
// MemAvailable:    3145632 kB
```

## Game Systems Integration

### Manager Pattern (Standard for all systems)

Each game system follows this lifecycle:

```cpp
class SystemManager {
public:
    bool initialize(...);   // One-time setup
    void update(float dt);  // Per-frame update
    void cleanup();         // Resource cleanup
};
```

**Examples**:
- `WorldManager` - Cell/object streaming
- `NpcManager` - NPC AI update
- `CombatManager` - Combat state, damage application
- `QuestManager` - Quest progression
- `SpellManager` - Spell effects

### NPC AI State Machine

```
IDLE
    ↓
WANDER (patrol randomly)
    ↓
[Detects player → distance < 30m]
    ↓
FOLLOW_PLAYER
    ↓
[Distance < 5m] OR [Player attacks]
    ↓
COMBAT
    ├─→ Attack roll (1s cooldown)
    ├─→ Spell casting (if low HP)
    └─→ [NPC defeated]
        ↓
    IDLE (death, removed from world)
```

### Quest System Integration

```
NPC offers quest
    ↓
Player accepts (QuestManager::acceptQuest)
    ↓
Quest objectives tracked
    ↓
[Objective conditions met]
    ↓
QuestManager::updateObjective()
    ↓
[All objectives complete]
    ↓
QuestManager::completeQuest()
    ↓
Reward applied (gold + experience)
```

## Localization System

### LocalizationManager

**Responsibility**:
- Load string translations for UI elements
- Provide getString(key) interface
- Support multiple languages

**Supported Languages**:
- Japanese (ja)
- English (en)

**String Keys** (menu_start, menu_settings, etc.):
```cpp
localizationManager->getString("menu_start")  // Returns "ゲーム開始" or "Start Game"
```

## Memory Management

### Smart Pointers (std::unique_ptr)

All major systems use `std::unique_ptr` for automatic memory management:

```cpp
std::unique_ptr<Renderer> renderer;
std::unique_ptr<WorldManager> worldManager;
std::unique_ptr<NpcManager> npcManager;
// Automatic cleanup when unique_ptr goes out of scope
```

### Object Pooling

For frequent allocation/deallocation:
- NPC entities
- Spell effects
- Combat instances

## Performance Considerations

### Frame Rate Control
- **Target**: 60 FPS
- **Frame Time**: 16.67 ms per frame
- **Margin**: 0.33 ms for system overhead

### Memory Budget
- **Heap**: 40-50 MB target
- **NPC Instances**: ~500 KB each
- **Texture Cache**: 100-200 MB
- **Total Limit**: < 500 MB

### CPU Budget
- **Game Logic**: 80% of frame budget
- **Rendering**: 15%
- **System**: 5%

## Build System

### CMakeLists.txt Structure

```cmake
cmake_minimum_required(VERSION 3.18.1)
project(oblivion_native)

# Source files organized by directory
set(SOURCES
    engine/renderer.cpp
    engine/shader.cpp
    # ... more files ...
    ui/text_renderer.cpp
    ui/debug_hud.cpp
    ui/settings_ui.cpp
    system/settings_manager.cpp
)

# Include directories
target_include_directories(native-lib PRIVATE ...)

# Link libraries
target_link_libraries(native-lib android EGL GLESv3 log)

# Compiler flags per architecture
if (ANDROID_ABI STREQUAL arm64-v8a)
    target_compile_options(native-lib PRIVATE -O3 -march=armv8-a)
endif()
```

## Thread Safety

### Current Architecture
- **Single-threaded rendering** via GLSurfaceView
- **JNI calls** only from render thread
- **Android LifecycleEvents** marshaled via onSurfaceCreated/onSurfaceDestroyed

### Future Considerations
- Async asset loading
- Background NPC AI calculation
- Parallel spell effect processing

## Error Handling

### Logging Strategy

Three levels of logging:
```cpp
LOGD(...)  // Debug: verbose system info
LOGI(...)  // Info: important events
LOGE(...)  // Error: failures requiring attention
```

### Null Checks

All manager methods check for nullptr:
```cpp
if (worldManager != nullptr) {
    worldManager->update(deltaTime);
}
```

## Testing Strategy

### Integration Testing
1. Initialize all systems in order
2. Simulate game loop (10+ frames)
3. Verify state consistency
4. Check memory cleanup

### Device Testing
- **Amazon Fire 7 (Android 9)**: 60 FPS, 42 MB
- **Xiaomi (Android 16)**: 60 FPS, 45 MB

### Performance Profiling
- Android Studio Profiler (CPU, Memory, GPU)
- Perfetto for systrace analysis
- Logcat for custom timing logs

## Future Extensions

### Phase 7.2: Save/Load System
- JSON serialization of game state
- NPC positions, quest progress, inventory
- Binary checkpoint format for efficiency

### Phase 7.3: Graphical UI
- Texture-based menu backgrounds
- Icon buttons with visual feedback
- Fade-in/out transitions

### Phase 8: Audio System
- OpenAL-Soft integration
- 3D sound positioning
- Music and ambient effects

---

**Last Updated**: 2026-04-18  
**Version**: 0.7.1  
**Phase**: 7.1 (Settings & Debug HUD Complete)
