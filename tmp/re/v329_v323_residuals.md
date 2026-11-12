# v329 — #93 次束P: v323 §5 残件11件の裁定（親直接解析・L1 レポートのみ）

- 検体: EXE（`re_util.EXE`）・v320_sample.ess 836,742B MD5 `FC32EFAEB90D26141D3DC4A325AC10E7`（本件では読みのみ）
- 出典: `v323_v319_residuals_ruling.md` §5（items 1〜11）・§6 帯遵守先例
- 実行 probe: `v329_A.out`（①②⑧先行）／`v329_B_old.out`（5窓・旧版クラッシュ残骸＝有効窓のみ採用）／`v329_C.out` `v329_D.out`（⑪）／`v329_E.out`（E0/E3/E4/E6・E5/E8 は不完全→F で差し替え）／`v329_F.out`（②③⑤⑥⑦⑧中核・E5 修正版）／`v329_G.out`（⑥⑦最終詰め）
- 判定語彙: [確定]/[部分確定]/[推定]/[未確定]/[確定負]（旧結論は削除せず訂正節 §3 で併記）

---

## 1. 11 項裁定表

| # | 残件（v323 §5 原文要約） | 本件裁定 | 根拠（probe） |
|---|---|---|---|
| 1 | FUN_00463700 の正式名称 — 挙動/callers は [確定]・文字列0件 | **正式名称は [確定負]**（文字列0・データポインタ0・vtable スロット0）／挙動（グローバル3個クリア連鎖・tail `jmp 0x4F9DD0`・唯一 caller 0x462B82）は [確定] 維持 | A1（callee 30 件＋tail fn 5命令・callers=1）／E0（非 .text dword ptr 0 hits） |
| 2 | 0x44F058 の包含 fn head（v318 帯 #89 専有で照会不可→**完了済のため深可**） | **包含 head = 0x44EE00 [確定]**。0x44F058 は **fn 0x44EE00 自身の再帰 retry ループ内**（0x44EFB0..0x44F058）。本体構造: SEH prologue（`push -1; push 0x9add8e`）・`[0xb33ad4]` bit0 once-gate・`0x44d720(0x25)` 呼び・`[0xb3f9a8/ac/b0]` formID 3つコピー出力・`0x4c97f0`/`0x4ef130` フィルタ・LinkedList 走査（`shr 3`/`shr 0xb` 除外ゲート）・`[0xb33ac0]` インクリメント | F4（head=0x44EE00・窓 0x44EE00..0x44F120 全文字列付）／E3（callers(0x44EE00)=3: 0x44F058/0x507896/0x5BA629） |
| 3 | 0x5BA629（find_head=0x5BA4D0）の上位文脈（帯外照会不可） | **機械 census で部分解決**: callers(0x5BA4D0)=4（0x5BB1F9→head 0x5BB1B0・0x5BB310→head 0x5BB210・0x5BB60E/0x5BB6AD→head 0x5BB350）。ctx 窓は `cmp eax,5/2/1` モードディスパッチ構造・近縁文字列 'Icons'/'%s\%s'/'Data\Menus\Main\map_menu.xml' → **UI/メニュー系リロード経路のモード1分岐**と読み取れる。0x5BB350 の fn 名・領域意味は [未確定]（帯外） | F5（callers 4 サイト・境界 head 3 個＋各 12-14命令窓・strings） |
| 4 | 0x659BE0（Actor +0x4C 基礎値）の本体（帯外） | **本体 [確定]**: `659BE0(this=ecx, arg)` = `u16( 0x4E2BF0(this,arg) + 1 ) + ( [this+0x58] ? vtbl[+0x3f0]([this+0x58], this, arg) : 0 )`。0x4E2BF0 head 14命令 = `0x46AC50(this,arg)`→`movzx ax`＋0x45A170（[0xB33B00] 経由）。**0x46AC50 = 引数 byte bit0 → 4/0 の getter**（`xor eax,eax; test byte[esp+4],1; mov eax,4; ret 4`）。意味語（+0x4C 基礎値の組立）は [部分確定]（vtbl+0x3f0 の先は帯外不追及） | E4（0x659BE0 全文・callers=3 復唱）／F7（0x4E2BF0 head・帯境界宣言）／G7（0x46AC50 全文 7命令） |
| 5 | bit7/SIB 形残余＋v316 530件カタログ bit0/6/9/12/18 対応＋第2サイト 0x4CDA79 の edi 値 | **edi = 0x80 [確定]**（0x4CDA41 `mov edi,0x80`・B_old 4CDA20 窓）。**imm 形 RMW 全 .text 表**（F1 修正版・modrm 0x40-0x7f＋SIB）: bit7 clear = **0x4445B5/0x4CDA79 の 2 サイトのみ**、bit0 clear 1（0x45F285）、bit18 clear imm 1（0x45F28B）、bit16 set 2（0x45CC0B/0x45DDC9）/clear 2（0x45CC34/0x45DDF5）、bit5 set 9・bit6 set 1・bit9 or 2（帯外 0x55xxxx）・test imm32 形 **0 件**。ただし **`[ebp+0x18]`（SIB base=5）形は F1 除外** → LoadGame（0x465860）内の実ペアを F2 で直接列挙: or bit0@0x465AE4・or bit18(0x40000)@0x465AE9・or bit11(0x800)@0x465AF3・and bit0@0x465B91・and bit18@0x465B97・and bit11@0x465BA5 ＝ **bit0/11/18 は条件分岐付き set/clear ペア [確定]**。bit9（0x200）は 0x462080 継続窓で `shr 9; test` ゲート多数（0x4622E3/0x462322/0x4623E3/0x462419/0x462461）→ シリアライズスキップゲート [確定]。v316 カタログ bit6/12 対応: bit6 set 1 サイト（0x667C81・帯外）／bit12 imm 形 **0 件 [確定負・imm形限定]** | F1（表）／F2・B_old 4CDA20窓・E6（0x462080 cont 0x4621B4..0x46221C・0x462280 窓） |
| 6 | FUN_00462080 の mode 分岐 — load 側 callers（0x465B12）で保存系分岐（'cannot save'/'save buffer'）が走るか。read/write 判別（[edi+8] 方向） | **走らない [確定]**: 'Player has no worldspace or parent cell, cannot save.'（=cstr(0xA3AF68)）は **fn 0x462280** 内@0x46239D にあり、callers(0x462280)=**1 件のみ 0x4652C1**（0x462080 窓外・0x465860 の 0x465B12 経路とは別ブランチ）。0x462080 本体（0x462080..0x46221C）= [esi+8] holder の release→new(0x10)+ctor 0x45A940 再セット・[esi+0x78] array クリア→0x446C50（0,0）で**状態リセット**［部分確定］。`[edi+8]` 呼びは bit9 セット時に `[esi+0x90] += size(4/0xc/2)` でスキップ＝**書き/シーク方向の advance パターン [部分確定]**（真の read/write 双向判別は 0x462280 の vtbl 先解決待ち→§4 残件） | G5（callers 復唱・0x462280=1）／F3（cstr）／E6・B_old 462080窓 |
| 7 | 0x465C94 read の arg 語義と reader 正体（[ebp+0x40] クラス／[esi+4] 正体／5push+add esp,0x18 の 1スロット差／[esp+0x30]=1／arg2(L+0x20) と frame） | **head(0x465C94) = 0x465860**（＝LoadGame fn 本体・v319 帯内）。**esp 差の解消 [確定]**: 5 push（esi, ecx=&L, 0xc, eax=&L, ebx=1）に加え、**arg6 枠は push ではなく `mov [esp+0x30],ebx`(=1) で事前確保**（0x465C73 の push 0 は 0x5AD980 の `ret 4` で消費済み）→ 6 枠=`add esp,0x18` と厳密整合。**出力 [確定]**: `mov edi,[esp+0x34]`（arg2 経由の formID 読み戻し）→ 直後 `cmp edi,0xfeffffff`（Player 特殊 formID 分岐）。**[esi+4] の esi = 0x45F2E0 の戻り値**（0x465923 call・`mov esi,eax`・G4 で jump-table fn/esp 0x628/`jmp [ebp*4+0x45F790]`、callers=8 のうち 0x465923=本文脈）。**[ebp+0x40] クラス**: ctor=**0x45F0F0**（8B = {+0: new(0x10)+0x45EF00(0x25), +4: new(8){0,0}}）、callers=2（0x465204/0x465A6A）、dtor=0x4530A0・delete 0x401F20・`mov [ebp+0x40],0` シーケンス確認（0x465AA1-AB8）。クラス正式名称は [未確定]（§4 残件） | F2（head・[ebp+0x40] 全ライフサイクル）／G1（0x465A90..0x465D20 整列窓）／G2（0x45F0F0）／G3（0x4530A0）／G4（0x45F2E0） |
| 8 | fn0x464910 の本名と上位（callers=1: 0x4CDA22・#91 帯浅層留保） | **上位 caller fn = 0x4CD960 [確定]**（head(0x4CDA22)=0x4CD960・`lea edi,[esi+0x48]` ループ・文字列 'NONE' @0x4CDAFD ＋ "Removing ownership data on exterior cell '%s' (%08X) at ( %i, %i ) in worldspace '%s' (%08X)." @0x4CDB4F → **ownership データ除去パス**）。callers(0x4CD960)=0（直接 E8 形なし→ポインタ/vtable 経由の疑い）。本名: E0 で 0x464910 への非 .text ptr 0 件＋文字列0 → **[確定負]**。帯（#106 専有）の深解析は向こう側 — 本件は境界+18命令+strings のみ（§5 宣言） | F6／E0／B_old 4CDA20窓（0x4CDA22 call 文脈） |
| 9 | 正式名称 [確定負] 維持: FUN_0045BB30/0x4DD4B0/0x4D8A10/0x4D8A30/FUN_005F0310 | **[確定負] 維持（強化）**: 5 個すべて非 .text dword ptr 0 件（E0 全ファイル照合）＋既知の文字列0・スロット0。命名根拠は出ない | E0 |
| 10 | docs 反映（親統合時・本 lane 無編集）: line9927 要反映・line9923 新規不整合・任意 line9926/9816/fn0x464440 範囲 | **docs 無編集**（L1）。反映候補は #66 note で追跡（本件新事実: LoadGame=0x465860 内 bit0/11/18 ペア・reader ctor 0x45F0F0・0x464910 caller=0x4CD960） | — |
| 11 | 副次: formatter `add eax,esi`@0x4555C4 の %i 語義／0x4445B5（帯外）の対象オブジェクト | (a) **%i = u16 演算の移動量 [確定（式）]/[部分確定（語義）]**: 包含 fn head=**0x453A90**。`esi = u16(0x452250(edi,8)) − u16[word@esp+0x14]`、eax=`u16(vtbl[edi][+0x4c](edi,8))`、`add eax,esi` → push。3 push サイト（0x45557F/0x45558E/0x4555CB）共通フォーマット **'Havok Moved(%i)'**@0xA39AC0、ゲート 0x455553＝**v311 L138 #48 CHANGE_REFR_HAVOK_MOVE と接続**。(b) 0x4445B5 = `and [eax+0x18],0xffffff7f`（bit7 クリア）で [確定]（全 .text imm 形 bit7 clear は 0x4CDA79 とこの 2 サイトのみ＝F1）。対象 eax の正体 = [推定]（head 候補 0x444340・帯外境界のみ。0x4CDA79 と同 bit7＝ latch クリア系との類推は根拠不足で [推定] 留まり） | C・D（窓全文字列付・A.out ⑪a）／F1／B_old（0x4CDA20 窓） |

