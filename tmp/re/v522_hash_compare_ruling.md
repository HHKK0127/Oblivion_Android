# v522 — registry hash＝modulo・compare＝equality・`FUN_00416000`＝TESForm cast の裁定（V-522-1..3 完遂・旧残点 V-406-3 継続第52弾）

- 採番: **v522**（旧残点群消化・probe prefix **v522_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-522-1..3（v521 残点）** — 1=hash fn（`FUN_006A9060`）／compare fn（`FUN_00763E80`）本文と `FUN_0046B030` identity／2=allocator の残り2 caller（`FUN_00416000`/`FUN_0045C7A0`）identity と caller 分離／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-522-1: `FUN_006A9060`（NiTMapBase hash fn・slot `+4`）本文** | **[確定]（modulo hash）** | L475136 実読 [確定(decompiled)]: **`uint __thiscall FUN_006a9060(int param_1, uint param_2) { return param_2 % *(uint *)(param_1 + 4); }`** → **hash＝`key % bucket_count`（`param_1+4`＝bucket 数）の modulo hash [確定]** → **registry（`NiTMapBase<…, TESForm*>`）の hash は form id（uint）を bucket 数で割る modulo [確定相当]**（v519 allocator の `virtual[*v+4](iVar5)`＝form id の modulo hash） |
| **V-522-1: `FUN_00763E80`（NiTMapBase compare fn・slot `+8`）本文** | **[確定]（equality compare）** | L607560 実読 [確定(decompiled)]: **`bool FUN_00763e80(int param_1, int param_2) { return param_1 == param_2; }`** → **compare＝`==`（equality）[確定]** → **registry の key 比較＝form id（uint）の直接一致判定 [確定相当]**（v519 allocator の `virtual[+8](iVar5, puVar3[1])`＝form id equality）→ **`form id が key`であることが hash/compare 両面で確定** |
| **V-522-2: `FUN_00416000`（allocator caller 1）identity** | **[部分確定]（TESForm cast＋`FUN_0046ACB0`）** | L14522 実読 [確定(decompiled)]: **`void __thiscall FUN_00416000(int param_1, undefined4 param_2)`: `iVar1 = FUN_009832e6(param_2, 0, &PTR_PTR_00b02f9c, &PTR_PTR_00b03534, 0)`（**RTDynamicCast: TESForm（`0xB02F9C`）→ `0xB03534` type**）→ `if (iVar1 != 0) { FUN_0046acb0(param_2); ... }`** → **`FUN_00416000`＝「`param_2` を TESForm→`0xB03534` type に cast し、成功したら `FUN_0046ACB0(param_2)` を呼ぶ fn [部分確定]」**（**`0xB03534` type identity は RTTI 未読＝残点**、`FUN_0046ACB0` identity は残点）→ **allocator（id counter）を使う cast 経路 [推定]** |
| **V-522-2: `FUN_0045C7A0`（allocator caller 2）identity** | **[部分確定]（DWORD locals・本文 cap 外）** | L73041 実読 [確定(decompiled)]: **`void __thiscall FUN_0045c7a0(int param_1, int *param_2)`: locals＝`DWORD DVar2; byte bVar3; DWORD DVar4; int iVar5; ...`（**DWORD locals＝thread/tick 系 [推定]**）→ 本文は cap 外＝[部分確定]**（`param_2` が `int*`＝ptr object [確定相当]） |
| **V-522-1: `FUN_0046B030`（`NiTMapBase` ctor/dtor 候補）** | **[未確定]（残点継続）** | 本 probe 未着手 |
| **V-522-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v522_1）

### §2.1 hash fn（v522_1a）
```
L475136: uint __thiscall FUN_006a9060(int param_1, uint param_2) {
L475139:   return param_2 % *(uint *)(param_1 + 4); }   // modulo by bucket count
=> NiTMapBase hash = key % bucket_count  [確定]
   registry hash = form_id % bucket_count  [確定相当]
```

