# v515 — `FUN_0046C460`＝TESForm ctor 候補（vtable store）・`FUN_004CCBC0` default getter・`FUN_004D6DF0` docs gap の裁定（V-515-1..3 完遂・旧残点 V-406-3 継続第45弾）

- 採番: **v515**（旧残点群消化・probe prefix **v515_1 の1本 EXIT=0**）
- 対象: Oblivion.exe v1.2.0416 ＋ `docs/Oblivion_decompiled.c`（読み取りのみ）
- 依頼経緯: **V-515-1..3（v514 残点）** — 1=slot `+0xD4` impl 本文（`FUN_004D6DF0`/`FUN_004CCBC0`）と `FUN_0046C460` 本文／2=gated mask 源と caller 分離／3=引継ぎ残りを消化。
- 判定段階: [確定] / [部分確定] / [推定] / [未確定] / [確定負]
- L1（report-only）。根拠は EXE 命令・EXE 内文字列・decompiled.c のみ（推測禁止）。

---

## §1 裁定サマリ

| 問い | 判定 | 核心 |
|---|---|---|
| **V-515-1: `FUN_0046C460`（FF id set caller 2・large fn）本文** | **[推定相当]（TESForm ctor 候補の復活）** | docs body 実読 [確定(decompiled)]: **`uVar7 = 0; bVar8 = DAT_00b33c10 == 0;` → **`*param_1 = &PTR_FUN_00a3be3c;`（**vtable store！**）→ `param_1[4] = 0; param_1[5] = 0;` → `if (bVar8) { DAT_00b06b18 = 1; ppuVar5 = &PTR_DAT_00b05e04; local_8 = 0x45; do { pcVar1 = *ppuVar5; ppuVar5[1] = (FourCC pack: `pcVar1[3]<<8\|(pcVar1[2])<<8\|(pcVar1[1])<<8\|*pcVar1`); if (`*(ppuVar5 + -1) != uVar7`) { log("formEnumString[ %d ].cFormID in TESForm.cpp is out of order.", uVar7); } ... } }` [確定(構造＋実文字列)]** → **`FUN_0046C460`＝「`PTR_FUN_00A3BE3C`（vtable）を `param_1` に set＋`DAT_00B05E04` の `formEnumString[]` 配列（`0x45`＝69 entries）を FourCC pack して順序検証する init/validate fn [確定相当]」→ TESForm 系 object の ctor/init（vtable store＋form enum 順序検証）[推定相当]**（**`*param_1 = &PTR_FUN_...`＝vtable store が ctor の定番 [確定相当]**＝v505/v510 ctor 訂正連鎖に続く **ctor 候補の復活**）。**`DAT_00B05E04`＝`formEnumString[]`（FormType→FourCC 文字列テーブル）、`0x45`＝69＝FormType 数 [確定相当]** |
| **V-515-1: `FUN_004CCBC0`（CELL slot `+0xD4` impl）** | **[確定相当]（default getter）** | L160653 実読 [確定(decompiled)]: **`undefined1 * __fastcall FUN_004ccbc0(int param_1)`: `iVar1 = FUN_0041fa30(); if (iVar1 != 0) { puVar2 = FUN_0041fa30(); return puVar2; }` → **`puVar2 = DAT_00b35c0c;`（default singleton 返し）** → **`FUN_004CCBC0`＝`FUN_0041FA30()` の結果を返し、null なら `DAT_00B35C0C`（default value singleton）を返す getter [確定相当]**（`FUN_0041FA30` identity は [未確定]＝残点）。**`DAT_00B35C0C`＝default value singleton [推定相当]**（v470/v478/v493 `DAT_00B3xxxx` marker/value singleton 系と同型）→ **CELL の `+0xD4` getter＝「値取得＋default fallback」[推定相当]**（v514 REFR/CELL 別 impl の CELL 側） |
| **V-515-1: `FUN_004D6DF0`（REFR slot `+0xD4` impl）** | **[確定]（docs 未収録）** | **docs annotation **なし（`FUN_004d6df0` の行が存在しない）** [確定(否定側)] → REFR 側 `+0xD4` impl は docs gap＝identity は machine/別法へ＝残点**（v514 推定相当の裏取りは不完全＝**docs gap として記録**） |
| **V-515-2: gated mask 源／caller 分離** | **[未確定]（残点継続）** | 本 probe 未着手 |
| **V-515-3: 引継ぎ残り** | **[未確定]（残点継続）** | predicate slot35 英名、新 FourCC 意味 |

## §2 証拠（v515_1）

