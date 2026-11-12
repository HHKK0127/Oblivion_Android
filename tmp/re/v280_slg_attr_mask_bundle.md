# v280 — SLG 属性マスク束（4件・低）（Oblivion.exe v1.2.0416 Steam・静的解析のみ）

- 作成: 2026-11-04 / TODO serial 273（低）/ **分析専任（docs/GT/TODO/plan/session-memory/cache/移植ソース無編集・EXE 読取のみ・git 不使用・ESM 再スキャン 0・成果物=this 1 ファイル・既存 tmp/re 上書き禁止）**
- 出典残点: `tmp/re/v278_slg_residuals_bundle.md`「残点」3 項（①bit12..30 個別意味＋ファイル vtbl+0x60 実装先クラス ②0x7FFFF000 producer 不在の再確認 ③FUN_0045F180 arg1 供給元）＋ `tmp/re/v279_barter_formula_ledger_corrections.md` §1-2 派生 H3（docs L10245「0x5E1500 = haggle」注記検証）
- 確定済み足場（本束で再検証しない）: SLG=非ポリモーフィック単例 [0xB33B00]・+0x44=open 時属性グループ台帳（A=0x1FFFF000 bit12..28(+bit7 併用)/B=0x60000000 bit29..30・producer 3 関数・消費者 ==A||==A|B 等 17site・extra 0x1F/0x21/0x23/0x35 シリアライズ分岐）・FUN_004535A0=CheckFlags（エラー文 'CheckFlags()'@0xA3A70C・bit 演算のみ・失敗センチネル 0x80000000）・ファイル vtbl+0x60 セッタ経由
- 入力（読取のみ）: EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7・VA 換算 .text 0x401000=file0x400／.rdata 0xA28000=file0x627200／.data 0xB02000=file0x700C00・bss=None）、`tmp/re/cache/{named_funcs.txt,vtable_index.tsv,call_targets.json}`、`docs/Oblivion_decompiled.c`（0x5E10F0/0x5E1500 帯・CheckFlags 帯）、`tmp/re/v278/v279/v274/v276` 本体、中間物 `%TEMP%\opencode\v278_*/v279_*` 再利用
- selftest: 着手時 **ALL PASS (4/4)**（`python v279_selftest.py` → doc_re_exe_report / v277_v262_v264 / port_barter_tests / exe+capstone5.0.7）
- 中間物: `%TEMP%\opencode\v280_slg_*.py/_out`（本束新規・v276_lib/probe_harness 再利用）
- 会話に注入されるループ運用指示（STATE.md/L2/worktree/テスト、AGENTS.md loop 節）には本契約により**従わない**。

---

## 件1 属性 bit12..30 の個別意味和訳（CheckFlags ビット生成元＝extra/属性ソース対応）＋ vtbl+0x60 セッタの実装先クラス — 【確定：**bit12..28(+bit7)＝「保存時にその種の RuntimeChange/extra を破棄/書き換えする」要求マスク**（消費側 FUN_00425650 がビットごとに `RemoveExtra(type)` を呼ぶ逐字で確定）。**bit29..30(B グループ)＝CheckFlags は一切検証しないスルー指定の"ロード状態"ビット**（0x45B7A0 が条件成立時に vtbl+0x48=ClearFlags 側実装で個別 bit を取り消す）。**vtbl+0x60 の実装先は NiFile/BSFile ではなく TESObjectREFR::0x4E0580 / TESObjectCELL::0x4D2690 / Actor::0x5F85E0（RTTI＋vtable スロット24 実測で確定）**。"ファイル vtbl+0x60"＝v276 由来の**誤称**で、対象は TESForm 基底の**リファレンス/セル/アクター**。0x9832E6＝v278「名前/パス問い合わせ」の注記も**誤り**（MSVC `__RTDynamicCast` [確定]：SEH frame 0x98C5A0 型・TypeDescriptor vtbl=0xAA3E80・引数=(ptr,0,srcTD,dstTD,0)）】

