# Oblivion Android - Known Issues

## Critical Issues (None Currently)

✅ Application is stable on tested devices with no critical crashes

---

## Medium Priority Issues

### Issue 1: Compiler Warnings (34 in jni_bridge.cpp)
**Status**: Open  
**Severity**: Low (doesn't affect functionality)  
**Description**: Unused parameter warnings in JNI method signatures

**Example**:
```
warning: unused parameter 'env' [-Wunused-parameter]
JNIEnv* env,
```

**Impact**: None - warnings are expected in JNI code where parameters are required by signature but not used in implementation

**Resolution**: Fix by adding `[[maybe_unused]]` attributes (C++17) - planned for Phase 7 polish

---

### Issue 2: Performance Monitor Variables
**Status**: Open  
**Severity**: Low  
**Description**: `vmPeak` variable set but not used in performance_monitor.cpp:80

**Impact**: Minimal - affects only profiling debug build, no runtime impact

**Resolution**: Planned for future optimization pass

---

## Low Priority Issues

### Issue 3: Text-Only UI
**Status**: Design Decision  
**Severity**: Low  
**Description**: Game uses text-based menus instead of graphical UI

**Current System**:
- Title screen: Text "Start Game"
- Quest UI: Text list of objectives
- Combat log: Console text output
- NPC interaction: Text display

**Impact**: 
- Reduced visual appeal
- Less intuitive for casual players
- Acceptable for prototype/testing phase

**Resolution**: Graphical UI planned for Phase 7 enhancement

---

### Issue 4: No Save/Load System
**Status**: By Design (Phase 7 feature)  
**Severity**: Low  
**Description**: Game progress not persisted between sessions

**Current Behavior**:
- Each launch creates fresh game state
- All quests reset
- All NPC positions reset
- No progress carries over

**Impact**: 
- Can't save game mid-play
- Progress lost on app close
- Testing must be continuous session

**Resolution**: Planned for Phase 7 (JSON-based save system)

---

### Issue 5: Limited NPC Interaction
**Status**: By Design (Phase 4 foundation)  
**Severity**: Low  
**Description**: NPC dialogue limited to quest offering

**Current Features**:
- ✅ View NPC name and health
- ✅ See offered quests
- ✅ Accept quest
- ❌ Extended dialogue
- ❌ Rumor dialogue
- ❌ Barter/Trade
- ❌ Relationship tracking

**Resolution**: Full dialogue system planned for Phase 7

---

### Issue 6: No Inventory Management UI
**Status**: By Design (Phase 3-4 feature)  
**Severity**: Low  
**Description**: Players cannot view/manage items visually

**Current State**:
- Backend inventory exists
- No frontend UI to display items
- Can't equip/unequip items
- Can't drop items
- Can't use potions

**Resolution**: UI implementation planned for Phase 7

---

## Hardware-Specific Issues

### Issue 7: High Screen Brightness = Battery Drain
**Status**: Environment-dependent  
**Severity**: Low  
**Description**: Screen brightness significantly affects battery consumption

**Observed**:
- Maximum brightness: ~3% drain/hour
- Medium brightness (50%): ~1.5% drain/hour
- Minimum brightness: ~0.5% drain/hour

**Workaround**: Reduce screen brightness before playing

**Note**: This is normal for all mobile games, not a bug

---

### Issue 8: Device Temperature Under Load
**Status**: Normal for intensive apps  
**Severity**: Low  
**Description**: Device heats up during continuous play

**Observed**:
- Initial temp (idle): 32°C
- After 30 minutes: 38-40°C
- Throttling threshold: Device-dependent (typically 45°C+)

**Impact**: 
- Minimal at moderate temps
- FPS may drop if throttling occurs
- Battery drain increases with heat

**Workaround**: 
- Play in cool environment
- Use fan cooling (external)
- Close background apps
- Take 10-minute break every hour

---

## Performance Limitations

### Issue 9: Single-Core CPU Optimization
**Status**: By Design (Phase 6 optimization)  
**Severity**: Low  
**Description**: App primarily uses single CPU core for game logic

**Current**:
- CPU usage: < 0.1% (below top 38 processes)
- 29 threads created but most sleeping
- GPU handles rendering (off-screen to avoid burden)

**Future**: Multi-core optimization for more complex physics

---

### Issue 10: Memory Footprint
**Status**: Acceptable  
**Severity**: Low  
**Description**: App memory usage is moderate but noticeable

**Current Usage**:
- APK size: 9.6 MB (debug), ~8 MB (release expected)
- Runtime memory: 40-50 MB base
- With assets loaded: 150-250 MB total
- Peak: <350 MB on tested devices

**Note**: Well within typical mobile app range (Instagram: 150-300 MB)

**Future**: Streaming and dynamic asset loading will further optimize

---

## Graphics Issues

### Issue 11: Limited 3D Features (Not a bug)
**Status**: By Design - Phase 1 foundation  
**Severity**: Not applicable  
**Description**: Graphics are functional but minimalist

**Current Features**:
- ✅ OpenGL ES 3.0 rendering
- ✅ Basic mesh display
- ✅ Texture mapping
- ❌ Advanced lighting (Phong, PBR)
- ❌ Shadow mapping
- ❌ Particle effects
- ❌ Post-processing effects

**Note**: This is intentional for Phase 1. Enhanced graphics planned for future phases.

---

## Testing Status

### Devices Tested Successfully ✅

| Device | OS | Resolution | Status |
|--------|----|-----------| ------|
| Amazon Fire Tablet | Android 9 | 1200×1920 | ✅ Stable, 60 FPS |
| Xiaomi 24018RPACG | Android 16 | 2032×3048 | ✅ Stable, 60 FPS |

### Devices Not Yet Tested
- Google Pixel (any generation)
- Samsung Galaxy series
- iPhone/iPad (iOS - N/A Android app)
- Devices with <2GB RAM
- Devices older than Android 10

---

## Workarounds for Known Issues

### FPS Drop
1. Close other apps (Settings > Apps > Force Stop)
2. Clear app cache (Settings > Apps > Oblivion > Storage > Clear Cache)
3. Restart device
4. Ensure device temp < 40°C

### App Crash on Start
1. Uninstall: `adb uninstall com.example.oblivion`
2. Reinstall APK
3. Verify Android version ≥ 10.0
4. Check free storage > 500 MB

### High Battery Drain
1. Reduce screen brightness to 30%
2. Close background apps
3. Play in shorter sessions (30-60 minutes)
4. Keep device on charger while testing

### Device Overheating
1. Play in air-conditioned room
2. Remove device case if present
3. Reduce play time to 30-minute sessions
4. Use external cooling fan

---

## Reporting New Issues

If you encounter issues not listed here:

1. **Collect Information**:
   - Device model and OS version
   - When it occurred (during startup, gameplay, etc.)
   - Screenshot of error (if applicable)
   - Logcat output: `adb logcat -d > log.txt`

2. **Check Existing List**: Verify it's not already known

3. **Test on Different Device**: Confirm if issue is device-specific

4. **Include Reproduction Steps**: Exact steps to reproduce the issue

---

## Version History

| Version | Phase | Key Fixes | Known Issues |
|---------|-------|-----------|--------------|
| 0.6.0 | Phase 6 | JNI bridge fix, multi-device test | Compiler warnings, no save system |
| 0.5.3 | Phase 5 | M5-3 magic system complete | Text UI only |
| 0.5.2 | Phase 5 | M5-2 quest system | NPC interactions limited |
| 0.5.1 | Phase 5 | M5-1 combat system | Minimal inventory UI |
| 0.1.0 | Phase 1 | Basic rendering | No game systems |

---

**Last Updated**: 2026-04-17  
**Next Review**: Phase 7 release
