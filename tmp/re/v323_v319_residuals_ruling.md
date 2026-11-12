# v323_v319_residuals_ruling.md — 次束K（ai-todo #87）: v319 新規残件 10 件の裁定（L1・静的解析）

- 作成: 2026-11-07（サブエージェント実行・L1 報告モード／統合は親セッション）
- 対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam、ImageBase 0x400000）
  - セクション: .text VA 0x401000 → file 0x400 (0x626E00) / .rdata VA 0xA28000 → file 0x627200 (0xD9A00) / .data VA 0xB02000 → file 0x700C00 (0x30C00)。`F(va) = foff + (va - sva)`（`%TEMP%\opencode\v307_common.py` 契約・読取専用・変更なし）
- 母集団: `tmp/re/v319_v317_static_residuals.md` §7 の新規残件 10 件（配布指示の「§6 10件」は本 §7 を指す。ess 依存の §6 E1-E4 は本タスク対象外・解析しない）
- 必読踏襲: v319 全文（重複調査せず）／`tmp/re/v318_v314_residuals.md` 全文（項目7 の整合確認用）
- 判定: 5 段階 **[確定] / [部分確定] / [推定] / [未確定] / [確定負]**。旧結論は削除せず §5 訂正節で併記
- 実 `.ess` は不在（v313 §1.1 踏襲・再検索なし）→ 動的依存は [未確定] のまま
- クリーンルーム: OpenTES4 / xOBSE / COEF 参照なし（本リポジトリの docs/tmp のみ）
- 成果物: **本ファイルのみ新規作成**。既存 tmp/re 成果物・docs・STATE・TODO（ai-todo）・GT・ソース・git は無編集
- 中間物: `%TEMP%\opencode\v323_01..12 系 .py/.out`（19 本・すべて再現可能、リポジトリ側は本 md のみ）

---

## 1. 判定表（10 件・5 段階）

| # | 残点（v319 §7） | 判定 | 一句话裁定 |
|---|---|---|---|
| 1 | FUN_00463700→0x4637C0 名称確定後の残り（fn 0x464440 本名等） | **[部分確定]** | **fn 0x464440 = 0x464440..0x464906 [確定(命令)]・挙動=ワールド再初期化＋ワールドスペース毎の LOD land 初期化 [確定(命令+文字列)]・正式名 [推定]**（名称文字列は 'Initializing LOD land…' のみ＝固有名なし [確定負]）。FUN_00463700 本名は [未確定] 維持（文字列0件・tail 0x4F9DD0 は帯外照会不可） |
| 2 | 45BB30 / 0x4DD4B0 正式名 | **[部分確定]** | 45BB30 本体の機械文字列走査=0件 [確定負]、4DD4B0 窓の機械走査で **TESObjectREFR 配置/3D 系エラー文字列群**（'Corrupt location found on reference…' 等）を検出 → 分類 [推定] 強化。正式名はいずれも文字列・スロットなし → **[推定] 維持**（45BB30=再配置ヘルパ／4DD4B0=親セル決定・接続） |
| 3 | FUN_005F0310 正式名 | **[未確定]** | 名称の裏付け（文字列0・.rdata/.data ポインタ0・.text imm32=0）を再確認 [確定負]。一方で**本体全文（0x5F0310-0x5F0400）と RTTI TD 名（0xB14914='.?AVActiveEffect@@' / 0xB14930='.?AVReanimateEffect@@'）で分類を [推定]→[確定(命令)] に格上げ**。提案呼称 `IsRuntimeChangeTarget` は非正典のまま |
| 4 | getter 厳密語義（GetSaveSize 系仮想・基準 word の出所） | **[部分確定]** | **基準 word = formatter 先頭 0x453AFC の `push 0` → vtable+0x4C getter → `mov [esp+0x14],ax`（窓内の書込みはこれのみ）＝getter(mask=0) の戻り値 [確定(命令)]**。**%i = getter(mask) − getter(0) = mask 指定 changeFlag フィールド群のセーブ時バイトサイズ（差分）[確定(命令)]**（ラベル52種 'Form Flags(%i)'/'Owner(%i)'…）。**0x45a170 = 現在 version∈[0x1F,0x5A) 述語 [確定(命令)]**（+6 条件の正体）。0x659be0（Actor 基礎値）は帯外照会不可 → 残件 |
| 5 | ロード側レコードフレーム fill 源 | **[部分確定]** | **fill 源 = LoadGame 内 0x465C94 の間接 read 呼び出し [確定(命令)]**（`push 0xc`=12B・`lea` でループトップ esp+0x1C/+0x20 の2ポインタ・esi=**[ebp+0x40]**＝SaveGame 成員 reader）。直後 `mov edi,[esp+0x34]`＝**読取 formID**（偽なら 'Load Error: Plugin for form with ID %08X…' 0xA3B3F8）→0x465D40 で解決 form ptr を frame+0 へ。**[未確定] 残り: [esi+4] reader の正体・arg スロットの1スロット差（5 push + `add esp,0x18`）の語義** |
| 6 | 投入ルーチナ（0x45E0EF-0x45E141）の上位フェーズ | **[部分確定]** | 投入=find-or-add∈**FUN_0045DE30**（文字列 'Enchantable item…created base objects list.' で created base objects 検証 [確定(文字列)]）← **唯一 caller 0x4627E1 ∈ FUN_00462080**（直後に `shr ecx,9; test cl,1`＝**bit9(dry) テスト** [確定(命令)]）。fn0x462080 = セクション処理＋保存バッファ構築系（'Save Game Header'/'TES Class'/'Process Lists Class'/'Spectator Events'/'Sky/Weather'/'Quick Keys'/'cannot save'/'save buffer' 文字列）で **callers=3: 起動 0x40F425 / fn0x464440 内 0x4644AA / LoadGame 内 0x465B12** [確定]。load 側呼び出しで保存系分岐が走るか（mode 判定）は未追跡 → 残り |
| 7 | 45AF0 上位（v318 禁止帯照会 → v318 完了済み） | **[部分確定]** | **v318 成果物との矛盾なし [確定]**（v318 の5項目=0x502500 表/0xB33E9C/0x44DCF0/間接call595/rule v3 はいずれも本件と交差せず、むしろ rule v3 の短 CC head 規則と同系統で整合）。上位 callers の浅層: **0x44F058**（0x4d6760(0xb33ac4) の al ゲート→0x44EE00。包含 head は find_head=0x44EDE0 が CC 不足型誤答で [未確定]＝v318 帯深解析は #89 専有→照会不可）／**0x507896**（head 0x507800・[0xb33a98] を this に0x44EE00→0x4f0620・fn 尾に 'All map markers %s'/'Verbose messages %s' → **コンソール/デバッグ系ハンドラ [推定(文字列)]**）／**0x5BA629**（find_head=0x5BA4D0・帯外照会不可）。上位フェーズは「コンソール系＋帯外1件」まで |
| 8 | bit7 リーダ帯外探索 | **[確定負]**（＋副次 [部分確定]） | **全 .text の機械走査（境界検証済）で test/cmp/bt 型の `[reg+0x18]` bit7 リーダ = 0 件 [確定負]**。`and dword [r+0x18],0xffffff7f`（書手側復元）は3件のみ（0x4639A9 / 0x4445B5 帯外 / 0x4CDA79 #91）。読み込み→test/and/shr≤10命令型も帯外 CRT 系3件（0x722AFA/0x7AD813/0x7DA757）のみ。**副次: [0xB33B00+0x18] の第2 set/restore サイト 0x4CDA74-0x4CDA79 を新規検出 [部分確定]**（v319 は許可帯のみ探索のため未検出）。v316 530件カタログとの bit0/6/9/12/18 対応は未整理 → 残件 |
| 9 | find_head の CC ラン不足型誤答 5 例の体系化 | **[確定]** | **5例（+1例追加=6例）の検証表・誤答再現・検出手順を確定**（§4-9）。全例: 16アライン・CC k∈{0,1,2,3}・前 fn が `ret imm`/`ret`/尾 jmp で終端 → find_head(interior) が直前の別 head を返す（MISMATCH 6/6）。**v318 rule v3 の fn-head 規則は終端を C3/C2 のみ規定 → 尾 jmp（E9/EB）終端の 0x4637C0 型が通過しない → E9/EB 追加の拡張提案**（本件の体系化成果）。`re_util.py` 変更禁止のため手順は文書化のみ |
| 10 | docs/旧記載との不整合 5 件（v319 §10）の反映判断 | **[確定]** | **docs は無編集・候補整理のみ**。5件の判断: 要反映1件（RE_EXE_REPORT.md line9927 の `0x463700`/`0x460BC0` bit7ラッチ → FUN_004637C0(LoadForm) 訂正）／不要3件（typeID・Level 系 getter・0x464440 境界はいずれも正典 docs に該当記載なし）／任意追記1件（line9816 に find-or-add 細部）。**新規検出として6件目を追加: line9923「0x5F0310 は vtable +0x4C getter を使う述語」はコードと不整合**（5F0310 全文に +0x4C アクセス0件） |

