# v447 — AV 名確定・level getter 同定・dispatcher case0x0B+ の裁定（V-414-1..3 完遂＝旧残点群移行第1弾）

- 採番: **v447**（旧残点群移行・probe prefix **v447_1..v447_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-414-1..3（v414 ruling の残点・旧群）** — 1=AV index9/10/0xC の語の裏取り／2=`FUN_004677F0` の `[+4]>>7` flag と `[0xB333C4]→[+0x32]` 加算の意味／3=V-413-2 引継ぎ（未注記2 fn と dispatcher case0x0B+）を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-414-1: AV index 9/10/0xC の語** | **[確定]（実読）** | **AV name table `.data 0xB0A1A8`（stride4）を全読**: `[0]Strength/[1]Intelligence/[2]Willpower/[3]Agility/[4]Speed/[5]Endurance/[6]Personality/[7]Luck/[8]Health/[9]Magicka/[10]Fatigue/[11]Encumbrance/[12]Armorer/[13]Athletics/[14]Blade/[15]Block/[16]Blunt/[17]HandToHand`…tail `[68]ResistShock/[69]Vampirism/[70]Darkness/[71]ResistWaterDamage`（`[72]=Begin` は keyword table `0xB0A2C8` 開始と完全一致）→ **`TESActorBaseData[+8]`=Magicka（既定50）／`[+0xA]`=Fatigue（既定50）／`[+0xC]`=Armorer（既定0）[確定]**。**v414 の「AV index9/10 [推定高: Health/Magicka 系]」は訂正: Health は index8、index9 は Magicka** |
| **V-414-2: `FUN_004677F0` の identity と内部** | **[確定(decompiled)]** | **`ushort __fastcall FUN_004677F0(int self)` ＝ level getter [確定]**: `local_4 = u16[self+0xE]`（base level・v414 と整合）→ **`if ((u32[self+4] >> 7 & 1) != 0)`（flag bit7 gate）** → `iVar1 = (**vtable[*DAT_00b333C4] + 0x170)()` → **`if (iVar1 != 0 && iVar1 != -0x24) local_4 += s16[iVar1 + 0x32]`（外部オブジェクトの `+0x32 s16` を level へ加算）** → clamp **`u16[self+0x10]`（min: `local_4 < min → return min`）／`u16[self+0x12]`（max: `max < local_4 → return max`）** → **`if (local_4 < 1) local_4 = 1`** → return。**field 語義全部を確定: `[+0xE]`=base level／`[+0x10]`=level min（0=無制限）／`[+0x12]`=level max（0=無制限）[確定]** |
| **V-414-2: `DAT_00b333C4` の identity** | **[確定]** | **Player singleton [確定(decompiled)]**: `L4999: DAT_00b333c4 = (int *)FUN_0066a740()`（＝v412 の Player ctor `0x66A740`）／`L4996: DAT_00b333c4 = 0`（破棄）。**docs mentions 1647件**。virtual calls 多数（`+0x170`（level getter が使用）／`+0x1cc`／`+0x6c`／`+0x284(10)/(8)`／`+0x380`）、fields `[+0x6b0]` counter 加算／`[+0x588]`（＝`DAT[0x162]` int index）byte flag → **`FUN_004677F0` の `+0x170` virtual ＝「Player の virtual fn」で返り値の `+0x32 s16` を level 加算 [確定]**。**`+0x170` fn の identity と `+0x32 s16` の語は [未確定]（残点）** |
| **V-414-2: `u32[self+4] >> 7 & 1` の意味** | **[部分確定]** | `self`＝`TESActorBaseData`（v414 確定の独立オブジェクト・Actor の `+0x24` 埋め込み）→ **`self+4` は同 struct の u32 flags フィールド、bit7＝その flags の第7ビット [確定（構造）]**。**bit7 の semantic（何を表すか）は [未確定]（残点）**（FormType 説は排他: `TESActorBaseData` は独立 object で `+4` は flags） |
| **V-414-3（＝V-413-2）: dispatcher `FUN_00519F50` の case0x0B+** | **[確定(decompiled)]** | `L210353: void FUN_00519f50(undefined4 index, int value)`: **`case 0x0B..0x20, 0x25,0x26,0x27: break;`（no-op・17+3 case）[確定]** → **AV index 11..32 と 37..39 の SET は dispatcher 上 no-op [確定]**／**`case 0x21: FUN_00468180(value)`／`case 0x22: FUN_004681b0(value)`（続行 [確定]**）／case0..7=`FUN_00468af0(id,value)`・8=`FUN_00519c50`・9=`FUN_00467290`（Magicka）・10=`FUN_004672b0`（Fatigue）（v413 確定と整合） |
| **V-414-3: 未注記2 fn** | **[部分確定]** | **backward annotation grep で所属特定**: **`FUN_00511570` の owning annotation＝`FUN_00510e60`（L204640）**／**`0x523F40` の owning＝`FUN_00523eb0`（L217884）** → **両者とも独立 annotation ではなく owning fn 内の sub-case chunk（Ghidra が1つの fn として扱った）[確定]**。`FUN_00511570` machine head 実読: `fldz; [ebp]=0.0(double)` → `virtual [edi]+0x190()` → `call 0x5e02e0` → **`movzx eax,byte[ebx+4]; call 0x447df0`（FormType を第1引数へ）** → **AV 計算系 case chunk [推定高]**（`+0x190` virtual 取得→FormType 判定→処理）。`0x523F40` は v411 の `cmp [esi+0xc],7` gate pattern と同型（v413 記載）で **formType7 ゲート chunk [推定高]**。**formal identity（どの AV か）は [未確定]＝残点** |

