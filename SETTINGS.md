# Settings & Debug System Guide

## Overview

The Oblivion Android app includes a comprehensive settings system that allows players to customize their experience and developers to monitor performance. This guide explains how to use and configure the settings.

## User Guide

### Accessing Settings

**From Title Screen:**
1. Launch the app - you'll see the Oblivion logo for 3 seconds
2. Tap the screen to proceed to the main menu
3. You'll see three options:
   - **Start** - Begin the game
   - **Settings** - Configure options (THIS)
   - **Quit** - Exit the app
4. Tap **Settings**

### Settings Menu

Once you've tapped Settings, you'll see a panel with three options:

#### 1. Debug Mode
**Current Status**: Shows ON/OFF

```
┌─────────────────────┐
│     SETTINGS        │
├─────────────────────┤
│ Debug Mode: ON      │  ← Red highlight = Selected
│ Language: Japanese  │     (shows FPS, memory, frame time)
│ Back                │
└─────────────────────┘
```

**What it does**:
- **ON**: Shows real-time debug information overlay (FPS, frame time, memory)
- **OFF**: Hides debug information for cleaner gameplay

**How to toggle**:
1. Tap "Debug Mode: ON/OFF"
2. Selection toggles between ON and OFF
3. Change is saved automatically

#### 2. Language
**Current Status**: Shows Japanese or English

```
Language: Japanese
```

**Supported Languages**:
- **Japanese (日本語)** - All menus in Japanese
- **English** - All menus in English

**How to change**:
1. Tap "Language: Japanese" or "Language: English"
2. Language toggles to the next option
3. All UI text updates immediately

**Affected Elements**:
- Menu text (Start, Settings, Quit)
- Debug HUD information
- NPC dialogue (when expanded)

#### 3. Back
Taps to close the Settings menu and return to the main menu.

### Saving Settings

Settings are **automatically saved** when changed:
- When you toggle Debug Mode ON/OFF
- When you switch language
- Settings persist between app sessions

**Storage Location**:
```
/data/data/com.example.oblivion/settings.txt
```

**Storage Format**:
```
DEBUG_MODE=1
LANGUAGE=ja
```

## Debug HUD System

### What is the Debug HUD?

The Debug HUD is an on-screen overlay that displays real-time performance metrics and system information. It's useful for:
- Monitoring frame rate (FPS)
- Tracking memory usage
- Diagnosing performance issues
- Seeing how many game objects are active

### Enabling Debug HUD

1. Access Settings menu (see "Accessing Settings" above)
2. Tap "Debug Mode: ON"
3. Settings saves automatically
4. Return to game by tapping "Back"

Debug HUD will now display during gameplay.

### Debug HUD Display

When **Debug Mode is ON**, you'll see overlaid text in the top-left corner:

```
FPS: 60.0           ← Frames per second (current)
Frame: 16.67 ms     ← Time for last frame in milliseconds
Avg: 16.50 ms       ← Average frame time (last 0.5 seconds)
Mem: 45 MB          ← Current memory usage
Cubes: 5            ← Number of active game objects
DEBUG: ON           ← Debug mode status
```

### Interpreting Debug Metrics

#### FPS (Frames Per Second)
- **60.0**: Excellent - game running at maximum frame rate
- **30.0**: Acceptable - game running at minimum target frame rate
- **< 30.0**: Warning - game may feel sluggish on this device

**Target**: 60 FPS for smooth gameplay

#### Frame Time (Milliseconds)
- **16.67 ms**: Perfect at 60 FPS (1000ms / 60 frames)
- **33.33 ms**: Equivalent to 30 FPS
- **> 50 ms**: Game may be laggy

**Understanding**: If frame time is high, rendering or logic is taking too long.

#### Average Frame Time
- Running average over 0.5 seconds
- Smooths out single-frame spikes
- Better indicator of overall performance

#### Memory (MB)
- Current heap memory usage
- **40-50 MB**: Typical usage
- **> 200 MB**: Warning - memory may be leaking
- **> 500 MB**: Critical - app may crash

#### Cubes (Object Count)
- Number of active game objects in the world
- Includes NPCs, items, decorations
- Higher count = more CPU/GPU load
- Useful for understanding scene complexity

#### DEBUG Status
- Shows whether Debug Mode is currently ON or OFF
- Confirms settings are applied

### Disabling Debug HUD

1. Access Settings menu
2. Tap "Debug Mode: OFF"
3. Debug HUD disappears
4. Return to game by tapping "Back"

## Performance Tuning

### Using Debug HUD to Optimize

**Scenario 1: FPS is too low (< 30)**
```
Possible causes:
- Too many NPCs active (Cubes > 100)
- Memory pressure (Mem > 200 MB)
- GPU bottleneck

Solutions:
- Reduce NPC count in world
- Clear memory by restarting level
- Reduce texture resolution (future setting)
```

**Scenario 2: Frame time spikes**
```
Possible causes:
- Occasional memory allocations
- Physics calculations
- NPC pathfinding

Solutions:
- Monitor Mem for allocation spikes
- Check if Cubes increases before spike
- Report to developers with logs
```

