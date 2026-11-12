# v180_letter_rows_audit.md — 旧 letter-row 55 件の棚卸しと V 系束化起票案

- 作成日: 2026-10-29 / 担当: TODO 棚卸しサブエージェント（TODO serial 167 / V167・低の分析専任）
- 編集範囲: **本新規 1 ファイルのみ**。docs/ / GT_CHEATSHEET.md / TODO_re49.md / 台帳 / plan / session-memory / 既存 tmp/re 成果物は一切変更していない（読取のみ）。git 不使用。ESM 再スキャンなし。Ghidra 呼出なし。
- 入力（読取専用）: `tmp/re/TODO_re49.md`（581 行 / 155,612 B 全文読解） / `tmp/re/v_data_relmap2.md`（§3-①・§4-4・§5） / `docs/RE_ANALYSIS_ROADMAP.md`（272 行 / 38,799 B） / `tmp/re/` の v・b・s 系成果物（判定根拠の必要箇所のみ）。
- 方法: `- [ ]` 箇条書きを正規表現で全数抽出（`%TEMP%\opencode\v180_extract.py` → 箇条書き合計 83 件のうち未チェック **55 件**（L33–L162）を確定）→ 各行の出典と既存成果物を突合し (a)/(b)/(c) 判定。

---

## 0. 結論サマリ

| 判定 | 件数 | 内容 |
|---|---:|---|
| **(a) 解決/包含済み** | **21** | 既存成果物で主要項目が確定。うち v/b 系（opencode）由来 4、s 系（Copilot）由来 14、G2 行内デコンパイル記録 3 |
| **(b) 人間/外部待ち** | **9** | B13 実機ダンプ・実機確認・.ess サンプル・ユーザー判断・L2 実装有効化 |
| **(c) 未着手のまま残る** | **25** | 現 V 系で未着手の分析対象（s 系土台があるものは「土台:」を根拠に明記） |
| 合計 | **55** | 抽出件数と完全一致（L33–L162） |

- `v_data_relmap2.md` §3-① の前提「55 件は一度も触れられていない」は **TODO 行の状態（[ ] のまま）として正確**。ただし **内容面では 21 件が既存成果物で解決/包含**されており（行が更新されていないだけで成果物は存在）、起票前に (a) 分のクローズ判定（親セッションが行 [x] 化）を行う価値がある。
- 真に未着手なのは **25 件**。これを **8 束**に圧縮して起票案を作る（§3）。推定工数合計 **9.5–13 人日**。
- 特筆: `s81_stream_audio_crt_io.md`（2026-10-18）が **O2/O3/O5/O6 を直接消化**しており、O 節は「全未着手」ではなく 4/6 行が (a)。これは relmap2 の「O 群は解析本体」記載の修正素材となる（§6）。

---

## 1. 判定基準（本監査での運用）

- **(a) 解決/包含済み**: その行の主要項目が既存成果物（v/b 系・s 系を問わず）で確定しているもの。成果物名を根拠に明記し、系列（v/b=opencode / s=Copilot）を併記。微小な残点は注記し、§3 の束へ持ち越し。
- **(b) 人間/外部待ち**: B13 実機ダンプ・実機確認・.ess サンプル・ユーザー判断・移植実装（L2 有効化）待ち。
- **(c) 未着手**: 現 V 系でまだ着手されていない分析対象。s 系土台があれば「土台:」として根拠に明記し、束化対象にする。

---

## 2. 55 件全数判定表（節別）

### F 節（Wave 4 — §56 残件 + 派生、L33–L38）: 6 件

| L | ID | 内容（要約） | 出典 | 判定 | 根拠（成果物・系列） |
|---|---|---|---|---|---|
| 33 | F2 | 音響残件 7 項目 — AM+0xA6 絶対/相対・FUN_6AD030 序盤 bit2・曲カーブ (2×[2F0]-1)×[2F8]・type bit 0x20・[0xA785E8] 適用箇所・0x65A9C6/0x65AC82・FUN_6AC210 呼出元 | §56.2/§56.8 | **(c)** | 土台(s): `s56_audio_residual.md`（出典本体）、`s50_audio.md`（FUN_6AD030=毎フレーム music director [確定]）、`s49_section.md`。type bit 0x20・フェード変数は s81 B2/B3 で **O3 側は解消済**（重複項目）。実質 5–6 項目が未着手・V 系未投入 → 束D |
| 34 | F3 | Sleep/Jail 残件 8 項目 — [0xB383A8]/[0xB383B0]・0x4AE8C0 未読分岐と 0xA41DE8 表・0x6738B3・[0xB13228] 意味・0x670CA0 終端/0x67108F・0xA73DF0・0x4FAE80・[0xB14E88]/[ebx+0x1DC] | §56.3/§56.8 | **(c)** | 土台(s): `s56_sleep_tail.md`/`s56_section.md`（0x670CA0=jail enter/exit [確定-強]、[0xB13228]=autosave ゲート利用 [確定]）。残る終端・未読分岐・表が未着手 → 束E |
| 35 | F4 | menu 式残件 4 項目 — &token→float 搬送路・ln/log (0x7E7/0x7E8)・第2コンテナ 0xB13BC4・VideoDisplayMenu 実行時更新 | §56.4/§56.8 | **(c)** | v/b 系・s 系ともに該当 4 項目の成果物なし → 束E |
| 36 | F5 | Container 残件 8 項目 — 同一再オープン実機確認・runtime メッセージ form・+0x63(a4) 0x59ABEA-0x59AC6F・user0==2.0・0x422DC0/0x41E210・0x57ACC0・handler id 0xFDF 系 | §56.5/§56.8 | **(c)** | 土台(s): `s56_container_residual.md` + B2（転送コア FUN_00492E70 [確定]）。残 8 項目は未着手。**うち「同一再オープン実機確認」は (b)（G8/H7 と同一）** → 束E（実機項は (b) へ分離注記） |
| 37 | F7 | 移植: settings_manager.cpp 難易度式修正の適用判断・実装（実効 M=5.0） | 移植 settings_manager.cpp | **(b)** | 式自体は A5/§50.1 で確定済み。**適用可否はユーザー判断待ち**（TODO 明記・relmap 優先順 12） |
| 38 | F8 | 移植: 裸 stackingtype を 0 扱いで実装（対象 XML 4） | 移植 XML 4 | **(b)** | 移植実装タスク。L2 有効化/移植セッション待ち（relmap 優先順 12） |

