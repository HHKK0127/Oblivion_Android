# v518 — `FUN_00448D40`＝current id（FF+2048）・type10 extra create-if-missing・`0x800` 3義の裁定（V-518-1..3 完遂・旧残点 V-406-3 継続第48弾）

- 採番: **v518**（旧残点群消化・probe prefix **v518_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-518-1..3（v517 残点）** — 1=`FUN_00448D40` identity＋`L26536` context（type 10 extra `+0xc` の値）＋`0x800` semantic／2=gated mask 源と caller 分離／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-518-1: `FUN_00448D40`（current id getter）identity** | **[確定相当]（current id ＝ FF byte＋2048 計算）** | L55957 実読＋本文内 `0x800` 行 [確定(decompiled)]: **`undefined4 __fastcall FUN_00448d40(int param_1)`（`__fastcall` 1引数・iStack_4=param_1 退避）→ body 内 L55975: **`iVar5 = (uVar2 & 0xff000000) + 0x800;`** → **current id ＝「`uVar2` の `0xFF` 上位 byte を残して下位24bit ＋ `0x800`（2048）」の計算 [確定相当]** → v517 ctor の `param_1[3] = FUN_00448d40()`（current id）と `0x800` insert がこの計算と整合（**`0x800`＝2048＝id base [確定相当]**）。body の残り（`uVar2` 出所）は cap 外＝[部分確定] |
| **V-518-1: `L26536` context（type 10 extra `+0xc` の値・usage2）** | **[確定相当]（create-if-missing pattern）** | L26531-26549 実読 [確定(decompiled)]: **`local_4 = 0xffffffff; puStack_8 = &LAB_009c309b;`（SEH setup）→ `iVar2 = FUN_0041e210(10)`（`GetExtraData(type=10)`）→ `if (param_1 == 0) { if (iVar2 != 0) { FUN_00422e20(iVar2, 1); return; } }` else `if (iVar2 == 0) { iVar2 = FUN_00401f00(0x14, uVar1); ... }` → **type 10 extra が無ければ `FUN_00401f00(0x14)`（**alloc `0x14`＝20B**）で作る create-if-missing、有れば `FUN_00422e20(iVar2, 1)`（set/call 候補 [推定]）** → **type 10 extra data は20B サイズの object [確定相当]**、`+0xc` field は20B 内の offset [推定] |
| **V-518-1: `0x800`（2048）semantic** | **[確定相当]（3義を確認）** | `0x800` mentions（filtered）＝**102件 [確定]**: **①bit11 mask（`uVar9 & 0x800`・L6218/L6394/L6663 等＝flag bit11 判定）②grid 半セル math（`-0x800U & 0xfffff000`／`* 0x1000 + 0x800`・L10663/L10753/L53290＝位置計算の 2048 半セル）③id base（L55975 `(uVar2 & 0xff000000) + 0x800`＝current id 計算、L54381 `param_1[0x230] = 0x800`＝field 初期化）→ **`0x800` は「bit11 flag mask／grid 半セル（2048）／form id base（2048）」の3義で使われる [確定相当]**（v517 ctor の `0x800` insert＝**3義目（id base）** [確定相当]） |
| **V-518-2: gated mask 源／caller 分離** | **[未確定]（残点継続）** | 本 probe 未着手 |
| **V-518-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v518_1）

### §2.1 `FUN_00448D40`（v518_1a）
```
L55957: undefined4 __fastcall FUN_00448d40(int param_1) {
L55968:   iStack_4 = param_1;
  ... body (cap partial)
L55975:   iVar5 = (uVar2 & 0xff000000) + 0x800;   // FF top byte + 2048
=> current id computation: (uVar2 & 0xFF000000) + 0x800  [確定相当]
   0x800 = 2048 = form id base  [確定相当]  (matches v517 ctor insert)
```

