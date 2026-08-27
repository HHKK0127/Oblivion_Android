# Oblivion Android - Project Handbook

## Project Philosophy / プロジェクト理念

### Core Principle: Faithful to Original / 原作厳守

**Primary Goal**: Replicate the original Oblivion experience as closely as possible on Android.
**主要目標**: オリジナルのOblivion体験をAndroid上で可能な限り忠実に再現すること。

This project is a **port**, not a remake or reimagining. All features should first be implemented to match the original game's behavior, visuals, and mechanics.
このプロジェクトは**移植**であり、リメイクや再解釈ではありません。すべての機能は、まずオリジナルゲームの動作、ビジュアル、メカニクスに合わせて実装されるべきです。

### Modern Enhancements as Options / 現代的な改変はオプション

Any modern improvements or quality-of-life features (e.g., RetroFilter effects, graphical enhancements, control scheme changes) **must** be:
現代的な改善やQoL機能（例：RetroFilter効果、グラフィック強化、操作変更など）は**必ず**：

1. **Optional / オプション** - Disabled by default, toggleable via Settings menu
   **オプション** - デフォルトで無効、設定メニューで切り替え可能
2. **Non-intrusive / 非侵襲的** - Must not break or alter the original gameplay loop when disabled
   **非侵襲的** - 無効時にオリジナルのゲームプレイループを破壊・変更してはならない
3. **Documented / 文書化** - Clearly labeled as "Enhancement" or "Optional Feature" in UI and docs
   **文書化** - UIやドキュメントで「強化機能」または「オプション機能」と明確に表記

### Implementation Order / 実装順序

1. **First**: Implement the original feature faithfully
   **最初に**: オリジナルの機能を忠実に実装
2. **Second**: Add optional modern enhancements (if desired)
   **次に**: オプションとして現代的な強化を追加（必要な場合）
3. **Never**: Replace original behavior with modern alternatives
   **決して**: オリジナルの動作を現代的な代替で置き換えない

### Asset Strategy / アセット戦略

- **Priority 1**: Use original game assets (BSA extraction, DDS textures, NIF meshes)
  **優先度1**: オリジナルゲームアセットを使用（BSA抽出、DDSテクスチャ、NIFメッシュ）
- **Priority 2**: Create faithful replacements matching original style
  **優先度2**: オリジナルスタイルに合わせた忠実な代替品を作成
- **Priority 3**: Placeholder assets as temporary measures only
  **優先度3**: プレースホルダーアセットは一時的な措置のみ

### UI Design / UIデザイン

- Follow original Oblivion menus: parchment texture backgrounds, stone-style fonts, medieval UI elements
  オリジナルのOblivionメニューに従う：羊皮紙テクスチャ背景、石造り風フォント、中世UI要素
- Modern UI frameworks (Material Design, etc.) are **not** to be used
  現代的なUIフレームワーク（Material Design等）は使用**しない**
- Touch controls should emulate original controller/keyboard behavior, not replace it
  タッチ操作はオリジナルのコントローラー/キーボード動作をエミュレートし、置き換えない

### Audio / オーディオ

- Use original audio files (WAV, MP3 from game data)
  オリジナルのオーディオファイルを使用（ゲームデータからWAV、MP3）
- Background music, sound effects, and dialogue must match original
  BGM、効果音、ボイスはオリジナルに合わせる

### Performance Target / パフォーマンス目標

- Target original game's performance profile, not modern standards
  現代的な標準ではなく、オリジナルゲームのパフォーマンスプロファイルを目標とする
- 30 FPS was the original console target; 60 FPS is acceptable as an optional enhancement
  30 FPSはオリジナルのコンソール目標；60 FPSはオプションの強化として許容される

### Versioning / バージョニング

- Major versions correspond to original game content completeness
  メジャーバージョンはオリジナルゲームコンテンツの完全性に対応
- Optional features do not bump major version numbers
  オプション機能はメジャーバージョン番号を上げない

---

## Development Guidelines / 開発ガイドライン

### Code Style
- C++17 standard
- Follow existing naming conventions in the codebase
- Comment in English, user-facing strings bilingual (EN/JA)

