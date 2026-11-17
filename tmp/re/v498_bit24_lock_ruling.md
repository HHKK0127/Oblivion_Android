# v498 — bit24 mask＝`FUN_00452C20` への固定 mask・lock acquire 本文・bit18 writer 否定側の裁定（V-498-1..3 完遂・旧残点 V-406-3 継続第28弾）

- 採番: **v498**（旧残点群消化・probe prefix **v498_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-498-1..3（v497 残点）** — 1=bit24 semantic と Player `+0x18` bit18/下位 byte の set 書手／2=lock/release pair 本文＋gated 側 mask 源／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-498-1: bit24（`0x1000000`）mask の使用箇所** | **[確定相当]** | **`FUN_0045D030` body（L73455-73532）に `0x1000000` は1件、`FUN_0045CEE0` body（L73379-73454）にも1件、**両方とも同型 [確定(decompiled)]**: **`L73525: FUN_00452c20(iVar7, 0x1000000);`（`FUN_0045D030`）／`L73447: FUN_00452c20(iVar7, 0x1000000);`（`FUN_0045CEE0`）**、前後は `FUN_004211e0(local_14)`/`FUN_004210e0(local_10)` → **`FUN_00452C20`** → `FUN_004524c0(uVar9)` → `while(true)` loop [確定(構造)] → **bit24（`0x1000000`）は `FUN_00452C20`（v493 lazy node `|=`）へ渡す固定 mask [確定相当]**（v495 で caller push census から得た mask と本文で完全一致）。→ **`iVar7`（`param_1+0x60` list の要素）の lazy node に bit24 を OR 設定する loop [確定相当]**（bit24 の最終 semantic＝「iVar7 要素の flag bit24」[推定]＝node 用途次第で残点） |
| **V-498-1: Player `+0x18` bit18/下位 byte の set 書手** | **[確定]（否定側）＋[部分確定]（新 bit6 read を獲得）** | **残り callers 4件（`FUN_00424C50`/`FUN_00424D00`/`FUN_0042A200`/`FUN_00434020`）の body に `+0x18` write 0件 [確定(否定側)]** → `FUN_0045A500` の callers 8件すべてで直接 write なし（v497 と合わせて **8/8 否定側**）→ **bit18/下位 byte の set 書手は間接経由（virtual/間接 store）＝本 grep では特定不能 [確定（否定側）]**。**副次収穫: `L176200`（`FUN_004DA4F0` 系 band）`if ((*(uint*)(DAT_00b33b00 + 0x18) >> 6 & 1) != 0) && ((param_2 & 0x40000000) != 0)`＝**bit6 read 実例を1件発見 [確定(decompiled)]** → Player `+0x18` の観測 bit が 1/6/7/8/10/13/15/16 の8種に拡大（read 側）[確定相当] |
| **V-498-2: lock pair 本文（`FUN_00496EA0`）** | **[確定相当]（リカーシブル acquire）** | L120801-120838 全読 [確定(decompiled)]: **`void __thiscall FUN_00496ea0(int param_1, undefined4 param_2)`: `do { FUN_00401020(&DAT_00a2f830)`（**enter critical/lock global**）→ `DStack_8=0; iStack_4=0; cVar2=FUN_00496df0(param_2, &DStack_8)`（**key（`param_2`）で owner/count lookup**）→ `if (cVar2==0) { DVar3=GetCurrentThreadId(); iVar4=1; LAB: FUN_00496d50(param_2, DVar3, iVar4); bVar1=false; }`（**未取得なら owner=thread, count=1 で acquire**）`else { DVar3=GetCurrentThreadId(); if (DStack_8==DVar3) { iVar4=iStack_4+1; ... } }`（**同 thread 再取得なら count++＝再帰**）→ `FUN_00401040()`（**leave critical**）→ `if (!bVar1) return;` → **`FUN_00498ee0(*(param_1+0x100), 1)`（**event/wait 唤起 [推定]**）** `} while(true);` [確定(構造)]** → **`FUN_00496EA0`＝「`DAT_00a2f830` critical section の内側で、key（`param_2`）の owner thread と recursion count を管理する**リカーシブル lock acquire fn**（未取得→acquire count=1／同 thread→count++／他 thread→`FUN_00498ee0` wait 候補）[確定相当]`** → **v496「prepare」推定を「リカーシブル lock acquire」に更新・追補**。**`FUN_00496F50`（release 側）は v497 で型（`void(void)`）まで・本文は本 probe 未読＝残点** |
| **V-498-2: gated 側 mask 源（`edx`）** | **[未確定]（残点継続）** | 本 probe 未着手（budget は bit24/lock に集中） |
| **V-498-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味 |

## §2 証拠（v498_1）

### §2.1 bit24 mask（v498_1a/a2）
```
FUN_0045D030 body L73455..L73532:
  L73524: FUN_004211e0(local_14);
  L73525: FUN_00452c20(iVar7, 0x1000000);     <- mask 0x1000000 (bit24)
  L73527: FUN_004524c0(uVar9);
  L73528: } while (true);
FUN_0045CEE0 body L73379..L73454:
  L73446: FUN_004210e0(local_10);
  L73447: FUN_00452c20(iVar7, 0x1000000);     <- same shape
  L73449: FUN_004524c0(uVar9);
  L73450: } while (true);