### §2.2 type 10 extra usage 2（v518_1b, L26536）
```
L26531: local_4 = 0xffffffff; puStack_8 = &LAB_009c309b;   // SEH
L26536: iVar2 = FUN_0041e210(10);                            // GetExtraData(10)
L26537: if (param_1 == 0) {
L26538:   if (iVar2 != 0) { FUN_00422e20(iVar2, 1); return; }  // set/call [推定]
L26544: } else {
L26545:   if (iVar2 == 0) { iVar2 = FUN_00401f00(0x14, uVar1); // alloc 20B
L26548:     if (iVar2 == 0) { uVar3 = 0; ...
=> type 10 extra = create-if-missing, size 0x14 (20B)  [確定相当]
   +0xc field lives within the 20B object  [推定]
```

### §2.3 `0x800` census（v518_1c）
```
0x800 mentions (filtered): 102
  bit11 mask:      L6218/L6394/L6663/L6678 (uVar9 & 0x800)
  grid half-cell:  L10663/L10667 (-0x800U & 0xfffff000), L10753/L10754
                   (* 0x1000 + 0x800), L53290/L53291 (* -0x1000) - 0x800
  id base:         L55975 ((uVar2 & 0xff000000) + 0x800), L54381 (param_1[0x230] = 0x800)
=> 0x800 = bit11 flag / grid 2048 half-cell / form id base  [確定相当]
   v517 ctor insert = id-base sense  [確定相当]
```

## §3 残点 **V-519-1..3**
1. 【中】**`FUN_00448D40` body の残り**（`uVar2` 出所＝current id の元）＋ **`FUN_00422E20(iVar2, 1)`（type10 extra set/call）identity** ＋ **`FUN_00401F00(0x14)` alloc object の `+0xc` field 語**。
2. 【低】**gated 側 mask 源（`edx` 設定元）** ＋ caller 分離（経路分離＝残点）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v518_1 の1本 EXIT=0**（V-518-1 `FUN_00448D40`＝FF+2048 current id 計算を本文行で確定相当・type10 extra＝create-if-missing 20B を確定相当・`0x800` 3義を census で確定相当／V-518-2/3 は引継ぎ）。
- **決定打の作り**: (i) **`FUN_00448D40` docs head＋本文内 `0x800` 行** → `(uVar2 & 0xff000000) + 0x800` で **current id 計算と id base 2048 を即決**（v517 ctor の `param_1[3] = FUN_00448d40()` と `0x800` insert に接続）→ (ii) **`L26536` context** → `FUN_00401f00(0x14)` alloc で **type10 extra＝20B create-if-missing** と確定 → (iii) **`0x800` census（102件）**で3義（bit11 mask／grid 半セル／id base）を分類。
- 妥当性チェック: ①`(uVar2 & 0xff000000) + 0x800`⇔v512 `0xff000000` mask（FF 上位 byte gate 同型）— ②`0x800`＝2048⇔v517 `FUN_00452570(0x800, param_1)` insert（id base）— ③`FUN_00401f00(0x14)`⇔`FUN_00401F00` alloc band（v505 alloc `len+1`／v507 alloc `0x1C`＝28B に続く size alloc）— ④`FUN_00422e20(iVar2, 1)`⇔set/call（`1`＝flag 引数 [推定]）— ⑤`uVar9 & 0x800`⇔bit11（v507 node bit 13種に bit11 あり＝整合）— **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_00448D40`/`L26536` とも docs と一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **`0xFF000000` mask＋`+ 0x800` の1行が「FF byte＋2048 id 計算」の定番**（**v512 FF id set の getter 側**＝`FUN_00446B40`（set）⇔`FUN_00448D40`（get）の対が揃う）。
  2. **`0x800` の3義（bit11 mask／grid 半セル／id base）は census 分類で判別**（**同定数でも文脈で意味が変わる＝「`0x800` 単独では id と断定しない」**）。
  3. **create-if-missing pattern（`get; if null -> alloc(size)`）が extra data setter の定番**（**`FUN_00401f00(0x14)`＝type10 の object size 20B**＝size alloc census に追加）。
  4. **getter/setter の対（`FUN_00448D40`/`FUN_00446B40`）を揃えると field の full picture が完成**（**current id field の get/set 両面が確定**）。
  5. **SEH setup（`puStack_8 = &LAB_...`／`*unaff_FS_OFFSET`）が fn head の目印**（**exception handling 付き fn の判別**＝v497/v500 に続く）。
