# v523 — cast 先＝`EffectSetting`（RTTI）・`FUN_0046B030`＝`NiTMapBase` dtor・`FUN_0046ACB0` signature の裁定（V-523-1..3 完遂・旧残点 V-406-3 継続第53弾）

- 採番: **v523**（旧残点群消化・probe prefix **v523_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-523-1..3（v522 残点）** — 1=`PTR_PTR_00b03534` RTTI type（`FUN_00416000` の cast 先）＋`FUN_0046ACB0` identity＋`FUN_0046B030` identity／2=`FUN_0045C7A0` 本文と caller 分離／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-523-1: `PTR_PTR_00b03534` RTTI type（`FUN_00416000` の cast 先）** | **[確定]（RTTI）→ `.?AVEffectSetting@@`** | `.rdata` TD 実読 [確定(実RTTI)]: **dword `@0xB03534`＝`0x00AA3E80`、direct TD name＝**`.?AVEffectSetting@@`**（ctx hex `2E 3F 41 56 45 66 66 65 63 74 53 65 74 74 69 6E 67 40 40`）→ **cast 先 type＝`EffectSetting` [確定]** → **`FUN_00416000`＝「`param_2` を `TESForm`→`EffectSetting` に `__RTDynamicCast` し、成功したら `FUN_0046ACB0(param_2)` を呼ぶ fn [確定相当]」**（v522 部分確定を確定へ引き上げ）。**`EffectSetting` は v504（`EffectSettingCollection.cpp` LoadGame）の同 family ＝接続点が判明 [確定相当]** |
| **V-523-1: `FUN_0046B030`（`NiTMapBase` ctor/dtor 候補・slot `+0`）identity** | **[確定相当]（scalar deleting destructor）** | L84623 実読 [確定(decompiled)]: **`undefined4 * __thiscall FUN_0046b030(undefined4 *param_1, byte param_2)`: **`*param_1 = &PTR_FUN_00a3bc94;`（**vtable store（v521 の `NiTMapBase` vtable！）**）→ `FUN_007c2a40()`（base cleanup 候補 [推定]）→ **`FUN_00401f20(param_1[2])`（**bucket 配列 free**）**→ `if ((param_2 & 1) != 0) { FUN_00401f20(param_1); }`（**flag bit0 で this を free＝scalar deleting destructor の定番 signature**）→ `return param_1`** → **`FUN_0046B030`＝`NiTMapBase` の scalar deleting destructor（vtable 復元→base cleanup→bucket free→`param_2 & 1` で this free）[確定相当]**（v521「ctor/dtor 候補」を **dtor 側と確定**） |
| **V-523-1: `FUN_0046ACB0`（cast 成功後の呼出し）identity** | **[部分確定]（signature 判明・本文 cap 外）** | L84411 実読 [確定(decompiled)]: **`void __thiscall FUN_0046acb0(undefined4 param_1, undefined4 param_2)`（2引数）→ SEH setup（`local_dc[208]`（**208B バッファ**）・`puStack_8 = &LAB_009ae9f3`・`*unaff_FS_OFFSET`・`DAT_00b30aac ^ &stack`（**cookie XOR**））→ 本文は cap 外＝[部分確定]**（**SEH＋stack cookie 付き large fn [確定相当]**、`param_2`＝cast 済み EffectSetting [推定]） |
| **V-523-2: `FUN_0045C7A0` 本文／caller 分離** | **[未確定]（残点継続）** | 本 probe 未着手（budget は RTTI/dtor/cast handler に集中） |
| **V-523-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v523_1）

### §2.1 cast 先 RTTI（v523_1a）
```
dword @0xB03534 = 0x00AA3E80
direct TD name = '.?AVEffectSetting@@'
  ctx hex: 80 3E AA 00 00 00 00 00 2E 3F 41 56 45 66 66 65 63 74 53 65 74 74 69 6E 67 40 40
=> FUN_00416000 casts TESForm -> EffectSetting  [確定]
   connects to v504 EffectSettingCollection.cpp LoadGame band  [確定相当]
```