### §2.2 compare fn（v522_1b）
```
L607560: bool FUN_00763e80(int param_1, int param_2) {
L607563:   return param_1 == param_2; }                 // equality
=> NiTMapBase compare = ==  [確定]
   registry key compare = form id equality  [確定相当]
   => form id IS the registry key (both faces: hash uint + compare ==)
```

### §2.3 allocator callers（v522_1c）
```
L14522: void __thiscall FUN_00416000(int param_1, undefined4 param_2) {
L14529:   iVar1 = FUN_009832e6(param_2, 0, &PTR_PTR_00b02f9c, &PTR_PTR_00b03534, 0);
           // RTDynamicCast: TESForm -> 0xB03534 type  (0xB03534 = residual)
L14530:   if (iVar1 != 0) { FUN_0046acb0(param_2); ... }   // FUN_0046ACB0 = residual

L73041: void __thiscall FUN_0045c7a0(int param_1, int *param_2) {
L73045:   DWORD DVar2; byte bVar3; DWORD DVar4; int iVar5; ...   // DWORD locals
  -> body beyond cap  [部分確定]
```

## §3 残点 **V-523-1..3**
1. 【中】**`PTR_PTR_00b03534` RTTI type**（`FUN_00416000` の cast 先）＋ **`FUN_0046ACB0`（cast 成功後の呼出し）identity** ＋ `FUN_0046B030`（`NiTMapBase` ctor/dtor）identity。
2. 【低】**`FUN_0045C7A0` 本文**（DWORD locals の中身）＋ caller 分離（経路分離＝残点）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v522_1 の1本 EXIT=0**（V-522-1 hash＝modulo・compare＝equality を本文で確定・registry key＝form id を両面で確定／V-522-2 `FUN_00416000`＝TESForm cast＋`FUN_0046ACB0` を部分確定・`FUN_0045C7A0` は DWORD locals で部分確定／V-522-3 引継ぎ）。
- **決定打の作り**: (i) **`FUN_006A9060` docs 4行** → `param_2 % *(param_1+4)` で modulo hash と即決 → (ii) **`FUN_00763E80` docs 3行** → `param_1 == param_2` で equality compare と即決 → **form id が key と hash/compare 両面で確定** → (iii) **`FUN_00416000` head** で `FUN_009832E6` cast（TESForm→`0xB03534`）と `FUN_0046ACB0` を獲得 → (iv) **`FUN_0045C7A0` head** で DWORD locals を確認。
- 妥当性チェック: ①`param_2 % *(param_1+4)`⇔v519 allocator の `virtual[*v+4](iVar5)`（hash が form id を受け取る）— ②`param_1 == param_2`⇔v519 allocator の `virtual[+8](iVar5, puVar3[1])`（equality 比較）— ③`FUN_009832E6(param_2, 0, &TESForm, &0xB03534, 0)`⇔v501/v502 RTDynamicCast wrapper（5度目）— ④DWORD locals⇔v493/v497 thread 系 pattern — ⑤`FUN_0046ACB0`⇔cast 成功後処理（`iVar1 != 0` gate）— **五方向矛盾なし**。
- docs との不整合: **なし**（hash/compare/cast とも docs と一致）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **container の hash/compare 実体は vtable slot（`+4`/`+8`）の fn docs を読めば1行で判明**（**`%`＝modulo、`==`＝equality が hash/compare の定番**＝v521 で slot特定→v522 で本文確定の2段）。
  2. **hash（uint key）＋compare（`==`）の両面が揃うと「key が何か」が確定**（**form id が registry key と確定**＝v519 allocator の key 追跡が完遂）。
  3. **RTDynamicCast wrapper（`FUN_009832E6`）が5度目**（**cast 先 type ptr（`0xB03534`）の RTTI が次の手**＝v501/v502/v513/v522 に続く）。
  4. **allocator の caller が「cast 経路」と「DWORD locals 経路」に分かれる**（**id counter を使う2つの経路 [部分確定]**）。
  5. **`%` と `==` の1行 fn が container の key semantics を決める**（**小さな fn ほど identity が即分かる＝docs head の短さが利点**）。
