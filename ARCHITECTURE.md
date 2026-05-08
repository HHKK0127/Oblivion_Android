# Oblivion Android - Architecture & System Design

## Overview

This document describes the architecture and design patterns used in the Oblivion Android native port. The codebase follows a **Manager Pattern** with clear separation of concerns across rendering, game logic, UI, and system layers.

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
JNI Bridge (native-lib.cpp)
```

**Key Classes**:
- `MainActivity.java` - Entry point, lifecycle management, permissions
- `GameSurfaceView.java` - OpenGL surface, touch input handling
- `GameRenderer.java` - Implements GLSurfaceView.Renderer

### 2. JNI Bridge Layer (C++)
**Responsibility**: Java ↔ C++ communication, lifecycle callbacks

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
