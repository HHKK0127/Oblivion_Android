# v335 — #85 段階2 の静的残件 6 項（次束J 完結 / 親直接解析）

- 出典: `tmp/re/v317_v315_residuals.md` §6（新規残件 11 件）＋ #85 note 系列（段階1=v319 統合済・段階2 のうち E1/E2 は #92(v328) で消化、E4 と Δ0x1C は動的ゲート）
- 本束の対象（**静的に救済可能な 6 項**）: v317 §6-4（+9 typeID vs 0x7D の loader 側再追跡）／§6-7（459950 境界の入力意味）／§6-8（459AF0 連鎖の発動文脈）／§6-9（459AF0 arg2 の契約）／§6-10（45ADC0 esp 範囲の伝搬）／§6-11（0x464440 窓と find_head）
- 対象外（残るもの）: §6-1/2/3/5/6 の動的・実機依存部分（E4=45BB30 副作用観測・Δ0x1C 実データ・5F0310/45BB30 の正式名文字列＝**#82 ゲートへ委任済み・不变**）
- 判定: 5段階。旧結論は削除せず §4 訂正節で併記。クリーンルーム（OpenTES4/xOBSE/COEF 未参照）
- EXE 読取のみ（re_util.EXE）。検体 .ess は本束不使用（静的のみ）

## 1. 判定表（6 項）

| # | 項目 | 旧判定 | 本束判定 | 一句話裁定 |
|---|------|--------|---------|----------|
| S1 | §6-8 459AF0 連鎖の発動文脈 | [未確定] | **[部分確定]** | 上位 fn=**0x4CBE50**（head 検証済・唯一の 459F30 呼出窓）。連鎖=**0x4CBE50 →(guard `test byte[edi+0x24],1`)→ 0x459F30 → 0x459AF0**。0x459F30 内 0x459F4B が文字列 **`'PurgeSavedDataForCell() can only be called on interiors, but it was just called on exterior cell %08X.'`(0xA3A180)** を伴う＝**セル単位の保存データ purge 経路**と発動文脈を特定。0x4CBE50 の呼び手 7 件（0x4B8993/0x4CC045/0x4F26FE/0x4F2716/0x515E7E/0x51607A/0x51625A・heads 0x4B86C0=0x4D6670 検査系／0x4F2270=[0xB360C0..C8] 遅延初期化単体系／0x515D20・0x515EF0=SEH 枠の参照列挙系）— いずれも帯外 head 24 命令の浅層照会のみ |
| S2 | §6-9 459AF0 arg2 の契約 | [未確定] | **[確定(命令)]** | **arg2 は読取箇所を特定（v317「特定せず」を訂正 §4-C1）**: prologue `sub esp,0x90` 直後の **0x459AF7 `mov ebx, [esp+0x98]` = arg2→ebx**（push ゼロの位置で esp 基準厳密検算済）。値は 2 呼出点で **1（0x459F67 `push 1`=purge 連鎖側）／0（0x459F8E `push 0`=thunk 0x459F80 側）** のモードフラグ。ebx は既知本文（bit12 gate・map find・payload 走査）で使用 |
| S3 | §6-10 45ADC0 のノード ptr/this 伝搬 | [部分確定] | **[確定(命令)]** | ローカル範囲を閉合: 呼出 0x4657E1 の直前 **0x4657D6 `mov ecx,[ebp+0x40]`＋0x4657D9 `cmp ecx,ebx/jne` ガード＝this=[SaveGame+0x40]（NULL チェックあり）**、引数=**0x4657DD `lea edx,[esi+0x3c]; push edx`（ループノード esi+0x3C の名前バッファ）**。45ADC0 内では `mov eax,[esp+0x558]`（=arg1）→ push → `[0xA28164]`（fopen 系 IAT）・`[0xA28160]`（fclose/write 系）＋文字列 **'.txt'(0xA3A4D4)** ＝**テキストダンプ書出ヘルパ**（0x465130 本体 ebp=SaveGame・0x465157 検出）。v316 の「this=SaveGame+0x40」伝搬未追跡が解消。callers=0x4657E1/0x4669AC の 2 件 |
| S4 | §6-4 loader 側 +9（typeID vs 0x7D） | [未確定] | **[部分確定]** | **loader は +9 を typeID として分岐しない**（再追跡確定）: 0x466787 `mov ax,[esp+0x26]`（tail u16）==0 → 0x466830 クリーンアップ経路（0x452C90→**0x452DF0(push 1)**＝v328 P4 の削除 fn と同一・0x45AD00 free）。!=0 → **0x466795-0x4667AF: `add edx,4` で確保長＝u16+4、ヘッダ 4B（[esp+0x52..55]＝+8/+9/+A/B の写し）を積んで 0x453500**（`'Could not create save buffer, out of memory.'`(0xA390D4) を持つ**保存バッファ確保**・書式化ではなく確保のみ）→ 0x45B9A0=**SaveGameData**（`'Error: TESSaveLoadGame::SaveGameData() was called while loading.\n'`(0xA3A4E0)）へ `push 4, lea [esp+0x54]` で**ヘッダ 4B をそのまま書写**、payload は 0x45BB00(466816) へ。＝+9 の 0x31-0x33/0x7D は**非解釈のパススルー**で、v312「typeID」解釈と tail==0 の 0x7D（v328 P2 の +9 スロット＝frame 分解の char/typeID 組・version=0x7D 合法性は E2 5,527/5,527 で追認済）の**矛盾が構造的に解消**（整合条件の要求自体が不要と判明）。残: +9 相当を意味消費する箇所は save 側 frame 組立のみ＝v328 で確定済み、これ以上の名付けは [未確定] のまま |
| S5 | §6-7 459950 テーブル境界の入力意味 | [推定/未確定] | **[部分確定]（前進）** | 補強証拠 2 件: ①0x453530（0x453500 の直後 fn・浅層）が **`[esi+0xc]` を 0x446B80(created 判定) へ渡す**＝**入力は formID**（オブジェクト+0xC の formID 説を実コードで確認）。②created 時は **0x9832E6（RTTI dynamic cast 系・TD 0xB03054=`.?AVTESObjectREFR@@` の先頭＝v326-項目4 と同一・TD 0xB03038）** へ 2 連続 cast してフラグ（`&~4|2`／`|6`）を組む＝**created form → TESObjectREFR 系統への変換ゲート**。v319 の [t+0xc]=count/境界解決に「入力=formID [確定(命令)]」が加わり §6-7 の『実解釈』は解消方向。残: created 非該当 formID が 0 に落ちる呼び手側の回復分岐は未全追跡 |
| S6 | §6-11 0x464440 窓 vs find_head | [未確定] | **[確定]** | **v323（#87 項・fn0x464440=0x464440..0x464906＋LOD land 初期化系 [確定]）で既決着**の再確認＋新規裏付け: loader 終端 **0x466889 `push 1; mov ecx,ebp(SaveGame); call 0x464440`**（LoadGame 本線からの最終化呼出・arg1=1）。find_head(0x4646D9)=0x464440 は v316 窓（0x464440..0x46458A）の**部分区間を指していただけ**で off-by-pad なし |

