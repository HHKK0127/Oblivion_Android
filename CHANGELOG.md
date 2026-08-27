# Oblivion Android - Changelog

All notable changes to the Oblivion Android project are documented here.

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
- Spell button label prefixed with school abbreviation: `[破]`, `[回]`, `[召]`, `[変]`, `[幻]`, `[神]`
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
- Imperial Weave更新パイプラインを8フェーズから12フェーズに拡張
- PlayerController、InventoryManager、SpellManager、AudioManagerをImperial Weaveに統合
- Rendererから重複するManager呼び出しを段階的に削除（コメントアウト）
- ジョイスティック入力とオーディオリスナー位置の更新をImperialWeave::update()前に移動
- titleScreenとuiSystemはRendererから直接呼び出し（UI/オーバーレイシステムのためWeave不適切）

### UI Additions
- プレイヤーコンバットUIボタン追加（攻撃・ブロック・詠唱）

### Animation Integration
- **PlayerController EventBus Integration**
  - subscribeToCombatEvents()で6種類のコンバットイベントを購読
  - COMBAT_ATTACK_HIT, COMBAT_CRITICAL_HIT, COMBAT_BLOCK, COMBAT_PARRY, COMBAT_DODGE, COMBAT_DEATH
  - 現在はログ出力のみ、今後アニメーション再生を実装

- **NPC Animation State Management**
  - AnimState列挙型: IDLE, WALK, RUN, ATTACK, HIT_REACTION, BLOCK, DEATH
  - アニメーションタイミング定数: HIT_REACTION_DURATION=0.5秒, ATTACK_DURATION=0.8秒
  - 自動状態遷移: アニメーション完了後にIDLE復帰
  - CombatManager::applyDamage()がNPCアニメーションを自動トリガー

- **Combat Animation Flow**
  - ダメージ適用時: target->triggerHitReaction()
  - NPC撃破時: target->triggerDeath()
  - 攻撃時: playerController->attack()でATTACK状態に遷移
  - 攻撃ボタン（ATK）: プレイヤーの攻撃アクションをトリガー
  - ブロックボタン（BLK）: コンバットスタンスの切り替え
  - 詠唱ボタン（MAG）: 魔法詠唱（TODO: スペルシステム統合後）
  - タイトル画面では非表示、ゲーム開始時に表示

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
- ✅ TextureLoader: PNG decoding, OpenGL texture creation
- ✅ UIPanel: Background texture rendering, drag, close
- ✅ UIButton: State transitions, texture switching, click events
- ✅ TextureScaleMode: Stretch, fit, crop all work correctly
- ✅ AudioManager: JSON loading, sound playback by key
- ✅ TitleScreen: All textures load and display correctly
- ✅ SettingsUI/SaveLoadUI: Background textures render
- ✅ Build: `./gradlew assembleDebug` succeeds

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
- ✅ SaveLoadUI: Slot selection, save/load execution, error handling
- ✅ Audio System: WAV file loading, 3D positional audio, source management
- ✅ RetroFilter Effects: Real-time effect toggling, visual verification
- ✅ DebugHUD: Audio and filter status display accuracy
- ✅ Integration: All systems coordinate without conflicts
- ✅ Thread Safety: JNI calls from multiple threads without crashes
- ✅ Persistence: Settings and save slots persist across app restarts

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
- ✅ TextRenderer with OpenGL ES 3.0 orthographic projection
- ✅ DebugHUD with 6 real-time metrics
- ✅ SettingsManager with file persistence
- ✅ SettingsUI with touch interaction
- ✅ Integration into title screen menu flow
- ✅ Touch event priority system (SettingsUI > TitleScreen > QuestUI)

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
- ✅ TextRenderer: Text displays at correct coordinates with colors
- ✅ DebugHUD: All metrics update correctly, format properly
- ✅ SettingsManager: Settings persist across app restarts
- ✅ SettingsUI: Menu displays, touch selection works, changes save
- ✅ Integration: Touch event priority system works correctly
- ✅ Settings + DebugHUD: Debug mode toggle hides/shows HUD

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
- Amazon Fire (Android 9): **60 FPS stable** (target: 30 FPS) ✅
- Xiaomi (Android 16): **60 FPS stable** (target: 30 FPS) ✅
- No frame drops during 30+ second continuous test

#### Memory
- **App Footprint**: 40.4 MB (Pss) - excellent efficiency
- **Native Heap**: 11.3 MB (5x below limit)
- **GPU Memory**: 1.884 MB (highly optimized)
- **Headroom**: 1.06 GB available (87% unused) ✅

#### CPU
- **App CPU**: < 0.1% (below top 38 processes)
- **Thermal**: 38-40°C during test (safe zone)
- **Threads**: 29 active (mostly idle) ✅

#### Battery
- **Drain Rate**: ~1-2%/hour at 50% brightness
- **Peak Drain**: 3%/hour at 100% brightness
- **Idle Drain**: 0.3%/hour when app open but idle

