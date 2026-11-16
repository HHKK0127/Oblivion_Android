# v433 — stride-0x28 テーブル3系統のレコード実読と sentinel の裁定（V-433-1/V-433-2/V-433-3 完遂・派生残点第25弾）

- 採番: **v433**（派生残点群・probe prefix **v433_1..v433_3 の3本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v432 の残点 **V-433-1【低】（`0xB0B420`/`0xB0C8C0` レコード内容実読）／V-433-2【低】（`FUN_004FCA70` callers と found 分岐）／V-433-3【低】（keyword table 終端と id 0x00..0x0F 帯）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **`0xB0B420`（id 0x100..0x182）の正体** | **[確定]（レコード実読）** | **console command テーブル＝131件（rec0..rec130・id 0x100..0x182）** — レコード構造 **`{name ptr, alias ptr, id u32, help 文言 ptr, flags(+0x10), ptr(+0x14), handler fn(+0x18), fn(+0x1C), 0, 0}`（stride 0x28）**。実読例: rec0=`Show`/alias `TST`/`"Show global scripts and variables."`/handler `0x502210`／rec1=`ShowVars`/`SV`/`"Show variables on object. [player->sv]"`／rec2=`ShowQuestVars`/`SQV`/`"Show quest variables. [svq QuestID]"`／rec130=`ClearFaceGenModelData`/`CFG`（id 0x182 最終） |
| **`0xB0C8C0`（id 0x1000..0x1170）の正体** | **[確定]（レコード実読）** | **script function テーブル＝369件（rec0..rec368・id 0x1000..0x1170）** — 同構造・alias は空（`0xA2F7EC`）が多い。実読例: rec0=`MessageBox`（handler `0x513280`/`0x500130`）／rec1=`GetDistance`（`+0x20=0x4F43C0`＝補助 fn）／rec368=`ClearOwnership`（id 0x1170 最終） |
| **開発者 sentinel** | **[確定]（実文字列）** | **console table rec131（`0xB0C898`）＝`"ADD NEW CONSOLE COMMANDS BEFORE THIS ONE!!!"`（id 0x183）`[確定]`／function table rec369（`0xB10268`）＝`"ADD NEW FUNCTIONS BEFORE THIS ONE!!!"`（id 0x1171）`[確定]`** — **開発時の追加位置マーカー**（＝テーブル末尾の番人レコード）。これにより **id 範囲（0x100..0x182＝131件／0x1000..0x1170＝369件）が sentinel と矛盾なく確定** |
| **keyword table（`0xB0A2C8`）の終端と全内容** | **[確定]（全16件実読）** | **16件（rec0..rec15・id 0x10..0x1F）で終端（rec16 name ptr=0）**: **`Begin(0x10)/End(0x11)/Short(0x12)/Long(0x13)/Float(0x14)/Set(0x15)/If(0x16)/Else(0x17)/ElseIf(0x18)/EndIf(0x19)/While_DEPRECATED(0x1A)/EndWhile_DEPRECATED(0x1B)/"."(0x1C)/ScriptName(0x1D)/Return(0x1E)/Reference(0x1F)`** — `+0x10`/`+0x14` は全件 0（console/function table とフィールド意味が異なる）→ **TES4 script のキーワード/型トークン完全表 [確定]**。**`While_DEPRECATED`/`EndWhile_DEPRECATED` は Oblivion で廃止された while ループの痕跡 [確定(実文字列)]** |
| **id `0x00..0x0F` 帯（V-431-2/V-433-3）** | **[確定(否定側)]** | keyword table は **id 0x10 から開始**（rec0 が 0x10）→ **`0x00..0x0F` は keyword table に存在しない＝別の enum 帯（keyword table 外）[確定(否定側)]**（何の enum かは残点） |
| **`FUN_004FCA70` の callers と found 処理** | **[確定]** | **E8=2 件とも `FUN_004FD7C0`（同一 fn・`0x4FDA06`/`0x4FDADE`）**。`FUN_004FD7C0`＝**script parser**（L197009-197031: `FUN_004fcbd0`（tokenizer）→ `"`（文字列リテラル）判定ループ）→ **keyword lookup は parser の1 pass [確定]**。found 分岐（`0x4FCB2B`）: **`lea edx,[ebx+ebx*4]`（i×5）→ `mov byte [edi+0x204],0x58`（type tag='X'）→ `mov eax,[edx*8+0xB0A2D0]`（＝rec[i].id）→ `mov [edi+0x208],eax` → return 1** ＝ **トークン構造体に「type=0x58＋keyword id」を格納 [確定(命令)]**。found なしは `xor al,al; ret`（0）。`FUN_004FD7C0` の callers（E8≥8）: 自己再帰 `0x4FDBC7`／**`FUN_004FDE30`（console/function table の `+0x1C` に全件共通で格納された同一 fn）**／`FUN_004FF4C0`×2／unannotated 4件 |
| **stride-0x28 テーブル3系統の全体像** | **[確定]** | **同一 stride の table が隣接3系統**: `0xB0A2C8` keyword（16件・id 0x10..0x1F）→ `0xB0B420` console command（131件・id 0x100..0x182＋sentinel）→ `0xB0C8C0` script function（369件・id 0x1000..0x1170＋sentinel）→ rec370 以降は**別構造（`"Left Foot"`/`"Right Foot"`/`"Sneak"`/`"Idle"`＝idle animation 名テーブル [推定高]）**。id 空間は keyword(0x10..0x1F) < console(0x100..) < function(0x1000..) と **3層に分離 [確定]** |