### G 節（Phase 1 Wave 4a — §57 統合、L47–L49）: 3 件

| L | ID | 内容（要約） | 出典 | 判定 | 根拠（成果物・系列） |
|---|---|---|---|---|---|
| 47 | G6 | 移植差分の確認 — combat_manager.cpp 固定値（ブロック×0.3・クリティカル 5%/×2.0）の根拠確認・player.cpp 疲労固定レート・script_functions.cpp:974 fnResurrect スタブ | 移植 3 ファイル | **(b)** | 移植差分の扱いはユーザー判断群（F7/I7/J6 と同一・relmap 優先順 12）。うち「固定値の根拠確認」は分析要件を含むが適用可否が本質 → 人間判断 §5-4 に集約 |
| 48 | G7 | 既存記述の訂正反映 — §56.3 テーブル基底 (0xB0AF48/stride 0x28/532 件)・0x4FDE30 の性格・FUN_004F3870 誤記・Actor vtable 0xA71814 誤記 | §57.6 | **(a)** | **事実面は v/b 系で確定**: 0x4FDE30=引数コンパイラ StandardCompile（`v159` §4-4 [確定] + V132 + §70.4 V84「38 型→9 経路」）、表 0xB0AF48 532×0x28（§70.8 確定・V132 で 533 行=フェンス含む口径裁定）、Actor vtable 0xA71814=HighProcess 誤記（G3 2026-10-21 デコンパイル）。**docs 反映は要承認バッチ（relmap §5 G-C）へ** → §5-3 |
| 49 | G8 | 実機確認（B13 と併せてユーザー依頼）— ContainerMenu 同一対象再オープン挙動 | 実機 | **(b)** | relmap §5 **G-F**（ユーザーのみ） |

### H 節（Phase 1 Wave 4b — §58 統合、L57–L58）: 2 件

| L | ID | 内容（要約） | 出典 | 判定 | 根拠（成果物・系列） |
|---|---|---|---|---|---|
| 57 | H6 | 訂正反映 — AlchemyMenu=0x410（0x3F1=DialogMenu）・条件関数は独立テーブルでなく関数帯 entry+0x20・esm_reader の AI 解析は実形式不一致 | §58.6 / §18.6 | **(a)** | AlchemyMenu=0x410（H5/§58 で確定）、条件関数=0xB0C8C0 関数帯 169 件（§58 + **b10/V132/v134** で帯 A/B 読者なしまで確定）、§18.6 は docs 既載。**docs 反映は要承認バッチへ** → §5-3 |
| 58 | H7 | 実機確認（B13 と併せて）— 実 .ess サンプル取得と突合・ContainerMenu 再オープン・難易度式の移植修正 | 実機 | **(b)** | relmap §5 G-F（ユーザーのみ） |

### I 節（Phase 1 Wave 5、L63–L64）: 2 件

| L | ID | 内容（要約） | 出典 | 判定 | 根拠（成果物・系列） |
|---|---|---|---|---|---|
| 63 | I6 | 実 .ess サンプル不在のためバイナリ突合は保留 | .ess サンプル | **(b)** | relmap §5 G-F（ユーザーのみ・s58_save.md 残件と同一） |
| 64 | I7 | KD 到達性の移植仕様判断（damage 整数部 0 の約 1% 再現要否）と難易度式・combat_manager 固定値・RNG (mt19937→MSVC LCG) 等の移植差分の扱い決定 | 移植判断 | **(b)** | ユーザー判断（relmap 優先順 12・§59.9 の RE 側確定は済み、判断のみ未了） |

### J 節（Phase 2 Wave 6a、L71–L72）: 2 件

| L | ID | 内容（要約） | 出典 | 判定 | 根拠（成果物・系列） |
|---|---|---|---|---|---|
| 71 | J5 | Wave 6b — 2-1 レンダラ / 2-2 Havok 物理 / 2-3 アセットストリーミング | Wave 6b §61 | **(a)** | **Wave 6b 完了 2026-10-17**（ロードマップ更新履歴「Phase 2 Wave 6b 完了 (§61)」+ `s61_render.md`/`s61_havok.md`/`s61_stream.md` + K1/K2/K3 [~] で残件管理）。**残件は O1/O2 へ分離済み** |
| 72 | J6 | 移植差分の反映 — RNG 置換・round half-up・KD 到達性・難易度式・combat_manager 固定値 | 移植差分 | **(b)** | ユーザー判断（G6/I7 と同一群・relmap 優先順 12） |

### N 節（Wave 7 残存、L91–L96）: 6 件