---

## 2. probe 一覧（再現手順）

| probe | 内容 | 状態 |
|---|---|---|
| v329_A | ①463700 callee/callers・②44F058 包含 head（線形走査）・⑧464910 bit18 ゲート | 完走 |
| v329_B_old | 5窓 dump（0x462080/0x465B00/0x464910/0x5BA4D0/0x4CDA20）※旧版 NameError クラッシュ残骸。窓は有効、冒頭 `-- 00462080 --` から順 | 部分（有効窓のみ採用） |
| v329_C | ⑪ formatter 包含 head=0x453A90・⑪b 0x4445B5 境界バイト（`81 60 18 7f ff ff ff`） | 完走 |
| v329_D | 0x4554E0..0x455680 窓（%i 式・'Havok Moved(%i)' 3 push） | 完走 |
| v329_E | E0 全ファイル ptr 対照・E3 callers（5BA4D0/659BE0/44EE00）・E4 0x659BE0・E6 462080 cont/465A40 | E5/E8 不完全（§3 C-5/C-6） |
| v329_F | **F1 bit 表（E5 修正）**・F2 head(465C94)=0x465860・F3 cstr/IAT・F4 44EE00 深・F5 callers ctx・F6 head(4CDA22)=0x4CD960・F7 0x4E2BF0・F8（無効） | 完走 |
| v329_G | G1 0x465A90..0x465D20 整列窓・G2 0x45F0F0・G3 0x4530A0・G4 0x45F2E0・G5 callers(0x462080/0x462280)・G6 0x4531B0・G7 0x46AC50 | 完走 |