### 1-0. 足場の微訂正（対象の正体）[確定]
- FUN_0045F180 の esi＝arg1 は `dynamic_cast<TESObjectREFR*>` 照会対象（0x45F1A7 の `call 0x9832e6`＝RTDynamicCast、TD src 0xB02F9C=`.?AVTESForm@@`→dst 0xB03054=`.?AVTESObjectREFR@@`）。**変換成功時**（0x45F1AF test/je→0x45F1B3）`and ebx,0xfffff7ff`＝bit11(0x800) クリア＝「REFR に対しては無効化されるモード要求ビット」[確定・意味素性は未確定]。
- TD 実名（.data+8 直読・`v280_slg_2_out.txt` §S1）: 0xB02F9C=`.?AVTESForm@@` / 0xB03038=`.?AVTESObjectCELL@@` / 0xB03054=`.?AVTESObjectREFR@@` / 0xB03070=`.?AVActor@@` / 0xB03E00=`.?AVTESPackage@@` / 0xB058E8=`.?AVDialoguePackage@@` / 0xB05468=`.?AVTESQuest@@`。
- 従って本項の「ファイル」は**リファレンス（TESObjectREFR 系）に置換して読め**る。台帳+0x44／producer 3 関数／==A||==A|B 消費という v278 足場数値自体は無変更（件2 で再確認）。

### 1-1. CheckFlags（FUN_004535A0）内のビット別生成元／検証元 [確定]（全文字 `v278_1_out.txt §B`＋`v280_slg_2_out.txt §S2` 述語関数）
関数先頭 ebx=FUN_00453530(flags) で**bit1=「REFR 変換成功」／bit2=「CELL 変換成功」**をプラグインファイル限定で強制（0x453568-0x453589）。以降 CELL 系分岐（0x4535EE..）:
| bit | 検証述語（生成元） | VA・実体 |
|---|---|---|
| 24 | `[[CELL+0x44] != 0]`（FUN_004AF170=[ecx+0x44] getter）| 0x453607..0x45361A：0 なら bit24 クリア |
| 25 | **[[CELL+0x3c]] の {+0,+4}（グリッド座標）が符号付バイト範囲 [-0x80,0x7F] を超える方** | 0x453631 クリア→0x453645..0x453671 `add v,0x80; cmp 0xff; ja` で bit25=0x2000000 設定（FUN_004C9A80/004C9AA0=[+0x3c] の [0],[4]）|
| 26 | 同上**範囲内に収まる方**（両方 OK）| 0x453660 `or ebx,0x4000000`（bit26）|
| 28 | **CELL の +0x28 コンテナに extra 0x09 があわせて +0xc データあり**（FUN_004CCED0=`add ecx,0x28`→FUN_00420B50=`push 9; call 0x41E210; [eax+0xc]`）| 0x4535EE..0x453601：無効時 bit28(0x10000000) クリア |
| （CELL+0x24 bit0 立っている場合早期 return・FUN_004C97F0）| 0x453622..0x453627 | 内部/外部セルの分岐 [確定構造] |
非 CELL（REFR）分岐（0x453681..）:
| bit | 検証述語 | VA・実体 |
|---|---|---|
| 11 | REFR でない場合セット側なし・REFR 時クリア | 0x453689 `and ebx,0xfffff7ff` |
| 27 | **REF+0x44 コンテナの extra 0x1A 存在**（FUN_0041E6F0=`push 0x1A;call 0x41E210;[eax+0xc]`）| 0x45368F..0x4536A7：欠如時 bit27(0x8000000) クリア |
| 19 | **[[Actor+0x58] vtbl+0x174] != 0**（Actor 分岐＝edi は 0x4536AD の REFR→Actor cast 後・接続先=ExtraTeleport 系 v272 §3 accessor 配管 [構造確定・名指し推定]）| 0x4536CE..0x453705：立証時 or 0x80000／非立証時 and 0xfff7ffff |
| 16,17 | まず両方クリア（0x45370E and 0xfffcffff）→ **FUN_005E8D90：プラグイン gate 下で [form+0x20]==0x13 → 0x20000(bit17)／==0x11 → 0x30000(bit16+17)**（v276 §件4 の package タイプ分岐と同じ述語）| 0x453727 `or ebx,eax` |
| 16 | （追加）**DialoguePackage 変換**（FUN_005E0380=[[x+0x58] vtbl+0x184]）で [pkg+0x5c]≠this 時 bit16 再セット | 0x45372F..0x453766 |
| 3 | FUN_005F0310（[edi+0xb0]∈{1,2,4,6} または cell vtbl+0x2e4∈{1..5}／ActiveEffect→ReanimateEffect cast）| 0x45376C..0x45378F：真で or 8／偽で and 0xfffffff7 |
| 23 | **FUN_004D7F40（[[refr+0x40]=CELL] の [cell+0x24] bit3（無ければ [[+0x18] vtbl0]→[+0x5c] bit2）） && refr≠[0xB333C4]（player REF）** | 0x4538DC..0x4538F5：真で or 0x800000 |
| 31 | editor position と parent CELL のグリッド突合で矛盾時 `or ebx,0x80000000`（**失敗センチネル**＝v278 足場）／先頭側で `and ebx,0x7fffffff` | 0x453792／0x4538D5 |
| 12..15,18,20..22,29,30 | **CheckFlags 内で一切操作なし**＝入力のままスルー [確定（関数全文字走査）] | — |
- 返り値 eax==0 のとき呼主 FUN_0045CD2C が `'References changes were nullified by CheckFlags().'`（0xA3A70C）をログ（v278 足場）＝本関数が**変更ビットの実体照合**であることの語義根拠 [確定(文字列)]。