| L | ID | 内容（要約） | 出典 | 判定 | 根拠（成果物・系列） |
|---|---|---|---|---|---|
| 91 | N2 | HavokContext 0xB3BD00 の実行時 vtable 確定 — B13 実機 .data ダンプ待ち | Wave 7（B13 依存） | **(a)** | **`s68_n2_context.md`（2026-10-18）「HavokContext は vtable を持たない」**: [+0x00] への書込 0 件（4 パターン全 0 hit）・`call [ecx+N]` 仮想呼出 0 件・92 メソッドは全て `mov ecx,0xB3BD00; call` 直接呼出（VC++7.1 devirtualize 推定）。`s69_hc92.md` が先行知識として引用。**※矛盾**: relmap2 §5 G-B と handover は「B13 実機ダンプ待ち」と記載 → §5-1 で人間裁定へ |
| 92 | N3 | bhkCharacterProxy vtable — 1584 件偽陽性 (0xAA3E80) を除外して再探索 | Wave 7 | **(a)** | **v/b 系で解消**: `b07_vtable_index.md` 行1596 `.?AVbhkCharacterProxy@@ → 0x00A9667C（32 slots）`、0xAA3E80 は TypeDescriptor の vftable と同定（b07 L39/b24 L176）＝偽陽性の正体判明。非 RTTI 側は `b13` §590-598 + `b26` §C で完全クラス hkCharacterProxy・vptr 書込復元（L4 の「0xAA3E80 偽陽性」記載は b13/b26 で更新済み）。残: b26 R-5（ahk/bk 型関係・小） |
| 93 | N4 | ゲーム側 RayCast 経路（activate → castRay 0x8A2650）— "HavokPick:HAVOK" 0xA126A6 手掛かり | Wave 7 | **(c)** | 土台(s): `s63_h2_raychar.md` C2（ゲーム側完全パス残件）+ `s65_havok_char.md` §5.1 + `s61_havok.md` R2。**前提に矛盾**: K2（2026-10-21 デコンパイル）「0x8A2650 = castRay ではなく等値比較子」 vs s62/s63「slot[11]=0x8A2650=Shape::CastRay」→ 束Fで裁定。V 系未投入 → 束F |
| 94 | N5 | 戦闘 AI: 呪文選択・戦闘状態遷移 3 補助関数 / Combat vtable slot[4] Update | Wave 7 | **(a)** | `s68_n5_combat_ai.md`（2026-10-18「完全解析…状態: 完了」）: Combat vtable 0xA70A14 **slot[4]=0x61F8D0 Update wrapper [確定]**（0x61C130 CombatUpdate_Body 5,808B）、slot[9]=0x612530 新確定。+ `s68_n5_spell_selection.txt`/`s68_n5_slot4_update_wrapper.txt` + `s69_cai_*`（spell_select/state_handler/attack_selector）+ Wave9 M14（`s98_magic_m15_misc.md` AI 呪文） |
| 95 | N6 | 0x5F2820（検知時間累算+隠密計算）の完全式と視野角統合 | Wave 7 | **(a)** | `s68_n6_detect.md`（「0x5F2820 全逆アセンブル完了、FOV/隠密/状態接続確定」第1次完了）+ `s68_n6_f2820_full.txt`（24,983 B）+ `s68_n6_sub_683D80_cone.txt`（視野角コーン）+ `s64_ai_combat_3/4`。M6 時点（2026-10-17）の残件は翌 18 日に s68 で解消 |
| 96 | N7 | PathingSystem 詳解（NavMesh 直線移動・障害物回避・steering）— Phase 3 連携 | Wave 7 / Phase 3 | **(c)** | 土台(s): `s72_navm.md` + `s72_navm_a1..a50`（NavMeshManager 0xB333A0 構造・ローダ・pathfinding dispatch・hkp_path・pathnode）。**M7 行で「NAVM 詳細は Phase 3 継続」と明示遅延**、steering/障害物回避の完全文書化は未。ロードマップ Phase 3-3 と接続 → 束F |

### O 節（Phase 2 エンジン残件・非 Havok、L99–L104）: 6 件

| L | ID | 内容（要約） | 出典 | 判定 | 根拠（成果物・系列） |
|---|---|---|---|---|---|
| 99 | O1 | レンダラ — SetLight 代替機構 / DrawIndexedPrimitive 経路 / BSShaderPPLighting 完全関数マップ / シェーダハンドルテーブル / 天候ロード / Portal・DMap カリング | s61_render.md | **(c)** | 部分土台(v/b): `v129` §1(1) FUN_007B4290 factory 28 種→クラス対応表 [確定]（= シェーダハンドルテーブル相当、relmap §3-① も「v159 で短縮可能」と指摘）+ `v159`（renderer[+0x280]=IDirect3DDevice9 等 15 件）。残 5 項目（SetLight/DrawIndexedPrimitive/BSShaderPPLighting/天候/Portal・DMap）が未着手 → 束C |
| 100 | O2 | ストリーミング — GridsToLoad 参照経路 / テクスチャキャッシュ 0xB4205C / NiNode ツリー NIF 組立 / Cell load thread / LOD テクスチャブレンド / メモリプール拡張 | s61_stream.md | **(a)** | **`s81_stream_audio_crt_io.md` Task A（2026-10-18）**: A1 GridsToLoad [RESOLVED]・A2 テクスチャキャッシュ 0xB4205C/0xB42060 [RESOLVED]・A3 Cell Load Thread [RESOLVED]・A4 LOD Blend [PARTIAL]。**残 3（NiNode NIF 組立・メモリプール拡張・LOD blend 経路詳細）→ 束C へ持ち越し** |
| 101 | O3 | オーディオ — AM+0xDC bit2 / type bit 0x20 / フェード変数 / DS3D vt+0x40/44 / GetState 戻り値 / FLT_MAX 意図 | s60_audio.md §6 | **(a)** | `s81` Task B: B1 AM+0xDC bit2・B2 type bit 0x20・B3 フェード変数・B4 DS3D vt+0x40/44・B5 FLT_MAX = **5/6 [RESOLVED]**。**残 1（GetState 戻り値）→ 束D へ持ち越し** |
| 102 | O4 | 入力 — コンソール grave 実経路 / ゲームパッド物理ボタン / mode 厳密意味 / 8 方向レイアウト +0x1BD5 / 0x9F6000 init / 方向既定値 | s60_input.md §60.7 | **(c)** | v/b 系・s 系ともに §60.7 向け成果物なし（s81 は Streaming/Audio/CRT/I/O のみで input 非含）→ 束D |
| 103 | O5 | CRT/乱数 — sin/cos 変種 / hypot / exp/tan/atan 入口 / sincos / srand 呼出元 3 件 / 価格 ceil/round 使い分け | s60_crt.md §60.6 | **(a)** | `s81` Task C: C1 sin/cos SSE 変種・C4 srand 呼出元・C5 価格 ceil/round [RESOLVED]、C2 hypot・C3 exp/tan/atan [PARTIAL]。**残 2（partial）→ 束D へ持ち越し**。v/b 系は tail CRT 帯（v156/v174）で別軸 |
| 104 | O6 | I/O — zlib inflate 呼出 VA / CRT ラッパ同定 / CSIDL 0x1C / FileFinder Data / Setting 全 9 型 / mod BSA 探索 / ドライブ走査フラグ | s60_io.md §8 | **(a)** | `s81` Task D: D1 FileFinder・D2 CSIDL 0x1C=CSIDL_LOCAL_APPDATA・D3 Setting 9 Types・D4 ドライブ走査 [RESOLVED] = **4/7**。**残 3（zlib inflate VA・CRT ラッパ・mod BSA 探索）→ 束D へ持ち越し**。部分土台(v/b): `v130`/`v160`（INI/Setting/Log 系） |

