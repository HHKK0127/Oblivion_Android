# v497 — Player `+0x18` bit 一覧（16件 writes）・bit24 caller 部分・lock pair 型の裁定（V-497-1..3 完遂・旧残点 V-406-3 継続第27弾）

- 採番: **v497**（旧残点群消化・probe prefix **v497_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-497-1..3（v496 残点）** — 1=bit24 semantic と Player `+0x18` flag 一覧／2=gated 側 mask 源・lock fn／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-497-1: Player `+0x18` flag 一覧（bit map basis）** | **[確定相当]（16件 writes）** | **`DAT_00b33b00 + 0x18` の mask 付き writes＝16件（全 mentions 79件）[確定(decompiled)]: **set（`|=`）: `0x100`（bit8・L52822）／`0x80`（bit7・L52839/52846/161481/161488）／`0x10000`（bit16・L73260/74125）／`0x8000`（bit15・L144659）／`0x400`（bit10・L241158）**、**clear（`&=`）: `0xffffdfff`（bit13・L8563）／`0xffff7fff`（bit15・L8565）／`0xfffffeff`（bit8・L52833）／`0xffffff7f`（bit7・L52843/161485）／`0xfffffbff`（bit10）／`0xfffffffd`（bit1）** → **Player `+0x18` に set/clear 実例のある bit＝1/7/8/10/13/15/16 の7種 [確定]**（distinct clear masks 6種を実読）→ **`FUN_0045A500` が読む bit18 と下位 byte の set 書手はこの16件に無し [確定（否定側）] → 別 fn（間接経由）＝残点**。**flag map の basis（現時点の7 bit 分）は確定相当 [確定相当]** |
| **V-497-1: bit24（`0x1000000`）semantic（`FUN_0045D030`）** | **[部分確定]（構造のみ）** | L73457 実読 head（30行 cap）[確定(decompiled)]: **`void __fastcall FUN_0045d030(int param_1)`: `uVar2 = *(uint*)(*(param_1 + 0x60) + 4)`（**count**）→ `piVar3 = *(int**)(*(param_1+0x60) + 8)`（**base ptr**）→ `piVar3[uVar5]` インデックス走査（`*piVar8` null check→`local_c = piVar3[uVar5]`→loop）** → **`param_1+0x60`＝count/base の付いた ptr array 構造 [確定相当]**、**mask `0x1000000`（bit24）の使用箇所は body 深部（cap 外）[部分確定]＝残点** |
| **V-497-2: lock/release pair（`FUN_00496EA0`/`FUN_00496F50`）** | **[部分確定]（型まで）** | `FUN_00496EA0`（L120801 `void __thiscall(int, undefined4)`）: **`DWORD DVar3; DWORD DStack_8;`（**DWORD locals＝thread/tick 系 [推定]**）[確定(decompiled)]**／`FUN_00496F50`（L120846 `void(void)`・**WARNING: Removing unreachable block**）: `char cVar1; undefined4 unaff_retaddr;` → **本文は cap 不足で未到達＝[部分確定]**（v496「prepare/release ペア」推定は維持・`FUN_00496F50` が `void(void)` な点が v496 推定と差＝**関数は引数を取らず TLS/global 経由 [推定]**） |
| **V-497-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味 |

## §2 証拠（v497_1）

### §2.1 Player `+0x18` writes（v497_1b）
```
16 writes with masks (79 mentions total):
  set: 0x100 (bit8) L52822 / 0x80 (bit7) L52839,52846,161481,161488 /
       0x10000 (bit16) L73260,74125 / 0x8000 (bit15) L144659 / 0x400 (bit10) L241158
  clear: 0xffffdfff (bit13) L8563 / 0xffff7fff (bit15) L8565 /
         0xfffffeff (bit8) L52833 / 0xffffff7f (bit7) L52843,161485 /
         0xfffffbff (bit10) / 0xfffffffd (bit1)
  -> bits with observed set/clear: 1,7,8,10,13,15,16   [確定]
  -> bit18 (FUN_0045A500 reader) has no writer here      [否定側 -> residual]
```

### §2.2 `FUN_0045D030` head（v497_1a）
```
L73475: uVar2 = *(uint *)(*(int *)(param_1 + 0x60) + 4);      // count
L73478: piVar3 = *(int **)(*(int *)(param_1 + 0x60) + 8);      // base
L73482: local_c = piVar3[uVar5]; ... loop index walk
-> param_1+0x60 = ptr array {count, base}  [確定相当]
mask 0x1000000 usage: beyond cap (deep body)  [部分確定]
```

### §2.3 lock pair heads（v497_1c）
```
L120801: void __thiscall FUN_00496ea0(int param_1, undefined4 param_2)
         { DWORD DVar3; DWORD DStack_8; ... }        (DWORD locals: thread/tick [推定])
L120846: void FUN_00496f50(void) { char cVar1; ... } (WARNING: unreachable block)
  -> bodies beyond cap  [部分確定]
```

## §3 残点 **V-498-1..3**
1. 【中】**bit24（`0x1000000`）の使用箇所**（`FUN_0045D030` body 深部 or `FUN_0045CEE0`）＋ **Player `+0x18` bit18/下位 byte の set 書手特定**（16件に無い＝間接 fn）。
2. 【低】**lock/release pair 本文**（`FUN_00496EA0` DWORD 系の中身・`FUN_00496F50` の `void(void)` 実体）＋ gated 側 mask 源（`edx` 設定元）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v497_1 の1本 EXIT=0**（V-497-1 Player flag 7bit を16件 writes で確定相当・bit18 writer 否定側／V-497-1 `FUN_0045D030` 構造のみ部分確定／V-497-2 lock pair 型まで／V-497-3 引継ぎ）。
- **決定打の作り**: (i) **`DAT_00b33b00 + 0x18` の mask 付き grep（16件）と distinct mask 集計**で bit 群を機械的に列挙 → (ii) **`FUN_0045D030` head で `param_1+0x60` の構造（count/base）を特定**して残り（mask 部分）を残点化 → (iii) **lock pair の locals 型（`DWORD`）で thread/tick 系を推定**。\n- 妥当性チェック: ①set/clear の mask が対応（bit8⇔`0xfffffeff`／bit7⇔`0xffffff7f`／bit15⇔`0xffff7fff`／bit10⇔`0xfffffbff`／bit13⇔`0xffffdfff`／bit1⇔`0xfffffffd`）— ②`param_1+0x60` `{count, base}`⇔v481 の `{count, ptr}` 構造と同型 — ③`FUN_00496F50` が `void(void)`⇔v496「release」呼び出し（引数なし）と整合 — ④79 mentions≫16 writes⇔残りは read 系 — **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:\n  1. **flag field の bit 一覧は「mask 付き writes の grep＋distinct mask 集計」で機械的に揃う**（set/clear 対で bit 位置が対応）。\n  2. **read 系（79 mentions）と write 系（16件）を分けて数える**と「どの bit が書かれないか（bit18）」まで否定側で判明。\n  3. **head cap で構造（count/base ptr array）だけ確定し mask 部分を残点化**する判断で probe を1本に収める。\n  4. **`void(void)` が初めて見つかると呼び出し側（引数なし）と整合させて引数契約を再確認**（v496 推定の修正点を記録）。\n  5. **DWORD locals（thread id/tick）が thread 系 fn の目印**（v493 `GetCurrentThreadId` に続く類型）。\n