**集計**: [確定(命令)/確定]3（S2,S3,S6）／[部分確定]3（S1,S4,S5）／[未確定]0（主判定）※旧 §6-1..6,11 の動的依存 5 項は対象外（#82 委任のまま）

## 2. probe 一覧（%TEMP%\opencode\v335_*・全 EXIT=0）

| probe | 内容 | 主結果 |
|-------|------|--------|
| v335_1.py/.out (400行) | A=0x4CBE50 head+窓 0x260+文字列／B=0x459AF0 全文 151 命令＋esp アクセス列挙＋callers／C=0x45ADC0 窓・callers・0x459F30 窓／D=0x46679C-0x466840 loader 窓 | S1-S4 の骨格・callers(459AF0)=2・callers(45ADC0)=0x4657E1/0x4669AC |
| v335_2.py/.out (289行) | E1=4CBEBF-4CBED1 精密・E2=0x4CBE50 callers 7・E3=0x459F30 全文（0x459F80 thunk 含む）・E4=45ADC0 call ctx 2・E5=0x453500/0x45B9A0 head＋0x466700 窓 | purge 連鎖確定・SaveGameData/確保 buffer の文字列確定 |
| v335_3.py/.out (216行) | F1=上位 4 heads（0x4B86C0/0x4F2270/0x515D20/0x515EF0・各 24 命令＋文字列）／F2=0x465130 head＋0x465740.. 窓／F3=tail==0 経路 0x466830..／F4=文字列 10 件 | S3/S4/S5 クロージャ・Warnings.txt/buffer 意味確定 |
| v335_4 | （不需要と判明——E3/F2 で esp 基準が閉合済みのため打ち切り。教訓: 追跡は push 収支の検算で先に打ち止め可否を判定） | — |

## 3. 主要根拠（VA・命令）