### §2.2 `FUN_0046B030` dtor（v523_1c）
```
L84623: undefined4 * __thiscall FUN_0046b030(undefined4 *param_1, byte param_2) {
L84626:   *param_1 = &PTR_FUN_00a3bc94;      // vtable store (NiTMapBase, v521)
L84627:   FUN_007c2a40();                      // base cleanup [推定]
L84628:   FUN_00401f20(param_1[2]);            // free bucket array
L84629:   if ((param_2 & 1) != 0) {
L84630:     FUN_00401f20(param_1); }           // free this (scalar deleting dtor flag)
L84632:   return param_1; }
=> NiTMapBase scalar deleting destructor  [確定相当]
```

### §2.3 `FUN_0046ACB0`（v523_1b）
```
L84411: void __thiscall FUN_0046acb0(undefined4 param_1, undefined4 param_2) {
L84416:   undefined1 local_dc [208];           // 208B buffer
L84420:   puStack_8 = &LAB_009ae9f3;           // SEH
L84422:   uVar1 = DAT_00b30aac ^ (uint)&stack0xffffff20;   // stack cookie
  -> body beyond cap  [部分確定]; SEH + cookie large fn  [確定相当]
```

## §3 残点 **V-524-1..3**
1. 【中】**`FUN_0046ACB0` 本文**（EffectSetting cast 成功後の処理＝何をするか）＋ **`FUN_007C2A40`（dtor base cleanup）identity**。
2. 【低】**`FUN_0045C7A0` 本文**（DWORD locals の中身）＋ caller 分離（経路分離＝残点）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v523_1 の1本 EXIT=0**（V-523-1 cast 先＝`EffectSetting` を RTTI 実読で確定・`FUN_0046B030`＝scalar deleting dtor を確定相当・`FUN_0046ACB0` は signature まで部分確定／V-523-2 は budget 外／V-523-3 引継ぎ）。
- **決定打の作り**: (i) **`PTR_PTR_00b03534` の TD 名実読** → `.?AVEffectSetting@@` で **cast 先を確定**（v522 残点解消・v504 EffectSettingCollection と接続）→ (ii) **`FUN_0046B030` docs 10行** → `*param_1 = &PTR_FUN_00a3bc94`（v521 vtable）＋`FUN_00401f20(param_1[2])`（bucket free）＋`param_2 & 1`（this free）で **scalar deleting dtor と即決** → (iii) **`FUN_0046ACB0` head** で SEH＋208B buffer＋stack cookie を獲得。
- 妥当性チェック: ①`.?AVEffectSetting@@`⇔v504 `EffectSettingCollection.cpp`（LoadGame band）— ②`PTR_FUN_00a3bc94`⇔v521 `NiTMapBase` RTTI（同一 vtable）— ③`param_2 & 1`⇔MSVC scalar deleting destructor 定番（`FUN_00401F20`＝free 系・v508/v512 と同 band）— ④`FUN_009832E6` cast（v522）⇔cast 先が `EffectSetting` と一致 — ⑤stack cookie（`DAT_00b30aac ^ &stack`）⇔v517/v522 SEH pattern — **五方向矛盾なし**。
- docs との不整合: **なし**（RTTI・dtor body とも docs と一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **cast 先 type ptr（`PTR_PTR_xxx`）の TD 実読が cast semantics を確定**（**`.?AVEffectSetting@@`＝v522 部分確定を確定へ引き上げ**＝RTTI 法の4度目・**`EffectSetting`＝v504 との接続点**）。
  2. **`*param_1 = &PTR_FUN_...`（vtable store）＋`param_2 & 1`（this free）が scalar deleting destructor の定番**（**dtor の identity は vtable 復元＋flag free の2点で即決**＝v505 dtor pattern の反復）。
  3. **`FUN_00401F20`（free 系）が bucket 配列の解放に使われる**（**`param_1[2]`＝bucket（v521 で確認した `param_1[2]` bucket 構造と整合**＝横断確認）。
  4. **SEH setup＋stack cookie（`DAT_00b30aac ^ &stack`）が large fn の目印**（**208B バッファ＝`local_dc[208]` が fn サイズの手がかり**）。
  5. **`EffectSetting`（cast 先）と `EffectSettingCollection.cpp`（v504 LoadGame）の接続が判明**（**cast 経路が LoadGame/EffectSetting 系業務に使われる [推定]**＝次の手の方向）。
