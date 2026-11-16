# v437 — arg type コード体系・docs 欠落の裁定（V-437-1..3 完遂・派生残点第29弾）

- 採番: **v437**（派生残点群・probe prefix **v437_1..v437_7 の7本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v436 の残点 **V-437-1【低】（type コード体系 0..0x1F 対応表）／V-437-2【低】（docs function 欠落範囲の census と dump owning fn の machine 特定）／V-437-3【低】（flat array 先頭10件の到達経路と `[esp+0x5c]` index 源）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-437-1: arg descriptor の type コード体系** | **[確定]（402件の実データから構築）** | **3表全 record（event31/console131/func369）を walkし `count` 分の descriptor `{name,type,flag}` を collection → 402件（bad 0）・38 コード [確定]**: `0x00=String/Filename`／`0x01=Integer`（135件・Emotion/Count/Speed/Amount…）／`0x02=Float`（47件・X/Y/Z/Rotation/Time）／`0x03=ObjectID`（7件）／`0x04=ObjectReferenceID`（19件）／`0x05=Actor Value`／`0x06=Actor`（34件・Criminal/Victim）／`0x07=Spell Item`／`0x08=Axis`／`0x09=Cell`／`0x0A=Animation Group`／`0x0B=Magic Item`／`0x0C=Sound`／`0x0D=Topic`／`0x0E=Quest`（10件）／`0x0F=Race`／`0x10=Class`／`0x11=Faction`（22件）／`0x12=Sex`／`0x13=Global`／`0x14=Furniture`／`0x15=ObjectID/Object`（7件）／`0x16=Variable Name`／`0x17=Stage`／`0x18=Map Marker`／`0x19=Actor Base`／`0x1A=Container`／`0x1B=WorldSpace`／`0x1C=Crime Type`／`0x1D=Package`（7件）／`0x1E=Combat Style`／`0x1F=Magic Effect`／`0x20=Birthsign`／`0x21=Form Type`／`0x22=Weather ID`／`0x23=NPC`／`0x24=Owner`／`0x25=Effect Shader ID` |
| **type `0x03` vs `0x15`（両方 name='ObjectID'）** | **[確定]（usage 分割）＋[未確定]（語義差）** | **`0x03`＝inventory item 系7件**: `AddItem/GetItemCount/RemoveItem/Drop/GetEquipped/EquipItem/UnequipItem`（**func のみ**）／**`0x15`＝identity/event 系7件**: `OnHitWith/OnActorEquip/OnActorUnequip`（event）＋`PlaceAtMe/GetIsID/GetIsUsedItem/SetQuestObject`（func）→ **usage は完全に分離 [確定]**。**語義差（なぜ同名で2 code か）は [未確定]**（consumer 未特定） |
| **type code の consumer** | **[未確定]（残点）** | `.text` 全体で `cmp reg,0x25`/`0x26`（byte/dword form 含む）＝**5件のみで parser band に0件**（`FUN_004cc660`/`FUN_00586000`/`FUN_005aa210`/`FUN_009a56d0`/`FUN_009a73c0` は無関係）→ **type の最大値 check は別形式（table lookup without explicit max cmp 等）[未確定]** |
| **V-437-2: docs function 欠落 census** | **[確定]（census）** | **`docs/Oblivion_decompiled.c` の `// FUN_` annotation＝20465件・全件統一形式 `// FUN_xxx @ 0xADDR`（他形式0件）**。**gap（次 fn 開始地址差）> 0x800 が374件** — 顶部は section 境界（`0xA27BF7→0xBAF000`＝1.6MB）と large function（`0x9A9A00→0x9D8020`＝188KB 等）。**本件 region の gap＝`FUN_0040d4d0 @0x40D4D0 → FUN_0040f640 @0x40F640`（0x2170）** |
| **V-437-2: dump owning fn** | **[確定(不整合)]（docs vs code）** | **machine**: `0x40DEFC`（`call 0x407ae0`）は **`0x40D4D0..0x40DF74` の連続コード内**（int3 境界なし・唯一の CC run は `0x40D4CE`）＋**branch census: `0x40D800..0x40DF74` への branch/call 91件すべて `head=0x40D4D0 // FUN_0040d4d0`**（＝同 fn の reachable code）→ **machine 上 dump call は `FUN_0040d4d0` 内 [確定]**。**docs**: `FUN_0040d4d0` body（L8522..L8659・141行）に **`FUN_00407ae0`/`FUN_005791a0`/`FUN_005791e0`/`FUN_00579220` 全て不在**＋**docs 内で `FUN_00407ae0` は annotation（L5705）と signature（L5710）の2件のみ・一度も call されない** → **docs の `FUN_0040d4d0` body が machine 範囲をカバーしていない＝docs vs code の不整合 [確定(不整合)]**（docs body の tail は machine `0x40D7BD..0x40D7FF` に対応: `FUN_0040fda0`→`0x40D7BD`／`FUN_00411100`→`0x40D7D9`／`FUN_00977c50(2)`→`0x40D7E0`／ret `0x40D7FF`） |
| **V-437-3: flat array 先頭10件の到達経路** | **[確定(否定側)]＋[推定高]** | **(i) 全セクション ref 0 件**（v434 確認）＋**(ii) `FUN_0051b490` の name index は `mov esi,1` 起点（`0x51B5DE`/`0x51B643`）＝正の index のみ** → base `0xB102B8` から **下方向（`0xB10290`＝-10）へ到達不可** → **先頭10件（foot4＋state6）は dead data [推定高]**（computed-address 経由は [未確定]） |
| **V-437-3: `[esp+0x5c]`（4th `%s` index）の書込み元** | **[未確定]（残点）** | `FUN_00407AE0` の `0x40A200..0x40A278` window では `[esp+0x5c]` の書込みが出現せず（`0x40A29D: mov eax,[esp+0x5c]` の読出のみ）→ **書込みは更に前の block [未確定]**。同 window で判明したのは: ループ counter＝`xor esi,esi; mov [esp+0x4c],esi`（`0x40A239`）→ `call 0x470720(counter)` → `movzx esi,ax` → `call 0x51aa00`（low byte 抽出）→ `lea edx,[eax+eax*8]` → **stride-0x24 group index** |

