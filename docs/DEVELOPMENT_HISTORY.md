# Oblivion Android - 開発履歴

**最終更新**: 2026-09-04
**現在バージョン**: 1.6.0
**現在フェーズ**: Phase 63 完了

---

## 目次

1. [プロジェクト概要](#プロジェクト概要)
2. [フェーズ一覧](#フェーズ一覧)
3. [Phase 1-9: 基盤構築](#phase-1-9-基盤構築)
4. [Phase 10-28: ESM統合とゲームシステム](#phase-10-28-esm統合とゲームシステム)
5. [Phase 29-36: 高度なゲーム機能](#phase-29-36-高度なゲーム機能)
6. [Phase 37-49: コアゲームプレイ](#phase-37-49-コアゲームプレイ)
7. [Phase 50-57: ビジュアル拡張と統合](#phase-50-57-ビジュアル拡張と統合)
8. [Phase 58-63: アセット最適化とリリース準備](#phase-58-63-アセット最適化とリリース準備)
9. [マイルストーン一覧](#マイルストーン一覧)
10. [コード統計](#コード統計)

---

## プロジェクト概要

Oblivion Androidは、The Elder Scrolls: OblivionのネイティブAndroid移植プロジェクトです。OpenGL ES 3.0を使用した3Dレンダリング、JNIブリッジによるJava/C++連携、OpenAL-Softによる3Dオーディオなど、オリジナルゲームの体験をAndroid上で再現することを目標としています。

### 技術スタック

| コンポーネント | 技術 |
|--------------|------|
| レンダリング | OpenGL ES 3.0 |
| ネイティブコード | C++17 (NDK r26.1) |
| Java連携 | JNI (Java Native Interface) |
| オーディオ | OpenAL-Soft |
| 物理エンジン | Jolt Physics |
| ビルドシステム | CMake 3.16+ / Gradle 9.4+ |
| 対象API | Android 10+ (API 29+) |

---

## フェーズ一覧

| フェーズ | バージョン | ステータス | 主要成果 |
|---------|----------|----------|---------|
| Phase 1-7 | 0.1.0-0.7.0 | 完了 | 基盤構築からリリース準備まで |
| Phase 8 | 0.8.0 | 完了 | オーディオシステム、RetroFilter |
| Phase 9-24 | 0.9.0-0.9.5 | 完了 | グラフィカルUI、HUDシステム |
| Phase 25-28 | 0.9.6-0.9.10 | 完了 | ESMパーサー、40レコードタイプ統合 |
| Phase 29 | 0.9.10 | 完了 | NAVM経路探索、DIAL/INFO対話、REFR配置 |
| Phase 30 | 0.9.10 | 完了 | NIFスケルトン、スキニング、アニメーション、衝突判定 |
| Phase 31 | 0.9.10 | 完了 | PlayerController統合、ワールドローディング |
| Phase 32 | 0.9.8 | 完了 | Imperial Weave EventBus、サブスクライバー |
| Phase 33 | 0.9.9 | 完了 | 専用戦闘音、NPC空間オーディオ |
| Phase 34 | 0.9.10 | 完了 | 武器種別音、クイックスロット呪文 |
| Phase 35 | 1.0.0 | 完了 | Radiant AIシステム |
| Phase 36 | 1.1.0 | 完了 | Jolt Physics統合 |
| Phase 37 | 1.2.0 | 完了 | Oblivion Script VM（47オペコード） |
| Phase 38 | 1.3.0 | 完了 | Script VM単体テスト（20件） |
| Phase 39 | 1.4.0 | 完了 | クエストフローシステム |
| Phase 40 | 1.5.0 | 完了 | NPCダイアログツリー |
| Phase 41 | 1.6.0 | 完了 | バイナリセーブシステム |
| Phase 42 | 2.0.0 | 完了 | ゲームループ統合 |
| Phase 43 | 2.1.0 | 完了 | UI/UXシステム |
| Phase 44 | 2.2.0 | 完了 | パフォーマンス最適化 |
| Phase 45 | 2.3.0 | 完了 | 単体テスト（37件） |
| Phase 46-49 | 2.4.0 | 完了 | アセットパイプライン、オーディオ、統合テスト、入力 |
| Phase 50-53 | 2.5.0 | 完了 | LOD、SpeedTree、FaceGen、Bink |
| Phase 54 | 3.0.0 | 完了 | Imperial Weave v4.0（15フェーズパイプライン） |
| Phase 55 | 3.1.0 | 完了 | エンジン磨き上げ、最適化 |
| Phase 56 | 3.2.0 | 完了 | Gamebryo完全互換（パーティクル、水面、空、マテリアル） |
| Phase 57 | 1.0.0 | 完了 | リリース統合 |
| Phase 58-63 | 1.1.0-1.6.0 | 完了 | アセット最適化、圧縮、リリース準備 |

---

## Phase 1-9: 基盤構築

### Phase 1: 基盤構築 (v0.1.0)

- OpenGL ES 3.0レンダリングエンジンの実装
- カメラ制御システム
- JNIブリッジ基盤

### Phase 2: アセット管理 (v0.2.0)

- NIFパーサー（メッシュデータ抽出）
- DDSテクスチャローダー（DXT1/DXT3/DXT5）
- LRUキャッシュ付きアセットマネージャー

### Phase 3: ゲームワールド (v0.3.0)

- セルベースワールドシステム
- ワールドストリーミング
- NPCスポーンシステム

### Phase 4: NPC＆インタラクション (v0.4.0)

- 双方向NPC↔セルマッピング
- ドアシステム（セル間遷移）
- コンテナシステム（インベントリ管理）

### Phase 5: 戦闘＆クエスト (v0.5.0)

- CharacterStatus、CombatManager、ダメージ計算式
- クエストシステム（受注、状態管理、完了判定）
- 魔法システム（6学校、10+呪文）

### Phase 6: パフォーマンス最適化 (v0.6.0)

- 60 FPS達成（目標30 FPSの2倍）
- メモリ40 MB（目標1 GBの25分の1）
- 実機検証（Amazon Fire、Xiaomi）

### Phase 7: リリース準備 (v0.7.0)

- TextRenderer、Debug HUD、SettingsManager
- Settings UI、タイトル画面統合
- リリースビルド（APK 8.4 MB）

### Phase 8: オーディオシステム (v0.8.0)

- OpenAL-Soft統合、JNI Audio Bridge
- RetroFilterエフェクト（ピクセル化、走査線、CRT歪み）
- SaveLoadUIシステム、設定UI

### Phase 9-24: グラフィカルUI & HUD (v0.9.0-v0.9.5)

- TextureLoader、UIPanel、UIButton
- HUD＆ステータス表示（ミニマップ、コンパス、浮動テキスト等）
- コアUIメニュー（ポーズ、キャラクター、ショップ、クエストログ、対話）

---

## Phase 10-28: ESM統合とゲームシステム

### Phase 25: BSA/ESMパーサー (v0.9.6)

- BSAアーカイブリーダー（ZLib展開）
- ESMファイルパーサー（完全GRUP階層）

### Phase 26: ESMデータ駆動ワールド (v0.9.6)

- 20レコードタイプ解析：CELL、NPC_、WEAP、REFR、LAND、WRLD、SPEL、LVLI/LVLC、NAVM、ARMO等

### Phase 27: ESM統合拡張 (v0.9.6)

- BOOK、CLOT、INGR、ALCH、MISC、FACT、RACE、CLAS、ROAD + ルート生成、錬金術、派閥システム

### Phase 28: ESM 40レコードタイプ統合 (v0.9.10)

- BSGN、CREA、CONT、DOOR、ACTI、STAT、TREE、FLOR、LIGH等の20レコード追加
- NpcManager、Container、PlayerのESM統合

---

## Phase 29-36: 高度なゲーム機能

### Phase 29: NAVM経路探索 + DIAL/INFO対話 (v0.9.10)

- NAVMランタイム統合（CombatManager A*経路探索）
- DIAL/INFOレコード解析（派閥分岐対話）
- REFRワールドオブジェクト配置（8種類）
- 4呪文効果（麻痺、透明化、属性強化、召喚）

### Phase 30: NIFスケルトン/スキニング/衝突判定 (v0.9.10)

- スキンパーティション、スケルトン（BFS）、スキニングメッシュ＋UBO
- NiControllerManager/Sequence解析、AnimationPlayer
- bhkCollisionObject＋bhkRigidBody解析（9形状タイプ）
- Dynamic AABB Tree（広域フェーズ）、CollisionWorld

### Phase 31: PlayerController統合 + ワールドローディング (v0.9.10)

- WorldEntity構造体、WorldLoader（loadStatic/loadDynamic/loadActor）
- PlayerController拡張（スケルトン＋AnimationPlayer＋CharacterController）
- ヒステリシスアニメーション状態機械

### Phase 32: Imperial Weave EventBus + サブスクライバー (v0.9.8)

- AnimationSubscriber（EventBus → AnimationPlayerブリッジ）
- AudioSubscriber（EventBus → AudioManagerブリッジ）
- SpellSelectionPanel UI
- Imperial Weave Event.targetIdフィールド

### Phase 33: 専用戦闘音 + NPC空間オーディオ (v0.9.9)

- 11専用戦闘音定義（ヒット、ブロック、パリィ、ドッジ、デス）
- NPC空間オーディオコールバック

### Phase 34: 武器種別音 + クイックスロット (v0.9.10)

- 武器タイプ別ヒット音ルーティング（ブレード、鈍器、斧、弓、杖）
- スペル選択パネルの学校カラー、F1-F4クイックスロット

### Phase 35: Radiant AIシステム (v1.0.0)

- 15種類のAIパッケージ（探索、追従、警護、パトロール、戦闘、逃走等）
- 優先度ベースPackageStack（戦闘/逃走オーバーライド）
- AIScheduler（24時間ベースのNPC日課）
- NavMesh経路探索（A*＋パススムージング）

### Phase 36: Jolt Physics統合 (v1.1.0)

- PhysicsManagerシングルトン
- CharacterVirtual（プレイヤー/NPC用カプセル形状コントローラー）
- HeightFieldShape地形衝突
- 固定タイムステップ（1/60秒）
- Raycast API

---

## Phase 37-49: コアゲームプレイ

### Phase 37: Oblivion Script VM (v1.2.0)

- バイトコードインタプリタ（47オペコード）
- 118ゲーム関数（Tier 1: 13基本、Tier 2: 105拡張）
- ScriptManager、ExecutionContext、ScriptDisasm

### Phase 38: Script VMテスト (v1.3.0)

- 20単体テスト（ExecutionContext、ScriptVM、Opcode、ScriptFunctions、ScriptManager）

### Phase 39: クエストフローシステム (v1.4.0)

- QuestFlowController、QuestStageManager、QuestObjectiveTracker
- QuestRewards（経験値、ゴールド、アイテム、スキル）

### Phase 40: NPCダイアログツリー (v1.5.0)

- DialogueTree、DialogueRunner、DialogueFilterEngine
- DialogueHistory、DialogueRecord

### Phase 41: バイナリセーブシステム (v1.6.0)

- SaveManager（バイナリフォーマット）
- SaveSlotManager、AutoSave、Serializableインターフェース

### Phase 42: ゲームループ統合 (v2.0.0)

- StateManager、InputRouter、GameLoopCoordinator
- SceneRenderer、DebugConsole、PerformanceProfiler

### Phase 43: UI/UXシステム (v2.1.0)

- TouchGestureHandler、MenuTransitionManager
- HudLayout、ControlSchemeManager、AccessibilityManager

### Phase 44: パフォーマンス最適化 (v2.2.0)

- MemoryPool、RenderOptimizer、AsyncTaskManager、CacheManager
- ProfilerDashboard

### Phase 45: 単体テスト (v2.3.0)

- 37テストケース

### Phase 46-49: アセット、オーディオ、統合、入力 (v2.4.0)

- TextureManager、MeshLoader、WorldDataLoader（+2,487行）
- AudioDecoder、BgmManager、SoundEffectManager（+2,176行）
- 12統合テストケース（+1,159行）
- GamepadMapper、TouchCalibration、InputVisualizer（+1,812行）

---

## Phase 50-57: ビジュアル拡張と統合

### Phase 50: Distant LODシステム (v2.5.0)

- DistantLodManager、LODメッシュ生成、6面フラスタムカリング
- 距離フェード、HorizonRing山プリセット

### Phase 51: SpeedTree植生 (v2.5.0)

- SpeedTreeManager、4段階LOD、インスタンス描画
- ビルボードフォールバック、パーリン風場

### Phase 52: FaceGenシステム (v2.5.0)

- FaceGenManager、種族モーフィング、表情モーフィング
- 髪/髭システム、テクスチャアトラス

### Phase 53: Binkビデオプレーヤー (v2.5.0)

- BinkVideoPlayer、MediaCodec JNIブリッジ

### Phase 54: Imperial Weave v4.0 (v3.0.0)

- 15フェーズパイプライン、ImperialWeaveConfig
- ServiceLocator、12イベントタイプ

### Phase 55: エンジン磨き上げ (v3.1.0)

- FrameBudgetManager（16.6ms/フレーム）
- MemoryDefrag、ShaderCache（LRU）
- OcclusionCuller、BatchRenderer

### Phase 56: Gamebryo完全互換 (v3.2.0)

- ParticleSystem（7プリセット）
- PostProcessPipeline（8エフェクト）
- WaterRenderer（Gerstner波、6タイプ）
- SkyWeatherSystem（8天候、昼夜サイクル）
- SceneGraph、MaterialSystem

### Phase 57: リリース統合 (v1.0.0)

- リリースビルド、APK最適化
- ドキュメント更新

---

## Phase 58-63: アセット最適化とリリース準備

### Phase 58-63: アセット最適化 (v1.1.0-v1.6.0)

- AssetExtractor：外部ストレージアセット管理
- TextureCompressor：ASTC 4x4/6x6/8x8、ETC2 RGB/RGBA
- AudioCompressor：WAV→OGG/MP3/AAC変換
- LODSystem：距離ベースメッシュ詳細レベル
- バッチ圧縮スクリプト
- 最終テスト、APK分析、圧縮ツール

---

## マイルストーン一覧

| マイルストーン | フェーズ | 状態 |
|-------------|---------|------|
| 基本レンダリング | Phase 1 | 完了 |
| NIFメッシュ表示 | Phase 2 | 完了 |
| セルベースワールド | Phase 3 | 完了 |
| NPC統合 | Phase 4 | 完了 |
| 戦闘＆クエスト | Phase 5 | 完了 |
| パフォーマンス最適化 | Phase 6 | 完了 |
| オーディオシステム | Phase 8 | 完了 |
| グラフィカルUI | Phase 9-24 | 完了 |
| ESM 40レコード統合 | Phase 28 | 完了 |
| NAVM経路探索 | Phase 29 | 完了 |
| Jolt Physics | Phase 36 | 完了 |
| Oblivion Script VM | Phase 37 | 完了 |
| Radiant AI | Phase 35 | 完了 |
| Imperial Weave v4.0 | Phase 54 | 完了 |
| Gamebryo完全互換 | Phase 56 | 完了 |
| リリースビルド | Phase 57 | 完了 |
| アセット最適化 | Phase 58-63 | 完了 |

---

## コード統計

### 現在のコードベース

| カテゴリ | 行数 |
|---------|------|
| C++ コード | 35,000+ |
| Java/Kotlin コード | 1,100+ |
| ヘッダーファイル | 12,000+ |
| **合計** | **48,000+** |

### ビルド統計

| 指標 | 値 |
|------|-----|
| コンパイル時間 | ~40秒（デバッグ、インクリメンタル） |
| APKサイズ | 8.4 MB（リリース） |
| 対応アーキテクチャ | arm64-v8a、armeabi-v7a、x86、x86_64 |

---

**最終更新**: 2026-09-04