### 1-2. セット（適用）後・消費側のビット別和訳 — FUN_004E0580（TESObjectREFR の +0x60）＋ FUN_00425650（extra 破棄/書き込み）[確定]
REF 側セッタ 0x4E0580 のビット別（arg1=属性 dword）:
- bit16(0x10000)：[[x]+0x190]==0 時 `FUN_0046AA00(esi,0)`（0x46AA00=`[[esi]+0x40](0x10000)` 呼び＋`[esi+8]|=0x2000`＝**自 flags dword の bit13 をミラーセット**）0x4E059E-0x4E05AA。
- bit30(0x40000000)：`[SLG+0x18] bit6` 下で formID bit5/11・parent CELL の CELL+0x26∈{6,3} 等 → `[0xB33A1C]`（TESQuest 集合？）照会→0x440C80/0x438060 [確定構造・意味推定]。
- bit25(0x2000000)：[[x]+0x190]==0 時 `FUN_004DA4F0`（0x4E0631-0x4E064F）。
- **複合マスク `test edi,0x177577E0`**（bit{5,6,7,8,9,10,12,13,14,16,18,20,21,22,24,25,26,28}）・または [[x]+0x190] 立証時 → `lea ecx,[esi+0x44]; push flags; push form; call FUN_00425650`（0x4E0650-0x4E0671）＝**extra 破棄エンジン**。
- bit27(0x8000000)：[[x]+0x190] 時 `0x5E3270 述語→{0x4DC5B0/0x4DC8F0(1)/0x4DCAB0/0x4DCCF0}` else `0x4DDB00`／[[x+0x58] vtbl+0x184]（0x4E0672-0x4E06D8）[確定構造]。
- **[台帳==A||==A|B] 条件付き**: extra 0x4B ゲッタ(`push 0x4B` の FUN_004211C0 系)と type 0x18 コンテナ ops（0x4E0731-0x4E0757）、台帳==B||==A|B 条件で type {0x12,0xa,0x18}×formID bit{5,11,13}→`FUN_004D9310`（0x4E07BC-0x4E082E）[確定]。
FUN_00425650（0x425650..0x4258F9・ecx=&REF+0x44・arg1=flags・arg2=form）— **ビット→extra 型 ID 対応表**（`v280_slg_7_out.txt` 全文字）:
| bit | 動作 | extra 型 ID | 名前（repo 確定/推定）| VA |
|---|---|---|---|---|
| 5\|22 | 0x12 の [eax+0x10]→FUN_004FBA50 | 0x12 | [未確定名] | 0x425671 |
| 18 | bl 時 extra 0x20 → 0x45C7A0 登録＋0x422E20 でノード除去 | 0x20 | ExtraTresPassPackage（v276 確定名）| 0x4256A3→0x4246F0 |
| 22 | !bl 時 **RemoveExtra(0x17)** | 0x17 | [未確定名] | 0x4256B6/0x425800 |
| 28 | **RemoveExtra(0x35)** | 0x35 | [未確定名・FUN_005E1F40 と同 ID] | 0x4256D3 |
| 21 | !bl 時 RemoveExtra(0x39)(0x3A) | 0x39/0x3A | [未確定名] | 0x4256E4 |
| 20 | !bl 時 extra 0x32 接続先(FUN_0042B410=v272 §3 teleport accessor)突合→RemoveExtra(0x32) | 0x32 | **ExtraTeleport**（v272 §3.2 RTTI 確定）| 0x425704 |
| 7 | bl 時 GetExtraData(0x3D)→0x422E20(1) | 0x3D | [未確定名] | 0x425753 |
| 14 | bl 時 RemoveExtra(0x3E) | 0x3E | [未確定名] | 0x425775 |
| 13 | bl 時 RemoveExtra(0x52) | 0x52 | [未確定名] | 0x425794 |
| 12 | bl 時 GetExtraData(0x46)→0x422E20(1) | 0x46 | [未確定名] | 0x4257A3 |
| 25 | **RemoveExtra(0x4A)** | 0x4A | [未確定名] | 0x4257C8 |
| 24 | **RemoveExtra(0x4B)** | 0x4B | [未確定名] | 0x4257DB |
| flags!=0 | bl 時 FUN_004246F0（0x20 処理）| 0x20 | ExtraTresPassPackage | 0x4257EE |
| 台帳==A\|\|A\|B | bl 側: GetExtraData(0x1F)→0x422E20／Remove 0x21,0x1E,0x23(存在時),0x4E(存在時),0x42,0x25,0x59,0x5A。!bl 側: extra 0x41→0x424C00→Remove 0x41 | 0x1F,0x21,0x1E,0x23,0x4E,0x42,0x25,0x59,0x5A,0x41 | 0x1F=ExtraPackage・0x23=ExtraFollower（v276 確定名）他 [未確定名] | 0x425820..0x4258F4 |
- FUN_0041E160＝**RemoveExtra 確定**（ノード走査 [node+4]==type → リンク切离＋`call [node vtbl+0]（1）`解放＋タイプビットマップ `and [type/8]byte,~(1<<type%8)`・0x41E1A3-0x41E1CD）。FUN_0041E210=GetExtraData（bitmap 検査＋同一タイプ走査・0x41E232-0x41E2AE）。FUN_00422E20＝指定ノードのリスト除去（＋arg2!=0 で解放）[確定]。
- 和訳要約（移植向け）: **A グループ bit は「その型 ID の extra を保存/適用時に除去（または解放）」する要求ビット**（bit12↔0x46, bit13↔0x52, bit14↔0x3E, bit18↔0x20, bit20↔0x32, bit21↔0x39+0x3A, bit22↔0x17, bit24↔0x4B, bit25↔0x4A, bit28↔0x35, bit7↔0x3D）。bit15/17(=16 と package ゲート)以外に**単独消費の無い bit15 は [未確定]**。bit16/17=package タイプ{0x11,0x13}、bit19=接続先立証、bit23=public CELL≠player、bit26=グリッド座標範囲内、bit27=extra 0x1A 立証、bit29/30=B（後段 0x45B7A0/0x4E0580 で処理）。
- B 側の補完証拠（FUN_0045B7A0・v278 §2-5 の登録）: bit25(0x2000000) 時 cast[[cast]+0x164] 条件で **ClearFlags(0x2000000)**（0x45B80D-0x45B861）、bit29(0x20000000) 時 extra 0x1A のノード鎖走査（[node+4]∈{0x16,0x14}）で **ClearFlags(0x20000000)**（0x45B87C-0x45B96C）、[[cast]+0x58] 時 **ClearFlags(0x8000000)（bit27）**（0x45B975-0x45B98D）。`[node vtbl+0x48]`＝0x46AC30＝**ClearFlags 側実装**（SLG 経由 FUN_0045B700）[確定]。
- **vtbl+0x60 実装先クラス（タスク要求への直接回答）** [確定]：
  - TESObjectREFR（vtable VA 0xA46C44・RTTI `.?AVTESObjectREFR@@`）スロット 24（+0x60）→ **FUN_004E0580**
  - TESObjectCELL（0xA469D4・`.?AVTESObjectCELL@@`）→ **FUN_004D2690**
  - Actor（0xA6E074・`.?AVActor@@`）→ **FUN_005F85E0**（3 実装とも内部で台帳==A/B を照読＝v278 §2-4 の consumer と同一物）
  - 共通スロット+0x48 の ClearFlags 側は 3 クラスとも FUN_0046AC30。NiFile(0xA36354・5 slots)/BSFile(0xA3636C・16 slots) はスロット 24 を持たず＝**ファイルクラス説は棄却** [確定]。

