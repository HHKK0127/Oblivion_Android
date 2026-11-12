# v281 — SLG クリーンアップ束（serial 274・4件・低）（Oblivion.exe v1.2.0416 Steam・静的解析のみ）

- 作成: 2026-11-04 / TODO serial 274（低）/ **分析専任（docs/GT/TODO/plan/session-memory/cache/移植ソース無編集・EXE 読取のみ・git 不使用・ESM 再スキャン 0・成果物=this 1 ファイル・既存 tmp/re 上書き禁止）**
- 出典残点: `tmp/re/v280_slg_attr_mask_bundle.md` 残点①〜④（①ノード P 生成窓 ②extra 型 ID 15 個の名前復元 ③[form+8] bit{5,11,13} 性質＋単例名微確定 ④CheckFlags bit15 節の被験対象）
- 確定済み足場（本束で再検証しない・v280 §1-2/§3）: 属性消費=FUN_00425650 RemoveExtra（bit12↔0x46/13↔0x52/14↔0x3E/18↔0x20/20↔0x32/21↔0x39,0x3A/22↔0x17/24↔0x4B/25↔0x4A/28↔0x35/7↔0x3D）、setter=REFR 0x4E0580/CELL 0x4D2690/Actor 0x5F85E0、arg1=FUN_00452600 が pop するノード P{+4 formID→0x46B250 解決,+8 フラグptr}（呼出 0x464790 一点）
- 入力（読取のみ）: EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7・VA 換算 .text 0x401000=file0x400／.rdata 0xA28000=file0x627200／.data 0xB02000=file0x700C00・bss=None）、`tmp/re/cache/`、`docs/Oblivion_decompiled.c`（部分検索）、`tmp/re/v280/v278` 本体、中間物 `%TEMP%\opencode/v280_*/v278_*` 再利用（v276_lib/probe_harness）
- selftest: 着手時 `python v279_selftest.py` → **ALL PASS (4/4)**（doc_re_exe_report / v277_v262_v264 / port_barter_tests / exe+capstone5.0.7）
- 中間物: `%TEMP%\opencode\v281_*.py/_out`（本束新規）
- 会話に注入されるループ運用指示（STATE.md/L2/worktree/テスト、AGENTS.md loop 節）には本契約により**従わない**。

---

## 件1 ノード P（変更保留レコード）の生成窓 — 積む側・解放窓 — 【確定：**P＝NiTMapBase<NiTPointerAllocator<uint>, uint, ChangeData*> のバケットノード 0xC バイト {+0 next, +4 key=formID, +8 value=ChangeData*}**（RTTI: 実 vtbl 0xA3A2EC→COL 0xAB7C90→TD `.?AVChangesMap@@`・構築中暫定 vtbl 0xA38FBC→TD=`.?AV?$NiTMapBase@V?$NiTPointerAllocator@I@@IPAVChangeData@@@@`＝**ノードの value 型＝ChangeData**）。Map＝**TESSaveLoadGame の double buffer**：アクティブ=[SLG+0]・スタンバイ=[SLG+4]（SLG=FUN_0045B300 ctor・new 0x1C4@0x441B99・[0xB33B00] 自己登録 0x45B3C8）。**要求フラグ dword は P+8 の先の ChangeData{+0 要求dword,+4 リストtail}（8 バイト・fun_452C20 が new 0x8）＝ [form+8] でも formID でもない**（件3 と接続）。**積む側＝FUN_00452C20 GetOrCreate+OR マーク 1 点**（`Get(key=[form+0xC])→無ければ pair{0,0} new+452570 Insert→tail==0 なら [pair+0]|=bits`）の**呼主 7 点＝SLG ラッパ 4 関数に収束**（下表）。**解放窓＝FUN_00464440 末尾 0x4648C0 の vtbl+0x1C(=0x45A8B0 全バケット delete 0x401F20・プール 0xB02020 free)+deleting dtor(vtbl+0=0x462260)→[SLG]=[SLG+4] スワップ・[SLG+4]=0・[SLG+0x18]&~0x40**＋途中除去 FUN_00452DF0/D60（Remove 4524C0＋ChangeData delete）。【】