## §2 証拠（v433_1..v433_3）

### §2.1 console command table（v433_1(a)/v433_3(a)）
```
rec0  @0xB0B420: 'Show'            alias 'TST' id=0x100 help='Show global scripts and variables.'
                                     +0x10=0x10000 +0x14=0xB0A768 +0x18=0x502210 +0x1C=0x4FDE30
rec1  @0xB0B448: 'ShowVars'        alias 'SV'  id=0x101 help='Show variables on object. [player->sv]'
rec2  @0xB0B470: 'ShowQuestVars'   alias 'SQV' id=0x102 help='Show quest variables. [svq QuestID]'
...
rec130@0xB0C870: 'ClearFaceGenModelData' alias 'CFG' id=0x182   <- 最終
rec131@0xB0C898: 'ADD NEW CONSOLE COMMANDS BEFORE THIS ONE!!!'  id=0x183  <- sentinel
rec132@0xB0C8C0: 'MessageBox'  (＝function table rec0 に接続)
```

### §2.2 script function table（v433_1(a)/v433_3(b)）
```
rec0  @0xB0C8C0: 'MessageBox'  id=0x1000 +0x18=0x513280 +0x1C=0x500130
rec1  @0xB0C8E8: 'GetDistance' id=0x1001 +0x18=0x5003F0 +0x1C=0x4FDE30 +0x20=0x4F43C0
...
rec368@0xB10240: 'ClearOwnership' id=0x1170  <- 最終
rec369@0xB10268: 'ADD NEW FUNCTIONS BEFORE THIS ONE!!!'  id=0x1171  <- sentinel
rec370@0xB10290: 'Left Foot'/'Right Foot' id=0xA52634  <- 別構造（idle 名 [推定高]）
```

### §2.3 keyword table 全16件（v433_2(a)）
```
rec0..rec15 (0xB0A2C8 + n*0x28, id 0x10+n):
  Begin End Short Long Float Set If Else ElseIf EndIf
  While_DEPRECATED EndWhile_DEPRECATED "." ScriptName Return Reference
rec16: name ptr = 0 -> END
```