### 1.1 5 段階集計（10 件）

| 判定 | 件数 | 項番 |
|---|---|---|
| [確定] | **2** | 9, 10 |
| [部分確定] | **6** | 1, 2, 4, 5, 6, 7 |
| [推定] | **0** | —（副次推定は各項内） |
| [未確定] | **1** | 3 |
| [確定負] | **1** | 8（＋副次 [部分確定] 第2サイト） |

（合計 10 件）

---

## 2. 使用 probe 一覧（19 本・全 EXIT=0）

実行形: `chcp 65001` → `python $env:TEMP\opencode\v323_<n>.py *> $env:TEMP\opencode\v323_<n>.out`。依存は `%TEMP%\opencode\v307_common.py`（読取専用）。`tmp/re/re_util.py` 不使用（変更なし）。raw E8=`va+5+rel==target`（callers_of）。

| script | 対応項 | 内容 / 主結果 | out (B/L) |
|---|---|---|---|
| v323_01 | 1 | fn0x464440 head/線形デコード（call 列・文字列）＋callers 3件文脈＋463700 head/文字列＋tail 0x4F9DD0 VA | 3180/101 |
| v323_02 | 2 | 45BB30 窓文字列0件＋キーワード文字列 hunt＋**v319_01.out 再利用**（4DD4B0/4D8A10/4D8A30 本文窓）＋callers 帯分類 | 2439/66 |
| v323_02b | 2 | 'parent cell'/'MoveTo' 全文＋refs=0＋**4DD4B0..4E0F80 機械 .rdata ポインタ走査**（REFR 配置系文字列群） | 2942/42 |
| v323_03 | 3 | 5F0310 head/先頭 decode＋文字列0＋.rdata/.data ポインタ0＋callers 文脈＋5F0410 callers=21 | 2875/90 |
| v323_03b | 3 | **5F0310 全文（0x5F0310-0x5F0400）**＋callee＋sibling 窓＋5F0410 decode | 5120/194 |
| v323_04 | 4 | formatter 窓 [esp+0x14] 全アクセス＋**+0x4C getter ディスパッチ39窓**＋push 0x80＋0x45a170 全文＋46AC30/50 境界＋0x659be0 VA | 22088/705 |
| v323_04b | 4 | si 更新全列挙＋**format ラベル52種の文字列抽出**＋base 分布（[esp+0x14]=32 / si=8） | 2881/62 |
| v323_05 | 5 | LoadGame head＋0x465C40-0x465D00 窓＋frame lea/書込み列挙 | 4893/164 |
| v323_05b | 5 | **整列デコード 0x465BF0-0x465D60**（0x465C94 read 全体）＋esi 供給元 [ebp+0x40]＋jne 先＋frame 消費点29箇所 | 6595/215 |
| v323_06 | 6 | FUN_0045DE30 head/prologue＋callers=1(0x4627E1) 文脈＋文字列 'Enchantable item…' | 1385/46 |
| v323_06b | 6 | fn0x462080 head＋**callers=3 文脈（0x40F425/0x4644AA/0x465B12）**＋セクション文字列群＋464440 内 462080 呼び出し | 1959/60 |
| v323_07 | 7 | 0x44F058/0x507896 浅層文脈＋head先頭＋0x5BA629 VA＋chain 再確認（callers 5系統） | 2744/83 |
| v323_07b | 7 | 0x44EE00..0x44F060 の16アライン候補全走査（cc=0/callers=0 のみ）＋0x507800 callers＋0x507800 窓文字列 | 3548/54 |
| v323_08 | 8 | **bit7 リーダ familyA（test/cmp/and/bt/shr メモリ型・境界検証）全 .text = 3件**＋familyB＋0x4CDA79 文脈 | 1942/49 |
| v323_08b | 8 | familyB 修正版（shr/sar 7 追加）=3件（全帯外）＋0x4CDA79 整列文脈＋0x4445B5 VA | 1108/24 |
| v323_09 | 9 | **CC 不足型5例の検証表・境界バイト・誤答再現（MISMATCH 5/5）・終端規則確認** | 3015/65 |
| v323_10 | 1/4/3/5 | 关键文字列（0xA3B3F8/0xA3B1A0 等）＋**RTTI TD 名8種**＋LoadGame ret/heads＋si 書き込み全列挙＋fn0x464440 の ret/文字列 | 3441/106 |
| v323_11 | 1/6 | 0x464700-0x464B40 head 走査（**0x464910 cc=10 / 0x464AB0 cc=3**）＋fn0x464440 尾エピローグ＋'TES Class'/'cannot save' 文脈＋'TES4SAVEGAME' 文脈 | 4252/152 |
| v323_12 | 1 | callers(0x464910)=0x4CDA22 / callers(0x464AB0)=0x465228 文脈＋LoadForm@0x46494A 文脈＋fn0x464440 終端 bytes | 2256/77 |

---

## 3. 各項の根拠（VA/命令/caller-callee）

### 3-1. 項1: fn 0x464440 の同定と範囲 — [部分確定]

