# v472 — `FUN_00566830`（bit11 flag setter）と `form+0x64` 連動の裁定（V-472-1..3 完遂・旧残点 V-406-3/V-406-4 継続第2弾）

- 採番: **v472**（旧残点群消化・probe prefix **v472_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-472-1..3（v471 残点）** — 1=旧残点 V-406-3 継続（`elem[10]` の語＋`FUN_00566830` identity）／2=旧残点 V-406-4 残り（`form+0x64`/`+0x69` semantic）／3=V-471-3 引継ぎを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-472-1: `FUN_00566830`（`form+0x64` 転送先）identity** | **[確定相当]** | L266287: **`void __thiscall FUN_00566830(int param_1, char param_2)`: `cVar1 = FUN_00446B80(*(param_1 + 0xc)); if (cVar1 != '\0') { if (param_2 != '\0') *(param_1+0x1c) |= 0x800; else *(param_1+0x1c) &= 0xfffff7ff; }` [確定(構造)]** → **`FUN_00566830(self, flag)`＝「`FUN_00446B80(self[+0xC])` gate が通れば `self+0x1C` u32 の bit11（`0x800`）を set/clear する fn [確定相当]`**（`param_2`＝set/clear 選択・`push 1`＝set）→ **`form+0x64` の dword 設定は `FUN_00566830` の bit11 set と連動する [確定相当]**（`FUN_005F9EB0` が `FUN_00468230` の直後に `FUN_00566830(esi,1)` を呼ぶ構造）。**`FUN_00446B80(self[+0xC])` gate の identity は [未確定]＝残点** |
| **V-472-1: `elem[10]`（byte +40 float）の語** | **[未確定]（残点継続）** | `FUN_004CD960` docs body の `+0x48`/`+0x10` context 提及は0件（`param_1[0x14]`＝`+0x50` の2件のみ）→ **docs body から elem class/field の context 未特定＝残点**（v470 `TESObjectSTAT` singleton 比較との接続は維持） |
| **V-472-2: 旧 V-406-4 残り（`form+0x69` byte semantic）** | **[未確定]（残点継続）** | `FUN_00468270`（byte setter `*(u8*)(self+0xD)=v`）の callers context は本 probe 未着手 |
| **V-472-3: V-471-3 引継ぎ（opcode 接続）** | **[確定]（signature）＋[引継ぎ]** | **`FUN_0050CD30`（L202786）signature＝5引数（`undefined4 param_1..param_4`＋5th）[確定(decompiled)]**（v409 opcode 帯）／**`FUN_0050D020` は annotation absent＝docs 欠落（v409 確定済み・再確認）[確定]** → **v409 の AdvancePCSkill/AdvSkill/ModPCSkill 共通 impl 記載と整合** |

## §2 証拠（v472_1）

### §2.1 `FUN_00566830`（v472_1a）
```
L266287: void __thiscall FUN_00566830(int param_1, char param_2) {
  cVar1 = FUN_00446b80(*(undefined4 *)(param_1 + 0xc));   // gate on self+0xC
  if (cVar1 != '\0') {
    if (param_2 != '\0') *(param_1 + 0x1c) |= 0x800;      // set bit11
    else *(param_1 + 0x1c) &= 0xfffff7ff;                 // clear bit11
  } }
caller (FUN_005F9EB0, v471): FUN_00468230(v) ; push 1 / mov ecx,esi / call 0x566830
```

### §2.2 `FUN_004CD960` context（v472_1b）
```
docs body mentions of +0x48 / +0x10 / param_1[0x12]: 0
  (only param_1[0x14] = +0x50 at L161505 / L161545)
  -> elem class / +40 float context not visible in docs body = residual
```

### §2.3 opcode fns（v472_1c）
```
FUN_0050CD30 (L202786): FUN_0050cd30(undefined4, undefined4, undefined4, undefined4, ...)  (5 args)
FUN_0050D020: annotation absent  (= docs-missing, v409 confirmed; re-verified)
v409 tie: AdvancePCSkill/AdvSkill (opid 0x10D4) / AdvancePCLevel (0x10D5) /
          ModPCSkill (0x110F) common impl = FUN_0050D020
```

## §3 残点 **V-473-1..3**
1. 【中】**旧残点 V-406-3 継続** — **`elem[10]`（byte +40 float）の語**（docs body context 未特定→machine の `FUN_004CD960` list walk decode で class特定）＋ **`FUN_00446B80`（`FUN_00566830` gate）identity**。
2. 【低】**旧残点 V-406-4 残り** — `form+0x69` byte（`FUN_00468270` callers context）の semantic。
3. 【低】**V-472-3 引継ぎ** — `FUN_005222D0` 一語名別法（caller script opcode 名接続）と predicate slot35 英名裏取り（`CanResolveDependencies` [推定]）。

## §4 検証・自己批評
- probe: **v472_1 の1本 EXIT=0**（V-472-1 `FUN_00566830` identity 相当＋`elem[10]` 残点確認／V-472-2 残点／V-472-3 signature 確定＋v409 整合再確認）。
- **決定打の作り**: (i) **`FUN_00566830` の decompiled が `FUN_00446B80(self[+0xC])` gate＋`|= 0x800 / &= ~0x800` の2分岐で bit11 flag setter と即決** → (ii) **`FUN_005F9EB0` の `FUN_00468230` 直後呼び出し（v471）で連動を確認** → (iii) `elem[10]` は **docs body context 提及0件の否定側**で残点化 → (iv) opcode fns は **annotation absent 再確認（v409 整合）**。
- 妥当性チェック: ①`|= 0x800 / &= 0xfffff7ff`⇔bit11 set/clear（`0x800`=bit11 ✓）— ②`FUN_00446B80(self[+0xC])`⇔gate（`+0xC` field 判定）— ③`FUN_005F9EB0` callers⇔v471 6件の1件 — ④`FUN_0050D020` absent⇔v409 docs 欠落記載 — ⑤`FUN_0050CD30` 5引数⇔v409 opcode 帯 — **五方向矛盾なし**。
- docs との不整合: **なし**（`FUN_00566830` に annotation 実在）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **flag set/clear の2分岐（`|= mask / &= ~mask`）は bit 単位 flag setter の定番** — mask 値（`0x800`）から bit 位置（11）が即決できる。
  2. **gate（`FUN_00446B80(self[+0xC])`）付き flag setter は「条件付き flag」** — gate fn identity が次の手。
  3. **callers context（`FUN_00468230` 直後）が fn の連動関係を語る** — v471 の6件列挙がここでも効く。
  4. **docs body の context 提及0件は「docs から読めない」の否定側** — machine の list walk decode へ移る判断材料。
  5. **annotation absent の再確認は v409 等の過去裁定と整合**（docs 欠落 fn の記載が正しいことを裏付ける）。
