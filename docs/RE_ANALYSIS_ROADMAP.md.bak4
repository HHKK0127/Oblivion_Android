# Oblivion RE 完全解析ロードマップ

**最終更新**: 2026-10-25
**目的**: Oblivion_Android 移植のためのリバースエンジニアリング (RE) を «全解析完了» まで継続するための恒久タスク一覧。
**関連**: [RE_EXE_REPORT.md](RE_EXE_REPORT.md) (主レポート) / `tmp/re/TODO_re49.md` (現行引継ぎ) / [../plan.md](../plan.md) (移植計画)

---

## 1. 完成の定義 (Definition of Done)

«全解析完了» は次の 3 層で定義する。本ロードマップの全フェーズはこの 3 層のいずれかに寄与する。

- **L1 移植パリティ**: 移植に必要な全機能について RE 仕様が存在し、実装・検証 (host test + 実機) までトレースできる。
- **L2 データ/アセット仕様化**: `Oblivion.esm` / ESP / DLC / BSA / NIF / KF / DDS / SDP / 音声 / 動画 / XML の全形式が文書化される。
- **L3 EXE 全域トリアージ**: `Oblivion.exe` 全関数 (22,295) が用途分類され、移植関連・主要エンジン関数は命名+文書化される。

---

## 2. 現状スナップショット (2026-10-25)

| 項目 | 現状 |
|---|---|
| RE_EXE_REPORT.md | §1〜§70 / 12,136 行 / 683 FUN 参照を文書化 (約 3.1%) |
| 直近の追記 | §70.8 のオペコード表フィールドオフセット**確定** (「要再導出」解消)、§70.12 へ V79 吸収仮説の棄却と V80 裏取り、§70.13 へ正典表 `docs/SCRIPT_OPCODE_TABLE.md`、§70.14 へ移植ヘッダ `script_opcodes.h` との突合と ABSENT 39 件の帰属**確定** (2026-10-25)、正典表 `docs/SCRIPT_OPCODE_TABLE.md` へブロック型 ID 注記節を追加、§70.4 へ (V83) 節 (表の列の実体 = `+0x10` パック構造と `+0x14` 引数記述子)、続けて (V84) 節 (引数コンパイラ `0x4FDE30` の型分岐 = 38 型 → 2 ファミリ → 9 経路) を追加、`docs/SCRIPT_OPCODE_TABLE.md` へ型コード別のコンパイル挙動表を追加 (2026-10-25)、`docs/RE_EXE_REPORT.md` §70.15 へ opencode V80 Part B（高参照グローバル 15 件）を .text imm32 再現検証付きで収録 (2026-10-25)、続けて **§70.16** を新設し **FUN ラベルなし関数の全数列挙**（Ghidra 台帳 21,597 件との差分方式、UNLABELED 10,175 / E8 呼出し先の高信頼 **214 件** / 再現率 76.9%）を収録 (2026-10-25) |
| 進行中 | Phase 2 Wave 6b 完了 (§61、2026-10-17)。Phase 2 全 7 項目が部分解析済み。残件 46 件 |
| 未着手 | Phase 1 以降 (§51〜§70 / Phase 3〜7)。B13 実機 .data ダンプのみユーザー実行待ち |
| Ghidra | 旧 `D:\Cargo\Ghidra\Oblivion.rep` (最終 2026-10-13) に加え、**12.1.4 PUBLIC** を導入 (`C:\Users\hiroki.kogarumai\.copilot\chats\2026-10-09\improved-waffle-2dbf7451\ghidra_12.1.4_PUBLIC`、2026-10-24 `analyzeHeadless` 完走実績あり)。新プロジェクト `D:\Cargo\Ghidra\temp\ghidra_projects\OblivionRe` |
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
- [~] 1-2: 魔法システム — Wave 1+2 完了 (2026-10-20, §66)。パイプライン全体 (Cast→delivery→dispatch→Reflect/Absorb→apply→ActiveEffect)、~~MGEF 32 コード全ハンドラ VA~~（**訂正 2026-10-23: コード数は 150種=ESM EFID distinct / MGEF EDID 145件。登録 ctor は23件のみ** → TODO V26/V35）、コスト固定値 (スキル割引なし)、Poison on-strike、AI 呪文選択を確定。残: Alchemy/Enchanting crafting 関数、SEFF VM、MGEF+0x68 ESM 書込元 (§66.11)
- [~] 1-3: 疲労・スタミナ・死亡処理 — 死亡 vtable +0x2c8、ノックダウン、蘇生（Fatigue 式・自動 KD は §56.1 で確定）
- [~] 1-4: AI 詳細 — 種別名表 0xB12988 (33 件)、パッケージ選択 0x569020、検知 4 状態は確定。残 11 件 (§58.1)
- [~] 1-5: クエスト・対話 — 条件関数 169 件 (0xB0C8C0 関数帯 +0x20)、QUST/INFO/説得は確定。残 8 件 (§58.2)
- [~] 1-6: スクリプト VM — opcode 完全表、イベント送出全種 (※ 0x9832e6 は ___RTDynamicCast でありイベント送信ではない — C5/§55.3 訂正)。Wave 5: イベントビット表と送出 77 件、SCDA 2,393 全数パースを確定
- [~] 1-7: セーブ/ロード — manager 0xB33B00、save 0x465130 / load 0x465860、.ess 版数 0x7d、bSaveOn* は確定。残 8 件 (§58.3)
- [~] 1-8: キャラクタ生成・レベルアップ — スキル上昇 0x668070、ApplyLevelUp 0x66C3E0、属性ボーナス表は確定。残 8 件、birthsign 付与が最優先 (§58.4)。Wave 5: 星座ピッカーは menu 0x408 mode 3、AddSpell 0x514950 を確定 (残: 付与本体)
- [~] 1-9: 経済・所持品 — 商人、家、盗み判定（ContainerMenu 全経路・窃盗判定は §56.5 で確定）。Wave 5: 価格経路とラッパ (0x5E11A0/0x5E1200/0x5E1500) を確定、0x676EE0 = 登録処理に訂正
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