**(a) 範囲の確定（v319 の窓を拡張・訂正を伴う）**
- head: `func_head(0x464440)=True`（cc-run=5、pre16 末尾 `83 c4 28 c3 cc×5`＝前 fn が `ret` で終端）。
- **終端: `ret 4`@0x464903**（エピローグ `mov ecx,[esp+0x58]; mov fs:[0],ecx; pop ecx/edi/esi/ebp/ebx; add esp,0x50; ret 4`＝SEH 関数）→ CC pad 10B → **head 0x464910（cc=10・callers=1）**。
- → **fn0x464440 = 0x464440..0x464906 [確定(命令)]**。v319 が同定した内容（463700@0x464499/0x4646D9、462080@0x4644AA、459870@0x4644B1、'Initializing LOD land' 系）はすべてこの範囲内 ✓。
- **訂正（v319 §7-9 の「fn0x464440 内 LoadForm 0x46494A」）**: 0x46494A は **fn0x464910（0x464910..0x464AAA・callers=1:0x4CDA22）内**。文脈: `shr eax,0x12; and al,1; test; jne 0x464942`（bit18 ゲート）→ `mov ecx,ebx; call 0x4637c0(LoadForm)`。v319 は線形デコードを 0x4646F0 までしか行っておらず ret@0x464903/head@0x464910 を見逃していた（§5-R1）。

**(b) 文字列（名称の唯一の根拠）**
```
0x464513 -> 0xA2FFB8 'Data\\'
0x464610 -> 0xA3B184 'Initializing LOD land...'
0x464662 -> 0xA3B158 "Initializing LOD land for worldspace '%s'"
```
- 固有関数名文字列は **0 件 [確定負]**（0x464440..0x464906 全域の imm32 文字列走査）。

**(c) callers=3 と文脈**
| site | 帯 | 文脈 |
|---|---|---|
| 0x40F3C1 | 帯外（浅層・下記宣言） | `mov ecx,[0xb33b00]; push 1; call 0x45a530; push 0; call 0x464440` → `mov ecx,[0xb333c4]…vfunc+0x150`（起動/新規系パイプライン） |
| 0x466889 | v319帯（深） | `push 1; mov ecx,ebp; call 0x464440` → `mov ecx,[0xb33a10]; call 0x432890`（LoadGame 内） |
| 0x500CBB | v318帯（浅層） | 0x40F3C1 と**同型**（`call 0x4533f0`→`push 1; call 0x45a530`→`push 0; call 0x464440`）。0x500CBB は Op0162 impl 0x500C90（size 160B・docs opcode 表 row226）内 |

**(d) 呼び出し列（0x464440..0x464740 窓）**: `call 0x463700 ×2 / 0x462080 / 0x459870 / 0x4599b0 / 0x46c300 / 0x44f3d0 / 0x45a530 / 0x6ecc80 / 0x447d80 / 0x582160 / 0x57ecb0 / 0x67f180 / 0x67fcf0 / 0x43f6e0 / 0x43f560 / 0x4492e0 / 0x418e50 / 0x5b7150 / 0x5a6a80 / 0x44a2b0 / 0x66a740 / 0x404ee0 …`＋`call eax/edi ×2 each`。

**(e) FUN_00463700（残り）**: head ✓（cc=8・pre16 `50 c2 04 00 cc×12`）、callers=2（0x464499/0x4646D9＝いずれも fn0x464440 内 ✓）、**文字列0件**。尾 `jmp 0x4F9DD0` の先は **v319 帯（0x4D8000-0x4F2000）外＝帯外 → 本体照会不可**（callers(0x4F9DD0)=1:0x462B82 のみ機械記録）。→ 名称 [未確定] 維持。

**(f) 裁定**: 挙動は「グローバル諸システムのクリア連鎖（463700×2）＋セクション/解決テーブルクリア（462080/459870）＋ワールドスペース毎の LOD land 初期化＋'Data\' 操作」＝**ワールド再初期化＋LOD land 初期化 [確定(命令+文字列)]**。呼び手が「起動・LoadGame・コンソール Op0162」の3系統であることから、**正式名 [推定]（暫定呼称: `InitializeWorldLOD` 系・非正典）**。名称文字列なし [確定負]。

### 3-2. 項2: 45BB30 / 4DD4B0 の名称 — [部分確定]

- **45BB30**: head ✓（cc=6・pre16 末尾 `c2 0c 00`）。**機械 .rdata ポインタ走査（0x45BB30..0x45BDCF）= 0 件 [確定負]**。callers=3:
  - 0x45BE40（ResetObject 内・`test byte[esp+0x24],0xc / setne al / push eax / push ebx / call`＝arg2=(changeFlags&0xc)!=0）、
  - 0x461DE7（pass2・`push 0`、直後に `cmp ebp,0x32/0x33`＝ACHR/ACRE 判定）、
  - 0x4CFF74（#91 帯・VA のみ）。
- **4DD4B0（#91 帯・浅層）**: v319_01.out の本文窓（先頭92命令）に文字列0件。追加で**機械データ走査**（raw LE u32 in .rdata/.data・命令境界検証なし・文脈展開なし＝v318 の機械走査先例に準拠・帯遵守宣言）を 0x4DD4B0..0x4E0F80 に適用 → 28件。系統として一貫するのは **TESObjectREFR 配置/3D 系**:
  - `'Conflicting shared data removed from linked door reference.'`(0xA46F1C) / `'Corrupt location found on reference, setting to (0, 0, 0).'`(0xA471E0) / `'Corrupt angle found on reference…'`(0xA471A8) / `'..\TES Shared\TESObjectREFR.cpp'`(0xA4721C)
  - location printf 群: `" in Cell '%s' (%08X) (%i, %i)"` / `" in WorldSpace '%s' (%08X)"` / `"%s Form '%s' (%08X)%s%s%s"`
  - 3D/Havok 系: `'BSFaceGenNiNodeBiped'` / `'FlameNode'` / `'Havok data bone count differs on reference…'` / `'Cannot load Havok data for reference…'`
  - 注: 0x4E019B 以降の `'LoadGame Buffer error…'` 群は**帰属 fn 未特定**（v319 の bounds 0x4E0F80 は複数 fn 合算の疑い）→ 窓レベルの情報として扱う。
- **キーワード文字列 hunt**: `'ParentCell'/'SetParent'/'GetParent'/'Relocate'/'Reparent'` = 0件。`'parent cell'`(0xA3AD08/0xA3AD51/0xA3AE05)・`'MoveTo'`(0xA4F244) は **.text imm32 参照=0 件**（部分文字列＝アドレス不一致）→ 名称根拠に使えない [確定負]。
- **callers(0x4DD4B0)=28**: in-band(v319)=7（0x45BC08/0x45BCEA/0x45EA8B/0x45EB5B/0x46005E/0x460EF6/0x460F65）／#91=1（0x4DDF1A）／帯外=20（0x4B9477…0x68BD0C、VA のみ）。
- 裁定: 名称はいずれも**文字列・スロットなし [確定負] → [推定] 維持**（45BB30=再配置ヘルパ、4DD4B0=親セル決定・接続ルーチン）。4DD4B0 の系統分類は REF R 配置系エラー群により [推定]→中程度の補強。

### 3-3. 項3: FUN_005F0310 — [未確定]（分類は [確定(命令)] へ格上げ）

