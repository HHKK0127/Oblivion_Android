# v496 — `FUN_004CBA80`＝list node lookup（`param_3`＝node bit5 flag）・Player `+0x18` flags の裁定（V-496-1..3 完遂・旧残点 V-406-3/V-406-4 継続第26弾）

- 採番: **v496**（旧残点群消化・probe prefix **v496_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-496-1..3（v495 残点）** — 1=`piVar2`（`FUN_004CBA80` 戻り carrier）と gated 側 mask 源・`FUN_0045A500` callers の `+0x18` 書手／2=bit24 callers identity／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-496-1: `FUN_004CBA80` の carrier（`piVar2`）と `param_3` の意味** | **[確定相当]（両方）** | body L159742-159765 全読 [確定(decompiled)]: **`if (param_2==0) return 0;` → `FUN_00496ea0(param_1)` → `piVar1 = (param_1 + 0x48)`（**`+0x48` list head**）→ do-loop: `if (piVar1==0 || (piVar1[1]==0 && *piVar1==0)) { FUN_00496f50(param_1); return 0; }` → **`piVar2 = *piVar1`（node）→ `iVar3 = virtual[*piVar2 + 0x170]()`（**getter・v480/v488 同 slot**）→ `piVar1 = piVar1[1]`（next）** → **`} while ((iVar3 == 0) \|\| ((param_3 != 0 && ((uint)piVar2[2] >> 5 & 1)) \|\| (iVar3 != param_2)));` → `FUN_00496f50(param_1); return piVar2;` [確定(構造)]** → **`FUN_004CBA80`＝「`param_1+0x48` list を走査し、各 node の `virtual+0x170()` 返り値が `param_2`（key・例: NorthMarker ptr）と一致する node を返す lookup fn [確定相当]`**、**`param_3`（v492/v494 で `1`）＝「`piVar2[2]>>5 & 1`（**node flag bit5**）を追加条件に含めるかの flag [確定相当]** → **v492/v494「arg3=1」の意味が確定（node bit5 条件付き lookup）**。**carrier `piVar2`＝list node [確定]** → **`[ret+0x28]`（v494 `fld`）＝その node の `+0x28` float で、**form `+0x28`（v482/v485 3float 群）とは**別载体 [確定相当]**（v495 残点2 が確定） |
| **V-496-1: `FUN_0045A500` callers の `+0x18` 書手（3件調査）** | **[確定相当]（Player `+0x18` flags）** | **`FUN_0040D4D0`（L8563/8565）: `*(DAT_00b33b00 + 0x18) = *(DAT_00b33b00 + 0x18) & 0xffffdfff`（**bit13 clear**）` / `& 0xffff7fff`（**bit15 clear**）[確定(decompiled)]** → **`DAT_00B33B00`＝Player singleton（v464/v480 既知）の `+0x18` field が「flag 群 u32（bit13/15 …）」で `FUN_0045A500` はその読取（bit18/下位 byte・v493）[確定相当]**。`FUN_0041FA40`/`FUN_004246F0` は body window に `+0x18` write なし（**他は間接経由 [部分確定]**）→ **`param_1+0x18` ＝ Player flags と特定 [確定相当]**（v489 bit5（`+0x8`）とは**別 field**である点を区別） |
| **V-496-2: bit24（`0x1000000`）caller（`FUN_0045CEE0`）identity** | **[未確定]（残点継続）** | signature **`void __fastcall FUN_0045cee0(int param_1)`（L73381）・large fn（locals 多）[確定(decompiled)]**、本文は本 probe 未到達＝残点 |
| **V-496-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味 |

## §2 証拠（v496_1）