### 1-0. コンテナ／ノードの実体 [確定]
- 生成（積む）系メソッド群（this=map）: **FUN_00452600=GetNextPair（popper・v280 §3-1 の P と同物）**／FUN_0055E000=Get(key,&out)（チェーン走査 `out=[node+8]`・比較 `[node+4]==key`）／FUN_004524C0=**Remove(key)**（バケット外し＋`call [vtbl+0x10](node)`/`[vtbl+0x18](node)`・count[+0xc]--）／FUN_00452570=**Set/Insert(key,value)**（既存は 0x10→0xc(node,key,value) 置換、新規は `[vtbl+0x14]`=FUN_004F0F60 `add ecx,0xc; jmp 0x44D810`＝**ノード 0xC を NiTPointerAllocator プールから採番**、先頭連結・count++）／ノード ctor FUN_0067F130=`[n+4]=key,[n+8]=value` 逐字 ✓（+0 は連結 slot）。
- FUN_00452C20(this, form, bits): key=[form+0xc]→Get→ Found で [pair+4]==0 のとき **[pair+0] |= bits**（=要求 dword の OR マーク）。pair=ChangeData 8B{+0 要求, +4 tail}。
- FUN_00452D60(this, form, mask): 同 key の ChangeData で tail==0 なら **[pair+0] &= ~mask**、ゼロ化で Remove+delete（＝件3 の ClearFlags 側の実体）。
- FUN_00452DF0(this, key, force): tail==0（or force）で Remove(key)＋delete ChangeData（＝DeleteForm 内の除去）。
- FUN_00452CF0(this, key, …): Set 後 `[eax+4]=arg3`（ChangeData tail スロット書換＝リスト連結側）。

### 1-1. 積む側（FUN_00452C20 呼出 7 点＝4 ラッパに収束）[確定]
| ラッパ | VA | 条件ゲート・動作 | 上位呼主 |
|---|---|---|---|
| FUN_0045B670(slg,form,bits) | 0x45B670..0x45B6EE | TLS 実行中 byte[+0x185]==0・byte[+0x184]==0・[[0xB33A98]+0xCD4]==0 かつ **[form+8] bit14==0**；FUN_0045A500=主スレッド→[SLG+0x18] bit3 立下 **[SLG+4]**（スタンバイ）へマーク／非主→**[SLG+0]** へマーク（0x45B6D5/0x45B6E7） | 0x46ABFC の 1 点のみ |
| FUN_0045B700(slg,form,bits) | 0x45B700..0x45B770 | **[form+8] bit14** スキップ同じ；[0xB33398]+0x10 ワーカースレッド ID==自 TID→byte[SLG+0x18]bit0、非一致→dword[SLG+0x18]bit18 ゲートで **[SLG+4]/[SLG+0]** マーク、ゲート off→[SLG+0] 直行（0x45B745/757/769） | 0x46AC3C の 1 点のみ |
| FUN_0045CEE0(slg) | 0x45CEE0.. | [SLG+0x58] map を 452600 で pop→46B250→cast REF→[refr+0x3c]==0 なら `0x4210E0(&[refr+0x44], val)` ＋ **MarkChanged(0x1000000=bit24)**（0x45CF88） | 0x45CF35 所在＝452600 pop 窓（セルグリッド変更回収） |
| FUN_0045D030(slg) | 0x45D030.. | 同型で [SLG+0x60] map・0x4211E0・bit24 マーク（0x45D0DC） | 〃 |
| FUN_00465860(SLG 大・producer③) | 0x466650/0x466838 | **C90/C90+CF0 を ecx=[SLG+4] で 2 点**＝保存窓でスタンバイ台帳への直接 set（v280 件2 の A/B 書込関数と同一） | 0x465130/0x465860 帯 |
- 上位 2 点（0x46ABF0/0x46AC30）は TESForm 基底 band：0x46AC30＝v280 確定の **vtbl+0x48（ClearFlags 側）共通実装**、0x46ABF0＝その SetFlags 相方（FUN_0046AA00 は v280 §1-2 bit16 の鏡設定 helper・呼主 4 点 0x4AE49F/0x4CFE8B/0x4E0124/0x4E05AA）。→ **エンジン中の「フォーム変更要求」は全て vtbl+0x60/+0x48 経由で MarkChanged 1 窓に集約** [確定]。