### P 節（Wave 8 派生タスク、L107–L111）: 5 件

| L | ID | 内容（要約） | 出典 | 判定 | 根拠（成果物・系列） |
|---|---|---|---|---|---|
| 107 | P1 | HavokContext 92 メソッド全抽出・分類（world/dispatch/pendingOps/character/collision）。移植は非 virtual クラスで実装可能 | Wave 8 | **(a)** | **`s69_hc92.md`（2026-10-18）「全 92 メソッド抽出・分類・Jolt マッピング」**＝行と1:1 の成果物（8 分類: step 3/dispatch 2/pending 50/charctrl 3/collision+broadphase 12/debug 6/init 14）。+ `s81_havok_context_pending.md`（50 pending methods）+ `s68_n2_context.md`（92 メソッド呼出元表）。非 virtual 説は s68_n2 と整合 |
| 108 | P2 | 戦闘 AI 呪文スコアリング 0x5E5850 詳細 + 武器スコア 0x48BDA0 統合判断。状態機械 (4/6/7/8/9/0xE/0x10) 全容 | Wave 8 | **(a)** | `s68_n5_func_5E5850_spell_select.txt`（9,295 B・行の対象アドレスを直接解析）+ `s69_cai_weapon_score.txt`/`cai_attack_selector.txt`/`cai_state_handler_0.txt` + `s68_n5_combat_state_machine.txt`（103,755 B）。**残: 呪文/武器スコアの統合判断（設計）→ §5-7** |
| 109 | P3 | 検知→戦闘開始パイプライン全文書化。FOV 定数直書きの移植判断（ハードコード vs パラメータ化） | Wave 8 | **(a)** | `s68_n6_detect.md`（検知式第1次完了）+ `s64_ai_combat_3_detection_core.txt`/`4_detect_state.txt` + `s68_n5_combat_state_machine.txt`。**残: FOV 移植仕様判断 = ユーザー → §5-4** |
| 110 | P4 | PathingSystem 移植実装 — `game/pathing/pathing_system.h/.cpp` 新設（NavMesh リンクリスト+障害物回避+velocity 出力ループ） | 移植実装 | **(b)** | コード新設タスク（L2 有効化/移植セッション待ち）。RE 側土台は s72（束F）が先行 |
| 111 | P5 | Havok 全型の RTTI vtable と instance vtable の二分 + instance vtable 探索手法の確立（CharacterProxy 等） | Wave 8 | **(a)** | **v/b 系で手法確立**: `b07_vtable_index.md`（RTTI vtable 全 1,715 件索引・TD/COL 判定式）+ `b13_nonrtti_vtable_classes.md`（非 RTTI 91 ブロック・ctor の vptr 書込解析手法）+ `b26_nontt_class_names.md`（名前復元の優先順位: レジストリ→__FILE__→RTTI 階層 mdisp→槽関数モニタ名）+ `v156`（vtable 1,715 本/29,063 スロット全走査）。残: b26 R-5（ahk/bk 型関係・小） |

### Q 節（Copilot「Reverse engineering」由来・未統合、L115–L128）: 12 件

> 共通: 統合先 `docs/RE_ESM_REPORT.md` / `docs/RE_ASSET_REPORT.md` は **未開設**（docs 目録実測）。Q 群は「走査済み素材の統合ドキュメント化」が本体で、**新規 ESM 走査は不要**（Q7/Q8 のみ解禁待ち）。relmap2 §4-4 の「Q 群は統合のみで高効率」と整合。

