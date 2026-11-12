# v344 — 次束Y（#117）: v342 §5 新規残件 5 件の裁定＋レビューラウンド（C-7 訂正）

- 作成: 2026-11-09 / L1 report-only / 静的解析のみ（親直接解析レーン）
- 出典: #117（v342 §5 新規残件 5・row318 親突合時に起票）
- 対象 EXE: Oblivion Steam v1.2.0416（ImageBase 0x400000）・原本 EXE/ESM/.ess/解析原本/移植ソースは無変更（読取のみ）
- probe: `v117_1..11.py/.out`（全 EXIT=0・`%TEMP%\opencode\` 保持）— v117_1(①②⑤)/v117_2(③④)/v117_3(①⑤追)/v117_4(①③⑤詰)/v117_5(①callee)/v117_6(②真MAIN復元)/v117_7⑧(③jump slot)/v117_9(③境界再検証)/v117_10(③DROP spot-check)/v117_11(④tier A再検証)
- 判定語: [確定]/[部分確定]/[推定]/[未確定]/[確定負]・旧結論は §5 訂正節で併記（非削除）
- 帯宣言: 深= v342 既定（v318/v319/#106/#91 旧帯深可）＋ thunk 先 0x51C710/0x51A6F0/0x525380（v325 69 census の帯・#91 旧帯として深可宣言）・0x4FBDB0/0x468A40/0x4D7C50/0x5239C0 命令窓（①⑤解決最小）・機械走査= 全 .text 境界検証 census（③④）。#inprogress の #116（BG・0x579CF0/0xA469D4 系）とは非交差確認済。

## 1. 五件裁定表

| # | 残件（v342 §5） | 本束裁定 | 根拠 |
|---|---|---|---|
| ① | tail-jmp thunk 3（0x51EAF0/0x51E750/0x526CC0）のジャンプ先 reader の成功/失敗帰属 | **[部分確定]**（帰属クローズ・callee 契約残り） | 3 thunk は全て `sub ecx,0x68; jmp T`（+0x68 sub-object **adjustor thunk**・ins=3）。先= 0x51C710/0x51A6F0/0x525380（350-800 ins の Load 実装群）。vtable 帰属 [確定]（zone ref 唯一・slot+0x1C 位置一致）: 0x51EAF0→TESCreature vftable 0xA53188（ref 0xA531A4）・0x51E750→TESActorBase 0xA52EC0・0x526CC0→TESNPC 0xA53D14。3 先とも **call 0x4511C0=0（窓 0x800 全文字）= 早期 type ゲートなし**（v325 無ゲート群と整合）。ゼロ返却形: **0x51C710** 直接 al=0 形 `or eax,0xc00`@0x51C9DD→ret@0x51C9FC（al=下位 0x00）＋arg 素通し `mov al,[esp+4]`@0x51CB6C、状態定数 ret（eax=1/2/3@0x51CA78/84/90）。**0x51A6F0** 直接 `xor al,al`@0x51AAFF→ret@0x51AB01（1 件）・`and eax,0xf`@0x51A9E7 由来。`0x525380` 自前ゼロ書込なし（4-5 ret すべて callee/scratch 由来）。callee 側にゼロ形あり: 0x468A40（xor al,al@0x468C72 他3）・0x5239C0（xor eax,eax×4）・0x4D7C50（xor al,al@0x4D7FB7）・IAT [0xA28078]=**InterlockedIncrement**（v117_8・非ゼロ返）→ 失敗帰属は「直接形は少数・callee 介在が主経路」まで確定。callee 契約詳細=新規残件 N1（→#119） |
| ② | 0xA320A4 の正式名（TESIcon vftable 仮説） | **[確定]** | RTTI 直読: COL 0xAB3A1C（sig=0）→TD 0xB031B8=**'.?AVTESIcon@@'**（slot+0x1C=0x470590・slot0=0x5A6A20）。**随伴確定（v117_6・真 MAIN 復元）**: 0xA320A4 を書く 7 型の ctor は **inline サブ ctor 書込**（TESIcon sub-object・this+0x44/+0x48 先）で、各型自らの MAIN は窓外に別途存在 → MGEF=0xA32B14(EffectSetting・slot7 0x416100)・APPA=0xA43FB4(TESObjectAPPA・0x4B45C0)・BOOK=0xA44454(0x4B5440)・LIGH=0xA4319C(0x4B0D10)・MISC=0xA44CA4(**slot7 0x4B9600=v325 無ゲート 21 と相互整合**)・WEAP=0xA45354(0x4BB150)・AMMO=0xA4068C(TESAmmo・0x4A9030)。INGR/ALCH/QUST の MAIN=0xA33F5C/0xA3217C/0xA541EC も RTTI 確証（IngredientItem/AlchemyItem/TESQuest）。**訂正 C-6**（§5）: v342 §1-④「MAIN 集約 7 型」は v115_3 fallback 誤選択（ecx-map 未捕捉のサブ書込を MAIN 扱い） |
| ③ | vtable+0x1C 179 サイトの tier 化＋NULL 主源寄与比 | **[確定（機械・境界検証済）]**（寄与比は #82 維持） | **レビュー-round で census 再検証**: 179 → **139**（40 サイトは命令中途の正規表現混入・§5 訂正 C-7。DROP 例 0x4072F4/0x483029/0x5169A1 は全て `call rel32`（末尾 FF）+ `push 0x1c`（6A 1C）= FF 6A 1C パターン・v117_10 spot-check 5/5 で head 線形 desync なし＝DROP 正当・UNKNOWN 0）。**139 の tier**: this-virtual 109 / no-load（tail-jmp 型で呼出側 reg）23 / frame-arg 6 / other 1 / **global-abs 0**（v117_2 の 3 件全て偽）。NULL 主源の静的側: グローバル直接 read の vtbl パスは**存在せず**、[0xB36208] は `_onexit` 登録 slot（C705@0x516988 が 0 書込・push 0xA1C100@0x516983＋call 0x981FB4=v341 の CRT _onexit 系と一致）、0xB25AC8/ACC=**float 1.0 ペア**（0x4072CF/0x483002 の実読取は `mov [eax+0x44/0x48],edx` の float コピーで call 形でない）。→ 寄与比は実行時事象のみ=#82（v342 §5-3 クローズ） |
| ④ | E9/EB 拡張 tier B（callers=0 約 3,986）の vtable 参照第 2 判定 | **[確定（機械 census）]** | zone 0xA00000..0xABFFFF 全 dword マップ（70,422 値）照合: **tier B+（vtable zone 参照あり）=54 → 採用可**（v323 検出手順の条件 (v)「vtable エントリ参照」と整合・例 0x419600←0xA3216C/0xA33F4C・0x4ADF30←0xA41BD0・0x4CA430←0xA46A8C・0x51C8C0←0xA5333C）。**B-=3,932 → 不採用**（callers=0・zone 参照 0・jmp 先 follow 救済 0）。拡張則の最終採用集合 = **tier A 69（v117_11 で境界再検証 69/69 生存・偽 site は 0x98B540 の 1/3 のみ）∪ B+ 54 = 123**。re_util 変更禁止継続＝文書化のみ（v342 §5-4 クローズ） |
| ⑤ | slot 実先候補（[[this+0x54] vftable]+0x40 / this vftable+0xD4 の所属候補 census） | **[部分確定]**（候補列挙 [確定（機械）]・実先=実行時） | **+0x54 書込 census: 26 サイト/17 vftable**（v117_1）: TESObject 系 12（AMMO 0xA40624→0x46D980・FLOR 0xA41AD0→0x46C880・KEY 0xA429E4→0x4AF4D0(thunk)・LIGH 0xA43134→0x4B1500(thunk)・APPA 0xA43F50→0x4B4900・ARMO/CLOT=0x46A840 共有・BOOK 0xA443F0→0x4B59B0・MISC 0xA44C3C→0x4B9850・WEAP 0xA452EC→0x4BB600・Sigil 0xA454D0→0x4BB950・SoulGem 0xA4568C→0x4BBD00）・TESCreature 0xA531AC→**0x4673F0**・TESNPC 0xA53D38→**0x468030**・NiTArray<NiTSet<SkinInfo(NiBoneLODController)>> 0xA7C304→0x6E9410(SEH prologue)・0xA99B58→0x8CAC70・0xA99B88→0x8CB2C0（TD なし=Havok 側）。v117_6 の this+0x54 サブ書込と 5 件相互一致（APPA/LIGH/MISC/WEAP/AMMO）＝二経路 cross-check ✓。**0x6B6B90 の owner 未決**: 非仮想（zone ref 0）・E8 callers 4（0x6AB5E4/0x6AB757/0x6AE7B3/0x6AEE23・this=esi/edi 由来・owner heads 0x6AB570/0x6AB720/0x6AE720/0x6AED70 も zone ref 0）=N2。**0x4FBB60 callers 3**: 0x4FBDB0=**Script vftable（ref 0xA49E10・TD '.?AVScript@@'）内法**・0x52B0C0（TESNPC 系帯）・0x53142C（this=0xB36530 inline singleton・.data）→ this vftable+0xD4 の concrete 先は実行時型依存=#82（候補リストは確定・v342 §5-5 クローズ） |

**集計**: [確定]3（②③④）／[部分確定]2（①⑤）／新規残件 N1-N3（§6）／訂正 C-6・C-7（§5）。主観点の [確定負] なし（global-vtbl パス不在は负の発見として ③ に内包）。

## 2. レビューラウンド（v342 既載裁定との突合結果）

- **⑥→③ 再検証（重要）**: v342「179 サイト [確定・機械]」は opcode+modrm+disp8 直接 regex＋**一致オフセットからの capstone 検証**だったため、`call rel32` の末尾バイト FF が直後の `push 0x1c`（6A 1C）と結合して `jmp [reg+0x1c]`（FF 6A 1C 系）に化ける類の偽サイトを除く検証になっていなかった。v117_9（owner head からの線形境界照合・head 規則は v318 rule v3＋E9/EB 拡張）で **139/179 生存・40 DROP・UNKNOWN 0**、v117_10 で DROP 5 件抽試全確認。**C-7 訂正**（§5）。
- **tier A 堅牢性**: v117_11 — E8 census（生走査）は偽陽性のみ・偽陰性なしの構造（全バイト走査のため）で、tier A 69 は境界検証後も **69/69 生存**（偽 caller site 合計 1 件のみ 0x98B540）。→ v342 ③ の採用可裁定は**無傷**（訂正不要）。
- **v342 §1-① の n=0 群記述**: 「tail-jmp thunk 3 件は先側未決」→ 本束で決着（adjustor＋Load 実装群・直接ゼロ形は 0x51C710/0x51A6F0 に各 1、0x525380 は自前ゼロなし）。
- **相互整合チェック（3 件追加）**: (a) MISC の真 MAIN slot7=0x4B9600 が v325 無ゲート 21 の登録と一致 ✓ (b) +0x54 書込 census（v117_1）↔ ctor サブ書込（v117_6）5 件一致 ✓ (c) IAT 0xA2807C=InterlockedDecrement（v341 の 679xxx walk）↔ 0xA28078=Increment の本束名前解決と隣接整合 ✓。
- **自己訂正 C-8**: v117_4 (3b) の「global 値→vtable 復元」読みは偽 site 前提の誤解釈（v117_7/8 で差し替え・正典は §1-③）。

## 3. probe 一覧（再現）

- v117_1: thunk 分解（`sub ecx,0x68` 形認識）・先 reader の xor/rets・0xA320A4 COL/TD・+0x54 `mov [reg+0x54],imm` census（C7 系＋capstone 検証）・[V+0x40] 先頭
- v117_2: 179 tier 化（v117_9 に置き換わる until C-7）／E8 targets precompute・tier B/B+/B-・jmp follow 救済 0 検証
- v117_3/4: targets full-window ゲート scan（call 0x4511C0=0）・FAIL@0x52544E 文脈（=**null 置換パターン**で al 不関与＝v115_1 の FAIL 分類は ebx のみ・al 無関係）・callers(0x6B6B90/0x4FBB60)＋ecx 由来・thunk の zone ref→slot+0x1C 位置検証・targets の al ret 直前 writer・0x4FBDB0 owner→Script
- v117_5: callee 0x468A40/0x5239C0/0x4D7C50 の al/eax zero census
- v117_6: 7 型 ctor 窓 0x300＋this/sub-chain（lea 追跡）→ 真 MAIN＋sub offsets（offset は heuristic・正典は MAIN/TD）
- v117_7/8: グローバル 3 slot の書込形 census（C705/890D/810D/830D）・0x516970-9F 全文・0xB25AC8 ペアの実意味（float）・IAT hint/name 直読（0xA28078/7C）
- v117_9/10/11: 境界検証器（census 系全般）・DROP spot-check・tier A 再検証

## 4. 帯遵守の自己宣言

深: 0x51C710/0x51A6F0/0x525380・0x4FBDB0・0x468A40/0x4D7C50/0x525380 周辺命令窓（①⑤解明に必要な最小）・0x516970-0x5169A4（③偽 site 立証）。機械: 全 .text regex×境界検証・zone dword マップ。浅層留保: 0x6AB-0x6AE（N2 として次束）・0xB36530 singleton 内部・0x52B0C0 本体未読。#116（BG 所有・0x579CF0/0x4CD960 系）と交差なし。v316/v317 自帯・ESM/git/docs 無接触。

## 5. 訂正節（旧記載は併記・非削除）

- **C-6（v342 §1-④／C-1 の refinement）**: 「MAIN 集約 7 型」→ **v342 自身の probe（v115_3）の fallback 誤選択**（ecx-map 未捕捉の `mov [edi]` 形=TESIcon sub-object 書を MAIN 扱い）。正: 0xA320A4=TESIcon 自 object の vftable [確定]（RTTI）で、7 型の ctor は it を **sub-object として inline 初期化**する; 真 MAIN は §1-② の 7 値。v325_01d の「10 型」誤りは C-1（7/3 分離）を挟み本束で最終形へ（v342 C-1 は訂正の一部として有効・集約数の主張のみ置換）。vtable_map census（mismatches=0）は無傷のまま。
- **C-7（v342 §1-⑥・数量訂正）**: 「全 .text 直接 call/jmp [reg+0x1c] = 179 サイト（機械 census [確定]）」→ **境界検証で 139**。40 サイトは命令中途混入（`call rel32` 末尾 FF＋`push 0x1c`）。tier 内訳は §1-③ の 139 基準へ置換。⑥ の個別裁定（C-4: 0x482170 の NULL 分岐は obj deref せず 等）は窓視測由来で無傷。教訓（一般化）: **opcode+modrm+disp 直接 regex census は命令境界検証を必須伴う**（v318 の find_head 系教訓の census 版・re_util 凍結のため文書のみ）。
- **C-8（本束 v117_4 (3b)）**: グローバル 3 サイトの「object vptr→slot 読取」解釈は偽 site 前提の誤り（v117_7/8 で float ペア/_onexit slot と確定）。本文件 §1-③ が正典。

## 6. 新規残件（→ #119 起票・成果物 v346+ 予約）

1. **N1 [未確定]** ① callee al 契約: 0x468A40/0x5239C0/0x4D7C50 のゼロ返却経路条件（3 者の ret 直前文脈・帯 0x46-0x52 深可）＋ 0x51C710/0x51A6F0/0x525380 の残 callee（call edx 仮想・0x9811e2 等 CRT）の bool 性切り分け。
2. **N2 [未確定]** ⑤ 0x6B6B90 の owner クラス（callers 4 の head 0x6AB570/0x6AB720/0x6AE720/0x6AED70 の this 連鎖＋当該 heads の静的呼出元遡上・帯 0x6Axxxx 未割当）＋ 0x4FBB60 の Script 経路 this+0xD4 concrete 先（Script vftable 実体確定から引ける範囲）。
3. **N3 [手法・任意]** 過去 census の境界再検証適用（GST 166・0x45F2E0 callers=8・v325 slot7 census 群）— v117_9 器で一括、優先低。

## 7. 自己確認

- v344 実測: 12,989B / 60 lines（この行を含む最終実測）
- 台帳 row321（V321）統合・round_verify 合格後 bak171 確定
- probe out 11 本 `%TEMP%\opencode\`（v117_2 の③初版 tier は v117_9 に置換済＝文内注記）
