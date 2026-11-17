# v524 — `FUN_0045C7A0`＝`DeleteForm()`（実文字列）・`FUN_007C2A40`＝bucket 解放・`FUN_0046ACB0` 構造の裁定（V-524-1..3 完遂・旧残点 V-406-3 継続第54弾）

- 採番: **v524**（旧残点群消化・probe prefix **v524_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-524-1..3（v523 残点）** — 1=`FUN_0046ACB0` 本文（EffectSetting cast 成功後）と `FUN_007C2A40` identity／2=`FUN_0045C7A0` 本文（DWORD locals）と caller 分離／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-524-2: `FUN_0045C7A0`（DWORD locals・allocator caller 2）本文** | **[確定]（実文字列）→ `DeleteForm()`** | L73052-73084 実読 [確定(decompiled)＋実文字列]: **`DVar2 = *(DAT_00b33398 + 0x10)`（owner thread）→ `DVar4 = GetCurrentThreadId()` → 同 thread なら `bVar3 = *(byte*)(param_1 + 0x18)`／異 thread なら `bVar3 = (*(uint*)(param_1 + 0x18) >> 0x12)`（**thread 依存 read（byte bit0 ⇔ u32 bit18）**）→ `if ((bVar3 & 1) == 0) { FUN_004a7a60("DeleteForm() was called, but the game is not being loaded."); }`（**gate: game-not-loaded flag＝実文字列で fn 名を確定**）→ `FUN_00452df0(param_2[3], 1)` → **`iVar5 = FUN_009832e6(param_2, 0, &PTR_PTR_00b02f9c, &PTR_PTR_00b03038, 0)`（**RTDynamicCast: TESForm→`0xB03038` type**）**→ `if ((((uint)param_2[2] >> 0xe & 1) == 0) && (iVar5 == 0))`（**bit14 gate（v512 family）AND cast 失敗**）→ **`FUN_0046c300(0, 1)`（set/replace=0・v511）→ `FUN_0046b2c0()`（reset・v514）→ `virtual[*param_2 + 0xd8](&DAT_00a2f7ec)`（**slot `+0xD8`＝SetName family・v474/v478**）→ `param_1+0x30` list 走査で `param_2` を探して `FUN_00446cb0(param_2)`（delete/free 候補 [推定]）** else **`uVar6 = FUN_00448d40(1)`（**allocator・v519**）→ `FUN_0046c300(uVar6, 1)`（**新 id で set/replace**）** → **`FUN_0045C7A0`＝`DeleteForm()`（form 削除 fn: game gate→cast 判定→unlink/`FUN_00446CB0` 呼び出し or 新 id 再割当）[確定]** → **allocator（v519）／set/replace（v511）／reset（v514）／bit14 gate（v512）／RTDynamicCast（v522/v523）／slot `+0xD8`（v474/v478）が1つの fn に集結＝横断接続が完成**。**副次: `param_1+0x18` の thread 依存 read（同 thread＝byte bit0／異 thread＝u32 bit18）が `>> 0x12`（bit18）で確認 [確定] → v497 bit18 residual に部分回答** |
| **V-524-1: `FUN_007C2A40`（`NiTMapBase` dtor base cleanup）identity** | **[確定相当]（bucket/node 解放）** | L680014 実読 [確定(decompiled)]: **`void __fastcall FUN_007c2a40(int *param_1)`: `uVar3 = 0; if (param_1[1] != 0) { do { iVar1 = *(param_1[2] + uVar3*4); while (iVar1 != 0) { ... } }` → **bucket 配列（`param_1[2]`）を走査して node を解放 [確定相当]**（`param_1[1]`＝bucket 数（v521/v522 と同構造））→ **`FUN_007C2A40`＝`NiTMapBase` dtor の bucket/node 解放 fn [確定相当]**（v523 dtor（`FUN_0046B030`）内の `FUN_007c2a40()` 呼び出しと整合） |
| **V-524-1: `FUN_0046ACB0`（cast-success handler）本文** | **[部分確定]（構造判明・semantic は残点）** | L84425-84438 実読 [確定(decompiled)]: **`*unaff_FS_OFFSET = &local_c`（SEH）→ `FUN_00466e60(uVar1)` → `FUN_00466e60()` → `FUN_00466e80(param_1)` → `FUN_00466e80(param_2)` → **`FUN_00466dd0(local_dc)`（**208B バッファを渡す**）**→ `FUN_0060d0a0()`（**empty stub・v517**）×2 → `*unaff_FS_OFFSET = local_c`（SEH 復元）→ return** → **`FUN_0046ACB0`＝`FUN_00466E60`/`FUN_00466E80`/`FUN_00466DD0`（＋`FUN_0060D0A0` stub）を呼ぶ handler [部分確定]**（`FUN_00466DD0` が 208B buffer を受け取る＝serialize/copy 候補 [推定]）→ **`FUN_00466E60/6E80/6DD0` family identity は残点** |
| **V-524-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v524_1）

