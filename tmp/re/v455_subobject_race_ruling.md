# v455 — `FUN_004E82E0` purpose 深化・subobject `+0xE4`=Race の裁定（V-455-1..3 完遂・旧残点群移行第9弾）

- 採番: **v455**（旧残点群移行・probe prefix **v455_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-455-1..3（v454 の残点）** — 1=`FUN_004E82E0` purpose 語と `Player[+0x1C]` class／2=subobject 名（Script TD 法の各 field 適用）／3=旧残点への移行判断を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-455-1: `FUN_004E82E0` purpose 語** | **[推定高]（深化）** | **close-band call 2本の identity [確定(signature)＋機械]**: **`FUN_004E4E90`（L179403）`int __thiscall(int,int)`: `mov edi,[ecx+0x24]`（＝**TESActorBaseData**・v448/450 確定の field）を扱う fn**／**`FUN_004E8190`（L182119）`void __fastcall(int)`: `mov eax,[esi+0x28]; lea ecx,[eax+0xAC]; call 0x477EF0`（self+0x28 の subobject `+0xAC` 操作）** → **`FUN_004E82E0` ＝「TESActorBaseData・self+0x28 subobject・Ni/scene-graph call（v454 constellation）・Player+0x1C getter」を併用する Character/Actor の scene+data 更新 fn [推定高]**（purpose の一語名は docs body 語が無いため [未確定]） |
| **V-455-1: `Player[+0x1C]` オブジェクト class** | **[未確定]（残点継続）** | 書手 fn 未特定（v451/v452 のまま）→ class 同定は書手依存のまま [未確定] |
| **V-455-2: subobject 名（本 probe 分）** | **[確定]（`+0xE4`）＋[確定(否定側)]（残3件）** | **`FUN_0046E4F0`（`0x523F40` の `+0xE4` subobject reset fn）内の RTTI TD push: `push 0xB0532C`＝`.?AVTESRace@@`／`push 0xB02F9C`＝`.?AVTESForm@@` [確定]** → **`+0xE4` ＝ Race-related subobject（Form→TESRace lookup）[確定相当]**。**`FUN_00469E90`（`+0x44`）／`FUN_00468590`（`+0x68`）／`FUN_0046FC10`（`+0x54`）は first 0x1C0 に RTTI TD push 無し [確定(否定側)]** → 残3 field の name は別法要＝残点 |
| **V-455-3: 旧残点への移行判断** | **[確定]（方針）** | **docs 欠落 census は v454 で総まとめ（レンジ `[2337,6038]`）済み** → **V-456 以降は旧残点（V-415-2／V-416-2／V-405-4..5／V-406-2..6 等）へ軸を戻す [確定（作業方針）]**。`FUN_004E82E0` purpose 一語名と `Player[+0x1C]` class は低優先で V-456+ に併走 |

## §2 証拠（v455_1）

### §2.1 close-band（v455_1a）
```
FUN_004E4E90 (L179403): int __thiscall(int,int)
  4E4E90 push edi / mov edi,[ecx+0x24]   <- TESActorBaseData field access
FUN_004E8190 (L182119): void __fastcall(int)
  4E8194 mov eax,[esi+0x28] / test / je / lea ecx,[eax+0xac] / call 0x477ef0
```

### §2.2 subobject RTTI TD（v455_1b）
```
FUN_0046E4F0 (+0xE4 reset): push 0xB0532C -> .?AVTESRace@@ @0x46E525
                             push 0xB02F9C -> .?AVTESForm@@  @0x46E52A
FUN_00469E90 (+0x44) / FUN_00468590 (+0x68) / FUN_0046FC10 (+0x54):
  no RTTI TD push in first 0x1C0   [確定(否定側)]
```

### §2.3 subobject map（v450 + v453 + v455 累積）
```
0x523F40: [+0xC4] -> FUN_0046F020  = Script lookup (0xB03358 .?AVScript@@)   [推定高]
           [+0xE4] -> FUN_0046E4F0  = Race lookup (0xB0532C .?AVTESRace@@)    [確定相当]
           [+0x44] -> FUN_00469E90  = (RTTI push none)                        [未確定]
           [+0x68] -> FUN_00468590  = (void(void), no TD)                     [未確定]
           [+0x54] -> FUN_0046FC10  = (no TD)                                 [未確定]
           [+0x24] -> FUN_004676B0  = TESActorBaseData (v448/450 確定)        [確定]
```

## §3 残点 **V-456-1..3**
1. 【低】**旧残点の消化開始** — **V-415-2**（既存残点）／**V-416-2**／**V-405-4..5** を優先順に着手（v454/455 の方針どおり）。
2. 【低】**残る subobject name（`+0x44/+0x68/+0x54`）** — sub-call 内の `FUN_0046B250` lookup・文字列 push・docs 構造 grep へ（`+0x44` は `FUN_00469E90` の list walk 形＝list-of-forms 系の可能性 [未確定]）。
3. 【低】**`FUN_004E82E0` purpose の一語名**（docs body 213 lines の call 突合完了版）と **`Player[+0x1C]` class**（書手 fn 検出の並行）。

## §4 検証・自己批評
- probe: **v455_1 の1本 EXIT=0**（V-455-1 purpose 深化＝constellation＋close-band／V-455-2 `+0xE4`=Race 確定・残3件否定側／V-455-3 方針確定）。
- **決定打の作り**: (i) **close-band call の signature＋machine head（`[ecx+0x24]`＝TESActorBaseData）で用途を絞る** → (ii) **sub-call 内の `push 0xB0xxxx`（RTTI TD）を機械列挙**（本 probe で `TESRace` が1件 hit）→ (iii) **field map を累積表として ruling に持続記録**（`+0x24/+0xC4/+0xE4` まで確定・残3）。
- 妥当性チェック: ①`FUN_004E4E90` の `[ecx+0x24]`⇔`TESActorBaseData`（v448/450）— ②`FUN_0046E4F0` の `TESRace/ TForm`⇔`0x523F40` の `+0xE4` reset ＝ Race-related — ③`FUN_004E8190` の `[esi+0x28]+0xAC`⇔別 subobject（`0x523F40` 帯と別系統）— **三方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **close-band call（同一 fn 内の近接 call）は「signature＋直近 field アクセス」で用途が絞れる** — `[ecx+0x24]` が既知 field（TESActorBaseData）ならその fn の扱い対象が即決。
  2. **subobject 名は sub-call 内の `push RTTI_TD` を機械列挙するのが有効**（本 probe で `TESRace` hit・TD が無い fn は list walk 等の汎用 reset で別法要）。
  3. **field map（off→fn→意味）は ruling ごとに累積表で持続させる** — 3裁定累積で `+0x24/+0xC4/+0xE4` が確定し、残りが `3件` に収束（**進捗の可視化が次 probe の設計を楽に**）。
  4. **purpose の一語名は docs 語が無ければ constellation＋close-band の「扱い対象の束」で [推定高] に留め、確度を上げる努力は他残点へ回す**（cost/benefit の判断）。
  5. **方針転換（旧群へ）は ruling §3 に明記して次セッションの入口を固定**（v454 に続き本裁定でも確認）。