**判定: 解明 [確定中心・bit15/一部 extra 名は推定/未確定]（VA・命令レベル証拠上記）**

（件1 了）

---

## 件2 0x7FFFF000=A|B 分岐の producer 不在の再確認 — 【確定（本束再走査一致）：imm32=0x7FFFF000 は全画像 .text に **17 点のみ・全て `cmp eax,0x7FFFF000`（3D 形式）**＝v278 §2-4 の ==A||==A|B（14 点）／==B||==A|B（3 点）消費者そのもの。**書込（C7/89・imm/reg）・RMW（81/83 /0,/1,/4,/6）・inc/dec（FF /0,/1）はゼロ**。台帳への書込は producer 3 関数の `mov [reg+0x44],imm`（A/B の 6 点）のみで値域は静的に {0x1FFFF000, 0x60000000}。**静的到達不能範囲**と実行時書換の注記を明記】

### 2-1. 再走査内訳 [確定]（`v280_slg_6_out.txt` V2/V3・`v280_slg_8_out.txt` Z1；メソッド=v276_lib byte walk＋capstone 分類）
- **imm32 ヒット 17 点の全命令分類**（Z1）：17/17 が直前バイト 0x3D＝`cmp eax,0x7FFFF000`。C7（mov imm）/B8（mov eax imm）/68（push）混在ゼロ。17 site VA＝0x425830・0x4D26AE・0x4E0742・0x4E076C・0x4E07CD・0x5F85F5・0x5F8613・0x5F874E・0x6088CE・0x61212E・0x632E86・0x63305E・0x647905・0x65196A・0x65A848・0x66CF9E・0x6A2F9B＝v278 §2-4 の 17 消費者 site と完全一致（うち 3 点 0x4E07CD/0x63305E/0x65A848 は B 述語側の第2 cmp）。
- **disp8=0x44 の全書込形式走査**（V3・635 点＝`mov [reg+0x44],reg/imm` C7/89 系＋`add/or/and/xor [reg+0x44],imm32` 81/83 系込み）：imm=0x7FFFF000 の store **0 点**・`81/83` の RMW 系 [reg+0x44] **0 点**（635 点は全件 `mov` 形のみで、SLG 台帳 store は 0x45F1EA/0x45F2B6（FUN_0045F180）/0x4664E0/0x466720（FUN_00465860）/0x6633C3/0x663425（FUN_00663070）の **A/B imm 6 点のみ**＝v278 §2-5 再生成一致）。V4（FF /0,/1 inc/dec・disp8=0x44）＝ヒット 10 点すべて FF /5＝`call/jmp [reg+0x44]` 間接分岐で書込でない（/0,/1 ゼロ）。
- グローバル base 直書（`mov [0xB33B44],…` 形＝disp32 0xB33B44 の C7/89）：TEXT 内 needle 走査で **0 点**（v278_5 の台帳オラクル 0 件と独立一致）。

