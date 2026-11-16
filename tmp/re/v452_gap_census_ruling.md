# v452 — docs 欠落 fn 機械化 census と subobject 名の裁定（V-452-1..3 完遂・旧残点群移行第6弾）

- 採番: **v452**（旧残点群移行・probe prefix **v452_1..v452_2 の2本全 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-452-1..3（v451 の残点）** — 1=`Player[+0x1C]` 書手別ルートと `+0x32 s16` 語／2=docs 欠落 fn census 機械化／3=`0x523F40` subobject 正式名を消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-452-2: docs 欠落 fn census の機械化** | **[確定]（手法・実測）＋[推定]（総数）** | **手法（2条件＋距離）**: `.text` を走査し「**int3 run≥8 の直後 code byte**」を code start とし「**docs annotation 無し**」でフィルタ → **`owning annotation`（直前 annotation）との距離**で3分類。**実測**: code start 総数 **16622**（＝v438 runs≥8 と一致✓内部整合）／**annotation 無し 7145（43.0%）**／**prologue-like 4197**。**距離分類**: `d≤0x40`（同 fn padding 続き）480／`0x40<d≤0x200` 2505／**`d>0x200` 4160（prologue 2202）** → **TRUE docs-missing candidates（`d>0x200` ∧ prologue）＝2202 [推定]、上位階（`0x40..0x200` ∧ prologue）込み＝4122 [推定]**。**samples に `0x40DF80 owning=0x40D4D0 dist=0xAB0`（v436/v437 で確定した実 fn entry）が現れ、手法妥当性を実証 [確定]** |
| **docs 欠落総数の2系統クロス** | **[推定]（2千〜6千）** | **境界法（v446）: gaps>0x200 の function end 境界 ≈6038** ／ **entry法（本裁定）: `d>0x200`&prologue＝2202（上位階込み4122）** → **独立2手法が同桁（2千〜6千）＝docs 欠落 fn は数千規模 [推定]**（entry 法は `d≤0x200` の欠落を取りこぼす下限側、境界法は intra-fn 境界の偽陽性を含む上位側 → **真値は 2202〜6038 の間に分布 [推定]**） |
| **V-452-1: `Player[+0x1C]` 読手（Player ctor band）** | **[確定]（否定側）** | **`FUN_0066A740..0x66AD40` の `mov reg,[reg+0x1c]` 読手＝0件 [確定(否定側)]**（disp8 読手も band 内に無し）→ Player ctor 自体は `+0x1C` を使わない。**書手は別 fn（v451 の class 絞り込み候補 `FUN_00437F00` QueuedCharacter 系や window 外大 ctor）[未確定]＝残点継続** |
| **V-452-3: `0x523F40` subobject 正式名** | **[確定]（否定側）＋[未確定]（残点）** | **5fn（`FUN_0046F020/469E90/468590/46E4F0/46FC10`）とも first 0x60 に vtable store 無し [確定(否定側)]** → **vtable RTTI 法では embedded object 名を取得不可**（reset fn は vtable を書かない）。subobject 名は別法（field 隣接の RTTI/構造読解）が要 [未確定]＝残点 |

## §2 証拠（v452_1/v452_2）

### §2.1 機械化 census（v452_1a / v452_2a）
```
code starts after CC>=8: 16622   (= v438 runs>=8 count, internal consistency)
  no docs annotation: 7145 (43.0%)
  unannotated AND prologue-like byte: 4197
distance to owning annotation:
  d<=0x40    : non-prologue 134 / prologue  346  (total  480)  <- same-fn padding
  0x40..0x200: non-prologue 585 / prologue 1920  (total 2505)
  d>0x200    : non-prologue 1958 / prologue 2202  (total 4160)
  TRUE docs-missing candidates (d>0x200 & prologue): 2202
samples (d>0x200 & prologue):
  0x401480 (8B) owning 0x401260 d=0x220 / 0x4052F0 (81) 0x405090 d=0x260
  0x40C180 (8B) 0x407AE0 d=0x46A0 / 0x40DF80 (81) 0x40D4D0 d=0xAB0  <- v437 known gap!
  0x417090 (56) 0x416DE0 d=0x2B0 / 0x4198C0 (83) 0x419640 d=0x280
  0x4291E0/429290/4292F0/4293A0 (53) 0x428E90 d=0x350..0x510 (cluster)
```

