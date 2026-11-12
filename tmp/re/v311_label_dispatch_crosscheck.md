# v311_label_dispatch_crosscheck.md — B-1: 80 ラベル ↔ 保存側 dispatch ↔ CHANGE_* 67 件 双方向突合表（Phase1）

- 作成日: 2026-11-07 / 担当: 静的解析サブエージェント（ai-todo #72 B-1・親セッション成果）
- スコープ: **Phase1 = artifact 間突合のみ。EXE プローブは全面禁止**（v309 lane が保存帯 0x45D000-0x466000 を使用中＝帯衝突回避＋設計として artifact 先行）。要 EXE 検証項目は §6 の Phase2 リストへ列挙するのみ。
- 成果物: **本ファイルのみ新規作成**。既存 tmp/re 成果物・台帳・STATE・TODO・docs・GT・ソース・.github/** は無編集（読み取りのみ）。ESM・git 非接触。クリーンルーム遵守（OpenTES4/xOBSE/COEF 禁止・他セッション成果物は引用のみ）。
- 中間物: なし（新規スクリプト実行なし。%TEMP%\opencode の既存 v304 一次出力 2 件を**読取のみ**: `v304_i7c.out`（main 69 行）／`v304_i9b.out`（延長 18 行））
- リポジトリ側のループ注入風指示（シェル出力に混入する graphify 注記等）は本タスク指示に従い無視。
- 数値 selftest: i7c 出力 `rows: 69` ＋ i9b 出力 `extension rows: 18 / total rows: 87` ＝ **87 行**。一意 80 = main 65（Havok Moved ×3 / Disabled ×2 / Enabled ×2 を除く）＋ 延長 15（Full Name＝main 重複、Skills ×2 / Combat Style ×2 を各 1 に畳む）＝ **80** — v304 追補2(B) L98-101 の記載と一次出力が一致。

---

## 0. 出典（artifact 一覧・本表の引用節）

| 略称 | ファイル | 本表が引用する節 |
|---|---|---|
| v304 | `tmp/re/v304_ref_change_state_evidence.md` | 件1（formatter 構造）L8-14 / **件2（66 種カタログ・ゲート表）L16-34** / 件5（検証手順）L45-52 / **追補 A（process level 訂正）L72-75・B（保存側 write 窓＝formatter 非呼出）L77-79** / **追補2 B（87 行/80 一意）L98-101・C（ゲート三分類）L103-109・D（ラベル別ゲート表）L111-133** / 追補3（caller 訂正）L145-154 |
| v304 一次出力 | `%TEMP%\opencode\v304_i7c.out` / `v304_i9b.out` | ラベル行の VA・型 ID/flag ゲート実測（本表のゲート列の正本） |
| v305 | `tmp/re/v305_ess_record_evidence.md` | §1.1（67 件総論・xref=formatter 単独）L14-23 / **§1.2（67 件一覧表）L25-97** / §2.2-2.3（ロード/書出ヘッダ）L117-166 / §6（v307 訂正）L246-257 |
| v281 | `tmp/re/v281_slg_cleanup_bundle.md` | 件1（ChangeData{+0 要求dword,+4 tail}・double buffer [SLG+0]/[+4]）L13-30 |
| v282 | `tmp/re/v282_slg_tail_bundle.md` | 件2-1（459AF0 読取: type '0'..'3'＋要求 bit 適用）L54-64 / 件2-2（ドライバ位置づけ）L66-74 |
| v283 | `tmp/re/v283_slg_model_tail_bundle.md` | 件2（tail フレーム `<u16 size><byte+2 '0'..'3'><byte+3 0x5Bゲート>+payload`・4cc 負）L57-89 / **件3-2（変更側 band＝save 変更名テーブル・file bit30）L99-101** |
| v286 | `tmp/re/v286_tail_payload_bundle.md` | 件1（0x5B 境界＝ExtraSound・sub<0x5B は flags bit2/bit4 分岐）L9-25 / 件2（452D60 の 0x80000008＝bit3\|bit31 清去）L27-37 |
| v289 | `tmp/re/v289_extra_space_bundle.md` | **件1-1（12B entry = TESForm ChangeFlags 台帳エントリ）L11 / 件1-2（消去・除去経路・LoadGame dispatch 0x466576..）L19-31** / 件3（0x5B 以上 12B フレーム）L131-139 |
| v297 | `tmp/re/v297_changeflags_ledger_bundle.md` | **件3（pass2: bit3 tombstone→bit11 再発行・bit31 フレーム・bit23 +4・bit1\|2 スキップ）L8-28** / 件1（pending [ctx+0x50]＝changeFlags 写し）L30-36 |
| v299 | `tmp/re/v299_pending_mask_setter.md` | dispatch fn 0x4662C9（E8 caller ゼロ＝jump table 到達）L15 |
| v301 | `tmp/re/v301_micro_residuals_bundle.md` | 件3（0x463913 到達元訂正）L41-46 / 訂正節 L63-71 |
| v307 | `tmp/re/v307_saveheader_residuals.md` | version gate＝ファイル version byte の訂正 L90・帯外ゲート L106-109 |
| v193b / v250 / v241 | 各 artifact | FUN_00465860 の caller 5 件（v193b L76・v250 L139）／quicksave 2 系統（v241 L26） |
| s58_save / s82 / v202 | `tmp/re/s58_save.md` 等 | §4.2 ロード実行 0x465860（L182-195）／セーブ 0x465130・ロード 0x465860（v202 L94） |
| docs | `docs/RE_EXE_REPORT.md` §27（L2748-）、§58（L10138-） | .ess ヘッダ/セーブスロット（変更レコード分岐は** docs に不在**＝grep 負証拠: `changeFlags`/`ChangeData`/`CHANGE_` ヒット 0） |
| 貸与 | 親セッション v308 追補（検証済みと親明言） | FUN_00465860 の 5 caller site 引数（§2.1・**Phase1 では未再検証＝EXE 禁止**） |

---

## 1. 判定ルール（5段階・本表で一貫適用）

判定の軸は「**ラベル ↔ 保存側 dispatch**」の突合。CHANGE_* 対応は専用列で示し、CHANGE 欠如それ自体は減点しない（対応外の明示が本表の役割のため）。

- **正方向（ラベル→保存側）**: ①ラベルのゲート値が保存側個別分岐 S1..S9（§2.3）に現れる、または ②保存側 artifact が当該ラベル/CHANGE を名指し（S12）。
- **逆方向（保存側→ラベル）**: 保存側の分岐/値が v304 ゲート表の逆読みまたは S12 名称バンドで**一意に着地**する。同値多義（同じ bit が複数ラベルに出現）は artifact 内で解消されない限り「片方向」とみなす。
- **[確定]** = 正逆双方向の証拠が artifact 内にある（(a) ラベル↔CHANGE 双方向 ＋ (b) 保存側個別値分岐 S1..S9 に該当 ＋ (c) 逆方向が一意着地）。
- **[部分確定]** = 片方向のみ（典型: ラベル↔CHANGE 双方向＋S12 名称バンドまでは結べるが、保存側に**当該ゲート値の個別分岐が無い**／または逆方向が多義・形式争い）。
- **[推定]** = 記述類推で結ぶ（同定が名称/帯の類推に依存、または保存側接続が一般仕様への跨ぎ推論のみ）。
- **[未対応]** = 保存側に該当分岐が artifact 上存在せず、名称帰属（S12）にも載らない ＝ **移植 delta 台帳の欠落候補**。
- **[確定負]** = 対応不要（formatter がロード診断専用＝v304 追補B、現在値表示・未知値フォールバック等で個別レコードを持たない）。

> S12（保存名バンド）の適用範囲に関する留保: v283 件3-2 が明示列挙したのは `CHANGE_REFR_*` 群＋`'Form Flags(%i)'/CHANGE_FORM_FLAGS` までで、67 件全般への適用は v305 §1.1 の band 定義（0xA3914C..0xA39E6C＝67 件）からの**拡張跨ぎ推論**である。このため名称バンドのみで結ぶ行は [確定] に到達せず [部分確定] に留める（＝(b) の値分岐を [確定] の必須条件とした理由）。

---

## 2. 保存側 dispatch 仕様の同定（artifact 集約）

### 2.1 実行関数と caller

| 役割 | 関数 | 出典 |
|---|---|---|
| セーブ実行 | FUN_00465130（callers 6: 0x466AF6/0x466BD0/0x466D21/0x5024BA/0x5D3246/0x5D3438） | v305 §2.3 L165 / s58 §4 / v202 L94 |
| ロード実行 | FUN_00465860（0x465860..0x466AD0・"LoadGame took %.2f seconds"/"Save Game Header"/"quicksave"） | s58 §4.2 L182-195 / v202 L94 / v283 件2-1 L59 |
| ロード実行の caller 5 件（**親貸与 v308 追補・検証済み・Phase1 未再検証**） | 0x466B31（arg1=0・0xA3B144='quicksave'）/ 0x466D5F（arg1=[ecx+0x1C0]・fn 先頭 0x466D50）/ 0x5025F7（0）/ 0x5AE1E6（arg1=[esi+0x4C]・FUN_005AE190）/ 0x5B5DA8（[[0xB33B00]+0x6C][0]）。arg 非0 の 3 site はオブジェクト ptr。 | 貸与＋v193b L76（0x5025EA/0x5AE1DB/0x5B5D90 で mov ecx,[0xB33B00] 直近）・v250 L139・v241 L26 |
| パス整形 | FUN_0045F2E0（+0x3C へ strrchr(path,'\\')＋.ess サフィックス検査 0x45F640 _strnicmp） | 貸与（v308 追補） |
| pass2（保存時 flags 整形再発行） | FUN_00461680（唯一 caller 0x4651C3 ∈ FUN_00465130＝SaveGame 側） | v283 件2 L57-・v297 件3 L8-28 |
| 台帳/dispacth ハブ | 0x4662C9（E8 caller ゼロ＝jump table 到達）、LoadGame dispatch @0x466576.. | v299 L15 / v289 件1-2 L19-31 |
| ヘッダ読/書 | FUN_0045DBC0（読）/ FUN_00464AB0（書・version 0x7D） | v305 §2.2-2.3 L117-166 |

> **用語注**: v304 追補B は 0x465860 を「保存書側（0x465860 dispatch）」と呼ぶが、v283 件2-1・s58 §4.2・v202 は 0x465860＝**LoadGame（ロード側）**、保存実行は 0x465130。→ §5-2 に矛盾として併記。

### 2.2 フレーム／台帳仕様（保存側の骨格）

1. **レコード名バンド**: `.rdata` 0xA3914C..0xA39E6C の CHANGE_* 帯を「**savegame 変更レコード名の完全棚卸し**」と帰属（v283 件3-2 L101）。ただし **xref は formatter FUN_00453A90 単独（67/67・保存コードからの文字列参照ゼロ）**（v305 §1.1 L17-19）＝名称帰属であって保存パスの実参照ではない。
2. **tail フレーム**: `<u16 size><byte+2 型文字 '0'..'3'><byte+3 種別(0x5B ゲート)> + payload`、先頭 4cc 無し（負証拠 sweep 0 件）。幅 type'1'→0x24、type'2'/'3'→flag 符号で 0x1C/0x2C（v283 件2 L57・L64・L67）。ロード側歩行は v304 件5 手順3（v283 §2 ＋ bit3→bit11 再発行形式）。
3. **台帳 entry**: 8B entry＋12B `<key=formID><flags><refcount>`（452CF0@0x466829 の唯一の外部呼出＝LoadGame dispatch）。flags は **TESForm ChangeFlags 台帳エントリ**（v289 件1-1 L11 訂正。v286 件2 の「extra エントリ flags」説はここで棄却済み）。
4. **pass2（SaveGame 側）**: `test bl,8 / test bl,6 / cmp ebx,8` → bit3 単独=452DF0 force 除去、bit3+他=`and ~8; or 0x800`（**bit3 tombstone→bit11 disable 標へ翻訳再発行**）、bit31 符号でフレーム 0x2c/0x1c、bit23 で +4（v297 件3 L13-26）。
5. **読取適用**: 459AF0 主ループが type byte '0'..'3'＋**要求 bit{2,4,0x2000000,0x4000000}** で 459950/459990 適用 → 452DF0 除去（v282 件2-1 L68）。
6. **0x5B 境界**: byte+3 ≥0x5B → 12B フレーム（ExtraSound=0x5B）、sub<0x5B → flags bit2/bit4 分岐（v286 件1 L9-25 / v289 件3 L131）。

### 2.3 保存側が個別に分岐する値（S セット・本表の判定キー）

| コード | 条件値 | 分岐内容 | 出典 |
|---|---|---|---|
| S1 | bit1\|bit2 (0x6) | pass2 の bit3 処理ブロック入口（立ちは 0x461dec スキップ） | v297 件3 L14-15 |
| S2 | bit3 (0x8) / 0x80000008 | 単独=452DF0 force 除去、併存=bit11 翻訳再発行／452D60 は bit3\|bit31 清去 | v297 件3 L17-23 / v286 件2 L27-37 |
| S3 | bit11 (0x800) | `or ebx,0x800`＝disable 標（引擎 disable ミラーと直結） | v297 件3 L23 / v283 件3 L91 / v304 件2 L29 |
| S4 | bit23 (0x800000) | フレーム長 +4 | v297 件3 L25 / v304 件5 L49 |
| S5 | bit31 (0x80000000) | 0x2c/0x1c フレーム分岐（符号清 0x7FFFFFFF） | v297 件3 L24 / v283 件2 L57 |
| S6 | type byte '0'..'3' (0x30..0x33) | tail フレーム種別→459950/459990 | v282 件2-1 L68 / v283 件2 L64 |
| S7 | extra 型 ID ≥0x5B | 12B フレーム（個別値は 0x5B=ExtraSound のみ文書化） | v283 件2 L57 / v286 件1 L9-25 / v289 件3 L131 |
| S8 | 要求 bit {0x2, 0x4, 0x2000000, 0x4000000} | 読取適用ゲート（＝ChangeFlags 同一空間・v281 件1＋v289 件1-1） | v282 件2-1 L68 / v281 件1 L13 |
| S9 | bit30 (0x40000000) | file bit30＝実行時 disable の save 側・呼出窓 0x455CF4..D59＝save 変更名テーブル | v283 件3-2 L99-101 |
| S10 | flags 汎用格納 | 台帳 12B `<key><flags><refcount>`（個別ビット分岐なしの通過格納） | v289 件1-1 L11・件1-2 L19 |
| S11 | [ctx+0x50] | pending マスク＝直近レコードの changeFlags 写し（丸値 mov のみ） | v297 件1 L32 |
| S12 | 保存名バンド | CHANGE_* 帯＝savegame 変更レコード名（明示列挙＋band 拡張） | v283 件3-2 L101 / v305 §1.1 L16-19 |

### 2.4 ラベル列の位置づけ（保存側に formatter は呼ばれない）

- formatter FUN_00453A90 の E8 呼び出し元は **2 点のみ**（caller1=FUN_0045ADC0・caller2=FUN_0045CC60、いずれも LoadGame 完了後レポート帯 0x45A-45Cxxx）。**保存書側（0x465130/0x465860 帯）からの formatter 呼出はゼロ**（v304 追補B L77-79、caller 帰属は追補3 L149-150 で訂正済み）。
- よって **80 ラベルはロード/診断側の出力語彙**であり、保存側との対応は「ゲート値（changeFlags bit / extra 型 ID）」と「S12 名称バンド」の 2 本柱でのみ結べる。これが本表の構造的前提。

---

## 3. 突合表（80 行・一意ラベル基準。ゲート列の正本＝v304 一次出力、D 表併記は v304 追補2-D）

### A. プロセスレベル系（6 行・jumptable 0x458E38 / vtbl+0x4C getter）

| # | v304 ラベル（VA） | 型 ID / flag ゲート | CHANGE_* 対応 | 保存側対応分岐/仕様 | 判定 |
|---|---|---|---|---|---|
| 1 | None(%i) @0x453C3D | jumptable 0x458E38（v304 追補A L72-75） | **対応外**（67 件に process level 名なし） | 個別分岐なし・S12 対象外（v304 追補B L77-79） | **[確定負]** |
| 2 | Low(%i) @0x453C6E | 同上 | **対応外** | 同上 | **[確定負]** |
| 3 | Mid Low(%i) @0x453C9F | 同上 | **対応外** | 同上 | **[確定負]** |
| 4 | Mid High(%i) @0x453CD0 | 同上 | **対応外** | 同上 | **[確定負]** |
| 5 | High(%i) @0x453D01 | 同上 | **対応外** | 同上 | **[確定負]** |
| 6 | Base(%i) @0x453D37 | 同上 | **対応外** | 同上 | **[確定負]** |

### B. main 帯 残り（59 ラベル）

| # | v304 ラベル（VA） | 型 ID / flag ゲート | CHANGE_* 対応 | 保存側対応分岐/仕様 | 判定 |
|---|---|---|---|---|---|
| 7 | Form Flags(%i) @0x453E6F | probe行: typeID 0x1@0x453DFB / D 表: flag 0x1（v304 件2-D L115） | #67 CHANGE_FORM_FLAGS @0x453E1F（差 0x50・双方向） | S12 名指し（v283 件3-2 L101 が `'Form Flags(%i)'/CHANGE_FORM_FLAGS` 明示）＋S10 汎用／**bit0 の個別分岐なし**（S1 は bit1\|2） | [部分確定] |
| 8 | Quest Flags(%i) @0x453FC7 | probe行: typeID 0x23/0x24/0x3B/0x4@0x453F32-4A / D 表: flag 0x4（形式 2 説） | #66 CHANGE_QUEST_FLAGS @0x453F6E（差 0x59） | S1/S8 に値 0x4 はあるが（v297 件3 L15・v282 件2-1 L68）、逆方向が 5 ラベルで多義・ゲート形式も未統一 | [部分確定] |
| 9 | Quest Stages(%i) @0x454110 | flag 0x10000000@0x45408B | #65 CHANGE_QUEST_STAGES @0x4540B2 | S1-S9 該当なし（0x10000000 は保存側分岐に不在）＋S10/S12 | [部分確定] |
| 10 | Quest Script(No longer has script locals) @0x454255 | flag 0x8000000@0x4541CB | #64 CHANGE_QUEST_SCRIPT @0x4541F2 との同定は**類推**（site が 2 ラベルの中間・どちらに帰属するか一意でない） | S 該当なし＋S10/S12 | **[推定]** |
| 11 | Quest Script(%i) @0x454289 | 行内ゲート未取得（帯） | #64 CHANGE_QUEST_SCRIPT（名称一意で双方向） | S 該当なし＋S10/S12 | [部分確定] |
| 12 | Topic Said Once(%i) @0x4543BE | typeID 0x3A@0x454333 ＋ flag 0x10000000@0x45433B | #63 CHANGE_TOPIC_SAIDONCE @0x454362 | 型 0x3A<0x5B は S7 の個別値外、flag も非該当＋S10/S12 | [部分確定] |
| 13 | Never Run Flag(%i) @0x4544CC | typeID 0x3D@0x454463 ＋ flag 0x10000000@0x45446B | #62 CHANGE_PACKAGE_NEVER_RUN @0x454498 | 同上 | [部分確定] |
| 14 | Waiting Flag(%i) @0x4545E2 | flag 0x8000000@0x454583 | #61 CHANGE_PACKAGE_WAITING @0x4545B0 | 非該当＋S10/S12 | [部分確定] |
| 15 | Cell Flags(%i) @0x4546FF | probe行: typeID 0x30/0x8@0x454683-8B / D 表: flag 0x8（形式 2 説） | #60 CHANGE_CELL_FLAGS @0x4546AF | S2 に値 0x8 はあるが（v297 件3 L17）逆方向は bit3=tombstone 説と衝突（§5-5）＋多義 | [部分確定] |
| 16 | Cell Created(%i) @0x454838 | probe行: typeID 0x2/0x4@0x4547B7-C0 / D 表: flag 0x2+0x4、call 0x452250(GetForm) | #59 CHANGE_CELL_CREATED @0x4547E4 | S1/S8 に値 0x2/0x4 はあるが多義（0x4 は 5 ラベル）・形式争い | [部分確定] |
| 17 | Seen Data(%i) @0x454954 | flag 0x10000000@0x4548F3 | #58 CHANGE_CELL_SEENDATA @0x454920 | 非該当＋S10/S12 | [部分確定] |
| 18 | Detach Time(%i) @0x454A74 | flag 0x8000000@0x454A13 | #57 CHANGE_CELL_DETACHTIME @0x454A42 | 非該当＋S10/S12 | [部分確定] |
| 19 | Ownership(%i) @0x454BBA | typeID 0x20@0x454B3B | #56 CHANGE_CELL_OWNERSHIP @0x454B5F | 型 0x20 は S7 個別値外（0x5B のみ）＋S10/S12 | [部分確定] |
| 20 | Full Name(%i) @0x454CD1（cell）／@0x4587CA（actor base・2 行 1 一意） | typeID 0x10@0x454C71 / base 側ゲート未取得 | #55 CHANGE_CELL_FULLNAME @0x454CA0 ＋ #6 CHANGE_ACTOR_BASE_FULLNAME @0x458798（2 CHANGE→1 ラベル・VA 双方向） | 型 0x10 個別分岐なし＋S10/S12 | [部分確定] |
| 21 | Pathgrid Modified(%i) @0x454DE1 | flag 0x1000000@0x454D81 | #54 CHANGE_CELL_PATHGRID_MODIFIED @0x454DB0 | 非該当＋S10/S12 | [部分確定] |
| 22 | Faction Flags(%i) @0x454EE4 | probe行: typeID 0x6/0x4@0x454E81-89 / D 表: flag 0x4 | #53 CHANGE_FACTION_FLAGS @0x454EB3 | S1/S8 値 0x4（多義・形式争い） | [部分確定] |
| 23 | Faction Reactions(%i) @0x455001 | probe行: typeID 0x8@0x454FA3 / D 表: flag 0x8 | #52 CHANGE_FACTION_REACTIONS @0x454FD0 | S2 値 0x8（多義・形式争い・§5-5） | [部分確定] |
| 24 | Book Skill(%i) @0x455101 | probe行: typeID 0x15/0x4@0x4550A3-AB / D 表: flag 0x4 | #51 CHANGE_BOOK_TEACHES_SKILL @0x4550D5 | S1/S8 値 0x4（多義・形式争い） | [部分確定] |
| 25 | Created(%i) @0x455208 | probe行: typeID 0x31/0x32/0x33/0x2@0x4551A3-B3、call 0x452250 / D 表: flag 0x2 | #50 CHANGE_CREATED_NEW_REFERENCE @0x4551E0 | S1/S8 値 0x2（Cell Created と 2 値多義） | [部分確定] |
| 26 | Cell Changed(%i) @0x455324 | **ゲート未取得**（GetForm＋TESForm cast のみ＝v304 件2 L25 の formID 解決 6 種） | **対応外**（67 件に CELL_CHANGED 無し） | **保存側該当分岐なし（artifact 上・S12 にも不在）** | **[未対応]** |
| 27 | Moved(%i) @0x455464 | ゲート未取得（float 3 部材帯・call 0x452250） | #49 CHANGE_REFR_MOVE @0x45540C | S 該当なし＋S10/S12 | [部分確定] |
| 28 | Havok Moved(%i) @0x45557F/0x45558E/0x4555CB（3 行 1 一意） | ゲート未取得 | #48 CHANGE_REFR_HAVOK_MOVE @0x455553 | S 該当なし（bit11=S3 は別ラベル #29 の値）＋S10/S12 | [部分確定] |
| 29 | Had Havok Move Flag(%i) @0x4556C3 | **flag 0x800@0x455683**（`cmp edx,0x800`＝changeFlags bit11） | **対応外**（67 件に該当名なし） | **S3（v297 件3 L23 の bit11 再発行）＋ v304 件2 L29 が「bit11＝本ラベル⇔v297/v275 disable ミラーと直結」を明記＝双方向、S9 は無し** | **[確定]** |
| 30 | Oblivion Flag(0) @0x45581D | **typeID 複合 0xE@0x455814（=test dl,0xE＝bit1\|2\|3）＋ flag 0x800000@0x455773・0x2000000@0x4557AA** | #47 CHANGE_REFR_OBLIVION_FLAG @0x4557A0 | **S4（bit23 +4）＋S8（要求 0x2000000 適用）＋S1/S2（0xE=bit1\|2\|3 入口）— 3 値とも保存側個別分岐に出現し、値は本ラベルに一意（0x800000/0x2000000 は他ラベル無し）** | **[確定]** |
| 31 | Oblivion Flag(%i) @0x455852 | 行内ゲート未取得（帯） | #47 共有（2 ラベル 1 CHANGE） | 同同帯（値の逆方向は #30 で着地済） | [部分確定] |
| 32 | Animation(%i) @0x45592E | ゲート未取得 | #46 CHANGE_REFR_ANIMATION @0x4557DC（名称一意・※site/label 順序が逆＝dword-inline hoist 嫌疑・§5-8） | S 該当なし＋S10/S12 | [部分確定] |
| 33 | Scale(%i) @0x455A61 | typeID 0x10@0x4559EB | #45 CHANGE_REFR_SCALE @0x455A0F | 型 0x10 個別分岐なし＋S10/S12 | [部分確定] |
| 34 | All Extra(%i) @0x455B97 | typeID 0x20@0x455B11 | #44 CHANGE_REFR_ALL_EXTRA @0x455B3C | 型 0x20 個別分岐なし＋S10/S12 | [部分確定] |
| 35 | Disabled(%i) @0x455CF3 | **flag 0x40000000@0x455C51**（＋0x10000000@0x455C82） | #43 CHANGE_REFR_DISABLE_STATE @0x455C78 | **S9（v283 件3-2 L99-101: 呼出窓 0x455CF4..0x455D59＝save 変更名テーブル、file bit30＝実行時 disable の save 側）＝値・名とも双方向・値は本ラベルに一意** | **[確定]** |
| 36 | Enabled(%i) @0x455D25/0x455D2F（2 行 1 一意） | 行内ゲート未取得（帯） | #43 共有（3 ラベル 1 CHANGE） | S9 バンド（v283 件3-2 が `'Enabled(%i)'` を明示）／自身のゲート値未取得 | [部分確定] |
| 37 | Disabled/Enabled(%i) @0x455D58 | 行内ゲート未取得（帯） | #43 共有 | S9 バンド（v283 件3-2 が `'Disabled/Enabled(%i)'` を明示）／ゲート未取得 | [部分確定] |
| 38 | Leveled Creature(%i) @0x455E3E | ゲート未取得 | #42 CHANGE_REFR_EXTRA_LEVELED_CREATURE @0x455CB3（名称一意・※VA 順序異常・§5-8） | S 該当なし＋S10/S12 | [部分確定] |
| 39 | Inventory(%i) @0x455F7A | flag 0x8000000@0x455EF3 | #41 CHANGE_REFR_INVENTORY @0x455F1C | 非該当＋S10/S12 | [部分確定] |
| 40 | Script(%i) @0x456094 | **flag 0x4000000@0x456033** | #40 CHANGE_REFR_EXTRA_SCRIPT @0x456060 | **S8（v282 件2-1 L68・要求 0x4000000 で 459950/459990 適用）＝保存/読取側の個別分岐に出現し値は本ラベルに一意** | **[確定]** |
| 41 | Movement Extra(%i) @0x4561B3 | flag 0x1000000@0x456153 | #39 CHANGE_REFR_EXTRA_SAVEDMOVEMENTDATA @0x456180 | 非該当（0x1000000 は Pathgrid と 2 値多義だが保存側分岐に不在）＋S10/S12 | [部分確定] |
| 42 | Map Marker Flags(%i) @0x4562DB | typeID 0x32/0x33＋flag 0x400@0x45627A | #38 CHANGE_MAPMARKER_EXTRA_FLAGS @0x4562A7 | 非該当（0x400・型 0x32/0x33 は S6/S7 一般規定のみ）＋S10/S12 | [部分確定] |
| 43 | Furniture Markers(%i) @0x4563F4 | flag 0x400000@0x456393 | #37 CHANGE_OBJECT_EXTRA_FURNITURE_MARKER @0x4563C0 | 非該当（0x400000 は Game Modifiers と多義）＋S10/S12 | [部分確定] |
| 44 | Extra Magic(%i) @0x456511 | flag 0x200000@0x4564B3 | #36 CHANGE_OBJECT_EXTRA_MAGIC @0x4564E0 | 非該当（0x200000 は Script Modifiers と多義）＋S10/S12 | [部分確定] |
| 45 | Lock(%i) @0x456621 | probe行: typeID 0x40@0x4565C3 / D 表: flag 0x40（形式 2 説・§5-3） | #35 CHANGE_OBJECT_EXTRA_LOCK @0x4565F0 | いずれの読みでも S1-S9 非該当＋S10/S12 | [部分確定] |
| 46 | Owner(%i) @0x456731 | ゲート未取得（行内 0x98208B のみ） | #34 CHANGE_OBJECT_EXTRA_OWNER @0x456700 | S 該当なし＋S10/S12 | [部分確定] |
| 47 | Global Owner(%i) @0x456842 | flag 0x100@0x4567E3 | #33 CHANGE_OBJECT_EXTRA_GLOBAL @0x456810 | 非該当＋S10/S12 | [部分確定] |
| 48 | Rank Owner(%i) @0x456954 | flag 0x200@0x4568F3 | #32 CHANGE_OBJECT_EXTRA_RANK @0x456920 | 非該当＋S10/S12 | [部分確定] |
| 49 | Dropped Item Flag(%i) @0x456A74 | flag 0x20000@0x456A13 | **対応外**（67 件に該当名なし） | **保存側該当分岐なし（S セットに 0x20000 不在・S12 にも不在）** | **[未対応]** |
| 50 | Empty Flag(%i) @0x456BBC | flag 0x10000@0x456B33 | #31 CHANGE_OBJECT_EMPTY @0x456B5C | 非該当（0x10000 は 3 ラベル多義・保存側分岐不在）＋S10/S12 | [部分確定] |
| 51 | Teleport(%i) @0x456D07 | typeID 0x18@0x456C97（`cmp byte[eax+4],0x18`＝vtbl+0x170 getter 経由・v304 件2-D' L133）＋flag 0x100000@0x456CA8 | #30 CHANGE_DOOR_EXTRA_TELEPORT @0x456CD5 | 型 0x18<0x5B は古典フレーム（S6/S7 一般規定）、flag は非該当＋S10/S12 | [部分確定] |
| 52 | Door State(%i) @0x456E24 | flag 0x80000@0x456DC3 | #29 CHANGE_DOOR_OPEN_STATE @0x456DF2 | 非該当＋S10/S12 | [部分確定] |
| 53 | Door Default State(%i) @0x456F48 | flag 0x40000@0x456EEB | #28 CHANGE_DOOR_OPEN_DEFAULT_STATE @0x456F16 | 非該当＋S10/S12 | [部分確定] |
| 54 | Equipment(%i) @0x457052 | flag 0x20000000@0x456FF1 | #27 CHANGE_ACTOR_EQUIPMENT @0x457020 | 非該当＋S10/S12 | [部分確定] |
| 55 | Game Modifiers(%i) @0x457165 | flag 0x400000@0x457103 | #26 CHANGE_ACTOR_GAME_MODIFIERS @0x457132 | 非該当＋S10/S12 | [部分確定] |
| 56 | Script Modifiers(%i) @0x457287 | flag 0x200000@0x45722B | #25 CHANGE_ACTOR_SCRIPT_MODIFIERS @0x457256 | 非該当＋S10/S12 | [部分確定] |
| 57 | Magic Modifiers(%i) @0x4573A4 | flag 0x100000@0x457341 | #24 CHANGE_ACTOR_MAGIC_MODIFIERS @0x457370 | 非該当（0x100000 は Teleport と多義）＋S10/S12 | [部分確定] |
| 58 | CHANGE_ACTOR_RUNONCE_PACKAGE %08X @0x4574AB（CHANGE 行＝自己同一） | flag 0x80000@0x457461 | **#23 自身（identity・命令先頭 0x4574AB＝v305 §1.2 註 L97 と一致）** | 非該当（0x80000 は Door State と多義）＋S10/S12 | [部分確定] |
| 59 | Run Once %08X(%i) @0x4574E5 | 行内ゲート未取得（直前 CHANGE 行と同一分岐） | #23 共有（2 ラベル 1 CHANGE） | S10 の formID キー付き記録（v304 件2 L34「Run Once/Trespass の formID キー付き extra」＋v289 件1-1 の `<key><flags><refcount>`）は片方向、値分岐なし | [部分確定] |
| 60 | CHANGE_ACTOR_EXTRA_TRESPASS_PACKAGE %08X @0x4575E0（自己同一） | flag 0x40000@0x4575A3 | **#22 自身（命令先頭 0x4575E0）** | 非該当（0x40000 は Door Default State と多義）＋S10/S12 | [部分確定] |
| 61 | Trespass %08X(%i) @0x45761A | 行内ゲート未取得 | #22 共有 | #59 と同様 S10 片方向 | [部分確定] |
| 62 | CHANGE_ACTOR_INTERRUPT_PACKAGE %08X @0x457705（自己同一） | flag 0x20000@0x4576D3 | **#21 自身（命令先頭 0x457705）** | 非該当（0x20000 は Dropped Item Flag と多義）＋S10/S12 | [部分確定] |
| 63 | %s %08X(%i) @0x457799 | flag 0x10000@0x457718＋package 名解決 0x5E4080（v304 追補A L75） | **対応外** | **個別レコードを持たない未知 extra/package の汎用フォールバック（v304 件2 L32）＝保存側個別対応は設計上不要** | **[確定負]** |
| 64 | Non-saved Package(%i) @0x4578B1 | flag 0x10000@0x457853 | #20 CHANGE_ACTOR_NONSAVED_PACKAGE @0x457880 | 非該当＋S10/S12 | [部分確定] |
| 65 | Disp Modifiers(%i) @0x4579C4 | flag 0x8000@0x457963 | #19 CHANGE_ACTOR_DISPOSITION_MODIFIERS @0x457992 | 非該当（0x8000 は S1-S9 不在）＋S10/S12 | [部分確定] |

### C. 延長帯（0x457A8C..0x458E50・新規一意 15 ラベル）

| # | v304 ラベル（VA） | 型 ID / flag ゲート | CHANGE_* 対応 | 保存側対応分岐/仕様 | 判定 |
|---|---|---|---|---|---|
| 66 | Oblivion Entry(%i) @0x457AE7 | flag 0x4000@0x457A8B | #18 CHANGE_ACTOR_EXTRA_OBLIVION_ENTRY @0x457AB6 | 非該当（0x4000 不在）＋S10/S12 | [部分確定] |
| 67 | Investment Gold(%i) @0x457C01 | flag 0x2000@0x457BA1 | #17 CHANGE_ACTOR_EXTRA_INVESTMENTGOLD @0x457BD0 | 非該当＋S10/S12 | [部分確定] |
| 68 | Life State(%i) @0x457D1E | flag 0x40@0x457CB1 | #16 CHANGE_ACTOR_LIFESTATE @0x457CE0 | 非該当（0x40 は Factions と多義）＋S10/S12 | [部分確定] |
| 69 | Persuasion(%i) @0x457E31 | flag 0x1000@0x457DD1 | #15 CHANGE_ACTOR_EXTRA_PERSUASION @0x457E00 | 非該当＋S10/S12 | [部分確定] |
| 70 | Crime Gold(%i) @0x457F4A | typeID word 0x32@0x457EE1 | #14 CHANGE_CHARACTER_EXTRA_CRIMEGOLD @0x457F16 | 型 0x32<0x5B は S6/S7 一般規定のみ、flag 分岐なし＋S10/S12 | [部分確定] |
| 71 | Base Health(%i) @0x458051 | **flag 0x4@0x457FF1（test dl・形式確定）** | #13 CHANGE_ACTOR_BASE_HEALTH @0x458020 | S1/S8 に値 0x4 はあるが、0x4 は D 表読みを含め 5 ラベルで多義＝逆方向が一意着地しない | [部分確定] |
| 72 | Base Attributes(%i) @0x458161 | **flag 0x8@0x458103（test dl・形式確定）** | #12 CHANGE_ACTOR_BASE_ATTRIBUTES @0x458130 | S2 値 0x8 はあるが（a）bit3=tombstone 説と意味衝突（§5-5）（b）0x8 は 3 ラベル多義 | [部分確定] |
| 73 | Base Data(%i) @0x458272 | flag 0x10@0x45821B | #11 CHANGE_ACTOR_BASE_DATA @0x458243 | 非該当＋S10/S12 | [部分確定] |
| 74 | Spell List(%i) @0x458380 | flag 0x20@0x458321 | #10 CHANGE_ACTOR_BASE_SPELLLIST @0x458350 | 非該当＋S10/S12 | [部分確定] |
| 75 | Factions(%i) @0x45848F | flag 0x40@0x458431 | #9 CHANGE_ACTOR_BASE_FACTIONS @0x458460 | 非該当（0x40 多義）＋S10/S12 | [部分確定] |
| 76 | Base Modifiers(%i) @0x4585A3 | flag 0x10000000@0x458541 | #8 CHANGE_ACTOR_BASE_MODIFIERS @0x458570 | 非該当（0x10000000 は 5 ラベル多義・保存側分岐不在）＋S10/S12 | [部分確定] |
| 77 | AI Data(%i) @0x4586B4 | flag 0x100@0x458651 | #7 CHANGE_ACTOR_BASE_AIDATA @0x458680 | 非該当＋S10/S12 | [部分確定] |
| 78 | Skills(%i) @0x458911（NPC 側）／@0x458C56（CREATURE 側）（2 行 1 一意） | typeID 0x23＋flag 0x200@0x458890 / typeID 0x24＋flag 0x200@0x458BF9 | **#5 CHANGE_NPC_SKILLS @0x4588BC（↔0x458911）＋ #2 CHANGE_CREATURE_SKILLS @0x458C24（↔0x458C56）＝2 CHANGE→1 ラベル・typeID で分離・VA 双方向** | 非該当（0x200 不在・型 0x23/0x24 は S7 個別値外）＋S10/S12 | [部分確定] |
| 79 | Combat Style(%i) @0x458A22（NPC 側）／@0x458D73（CREATURE 側）（2 行 1 一意） | flag 0x400@0x4589C1 / flag 0x400@0x458D11 | **#4 CHANGE_NPC_COMBATSTYLE @0x4589F0 ＋ #1 CHANGE_CREATURE_COMBATSTYLE @0x458D40 ＝2 CHANGE→1 ラベル・VA 双方向** | 非該当（0x400 は Map Marker と多義・保存側分岐不在）＋S10/S12 | [部分確定] |
| 80 | Fame(%i) @0x458B53 | **flag 0x800@0x458AD1（test edx）** | #3 CHANGE_NPC_FAME @0x458AFC | S3 に値 0x800 はあるが、S3 の保存側意味は v297/v283 で「disable 標」であり v304 件2 が 0x800 と明記リンクしたのは #29 Had Havok Move Flag 側（§5-4）＝逆方向が本ラベルに着地しない | [部分確定] |

---

## 4. 集計

### 4.1 80 ラベルの判定内訳

| 判定 | 件数 | 該当 |
|---|---|---|
| **[確定]** | **4** | #29 Had Havok Move Flag / #30 Oblivion Flag(0) / #35 Disabled / #40 Script |
| **[部分確定]** | **66** | 上記以外の有効ラベル全般（#7-9, #11-25, #27-28, #31-34, #36-39, #41-48, #50-62, #64-77, #78-80 ほか） |
| **[推定]** | **1** | #10 Quest Script(No longer has script locals) |
| **[未対応]** | **2** | #26 Cell Changed(%i) / #49 Dropped Item Flag(%i) |
| **[確定負]** | **7** | #1-6 プロセスレベル 6 種 / #63 %s %08X(%i) |
| 合計 | **80** | 87 行の一意化結果と一致 |

### 4.2 [未対応] リスト（本成果物の主要成果＝移植 delta 台帳の欠落候補）

1. **Cell Changed(%i)**（formatter @0x455324）
   - ラベル↔CHANGE: **67 件に CELL_CHANGED なし**（v305 §1.2 全表・セル系は PATHGRID/FULLNAME/OWNERSHIP/DETACHTIME/SEENDATA/CREATED/FLAGS の 7 件のみ）。
   - 保存側: 行内ゲート取得なし（GetForm 0x452250＋TESForm cast の formID 解決帯・v304 件2 L25 の 6 種）、S1-S9 のいずれにも該当値なし、S12 名称バンドにも不在。
   - 含意: 「セルが変わった」事を示す changeFlags ビットが formatter に存在するのに、**保存側の個別分岐・レコード名の双方が artifact 上に無い**。delta 台帳に載せるべきか否かは Phase2-5 で裏取り。
2. **Dropped Item Flag(%i)**（formatter @0x456A74・gate flag 0x20000@0x456A13）
   - ラベル↔CHANGE: **67 件に DROPPED 系なし**（OBJECT 系は EMPTY/RANK/GLOBAL/OWNER/LOCK/MAGIC/FURNITURE_MARKER の 7 件のみ）。
   - 保存側: 値 0x20000 は S セット（0x2/0x4/0x8/0x800/0x800000/0x2000000/0x4000000/0x40000000）に**不在**、S12 にも不在。
   - 含意: flags 一般格納（S10）の恩恵は受けうるが、**個別証拠（名称・分岐）が両方欠落**。

> 補足: 「保存側に該当分岐なし」は S10（台帳 12B `<key><flags><refcount>` への通過格納・v289 件1-1）までは及ぶため、上記 2 件もビット値自体はファイルへ流れうる。ただし**分岐（branch）としての証拠と名称証拠の両方が無い**ことをもって [未対応] とした（PASS-THROUGH は分岐ではない）。

### 4.3 CHANGE_* 67 ↔ ラベル 80 の対応形状（同一 formatter FUN_00453A90 内・v304/v305 から対応表化）

- **どちらが多いか: ラベル 80 > CHANGE 67**（差 13）。理由は (i) プロセスレベル 6 種と %s フォールバックが CHANGE を持たない、(ii) 同一 CHANGE が複数ラベル（verbose/quiet・`0`/`%i`・種別分離）に分岐する、(iii) 一部 CHANGE が 1 ラベルに合流。
- **CHANGE 側の対応率: 67/67 = 100%**（67 件すべてが ≥1 ラベルに着地。ラベル未対応の CHANGE は 0 件）。
- **ラベル側の対応率: 70/80 = 87.5%**（**対応外 10 ラベル**: None / Low / Mid Low / Mid High / High / Base（プロセスレベル 6）、Cell Changed、Had Havok Move Flag、Dropped Item Flag、%s %08X(%i)）。
- 対応形状の内訳:
  - **1 CHANGE → 複数ラベル = 5 CHANGE → 11 ラベル**: #43 CHANGE_REFR_DISABLE_STATE → Disabled / Enabled / Disabled-Enabled（3）、#47 CHANGE_REFR_OBLIVION_FLAG → Oblivion Flag(0) / (%i)（2）、#64 CHANGE_QUEST_SCRIPT → Quest Script(%i) / No longer（2）、#23 CHANGE_ACTOR_RUNONCE_PACKAGE → CHANGE 行 / Run Once %08X(%i)（2）、#22 CHANGE_ACTOR_EXTRA_TRESPASS_PACKAGE → CHANGE 行 / Trespass %08X(%i)（2）。
  - **複数 CHANGE → 1 ラベル = 6 CHANGE → 3 ラベル**: #2+#5（SKILLS→Skills）、#1+#4（COMBATSTYLE→Combat Style）、#6+#55（ACTOR_BASE_FULLNAME＋CELL_FULLNAME→Full Name）。
  - **1:1 = 56 CHANGE → 56 ラベル**。
  - 上記 5＋6＋56 ＝ 67 CHANGE、11＋3＋56 ＝ 70 ラベル（＋対応外 10 ＝ 80）で閉じる。
  - **3 行は CHANGE 文字列行そのもの**（#21/#22/#23 = 0x457705/0x4575E0/0x4574AB・v305 §1.2 註 L97 の命令先頭と一致＝identity 対応）。
  - ※ VA 順序が名称対応と逆転する 2 組あり（#42 LEVELED_CREATURE の site 0x455CB3 が 'Disabled' ラベルより前、#46 ANIMATION の site 0x4557DC が Oblivion Flag 両ラベルより前）＝名称一意性で裁定・Phase2-8 で VA 検証。

### 4.4 保存側カバレッジ（ラベル 80 の視点）

| 保存側で結べる最強の根拠 | 件数 | 備考 |
|---|---|---|
| **S1-S9 個別値分岐に到達** | 14 | #8, #15, #16, #22, #23, #24, #25（D 表由来・形式争い 7）＋ #29, #30, #35, #40, #71, #72, #80（probe 行 `test` 形 7）。うち (b)(c) を満たすのは 4（[確定]）。 |
| S12 名称バンドのみ（個別分岐なし） | 70（CHANGE 対応 70 ラベル全般） | S12 の 67 件適用は band 拡張推論（§1 留保）＝[部分確定] の主因 |
| 個別証拠なし | 2（[未対応]）＋ 7（[確定負]） | §4.2 |

---

## 5. 矛盾・不整合（旧結論は消さず両論併記＋裁定理由）

1. **v304 件5 の引用誤り（新規検出）**: v304 §件5 手順3 L49 は再発行形式を「**v301 §件3** の再発行形式（bit3 tombstone→bit11 翻訳）」と引用するが、v301 §件3（L41-46）は「0x463913 到達元=fstart 誤検出の訂正」であり該当内容なし。当該内容は **v297 §件3 L8-28** のもの。→ 裁定: **引用先を v297 §件3 に読み替える**。v304 の本文（再発行形式の記述内容）自体は v297 と一致するため結論は不変。旧記述は v304 内に残る。
2. **「保存書側（0x465860 dispatch）」表記 vs 0x465860＝LoadGame**: v304 追補B L79 は 0x465860 を保存書側と呼ぶが、v283 件2-1 L59（「v282 の『セーブ窓』表記の訂正」）、s58 §4.2 L182-195、v202 §3 L94、v307 L90 は 0x465860＝**ロード実行**、保存実行は 0x465130 で一致。→ 裁定: **0x465860＝LoadGame が正典**（4 文書一致）。v304 追補B の結論「保存/ロード帯の書込系から formatter への E8 はゼロ（formatter caller 全数=2）」は負証拠として有効だが、厳密には「0x465130（save）鎖からの formatter 呼出ゼロ」は caller2 窓の計測範囲外である可能性を残す → Phase2-6 へ。
3. **ゲート形式の intra-artifact 不一致 7 ラベル**: v304 追補2-D 表 L111-133 が「changeFlags bit」とする値（Quest Flags 0x4 / Cell Flags 0x8 / Faction Flags 0x4 / Faction Reactions 0x8 / Book Skill 0x4 / Cell Created 0x2+0x4 / Created 0x2 / Lock 0x40）のうち、一次出力（i7c）は同位置を `cmp al/word`＝**extra 型 ID ゲート**として記録。D 表自身が「ウィンドウ法・±1 行の揺れあり→[部分]」と注記（L111）。→ 両論併記: (a) D 表の bit 読み、(b) probe 行の type 読み。裁定: **形式が未統一のラベルは [確定] を認めず [部分確定]**（本表の判定規則 (b) の根拠）。
4. **同一 bit 値の多義（9 組）**: 0x800＝Had Havok Move Flag＋Fame / 0x4＝最多 5 ラベル / 0x8＝3 ラベル / 0x2＝2 ラベル / 0x10000＝3 ラベル / 0x100000＝Teleport＋Magic Modifiers / 0x400＝Map Marker＋Combat Style / 0x400000＝Furniture Markers＋Game Modifiers / 0x200000＝Extra Magic＋Script Modifiers / 0x1000000＝Pathgrid＋Movement Extra / 0x8000000＝4 ラベル / 0x10000000＝5 ラベル / 0x40＝Life State＋Factions。→ 両論: (a) formType ごとに同一ビットが別意味を持つ（v304 D 表が意図的に同値を複数ラベルに割当）、(b) 700 命令ウィンドウの帰属 bleed。裁定: Phase1 では (a) を v304 の結論として尊重しつつ、逆方向の一意着地を要求する判定では多義を「片方向」と扱う（#71/#72/#80 がこれで [部分確定] に落ちる理由）。Phase2-3 で裁定。
5. **bit3 の意味衝突**: v304 D 表/i9b は bit3(0x8) を「Cell Flags / Base Attributes」の変更内容ビットとして扱う一方、v297 件3 L17-23 は pass2 で bit3 を「**破棄 tombstone**」として扱う。同一 ChangeFlags dword（v281 件1 L13 の要求 dword＝v289 件1-1 の台帳 flags）なら意味が衝突する。→ 両論: (a) 同一 dword で段階/文脈依存の多義、(b) pass2 の ebx はレコード制御フィールドで別物（v289 件1-2 L19 の「Header flags / Current Flags」2 種記述が旁証）。裁定: **未決着・Phase1 では両論併記**。確定には 452CF0 の flags 実体確認（Phase2-4）を要する。
6. **CHANGE_* の「保存レコード名」帰属 vs xref 負証拠**: v283 件3-2 L101 は CHANGE 帯を「savegame 変更レコード名の完全棚卸し」と帰属するが、v305 §1.1 L17-19 は **67/67 の xref が formatter FUN_00453A90 内のみ（保存コードからの参照ゼロ）**。→ 整合解: 名称の**帰属**であって保存パスの実参照ではない。本表は S12 を「名称同定の根拠」としてのみ用い、保存側分岐（S1-S9）とは区別した。
7. **452DF0 の役割**: v286 件2 L37 は 452DF0 を「設定側（or/挿入系）」と記載するが、v281 件1 L13/L34（DeleteForm 内の除去）と v289 件1-2 L23（`push 1` 固定の force 除去・14 callers）は「除去側」。→ 裁定: **後発の v289（全 caller 実測）を正**、v286 の記述は誤り。本表の S2 表記は v289/v297 準拠。
8. **既存訂正の参照（旧結論は保持）**: (i) formatter 範囲 0x457A8C（v304 件1）→ 0x458E50（追補2-A L93-96）、(ii) 「プロセス優先度ラベル」→「process level 名」（追補A L72-75）、(iii) caller 先頭 0x45AE65/0x45CC5F・caller2∈FUN_0045C9C0 → 0x45ADC0/0x45CC60/FUN_0045CC60（追補3 L149-151）、(iv) 「66 種」→「87 行/一意 80」（追補2-B L100-101）、(v) v305 §2.4「現ゲームバージョン」→ ファイル version byte（v307 L90）、(vi) v301 件2 sink 結論の破棄（v301 訂正節 L63-71）。いずれも本表は**新採用**し、旧記述は原 artifact 内に残っている。

---

## 6. Phase2 リスト（要 EXE スポット検証・v309 lane 完了後・親セッションが実施）

> 帯 0x45D000-0x466000 は v309 lane 使用中のため、以下はすべて v309 完了後の EXE プローブ対象。Phase1 では列挙のみ。

1. **保存帯 0x465860 の 5 caller site 引数の再検証**（親貸与 v308 追補の裏取り）: 0x466B31（arg1=0・0xA3B144='quicksave'）/ 0x466D5F（arg1=[ecx+0x1C0]・fn 先頭 0x466D50）/ 0x5025F7（0）/ 0x5AE1E6（arg1=[esi+0x4C]・FUN_005AE190）/ 0x5B5DA8（[[0xB33B00]+0x6C][0]）＋ arg 非0 の 3 site がオブジェクト ptr という帰属。あわせて FUN_0045F2E0 のパス整形（strrchr＋.ess サフィックス 0x45F640）。
2. **ゲート形式争い 7 ラベルの命令単位裁定**: Quest Flags / Cell Flags / Faction Flags / Faction Reactions / Book Skill / Cell Created / Created（`cmp al`（型 ID）か `test dl`（changeFlags）か・v304 件2-D と probe 行の不一致・§5-3）。
3. **同値多義 bit の裁定（13 組・§5-4）**: formType 別の意味共有かウィンドウ帰属 bleed か。特に 0x800（Had Havok↔Fame）、0x4/0x8（D 表系）、0x100000/0x400/0x400000/0x200000/0x1000000/0x8000000/0x10000000。
4. **452CF0 の flags 実体確認**: `<key><flags><refcount>` の flags が changeFlags（台帳 flags）と同物か、レコード制御フィールドか。→ **bit3 tombstone（v297）と Cell Flags/Base Attributes の 0x8 の二義解消（§5-5）**、および S10 通過格納の確定。
5. **[未対応] 2 件の裏取り**: 'Cell Changed(%i)'／'Dropped Item Flag(%i)' の保存側不在を 0x45C000..0x467000（保存・ロード両側）で再走査し、本当に個別分岐・レコード名を持たないか。
6. **保存実行 FUN_00465130 鎖から formatter への呼出ゼロの再確認**: v304 追補B の負証拠は 0x465860（LoadGame）帯を「保存書側」と表記して測定したもの（§5-2）＝対象関数を 0x465130 側に特定して E8 全数を再計測。
7. **プロセスレベル 6 ラベルの保存有無**: .ess の process 保持帯（該当 change record の存否）＝[確定負] 判定の裏取り。存在する場合は [未対応] へ格上げ。
8. **CHANGE_QUEST_SCRIPT の 1:2 同定**（'Quest Script(%i)' か 'No longer has script locals' か、両方か）＋ VA 順序逆転 2 組（LEVELED/DISABLE・ANIMATION/OBLIVION）の site 帰属。
9. **変更レコードの flags 載せ位置**: tail frame `<len><type><typeID>+payload` のどこに flags があるか（S10 12B entry との関係・v283 件2/v297 件3 の接続）。
10. **0x5B 境界と formatter 型 ID の対関係**: formatter 側の型 ID ゲート最大は 0x40（Lock）で 0x5B 未満＝12B フレーム（S7）に着地するラベルが存在しないことの確認、ExtraSound(0x5B) にラベルが無いことの確認。
11. **pass2（FUN_00461680）到達の単一性**: caller が 0x4651C3（SaveGame）のみかの全数確認（ロード側にも pass2 相当がある場合、判定が変わる）。
12. **Warnings.txt oracle 突合（v304 件5 手順4）**: 実機 Warnings.txt 生成待ちの B 型ゲート（Phase1 不可・v306/v308 で出力経路は負証拠化済み）。

---

## 7. 遵守宣言

- EXE プローブ: **0 回**（Phase1 禁止遵守。%TEMP% の既存出力読取のみ）／ESM: 未接触／git: 未実行／クリーンルーム: 遵守。
- 編集: 本ファイルのみ新規。既存 tmp/re・docs・GT・ソース・.github/** は読み取りのみ。
- 中間物: 新規作成なし（%TEMP%\opencode\v311_* 未使用）。
- 検証: 87 行・一意 80・CHANGE 67/67・判定合計 80・対応形状 5+6+56=67 / 11+3+56=70 の各一致を自己照合済み。

## Phase2 検証結果（v312 統合時・2026-11-07）

`v312_phase2_gate_verification.md`（34,725B/245行・EXE スポット検証 12 件）により本表の判定を以下で**更新**する（旧判定は上記表に残し、本節が上書き裁定）:

1. **[未対応] 2 件 → [部分確定] へ格上げ・§4.2 の残 [未対応] は 0 件**:
   - `Cell Changed(%i)`: bit31 ゲート `test edx,edx / jns` @0x4552C3 ＋ quiet `CHANGEFLAG_REFR_CELL_CHANGED` @0xA39B20 ＋ロード側 bit31 分岐 2 件（0x4647A6/0x46630D）
   - `Dropped Item Flag(%i)`: quiet `CHANGEFLAG_..._DROPPED_NON_QUEST_ITEM` @0xA39730 ＋ **保存側 pass2 `test ebx,0x20000` @0x461E67（formType 0x31 前置・452DF0 force remove）を新発見** → 本表の「S セットに 0x20000 不在」は**誤りと訂正**（保存側対応分岐が実在）
2. **ゲート形式不一致 7 ラベル → 解消 [確定]**: 7/7 が**二段ゲート**（第 1 段 formType `cmp al` → 第 2 段 changeFlags `test dl/edx`）であり、D 表の flag 読みを正とし、probe 行の typeID 呼称は型ゲートへ読み替え。行判定は保存側値の多義が残るため [部分確定] 維持。
3. **bit3 二義 → 同一 changeFlags dword 上の文脈依存多義 (a) を採用 [確定]**、(b) 別物説は [確定負]。証拠 3 段連鎖: 12B レコード +5 → 452CF0 arg2（call site 0x46681B-0x466829）→ pass2 ebx ← 452600 value[0] → formatter 入口 `call 0x4535A0`@0x453AD0 の返値 [esp+0x170]。
4. **更新後の判定内訳（80 ラベル）**: [確定]4 / **[部分確定]68** / [推定]1 / **[未対応]0** / [確定負]7。件3（同値多義 13 組）は [部分確定] 維持。
5. 訂正更新案 13 項の詳細は `v312_phase2_gate_verification.md` の **§1 各項目の「裁定更新案」＋§2.3 サマリ**に記載（本ファイルには適用せず提示のみ＝Phase1 成果物の無改変方針）。新規残点 10 項（R1-R9＋R6'・bit7(0x80) 新多義含む）は #79 へ。

### 次束E（v315）追加裁定（2026-11-07）

`v315_v312_residuals.md`（31,900B）により本表をさらに更新する:

1. **row63（0x10000 組）ゲート列の誤帰属を訂正**: `%s %08X(%i)` ブロックの実ゲートは **bit17 @0x4576D3**、`Non-saved Package` は **bit16 @0x457853**、`0x457718` の `0x10000` は **getter のマスク選択**（ゲートではない）。[確定]
2. **row30（`0x2000000` ANIMATION「一意」）は FALSE** — `v3282_9` 相当 artifact の再検証で `459AF0` の `0x2000000` は一般ペイロード整形分岐、formatter 側の `0x2000000` は全域 1 サイト `0x4557AA`=ANIMATION 専用 → **row30 は [確定]→[部分確定] へ格上げ**（「一意」記述を撤回）。[確定]
3. **§5-4（同値多義）に bit7(0x80) を追記**: Owner=`test dl,dl / jns` @0x4566D3（区画 formType 0x31）↔ Crime Gold=同 @0x457EEF（type 0x32 インライン）＝**formType で分離**、getter は共通 `push 0x80`。調査可能帯の保存側 `test 0x80` は 0 件＝残件は v313 帯（#80 継承）。[部分確定]