## §2 証拠（v447_1/v447_2）

### §2.1 AV table 実読（v447_1a）
```
0xB0A1A8 stride4: [0]Strength [1]Intelligence [2]Willpower [3]Agility [4]Speed
  [5]Endurance [6]Personality [7]Luck [8]Health [9]Magicka [10]Fatigue
  [11]Encumbrance [12]Armorer [13]Athletics [14]Blade [15]Block [16]Blunt [17]HandToHand
  ... [68]ResistShock [69]Vampirism [70]Darkness [71]ResistWaterDamage
  [72]0xB0A2C8='Begin'  <- keyword table start (v433) confirms 72 entries
```

### §2.2 level getter（v447_1b）
```
L81348: ushort __fastcall FUN_004677f0(int param_1) {
  local_4 = *(ushort*)(param_1 + 0xe);
  if ((*(uint*)(param_1 + 4) >> 7 & 1) != 0) {
    iVar1 = (**(code**)(*DAT_00b333c4 + 0x170))();
    if ((iVar1 != 0) && (iVar1 != -0x24)) local_4 += *(short*)(iVar1 + 0x32);
    uVar2 = *(ushort*)(param_1 + 0x10);   // min
    if ((uVar2 && local_4 < uVar2) || (uVar2=[+0x12], uVar2 && uVar2 < local_4)) return uVar2;
    if (local_4 < 1) local_4 = 1;
  }
  return local_4;
}
machine: 4677F4 mov ecx,[esi+4] / shr ecx,7 / test cl,1 / je 467872 (skip all)
         467807 mov ecx,[0xb333c4] / mov edx,[ecx] / mov eax,[edx+0x170] / call eax
         46781B lea ecx,[eax+0x24] / ... / 467827 add ax,[ecx+0xe]  (= +0x32 via +0x24 base)
```
- machine 上は `lea ecx,[eax+0x24]; add ax,[ecx+0xe]` = `eax+0x24+0xE = eax+0x32` ✓（docs の `iVar1+0x32` と一致）

### §2.3 DAT_00b333C4（v447_2a）
```
refs: 402A5D test / 402A82 add [+0x6b0],1 / 4067A6 store (= FUN_0066a740 result?) /
      406866..40687B virtual +0x284 / 407508 cmp byte [+0x588],0 ...
docs 1647 hits: L4996 DAT_00b333c4 = 0; L4999 DAT_00b333c4 = FUN_0066a740();
      L5013 virtual +0x1cc / L5015 +0x6c / L5016 +0x284(10) / L5510 +0x380 ...
      L5509 DAT_00b333c4[0x162] (byte at +0x588)
```