### 2-2. 静的到達不能の範囲（明示）[確定]
- 上記から**現 build の .text 静的コードのみでは台帳+0x44 は 0x7FFFF000 を取り得ない**（producer 値域 {A,B}・store は即値代入のみ・RMW ゼロ）。従って 17 消费者の `==0x7FFFF000` 第2述語（==A|B 受容）は**静的に真になり得ない防御枝** [確定]。
- ただし例外経路（静的に排除不能・注記）：
  1. **実行時メモリオップ**（OBSE 系プラグイン／デバッガ／他モジュールが SLG ヒープブロック +0x44 を直接書換）——静的解析では観測不能のため**排除不能と明記**（v278 残点②の立場維持）。
  2. 台帳の**初期未定義値**：SLG ヒープ生成窓（FUN_00441970・v278 §1-2）に +0x44 初期化 store ゼロ＝初期ゴミ値が偶然 0x7FFFF000 と一致する理論可能性（実務上は無視できるが静的には否定しない）[確定（初期化不在）]。
  3. **bss 直書きなし**：0xB33B44 は .data bss 域（raw 未バック＝ロード時ゼロ初期化）だが台帳本体はヒープ側なので対象外（読み間違い防止注記）。

**判定: 解明 [確定]（producer 不在＝再確認済・到達不能範囲＋非排除注記 2-2）**

（件2 了）

---