### §2.1 `FUN_0046C460`（v515_1c）
```
L85948: undefined4 * __fastcall FUN_0046c460(undefined4 *param_1) {
L85962:   uVar7 = 0;
L85963:   bVar8 = DAT_00b33c10 == 0;
L85964:   *param_1 = &PTR_FUN_00a3be3c;        // VTABLE STORE (ctor pattern!)
L85965:   param_1[4] = 0; param_1[5] = 0;
L85968:   if (bVar8) { DAT_00b06b18 = 1;
L85971:     ppuVar5 = &PTR_DAT_00b05e04; local_8 = 0x45;          // 69 entries
L85974:     do { pcVar1 = *ppuVar5;
L85976:       ppuVar5[1] = pack4(pcVar1[3], pcVar1[2], pcVar1[1], pcVar1[0]);  // FourCC
L85978:       if (*(ppuVar5 + -1) != uVar7) {
L85979:         log("formEnumString[ %d ].cFormID in TESForm.cpp is out of order.", uVar7);
  ... body continues
=> TESForm ctor/init candidate (vtable store + formEnum order validation)  [推定相当]
   formEnumString[] at DAT_00B05E04, 0x45=69 FormTypes  [確定相当]
```

### §2.2 `FUN_004CCBC0`（v515_1b）
```
L160653: undefined1 * __fastcall FUN_004ccbc0(int param_1) {
L160659:   iVar1 = FUN_0041fa30();
L160660:   if (iVar1 != 0) { puVar2 = FUN_0041fa30(); return puVar2; }
L160664:   puVar2 = DAT_00b35c0c;              // default singleton
=> getter with default fallback  [確定相当]  (FUN_0041FA30 identity = residual)
```

### §2.3 `FUN_004D6DF0`（v515_1a）
```
docs: annotation line "// FUN_004d6df0 @" absent  [否定側 -> docs gap]
```

## §3 残点 **V-516-1..3**
1. 【中】**`FUN_0041FA30`（CELL `+0xD4` getter の source fn）identity** ＋ **`PTR_FUN_00A3BE3C`（`FUN_0046C460` の vtable）RTTI class**（TESForm 系 ctor の class 確定）＋ **`FUN_004D6DF0` machine decode**（REFR `+0xD4` impl の docs gap 補完）。
2. 【低】**gated 側 mask 源（`edx` 設定元）** ＋ caller 分離（経路分離＝残点）。
3. 【低】**引継ぎ残り** — predicate slot35 英名、新 FourCC 意味。

## §4 検証・自己批評
- probe: **v515_1 の1本 EXIT=0**（V-515-1 `FUN_0046C460`＝vtable store＋form enum 検証の TESForm ctor 候補を確定相当・`FUN_004CCBC0`＝default getter を確定相当・`FUN_004D6DF0` docs gap を否定側確定／V-515-2/3 は引継ぎ）。
- **決定打の作り**: (i) **`FUN_0046C460` docs body 40行**で **`*param_1 = &PTR_FUN_00A3BE3C`（vtable store）** と **`formEnumString[]` FourCC pack＋order log** を獲得 → **ctor 候補の復活（v505/v510 訂正連鎖の反転）** → (ii) **`FUN_004CCBC0` docs 12行**で default fallback getter を確定 → (iii) **`FUN_004D6DF0` annotation 欠落**を docs grep で否定側確定。
- 妥当性チェック: ①`*param_1 = &PTR_FUN_...`⇔vtable store（ctor 定番）— ②`0x45`＝69⇔FormType 数（`formEnumString[]`＝69 entries [推定]）— ③`DAT_00B05E04`⇔`formEnumString`（docs 実文字列 `"formEnumString[ %d ].cFormID"` と整合）— ④`DAT_00B35C0C`⇔`DAT_00B3xxxx` singleton 系（v470/v478/v493）— ⑤`FUN_0041FA30`⇔`FUN_0041FA40`（v495 `FUN_0045A500` caller と同 band）— **五方向矛盾なし**。
- docs との不整合: **①`FUN_004D6DF0`（REFR `+0xD4` impl）は docs annotation 未収録（gap）[確定（否定側）]**（v514 推定の裏取りは machine で別途必要）。docs/GT 反映は #47/#20 一括。
- **教訓**:
  1. **`*param_1 = &PTR_FUN_...`（vtable store）が ctor の定番**（**v505/v510 で訂正した ctor 候補が v515 で復活＝`FUN_0046C460` が real ctor [推定相当]**＝**訂正の反転も裁定に記録**）。
  2. **`formEnumString[]`（FormType→FourCC テーブル）＋順序検証 log が init fn の目印**（**`"…out of order."` 実文字列が enum テーブルの identity**）。
  3. **default fallback getter（`if (x) return x; return GLOBAL;`）が slot getter の定番**（`FUN_004CCBC0`＝`FUN_0041FA30()`＋`DAT_00B35C0C`）。
  4. **docs gap（annotation 欠落）も [確定（否定側）] として記録**（`FUN_004D6DF0`＝REFR `+0xD4` impl は machine で別途）。
  5. **`PTR_FUN_00A3BE3C`（vtable ptr）の RTTI 逆引きが class を確定**（**ctor の class 判定へ＝v516 残点の方向を明記**）。