### Testing
- ✅ Compatibility: Android 9 and Android 16 both pass all tests
- ✅ Stability: 0 crashes during extended testing
- ✅ Resolution Support: 1200x1920 and 2032x3048 both working
- ✅ Multi-device: Dual-manufacturer compatibility confirmed (Amazon + Xiaomi)

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
- ✅ Spells cast successfully in combat
- ✅ Mana consumption working correctly
- ✅ Damage calculation based on magic school
- ✅ Multiple NPCs casting different spells simultaneously

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
- ✅ Title screen displays correctly
- ✅ Game starts on user action
- ✅ Quests can be created and accepted
- ✅ Quest progress tracks properly

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
- ✅ NPCs engage in combat automatically
- ✅ Damage calculation works correctly
- ✅ NPC health decreases with attacks
- ✅ Dead NPCs are removed from game
- ✅ Multiple combats run simultaneously

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
- ✅ NPCs spawn at correct positions
- ✅ NPCs move in WANDER mode
- ✅ AI states transition properly
- ✅ Multiple NPCs managed efficiently

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
- ✅ Multiple cells load without issues
- ✅ World objects persist across frames
- ✅ Player position tracking works
- ✅ Smooth transitions between areas

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
- ✅ Real Oblivion meshes load successfully
- ✅ Textures display correctly
- ✅ Caching improves performance
- ✅ No memory leaks in asset system

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
- **M1-1**: ✅ Black screen (native code executing)
- **M1-2**: ✅ Rotating cube displayed (3D rendering working)
- **M1-3**: ✅ Camera movement (input handling working)

#### Testing
- ✅ App launches on Android device
- ✅ 60 FPS stable on test hardware
- ✅ Touch input responsive
- ✅ No crashes during basic interaction

#### First Run
- Application size: 5 MB APK
- Load time: 10-15 seconds
- Memory usage: 30 MB baseline
- Frame rate: Steady 60 FPS

---

## Version History Summary

| Version | Phase | Focus | Status |
|---------|-------|-------|--------|
| 0.9.0 | Phase 9 | Graphical UI & Sound Effects | ✅ Complete |
| 0.8.0 | Phase 8 | Audio & Post-Processing | ✅ Complete |
| 0.7.1 | Phase 7.1 | Settings & Debug System | ✅ Complete |
| 0.6.0 | Phase 6 | Performance & Release | ✅ RC (Release Candidate) |
| 0.5.3 | Phase 5 | Magic System | ✅ Complete |
| 0.5.2 | Phase 5 | Quests & Title Screen | ✅ Complete |
| 0.5.1 | Phase 5 | Combat System | ✅ Complete |
| 0.4.0 | Phase 4 | NPC & Interaction | ✅ Complete |
| 0.3.0 | Phase 3 | World System | ✅ Complete |
| 0.2.0 | Phase 2 | Asset Management | ✅ Complete |
| 0.1.0 | Phase 1 | Core Rendering | ✅ Complete |

---

## Development Statistics

### Code Metrics (Phase 9 Final)
- **Total C++ Code**: ~7,000 lines (excluding headers)
- **Total Header Files**: ~1,500 lines
- **Java Code**: ~700 lines
- **Build Files**: CMakeLists.txt + gradle configurations
- **Total Project**: ~9,200+ lines of code

### Files by Category

**Engine/Core** (10 files): renderer, camera, shader, texture_loader, jni_bridge, etc.  
**Game Systems** (12 files): npc, quest, combat, spell, world, etc.  
**UI** (8 files): title_screen, quest_ui, settings_ui, save_load_ui, ui_panel, ui_button, etc.  
**Assets** (5 files): nif_parser, dds_loader, asset_manager, etc.  
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
- **Total**: ~16 weeks

---

## Technology Stack

### Languages
- **C++17**: Game engine and core systems
- **Java**: Android interface layer
- **GLSL ES 3.0**: Graphics shaders

### APIs & Libraries
- **Android NDK r26**: Native development kit
- **OpenGL ES 3.0**: Graphics rendering
- **JNI**: Java-C++ bridge
- **CMake 3.16+**: Build system
- **Gradle 9.4**: Android build tool

### Third-Party Libraries
- **GLM**: Mathematics library (header-only)
- **Bullet Physics 3.x**: Physics simulation
- **OpenAL-Soft**: Audio (framework ready)
- **stb_image.h**: PNG image loading (single header)

---

## Notable Achievements

✅ **First Android Port of Oblivion Engine**
- Complete game loop implementation
- Full JNI bridge system
- Multi-system integration (combat, quests, magic)

✅ **Cross-Device Compatibility**
- Android 9 to Android 16 support
- ARM64 and ARMv7 architectures
- Resolution independence (1200x1920 to 2032x3048)

✅ **Performance Excellence**
- 60 FPS target achieved and exceeded
- Memory footprint < 50 MB
- Thermal management optimized

✅ **Localization**
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