**Scenario 3: Memory gradually increasing**
```
Possible causes:
- Memory leak in game systems
- Texture cache not clearing
- Old NPC instances not deleted

Solutions:
- Restart app to clear memory
- Enable debug mode to track Mem over time
- Report memory graphs to developers
```

## Developer Guide

### Accessing Settings in Code

#### Reading Current Settings

```cpp
// Get SettingsManager from Renderer
SettingsManager* settings = renderer->getSettingsManager();

// Check if debug mode is enabled
if (settings->isDebugModeEnabled()) {
    // Show debug HUD
    debugHUD->render();
}

// Get current language
std::string lang = settings->getCurrentLanguage();
if (lang == "ja") {
    // Use Japanese strings
}
```

#### Changing Settings Programmatically

```cpp
// Enable debug mode
settings->setDebugMode(true);
settings->saveSettings();

// Change language to English
settings->setLanguage("en");
settings->saveSettings();

// Reset to defaults
settings->resetToDefaults();
```

### Settings File Format

**Location**: `/data/data/com.example.oblivion/settings.txt`

**Format**: Plain text, KEY=VALUE pairs

```
DEBUG_MODE=1
LANGUAGE=ja
```

**Values**:
- `DEBUG_MODE`: 1 (ON) or 0 (OFF)
- `LANGUAGE`: "ja" (Japanese) or "en" (English)

### Adding New Settings

**Step 1**: Update SettingsManager struct

```cpp
struct Settings {
    bool debugModeEnabled;
    std::string currentLanguage;
    bool musicEnabled;  // ← NEW
    int masterVolume;   // ← NEW
};
```

**Step 2**: Update SettingsManager::save/load

```cpp
// In save():
file << "MUSIC_ENABLED=" << (settings.musicEnabled ? "1" : "0") << "\n";
file << "MASTER_VOLUME=" << settings.masterVolume << "\n";

// In load():
if (line.find("MUSIC_ENABLED") != std::string::npos) {
    settings.musicEnabled = (value == "1");
}
```

**Step 3**: Update SettingsUI menu

```cpp
// Add to menu items
menuItems.push_back("music_enabled");
```

**Step 4**: Handle in SettingsUI::selectItem()

```cpp
if (menuItems[selectedIndex] == "music_enabled") {
    settingsManager->setMusicEnabled(
        !settingsManager->isMusicEnabled()
    );
}
```

### Debug HUD Metrics

#### Reading System Memory

```cpp
// In DebugHUD::update()
float memUsageMB = getSystemMemoryUsage();  // Reads /proc/meminfo

// Format as "45 MB"
std::string formattedMem = formatBytes(memUsageMB * 1024 * 1024);
```

#### Measuring Frame Time

```cpp
// In Renderer::render()
auto frameStart = std::chrono::high_resolution_clock::now();

// ... render frame ...

auto frameEnd = std::chrono::high_resolution_clock::now();
float frameTimeMs = std::chrono::duration<float, std::milli>(
    frameEnd - frameStart
).count();
```

#### Calculating FPS

```cpp
// In DebugHUD
float fps = 1000.0f / frameTimeMs;  // If frameTimeMs = 16.67, fps = 60.0
```

### Localization in Settings

Settings menu respects current language:

```cpp
std::string menuTitle = localizationManager->getString("settings_title");
// Returns "設定" in Japanese, "Settings" in English
```

**Localization Keys**:
```
settings_title        → "設定" / "Settings"
settings_debug_mode   → "デバッグモード" / "Debug Mode"
settings_language     → "言語" / "Language"
settings_back         → "戻る" / "Back"
```

## Troubleshooting

### Q: Settings not saving
**A**: Ensure the app has write permission to `/data/data/com.example.oblivion/`. Check logcat:
```bash
adb logcat | grep "SettingsManager"
```

### Q: Debug HUD not showing even when set to ON
**A**: Check:
1. Settings file exists and DEBUG_MODE=1
2. Debug HUD is initialized in Renderer
3. Check logcat for errors: `adb logcat | grep "DebugHUD"`

### Q: Language doesn't change
**A**: Verify LocalizationManager has translations for all UI strings:
```cpp
// Check in logcat
adb logcat | grep "LocalizationManager"
```

### Q: Memory keeps increasing
**A**: Likely memory leak. Monitor with:
```bash
adb shell dumpsys meminfo com.example.oblivion | head -20
```

## Known Limitations

- Settings menu is text-based (graphical version planned for Phase 7.3)
- Limited to 2 languages (English/Japanese)
- No audio volume control in settings yet (Phase 7.2)
- Debug HUD only shows 6 metrics (expandable in future)

## Future Enhancements

### Phase 7.2
- Save/Load game state (will add save management to settings)
- Audio volume control
- Brightness adjustment

### Phase 7.3
- Graphical settings menu with icons
- Animated transitions
- Settings profiles/presets

### Phase 8
- Gameplay difficulty settings
- Graphics quality presets
- Control remapping

---

**Last Updated**: 2026-04-18  
**Version**: 0.7.1  
**Document Status**: Complete