## §2 証拠（v437_1..v437_7）

### §2.1 type コード表（v437_1/v437_2）
```
walk: event(31) + console(131) + func(369) records
      count=(+0x10>>16)&0xFFFF, ptr=+0x14, desc stride 0x0C
collected: 402 descriptors, bad 0
type -> names（実データ）:
  0x00 String/Filename(27)   0x01 Integer(135)          0x02 Float(47)
  0x03 ObjectID(7)           0x04 ObjectReferenceID(19) 0x05 Actor Value(8)
  0x06 Actor(34)             0x07 Spell Item(3)         0x08 Axis(7)
  0x09 Cell(9)               0x0A Animation Group(2)    0x0B Magic Item(3)
  0x0C Sound(2)              0x0D Topic(4)              0x0E Quest(10)
  0x0F Race(2)               0x10 Class(4)              0x11 Faction(22)
  0x12 Sex(2)                0x13 Global(1)             0x14 Furniture(1)
  0x15 ObjectID/Object(7)    0x16 Variable Name(2)      0x17 Stage(2)
  0x18 Map Marker(1)         0x19 Actor Base(2)         0x1A Container(7)
  0x1B WorldSpace(3)         0x1C Crime Type(4)         0x1D Package(7)
  0x1E Combat Style(1)       0x1F Magic Effect(5)       0x20 Birthsign(1)
  0x21 Form Type(1)          0x22 Weather ID(3)         0x23 NPC(1)
  0x24 Owner(4)              0x25 Effect Shader ID(2)
name with multiple types: 'ObjectID' -> ['0x03','0x15']   (唯一の ambiguity)
type 0x03 users: func:AddItem/GetItemCount/RemoveItem/Drop/GetEquipped/EquipItem/UnequipItem
type 0x15 users: event:OnHitWith/OnActorEquip/OnActorUnequip
                 func:PlaceAtMe/GetIsID/GetIsUsedItem/SetQuestObject
```

### §2.2 docs census（v437_3/v437_4）
```
'// FUN_' lines: 20465   with ' @ 0x': 20465   other format: 0
gaps > 0x800: 374
  0x187409: FUN_00a27bf7 @0xA27BF7 -> FUN_00baf000 @0xBAF000   (section boundary)
  0x2E620 : FUN_009a9a00 @0x9A9A00 -> FUN_009d8020 @0x9D8020
  0x11000 : FUN_00820910 @0x820910 -> FUN_00831910 @0x831910
  0x4740  : FUN_00407ae0 @0x407AE0 -> FUN_0040c220 @0x40C220  (dump fn itself, 1999-line body)
raw greps '0x40d8'..'0x40f5' (16 patterns): ALL 0 hits
```

### §2.3 dump call の docs/code 不整合（v437_5/v437_6）
```
machine branch census into 0x40D800..0x40DF74: 91 branches/calls
  ALL have head=0x40D4D0 // FUN_0040d4d0 @ 0x40d4d0   -> 同 fn の reachable code
FUN_00407ae0 in docs: 2 hits (L5705 annotation, L5710 signature) — NEVER CALLED
FUN_005791a0 in docs: L50025/L70527/L119198/L326504 called (other fns) + L281292 annotation
FUN_0040d4d0 body: 141 lines, 27 distinct FUN_ called
  (FUN_004032d0, FUN_00403520, FUN_0040c830, FUN_0040fda0, FUN_00411100, FUN_00411750,
   FUN_00440af0, FUN_00466b70, FUN_004826f0, FUN_00497c30, FUN_00572df0, FUN_00572e30,
   FUN_00572e70, FUN_00572ec0, FUN_00579060, FUN_00579260, FUN_00674500, FUN_00709c60,
   FUN_007a99a0, FUN_007b4290, FUN_007b4900, FUN_007b8400, FUN_007c1ee0, FUN_007d71c0,
   FUN_007d7210, FUN_00977c50)  — FUN_00407ae0 は不在
```