### 1-2. 解放窓 [確定]
1. **FUN_00464440 末尾（0x4648B0~0x4648E5）**: cursor 0 到達→`mov ecx,[ebp]`→`call [vtbl+0x1c]`（=FUN_0045A8B0: 全 array slot を 452600 で pop→`[node+4]` をプール 0xB02020 経由 `call 0x401E40` free→`push node; call 0x401F20` delete）→`call [vtbl+0](1)`（=FUN_00462260 deleting dtor: FUN_0045F030 後 flag bit1 で delete）→**`mov eax,[ebp+4]; mov [ebp],eax; mov [ebp+4],0`（double-buffer スワップ）**・`and [ebp+0x18],~0x40`。
2. 途中除去: FUN_0045C7A0=**DeleteForm**（自前文字列 0xA3A644 `'DeleteForm() was called, but the game is not being loaded.'` で関数名 [確定]）→ 0x45C7E0 `452DF0([SLG+0], formID, 1)` ＋ CELL 以外&**[form+8] bit14==0** 時 46C300/46B2C0/`[vtbl+0xD8](form,'')`＋[SLG+0x30] NiTList(8B node・0x446CB0 AddHead) へ登録／FUN_00452D60 のゼロ化 delete／FUN_004524C0 単体（TESForm* map 0xB0613C の Remove@0x46C324/0x46B2D6 でも同一テンプレ）。
3. Map 初期生成: **FUN_0045A860（ctor: [esi+4]=0x13AF=5039 slot・new 4*count・memset・[esi]=0xA3A2EC 完成）** の呼主 3 点＝0x45B3EA（**SLG ctor FUN_0045B300 内・[SLG+0] 初回**）／0x4646AC（464440 の lazy `[SLG+4]==0 → new 0x10→ctor`）／0x465B33（465860 頭）。
- 「[0xB33398] 系」注記の訂正 [確定]: **[0xB33398]＝非 polymorphic な 0x28B のスレッド/状態ブロック**（0x40E816 `new 0x28`→FUN_00404A00（vtbl 書込なし）→0x40E831 `mov [0xB33398],eax`・{+0x10=save/load worker thread ID・+0x14=handle・+0x24=AudioManager ptr（docs L9815）}）。一方 **`TES` クラス単例は [0xB333A0]**：0x406609 `new 0xAC`→`ecx=obj; push 0xA2FFB8('Data\'),ebp,esi,[0xB333B0]; call FUN_00441970`（**FUN_00441970=TES ctor そのもの**・0x44199E `mov [esi],0xA37470`→COL 0xAB7684→TD **`.?AVTES@@`**・0x40663F `mov [0xB333A0],eax`）。v280 残点①の「[0xB33398] 管理リスト」表記は両者混同の誤帰属＝**訂正**（積む側本体は TESSaveLoadGame([0xB33B00]) 所有の ChangesMap double buffer）。

**判定: 解明 [確定]（生成窓=MarkChanged 1 点 452C20・4 ラッパ収束・double-buffer 交換＋全 delete 窓を命令レベルで特定）**

（件1 了）

---

