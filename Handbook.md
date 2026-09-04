# Oblivion Android - Project Handbook

## Project Philosophy

### Core Principle: Faithful to Original

**Primary Goal**: Replicate the original Oblivion experience as closely as possible on Android.

This project is a **port**, not a remake or reimagining. All features should first be implemented to match the original game's behavior, visuals, and mechanics.

### Modern Enhancements as Options

Any modern improvements or quality-of-life features (e.g., RetroFilter effects, graphical enhancements, control scheme changes) **must** be:

1. **Optional** - Disabled by default, toggleable via Settings menu
2. **Non-intrusive** - Must not break or alter the original gameplay loop when disabled
3. **Documented** - Clearly labeled as "Enhancement" or "Optional Feature" in UI and docs

### Implementation Order

1. **First**: Implement the original feature faithfully
2. **Second**: Add optional modern enhancements (if desired)
3. **Never**: Replace original behavior with modern alternatives

### Asset Strategy

- **Priority 1**: Use original game assets (BSA extraction, DDS textures, NIF meshes)
- **Priority 2**: Create faithful replacements matching original style
- **Priority 3**: Placeholder assets as temporary measures only

### UI Design

- Follow original Oblivion menus: parchment texture backgrounds, stone-style fonts, medieval UI elements
- Modern UI frameworks (Material Design, etc.) are **not** to be used
- Touch controls should emulate original controller/keyboard behavior, not replace it

### Audio

- Use original audio files (WAV, MP3 from game data)
- Background music, sound effects, and dialogue must match original

### Performance Target

- Target original game's performance profile, not modern standards
- 30 FPS was the original console target; 60 FPS is acceptable as an optional enhancement

### Versioning

- Major versions correspond to original game content completeness
- Optional features do not bump major version numbers

---

## Development Guidelines

### Code Style

- C++17 standard
- Follow existing naming conventions in the codebase
- Comment in English, user-facing strings bilingual (EN/JA)

### Naming Conventions

- **C++**: snake_case (variables, functions), PascalCase (classes), UPPER_SNAKE_CASE (constants)
- **Kotlin/Java**: camelCase (methods, variables), PascalCase (classes)
- **File names**: snake_case (C++), PascalCase (Kotlin)

### Testing Requirements

- All original features must work without optional enhancements enabled
- Optional features must have independent test coverage
- Device testing on Android 9+ required before merge

### Documentation

- Update CHANGELOG.md for every feature
- Update README.md for user-facing changes
- Document optional features separately from core features

---

## Key Technical Decisions

### Namespace Architecture

| Namespace | Classes |
|-----------|---------|
| Global | Renderer, WorldManager, NpcManager, CombatManager, QuestManager, CollisionWorld, PlayerController, InventoryManager, SpellManager, AudioManager, EquipmentEffectSystem |
| `animation::` | AnimationPlayer |
| `ai::` | AIScheduler |
| `oblivion::` | NavMeshManager, PhysicsManager, AlchemySystem, BookReader, ClothingConverter |

### Player is NPC ID 1

`npcManager->getNPC(1)` returns the player's NPC object. The player is treated as the first NPC in the system.

### GLM on Android NDK

`glm::mat4(1.0f)` does NOT compile on Android NDK. Use `glm::mat4()` instead.

### Build System

- NDK 26.1.10909125
- Clang++
- Android API 29+ target
- C++17

### EventBus Pattern

CombatManager and other systems emit events to the Imperial Weave EventBus. Subscribers (AnimationSubscriber, AudioSubscriber) react independently. Systems never call each other directly.

```
ATK button -> PlayerController.attack()
           -> CombatManager.playerAttack()
           -> EventBus emit "COMBAT_ATTACK_HIT"
                +-- AnimationSubscriber -> target plays hit-reaction anim
                +-- AudioSubscriber     -> combat hit SE (weapon-type routed)
                +-- UIFloatingText      -> "Hit!" appears on screen
```

### Manager Pattern

All managers follow the lifecycle: `initialize()` -> `update(dt)` -> `cleanup()`.

---

## Contribution Guide

### Before Starting

1. Read this Handbook thoroughly
2. Check CHANGELOG.md for current project status
3. Review existing code style in nearby files

### Making Changes

1. Create a feature branch from master
2. Make focused, atomic commits
3. Update CHANGELOG.md with each meaningful change
4. Test on a physical Android device (API 29+)
5. Submit a pull request

### Commit Messages

- Use clear, descriptive commit messages
- Prefix with component name when relevant (e.g., "audio:", "combat:", "ui:")

---

*Last Updated: 2026-09-04*
