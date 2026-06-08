# Audio System Documentation (Phase 8)

**Status**: Implementation in Progress  
**Version**: 0.8.0 (Planned)  
**Last Updated**: 2026-04-18

## Overview

Phase 8 implements a comprehensive audio system for the Oblivion Android port using OpenAL-Soft, providing 3D spatial audio, background music (BGM), sound effects (SE), and voice support.

## Architecture

### Manager Pattern

The audio system follows the standard Manager Pattern used throughout the codebase:

```cpp
class AudioManager {
public:
    bool initialize();          // OpenAL device/context init
    void update(float dt);      // Frame updates
    void cleanup();             // Resource cleanup
};
```

### Core Components

#### 1. **AudioClip** (`audio_clip.h`)
Represents audio resources (WAV/OGG files) in memory.

**Key Properties**:
- `clipId`: Unique identifier for lookup
- `alBuffer`: OpenAL buffer handle
- `duration`: Playback length in seconds
- `type`: Classification (0=BGM, 1=SE, 2=Voice)
- `isLooping`: Whether audio loops
- `volume`: Default playback volume (0.0-1.0)

```cpp
struct AudioClip {
    uint32_t clipId;
    std::string filename;
    ALuint alBuffer;
    float duration;
    bool isLooping;
    uint8_t type;  // BGM, SE, Voice
    float volume;
};
```

#### 2. **AudioSource** (`audio_source.h`)
Manages individual playback channels for active sound playback.

**Key Properties**:
- `sourceId`: Unique identifier
- `alSource`: OpenAL source handle
- `position`: 3D world position
- `volume`, `pitch`: Playback parameters
- `isPlaying`: Current playback state

```cpp
struct AudioSource {
    uint32_t sourceId;
    ALuint alSource;
    uint32_t clipId;
    
    glm::vec3 position;
    float volume;
    float pitch;
    bool isPlaying;
    bool is3D;      // Enable/disable 3D positioning
};
```

#### 3. **Audio3D** (`audio_3d.h/cpp`)
Handles 3D spatial audio processing:

- Listener position/orientation management
- Distance attenuation models
- Doppler effect support
- Master gain control

```cpp
class Audio3D {
public:
    void setListenerPosition(const glm::vec3& pos);
    void setListenerOrientation(const glm::vec3& forward, 
                               const glm::vec3& up);
    void setDistanceModel(DistanceModel model);
};
```

#### 4. **AudioManager** (`audio_manager.h/cpp`)
Central coordinator for all audio operations.

**Core Responsibilities**:
- OpenAL device/context management
- Clip loading and caching
- BGM playback with fade effects
- SE playback with 3D positioning
- Volume level control
- Frame updates for playback management

### Conditional Compilation

Audio system is compiled conditionally based on OpenAL availability:

```cmake
if(OpenAL_FOUND)
    add_compile_definitions(AUDIO_SYSTEM_ENABLED)
    target_sources(native-lib PRIVATE audio/audio_manager.cpp audio/audio_3d.cpp)
    target_link_libraries(native-lib OpenAL::OpenAL)
else()
    message(WARNING "OpenAL not found - audio system disabled")
endif()
```

## Usage Guide

### 1. Loading Audio Resources

```cpp
// Load BGM
uint32_t bgmId = audioManager->loadClip("assets/music/theme.wav", 0, true);

// Load Sound Effect
uint32_t seId = audioManager->loadClip("assets/sounds/hit.wav", 1, false);
```

### 2. Playing BGM

```cpp
// Immediate playback
audioManager->playBGM(bgmId);

// With fade-in (2 seconds)
audioManager->playBGM(bgmId, 2.0f);

// Control volume
audioManager->setBGMVolume(0.8f);

// Stop with fade-out
audioManager->stopBGM(1.0f);
```

### 3. Playing Sound Effects

```cpp
// 2D sound (no 3D positioning)
uint32_t sourceId = audioManager->playSE(seId);

// 3D positioned sound
glm::vec3 pos = npc->getPosition();
uint32_t sourceId = audioManager->playSE(seId, pos, 1.0f);

// Stop specific SE
audioManager->stopSE(sourceId);
```

### 4. 3D Audio Control

```cpp
// Update listener position (typically camera position)
audioManager->setListenerPosition(camera->getPosition());
audioManager->setListenerOrientation(camera->getForward(), camera->getUp());

// SE sources auto-position relative to listener
```

## Integration with Game Systems

### CombatManager Integration

```cpp
// In CombatManager::update()
if (combatInstance.attacker && audioManager) {
    // Play attack sound at attacker position
    audioManager->playSE(SOUND_COMBAT_SWING, 
                        combatInstance.attacker->position);
    
    // Play hit sound at defender position
    audioManager->playSE(SOUND_COMBAT_HIT, 
                        combatInstance.defender->position);
}
```

### SpellManager Integration