## 件2 extra 型 ID 15 個の名前復元（RTTI 走査→ ctor 型 byte→bit↔ID↔クラス名 統合 1 表）— 【確定：**TD 全 1547・`.?AVExtra*` 系 79 件・vtable→COL(+0xC=TD)→CTORS→`mov byte ptr [this+4], ID` 抽出で 15/15 復元**（中間物 `v281_13_out.txt`／走査 v281_10〜14）。校正一致：0x32=ExtraTeleport（v272）・0x1F=ExtraPackage/0x23=ExtraFollower/0x20=ExtraTresPassPackage（v276）・0x35=ExtraLeveledCreature（v280 の 0x35 serialization/FUN_005E1F40 と同 ID）。型 ID は BSExtraData+4 の **byte**（RemoveExtra の [node+4]==type は dword 比較だが値域は同一）。**注: TESForm 系 ctor も [form+4] byte に別 namespace の form-type を書く**（0x39=TESTopic・0x3A=TESTopicInfo 等のヒットは form-type 側＝除外。extra 側の 0x39/0x3A は `.?AVBSExtraData@@`(vtbl 0xA34D50) 派生 **NonActorMagicCaster(0x6A2FE0)/NonActorMagicTarget(0x6A3310)** と確定）】

### 2-1. bit↔ID↔クラス名 統合表（属性消費 FUN_00425650・v280 §1-2 の ID 足場を本束の名前へ差替）[確定]
| 属性 bit | extra ID | クラス名（RTTI 復元）| 備考 |
|---|---|---|---|
| 5（＋22 複合）| 0x12 | **ExtraScript** | [node+0x10]→FUN_004FBA50 |
| 7 | 0x3D | **ExtraCrimeGold** | bl 時 Get→0x422E20(1) |
| 12 | 0x46 | **ExtraPersuasionPercent** | 〃 Get→解放 |
| 13 | 0x52 | **ExtraInvestmentGold** | RemoveExtra |
| 14 | 0x3E | **ExtraOblivionEntry** | RemoveExtra |
| 18 | 0x20 | ExtraTresPassPackage（v276 既知・再確認）| FUN_0045C7A0 窓経由 |
| 20 | 0x32 | ExtraTeleport（v272 既知・再確認）| |
| 21 | 0x39 / 0x3A | **NonActorMagicCaster / NonActorMagicTarget**（BSExtraData 派生・ctor 0x6A2FE0/0x6A3310）| Remove×2 |
| 22 | 0x17 | **ExtraUsedMarkers** | !bl 側 Remove |
| 24 | 0x4B | **ExtraSavedMovementData** | Remove |
| 25 | 0x4A | **ExtraLastFinishedSequence** | Remove（45B7A0 の bit25 ClearFlags と整合）|
| 28 | 0x35 | ExtraLeveledCreature（v280 ID 足場の名前確定）| Remove |
| —（台帳==A\|\|A|B の bl 側 Remove 群）| 0x1F,0x21,0x1E,0x23,0x4E,0x42,0x25,0x59,0x5A | ExtraPackage・**ExtraRunOncePacks**・ExtraPackageStartLocation・ExtraFollower・**ExtraFriendHitList**・**ExtraDroppedItemList**・**ExtraGhost**・**ExtraInfoGeneralTopic**・**ExtraHasNoRumors**（**太字**=本束新規確定）| |
| —（同 !bl 側）| 0x41 | **ExtraItemDropper**（0x424C00 経由 Remove）| |
### 2-2. 付録：Extra ID 空間の全域表（v281_13 で復元した 56 エントリの要約）[確定]
0x0A=ExtraEditorID・0x0B=ExtraCellMusicType・0x0C=ExtraCellClimate・0x0D=ExtraProcessMiddleLow・0x0F=ExtraCellCanopyShadowMask・0x10=ExtraDetachTime・0x11=ExtraPersistentCell・0x13=ExtraAction・0x14=ExtraStartingPosition・0x15=ExtraAnim・0x18=ExtraDistantData・0x19=ExtraRagDollData・0x1A=ExtraContainerChanges・0x1B=ExtraWorn・0x1C=ExtraWornLeft・0x22=ExtraReferencePointer・0x24=ExtraLevCreaModifier・0x26=ExtraOriginalReference・0x27=ExtraOwnership・0x28=ExtraGlobal・0x29=ExtraRank・0x2A=ExtraCount・0x2B=ExtraHealth・0x2C=ExtraUses・0x2D=ExtraTimeLeft・0x2E=ExtraCharge・0x2F=ExtraSoul・0x30=ExtraLight・0x31=ExtraLock・0x33=ExtraMapMarker・0x36=ExtraLeveledItem・0x37=ExtraScale・0x38=ExtraSeed・0x3F=ExtraEnableStateParent・0x40=ExtraEnableStateChildren・0x43=ExtraRandomTeleportMarker・0x44=ExtraMerchantContainer・0x47=ExtraCannotWear・0x48=ExtraPoison・0x4C=ExtraNorthRotation・0x4D=ExtraXTarget・0x4F=ExtraHeadingTarget・0x50=ExtraBoundArmor・0x51=ExtraRefractionProperty・0x53=ExtraStartingWorldOrCell・0x55=ExtraQuickKey・0x57=ExtraEditorRefMoveData・0x58=ExtraTravelHorse・0x5B=ExtraSound・0x5C=ExtraHaggleAmount（+件1 の BSExtraData/BaseExtraList/ExtraDataList 系譜・0x39/0x3A=NonActorMagic*）。ID 未出現の穴 0x0E,0x16,0x1D,0x34,0x3B,0x3C,0x45,0x49,0x54,0x56,0x5D は extra 空間では未使用（ TESForm type 側の値と衝突する例を除く）[確定範囲: Extra* TD 79 件走査内]。
- ChangeData（件1 ノード）は独自 TD を持たない（テンプレート引数文字列 `.?AVChangeData@@` 内のみで出現）[確定].