### §2.1 `FUN_004CBA80` full body（v496_1a）
```
L159742: int * __thiscall FUN_004cba80(int param_1, int param_2, char param_3) {
L159749:   if (param_2 == 0) { return 0; }
L159752:   FUN_00496ea0(param_1);                       // lock/prepare
L159753:   piVar1 = (int *)(param_1 + 0x48);            // +0x48 list head
L159755:   if (piVar1==0 || (piVar1[1]==0 && *piVar1==0)) { FUN_00496f50(param_1); return 0; }
L159759:   piVar2 = (int *)*piVar1;                     // node
L159760:   iVar3 = (**(code **)(*piVar2 + 0x170))();    // node getter (slot +0x170)
L159761:   piVar1 = (int *)piVar1[1];                   // next
L159763:   while ((iVar3 == 0) || ((param_3 != 0 && ((uint)piVar2[2] >> 5 & 1)) || (iVar3 != param_2)));
L159765:   return piVar2;                               // -> node carrier
```

### §2.2 `FUN_0040D4D0` Player `+0x18` writes（v496_1b）
```
L8563: *(DAT_00b33b00 + 0x18) &= 0xffffdfff   (clear bit13)
L8565: *(DAT_00b33b00 + 0x18) &= 0xffff7fff   (clear bit15)
FUN_0041FA40 / FUN_004246F0: no +0x18 write in body window  [部分確定]
```

### §2.3 `FUN_0045CEE0`（v496_1c）
```
L73381: void __fastcall FUN_0045cee0(int param_1) { ... (large fn, unread body)
```

## §3 残点 **V-497-1..3**
1. 【中】**bit24（`0x1000000`）の semantic**（`FUN_0045CEE0`/`FUN_0045D030` 本文）＋ **`param_1+0x18` flag 群の bit 一覧**（bit13/15/18 ほか・Player `+0x18` の set 書手）。
2. 【低】**gated 側 mask 値源（`edx` 設定元・`FUN_0045B670` caller 上流）** ＋ `FUN_00496EA0`/`FUN_00496F50`（lock/prepare と release）identity。
3. 【低】**引継ぎ残り** — predicate slot35 英名、sink `DAT_00b34d90` 正体、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v496_1 の1本 EXIT=0**（V-496-1 carrier＝node と `param_3`＝node bit5 flag を確定相当・Player `+0x18` を確定相当／V-496-2 bit24 caller は large fn で残点／V-496-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_004CBA80` body 全読（42行）**で「list walk＋`virtual+0x170`＝key 比較＋`param_3`＝node bit5 条件」が一気に判明 → **v492/v494 の arg3=1 と v494 の `[ret+0x28]` が同時に確定** → (ii) **caller の `+0x18` write grep**で Player singleton（`0xB33B00`）と bit13/15 clear を特定 → field の carrier（Player）確定 → (iii) bit24 caller は large fn と自覚して残点化。
- 妥当性チェック: ①`piVar2[2]>>5&1`⇔**node flag bit5**（v489 とは別 field：`piVar2` は node）— ②`param_1+0x48`⇔v471/v479 list head（同一 field 横断）— ③`virtual+0x170`⇔v480 getter（node 側・`iVar3 != param_2`＝key 比較）— ④`DAT_00b33b00`＝Player⇔v464/v480 既知 — ⑤`+0x18`（Player）と `+0x8`（class form）が別 field である区別 — **五方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **lookup fn の body 全読が「引数3つの意味」を同時確定させる**（`param_2`＝key・`param_3`＝node flag 条件・return＝node carrier）＝ **v492/v494 の残点3つを1本で解消**。
  2. **`>>5 & 1` の寄り先（`piVar2[2]`＝node vs `param_1+0x18`＝Player）で同 bit 番号でも別 field** — **どの object の bit かを明記**（v489 `+0x8`/v496 `+0x18`/node `[2]` の区別）。
  3. **`&= 0xffffdfff/7fff`（clear）の constants から bit 位置を即特定**（bit13/bit15）。
  4. **large fn（locals 多）は signature までで残点化する習慣**（`FUN_0045CEE0`）。
  5. **`[ret+0x28]` の载体（node/form）問題が解決** — **同じ offset でも carrier が別物**（node `+0x28` vs form `+0x28`）という**offset 同一・载体別**の典型を記録。