### §2.1 `FUN_0045C7A0` = DeleteForm（v524_1c）
```
L73052: DVar2 = *(DAT_00b33398 + 0x10);              // owner thread
L73053: DVar4 = GetCurrentThreadId();
L73054: if (DVar4 == DVar2) { bVar3 = *(byte *)(param_1 + 0x18); }        // same thread: byte bit0
L73057: else { bVar3 = (byte)(*(uint *)(param_1 + 0x18) >> 0x12); }       // other: u32 bit18
L73060: if ((bVar3 & 1) == 0) {
L73061:   FUN_004a7a60("DeleteForm() was called, but the game is not being loaded.");  // REAL STRING
L73063: FUN_00452df0(param_2[3], 1);
L73064: iVar5 = FUN_009832e6(param_2, 0, &PTR_PTR_00b02f9c, &PTR_PTR_00b03038, 0);
           // RTDynamicCast: TESForm -> 0xB03038 type  (RTTI = residual)
L73065: if ((((uint)param_2[2] >> 0xe & 1) == 0) && (iVar5 == 0)) {   // bit14 gate AND cast fail
L73066:   FUN_0046c300(0, 1);                        // set/replace = 0 (v511)
L73067:   FUN_0046b2c0();                            // reset (v514)
L73068:   virtual[*param_2 + 0xd8](&DAT_00a2f7ec);   // slot +0xD8 SetName family (v474/v478)
L73069:   puVar1 = param_1 + 0x30;                   // list walk
L73072:   FUN_00446cb0(param_2);                     // delete/free [推定]
L73079: } else {
L73081:   uVar6 = FUN_00448d40(1);                   // ALLOCATOR (v519)
L73082:   FUN_0046c300(uVar6, 1); }                  // set/replace with new id (v511)
=> FUN_0045C7A0 = DeleteForm()  [確定]
   allocator + set/replace + reset + bit14 gate + cast + slot+0xD8 all converge here
   param_1+0x18 thread-dependent read: byte bit0 (same) vs u32 bit18 (other)  [確定]
   -> partial answer to v497 bit18 residual
```

### §2.2 `FUN_007C2A40`（v524_1b）
```
L680014: void __fastcall FUN_007c2a40(int *param_1) {
L680021:   uVar3 = 0;
L680022:   if (param_1[1] != 0) {                     // bucket count (v521/v522 structure)
L680024:     iVar1 = *(param_1[2] + uVar3 * 4);      // bucket array (param_1[2])
L680025:     while (iVar1 != 0) { ... walk + free ...
=> NiTMapBase dtor bucket/node release  [確定相当]
   called by FUN_0046B030 dtor (v523)  [整合]
```