- **S1/S2 連鎖**: 0x4CBE50（head・CC 境界検証済）0x4CBEA2 `mov ecx,[0xb333a0]`→0x43F8C0 検査→0x4CBEB3 `mov ecx,[0xb333a0]`→0x440120→**0x4CBEC5 `mov ecx,[0xb33b00]`→push edi→0x4CBEC9 call 0x459F30**（守り=0x4CBEBF `test byte[edi+0x24],1`）。0x459F30: 0x459F4B `'PurgeSavedDataForCell...'`＋0x459F62 `push 1; push [esi+0xc]; 0x459F67 call 0x459AF0`（ret 4）。thunk 0x459F80: `push 0; push [eax+0xc]; call 0x459AF0`。
- **S2 esp 検算**: `0x459AF0 sub esp,0x90`（7B）→直後 `0x459AF7 mov ebx,[esp+0x98]`＝entry [esp+8]=arg2（retaddr=+0x90・arg1=+0x94・arg2=+0x98）。0x459BE1 `cmp byte[esp+0xa8],0` は中盤 push 後の別ローカル（arg 基準外）と区別して記述。
- **S3**: 0x465749 `mov ecx,ebp; call 0x45E220`（ebp=SaveGame・0x465157 `mov ebp,ecx`）直後窓に dry ガード `shr 9/test al,1`（v316 §4-1 と同物）→0x4657D6-0x4657E1（this=[ebp+0x40]・arg1=esi+0x3C・call 0x45ADC0）。45ADC0: `sub esp,0x554`＋セーフ cookie・0x45ADE9 `mov [esp+0x3c],edi`(this 退避)・fopen 系 IAT [0xA28164]/[0xA28160]＋'.txt'。
- **S4**: 0x46676B `mov [ebp+0x50],0`／0x46678F `test ax,ax; je 0x466830`／0x4667A0 `add edx,4`／0x4667AA `mov [esp+0x54],ax`／0x4667AF call 0x453500／0x4667D7-0x4667E4 `push 4; lea ecx,[esp+0x54]; mov ecx,[SaveGame]; call 0x45B9A0`／0x466805-0x466816 `push u16; push [ebp+0x14]; push [esp+0x38]; call 0x45BB00`。0x453530（確保 fn 直後）の 0x446B80→0x9832E6×2（TD 0xB03054/0xB03038）→flg 群。
- **S6**: v323 §統合済み（0x464440..0x464906）＋ loader 呼出 0x466889。

## 4. 訂正節（旧結論は削除せず併記）

- **C1（v317 §6-9 の訂正）**: 「459AF0 本体で arg2 の読取を特定せず」→ **0x459AF7 `mov ebx,[esp+0x98]` が arg2 読取**（purge 連鎖=1／thunk=0 のモード）。旧記述は本節で上書き（本文は残置）。
- **C2（v317 §6-10 の前進）**: 「this 伝搬をローカル範囲で未完全追跡」→ 0x4657D6-0x4657E1 で閉合（[確定]）。
- **C3（v317 §6-4 の構図変更）**: 「0x7D が typeID として合法か」の問いは** loader 側が +9 を typeID として解釈しない**ため不成立（整合は自動的に成立）。問い自体の取り下げを記録（旧記述残置）。
- #85 note 系列（段階1 完了・Phase A 突合・委任消化）の各記録と矛盾なし。v328 P2/P4 と相互整合（0x452DF0 の再登場＝削除 fn 再確認）。

## 5. 新規残件（次束用）

1. 0x4CBE50 の正式名（purge 単一候補。'PurgeSavedDataForCell' 文字列は 0x459F30 側の interior ガード＝**0x459F30 候補が強い**）と、0x4CBE50 の [edi+0x24] bit0/bit3/bit8（0x108）フラグ群の意味。
2. 0x459AF0ebx モード（1/0）が本文のどの分岐（bit12 gate ほか）で消費されるかの完全マップ。
3. 0x4CBE50 呼び手 7 件の上位（0x4B86C0=0x4D6670 経由の cell 検査／0x4F2270=[0xB360C0..C8] 遅延単体＝何の単体か／0x515xxx 群）の同定（帯外・浅層留保中）。
4. 45ADC0 の fopen モード文字列（0x45ADF3 push 0xA3A4D4='.txt' の直後引数群）と 'Warnings.txt' 0xA3AFA0 の同一ファイル経路か。
5. loader tail==0 の 0x452DF0(push 1) が pass2 の bit3 削除と呼び出し規約同一か（引数 slot 対応）。
6. 0xB03038 TD のクラス名（0x9832E6 二番 cast の先）。

## 6. 再現手順

```
python %TEMP%\opencode\v335_1.py   # A-D（head/全文/callers/loader 窓）
python %TEMP%\opencode\v335_2.py   # E1-E5（連鎖 site・thunk・call ctx・consumers）
python %TEMP%\opencode\v335_3.py   # F1-F4（上位 heads 浅層・tail==0・文字列）
# 各同名 .out（全 EXIT=0）。EXE 読取のみ。
```

## 7. 帯遵守宣言

- **深解析**: v317 許可帯のみ（0x452000-0x467920・0x430000-0x431000＋文字列帯）。
- **帯外照会（浅層・宣言済み・v317 §6-8 自身が「0x4CBEC0 帯（帯外）」と明記した項目の解決に必要な範囲）**: 0x4CBE50（head＋窓 0x260B＋call site・**0x4C0000-0x4E0000 は #106 専有のため命令窓の関数境界追跡なし**）、0x4B86C0/0x4F2270/0x515D20/0x515EF0（各 head 24 命令＋文字列）。
- v318 自帯（#94/#108 関連）: 0x44F000-0x452000 は 0x44F225 窓のみ参照済（v326）で本束新規アクセスなし。**逸脱なし**。
- Numba: 0.68.1 import 可を確認したが capstone/I/O 主体のため**未使用**（教訓どおり）。
- クリーンルーム: 参照文字列は EXE 内蔵のみ。docs 交差なし（新規不整合も なし — 0xA3A180/0xA3A4E0/0xA3AFA0/0xA390D4 は exe 内英文）。

## 自己確認

v335: 11,841B / 74 lines（Get-Item/Get-Content 実測）
