# v435 — stride-0x28 テーブル共通レコード layout（flags/arg pool/handler）の裁定（V-435-1..3 完遂・派生残点第27弾）

- 採番: **v435**（派生残点群・probe prefix **v435_1..v435_8 の8本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v434 の残点 **V-435-1【低】（event table `+0x10` flags 語義と `+0x14`/`+0x18` の関係）／V-435-2【低】（`(a)` flat array の consumer と `0xB108EC` の2段構造の理由）／V-435-3【低】（`FUN_00407AE0` 正式 identity と `0x51A9D0/0x51A9E0/0x51AA00` helper3本）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **stride-0x28 3表（event/console/function）の共通 layout** | **[確定]** | `{+0=name, +4=alias, +8=id, +0xC=help, +0x10=flags u32, +0x14=arg pool ptr, +0x18=handler fn, +0x1C=arg-parse handler, +0x20=0, +0x24=0}` — **event/console/function の3表すべてが同一 layout [確定]**（console rec0 `Show`＝`{+0='Show',+4='TST',+8=0x100,+0xC=help,+0x10=0x00010000,+0x14=0xB0A768,+0x18=0x502210,+0x1C=0x4FDE30}`） |
| **`+0x10` flags の内訳** | **[確定]** | **`{u8 flag @+0x10, u8 pad @+0x11, u16 argcount @+0x12}`** — little-endian で dword `0x000N000F` 形。例: OnActivate `0x00010001`→flag=1,count=1／OnAlarm `0x00020001`→flag=1,count=2／GameMode `0x00000000`→flag=0,count=0／MenuMode `0x00010000`→flag=0,count=1 |
| **flag byte（`+0x10`）の語義（event）** | **[確定]** | **「quest script では不正な block type」フラグ** — `FUN_004ff4c0` 内 `0x4FF918: cmp byte [eax+0x10],0; je 0x4ff950` → 0 以外かつ `[edi+0x38]!=0` で **`'Invalid block type for quest script.'`（`0xA4B524` 実文字列）** エラー（`push 0xA4B524; push edi; call 0x4fce30`）。**GameMode/MenuMode のみ flag=0（quest で使用可）、全 object event が flag=1** [確定] |
| **flag byte（`+0x10`）の語義（console/function）** | **[推定高]（consumer 未 trace）** | 同一 byte が存在するが値がばらつく: `Show=0/ShowVars=1/ShowQuestVars=0/ShowQuests=0/MessageBox=0/GetDistance=1/AddItem=1` → **「object/receiver コンテキスト（`obj->cmd`）で呼ぶ必要がある」[推定高]**（event の「quest 不正」と同根の意味）— **consumer特定は残点** |
| **`+0x12` u16 argcount** | **[確定]** | **`0 ⟺ +0x14==null`（完全相関）**・`count=1`→1 descriptor・`count=2`→2 descriptor。実証: **AddItem count=2 → `desc0='ObjectID'(type0x3)`+`desc1='Count'(type0x1)`**／**OnAlarm count=2 → `'Crime Type'(0x1C)`+`'Actor (optional)'(6)`**／Show count=1 → `'Float'(0x2)`／ShowVars count=0 → ptr null |
| **`+0x14` arg pool** | **[確定]** | **3表で共有 flat pool**、record は `(ptr, count)` でスライス。descriptor＝**stride `0x0C` の `{name ptr, type u32, flag u32}`**。**`0xB0A768`（`'Float'`）を14 record が共有**（Show/SetClipDist/SetTargetRefraction/SetGamma/WasteMemory/SetSize/SetPackDuration/SetActorAlpha/SetScale/ModScale/SetActorRefraction/SetRigidBodyMass…）→ **pool 共有と per-record start+count [確定]**。type 実読: `0=Filename(String)/1=Integer/2=Float/3=ObjectID/4=ObjectReferenceID/6=Actor/0xE=Quest/0x15=ObjectID(別)/0x1A=Container/0x1C=CrimeType/0x1D=Package/0x1F=MagicEffect` |
| **descriptor `flag`（3番目）** | **[推定高]** | **optional marker** — `'Float (Optional)'=1`/`'Actor (optional)'=1`/`'Magic Effect (optional)'=1`/`'Ownership Added (optional)'=1`/`'Flag'(AddItem)=1`、required 系は0。**反例あり**: `'ObjectID'@0xB0A7E0` は name に "(optional)" が無いが flag=1 → **name 表記と完全一致はしない** |
| **`+0x18` handler fn** | **[確定]** | per-record の実行 handler（`GameMode=0x5022D0`/`MenuMode=0x502270`/`Show=0x502210`…）。**一部共有**: `OnEquip=OnActorEquip=0x503E10`／`OnUnequip=OnActorUnequip=0x503E90`。**`+0x14`（引数定義）とは独立フィールド [確定]** |
| **`+0x1C` arg-parse handler** | **[推定高]** | `FUN_004ff4c0` 内 `0x4FF963: mov ebx,[eax+0x1c]` → **arg pool ptr と count を渡して `call ebx`（`0x4FF9B9`）**＝**その record の引数 parse/validate fn**。common default＝**`FUN_004FDE30`**（`void FUN_004fde30(ushort,int,int,undefined4)`・512B ローカル buffer・L197309）、**per-record 上書きあり**（MessageBox `0x500130`／AddItem `0x4F43C0`） |
| **V-435-2: flat array と consumer** | **[確定]（構造・consumer）＋[確定(否定側)]（先頭10件）** | flat array **`0xB10290..0xB102DC`（21件・stride4）を全実読**: `Left Foot/Right Foot/Left Back Foot/Right Back Foot/Idle/Aware/Attack/Hit/Death/Weapon/(空)/Sneak/Swim/Fly/(空)/HandToHand/OneHand/TwoHand/Staff/Bow` → **(空)文字列を区切り＋index0="none" として2つの sub-list に分割**: **`base 0xB102B8`＝`['', 'Sneak','Swim','Fly']`（4）**／**`base 0xB102C8`＝`['', 'HandToHand','OneHand','TwoHand','Staff','Bow']`（6）**。**consumer＝`FUN_00407AE0` ほか（base refs: `0xB102B8`=6／`0xB102C8`=11／`0xB102E0`=22／`0xB108EC`=3）**。**先頭10件（`0xB10290..0xB102B4`＝foot4＋state6）は全セクションで ref 0 件 [確定(否定側)]**（`'Left Foot'` 文字列自体の ref も `.data 0xB10290` のみ）→ consumer は computed-address 経由か未使用＝**残点** |
| **V-435-2: `0xB108EC` の2段構造** | **[確定]（内容）＋[推定高]（理由）** | **`['LowerBody','LeftArm','LeftHand','RightArm','SpecialIdle']`（5＝NiControllerManager sequence group 名）＋ `['Start'×8,'End'×2,'Attach','Detach','Hit'×2,'Attack','Attach']`（16＝text key/event 名）** の1配列に混在。**理由 [推定高]**: `FUN_00407AE0` の4本目の `%s` が `[esp+0x5c]` を index にこの配列を引く＝**単一 name lookup で sequence group と text key の両方をカバーする flat name pool** |
| **V-435-3: `FUN_00407AE0` identity** | **[確定]（実文字列）＋[推定高]（呼び出し元）** | **debug dump fn** — body 内 string push 実読: `'FPS %d (Type %d)'`/`'Actor Loc: Interior/World/UNKNOWN'`/`'Actor Heading: %0.2f'`/`'Actor Pos: ( %.0f, %.0f, %.0f )'`/`'Overall Pathing Status: FAILED/Active'`/`'Final Target Loc/Pos'`/`'Current Low/High Target Pos'`/`'Failure Time/State'`/`'High Pathing Status: Turning/Avoiding/Pathing'`/`'Avoidance Status/Wait Time'` ＋ **`'%s -> %s/%s/%s, Count: %d'`（animation group dump）** → **actor の AI/pathing/animation 状態を出力する debug report [確定(実文字列)]**。**唯一の E8 caller＝`0x40DEFC`（`FUN_0040d4d0` L8526 内・`void FUN_0040d4d0(void)` が `FUN_00977c50(3)` を呼ぶ no-arg command entry）→ console command handler [推定高]** |
| **V-435-3: helper3本** | **[確定]** | **`FUN_0051a9d0(ushort p) = p >> 0xc`**（bits 12-15）／**`FUN_0051a9e0(uint p) = p >> 8 & 0xf`**（bits 8-11）／**`FUN_0051aa00(uchar p) = p`**（identity＝low byte）（L210789/210799/210819）。→ **packed 値の bitfield 抽出 fn 群 [確定]**。`FUN_00407AE0` が同一 `esi` を3本に渡し: **bits0-7→group index（stride-0x24 table `0xB102E0`）／bits8-11→`0xB102C8` list／bits12-15→`0xB102B8` list／`[esp+0x5c]`→`0xB108EC` list** → **`'%s -> %s/%s/%s'` の4 name を packed 値から復元 [確定(命令)]** |