**(a) 全文（0x5F0310..0x5F0400・ret 4 @0x5F03F7/0x5F03FE・CC pad → head 0x5F0410 cc=8）**
```
5F0310: push edi; mov edi,ecx; cmp dword[edi+0x3c],0 / jne → xor al,al; ret 4   ; [3D]==0 → false
5F031F: test byte[esp+8],0x40 / je 0x5f0395                                      ; flags bit6
bit6 立ち: [edi+0xb0] ∈ {1,2,6} → true(0x5f03f3)
           [edi+0xb0]==4 → [edi+0x68] リスト走査:
             [eax+8] リスト・各要素を RTDynamicCast(0xb14930=ReanimateEffect / 0xb14914=ActiveEffect)
             → [ae+0x3c] >= 0x1e で true(0x5f03f3)
bit6 未/非該当 → 0x5f0395: [edi+0x58]（script）vfunc+0x2e4 を5回呼び {1,3,2,4,5} のいずれかで true
false: xor al,al; ret 4
```
- **RTTI TD 名（TD+8 読取）で v319 の主張を裏付け [確定]**: `0xB14914='.?AVActiveEffect@@'`、`0xB14930='.?AVReanimateEffect@@'`（他: 0xB02F9C='.?AVTESForm@@'、0xB03248='.?AVTESBoundObject@@'、0xB05538='.?AVMobileObject@@'）。
- → **分類 [確定(命令)]: 「3D 所持 ∧（(flags bit6 ∧ [form+0xb0]∈{1,2,6}) ∨ (bit6 ∧ [form+0xb0]==4 ∧ ActiveEffect/ReanimateEffect([ae+0x3c]≥0x1E) 所持) ∨ (script の vfunc+0x2e4 ∈{1..5})）＝実行時変更データ所持述語」**（v319 の [推定] を格上げ）。

**(b) 名称の裏付け（全て0件）**: 文字列（0x5F0280..0x5F0500）= `'Bip01 NonAccum'`(0xA64228@0x5F0291) のみ＝**sibling 別 fn のもの**（0x5F0280 は head ではなく mid-instruction。v319 の「heads=5F0310/5F0410 の2つ」と矛盾しない）。.rdata/.data への0x5F0310/0x5F0410 ポインタ=0 件、.text imm32 出現=0 件 → 非仮想の直接呼び出しのみ [確定]。
- callers=4（0x45376F: 真→`or ebx,8` bit3 再合成 / 0x45D296: map find→flags 適用 / 0x4DC22C: #91 浅層・真→vfunc+0x284(0xa)+0x4212E0 スキップ / 0x65B813: 帯外 VA）。
- 参考: sibling 0x5F0410（callers=21）= vfunc+0x380/0x388 getter・formType==0x24(CREA) 判定・PlayerCharacter 比較を含む大関数。
- → **正式名 [未確定] 維持**（提案呼称 `IsRuntimeChangeTarget(form, flags)`・非正典）。

### 3-4. 項4: vtable+0x4C getter の厳密語義 — [部分確定]

**(a) 基準 word の出所 [確定(命令)]**
- formatter は**単一関数**（head 0x453A90: cc=4・callers=2・窓 0x453A90..0x456780 に他の head なし）。
- 窓内の `[esp+0x14]` 書き込みは **2 箇所のみ**: `0x453AE4 mov [esp+0x14],0`（初期化）と **`0x453AFC mov edx,[edx+0x4c]; push 0; mov ecx,ebp; call edx` → `0x453B01 movzx ecx,ax; mov [esp+0x14],ecx`**。
- → **基準 word = getter(mask=0) の戻り値（u16）**。以降の39箇所の getter 呼び出しはすべて差分計算にのみ使用され、基準の再設定はない。

**(b) %i の厳密意味 [確定(命令)]**
- 全ディスパッチ窓の共通パターン:
  ```
  push <mask>            ; 0x1 / 0x4 / 0x8 / 0x10 / 0x20 / 0x40 / 0x80 / 0x400 / 0x1000000 / 0x2000000 / ... / 0x80000000
  mov ecx, ebp; call getter        ; ax = size(mask)
  movzx eax, ax / movzx ecx, [esp+0x14] (or si) / sub eax, ecx
  push eax / push 'Label(%i)' / call 0x98208b (sprintf)
  ```
- → **%i = getter(mask) − getter(0) = mask 指定 changeFlag フィールド群のセーブ時バイトサイズ（差分）**。
- ラベル52種を抽出（抜粋）: `'Form Flags(%i)'`(mask 1) / `'Quest Stages(%i)'`(0x10000000) / `'Quest Flags(%i)'`(0x8000000) / `'Owner(%i)'`(0x80) / `'Lock(%i)'`(0x40) / `'Script(%i)'`(0x400) / `'Inventory(%i)'`(0x8000000) / `'Cell Changed(%i)'`(0x80000000) / `'Created(%i)'` … / quiet 名 `'CHANGE_OBJECT_EXTRA_OWNER'`(0xA397C8)。最初の6窓は `'None(%i)'/'Low(%i)'/'Mid Low(%i)'/'Mid High(%i)'/'High(%i)'/'Base(%i)'`（detail 系）。
- `si` は scratch（quiet 経路の文字列 ptr `mov esi,0xa39xxx` 群・`movzx esi,ax`・**base の再ロード `mov esi,[esp+0x14]` を5回**（0x453D14/0x453DEC/0x453F27/0x454087/0x4541C7））。si ベースの差分8件＝base のコピー [確定(命令)]。`add eax,esi`@0x4555C4 の1経路のみ [副次・未確定]。

**(c) 0x45a170 の意味 [確定(命令)]**
```
45A170: mov eax,[0xb33b00] / mov al,[eax+0x7c]
        cmp al,0x1f / jb 0x45a183 / cmp al,0x5a / jae 0x45a183 / mov al,1 / ret
```
= **「現在セーブ version ∈ [0x1F, 0x5A)」述語**（callers=200超）。現在 version 0x7D では偽。getter の `+6` 条件＝この述語（旧形式時のみ +6 バイト）→ **+6 条件の正体も [確定]**。

**(d) 残る [未確定]**: `0x659be0`（Actor +0x4C の基礎値、callers=3: 0x5E7D0B/0x60831A/0x69F745）は**帯外 → 本体照会不可**。0x46AC30/0x46AC50 境界（浅層）は v319 記載と一致（`test byte[esp+4],1 → 4:0`）。

### 3-5. 項5: ロード側12B レコードフレームの fill 源 — [部分確定]

- **LoadGame = fn 0x465860..（最初の ret = 0x466AC3 `ret 0xc`）[確定(命令)]**。整列線形デコードで 0x465860..0x466AC3 に ret なし（ドリフトなし＝既知 VA 0x465C94/0x466688/0x46679C が v319 の独立デコードと一致）。`find_head(0x465AA1/0x465B12/0x465C94)=0x465860`。
- **fill 源 [確定(命令)]**（ループトップ esp = L）:
  ```
  465C7A: mov edx,[esi+4]            ; esi = [ebp+0x40]（0x465AA1 で代入）= SaveGame 成員の reader
  465C82: push ebx(1) / lea eax,[esp+0x20]→L+0x1C / push eax
  465C88: push 0xc (12B) / lea ecx,[esp+0x2c]→L+0x20 / push ecx / push esi
  465C90: mov [esp+0x30],ebx → L+0x1C に 1 を事前書込み
  465C94: call edx                    ; 12B read（間接）
  465C96: mov edi,[esp+0x34] → L+0x20 の出力
  465C9D: cmp edi,0xfeffffff / jne 0x465d04
  465D04: push edi / mov ecx,ebp / call 0x452180   ; form 解決
          0 → push 'Load Error: Plugin for form with ID %08X does not exist.'(0xA3B3F8) / call 0x4a7a60
  465D39: mov ecx,[esp+0x25] / push ecx / mov [esp+0x20],eax(解決 form ptr) / call 0x45a140
  ```
  → **edi = 読取 formID [確定(命令)]**、frame+0 = 解決 form ptr に上書き。同型 read がさらに2連（0x465CC4: size=1 / 0x465CDB: push 4）。
