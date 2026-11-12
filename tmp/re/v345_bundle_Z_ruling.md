# v345 — 次束Z 狭い版（#118）: v341 §5 待記の別レーン裁定分 3 fn 正式名候補の裁定

- 作成: 2026-11-09 / L1 report-only / 静的のみ（親直接解析レーン）
- 出典: #118（row320 親突合で起票→**BG の #116 scope 調整注により減らスコープ化**：当初① 59名登録順=**#116⑧**・当初② push 側 0x6765F0/0x678D30=**#116⑦** へ統合、随伴の 0xB13970 宣言元 init（0x587xxx）は #116⑧ の XML code 域と共有のため完了後まで延期）。本束残=③ 0x4E4A10/0x42BB90/0x65C620 の正式名候補（v341 §5「帯外浅層＋RTTI テンプレ引数対応の別レーン裁定」の受け皿）
- 対象 EXE: Oblivion Steam v1.2.0416（ImageBase 0x400000）・原本類は無変更（読取のみ）
- probe: `v118_1.py/.out`（callers 境界検証 census＋this 直接解決＝低収量 2-5 サイト）・`v118_2.py/.out`（(a) NiTArray TD 瞬時Instantiate 52／(b) 0x4040E0 callee／(c) DTOR thunk 域 callee multiset／(d) 0xA2490D-0xA24950 全文／(e) 本体帯 217 サイト引数形）— 全 EXIT=0・`%TEMP%\opencode\`
- 教訓適用: callers は v117_9 方式の**命令境界検証**付き（C-7 教訓）・v118_1 の vptr ゴミ表示（BSS/非 vptr の [abs] 読）は手法注記（v118_2 が正典）
- 判定語: [確定]/[部分確定]/[推定]/[未確定]/[確定負]

## 1. 裁定表

| 対象 | 裁定 | 要旨・根拠 |
|---|---|---|
| **0x42BB90** | 役割名 **[部分確定]**／正式 C++ シンボル **[未確定→待記宣言クローズ]** | **NiTArray<T> family の汎用 insert/remove 系メソッド（非仮想）**。zone imm32 ref=0（非仮想・v118_1）／E8 callers=992（**境界検証 992/992・中途偽陽性 0**）／引数形 (this, idx, ptr-to-item)・`ret 8`／grow 分岐 `mov word[ecx+0xA], idx+1`@0x42BB9C-9F・上書き分岐 +0xC±1・null-item 分岐は +0xA 不変（v341 §3.3 复唱一致）。this=グローバル直接解決 2 サイト**すべて** `.?AV?$NiTArray@PAVTile@@@@`（0xB13970 系）＋画像内 **NiTArray<T> TD 実体化 52 種**（BSHash/PBD/NiPointer 群/TESObjectREFR*/Tile*/StringListElement@Tile/NiTSet*/FurnitureMark 他・v118_2(a)）＝+0xA(u16)/+0xC レイアウト共通のテンプレ汎用コード帰属。シンボル確定には外部シンボル/PDB 相当＝静的到達外（v341 と同型・宣言で最終クローズ） |
| **0x4E4A10** | 同上 **[部分確定]/[未確定→待記]** | **NiTArray<T> family の容量/サイズ設定メソッド（非仮想）**。zone ref=0／callers=1043（境界検証 1043/1043）／引数 (this, n)・0x4E4A62 `mov word[esi+0xA], arg`。named this 5 サイト（NiTArray<Tile*>×2・NiTArray<StringListElement@Tile>×1・他 2 は [abs] が非 vptr データ読みのため不問）＋ v341 唯一書手（0x587B74/92 push 0x33）の NiTArray<Tile*> 帰属と整合 |
| **0x65C620** | 役割名 **[部分確定]**／正式シンボル **[未確定→待記]** | **単方向リスト除去 by value＋free（0x401F20）の汎用登録解除/dtor ヘルパ（非仮想）**。zone ref=0／callers 852（生 854−中途 2・v341 の 865=E8 854+E9 11 を含む全数内訳と矛盾なし）。**atexit/DTOR thunk 域 0xA1643A..0xA2712A から 636 サイト**（v341 の ≈655 推定を実測確定に更新・0x401F20 は同域 650）／本体帯 217 サイトは `lea ecx,[mgr+N]; push ptr` 形（N=+0x4..+0x14C の組込み list head・例 0x449DFF [esi+0x84]/0x449EB0 [esi+0x5c]／0x424B2F の `lea esi,[edi+0xc]` 系）＝**マネージャ型群の組込みサブ object を this とする登録解除**。v341 §3.4 の除去/解放セマンティクス [確定] を継承（差替なし・v337「追加 helper」語の訂正状態維持） |
| （付随）0x4040E0 | 一致复唱 **[確定（命令）]** | 0x9FBA90 窓の adder は **NiTArray ではない単方向リスト追加**（head `cmp [ecx+0x110],0`→`lea edi,[ecx+0x10C]`・callees=0x6BB260/0x4A7A60/0x446CB0/0x98262D の 4・**0x42BB90/0x4E4A10 を呼ばない**）＝v341 §3.3 の「this+0x10C の list 追加」記述と矛盾なし（ノード割当候補 0x6BB260 の identity は今回未追及＝§4） |

**集計**: 主 3 件 [部分確定]（役割名レベル）＋正式シンボルは 3 件とも**待記宣言クローズ**（静的経路=RTTI テンプレ引数対応まで到達・外部シンボル/PDB 相当が唯一の残証）。付随 1 件 [確定（命令）]。[確定負]1（「0x42BB90/0x4E4A10/0x65C620 はいずれも仮想メソッド」説=zone ref 0 件×3。教訓: 0 件 census は**不在証拠として使用可の形**—命令境界検証済みの直接 imm 走査であるため）。

## 2. 帯遵守の自己宣言

深: 0x42BB90/0x4E4A10/0x65C620 の head＋分岐窓（v341 で浅層照会済みの再視測中心）・0x4040E0 head 10 命令・0xA2490D-0xA24950（v341 済みの复唱）。機械: callers 境界検証 census（v117_9 方式）・TD prefix 走査（全セクション）。**非追跡宣言**: 0x6765F0/0x678D30（#116⑦ BG）・XML code 域 0x586C6A..0x5DD544 本体（#116⑧ BG・(e) の引数窓は命令文脈のみで登録順調査はせず）・0x587xxx init 本体（延期ゲート）・0xB36530/0xB3BD00（#82/B13）。#116（active）と本束成果物のアドレス集合は非交差。

## 3. 訂正節

- なし（v341/v337/v331 の既存裁定を差替えない。0x65C620 の「636 thunk サイト」実測は v341 の「≈655 件（stride 0x30 域）」の**精緻化**であり訂正ではない）
- 手法注記: v118_1 の this=直接解決は「`mov ecx,[abs]`/`mov ecx,imm`」のみで、レジスタ連鎖（this=this+off・ローカル）を拾えず収量 2-5/2900。**レイアウト+RTTI 対応＋thunk 域 multiset（v118_2 方式）が正**。

## 4. 新規残件

1. [部分確定→残] 0x65C620 本体帯 217 サイトの**マネージャ型帰属**（`lea ecx,[esi+0x4..0x14C]` 群の owner fn 0x449D??/0x424B?? 等の head 同定とクラス名 RTTI — 帯 0x449xxx 未割当・機械で到達可）。
2. [未確定] 0x6BB260（0x4040E0 のノード割当候補）/0x446CB0 の identity（付随・浅層）。
3. （外部依存・追記なし）正式シンボル 3 件の最終確定=外部シンボル/PDB 相当=#82 系ゲート（待記宣言クローズとして確定済み）。

→ 1-2 は **#119 へ N4/N5 として加刷**（既存 N1-N3 と同格の静的残件）。

## 5. クロージング状態

- ai-todo: #118 note→complete・#119 description 改訂（N4/N5 加刷）。#116（BG・active）へは非干渉。
- 台帳 row322（V322）統合・bak172・round_verify 合格。STATE (ba)。memory 712 件登録・validate 0 error。supermemory は書込時に再度 tool 不通（復帰時に滞留分として追登録）。
- **レビュー-round（クロージング後実施）**: ①callers 数値の再突合=992/992・1043/1043・852(=854−中途2) すべて v118_1.out と一致 ✓ ②「TD 実体化 52 種」は distinct 検証（file-wide 走査で hits=52/distinct=52）✓ ③指摘 2 件を精緻化：0x4E4A10 の named-5 のうち 2 サイトは [abs] が非 vptr データ読み（v118_1 の "vptr 0xBFCEC923" 類＝BSS ではなくデータ領域誤読）→本文/台帳 row322 を「NiTArray 3＋非 vptr 2 不問」に修正済 ④STATE (ba) の "992/999" タイポ修正済。訂正節に上げる性質の誤りはなし（③は精度向上・旧文言は置換而非削除方針上ここに記録）。
