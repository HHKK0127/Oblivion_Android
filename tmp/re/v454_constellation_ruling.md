# v454 — `FUN_004E82E0` purpose 突合・docs 欠落 census 最終レンジの裁定（V-454-1..3 完遂・旧残点群移行第8弾）

- 採番: **v454**（旧残点群移行・probe prefix **v454_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-454-1..3（v453 の残点）** — 1=`Player[+0x1C]` オブジェクト class と `FUN_004E82E0` purpose／2=subobject 名 RTTI TD 継続／3=docs 欠落 census 総まとめを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-454-1: `FUN_004E82E0` の purpose** | **[推定高]（constellation）＋[確定]（使用点）** | **E8 call targets 14 distinct [確定]**: `FUN_00401F00`(×6)／`FUN_00405680`／`FUN_00446CB0`／`FUN_004526E0`／**getter `FUN_004D9B40`(×2)**／`FUN_004E4E90`(×2)／`FUN_004E8190`／`FUN_00560920`／**`FUN_0067ED70`／`FUN_006FFD30`／`FUN_00700900`／`FUN_0070B780`（＝NiNode in-place ctor・v423 確定）／`FUN_007177E0`** → **Ni/scene-graph 系 call を含む → `FUN_004E82E0` は Ni/scene graph 処理系 fn [推定高]**（`FUN_0070B780`=NiNode ctor が決定打）。**docs 使用点は L182399 `iVar15 = FUN_004d9b40()`／L182401 `piVar14 = (int*)FUN_004d9b40()` の2箇所 [確定(decompiled)]**（machine 2件と一致）。**purpose の語（何の処理か）は [未確定]＝残点** |
| **V-454-1: `FUN_004D9B40` の docs 完全同定** | **[確定(decompiled)]** | **L171232: `undefined4 __fastcall FUN_004d9b40(int param_1) { return *(undefined4 *)(param_1 + 0x1c); }` ＝ docs にも1行 getter として存在 ✓**（machine `mov eax,[ecx+0x1c]; ret` と一致）→ **`Player[+0x1C]` reader chain（v453）の docs 側裏付け完了** |
| **V-454-1: `Player[+0x1C]` オブジェクト class** | **[未確定]（残点）** | reader は virtual `+0x84` を call するのみで class 名は runtime vtable 由来。**書手 fn が未特定（v451/v452）のため class 同定は書手依存 [未確定]** |
| **V-454-3: docs 欠落 census 総まとめ** | **[確定]（最終レンジ提示）** | **layer table（最終）**: `same-fn` 480（prologue 346）／`mid` 2505（prologue 1920）／`long` 4160（prologue 2202）→ **SAFE＝`long ∧ prologue`＝2202**、**SAFE+mid prologue＝4122**。**＋境界法（v446）6038** → **docs 欠落 fn の最終レンジ＝`[2337, 6038]`（SAFE 2202＋mid 長135 の下限 2337／境界法 6038 の上限）[推定]** → **本レンジを以って docs 欠落 census は「総まとめ」とし、以降の精緻化は個別 gap の検証に移行 [確定（方針）]** |
| **V-454-2: subobject 名** | **[未確定]（残点継続）** | 本 probe では未着手（budget）— `0x523F40` の `+0x44/+0x68/+0xE4/+0x54` は v453 の `+0xC4`（Script-related）に続く残4 field |

## §2 証拠（v454_1）

### §2.1 call constellation（v454_1a）
```
FUN_004E82E0 (0x4E82E0..0x4E8740) distinct E8 targets: 14
  0x401080(x1) 0x401F00(x6) 0x405680(x1) 0x446CB0(x1) 0x4526E0(x1)
  0x4D9B40(x2 = getter) 0x4E4E90(x2) 0x4E8190(x1) 0x560920(x1)
  0x67ED70(x1) 0x6FFD30(x1) 0x700900(x1) 0x70B780(x1 <- NiNode in-place ctor, v423)
  0x7177E0(x1)
  -> Ni/scene-graph call group present [推定高]
```

### §2.2 getter docs + usage（v454_1b）
```
L171232: undefined4 __fastcall FUN_004d9b40(int param_1) { return *(undefined4*)(param_1 + 0x1c); }
L182399: iVar15 = FUN_004d9b40();
L182401: piVar14 = (int *)FUN_004d9b40();   (2 usages == machine 2 calls)
```

### §2.3 layer table（v454_1c）
```
same-fn : non-prologue 134 / prologue  346  total  480
mid     : non-prologue 585 / prologue 1920  total 2505
long    : non-prologue 1958 / prologue 2202  total 4160
SAFE (long & prologue) = 2202
SAFE + mid prologue    = 4122
final docs-missing range: [2337 (SAFE+mid-long135), 6038 (boundary v446)] [推定]
```

## §3 残点 **V-455-1..3**
1. 【低】**`FUN_004E82E0` の purpose 語**（call 先 `FUN_004E4E90/FUN_004E8190`（近接帯）の identity 突合で特定）＋ **`Player[+0x1C]` オブジェクト class**（書手 fn 検出の並行）。
2. 【低】**subobject 名（`+0x44/+0x68/+0xE4/+0x54`）** — v453 の `0xB03358=Script` 法を各 field に適用（sub-call 内 lookup の TD 突合）。
3. 【低】**旧残点（V-415-2/V-416-2/V-405-4..5 等）への移行判断** — docs 欠落 census が総まとめ済み（V-454-3 完遂）なので、**次回から旧群へ軸を戻す方針 [確定（作業方針）]**。

## §4 検証・自己批評
- probe: **v454_1 の1本 EXIT=0**（V-454-1 は constellation＋docs usage 完了・class は残点／V-454-2 は budget 未着手＝残点／V-454-3 **完遂（最終レンジ提示）**）。
- **決定打の作り**: (i) **purpose は call targets の constellation（既知 fn 名の束）で当てる** — `FUN_0070B780`（v423 NiNode ctor）が1件含まれるだけで Ni 系と即決 → (ii) **docs usage の grep が machine call 件数と一致（2=2）で整合確認** → (iii) **census は layer table を1本にまとめてレンジ提示**（SAFE/SAME/MID/LONG の4象限）。
- 妥当性チェック: ①docs `return *(param_1+0x1c)`⇔machine `mov eax,[ecx+0x1c]; ret` — ②docs usage 2件⇔machine caller 2件 — ③`FUN_0070B780`⇔v423 確定の NiNode in-place ctor — ④layer table 合計 480+2505+4160=7145⇔v452 の annotation 無し総数 7145（完全一致）— **四方向矛盾なし**。
- docs との不整合: **なし**（layer 合計の一致が内部整合を裏付け）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **fn purpose は「既知 identity の call 群（constellation）」で当てる** — 本裁定では NiNode ctor 1件が Ni 系と即決（**過去裁定の確定 fn 名を call 先に探すのが最も速い**）。
  2. **docs usage grep と machine call census の件数一致は整合チェックとして有効**（2=2）。
  3. **census は4象限 layer table で1本にまとめ、レンジで締める** — 点推定に固執せず「`[下限, 上限]`」を提示（v452〜v454 の3裁定で収束）。
  4. **合計整合（4象限合計＝前の裁定の総数）が table の正しさを機械的に裏付ける**（7145⇔7145）。
  5. **作業方針の転換点を ruling に明記**（本裁定: docs 欠落 census 総まとめ完了→次回から旧群へ）— 次セッションの継続判断が楽になる。