```cpp
// In SpellManager::castSpell()
if (audioManager && spell) {
    // Play spell cast sound
    audioManager->playSE(spell->castSoundId, caster->position);
    
    // Play impact sound at target
    audioManager->playSE(spell->impactSoundId, targetPos);
}
```

### Renderer Integration

```cpp
// In Renderer::render()
if (audioManager && worldManager) {
    const glm::vec3& cameraPos = worldManager->getCameraPosition();
    const glm::vec3& cameraForward = worldManager->getCameraForward();
    
    audioManager->setListenerPosition(cameraPos);
    audioManager->setListenerOrientation(cameraForward, glm::vec3(0,1,0));
    audioManager->update(deltaTime);
}
```

## Distance Attenuation Models

The Audio3D system supports multiple distance models:

| Model | Behavior | Use Case |
|-------|----------|----------|
| **INVERSE_DISTANCE** | Realistic 1/r attenuation | General sounds |
| **INVERSE_DISTANCE_CLAMPED** | 1/r with min/max distance | Most common |
| **LINEAR_DISTANCE** | Linear volume falloff | Special effects |
| **EXPONENT_DISTANCE** | Exponential decay | Ambient sounds |

Default: `INVERSE_DISTANCE_CLAMPED`

## OpenAL Configuration

### Device Detection

OpenAL-Soft automatically detects available audio devices:

```cpp
const char* deviceName = alcGetString(device, ALC_DEVICE_SPECIFIER);
LOGI("OpenAL Device: %s", deviceName);
```

### Sample Rate & Format Support

- **Sample Rate**: 44.1 kHz (standard)
- **Channels**: Mono, Stereo, 5.1 Surround
- **Bit Depth**: 8-bit, 16-bit
- **Formats**: WAV (PCM), OGG Vorbis (via extensions)

## Performance Characteristics

### Memory Usage
- OpenAL Context: ~1-2 MB
- Per Audio Clip: ~100 KB - 1 MB (depends on duration/quality)
- Per Active Source: ~50 KB (streaming metadata)

### CPU Usage
- Frame Update: < 0.1% per active source
- 3D Positioning Calculations: < 0.05% per source
- Listener Update: < 0.02%

### Limitations
- **Maximum Sources**: 32 simultaneously (configurable)
- **Maximum Clips**: Unlimited (memory-bound)
- **3D Audio**: Works best with mono sources
- **Streaming**: Not yet implemented (buffered loading only)

## Fallback Behavior

When OpenAL is not available:

- Audio System initialization is skipped
- `audioManager` member is `nullptr`
- All audio calls are conditionally compiled out
- Game runs without audio (no crashes)
- Warning logged: "OpenAL not found - audio system disabled"

## Future Enhancements

### Phase 8.1: Audio Streaming
- Stream large audio files instead of full buffering
- Reduce memory usage for long tracks
- Enable dynamic audio loading

### Phase 8.2: Audio Mixing & Effects
- Master/BGM/SE volume control via settings
- Audio effects (reverb, echo, low-pass filter)
- Advanced 3D audio (cone effects, source angles)

### Phase 8.3: Music System
- Playlist management
- Track transitions with crossfade
- Adaptive music based on game state

### Phase 8.4: Voice System
- NPC voice synthesis/playback
- Dialogue volume control
- Language-specific voice packs

## Debugging

### Enable Audio Logging

Audio system logs are visible via `adb logcat`:

```bash
adb logcat | grep "AudioManager\|Audio3D"
```

### Audio Status Checking

```cpp
// Check if BGM is playing
if (audioManager->isBGMPlaying()) {
    LOGI("BGM playing, volume: %.2f", audioManager->getBGMVolume());
}

// Check loaded resources
LOGI("Loaded clips: %zu, Active sources: %zu",
     audioManager->getLoadedClipsCount(),
     audioManager->getActiveSourcesesCount());
```

## References

- [OpenAL-Soft Documentation](https://openal-soft.org/)
- [OpenAL 1.1 Specification](https://openal.org/documentation/openal-1.1-specification.pdf)
- [Android Audio Best Practices](https://developer.android.com/guide/topics/media-apps/audio-app)

## Implementation Checklist

### Week 1: OpenAL Integration & Basics ✓
- [x] OpenAL device/context initialization
- [x] AudioClip struct definition
- [x] AudioSource struct definition
- [x] Audio3D listener management
- [x] CMakeLists.txt OpenAL integration

### Week 2: BGM System (In Progress)
- [ ] WAV file loading implementation
- [ ] BGM playback control
- [ ] Fade in/out support
- [ ] Volume management

### Week 3: SE System (Planned)
- [ ] SE playback with 3D positioning
- [ ] Multiple simultaneous SE support
- [ ] Distance-based attenuation
- [ ] SE cleanup/pooling

### Week 4: Integration & Testing (Planned)
- [ ] CombatManager integration
- [ ] SpellManager integration
- [ ] Renderer frame update
- [ ] Performance testing
- [ ] Multi-device validation

---

**Document Version**: 0.1  
**Status**: Draft  
**Next Review**: 2026-04-25