### §2.4 dispatcher / unannotated fns（v447_2b/c）
```
L210353 FUN_00519f50(index, value) switch:
  case 0..7 -> FUN_00468af0(id, value)
  case 8 -> FUN_00519c50 / 9 -> FUN_00467290 (Magicka) / 10 -> FUN_004672b0 (Fatigue)
  case 0x0B..0x20, 0x25, 0x26, 0x27 -> break   (no-op)
  case 0x21 -> FUN_00468180 / 0x22 -> FUN_004681b0 / ...
backward grep: FUN_00511570 owning = FUN_00510e60 @0x510E60 (L204640)
               0x523F40 owning    = FUN_00523eb0 @0x523EB0 (L217884)
FUN_00511570 machine: fldz / [ebp]=0.0 / virtual [edi]+0x190 / call 0x5e02e0 /
                      movzx eax, byte[ebx+4] / call 0x447df0 (FormType arg)
```

## §3 残点 **V-448-1..3**
1. 【低】**`Player+0x170` virtual fn の identity**（level bonus 供給元＝何を返すか、`+0x32 s16` の語）＋ **`TESActorBaseData+4` flags bit7 の semantic**。
2. 【低】**未注記2 fn の case-chunk formal identity** — `FUN_00511570` の所属 switch（`FUN_00510e60` の case 番号）と `0x523F40` の case context（v411 `bVar18` pattern 突合の裏取り）。
3. 【低】**dispatcher `case 0x21+` の全列挙**（`FUN_00468180/81b0/…` と AV name table index 対応・`case 0x21..0x27` の AV 語）。

## §4 検証・自己批評
- probe: **v447_1..v447_2 の2本全 EXIT=0**（V-414-1 完遂（AV 語確定・v414 推定を訂正）／V-414-2 完遂（level getter 全文同定・Player singleton 確定・bit7/`+0x170` は残点）／V-414-3 大部分確定（case0x0B+ no-op 確定・未注記2 fn は owning 特定））。
- **決定打の作り**: (i) **AV 名は table 実読の1本**（`0xB0A1A8` を index で読むだけ・`[72]=Begin` の keyword table 隣接が 72件終端を裏付け）→ (ii) **level getter は decompiled 全文と machine head の対比**で field 語（min/max）を一括確定 → (iii) **`DAT_00b333C4` は docs の代入行（`L4999 = FUN_0066a740()`）で identity 即決**（v412 の Player ctor 名と接続）→ (iv) **未注記 fn は backward annotation grep で「独立 fn ではなく sub-case chunk」だと判明**（欠落ではなく Ghidra の fn 分割）→ (v) **dispatcher は docs の switch 本文を case 順に読むだけ**。
- 妥当性チェック: ①`[72]=Begin`⇔keyword table `0xB0A2C8`（v433）— ②`lea ecx,[eax+0x24]; +0xE`⇔docs `iVar1+0x32` — ③`DAT_00b333c4 = FUN_0066a740()`⇔v412 Player ctor — ④case9/10=`FUN_00467290/B0`⇔`[+8]/[+0xA]`＝Magicka/Fatigue（本裁定の AV 語）— ⑤`FUN_00511570` の owning `FUN_00510e60`⇔annotation L204640 — **五方向矛盾なし**。
- docs との不整合: **なし**（v414 の「Health/Magicka 系 [推定高]」を**本裁定で自己訂正**＝推定が wrong、実読で Magicka/Fatigue/Armorer）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **name table が既知（`0xB0A1A8`・72件）なら index→word は table 実読の1手** — 推測で残さず即読する（v414 の [推定高] を1 probe で解消）。
  2. **getter は decompiled 全文（clamp 構造）と machine head を対比**すると「`[+0x10]`=min／`[+0x12]`=max」の様な語義が同時に確定する（`jbe`/`jg` の向きが clamp 方向を示す）。
  3. **グローバル ptr の identity は「docs の代入行（`= ctor()`）」が最速** — ref 数（1647件）を追う前に代入を探す。
  4. **「未注記 fn」は backward annotation grep で「欠落 vs sub-chunk」を切り分ける** — owning annotation が近い所にあれば欠落ではない（本件2件とも owning あり＝Ghidra fn 分割）。**annotation が本当に無い＝v437 の docs 欠落ケースとは別物**。
  5. **dispatcher の case 本文は docs が100%持っている** — machine decode は不要、`switch` 行から読んでいくだけで完結する。