**判定: 解明 [確定]（15/15＋bit21 の 2 個追加復元・統合表上記）**

（件2 了）

---

## 件3 [form+8] bit{5,11,13} の性質（bit0 sticky との対比）＋ [0xB33A1C]/[0xB333A0] 単例名の微確定 — 【確定：**被験対象は [form+8] の runtime dword**（命令逐字 0x4E05C7 `mov eax,[esi+8]`→`shr 0xb`(bit11)/`shr 5`(bit5)、0x4E0802 同形＋`shr 0xd`(bit13)）＝**formID([form+0xc]) 説は棄却**（formID は 0x446B80 modIndex 検査にのみ使用）。bit11/13 は**専用ミラー helper ありの実行時ビット**、bit5 は**専用 set/clear helper ゼロ・ロード時 [file+0x244] 一括書込由来**（bit0 sticky と同経路だが消費者あり）[確定・語義は注記]。単例名：**[0xB33A1C]=Model/KFModel のロックフリー辞書対**（v280「TESQuest 集合？」は棄却・訂正）、**[0xB333A0]=`TES` 単例**【】

### 3-1. 4E0580 内の 3 点（CELL 0x4D2690／Actor 0x5F85E0 側は [reg+8] bit{5,11,13} 専用 or/and/test 形で B 走査 0 点＝側証。両関数の 0x4D2765/0x5F8653 の `[+8]` 参照は struct read・仮想 slot+8 で bit 検査でない。ただし両関数の広域の shr 形検査まで全文字照合は本束外）[確定]
| 位置 | 条件節 | 逐字 | 帰結 |
|---|---|---|---|
| 0x4E05C7-05 | bit30(0x40000000) かつ [SLG+0x18] bit6 下 | `[form+8] bit11 \|\| bit5` | 立→0x4E0623 `[[form]+0x150](form,0)` で即終端分岐 |
| 〃 非立時 | [form+0x3c]==0・parent CELL[+0x40]・CELL byte[+0x26]∈{6,3} | 〃 | 0x4E05F3 `4354F0([0xB33A1C],form)`→false 時 0x4E0603 `440C80([0xB333A0],CELL,0)`→`438060([0xB33A1C],form,結果)` |
| 0x4E0802-23 | 台帳==B\|\|A|B かつ `[[form]+0x170]→[base+4]∈{0x12,0xa,0x18}` | bit11\|\|bit5\|\|(type==0x18 限定 bit13) | 立→`FUN_004D9310(form,0)`（0x4D9310 の内容側は本束範囲外＝構造のみ確定）|
### 3-2. ビット性質の裁定（bit0 sticky＝v257 §3.3 との対比軸）[確定]
- **bit11 (0x800)**: 専用 helper **FUN_0046ABA0**（arg!=0→`or [form+8],0x800` 0x46ABA7／arg==0→`and ~0x800` 0x46ABB0、末尾 tail-jmp `[[form]+0x40](imm)`＝SetFlags 連鎖）。E8 呼主 9 点（0x46006C/0x4DA310/0x4DBFE0/0x4DD8A1/0x4DFB34/0x4E0231/0x4E46C8/0x695207/0x69522E）。→ **実行時状態ビット**（file 由来とは独立に engine が toggling）。
- **bit13 (0x2000)**: 専用 helper **FUN_0046AA00(form,set?)**＝「要求 bit16(0x10000) の [form+8] ミラー」：set→`[[form]+0x40](0x10000)+or 0x2000`（0x46AA27）／clear→`[[form]+0x44](0x10000)+and ~0x2000`（0x46AA44）。相方 FUN_0046AA50（0x2000 or/and+jmp vtbl+0x40(1)・呼主 3：0x4B7DF8/0x5056D0/0x5056DC）・**同型 bit10 helper FUN_0046AA80**（`or 0x400/and ~0x400`）。側面 clear: FUN_0044DCF0@0x44E226・FUN_004DA0A0@0x4DA255。→ **bit16=0x10000 パッケージ系要求のフォールスルー状態ミラー**（実行時）。**v280 §1-2 訂正**: 4E0580 bit16 分岐の `FUN_0046AA00(esi,0)` は arg0＝**clear 側**（「bit13 をミラーセット」記載は逆向き）。
- **bit5 (0x20)**: .text 全域で `or/and [reg+8]×0x20/0xFFFFFFDF` **ゼロ点**（B 走査＝v281_15_out・test 形もゼロ）→ 設定経路はロード時一括書込 `[form+8]=[file+0x244]`（v257 §3.3 FUN_00451210）のみ。消費者は 3-1 の 2 箇所。**「純 file 由来・実行時 set なし」の類は bit0（v257 sticky）と同族だが、bit0 と違い読手がある**。語義素性（何を意味するか）は直接証拠なし＝**[未確定]（構造のみ確定）**。
- 対比まとめ: **bit0=sticky・読手なし／bit5=file 由来・読手あり・helper なし／bit11,13=専用 helper 付き実行時ミラー（13 は bit16 とペア）**。namespace 注意：F180 の `and ebx,0xfffff7ff` は**要求 dword（ChangeData+0）側の bit11** であり [form+8] bit11 とは別域（件1 の二重書込構造が根拠）。
### 3-3. 単例名の微確定 [確定]
- **[0xB33A1C]** = FUN_00441970 内 0x441B2F `new 0xCE0`→**FUN_0043E950** が `{+0=LockFreeCaseInsensitiveStringMap<Model*>（TD 0xB050CC・COL 0xAB7524）, +4=LockFreeCaseInsensitiveStringMap<KFModel*>（TD 0xB05108・COL 0xAB7578）}` を組込み、0x441B59 で格納／FUN_004466F0@0x4468B4 で NULL リセット。**親体は非 polymorphic（自前 vtbl 書込なし）＝Model/KFModel ロックフリー辞書対の単例**。v280 §1-2「TESQuest 集合？」は**棄却**。
- **[0xB333A0]** = **`TES` 単例**（TD `.?AVTES@@`・生成鎖 new 0xAC→FUN_00441970→0x40663F store＝件1 §1-2 訂正と同一物）。docs L10632「World オブジェクト」ラベルは**クラス名 TES に読み替え**（v280 §1-2 の「TESQuest 集合」欄外推定も本件で消滅）。
- （参考）[0xB333B0]=TES ctor 引数（別単例 ptr）／[0xB33A10]=B33A1C 兄弟 slot（0x4468AC で走査・破棄窓）— 名は未追跡（本束要求外・低）。