| L | ID | 内容（要約） | 出典 | 判定 | 根拠（成果物・系列） |
|---|---|---|---|---|---|
| 115 | Q1 | ESM 全記録棚卸しの main レポート統合 — content_inventory.txt（1,167,017 件/64 種/0 error）を docs 化 | content_inventory.txt | **(c)** | 統合未了。土台: `copilot_archive/content_inventory.txt` + **v/b**: `v32_esm_efid_report.md`（同じ 1,167,017 件/0 error を V 系で再走査済）。→ 束A |
| 116 | Q2 | WRLD 解析統合 — NAM0/NAM9/OFST/WNAM・84 worldspaces・14,770 cells | wrld_analysis.txt | **(c)** | 統合未了。土台(s): `s69_wrld.md` + `s69_wrld_a1..a16/b1`（16 プローブ）→ 束A |
| 117 | Q3 | LAND/REFR/NAVM — cell class / decompression / REFR / CELL プローブ群の統合 | esm_*.py 群 | **(c)** | 統合未了。土台(s): `s73_land_refr_comp.md` + `s72_navm_*` → 束A |
| 118 | Q4 | SCDA opcode 意味付け（141 opcodes）× script_disasm.cpp 連動 — scda_opcode_table_out.txt | scda_opcode_table_out.txt | **(c)** | **残小**: 表側は v/b 系で大半確定（V77–V84 → `docs/SCRIPT_OPCODE_TABLE.md` 638 行 + §67 MISS480 台帳 + §70.13/70.14）。未了=141 opcode の意味付け docs 統合 + **29 opid が §67.6 未登録（v179 R系）**。ESM 再スキャン禁止遵守 → 束A（低工数） |
| 119 | Q5 | MGEF 全レコード解析 — mgef_probe.py（12 本）の統合 | mgef_probe.py 12 本 | **(c)** | 統合のみ。**実体は v/b 系で確定済**: `v24_mgef_codes` + `v32`（ESM EFID 150 種）+ `v35`（MGEF 145 件/登録 ctor 23 件、KPI 更新 2026-10-23）+ s90/s91 → 束A |
| 120 | Q6 | WATR（水）/天候/GMST 解析 — dump_watr*.py の統合 | dump_watr*.py | **(c)** | 統合未了。土台(s): `s73_watr_dlc.md`。**GMST 側は v/b 系で完了**（v55/v91/v130/v160/v173: 登録 2,304 件・ESM insert 0 件）→ 束A |
| 121 | Q7 | 圧縮レコード (zlib) + 異常 STAT size=0 の解消 | esm_inventory4.txt | **(c)** | **ESM 解禁待ち（relmap §5 G-D）**。土台: `esm_inventory4.txt`（STAT size=0 @7,984,486）+ `v164` §9（zlib/CTDA 走査仕様）→ 束A（解禁サブ項） |
| 122 | Q8 | DLCShiveringIsles.esp parse 失敗の原因調査と修正 | esp_inventory.txt | **(c)** | **DLC 解禁待ち（G-D）**。土台: `esp_inventory.txt` + `v164` §9.3（`*.esp` 10 件・85 B スタブ 1 件を特定済）→ 束A（解禁サブ項） |
| 125 | Q9 | BSA 全形式文書化（v103/v104、圧縮/非圧縮）の統合 | bsa_*.py + textures_bsa_index.txt | **(c)** | 統合未了。土台(s): `s76_bsa_v104.md` + `s76_file_hash_1..9.txt` + `s73_bsa_dds_tel.md`。**hash は docs 側化済**（`docs/BSA_HASH_ALGORITHM.md`）→ 束B |
| 126 | Q10 | NIF 仕様の全文書化（20.0.0.4 / user 2817 / BSVER 11）を main レポートへ | nif_oblivion_layout_spec.md | **(c)** | 統合未了（main レポートへ）。土台(s): `copilot_archive/nif_oblivion_layout_spec.md`（17 実アセット byte 検証済み）+ `s69_nif.md` → 束B |
| 127 | Q11 | DDS 形式（DXT1/3/5 等）のデコード確認 — dds_decode_probe.py | dds_decode_probe.py | **(c)** | 統合未了。土台(s): `s73_bsa_dds_tel.md` + `s76`（拡張テーブル 0xB25380: NIF/KF/DDS/WAV）→ 束B |
| 128 | Q12 | ドア/テレポート国勢調査の統合 — x_tel_census.py (+exe) | x_tel_census.py | **(c)** | 統合未了。土台(s): `s69_wrld_a6/a8/a15/a16`（teleport entry/extended/xtel）+ `s77_ft_1..3` + `s77_oth_1_fasttravel.py` → 束B |

### R 節（未解析領域、L143–L146）: 4 件

| L | ID | 内容（要約） | 出典 | 判定 | 根拠（成果物・系列） |
|---|---|---|---|---|---|
| 143 | R2 | アニメーションシステム — NiControllerManager / kf ロード・再生 / ブレンド (idle↔walk↔run↔attack) / 武器アニメ↔HitFrame / モーション状態遷移 | 未解析領域 | **(c)** | 土台(s): `s75_anim.md`（B1 NiControllerManager vt 0xA79804・B2 NiControllerSequence・B3 KF 参照 site・B4 状態名表 21 種 0xB10290・B5 武器アニメ dispatch・B6 vt+0x398=0x5EE760/vt+0x3AC=0x5FEBF0 HitFrame・B7 blend interpolator・B8 GMST）→ **自己残件 C1-C4**（Activate 全読・update ループ・ブレンド重み・sneak/swim dispatch）が未着手。ロードマップ Phase 4A-3 (KF/KFM) と接続 → 束G |
| 144 | R3 | シェーダ 48 変種確定 — 224 種中 176 種一致、残 48 の差分同定 + 移植シェーダ対応 | 未解析領域 | **(a)** | **`s75_shader.md` §2.4「Confirmed Missing Variants: 38+10=48 [Confirmed]」+ §4.1/§5（48 = FaceGen×Glow×Alpha×Fog×VertexColors 組合せ、原作 EXE にも未実装＝OG で SHADER ERROR）** → 差分同定は s 系で完了。残: 移植フォールバック方針（KPI「176/224→224/224」の再定義要否）= ユーザー → §5-2 |
| 145 | R4 | 字幕/文字列システム — .STRT ロード / 言語テーブル / フォント / 日本語未対応確認 | 未解析領域 | **(a)** | **`s76_strings.md` が 4 項目すべて解答**: .STRT/DLF/ILG **不在確定**（Fallout 3 以降の方式で Oblivion に無関係）・言語選択不在（英単一言語固定）・フォント 5 種+FontManager RTTI 階層・字幕=HUDSubtitleMenu/hud_subtitle_menu.xml・日本語非対応確定。残: s76 残件 R1-R3（小）+ `docs/RE_FONT_REPORT.md` 参照 |
| 146 | R5 | BSA v104 圧縮形式の細部 — 36B ヘッダ以降の folder/file chain 書式 | 未解析領域 | **(a)** | `s76_bsa_v104.md`（v103→v104 デルタ・**EXE は v104 を拒否 [Def]**＝新事実）+ `s76_bsa_header2.txt` + `s76_file_hash_analysis.md`（folder/file chain・hash 全 9 ステップ）+ `docs/BSA_HASH_ALGORITHM.md` |

### W3 節（T 節 Phase 4 — Wave 3 残深掘り、L156–L162）: 7 件

> 共通: 正本 `TODO_magic.md`「Wave 3」節も **全 7 件が [ ] のまま**（L205-211 実測）。ただし TODO_re49 の **G2 行（[~]）に 2026-10-21 デコンパイル確定として W3-4/6/7 の答案が記録済み**。