- **frame の後続消費（v319 レイアウトと整合）**: 0x466688 `lea edx,[esp+0x20]`→`[ebp+0x80]`（→0x460bc0）／0x46669C `[esp+0x21]`(+5)／0x466661・0x466674 `[esp+0x26]`(+0xa u16 tailSize)／0x46679C `[esp+0x25]`(+9 b3→tail header)／0x466858 `lea edx,[esp+0x1c]`→0x45ad00。ほか frame 書込み: 0x465CAB `fstp [esp+0x1c]`(fldz)、0x465D40。
- **[未確定] 残り**: ①`[esi+4]` reader の正体（[ebp+0x40] のクラス）②arg スロットの1スロット差（5 push だが `add esp,0x18`＝6 slots）と `mov [esp+0x30],ebx`（L+0x1C への1 事前書込み）の語義 ③arg2(L+0x20) 出力と「formID@+4 は type と衝突する」レイアウト問題（＝①が判明するまで両論）。

### 3-6. 項6: 投入ルーチナの上位フェーズ — [部分確定]

- 投入=find-or-add@0x45E0EF ∈ **FUN_0045DE30**（v319 踏襲 [確定]）。
- **FUN_0045DE30**: head ✓（cc=8）。文字列: **`'Enchantable item %08X with no enchantment exists in the created base objects list.'`(0xA3A960@0x45DFB0)** → created base objects 検証 [確定(文字列)]（docs §55.9 line9816「FUN_0045DE30 は created base objects の検証・保存経路」と**矛盾なし**）。
- **callers(0x45DE30) = 1: 0x4627E1 ∈ FUN_00462080**（head cc=4・pre16 `… c2 04 00 cc×…`）。
  文脈: `…call edx; add esp,0x14; push edi; mov ecx,esi; call 0x45de30; call 0x5c0d60; mov ecx,[esi+0x18]; shr ecx,9; test cl,1` → **bit9(dry) テストが直後 [確定(命令)]**（v316 の dry(bit9) 主張の命令裏付けでもある）。
- **FUN_00462080 の正体（文字列）**: `'Player has no worldspace or parent cell, cannot save.'`(0xA3AF68@0x46239D、PlayerCharacter [0xb333c4] の parent(0x4d6670)/worldspace(0x6ecc80) が両方0のとき) ／ `'TES Class'`(0xA3AF5C) ／ `'Could not create save buffer, out of memory.'`(0xA390D4、buffer=[esi+0x14] 確保失敗) ／ `'Process Lists Class'` ／ `'Spectator Events'` ／ `'Sky/Weather'` ／ `'Quick Keys'` ＝ **セクション処理＋保存バッファ構築系 [確定(文字列)]**。処理本体は `[edi+8]` 経由の5引数間接 call＋`0x4531b0`(セクション名ログ)＋`mov ecx,0xb02020; call 0x401e20`（バッファ取得）。
- **callers(0x462080) = 3**:
  | site | 帯 | 文脈 |
  |---|---|---|
  | 0x40F425 | 帯外（浅層・宣言） | 起動系: `mov ecx,[0xb33b00]; call 0x462080; mov ecx,[0xb33b00]; call 0x459870`（0x464440 内と同一ペア） |
  | 0x4644AA | v319帯 | fn0x464440 内: `mov ecx,ebp; call 0x462080; mov ecx,ebp; call 0x459870` |
  | 0x465B12 | v319帯 | **LoadGame 内**: 直前に `push 'Save Game Header'(0xA3B1A0); push ebx; call 0x4531b0`（セクション名ログ）＋`or dword[ebp+0x18],0x800` |
- 裁定: **上位フェーズ = FUN_00462080（セクション/バッファ処理系）← 起動・fn0x464440（ワールド初期化）・LoadGame の3系統 [確定]**。45DE30 の呼び出しは bit9(dry) 直後＝**保存系文脈 [確定(命令)]**。ただし fn0x462080 が load 側呼び出し（0x465B12）で保存系分岐を実行するかの mode 判定は未追跡 → 主判定 [部分確定]。
- v319 残件6 の問い「fn0x45DE30 自身の呼び手はロード/検証のどちらか」→ **解消: 唯一 caller は fn0x462080 内0x4627E1・bit9(dry) 直後＝保存（セーブ）フェーズ [確定]**。

### 3-7. 項7: 45AF0 上位（v318 照会）— [部分確定]

**(a) v318 成果物との整合 [確定]**: `tmp/re/v318_v314_residuals.md` 全文を読了。v318 の5項目（0x502500 表532 entries/impl 列0xB0AF60、0xB33E9C=タイマ+0x0C dt、0x44DCF0 の F1-F8、間接 call595 分類、rule v3 walker ラベル16/16）と本件の結論は**交差せず・矛盾なし**。むしろ v318 の「短 CC head 規則」は本件項9の問題系と同系統で整合。

**(b) chain 再確認（callers_of）**: `callers(0x44EE00)=3 (0x44F058/0x507896/0x5BA629)`、`callers(0x4CBD30)=1 (0x44EF19)`、`callers(0x459F30)=1 (0x4CBECC)`、`callers(0x459AF0)=2 (0x459F67/0x459F8E)`、`callers(0x45ADC0)=2 (0x4657E1/0x4669AC)`。

**(c) 上位 callers の浅層文脈（帯遵守: v318 帯は浅層のみ）**
- **0x44F058（0x44F000-0x452000 帯・浅層）**: `lea eax,[esp+0x38]; push eax; push esi; mov ecx,0xb33ac4; call 0x4d6760; test al,al; je`（=0x44F049）→ `mov edi,[esp+0x40]; mov ecx,[esp+0x3c]; push edi; push ecx; mov ecx,[esp+0x24]; push esi; call 0x44ee00` → 結果を `[esp+0x14]` へ。
  - `find_head(0x44F058)=0x44EDE0` は**CC ラン不足型の誤答**（0x44EDE0 は tiny fn（`push esi; mov esi,ecx; call 0x44d8e0; …; add esp,4; mov eax,esi; pop esi; ret 4`）で0x44EE00（SEH head）を挟んで 0x44F058 を包含しえない）。0x44EE00..0x44F058 の16アライン候補はすべて **cc=0・callers=0**（v318_07b 走査）→ 真の包含 head 特定は**深解析が必要＝v318 帯は #89 専有 → 照会不可 [未確定]（残件）**。
- **0x507896（0x500000-0x504000 帯・浅層）**: head=0x507800（callers=0）。文脈: `mov ecx,[0xb333c4](PlayerCharacter); call 0x6ecc80 → eax; mov ecx,[0xb33a98]; push eax; call 0x44ee00 → esi; test esi; je; push edi; mov ecx,esi; call 0x4f0620…`。fn 尾の文字列: `'All map markers %s'`(0xA4C08C@0x50791C) / `'Verbose messages %s'`(0xA4C0A0@0x50794E) / `'shown.'/'hidden.'` → **コンソール/デバッグ系コマンドハンドラ [推定(文字列)]**。
- **0x5BA629**: `find_head=0x5BA4D0`、**帯外 → 文脈照会不可 [未確定]**（VA のみ）。