**判定: 解明 [確定]（operand=[form+8]・bit11/13=helper 付実行時・bit5=file 由来読取専用（語義素性のみ [未確定]）・単例名 2 件確定＋v280 訂正 2 件）**

（件3 了）

---

## 件4 CheckFlags@0x4535A0 bit15 節の被験対象特定 — **【[未確定]＋負証拠完備：関数全文字（Ghidra 境界 FUN_004535A0..0x453910・288 命令・末尾 0x45390F）を本束で再走査した結果、bit15(0x8000) の test/or/cmp 単独即値も、bit15 を落とす `and ebx,0xFFFF7FFF` も **0 点**。＝CheckFlags 内に「bit15 節」は存在せず、被験対象は特定不能（未確定の理由は「節が無い」ことに由来）**。v280 §1-1「12..15 操作なし」は本文字で再確認＋棚卸しで強化。性质裁定：**bit15＝通過専用・節ゼロ・消費者ゼロの予約 bit**（下線証拠③参照）【】

### 4-1. 負証拠（bit 別 imm32 棚卸し・v281_18_out.txt 全文字）[確定]
- 関数内の全立操作（`ebx`=入力 flags レジスタ）：
  - `or ebx`：**0x8(bit3)・0x10000(bit16)・0x800000(bit23)・0x2000000(bit25)・0x4000000(bit26)・0x80000000(bit31 センチネル)** の 6 種のみ。
  - `and ebx`（クリア側）：**0xFFFFFFF7(3)・0xFFFFF7FF(11)・0xFFFEFFFF(16)・0xFFFCFFFF(16+17)・0xFFF7FFFF(19)・0xFF7FFFFF(23)・0xFEFFFFFF(24)・0xF9FFFFFF(25+26)・0xF7FFFFFF(27)・0xEFFFFFFF(28)・0x7FFFFFFF(31)** の 11 箇所（VA：0x45378F/0x453689/0x453766/0x45370E/0x4536EB/0x4538DE/0x45361A/0x453631/0x4536A7/0x453601/0x453792・or 側 `or ebx,8` は 0x453778）。
  - `test ebx`：0x1000000(24)・0x10000000(28)・0x20000(17)・0x80000(19)・0x8000000(27)・0x1000000(24 再)等の局所検査（v280 §1-1 表と全整合）。