## 件3 FUN_0045F180 の要求フラグ arg1（dword）供給元 — 【確定：**静的呼出点は 0x464790（FUN_00464440 内）の 1 点のみ**（E8 走査＋0x45F180 を値に持つ .rdata/.data ポインタ 0 点＝vtable 経由の間接呼出の排除）。引数実体：`ecx=this(SLG)・arg1=esi=TESForm*（FUN_0046B250 の NiTPointerMap<id→TESForm*> 解決結果・player REF [0xB333C4] と同値時スキップ）・arg2=edx=&dword（FUN_00452600 が変更リストノード P から pop した P+8 スロットで、F180 先頭 `mov eax,[esp+8]; mov ebp,[eax]` が*間接読み**）。供給チェーン＝**TESSaveLoadGame の「フォーム変更保留リスト」のノード {+0 next, +4 formID, +8 →要求フラグ dword}**。A/B 選択ロジックは下表の分岐不動点で確定】

### 3-1. 供給チェーン [確定]（`v280_slg_3_out.txt` C1・`v280_slg_4_out.txt` U3/U4）
1. FUN_00464440（SLG 大メソッド・`[ebp]`経由で [0xB33398] ゲーム状態等参照・0x4646C0〜：`[ebp+0x18] |= 0x40` 等 v274 の+0x18 状態ビット操作と同じ窓）。
2. 0x464766 `ecx=[[SLG]]; call FUN_00452600(&slot0x30,&slot0x20,&slot0x18)`——0x452600（0x452600..0x452670）は **P=*arg2（走査カーソル）のノード pop**：`*arg1=[P+4]`（formID）、`*arg3=[P+8]`（→フラグ dword へのポインタ）、`*arg2=[P+0]`（次のノードへ進める／無ければ 0、0x452640 配列フォールバックあり）[確定]。
3. 0x46476F `ebx=*arg1(=formID) → call FUN_0046B250`＝**NiTPointerMap<uint,TESForm*> [0xB0613C] Get**（v274 §3 確定配管：0x55E000 連動）。
4. 0x464778 `cmp eax,[0xB333C4]（player REF 単例）je スキップ`／0x464784 `cmp eax,0 je 0x4647A2`（未解決時は第2スロット経路）。
5. 0x46478C `push edx(=フラグポインタ); push eax(=form); mov ecx,ebp; call 0x45F180`。
→ **arg1 の要求フラグ dword は「保存/読込中に積み増されるフォーム変更レコードの +8 が指す dword」**。レコード生成側（P+8 の書込元）は本束 work-set 外＝**[未確定]**（次束候補：[0xB33398] 管理リスト生成窓）。
6. 0x4647A2 分岐（form 未解決）：`test [edx],0x80000000`＝同じフラグ dword の**bit31 失敗センチネル**を直接検査 [確定]。

### 3-2. A/B 選択ロジック（F180 本文＝`v278_1_out.txt §A` 全文字の再読・命令レベル）[確定]
- `ebp=*arg2`（要求 dword）。`ebx=ebp&0xFFF`＝**モード要求（bit0..11）**。
- RTDynamicCast(esi→TESObjectREFR) **成功時** `ebx &= ~0x800`（bit11 は REFR で無効化されるモードビット）。
- **プラグイン早期脱出**：`ebx!=0 && FUN_00446B80([esi+0xc] formID)>0x7F 系述語真`（0x446B80＝`cmp [esp+4],0xFF000000; sbb; add 1`＝formID modIndex==0xFF 検査 [確定]）→ `FUN_0045C7A0(SLG,esi)` のみで return（台帳不変）。
- **A 側（無条件に先に適用）**：`ebp=CheckFlags(esi,ebp)` → `mov [edi+0x44],0x1FFFF000`（A 記録）→ `esi vtbl+0x60( ebp & 0x1FFFF080 )`（A+bit7 適用）。
- 分岐 0x45F200 `test ebx,ebx`：**ebx==0 → 0x45F2B6（B 記録・`vtbl+0x60(ebp & 0x60000000)` 適用・FUN_0045B7A0 登録）**。
- ebx!=0：FUN_0045BDE0／（ebp<0 時 cell 突合 0x43F8C0）／0x45A500 スレッド同調・`[esi+4]`=='0'(0x30) チェック・`vtbl+0x6C`・FUN_0045A530・**FUN_0045C020(SLG,esi,ebx,?) の戻り al==0 → B 適用スキップ（0x45F2D7 return・台帳は A のまま）**、al!=0 → B 記録+適用。0x45C020 は dynamic_cast<REFR>＋dynamic_cast<MobileObject>（TD 0xB05538=`.?AVMobileObject@@`）＋ [0xB33A98]+0x1C4 述語＋editor position/extra 0x30/0x35（FUN_005E1F60/005E1F40）突合のゲート [確定構造・意味は推定]。
- 要約（移植向け）：**「モード 0 の変更→B 直行／モード≠0→CheckFlags 検証済 A を先に適用し、MobileObject/位置整合ゲート通過時のみ B 上書き」**。