| 指標 | 現状 (2026-10-23) | 目標 |
|---|---|---|
| EXE 関数トリアージ率 | 未計測 | 100% (Phase 5) |
| 文書化関数 (tier A) | 683 (FUN 参照ユニーク) | 移植関連 + エンジン主要の全数 |
| RE_EXE_REPORT § 数 | 69 (§50〜§69 追加済み、§53 欠番) | 継続 (§100 以上) |
| Script VM ハンドラ | MISS 480 を台帳化 (§67)、移植済み 0 | 480/480 |
| Script VM FunctionID | 欠落 7 件を仕様化 (§68)、実装済み 0 | 7/7 |
| ESM レコードタイプ | 部分 (数種) | 全 ~50 種 |
| シェーダ変種 | 176/224 | 224/224 |
| BSA / NIF / KF 形式 | 部分 | 全形式 |
| 補助バイナリ | 0 | 一次解析完了 |
| 移植トレーサビリティ | 暗黙 | 表で 100% |
| 0x400-low 誤アドレス | **0 件** (V3 監査: 131ファイル/16,081件、17候補は全件誤検出) | 0 件 |
| 呼出しリスト GT 一致率 | **100%** (1,010/1,010 行、ノイズ0件) | 100% |
| Ghidra デコンパイルギャップ | impl 527件中 **362件** 欠落 (対応表 `tmp/re/v5_ghidra_gaps_report.md`) | 0 件 |

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
- 2026-10-22: **§67〜§69 追記 (opencode 解析成果の統合)** — §67 Script VM MISS ハンドラ台帳 480 件 (VM 関数テーブル 0xB0AF48 / 532 エントリ / stride 0x28、gate 0x4FAE80、実装サイズ 153B〜160B の定型関数、block-type 29 / 中間域 124 / command-type 327、頻度実測 112 件)。§68 Script VM FunctionID 追加仕様 7 件 (StopQuest 0x1037 / Evp 0x105E / PlaySound 0x1026 / ModCrimeGold 0x1076 / SetCrimeGold 0x1075 / KillActor 0x108B / ResurrectActor 0x108C、新規 enum 0x009A〜0x009F)。§69 リスト/イベント・コンテナ構造 (Push 0x446CB0 / Pop 0x6135F0 / Search 0x613640 / Event Add 0x4FB510 / Check 0x4FB5F0 / Cleanup 0x4FA020、NodeData 8B、`+0x40` 所有 = TESObjectREFR vt+0x330 [推定])。訂正: `0x613605` / `0x61E5D9` は独立関数ではない。KPI 更新 (683 FUN / 11,629 行 / §69)。