## §2 証拠（v435_1..v435_8）

### §2.1 record layout（v435_1/v435_6/v435_7）
```
event   rec0 GameMode          +0x10=0x00000000 (flag0,count0) +0x14=0      +0x18=0x5022D0
        rec1 MenuMode          +0x10=0x00010000 (flag0,count1) +0x14=0xB0A6D8 +0x18=0x502270
        rec2 OnActivate        +0x10=0x00010001 (flag1,count1) +0x14=0xB0ABAC +0x18=0x503F90
        rec13 OnAlarm          +0x10=0x00020001 (flag1,count2) +0x14=0xB0ABDC +0x18=0x504310
console rec0 Show              +0x10=0x00010000 (flag0,count1) +0x14=0xB0A768 +0x18=0x502210 +0x1C=0x4FDE30
        rec1 ShowVars          +0x10=0x00000001 (flag1,count0) +0x14=0
        rec2 ShowQuestVars     +0x10=0x00010000 (flag0,count1) +0x14=0xB0AA20
func    rec0 MessageBox        +0x10=0x00010000 (count1) +0x14=0xB0A11C +0x1C=0x500130
        rec1 GetDistance       +0x10=0x00010001 (count1) +0x14=0xB0A7EC +0x1C=0x4FDE30
        rec2 AddItem           +0x10=0x00020001 (count2) +0x14=0xB0A858 +0x1C=0x4F43C0
```