**(d) 解釈**: 0x44EE00 は `[0xb33a98]`（459950/463700 が使う解決テーブル所有オブジェクト）を this として呼ばれ、**ID→form 解決系 [推定]**。上位の同定は「コンソール系ハンドラ＋帯外1件」まで [部分確定]。0x44F058 側の上位（0x4d6760/0xb33ac4 系）は #89 帯のため次束へ。

**(e) 「v318 完了済みで解放」の解釈（明記）**: 配布指示の帯遵守表が「v318 自帯＝#89 専有・浅層のみ」と明示しているため、**成果物照会（v318 md 読了）と浅層文脈展開を解放**とし、**深解析は #89 専有を優先**した（矛盾する2指示のうち、帯遵守表を優先）。

### 3-8. 項8: bit7 リーダの帯外探索 — [確定負]（＋副次 [部分確定]）

- **family A（メモリオペランド型: test/cmp/and/bt/shr × `[reg+disp8/disp32+0x18]` × imm 0x80/0x7f/7・全 .text raw走査＋各 hit を capstone 境界検証）= 3 件のみ**:
  | VA | 帯 | 命令 |
  |---|---|---|
  | 0x4639A9 | v319(深) | `and dword [esi+0x18],0xffffff7f`（LoadForm 復元・既知） |
  | 0x4445B5 | 帯外（VA のみ） | `and dword [eax+0x18],0xffffff7f` |
  | 0x4CDA79 | #91(浅層) | `and dword [eax+0x18],0xffffff7f` |
  - **`test`/`cmp`/`bt` 型の bit7 リーダ = 0 件 [確定負]**（`and …,0xffffff7f` は書手側 RMW 復元）。
- **family B（[reg+0x18] 読み込み→10命令以内の test/and 0x80・shr/sar 7・境界検証）= 3 件すべて帯外**: 0x722AFA（`mov cl,[esi+0x18]; shr cl,7`）/ 0x7AD813（`mov al,[esi+0x18]; shr al,7`）/ 0x7DA757 — いずれも 0x7xxxxx 静的リブ領域＝**照会不可（VA のみ）**。
- **新規発見（副次 [部分確定]）**: **0x4CDA6D-0x4CDA80（#91 帯・浅層文脈）**:
  ```
  4CDA6D: mov eax,[0xb33b00] / 4CDA72: je 0x4cda79
  4CDA74: or dword [eax+0x18],edi        ; SaveGame+0x18 へ mask=edi でセット
  4CDA77: jmp 0x4cda80
  4CDA79: and dword [eax+0x18],0xffffff7f ; bit7 復元
  4CDA80: mov edx,[esi+8]; shr edx,3; test dl,1; jne 0x4cdc81
  ```
  = **SaveGame+0x18 の第2 set/restore サイト**（v319 は許可帯のみ探索のため未検出）。`edi` の値は浅層で未特定。既知の第1サイト（0x463966/0x4639A9・LoadForm 内）と併せ、bit7 ラッチは**少なくとも2系統**。
- **限定（負の範囲）**: SIB エンコード形（rm=4）と、読み込み→bit 判定が10命令超／レジスタ経由で離れるケースは網羅外 → 負は上記 pattern 範囲付き。
- **v316 530件カタログ（[reg+0x18] 書手 or/and/xor/bts/add/sub）との bit0/6/9/12/18 対応整理は未実施 → 残件維持**。

### 3-9. 項9: find_head の CC ラン不足型誤答の体系化 — [確定]

**(a) 検証表（v323_09）**
| 真 head | align | CCラン | func_head | 直前終端 | find_head(interior) | 誤答 | callers(E8) |
|---|---|---|---|---|---|---|---|
| 0x4637C0 | ✓ | **0** | False | 尾 `jmp`(`e9 …`) が head 直前で終端・pad0 | find_head(0x463966)=**0x463700** | ✗ | 4 |
| 0x446B80 | ✓ | 2 | False | `c2 04 00`(ret 4) が A−2 で終端 | find_head(0x446BC0)=**0x446B20** | ✗ | 39 |
| 0x4D1EB0 | ✓ | 1 | False | `c2 04 00` が A−1 で終端 | find_head(0x4D1F00)=**0x4D1E40** | ✗ | 0（vtable 経由） |
| 0x5E7D00 | ✓ | 2 | False | `c2 08 00` が A−2 で終端 | find_head(0x5E7D80)=**0x5E7B90** | ✗ | 0（vtable 経由） |
| 0x44EE00 | ✓ | 2 | False | `c2 04 00` が A−2 で終端 | find_head(0x44EF19)=**0x44EDE0** | ✗ | 3 |
| **0x464AB0（本件追加6例目）** | ✓ | **3** | False | `c2 04 00` が A−3 で終端 | — | — | 1（0x465228＝FUN_00465130 SAVE オーケストレータ内） |

- 全例の共通型: **前 fn の `ret`/`ret imm`/尾 `jmp` で終端 → CC pad 0-3B → 16アライン新 head**。`func_head(minrun=4)` が pad<4 を不合格にし、`find_head` は**直前の別 head（場合によってはそのまた前）**を返す＝「偽ヘッド混入（off-by-pad7例）」の**逆系＝真ヘッドの見逃し**。
- 0x4D1EB0/0x5E7D00 の callers(E8)=0 は vtable ポインタ経由の参照（v319 の vtable_map.txt と整合）→ **外部証拠は E8 callers と vtable エントリの両方を見る必要がある**。

**(b) 検出手順（体系化・文書のみ。`re_util.py` 変更禁止）**
1. 照会 VA から16アライン候補を逆走査し、CCラン k<4 でも以下を満たせば **head と即断**:
   - (i) 直前終端が `C3` / `C2 imm16` / **`E9` / `EB`（尾 jmp）** で A−k に一致（pad = k×0xCC）、
   - (ii) 16アライン・先頭非 CC、
   - (iii) 独立 prologue（SEH: `6a ff 68 … 64 a1 00 00 00 00`＋cookie `xor [0xb30aac]`、または先頭で `[esp+4]/[esp+8]` を読む引数アクセス）、
   - (iv) 末尾の pop 数とフレーム整合（`pop` 群＋`add esp,N`＋`ret imm`）、
   - (v) 外部証拠: E8 callers>0 **or** vtable エントリ参照。
2. **検出器の併用**: `find_head(interior)` の結果が独立 prologue の頭と不一致なら誤答を疑う。
3. **v318 rule v3 の fn-head 規則への拡張提案**: rule v3 は終端条件を `C3`/`C2 imm16` のみ規定 → **尾 jmp（E9/EB）終端の 0x4637C0 型が通過しない**。`E9/EB` を終端集合に追加すべき（本項の体系化成果。rule v3 のラベル16/16 検証は v318 済みで本件と矛盾しない）。
4. 本件で検出した**新規 head 群（func_head=False だが実 head）**: 0x4637C0 / 0x446B80 / 0x4D1EB0 / 0x5E7D00 / 0x44EE00 / 0x464AB0 の6件を既知リストに追加（v319 の5例＋本件1例）。