-> bit24 = fixed mask passed to FUN_00452C20 (lazy node |=)  [確定相当]
```

### §2.2 Player `+0x18` bit18 writer（v498_1b/b2）
```
direct bit18/0xffff0000-mask hits: 1 (read, not write)
  L176200: if ((*(uint *)(DAT_00b33b00 + 0x18) >> 6 & 1) != 0) && ((param_2 & 0x40000000) != 0)
  -> bit6 READ instance found  [確定]
remaining FUN_0045A500 callers +0x18 writes:
  FUN_00424C50 / FUN_00424D00 / FUN_0042A200 / FUN_00434020: all 0  [否定側]
-> 8/8 callers have no direct +0x18 write -> bit18 writer is indirect  [否定側]
observed bits now: 1,6,7,8,10,13,15,16
```

### §2.3 `FUN_00496EA0` body（v498_1c）
```
L120801: void __thiscall FUN_00496ea0(int param_1, undefined4 param_2) {
L120813:   FUN_00401020(&DAT_00a2f830);          // enter critical
L120816:   cVar2 = FUN_00496df0(param_2, &DStack_8);   // owner/count lookup
L120817:   if (cVar2 == 0) { DVar3 = GetCurrentThreadId(); iVar4 = 1;
L120821:     FUN_00496d50(param_2, DVar3, iVar4);       // acquire (thread, count=1)
L120822:     bVar1 = false; }
L120825:   else { DVar3 = GetCurrentThreadId();
L120826:     if (DStack_8 == DVar3) { iVar4 = iStack_4 + 1; ... }  // same thread: count++
L120832:   FUN_00401040();                              // leave critical
L120836:   FUN_00498ee0(*(param_1 + 0x100), 1);         // wake/wait [推定]
L120837: } while (true);
=> recursive lock acquire with owner thread + count  [確定相当]
```

## §3 残点 **V-499-1..3**
1. 【中】**Player `+0x18` bit18/下位 byte の set 書手**（8/8 callers 否定側＝**間接 store（virtual/`mov [reg]`）経由の探索へ転換**）＋ **bit24 の最終 semantic**（`iVar7`（`param_1+0x60` list 要素）の lazy node が何の flag 群か）。
2. 【低】**`FUN_00496F50`（release 側）本文**（`void(void)` の実体）＋ gated 側 mask 源（`edx` 設定元）＋ `FUN_00496DF0`/`FUN_00496D50`（lookup/acquire sub）identity。
3. 【低】**引継ぎ残り** — predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v498_1 の1本 EXIT=0**（V-498-1 bit24 mask を2 body で確定相当・bit18 writer は8/8 否定側＋bit6 read 獲得／V-498-2 `FUN_00496EA0` 本文完読でリカーシブル acquire 確定相当／V-498-3 引継ぎ）。
- **決定打の作り**: (i) **`0x1000000` を両 caller body 内 grep** → 両方とも `FUN_00452c20(iVar7, 0x1000000)` の1行に一致 → **mask の行き先（v493 lazy node）が本文で確定**（v495 caller push census との双向一致）→ (ii) **残り callers 4件の `+0x18` write grep 0件**で8/8 否定側を完遂、**`>>6 & 1` read 1件を副次発見** → (iii) **`FUN_00496EA0` 本文55行を全読** → `GetCurrentThreadId` 比較＋count++＋`FUN_00496d50` acquire の連鎖でリカーシブル lock と即決。
- 妥当性チェック: ①`FUN_00452c20(iVar7, 0x1000000)`⇔v495 push census `pushes=['eax','0x1000000','edi']`（mask=param_2 と一致）— ②`FUN_00496df0(param_2, &DStack_8)`⇔owner/count の2値取得（`DStack_8`=owner thread、`iStack_4`=count）— ③`DStack_8==DVar3`⇔同 thread 判定（v493 `FUN_0045A500` の GetCurrentThreadId 比較と同型）— ④`FUN_00401020`/`FUN_00401040`⇔enter/leave critical（対）— ⑤bit6 read⇔mask 群（1/6/7/8/10/13/15/16）— **五方向矛盾なし**。
- docs との不整合: **①v496 ruling「`FUN_00496EA0`＝prepare（DWORD locals で thread/tick 系 [推定]）」→ 本文で「リカーシブル lock acquire（owner thread＋count）」に更新・追補（推定の精緻化）**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **mask の semantic は「その mask を渡す fn の本文」で決まる** — `0x1000000` が `FUN_00452C20`（`|=`）へ渡ることで「bit24＝lazy node flag の1つ」と確定（**caller push census（v495）と callee body（v498）の双向突合**）。
  2. **同じ mask 呼び出しが複数 fn に同型で現れる（2 body で一致）＝共通パターン** — 一方を特定すれば他方も同型と即分かる。
  3. **write grep の8/8 否定側は「間接 store 経由」への転換点** — 次は `mov [reg+0x18], …` の machine 側探索へ（docs grep の限界を明記）。
  4. **リカーシブル lock の定番（critical enter→lookup owner/count→同 thread なら count++／異 thread なら acquire→leave→wait 候補）**が本文で判明 — **`GetCurrentThreadId` 比較＋count 整数の同時存在が目印**（v493 thread gate に続く類型）。
  5. **read 実例（`>>6 & 1`）の副次発見**で flag map が read/write 両面で拡大する（**grep を1つの mask 条件に絞らず、別 bit read も拾う**）。