### §2.2 flag consumer（v435_3）
```
FUN_004ff4c0:
  4FF904: lea eax, [ebx + 0xb0af48]        ; event record = base + offset
  4FF90A: test eax, eax / je 0x4ff9ff
  4FF918: cmp byte ptr [eax + 0x10], 0      ; ★ flag byte
  4FF91C: je 0x4ff950                       ; flag==0 -> OK
  4FF91E: cmp byte ptr [edi + 0x38], 0      ; is-quest-script?
  4FF922: je 0x4ff950
  4FF924: push 0xa4b524 ; push edi ; call 0x4fce30
         -> 'Invalid block type for quest script.'
  4FF950: mov edx, [eax+0x14] ; test / je 0x4ff9ff   ; arg pool null -> skip
  4FF95F: movzx edx, word [eax + 0x12]      ; ★ argcount
  4FF963: mov ebx, [eax + 0x1c]             ; ★ arg-parse handler
  4FF9B9: call ebx                          ; handler(pool, count, name, ctx)
```

### §2.3 arg descriptor pool（v435_2/v435_4/v435_6/v435_7）
```
stride 0x0C {name ptr, type u32, flag u32}:
0xB0ABDC (OnAlarm count=2): 'Crime Type'(0x1C,0) + 'Actor (optional)'(6,1)
                             + 'Crime Type'(0x1C,0) + 'Criminal'(6,0)   <- 共有 pool 続き
0xB0A858 (AddItem count=2): 'ObjectID'(3,0) + 'Count'(1,0)
                             + 'ObjectID'(3,0) + 'Flag'(1,1)
0xB0A768 (Show count=1)   : 'Float'(2,0) + 'Float (Optional)'(2,1)
0xB0ACE4 (OnMagicEffectHit): 'Magic Effect (optional)'(0x1F,1) + 'Magic Effect'(0x1F,0)
                              + 'Container (optional)'(0x1A,1) + 'Ownership Added (optional)'(1,1)
0xB0A7D4 (OnActorEquip)   : 'ObjectID'(0x15,0) + 'ObjectID'(0x15,1)
                              + 'ObjectReferenceID'(4,0) + 'ObjectReferenceID'(4,1)
0xB0A6D8 (MenuMode)       : 'Integer (Optional)'(1,1)
0xB0ACCC (OnPackage*)     : 'Package'(0x1D,0)
0xB0A7F8 (OnTrigger*)     : 'ObjectReferenceID'(4,0)
```
- **pool 共有**: `0xB0A768` の base を持つ record を全3表で走査 → **14件**（v435_7(b)）。