| L | ID | 内容（要約） | 出典 | 判定 | 根拠（成果物・系列） |
|---|---|---|---|---|---|
| 156 | W3-1 | Alchemy crafting (INGR→ALCH) — AlchemyMenu=0x410 特定済み、メニューハンドラ探索 | TODO_magic Wave 3 | **(c)** | 未着手。土台: H5（AlchemyMenu=0x410 [確定]）+ `s96_magic_m13_alchemy_poison.md`。ロードマップ 1-2 残件「Alchemy/Enchanting crafting」と一致 → 束H |
| 157 | W3-2 | Enchanting creation — EnchantMenu ID 特定 → ハンドラ探索 | TODO_magic Wave 3 | **(c)** | 未着手。土台: H5（EnchantmentMenu=0x412 [確定]）+ `s77_oth_4_enchant.py`（小）→ 束H |
| 158 | W3-3 | SEFF スクリプト VM 実行経路 — 0x6A83D0 以降 | TODO_magic Wave 3 | **(c)** | 未着手（本項）。**土台(v/b): `v17_scripteffect_noop_report.md` §5.4**（opcode 側に SEFF 処理なし＝call 0 件、実行系は MGEF SEFF 側: SEFF 判定 0x6A2879 / dispatcher 0x6A2CF0 / apply 0x6A27F0 / tick 0x6A86F0 [確定]）+ `s98`（0x6A83D0 = SEFF tick と特定、ただし残件として再掲）。relmap §3-① も「v17/v172 系で短縮可能」と指摘 → 束H |
| 159 | W3-4 | MGEF+0x68 ESM ローダ — ESM パーサ内 MGEF レコード書き込み箇所 | TODO_magic Wave 3 | **(a)** | **TODO G2 行 2026-10-21 デコンパイル確定**: 「MGEF+0x68 = DATA ブロック+0x10（0x416100 MGEF_Reader で確定）」 |
| 160 | W3-5 | Poison stack/期限 — 0x69BA90 以降の重ね合わせ・消滅条件 | TODO_magic Wave 3 | **(c)** | 未着手（本項）。土台: `TODO_magic` L190「Poison on-strike: 0x69BA90 効果適用/0x69AC50 ActiveEffect 生成 [確定]」+ `s96` + M6 ActiveEffect update（0x68E670: timer 積算→expire vt+0x3C）。※G2 行の「W3-5 毒」注記は面範囲ターゲット収集（0x69AC50）で **stack/期限とは別** → 束H |
| 161 | W3-6 | Absorb Magicka 実装 — 回復量・上限 | TODO_magic Wave 3 | **(a)** | **TODO G2 行 2026-10-21 デコンパイル確定**: 「吸収 Magicka 回復 = max(1, round(コスト式 0x548B50))、skill 分岐は死コード」 |
| 162 | W3-7 | Reflect→Absorb 相互作用 — 排他 or 連鎖 | TODO_magic Wave 3 | **(a)** | **TODO G2 行 2026-10-21 デコンパイル確定**: 「reflect 成功時 absorb 不走（相互排他）」+ W3-7 は I2 行でも「W3-7: reflect 成功時 absorb 不走 (相互排他)」と記録 |

---

## 3. 束化起票案（(c) 25 件 → 8 束）【提案のみ・起票は親セッションが serial を割り当てる】

> 暫定成果物名は v181〜v188 の連番を想定（親セッションが serial 169+ に割当）。工数は relmap2 §3-①「起票束化 0.5 人日 + 各束 1–2 人日」に準拠して見積もる。優先度は移植直結度とロードマップ DoD から判定。

| 束 | 暫定名（成果物名案） | 対象 (c) 行 | (a) 行からの残点持ち越し | 優先度 | 推定工数 | 出典/土台 |
|---|---|---|---|---|---|---|
| **束A** | **ESM 統合ドキュメント化束** `v181_esm_integration.md` | Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8（8 件） | — | **中**（relmap §4-4「高効率」） | 1.5–2 人日（0.5×8 + 統合） | copilot_archive 群 + v32/v35/v164/v176 + s69_wrld/s73/s72。**Q7/Q8 のみ ESM/DLC 解禁（G-D）前提**。新規走査なし・ESM 再スキャン禁止遵守 |
| **束B** | **アセット形式統合束** `v182_asset_integration.md` | Q9, Q10, Q11, Q12（4 件） | — | 中 | 1–1.5 人日 | s76_bsa/nif/dds + copilot nif spec + s77_ft/s69_wrld_xtel。統合先 `docs/RE_ASSET_REPORT.md`（未開設） |
| **束C** | **レンダラ/ストリーミング残件束** `v183_renderer_stream_residuals.md` | O1（1 件） | **O2 残 3**: NiNode ツリー NIF 組立経路・メモリプール拡張・LOD blend 経路詳細 | **高**（移植 renderer/streaming の根拠・relmap 優先順 5） | 1.5 人日 | s61_render/stream + v129/v159/v173（シェーダハンドルテーブル・DeviceWrapper 等で短縮）+ s81 Task A |
| **束D** | **音響・入力・CRT・I/O 少数残件束** `v184_engine_small_residuals.md` | F2, O4（2 件） | **O3 残 1**: GetState 戻り値 / **O5 残 2**: hypot・exp/tan/atan [PARTIAL] / **O6 残 3**: zlib inflate VA・CRT ラッパ・mod BSA 探索 | 低（relmap 優先順 11「実装時に随時」） | 1.5 人日 | s60_audio/input/crt/io §6・§8 + s81 Task B/C/D + s56_audio |
| **束E** | **§56 ゲームプレイ残件束** `v185_gameplay_residuals56.md` | F3, F4, F5（3 件 = 20 項目） | —（F5 内「同一再オープン実機確認」1 項は (b) G8/H7 へ分離済み） | 中（Phase 1 §56 残件） | 1.5–2 人日 | s56_sleep/container/menu + s58 系 |
| **束F** | **Havok RayCast/Pathing 継続束** `v186_havok_ray_pathing.md` | N4, N7（2 件） | — | 中 | 1–1.5 人日 | s63_h2/s65_havok_char/s61_havok（RayCast・前提矛盾 0x8A2650 を裁定）+ s72_navm（steering/障害物回避・Phase 3-3 連携） |
| **束G** | **アニメーション残件束** `v187_anim_residuals.md` | R2（1 件） | — | 中（Phase 4A-3 KF/KFM・KPI「BSA/NIF/KF 形式」） | 1 人日 | s75_anim 残件 C1-C4（Activate 全読・update ループ・ブレンド重み・sneak/swim dispatch）+ s75_anim_7_blend 群 |
| **束H** | **魔法 Wave3 残束** `v188_magic_wave3_residuals.md` | W3-1, W3-2, W3-3, W3-5（4 件） | — | 中（Phase 1 1-2 残件と 1:1・relmap 優先順 1 対） | 1–1.5 人日 | H5（menu ID）+ v17 §5.4（SEFF 短縮）+ s96/s98/s77_sm + TODO_magic Wave 3 正本 |