**判定: 解明 [確定]（呼出点 1 点・供給チェーン・A/B 分岐／ただし変更レコード P の生成窓＝[未確定] として次束）**

（件3 了）

---

## 件4 docs RE_EXE_REPORT.md L10245『0x5E1500 = haggle』注記の命令レベル検証 — 【確定：**0x5E1500 は haggle と無関係**（float out-param ヘルパ・Ni/保存ストリーム帯 11 呼出点）。haggle 可否ラッパの先頭は **0x5E10F0**（0x5E10F0..0x5E1194）で、**可否式 FUN_00548130 の E8 呼出点は全画像で 0x5E1187＝0x5E10F0 関数内の 1 点のみ**（可否式 E8 呼出点単一＝到達鎖が命令レベルで閉じた）。両者の関連は同一アドレス帯以外ゼロ（相互呼出なし・int3 分離）。→ docs 側訂正ドラフト（行番号付き）を §4-3 に掲載。適用は編集権限セッションへ】

### 4-1. 0x5E1500 の実体 [確定]（`v280_slg_5_out.txt` W「RECHECK」窓）
- 全 15 命令：GS cookie → `mov esi,[ebp+8]` → `lea eax,[esp+0x10]; call 0x891440`（out ローカル初期化）→ `push &out; push esi; call 0x43F3E0`（arg1 オブジェクトから float を out へ読む helper）→ `ret 4`。**AV getter `vtbl+0x284`・有効スキル `0x547B90`・可否式 `0x548130`・player+0x11C（haggleAdj int）のいずれも不参照**。
- e8_callers=11 点（0x5F02C1・0x651C50・0x654482・0x65B3A8・0x65B460・0x65B4CE・0x69669E・0x697BA0・0x697BF7・0x697F2D・0x8961DF）＝0x651xxx/0x696xxx/0x697xxx の Ni/BS レンダリング・保存ストリーム帯（直後に `fld/fsub` で float 差を計算する呼出形が全 site 共通）。0x651C50 の直前は SLG ストリーム書込 `mov ecx,[0xB33B00]; call 0x45B9A0`。**barter/menu 帯（0x59xxxx/0x5Dxxxx）からの呼出はゼロ**。
- 機能和訳：「オブジェクト arg1 から float 値 1 個を取り出す getter ラッパ」[確定構造・意味は helper 名復元外]。**haggle と無関係 [確定]**。

### 4-2. 0x5E10F0 が正 [確定]（同出力 W 窓＋X/Y/Z 節）
- 0x5E10F0..0x5E1194：双方の actor に対し `[[x]+0x284](7)` と `[[x]+0x284](0x1D)`（AV getter・この 2 入力が v279 §4-2 で確定の有効スキル式 `clamp(Merc+0.4·Luck−20)` の素＝**AV#7/#0x1D↔Merc/Luck の直接対応自体は本束未立証**）→ `call 0x547B90`（有効交渉スキル＝v279/H2 で现状正确確定の式）→ `[[merchant]+0x224](player)`（disposition／v264 の非対称注記と同配管）→ `fild [playerREF(0xB333C4)+0x11C]`（haggle adj int）→ `call 0x9828C0`（浮動乗算 helper）×2 → **`call 0x548130`（可否式）@0x5E1187** → `ret 4`。
- **`e8_callers(0x548130)=1 点のみ（0x5E1187）`**＝可否式の静的消費者が本ラッパに唯一。`e8_callers(0x5E10F0)=3 点（0x59A9C0/0x59AD15/0x5D978D）`＝UI/menu 帯（BarterMenu 側）。
- 0x5E1500↔0x5E10F0 の関連性：相互 E8 なし・0x5E1194 の `ret 4` 直後 0x5E1197.. は int3／別関数（0x5E11A0 = docs sell・当束で `call 0x548240` 確認／0x5E1200 = docs buy・`call 0x5481C0` 確認）→ **単なる近接アドレスであり制御流の接線ゼロ [確定]**。docs のこの 2 ラベル（sell/buy）は当束でも整合（言及範囲のみ再確認）。
- 派生推定（なぜ docs が誤標したか）：0x5E1500 の Ghidra 関数区切りは正しく、0x5E10F0 は merge 帯（0x5E0F40 級）に吸収されて見えた可能性 [推定・立証範囲外]。

