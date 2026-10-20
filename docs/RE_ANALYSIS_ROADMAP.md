# Oblivion RE 完全解析ロードマップ

**最終更新**: 2026-10-17
**目的**: Oblivion_Android 移植のためのリバースエンジニアリング (RE) を «全解析完了» まで継続するための恒久タスク一覧。
**関連**: [RE_EXE_REPORT.md](RE_EXE_REPORT.md) (主レポート) / `tmp/re/TODO_re49.md` (現行引継ぎ) / [../plan.md](../plan.md) (移植計画)

---

## 1. 完成の定義 (Definition of Done)

«全解析完了» は次の 3 層で定義する。本ロードマップの全フェーズはこの 3 層のいずれかに寄与する。

- **L1 移植パリティ**: 移植に必要な全機能について RE 仕様が存在し、実装・検証 (host test + 実機) までトレースできる。
- **L2 データ/アセット仕様化**: `Oblivion.esm` / ESP / DLC / BSA / NIF / KF / DDS / SDP / 音声 / 動画 / XML の全形式が文書化される。
- **L3 EXE 全域トリアージ**: `Oblivion.exe` 全関数 (22,295) が用途分類され、移植関連・主要エンジン関数は命名+文書化される。

---

## 2. 現状スナップショット (2026-10-17)