- **合計**: (c) 25 行 = 8+4+1+2+3+2+1+4 ✓ / 工数合計 **9.5–13 人日**（+起票 0.5 人日は親セッション）。
- **(b) 9 件は束化しない**: F7/F8/G6/I7/J6（移植差分・実装判断）+ G8/H7/I6（実機・.ess）+ P4（L2 実装）。既存の relmap §5 ゲート（G-D/G-F）と優先順 12 に集約。
- **(a) 21 件は束化しない**: 行の [x] 化（親セッション）のみ。ただし残点（O2/O3/O5/O6 由来 9 項目、N3/P5 由来 b26 R-5、R4 由来 s76 R1-R3）は上記束へ明示的に持ち越し済み。

---

## 4. ロードマップ接続（docs/RE_ANALYSIS_ROADMAP.md — 読取のみ）

### 4.1 束 ↔ Phase / Wave / KPI 対応

| 束 | ロードマップ節 | Wave/Phase | KPI 行との接続 |
|---|---|---|---|
| 束A | §7 Phase 3（3-1〜3-12、全 [ ]） | Phase 3 ESM/DLC 全面解析 | **「ESM レコードタイプ: 部分 (数種) → 全 ~50 種」** の主推進。成果物 `docs/RE_ESM_REPORT.md`（未開設） |
| 束B | §8 Phase 4（4A-1 BSA / 4A-2・4A-11 NIF / 4A-4 DDS）+ 3 節 Q12 | Phase 4 アセット形式 | **「BSA / NIF / KF 形式: 部分 → 全形式」**。成果物 `docs/RE_ASSET_REPORT.md`（未開設） |
| 束C | §6 Phase 2 2-1・2-3（[~] 残 6/6） | Wave 6b §61 の残件層 | DoD「移植に影響する差異を一覧化」の残り。J5（Wave 6b 本体）は (a) 完了済み |
| 束D | §6 Phase 2 2-4〜2-7（[~] 残 7/6/6/7）+ F2=Phase 1 域 | Wave 6a §60 の残件層 | DoD 同上。relmap 優先順 11 と一致 |
| 束E | §5 Phase 1（1-7 セーブ/1-9 経済/1-10 クラフトの周辺） | Wave 3 §56 残件 | Phase 1 DoD「実装対応先と検証方法の併記」の残り |
| 束F | §6 Phase 2 2-2（[~] 残 8）+ §7 Phase 3-3 LAND/NAVM（[ ]） | Wave 7 → Phase 3 連携 | TODO M7 行「NAVM 詳細は Phase 3 継続」と N7 行の「Phase 3 連携」明記を回収 |
| 束G | §8 Phase 4 4A-3 KF/KFM（[ ]） | アセット形式（アニメ） | **「BSA / NIF / KF 形式」** の KF 側。S 節「移植必須・未解析: アニメーションシステム (R2)」と同一 |
| 束H | §5 Phase 1 1-2 魔法（[~] 残件明記: Alchemy/Enchanting crafting・SEFF VM・MGEF+0x68） | Wave 9 → Wave 3 深掘り | **MGEF+0x68 は (a) W3-4 で解消済 → ロードマップ 1-2 の残件リスト自体が 1 項目縮小**（親セッションの次回 KPI 更新候補） |

### 4.2 判定がロードマップ KPI に影響する点（読取からの所見）

- **KPI「シェーダ変種 176/224 → 224/224」**: R3 判定 (a) により、残 48 は**原作 EXE にも未実装**（OG でも SHADER ERROR）と `s75_shader.md` で同定済み → 「224/224」は到達不能であり、**KPI の再定義（例: 176 実装 + 48 フォールバック方針）が人間判断を要する**（§5-2）。
- **KPI「ESM レコードタイプ 全 ~50 種」**: 束A が唯一の推進ルート。Q1/Q5 の実体は v32/v35 で既に計測済みのため、**統合のみで KPI が前進する**（relmap §4-4 の効率予測と一致）。
- **KPI「Script VM ハンドラ 480/480」**: Q4 の残 29 opid（v179 R系）が未登録として残存 → 束A の Q4 サブ項が回収。
- **Phase 0〜2 のチェックボックス**: J5（Wave 6b）は (a) 完了済みだがロードマップ側は 2-1〜2-3 が [~] のまま（残件層 = 束C/D が消化すれば前進）。letter-row 55 件はロードマップ §4〜§11 のどのチェックボックスにも直接紐付いておらず、**接続は Phase 1（F/G/H/I/W3）・Phase 2（J/N4/O/P1,P4,P5）・Phase 3（Q/N7）・Phase 4（Q9-12/R2/R3/R5）の 4 フェーズに分散**する（§4.1 表が対応表）。

---

## 5. 残る人間判断（親セッション/ユーザー）