- 2026-10-23: **0x400-low 訂正キャンペーン完遂 + 台帳再生成 (§49.0.1)** — マッピング ground truth (`.text` 0x401000〜0xA27C39 / `VA=0x400C00+file_off`) を確定し、narrative 約35ファイル・約3,156箇所を +0x400 訂正 (`.bak`〜`.bak7`)、オラクル LOW 925→3。呼出しリスト列はオペコードテーブル由来の impl から capstone で **997行再生成** (`.bak8`、doc==GT 100%)。残ノイズ1行 (opid 0x1078) は窓先頭 impl+0x400 の境界ずれ + 0x400-low の2層原因と解明し修正 (`.bak9`) → 全1,010行でノイズ0件。**V3 過去確定値監査は新規 0x400-low 0件**。V9 は空欄20 opid を K1スタブ8/K2間接1/K3空0/K4インライン11 に分類 (impl 0x977C50 を7 opid が共有)。V5 は FUN_* 20,465件を抽出し impl 527件中362件のデコンパイル欠落を表化。V2 総括を RE_EXE_REPORT §49.0.1 へ追記。報告書: `tmp/re/v3_audit_report.md` / `tmp/re/v4v9_unresolved_report.md` / `tmp/re/v5_ghidra_gaps_report.md`。KPI 更新 (RE_EXE_REPORT 11,755 行 / 0x400-low 0件 / GT 100%)。
- 2026-10-23: **【訂正】「MGEF 32 コード」は誤り** — 実 ESM (Oblivion.esm 277,504,985 bytes) を全走査した結果、**MGEF レコード 145件（EDID=効果コードの正本）/ EFID distinct 150種** が正で 32 ではない。EXE 側 push<fourcc> 検出の115種はその部分集合（共有113）。登録 ctor は `0x9FEA90` の**23件のみ**（形式 `push <ctor>; push <fourcc>; call 0x68DA10`、ctor が fourcc の5バイト前）、**登録23 ⊂ ESM145 = 100%一致**。未登録122種は `cmp_branch 8 / default_path 114 / undetermined 0`。**Z*/BA*/BW*/MY* は MGEF コード**（旧「別名空間」説は撤回、実ESMのEFIDに全33種実在）。FormID リンクは**実測60件**（旧「62件」は Conjuration MGEF 総数との意味ずれ、REAN/TURN は actorValue=0）。証跡: `tmp/re/v32_esm_efid_report.md`（レコード1,167,017 / errors 0 / 圧縮41,789 / XXXX 1件）+ `tmp/re/v35_mgef_code_report.md`（23組の code→ctor 完全表、23/23 が RTTI でクラス同定）。KPI 更新 (MGEF 145件 / 登録 ctor 23件)。
- 2026-10-24: **V61 headless 再解析の実測を RE_EXE_REPORT §67.6.1 へ反映** — Ghidra **12.1.4 PUBLIC** (Java21) を導入し `analyzeHeadless` を完走 (2026-10-24 19:38:29–19:44:52、分析353秒、ERROR 0件 / WARN 11件、関数総数 21,240)。ギャップ362件は **362/362 `NO_FUNCTION`** (`defined=0` / `contained=0` / `containing_name` 全行空) で、新旧どちらの自動解析でも関数化されない。うち 260件 (`ghidra_missing`) は V5 cc前置≥4 集合と完全一致 (257ユニークVA・対称差0)、102件 (`absorbed_suspect`) の「前関数に吸収」機構説は V61 では **0/102 で観測されず弱化**。§67.6.1 の旧記述「Ghidra 本体不在・未実施 (TODO V23)」は**失効**として置換。`decompiled.c` との差は lowercased `FUN_` 定義 20,465 対 21,240 = **+775**。opencode の解析ノート V60〜V76 (12 本、`tmp/re/`) は**未統合**のため §70 候補として保留。
- 2026-10-24: **opencode 解析ノート V60〜V76 (12 本) を RE_EXE_REPORT §70 へ統合** — 確定事実を「GMST/GameSettings (382=378+4、`sSpecialization` 登録、runtime vtable `0xA40454`、BlendSettings 62、Water 実行時キー)」「Ghidra headless 再解析 (21,240 関数 / 362/362 `NO_FUNCTION`)」「opcode 表 `0xB0AF48` 532×0x28 と dispatcher `0x4FDE30`」「Show1stPerson (opcode `0x113` → `0x50A050` → `+0x588`)」「`0xB333C4` 0x804B 状態オブジェクト」「Topic_Resolve `FUN_0052F010`・FunctionID 6 件・`StopQuest=0x7A`」「シェーダ/D3D/INI の 7 ハブ」として文書化。**要再導出**: オペコード表のフィールドオフセット (V76 は `+0x18`/`+0x1C`、現行 doc は `+0x20`)。**根拠なし**: 「EVMP 516」(実測 532)。**ハーネス既知バグ 2 件**: `rd()` の `.rdata`/`.data` 0x600 ギャップ、`dec_index` の網羅漏れ。KPI 更新 (RE_EXE_REPORT 11,939 行 / 12 本のノートを統合完了)。
- 2026-10-25: **オペコード表のフィールドオフセットを確定（「要再導出」の解消）** — 532 件全数を `pe.get_offset_from_rva()` 経由で読了し、`+0x04` 別名 / `+0x08` オペコード id / `+0x0C` 説明 / `+0x12` word 引数個数 / `+0x14` 引数記述子 / `+0x18` 実装 / `+0x1C` 共有 dispatcher / `+0x20` 条件関数 / `+0x24` 小フラグを確定。**V76 の「`+0x18`/`+0x1C`」と doc の「`+0x20` 条件関数 169 件」は矛盾せず別フィールドで、doc の記述は正しい。** V76 の「`+0x20`/`+0x24`=0」は調査対象が idx 0〜5 帯に限られたサンプリング誤り。逆アセンブル `0x4FF880`〜`0x4FF9BE` で `mov ebx,[eax+0x1c]; call ebx` を確認。id→idx は `op ≤ 0x1E` は idx=op、`0x100-0x183` は idx=op−0xE1、`op ≥ 0x1000` は idx=op−0xF5D。全件表は `docs/SCRIPT_OPCODE_TABLE.md`。
- 2026-10-25: **V79 吸収仮説の検証と V80 裏取り** — ギャップ 357 VA の「既存関数へ吸収された断片」仮説は **不成立**（class1 256/71.7%、class2 98/27.5%、class3 3/0.8%、cc バンドとの相関 χ²=0.611・p≈0.43、本文完全一致 0 組）。V80 で薄片 101 件をオペコード表と照合し **101/101 が `+0x18` ハンドラ**と確認（データ誤解釈ではない）。class3 の 3 件は `0x500820`=ShowRenderPasses(0x0152) / `0x501A60`=ToggleGrassUpdate(0x011A) / `0x5087A0`=ToggleShadowVolumes(0x0124)。V5 表の重複 5 VA は**別名オペコードのハンドラ共有**（テーブル誤りではない）。帯域 `0x500770`〜`0x50D8C0` に全 532 のうち 445 ハンドラ（distinct 439、84%）が集中。RE_EXE_REPORT §70.12 へ統合。
- 2026-10-25: **【注記】§67 台帳の 480 件分類との母数差** — 256 行の「block-type 29 / 中間域 124 / command-type 327」は計 480 で、§67 の MISS ハンドラ台帳 480 件の分類であり全 532 エントリの分類ではない。全 532 件の帯域別内訳は `0x00-0x1E` 31 / `0x100-0x183` 132 / `0x1000-0x1170` 369。両者は母数が異なるため直接比較できない。
- 2026-10-25: **移植ヘッダ ⟷ 正典オペコード表の突合 (§70.14)** — `app/src/main/cpp/script/script_opcodes.h` の 3 enum を `docs/SCRIPT_OPCODE_TABLE.md` と**名前ベース**で突合 (数値突合は不可。ヘッダ 171〜172 行が「FunctionID は合成 VM コール ID。native SCDA 値から remap してはならない」と明記)。証跡: `tmp/re/v81b_h_to_table_xref.py` / `tmp/re/v81c_h_to_table_map.py` (成果物 TSV `tmp/re/v81c_h_to_table_map.map.tsv` 156 行) / `tmp/re/v81d_scda_marker_evidence.py`。`Opcode` は VM 固有のスタック/コール符号化で正典表とは**別空間** (突合対象外)。`NativeScdaOpcode` の 12 値は**すべて実測で裏付けられ、ヘッダ修正は不要** — `Stop = 0x0011` はブロック終端 (`0x0010 Begin` と完全同数 4,344 回) / `Marker = 0x001C` は参照スロット接頭辞 (固定 4 B `[u16 op][u16 refSlot]`、5,647 回、直後は `0x1021 Enable` 520 / `0x100D Activate` 519 など参照省略系) / `Prologue = 0x001D` はスクリプト先頭プロローグ (2,393 = スクリプト総数、全件 `+0000`)。当初この 3 値を「正典表と名前不一致」と見たのは**空間の混同**で、正典表の 0x0000〜0x001E 行は**ブロック型 ID** (`0x0010 Begin` の第 1 引数、§57.4 の注記どおり) であり同名 opcode ではない。ブロック型 ID の国勢調査 (2,393 スクリプト): GameMode 1332 / OnActivate 891 / OnDeath 441 / OnPackageDone 307 / OnReset 215 / OnLoad 208 / ScriptEffectStart 84 / OnActorEquip 1 / **OnActorUnequip 0** (バニラ未使用)。`FunctionID` 156 件の分類は **EXACT 85 / CAND 32 / ABSENT 39**、正典 532 件のうち **448 件はヘッダ未参照**。ヘッダのコメント中の native 名ヒント 7 件は 7/7 一致。**残務**: ABSENT 39 件の機能帰属の確定 (Phase 68 の dispatch 結線前)。`docs/SCRIPT_OPCODE_TABLE.md` の 0x0000〜0x001E 行への「ブロック型 ID」注記は**2026-10-25 に追記済み**。
- 2026-10-25: **正典表へブロック型 ID 注記節を追加** — `docs/SCRIPT_OPCODE_TABLE.md` に `### 0x0000〜0x001E 帯の注意（ブロック型 ID）` を新設 (597 → 608 行)。ブロック型 ID の出現数と SCDA 制御オペコードの出現数を並記し、`0x0011` / `0x001C` / `0x001D` の同名値が**別用途**であることを明記。掲載値は `tmp/re/s59_svm_r7_bytecode.txt` (2,393 スクリプト / 65,628 命令) と全件一致を確認。同時に `docs/RE_EXE_REPORT.md` §70.14 の残務項目を完了化。KPI 更新 (RE_EXE_REPORT 12,085 行)。
- 2026-10-25: **ABSENT 39 件の帰属を確定 (v82)** — `script_opcodes.h` の `FunctionID` 156 件のうち正典表に名前がない 39 件を 3 系統の一次証跡で確定。(1) EXE に登録名として存在しない — 正典表の登録長名 532 件は EXE 内 ASCII 文字列として実在し (出現範囲 `0x62EC2C .. 0x714CED`、1 名 1 箇所)、EXACT 85 件のうち 83 (98%) がヒットするのに対し ABSENT は 5/39 (13%)。単語境界つき検索では REGISTERED は実質 0 件で、ヒット 5 件はすべて部分文字列アーティファクト (`IsCreature`⊂`GetIsCreature` / `IsGhost`⊂`GetIsGhost` / `IsAlerted`⊂`GetIsAlerted` / `Random`⊂`GetRandomPercent` / `GetValue` は表の名前領域外)。UTF-16LE は全 0 件 (識別名は ASCII のみ)。(2) 正典表 532 件の登録長名との完全一致は 0 件 (ALT_EXACT 0)。(3) 移植コード 1,652 ファイルの全数走査で 39 件すべてが `script_disasm.cpp` (約 300〜410 行) と `script_functions.cpp` (約 155〜254 行) の**表示名 switch のみ**に出現 (各 2 箇所、`case FunctionID::X: return "X";`)。fid 解決・dispatch には不関与。関係分類: `EXACT_TABLE 0 / SUBSTR_TABLE 4 / RELATED 18 / UNRELATED 17`。RELATED の高確度例は `IsLocked`→`GetLocked 0x1005` / `IsDead`→`GetDead 0x102E` / `GetSex`→`GetIsSex 0x1046` / `GetRace`→`GetIsRace 0x1045` / `IsInFaction`→`GetInFaction 0x1047` / `IsUnconscious`→`GetUnconscious 0x10F2` / `IsDetected`→`GetDetected 0x102D` / `GetFame`→`GetPCFame 0x10F9` / `GetOwner`→`IsOwner 0x1116` / `IsOpen`→`GetOpenState 0x109D`。UNRELATED 17 件は正典表に一切関係名がない (`SetObjectiveCompleted` / `GetObjectiveCompleted` / `IsQuestStageDone` / `IsImmobile` / `IsInvulnerable` / `GetSpellCount` / `GetRelationship` など)。**結論: 表示名のみの差異であり fid・dispatch・条件評価に影響しないため機能的欠陥ではなく、是正不要** (`FunctionID` は合成 VM コール ID で remap 禁止)。証跡: `tmp/re/v82_absent_fid_classify.{py,txt}` / `tmp/re/v82b_absent_boundary.{py,txt}` / `tmp/re/v82c_absent_related_names.{py,txt}`。§70.14 に (4) 節を追記。
- 2026-10-25: **オペコード表の列の実体を確定 (V83)** — 532 件の raw PE 再計測で `+0x10` が単一フィールドでないことを確定。byte0 = フラグ A (272 件が 1、begin ブロック適法性) / byte1 = **未使用 (全 532 件 0。旧「フラグ B = 0x01/0x02」は失効)** / u16 `+0x12` = 引数個数 (総スロット 402)。u32 としての「nonzero = 408」は byte0 フラグ (272) と argn>0 (281) の**和集合** (重複 45)。`+0x14` は **引数記述子配列 ptr** で、`.data` プール `[0xB0A11C, 0xB0AF48)` の 12 バイトレコード `{name ptr(.rdata), type 0x00-0x25 = 38 種, optional 0/1}` が引数個数ぶん連続する (281/532 が非ゼロ = `+0x12 > 0` と完全一致、実体 179 ユニーク、共有例 `0xB0A6CC = {Integer,1,0}` を 42 コマンドが共用)。`.text` 落ち 0 件により「alternate handler」説は棄却。決定的証拠はコンパイラ `FUN_004ff710` (`0x4FF710`) が `argn = +0x12` / `argdesc = +0x14` / `dispatcher = +0x1C` を取り出し `(*dispatcher)(argn, argdesc, &argbuf, ctx)` を呼ぶ点で、共有 dispatcher `0x4FDE30` (528/532) が記述子列に従って引数をデコード・検証する。旧 GT「`+0x14` = OnHit/OnActivate の 2 件のみ」は、両者が同一記述子 `0xB0ABAC` を共有しているだけのサンプリング偶然に由来する誤り。`+0x04` (実別名 176 件 / 空は共有 `0xA2F7EC` が 356 件)、`+0x0C` (非空 132 件・0 は idx162 のみ) も全数計測で更新。`docs/SCRIPT_OPCODE_TABLE.md` のレイアウト表と型コード目録も同時に更新 (608 → 638 行)。残る未確定は `+0x10` byte0 の**コンソール帯での意味** (`0x100` 帯 8/132・`0x1000` 帯 235/369 が 1)。証跡: `tmp/re/v83_col14_meaning.md` (Git 管理外)。
- 2026-10-25: **V84 引数コンパイラ `0x4FDE30` の型分岐を解読 (38 型 → 2 ファミリ → 9 経路)** — `+0x1C` の共有 dispatcher が `ScriptCompiler::StandardCompile` の**引数コンパイラ**であることを確定し、記述子の型コード (38 種) がどのように compiled 形式へ落ちるかを capstone 直接逆アセンブルで実測した (Ghidra 未接続・REA はタイムアウトのため自前解析)。**V84a**: 候補ヒューリスティクスを厳密化 (名前非空を必須) し **候補 192 / 消費 179 / 未消費 13 / グリッド外 0** (V83 の 179 と一致)。記述子プール `0xB0A11C〜0xB0AF48` は均質な 12 B 配列ではなく、**8 B 型情報テーブル `0xB0A548`** (38 件、`{u32 type_code; u8 fieldA; u8 fieldB; u8 pad2}`) が混在する。**V84b**: スイッチ前文 (`0x4FDF7C`) が `fieldB` をファミリセレクタとして読み (**main 27 型 / alt 11 型**)、main テーブル `0x4FEEAC` は 35 エントリ (`0x4FEEAC + 35*4 = 0x4FEF38` で alt と連続)、うち 8 エントリは `0x4FED7E` トラップで `fieldB==0` の範囲内 8 型と完全一致 → **38 = 3 + 27 + 8**。alt は `0x4FEF5C` の 38 B バイト分配テーブルで **9 経路**に落ちる (文字列 u16 / インライン数値 (先頭 `0x47`='G') / Actor Value / Axis (X/Y/Z) / Animation Group / Sex / Crime Type / Form Type / Variable Name=未実装)。**main 27 型は全ハンドラが `0x9832E6`+`0x4FCE30` へ委譲し型に依らず同一形式**。型別スロット数 (`0x01`=135 / `0x02`=47 / `0x06`=34 …) と optional 52 件 (うち 11 件は名前マーカーなし) も確定。alt 9 経路の数は移植側 `NativeTokenKind` 9 種と一致し、idx 1 の `0x47`='G' は移植側 `isVariableTypeChar` (`f/s/r/l/G`) に含まれる。証跡: `tmp/re/v84a_argtype_parity.py` / `tmp/re/v84a_run2.txt` / `tmp/re/v84b_switch.txt`。反映先: `docs/RE_EXE_REPORT.md` §70.4 (V84) 節 / `docs/SCRIPT_OPCODE_TABLE.md` 「型コード 38 種のコンパイル挙動」節。**残る未確定**: `+0x20C` に書かれる幅・先頭文字の全量 (呼出し先へ委譲される部分) と `+0x10` byte0 のコンソール帯での意味。
- 2026-10-25: **V80 Part B（高参照グローバル 15 件）を `docs/RE_EXE_REPORT.md` §70.15 に収録**。.text imm32 出現数を PE 実体のバイト計数で再現し **15/15 完全一致**。`0xB42754`（レンダラ実体）・`0xB42E8C`（シェーダエラー callback）・`0xB02E24`（INI 設定ノード値）を確定、`0xB43104/108/10C/110` の構造体説を棄却、FUN ラベルなし実在関数 `0x7B8530` を検出。

- 2026-10-25: **FUN ラベルなし関数の全数列挙 (§70.16)** — opencode の Ghidra FunctionManager 台帳 `%TEMP%\opencode\func_list_new.txt` (21,597 件) を ground truth として確立し、`.text` の `ret + 0xCC` 境界走査 → capstone 妥当性検証 → 台帳差分 → E8 呼出し分類で **UNLABELED 10,175** (A: E8 呼出し先 **214** / B: imm32 参照のみ 9,809 / C: 無参照 152) を列挙。既知 3 例を再判定し、真の FunctionManager 未登録は `0x7B8530` / `0x405150` の 2 例、`0x50AA20` は `FUN_0050aa20` として登録済みと確定。境界規則の再現率は 16,607/21,597 = **76.9%** であることを明示 (限界)。