注: v329_B の新版（B0-B8）は孤児 PID 42584 が 12:29 起動のまま実行継続→**E/F/G で全欠口を別確保できたため PID を停止**（out 上書きは発生せず・B_old 99,810B は温存）。

---

## 3. 訂正節（旧結論は併記・削除しない）

- **C-1（本件②）**: v323 §5-2「0x44F058 は別 fn の呼び出し.site の可能性・包含 head 未取」→ **訂正: 0x44F058 は fn 0x44EE00 自己の retry ループ内再帰サイト**（0x44EFB0..0x44F07E は SEH 継続部）。旧 R2 訂正（0x44EDE0 との取り違え）は維持。
- **C-2（本件⑦）**: v323/v319 では 0x465C94 read を「0x465B00 窓」と呼んでいたが、**head(0x465C94)=0x465860 ＝ fn 0x465860（LoadGame）本体**であることを確認。旧「0x465B00 窓」表記は misalign スタート（0x465B00-0x465B0A は前命令の途中バイト）であり、G1 整列窓に置き換え。旧表記は履歴として併記。
- **C-3（本件⑥）**: v319 §7-9 の「fn0x464440/464910/462080 の mode 分岐未解」に対し、**'cannot save' 文字列は 0x462080 内ではなく fn 0x462280@0x46239D**、その唯一 callers=0x4652C1（0x462080 の load 側 caller 0x465B12 とは別ブランチ）。
- **C-4（本件⑦）**: 「5 push + add esp,0x18 の 1スロット差」は**差ではなく arg6 事前枠**（`mov [esp+0x30],ebx` が push なしの第6引数書込み・0x465C73 push 0 は 0x5AD980 `ret 4` 側で消費済み）。esp 収支検算で整合確認。
- **C-5（手法・記録）**: v329_E の E5（bit 表）初版は modrm mod=10 のみスキャンで**全件こぼし**（0x4445B5 の実体 `81 60 18` = mod=01）。F1 で mod=01（0x40-0x7f）+SIB に修正し capstone STRICT 検証で確定。**教訓: disp8 形式は MSVC では mod=01。modrm レンジは 0x40-0x7f/SIB rm=4 を必ず含む**。
- **C-6（手法・記録）**: F8 の `mov [reg+0x40],eax` バイト census は 0 件だが、F2 が `89 8D 40 00 00 00`（=mod=10 SIB base=5 形 [ebp+disp8]）を直接視測済み → **F8 はレンジ設計ミスで無効**。0 件＝不在の証拠にしない（教訓: SIB base=5 形を別コードで持つ）。E8 の band 先頭線形 disasm もデータバイトでイテレータ停止（既知の落とし穴）→ F2/G1 は整列 head スタートで回避。
- **C-7（記録）**: v329_E.out の E5/E8 セクションは不完全（空/0件）。E.out は中間成果物として保持し、**採用は v329_F.out の F1 と v329_G.out の直接視測**。