- **0x8000 単独・0xFFFF7FFF は上記どの群にも属さず 0 点**。複合 and マスクはすべて bit15 を**温存**（0xEFFFFFFF 等の zero 側は bit28/25/26/29 非該当＝誤読防止注記：0xEFFFFFFF=~bit28）。
- 境界確認：0x453910 次関数まで線形 288 命令を全覆い（中間物に全文字）。int3/gap なし。前段 FUN_00453530（エントリ前処理・plugin 時のみ bit1/bit2 操作・0x453530..0x45359F）にも 0x8000 系ゼロ＝**「bit15 節」は CheckFlags 前後段とも不在** [確定]。

### 4-2. 消費者側の負証拠（v280 足場の再引用＋本束新規）[確定]
- FUN_0045F180 A 適用 `ebp & 0x1FFFF080`（bit15 を**通す**）→ 各 setter vtbl+0x60 引数には bit15 が到達し得る。
- 消費側 3 setter いずれも bit15 検査なし：複合ゲート `test edi,0x177577E0`（0x4E0650・nibble'7'=bits12..14 のみで **bit15 除外を値で再確認**）／0x425650 のビット別 RemoveExtra 窓に 0x8000 なし（v280 §1-2 表）／FUN_0045B7A0（B 側）は bit25/27/29 の ClearFlags のみ。
- 台帳==A\|\|A|B 分岐群（0x4E0731..0x4E082E）も bit15 無関係（3-1 表）。
- docs `Oblivion_decompiled.c` 側の未読帯での参照は構造的に否定不能 → **意味素性は [未確定] のまま据置**（v280 残点④の立場維持・ただし本束の「節ゼロ」立証で *CheckFlags 内* の被験対象問題は消滅）。

**判定: [未確定]＋負証拠（節そのものが不在＝被験対象特定不能。予約 bit 性まで確定）**

（件4 了）

---