### 3-10. 項10: docs/旧記載との不整合 5 件の反映判断 — [確定]（docs 無編集）

**docs 読取結果（`docs/RE_EXE_REPORT.md` / `docs/SAVE_LOAD_IMPLEMENTATION.md`・いずれも無編集）**

| v319 §10 の不整合 | 正典 docs の現状（grep/行確認） | 反映判断（候補） |
|---|---|---|
| 1. 「typeID」呼称 vs コード | **RE_EXE_REPORT.md に "typeID" 記載 = 0 件**（grep 確認）。line9926 は「保存レコード +9 byte」と呼称を置き換えて記載 | **docs 訂正不要**。任意候補: §55.12 line9926 に「+9 = version byte [推定]（v319 §5-C4）」を1行追記 |
| 2. v317 §4-7(c) ラッチ帰属 FUN_00463700 vs コード | **line9927 に「`FUN_0045BB30` の副作用、`0x463700`/`0x460BC0` の bit7ラッチ、各 helper の正式名は、実 .ess または追加の限定静解析が必要」＝不整合あり**（bit7ラッチの含有は FUN_004637C0 = TESSaveLoadGame::LoadForm）。line9914-9915 の VA「0x463966-0x4639A9 の一時ラッチ」自体は正しい（維持可） | **要反映（最優先）**: line9927 の「`0x463700`/`0x460BC0`」→「`FUN_004637C0`（`TESSaveLoadGame::LoadForm`）内の `0x463966-0x4639A9`」へ訂正。任意: 第2サイト 0x4CDA74-79 の追記 |
| 3. v317 §4-4(b)「Level 系 getter 隣接」vs コード | **該当記載なし**（line9850 の `0x5E1FD0`→`0x4677F0` レベル表示経路は別物でコードと矛盾なし） | **docs 訂正不要**（tmp/re v317 側の訂正のみ・v319 C3 で済） |
| 4. v317 §6-11「0x464440 窓境界は要再確認」vs コード | **境界懸念の記載なし**（line11444/12227 は0x464440を参照するのみ） | **docs 訂正不要**。任意候補: fn0x464440=0x464440..0x464906・LoadForm@0x46494A は fn0x464910（§5-R1）の新事実を §55.x に追記 |
| 5. v316 §6-9「[+0x74] 投入元未特定」vs コード | **§55.9 line9816 は既に「FUN_0045DE30 は created base objects の検証・保存経路」と記載＝矛盾なし** | **docs 訂正不要**。任意候補: find-or-add 0x45E0EF・唯一 caller fn0x462080@0x4627E1・callers 3件（§3-6）の追記 |

**新規検出（本件6件目・上記5件以外）**

| # | docs 記載 | コード | 判断 |
|---|---|---|---|
| 6 | **line9923-9924**: 「`0x5F0310` は vtable `+0x4C` getter を使う述語構造まで確定したが、正式名称は未同定 [部分確定]」 | 5F0310 全文（0x5F0310-0x5F0400）に **+0x4C アクセス = 0 件**（呼ぶのは formatter 0x453A90 系・0x4535A0 系）。5F0310 が呼ぶのは vfunc+0x2e4 / RTDynamicCast のみ | **不整合あり → 訂正候補**: 「0x5F0310 は bit6/ActiveEffect/Reanimate/script-vfunc 判定の述語（+0x4C getter は呼ばない）」へ。「正式名称は未同定」は維持で正しい |

※ いずれも**本 lane では無編集**。反映は親統合時の判断事項（docs 編集は禁止事項）。

---

## 4. 訂正節（旧結論は削除せず併記）

| # | 旧記載 | 出典 | 本件の裁定 | 根拠 |
|---|---|---|---|---|
| R1 | 「fn 0x464440 の…**内部で LoadForm 0x46494A**・解決テーブルクリア 0x4644AA・463700 2回呼出」 | v319 §7-9（§A6-7 の窓は 0x4646F0 まで） | **訂正: fn0x464440 = 0x464440..0x464906（ret4@0x464903＋CC10）。LoadForm 0x46494A は fn0x464910（0x464910..0x464AAA・callers=1:0x4CDA22）内**。4644AA/463700×2 は fn0x464440 内で従来どおり | v323_11/12（ret/epilogue bytes・head0x464910 cc=10 callers=1・0x46494A 文脈） |
| R2 | 「fn **0x44EE00** の prologue は `push esi; mov esi,ecx; call 0x44d8e0…`（…0x44EDE0 は直前の scalar-dtor）」 | v319 §4-6 | **訂正: `push esi; mov esi,ecx; call 0x44d8e0` は 0x44EDE0（tiny fn・`…; add esp,4; mov eax,esi; pop esi; ret 4`）の prologue。0x44EE00 本体は SEH prologue（`push -1; push 0x9add8e; mov eax,fs:[0]; sub esp,0x14`）** | v323_09 [2]（pre16/tail16）・v323_07 [1]（0x44EDE0 先頭6命令） |
| R3 | v319 §1-3「深帯 0x5E1000-0x5E2800（…5E7D00）」 | v319 | 注記のみ: 0x5E7D00 は 0x5E2800 **外**（帯表記の範囲外だが v319 は同帯として扱った）。本件では境界浅層（pre16/先頭命令）のみ実施 | v323_09 [2] |
| R4 | 「0x464440 窓 0x464440..0x46458A は部分窓（off-by-pad 候補ではない）」 | v319 A6-7 | **補強（矛盾なし）**: 窓は正しいが**終端は 0x464906**。find_head(0x4646D9)=0x464440 も正しい | v323_11 [3] |
| R5 | bit7 ラッチは「許可帯リーダ0件」 | v319 A6-2 | **拡張（矛盾なし）**: 全 .text で test/cmp/bt リーダ=0件 [確定負]＋**第2 set/restore サイト 0x4CDA74/79 を新規検出** | v323_08/08b |

（v318 既知との矛盾 = **0 件**：§3-7(a)）

---

## 5. 新規残件（次束・他 lane 用）

1. **[未確定] FUN_00463700 の正式名称** — 挙動（グローバルクリア連鎖）・callers=2（fn0x464440 内）とも [確定]、文字列0件。tail `jmp 0x4F9DD0` は帯外照会不可（callers(0x4F9DD0)=1:0x462B82 のみ）。
2. **[未確定] 0x44F058 の包含 fn head** — v318 帯（0x44F000-0x452000）深解析は #89 専有 → 照会不可。0x44EE00..0x44F058 の16アライン候補はすべて cc=0/callers=0（浅層走査済）。
3. **[未確定] 0x5BA629（find_head=0x5BA4D0）の上位文脈** — 帯外照会不可。
4. **[未確定] 0x659be0（Actor +0x4C 基礎値）の本体** — 帯外照会不可（callers=3: 0x5E7D0B/0x60831A/0x69F745）。
5. **[未確定] bit7/SIB 形・10命令超リーダの残余＋v316 530件カタログとの bit0/6/9/12/18 対応** — familyA/B は pattern 範囲付きの負のみ。第2サイト 0x4CDA79 の `edi` 値も未特定（#91 帯浅層留保）。
6. **[未確定] FUN_00462080 の mode 分岐** — load 側 callers（0x465B12）で保存系分岐（'cannot save'/'save buffer'）が走るか。セクション read/write の判別（[edi+8] 呼び出しの方向）。
7. **[未確定] 0x465C94 read の arg 語義と reader 正体** — [ebp+0x40] のクラス／`[esi+4]` の正体／5 push + `add esp,0x18` の1スロット差／`mov [esp+0x30],ebx`（L+0x1C への1）の意味／arg2(L+0x20) 出力と frame レイアウトの関係。
8. **[未確定] fn0x464910 の本名と上位** — callers=1: 0x4CDA22（#91 帯・浅層留保）。bit18 ゲート付きで LoadForm を呼ぶ「変更適用系」[推定]。
9. **[未確定] 正式名称（文字列・スロットなし [確定負] 維持）**: FUN_0045BB30 / 0x4DD4B0 / 0x4D8A10 / 0x4D8A30 / FUN_005F0310。
10. **docs 反映（親統合時・本 lane 無編集）**: 要反映=RE_EXE_REPORT.md line9927、新規不整合=line9923、任意追記=line9926(+9 version byte)/line9816(find-or-add 細部)/fn0x464440 範囲。
11. **[副次・未確定]**: formatter の `add eax,esi`@0x4555C4 経路の %i 語義／0x4445B5（帯外）の対象オブジェクト。