## 4. 新規残件（次束・#113 起票先・v338+ 採番）

1. [未確定] 0x45F2E0 の戻りオブジェクト（`[esi+4]` 生ファンクションポインタ）の正体クラス・jump table 0x45F790 のモード分解（帯内）。
2. [未確定] reader クラス（0x45F0F0 ctor・8B）の正式名称と +0 の 0x45EF00(0x25)（type 37）の意味（帯内）。
3. [部分確定→確定化] 0x462280 の `[edi+8]` vtbl 先＝read/write 双向判別・0x4652C1 所属 fn の identity（帯内）。
4. [未確定] bit16 in-band ペア（0x45CC0B/0x45CC34・0x45DDC9/0x45DDF5）の対象オブジェクト（帯内 fn 帰属確認）。
5. [推定→] 0x444340（帯外・v318 隣接外）head 本体確認＝0x4445B5 bit7 クリアの対象種別。帯 owner へ委任。
6. [未確定] 0x4CD960 の呼び出され方（E8 callers=0 → vtable/ポインタ探索、#106 協働）。
7. [未確定] [0xA2808C]=0x700A84 の import 実体（GetTickCount 系疑い）と 0x465AD5 cmp→bit18 条件の意味（TimeBase フラグ仮説）。

## 5. 帯遵守の自己宣言（v323 §6 方式）