| 項目 | 現状 |
|---|---|
| RE_EXE_REPORT.md | §1〜§61 / 10194 行 / 631 FUN 参照を文書化 (約 2.8%) |
| 進行中 | Phase 2 Wave 6b 完了 (§61、2026-10-17)。Phase 2 全 7 項目が部分解析済み。残件 46 件 |
| 未着手 | Phase 1 以降 (§51〜§70 / Phase 3〜7)。B13 実機 .data ダンプのみユーザー実行待ち |
| Ghidra | `D:\Cargo\Ghidra\Oblivion.rep` (最終 2026-10-13)、カスタムスクリプト約 80 本 |
| 解析対象 EXE | `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416, ImageBase 0x400000) |
| シェーダ | 224 種中 176 種バイトコード一致、48 変種未確定 |
| ESM | 部分解析 (セル / ICON / AI パッケージ / ボイス等) |
| 補助バイナリ | OblivionLauncher.exe / binkw32.dll / atimgpud.dll 未解析 |

---

## 3. フェーズ一覧

| フェーズ | 名称 | 成果物 | 優先度 |
|---|---|---|---|
| Phase 0 | 進行案件の完了 (§49 残・便2) | §50 追記、TODO_re49 クローズ | P0 |
| Phase 1 | ゲームプレイ系の完全化 | §51〜§60 | P0 |
| Phase 2 | エンジン系の完全化 | §60〜§69 | P1 |
| Phase 3 | ESM / DLC データ全面解析 | `docs/RE_ESM_REPORT.md` | P1 |
| Phase 4 | アセット形式の全面解析 | `docs/RE_ASSET_REPORT.md` | P2 |
| Phase 5 | EXE 全域カバレッジ (トリアージ 100%) | 関数台帳 + KPI ダッシュボード | P2 |
| Phase 6 | 補助バイナリの一次解析 | §90 以降に追記 | P3 |
| Phase 7 | パリティ検証と保守 | トレーサビリティ表 + 運用 | P3 |

§ 番号は [RE_EXE_REPORT.md](RE_EXE_REPORT.md) の連番を継続する。新領域は専用レポートに分離し、§ はクロスリンクで参照する。

---

## 4. Phase 0: 進行案件の完了 (§49 残・便2 → §50) — **完了 2026-10-17**

出典: `tmp/re/TODO_re49.md`。状態記号: `[ ]` 未着手 / `[~]` 進行中 / `[x]` 完了。

### A. §49 完成スコープ

- [x] A5: 難易度式確定 (2026-10-16) — 実式 f=(d>0)?x+1:(d<0)?1/|x-1|:1、x=d*M (M 実効 5.0) (§50.1 / §51.7 / §51.13)。d=73→2.280 は移植自身のログ循環 (renderer.cpp:7822) と判明 (s50_a5.md)。移植修正案: settings_manager.cpp の pow(6,(d-50)/50.0) → V=(d_ui-50)/50.0; scale=V>0?1+5V:V<0?1/(1-5V):1
- [x] A6: 完了 (2026-10-17) — メニュー XML レイアウト確定 (§50.7 / §55.7、s50_menus_a6.md)

### B. 便2 (バッチ2、§50 以降) 13 件 — 全件完了 (2026-10-17)

- [x] B1: → §50.3 / §51.11 (s50_dmg.md)
- [x] B2: ContainerMenu 残 → §50.6 / §55.4 (cmd 51・窃盗 ExtraData 0x27・転送コア FUN_00492E70)
- [x] B3: FUN_0059bd80 → §55.4 (ControlsMenu vtable[11] キーリバインド入力キャプチャ [確定])
- [x] B4: FUN_005febf0 中間式 → §50.2 (s50_dmg.md)
- [x] B5: Fatigue (AV 0x0a) → §50.2
- [x] B6: 魔法経路 vtable +0x398 → §50.4 (0x5EE760)
- [x] B7: stackingtype 全種 → §50.4 (s50_magic_stack.md)
- [x] B8: ControlsMenu INSTRUCTIONS → §50.5 (s50_input.md)
- [x] B9: ワールド入力抑制フラグ → §50.5
- [x] B10: GMST 網羅一覧 → §50.8 / §55.6 (全 2304 件、s50_gmst_full.txt)
- [x] B11: ポーズ判定 byte 0xb333b8 → §50.5 / §54.2
- [x] B12: SleepWaitMenu 0x5d6a80 → §50.9 / §55.2 (月日数表 0xB06710 = {31,28,31,…} 閏なし 確定)
- [x] B13: .data ダンプ — 手順書 s50_b13_plan.md 完成。実行のみユーザー操作待ち (P0〜P3)

### C. 引継ぎ後追加調査候補

- [x] C1: → §55.1 (commit FUN_595a20 は奇数 index = スライダー5本)
- [x] C2: → §55.1 (arg2/arg3 は本体未参照 [確定])
- [x] C3: → §55.1 ([0x2F4] = フェード目標音量キャプチャ [確定])
- [x] C4: → §55.1 (FUN_006ae860 = AudioManager::Update / FUN_509610 = StreamMusic)
- [x] C5: → §55.3 / §56.3 ('SUDG' = Sun Damage MGEF [確定]。FUN_0066F420 = 睡眠/待機実行へ訂正 [確定-強])
- [x] C6: → §50.10 / §55.5 (video_display_menu.xml、SHA256 DE9BB53F…0CD5 一致)

**DoD**: §50 を RE_EXE_REPORT.md に追記し、本フェーズ全項目をクローズ。TODO_re49.md を完了状態に更新。 — **達成 (2026-10-17)**: §50 (2026-10-16 統合) + §55 補遺 (2026-10-17) を追記、TODO_re49.md クローズ済み (B13 はユーザー実行待ち)。

---

## 5. Phase 1: ゲームプレイ系の完全化 (§51〜§60)

移植挙動の正しさに直結する領域。B バッチ完了後の本命。

- [~] 1-1: 戦闘パイプライン完全化 — 命中判定 / ブロック / 弓 / 罠 / クリティカル、武器速度（FEBF0 テール・Fatigue・KD/KB は §56.1 で確定済み 2026-10-17）。Wave 5: dmgInt 非負 → KD ロールは damage 整数部 0 のみ約 1% で成立 (§59.9)、スニーク/弓/罠の実体を確定
- [~] 1-2: 魔法システム — Wave 1+2 完了 (2026-10-20, §66)。パイプライン全体 (Cast→delivery→dispatch→Reflect/Absorb→apply→ActiveEffect)、MGEF 32 コード全ハンドラ VA、コスト固定値 (スキル割引なし)、Poison on-strike、AI 呪文選択を確定。残: Alchemy/Enchanting crafting 関数、SEFF VM、MGEF+0x68 ESM 書込元 (§66.11)
- [~] 1-3: 疲労・スタミナ・死亡処理 — 死亡 vtable +0x2c8、ノックダウン、蘇生（Fatigue 式・自動 KD は §56.1 で確定）
- [~] 1-4: AI 詳細 — 種別名表 0xB12988 (33 件)、パッケージ選択 0x569020、検知 4 状態は確定。残 11 件 (§58.1)
- [~] 1-5: クエスト・対話 — 条件関数 169 件 (0xB0C8C0 関数帯 +0x20)、QUST/INFO/説得は確定。残 8 件 (§58.2)
- [~] 1-6: スクリプト VM — opcode 完全表、イベント送出全種 (※ 0x9832e6 は ___RTDynamicCast でありイベント送信ではない — C5/§55.3 訂正)。Wave 5: イベントビット表と送出 77 件、SCDA 2,393 全数パースを確定
- [~] 1-7: セーブ/ロード — manager 0xB33B00、save 0x465130 / load 0x465860、.ess 版数 0x7d、bSaveOn* は確定。残 8 件 (§58.3)
- [~] 1-8: キャラクタ生成・レベルアップ — スキル上昇 0x668070、ApplyLevelUp 0x66C3E0、属性ボーナス表は確定。残 8 件、birthsign 付与が最優先 (§58.4)。Wave 5: 星座ピッカーは menu 0x408 mode 3、AddSpell 0x514950 を確定 (残: 付与本体)
- [~] 1-9: 経済・所持品 — 商人、家、盗み判定（ContainerMenu 全経路・窃盗判定は §56.5 で確定）。Wave 5: 価格経路とラッパ (0x5E11A0/0x5E1200/0x5E1100) を確定、0x676EE0 = 登録処理に訂正
- [~] 1-10: 錬金・修理・ソウルトラップ等 — 各メニュー ID と修理量式、魂レベル表、SoulTrap を確定。残 9 件 (§58.5)

**DoD**: 各項目に実装対応先 (C++ ファイル) と検証方法を併記して §51〜§60 を完成。

---

## 6. Phase 2: エンジン系の完全化 (§61〜§70)

- [~] 2-1: レンダラ — D3D9 動的ロード、16 種 vtable 呼出、BSShader 系、shaderpackage 19 ファイルを特定 (§61.1)。残 6
- [~] 2-2: Havok 物理 — Havok 4.0 系、55+ bhk 型、bhkWorld queue 0x679340、impulse apply 0x8907A0。移植は基本 Jolt 実装済み、不足 3 点 (§61.2)。残 8
- [~] 2-3: アセットストリーミング — セル範囲 0x5AFE00 (9 段階)、テクスチャロード 0x760D80 (DDS+DXT+BSA)、NiNode Pool 3 サイズ、LoadingMenu 確認 (§61.3)。残 6
- [~] 2-4: オーディオエンジン — DirectShow (mp3) + DirectSound8 (wav/voice) の 2 系統、5 スライダー、DS3D rolloff 4.0 を確定。訂正 3 件 (§60.1)。残 7
- [~] 2-5: 入力系 — DirectInput8 単一 (XInput なし)、29 アクション + 特殊 3、既定 0x403960、照会 0x403520 (§60.2)。残 6
- [~] 2-6: CRT / 数値ヘルパ / 乱数 — 0x9828C0 = _ftol2 (切捨て)、rand = MSVC LCG 0x9859DD、丸め系 4 種 (§60.3)。残 6
- [~] 2-7: ファイル I/O・INI パス解決 — 読 0x4A8800 / 書 0x4A85F0、My Games dir 0xB3F280/0xB3F178、BSA 0x42EE80、Plugins.txt 行順 (§60.4)。残 7

**DoD**: 各項目を §61〜§70 に文書化、移植に影響する差異を一覧化。

---

## 7. Phase 3: ESM / DLC データ全面解析 → `docs/RE_ESM_REPORT.md`

比較資料: `tmp/re/wbDefinitionsTES4.pas` (xEdit 定義) を照合表として使用。

- [ ] 3-1: 全レコードタイプの網羅チェックリスト化 (TES4 全 ~50 種)
- [ ] 3-2: CELL / WRLD / REFR / ACHR の完全仕様 (座標、リンク、オーナー)
- [ ] 3-3: LAND / NAVM — 地形・AI 経路メッシュ
- [ ] 3-4: QUST / DIAL / INFO / SCEN — クエスト・対話グラフ
- [ ] 3-5: SCPT — スクリプトバイトコードのオペコード仕様
- [ ] 3-6: クリーチャー / レース / クラス / ファクション / 魔法 / 錬金
- [ ] 3-7: NPC_ / CREA FaceGen データ
- [ ] 3-8: GLOB / GMST / 天候 / 鳴き声 / 世界観データ
- [ ] 3-9: 圧縮レコード (zlib) とエッジケース
- [ ] 3-10: DLC 4 種 + Knights + Shivering Isles の追加レコード
- [ ] 3-11: SCDA オペコード表の確定 — `tmp/re/copilot_archive/scda_opcode_table_out.txt` (2,393 スクリプト / 141 opcode) を基底に、オペコード名・引数仕様を `script_disasm.cpp` と連動して文書化
- [ ] 3-12: 既知の異常の解消 — `DLCShiveringIsles.esp` parse 失敗 (`esp_inventory.txt`) / `Oblivion.esm` STAT size=0 @7,984,486 (`esm_inventory4.txt`)

**DoD**: 全レコードタイプに «解析済み / 移植で未使用と明記» のどちらかを付与。

---

## 8. Phase 4: アセット形式の全面解析 → `docs/RE_ASSET_REPORT.md`

- [ ] 4A-1: BSA 全形式 (v103/v104、圧縮あり/なし、フォルダハッシュ)
- [ ] 4A-2: NIF 全ブロックタイプ (現移植で未使用のブロック含む)
- [ ] 4A-3: KF / KFM アニメーション形式
- [ ] 4A-4: DDS 全形式 (DXT1/3/5、未圧縮、ミップ)
- [ ] 4A-5: SDP シェーダ — 48 変種の差分確定 → 224/224
- [ ] 4A-6: EFSH / パーティクル / マテリアル (BSShaderPPLighting 系)
- [ ] 4A-7: LIP / リップシンク (既存解析の検証)
- [ ] 4A-8: BIK 動画 (既存解析の検証 + 再生仕様)
- [ ] 4A-9: 音声形式 — MP3 / WAV / OGG / FUZ
- [ ] 4A-10: XML メニュー素材 — 全 59 メニューの XML をコーパス化
- [ ] 4A-11: NIF 仕様の文書化 — `tmp/re/copilot_archive/nif_oblivion_layout_spec.md` (17 実アセットでバイト検証済み) を基底に統合

**DoD**: 各形式に «解析済み / 移植で未使用と明記» を付与。コーパスは `tmp/re/` に保存。

---

## 9. Phase 5: EXE 全域カバレッジ (トリアージ 100%)

22,295 関数を «分類 (tier C) → 命名 (tier B) → 文書化 (tier A)» の 3 層で扱う。

- [ ] 5-1: 関数台帳の自動生成 — `CountFuncs.java` を拡張し CSV 化 (VA / サイズ / 呼出数 / 文字列 / vtable 所属)
- [ ] 5-2: 自動分類 — imports / xref / 文字列 / 呼出グラフからサブシステムタグを付与
- [ ] 5-3: KPI ダッシュボード — 命名率・文書化率を Ghidra スクリプトで自動集計
- [ ] 5-4: トリアージ 100% — 全関数に tier C 分類を付与
- [ ] 5-5: tier B 命名 — 自動候補 (FunctionID / RTTI) + 手動確認
- [ ] 5-6: tier A 文書化 — 移植関連 + 主要エンジンの完全文書化
- [ ] 5-7: 未解析領域ウェーブ — § 番号を継続して順次消化

**DoD**: トリアージ 100% を達成し、KPI ダッシュボードで常時可視化。

---

## 10. Phase 6: 補助バイナリの一次解析 (P3)

- [ ] 6-1: OblivionLauncher.exe — 起動フロー、INI 検証、GPU 検出
- [ ] 6-2: binkw32.dll — BIK 再生 API (動画移植の参照)
- [ ] 6-3: atimgpud.dll — 役割の一次確認のみ

---

## 11. Phase 7: パリティ検証と保守

- [ ] 7-1: RE → 実装トレーサビリティ表 (§ ⇔ C++ ファイル ⇔ テスト)
- [ ] 7-2: RE 由来のテストケースを host tests へ反映
- [ ] 7-3: 実機ビート (original vs port) の自動比較手順
- [ ] 7-4: レポート索引の自動生成 (§ 目次)
- [ ] 7-5: メモリ登録・圧縮サイクル (§ 完了ごとに session-memory 登録、節目で compact)
- [ ] 7-6: Ghidra プロジェクト / スクリプトの定期バックアップ

---

## 12. KPI 追跡表

§ 完了ごとに本表を更新する。

| 指標 | 現状 (2026-10-17) | 目標 |
|---|---|---|
| EXE 関数トリアージ率 | 未計測 | 100% (Phase 5) |
| 文書化関数 (tier A) | 631 (FUN 参照ユニーク) | 移植関連 + エンジン主要の全数 |
| RE_EXE_REPORT § 数 | 61 (§50〜§61 追加済み、§53 欠番) | 継続 (§100 以上) |
| ESM レコードタイプ | 部分 (数種) | 全 ~50 種 |
| シェーダ変種 | 176/224 | 224/224 |
| BSA / NIF / KF 形式 | 部分 | 全形式 |
| 補助バイナリ | 0 | 一次解析完了 |
| 移植トレーサビリティ | 暗黙 | 表で 100% |

---

## 13. 運用ルール

### 解析手法
- 呼出元走査は生バイト E8 走査 (`va+5+rel==target`)。capstone 逆アセンブル走査は命令境界ずれで穴があるため全数列挙に使わない
- VA 変換: `pe.get_offset_from_rva(VA - 0x400000)` / 逆は `pe.get_rva_from_offset(offset) + 0x400000`
- Ghidra headless + カスタムスクリプト (`D:\Cargo\Ghidra\scripts`) を使用。REA は毎回再解析が走る制約に注意
- 生データは `tmp/re/` に保存し、永続文書は `docs/` に集約する

### 文書
- 追記は `docs/RE_EXE_REPORT.md` の § 連番。編集は Python `io.open` (LF / BOM なし)
- 新領域は本ロードマップ記載の専用レポートへ分離し、§ からクロスリンク
- Copilot セッション由来の解析成果は `tmp/re/copilot_archive/` に保全済み (索引 `README.md`)。新たな解析資産は同 README に追記する

### リスク管理
- git commit はユーザーの明示承諾があるまで禁止。`tmp/` は保全 (削除しない)
- 逆コンパイル由来コードベース (OpenTES4 / xOBSE / COEF 等) との汚染回避を継続。BYO-data 方針を維持
- ベセスダ由来アセット / コードの再配布は行わない

### 進捗管理
- § 完了ごとに: RE_EXE_REPORT 追記 → session-memory 登録 → 本ロードマップ (KPI + チェックボックス) 更新
- 節目で compact-mini を実施

---

## 14. 更新履歴

- 2026-10-16: 初版作成 (Phase 0〜7、KPI 表、運用ルール)
- 2026-10-17: Copilot 解析資産を `tmp/re/copilot_archive/` へ保全 (109+α ファイル、索引 README.md)。Phase 3-11/3-12/4A-11 を追記、§50 解析 (A5/B1-B13/C1-C6) を子セッションで実施中
- 2026-10-17: **Phase 0 完了** — 便2 (A5/A6/B1〜B13/C1〜C6) 全解析、§50 (10-16 統合) + §55 補遺 (10-17) を RE_EXE_REPORT に追記。TODO_re49.md 全項目クローズ (B13 のみユーザー実行待ち)。KPI 更新 (§55 / 629 FUN / 9,527 行)
- 2026-10-17: **Wave 3 完了** — 最終残件 6 テーマ (戦闘 FEBF0 テール / 音響 / Sleep・Jail / menu 式 onlyif / Container 全経路 / GMST ローダ) を子セッション 6 本で解析し §56 へ統合 (9,696 行 / 630 FUN)。訂正: §51.4/§52.6 真偽値反転、FUN_0066F420=睡眠/待機実行、GMST は ESM ロードで適用 (旧「幽霊差分」撤回)。Phase 1 の一部に着手 (1-1/1-3/1-9/2-4 に部分注記)
- 2026-10-17: **Phase 1 Wave 4a (§57) 部分完了** — 子セッション 5 本 (戦闘 / 魔法 / 疲労・死亡 / スクリプト VM / 経済) を統合。1-1/1-2/1-3/1-6/1-9 を [~] に変更。魔法 (1-2) は主要経路が未解読。既存記述の訂正 (§56.3 テーブル基底 0xB0AF48、Actor vtable 0xA6E074、FUN_004F3870 誤記) は §57.6 参照。Wave 4b (§58) は未着手。
- 2026-10-17: **Phase 1 Wave 4a 第2次 (§57.9) 完了** — 残件の最優先項目を子セッション再開で消化。魔法の耐性/吸収/反射 3 関数と式、スクリプト VM の制御 opcode switch (0x517320/0x517300)、KB 完全式、蘇生/KD の実体、経済の extra node を確定。訂正: 0x5FE380=CHRM・0x5EE760=impact、0x1C は no-op (§57.9.1)。残件は戦闘 10 / 魔法 7 / 疲労 15 / SVM 9 / 経済 14。
- 2026-10-17: **Phase 1 Wave 4b (§58) 完了** — 子セッション 5 本 (AI / クエスト・対話 / セーブ・ロード / キャラ生成 / 錬金・修理等) を統合。条件関数 169 件 (関数帯 0xB0C8C0 + entry+0x20)、ApplyLevelUp 0x66C3E0、.ess 版数 0x7d、各クラフト系メニュー ID を確定。訂正: AlchemyMenu = 0x410 (0x3F1 = DialogMenu)。Phase 1 全 10 項目が [~]。残件計 ~99 件。
- 2026-10-17: **Phase 1 Wave 5 (§59) 完了** — 高優先残件を子セッション 5 本で消化。戦闘 (dmgInt 非負 → KD ロールは damage 整数部 0 のみ約 1% で成立 (§59.9)、スニーク/弓/罠の実体)、魔法 (MagicTarget vtable 0xA6DFF4)、SVM (ScriptEventList + イベントビット表 + SCDA 2,393 全数)、キャラ生成 (星座ピッカー = menu 0x408 mode 3)、経済 (価格経路とラッパ) を確定。訂正 6 件 (§59.6)。
- 2026-10-17: **§59.9 (KD 再検証)** — KD の「デッドコード」断定を精密化 (damage 整数部 0 のときのみ約 1% 成立)。**Phase 2 Wave 6a (§60) 完了** — オーディオ (DirectShow + DirectSound8)、入力 (DirectInput8 / 29 アクション)、CRT・乱数 (_ftol2 = trunc、MSVC LCG)、ファイル I/O (INI 615 キー、BSA、Plugins.txt) を子セッション 4 本で解析。訂正 7 件 (§60.5)。
- 2026-10-17: **Phase 2 Wave 6b (§61) 完了** — レンダラ (D3D9 vtable / BSShader / 19 shaderpackage)、Havok 物理 (bhk 55+ 型 / FSM 6 状態 / 移植マッピング)、アセットストリーミング (セル範囲 9 段階 / テクスチャロード / NiNode Pool) を子セッション 3 本で解析。Phase 2 全 7 項目 [~]。
- 2026-10-20: **Phase 1 魔法 Wave 1+2 完了 (§66)** — 魔法パイプライン全体を逆アセンブリで深掘り。M1-M8 (Wave 1, 2026-10-19): パイプライン骨格 (Cast→delivery→dispatch→Reflect/Absorb→apply)、耐性/吸収/反射の式、コスト固定値 (スキル割引なし)、ActiveEffect 更新、エンチャント減算、飛翔体。M9-M15 (Wave 2, 2026-10-20): MGEF 32 コード全ハンドラ VA、適用本体 0x6A27F0、配送トリガ、Poison on-strike、AI 呪文選択、残小物。証跡: s90〜s98_magic_m*.md (9ファイル)。残: Alchemy/Enchanting crafting、SEFF VM、MGEF+0x68 ESM 書込元。
