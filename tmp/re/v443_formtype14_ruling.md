# v443 — FormType 14/14 確定・compiler chain・write-block 特定の裁定（V-443-1..3 完遂・派生残点第35弾）

- 採番: **v443**（派生残点群・probe prefix **v443_1..v443_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: v442 の残点 **V-443-1【低】（`0x19`/`0x28` FormType と許可リストの所属 opcode case）／V-443-2【低】（`FUN_0056b220/2e0` の callers と `FUN_0056ad60` 呼出の case）／V-443-3【低】（`[esp+0x5c]` 非ゼロ経路 CFG と docs census non-ret 3159 の機械分離）** を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-443-1: `0x19`/`0x28` FormType** | **[確定]** | **fn-head 直接法（hit→owning fn→fn 内 vtable store の RTTI）で解決**: **`0x19 hit @0x419993（head `FUN_004198d0`）→ `.?AVIngredientItem@@` [確定]**（＝INGR）／**`0x28 hit @0x412C83（head `FUN_00412bc0`）→ `.?AVAlchemyItem@@` [確定]**（＝ALCH）。**→ `FUN_00469520` の switch 14 値すべての FormType が 14/14 確定（v441 の49件表に2件追加で51件）— 全件 inventory 品目と完全整合 [確定]**（他の hit `0x429E26/0x42A304/0x6B3805` は vtable class None＝ExtraData ctor 群等で無関係） |
| **V-443-1: 許可リスト `{Faction, NPC}` の所属 opcode case** | **[部分確定]** | **許可リスト/`0x43` gate は FUN_004fab20 の switch-case target [確定]**: **jump table `0x4FB3E0..0x4FB3FC` が `0x4FB146/0x4FB14C/0x4FB152/0x4FB183/0x4FB1B8/0x4FB206/0x4FB25F/0x4FB30A` を指す**（`0x4FB183`＝`cmp byte[eax+4],0x43` gate、`0x4FB152`＝許可リスト直前）。＋ **`0x4FAEDE: jcc → 0x4FB1A1`（flag B==0 時に type==FormType 判定へ合流）[確定]**。**table base の imm32 ref は0件（SIB/基底別形式）→ case index の特定は [未確定]＝残点** |
| **V-443-2: `FUN_0056b220/2e0` の callers** | **[確定]（chain）＋[部分確定]（端の identity）** | **`FUN_0056b220` ← E8 2件とも `FUN_0056b2e0`（`0x56B52F/0x56B5C3`）[確定]**／**`FUN_0056b2e0` ← E8 1件 `FUN_0056ae00`（`0x56AE72`）[確定]** → **chain `FUN_0056ae00 → FUN_0056b2e0（flag B 判定 consumer）→ FUN_0056b220` [確定]**。**`FUN_0056ae00` の decompiled identity は [未確定]（残点）** |
| **V-443-2: `FUN_0056ad60` 呼出の case** | **[未確定]（残点継続）** | v443 群では未着手（`FUN_0056a3b0` 内 `0x56A464` の所属 switch-case 未特定） |
| **V-443-3: `[esp+0x5c]` 非ゼロ経路** | **[部分確定]** | **write の block entry を特定**: `0x407CBA` の nearest branch target＝**`0x407CB4`**／`0x407E5B` の nearest branch target＝**`0x407E47`**（`0x407E4D: call` 直後）→ **非ゼロ経路はこの2ブロック（うち1つ）[確定]、到達条件（branch 側の predicate）は shallow decode に留まり [未確定]＝残点** |
| **V-443-3: docs census non-ret 3159 の機械分離** | **[未確定]（残点継続）** | 本 v443 群では未着手。v442 の histogram 結果（`0xFF=1859/0x00=1214`・jcc32=0）を維持 |

## §2 証拠（v443_1/v443_2）

### §2.1 FormType 14/14（v443_1a）
```
method: imm 0x19/0x28 の `mov byte [reg+4], imm` hits → owning fn（find_head）
        → fn window 内の `mov [reg], imm32`（vtable 範囲）→ RTTI
  0x19 hit @0x419993 head FUN_004198d0 -> .?AVIngredientItem@@   [確定]
  0x28 hit @0x412C83 head FUN_00412bc0 -> .?AVAlchemyItem@@      [確定]
  (他 hit 0x429E26/0x42A304/0x6B3805 -> class None = ExtraData ctor 系)
switch set: {0x13 APPA,0x14 ARMO,0x15 BOOK,0x16 CLOT,0x19 INGR,0x1A LIGH,
             0x1B MISC,0x21 WEAP,0x22 AMMO,0x26 SLGM,0x27 KEYM,0x28 ALCH,
             0x2A SGST,0x2B LVLI} = 14/14 全件 inventory 品目 [確定]
```