### §2.4 flat array と sub-list（v435_8(v)/v434_18）
```
0xB10290 'Left Foot'      0xB102B8 (空)      <- base A (list start, idx0=none)
0xB10294 'Right Foot'     0xB102BC 'Sneak'
0xB10298 'Left Back Foot' 0xB102C0 'Swim'
0xB1029C 'Right Back Foot'0xB102C4 'Fly'
0xB102A0 'Idle'           0xB102C8 (空)      <- base B (list start, idx0=none)
0xB102A4 'Aware'          0xB102CC 'HandToHand'
0xB102A8 'Attack'         0xB102D0 'OneHand'
0xB102AC 'Hit'            0xB102D4 'TwoHand'
0xB102B0 'Death'          0xB102D8 'Staff'
0xB102B4 'Weapon'         0xB102DC 'Bow'
                          0xB102E0 'Idle'   <- stride-0x24 table rec0
```
- base refs（v434_18）: `0xB102B8`=6／`0xB102C8`=11／`0xB102E0`=22／`0xB108EC`=3。**`0xB10290`/`0xB102A0` は全セクション ref 0 件**。
- `0xB108EC`: `LowerBody/LeftArm/LeftHand/RightArm/SpecialIdle` + `Start`×8 + `End`×2 + `Attach/Detach/Hit`×2 + `Attack/Attach`。

### §2.5 dump fn と helper（v435_5/v435_8）
```
FUN_00407AE0 (L5705, 0x407AE0..):
  string pushes: 'FPS %d (Type %d)' / 'Actor Loc: ...' / 'Actor Heading: %0.2f'
                 'Actor Pos: ( %.0f, %.0f, %.0f )' / 'Overall Pathing Status: ...'
                 'Final Target Loc/Pos' / 'Current Low/High Target Pos'
                 'Failure Time/State' / 'High Pathing Status: Turning/Avoiding/Pathing'
                 'Avoidance Status/Wait Time' / '%s -> %s/%s/%s, Count: %d'
  40A26C: call 0x51aa00  -> bits0-7  -> [edx*4+0xB102E0] (stride0x24 group name)
  40A280: call 0x51a9e0  -> bits8-11 -> [eax*4+0xB102C8]
  40A291: call 0x51a9d0  -> bits12-15-> [eax*4+0xB102B8]
  40A2A1: mov ecx,[eax*4+0xB108EC]   (eax=[esp+0x5c])
  40A2B4: push 0xA30670 ('%s -> %s/%s/%s, Count: %d')
唯一 E8 caller: 0x40DEFC @ FUN_0040d4d0 (L8526, void(void) -> FUN_00977c50(3))

FUN_0051a9d0(ushort p) { return p >> 0xc; }        L210789
FUN_0051a9e0(uint p)   { return p >> 8 & 0xf; }    L210799
FUN_0051aa00(uchar p)  { return p; }               L210819
```