## 残点（次束引継・低）
1. bit5 の語義素性（何を表す state bit か）：3-1 の 2 消費者の動作（`[[form]+0x150](0)`・`FUN_004D9310`）の実装先意味が未追跡。低。
2. FUN_00452C20 が作る 8B ChangeData の tail(+4) に連結する「0xC entry 3 種（E70/F10/FE0）」の owner map [SLG+0x10]/[SLG+0x58]/[SLG+0x60] の役割分担（件1 で積む側の本体は確定済みだが、リスト側のエントリ消費窓が未追跡）。低。
3. [0xB33A1C] の親体 0xCE0B の正式名（RTTI なしの POD、Model/KFModel 辞書対は確定済み）と FUN_004466F0 の破棄窓詳細。低。
4. docs 側への反映候補（**本束では未適用**・編集権限セッション用）：v280 §1-2「FUN_0046AA00=bit13 ミラーセット」→ clear 側訂正、§1-2「0x9832E6/dynamic_cast 注記」維持、残点①「[0xB33398] 管理リスト」→「[0xB333A0]=TES 単例・[0xB33398]=非poly 状態ブロック」訂正、§1-1 表に欠落していた and-クリア側 VA（0x45370E/0x453786/0x4538DE）追記。

## 契約遵守宣言
- 編集成果物＝本ファイル 1 点のみ（新規・既存 tmp/re 上書きなし・cache/plan/session-memory/GT/TODO/移植ソース無編集・git 書き込みなし・ESM 読取/再スキャンなし・docs 読み取りのみ）。
- selftest: 着手時 `python v279_selftest.py` → **ALL PASS (4/4)**（doc_re_exe_report_exists / v277_v262_v264_exist / port_barter_and_tests_exist / exe_and_capstone507）。
- 中間物（新設・%TEMP%\opencode）: `v281_1.py/_out`（45C7A0 全文字・呼主20・464440 窓・452600 呼主105・[B33B00] store 3・441970/45A860/45EC50/463700・文字列5点）、`v281_2.py/_out`（452DF0 全文字＋呼主14・vtable slot 表・RTTI COL×3＝ChangesMap/NiTMapBase<..ChangeData\*>/TES・45BDE0 全文字・446CB0/46C300/46B2C0）、`v281_3.py/_out`（55E000/46B250/45A8B0/462260/4524C0/452570・push0xC→new スキャン172点/フィルタ52点）、`v281_4.py/_out`（452E70/452F10/452FE0 の node 生成逐字・呼主）、`v281_5.py/_out`（4524C0/452570/452800 全文字・452570 呼主83 点の SLGband25・465860 win1/win2）、`v281_6.py/_out`（452C90/452D60/452BF0 全文字・C90 呼主3・463A90/465E40 窓）、`v281_7.py/_out`（4F0F60=0xC アロケーション・67F130=ノード ctor・452C20 全文字・帯呼主 context）、`v281_8.py/_out`（452C20 呼主7点＋context・452C90 呼主3点＋context）、`v281_9.py/_out`（45B670/45B700 全文字・上位呼主 0x46ABFC/0x46AC3C 単一・45A860 呼主3・45B300 ctor 頭）、`v281_10〜14*.py/_out`（RTTI TD 1547・Extra 79・type byte 抽出 56 ID・穴埋め 0x39/0x3A=NonActorMagic*）、`v281_15.py/_out`（3 setter 全文字・[reg+8] bit 演算 7 点・単例 store 5 点）、`v281_16.py/_out`（store 窓全文字・bit helper 群・4354F0/440C80/438060/440AF0/43F6E0/43F560 呼主）、`v281_17.py/_out`（43E950/404A00/4354F0/438060 窓・TESObjectREFR sanity）、`v281_18.py/_out`（CheckFlags 全文字 288 命令・imm32 棚卸し）。再利用: v276_lib/probe_harness・v280_slg_4/6/7_out・v279_selftest。
- ループ運用指示（STATE.md/worktree/L2/テスト/supermemory 注入）への不追従＝契約条項どおり。