| # | 対象 | 判断内容 | 判断者 |
|---|---|---|---|
| 1 | **N2 の矛盾裁定** | `s68_n2_context.md`「HavokContext は vtable を持たない」（静的証拠: 書込 0/仮想呼出 0/92 メソッド直接呼出）と、`v_data_relmap2.md` §5 G-B・handover の「B13 実機 .data ダンプ待ち」が**矛盾**。B13 を実行して裏取りするか、静的結論で N2 をクローズするか（B13 は手順書 `s50_b13_plan.md` 済み・ユーザー操作のみ） | ユーザー + 親セッション |
| 2 | **R3/KPI 再定義** | 残 48 シェーダ変種は**原作 EXE にも未実装**（`s75_shader.md`: OG でも SHADER ERROR）。ロードマップ KPI「176/224 → 224/224」を再定義するか、移植フォールバック（親へ近い変種へ）方針を決めるか | ユーザー |
| 3 | **G7/H6 の docs 反映** | 事実面は確定済み（0x4FDE30=引数コンパイラ・表 0xB0AF48 532×0x28・HighProcess 0xA71814・AlchemyMenu=0x410・条件関数帯 169 件・§18.6）。docs/GT への反映を **V168 系の要承認バッチへ追加**するか | 親セッション（承認制） |
| 4 | **移植差分・L2 実装群（(b) 6 件）** | F7（難易度式適用）・F8（stackingtype 0 扱い）・G6（combat_manager 固定値の根拠確認と扱い）・I7（KD 到達性/RNG/難易度式）・J6（同差分一括）・P4（pathing 新設）＋ P3 の FOV 定数仕様（ハードコード vs パラメータ化）＋ P2 の呪文/武器スコア統合判断 — **いずれも L2 有効化とユーザーの適用方針待ち**（relmap 優先順 12） | ユーザー |
| 5 | **Q7/Q8 の ESM/DLC 解禁** | zlib 圧縮レコード/STAT size=0（Q7）と DLCShiveringIsles.esp parse 失敗（Q8）は**実ファイル走査が必要** → relmap §5 G-D（serial 163 の仕様確定後、v164 walker 実行）に合流。束A の該当 2 項は解禁サブ項として保留可 | ユーザー承認 → サブエージェント |
| 6 | **実機確認系（(b) 4 件）** | G8/H7（ContainerMenu 再オープン・難易度式移植修正）・I6（.ess サンプル取得と突合）・F5 内の実機確認 1 項 — relmap §5 **G-F**（ユーザーのみ）と同一 | ユーザー |
| 7 | **束化起票の受諾** | §3 の 8 束（暫定 v181–v188）の serial 割当・優先順・(a) 21 件の行 [x] 化の要否。**本ファイルは提案のみで TODO への起票は行っていない** | 親セッション |

---

## 6. 既存前提への修正提案（読取のみ・編集していない）

1. **`v_data_relmap2.md` §3-①「55 件は一度も触れられていない」の精緻化**: 行状態（[ ] のまま）としては正確だが、**内容面では 21 件（38%）が既存成果物で解決/包含**済み。特に `s81_stream_audio_crt_io.md`（2026-10-18）が **O2/O3/O5/O6 の 4 行を直接消化**しており、「O 群は解析本体（未着手）」（§4-4 の束骨子）は O1/O4 のみが対象。次版 relmap で §3-① の中身の分布を「(a) 21 / (b) 9 / (c) 25」へ更新する提案。
2. **N2 のゲート記載**: relmap §5 G-B の「HavokContext 実 vtable（N2 = B13 実機ダンプ）」は `s68_n2_context.md` の静的結論と矛盾。§5-1 の裁定結果に応じて G-B から N2 を外す（または「B13 は裏取りのみ」と注記）提案。
3. **ロードマップ 1-2 の残件リスト**: 「残: Alchemy/Enchanting crafting、SEFF VM、MGEF+0x68 ESM 書込元」のうち **MGEF+0x68 は W3-4 で確定済み**（TODO G2 行 2026-10-21 記録）→ 次回 KPI 更新で削除する提案。
4. **Q4 の位置づけ**: 「opcode 名の意味付けのみ未了」（TODO L131）は V77–V84/`docs/SCRIPT_OPCODE_TABLE.md`/§67 で**表側は大半確定**済み。残は scda 141 opcode の docs 統合と 29 opid 未登録（v179 R系）のみ → 束A 内で最小工数化。

---

## 7. 検証・遵守事項

- **抽出検証**: `%TEMP%\opencode\v180_extract.py` の正規抽出 = 箇条書き合計 83 件 / 未チェック **55 件**（L33 F2 〜 L162 W3-7）。ID 列合計 = F6+G3+H2+I2+J2+N6+O6+P5+Q12+R4+W3-7 = **55** ✓（本文 §2 の各節合計と一致）。
- **判定カウント**: (a) 21（G7/H6/J5/N2/N3/N5/N6/P1/P2/P3/P5/O2/O3/O5/O6/R3/R4/R5/W3-4/W3-6/W3-7）+ (b) 9（F7/F8/G6/G8/H7/I6/I7/J6/P4）+ (c) 25（F2-F5/N4/N7/O1/O4/Q1-Q12/R2/W3-1/W3-2/W3-3/W3-5）= **55** ✓。
- **遵守**: git 不使用 / docs・TODO_re49・GT・session-memory・plan・台帳の**編集 0 件** / 既存 tmp/re 上書き 0 件（本ファイル v180 のみ新規）/ ESM・ESP 本体読み取り 0 回（再スキャン禁止遵守）/ Ghidra 呼出 0 回（Q 群判定は既存成果物のみ）。中間物は `%TEMP%\opencode\v180_extract.py`（+抽出結果 txt）のみ。
- **本監査は L1 report-only（分析専任）**: 起票・行状態更新・docs 反映はすべて親セッション/ユーザーの判断事項（§5/§3）。

（了 — v180_letter_rows_audit.md / 55 件全数判定・8 束起票案・ロードマップ接続・人間判断 7 件）