## §3 残点 **V-436-1..3**
1. 【低】**console/function record の flag byte（`+0x10` bit0）の consumer 特定**（event は quest 判定で確定、console/func は `Show=0/ShowVars=1/GetDistance=1` の差の使い所）。
2. 【低】**flat array 先頭10件（`0xB10290..0xB102B4`）の consumer** — 全セクション ref 0 件 [確定(否定側)]。computed-address 経由か dead data かの切り分け。＋ **`0xB108EC` の index 源（`[esp+0x5c]` の由来＝packed 値のどの field か）**。
3. 【低】**`FUN_00407AE0` の command 名特定**（`FUN_0040d4d0` がどの console command の handler か＝`FUN_00977c50(3)` の意味と command table への登録確認）＋ **type コード `0x3` vs `0x15`（両方 name='ObjectID'）の語義差**。

## §4 検証・自己批評
- probe: **v435_1..v435_8 の8本全 EXIT=0**（V-435-1/V-435-2/V-435-3 完遂）。
- **決定打の作り**: (i) **event record 全31件の `+0x10/+0x14/+0x18` を列挙し handler 共有を group 化** → (ii) **`+0x14` の指す先を string として実読**して arg descriptor と判明 → (iii) **flag byte の consumer（`cmp byte [eax+0x10],0`）を aligned decode で特定し error 文言を実読**して quest 判定と確定 → (iv) **`+0x12` を argcount と仮説し `AddItem count=2`/`OnAlarm count=2` の desc 2件実読で検証** → (v) **全3表の `+0x14` を走査して pool 共有（14件）を発見** → (vi) dump fn は **body 内 string push 列挙**で identity、helper3本は **decompiled 1行 body**で即確定。
- 妥当性チェック: ①count=2⇔2 desc 実読（AddItem/OnAlarm）— ②count=0⇔ptr null（GameMode/ShowVars/ShowQuests/Toggle*）— ③flag byte⇔quest error 文言⇔GameMode/MenuMode のみ flag0 — ④pool 共有⇔14 record が同一 base — ⑤helper の戻り値⇔`FUN_00407AE0` の index 計算（`lea edx,[eax+eax*8]`→stride0x24）— **五方向矛盾なし**。
- docs との不整合: **なし**（v434 の残点3件を一括確定）。**副次**: v434 ruling の「`+0x1C`＝共通 name/parse callback [推定高]」は本裁定で**「arg-parse handler（common default 0x4FDE30・per-record 上書きあり）」へ精緻化**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **flag dword は「byte + pad + u16」に分解して consumer の cmp 幅（`cmp byte` vs `cmp word`）と突合する** — `cmp byte [eax+0x10]` が byte 部分、`movzx word [eax+0x12]` が u16 部分と読めば2フィールドと確定できる。
  2. **argcount の仮説は「ptr null⟺count0」の完全相関＋「count=2 の record の desc 2件実読」の2系統で検証する**（片方だけだと偶然の可能性が残る）。
  3. **共有 pool の発見は「ある1つの `+0x14` 値を持つ record を全3表で走査」** — 1本の probe で「pool 共有」構造が一撃で分かる（本件 `0xB0A768`→14件）。
  4. **fn identity は「body 内の string push 列挙」が最速** — `FUN_00407AE0` は10KB 超の巨大 fn でも string 群（FPS/Pathing/Avoidance）で役割が即決まる。
  5. **`>>n` / `>>n & 0xf` の1行 decompiled は bitfield 抽出の即決パターン** — 3本並べると packed 値の field 分割（0-7/8-11/12-15）が同時に見える。