### Testing Requirements
- All original features must work without optional enhancements enabled
- Optional features must have independent test coverage
- Device testing on Android 9+ required before merge

### Documentation
- Update CHANGELOG.md for every feature
- Update README.md for user-facing changes
- Document optional features separately from core features

---

## Current Project Status / 現在のプロジェクト状況

### Phase Completion (as of 2026-08-27)

| Phase | Name | Status |
|-------|------|--------|
| 1-28 | Core Engine + ESM Integration | ✅ COMPLETE |
| 29 | NAVM Pathfinding + DIAL/INFO + REFR | ✅ COMPLETE |
| 30 | NIF/Skeleton/Skinning/Animation/Collision | ✅ COMPLETE |
| 31 | PlayerController + World Loading | ✅ COMPLETE |
| 32 | Imperial Weave EventBus + 12-phase Coordinator | ✅ COMPLETE |
| 33 | Dedicated Combat Sounds + NPC Spatial Audio | ✅ COMPLETE |
| 34 | Weapon Sound Routing + Quick-Slot Spells | ✅ COMPLETE |
| 35 | Radiant AI System | ✅ COMPLETE |
| 36 | Jolt Physics Integration | ✅ COMPLETE |
| 37 | Script VM (Oblivion Script Execution) | 📋 DESIGNED |

### Code Metrics
- **C++**: 22,500+ lines
- **Java/Kotlin**: 700+ lines
- **ESM Record Types**: 40
- **Bug Fixes Completed**: 72 (across 6 batches)

### Architecture Overview

```
Android JNI
    └── Renderer  (initialization & render loop)
         ├── Imperial Weave  (12-phase update coordinator)
         │    ├── EventBus  (loose-coupled messaging)
         │    └── Phase pipeline:
         │         ①EventProcess → ②World → ③AI → ④Player → ⑤Inventory
         │         → ⑥Spell → ⑦Animation → ⑧Physics → ⑨Combat
         │         → ⑩Quest → ⑪Audio → ⑫RenderSubmit
         ├── UISystem  (HUD, panels, floating text)
         └── Subscriber bridges (thin, EventBus-driven):
              ├── AnimationSubscriber  (Event → AnimationPlayer)
              └── AudioSubscriber     (Event → AudioManager)
```

### Key Technical Decisions

1. **Namespace Architecture**:
   - Global: Renderer, WorldManager, NpcManager, CombatManager, QuestManager, CollisionWorld, PlayerController, InventoryManager, SpellManager, AudioManager
   - `animation::` namespace: AnimationPlayer
   - `ai::` namespace: AIScheduler
   - `oblivion::` namespace: NavMeshManager, PhysicsManager, AlchemySystem, BookReader, ClothingConverter

2. **Player is NPC ID 1**: `npcManager->getNPC(1)` returns the player's NPC object

3. **GLM on Android NDK**: `glm::mat4(1.0f)` does NOT compile - must use `glm::mat4()`

4. **Build System**: NDK 26.1.10909125, Clang++, Android API 24 target, C++17

### Bug Fix History

| Batch | Bugs Fixed | Key Issues |
|-------|------------|------------|
| 1 | 12 | Uninitialized variables, missing bounds checks |
| 2 | 11 | Division by zero, temp file leaks |
| 3 | 12 | Self-assignment bugs, raw pointer leaks |
| 4 | 11 | Stat accumulation, null pointer access |
| 5 | 10 | Transition-only callbacks, empty function bodies |
| 6 | 8 | Integer overflow, RNG initialization, state transitions |
| **Total** | **64** | |

### Phase 37: Script VM (Designed)

**Status**: Design complete, implementation pending

**Key Components**:
- SCPT record parser (EDID, SCHR, SCDA, SCTX, SLSD/SCVR, SCRO)
- Bytecode interpreter (opcode + arg length + args format)
- Game function API (SetStage, AddItem, Enable, Disable, etc.)
- Imperial Weave integration (SCRIPT phase)

**Estimated**: ~2,050 lines of new code

---

*Last Updated: 2026-08-27*
*Version: 2.0*