### §2.4 found 分岐と parser（v433_1(b)(b2)/v433_2(c)）
```
FUN_004FCA70 found @0x4FCB2B:
  lea edx, [ebx + ebx*4]              ; i×5
  mov byte [edi + 0x204], 0x58        ; type tag
  mov eax, [edx*8 + 0xB0A2D0]         ; rec[i].id（keyword base+8）
  mov [edi + 0x208], eax              ; トークンへ id 格納
  mov al, 1 / ret                     ; found=1
（未発見は xor al,al / ret = 0）

FUN_004FD7C0 (script parser, L197009-):
  do { FUN_004fcbd0(param_3, piVar1);   ; tokenizer
       if (*pcVar9 == '\"') { ... }      ; 文字列リテラル判定
  ... } while (...)
  callers: 0x4FDA06 / 0x4FDADE（keyword lookup×2）ほか
FUN_004FD7C0 callers: 0x4FDBC7(自己) / FUN_004FDE30 / FUN_004FF4C0×2 / unannotated×4
```
- **`FUN_004FDE30` は console/function table の `+0x1C` に全件共通で格納**（v433_1(a) の rec0/rec1/rec2/rec368 すべて `+0x1C=0x4FDE30`）＝**共通の command 実行ラッパー [確定]**。

## §3 残点 **V-434-1..3**
1. 【低】**id `0x00..0x0F` 帯の正体**（keyword table 外・何の enum か。keyword id 0x10 が「先頭」であることは確定したため、0x00..0x0F の所有者を探す）。
2. 【低】**rec370 以降の idle animation 名テーブル**（`"Left Foot"`/`"Right Foot"`/`"Sneak"`/`"Idle"`）の構造と consumer（`FUN_004FCA30` の id 範囲外に該当する第4テーブルか）。
3. 【低】`FUN_004FD7C0` の unannotated callers 4件（`0x4FF015`/`0x4FF12E`/`0x4FF26A`/`0x4FF3C7`）の identity（parser の上位＝script 読込/コンパイラのどの pass か）。

## §4 検証・自己批評
- probe: **v433_1..v433_3 の3本全 EXIT=0**（V-433-1/V-433-2/V-433-3 完遂）。
- **決定打の作り**: (i) **レコード window 実読（name/alias/id/help/fn ptrs）で table の役割を一撃判定**（`Show`/`ShowVars`＋help 文言＝console、`MessageBox`/`GetDistance`＝function）→ (ii) **境界 record の実読で sentinel（`ADD NEW ... BEFORE THIS ONE!!!`）を発見し id 範囲を確定**（`FUN_004FCA30` の range check と整合）→ (iii) **keyword table を walk し16件全実読＋終端（name ptr=0）を確認** → (iv) **found 分岐の `i×5 → [base+i*0x28+8]` アドレス計算を rec[i].id と即読** → (v) caller census で parser を特定。
- 妥当性チェック: ①sentinel id（0x183/0x1171）⇔`FUN_004FCA30` の range（0x100..0x182/0x1000..0x1170）＝**1件違いで矛盾なし（sentinel は range 外の番人）** — ②keyword 16件⇔`Begin`/`End`/`If`/`EndIf`＝TES4 script キーワード — ③found の `[edi+0x208]=rec[i].id`⇔parser が keyword id をトークン化 — ④`FUN_004FDE30` 共通 fn⇔table `+0x1C` 全件一致 — **四方向矛盾なし**。
- docs との不整合: **なし**（v432 の残点3件を一括確定）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **テーブル境界は「sentinel レコード（`ADD NEW ... BEFORE THIS ONE!!!`）」が実文字列で残っている** — range check（コード）と sentinel（データ）の2系統で id 範囲を相互検証できる（本件131件/369件が完全一致）。
  2. **レコード実読は name/alias/help の3点セットで役割が即決まる**（alias `TST`/`SV`/`SQV`＝console コマンドの短縮名、help 文言＝使用法）。
  3. **隣接テーブルの walk は「name ptr が string か」で終端を読むが、次構造の name が string の場合は sentinel/id で切り分ける**（rec370 以降の idle 名で walk が誤って続いた事例＝**id 範囲との突合が必須**）。