- **深解析（自帯）**: v319 自帯 0x452000-0x467920（0x465860 全文の整列窓・0x462080 cont・0x462280・0x45F0F0/0x4530A0/0x45F2E0/0x4531B0/0x45A170 文脈・formatter 0x453A90-0x455680・45BC0x/45CCxx/45DDxx/45F28x のサイト行のみ）・v318 帯 0x44EE00-0x44F120（#89 完了済のため深可）。
- **機械走査（全域）**: E0（dword ptr 対照・I/O のみ）・F1（regex prefilter+capstone 検証・文脈展開なし）。
- **浅層/最小逸脱（帯外・v323 §6 先例で宣言）**: 0x444340/0x4445B5 境界バイト＋head 候補・0x5BA4D0 callers census＋各 12-14命令窓＋strings（0x5BB1B0/0x5BB210/0x5BB350）*2. 0x659BE0 head（E4・宣言済）*3. 0x4CD960 head 18命令＋strings（#106 帯・0x464910 caller 確定に必要な最小）*4. 0x4E2BF0 head 14命令・0x46AC50 全文7命令・0x4F9DD0 tail fn・0x5AD980 呼び出し文脈のみ。
- **未踏**: 0x4C0000-0x4E0000 の上記例外以外／save 実体の EXE 動的実行を要する項目は本件でも [未確定] 維持。

## 6. 自己確認

- v329: 15,022B / 75 lines（この行を含む最終実測）
- ファイル: `tmp/re/v329_v323_residuals.md`（本ファイル）・probe out 7 本（A/C/D/E/F/G/B_old）は `%TEMP%\opencode\` に保持
- 台帳反映: row311（V311）として統合・round_verify 合格後 bak 更新