### §2.4 name index（v437_7）
```
FUN_0051b490:
  51B5DE: mov esi, 1 / 51B5E3: add [esp+0x18], esi / 51B5F0: mov edx,[esi*4+0xb102b8]
  51B643: mov esi, 1 / 51B650: mov edx,[esi*4+0xb102c8]
  -> index は 1 起点（正のみ）＝ 0xB102B8 の下方向へ到達しない
FUN_00407AE0 loop:
  40A239: xor esi,esi / 40A23B: mov [esp+0x4c], esi      ; counter=0
  40A240: push esi / 40A243: call 0x4706e0
  40A254: mov eax,[esp+0x4c] / 40A25B: call 0x470720      ; returns ax
  40A264: movzx esi, ax / 40A26C: call 0x51aa00           ; low byte
  40A271: lea edx,[eax+eax*8] / 40A274: mov eax,[edx*4+0xb102e0]  ; group name
  ([esp+0x5c] の書込みはこの window 外)
```

## §3 残点 **V-438-1..3**
1. 【低】**type code の consumer 特定**（`cmp 0x25/0x26` が無いので table lookup 形を疑う）＋ **`0x03` vs `0x15` の語義差**（同名2 code の設計理由）。
2. 【低】**docs と machine の function 範囲不一致の全体 census** — 本 region で dump call が docs body に無いことが判明。他の function でも同種の不一致（docs body が machine 範囲を欠く）があるかの横断チェック手法の確立。
3. 【低】`[esp+0x5c]`（4th `%s` index）の書込み元特定＋ flat array 先頭10件の computed-address 到達の有無。

## §4 検証・自己批評
- probe: **v437_1..v437_7 の7本全 EXIT=0**（V-437-1 完遂／V-437-2 完遂（不整合を確定）／V-437-3 部分解）。
- **決定打の作り**: (i) **type 表は「全 record を walkし `count` 分の descriptor を collection」**（scatter region の flat scan は失敗→手順訂正で402件取得）→ (ii) **usage 分割は「type ごとに使用 record を列挙」**で `0x03`/`0x15` の分離を確定 → (iii) **docs census は「annotation 全数＋形式統一性＋gap 分布＋raw address grep」の4系統**で形式起因を排除 → (iv) **dump 帰属は「branch census（91件全 head）」と「docs 内 FUN_00407ae0 の call 数＝0」の対比で不整合を確定** → (v) **name index は `mov esi,1` の起点特定で negative到達を否定**。
- 妥当性チェック: ①402 descriptors＝3表全 record の count 合計と整合（bad 0）— ②`0x03`/`0x15` の usage が name='ObjectID' のみで交錯せず（唯一 ambiguity を検出）— ③docs annotation 20465件が全件統一形式⇔raw grep 0 hits（形式起因でない）— ④branch census 91件全 head=FUN_0040d4d0⇔docs body に dump call 不在 — ⑤`mov esi,1`⇔`0xB10290` ref 0件 — **五方向矛盾なし**。
- docs との不整合: **①`docs/Oblivion_decompiled.c` の `FUN_0040d4d0` body（141行）が machine 範囲 `0x40D4D0..0x40DF74` をカバーしていない — `FUN_00407ae0`（dump）・`FUN_005791a0/1e0/220` の call が docs に無く、docs 内で `FUN_00407ae0` は一度も call されない [確定(不整合)]**／②docs body tail は machine `0x40D7BD..0x40D7FF` に対応（`FUN_0040fda0`/`FUN_00411100`/`FUN_00977c50(2)` の唯一 call site 対応）／③v436 ruling の「docs function 欠落（`0x40D800..0x40F640`）」は **branch census 91件で同 fn の reachable code と判明したため「欠落」から「docs body の範囲不一致」へ表現を訂正**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **type/enum 対応表は「全 record を walkして count 分だけ collection」** — scatter したデータを region flat scan すると1件しか当たらない（本件 v437_1 初版の失敗と訂正）。
  2. **同名で複数 code がある enum は「usage 列挙」で分離を確定できる**（`0x03`＝func inventory 系／`0x15`＝event+identity 系）— 語義差は consumer が無いと [未確定] で正直に残す。
  3. **docs の完全性は「annotation 形式統一性＋raw address grep＋gap 分布」の3系統で検証** — 形式が全件統一なら gap は欠落か large function のどちらか。
  4. **「docs body に call が無いのに machine に call がある」＝ docs body の範囲不一致の決定打** — 対象 fn の docs 内 call 数を grep するだけで判別できる（本件 `FUN_00407ae0`＝docs call 数0）。
  5. **branch census（対象 range への全 jump/call の head 集計）で「別 fn か同 fn か」を機械判定** — head が全部同じなら同 fn、docs body に無いなら docs 側の範囲問題。