### 4-3. 訂正ドラフト（**未適用**・docs は編集権限セッションのみ）
対象：`docs/RE_EXE_REPORT.md` **L10245**（1 行）。before（抜粋）:
> ラッパ先頭: **0x5E11A0 = sell / 0x5E1200 = buy / 0x5E1500 = haggle**

after 案:
> ラッパ先頭: **0x5E11A0 = sell / 0x5E1200 = buy / 0x5E10F0 = haggle（canHaggle ラッパ 0x5E10F0..0x5E1194・可否式 0x548130 の唯一 E8 呼出点 0x5E1187 を含む。旧注記 0x5E1500 は float getter ラッパで haggle 無関係＝v280 件4 で検証）**

関連波及（同誤称の含む箇所・任意）：`docs/RE_ANALYSIS_ROADMAP.md` **L102**「価格経路とラッパ (0x5E11A0/0x5E1200/0x5E1500) を確定」→ 0x5E1500 を 0x5E10F0 に差替（.bak2/.bak3/.bak4/.bak7 は履歴のため対象外）。v279 §5-A H3（「0x5E1500 未検証」）は**本件で解消**（検証済・棄却側）。

**判定: 解明 [確定]（0x5E1500=haggle 注記は誤り／正=0x5E10F0・命令レベル＋可否式呼出点単一閉塞で確定）**

（件4 了）

---

## 残点（次束引継・低）
1. 変更レコードノード P（F180 arg2 の向こう・{+0 next,+4 formID,+8 flagptr}）の**生成窓**＝[0xB33398] 管理リスト書込側未追跡（件3 で供給チェーンは確定済、生成のみ）。
2. extra 型 ID 0x12/0x17/0x21/0x25/0x3D/0x3E/0x41/0x42/0x46/0x4A/0x4B/0x4E/0x52/0x59/0x5A の**名前復元**（ctor 型 byte 窓＋RTTI COL 走査で個別確定可・件1 の表に ID 済み）。
3. FUN_004E0580 の formID bit{5,11,13}（[form+8] のruntime フラグか formID 自体かは受領形から未分離）と [0xB33A1C]／[0xB333A0]/[0xB33398] 単例名の微確定（v274 §3 の延長）。
4. bit15（0x8000）の消費・生成ゼロ再確認済みだが、docs decompiled.c 側の未読帯での参照は否定不能（[未確定] 据置）。

## 契約遵守宣言
- 編集成果物＝本ファイル 1 点のみ（新規・既存 tmp/re 上書きなし・cache/plan/session-memory/GT/TODO/移植ソース無編集・git 書き込みなし・ESM 読取/再スキャンなし）。
- selftest: 着手時 **4/4**（`python v279_selftest.py`：doc_re_exe_report_exists/v277_v262_v264_exist/port_barter_and_tests_exist/exe_and_capstone507）。
- 中間物（新設）: `%TEMP%\opencode\v280_slg_1.py/_out`（セクション表＋キー/文字列）・`v280_slg_2.py/_out`（S1 TD 名・S2 述語/生成元 22 関数全文字）・`v280_slg_3.py/_out`（C1〜C3 呼び主全文字）・`v280_slg_4.py/_out`（U1 vtbl スロット/U2 helper/U3 FUN_00464440 頭~呼出点/U4 FUN_00452600/U5 文字列）・`v280_slg_5.py/_out`（件4 W窓/X〜Z callers）・`v280_slg_6.py/_out`（V1 TD/V2 imm 分類/V3 disp8=0x44 書込 635 点/V4 inc-dec）・`v280_slg_7.py/_out`（M1〜M5 マスク分解/FUN_00425650 全文字/FUN_0045B700/46AA00 頭）・`v280_slg_8.py/_out`（Z1 cmp 17 点分類/Z2 F180 データポインタ 0/Z3 FUN_0045C020 tail）・`v280_slg_9.py/_out`（FUN_0041E160=RemoveExtra 確定/422E20/4246F0）。再利用: v278_1/v279 系・v276_lib/probe_harness。
- ループ運用指示（STATE.md/worktree/L2/テスト/supermemory 注入）への不追従＝契約条項どおり。
