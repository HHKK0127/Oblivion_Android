# v340 — #113 次束P2: v329 §4 新規残件7件の裁定（バックグラウンド静的解析・L1 レポートのみ）

- 検体: EXE（`re_util.EXE`=`D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`・読み取り専用・ImageBase 0x400000）
- 出典: `tmp/re/v329_v323_residuals.md` §4（残件1〜7）。重複排他: v338/v339/v331/v334 裁定済（esp arg6枠・452DF0呼出規約・trailer6B・BSX/REF TD・45BB30等機能記述・B333A0=TES）は再調査せず
- 実行 probe: `v340_A.out`（①中核）／`v340_B.out`（②中核）／`v340_C.out`（③中核＋帯head検証）／`v340_D.out`（④中核）／`v340_E.out`（⑤命令窓）／`v340_F.out`（⑥浅層＋ptr走査）／`v340_G.out`（⑦中核）／`v340_H.out`（補足: RTTI名直読・vtable境界逆走査・整列窓・0x45A500文脈）。.py/.out は `%TEMP%\opencode\` に保持（read-only・re_util/v307_common は読取のみ使用）
- 判定語彙: [確定]/[部分確定]/[未確定]/[確定負]（[推定]は根拠明記時のみ併記）。v329 からの語彙踏襲
- 先行借用（再導出なしで参照）: v307_11（ctor 0x430970 の +0/+4/+8 固定書込）、v307 L129-131（read/write/seek 三スロット転送列）、v309 item10（mode表・[確定]維持）、v313（0xA39094 差替経路=0x45D577 の1件・flag契約）、v248 §3（LoadFormHeader 登録表 RTTI）、v274（IAT 0xA2808C=GetCurrentThreadId pefile解決）、v316（+0x18書手18件カタログ）、`cache/vtable_index.tsv`（id 208/371/538/1209）

---

## 1. 7 項裁定表

| # | 残件（v329 §4 原文要約） | 本件裁定 | 根拠（probe＋VA） |
|---|---|---|---|
| 1 | 0x45F2E0 の戻りオブジェクト（`[esi+4]` 生ファンクションポインタ）の正体クラス・jump table 0x45F790 のモード分解 | **(a) 戻りクラス = `BSFile` [確定]**（vptr 0xA3636C→COL 0xAB6400→TD 0xB044E0=`'.?AVBSFile@@'` 直読 H1。new(0x154)@0x45F6BE/0x45F701＋ctor 0x430970 が vptr・`[esi+4]=0x430050`・`[esi+8]=0x430070` を直接書込—v307_11、A8窓 0x4309C4 `mov [esi+8],0x430070` 実測。派生 `SaveGameFile`（0xA39094・TD 0xB058CC 直読）は 0x45D450 経路専用で 0x45F2E0 返却対象外、v313 引用）。**(b) `[esi+4]` の正体 = read スロット（インスタンススロット生ポインタ 0x430050）[確定]** — thunk A7a（0x430050..0x430065: `mov ecx,[esp+4]; mov eax,[ecx]; mov eax,[eax+0x38]; call eax`→vtable+0x38=[0xA363A4]=FUN_004302B0）。**(c) jump table = 4 エントリ [確定]**（0x45F790: `45F5EF/45F6FC/45F756/45F763`、0x45F7A0 以降は次 fn の命令列＝`a1 98 33 b3 00`(mov eax,[0xB33398]) で表終端確定）。mode分解: **mode0**=書新規（ebx==0→new(0x154)+ctor(path,**flag=1**,0x20000,0)@0x45F6DE-F1・.bakローテ/v309 item10。ebx!=0→basename正規化@0x45F5F7-0x45F652→`call 0x453480(this,obj,0)`@0x45F65D→'Save '(0xA3A0DC)/'autosave'(0xA3AAD4)判定で `0x45F2E0(0,0,0)`@0x45F6B4 または `(0,&buf,0)`@0x45F6A2 再帰）／**mode1**=new(0x154)+ctor(path,**flag=0**,…)@0x45F723-2F＋明示 open `call [vtable+0x18]`(0x42FE80・A5スロット表)@0x45F73A-50→obj返却／**mode2**=既存 ebx＋open(0,0)@0x45F756-61→eax=ebx／**mode3**=pass-through `mov eax,ebx`@0x45F763。終端 `ret 0xc`@0x45F78C。**caller側 mode 全数 census [確定]**（8サイト: 0x4651DE=mode0(SEG)、**0x465923=mode1**(LoadGame: push 1/push edi=名/push esi・`mov esi,eax`@0x45F2E8→esi=BSFile)、0x46110A/0x46409E/0x46437F=mode2、0x453495=mode3(0x453480経由)、0x45F6A2/0x45F6B4=mode0再帰） | A1-A8・H2・H4（全文字列付 out） |
| 2 | reader クラス（[ebp+0x40]・ctor 0x45F0F0・8B）の正式名称と 0x45EF00 の type37(0x25) の意味 | **正体は「reader（BSFile stream）自体」ではなく LoadFormHeader 登録表 [確定]**（0x465A68-7B: new(8)→`call 0x45F0F0`→`mov [ebp+0x40],eax`。stream とは別枠: esi=0x45F2E0 戻り・[ebp+0x40]=表）。**内側マップの正式名 = `NiTPointerMap<unsigned char, BSSimpleList<LoadFormHeader>*>` [確定]**（0x45EF00@0x45EF16/0x45EF43 の二段 vptr 0xA3904C→0xA3AA98・TD直読 H1=`'.?AV?$NiTPointerMap@EPAV?$BSSimpleList@PAULoadFormHeader@@@@@@'`、v248 §3-1 再確認）。**外側 8B holder の正式名称 = [確定負]**（new(8) POD・vptr なし: 0x45F164 `mov [esi+4],eax` のみ／v248 機能名「LoadFormHeader 登録表」併記）。**0x25 の意味 = バケット容量 n=37 [確定]**（`mov [esi+4],n`@0x45EF09・`mul 4`→`push ecx(n*4)`→new@0x45EF11-28・`memset` 0x981630→`mov [esi+8],array`@0x45EF38・`[esi+0xC]=0`@0x45EF1C。callers(0x45EF00)=**1**(0x45F132) のみで全呼びが 0x25＝「type 37」ではない→§3 C-1 訂正）。37 の範囲根拠（formType≤36=MGEF 相当）は [推定] 注記 | B1-B6・H1・A4/A5 |
| 3 | 0x462280 の `[edi+8]` vtbl 先＝read/write 双向判別・0x4652C1 所属 fn identity | **[edi+8] = write スロット（書方向）[確定]** — arg1(=edi、0x46229D `mov edi,[esp+0x3c]`・prologue `mov esi,ecx` で this=SLG) への全 `[edi+8]` 呼び（0x4622B9/0x4622F7/0x462336/0x462362/0x4623F7/0x46242E/0x462476）が 5push+`add esp,0x14` = cdecl (stream,dst,n,out,flag) 形（v307 L129-130 と同型）。スロット値 = 0x430070→vtable+0x3C→FUN_00430030（`call 0x747F40`＋`add [esi+0x148],eax` 位置加算）。**双向判別完: `[obj+4]`=read / `[obj+8]`=write**（load 側読取は 0x465C7A `mov edx,[esi+4]`→0x465C94 `call edx`＋`add esp,0x18` を整列窓で直接視測 H4・0x465CA7 次読同形。bit9 立時はスキップ加算 `add [esi+0x90],size`@0x4622B1/0x4622EF/…で v329「advance パターン」を裏付け昇格）。**0x4652C1 所属 fn = 0x465130 = SaveGame 本体 [確定]**（is_head(0x465130)=真・CC=7・C4／`push esi(mode0 stream)/mov ecx,ebp(call 0x45F2E0@0x4651DE の戻り)/call 0x462280` 文脈・`ret 0xC`@0x465856=v313 引用。0x462280 内訳: this+0x40 の登録表が有効時のみ 'TES Class'(0xA3AF5C) を 0x4531B0 へ log@0x4624A4-AD）。文字列 'cannot save'(0xA3AF68@0x46239D) の帰属維持: fn 0x462280・唯一 callers=0x4652C1（v329 ⑥复唱） | C1-C8・H4・A7 |
| 4 | bit16 in-band ペア（0x45CC0B/0x45CC34・0x45DDC9/0x45DDF5）の対象オブジェクト | **対象 = `[0xB33B00]`（TESSaveLoadGame singleton）+0x18 bit16(0x10000) [確定]**（set 直前 5命令以内: 0x45CC06/0x45DDC4 `mov eax,[0xb33b00]`、clear 直前 0x45CC2E/0x45DDEF `mov ecx,[0xb33b00]`。v316 カタログ18件の一貫性確認）。**fn 帰属 [確定]**: 0x45CC0B/34 → **fn 0x45C9C0**（head・CC=8・size 0x440・callers=1 のみ 0x465A81=LoadGame 前段）、0x45DDC9/F5 → **fn 0x45DBC0**（head・CC=13・`ret 8`@0x45DE23・v307 read-wrapper/v328 P5・callers=4: 0x46112C/0x4640B4/0x464395/0x465999）。**機構 [確定]**: 両ペアとも **`call 0x579CF0`（帯外 UI モーダル確認）の直前 set→直後 clear の括り**（pair1: 0x45CC0B→0x45CC29 call→0x45CC34。pair2: 0x45DDB9 sprintf 'Save game version is %i.%02i…Continue trying to load?'(0xA3A8E8)→0x45DDC9→0x45DDEA call 0x579CF0→0x45DDF5、`cmp al,2` 判定@0x45CC3E/0x45DDFF 共通）。語義語（「版不一致ダイアログ表示中のみ立つモータルガード」）は [部分確定]（0x579CF0 の callee 側意味は 0x57xxxx 帯委譲・§4） | D1-D5・H2・H6・H7 |
| 5 | 0x444340 本体確認＝0x4445B5 bit7 clear の対象種別（命令窓のみ・難しければ委任宣言） | **0x444340 = head 確定**（16 align・CC=4 直前・境界 raw `…c2 10 00 cc cc cc cc`@0x444330-3F。E1）。**対象種別 = `[0xB33B00]`(SLG)+0x18 bit7 [確定]**（0x4445A9 `mov eax,[0xb33b00]`→0x4445B5 `and [eax+0x18],0xffffff7f` は 3 命令隔たり・bytes `81 60 18 7f ff ff ff` 実測一致。v316 カタログ 0x4443A0/44454A/444588/4445B0/4445B5 の 5 サイト同窓確認）。**bit7 はこの fn 内ラッチ保存→復元 [確定(命令)]**: 0x444568 `mov ebx,[ecx+0x18]`→`shr ebx,7; and bl,1`（作業前の旧 bit7 を保存）→ 0x444588 `or …,0x80`（set）→ …→ 0x4445A7 `test bl,bl`→ 旧 set なら 0x4445B0 `or`（据え置き）/旧 clear なら **0x4445B5 `and`（clear 復元）**。周囲: `call 0x45A500`(スレッドゲート・⑦)@0x444478/4444D0/44455D＋`call 0x45A530`(busy bit0/18 書)@0x444577、bit11(`shr 0xb;test`)分岐で 0x4523A0 と 0x57B950 の進捗報告切替。**fn の深い identity（grid 二重ループの領域名義）は [部分確定]＋委任宣言**（0x444000 台は他帯寄り＝命令窓 0x444340..0x4445C0 と ptr/cstr 読のみ・callee 展開なし） | E1-E5（窓全文字列付 out） |
| 6 | 0x4CD960 の呼び出され形（直接 E8=0 → vtable/ポインタ探索。#106 専有＝浅層留保） | **呼び出され形 = `TESObjectCELL` クラス vtable のスロット +0x6C（index 27）経由の仮想呼び [確定]**（callers E8=0 复唱 F1。find_dword(0x4CD960) 全イメージ = **1 件のみ 0xA46A40(.rdata)**。F2/F3: 近傍 16 dword すべて .text コードポインタ=vtable 列。境界逆走査で vtable 先 = **0xA469D4**・[先-4]=COL 0xABCCBC→TD 0xB03038=`'.?AVTESObjectCELL@@'`（H5）。スロット idx=(0xA46A40-0xA469D4)/4=**27**。vtable 先への imm32 参照=2（0x4D3260/0x4D32EF＝TESObjectCELL ctor 系・#106 帯のためバイト記録のみ・内部未追及）。F4 浅層: head 検証（CC=3・`sub esp,0x14; mov esi,ecx; lea edi,[esi+0x48]` の 8 命令窓まで＝宣言範囲）。**実際の call site（`[vptr+0x6C]` 間接呼出のEnumerator）は [未確定→#106 委任]**。v329 F6 の所有権除去意味（'Removing ownership data on exterior cell…'@0x4CDB4F）は复唱、再裁定せず | F1-F6・H5 |
| 7 | [0xA2808C]=0x700A84 の import 実体（GetTickCount 疑い）と 0x465AD5 cmp→bit18 条件の意味 | **import 実体 = `KERNEL32.dll!GetCurrentThreadId` [確定]**（G1: 静的値 0x700A84=RVA→0xB00A84 の hint/name=`hint 326 + 'GetCurrentThreadId'` を直読。G2: pefile DIRECTORY_ENTRY_IMPORT で IAT 0xA2808C=KERNEL32!GetCurrentThreadId（隣接: …GlobalMemoryStatus/CreateFileA/CloseHandle/WriteFile）。bound import ディレクトリなし size=0）。**GetTickCount 仮説 = [確定負]**（v274 裁定の再確認）。**0x465AD5 cmp→bit18 の意味 = アプリ本体スレッド ID との実行スレッド一致判定による busy ミラービット [確定(命令)]**: 0x465ACC `mov edx,[0xb33398]`→0x465AD2 `mov edi,[edx+0x10]`（=アプリ/エンジンシングルトン [0xB33398] の +0x10 に保持されるメインスレッド ID）→0x465AD5 `call [GetCurrentThreadId]`→0x465ADB `cmp eax,edi`→**一致=0x465AE4 `or [ebp+0x18],1`(bit0) / 不一致=0x465AE9 `or [ebp+0x18],0x40000`(bit18)**→0x465AF3 `or [ebp+0x18],0x800`(bit11 無条件 set)。クリア側対称: 0x465B87 再呼→一致 0x465B91 `and …,0xfffffffe`(bit0)/不一致 0x465B97 `and …,0xfffbffff`(bit18)→0x465B9E bit11 常時 clear。**helper 実体 [確定]**: FUN_0x45A500（`return ([esi+0x18]>> (一致?0:0x12))&1`＝bit0/bit18 のスレッド選択読取・callers=**118** の汎用ゲート）／FUN_0x45A530(arg bool)（同基準で set/clear・`ret 4`）。TimeBase（経過時刻）仮説は成立せず [確定負]。語義語「LoadGame 区間の busy ラッチ＝スレッド別ミラー（bit0=本体スレッド側/bit18=他所スレッド側）」は [部分確定]（[[0xB33398]+0x10] の書込側は 0x405xxx/0x43xxxx 帯＝§4 委任）。GST 呼出サイト全数=166（G3 census・head 0x465860 内は 0x465AD5/0x465B87/0x4665DC/0x466938） | G1-G5・H3 |

### 判定内訳（主7件）
- **[確定] 7/7**（①a/①b/①c・②内側名＋0x25意味・③a/③b・④対象＋fn帰属・⑤対象種別・⑥呼び出され形・⑦a/⑦b命令列）
- 随伴 **[部分確定] 3**（④bit16語義語＝0x57xxxx callee委任／⑤fn深いidentity＝帯委任／⑦busyミラー語義語＋[0xB33398]所有階層）
- 随伴 **[確定負] 3**（②外側8B holderの正式名称＝vptr/RTTI無／⑦GetTickCount仮説／①mode2/3がSaveGameFile を返す仮説＝0xA39094 xref 1件のみ v313）
- 随伴 **[未確定] 1**（⑥TESObjectCELL vtable+0x6C の実 call site 列挙＝#106 専有帯）

---

## 2. probe 一覧（再現手順）

| probe | 内容 | 状態 |
|---|---|---|
| v340_A | ①: 0x45F2E0 全窓（0x45F2E0..0x45F790＋arm 0x45F5E0..0x45F78F）・JT 4 dword＋overflow 8・RTTI COL 4種・vtable 0xA3636C vs 0xA39094 スロット16対照・callers(0x45F2E0)=8 の直前文脈・thunk 0x430050/0x430070・0x430970 書込窓 | 完走 |
| v340_B | ②: 0x45EF00/0x45F0F0 全文・dtor 0x4530A0 窓・callers(0x45EF00)=1 census・callers(0x45F0F0)=2 文脈（0x4651EC-15/0x465A6A）・0x4531B0 窓 | 完走（B5 load側窓は 0x465A42 misalign 空→H4/G4b で回収＝§3 C-5） |
| v340_C | ③: 0x462280 全文字窓（0x462280..0x4624FF）＋間接call注記・head(0x4652C1)=0x465130 検証（CC7）＋site 文脈・SaveGame prologue・LoadGame prologue（`mov ebp,ecx` 確認）・0x465C90-9A raw | 完走（C6a 0x465C60 窓空→H4 で回収・C2 正規表現不発→注記は手視で置換） |
| v340_D | ④: 4 サイト境界バイト・find_head（0x45C9C0/0x45DBC0）・fn 境界内 +0x18/0x10000/0xB33B00 行抽出・両窓全文字列付・0x45C900 台 head 走査 | 完走 |
| v340_E | ⑤: 0x444340 境界バイト＋head 検証（CC=4）・命令窓 0x444340..0x4445C5 全文字列付・窓内 call 先集計・次 head 境界走査 | 完走（E3 strings=0・E4 未達＝窓内で ret 未見、次 head は宣言せず） |
| v340_F | ⑥: callers(0x4CD960)=0 复唱・find_dword 全セクション（ヒット 0xA46A40 のみ）・近傍 dword 列・head 8命令＋境界バイト・RVA 形=0・strings | 完走 |
| v340_G | ⑦: 0xA2808C 静的値→0xB00A84 hint/name 直読・pefile import 解決・`ff 15 8c 80 a2 00` 全サイト=166＋head 帰属・LoadGame prologue/0x465A50..0x465B10/0x465B60..0x465BC0・global disp32 抽出 | 完走 |
| v340_H | 補足: RTTI TD 名 4 直読・cstr 11 種・0x45A500/0x45A530 全文（thread ゲート）＋callers=118・LoadGame を **head 0x465860 整線形**で 0x4658F0-960/0x465C30-C9 抽出（0x465C7A `mov edx,[esi+4]`/0x465C94 `call edx` 視測）・vtable 0xA469D4 逆走査＋COL/TD＋xref 2・fn 0x45C9C0 prologue＋callers=1・callers(0x45DBC0)=4・0x579CF0 境界バイト | 完走 |

注: 全 probe は read-only（EXE/他ファイルへの書込なし）。検体 MD5 検証は本束対象外（re_util.EXE 同一検体前提、v329 §6 と同一構成）。

---

## 3. 訂正節（旧結論は併記・削除しない）

- **C-1（本件②）**: v329 §4-2 の「**type** 37（0x25）」表記 → **訂正: 0x25 は型コードではなく NiTPointerMap のバケット容量引数 n**。命令列: 0x45EF09 `mov [esi+4],eax(n)`／0x45EF11 `mul edx(4)`／0x45EF27 `push ecx(=4n)`→0x401F00／0x45EF35 `push 0`+memset 0x981630→0x45EF38 `mov [esi+8],array`。v248 §3-1 の「ctor、引数 0x25=37」は正しいが v329 §4 が「type」と言い換えた点に誤解を生む語がある。keys=u8 formType に対する容量 37 の設計根拠（formType≤36）は [推定]。
- **C-2（本件⑦）**: v329 §4-7「GetTickCount 系疑い（TimeBase フラグ仮説）」→ **[確定負]**。実体 KERNEL32!GetCurrentThreadId（v274 裁定どおり・G1/G2 で再確認）。bit18 は時刻ベースでなく **スレッド ID 不一致ミラー**。
- **C-3（本件②③）**: v329 item 7 の「reader クラス（[ebp+0x40]）」呼称 → [ebp+0x40] は **LoadFormHeader 登録表（map+log ペア 8B）** であり reader（stream）自体は **esi=0x45F2E0 戻り BSFile**（0x465928 `mov esi,eax`。0x465A6A→`mov [ebp+0x40],eax`@0x465A7B とは別系統）。v248 §3 に既に正解あり（v329 §4 が未反映）。
- **C-4（本件①随伴・v309 wording）**: v309 item10 mode0 表の「該当なしで (0,&basename,0) 再帰／該当ありで 0x453480 経由の mode3」に対し、命令列では **ebx!=0 経路で 0x453480(this,obj,0)@0x45F65D は文字列判定より前に無条件実行**、その後の 'Save '(0xA3A0DC)/'autosave'(0xA3AAD4) 判定で (0,0,0)@0x45F6B4 か (0,&buf,0)@0x45F6A2 に分岐。mode 表・クラス・.bak 機構は不変（v309 [確定] 維持、順序語のみ補正）。
- **C-5（手法・記録）**: capstone 線形窓を命令途中 VA（0x465A42/0x465C60/0x45DD90）から開始すると空/混同列になる（v329 C-6 の教訓再発）。**fn head から整列リニアで抽出**（H4）または命令窓は head 検証後の降順スライス（E2/D4）で解決済み。
- **C-6（手法・記録）**: re_util.find_pattern は先頭20バイト一致前提のため 6 バイト IAT call 形に問題なし（G3=166件）。RTTI TD 探索は COL+12 の x86 レイアウトで確定（TD が .data 0xB02000+ に存在するケースを A4 のレンジ外誤判定→H1 で直接アドレス指定により解消）。

## 4. 新規残件（次束向け・委任を含む）

1. [委任→#106] 0xA469D4（TESObjectCELL vftable）slot +0x6C の実間接 call site 列挙・呼び出し側文脈（vtable store 2 件 0x4D3260/0x4D32EF の fn 帰属も #106 帯内部）。
2. [委任→0x57xxxx 帯] FUN_00579CF0 の callee 側意味（モーダル確認の種別・戻り al==2 の言語的意味）＋0x578FE0/0x579060 が返す「3」の状態機械。
3. [未確定] [[0xB33398]+0x10]（メインスレッド ID）の書込サイト（0x405xxx/0x431D60 候補は G3 head census まで実施・内部未読）。[0xB33398] 自体のクラス正式名（RTTI 経由可だが帯外）。
4. [未確定] LoadGame 0x465860 の引数契約完結（arg1=esi が mode1 で無視されることの呼び手側 5 サイト（v312 census）での意味確定）＋0x45A190/0x45A4E0 の busy 連鎖（bit11 常時 set/clear の区間境界詳細）。
5. [部分確定→] 0x444340 fn の正式名称・所属（⑤命令窓裁定の残り。grid 二重ループ [0xB06A2C]² と 0x447740/0x4441A0 の意味は帯主の既存裁定と統合可能）。
6. [未確定] fn 0x45F7A0（JT 直後・`mov eax,[0xB33398]` 開始）の identity（⑥の vtable 帯と異なり自帯だが時間切れ・次命令列のみ記録）。

## 5. 帯遵守の自己宣言（v323 §6 方式）

- **深解析（自帯 0x452000-0x467920・①②③④⑦指定）**: 0x45F2E0 全文・0x45F790 表・0x45EF00/0x45F0F0/0x4530A0/0x4531B0/0x45A500/0x45A530・0x45C9C0 窓/0x45DBC0 窓・0x462280 窓・0x465130 prologue・**0x465860 LoadGame 整列リニア（head から 0x465CC0 まで・G4/H4）**・0x46110A/0x46409E/0x46437F 直前文脈のみ。
- **機械走査（全域・入出力のみ）**: callers()/find_dword()/find_pattern()（E8=0 复唱・0x4CD960 ptr 全数・GST call 166 sites）・RTTI COL/TD/cstr 読取・vtable スロット読取（0xA3636C/0xA39094 他）。
- **最小逸脱（帯外・宣言）**: 0x430050/0x430070 thunk 全文（6-7命令・v307 先例再確認）・0x4309B0-0x4309DA ctor 書込窓（v307_11 引用＋境界確認）・**0x444340..0x4445C5 命令窓（タスク指示どおり命令窓のみ、callee 非展開）**・0x4CD960 head 8命令＋境界バイト＋ptr 走査（#106 浅層留守）・0x579CF0 境界バイト（is_head 確認のみ）。
- **未踏**: 0x4C0000-0x4E0000 の上記例外以外（TESObjectCELL ctor 0x4D3260/0x4D32EF 内部未読）・0x57xxxx 帯 callee 内部・[0xB33398] オブジェクト内部。docs/TODO/STATE/MCP 台帳は本 subagent で無編集（成果物ファイルのみ新規作成）。

## 6. 自己確認

- 本文件: `tmp/re/v340_v329_next_bundle_P2_ruling.md`（新規作成・既存ファイル上書きなし）
- probe out 8 本（A/B/C/D/E/F/G/H）は `%TEMP%\opencode\` に保持。`.py` は同梱（再実行可・read-only）
- 判定は命令列 VA 引用と 1:1 対応（各セルの probe 欄参照）。既存裁定（v248/v274/v307/v309/v313/v316/v328/v329）は出典明記の上で再導出を省いた