---

## 6. 帯遵守の自己宣言

- **深解析した自帯（v319 自帯のうち本件で使用）**: `0x452000-0x467920`（45BB30/45BDE0 窓・459950 系文脈・45DE30/45E0EF・462080 系・464440/464910/464AB0・465860 LoadGame 全域・453A90 formatter 窓 0x453A90-0x456780・45a170・452180/4531b0/45a140 呼び出し文脈）／`0x5F0000-0x5F1000`（5F0280-5F0500・5F0310 全文・5F0410 窓）。`0x430000-0x431000`・`0x5E1000-0x5E2800` は該当なし。
- **浅層照会（10-20命令文脈・境界バイト・機械走査）**:
  - **v318 自帯（#89 専有のため浅層のみ）**: 0x44F058 文脈・0x44EDE0/0x44EE00 head 境界・0x44EE00..0x44F060 の16アライン候補走査（機械）／0x507896 文脈・0x507800 head・0x507800..0x507960 文字列／0x464910 caller 文脈（0x4CDA22 直近は #91）。
  - **0x4C0000-0x4E0000 深帯（#91 専有のため浅層のみ）**: 0x4DD4B0..0x4E0F80 の機械 .rdata ポインタ走査（データ走査・文脈展開なし）／0x4CDA79 整列文脈16命令／0x4CDA22 文脈10命令／0x4D1EB0 境界バイト＋先頭命令／0x4CFF74・0x4DDF1A は VA のみ。
  - 0x44EF19・0x4CBECC・0x45BE40 等の既知浅層は v319 踥襲。
- **帯外（VA/callers のみ・文脈照会不可）**: 0x4F9DD0、0x659BE0、0x4445B5、0x5BA629/0x5BA4D0、0x65B813、0x4212E0/0x41F7F0、0x66xxxx-0x69xxxx 系 callers、0x722AFA/0x7AD813/0x7DA757。
- **逸脱の明記（最小限・深解析に非該当）**: **0x40F3C1 と 0x40F425（帯外・起動系 caller site）の文脈を10-20命令機械展開した**（callers 文脈の同一パターン確認のため。VA 記録のみという v319 方針からの逸脱であり、深層への入りはしていない）。0x500CBB/0x500CBB 系は v318 帯内＝浅層許容。
- **機械走査（命令境界検証を伴うデータ走査＝深解析に非該当・v318 先例準拠）**: .text 全域 E8 callers_of／bit7 パターン raw 走査（familyA/B・各 hit を capstone で境界検証）／.rdata/.data の u32 ポインタ走査（4DD4B0 窓・5F0310/5F0410）／RTTI TD 名の +8 読取。
- ESM/GT/ソース/STATE/TODO/LOOP/docs/git は**一切無編集**。ai-todo 操作なし（契約遵守）。`re_util.py` 変更なし。実 .ess 未接触（B 分類は解析せず）。

---

## 7. 再現手順と selftest

1. 環境: Windows / python 3.11（capstone 5.0.7・pefile）/ EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`。
2. 依存（読取専用・変更禁止）: `%TEMP%\opencode\v307_common.py`。補助読取: `%TEMP%\opencode\v319_01.out`（項目2 で再利用）、`tmp/re/v318_v314_residuals.md`、`docs/RE_EXE_REPORT.md`（項目10・無編集）。
3. 実行（19本）:
   ```powershell
   chcp 65001
   01,02,02b,03,03b,04,04b,05,05b,06,06b,07,07b,08,08b,09,10,11,12 に相当する:
   python $env:TEMP\opencode\v323_<n>.py *> $env:TEMP\opencode\v323_<n>.out
   ```
4. 解析要点（本件で実効した教訓）:
   - **fn 範囲は「ret＋CC pad＋16アライン head」のセットで確定**（fn0x464440=0x464906 終端・head0x464910 cc=10 のように、線形デコードの窓を広げないと見逃す）。
   - **CC ラン不足型 head は「直前終端（C3/C2/E9/EB）＋独立 prologue＋外部証拠」で補完**（項9）。E8 callers=0 は vtable 経由の可能性（0x4D1EB0/0x5E7D00）。
   - **formatter の基準準値は「窓内の書き込み点を全列挙」で出所が特定できる**（[esp+0x14] 書き込み=2件のみ）。
   - **RTTI TD 名は TD+8 の読取で得られる**（0xB14914='.?AVActiveEffect@@'）。
   - 帯外は**機械走査（VA/パターン）に限定**し、文脈は浅層規則に従う（逸脱は明記）。
5. **selftest**: 全19本を再実行 → **run2 failures=0 / MD5 差分=0**（1回目の出力と完全一致＝現行状態で決定的）。実行エラー0件（Traceback なし）。

---

## 8. 完了報告サマリ

- 成果物: `tmp/re/v323_v319_residuals_ruling.md`（本ファイル・1点のみ新規）
- 判定内訳: **[確定]2（項9,10）/ [部分確定]6（項1,2,4,5,6,7）/ [推定]0 / [未確定]1（項3）/ [確定負]1（項8・副次[部分確定]）**＝10件
- 主要裁定: ①fn0x464440=0x464440..0x464906・LOD land 初期化系 [推定]（LoadForm は fn0x464910 へ訂正）②45BB30/4DD4B0 名称は文字列なし [確定負]・分類補強 ③5F0310 分類は TD 名で [確定(命令)]・名称 [未確定] ④基準 word=getter(mask=0)・%i=差分バイト数・0x45a170=version∈[0x1F,0x5A) 述語 ⑤fill 源=0x465C94 の12B 間接 read（reader 正体は残）⑥投入の上位=fn0x462080（created base objects・bit9 dry 直後）⑦v318 矛盾なし・上位はコンソール系まで ⑧全 .text で bit7 リーダ0件＋第2サイト0x4CDA74/79 ⑨CC 不足型6例を体系化し rule v3 に E9/EB 拡張を提案 ⑩docs 反映=要反映1件＋新規不整合1件（無編集・候補のみ）
- probe: 19本（`%TEMP%\opencode\v323_*`）全 EXIT=0・selftest MD5 差分0
