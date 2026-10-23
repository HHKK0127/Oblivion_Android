# GTA V ソースコード調査レポート — Oblivion Android 移植可能性分析

## 調査対象
`D:\Cargo\GTAVSP\GTAV Source\src\dev_ng\`

## 1. オーディオシステム (game/audio/)

### 機能概要
- **North Audio Engine**: 高レベルオーディオエンジン。3D音、ラジオ、会話、環境音を統括
- **SuperConductor**: 全オーディオコンダクターを統括するシングルトン。QuietScene（静寂シーン）管理、プレイヤー状態（水中/ステルス/死亡/乗物）に応じた動的ミックス制御
- **GunFightConductor**: FSM（有限状態機械）ベースの戦闘音制御。64体までの戦闘PEDをトラッキング、fitness値で重要ターゲットを選別
- **VehicleConductor**: 車両音の動的制御
- **DynamicMixer**: シーン/パッチベースの動的ミキシング。MixerScene + MixerPatch の組み合わせでカテゴリ階層に適用
- **RadioEmitter**: ラジオエミッターの抽象化（Entity/Static/Ambientの3種）
- **SpeechManager**: 会話管理
- **audCurve**: カーブベースのパラメータ制御（距離→音量など）

### 移植可能な設計パターン
1. **Conductor パターン**: FSM + イベント駆動の音響制御。Oblivion の AudioManager に EventBus 経由で統合可能
2. **DynamicMixer の Scene/Patch モデル**: 状況に応じたミックスプリセット（戦闘中/探索中/水中）を動的に切り替え
3. **audCurve**: 距離減衰やパラメータ補間に汎用的に使えるカーブ評価器
4. **QuietScene**: 環境音のフェード管理。Oblivion のダンジョン/野外の音響切り替えに応用可能

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| northaudioengine.h | オーディオエンジン全体のインターフェース | 難しい |
| superconductor.h | コンダクター統括 + QuietScene 管理 | 中程度 |
| gunfightconductor.h | FSM + fitness-based ターゲット選定 | 中程度 |
| dynamicmixer.h | シーンベース動的ミキシング | 中程度 |
| radioemitter.h | エミッター抽象化（ポリモーフィック） | 簡単 |
| speechmanager.h | 会話キューイング | 中程度 |

### 推奨アクション
- **優先度高**: audCurve クラスを移植（汎用性が高い）
- **優先度中**: DynamicMixer の Scene/Patch モデルを参考にした状況適応型ミキシング
- **優先度低**: Conductor FSM パターン（Oblivion の規模にはオーバースペックの可能性）

---

## 2. レンダリングパイプライン (game/renderer/)

### 機能概要
- **CRenderer**: レンダリング全体を統括する静的クラス。バケットベースのレンダリング（Opaque/Alpha/Decal/Cutout/Water）
- **Water**: 高度な水シミュレーション（波、屈折、反射、フォグ、コースティクス、泡）
- **PlantsGrassRenderer**: 植物/草のレンダリング（SPU対応）
- **PostProcessFX**: トーンマッピング、被写界深度、ブルーム、HDR
- **SSAO**: スクリーンスペースアンビエントオクルージョン
- **RenderThread**: マルチスレッドレンダリング（メインスレッドとレンダースレッドの分離）
- **RenderListBuilder**: レンダーリストの構築（可視判定含む）
- **Occlusion**: オクルージョンカリング

### 移植可能な設計パターン
1. **Render Bucket システム**: 描画順序のバケット分類（Opaque→Alpha→Decal→Water）
2. **Water シミュレーション**: 波の合成（WaveQuad）、屈折/反射パス、フォグ事前パス
3. **PostProcessFX パイプライン**: トーンマッピング、ブルーム、DOF
4. **RenderThread 分離**: メインスレッドとレンダースレッドの分離パターン

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| Renderer.h | レンダリング全体の設計（バケット、可視性） | 中程度 |
| Water.h | 水シミュレーション全般 | 難しい |
| PostProcessFX.h | ポストエフェクトパイプライン | 難しい |
| RenderThread.h | レンダースレッド分離 | 中程度 |
| SSAO.h | アンビエントオクルージョン | 難しい |
| PlantsGrassRenderer.h | 植物レンダリング | 難しい |

### 推奨アクション
- **優先度高**: Render Bucket システム（Oblivion のレンダリング順序改善に直接有用）
- **優先度中**: Water シミュレーションの基本波合成アルゴリズム
- **優先度低**: PostProcessFX（OpenGL ES 3.0 の制約あり）

---

## 3. AI システム (game/ai/)

### 機能概要
- **AITarget**: AIターゲット管理
- **AnimBlackboard**: アニメーションブラックボード（状態共有）
- **EntityScanner**: エンティティスキャン（周囲のPED/乗物検出）
- **SpatialArray**: 空間分割配列（高速近傍探索）
- **ExpensiveProcess**: 高コスト処理の分散実行

### 移植可能な設計パターン
1. **AnimBlackboard**: ブラックボードパターンによるAI状態共有
2. **SpatialArray**: 空間ハッシュ/グリッドによる近傍探索
3. **EntityScanner**: 定期的な周囲スキャン（Oblivion の感知システムに応用）

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| AnimBlackboard.h | ブラックボードパターン | 簡単 |
| spatialarray.h | 空間分割配列 | 中程度 |
| EntityScanner.h | エンティティスキャン | 簡単 |

### 推奨アクション
- **優先度高**: AnimBlackboard パターン（NPC 状態管理に直接応用可能）
- **優先度中**: SpatialArray（近傍探索の最適化）

---

## 4. NPC 管理 (game/Peds/)

### 機能概要
- **CPed**: ペッドの基本クラス（162KBの大規模ヘッダ）
- **PedIntelligence**: ペッドの知能/行動管理（FSM + タスクシステム）
- **CPopulation**: ポピュレーション管理（生成/削除/密度制御）
- **PedFactory**: ペッド生成ファクトリー
- **Relationships**: 関係性マトリックス（CPed 間の友好/敵対）
- **PedType**: ペッドタイプ定義（市民/警察/ギャング等）
- **PlayerPed**: プレイヤーペッド特殊化
- **TaskData**: タスクデータ構造

### 移植可能な設計パターン
1. **Population 管理**: 距離ベースの生成/削除（ADD_MIN_RADIUS/CULL_RANGE）
2. **PedIntelligence + Task システム**: タスクキューによる行動管理
3. **Relationships マトリックス**: タイプベースの関係性定義
4. **PedFactory**: ファクトリーパターンによる生成

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| PedIntelligence.h | NPC 知能システム全体 | 難しい |
| Population.h | ポピュレーション管理（距離ベース制御） | 中程度 |
| pedpopulation.h | 詳細ポピュレーション | 中程度 |
| Relationships.h | 関係性マトリックス | 簡単 |
| PedFactory.h | ファクトリーパターン | 簡単 |
| PedType.h | タイプ階層 | 簡単 |

### 推奨アクション
- **優先度高**: Population 管理の距離ベース生成/削除ロジック
- **優先度高**: Relationships マトリックス（Oblivion の派閥システムに応用）
- **優先度中**: Task システムの設計思想

---

## 5. 乗り物システム (game/Vehicles/)

### 機能概要
- **CVehicle**: 乗り物基本クラス（149KB）
- **Automobile/Bike/Boat/Heli/Planes/Submarine**: 各乗り物タイプ
- **CTransmission**: トランスミッション（ギア比、エンジンダメージ、ターボ、ニトロ、KERS）
- **CHandlingData**: ハンドリングデータ（handlingMgr.h）
- **CVehiclePopulation**: 乗り物ポピュレーション
- **wheel.h**: ホイール物理
- **vehicleDamage.h**: 車両ダメージモデル

### 移植可能な設計パターン
1. **CTransmission**: エンジン/トランスミッションシミュレーション（Oblivion には不要だが設計は参考に）
2. **HandlingData**: データ駆動のハンドリング定義
3. **VehiclePopulation**: 乗り物の自動生成/管理

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| Transmission.h | エンジン/ギアシミュレーション | 中程度 |
| handlingMgr.h | データ駆動ハンドリング | 中程度 |
| vehiclepopulation.h | 乗り物ポピュレーション | 中程度 |
| wheel.h | ホイール物理 | 難しい |

### 推奨アクション
- **優先度低**: Oblivion には乗り物がないため優先度低。ただし馬（Horse）システムが追加される場合は参考に

---

## 6. 物理エンジン (game/physics/)

### 機能概要
- **gtaArchetype**: 衝突タイプ/インクルードフラグのビットマスク定義（32bit）
- **gtaMaterial**: マテリアル定義
- **Breakable**: 破壊可能オブジェクト
- **Deformable**: 変形可能オブジェクト
- **RagdollConstraints**: ラグドール制約
- **Rope**: ロープ物理
- **Floater**: 浮遊物理

### 移植可能な設計パターン
1. **ArchetypeFlags**: ビットマスクベースの衝突フィルタリング（Oblivion の衝突判定に応用）
2. **Material システム**: サーフェスプロパティのデータ駆動定義

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| gtaArchetype.h | 衝突タイプフラグ設計 | 簡単 |
| gtaMaterial.h | マテリアルシステム | 中程度 |
| physics.h | 物理エンジン全体 | 難しい |

### 推奨アクション
- **優先度高**: ArchetypeFlags のビットマスク衝突フィルタリング設計
- **優先度中**: Material システム

---

## 7. ストリーミング/アセット管理 (game/streaming/)

### 機能概要
- **CStreaming**: ストリーミングエンジン（静的クラス）
- **RequestObject/LoadObject/RemoveObject**: アセットの要求/ロード/削除
- **優先度付きリクエスト**: ミッション優先、シーン優先
- **BoxStreamer**: ボックスベースのストリーミング
- **PrioritizedClipSetStreamer**: 優先度付きクリップセット
- **ZonedAssetManager**: ゾーン管理

### 移植可能な設計パターン
1. **Request/Release モデル**: アセットの参照カウント管理
2. **優先度付きロード**: ミッションクリティカルなアセットを優先
3. **ゾーンベース管理**: エリア単位のアセット管理

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| streaming.h | ストリーミングエンジン全体 | 中程度 |
| zonedassetmanager.h | ゾーン管理 | 中程度 |
| BoxStreamer.h | ボックスストリーミング | 中程度 |

### 推奨アクション
- **優先度高**: Request/Release モデル（Oblivion のセルストリーミングに応用）
- **優先度中**: 優先度付きロード

---

## 8. セーブ/ロード (game/SaveLoad/)

### 機能概要
- **CGenericGameStorage**: セーブ/ロード管理（シングルトン的静的クラス）
- **キュー操作**: 非同期セーブ/ロードキュー（CSavegameQueuedOperation）
- **バッファ管理**: SP/MP バッファ分離
- **写真ギャラリー**: スクリーンショット管理
- **クラウド対応**: セーブデータのクラウドアップロード

### 移植可能な設計パターン
1. **キュー操作パターン**: 非同期セーブ/ロードのキューイング
2. **バッファ分離**: セーブデータのブロック単位管理

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| GenericGameStorage.h | セーブ/ロード管理全体 | 中程度 |
| savegame_queue.h | キュー操作 | 中程度 |
| savegame_data.h | データ管理 | 簡単 |

### 推奨アクション
- **優先度中**: キュー操作パターン（Oblivion の SaveManager 改善に応用）

---

## 9. UI/HUD/メニュー (game/frontend/)

### 機能概要
- **NewHud**: HUD システム
- **CMapMenu**: ポーズマップ（Scaleform + ブリップ管理）
- **CMenuBase**: メニューベース
- **MiniMap**: ミニマップ
- **HudMarkerManager**: HUD マーカー管理
- **UIReticule**: 照準器
- **CWeaponWheel**: ウェポンホイール
- **MobilePhone**: 携帯電話UI
- **PauseMenu**: ポーズメニュー

### 移植可能な設計パターン
1. **HudMarkerManager**: マーカーの管理/更新パターン
2. **MiniMap**: ミニマップレンダリング
3. **Menu 階層**: メニュースクリーンのスタック管理

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| NewHud.h | HUD システム | 中程度 |
| CMapMenu.h | ポーズマップ（ブリップ管理） | 中程度 |
| MiniMap.h | ミニマップ | 中程度 |
| HudMarkerManager.h | マーカー管理 | 簡単 |

### 推奨アクション
- **優先度中**: HudMarkerManager のマーカー管理パターン
- **優先度低**: Scaleform 依存部分は移植不可

---

## 10. 武器/戦闘 (game/weapons/)

### 機能概要
- **CWeaponManager**: 武器管理（シングルトン的静的クラス）
- **CWeapon**: 武器基本クラス
- **WeaponTypes**: 武器タイプ定義（ハッシュベース）
- **CExplosion**: 爆発管理
- **WeaponDamage**: ダメージ計算
- **FiringPattern**: 発射パターン定義
- **WeaponFactory**: 武器生成ファクトリー

### 移植可能な設計パターン
1. **WeaponTypes のハッシュベース定義**: 拡張可能な武器タイプ管理
2. **WeaponFactory**: ファクトリーパターン
3. **FiringPattern**: データ駆動の発射パターン

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| WeaponManager.h | 武器管理 | 簡単 |
| WeaponTypes.h | ハッシュベース武器定義 | 簡単 |
| WeaponFactory.h | ファクトリーパターン | 簡単 |
| Explosion.h | 爆発管理 | 中程度 |

### 推奨アクション
- **優先度高**: WeaponTypes のハッシュベース定義（Oblivion の武器/魔法タイプ管理に応用）
- **優先度高**: WeaponFactory パターン

---

## 11. シーン管理/エンティティ (game/scene/)

### 機能概要
- **CScene**: シーン管理（レベルロード、更新、レンダリングセットアップ）
- **CEntity**: エンティティ基本クラス
- **CPhysical**: 物理エンティティ
- **CBuilding/CDynamicEntity**: エンティティ派生
- **CEntityIterator**: エンティティ反復子
- **DataFileMgr**: データファイル管理
- **ExtraContent**: DLC コンテンツ管理

### 移植可能な設計パターン
1. **Entity 階層**: エンティティの継承構造（Entity→Physical→Building/DynamicEntity）
2. **EntityIterator**: 安全なエンティティ反復
3. **DataFileMgr**: データファイルのロード/管理

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| Entity.h | エンティティ基本クラス | 中程度 |
| Physical.h | 物理エンティティ | 中程度 |
| scene.h | シーン管理 | 中程度 |
| EntityIterator.h | エンティティ反復子 | 簡単 |
| DataFileMgr.h | データファイル管理 | 中程度 |

### 推奨アクション
- **優先度高**: EntityIterator パターン（Oblivion のエンティティ走査に応用）
- **優先度中**: Entity 階層設計

---

## 12. スクリプトシステム (game/script/)

### 機能概要
- **gta_thread**: スクリプトスレッド管理
- **commands_*.h**: カテゴリ別コマンド定義（ped/vehicle/weapon/audio/camera 等）
- **script.h**: スクリプトエンジン全体
- **ScriptMetadataManager**: スクリプトメタデータ管理
- **streamedscripts**: ストリームドスクリプト

### 移植可能な設計パターン
1. **コマンド登録パターン**: カテゴリ別コマンド定義
2. **スクリプトスレッド**: スクリプト実行管理

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| script.h | スクリプトエンジン | 難しい |
| gta_thread.h | スクリプトスレッド | 中程度 |

### 推奨アクション
- **優先度低**: Oblivion にスクリプトエンジンは不要だが、コマンド登録パターンは参考に

---

## 13. 入力/コントロール (game/control/)

### 機能概要
- **CGameLogic**: ゲーム状態管理（プレイ中/死亡/逮捕/リスポーン）
- **gps.h**: GPS 経路探索
- **garages.h**: ガレージ管理
- **trafficlights.h**: 信号機管理
- **stuntjump.h**: スタントジャンプ
- **WaypointRecording**: ウェイポイント記録

### 移植可能な設計パターン
1. **CGameLogic の状態機械**: ゲーム状態遷移（Playing→Death→Respawn）
2. **GPS 経路探索**: ウェイポイントベースの経路案内

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| gamelogic.h | ゲーム状態管理 | 簡単 |
| gps.h | GPS 経路探索 | 中程度 |

### 推奨アクション
- **優先度高**: CGameLogic の状態機械パターン（Oblivion のゲーム状態管理に応用）

---

## 14. コアフレームワーク (rage/framework/)

### 機能概要
- **fwScene**: シーンインターフェース
- **fwFsm**: FSM 基底クラス（GunFightConductor 等で使用）
- **fwEntity**: エンティティフレームワーク
- **fwPtrList**: ポインターリスト
- **fwDrawList**: 描画リスト

### 移植可能な設計パターン
1. **fwFsm**: 汎用 FSM フレームワーク
2. **fwPtrList**: 安全なポインターリスト

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| fsm.h (fwsys) | 汎用 FSM | 簡単 |
| scene.h (fwscene) | シーンインターフェース | 中程度 |

### 推奨アクション
- **優先度高**: fwFsm の汎用 FSM（Oblivion の NPC 状態機械に応用）

---

## 15. 経路探索 (game/pathserver/)

### 機能概要
- **PathServer**: 経路探索サーバー
- **NavMeshGta**: ナビメッシュ
- **PathServer_Ped**: PED 用経路探索
- **PathServer_Store**: 経路データストア
- **PathServerThread**: 別スレッドでの経路探索

### 移植可能な設計パターン
1. **別スレッド経路探索**: メインスレッドをブロックしない経路探索
2. **ナビメッシュ**: ポリゴンベースの経路探索

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| PathServer.h | 経路探索サーバー | 難しい |
| NavMeshGta.h | ナビメッシュ | 難しい |
| PathServer_Ped.h | PED 経路探索 | 中程度 |

### 推奨アクション
- **優先度中**: 別スレッド経路探索の設計（Oblivion のパスファインディング改善に応用）

---

## 16. 視覚エフェクト (game/vfx/)

### 機能概要
- **CVisualEffects**: 視覚エフェクト管理（コロナ、レンズフレア、マーカー、スカイ）
- **VfxHelper**: VFX ヘルパー
- **VfxSettings**: VFX 設定

### 移植可能な設計パターン
1. **レンダリングモード分離**: 通常/ミラー/水/キューブ反射/シースルー
2. **マーカーマスク**: ビットマスクによるマーカー種別制御

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| VisualEffects.h | VFX 管理全体 | 中程度 |
| VfxSettings.h | VFX 設定 | 簡単 |

### 推奨アクション
- **優先度中**: マーカーマスクパターン（HUD マーカー管理に応用）

---

## 17. デバッグシステム (game/debug/)

### 機能概要
- **OverviewScreen**: FPS / パーセンタイル / ヒストグラム / 律速 (BOUND) 分析を表示する開発用オーバーレイ
- **DebugScene / Debug**: `__BANK` ビルド限定の銀行 (ランタイム編集可能なウィジェット群) とシーン情報の可視化
- **DebugDraw**: 3D / 2D の即時描画ヘルパー。実体は `rage/grcore/debugdraw.h`。全関数が `framesToLive` を受け取り表示フレーム数を制御する
- **DebugRecorder / FrameDump / TiledScreenCapture / SceneGeometryCapture**: フレーム単位の記録とキャプチャ
- **UiGadget/**: デバッグ UI 用のウィジェットツリー (Button / Text / List / Window / Box / FullscreenMenu)
- **GtaPicker / NodeViewer / VectorMap / BlockView / LightProbe / BudgetDisplay**: 個別インスペクタと予算表示
- **BoundsDebug / AmbientMarkers / DebugWanted / DebugVehicleInteraction**: ゲーム機能別の可視化

### 移植可能な設計パターン
1. **framesToLive セマンティクス**: 「何フレーム表示し続けるか」を描画関数の引数に持たせ、呼び出し側から寿命管理を排除する
2. **パーセンタイルベースのフレームタイム分析**: 平均値では隠れるスパイクを 50/66/75/80/90/95/98/99% で捉える
3. **律速 (BOUND) 分類**: フレームが何に律速されたかを列挙し、原因の切り分けを可能にする
4. **UiGadget のウィジェットツリー**: 親子階層 + アライメント定数 + イベントマスクによる UI 構造の宣言化
5. **コールバックによる遅延セル生成**: リストの可視行のみ内容を生成し、項目数に関わらずコストを一定に保つ

### 注目ファイル
| ファイル | 説明 | 移植難易度 |
|----------|------|-----------|
| OverviewScreen.cpp / .h | パーセンタイル FPS と律速分析 (99KB) | 中程度 |
| DebugDraw.h + rage/grcore/debugdraw.h | 3D / 2D 即時描画 API | 簡単 |
| UiGadget/UiGadgetBase.h | ウィジェットツリー・イベント伝搬・アライメント | 中程度 |
| UiGadget/UiGadgetList.h | スクロールリスト + 遅延セル生成 | 中程度 |
| UiGadget/UiEvent.h | イベントマスクとクリック判定 | 簡単 |
| DebugRecorder.h | フレーム記録と再生 | 難しい |

### 推奨アクション
- **優先度: 最高**。うち 2 項目 (3D 可視デバッグ / パーセンタイル分析) は実装済み (§17.2)
- 律速分類は Android で GPU 時間の取得が難しいため、当面は Update / Render の 2 分類に留める

### 17.1 Oblivion Android との比較

**3D デバッグ描画**

| 観点 | GTA V (grcDebugDraw) | Oblivion Android (移植前) | Oblivion Android (現在) |
|------|----------------------|---------------------------|-------------------------|
| 3D 描画 | line / quad / poly / cross / circle / arc / sphere / ellipsoid / partial sphere / AABB / OBB / cube / cylinder | なし (JoltPhysics の DebugRenderer は未使用) | line / lineGradient / cross / boxAABB / boxOBB / sphere / circle / cylinder / cone / grid / arrow |
| 2D 描画 | line / quad / poly / cross / circle / arc / rect / text / meter | テキストのみ | line2D / rect2D |
| 寿命管理 | 全関数が `framesToLive` を取る | なし | `framesToLive` を採用 (負=永続 / 0=そのフレームのみ / 正=n フレーム) |
| GL 状態 | エンジン側で管理 | - | 描画前に保存し描画後に復元 |

**パフォーマンス分析**

| 機能 | GTA V (OverviewScreen) | Oblivion Android (移植前) | Oblivion Android (現在) |
|------|------------------------|---------------------------|-------------------------|
| FPS | 数値 + 色分け | 数値 + 色分け | 数値 + 色分け |
| パーセンタイル | 50/66/75/80/90/95/98/99% の 8 段階 | なし (平均のみ) | 同 8 段階を実装 |
| サンプル窓 | 100 フレーム | 60 フレーム (テキストグラフ) | 100 フレーム + 60 フレームのテキストグラフを併用 |
| サンプル縮約 | 10 個で `HalveSamples()` (偶数番目のみ残す = 最近値重視) | なし | 同一方式 |
| 律速分類 | Update / Render / GPU / GPU Flip / GPU DrawableSpu / GPU Bubble の 6 種 | CPU / GPU / Wait の 3 種 (常に 0) | 同じ (接続は未実施) |

### 17.2 移植実施済み (2026-10-23)

- **3D 可視デバッグ (DebugDraw)**: `app/src/main/cpp/ui/debug_draw.h` / `debug_draw.cpp` を新規追加。`framesToLive` と GL 状態の保存/復元を実装
- **実利用箇所**: `app/src/main/cpp/ui/npc_debug_visualizer.h` / `.cpp` に `setDebugDraw()` / `setShowWorldMarkers()` / `renderWorldMarkers()` を追加。NPC の体の箱 (boxAABB)、プレイヤー頭部から NPC 頭部への視線 (line)、足元マーカー (cross) を距離で色分けして描画。`engine/renderer.cpp` で DebugDraw を注入
- **パーセンタイル分析**: `app/src/main/cpp/ui/debug_hud.h` / `.cpp` に `PercentileSample`、`timeStepHistory[100]`、`percentileSamples[8]`、`updatePercentiles()`、`renderPercentileSection()` を追加。Performance ページに `TIME STEP PERCENTILES` を表示
- **副次修正**: DebugHUD の FPS が `frameCount` (RESET_INTERVAL = 5 秒でしかリセットされない) ÷ `timeSinceLastUpdate` (0.5 秒) で計算されており、約 10 倍の過大表示になっていた。窓ごとのカウンタ `framesInWindow` を導入して修正
- **未実装**: 律速 (BOUND) ヒストグラムは GPU 時間取得の制約から見送り。`setFrameTimeBreakdown()` は宣言・定義のみで renderer から呼ばれておらず、CPU / GPU / Wait 内訳は常に 0

### 17.3 デバッグ UI (UiGadget) 設計メモ

Oblivion の `DebugMenu` (`app/src/main/cpp/ui/debug_menu.h` / `.cpp`) は、タブごとに `Button` / `Slider` のフラットな配列を持ち、`calculateButtonPositions()` で座標を手計算し、`hitTestTab()` / `hitTestContent()` / `hitTestSlider()` で当たり判定を行う構成になっている。タブ数は 31 で、タブバーはドラッグスクロール、タップとスクロールの判別は `TouchState` の閾値で行う。

UiGadget から採用候補となる設計パターン:

| UiGadget の仕組み | 内容 | Oblivion への適用余地 |
|-------------------|------|----------------------|
| ウィジェットツリー (`AttachChild` / `DetachFromParent`) | 親子階層で座標とイベントを伝搬 | `calculateButtonPositions()` の手計算を置き換え |
| アライメント定数 (9 方向) | 親の矩形に対する相対配置 | セーフエリア対応 (`safeLeft` / `safeTop` など) の自動化 |
| イベントマスク (`m_handledEventsMask`) | ウィジェットごとに処理するイベント種別を制限 | タブバーと本文の入力振り分けを宣言的に表現 |
| クリック判定 (`UIEVENT_CLICK_MAX_DURATION`) | 押下から N フレーム以内の解放をシングルクリックとみなす | `TouchState` の閾値ヒューリスティックを置き換え |
| `CUiGadgetList` | ビューポート + スクロールバー + 遅延セル生成 | LOGS タブなど長い一覧の描画コストを一定化 |
| `CUiGadgetWindow` | ドラッグ可能なフローティングウィンドウ | NPC 情報などの常時表示パネル |
| `CUiColorScheme` | 9 種の色を一元管理 | テーマ切り替え |

**結論**: 現状の `DebugMenu` はタッチ操作に最適化されており、機能的な不足はない。UiGadget の知見は**設計パターンとしての参照に留め、実リファクタリングはゲームが形になってから判断する**。RAGE 固有の依存 (`grcFont` / `datCallback` / `fwBox2D` / `atArray`) はそのままでは流用できないため、移植する場合も構造のみを取り込む。

---

## 総合推奨アクション一覧（優先度順）

### 最優先で移植すべき設計パターン
1. **fwFsm（汎用FSM）** — NPC 状態機械、戦闘状態管理に直接応用可能
2. **AnimBlackboard（ブラックボード）** — NPC 間の状態共有
3. **Population 距離ベース管理** — セルベースの NPC 生成/削除
4. **Relationships マトリックス** — 派閥/友好度システム
5. **ArchetypeFlags 衝突フィルタリング** — ビットマスク衝突判定
6. **EntityIterator** — 安全なエンティティ走査
7. **CGameLogic 状態機械** — ゲーム状態遷移
8. **WeaponTypes ハッシュベース定義** — 拡張可能なタイプ管理
9. **audCurve** — 汎用カーブ評価器
10. **Request/Release ストリーミングモデル** — アセット管理

### 中優先度
11. **DynamicMixer Scene/Patch** — 状況適応型オーディオミキシング
12. **Render Bucket システム** — 描画順序管理
13. **Water 波合成** — 基本アルゴリズム
14. **SpatialArray** — 空間分割近傍探索
15. **Task システム設計** — NPC タスク管理
16. **別スレッド経路探索** — パフォーマンス改善
17. **HudMarkerManager** — マーカー管理
18. **セーブ/ロードキュー** — 非同期保存

### 低優先度（Oblivion のスコープ外だが参考）
19. **Conductor FSM** — 大規模オーディオ制御
20. **PostProcessFX** — OpenGL ES 3.0 制約あり
21. **乗り物システム全般** — 馬システム追加時に再評価
22. **スクリプトエンジン** — 現在のスコープ外

### 実装済み（第 17 章 デバッグシステム）
- **3D 可視デバッグ (DebugDraw)** — 実装・ビルド確認済み。詳細は §17.2
- **パーセンタイルベースのパフォーマンス分析** — 実装・ビルド確認済み。詳細は §17.2