### §2.2 許可リスト case（v443_1b）
```
branches into [0x4FB150,0x4FB1A5] (all head FUN_004fab20):
  0x4FAEDE jcc -> 0x4FB1A1   (v441: flag B==0 時 je)
  E9/EB -> 0x4FB187 / 0x4FB18D / 0x4FB194 (合流点多数)
jump-table dwords @0x4FB3E8 -> 0x4FB152 / @0x4FB3EC -> 0x4FB183
table dump 0x4FB3D0..0x4FB3FC:
  0x4FB140, 0x4FB082, 0x4FB0B8, 0x4FB357, 0x4FB146, 0x4FB14C,
  0x4FB152 (許可リスト直前), 0x4FB183 (EffectShader gate), 0x4FB1B8,
  0x4FB206, 0x4FB25F, 0x4FB30A
imm32 refs of 0x4FB3E8/0x4FB3E4: 0 (base 形式が別)
```

### §2.3 chain（v443_2a）
```
FUN_0056ae00 --call@0x56AE72--> FUN_0056b2e0 --call@0x56B52F, 0x56B5C3--> FUN_0056b220
FUN_0056b2e0 contains flag-B consumer (v440: cmp byte[ecx*8+0xb0a54d] @0x56B3A5)
```

### §2.4 write blocks（v443_2c）
```
write 0x407CBA: nearest preceding branch target = 0x407CB4
write 0x407E5B: nearest preceding branch target = 0x407E47
                (call @0x407E4D in block)
```

## §3 残点 **V-444-1..3**
1. 【低】**許可リスト case の jump table index 特定**（table base の ref を SIB/`jmp [reg*4+disp]` 直接 byte-pattern で探索）＋ **`FUN_0056ae00` の identity**（chain 端＝script compiler のどの pass か）。
2. 【低】**`FUN_0056ad60` 呼出の case**（`FUN_0056a3b0` の switch-case 特定）＋ **`[esp+0x5c]` 非ゼロ経路の到達条件**（`0x407CB4`/`0x407E47` 以降の predicate 完全 decode）。
3. 【低】**docs census 機械分離の完成**（switch table dwords が並ぶ region の CC 境界を「表境界」として除外する手法の実装と、non-ret 3159 からの減算）。

## §4 検証・自己批評
- probe: **v443_1..v443_2 の2本全 EXIT=0**（V-443-1 完遂（FormType 14/14）＋ case index は残点／V-443-2 chain 確定・端 identity は残点／V-443-3 block entry 特定・predicate は残点）。
- **決定打の作り**: (i) **「masked imm → owning fn → fn 全体の vtable store を探す」直接法**で2値を1 probe で解決（window 法の限界を fn-head decode で回避）→ (ii) **switch case は jump table dwords の値が target 範囲と一致する箇所を列挙するだけ**で表の実体を確定 → (iii) **chain は E8 census を caller 側へ1段ずつ**（2本とも1〜2件）→ (iv) write block は **nearest-preceding-branch-target 探索**。
- 妥当性チェック: ①`0x19=IngredientItem`／`0x28=AlchemyItem`⇔switch 外でない（inventory 品目）✓ ②`0x4FB3E8→0x4FB152`⇔v441_3 で decode した許可リスト直前アドレス ✓ ③`FUN_0056b220` callers 2件が同 fn⇔`FUN_0056b2e0` の body 内整合（`0x56B220 < 0x56B52F < 0x56B5C3`… wait `0x56B220 < 0x56B52F` は `FUN_0056b2e0` の body 内に `FUN_0056b220` の call 2件が収まるか＝`0x56B2E0..0x56B5C3` は約0x2E3 で十分大 ✓）— **三方向矛盾なし**。
- docs との不整合: **なし**。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **masked imm は「fn-head 直接法」が最強** — hit→find_head→fn window 内の vtable store を探すだけ（window 依存の denoise 改善より1手で確定する）。
  2. **switch case の index 特定は「jump table dwords の値が target 範囲にあるか」列挙で表を先に掴み、残りは base ref の byte-pattern 探索へ**（imm32 直 ref が0件でも表は既に見える）。
  3. **E8 caller が1〜2件の fn は chain の中段** — 上下2方向に census すれば端が見える。
  4. **write の path 特定は「nearest preceding branch target」を先に決め、そこからの predicate decode に絞る**（全体 CFG を描かずに済む）。