### §2.3 `FUN_0046ACB0`（v524_1a）
```
L84425: *unaff_FS_OFFSET = &local_c;                 // SEH
L84426: FUN_00466e60(uVar1);
L84428: FUN_00466e60();
L84430: FUN_00466e80(param_1);
L84431: FUN_00466e80(param_2);
L84432: FUN_00466dd0(local_dc);                      // 208B buffer
L84434: FUN_0060d0a0();  L84436: FUN_0060d0a0();     // empty stub x2 (v517)
L84437: *unaff_FS_OFFSET = local_c;                  // SEH restore
=> handler calling FUN_00466E60/6E80/6DD0 family  [部分確定]
   FUN_00466DD0 takes the 208B buffer (serialize/copy candidate [推定])
   FUN_00466E60/6E80/6DD0 identities = residual
```

## §3 残点 **V-525-1..3**
1. 【中】**`PTR_PTR_00b03038` RTTI type**（`DeleteForm` の cast 先）＋ **`FUN_00446CB0`（delete/free 候補）identity** ＋ `FUN_00466E60`/`FUN_00466E80`/`FUN_00466DD0` family identity。
2. 【低】**`param_1+0x18` thread 依存 read の owner object**（`DAT_00b33398+0x10`＝owner thread・`param_1` が何の object か）＋ caller 分離（経路分離＝残点）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v524_1 の1本 EXIT=0**（V-524-2 `FUN_0045C7A0`＝`DeleteForm()` を実文字列で確定・6系統（allocator/set/replace/reset/gate/cast/slot）の横断接続を獲得・bit18 に部分回答／V-524-1 `FUN_007C2A40`＝bucket 解放を確定相当・`FUN_0046ACB0` は構造まで部分確定／V-524-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_0045C7A0` docs body 34行** → **`"DeleteForm() was called…"` 実文字列**で fn 名を即決 → body の `FUN_00448D40`/`FUN_0046C300`/`FUN_0046B2C0`/bit14/cast/`+0xD8` が全て揃い **横断接続が完成** → (ii) **`FUN_007C2A40` head** → bucket walk で dtor 解放と確定 → (iii) **`FUN_0046ACB0` 続き** → `FUN_00466Exx` family と `FUN_0060D0A0` stub を獲得。
- 妥当性チェック: ①`FUN_00448d40(1)`⇔v519 allocator（同 fn）— ②`FUN_0046c300(uVar6, 1)`⇔v511 set/replace（同 fn）— ③`FUN_0046b2c0()`⇔v514 reset（同 fn）— ④`param_2[2] >> 0xe & 1`⇔v512 bit14 gate family — ⑤`virtual[*param_2 + 0xd8]`⇔v474/v478 `+0xD8` SetName slot — ⑥`>> 0x12`＝bit18⇔v497 bit18 residual（**thread 依存 read で bit18 を観測**）— **六方向矛盾なし**。
- docs との不整合: **なし**（`DeleteForm` 文字列・docs body とも一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **fn 名は `"XxxFn() was called, but…"` 形の log 文字列で一撃**（**`DeleteForm()`＝v504/v505/v511 に続く log context 法の再現**＝**業務 fn の実名が判明**）。
  2. **1つの業務 fn に container 系の全要素（allocator/set/reset/gate/cast/slot）が集結する**（**横断接続の完成点＝`DeleteForm()` が hub**＝v511..v523 の連鎖が1箇所に束まる）。
  3. **thread 依存 read（同 thread＝byte／異 thread＝`>>0x12`）が bit18 を可視化**（**v497 bit18 residual に部分回答＝`param_1+0x18` bit18 は「異 thread 観測時の flag」[推定相当]**）。
  4. **`GetCurrentThreadId()` 比較＋owner thread field（`DAT_00b33398+0x10`）が thread-gate pattern**（v493/v498/v499 に続く thread 系の反復）。
  5. **dtor の bucket 解放 fn（`FUN_007C2A40`）が `param_1[1]`（count）＋`param_1[2]`（bucket）構造を確認**（**v521/v522 の `NiTMapBase` 構造と横断整合**）。