### §2.2 Player band reads（v452_1b）
```
mov reg,[reg+0x1c] in 0x66A740..0x66AD40: 0 hits  [確定(否定側)]
```

### §2.3 sub-call vtable scan（v452_2b）
```
FUN_0046F020 / 0x469E90 / 0x468590 / 0x46E4F0 / 0x46FC10:
  vtable store in first 0x60: NONE (all five)
```

## §3 残点 **V-453-1..3**
1. 【低】**docs 欠落 census の第3系統（entry 法の `d≤0x200` 層の真偽判定）** — `0x40..0x200` 層（prologue 1920件）のサンプル個別検証で、欠落総数レンジ（2202〜6038）を狭める。
2. 【低】**`Player[+0x1C]` 書手の残ルート** — `FUN_00437F00`（QueuedCharacter）候補の実体確認と、**Reader 側から逆引き**（`+0x1C` を読む `FUN_004D9B40` の caller 群から stream を追う）。
3. 【低】**subobject 名の別法** — `0x523F40` の `lea ecx,[esi+off]` 後の sub-call fn body に store される定数/文字列、または docs の該当構造コメントを追う。

## §4 検証・自己批評
- probe: **v452_1..v452_2 の2本全 EXIT=0**（V-452-2 機械化 census 完遂・総数は [推定] レンジ／V-452-1 否定側確定・書手残点／V-452-3 否定側確定・別法残点）。
- **決定打の作り**: (i) **「int3 run≥8 直後 code byte」と「annotation 無し」の2条件で機械走査**（`owning ann` 距離を histogram 化）→ (ii) **`d>0x200` ∧ prologue で SAFE 候補を2202と確定** → (iii) **v437 既知 gap（`0x40DF80`）が samples に出現＝手法妥当性を実証** → (iv) **Player band 読手・sub-call vtable の2つの否定側**で残点の形を明確化。
- 妥当性チェック: ①code start 16622⇔v438 runs≥8 16622（完全一致）— ②`0x40DF80`∈samples⇔v436 で「next fn 0x40DF80」と実証済み— ③`0x40C180 owning=0x407AE0`⇔v435 の dump fn gap（`0x40C220` まで annotation 無し）— ④entry 法 2202〜4122⇔境界法 6038 の桁整合（両者独立手法）— ⑤Player band 読手0⇔v451 書手也0（ctor が +0x1C 不使用で一貫）— **五方向矛盾なし**。
- docs との不整合: **なし**（v437 の既知 gap と本 census が整合）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **docs 欠落 census は「code start（int3 run≥8 直後）×annotation 有無×owning 距離」の3次元で機械化できる** — 既知 gap（v437 `0x40DF80`）が samples に出れば手法自体が実証される（**サンプルに既知事例が混ざるかを必ず確認**）。
  2. **距離閾値（`d>0x200`）は SAFE 候補の下限を作る** — `d≤0x40`＝同 fn padding と切り分けられる（480件が同 fn 層として除外される）。
  3. **独立2手法（entry 法／境界法）の桁整合が総数レンジの信頼度を上げる** — 単手法の点推定より「2202〜6038」のようなレンジ提示が誠実。
  4. **reset/impl fn は vtable を書かない**（embedded object のため）→ **vtable RTTI 法は reset fn に効かない** — subobject 名は field 構造・文字列・docs の別手を探す。
  5. **否定側 census（0 hits）も「その fn は `+0x1C` 不使用」の形で残点の輪郭を絞る** — Player ctor 不使用は v451 書手未特定と整合し、調査経路を消す。
